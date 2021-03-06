/*
 * Copyright © 2010 Codethink Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

/* Prologue {{{1 */

#include "config.h"

#include "gtimezone.h"

#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "gmappedfile.h"
#include "gtestutils.h"
#include "gfileutils.h"
#include "gstrfuncs.h"
#include "ghash.h"
#include "gthread.h"
#include "gbytes.h"
#include "gslice.h"
#include "gdatetime.h"
#include "gdate.h"
#include "genviron.h"

#ifdef G_OS_WIN32

#define STRICT
#include <windows.h>
#include <wchar.h>
#endif

/**
 * SECTION:timezone
 * @title: xtimezone_t
 * @short_description: a structure representing a time zone
 * @see_also: #xdatetime_t
 *
 * #xtimezone_t is a structure that represents a time zone, at no
 * particular point in time.  It is refcounted and immutable.
 *
 * Each time zone has an identifier (for example, ‘Europe/London’) which is
 * platform dependent. See xtime_zone_new() for information on the identifier
 * formats. The identifier of a time zone can be retrieved using
 * xtime_zone_get_identifier().
 *
 * A time zone contains a number of intervals.  Each interval has
 * an abbreviation to describe it (for example, ‘PDT’), an offset to UTC and a
 * flag indicating if the daylight savings time is in effect during that
 * interval.  A time zone always has at least one interval — interval 0. Note
 * that interval abbreviations are not the same as time zone identifiers
 * (apart from ‘UTC’), and cannot be passed to xtime_zone_new().
 *
 * Every UTC time is contained within exactly one interval, but a given
 * local time may be contained within zero, one or two intervals (due to
 * incontinuities associated with daylight savings time).
 *
 * An interval may refer to a specific period of time (eg: the duration
 * of daylight savings time during 2010) or it may refer to many periods
 * of time that share the same properties (eg: all periods of daylight
 * savings time).  It is also possible (usually for political reasons)
 * that some properties (like the abbreviation) change between intervals
 * without other properties changing.
 *
 * #xtimezone_t is available since GLib 2.26.
 */

/**
 * xtimezone_t:
 *
 * #xtimezone_t is an opaque structure whose members cannot be accessed
 * directly.
 *
 * Since: 2.26
 **/

/* IANA zoneinfo file format {{{1 */

/* unaligned */
typedef struct { xchar_t bytes[8]; } gint64_be;
typedef struct { xchar_t bytes[4]; } gint32_be;
typedef struct { xchar_t bytes[4]; } guint32_be;

static inline sint64_t gint64_from_be (const gint64_be be) {
  sint64_t tmp; memcpy (&tmp, &be, sizeof tmp); return GINT64_FROM_BE (tmp);
}

static inline gint32 gint32_from_be (const gint32_be be) {
  gint32 tmp; memcpy (&tmp, &be, sizeof tmp); return GINT32_FROM_BE (tmp);
}

static inline xuint32_t guint32_from_be (const guint32_be be) {
  xuint32_t tmp; memcpy (&tmp, &be, sizeof tmp); return GUINT32_FROM_BE (tmp);
}

/* The layout of an IANA timezone file header */
struct tzhead
{
  xchar_t      tzh_magic[4];
  xchar_t      tzh_version;
  xuchar_t     tzh_reserved[15];

  guint32_be tzh_ttisgmtcnt;
  guint32_be tzh_ttisstdcnt;
  guint32_be tzh_leapcnt;
  guint32_be tzh_timecnt;
  guint32_be tzh_typecnt;
  guint32_be tzh_charcnt;
};

struct ttinfo
{
  gint32_be tt_gmtoff;
  xuint8_t    tt_isdst;
  xuint8_t    tt_abbrind;
};

/* A Transition Date structure for TZ Rules, an intermediate structure
   for parsing MSWindows and Environment-variable time zones. It
   Generalizes MSWindows's SYSTEMTIME struct.
 */
typedef struct
{
  xint_t     year;
  xint_t     mon;
  xint_t     mday;
  xint_t     wday;
  xint_t     week;
  gint32   offset;  /* hour*3600 + min*60 + sec; can be negative.  */
} TimeZoneDate;

/* POSIX Timezone abbreviations are typically 3 or 4 characters, but
   Microsoft uses 32-character names. We'll use one larger to ensure
   we have room for the terminating \0.
 */
#define NAME_SIZE 33

/* A MSWindows-style time zone transition rule. Generalizes the
   MSWindows TIME_ZONE_INFORMATION struct. Also used to compose time
   zones from tzset-style identifiers.
 */
typedef struct
{
  xuint_t        start_year;
  gint32       std_offset;
  gint32       dlt_offset;
  TimeZoneDate dlt_start;
  TimeZoneDate dlt_end;
  xchar_t std_name[NAME_SIZE];
  xchar_t dlt_name[NAME_SIZE];
} TimeZoneRule;

/* xtimezone_t's internal representation of a Daylight Savings (Summer)
   time interval.
 */
typedef struct
{
  gint32     gmt_offset;
  xboolean_t   is_dst;
  xchar_t     *abbrev;
} TransitionInfo;

/* xtimezone_t's representation of a transition time to or from Daylight
   Savings (Summer) time and Standard time for the zone. */
typedef struct
{
  sint64_t time;
  xint_t   info_index;
} Transition;

/* xtimezone_t structure */
struct _GTimeZone
{
  xchar_t   *name;
  xarray_t  *t_info;         /* Array of TransitionInfo */
  xarray_t  *transitions;    /* Array of Transition */
  xint_t     ref_count;
};

G_LOCK_DEFINE_STATIC (time_zones);
static xhashtable_t/*<string?, xtimezone_t>*/ *time_zones;
G_LOCK_DEFINE_STATIC (tz_default);
static xtimezone_t *tz_default = NULL;
G_LOCK_DEFINE_STATIC (tz_local);
static xtimezone_t *tz_local = NULL;

#define MIN_TZYEAR 1916 /* Daylight Savings started in WWI */
#define MAX_TZYEAR 2999 /* And it's not likely ever to go away, but
                           there's no point in getting carried
                           away. */

#ifdef G_OS_UNIX
static xtimezone_t *parse_footertz (const xchar_t *, size_t);
#endif

/**
 * xtime_zone_unref:
 * @tz: a #xtimezone_t
 *
 * Decreases the reference count on @tz.
 *
 * Since: 2.26
 **/
void
xtime_zone_unref (xtimezone_t *tz)
{
  int ref_count;

again:
  ref_count = g_atomic_int_get (&tz->ref_count);

  xassert (ref_count > 0);

  if (ref_count == 1)
    {
      if (tz->name != NULL)
        {
          G_LOCK(time_zones);

          /* someone else might have grabbed a ref in the meantime */
          if G_UNLIKELY (g_atomic_int_get (&tz->ref_count) != 1)
            {
              G_UNLOCK(time_zones);
              goto again;
            }

          if (time_zones != NULL)
            xhash_table_remove (time_zones, tz->name);
          G_UNLOCK(time_zones);
        }

      if (tz->t_info != NULL)
        {
          xuint_t idx;
          for (idx = 0; idx < tz->t_info->len; idx++)
            {
              TransitionInfo *info = &g_array_index (tz->t_info, TransitionInfo, idx);
              g_free (info->abbrev);
            }
          g_array_free (tz->t_info, TRUE);
        }
      if (tz->transitions != NULL)
        g_array_free (tz->transitions, TRUE);
      g_free (tz->name);

      g_slice_free (xtimezone_t, tz);
    }

  else if G_UNLIKELY (!g_atomic_int_compare_and_exchange (&tz->ref_count,
                                                          ref_count,
                                                          ref_count - 1))
    goto again;
}

/**
 * xtime_zone_ref:
 * @tz: a #xtimezone_t
 *
 * Increases the reference count on @tz.
 *
 * Returns: a new reference to @tz.
 *
 * Since: 2.26
 **/
xtimezone_t *
xtime_zone_ref (xtimezone_t *tz)
{
  xassert (tz->ref_count > 0);

  g_atomic_int_inc (&tz->ref_count);

  return tz;
}

/* fake zoneinfo creation (for RFC3339/ISO 8601 timezones) {{{1 */
/*
 * parses strings of the form h or hh[[:]mm[[[:]ss]]] where:
 *  - h[h] is 0 to 24
 *  - mm is 00 to 59
 *  - ss is 00 to 59
 * If RFC8536, TIME_ is a transition time sans sign,
 * so colons are required before mm and ss, and hh can be up to 167.
 * See Internet RFC 8536 section 3.3.1:
 * https://tools.ietf.org/html/rfc8536#section-3.3.1
 * and POSIX Base Definitions 8.3 TZ rule time:
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html#tag_08_03
 */
static xboolean_t
parse_time (const xchar_t *time_,
            gint32      *offset,
            xboolean_t    rfc8536)
{
  if (*time_ < '0' || '9' < *time_)
    return FALSE;

  *offset = 60 * 60 * (*time_++ - '0');

  if (*time_ == '\0')
    return TRUE;

  if (*time_ != ':')
    {
      if (*time_ < '0' || '9' < *time_)
        return FALSE;

      *offset *= 10;
      *offset += 60 * 60 * (*time_++ - '0');

      if (rfc8536)
        {
          /* Internet RFC 8536 section 3.3.1 and POSIX 8.3 TZ together say
             that a transition time must be of the form [+-]hh[:mm[:ss]] where
             the hours part can range from -167 to 167.  */
          if ('0' <= *time_ && *time_ <= '9')
            {
              *offset *= 10;
              *offset += 60 * 60 * (*time_++ - '0');
            }
          if (*offset > 167 * 60 * 60)
            return FALSE;
        }
      else if (*offset > 24 * 60 * 60)
        return FALSE;

      if (*time_ == '\0')
        return TRUE;
    }

  if (*time_ == ':')
    time_++;
  else if (rfc8536)
    return FALSE;

  if (*time_ < '0' || '5' < *time_)
    return FALSE;

  *offset += 10 * 60 * (*time_++ - '0');

  if (*time_ < '0' || '9' < *time_)
    return FALSE;

  *offset += 60 * (*time_++ - '0');

  if (*time_ == '\0')
    return TRUE;

  if (*time_ == ':')
    time_++;
  else if (rfc8536)
    return FALSE;

  if (*time_ < '0' || '5' < *time_)
    return FALSE;

  *offset += 10 * (*time_++ - '0');

  if (*time_ < '0' || '9' < *time_)
    return FALSE;

  *offset += *time_++ - '0';

  return *time_ == '\0';
}

