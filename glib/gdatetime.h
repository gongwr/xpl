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
 * GDateTime:
 *
 * An opaque structure that represents a date and time, including a time zone.
 *
 * Since: 2.26
 */
typedef struct _GDateTime GDateTime;

XPL_AVAILABLE_IN_ALL
void                    g_date_time_unref                               (GDateTime      *datetime);
XPL_AVAILABLE_IN_ALL
GDateTime *             g_date_time_ref                                 (GDateTime      *datetime);

XPL_AVAILABLE_IN_ALL
GDateTime *             g_date_time_new_now                             (GTimeZone      *tz);
XPL_AVAILABLE_IN_ALL
GDateTime *             g_date_time_new_now_local                       (void);
XPL_AVAILABLE_IN_ALL
GDateTime *             g_date_time_new_now_utc                         (void);

XPL_AVAILABLE_IN_ALL
GDateTime *             g_date_time_new_from_unix_local                 (gint64          t);
XPL_AVAILABLE_IN_ALL
GDateTime *             g_date_time_new_from_unix_utc                   (gint64          t);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_DEPRECATED_IN_2_62_FOR(g_date_time_new_from_unix_local)
GDateTime *             g_date_time_new_from_timeval_local              (const GTimeVal *tv);
XPL_DEPRECATED_IN_2_62_FOR(g_date_time_new_from_unix_utc)
GDateTime *             g_date_time_new_from_timeval_utc                (const GTimeVal *tv);
G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_2_56
GDateTime *             g_date_time_new_from_iso8601                    (const xchar_t    *text,
                                                                         GTimeZone      *default_tz);

XPL_AVAILABLE_IN_ALL
GDateTime *             g_date_time_new                                 (GTimeZone      *tz,
                                                                         xint_t            year,
                                                                         xint_t            month,
                                                                         xint_t            day,
                                                                         xint_t            hour,
                                                                         xint_t            minute,
                                                                         xdouble_t         seconds);
XPL_AVAILABLE_IN_ALL
GDateTime *             g_date_time_new_local                           (xint_t            year,
                                                                         xint_t            month,
                                                                         xint_t            day,
                                                                         xint_t            hour,
                                                                         xint_t            minute,
                                                                         xdouble_t         seconds);
XPL_AVAILABLE_IN_ALL
GDateTime *             g_date_time_new_utc                             (xint_t            year,
                                                                         xint_t            month,
                                                                         xint_t            day,
                                                                         xint_t            hour,
                                                                         xint_t            minute,
                                                                         xdouble_t         seconds);

XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
GDateTime *             g_date_time_add                                 (GDateTime      *datetime,
                                                                         GTimeSpan       timespan);

XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
GDateTime *             g_date_time_add_years                           (GDateTime      *datetime,
                                                                         xint_t            years);
XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
GDateTime *             g_date_time_add_months                          (GDateTime      *datetime,
                                                                         xint_t            months);
XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
GDateTime *             g_date_time_add_weeks                           (GDateTime      *datetime,
                                                                         xint_t            weeks);
XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
GDateTime *             g_date_time_add_days                            (GDateTime      *datetime,
                                                                         xint_t            days);

XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
GDateTime *             g_date_time_add_hours                           (GDateTime      *datetime,
                                                                         xint_t            hours);
XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
GDateTime *             g_date_time_add_minutes                         (GDateTime      *datetime,
                                                                         xint_t            minutes);
XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
GDateTime *             g_date_time_add_seconds                         (GDateTime      *datetime,
                                                                         xdouble_t         seconds);

XPL_AVAILABLE_IN_ALL
G_GNUC_WARN_UNUSED_RESULT
GDateTime *             g_date_time_add_full                            (GDateTime      *datetime,
                                                                         xint_t            years,
                                                                         xint_t            months,
                                                                         xint_t            days,
                                                                         xint_t            hours,
                                                                         xint_t            minutes,
                                                                         xdouble_t         seconds);

XPL_AVAILABLE_IN_ALL
xint_t                    g_date_time_compare                             (gconstpointer   dt1,
                                                                         gconstpointer   dt2);
XPL_AVAILABLE_IN_ALL
GTimeSpan               g_date_time_difference                          (GDateTime      *end,
                                                                         GDateTime      *begin);
XPL_AVAILABLE_IN_ALL
xuint_t                   g_date_time_hash                                (gconstpointer   datetime);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_date_time_equal                               (gconstpointer   dt1,
                                                                         gconstpointer   dt2);

XPL_AVAILABLE_IN_ALL
void                    g_date_time_get_ymd                             (GDateTime      *datetime,
                                                                         xint_t           *year,
                                                                         xint_t           *month,
                                                                         xint_t           *day);

XPL_AVAILABLE_IN_ALL
xint_t                    g_date_time_get_year                            (GDateTime      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    g_date_time_get_month                           (GDateTime      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    g_date_time_get_day_of_month                    (GDateTime      *datetime);

XPL_AVAILABLE_IN_ALL
xint_t                    g_date_time_get_week_numbering_year             (GDateTime      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    g_date_time_get_week_of_year                    (GDateTime      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    g_date_time_get_day_of_week                     (GDateTime      *datetime);

XPL_AVAILABLE_IN_ALL
xint_t                    g_date_time_get_day_of_year                     (GDateTime      *datetime);

XPL_AVAILABLE_IN_ALL
xint_t                    g_date_time_get_hour                            (GDateTime      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    g_date_time_get_minute                          (GDateTime      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    g_date_time_get_second                          (GDateTime      *datetime);
XPL_AVAILABLE_IN_ALL
xint_t                    g_date_time_get_microsecond                     (GDateTime      *datetime);
XPL_AVAILABLE_IN_ALL
xdouble_t                 g_date_time_get_seconds                         (GDateTime      *datetime);

XPL_AVAILABLE_IN_ALL
gint64                  g_date_time_to_unix                             (GDateTime      *datetime);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
XPL_DEPRECATED_IN_2_62_FOR(g_date_time_to_unix)
xboolean_t                g_date_time_to_timeval                          (GDateTime      *datetime,
                                                                         GTimeVal       *tv);
G_GNUC_END_IGNORE_DEPRECATIONS

XPL_AVAILABLE_IN_ALL
GTimeSpan               g_date_time_get_utc_offset                      (GDateTime      *datetime);
XPL_AVAILABLE_IN_2_58
GTimeZone *             g_date_time_get_timezone                        (GDateTime      *datetime);
XPL_AVAILABLE_IN_ALL
const xchar_t *           g_date_time_get_timezone_abbreviation           (GDateTime      *datetime);
XPL_AVAILABLE_IN_ALL
xboolean_t                g_date_time_is_daylight_savings                 (GDateTime      *datetime);

XPL_AVAILABLE_IN_ALL
GDateTime *             g_date_time_to_timezone                         (GDateTime      *datetime,
                                                                         GTimeZone      *tz);
XPL_AVAILABLE_IN_ALL
GDateTime *             g_date_time_to_local                            (GDateTime      *datetime);
XPL_AVAILABLE_IN_ALL
GDateTime *             g_date_time_to_utc                              (GDateTime      *datetime);

XPL_AVAILABLE_IN_ALL
xchar_t *                 g_date_time_format                              (GDateTime      *datetime,
                                                                         const xchar_t    *format) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_2_62
xchar_t *                 g_date_time_format_iso8601                      (GDateTime      *datetime) G_GNUC_MALLOC;

G_END_DECLS

#endif /* __G_DATE_TIME_H__ */
