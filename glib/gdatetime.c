/* gdatetime.c
 *
 * Copyright (C) 2009-2010 Christian Hergert <chris@dronelabs.com>
 * Copyright (C) 2010 Thiago Santos <thiago.sousa.santos@collabora.co.uk>
 * Copyright (C) 2010 Emmanuele Bassi <ebassi@linux.intel.com>
 * Copyright © 2010 Codethink Limited
 * Copyright © 2018 Tomasz Miąsko
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * licence, or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Christian Hergert <chris@dronelabs.com>
 *          Thiago Santos <thiago.sousa.santos@collabora.co.uk>
 *          Emmanuele Bassi <ebassi@linux.intel.com>
 *          Ryan Lortie <desrt@desrt.ca>
 *          Robert Ancell <robert.ancell@canonical.com>
 */

/* Algorithms within this file are based on the Calendar FAQ by
 * Claus Tondering.  It can be found at
 * http://www.tondering.dk/claus/cal/calendar29.txt
 *
 * Copyright and disclaimer
 * ------------------------
 *   This document is Copyright (C) 2008 by Claus Tondering.
 *   E-mail: claus@tondering.dk. (Please include the word
 *   "calendar" in the subject line.)
 *   The document may be freely distributed, provided this
 *   copyright notice is included and no money is charged for
 *   the document.
 *
 *   This document is provided "as is". No warranties are made as
 *   to its correctness.
 */

/* Prologue {{{1 */

#include "config.h"

/* langinfo.h in glibc 2.27 defines ALTMON_* only if _GNU_SOURCE is defined.  */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LANGINFO_TIME
#include <langinfo.h>
#endif

#include "gatomic.h"
#include "gcharset.h"
#include "gcharsetprivate.h"
#include "gconvert.h"
#include "gconvertprivate.h"
#include "gdatetime.h"
#include "gfileutils.h"
#include "ghash.h"
#include "glibintl.h"
#include "gmain.h"
#include "gmappedfile.h"
#include "gslice.h"
#include "gstrfuncs.h"
#include "gtestutils.h"
#include "gthread.h"
#include "gtimezone.h"

#ifndef G_OS_WIN32
#include <sys/time.h>
#include <time.h>
#else
#if defined (_MSC_VER) && (_MSC_VER < 1800)
/* fallback implementation for isnan() on VS2012 and earlier */
#define isnan _isnan
#endif
#endif /* !G_OS_WIN32 */

/**
 * SECTION:date-time
 * @title: xdatetime_t
 * @short_description: a structure representing Date and Time
 * @see_also: #xtimezone_t
 *
 * #xdatetime_t is a structure that combines a Gregorian date and time
 * into a single structure.  It provides many conversion and methods to
 * manipulate dates and times.  Time precision is provided down to
 * microseconds and the time can range (proleptically) from 0001-01-01
 * 00:00:00 to 9999-12-31 23:59:59.999999.  #xdatetime_t follows POSIX
 * time in the sense that it is oblivious to leap seconds.
 *
 * #xdatetime_t is an immutable object; once it has been created it cannot
 * be modified further.  All modifiers will create a new #xdatetime_t.
 * Nearly all such functions can fail due to the date or time going out
 * of range, in which case %NULL will be returned.
 *
 * #xdatetime_t is reference counted: the reference count is increased by calling
 * xdate_time_ref() and decreased by calling xdate_time_unref(). When the
 * reference count drops to 0, the resources allocated by the #xdatetime_t
 * structure are released.
 *
 * Many parts of the API may produce non-obvious results.  As an
 * example, adding two months to January 31st will yield March 31st
 * whereas adding one month and then one month again will yield either
 * March 28th or March 29th.  Also note that adding 24 hours is not
 * always the same as adding one day (since days containing daylight
 * savings time transitions are either 23 or 25 hours in length).
 *
 * #xdatetime_t is available since GLib 2.26.
 */

struct _GDateTime
{
  /* Microsecond timekeeping within Day */
  xuint64_t usec;

  /* TimeZone information */
  xtimezone_t *tz;
  xint_t interval;

  /* 1 is 0001-01-01 in Proleptic Gregorian */
  gint32 days;

  xint_t ref_count;  /* (atomic) */
};

/* Time conversion {{{1 */

#define UNIX_EPOCH_START     719163
#define INSTANT_TO_UNIX(instant) \
  ((instant)/USEC_PER_SECOND - UNIX_EPOCH_START * SEC_PER_DAY)
#define INSTANT_TO_UNIX_USECS(instant) \
  ((instant) - UNIX_EPOCH_START * SEC_PER_DAY * USEC_PER_SECOND)
#define UNIX_TO_INSTANT(unix) \
  (((sint64_t) (unix) + UNIX_EPOCH_START * SEC_PER_DAY) * USEC_PER_SECOND)
#define UNIX_USECS_TO_INSTANT(unix_usecs) \
  ((sint64_t) (unix_usecs) + UNIX_EPOCH_START * SEC_PER_DAY * USEC_PER_SECOND)
#define UNIX_TO_INSTANT_IS_VALID(unix) \
  ((sint64_t) (unix) <= INSTANT_TO_UNIX (G_MAXINT64))
#define UNIX_USECS_TO_INSTANT_IS_VALID(unix_usecs) \
  ((sint64_t) (unix_usecs) <= INSTANT_TO_UNIX_USECS (G_MAXINT64))

#define DAYS_IN_4YEARS    1461    /* days in 4 years */
#define DAYS_IN_100YEARS  36524   /* days in 100 years */
#define DAYS_IN_400YEARS  146097  /* days in 400 years  */

#define USEC_PER_SECOND      (G_GINT64_CONSTANT (1000000))
#define USEC_PER_MINUTE      (G_GINT64_CONSTANT (60000000))
#define USEC_PER_HOUR        (G_GINT64_CONSTANT (3600000000))
#define USEC_PER_MILLISECOND (G_GINT64_CONSTANT (1000))
#define USEC_PER_DAY         (G_GINT64_CONSTANT (86400000000))
#define SEC_PER_DAY          (G_GINT64_CONSTANT (86400))

#define SECS_PER_MINUTE (60)
#define SECS_PER_HOUR   (60 * SECS_PER_MINUTE)
#define SECS_PER_DAY    (24 * SECS_PER_HOUR)
#define SECS_PER_YEAR   (365 * SECS_PER_DAY)
#define SECS_PER_JULIAN (DAYS_PER_PERIOD * SECS_PER_DAY)

#define GREGORIAN_LEAP(y)    ((((y) % 4) == 0) && (!((((y) % 100) == 0) && (((y) % 400) != 0))))
#define JULIAN_YEAR(d)       ((d)->julian / 365.25)
#define DAYS_PER_PERIOD      (G_GINT64_CONSTANT (2914695))

