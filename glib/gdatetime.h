/*
 * Copyright (C) 2009-2010 Christian Hergert <chris@dronelabs.com>
 * Copyright © 2010 Codethink Limited
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
 */

#ifndef __G_DATE_TIME_H__
#define __G_DATE_TIME_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtimezone.h>

G_BEGIN_DECLS

/**
 * G_TIME_SPAN_DAY:
 *
 * Evaluates to a time span of one day.
 *
 * Since: 2.26
 */
#define G_TIME_SPAN_DAY                 (G_GINT64_CONSTANT (86400000000))

/**
 * G_TIME_SPAN_HOUR:
 *
 * Evaluates to a time span of one hour.
 *
 * Since: 2.26
 */
#define G_TIME_SPAN_HOUR                (G_GINT64_CONSTANT (3600000000))

/**
 * G_TIME_SPAN_MINUTE:
 *
 * Evaluates to a time span of one minute.
 *
 * Since: 2.26
 */
#define G_TIME_SPAN_MINUTE              (G_GINT64_CONSTANT (60000000))

/**
 * G_TIME_SPAN_SECOND:
 *
 * Evaluates to a time span of one second.
 *
 * Since: 2.26
 */
#define G_TIME_SPAN_SECOND              (G_GINT64_CONSTANT (1000000))

/**
 * G_TIME_SPAN_MILLISECOND:
 *
 * Evaluates to a time span of one millisecond.
 *
 * Since: 2.26
 */
#define G_TIME_SPAN_MILLISECOND         (G_GINT64_CONSTANT (1000))

/**
 * GTimeSpan:
 *
 * A value representing an interval of time, in microseconds.
 *
 * Since: 2.26
 */
typedef gint64 GTimeSpan;

/**
 * xdatetime_t:
 *
 * An opaque structure that represents a date and time, including a time zone.
 *
 * Since: 2.26
 */
typedef struct _GDateTime xdatetime_t;