static xboolean_t
parse_constant_offset (const xchar_t *name,
                       gint32      *offset,
                       xboolean_t    rfc8536)
{
  /* Internet RFC 8536 section 3.3.1 and POSIX 8.3 TZ together say
     that a transition time must be numeric.  */
  if (!rfc8536 && xstrcmp0 (name, "UTC") == 0)
    {
      *offset = 0;
      return TRUE;
    }

  if (*name >= '0' && '9' >= *name)
    return parse_time (name, offset, rfc8536);

  switch (*name++)
    {
    case 'Z':
      *offset = 0;
      /* Internet RFC 8536 section 3.3.1 requires a numeric zone.  */
      return !rfc8536 && !*name;

    case '+':
      return parse_time (name, offset, rfc8536);

    case '-':
      if (parse_time (name, offset, rfc8536))
        {
          *offset = -*offset;
          return TRUE;
        }
      else
        return FALSE;

    default:
      return FALSE;
    }
}

static void
zone_for_constant_offset (xtimezone_t *gtz, const xchar_t *name)
{
  gint32 offset;
  TransitionInfo info;

  if (name == NULL || !parse_constant_offset (name, &offset, FALSE))
    return;

  info.gmt_offset = offset;
  info.is_dst = FALSE;
  info.abbrev =  xstrdup (name);

  gtz->name = xstrdup (name);
  gtz->t_info = g_array_sized_new (FALSE, TRUE, sizeof (TransitionInfo), 1);
  g_array_append_val (gtz->t_info, info);

  /* Constant offset, no transitions */
  gtz->transitions = NULL;
}

#ifdef G_OS_UNIX

#if defined(__sun) && defined(__SVR4)
/*
 * only used by Illumos distros or Solaris < 11: parse the /etc/default/init
 * text file looking for TZ= followed by the timezone, possibly quoted
 *
 */
static xchar_t *
zone_identifier_illumos (void)
{
  xchar_t *resolved_identifier = NULL;
  xchar_t *contents = NULL;
  const xchar_t *line_start = NULL;
  xsize_t tz_len = 0;

  if (!xfile_get_contents ("/etc/default/init", &contents, NULL, NULL) )
    return NULL;

  /* is TZ= the first/only line in the file? */
  if (strncmp (contents, "TZ=", 3) == 0)
    {
      /* found TZ= on the first line, skip over the TZ= */
      line_start = contents + 3;
    }
  else
    {
      /* find a newline followed by TZ= */
      line_start = strstr (contents, "\nTZ=");
      if (line_start != NULL)
        line_start = line_start + 4; /* skip past the \nTZ= */
    }

  /*
   * line_start is NULL if we didn't find TZ= at the start of any line,
   * otherwise it points to what is after the '=' (possibly '\0')
   */
  if (line_start == NULL || *line_start == '\0')
    return NULL;

  /* skip past a possible opening " or ' */
  if (*line_start == '"' || *line_start == '\'')
    line_start++;

  /*
   * loop over the next few characters, building up the length of
   * the timezone identifier, ending with end of string, newline or
   * a " or ' character
   */
  while (*(line_start + tz_len) != '\0' &&
         *(line_start + tz_len) != '\n' &&
         *(line_start + tz_len) != '"'  &&
         *(line_start + tz_len) != '\'')
    tz_len++;

  if (tz_len > 0)
    {
      /* found it */
      resolved_identifier = xstrndup (line_start, tz_len);
      xstrchomp (resolved_identifier);
      g_free (contents);
      return g_steal_pointer (&resolved_identifier);
    }
  else
    return NULL;
}
#endif /* defined(__sun) && defined(__SRVR) */

/*
 * returns the path to the top of the Olson zoneinfo timezone hierarchy.
 */
static const xchar_t *
zone_info_base_dir (void)
{
  if (xfile_test ("/usr/share/zoneinfo", XFILE_TEST_IS_DIR))
    return "/usr/share/zoneinfo";     /* Most distros */
  else if (xfile_test ("/usr/share/lib/zoneinfo", XFILE_TEST_IS_DIR))
    return "/usr/share/lib/zoneinfo"; /* Illumos distros */

  /* need a better fallback case */
  return "/usr/share/zoneinfo";
}

static xchar_t *
zone_identifier_unix (void)
{
  xchar_t *resolved_identifier = NULL;
  xsize_t prefix_len = 0;
  xchar_t *canonical_path = NULL;
  xerror_t *read_link_err = NULL;
  const xchar_t *tzdir;

  /* Resolve the actual timezone pointed to by /etc/localtime. */
  resolved_identifier = xfile_read_link ("/etc/localtime", &read_link_err);
  if (resolved_identifier == NULL)
    {
      xboolean_t not_a_symlink = xerror_matches (read_link_err,
                                                XFILE_ERROR,
                                                XFILE_ERROR_INVAL);
      g_clear_error (&read_link_err);

      /* if /etc/localtime is not a symlink, try:
       *  - /var/db/zoneinfo : 'tzsetup' program on FreeBSD and
       *    DragonflyBSD stores the timezone chosen by the user there.
       *  - /etc/timezone : Gentoo, OpenRC, and others store
       *    the user choice there.
       *  - call zone_identifier_illumos iff __sun and __SVR4 are defined,
       *    as a last-ditch effort to parse the TZ= setting from within
       *    /etc/default/init
       */
      if (not_a_symlink && (xfile_get_contents ("/var/db/zoneinfo",
                                                 &resolved_identifier,
                                                 NULL, NULL) ||
                            xfile_get_contents ("/etc/timezone",
                                                 &resolved_identifier,
                                                 NULL, NULL)
#if defined(__sun) && defined(__SVR4)
                                                             ||
                            (resolved_identifier = zone_identifier_illumos ())
#endif
                                                             ))
        xstrchomp (resolved_identifier);
      else
        {
          /* Error */
          xassert (resolved_identifier == NULL);
          goto out;
        }
    }
  else
    {
      /* Resolve relative path */
      canonical_path = g_canonicalize_filename (resolved_identifier, "/etc");
      g_free (resolved_identifier);
      resolved_identifier = g_steal_pointer (&canonical_path);
    }

  tzdir = g_getenv ("TZDIR");
  if (tzdir == NULL)
    tzdir = zone_info_base_dir ();

  /* Strip the prefix and slashes if possible. */
  if (xstr_has_prefix (resolved_identifier, tzdir))
    {
      prefix_len = strlen (tzdir);
      while (*(resolved_identifier + prefix_len) == '/')
        prefix_len++;
    }

  if (prefix_len > 0)
    memmove (resolved_identifier, resolved_identifier + prefix_len,
             strlen (resolved_identifier) - prefix_len + 1  /* nul terminator */);

  xassert (resolved_identifier != NULL);

out:
  g_free (canonical_path);

  return resolved_identifier;
}

static xbytes_t*
zone_info_unix (const xchar_t *identifier,
                const xchar_t *resolved_identifier)
{
  xchar_t *filename = NULL;
  xmapped_file_t *file = NULL;
  xbytes_t *zoneinfo = NULL;
  const xchar_t *tzdir;

  tzdir = g_getenv ("TZDIR");
  if (tzdir == NULL)
    tzdir = zone_info_base_dir ();

  /* identifier can be a relative or absolute path name;
     if relative, it is interpreted starting from /usr/share/zoneinfo
     while the POSIX standard says it should start with :,
     glibc allows both syntaxes, so we should too */
  if (identifier != NULL)
    {
      if (*identifier == ':')
        identifier ++;

      if (g_path_is_absolute (identifier))
        filename = xstrdup (identifier);
      else
        filename = g_build_filename (tzdir, identifier, NULL);
    }
  else
    {
      if (resolved_identifier == NULL)
        goto out;

      filename = xstrdup ("/etc/localtime");
    }

  file = xmapped_file_new (filename, FALSE, NULL);
  if (file != NULL)
    {
      zoneinfo = xbytes_new_with_free_func (xmapped_file_get_contents (file),
                                             xmapped_file_get_length (file),
                                             (xdestroy_notify_t)xmapped_file_unref,
                                             xmapped_file_ref (file));
      xmapped_file_unref (file);
    }

  xassert (resolved_identifier != NULL);

out:
  g_free (filename);

  return zoneinfo;
}

