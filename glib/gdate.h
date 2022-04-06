/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_DATE_H__
#define __G_DATE_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <time.h>

#include <glib/gtypes.h>
#include <glib/gquark.h>

G_BEGIN_DECLS

/* xdate_t
 *
 * Date calculations (not time for now, to be resolved). These are a
 * mutant combination of Steffen Beyer's DateCalc routines
 * (http://www.perl.com/CPAN/authors/id/STBEY/) and Jon Trowbridge's
 * date routines (written for in-house software).  Written by Havoc
 * Pennington <hp@pobox.com>
 */

typedef gint32  GTime XPL_DEPRECATED_TYPE_IN_2_62_FOR(xdatetime_t);
typedef xuint16_t GDateYear;
typedef xuint8_t  GDateDay;   /* day of the month */
typedef struct _GDate xdate_t;

/* enum used to specify order of appearance in parsed date strings */
typedef enum
{
  G_DATE_DAY   = 0,
  G_DATE_MONTH = 1,
  G_DATE_YEAR  = 2
} GDateDMY;

/* actual week and month values */
typedef enum
{
  G_DATE_BAD_WEEKDAY  = 0,
  G_DATE_MONDAY       = 1,
  G_DATE_TUESDAY      = 2,
  G_DATE_WEDNESDAY    = 3,
  G_DATE_THURSDAY     = 4,
  G_DATE_FRIDAY       = 5,
  G_DATE_SATURDAY     = 6,
  G_DATE_SUNDAY       = 7
} GDateWeekday;
typedef enum
{
  G_DATE_BAD_MONTH = 0,
  G_DATE_JANUARY   = 1,
  G_DATE_FEBRUARY  = 2,
  G_DATE_MARCH     = 3,
  G_DATE_APRIL     = 4,
  G_DATE_MAY       = 5,
  G_DATE_JUNE      = 6,
  G_DATE_JULY      = 7,
  G_DATE_AUGUST    = 8,
  G_DATE_SEPTEMBER = 9,
  G_DATE_OCTOBER   = 10,
  G_DATE_NOVEMBER  = 11,
  G_DATE_DECEMBER  = 12
} GDateMonth;

#define G_DATE_BAD_JULIAN 0U
#define G_DATE_BAD_DAY    0U
#define G_DATE_BAD_YEAR   0U

/* Note: directly manipulating structs is generally a bad idea, but
 * in this case it's an *incredibly* bad idea, because all or part
 * of this struct can be invalid at any given time. Use the functions,
 * or you will get hosed, I promise.
 */
struct _GDate
{
  xuint_t julian_days : 32; /* julian days representation - we use a
                           *  bitfield hoping that 64 bit platforms
                           *  will pack this whole struct in one big
                           *  int
                           */

  xuint_t julian : 1;    /* julian is valid */
  xuint_t dmy    : 1;    /* dmy is valid */

  /* DMY representation */
  xuint_t day    : 6;
  xuint_t month  : 4;
  xuint_t year   : 16;
};

/* xdate_new() returns an invalid date, you then have to _set() stuff
 * to get a usable object. You can also allocate a xdate_t statically,
 * then call xdate_clear() to initialize.
 */
XPL_AVAILABLE_IN_ALL
xdate_t*       xdate_new                   (void);
XPL_AVAILABLE_IN_ALL
xdate_t*       xdate_new_dmy               (GDateDay     day,
                                           GDateMonth   month,
                                           GDateYear    year);
XPL_AVAILABLE_IN_ALL
xdate_t*       xdate_new_julian            (xuint32_t      julian_day);
XPL_AVAILABLE_IN_ALL
void         xdate_free                  (xdate_t       *date);
XPL_AVAILABLE_IN_2_56
xdate_t*       xdate_copy                  (const xdate_t *date);

/* check xdate_valid() after doing an operation that might fail, like
 * _parse.  Almost all xdate operations are undefined on invalid
 * dates (the exceptions are the mutators, since you need those to
 * return to validity).
 */