XPL_AVAILABLE_IN_ALL
void                    xdate_time_unref                               (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_ALL
xdatetime_t *             xdate_time_ref                                 (xdatetime_t      *datetime);

XPL_AVAILABLE_IN_ALL
xdatetime_t *             xdate_time_new_now                             (xtimezone_t      *tz);
XPL_AVAILABLE_IN_ALL
xdatetime_t *             xdate_time_new_now_local                       (void);
XPL_AVAILABLE_IN_ALL
xdatetime_t *             xdate_time_new_now_utc                         (void);

XPL_AVAILABLE_IN_ALL
xdatetime_t *             xdate_time_new_from_unix_local                 (gint64          t);
XPL_AVAILABLE_IN_ALL
xdatetime_t *             xdate_time_new_from_unix_utc                   (gint64          t);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_DEPRECATED_IN_2_62_FOR(xdate_time_new_from_unix_local)
xdatetime_t *             xdate_time_new_from_timeval_local              (const GTimeVal *tv);
XPL_DEPRECATED_IN_2_62_FOR(xdate_time_new_from_unix_utc)
xdatetime_t *             xdate_time_new_from_timeval_utc                (const GTimeVal *tv);
G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_2_56
xdatetime_t *             xdate_time_new_from_iso8601                    (const xchar_t    *text,
                                                                         xtimezone_t      *default_tz);

XPL_AVAILABLE_IN_ALL
xdatetime_t *             xdate_time_new                                 (xtimezone_t      *tz,
                                                                         xint_t            year,
                                                                         xint_t            month,
                                                                         xint_t            day,
                                                                         xint_t            hour,
                                                                         xint_t            minute,
                                                                         xdouble_t         seconds);
XPL_AVAILABLE_IN_ALL
xdatetime_t *             xdate_time_new_local                           (xint_t            year,
                                                                         xint_t            month,
                                                                         xint_t            day,
                                                                         xint_t            hour,
                                                                         xint_t            minute,
                                                                         xdouble_t         seconds);
XPL_AVAILABLE_IN_ALL
xdatetime_t *             xdate_time_new_utc                             (xint_t            year,
                                                                         xint_t            month,
                                                                         xint_t            day,
                                                                         xint_t            hour,
                                                                         xint_t            minute,
                                                                         xdouble_t         seconds);

XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
xdatetime_t *             xdate_time_add                                 (xdatetime_t      *datetime,
                                                                         GTimeSpan       timespan);

XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
xdatetime_t *             xdate_time_add_years                           (xdatetime_t      *datetime,
                                                                         xint_t            years);
XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
xdatetime_t *             xdate_time_add_months                          (xdatetime_t      *datetime,
                                                                         xint_t            months);
XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
xdatetime_t *             xdate_time_add_weeks                           (xdatetime_t      *datetime,
                                                                         xint_t            weeks);
XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
xdatetime_t *             xdate_time_add_days                            (xdatetime_t      *datetime,
                                                                         xint_t            days);

XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
xdatetime_t *             xdate_time_add_hours                           (xdatetime_t      *datetime,
                                                                         xint_t            hours);
XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
xdatetime_t *             xdate_time_add_minutes                         (xdatetime_t      *datetime,
                                                                         xint_t            minutes);
XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
xdatetime_t *             xdate_time_add_seconds                         (xdatetime_t      *datetime,
                                                                         xdouble_t         seconds);

XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
xdatetime_t *             xdate_time_add_full                            (xdatetime_t      *datetime,
                                                                         xint_t            years,
                                                                         xint_t            months,
                                                                         xint_t            days,
                                                                         xint_t            hours,
                                                                         xint_t            minutes,
                                                                         xdouble_t         seconds);

XPL_AVAILABLE_IN_ALL
xint_t                    xdate_time_compare                             (xconstpointer   dt1,
                                                                         xconstpointer   dt2);
XPL_AVAILABLE_IN_ALL
GTimeSpan               xdate_time_difference                          (xdatetime_t      *end,
                                                                         xdatetime_t      *begin);
XPL_AVAILABLE_IN_ALL
xuint_t                   xdate_time_hash                                (xconstpointer   datetime);
XPL_AVAILABLE_IN_ALL
xboolean_t                xdate_time_equal                               (xconstpointer   dt1,
                                                                         xconstpointer   dt2);

XPL_AVAILABLE_IN_ALL
void                    xdate_time_get_ymd                             (xdatetime_t      *datetime,
                                                                         xint_t           *year,
                                                                         xint_t           *month,
                                                                         xint_t           *day);

XPL_AVAILABLE_IN_ALL
xint_t                    xdate_time_get_year                            (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    xdate_time_get_month                           (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    xdate_time_get_day_of_month                    (xdatetime_t      *datetime);

XPL_AVAILABLE_IN_ALL
xint_t                    xdate_time_get_week_numbering_year             (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    xdate_time_get_week_of_year                    (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    xdate_time_get_day_of_week                     (xdatetime_t      *datetime);

XPL_AVAILABLE_IN_ALL
xint_t                    xdate_time_get_day_of_year                     (xdatetime_t      *datetime);

XPL_AVAILABLE_IN_ALL
xint_t                    xdate_time_get_hour                            (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    xdate_time_get_minute                          (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    xdate_time_get_second                          (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    xdate_time_get_microsecond                     (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_ALL
xdouble_t                 xdate_time_get_seconds                         (xdatetime_t      *datetime);

XPL_AVAILABLE_IN_ALL
gint64                  xdate_time_to_unix                             (xdatetime_t      *datetime);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_DEPRECATED_IN_2_62_FOR(xdate_time_to_unix)
xboolean_t                xdate_time_to_timeval                          (xdatetime_t      *datetime,
                                                                         GTimeVal       *tv);
G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_ALL
GTimeSpan               xdate_time_get_utc_offset                      (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_2_58
xtimezone_t *             xdate_time_get_timezone                        (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_ALL
const xchar_t *           xdate_time_get_timezone_abbreviation           (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_ALL
xboolean_t                xdate_time_is_daylight_savings                 (xdatetime_t      *datetime);

XPL_AVAILABLE_IN_ALL
xdatetime_t *             xdate_time_to_timezone                         (xdatetime_t      *datetime,
                                                                         xtimezone_t      *tz);
XPL_AVAILABLE_IN_ALL
xdatetime_t *             xdate_time_to_local                            (xdatetime_t      *datetime);
XPL_AVAILABLE_IN_ALL
xdatetime_t *             xdate_time_to_utc                              (xdatetime_t      *datetime);

XPL_AVAILABLE_IN_ALL
xchar_t *                 xdate_time_format                              (xdatetime_t      *datetime,
                                                                         const xchar_t    *format) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_2_62
xchar_t *                 xdate_time_format_iso8601                      (xdatetime_t      *datetime) G_GNUC_MALLOC;

G_END_DECLS

#endif /* __G_DATE_TIME_H__ */
