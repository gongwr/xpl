/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997, 1999  Peter Mattis, Red Hat, Inc.
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
 */

#ifndef __G_PATTERN_H__
#define __G_PATTERN_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS


typedef struct _GPatternSpec    GPatternSpec;

XPL_AVAILABLE_IN_ALL
GPatternSpec* g_pattern_spec_new       (const xchar_t  *pattern);
XPL_AVAILABLE_IN_ALL
void          g_pattern_spec_free      (GPatternSpec *pspec);
XPL_AVAILABLE_IN_2_70
GPatternSpec *g_pattern_spec_copy (GPatternSpec *pspec);
XPL_AVAILABLE_IN_ALL
xboolean_t      g_pattern_spec_equal     (GPatternSpec *pspec1,
					GPatternSpec *pspec2);
XPL_AVAILABLE_IN_2_70
xboolean_t g_pattern_spec_match (GPatternSpec *pspec,
                               xsize_t string_length,
                               const xchar_t *string,
                               const xchar_t *string_reversed);
XPL_AVAILABLE_IN_2_70
xboolean_t g_pattern_spec_match_string (GPatternSpec *pspec,
                                      const xchar_t *string);
XPL_DEPRECATED_IN_2_70_FOR (g_pattern_spec_match)
xboolean_t      g_pattern_match          (GPatternSpec *pspec,
					xuint_t         string_length,
					const xchar_t  *string,
					const xchar_t  *string_reversed);
XPL_DEPRECATED_IN_2_70_FOR (g_pattern_spec_match_string)
xboolean_t      g_pattern_match_string   (GPatternSpec *pspec,
					const xchar_t  *string);
XPL_AVAILABLE_IN_ALL
xboolean_t      g_pattern_match_simple   (const xchar_t  *pattern,
					const xchar_t  *string);

G_END_DECLS

#endif /* __G_PATTERN_H__ */