static void
init_zone_from_iana_info (xtimezone_t *gtz,
                          xbytes_t    *zoneinfo,
                          xchar_t     *identifier  /* (transfer full) */)
{
  xsize_t size;
  xuint_t index;
  xuint32_t time_count, type_count;
  xuint8_t *tz_transitions, *tz_type_index, *tz_ttinfo;
  xuint8_t *tz_abbrs;
  xsize_t timesize = sizeof (gint32);
  xconstpointer header_data = xbytes_get_data (zoneinfo, &size);
  const xchar_t *data = header_data;
  const struct tzhead *header = header_data;
  xtimezone_t *footertz = NULL;
  xuint_t extra_time_count = 0, extra_type_count = 0;
  sint64_t last_explicit_transition_time;

  g_return_if_fail (size >= sizeof (struct tzhead) &&
                    memcmp (header, "TZif", 4) == 0);

  /* FIXME: Handle invalid TZif files better (Issue#1088).  */

  if (header->tzh_version >= '2')
      {
        /* Skip ahead to the newer 64-bit data if it's available. */
        header = (const struct tzhead *)
          (((const xchar_t *) (header + 1)) +
           guint32_from_be(header->tzh_ttisgmtcnt) +
           guint32_from_be(header->tzh_ttisstdcnt) +
           8 * guint32_from_be(header->tzh_leapcnt) +
           5 * guint32_from_be(header->tzh_timecnt) +
           6 * guint32_from_be(header->tzh_typecnt) +
           guint32_from_be(header->tzh_charcnt));
        timesize = sizeof (sint64_t);
      }
  time_count = guint32_from_be(header->tzh_timecnt);
  type_count = guint32_from_be(header->tzh_typecnt);

  if (header->tzh_version >= '2')
    {
      const xchar_t *footer = (((const xchar_t *) (header + 1))
                             + guint32_from_be(header->tzh_ttisgmtcnt)
                             + guint32_from_be(header->tzh_ttisstdcnt)
                             + 12 * guint32_from_be(header->tzh_leapcnt)
                             + 9 * time_count
                             + 6 * type_count
                             + guint32_from_be(header->tzh_charcnt));
      const xchar_t *footerlast;
      size_t footerlen;
      g_return_if_fail (footer <= data + size - 2 && footer[0] == '\n');
      footerlast = memchr (footer + 1, '\n', data + size - (footer + 1));
      g_return_if_fail (footerlast);
      footerlen = footerlast + 1 - footer;
      if (footerlen != 2)
        {
          footertz = parse_footertz (footer, footerlen);
          g_return_if_fail (footertz);
          extra_type_count = footertz->t_info->len;
          extra_time_count = footertz->transitions->len;
        }
    }

  tz_transitions = ((xuint8_t *) (header) + sizeof (*header));
  tz_type_index = tz_transitions + timesize * time_count;
  tz_ttinfo = tz_type_index + time_count;
  tz_abbrs = tz_ttinfo + sizeof (struct ttinfo) * type_count;

  gtz->name = g_steal_pointer (&identifier);
  gtz->t_info = g_array_sized_new (FALSE, TRUE, sizeof (TransitionInfo),
                                   type_count + extra_type_count);
  gtz->transitions = g_array_sized_new (FALSE, TRUE, sizeof (Transition),
                                        time_count + extra_time_count);

  for (index = 0; index < type_count; index++)
    {
      TransitionInfo t_info;
      struct ttinfo info = ((struct ttinfo*)tz_ttinfo)[index];
      t_info.gmt_offset = gint32_from_be (info.tt_gmtoff);
      t_info.is_dst = info.tt_isdst ? TRUE : FALSE;
      t_info.abbrev = xstrdup ((xchar_t *) &tz_abbrs[info.tt_abbrind]);
      g_array_append_val (gtz->t_info, t_info);
    }

  for (index = 0; index < time_count; index++)
    {
      Transition trans;
      if (header->tzh_version >= '2')
        trans.time = gint64_from_be (((gint64_be*)tz_transitions)[index]);
      else
        trans.time = gint32_from_be (((gint32_be*)tz_transitions)[index]);
      last_explicit_transition_time = trans.time;
      trans.info_index = tz_type_index[index];
      xassert (trans.info_index >= 0);
      xassert ((xuint_t) trans.info_index < gtz->t_info->len);
      g_array_append_val (gtz->transitions, trans);
    }

  if (footertz)
    {
      /* Append footer time types.  Don't bother to coalesce
         duplicates with existing time types.  */
      for (index = 0; index < extra_type_count; index++)
        {
          TransitionInfo t_info;
          TransitionInfo *footer_t_info
            = &g_array_index (footertz->t_info, TransitionInfo, index);
          t_info.gmt_offset = footer_t_info->gmt_offset;
          t_info.is_dst = footer_t_info->is_dst;
          t_info.abbrev = g_steal_pointer (&footer_t_info->abbrev);
          g_array_append_val (gtz->t_info, t_info);
        }

      /* Append footer transitions that follow the last explicit
         transition.  */
      for (index = 0; index < extra_time_count; index++)
        {
          Transition *footer_transition
            = &g_array_index (footertz->transitions, Transition, index);
          if (time_count <= 0
              || last_explicit_transition_time < footer_transition->time)
            {
              Transition trans;
              trans.time = footer_transition->time;
              trans.info_index = type_count + footer_transition->info_index;
              g_array_append_val (gtz->transitions, trans);
            }
        }

      xtime_zone_unref (footertz);
    }
}

#elif defined (G_OS_WIN32)

static void
copy_windows_systemtime (SYSTEMTIME *s_time, TimeZoneDate *tzdate)
{
  tzdate->offset
    = s_time->wHour * 3600 + s_time->wMinute * 60 + s_time->wSecond;
  tzdate->mon = s_time->wMonth;
  tzdate->year = s_time->wYear;
  tzdate->wday = s_time->wDayOfWeek ? s_time->wDayOfWeek : 7;

  if (s_time->wYear)
    {
      tzdate->mday = s_time->wDay;
      tzdate->wday = 0;
    }
  else
    tzdate->week = s_time->wDay;
}

/* UTC = local time + bias while local time = UTC + offset */
static xboolean_t
rule_from_windows_time_zone_info (TimeZoneRule *rule,
                                  TIME_ZONE_INFORMATION *tzi)
{
  xchar_t *std_name, *dlt_name;

  std_name = xutf16_to_utf8 ((xunichar2_t *)tzi->StandardName, -1, NULL, NULL, NULL);
  if (std_name == NULL)
    return FALSE;

  dlt_name = xutf16_to_utf8 ((xunichar2_t *)tzi->DaylightName, -1, NULL, NULL, NULL);
  if (dlt_name == NULL)
    {
      g_free (std_name);
      return FALSE;
    }

  /* Set offset */
  if (tzi->StandardDate.wMonth)
    {
      rule->std_offset = -(tzi->Bias + tzi->StandardBias) * 60;
      rule->dlt_offset = -(tzi->Bias + tzi->DaylightBias) * 60;
      copy_windows_systemtime (&(tzi->DaylightDate), &(rule->dlt_start));

      copy_windows_systemtime (&(tzi->StandardDate), &(rule->dlt_end));
    }

  else
    {
      rule->std_offset = -tzi->Bias * 60;
      rule->dlt_start.mon = 0;
    }
  strncpy (rule->std_name, std_name, NAME_SIZE - 1);
  strncpy (rule->dlt_name, dlt_name, NAME_SIZE - 1);

  g_free (std_name);
  g_free (dlt_name);

  return TRUE;
}

static xchar_t*
windows_default_tzname (void)
{
  const xunichar2_t *subkey =
    L"SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation";
  HKEY key;
  xchar_t *key_name = NULL;
  xunichar2_t *key_name_w = NULL;
  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE, subkey, 0,
                     KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
    {
      DWORD size = 0;
      if (RegQueryValueExW (key, L"TimeZoneKeyName", NULL, NULL,
                            NULL, &size) == ERROR_SUCCESS)
        {
          key_name_w = g_malloc ((xint_t)size);

          if (key_name_w == NULL ||
              RegQueryValueExW (key, L"TimeZoneKeyName", NULL, NULL,
                                (LPBYTE)key_name_w, &size) != ERROR_SUCCESS)
            {
              g_free (key_name_w);
              key_name = NULL;
            }
          else
            key_name = xutf16_to_utf8 (key_name_w, -1, NULL, NULL, NULL);
        }
      RegCloseKey (key);
    }
  return key_name;
}

typedef   struct
{
  LONG Bias;
  LONG StandardBias;
  LONG DaylightBias;
  SYSTEMTIME StandardDate;
  SYSTEMTIME DaylightDate;
} RegTZI;

static void
system_time_copy (SYSTEMTIME *orig, SYSTEMTIME *target)
{
  g_return_if_fail (orig != NULL);
  g_return_if_fail (target != NULL);

  target->wYear = orig->wYear;
  target->wMonth = orig->wMonth;
  target->wDayOfWeek = orig->wDayOfWeek;
  target->wDay = orig->wDay;
  target->wHour = orig->wHour;
  target->wMinute = orig->wMinute;
  target->wSecond = orig->wSecond;
  target->wMilliseconds = orig->wMilliseconds;
}

static void
register_tzi_to_tzi (RegTZI *reg, TIME_ZONE_INFORMATION *tzi)
{
  g_return_if_fail (reg != NULL);
  g_return_if_fail (tzi != NULL);
  tzi->Bias = reg->Bias;
  system_time_copy (&(reg->StandardDate), &(tzi->StandardDate));
  tzi->StandardBias = reg->StandardBias;
  system_time_copy (&(reg->DaylightDate), &(tzi->DaylightDate));
  tzi->DaylightBias = reg->DaylightBias;
}