static const xuint16_t days_in_months[2][13] =
{
  { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static const xuint16_t days_in_year[2][13] =
{
  {  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
  {  0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

#ifdef HAVE_LANGINFO_TIME

#define GET_AMPM(d) ((xdate_time_get_hour (d) < 12) ? \
                     nl_langinfo (AM_STR) : \
                     nl_langinfo (PM_STR))
#define GET_AMPM_IS_LOCALE TRUE

#define PREFERRED_DATE_TIME_FMT nl_langinfo (D_T_FMT)
#define PREFERRED_DATE_FMT nl_langinfo (D_FMT)
#define PREFERRED_TIME_FMT nl_langinfo (T_FMT)
#define PREFERRED_12HR_TIME_FMT nl_langinfo (T_FMT_AMPM)

static const xint_t weekday_item[2][7] =
{
  { ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7, ABDAY_1 },
  { DAY_2, DAY_3, DAY_4, DAY_5, DAY_6, DAY_7, DAY_1 }
};

static const xint_t month_item[2][12] =
{
  { ABMON_1, ABMON_2, ABMON_3, ABMON_4, ABMON_5, ABMON_6, ABMON_7, ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12 },
  { MON_1, MON_2, MON_3, MON_4, MON_5, MON_6, MON_7, MON_8, MON_9, MON_10, MON_11, MON_12 },
};

#define WEEKDAY_ABBR(d) nl_langinfo (weekday_item[0][xdate_time_get_day_of_week (d) - 1])
#define WEEKDAY_ABBR_IS_LOCALE TRUE
#define WEEKDAY_FULL(d) nl_langinfo (weekday_item[1][xdate_time_get_day_of_week (d) - 1])
#define WEEKDAY_FULL_IS_LOCALE TRUE
#define MONTH_ABBR(d) nl_langinfo (month_item[0][xdate_time_get_month (d) - 1])
#define MONTH_ABBR_IS_LOCALE TRUE
#define MONTH_FULL(d) nl_langinfo (month_item[1][xdate_time_get_month (d) - 1])
#define MONTH_FULL_IS_LOCALE TRUE

#else

#define GET_AMPM(d)          (get_fallback_ampm (xdate_time_get_hour (d)))
#define GET_AMPM_IS_LOCALE   FALSE

/* Translators: this is the preferred format for expressing the date and the time */
#define PREFERRED_DATE_TIME_FMT C_("xdatetime_t", "%a %b %e %H:%M:%S %Y")

/* Translators: this is the preferred format for expressing the date */
#define PREFERRED_DATE_FMT C_("xdatetime_t", "%m/%d/%y")

/* Translators: this is the preferred format for expressing the time */
#define PREFERRED_TIME_FMT C_("xdatetime_t", "%H:%M:%S")

/* Translators: this is the preferred format for expressing 12 hour time */
#define PREFERRED_12HR_TIME_FMT C_("xdatetime_t", "%I:%M:%S %p")

#define WEEKDAY_ABBR(d)       (get_weekday_name_abbr (xdate_time_get_day_of_week (d)))
#define WEEKDAY_ABBR_IS_LOCALE FALSE
#define WEEKDAY_FULL(d)       (get_weekday_name (xdate_time_get_day_of_week (d)))
#define WEEKDAY_FULL_IS_LOCALE FALSE
/* We don't yet know if nl_langinfo (MON_n) returns standalone or complete-date
 * format forms but if nl_langinfo (ALTMON_n) is not supported then we will
 * have to use MONTH_FULL as standalone.  The same if nl_langinfo () does not
 * exist at all.  MONTH_ABBR is similar: if nl_langinfo (_NL_ABALTMON_n) is not
 * supported then we will use MONTH_ABBR as standalone.
 */
#define MONTH_ABBR(d)         (get_month_name_abbr_standalone (xdate_time_get_month (d)))
#define MONTH_ABBR_IS_LOCALE  FALSE
#define MONTH_FULL(d)         (get_month_name_standalone (xdate_time_get_month (d)))
#define MONTH_FULL_IS_LOCALE  FALSE

static const xchar_t *
get_month_name_standalone (xint_t month)
{
  switch (month)
    {
    case 1:
      /* Translators: Some languages (Baltic, Slavic, Greek, and some more)
       * need different grammatical forms of month names depending on whether
       * they are standalone or in a complete date context, with the day
       * number.  Some other languages may prefer starting with uppercase when
       * they are standalone and with lowercase when they are in a complete
       * date context.  Here are full month names in a form appropriate when
       * they are used standalone.  If your system is Linux with the glibc
       * version 2.27 (released Feb 1, 2018) or newer or if it is from the BSD
       * family (which includes OS X) then you can refer to the date command
       * line utility and see what the command `date +%OB' produces.  Also in
       * the latest Linux the command `locale alt_mon' in your native locale
       * produces a complete list of month names almost ready to copy and
       * paste here.  Note that in most of the languages (western European,
       * non-European) there is no difference between the standalone and
       * complete date form.
       */
      return C_("full month name", "January");
    case 2:
      return C_("full month name", "February");
    case 3:
      return C_("full month name", "March");
    case 4:
      return C_("full month name", "April");
    case 5:
      return C_("full month name", "May");
    case 6:
      return C_("full month name", "June");
    case 7:
      return C_("full month name", "July");
    case 8:
      return C_("full month name", "August");
    case 9:
      return C_("full month name", "September");
    case 10:
      return C_("full month name", "October");
    case 11:
      return C_("full month name", "November");
    case 12:
      return C_("full month name", "December");

    default:
      g_warning ("Invalid month number %d", month);
    }

  return NULL;
}

static const xchar_t *
get_month_name_abbr_standalone (xint_t month)
{
  switch (month)
    {
    case 1:
      /* Translators: Some languages need different grammatical forms of
       * month names depending on whether they are standalone or in a complete
       * date context, with the day number.  Some may prefer starting with
       * uppercase when they are standalone and with lowercase when they are
       * in a full date context.  However, as these names are abbreviated
       * the grammatical difference is visible probably only in Belarusian
       * and Russian.  In other languages there is no difference between
       * the standalone and complete date form when they are abbreviated.
       * If your system is Linux with the glibc version 2.27 (released
       * Feb 1, 2018) or newer then you can refer to the date command line
       * utility and see what the command `date +%Ob' produces.  Also in
       * the latest Linux the command `locale ab_alt_mon' in your native
       * locale produces a complete list of month names almost ready to copy
       * and paste here.  Note that this feature is not yet supported by any
       * other platform.  Here are abbreviated month names in a form
       * appropriate when they are used standalone.
       */
      return C_("abbreviated month name", "Jan");
    case 2:
      return C_("abbreviated month name", "Feb");
    case 3:
      return C_("abbreviated month name", "Mar");
    case 4:
      return C_("abbreviated month name", "Apr");
    case 5:
      return C_("abbreviated month name", "May");
    case 6:
      return C_("abbreviated month name", "Jun");
    case 7:
      return C_("abbreviated month name", "Jul");
    case 8:
      return C_("abbreviated month name", "Aug");
    case 9:
      return C_("abbreviated month name", "Sep");
    case 10:
      return C_("abbreviated month name", "Oct");
    case 11:
      return C_("abbreviated month name", "Nov");
    case 12:
      return C_("abbreviated month name", "Dec");

    default:
      g_warning ("Invalid month number %d", month);
    }

  return NULL;
}

static const xchar_t *
get_weekday_name (xint_t day)
{
  switch (day)
    {
    case 1:
      return C_("full weekday name", "Monday");
    case 2:
      return C_("full weekday name", "Tuesday");
    case 3:
      return C_("full weekday name", "Wednesday");
    case 4:
      return C_("full weekday name", "Thursday");
    case 5:
      return C_("full weekday name", "Friday");
    case 6:
      return C_("full weekday name", "Saturday");
    case 7:
      return C_("full weekday name", "Sunday");

    default:
      g_warning ("Invalid week day number %d", day);
    }

  return NULL;
}

static const xchar_t *
get_weekday_name_abbr (xint_t day)
{
  switch (day)
    {
    case 1:
      return C_("abbreviated weekday name", "Mon");
    case 2:
      return C_("abbreviated weekday name", "Tue");
    case 3:
      return C_("abbreviated weekday name", "Wed");
    case 4:
      return C_("abbreviated weekday name", "Thu");
    case 5:
      return C_("abbreviated weekday name", "Fri");
    case 6:
      return C_("abbreviated weekday name", "Sat");
    case 7:
      return C_("abbreviated weekday name", "Sun");

    default:
      g_warning ("Invalid week day number %d", day);
    }

  return NULL;
}

#endif  /* HAVE_LANGINFO_TIME */

#ifdef HAVE_LANGINFO_ALTMON

/* If nl_langinfo () supports ALTMON_n then MON_n returns full date format
 * forms and ALTMON_n returns standalone forms.
 */

#define MONTH_FULL_WITH_DAY(d) MONTH_FULL(d)
#define MONTH_FULL_WITH_DAY_IS_LOCALE MONTH_FULL_IS_LOCALE

static const xint_t alt_month_item[12] =
{
  ALTMON_1, ALTMON_2, ALTMON_3, ALTMON_4, ALTMON_5, ALTMON_6,
  ALTMON_7, ALTMON_8, ALTMON_9, ALTMON_10, ALTMON_11, ALTMON_12
};

#define MONTH_FULL_STANDALONE(d) nl_langinfo (alt_month_item[xdate_time_get_month (d) - 1])
#define MONTH_FULL_STANDALONE_IS_LOCALE TRUE

#else

/* If nl_langinfo () does not support ALTMON_n then either MON_n returns
 * standalone forms or nl_langinfo (MON_n) does not work so we have defined
 * it as standalone form.
 */

#define MONTH_FULL_STANDALONE(d) MONTH_FULL(d)
#define MONTH_FULL_STANDALONE_IS_LOCALE MONTH_FULL_IS_LOCALE
#define MONTH_FULL_WITH_DAY(d) (get_month_name_with_day (xdate_time_get_month (d)))
#define MONTH_FULL_WITH_DAY_IS_LOCALE FALSE

static const xchar_t *
get_month_name_with_day (xint_t month)
{
  switch (month)
    {
    case 1:
      /* Translators: Some languages need different grammatical forms of
       * month names depending on whether they are standalone or in a full
       * date context, with the day number.  Some may prefer starting with
       * uppercase when they are standalone and with lowercase when they are
       * in a full date context.  Here are full month names in a form
       * appropriate when they are used in a full date context, with the
       * day number.  If your system is Linux with the glibc version 2.27
       * (released Feb 1, 2018) or newer or if it is from the BSD family
       * (which includes OS X) then you can refer to the date command line
       * utility and see what the command `date +%B' produces.  Also in
       * the latest Linux the command `locale mon' in your native locale
       * produces a complete list of month names almost ready to copy and
       * paste here.  In older Linux systems due to a bug the result is
       * incorrect in some languages.  Note that in most of the languages
       * (western European, non-European) there is no difference between the
       * standalone and complete date form.
       */
      return C_("full month name with day", "January");
    case 2:
      return C_("full month name with day", "February");
    case 3:
      return C_("full month name with day", "March");
    case 4:
      return C_("full month name with day", "April");
    case 5:
      return C_("full month name with day", "May");
    case 6:
      return C_("full month name with day", "June");
    case 7:
      return C_("full month name with day", "July");
    case 8:
      return C_("full month name with day", "August");
    case 9:
      return C_("full month name with day", "September");
    case 10:
      return C_("full month name with day", "October");
    case 11:
      return C_("full month name with day", "November");
    case 12:
      return C_("full month name with day", "December");

    default:
      g_warning ("Invalid month number %d", month);
    }

  return NULL;
}

#endif  /* HAVE_LANGINFO_ALTMON */

#ifdef HAVE_LANGINFO_ABALTMON

/* If nl_langinfo () supports _NL_ABALTMON_n then ABMON_n returns full
 * date format forms and _NL_ABALTMON_n returns standalone forms.
 */

#define MONTH_ABBR_WITH_DAY(d) MONTH_ABBR(d)
#define MONTH_ABBR_WITH_DAY_IS_LOCALE MONTH_ABBR_IS_LOCALE

static const xint_t ab_alt_month_item[12] =
{
  _NL_ABALTMON_1, _NL_ABALTMON_2, _NL_ABALTMON_3, _NL_ABALTMON_4,
  _NL_ABALTMON_5, _NL_ABALTMON_6, _NL_ABALTMON_7, _NL_ABALTMON_8,
  _NL_ABALTMON_9, _NL_ABALTMON_10, _NL_ABALTMON_11, _NL_ABALTMON_12
};

#define MONTH_ABBR_STANDALONE(d) nl_langinfo (ab_alt_month_item[xdate_time_get_month (d) - 1])
#define MONTH_ABBR_STANDALONE_IS_LOCALE TRUE

#else

/* If nl_langinfo () does not support _NL_ABALTMON_n then either ABMON_n
 * returns standalone forms or nl_langinfo (ABMON_n) does not work so we
 * have defined it as standalone form. Now it's time to swap.
 */

#define MONTH_ABBR_STANDALONE(d) MONTH_ABBR(d)
#define MONTH_ABBR_STANDALONE_IS_LOCALE MONTH_ABBR_IS_LOCALE
#define MONTH_ABBR_WITH_DAY(d) (get_month_name_abbr_with_day (xdate_time_get_month (d)))
#define MONTH_ABBR_WITH_DAY_IS_LOCALE FALSE

static const xchar_t *
get_month_name_abbr_with_day (xint_t month)
{
  switch (month)
    {
    case 1:
      /* Translators: Some languages need different grammatical forms of
       * month names depending on whether they are standalone or in a full
       * date context, with the day number.  Some may prefer starting with
       * uppercase when they are standalone and with lowercase when they are
       * in a full date context.  Here are abbreviated month names in a form
       * appropriate when they are used in a full date context, with the
       * day number.  However, as these names are abbreviated the grammatical
       * difference is visible probably only in Belarusian and Russian.
       * In other languages there is no difference between the standalone
       * and complete date form when they are abbreviated.  If your system
       * is Linux with the glibc version 2.27 (released Feb 1, 2018) or newer
       * then you can refer to the date command line utility and see what the
       * command `date +%b' produces.  Also in the latest Linux the command
       * `locale abmon' in your native locale produces a complete list of
       * month names almost ready to copy and paste here.  In other systems
       * due to a bug the result is incorrect in some languages.
       */
      return C_("abbreviated month name with day", "Jan");
    case 2:
      return C_("abbreviated month name with day", "Feb");
    case 3:
      return C_("abbreviated month name with day", "Mar");
    case 4:
      return C_("abbreviated month name with day", "Apr");
    case 5:
      return C_("abbreviated month name with day", "May");
    case 6:
      return C_("abbreviated month name with day", "Jun");
    case 7:
      return C_("abbreviated month name with day", "Jul");
    case 8:
      return C_("abbreviated month name with day", "Aug");
    case 9:
      return C_("abbreviated month name with day", "Sep");
    case 10:
      return C_("abbreviated month name with day", "Oct");
    case 11:
      return C_("abbreviated month name with day", "Nov");
    case 12:
      return C_("abbreviated month name with day", "Dec");

    default:
      g_warning ("Invalid month number %d", month);
    }

  return NULL;
}

#endif  /* HAVE_LANGINFO_ABALTMON */

/* Format AM/PM indicator if the locale does not have a localized version. */
static const xchar_t *
get_fallback_ampm (xint_t hour)
{
  if (hour < 12)
    /* Translators: 'before midday' indicator */
    return C_("xdatetime_t", "AM");
  else
    /* Translators: 'after midday' indicator */
    return C_("xdatetime_t", "PM");
}

static inline xint_t
ymd_to_days (xint_t year,
             xint_t month,
             xint_t day)
{
  sint64_t days;

  days = ((sint64_t) year - 1) * 365 + ((year - 1) / 4) - ((year - 1) / 100)
      + ((year - 1) / 400);

  days += days_in_year[0][month - 1];
  if (GREGORIAN_LEAP (year) && month > 2)
    day++;

  days += day;

  return days;
}

static void
xdate_time_get_week_number (xdatetime_t *datetime,
                             xint_t      *week_number,
                             xint_t      *day_of_week,
                             xint_t      *day_of_year)
{
  xint_t a, b, c, d, e, f, g, n, s, month = -1, day = -1, year = -1;

  xdate_time_get_ymd (datetime, &year, &month, &day);

  if (month <= 2)
    {
      a = xdate_time_get_year (datetime) - 1;
      b = (a / 4) - (a / 100) + (a / 400);
      c = ((a - 1) / 4) - ((a - 1) / 100) + ((a - 1) / 400);
      s = b - c;
      e = 0;
      f = day - 1 + (31 * (month - 1));
    }
  else
    {
      a = year;
      b = (a / 4) - (a / 100) + (a / 400);
      c = ((a - 1) / 4) - ((a - 1) / 100) + ((a - 1) / 400);
      s = b - c;
      e = s + 1;
      f = day + (((153 * (month - 3)) + 2) / 5) + 58 + s;
    }

  g = (a + b) % 7;
  d = (f + g - e) % 7;
  n = f + 3 - d;

  if (week_number)
    {
      if (n < 0)
        *week_number = 53 - ((g - s) / 5);
      else if (n > 364 + s)
        *week_number = 1;
      else
        *week_number = (n / 7) + 1;
    }

  if (day_of_week)
    *day_of_week = d + 1;

  if (day_of_year)
    *day_of_year = f + 1;
}

/* Lifecycle {{{1 */

static xdatetime_t *
xdate_time_alloc (xtimezone_t *tz)
{
  xdatetime_t *datetime;

  datetime = g_slice_new0 (xdatetime_t);
  datetime->tz = xtime_zone_ref (tz);
  datetime->ref_count = 1;

  return datetime;
}

/**
 * xdate_time_ref:
 * @datetime: a #xdatetime_t
 *
 * Atomically increments the reference count of @datetime by one.
 *
 * Returns: the #xdatetime_t with the reference count increased
 *
 * Since: 2.26
 */
xdatetime_t *
xdate_time_ref (xdatetime_t *datetime)
{
  xreturn_val_if_fail (datetime != NULL, NULL);
  xreturn_val_if_fail (datetime->ref_count > 0, NULL);

  g_atomic_int_inc (&datetime->ref_count);

  return datetime;
}

/**
 * xdate_time_unref:
 * @datetime: a #xdatetime_t
 *
 * Atomically decrements the reference count of @datetime by one.
 *
 * When the reference count reaches zero, the resources allocated by
 * @datetime are freed
 *
 * Since: 2.26
 */
void
xdate_time_unref (xdatetime_t *datetime)
{
  g_return_if_fail (datetime != NULL);
  g_return_if_fail (datetime->ref_count > 0);

  if (g_atomic_int_dec_and_test (&datetime->ref_count))
    {
      xtime_zone_unref (datetime->tz);
      g_slice_free (xdatetime_t, datetime);
    }
}

/* Internal state transformers {{{1 */
/*< internal >
 * xdate_time_to_instant:
 * @datetime: a #xdatetime_t
 *
 * Convert a @datetime into an instant.
 *
 * An instant is a number that uniquely describes a particular
 * microsecond in time, taking time zone considerations into account.
 * (ie: "03:00 -0400" is the same instant as "02:00 -0500").
 *
 * An instant is always positive but we use a signed return value to
 * avoid troubles with C.
 */
static sint64_t
xdate_time_to_instant (xdatetime_t *datetime)
{
  sint64_t offset;

  offset = xtime_zone_get_offset (datetime->tz, datetime->interval);
  offset *= USEC_PER_SECOND;

  return datetime->days * USEC_PER_DAY + datetime->usec - offset;
}

/*< internal >
 * xdate_time_from_instant:
 * @tz: a #xtimezone_t
 * @instant: an instant in time
 *
 * Creates a #xdatetime_t from a time zone and an instant.
 *
 * This might fail if the time ends up being out of range.
 */
static xdatetime_t *
xdate_time_from_instant (xtimezone_t *tz,
                          sint64_t     instant)
{
  xdatetime_t *datetime;
  sint64_t offset;

  if (instant < 0 || instant > G_GINT64_CONSTANT (1000000000000000000))
    return NULL;

  datetime = xdate_time_alloc (tz);
  datetime->interval = xtime_zone_find_interval (tz,
                                                  G_TIME_TYPE_UNIVERSAL,
                                                  INSTANT_TO_UNIX (instant));
  offset = xtime_zone_get_offset (datetime->tz, datetime->interval);
  offset *= USEC_PER_SECOND;

  instant += offset;

  datetime->days = instant / USEC_PER_DAY;
  datetime->usec = instant % USEC_PER_DAY;

  if (datetime->days < 1 || 3652059 < datetime->days)
    {
      xdate_time_unref (datetime);
      datetime = NULL;
    }

  return datetime;
}


/*< internal >
 * xdate_time_deal_with_date_change:
 * @datetime: a #xdatetime_t
 *
 * This function should be called whenever the date changes by adding
 * days, months or years.  It does three things.
 *
 * First, we ensure that the date falls between 0001-01-01 and
 * 9999-12-31 and return %FALSE if it does not.
 *
 * Next we update the ->interval field.
 *
 * Finally, we ensure that the resulting date and time pair exists (by
 * ensuring that our time zone has an interval containing it) and
 * adjusting as required.  For example, if we have the time 02:30:00 on
 * March 13 2010 in Toronto and we add 1 day to it, we would end up with
 * 2:30am on March 14th, which doesn't exist.  In that case, we bump the
 * time up to 3:00am.
 */
static xboolean_t
xdate_time_deal_with_date_change (xdatetime_t *datetime)
{
  GTimeType was_dst;
  sint64_t full_time;
  sint64_t usec;

  if (datetime->days < 1 || datetime->days > 3652059)
    return FALSE;

  was_dst = xtime_zone_is_dst (datetime->tz, datetime->interval);

  full_time = datetime->days * USEC_PER_DAY + datetime->usec;


  usec = full_time % USEC_PER_SECOND;
  full_time /= USEC_PER_SECOND;
  full_time -= UNIX_EPOCH_START * SEC_PER_DAY;

  datetime->interval = xtime_zone_adjust_time (datetime->tz,
                                                was_dst,
                                                &full_time);
  full_time += UNIX_EPOCH_START * SEC_PER_DAY;
  full_time *= USEC_PER_SECOND;
  full_time += usec;

  datetime->days = full_time / USEC_PER_DAY;
  datetime->usec = full_time % USEC_PER_DAY;

  /* maybe daylight time caused us to shift to a different day,
   * but it definitely didn't push us into a different year */
  return TRUE;
}

static xdatetime_t *
xdate_time_replace_days (xdatetime_t *datetime,
                          xint_t       days)
{
  xdatetime_t *new;

  new = xdate_time_alloc (datetime->tz);
  new->interval = datetime->interval;
  new->usec = datetime->usec;
  new->days = days;

  if (!xdate_time_deal_with_date_change (new))
    {
      xdate_time_unref (new);
      new = NULL;
    }

  return new;
}

/* now/unix/timeval Constructors {{{1 */

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
/*< internal >
 * xdate_time_new_from_timeval:
 * @tz: a #xtimezone_t
 * @tv: a #GTimeVal
 *
 * Creates a #xdatetime_t corresponding to the given #GTimeVal @tv in the
 * given time zone @tz.
 *
 * The time contained in a #GTimeVal is always stored in the form of
 * seconds elapsed since 1970-01-01 00:00:00 UTC, regardless of the
 * given time zone.
 *
 * This call can fail (returning %NULL) if @tv represents a time outside
 * of the supported range of #xdatetime_t.
 *
 * You should release the return value by calling xdate_time_unref()
 * when you are done with it.
 *
 * Returns: a new #xdatetime_t, or %NULL
 *
 * Since: 2.26
 **/
static xdatetime_t *
xdate_time_new_from_timeval (xtimezone_t      *tz,
                              const GTimeVal *tv)
{
  sint64_t tv_sec = tv->tv_sec;

  if (tv_sec > G_MAXINT64 - 1 || !UNIX_TO_INSTANT_IS_VALID (tv_sec + 1))
    return NULL;

  return xdate_time_from_instant (tz, tv->tv_usec +
                                   UNIX_TO_INSTANT (tv->tv_sec));
}
G_GNUC_END_IGNORE_DEPRECATIONS

/*< internal >
 * xdate_time_new_from_unix:
 * @tz: a #xtimezone_t
 * @usecs: the Unix time, in microseconds since the epoch
 *
 * Creates a #xdatetime_t corresponding to the given Unix time @t_us in the
 * given time zone @tz.
 *
 * Unix time is the number of seconds that have elapsed since 1970-01-01
 * 00:00:00 UTC, regardless of the time zone given.
 *
 * This call can fail (returning %NULL) if @t represents a time outside
 * of the supported range of #xdatetime_t.
 *
 * You should release the return value by calling xdate_time_unref()
 * when you are done with it.
 *
 * Returns: a new #xdatetime_t, or %NULL
 *
 * Since: 2.26
 **/
static xdatetime_t *
xdate_time_new_from_unix (xtimezone_t *tz,
                           sint64_t     usecs)
{
  if (!UNIX_USECS_TO_INSTANT_IS_VALID (usecs))
    return NULL;

  return xdate_time_from_instant (tz, UNIX_USECS_TO_INSTANT (usecs));
}

/**
 * xdate_time_new_now: (constructor)
 * @tz: a #xtimezone_t
 *
 * Creates a #xdatetime_t corresponding to this exact instant in the given
 * time zone @tz.  The time is as accurate as the system allows, to a
 * maximum accuracy of 1 microsecond.
 *
 * This function will always succeed unless GLib is still being used after the
 * year 9999.
 *
 * You should release the return value by calling xdate_time_unref()
 * when you are done with it.
 *
 * Returns: (transfer full) (nullable): a new #xdatetime_t, or %NULL
 *
 * Since: 2.26
 **/
xdatetime_t *
xdate_time_new_now (xtimezone_t *tz)
{
  sint64_t now_us;

  now_us = g_get_real_time ();

  return xdate_time_new_from_unix (tz, now_us);
}

/**
 * xdate_time_new_now_local: (constructor)
 *
 * Creates a #xdatetime_t corresponding to this exact instant in the local
 * time zone.
 *
 * This is equivalent to calling xdate_time_new_now() with the time
 * zone returned by xtime_zone_new_local().
 *
 * Returns: (transfer full) (nullable): a new #xdatetime_t, or %NULL
 *
 * Since: 2.26
 **/
xdatetime_t *
xdate_time_new_now_local (void)
{
  xdatetime_t *datetime;
  xtimezone_t *local;

  local = xtime_zone_new_local ();
  datetime = xdate_time_new_now (local);
  xtime_zone_unref (local);

  return datetime;
}

/**
 * xdate_time_new_now_utc: (constructor)
 *
 * Creates a #xdatetime_t corresponding to this exact instant in UTC.
 *
 * This is equivalent to calling xdate_time_new_now() with the time
 * zone returned by xtime_zone_new_utc().
 *
 * Returns: (transfer full) (nullable): a new #xdatetime_t, or %NULL
 *
 * Since: 2.26
 **/
xdatetime_t *
xdate_time_new_now_utc (void)
{
  xdatetime_t *datetime;
  xtimezone_t *utc;

  utc = xtime_zone_new_utc ();
  datetime = xdate_time_new_now (utc);
  xtime_zone_unref (utc);

  return datetime;
}

/**
 * xdate_time_new_from_unix_local: (constructor)
 * @t: the Unix time
 *
 * Creates a #xdatetime_t corresponding to the given Unix time @t in the
 * local time zone.
 *
 * Unix time is the number of seconds that have elapsed since 1970-01-01
 * 00:00:00 UTC, regardless of the local time offset.
 *
 * This call can fail (returning %NULL) if @t represents a time outside
 * of the supported range of #xdatetime_t.
 *
 * You should release the return value by calling xdate_time_unref()
 * when you are done with it.
 *
 * Returns: (transfer full) (nullable): a new #xdatetime_t, or %NULL
 *
 * Since: 2.26
 **/
xdatetime_t *
xdate_time_new_from_unix_local (sint64_t t)
{
  xdatetime_t *datetime;
  xtimezone_t *local;

  if (t > G_MAXINT64 / USEC_PER_SECOND ||
      t < G_MININT64 / USEC_PER_SECOND)
    return NULL;

  local = xtime_zone_new_local ();
  datetime = xdate_time_new_from_unix (local, t * USEC_PER_SECOND);
  xtime_zone_unref (local);

  return datetime;
}

/**
 * xdate_time_new_from_unix_utc: (constructor)
 * @t: the Unix time
 *
 * Creates a #xdatetime_t corresponding to the given Unix time @t in UTC.
 *
 * Unix time is the number of seconds that have elapsed since 1970-01-01
 * 00:00:00 UTC.
 *
 * This call can fail (returning %NULL) if @t represents a time outside
 * of the supported range of #xdatetime_t.
 *
 * You should release the return value by calling xdate_time_unref()
 * when you are done with it.
 *
 * Returns: (transfer full) (nullable): a new #xdatetime_t, or %NULL
 *
 * Since: 2.26
 **/
xdatetime_t *
xdate_time_new_from_unix_utc (sint64_t t)
{
  xdatetime_t *datetime;
  xtimezone_t *utc;

  if (t > G_MAXINT64 / USEC_PER_SECOND ||
      t < G_MININT64 / USEC_PER_SECOND)
    return NULL;

  utc = xtime_zone_new_utc ();
  datetime = xdate_time_new_from_unix (utc, t * USEC_PER_SECOND);
  xtime_zone_unref (utc);

  return datetime;
}

/**
 * xdate_time_new_from_timeval_local: (constructor)
 * @tv: a #GTimeVal
 *
 * Creates a #xdatetime_t corresponding to the given #GTimeVal @tv in the
 * local time zone.
 *
 * The time contained in a #GTimeVal is always stored in the form of
 * seconds elapsed since 1970-01-01 00:00:00 UTC, regardless of the
 * local time offset.
 *
 * This call can fail (returning %NULL) if @tv represents a time outside
 * of the supported range of #xdatetime_t.
 *
 * You should release the return value by calling xdate_time_unref()
 * when you are done with it.
 *
 * Returns: (transfer full) (nullable): a new #xdatetime_t, or %NULL
 *
 * Since: 2.26
 * Deprecated: 2.62: #GTimeVal is not year-2038-safe. Use
 *    xdate_time_new_from_unix_local() instead.
 **/
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
xdatetime_t *
xdate_time_new_from_timeval_local (const GTimeVal *tv)
{
  xdatetime_t *datetime;
  xtimezone_t *local;

  local = xtime_zone_new_local ();
  datetime = xdate_time_new_from_timeval (local, tv);
  xtime_zone_unref (local);

  return datetime;
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * xdate_time_new_from_timeval_utc: (constructor)
 * @tv: a #GTimeVal
 *
 * Creates a #xdatetime_t corresponding to the given #GTimeVal @tv in UTC.
 *
 * The time contained in a #GTimeVal is always stored in the form of
 * seconds elapsed since 1970-01-01 00:00:00 UTC.
 *
 * This call can fail (returning %NULL) if @tv represents a time outside
 * of the supported range of #xdatetime_t.
 *
 * You should release the return value by calling xdate_time_unref()
 * when you are done with it.
 *
 * Returns: (transfer full) (nullable): a new #xdatetime_t, or %NULL
 *
 * Since: 2.26
 * Deprecated: 2.62: #GTimeVal is not year-2038-safe. Use
 *    xdate_time_new_from_unix_utc() instead.
 **/
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
xdatetime_t *
xdate_time_new_from_timeval_utc (const GTimeVal *tv)
{
  xdatetime_t *datetime;
  xtimezone_t *utc;

  utc = xtime_zone_new_utc ();
  datetime = xdate_time_new_from_timeval (utc, tv);
  xtime_zone_unref (utc);

  return datetime;
}
G_GNUC_END_IGNORE_DEPRECATIONS

/* Parse integers in the form d (week days), dd (hours etc), ddd (ordinal days) or dddd (years) */
static xboolean_t
get_iso8601_int (const xchar_t *text, xsize_t length, xint_t *value)
{
  xsize_t i;
  xuint_t v = 0;

  if (length < 1 || length > 4)
    return FALSE;

  for (i = 0; i < length; i++)
    {
      const xchar_t c = text[i];
      if (c < '0' || c > '9')
        return FALSE;
      v = v * 10 + (c - '0');
    }

  *value = v;
  return TRUE;
}

/* Parse seconds in the form ss or ss.sss (variable length decimal) */
static xboolean_t
get_iso8601_seconds (const xchar_t *text, xsize_t length, xdouble_t *value)
{
  xsize_t i;
  xuint64_t divisor = 1, v = 0;

  if (length < 2)
    return FALSE;

  for (i = 0; i < 2; i++)
    {
      const xchar_t c = text[i];
      if (c < '0' || c > '9')
        return FALSE;
      v = v * 10 + (c - '0');
    }

  if (length > 2 && !(text[i] == '.' || text[i] == ','))
    return FALSE;

  /* Ignore leap seconds, see xdate_time_new_from_iso8601() */
  if (v >= 60.0 && v <= 61.0)
    v = 59.0;

  i++;
  if (i == length)
    return FALSE;

  for (; i < length; i++)
    {
      const xchar_t c = text[i];
      if (c < '0' || c > '9' ||
          v > (G_MAXUINT64 - (c - '0')) / 10 ||
          divisor > G_MAXUINT64 / 10)
        return FALSE;
      v = v * 10 + (c - '0');
      divisor *= 10;
    }

  *value = (xdouble_t) v / divisor;
  return TRUE;
}

static xdatetime_t *
xdate_time_new_ordinal (xtimezone_t *tz, xint_t year, xint_t ordinal_day, xint_t hour, xint_t minute, xdouble_t seconds)
{
  xdatetime_t *dt;

  if (ordinal_day < 1 || ordinal_day > (GREGORIAN_LEAP (year) ? 366 : 365))
    return NULL;

  dt = xdate_time_new (tz, year, 1, 1, hour, minute, seconds);
  if (dt == NULL)
    return NULL;
  dt->days += ordinal_day - 1;

  return dt;
}

static xdatetime_t *
xdate_time_new_week (xtimezone_t *tz, xint_t year, xint_t week, xint_t week_day, xint_t hour, xint_t minute, xdouble_t seconds)
{
  sint64_t p;
  xint_t max_week, jan4_week_day, ordinal_day;
  xdatetime_t *dt;

  p = (year * 365 + (year / 4) - (year / 100) + (year / 400)) % 7;
  max_week = p == 4 ? 53 : 52;

  if (week < 1 || week > max_week || week_day < 1 || week_day > 7)
    return NULL;

  dt = xdate_time_new (tz, year, 1, 4, 0, 0, 0);
  if (dt == NULL)
    return NULL;
  xdate_time_get_week_number (dt, NULL, &jan4_week_day, NULL);
  xdate_time_unref (dt);

  ordinal_day = (week * 7) + week_day - (jan4_week_day + 3);
  if (ordinal_day < 0)
    {
      year--;
      ordinal_day += GREGORIAN_LEAP (year) ? 366 : 365;
    }
  else if (ordinal_day > (GREGORIAN_LEAP (year) ? 366 : 365))
    {
      ordinal_day -= (GREGORIAN_LEAP (year) ? 366 : 365);
      year++;
    }

  return xdate_time_new_ordinal (tz, year, ordinal_day, hour, minute, seconds);
}

static xdatetime_t *
parse_iso8601_date (const xchar_t *text, xsize_t length,
                    xint_t hour, xint_t minute, xdouble_t seconds, xtimezone_t *tz)
{
  /* YYYY-MM-DD */
  if (length == 10 && text[4] == '-' && text[7] == '-')
    {
      int year, month, day;
      if (!get_iso8601_int (text, 4, &year) ||
          !get_iso8601_int (text + 5, 2, &month) ||
          !get_iso8601_int (text + 8, 2, &day))
        return NULL;
      return xdate_time_new (tz, year, month, day, hour, minute, seconds);
    }
  /* YYYY-DDD */
  else if (length == 8 && text[4] == '-')
    {
      xint_t year, ordinal_day;
      if (!get_iso8601_int (text, 4, &year) ||
          !get_iso8601_int (text + 5, 3, &ordinal_day))
        return NULL;
      return xdate_time_new_ordinal (tz, year, ordinal_day, hour, minute, seconds);
    }
  /* YYYY-Www-D */
  else if (length == 10 && text[4] == '-' && text[5] == 'W' && text[8] == '-')
    {
      xint_t year, week, week_day;
      if (!get_iso8601_int (text, 4, &year) ||
          !get_iso8601_int (text + 6, 2, &week) ||
          !get_iso8601_int (text + 9, 1, &week_day))
        return NULL;
      return xdate_time_new_week (tz, year, week, week_day, hour, minute, seconds);
    }
  /* YYYYWwwD */
  else if (length == 8 && text[4] == 'W')
    {
      xint_t year, week, week_day;
      if (!get_iso8601_int (text, 4, &year) ||
          !get_iso8601_int (text + 5, 2, &week) ||
          !get_iso8601_int (text + 7, 1, &week_day))
        return NULL;
      return xdate_time_new_week (tz, year, week, week_day, hour, minute, seconds);
    }
  /* YYYYMMDD */
  else if (length == 8)
    {
      int year, month, day;
      if (!get_iso8601_int (text, 4, &year) ||
          !get_iso8601_int (text + 4, 2, &month) ||
          !get_iso8601_int (text + 6, 2, &day))
        return NULL;
      return xdate_time_new (tz, year, month, day, hour, minute, seconds);
    }
  /* YYYYDDD */
  else if (length == 7)
    {
      xint_t year, ordinal_day;
      if (!get_iso8601_int (text, 4, &year) ||
          !get_iso8601_int (text + 4, 3, &ordinal_day))
        return NULL;
      return xdate_time_new_ordinal (tz, year, ordinal_day, hour, minute, seconds);
    }
  else
    return FALSE;
}

static xtimezone_t *
parse_iso8601_timezone (const xchar_t *text, xsize_t length, xssize_t *tz_offset)
{
  xint_t i, tz_length, offset_hours, offset_minutes;
  xint_t offset_sign = 1;
  xtimezone_t *tz;

  /* UTC uses Z suffix  */
  if (length > 0 && text[length - 1] == 'Z')
    {
      *tz_offset = length - 1;
      return xtime_zone_new_utc ();
    }

  /* Look for '+' or '-' of offset */
  for (i = length - 1; i >= 0; i--)
    if (text[i] == '+' || text[i] == '-')
      {
        offset_sign = text[i] == '-' ? -1 : 1;
        break;
      }
  if (i < 0)
    return NULL;
  tz_length = length - i;

  /* +hh:mm or -hh:mm */
  if (tz_length == 6 && text[i+3] == ':')
    {
      if (!get_iso8601_int (text + i + 1, 2, &offset_hours) ||
          !get_iso8601_int (text + i + 4, 2, &offset_minutes))
        return NULL;
    }
  /* +hhmm or -hhmm */
  else if (tz_length == 5)
    {
      if (!get_iso8601_int (text + i + 1, 2, &offset_hours) ||
          !get_iso8601_int (text + i + 3, 2, &offset_minutes))
        return NULL;
    }
  /* +hh or -hh */
  else if (tz_length == 3)
    {
      if (!get_iso8601_int (text + i + 1, 2, &offset_hours))
        return NULL;
      offset_minutes = 0;
    }
  else
    return NULL;

  *tz_offset = i;
  tz = xtime_zone_new_identifier (text + i);

  /* Double-check that the xtimezone_t matches our interpretation of the timezone.
   * This can fail because our interpretation is less strict than (for example)
   * parse_time() in gtimezone.c, which restricts the range of the parsed
   * integers. */
  if (tz == NULL || xtime_zone_get_offset (tz, 0) != offset_sign * (offset_hours * 3600 + offset_minutes * 60))
    {
      g_clear_pointer (&tz, xtime_zone_unref);
      return NULL;
    }

  return tz;
}

static xboolean_t
parse_iso8601_time (const xchar_t *text, xsize_t length,
                    xint_t *hour, xint_t *minute, xdouble_t *seconds, xtimezone_t **tz)
{
  xssize_t tz_offset = -1;

  /* Check for timezone suffix */
  *tz = parse_iso8601_timezone (text, length, &tz_offset);
  if (tz_offset >= 0)
    length = tz_offset;

  /* hh:mm:ss(.sss) */
  if (length >= 8 && text[2] == ':' && text[5] == ':')
    {
      return get_iso8601_int (text, 2, hour) &&
             get_iso8601_int (text + 3, 2, minute) &&
             get_iso8601_seconds (text + 6, length - 6, seconds);
    }
  /* hhmmss(.sss) */
  else if (length >= 6)
    {
      return get_iso8601_int (text, 2, hour) &&
             get_iso8601_int (text + 2, 2, minute) &&
             get_iso8601_seconds (text + 4, length - 4, seconds);
    }
  else
    return FALSE;
}

/**
 * xdate_time_new_from_iso8601: (constructor)
 * @text: an ISO 8601 formatted time string.
 * @default_tz: (nullable): a #xtimezone_t to use if the text doesn't contain a
 *                          timezone, or %NULL.
 *
 * Creates a #xdatetime_t corresponding to the given
 * [ISO 8601 formatted string](https://en.wikipedia.org/wiki/ISO_8601)
 * @text. ISO 8601 strings of the form <date><sep><time><tz> are supported, with
 * some extensions from [RFC 3339](https://tools.ietf.org/html/rfc3339) as
 * mentioned below.
 *
 * Note that as #xdatetime_t "is oblivious to leap seconds", leap seconds information
 * in an ISO-8601 string will be ignored, so a `23:59:60` time would be parsed as
 * `23:59:59`.
 *
 * <sep> is the separator and can be either 'T', 't' or ' '. The latter two
 * separators are an extension from
 * [RFC 3339](https://tools.ietf.org/html/rfc3339#section-5.6).
 *
 * <date> is in the form:
 *
 * - `YYYY-MM-DD` - Year/month/day, e.g. 2016-08-24.
 * - `YYYYMMDD` - Same as above without dividers.
 * - `YYYY-DDD` - Ordinal day where DDD is from 001 to 366, e.g. 2016-237.
 * - `YYYYDDD` - Same as above without dividers.
 * - `YYYY-Www-D` - Week day where ww is from 01 to 52 and D from 1-7,
 *   e.g. 2016-W34-3.
 * - `YYYYWwwD` - Same as above without dividers.
 *
 * <time> is in the form:
 *
 * - `hh:mm:ss(.sss)` - Hours, minutes, seconds (subseconds), e.g. 22:10:42.123.
 * - `hhmmss(.sss)` - Same as above without dividers.
 *
 * <tz> is an optional timezone suffix of the form:
 *
 * - `Z` - UTC.
 * - `+hh:mm` or `-hh:mm` - Offset from UTC in hours and minutes, e.g. +12:00.
 * - `+hh` or `-hh` - Offset from UTC in hours, e.g. +12.
 *
 * If the timezone is not provided in @text it must be provided in @default_tz
 * (this field is otherwise ignored).
 *
 * This call can fail (returning %NULL) if @text is not a valid ISO 8601
 * formatted string.
 *
 * You should release the return value by calling xdate_time_unref()
 * when you are done with it.
 *
 * Returns: (transfer full) (nullable): a new #xdatetime_t, or %NULL
 *
 * Since: 2.56
 */
xdatetime_t *
xdate_time_new_from_iso8601 (const xchar_t *text, xtimezone_t *default_tz)
{
  xint_t length, date_length = -1;
  xint_t hour = 0, minute = 0;
  xdouble_t seconds = 0.0;
  xtimezone_t *tz = NULL;
  xdatetime_t *datetime = NULL;

  xreturn_val_if_fail (text != NULL, NULL);

  /* Count length of string and find date / time separator ('T', 't', or ' ') */
  for (length = 0; text[length] != '\0'; length++)
    {
      if (date_length < 0 && (text[length] == 'T' || text[length] == 't' || text[length] == ' '))
        date_length = length;
    }

  if (date_length < 0)
    return NULL;

  if (!parse_iso8601_time (text + date_length + 1, length - (date_length + 1),
                           &hour, &minute, &seconds, &tz))
    goto out;
  if (tz == NULL && default_tz == NULL)
    return NULL;

  datetime = parse_iso8601_date (text, date_length, hour, minute, seconds, tz ? tz : default_tz);

out:
    if (tz != NULL)
      xtime_zone_unref (tz);
    return datetime;
}

/* full new functions {{{1 */

/**
 * xdate_time_new: (constructor)
 * @tz: a #xtimezone_t
 * @year: the year component of the date
 * @month: the month component of the date
 * @day: the day component of the date
 * @hour: the hour component of the date
 * @minute: the minute component of the date
 * @seconds: the number of seconds past the minute
 *
 * Creates a new #xdatetime_t corresponding to the given date and time in
 * the time zone @tz.
 *
 * The @year must be between 1 and 9999, @month between 1 and 12 and @day
 * between 1 and 28, 29, 30 or 31 depending on the month and the year.
 *
 * @hour must be between 0 and 23 and @minute must be between 0 and 59.
 *
 * @seconds must be at least 0.0 and must be strictly less than 60.0.
 * It will be rounded down to the nearest microsecond.
 *
 * If the given time is not representable in the given time zone (for
 * example, 02:30 on March 14th 2010 in Toronto, due to daylight savings
 * time) then the time will be rounded up to the nearest existing time
 * (in this case, 03:00).  If this matters to you then you should verify
 * the return value for containing the same as the numbers you gave.
 *
 * In the case that the given time is ambiguous in the given time zone
 * (for example, 01:30 on November 7th 2010 in Toronto, due to daylight
 * savings time) then the time falling within standard (ie:
 * non-daylight) time is taken.
 *
 * It not considered a programmer error for the values to this function
 * to be out of range, but in the case that they are, the function will
 * return %NULL.
 *
 * You should release the return value by calling xdate_time_unref()
 * when you are done with it.
 *
 * Returns: (transfer full) (nullable): a new #xdatetime_t, or %NULL
 *
 * Since: 2.26
 **/
xdatetime_t *
xdate_time_new (xtimezone_t *tz,
                 xint_t       year,
                 xint_t       month,
                 xint_t       day,
                 xint_t       hour,
                 xint_t       minute,
                 xdouble_t    seconds)
{
  xdatetime_t *datetime;
  sint64_t full_time;
  /* keep these variables as volatile. We do not want them ending up in
   * registers - them doing so may cause us to hit precision problems on i386.
   * See: https://bugzilla.gnome.org/show_bug.cgi?id=792410 */
  volatile sint64_t usec;
  volatile xdouble_t usecd;

  xreturn_val_if_fail (tz != NULL, NULL);

  if (year < 1 || year > 9999 ||
      month < 1 || month > 12 ||
      day < 1 || day > days_in_months[GREGORIAN_LEAP (year)][month] ||
      hour < 0 || hour > 23 ||
      minute < 0 || minute > 59 ||
      isnan (seconds) ||
      seconds < 0.0 || seconds >= 60.0)
    return NULL;

  datetime = xdate_time_alloc (tz);
  datetime->days = ymd_to_days (year, month, day);
  datetime->usec = (hour   * USEC_PER_HOUR)
                 + (minute * USEC_PER_MINUTE)
                 + (sint64_t) (seconds * USEC_PER_SECOND);

  full_time = SEC_PER_DAY *
                (ymd_to_days (year, month, day) - UNIX_EPOCH_START) +
              SECS_PER_HOUR * hour +
              SECS_PER_MINUTE * minute +
              (int) seconds;

  datetime->interval = xtime_zone_adjust_time (datetime->tz,
                                                G_TIME_TYPE_STANDARD,
                                                &full_time);

  /* This is the correct way to convert a scaled FP value to integer.
   * If this surprises you, please observe that (int)(1.000001 * 1e6)
   * is 1000000.  This is not a problem with precision, it's just how
   * FP numbers work.
   * See https://bugzilla.gnome.org/show_bug.cgi?id=697715. */
  usec = seconds * USEC_PER_SECOND;
  usecd = (usec + 1) * 1e-6;
  if (usecd <= seconds) {
    usec++;
  }

  full_time += UNIX_EPOCH_START * SEC_PER_DAY;
  datetime->days = full_time / SEC_PER_DAY;
  datetime->usec = (full_time % SEC_PER_DAY) * USEC_PER_SECOND;
  datetime->usec += usec % USEC_PER_SECOND;

  return datetime;
}

/**
 * xdate_time_new_local: (constructor)
 * @year: the year component of the date
 * @month: the month component of the date
 * @day: the day component of the date
 * @hour: the hour component of the date
 * @minute: the minute component of the date
 * @seconds: the number of seconds past the minute
 *
 * Creates a new #xdatetime_t corresponding to the given date and time in
 * the local time zone.
 *
 * This call is equivalent to calling xdate_time_new() with the time
 * zone returned by xtime_zone_new_local().
 *
 * Returns: (transfer full) (nullable): a #xdatetime_t, or %NULL
 *
 * Since: 2.26
 **/
xdatetime_t *
xdate_time_new_local (xint_t    year,
                       xint_t    month,
                       xint_t    day,
                       xint_t    hour,
                       xint_t    minute,
                       xdouble_t seconds)
{
  xdatetime_t *datetime;
  xtimezone_t *local;

  local = xtime_zone_new_local ();
  datetime = xdate_time_new (local, year, month, day, hour, minute, seconds);
  xtime_zone_unref (local);

  return datetime;
}

/**
 * xdate_time_new_utc: (constructor)
 * @year: the year component of the date
 * @month: the month component of the date
 * @day: the day component of the date
 * @hour: the hour component of the date
 * @minute: the minute component of the date
 * @seconds: the number of seconds past the minute
 *
 * Creates a new #xdatetime_t corresponding to the given date and time in
 * UTC.
 *
 * This call is equivalent to calling xdate_time_new() with the time
 * zone returned by xtime_zone_new_utc().
 *
 * Returns: (transfer full) (nullable): a #xdatetime_t, or %NULL
 *
 * Since: 2.26
 **/
xdatetime_t *
xdate_time_new_utc (xint_t    year,
                     xint_t    month,
                     xint_t    day,
                     xint_t    hour,
                     xint_t    minute,
                     xdouble_t seconds)
{
  xdatetime_t *datetime;
  xtimezone_t *utc;

  utc = xtime_zone_new_utc ();
  datetime = xdate_time_new (utc, year, month, day, hour, minute, seconds);
  xtime_zone_unref (utc);

  return datetime;
}

/* Adders {{{1 */

/**
 * xdate_time_add:
 * @datetime: a #xdatetime_t
 * @timespan: a #GTimeSpan
 *
 * Creates a copy of @datetime and adds the specified timespan to the copy.
 *
 * Returns: (transfer full) (nullable): the newly created #xdatetime_t which
 *   should be freed with xdate_time_unref(), or %NULL
 *
 * Since: 2.26
 */
xdatetime_t*
xdate_time_add (xdatetime_t *datetime,
                 GTimeSpan  timespan)
{
  xreturn_val_if_fail (datetime != NULL, NULL);

  return xdate_time_from_instant (datetime->tz, timespan +
                                   xdate_time_to_instant (datetime));
}

/**
 * xdate_time_add_years:
 * @datetime: a #xdatetime_t
 * @years: the number of years
 *
 * Creates a copy of @datetime and adds the specified number of years to the
 * copy. Add negative values to subtract years.
 *
 * As with xdate_time_add_months(), if the resulting date would be 29th
 * February on a non-leap year, the day will be clamped to 28th February.
 *
 * Returns: (transfer full) (nullable): the newly created #xdatetime_t which
 *   should be freed with xdate_time_unref(), or %NULL
 *
 * Since: 2.26
 */
xdatetime_t *
xdate_time_add_years (xdatetime_t *datetime,
                       xint_t       years)
{
  xint_t year, month, day;

  xreturn_val_if_fail (datetime != NULL, NULL);

  if (years < -10000 || years > 10000)
    return NULL;

  xdate_time_get_ymd (datetime, &year, &month, &day);
  year += years;

  /* only possible issue is if we've entered a year with no February 29
   */
  if (month == 2 && day == 29 && !GREGORIAN_LEAP (year))
    day = 28;

  return xdate_time_replace_days (datetime, ymd_to_days (year, month, day));
}

/**
 * xdate_time_add_months:
 * @datetime: a #xdatetime_t
 * @months: the number of months
 *
 * Creates a copy of @datetime and adds the specified number of months to the
 * copy. Add negative values to subtract months.
 *
 * The day of the month of the resulting #xdatetime_t is clamped to the number
 * of days in the updated calendar month. For example, if adding 1 month to
 * 31st January 2018, the result would be 28th February 2018. In 2020 (a leap
 * year), the result would be 29th February.
 *
 * Returns: (transfer full) (nullable): the newly created #xdatetime_t which
 *   should be freed with xdate_time_unref(), or %NULL
 *
 * Since: 2.26
 */
xdatetime_t*
xdate_time_add_months (xdatetime_t *datetime,
                        xint_t       months)
{
  xint_t year, month, day;

  xreturn_val_if_fail (datetime != NULL, NULL);
  xdate_time_get_ymd (datetime, &year, &month, &day);

  if (months < -120000 || months > 120000)
    return NULL;

  year += months / 12;
  month += months % 12;
  if (month < 1)
    {
      month += 12;
      year--;
    }
  else if (month > 12)
    {
      month -= 12;
      year++;
    }

  day = MIN (day, days_in_months[GREGORIAN_LEAP (year)][month]);

  return xdate_time_replace_days (datetime, ymd_to_days (year, month, day));
}

/**
 * xdate_time_add_weeks:
 * @datetime: a #xdatetime_t
 * @weeks: the number of weeks
 *
 * Creates a copy of @datetime and adds the specified number of weeks to the
 * copy. Add negative values to subtract weeks.
 *
 * Returns: (transfer full) (nullable): the newly created #xdatetime_t which
 *   should be freed with xdate_time_unref(), or %NULL
 *
 * Since: 2.26
 */
xdatetime_t*
xdate_time_add_weeks (xdatetime_t *datetime,
                       xint_t             weeks)
{
  xreturn_val_if_fail (datetime != NULL, NULL);

  return xdate_time_add_days (datetime, weeks * 7);
}

/**
 * xdate_time_add_days:
 * @datetime: a #xdatetime_t
 * @days: the number of days
 *
 * Creates a copy of @datetime and adds the specified number of days to the
 * copy. Add negative values to subtract days.
 *
 * Returns: (transfer full) (nullable): the newly created #xdatetime_t which
 *   should be freed with xdate_time_unref(), or %NULL
 *
 * Since: 2.26
 */
xdatetime_t*
xdate_time_add_days (xdatetime_t *datetime,
                      xint_t       days)
{
  xreturn_val_if_fail (datetime != NULL, NULL);

  if (days < -3660000 || days > 3660000)
    return NULL;

  return xdate_time_replace_days (datetime, datetime->days + days);
}

/**
 * xdate_time_add_hours:
 * @datetime: a #xdatetime_t
 * @hours: the number of hours to add
 *
 * Creates a copy of @datetime and adds the specified number of hours.
 * Add negative values to subtract hours.
 *
 * Returns: (transfer full) (nullable): the newly created #xdatetime_t which
 *   should be freed with xdate_time_unref(), or %NULL
 *
 * Since: 2.26
 */
xdatetime_t*
xdate_time_add_hours (xdatetime_t *datetime,
                       xint_t       hours)
{
  return xdate_time_add (datetime, hours * USEC_PER_HOUR);
}

/**
 * xdate_time_add_minutes:
 * @datetime: a #xdatetime_t
 * @minutes: the number of minutes to add
 *
 * Creates a copy of @datetime adding the specified number of minutes.
 * Add negative values to subtract minutes.
 *
 * Returns: (transfer full) (nullable): the newly created #xdatetime_t which
 *   should be freed with xdate_time_unref(), or %NULL
 *
 * Since: 2.26
 */
xdatetime_t*
xdate_time_add_minutes (xdatetime_t *datetime,
                         xint_t             minutes)
{
  return xdate_time_add (datetime, minutes * USEC_PER_MINUTE);
}


/**
 * xdate_time_add_seconds:
 * @datetime: a #xdatetime_t
 * @seconds: the number of seconds to add
 *
 * Creates a copy of @datetime and adds the specified number of seconds.
 * Add negative values to subtract seconds.
 *
 * Returns: (transfer full) (nullable): the newly created #xdatetime_t which
 *   should be freed with xdate_time_unref(), or %NULL
 *
 * Since: 2.26
 */
xdatetime_t*
xdate_time_add_seconds (xdatetime_t *datetime,
                         xdouble_t    seconds)
{
  return xdate_time_add (datetime, seconds * USEC_PER_SECOND);
}

/**
 * xdate_time_add_full:
 * @datetime: a #xdatetime_t
 * @years: the number of years to add
 * @months: the number of months to add
 * @days: the number of days to add
 * @hours: the number of hours to add
 * @minutes: the number of minutes to add
 * @seconds: the number of seconds to add
 *
 * Creates a new #xdatetime_t adding the specified values to the current date and
 * time in @datetime. Add negative values to subtract.
 *
 * Returns: (transfer full) (nullable): the newly created #xdatetime_t which
 *   should be freed with xdate_time_unref(), or %NULL
 *
 * Since: 2.26
 */
xdatetime_t *
xdate_time_add_full (xdatetime_t *datetime,
                      xint_t       years,
                      xint_t       months,
                      xint_t       days,
                      xint_t       hours,
                      xint_t       minutes,
                      xdouble_t    seconds)
{
  xint_t year, month, day;
  sint64_t full_time;
  xdatetime_t *new;
  xint_t interval;

  xreturn_val_if_fail (datetime != NULL, NULL);
  xdate_time_get_ymd (datetime, &year, &month, &day);

  months += years * 12;

  if (months < -120000 || months > 120000)
    return NULL;

  if (days < -3660000 || days > 3660000)
    return NULL;

  year += months / 12;
  month += months % 12;
  if (month < 1)
    {
      month += 12;
      year--;
    }
  else if (month > 12)
    {
      month -= 12;
      year++;
    }

  day = MIN (day, days_in_months[GREGORIAN_LEAP (year)][month]);

  /* full_time is now in unix (local) time */
  full_time = datetime->usec / USEC_PER_SECOND + SEC_PER_DAY *
    (ymd_to_days (year, month, day) + days - UNIX_EPOCH_START);

  interval = xtime_zone_adjust_time (datetime->tz,
                                      xtime_zone_is_dst (datetime->tz,
                                                          datetime->interval),
                                      &full_time);

  /* move to UTC unix time */
  full_time -= xtime_zone_get_offset (datetime->tz, interval);

  /* convert back to an instant, add back fractional seconds */
  full_time += UNIX_EPOCH_START * SEC_PER_DAY;
  full_time = full_time * USEC_PER_SECOND +
              datetime->usec % USEC_PER_SECOND;

  /* do the actual addition now */
  full_time += (hours * USEC_PER_HOUR) +
               (minutes * USEC_PER_MINUTE) +
               (sint64_t) (seconds * USEC_PER_SECOND);

  /* find the new interval */
  interval = xtime_zone_find_interval (datetime->tz,
                                        G_TIME_TYPE_UNIVERSAL,
                                        INSTANT_TO_UNIX (full_time));

  /* convert back into local time */
  full_time += USEC_PER_SECOND *
               xtime_zone_get_offset (datetime->tz, interval);

  /* split into days and usec of a new datetime */
  new = xdate_time_alloc (datetime->tz);
  new->interval = interval;
  new->days = full_time / USEC_PER_DAY;
  new->usec = full_time % USEC_PER_DAY;

  /* XXX validate */

  return new;
}

/* Compare, difference, hash, equal {{{1 */
/**
 * xdate_time_compare:
 * @dt1: (type xdatetime_t) (not nullable): first #xdatetime_t to compare
 * @dt2: (type xdatetime_t) (not nullable): second #xdatetime_t to compare
 *
 * A comparison function for #GDateTimes that is suitable
 * as a #GCompareFunc. Both #GDateTimes must be non-%NULL.
 *
 * Returns: -1, 0 or 1 if @dt1 is less than, equal to or greater
 *   than @dt2.
 *
 * Since: 2.26
 */
xint_t
xdate_time_compare (xconstpointer dt1,
                     xconstpointer dt2)
{
  sint64_t difference;

  difference = xdate_time_difference ((xdatetime_t *) dt1, (xdatetime_t *) dt2);

  if (difference < 0)
    return -1;

  else if (difference > 0)
    return 1;

  else
    return 0;
}

/**
 * xdate_time_difference:
 * @end: a #xdatetime_t
 * @begin: a #xdatetime_t
 *
 * Calculates the difference in time between @end and @begin.  The
 * #GTimeSpan that is returned is effectively @end - @begin (ie:
 * positive if the first parameter is larger).
 *
 * Returns: the difference between the two #xdatetime_t, as a time
 *   span expressed in microseconds.
 *
 * Since: 2.26
 */
GTimeSpan
xdate_time_difference (xdatetime_t *end,
                        xdatetime_t *begin)
{
  xreturn_val_if_fail (begin != NULL, 0);
  xreturn_val_if_fail (end != NULL, 0);

  return xdate_time_to_instant (end) -
         xdate_time_to_instant (begin);
}

/**
 * xdate_time_hash:
 * @datetime: (type xdatetime_t) (not nullable): a #xdatetime_t
 *
 * Hashes @datetime into a #xuint_t, suitable for use within #xhashtable_t.
 *
 * Returns: a #xuint_t containing the hash
 *
 * Since: 2.26
 */
xuint_t
xdate_time_hash (xconstpointer datetime)
{
  xreturn_val_if_fail (datetime != NULL, 0);

  return xdate_time_to_instant ((xdatetime_t *) datetime);
}

/**
 * xdate_time_equal:
 * @dt1: (type xdatetime_t) (not nullable): a #xdatetime_t
 * @dt2: (type xdatetime_t) (not nullable): a #xdatetime_t
 *
 * Checks to see if @dt1 and @dt2 are equal.
 *
 * Equal here means that they represent the same moment after converting
 * them to the same time zone.
 *
 * Returns: %TRUE if @dt1 and @dt2 are equal
 *
 * Since: 2.26
 */
xboolean_t
xdate_time_equal (xconstpointer dt1,
                   xconstpointer dt2)
{
  return xdate_time_difference ((xdatetime_t *) dt1, (xdatetime_t *) dt2) == 0;
}

/* Year, Month, Day Getters {{{1 */
/**
 * xdate_time_get_ymd:
 * @datetime: a #xdatetime_t.
 * @year: (out) (optional): the return location for the gregorian year, or %NULL.
 * @month: (out) (optional): the return location for the month of the year, or %NULL.
 * @day: (out) (optional): the return location for the day of the month, or %NULL.
 *
 * Retrieves the Gregorian day, month, and year of a given #xdatetime_t.
 *
 * Since: 2.26
 **/
void
xdate_time_get_ymd (xdatetime_t *datetime,
                     xint_t      *year,
                     xint_t      *month,
                     xint_t      *day)
{
  xint_t the_year;
  xint_t the_month;
  xint_t the_day;
  xint_t remaining_days;
  xint_t y100_cycles;
  xint_t y4_cycles;
  xint_t y1_cycles;
  xint_t preceding;
  xboolean_t leap;

  g_return_if_fail (datetime != NULL);

  remaining_days = datetime->days;

  /*
   * We need to convert an offset in days to its year/month/day representation.
   * Leap years makes this a little trickier than it should be, so we use
   * 400, 100 and 4 years cycles here to get to the correct year.
   */

  /* Our days offset starts sets 0001-01-01 as day 1, if it was day 0 our
   * math would be simpler, so let's do it */
  remaining_days--;

  the_year = (remaining_days / DAYS_IN_400YEARS) * 400 + 1;
  remaining_days = remaining_days % DAYS_IN_400YEARS;

  y100_cycles = remaining_days / DAYS_IN_100YEARS;
  remaining_days = remaining_days % DAYS_IN_100YEARS;
  the_year += y100_cycles * 100;

  y4_cycles = remaining_days / DAYS_IN_4YEARS;
  remaining_days = remaining_days % DAYS_IN_4YEARS;
  the_year += y4_cycles * 4;

  y1_cycles = remaining_days / 365;
  the_year += y1_cycles;
  remaining_days = remaining_days % 365;

  if (y1_cycles == 4 || y100_cycles == 4) {
    xassert (remaining_days == 0);

    /* special case that indicates that the date is actually one year before,
     * in the 31th of December */
    the_year--;
    the_month = 12;
    the_day = 31;
    goto end;
  }

  /* now get the month and the day */
  leap = y1_cycles == 3 && (y4_cycles != 24 || y100_cycles == 3);

  xassert (leap == GREGORIAN_LEAP(the_year));

  the_month = (remaining_days + 50) >> 5;
  preceding = (days_in_year[0][the_month - 1] + (the_month > 2 && leap));
  if (preceding > remaining_days)
    {
      /* estimate is too large */
      the_month -= 1;
      preceding -= leap ? days_in_months[1][the_month]
                        : days_in_months[0][the_month];
    }

  remaining_days -= preceding;
  xassert(0 <= remaining_days);

  the_day = remaining_days + 1;

end:
  if (year)
    *year = the_year;
  if (month)
    *month = the_month;
  if (day)
    *day = the_day;
}

/**
 * xdate_time_get_year:
 * @datetime: A #xdatetime_t
 *
 * Retrieves the year represented by @datetime in the Gregorian calendar.
 *
 * Returns: the year represented by @datetime
 *
 * Since: 2.26
 */
xint_t
xdate_time_get_year (xdatetime_t *datetime)
{
  xint_t year;

  xreturn_val_if_fail (datetime != NULL, 0);

  xdate_time_get_ymd (datetime, &year, NULL, NULL);

  return year;
}

/**
 * xdate_time_get_month:
 * @datetime: a #xdatetime_t
 *
 * Retrieves the month of the year represented by @datetime in the Gregorian
 * calendar.
 *
 * Returns: the month represented by @datetime
 *
 * Since: 2.26
 */
xint_t
xdate_time_get_month (xdatetime_t *datetime)
{
  xint_t month;

  xreturn_val_if_fail (datetime != NULL, 0);

  xdate_time_get_ymd (datetime, NULL, &month, NULL);

  return month;
}

/**
 * xdate_time_get_day_of_month:
 * @datetime: a #xdatetime_t
 *
 * Retrieves the day of the month represented by @datetime in the gregorian
 * calendar.
 *
 * Returns: the day of the month
 *
 * Since: 2.26
 */
xint_t
xdate_time_get_day_of_month (xdatetime_t *datetime)
{
  xint_t           day_of_year,
                 i;
  const xuint16_t *days;
  xuint16_t        last = 0;

  xreturn_val_if_fail (datetime != NULL, 0);

  days = days_in_year[GREGORIAN_LEAP (xdate_time_get_year (datetime)) ? 1 : 0];
  xdate_time_get_week_number (datetime, NULL, NULL, &day_of_year);

  for (i = 1; i <= 12; i++)
    {
      if (days [i] >= day_of_year)
        return day_of_year - last;
      last = days [i];
    }

  g_warn_if_reached ();
  return 0;
}

/* Week of year / day of week getters {{{1 */
/**
 * xdate_time_get_week_numbering_year:
 * @datetime: a #xdatetime_t
 *
 * Returns the ISO 8601 week-numbering year in which the week containing
 * @datetime falls.
 *
 * This function, taken together with xdate_time_get_week_of_year() and
 * xdate_time_get_day_of_week() can be used to determine the full ISO
 * week date on which @datetime falls.
 *
 * This is usually equal to the normal Gregorian year (as returned by
 * xdate_time_get_year()), except as detailed below:
 *
 * For Thursday, the week-numbering year is always equal to the usual
 * calendar year.  For other days, the number is such that every day
 * within a complete week (Monday to Sunday) is contained within the
 * same week-numbering year.
 *
 * For Monday, Tuesday and Wednesday occurring near the end of the year,
 * this may mean that the week-numbering year is one greater than the
 * calendar year (so that these days have the same week-numbering year
 * as the Thursday occurring early in the next year).
 *
 * For Friday, Saturday and Sunday occurring near the start of the year,
 * this may mean that the week-numbering year is one less than the
 * calendar year (so that these days have the same week-numbering year
 * as the Thursday occurring late in the previous year).
 *
 * An equivalent description is that the week-numbering year is equal to
 * the calendar year containing the majority of the days in the current
 * week (Monday to Sunday).
 *
 * Note that January 1 0001 in the proleptic Gregorian calendar is a
 * Monday, so this function never returns 0.
 *
 * Returns: the ISO 8601 week-numbering year for @datetime
 *
 * Since: 2.26
 **/
xint_t
xdate_time_get_week_numbering_year (xdatetime_t *datetime)
{
  xint_t year = -1, month = -1, day = -1, weekday;

  xdate_time_get_ymd (datetime, &year, &month, &day);
  weekday = xdate_time_get_day_of_week (datetime);

  /* January 1, 2, 3 might be in the previous year if they occur after
   * Thursday.
   *
   *   Jan 1:  Friday, Saturday, Sunday    =>  day 1:  weekday 5, 6, 7
   *   Jan 2:  Saturday, Sunday            =>  day 2:  weekday 6, 7
   *   Jan 3:  Sunday                      =>  day 3:  weekday 7
   *
   * So we have a special case if (day - weekday) <= -4
   */
  if (month == 1 && (day - weekday) <= -4)
    return year - 1;

  /* December 29, 30, 31 might be in the next year if they occur before
   * Thursday.
   *
   *   Dec 31: Monday, Tuesday, Wednesday  =>  day 31: weekday 1, 2, 3
   *   Dec 30: Monday, Tuesday             =>  day 30: weekday 1, 2
   *   Dec 29: Monday                      =>  day 29: weekday 1
   *
   * So we have a special case if (day - weekday) >= 28
   */
  else if (month == 12 && (day - weekday) >= 28)
    return year + 1;

  else
    return year;
}

/**
 * xdate_time_get_week_of_year:
 * @datetime: a #xdatetime_t
 *
 * Returns the ISO 8601 week number for the week containing @datetime.
 * The ISO 8601 week number is the same for every day of the week (from
 * Moday through Sunday).  That can produce some unusual results
 * (described below).
 *
 * The first week of the year is week 1.  This is the week that contains
 * the first Thursday of the year.  Equivalently, this is the first week
 * that has more than 4 of its days falling within the calendar year.
 *
 * The value 0 is never returned by this function.  Days contained
 * within a year but occurring before the first ISO 8601 week of that
 * year are considered as being contained in the last week of the
 * previous year.  Similarly, the final days of a calendar year may be
 * considered as being part of the first ISO 8601 week of the next year
 * if 4 or more days of that week are contained within the new year.
 *
 * Returns: the ISO 8601 week number for @datetime.
 *
 * Since: 2.26
 */
xint_t
xdate_time_get_week_of_year (xdatetime_t *datetime)
{
  xint_t weeknum;

  xreturn_val_if_fail (datetime != NULL, 0);

  xdate_time_get_week_number (datetime, &weeknum, NULL, NULL);

  return weeknum;
}

/**
 * xdate_time_get_day_of_week:
 * @datetime: a #xdatetime_t
 *
 * Retrieves the ISO 8601 day of the week on which @datetime falls (1 is
 * Monday, 2 is Tuesday... 7 is Sunday).
 *
 * Returns: the day of the week
 *
 * Since: 2.26
 */
xint_t
xdate_time_get_day_of_week (xdatetime_t *datetime)
{
  xreturn_val_if_fail (datetime != NULL, 0);

  return (datetime->days - 1) % 7 + 1;
}

/* Day of year getter {{{1 */
/**
 * xdate_time_get_day_of_year:
 * @datetime: a #xdatetime_t
 *
 * Retrieves the day of the year represented by @datetime in the Gregorian
 * calendar.
 *
 * Returns: the day of the year
 *
 * Since: 2.26
 */
xint_t
xdate_time_get_day_of_year (xdatetime_t *datetime)
{
  xint_t doy = 0;

  xreturn_val_if_fail (datetime != NULL, 0);

  xdate_time_get_week_number (datetime, NULL, NULL, &doy);
  return doy;
}

/* Time component getters {{{1 */

/**
 * xdate_time_get_hour:
 * @datetime: a #xdatetime_t
 *
 * Retrieves the hour of the day represented by @datetime
 *
 * Returns: the hour of the day
 *
 * Since: 2.26
 */
xint_t
xdate_time_get_hour (xdatetime_t *datetime)
{
  xreturn_val_if_fail (datetime != NULL, 0);

  return (datetime->usec / USEC_PER_HOUR);
}

/**
 * xdate_time_get_minute:
 * @datetime: a #xdatetime_t
 *
 * Retrieves the minute of the hour represented by @datetime
 *
 * Returns: the minute of the hour
 *
 * Since: 2.26
 */
xint_t
xdate_time_get_minute (xdatetime_t *datetime)
{
  xreturn_val_if_fail (datetime != NULL, 0);

  return (datetime->usec % USEC_PER_HOUR) / USEC_PER_MINUTE;
}

/**
 * xdate_time_get_second:
 * @datetime: a #xdatetime_t
 *
 * Retrieves the second of the minute represented by @datetime
 *
 * Returns: the second represented by @datetime
 *
 * Since: 2.26
 */
xint_t
xdate_time_get_second (xdatetime_t *datetime)
{
  xreturn_val_if_fail (datetime != NULL, 0);

  return (datetime->usec % USEC_PER_MINUTE) / USEC_PER_SECOND;
}

/**
 * xdate_time_get_microsecond:
 * @datetime: a #xdatetime_t
 *
 * Retrieves the microsecond of the date represented by @datetime
 *
 * Returns: the microsecond of the second
 *
 * Since: 2.26
 */
xint_t
xdate_time_get_microsecond (xdatetime_t *datetime)
{
  xreturn_val_if_fail (datetime != NULL, 0);

  return (datetime->usec % USEC_PER_SECOND);
}

/**
 * xdate_time_get_seconds:
 * @datetime: a #xdatetime_t
 *
 * Retrieves the number of seconds since the start of the last minute,
 * including the fractional part.
 *
 * Returns: the number of seconds
 *
 * Since: 2.26
 **/
xdouble_t
xdate_time_get_seconds (xdatetime_t *datetime)
{
  xreturn_val_if_fail (datetime != NULL, 0);

  return (datetime->usec % USEC_PER_MINUTE) / 1000000.0;
}

/* Exporters {{{1 */
/**
 * xdate_time_to_unix:
 * @datetime: a #xdatetime_t
 *
 * Gives the Unix time corresponding to @datetime, rounding down to the
 * nearest second.
 *
 * Unix time is the number of seconds that have elapsed since 1970-01-01
 * 00:00:00 UTC, regardless of the time zone associated with @datetime.
 *
 * Returns: the Unix time corresponding to @datetime
 *
 * Since: 2.26
 **/
sint64_t
xdate_time_to_unix (xdatetime_t *datetime)
{
  xreturn_val_if_fail (datetime != NULL, 0);

  return INSTANT_TO_UNIX (xdate_time_to_instant (datetime));
}

/**
 * xdate_time_to_timeval:
 * @datetime: a #xdatetime_t
 * @tv: a #GTimeVal to modify
 *
 * Stores the instant in time that @datetime represents into @tv.
 *
 * The time contained in a #GTimeVal is always stored in the form of
 * seconds elapsed since 1970-01-01 00:00:00 UTC, regardless of the time
 * zone associated with @datetime.
 *
 * On systems where 'long' is 32bit (ie: all 32bit systems and all
 * Windows systems), a #GTimeVal is incapable of storing the entire
 * range of values that #xdatetime_t is capable of expressing.  On those
 * systems, this function returns %FALSE to indicate that the time is
 * out of range.
 *
 * On systems where 'long' is 64bit, this function never fails.
 *
 * Returns: %TRUE if successful, else %FALSE
 *
 * Since: 2.26
 * Deprecated: 2.62: #GTimeVal is not year-2038-safe. Use
 *    xdate_time_to_unix() instead.
 **/
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
xboolean_t
xdate_time_to_timeval (xdatetime_t *datetime,
                        GTimeVal  *tv)
{
  xreturn_val_if_fail (datetime != NULL, FALSE);

  tv->tv_sec = INSTANT_TO_UNIX (xdate_time_to_instant (datetime));
  tv->tv_usec = datetime->usec % USEC_PER_SECOND;

  return TRUE;
}
G_GNUC_END_IGNORE_DEPRECATIONS

/* Timezone queries {{{1 */
/**
 * xdate_time_get_utc_offset:
 * @datetime: a #xdatetime_t
 *
 * Determines the offset to UTC in effect at the time and in the time
 * zone of @datetime.
 *
 * The offset is the number of microseconds that you add to UTC time to
 * arrive at local time for the time zone (ie: negative numbers for time
 * zones west of GMT, positive numbers for east).
 *
 * If @datetime represents UTC time, then the offset is always zero.
 *
 * Returns: the number of microseconds that should be added to UTC to
 *          get the local time
 *
 * Since: 2.26
 **/
GTimeSpan
xdate_time_get_utc_offset (xdatetime_t *datetime)
{
  xint_t offset;

  xreturn_val_if_fail (datetime != NULL, 0);

  offset = xtime_zone_get_offset (datetime->tz, datetime->interval);

  return (sint64_t) offset * USEC_PER_SECOND;
}

/**
 * xdate_time_get_timezone:
 * @datetime: a #xdatetime_t
 *
 * Get the time zone for this @datetime.
 *
 * Returns: (transfer none): the time zone
 * Since: 2.58
 */
xtimezone_t *
xdate_time_get_timezone (xdatetime_t *datetime)
{
  xreturn_val_if_fail (datetime != NULL, NULL);

  xassert (datetime->tz != NULL);
  return datetime->tz;
}

/**
 * xdate_time_get_timezone_abbreviation:
 * @datetime: a #xdatetime_t
 *
 * Determines the time zone abbreviation to be used at the time and in
 * the time zone of @datetime.
 *
 * For example, in Toronto this is currently "EST" during the winter
 * months and "EDT" during the summer months when daylight savings
 * time is in effect.
 *
 * Returns: (transfer none): the time zone abbreviation. The returned
 *          string is owned by the #xdatetime_t and it should not be
 *          modified or freed
 *
 * Since: 2.26
 **/
const xchar_t *
xdate_time_get_timezone_abbreviation (xdatetime_t *datetime)
{
  xreturn_val_if_fail (datetime != NULL, NULL);

  return xtime_zone_get_abbreviation (datetime->tz, datetime->interval);
}

/**
 * xdate_time_is_daylight_savings:
 * @datetime: a #xdatetime_t
 *
 * Determines if daylight savings time is in effect at the time and in
 * the time zone of @datetime.
 *
 * Returns: %TRUE if daylight savings time is in effect
 *
 * Since: 2.26
 **/
xboolean_t
xdate_time_is_daylight_savings (xdatetime_t *datetime)
{
  xreturn_val_if_fail (datetime != NULL, FALSE);

  return xtime_zone_is_dst (datetime->tz, datetime->interval);
}

/* Timezone convert {{{1 */
/**
 * xdate_time_to_timezone:
 * @datetime: a #xdatetime_t
 * @tz: the new #xtimezone_t
 *
 * Create a new #xdatetime_t corresponding to the same instant in time as
 * @datetime, but in the time zone @tz.
 *
 * This call can fail in the case that the time goes out of bounds.  For
 * example, converting 0001-01-01 00:00:00 UTC to a time zone west of
 * Greenwich will fail (due to the year 0 being out of range).
 *
 * Returns: (transfer full) (nullable): the newly created #xdatetime_t which
 *   should be freed with xdate_time_unref(), or %NULL
 *
 * Since: 2.26
 **/
xdatetime_t *
xdate_time_to_timezone (xdatetime_t *datetime,
                         xtimezone_t *tz)
{
  xreturn_val_if_fail (datetime != NULL, NULL);
  xreturn_val_if_fail (tz != NULL, NULL);

  return xdate_time_from_instant (tz, xdate_time_to_instant (datetime));
}

/**
 * xdate_time_to_local:
 * @datetime: a #xdatetime_t
 *
 * Creates a new #xdatetime_t corresponding to the same instant in time as
 * @datetime, but in the local time zone.
 *
 * This call is equivalent to calling xdate_time_to_timezone() with the
 * time zone returned by xtime_zone_new_local().
 *
 * Returns: (transfer full) (nullable): the newly created #xdatetime_t which
 *   should be freed with xdate_time_unref(), or %NULL
 *
 * Since: 2.26
 **/
xdatetime_t *
xdate_time_to_local (xdatetime_t *datetime)
{
  xdatetime_t *new;
  xtimezone_t *local;

  local = xtime_zone_new_local ();
  new = xdate_time_to_timezone (datetime, local);
  xtime_zone_unref (local);

  return new;
}

/**
 * xdate_time_to_utc:
 * @datetime: a #xdatetime_t
 *
 * Creates a new #xdatetime_t corresponding to the same instant in time as
 * @datetime, but in UTC.
 *
 * This call is equivalent to calling xdate_time_to_timezone() with the
 * time zone returned by xtime_zone_new_utc().
 *
 * Returns: (transfer full) (nullable): the newly created #xdatetime_t which
 *   should be freed with xdate_time_unref(), or %NULL
 *
 * Since: 2.26
 **/
xdatetime_t *
xdate_time_to_utc (xdatetime_t *datetime)
{
  xdatetime_t *new;
  xtimezone_t *utc;

  utc = xtime_zone_new_utc ();
  new = xdate_time_to_timezone (datetime, utc);
  xtime_zone_unref (utc);

  return new;
}

/* Format {{{1 */

static xboolean_t
format_z (xstring_t *outstr,
          xint_t     offset,
          xuint_t    colons)
{
  xint_t hours;
  xint_t minutes;
  xint_t seconds;
  xchar_t sign = offset >= 0 ? '+' : '-';

  offset = ABS (offset);
  hours = offset / 3600;
  minutes = offset / 60 % 60;
  seconds = offset % 60;

  switch (colons)
    {
    case 0:
      xstring_append_printf (outstr, "%c%02d%02d",
                              sign,
                              hours,
                              minutes);
      break;

    case 1:
      xstring_append_printf (outstr, "%c%02d:%02d",
                              sign,
                              hours,
                              minutes);
      break;

    case 2:
      xstring_append_printf (outstr, "%c%02d:%02d:%02d",
                              sign,
                              hours,
                              minutes,
                              seconds);
      break;

    case 3:
      xstring_append_printf (outstr, "%c%02d", sign, hours);

      if (minutes != 0 || seconds != 0)
        {
          xstring_append_printf (outstr, ":%02d", minutes);

          if (seconds != 0)
            xstring_append_printf (outstr, ":%02d", seconds);
        }
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

#ifdef HAVE_LANGINFO_OUTDIGIT
/* Initializes the array with UTF-8 encoded alternate digits suitable for use
 * in current locale. Returns NULL when current locale does not use alternate
 * digits or there was an error converting them to UTF-8.
 */
static const xchar_t * const *
initialize_alt_digits (void)
{
  xuint_t i;
  xsize_t digit_len;
  xchar_t *digit;
  const xchar_t *locale_digit;
#define N_DIGITS 10
#define MAX_UTF8_ENCODING_LEN 4
  static xchar_t buffer[N_DIGITS * (MAX_UTF8_ENCODING_LEN + 1 /* null separator */)];
#undef N_DIGITS
#undef MAX_UTF8_ENCODING_LEN
  xchar_t *buffer_end = buffer;
  static const xchar_t *alt_digits[10];

  for (i = 0; i != 10; ++i)
    {
      locale_digit = nl_langinfo (_NL_CTYPE_OUTDIGIT0_MB + i);

      if (xstrcmp0 (locale_digit, "") == 0)
        return NULL;

      digit = _g_ctype_locale_to_utf8 (locale_digit, -1, NULL, &digit_len, NULL);
      if (digit == NULL)
        return NULL;

      xassert (digit_len < (xsize_t) (buffer + sizeof (buffer) - buffer_end));

      alt_digits[i] = buffer_end;
      buffer_end = g_stpcpy (buffer_end, digit);
      /* skip trailing null byte */
      buffer_end += 1;

      g_free (digit);
    }

  return alt_digits;
}
#endif /* HAVE_LANGINFO_OUTDIGIT */

static void
format_number (xstring_t     *str,
               xboolean_t     use_alt_digits,
               const xchar_t *pad,
               xint_t         width,
               xuint32_t      number)
{
  const xchar_t *ascii_digits[10] = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
  };
  const xchar_t * const *digits = ascii_digits;
  const xchar_t *tmp[10];
  xint_t i = 0;

  g_return_if_fail (width <= 10);

#ifdef HAVE_LANGINFO_OUTDIGIT
  if (use_alt_digits)
    {
      static const xchar_t * const *alt_digits = NULL;
      static xsize_t initialised;

      if G_UNLIKELY (g_once_init_enter (&initialised))
        {
          alt_digits = initialize_alt_digits ();

          if (alt_digits == NULL)
            alt_digits = ascii_digits;

          g_once_init_leave (&initialised, TRUE);
        }

      digits = alt_digits;
    }
#endif /* HAVE_LANGINFO_OUTDIGIT */

  do
    {
      tmp[i++] = digits[number % 10];
      number /= 10;
    }
  while (number);

  while (pad && i < width)
    tmp[i++] = *pad == '0' ? digits[0] : pad;

  /* should really be impossible */
  xassert (i <= 10);

  while (i)
    xstring_append (str, tmp[--i]);
}

static xboolean_t
format_ampm (xdatetime_t *datetime,
             xstring_t   *outstr,
             xboolean_t   locale_is_utf8,
             xboolean_t   uppercase)
{
  const xchar_t *ampm;
  xchar_t       *tmp = NULL, *ampm_dup;

  ampm = GET_AMPM (datetime);

  if (!ampm || ampm[0] == '\0')
    ampm = get_fallback_ampm (xdate_time_get_hour (datetime));

  if (!locale_is_utf8 && GET_AMPM_IS_LOCALE)
    {
      /* This assumes that locale encoding can't have embedded NULs */
      ampm = tmp = g_locale_to_utf8 (ampm, -1, NULL, NULL, NULL);
      if (tmp == NULL)
        return FALSE;
    }
  if (uppercase)
    ampm_dup = xutf8_strup (ampm, -1);
  else
    ampm_dup = xutf8_strdown (ampm, -1);
  g_free (tmp);

  xstring_append (outstr, ampm_dup);
  g_free (ampm_dup);

  return TRUE;
}

static xboolean_t xdate_time_format_utf8 (xdatetime_t   *datetime,
					 const xchar_t *format,
					 xstring_t     *outstr,
					 xboolean_t     locale_is_utf8);

/* xdate_time_format() subroutine that takes a locale-encoded format
 * string and produces a UTF-8 encoded date/time string.
 */
static xboolean_t
xdate_time_format_locale (xdatetime_t   *datetime,
			   const xchar_t *locale_format,
			   xstring_t     *outstr,
			   xboolean_t     locale_is_utf8)
{
  xchar_t *utf8_format;
  xboolean_t success;

  if (locale_is_utf8)
    return xdate_time_format_utf8 (datetime, locale_format, outstr, locale_is_utf8);

  utf8_format = _g_time_locale_to_utf8 (locale_format, -1, NULL, NULL, NULL);
  if (utf8_format == NULL)
    return FALSE;

  success = xdate_time_format_utf8 (datetime, utf8_format, outstr,
                                     locale_is_utf8);
  g_free (utf8_format);
  return success;
}

static inline xboolean_t
string_append (xstring_t     *string,
               const xchar_t *s,
               xboolean_t     s_is_utf8)
{
  xchar_t *utf8;
  xsize_t  utf8_len;

  if (s_is_utf8)
    {
      xstring_append (string, s);
    }
  else
    {
      utf8 = _g_time_locale_to_utf8 (s, -1, NULL, &utf8_len, NULL);
      if (utf8 == NULL)
        return FALSE;
      xstring_append_len (string, utf8, utf8_len);
      g_free (utf8);
    }

  return TRUE;
}

/* xdate_time_format() subroutine that takes a UTF-8 encoded format
 * string and produces a UTF-8 encoded date/time string.
 */
static xboolean_t
xdate_time_format_utf8 (xdatetime_t   *datetime,
			 const xchar_t *utf8_format,
			 xstring_t     *outstr,
			 xboolean_t     locale_is_utf8)
{
  xuint_t     len;
  xuint_t     colons;
  xunichar_t  c;
  xboolean_t  alt_digits = FALSE;
  xboolean_t  pad_set = FALSE;
  xboolean_t  name_is_utf8;
  const xchar_t *pad = "";
  const xchar_t *name;
  const xchar_t *tz;

  while (*utf8_format)
    {
      len = strcspn (utf8_format, "%");
      if (len)
        xstring_append_len (outstr, utf8_format, len);

      utf8_format += len;
      if (!*utf8_format)
	break;

      xassert (*utf8_format == '%');
      utf8_format++;
      if (!*utf8_format)
	break;

      colons = 0;
      alt_digits = FALSE;
      pad_set = FALSE;

    next_mod:
      c = xutf8_get_char (utf8_format);
      utf8_format = xutf8_next_char (utf8_format);
      switch (c)
	{
	case 'a':
	  name = WEEKDAY_ABBR (datetime);
          if (xstrcmp0 (name, "") == 0)
            return FALSE;

          name_is_utf8 = locale_is_utf8 || !WEEKDAY_ABBR_IS_LOCALE;

          if (!string_append (outstr, name, name_is_utf8))
            return FALSE;

	  break;
	case 'A':
	  name = WEEKDAY_FULL (datetime);
          if (xstrcmp0 (name, "") == 0)
            return FALSE;

          name_is_utf8 = locale_is_utf8 || !WEEKDAY_FULL_IS_LOCALE;

          if (!string_append (outstr, name, name_is_utf8))
            return FALSE;

	  break;
	case 'b':
	  name = alt_digits ? MONTH_ABBR_STANDALONE (datetime)
			    : MONTH_ABBR_WITH_DAY (datetime);
          if (xstrcmp0 (name, "") == 0)
            return FALSE;

          name_is_utf8 = locale_is_utf8 ||
            ((alt_digits && !MONTH_ABBR_STANDALONE_IS_LOCALE) ||
             (!alt_digits && !MONTH_ABBR_WITH_DAY_IS_LOCALE));

          if (!string_append (outstr, name, name_is_utf8))
            return FALSE;

	  break;
	case 'B':
	  name = alt_digits ? MONTH_FULL_STANDALONE (datetime)
			    : MONTH_FULL_WITH_DAY (datetime);
          if (xstrcmp0 (name, "") == 0)
            return FALSE;

          name_is_utf8 = locale_is_utf8 ||
            ((alt_digits && !MONTH_FULL_STANDALONE_IS_LOCALE) ||
             (!alt_digits && !MONTH_FULL_WITH_DAY_IS_LOCALE));

          if (!string_append (outstr, name, name_is_utf8))
              return FALSE;

	  break;
	case 'c':
	  {
            if (xstrcmp0 (PREFERRED_DATE_TIME_FMT, "") == 0)
              return FALSE;
            if (!xdate_time_format_locale (datetime, PREFERRED_DATE_TIME_FMT,
                                            outstr, locale_is_utf8))
              return FALSE;
	  }
	  break;
	case 'C':
	  format_number (outstr, alt_digits, pad_set ? pad : "0", 2,
			 xdate_time_get_year (datetime) / 100);
	  break;
	case 'd':
	  format_number (outstr, alt_digits, pad_set ? pad : "0", 2,
			 xdate_time_get_day_of_month (datetime));
	  break;
	case 'e':
	  format_number (outstr, alt_digits, pad_set ? pad : " ", 2,
			 xdate_time_get_day_of_month (datetime));
	  break;
	case 'f':
	  xstring_append_printf (outstr, "%06" G_GUINT64_FORMAT,
			datetime->usec % G_TIME_SPAN_SECOND);
	  break;
	case 'F':
	  xstring_append_printf (outstr, "%d-%02d-%02d",
				  xdate_time_get_year (datetime),
				  xdate_time_get_month (datetime),
				  xdate_time_get_day_of_month (datetime));
	  break;
	case 'g':
	  format_number (outstr, alt_digits, pad_set ? pad : "0", 2,
			 xdate_time_get_week_numbering_year (datetime) % 100);
	  break;
	case 'G':
	  format_number (outstr, alt_digits, pad_set ? pad : 0, 0,
			 xdate_time_get_week_numbering_year (datetime));
	  break;
	case 'h':
	  name = alt_digits ? MONTH_ABBR_STANDALONE (datetime)
			    : MONTH_ABBR_WITH_DAY (datetime);
          if (xstrcmp0 (name, "") == 0)
            return FALSE;

          name_is_utf8 = locale_is_utf8 ||
            ((alt_digits && !MONTH_ABBR_STANDALONE_IS_LOCALE) ||
             (!alt_digits && !MONTH_ABBR_WITH_DAY_IS_LOCALE));

          if (!string_append (outstr, name, name_is_utf8))
            return FALSE;

	  break;
	case 'H':
	  format_number (outstr, alt_digits, pad_set ? pad : "0", 2,
			 xdate_time_get_hour (datetime));
	  break;
	case 'I':
	  format_number (outstr, alt_digits, pad_set ? pad : "0", 2,
			 (xdate_time_get_hour (datetime) + 11) % 12 + 1);
	  break;
	case 'j':
	  format_number (outstr, alt_digits, pad_set ? pad : "0", 3,
			 xdate_time_get_day_of_year (datetime));
	  break;
	case 'k':
	  format_number (outstr, alt_digits, pad_set ? pad : " ", 2,
			 xdate_time_get_hour (datetime));
	  break;
	case 'l':
	  format_number (outstr, alt_digits, pad_set ? pad : " ", 2,
			 (xdate_time_get_hour (datetime) + 11) % 12 + 1);
	  break;
	case 'm':
	  format_number (outstr, alt_digits, pad_set ? pad : "0", 2,
			 xdate_time_get_month (datetime));
	  break;
	case 'M':
	  format_number (outstr, alt_digits, pad_set ? pad : "0", 2,
			 xdate_time_get_minute (datetime));
	  break;
	case 'n':
	  xstring_append_c (outstr, '\n');
	  break;
	case 'O':
	  alt_digits = TRUE;
	  goto next_mod;
	case 'p':
          if (!format_ampm (datetime, outstr, locale_is_utf8, TRUE))
            return FALSE;
          break;
	case 'P':
          if (!format_ampm (datetime, outstr, locale_is_utf8, FALSE))
            return FALSE;
	  break;
	case 'r':
	  {
            if (xstrcmp0 (PREFERRED_12HR_TIME_FMT, "") == 0)
              return FALSE;
	    if (!xdate_time_format_locale (datetime, PREFERRED_12HR_TIME_FMT,
					    outstr, locale_is_utf8))
	      return FALSE;
	  }
	  break;
	case 'R':
	  xstring_append_printf (outstr, "%02d:%02d",
				  xdate_time_get_hour (datetime),
				  xdate_time_get_minute (datetime));
	  break;
	case 's':
	  xstring_append_printf (outstr, "%" G_GINT64_FORMAT, xdate_time_to_unix (datetime));
	  break;
	case 'S':
	  format_number (outstr, alt_digits, pad_set ? pad : "0", 2,
			 xdate_time_get_second (datetime));
	  break;
	case 't':
	  xstring_append_c (outstr, '\t');
	  break;
	case 'T':
	  xstring_append_printf (outstr, "%02d:%02d:%02d",
				  xdate_time_get_hour (datetime),
				  xdate_time_get_minute (datetime),
				  xdate_time_get_second (datetime));
	  break;
	case 'u':
	  format_number (outstr, alt_digits, 0, 0,
			 xdate_time_get_day_of_week (datetime));
	  break;
	case 'V':
	  format_number (outstr, alt_digits, pad_set ? pad : "0", 2,
			 xdate_time_get_week_of_year (datetime));
	  break;
	case 'w':
	  format_number (outstr, alt_digits, 0, 0,
			 xdate_time_get_day_of_week (datetime) % 7);
	  break;
	case 'x':
	  {
            if (xstrcmp0 (PREFERRED_DATE_FMT, "") == 0)
              return FALSE;
	    if (!xdate_time_format_locale (datetime, PREFERRED_DATE_FMT,
					    outstr, locale_is_utf8))
	      return FALSE;
	  }
	  break;
	case 'X':
	  {
            if (xstrcmp0 (PREFERRED_TIME_FMT, "") == 0)
              return FALSE;
	    if (!xdate_time_format_locale (datetime, PREFERRED_TIME_FMT,
					    outstr, locale_is_utf8))
	      return FALSE;
	  }
	  break;
	case 'y':
	  format_number (outstr, alt_digits, pad_set ? pad : "0", 2,
			 xdate_time_get_year (datetime) % 100);
	  break;
	case 'Y':
	  format_number (outstr, alt_digits, 0, 0,
			 xdate_time_get_year (datetime));
	  break;
	case 'z':
	  {
	    sint64_t offset;
	    offset = xdate_time_get_utc_offset (datetime) / USEC_PER_SECOND;
	    if (!format_z (outstr, (int) offset, colons))
	      return FALSE;
	  }
	  break;
	case 'Z':
	  tz = xdate_time_get_timezone_abbreviation (datetime);
          xstring_append (outstr, tz);
	  break;
	case '%':
	  xstring_append_c (outstr, '%');
	  break;
	case '-':
	  pad_set = TRUE;
	  pad = "";
	  goto next_mod;
	case '_':
	  pad_set = TRUE;
	  pad = " ";
	  goto next_mod;
	case '0':
	  pad_set = TRUE;
	  pad = "0";
	  goto next_mod;
	case ':':
	  /* Colons are only allowed before 'z' */
	  if (*utf8_format && *utf8_format != 'z' && *utf8_format != ':')
	    return FALSE;
	  colons++;
	  goto next_mod;
	default:
	  return FALSE;
	}
    }

  return TRUE;
}

/**
 * xdate_time_format:
 * @datetime: A #xdatetime_t
 * @format: a valid UTF-8 string, containing the format for the
 *          #xdatetime_t
 *
 * Creates a newly allocated string representing the requested @format.
 *
 * The format strings understood by this function are a subset of the
 * strftime() format language as specified by C99.  The \%D, \%U and \%W
 * conversions are not supported, nor is the 'E' modifier.  The GNU
 * extensions \%k, \%l, \%s and \%P are supported, however, as are the
 * '0', '_' and '-' modifiers. The Python extension \%f is also supported.
 *
 * In contrast to strftime(), this function always produces a UTF-8
 * string, regardless of the current locale.  Note that the rendering of
 * many formats is locale-dependent and may not match the strftime()
 * output exactly.
 *
 * The following format specifiers are supported:
 *
 * - \%a: the abbreviated weekday name according to the current locale
 * - \%A: the full weekday name according to the current locale
 * - \%b: the abbreviated month name according to the current locale
 * - \%B: the full month name according to the current locale
 * - \%c: the preferred date and time representation for the current locale
 * - \%C: the century number (year/100) as a 2-digit integer (00-99)
 * - \%d: the day of the month as a decimal number (range 01 to 31)
 * - \%e: the day of the month as a decimal number (range  1 to 31)
 * - \%F: equivalent to `%Y-%m-%d` (the ISO 8601 date format)
 * - \%g: the last two digits of the ISO 8601 week-based year as a
 *   decimal number (00-99). This works well with \%V and \%u.
 * - \%G: the ISO 8601 week-based year as a decimal number. This works
 *   well with \%V and \%u.
 * - \%h: equivalent to \%b
 * - \%H: the hour as a decimal number using a 24-hour clock (range 00 to 23)
 * - \%I: the hour as a decimal number using a 12-hour clock (range 01 to 12)
 * - \%j: the day of the year as a decimal number (range 001 to 366)
 * - \%k: the hour (24-hour clock) as a decimal number (range 0 to 23);
 *   single digits are preceded by a blank
 * - \%l: the hour (12-hour clock) as a decimal number (range 1 to 12);
 *   single digits are preceded by a blank
 * - \%m: the month as a decimal number (range 01 to 12)
 * - \%M: the minute as a decimal number (range 00 to 59)
 * - \%f: the microsecond as a decimal number (range 000000 to 999999)
 * - \%p: either "AM" or "PM" according to the given time value, or the
 *   corresponding  strings for the current locale.  Noon is treated as
 *   "PM" and midnight as "AM". Use of this format specifier is discouraged, as
 *   many locales have no concept of AM/PM formatting. Use \%c or \%X instead.
 * - \%P: like \%p but lowercase: "am" or "pm" or a corresponding string for
 *   the current locale. Use of this format specifier is discouraged, as
 *   many locales have no concept of AM/PM formatting. Use \%c or \%X instead.
 * - \%r: the time in a.m. or p.m. notation. Use of this format specifier is
 *   discouraged, as many locales have no concept of AM/PM formatting. Use \%c
 *   or \%X instead.
 * - \%R: the time in 24-hour notation (\%H:\%M)
 * - \%s: the number of seconds since the Epoch, that is, since 1970-01-01
 *   00:00:00 UTC
 * - \%S: the second as a decimal number (range 00 to 60)
 * - \%t: a tab character
 * - \%T: the time in 24-hour notation with seconds (\%H:\%M:\%S)
 * - \%u: the ISO 8601 standard day of the week as a decimal, range 1 to 7,
 *    Monday being 1. This works well with \%G and \%V.
 * - \%V: the ISO 8601 standard week number of the current year as a decimal
 *   number, range 01 to 53, where week 1 is the first week that has at
 *   least 4 days in the new year. See xdate_time_get_week_of_year().
 *   This works well with \%G and \%u.
 * - \%w: the day of the week as a decimal, range 0 to 6, Sunday being 0.
 *   This is not the ISO 8601 standard format -- use \%u instead.
 * - \%x: the preferred date representation for the current locale without
 *   the time
 * - \%X: the preferred time representation for the current locale without
 *   the date
 * - \%y: the year as a decimal number without the century
 * - \%Y: the year as a decimal number including the century
 * - \%z: the time zone as an offset from UTC (+hhmm)
 * - \%:z: the time zone as an offset from UTC (+hh:mm).
 *   This is a gnulib strftime() extension. Since: 2.38
 * - \%::z: the time zone as an offset from UTC (+hh:mm:ss). This is a
 *   gnulib strftime() extension. Since: 2.38
 * - \%:::z: the time zone as an offset from UTC, with : to necessary
 *   precision (e.g., -04, +05:30). This is a gnulib strftime() extension. Since: 2.38
 * - \%Z: the time zone or name or abbreviation
 * - \%\%: a literal \% character
 *
 * Some conversion specifications can be modified by preceding the
 * conversion specifier by one or more modifier characters. The
 * following modifiers are supported for many of the numeric
 * conversions:
 *
 * - O: Use alternative numeric symbols, if the current locale supports those.
 * - _: Pad a numeric result with spaces. This overrides the default padding
 *   for the specifier.
 * - -: Do not pad a numeric result. This overrides the default padding
 *   for the specifier.
 * - 0: Pad a numeric result with zeros. This overrides the default padding
 *   for the specifier.
 *
 * Additionally, when O is used with B, b, or h, it produces the alternative
 * form of a month name. The alternative form should be used when the month
 * name is used without a day number (e.g., standalone). It is required in
 * some languages (Baltic, Slavic, Greek, and more) due to their grammatical
 * rules. For other languages there is no difference. \%OB is a GNU and BSD
 * strftime() extension expected to be added to the future POSIX specification,
 * \%Ob and \%Oh are GNU strftime() extensions. Since: 2.56
 *
 * Returns: (transfer full) (nullable): a newly allocated string formatted to
 *    the requested format or %NULL in the case that there was an error (such
 *    as a format specifier not being supported in the current locale). The
 *    string should be freed with g_free().
 *
 * Since: 2.26
 */
xchar_t *
xdate_time_format (xdatetime_t   *datetime,
                    const xchar_t *format)
{
  xstring_t  *outstr;
  const xchar_t *charset;
  /* Avoid conversions from locale (for LC_TIME and not for LC_MESSAGES unless
   * specified otherwise) charset to UTF-8 if charset is compatible
   * with UTF-8 already. Check for UTF-8 and synonymous canonical names of
   * ASCII. */
  xboolean_t time_is_utf8_compatible = _g_get_time_charset (&charset) ||
    xstrcmp0 ("ASCII", charset) == 0 ||
    xstrcmp0 ("ANSI_X3.4-1968", charset) == 0;

  xreturn_val_if_fail (datetime != NULL, NULL);
  xreturn_val_if_fail (format != NULL, NULL);
  xreturn_val_if_fail (xutf8_validate (format, -1, NULL), NULL);

  outstr = xstring_sized_new (strlen (format) * 2);

  if (!xdate_time_format_utf8 (datetime, format, outstr,
                                time_is_utf8_compatible))
    {
      xstring_free (outstr, TRUE);
      return NULL;
    }

  return xstring_free (outstr, FALSE);
}

/**
 * xdate_time_format_iso8601:
 * @datetime: A #xdatetime_t
 *
 * Format @datetime in [ISO 8601 format](https://en.wikipedia.org/wiki/ISO_8601),
 * including the date, time and time zone, and return that as a UTF-8 encoded
 * string.
 *
 * Since GLib 2.66, this will output to sub-second precision if needed.
 *
 * Returns: (transfer full) (nullable): a newly allocated string formatted in
 *   ISO 8601 format or %NULL in the case that there was an error. The string
 *   should be freed with g_free().
 *
 * Since: 2.62
 */
xchar_t *
xdate_time_format_iso8601 (xdatetime_t *datetime)
{
  xstring_t *outstr = NULL;
  xchar_t *main_date = NULL;
  sint64_t offset;
  xchar_t *format = "%Y-%m-%dT%H:%M:%S";

  /* if datetime has sub-second non-zero values below the second precision we
   * should print them as well */
  if (datetime->usec % G_TIME_SPAN_SECOND != 0)
    format = "%Y-%m-%dT%H:%M:%S.%f";

  /* Main date and time. */
  main_date = xdate_time_format (datetime, format);
  outstr = xstring_new (main_date);
  g_free (main_date);

  /* Timezone. Format it as `%:::z` unless the offset is zero, in which case
   * we can simply use `Z`. */
  offset = xdate_time_get_utc_offset (datetime);

  if (offset == 0)
    {
      xstring_append_c (outstr, 'Z');
    }
  else
    {
      xchar_t *time_zone = xdate_time_format (datetime, "%:::z");
      xstring_append (outstr, time_zone);
      g_free (time_zone);
    }

  return xstring_free (outstr, FALSE);
}


/* Epilogue {{{1 */
/* vim:set foldmethod=marker: */