XPL_AVAILABLE_IN_ALL
xboolean_t     xdate_valid                 (const xdate_t *date);
XPL_AVAILABLE_IN_ALL
xboolean_t     xdate_valid_day             (GDateDay     day) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t     xdate_valid_month           (GDateMonth month) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t     xdate_valid_year            (GDateYear  year) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t     xdate_valid_weekday         (GDateWeekday weekday) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t     xdate_valid_julian          (xuint32_t julian_date) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t     xdate_valid_dmy             (GDateDay     day,
                                           GDateMonth   month,
                                           GDateYear    year) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
GDateWeekday xdate_get_weekday           (const xdate_t *date);
XPL_AVAILABLE_IN_ALL
GDateMonth   xdate_get_month             (const xdate_t *date);
XPL_AVAILABLE_IN_ALL
GDateYear    xdate_get_year              (const xdate_t *date);
XPL_AVAILABLE_IN_ALL
GDateDay     xdate_get_day               (const xdate_t *date);
XPL_AVAILABLE_IN_ALL
xuint32_t      xdate_get_julian            (const xdate_t *date);
XPL_AVAILABLE_IN_ALL
xuint_t        xdate_get_day_of_year       (const xdate_t *date);
/* First monday/sunday is the start of week 1; if we haven't reached
 * that day, return 0. These are not ISO weeks of the year; that
 * routine needs to be added.
 * these functions return the number of weeks, starting on the
 * corrsponding day
 */
XPL_AVAILABLE_IN_ALL
xuint_t        xdate_get_monday_week_of_year (const xdate_t *date);
XPL_AVAILABLE_IN_ALL
xuint_t        xdate_get_sunday_week_of_year (const xdate_t *date);
XPL_AVAILABLE_IN_ALL
xuint_t        xdate_get_iso8601_week_of_year (const xdate_t *date);

/* If you create a static date struct you need to clear it to get it
 * in a safe state before use. You can clear a whole array at
 * once with the ndates argument.
 */
XPL_AVAILABLE_IN_ALL
void         xdate_clear                 (xdate_t       *date,
                                           xuint_t        n_dates);

/* The parse routine is meant for dates typed in by a user, so it
 * permits many formats but tries to catch common typos. If your data
 * needs to be strictly validated, it is not an appropriate function.
 */
XPL_AVAILABLE_IN_ALL
void         xdate_set_parse             (xdate_t       *date,
                                           const xchar_t *str);
XPL_AVAILABLE_IN_ALL
void         xdate_set_time_t            (xdate_t       *date,
					   time_t       timet);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_DEPRECATED_IN_2_62_FOR(xdate_set_time_t)
void         xdate_set_time_val          (xdate_t       *date,
					   GTimeVal    *timeval);
XPL_DEPRECATED_FOR(xdate_set_time_t)
void         xdate_set_time              (xdate_t       *date,
                                           GTime        time_);
G_GNUC_END_IGNORE_DEPRECATIONS
XPL_AVAILABLE_IN_ALL
void         xdate_set_month             (xdate_t       *date,
                                           GDateMonth   month);
XPL_AVAILABLE_IN_ALL
void         xdate_set_day               (xdate_t       *date,
                                           GDateDay     day);
XPL_AVAILABLE_IN_ALL
void         xdate_set_year              (xdate_t       *date,
                                           GDateYear    year);
XPL_AVAILABLE_IN_ALL
void         xdate_set_dmy               (xdate_t       *date,
                                           GDateDay     day,
                                           GDateMonth   month,
                                           GDateYear    y);
XPL_AVAILABLE_IN_ALL
void         xdate_set_julian            (xdate_t       *date,
                                           xuint32_t      julian_date);
XPL_AVAILABLE_IN_ALL
xboolean_t     xdate_is_first_of_month     (const xdate_t *date);
XPL_AVAILABLE_IN_ALL
xboolean_t     xdate_is_last_of_month      (const xdate_t *date);

/* To go forward by some number of weeks just go forward weeks*7 days */
XPL_AVAILABLE_IN_ALL
void         xdate_add_days              (xdate_t       *date,
                                           xuint_t        n_days);
XPL_AVAILABLE_IN_ALL
void         xdate_subtract_days         (xdate_t       *date,
                                           xuint_t        n_days);

