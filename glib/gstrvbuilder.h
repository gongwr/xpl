/*
 * Copyright © 2020 Canonical Ltd.
 * Copyright © 2021 Alexandros Theodotou
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

#ifndef __G_STRVBUILDER_H__
#define __G_STRVBUILDER_H__

#if !defined(__XPL_H_INSIDE__) && !defined(XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gstrfuncs.h>
#include <glib/gtypes.h>

G_BEGIN_DECLS

/**
 * xstrv_builder_t:
 *
 * A helper object to build a %NULL-terminated string array
 * by appending. See xstrv_builder_new().
 *
 * Since: 2.68
 */
typedef struct _xstrv_builder xstrv_builder_t;

XPL_AVAILABLE_IN_2_68
xstrv_builder_t *xstrv_builder_new (void);

XPL_AVAILABLE_IN_2_68
void xstrv_builder_unref (xstrv_builder_t *builder);

XPL_AVAILABLE_IN_2_68
xstrv_builder_t *xstrv_builder_ref (xstrv_builder_t *builder);

XPL_AVAILABLE_IN_2_68
void xstrv_builder_add (xstrv_builder_t *builder,
                         const char *value);

XPL_AVAILABLE_IN_2_70
void xstrv_builder_addv (xstrv_builder_t *builder,
                          const char **value);

XPL_AVAILABLE_IN_2_70
void xstrv_builder_add_many (xstrv_builder_t *builder,
                              ...) G_GNUC_NULL_TERMINATED;

XPL_AVAILABLE_IN_2_68
xstrv_t xstrv_builder_end (xstrv_builder_t *builder);

G_END_DECLS

#endif /* __G_STRVBUILDER_H__ */