static xuint_t
rules_from_windows_time_zone (const xchar_t   *identifier,
                              const xchar_t   *resolved_identifier,
                              TimeZoneRule **rules)
{
  HKEY key;
  xchar_t *subkey = NULL;
  xchar_t *subkey_dynamic = NULL;
  const xchar_t *key_name;
  const xchar_t *reg_key =
    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\";
  TIME_ZONE_INFORMATION tzi;
  DWORD size;
  xuint_t rules_num = 0;
  RegTZI regtzi = { 0 }, regtzi_prev;
  WCHAR winsyspath[MAX_PATH];
  xunichar2_t *subkey_w, *subkey_dynamic_w;

  subkey_dynamic_w = NULL;

  if (GetSystemDirectoryW (winsyspath, MAX_PATH) == 0)
    return 0;

  xassert (rules != NULL);

  *rules = NULL;
  key_name = NULL;

  if (!identifier)
    key_name = resolved_identifier;
  else
    key_name = identifier;

  if (!key_name)
    return 0;

  subkey = xstrconcat (reg_key, key_name, NULL);
  subkey_w = xutf8_to_utf16 (subkey, -1, NULL, NULL, NULL);
  if (subkey_w == NULL)
    goto utf16_conv_failed;

  subkey_dynamic = xstrconcat (subkey, "\\Dynamic DST", NULL);
  subkey_dynamic_w = xutf8_to_utf16 (subkey_dynamic, -1, NULL, NULL, NULL);
  if (subkey_dynamic_w == NULL)
    goto utf16_conv_failed;

  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE, subkey_w, 0,
                     KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
      goto utf16_conv_failed;

  size = sizeof tzi.StandardName;

  /* use RegLoadMUIStringW() to query MUI_Std from the registry if possible, otherwise
     fallback to querying Std */
  if (RegLoadMUIStringW (key, L"MUI_Std", tzi.StandardName,
                         size, &size, 0, winsyspath) != ERROR_SUCCESS)
    {
      size = sizeof tzi.StandardName;
      if (RegQueryValueExW (key, L"Std", NULL, NULL,
                            (LPBYTE)&(tzi.StandardName), &size) != ERROR_SUCCESS)
        goto registry_failed;
    }

  size = sizeof tzi.DaylightName;

  /* use RegLoadMUIStringW() to query MUI_Dlt from the registry if possible, otherwise
     fallback to querying Dlt */
  if (RegLoadMUIStringW (key, L"MUI_Dlt", tzi.DaylightName,
                         size, &size, 0, winsyspath) != ERROR_SUCCESS)
    {
      size = sizeof tzi.DaylightName;
      if (RegQueryValueExW (key, L"Dlt", NULL, NULL,
                            (LPBYTE)&(tzi.DaylightName), &size) != ERROR_SUCCESS)
        goto registry_failed;
    }

  RegCloseKey (key);
  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE, subkey_dynamic_w, 0,
                     KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
    {
      DWORD i, first, last, year;
      wchar_t s[12];

      size = sizeof first;
      if (RegQueryValueExW (key, L"FirstEntry", NULL, NULL,
                            (LPBYTE) &first, &size) != ERROR_SUCCESS)
        goto registry_failed;

      size = sizeof last;
      if (RegQueryValueExW (key, L"LastEntry", NULL, NULL,
                            (LPBYTE) &last, &size) != ERROR_SUCCESS)
        goto registry_failed;

      rules_num = last - first + 2;
      *rules = g_new0 (TimeZoneRule, rules_num);

      for (year = first, i = 0; *rules != NULL && year <= last; year++)
        {
          xboolean_t failed = FALSE;
          swprintf_s (s, 11, L"%d", year);

          if (!failed)
            {
              size = sizeof regtzi;
              if (RegQueryValueExW (key, s, NULL, NULL,
                                    (LPBYTE) &regtzi, &size) != ERROR_SUCCESS)
                failed = TRUE;
            }

          if (failed)
            {
              g_free (*rules);
              *rules = NULL;
              break;
            }

          if (year > first && memcmp (&regtzi_prev, &regtzi, sizeof regtzi) == 0)
              continue;
          else
            memcpy (&regtzi_prev, &regtzi, sizeof regtzi);

          register_tzi_to_tzi (&regtzi, &tzi);

          if (!rule_from_windows_time_zone_info (&(*rules)[i], &tzi))
            {
              g_free (*rules);
              *rules = NULL;
              break;
            }

          (*rules)[i++].start_year = year;
        }

      rules_num = i + 1;

registry_failed:
      RegCloseKey (key);
    }
  else if (RegOpenKeyExW (HKEY_LOCAL_MACHINE, subkey_w, 0,
                          KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
    {
      size = sizeof regtzi;
      if (RegQueryValueExW (key, L"TZI", NULL, NULL,
                            (LPBYTE) &regtzi, &size) == ERROR_SUCCESS)
        {
          rules_num = 2;
          *rules = g_new0 (TimeZoneRule, 2);
          register_tzi_to_tzi (&regtzi, &tzi);

          if (!rule_from_windows_time_zone_info (&(*rules)[0], &tzi))
            {
              g_free (*rules);
              *rules = NULL;
            }
        }

      RegCloseKey (key);
    }

utf16_conv_failed:
  g_free (subkey_dynamic_w);
  g_free (subkey_dynamic);
  g_free (subkey_w);
  g_free (subkey);

  if (*rules)
    {
      (*rules)[0].start_year = MIN_TZYEAR;
      if ((*rules)[rules_num - 2].start_year < MAX_TZYEAR)
        (*rules)[rules_num - 1].start_year = MAX_TZYEAR;
      else
        (*rules)[rules_num - 1].start_year = (*rules)[rules_num - 2].start_year + 1;

      return rules_num;
    }

  return 0;
}

#endif

static void
find_relative_date (TimeZoneDate *buffer)
{
  xuint_t wday;
  xdate_t date;
  xdate_clear (&date, 1);
  wday = buffer->wday;

  /* Get last day if last is needed, first day otherwise */
  if (buffer->mon == 13 || buffer->mon == 14) /* Julian Date */
    {
      xdate_set_dmy (&date, 1, 1, buffer->year);
      if (wday >= 59 && buffer->mon == 13 && xdate_is_leap_year (buffer->year))
        xdate_add_days (&date, wday);
      else
        xdate_add_days (&date, wday - 1);
      buffer->mon = (int) xdate_get_month (&date);
      buffer->mday = (int) xdate_get_day (&date);
      buffer->wday = 0;
    }
  else /* M.W.D */
    {
      xuint_t days;
      xuint_t days_in_month = xdate_get_days_in_month (buffer->mon, buffer->year);
      GDateWeekday first_wday;

      xdate_set_dmy (&date, 1, buffer->mon, buffer->year);
      first_wday = xdate_get_weekday (&date);

      if ((xuint_t) first_wday > wday)
        ++(buffer->week);
      /* week is 1 <= w <= 5, we need 0-based */
      days = 7 * (buffer->week - 1) + wday - first_wday;

      /* "days" is a 0-based offset from the 1st of the month.
       * Adding days == days_in_month would bring us into the next month,
       * hence the ">=" instead of just ">".
       */
      while (days >= days_in_month)
        days -= 7;

      xdate_add_days (&date, days);

      buffer->mday = xdate_get_day (&date);
    }
}

/* Offset is previous offset of local time. Returns 0 if month is 0 */
static sint64_t
boundary_for_year (TimeZoneDate *boundary,
                   xint_t          year,
                   gint32        offset)
{
  TimeZoneDate buffer;
  xdate_t date;
  const xuint64_t unix_epoch_start = 719163L;
  const xuint64_t seconds_per_day = 86400L;

  if (!boundary->mon)
    return 0;
  buffer = *boundary;

  if (boundary->year == 0)
    {
      buffer.year = year;

      if (buffer.wday)
        find_relative_date (&buffer);
    }

  xassert (buffer.year == year);
  xdate_clear (&date, 1);
  xdate_set_dmy (&date, buffer.mday, buffer.mon, buffer.year);
  return ((xdate_get_julian (&date) - unix_epoch_start) * seconds_per_day +
          buffer.offset - offset);
}

static void
fill_transition_info_from_rule (TransitionInfo *info,
                                TimeZoneRule   *rule,
                                xboolean_t        is_dst)
{
  xint_t offset = is_dst ? rule->dlt_offset : rule->std_offset;
  xchar_t *name = is_dst ? rule->dlt_name : rule->std_name;

  info->gmt_offset = offset;
  info->is_dst = is_dst;

  if (name)
    info->abbrev = xstrdup (name);

  else
    info->abbrev = xstrdup_printf ("%+03d%02d",
                                      (int) offset / 3600,
                                      (int) abs (offset / 60) % 60);
}

static void
init_zone_from_rules (xtimezone_t    *gtz,
                      TimeZoneRule *rules,
                      xuint_t         rules_num,
                      xchar_t        *identifier  /* (transfer full) */)
{
  xuint_t type_count = 0, trans_count = 0, info_index = 0;
  xuint_t ri; /* rule index */
  xboolean_t skip_first_std_trans = TRUE;
  gint32 last_offset;

  type_count = 0;
  trans_count = 0;

  /* Last rule only contains max year */
  for (ri = 0; ri < rules_num - 1; ri++)
    {
      if (rules[ri].dlt_start.mon || rules[ri].dlt_end.mon)
        {
          xuint_t rulespan = (rules[ri + 1].start_year - rules[ri].start_year);
          xuint_t transitions = rules[ri].dlt_start.mon > 0 ? 1 : 0;
          transitions += rules[ri].dlt_end.mon > 0 ? 1 : 0;
          type_count += rules[ri].dlt_start.mon > 0 ? 2 : 1;
          trans_count += transitions * rulespan;
        }
      else
        type_count++;
    }

  gtz->name = g_steal_pointer (&identifier);
  gtz->t_info = g_array_sized_new (FALSE, TRUE, sizeof (TransitionInfo), type_count);
  gtz->transitions = g_array_sized_new (FALSE, TRUE, sizeof (Transition), trans_count);

  last_offset = rules[0].std_offset;

  for (ri = 0; ri < rules_num - 1; ri++)
    {
      if ((rules[ri].std_offset || rules[ri].dlt_offset) &&
          rules[ri].dlt_start.mon == 0 && rules[ri].dlt_end.mon == 0)
        {
          TransitionInfo std_info;
          /* Standard */
          fill_transition_info_from_rule (&std_info, &(rules[ri]), FALSE);
          g_array_append_val (gtz->t_info, std_info);

          if (ri > 0 &&
              ((rules[ri - 1].dlt_start.mon > 12 &&
                rules[ri - 1].dlt_start.wday > rules[ri - 1].dlt_end.wday) ||
                rules[ri - 1].dlt_start.mon > rules[ri - 1].dlt_end.mon))
            {
              /* The previous rule was a southern hemisphere rule that
                 starts the year with DST, so we need to add a
                 transition to return to standard time */
              xuint_t year = rules[ri].start_year;
              sint64_t std_time =  boundary_for_year (&rules[ri].dlt_end,
                                                    year, last_offset);
              Transition std_trans = {std_time, info_index};
              g_array_append_val (gtz->transitions, std_trans);

            }
          last_offset = rules[ri].std_offset;
          ++info_index;
          skip_first_std_trans = TRUE;
         }
      else
        {
          const xuint_t start_year = rules[ri].start_year;
          const xuint_t end_year = rules[ri + 1].start_year;
          xboolean_t dlt_first;
          xuint_t year;
          TransitionInfo std_info, dlt_info;
          if (rules[ri].dlt_start.mon > 12)
            dlt_first = rules[ri].dlt_start.wday > rules[ri].dlt_end.wday;
          else
            dlt_first = rules[ri].dlt_start.mon > rules[ri].dlt_end.mon;
          /* Standard rules are always even, because before the first
             transition is always standard time, and 0 is even. */
          fill_transition_info_from_rule (&std_info, &(rules[ri]), FALSE);
          fill_transition_info_from_rule (&dlt_info, &(rules[ri]), TRUE);

          g_array_append_val (gtz->t_info, std_info);
          g_array_append_val (gtz->t_info, dlt_info);

          /* Transition dates. We hope that a year which ends daylight
             time in a southern-hemisphere country (i.e., one that
             begins the year in daylight time) will include a rule
             which has only a dlt_end. */
          for (year = start_year; year < end_year; year++)
            {
              gint32 dlt_offset = (dlt_first ? last_offset :
                                   rules[ri].dlt_offset);
              gint32 std_offset = (dlt_first ? rules[ri].std_offset :
                                   last_offset);
              /* NB: boundary_for_year returns 0 if mon == 0 */
              sint64_t std_time =  boundary_for_year (&rules[ri].dlt_end,
                                                    year, dlt_offset);
              sint64_t dlt_time = boundary_for_year (&rules[ri].dlt_start,
                                                   year, std_offset);
              Transition std_trans = {std_time, info_index};
              Transition dlt_trans = {dlt_time, info_index + 1};
              last_offset = (dlt_first ? rules[ri].dlt_offset :
                             rules[ri].std_offset);
              if (dlt_first)
                {
                  if (skip_first_std_trans)
                    skip_first_std_trans = FALSE;
                  else if (std_time)
                    g_array_append_val (gtz->transitions, std_trans);
                  if (dlt_time)
                    g_array_append_val (gtz->transitions, dlt_trans);
                }
              else
                {
                  if (dlt_time)
                    g_array_append_val (gtz->transitions, dlt_trans);
                  if (std_time)
                    g_array_append_val (gtz->transitions, std_trans);
                }
            }

          info_index += 2;
        }
    }
  if (ri > 0 &&
      ((rules[ri - 1].dlt_start.mon > 12 &&
        rules[ri - 1].dlt_start.wday > rules[ri - 1].dlt_end.wday) ||
       rules[ri - 1].dlt_start.mon > rules[ri - 1].dlt_end.mon))
    {
      /* The previous rule was a southern hemisphere rule that
         starts the year with DST, so we need to add a
         transition to return to standard time */
      TransitionInfo info;
      xuint_t year = rules[ri].start_year;
      Transition trans;
      fill_transition_info_from_rule (&info, &(rules[ri - 1]), FALSE);
      g_array_append_val (gtz->t_info, info);
      trans.time = boundary_for_year (&rules[ri - 1].dlt_end,
                                      year, last_offset);
      trans.info_index = info_index;
      g_array_append_val (gtz->transitions, trans);
     }
}

/*
 * parses date[/time] for parsing TZ environment variable
 *
 * date is either Mm.w.d, Jn or N
 * - m is 1 to 12
 * - w is 1 to 5
 * - d is 0 to 6
 * - n is 1 to 365
 * - N is 0 to 365
 *
 * time is either h or hh[[:]mm[[[:]ss]]]
 *  - h[h] is 0 to 24
 *  - mm is 00 to 59
 *  - ss is 00 to 59
 */
static xboolean_t
parse_mwd_boundary (xchar_t **pos, TimeZoneDate *boundary)
{
  xint_t month, week, day;

  if (**pos == '\0' || **pos < '0' || '9' < **pos)
    return FALSE;

  month = *(*pos)++ - '0';

  if ((month == 1 && **pos >= '0' && '2' >= **pos) ||
      (month == 0 && **pos >= '0' && '9' >= **pos))
    {
      month *= 10;
      month += *(*pos)++ - '0';
    }

  if (*(*pos)++ != '.' || month == 0)
    return FALSE;

  if (**pos == '\0' || **pos < '1' || '5' < **pos)
    return FALSE;

  week = *(*pos)++ - '0';

  if (*(*pos)++ != '.')
    return FALSE;

  if (**pos == '\0' || **pos < '0' || '6' < **pos)
    return FALSE;

  day = *(*pos)++ - '0';

  if (!day)
    day += 7;

  boundary->year = 0;
  boundary->mon = month;
  boundary->week = week;
  boundary->wday = day;
  return TRUE;
}

/*
 * This parses two slightly different ways of specifying
 * the Julian day:
 *
 * - ignore_leap == TRUE
 *
 *   Jn   This specifies the Julian day with n between 1 and 365. Leap days
 *        are not counted. In this format, February 29 can't be represented;
 *        February 28 is day 59, and March 1 is always day 60.
 *
 * - ignore_leap == FALSE
 *
 *   n   This specifies the zero-based Julian day with n between 0 and 365.
 *       February 29 is counted in leap years.
 */
static xboolean_t
parse_julian_boundary (xchar_t** pos, TimeZoneDate *boundary,
                       xboolean_t ignore_leap)
{
  xint_t day = 0;
  xdate_t date;

  while (**pos >= '0' && '9' >= **pos)
    {
      day *= 10;
      day += *(*pos)++ - '0';
    }

  if (ignore_leap)
    {
      if (day < 1 || 365 < day)
        return FALSE;
      if (day >= 59)
        day++;
    }
  else
    {
      if (day < 0 || 365 < day)
        return FALSE;
      /* xdate_t wants day in range 1->366 */
      day++;
    }

  xdate_clear (&date, 1);
  xdate_set_julian (&date, day);
  boundary->year = 0;
  boundary->mon = (int) xdate_get_month (&date);
  boundary->mday = (int) xdate_get_day (&date);
  boundary->wday = 0;

  return TRUE;
}

static xboolean_t
parse_tz_boundary (const xchar_t  *identifier,
                   TimeZoneDate *boundary)
{
  xchar_t *pos;

  pos = (xchar_t*)identifier;
  /* Month-week-weekday */
  if (*pos == 'M')
    {
      ++pos;
      if (!parse_mwd_boundary (&pos, boundary))
        return FALSE;
    }
  /* Julian date which ignores Feb 29 in leap years */
  else if (*pos == 'J')
    {
      ++pos;
      if (!parse_julian_boundary (&pos, boundary, TRUE))
        return FALSE ;
    }
  /* Julian date which counts Feb 29 in leap years */
  else if (*pos >= '0' && '9' >= *pos)
    {
      if (!parse_julian_boundary (&pos, boundary, FALSE))
        return FALSE;
    }
  else
    return FALSE;

  /* Time */

  if (*pos == '/')
    return parse_constant_offset (pos + 1, &boundary->offset, TRUE);
  else
    {
      boundary->offset = 2 * 60 * 60;
      return *pos == '\0';
    }
}

static xuint_t
create_ruleset_from_rule (TimeZoneRule **rules, TimeZoneRule *rule)
{
  *rules = g_new0 (TimeZoneRule, 2);

  (*rules)[0].start_year = MIN_TZYEAR;
  (*rules)[1].start_year = MAX_TZYEAR;

  (*rules)[0].std_offset = -rule->std_offset;
  (*rules)[0].dlt_offset = -rule->dlt_offset;
  (*rules)[0].dlt_start  = rule->dlt_start;
  (*rules)[0].dlt_end = rule->dlt_end;
  strcpy ((*rules)[0].std_name, rule->std_name);
  strcpy ((*rules)[0].dlt_name, rule->dlt_name);
  return 2;
}

static xboolean_t
parse_offset (xchar_t **pos, gint32 *target)
{
  xchar_t *buffer;
  xchar_t *target_pos = *pos;
  xboolean_t ret;

  while (**pos == '+' || **pos == '-' || **pos == ':' ||
         (**pos >= '0' && '9' >= **pos))
    ++(*pos);

  buffer = xstrndup (target_pos, *pos - target_pos);
  ret = parse_constant_offset (buffer, target, FALSE);
  g_free (buffer);

  return ret;
}

static xboolean_t
parse_identifier_boundary (xchar_t **pos, TimeZoneDate *target)
{
  xchar_t *buffer;
  xchar_t *target_pos = *pos;
  xboolean_t ret;

  while (**pos != ',' && **pos != '\0')
    ++(*pos);
  buffer = xstrndup (target_pos, *pos - target_pos);
  ret = parse_tz_boundary (buffer, target);
  g_free (buffer);

  return ret;
}

static xboolean_t
set_tz_name (xchar_t **pos, xchar_t *buffer, xuint_t size)
{
  xboolean_t quoted = **pos == '<';
  xchar_t *name_pos = *pos;
  xuint_t len;

  xassert (size != 0);

  if (quoted)
    {
      name_pos++;
      do
        ++(*pos);
      while (g_ascii_isalnum (**pos) || **pos == '-' || **pos == '+');
      if (**pos != '>')
        return FALSE;
    }
  else
    while (g_ascii_isalpha (**pos))
      ++(*pos);

  /* Name should be three or more characters */
  /* FIXME: Should return FALSE if the name is too long.
     This should simplify code later in this function.  */
  if (*pos - name_pos < 3)
    return FALSE;

  memset (buffer, 0, size);
  /* name_pos isn't 0-terminated, so we have to limit the length expressly */
  len = (xuint_t) (*pos - name_pos) > size - 1 ? size - 1 : (xuint_t) (*pos - name_pos);
  strncpy (buffer, name_pos, len);
  *pos += quoted;
  return TRUE;
}

static xboolean_t
parse_identifier_boundaries (xchar_t **pos, TimeZoneRule *tzr)
{
  if (*(*pos)++ != ',')
    return FALSE;

  /* Start date */
  if (!parse_identifier_boundary (pos, &(tzr->dlt_start)) || *(*pos)++ != ',')
    return FALSE;

  /* End date */
  if (!parse_identifier_boundary (pos, &(tzr->dlt_end)))
    return FALSE;
  return TRUE;
}

/*
 * Creates an array of TimeZoneRule from a TZ environment variable
 * type of identifier.  Should free rules afterwards
 */
static xuint_t
rules_from_identifier (const xchar_t   *identifier,
                       TimeZoneRule **rules)
{
  xchar_t *pos;
  TimeZoneRule tzr;

  xassert (rules != NULL);

  *rules = NULL;

  if (!identifier)
    return 0;

  pos = (xchar_t*)identifier;
  memset (&tzr, 0, sizeof (tzr));
  /* Standard offset */
  if (!(set_tz_name (&pos, tzr.std_name, NAME_SIZE)) ||
      !parse_offset (&pos, &(tzr.std_offset)))
    return 0;

  if (*pos == 0)
    {
      return create_ruleset_from_rule (rules, &tzr);
    }

  /* Format 2 */
  if (!(set_tz_name (&pos, tzr.dlt_name, NAME_SIZE)))
    return 0;
  parse_offset (&pos, &(tzr.dlt_offset));
  if (tzr.dlt_offset == 0) /* No daylight offset given, assume it's 1
                              hour earlier that standard */
    tzr.dlt_offset = tzr.std_offset - 3600;
  if (*pos == '\0')
#ifdef G_OS_WIN32
    /* Windows allows us to use the US DST boundaries if they're not given */
    {
      xuint_t i, rules_num = 0;

      /* Use US rules, Windows' default is Pacific Standard Time */
      if ((rules_num = rules_from_windows_time_zone ("Pacific Standard Time",
                                                     NULL,
                                                     rules)))
        {
          for (i = 0; i < rules_num - 1; i++)
            {
              (*rules)[i].std_offset = - tzr.std_offset;
              (*rules)[i].dlt_offset = - tzr.dlt_offset;
              strcpy ((*rules)[i].std_name, tzr.std_name);
              strcpy ((*rules)[i].dlt_name, tzr.dlt_name);
            }

          return rules_num;
        }
      else
        return 0;
    }
#else
  return 0;
#endif
  /* Start and end required (format 2) */
  if (!parse_identifier_boundaries (&pos, &tzr))
    return 0;

  return create_ruleset_from_rule (rules, &tzr);
}

#ifdef G_OS_UNIX
static xtimezone_t *
parse_footertz (const xchar_t *footer, size_t footerlen)
{
  xchar_t *tzstring = xstrndup (footer + 1, footerlen - 2);
  xtimezone_t *footertz = NULL;

  /* FIXME: The allocation for tzstring could be avoided by
     passing a xsize_t identifier_len argument to rules_from_identifier
     and changing the code in that function to stop assuming that
     identifier is nul-terminated.  */
  TimeZoneRule *rules;
  xuint_t rules_num = rules_from_identifier (tzstring, &rules);

  g_free (tzstring);
  if (rules_num > 1)
    {
      footertz = g_slice_new0 (xtimezone_t);
      init_zone_from_rules (footertz, rules, rules_num, NULL);
      footertz->ref_count++;
    }
  g_free (rules);
  return footertz;
}
#endif

/* Construction {{{1 */
/**
 * xtime_zone_new:
 * @identifier: (nullable): a timezone identifier
 *
 * A version of xtime_zone_new_identifier() which returns the UTC time zone
 * if @identifier could not be parsed or loaded.
 *
 * If you need to check whether @identifier was loaded successfully, use
 * xtime_zone_new_identifier().
 *
 * Returns: (transfer full) (not nullable): the requested timezone
 * Deprecated: 2.68: Use xtime_zone_new_identifier() instead, as it provides
 *     error reporting. Change your code to handle a potentially %NULL return
 *     value.
 *
 * Since: 2.26
 **/
xtimezone_t *
xtime_zone_new (const xchar_t *identifier)
{
  xtimezone_t *tz = xtime_zone_new_identifier (identifier);

  /* Always fall back to UTC. */
  if (tz == NULL)
    tz = xtime_zone_new_utc ();

  xassert (tz != NULL);

  return g_steal_pointer (&tz);
}

/**
 * xtime_zone_new_identifier:
 * @identifier: (nullable): a timezone identifier
 *
 * Creates a #xtimezone_t corresponding to @identifier. If @identifier cannot be
 * parsed or loaded, %NULL is returned.
 *
 * @identifier can either be an RFC3339/ISO 8601 time offset or
 * something that would pass as a valid value for the `TZ` environment
 * variable (including %NULL).
 *
 * In Windows, @identifier can also be the unlocalized name of a time
 * zone for standard time, for example "Pacific Standard Time".
 *
 * Valid RFC3339 time offsets are `"Z"` (for UTC) or
 * `"±hh:mm"`.  ISO 8601 additionally specifies
 * `"±hhmm"` and `"±hh"`.  Offsets are
 * time values to be added to Coordinated Universal Time (UTC) to get
 * the local time.
 *
 * In UNIX, the `TZ` environment variable typically corresponds
 * to the name of a file in the zoneinfo database, an absolute path to a file
 * somewhere else, or a string in
 * "std offset [dst [offset],start[/time],end[/time]]" (POSIX) format.
 * There  are  no spaces in the specification. The name of standard
 * and daylight savings time zone must be three or more alphabetic
 * characters. Offsets are time values to be added to local time to
 * get Coordinated Universal Time (UTC) and should be
 * `"[±]hh[[:]mm[:ss]]"`.  Dates are either
 * `"Jn"` (Julian day with n between 1 and 365, leap
 * years not counted), `"n"` (zero-based Julian day
 * with n between 0 and 365) or `"Mm.w.d"` (day d
 * (0 <= d <= 6) of week w (1 <= w <= 5) of month m (1 <= m <= 12), day
 * 0 is a Sunday).  Times are in local wall clock time, the default is
 * 02:00:00.
 *
 * In Windows, the "tzn[+|–]hh[:mm[:ss]][dzn]" format is used, but also
 * accepts POSIX format.  The Windows format uses US rules for all time
 * zones; daylight savings time is 60 minutes behind the standard time
 * with date and time of change taken from Pacific Standard Time.
 * Offsets are time values to be added to the local time to get
 * Coordinated Universal Time (UTC).
 *
 * xtime_zone_new_local() calls this function with the value of the
 * `TZ` environment variable. This function itself is independent of
 * the value of `TZ`, but if @identifier is %NULL then `/etc/localtime`
 * will be consulted to discover the correct time zone on UNIX and the
 * registry will be consulted or GetTimeZoneInformation() will be used
 * to get the local time zone on Windows.
 *
 * If intervals are not available, only time zone rules from `TZ`
 * environment variable or other means, then they will be computed
 * from year 1900 to 2037.  If the maximum year for the rules is
 * available and it is greater than 2037, then it will followed
 * instead.
 *
 * See
 * [RFC3339 §5.6](http://tools.ietf.org/html/rfc3339#section-5.6)
 * for a precise definition of valid RFC3339 time offsets
 * (the `time-offset` expansion) and ISO 8601 for the
 * full list of valid time offsets.  See
 * [The GNU C Library manual](http://www.gnu.org/s/libc/manual/html_node/TZ-Variable.html)
 * for an explanation of the possible
 * values of the `TZ` environment variable. See
 * [Microsoft Time Zone Index Values](http://msdn.microsoft.com/en-us/library/ms912391%28v=winembedded.11%29.aspx)
 * for the list of time zones on Windows.
 *
 * You should release the return value by calling xtime_zone_unref()
 * when you are done with it.
 *
 * Returns: (transfer full) (nullable): the requested timezone, or %NULL on
 *     failure
 * Since: 2.68
 */
xtimezone_t *
xtime_zone_new_identifier (const xchar_t *identifier)
{
  xtimezone_t *tz = NULL;
  TimeZoneRule *rules;
  xint_t rules_num;
  xchar_t *resolved_identifier = NULL;

  if (identifier)
    {
      G_LOCK (time_zones);
      if (time_zones == NULL)
        time_zones = xhash_table_new (xstr_hash, xstr_equal);

      tz = xhash_table_lookup (time_zones, identifier);
      if (tz)
        {
          g_atomic_int_inc (&tz->ref_count);
          G_UNLOCK (time_zones);
          return tz;
        }
      else
        resolved_identifier = xstrdup (identifier);
    }
  else
    {
      G_LOCK (tz_default);
#ifdef G_OS_UNIX
      resolved_identifier = zone_identifier_unix ();
#elif defined (G_OS_WIN32)
      resolved_identifier = windows_default_tzname ();
#endif
      if (tz_default)
        {
          /* Flush default if changed. If the identifier couldn’t be resolved,
           * we’re going to fall back to UTC eventually, so don’t clear out the
           * cache if it’s already UTC. */
          if (!(resolved_identifier == NULL && xstr_equal (tz_default->name, "UTC")) &&
              xstrcmp0 (tz_default->name, resolved_identifier) != 0)
            {
              g_clear_pointer (&tz_default, xtime_zone_unref);
            }
          else
            {
              tz = xtime_zone_ref (tz_default);
              G_UNLOCK (tz_default);

              g_free (resolved_identifier);
              return tz;
            }
        }
    }

  tz = g_slice_new0 (xtimezone_t);
  tz->ref_count = 0;

  zone_for_constant_offset (tz, identifier);

  if (tz->t_info == NULL &&
      (rules_num = rules_from_identifier (identifier, &rules)))
    {
      init_zone_from_rules (tz, rules, rules_num, g_steal_pointer (&resolved_identifier));
      g_free (rules);
    }

  if (tz->t_info == NULL)
    {
#ifdef G_OS_UNIX
      xbytes_t *zoneinfo = zone_info_unix (identifier, resolved_identifier);
      if (zoneinfo != NULL)
        {
          init_zone_from_iana_info (tz, zoneinfo, g_steal_pointer (&resolved_identifier));
          xbytes_unref (zoneinfo);
        }
#elif defined (G_OS_WIN32)
      if ((rules_num = rules_from_windows_time_zone (identifier,
                                                     resolved_identifier,
                                                     &rules)))
        {
          init_zone_from_rules (tz, rules, rules_num, g_steal_pointer (&resolved_identifier));
          g_free (rules);
        }
#endif
    }

#if defined (G_OS_WIN32)
  if (tz->t_info == NULL)
    {
      if (identifier == NULL)
        {
          TIME_ZONE_INFORMATION tzi;

          if (GetTimeZoneInformation (&tzi) != TIME_ZONE_ID_INVALID)
            {
              rules = g_new0 (TimeZoneRule, 2);

              if (rule_from_windows_time_zone_info (&rules[0], &tzi))
                {
                  memset (rules[0].std_name, 0, NAME_SIZE);
                  memset (rules[0].dlt_name, 0, NAME_SIZE);

                  rules[0].start_year = MIN_TZYEAR;
                  rules[1].start_year = MAX_TZYEAR;

                  init_zone_from_rules (tz, rules, 2, g_steal_pointer (&resolved_identifier));
                }

              g_free (rules);
            }
        }
    }
#endif

  g_free (resolved_identifier);

  /* Failed to load the timezone. */
  if (tz->t_info == NULL)
    {
      g_slice_free (xtimezone_t, tz);

      if (identifier)
        G_UNLOCK (time_zones);
      else
        G_UNLOCK (tz_default);

      return NULL;
    }

  xassert (tz->name != NULL);
  xassert (tz->t_info != NULL);

  if (identifier)
    xhash_table_insert (time_zones, tz->name, tz);
  else if (tz->name)
    {
      /* Caching reference */
      g_atomic_int_inc (&tz->ref_count);
      tz_default = tz;
    }

  g_atomic_int_inc (&tz->ref_count);

  if (identifier)
    G_UNLOCK (time_zones);
  else
    G_UNLOCK (tz_default);

  return tz;
}

/**
 * xtime_zone_new_utc:
 *
 * Creates a #xtimezone_t corresponding to UTC.
 *
 * This is equivalent to calling xtime_zone_new() with a value like
 * "Z", "UTC", "+00", etc.
 *
 * You should release the return value by calling xtime_zone_unref()
 * when you are done with it.
 *
 * Returns: the universal timezone
 *
 * Since: 2.26
 **/
xtimezone_t *
xtime_zone_new_utc (void)
{
  static xtimezone_t *utc = NULL;
  static xsize_t initialised;

  if (g_once_init_enter (&initialised))
    {
      utc = xtime_zone_new_identifier ("UTC");
      xassert (utc != NULL);
      g_once_init_leave (&initialised, TRUE);
    }

  return xtime_zone_ref (utc);
}

/**
 * xtime_zone_new_local:
 *
 * Creates a #xtimezone_t corresponding to local time.  The local time
 * zone may change between invocations to this function; for example,
 * if the system administrator changes it.
 *
 * This is equivalent to calling xtime_zone_new() with the value of
 * the `TZ` environment variable (including the possibility of %NULL).
 *
 * You should release the return value by calling xtime_zone_unref()
 * when you are done with it.
 *
 * Returns: the local timezone
 *
 * Since: 2.26
 **/
xtimezone_t *
xtime_zone_new_local (void)
{
  const xchar_t *tzenv = g_getenv ("TZ");
  xtimezone_t *tz;

  G_LOCK (tz_local);

  /* Is time zone changed and must be flushed? */
  if (tz_local && xstrcmp0 (xtime_zone_get_identifier (tz_local), tzenv))
    g_clear_pointer (&tz_local, xtime_zone_unref);

  if (tz_local == NULL)
    tz_local = xtime_zone_new_identifier (tzenv);
  if (tz_local == NULL)
    tz_local = xtime_zone_new_utc ();

  tz = xtime_zone_ref (tz_local);

  G_UNLOCK (tz_local);

  return tz;
}

/**
 * xtime_zone_new_offset:
 * @seconds: offset to UTC, in seconds
 *
 * Creates a #xtimezone_t corresponding to the given constant offset from UTC,
 * in seconds.
 *
 * This is equivalent to calling xtime_zone_new() with a string in the form
 * `[+|-]hh[:mm[:ss]]`.
 *
 * It is possible for this function to fail if @seconds is too big (greater than
 * 24 hours), in which case this function will return the UTC timezone for
 * backwards compatibility. To detect failures like this, use
 * xtime_zone_new_identifier() directly.
 *
 * Returns: (transfer full): a timezone at the given offset from UTC, or UTC on
 *   failure
 * Since: 2.58
 */
xtimezone_t *
xtime_zone_new_offset (gint32 seconds)
{
  xtimezone_t *tz = NULL;
  xchar_t *identifier = NULL;

  /* Seemingly, we should be using @seconds directly to set the
   * #TransitionInfo.gmt_offset to avoid all this string building and parsing.
   * However, we always need to set the #xtimezone_t.name to a constructed
   * string anyway, so we might as well reuse its code.
   * xtime_zone_new_identifier() should never fail in this situation. */
  identifier = xstrdup_printf ("%c%02u:%02u:%02u",
                                (seconds >= 0) ? '+' : '-',
                                (ABS (seconds) / 60) / 60,
                                (ABS (seconds) / 60) % 60,
                                ABS (seconds) % 60);
  tz = xtime_zone_new_identifier (identifier);

  if (tz == NULL)
    tz = xtime_zone_new_utc ();
  else
    xassert (xtime_zone_get_offset (tz, 0) == seconds);

  xassert (tz != NULL);
  g_free (identifier);

  return tz;
}

#define TRANSITION(n)         g_array_index (tz->transitions, Transition, n)
#define TRANSITION_INFO(n)    g_array_index (tz->t_info, TransitionInfo, n)

/* Internal helpers {{{1 */
/* NB: Interval 0 is before the first transition, so there's no
 * transition structure to point to which TransitionInfo to
 * use. Rule-based zones are set up so that TI 0 is always standard
 * time (which is what's in effect before Daylight time got started
 * in the early 20th century), but IANA tzfiles don't follow that
 * convention. The tzfile documentation says to use the first
 * standard-time (i.e., non-DST) tinfo, so that's what we do.
 */
inline static const TransitionInfo*
interval_info (xtimezone_t *tz,
               xuint_t      interval)
{
  xuint_t index;
  xreturn_val_if_fail (tz->t_info != NULL, NULL);
  if (interval && tz->transitions && interval <= tz->transitions->len)
    index = (TRANSITION(interval - 1)).info_index;
  else
    {
      for (index = 0; index < tz->t_info->len; index++)
        {
          TransitionInfo *tzinfo = &(TRANSITION_INFO(index));
          if (!tzinfo->is_dst)
            return tzinfo;
        }
      index = 0;
    }

  return &(TRANSITION_INFO(index));
}

inline static sint64_t
interval_start (xtimezone_t *tz,
                xuint_t      interval)
{
  if (!interval || tz->transitions == NULL || tz->transitions->len == 0)
    return G_MININT64;
  if (interval > tz->transitions->len)
    interval = tz->transitions->len;
  return (TRANSITION(interval - 1)).time;
}

inline static sint64_t
interval_end (xtimezone_t *tz,
              xuint_t      interval)
{
  if (tz->transitions && interval < tz->transitions->len)
    {
      sint64_t lim = (TRANSITION(interval)).time;
      return lim - (lim != G_MININT64);
    }
  return G_MAXINT64;
}

inline static gint32
interval_offset (xtimezone_t *tz,
                 xuint_t      interval)
{
  xreturn_val_if_fail (tz->t_info != NULL, 0);
  return interval_info (tz, interval)->gmt_offset;
}

inline static xboolean_t
interval_isdst (xtimezone_t *tz,
                xuint_t      interval)
{
  xreturn_val_if_fail (tz->t_info != NULL, 0);
  return interval_info (tz, interval)->is_dst;
}


inline static xchar_t*
interval_abbrev (xtimezone_t *tz,
                  xuint_t      interval)
{
  xreturn_val_if_fail (tz->t_info != NULL, 0);
  return interval_info (tz, interval)->abbrev;
}

inline static sint64_t
interval_local_start (xtimezone_t *tz,
                      xuint_t      interval)
{
  if (interval)
    return interval_start (tz, interval) + interval_offset (tz, interval);

  return G_MININT64;
}

inline static sint64_t
interval_local_end (xtimezone_t *tz,
                    xuint_t      interval)
{
  if (tz->transitions && interval < tz->transitions->len)
    return interval_end (tz, interval) + interval_offset (tz, interval);

  return G_MAXINT64;
}

static xboolean_t
interval_valid (xtimezone_t *tz,
                xuint_t      interval)
{
  if ( tz->transitions == NULL)
    return interval == 0;
  return interval <= tz->transitions->len;
}

/* xtime_zone_find_interval() {{{1 */

/**
 * xtime_zone_adjust_time:
 * @tz: a #xtimezone_t
 * @type: the #GTimeType of @time_
 * @time_: a pointer to a number of seconds since January 1, 1970
 *
 * Finds an interval within @tz that corresponds to the given @time_,
 * possibly adjusting @time_ if required to fit into an interval.
 * The meaning of @time_ depends on @type.
 *
 * This function is similar to xtime_zone_find_interval(), with the
 * difference that it always succeeds (by making the adjustments
 * described below).
 *
 * In any of the cases where xtime_zone_find_interval() succeeds then
 * this function returns the same value, without modifying @time_.
 *
 * This function may, however, modify @time_ in order to deal with
 * non-existent times.  If the non-existent local @time_ of 02:30 were
 * requested on March 14th 2010 in Toronto then this function would
 * adjust @time_ to be 03:00 and return the interval containing the
 * adjusted time.
 *
 * Returns: the interval containing @time_, never -1
 *
 * Since: 2.26
 **/
xint_t
xtime_zone_adjust_time (xtimezone_t *tz,
                         GTimeType  type,
                         sint64_t    *time_)
{
  xuint_t i, intervals;
  xboolean_t interval_is_dst;

  if (tz->transitions == NULL)
    return 0;

  intervals = tz->transitions->len;

  /* find the interval containing *time UTC
   * TODO: this could be binary searched (or better) */
  for (i = 0; i <= intervals; i++)
    if (*time_ <= interval_end (tz, i))
      break;

  xassert (interval_start (tz, i) <= *time_ && *time_ <= interval_end (tz, i));

  if (type != G_TIME_TYPE_UNIVERSAL)
    {
      if (*time_ < interval_local_start (tz, i))
        /* if time came before the start of this interval... */
        {
          i--;

          /* if it's not in the previous interval... */
          if (*time_ > interval_local_end (tz, i))
            {
              /* it doesn't exist.  fast-forward it. */
              i++;
              *time_ = interval_local_start (tz, i);
            }
        }

      else if (*time_ > interval_local_end (tz, i))
        /* if time came after the end of this interval... */
        {
          i++;

          /* if it's not in the next interval... */
          if (*time_ < interval_local_start (tz, i))
            /* it doesn't exist.  fast-forward it. */
            *time_ = interval_local_start (tz, i);
        }

      else
        {
          interval_is_dst = interval_isdst (tz, i);
          if ((interval_is_dst && type != G_TIME_TYPE_DAYLIGHT) ||
              (!interval_is_dst && type == G_TIME_TYPE_DAYLIGHT))
            {
              /* it's in this interval, but dst flag doesn't match.
               * check neighbours for a better fit. */
              if (i && *time_ <= interval_local_end (tz, i - 1))
                i--;

              else if (i < intervals &&
                       *time_ >= interval_local_start (tz, i + 1))
                i++;
            }
        }
    }

  return i;
}

/**
 * xtime_zone_find_interval:
 * @tz: a #xtimezone_t
 * @type: the #GTimeType of @time_
 * @time_: a number of seconds since January 1, 1970
 *
 * Finds an interval within @tz that corresponds to the given @time_.
 * The meaning of @time_ depends on @type.
 *
 * If @type is %G_TIME_TYPE_UNIVERSAL then this function will always
 * succeed (since universal time is monotonic and continuous).
 *
 * Otherwise @time_ is treated as local time.  The distinction between
 * %G_TIME_TYPE_STANDARD and %G_TIME_TYPE_DAYLIGHT is ignored except in
 * the case that the given @time_ is ambiguous.  In Toronto, for example,
 * 01:30 on November 7th 2010 occurred twice (once inside of daylight
 * savings time and the next, an hour later, outside of daylight savings
 * time).  In this case, the different value of @type would result in a
 * different interval being returned.
 *
 * It is still possible for this function to fail.  In Toronto, for
 * example, 02:00 on March 14th 2010 does not exist (due to the leap
 * forward to begin daylight savings time).  -1 is returned in that
 * case.
 *
 * Returns: the interval containing @time_, or -1 in case of failure
 *
 * Since: 2.26
 */
xint_t
xtime_zone_find_interval (xtimezone_t *tz,
                           GTimeType  type,
                           sint64_t     time_)
{
  xuint_t i, intervals;
  xboolean_t interval_is_dst;

  if (tz->transitions == NULL)
    return 0;
  intervals = tz->transitions->len;
  for (i = 0; i <= intervals; i++)
    if (time_ <= interval_end (tz, i))
      break;

  if (type == G_TIME_TYPE_UNIVERSAL)
    return i;

  if (time_ < interval_local_start (tz, i))
    {
      if (time_ > interval_local_end (tz, --i))
        return -1;
    }

  else if (time_ > interval_local_end (tz, i))
    {
      if (time_ < interval_local_start (tz, ++i))
        return -1;
    }

  else
    {
      interval_is_dst = interval_isdst (tz, i);
      if  ((interval_is_dst && type != G_TIME_TYPE_DAYLIGHT) ||
           (!interval_is_dst && type == G_TIME_TYPE_DAYLIGHT))
        {
          if (i && time_ <= interval_local_end (tz, i - 1))
            i--;

          else if (i < intervals && time_ >= interval_local_start (tz, i + 1))
            i++;
        }
    }

  return i;
}

/* Public API accessors {{{1 */

/**
 * xtime_zone_get_abbreviation:
 * @tz: a #xtimezone_t
 * @interval: an interval within the timezone
 *
 * Determines the time zone abbreviation to be used during a particular
 * @interval of time in the time zone @tz.
 *
 * For example, in Toronto this is currently "EST" during the winter
 * months and "EDT" during the summer months when daylight savings time
 * is in effect.
 *
 * Returns: the time zone abbreviation, which belongs to @tz
 *
 * Since: 2.26
 **/
const xchar_t *
xtime_zone_get_abbreviation (xtimezone_t *tz,
                              xint_t       interval)
{
  xreturn_val_if_fail (interval_valid (tz, (xuint_t)interval), NULL);

  return interval_abbrev (tz, (xuint_t)interval);
}

/**
 * xtime_zone_get_offset:
 * @tz: a #xtimezone_t
 * @interval: an interval within the timezone
 *
 * Determines the offset to UTC in effect during a particular @interval
 * of time in the time zone @tz.
 *
 * The offset is the number of seconds that you add to UTC time to
 * arrive at local time for @tz (ie: negative numbers for time zones
 * west of GMT, positive numbers for east).
 *
 * Returns: the number of seconds that should be added to UTC to get the
 *          local time in @tz
 *
 * Since: 2.26
 **/
gint32
xtime_zone_get_offset (xtimezone_t *tz,
                        xint_t       interval)
{
  xreturn_val_if_fail (interval_valid (tz, (xuint_t)interval), 0);

  return interval_offset (tz, (xuint_t)interval);
}

/**
 * xtime_zone_is_dst:
 * @tz: a #xtimezone_t
 * @interval: an interval within the timezone
 *
 * Determines if daylight savings time is in effect during a particular
 * @interval of time in the time zone @tz.
 *
 * Returns: %TRUE if daylight savings time is in effect
 *
 * Since: 2.26
 **/
xboolean_t
xtime_zone_is_dst (xtimezone_t *tz,
                    xint_t       interval)
{
  xreturn_val_if_fail (interval_valid (tz, interval), FALSE);

  if (tz->transitions == NULL)
    return FALSE;

  return interval_isdst (tz, (xuint_t)interval);
}

/**
 * xtime_zone_get_identifier:
 * @tz: a #xtimezone_t
 *
 * Get the identifier of this #xtimezone_t, as passed to xtime_zone_new().
 * If the identifier passed at construction time was not recognised, `UTC` will
 * be returned. If it was %NULL, the identifier of the local timezone at
 * construction time will be returned.
 *
 * The identifier will be returned in the same format as provided at
 * construction time: if provided as a time offset, that will be returned by
 * this function.
 *
 * Returns: identifier for this timezone
 * Since: 2.58
 */
const xchar_t *
xtime_zone_get_identifier (xtimezone_t *tz)
{
  xreturn_val_if_fail (tz != NULL, NULL);

  return tz->name;
}

/* Epilogue {{{1 */
/* vim:set foldmethod=marker: */