/* If you add/sub months while day > 28, the day might change */
XPL_AVAILABLE_IN_ALL
void         xdate_add_months            (xdate_t       *date,
                                           xuint_t        n_months);
XPL_AVAILABLE_IN_ALL
void         xdate_subtract_months       (xdate_t       *date,
                                           xuint_t        n_months);

/* If it's feb 29, changing years can move you to the 28th */
XPL_AVAILABLE_IN_ALL
void         xdate_add_years             (xdate_t       *date,
                                           xuint_t        n_years);
XPL_AVAILABLE_IN_ALL
void         xdate_subtract_years        (xdate_t       *date,
                                           xuint_t        n_years);
XPL_AVAILABLE_IN_ALL
xboolean_t     xdate_is_leap_year          (GDateYear    year) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xuint8_t       xdate_get_days_in_month     (GDateMonth   month,
                                           GDateYear    year) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xuint8_t       xdate_get_monday_weeks_in_year  (GDateYear    year) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xuint8_t       xdate_get_sunday_weeks_in_year  (GDateYear    year) G_GNUC_CONST;

/* Returns the number of days between the two dates.  If date2 comes
   before date1, a negative value is return. */
XPL_AVAILABLE_IN_ALL
xint_t         xdate_days_between          (const xdate_t *date1,
					   const xdate_t *date2);

/* qsort-friendly (with a cast...) */
XPL_AVAILABLE_IN_ALL
xint_t         xdate_compare               (const xdate_t *lhs,
                                           const xdate_t *rhs);
XPL_AVAILABLE_IN_ALL
void         xdate_to_struct_tm          (const xdate_t *date,
                                           struct tm   *tm);

XPL_AVAILABLE_IN_ALL
void         xdate_clamp                 (xdate_t *date,
					   const xdate_t *min_date,
					   const xdate_t *max_date);

/* Swap date1 and date2's values if date1 > date2. */
XPL_AVAILABLE_IN_ALL
void         xdate_order                 (xdate_t *date1, xdate_t *date2);

/* Just like strftime() except you can only use date-related formats.
 *   Using a time format is undefined.
 */
XPL_AVAILABLE_IN_ALL
xsize_t        xdate_strftime              (xchar_t       *s,
                                           xsize_t        slen,
                                           const xchar_t *format,
                                           const xdate_t *date);

#define xdate_weekday 			xdate_get_weekday XPL_DEPRECATED_MACRO_IN_2_26_FOR(xdate_get_weekday)
#define xdate_month 			xdate_get_month XPL_DEPRECATED_MACRO_IN_2_26_FOR(xdate_get_month)
#define xdate_year 			xdate_get_year XPL_DEPRECATED_MACRO_IN_2_26_FOR(xdate_get_year)
#define xdate_day 			xdate_get_day XPL_DEPRECATED_MACRO_IN_2_26_FOR(xdate_get_day)
#define xdate_julian 			xdate_get_julian XPL_DEPRECATED_MACRO_IN_2_26_FOR(xdate_get_julian)
#define xdate_day_of_year 		xdate_get_day_of_year XPL_DEPRECATED_MACRO_IN_2_26_FOR(xdate_get_day_of_year)
#define xdate_monday_week_of_year 	xdate_get_monday_week_of_year XPL_DEPRECATED_MACRO_IN_2_26_FOR(xdate_get_monday_week_of_year)
#define xdate_sunday_week_of_year 	xdate_get_sunday_week_of_year XPL_DEPRECATED_MACRO_IN_2_26_FOR(xdate_get_sunday_week_of_year)
#define xdate_days_in_month 		xdate_get_days_in_month XPL_DEPRECATED_MACRO_IN_2_26_FOR(xdate_get_days_in_month)
#define xdate_monday_weeks_in_year 	xdate_get_monday_weeks_in_year XPL_DEPRECATED_MACRO_IN_2_26_FOR(xdate_get_monday_weeks_in_year)
#define xdate_sunday_weeks_in_year	xdate_get_sunday_weeks_in_year XPL_DEPRECATED_MACRO_IN_2_26_FOR(xdate_get_sunday_weeks_in_year)

G_END_DECLS

#endif /* __G_DATE_H__ */
