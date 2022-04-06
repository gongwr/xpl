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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __G_SIMPLE_ACTION_H__
#define __G_SIMPLE_ACTION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_SIMPLE_ACTION                                (g_simple_action_get_type ())
#define G_SIMPLE_ACTION(inst)                               (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_SIMPLE_ACTION, xsimple_action))
#define X_IS_SIMPLE_ACTION(inst)                            (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_SIMPLE_ACTION))

XPL_AVAILABLE_IN_ALL
xtype_t                   g_simple_action_get_type                        (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xsimple_action_t *         g_simple_action_new                             (const xchar_t        *name,
                                                                         const xvariant_type_t *parameter_type);

XPL_AVAILABLE_IN_ALL
xsimple_action_t *         g_simple_action_new_stateful                    (const xchar_t        *name,
                                                                         const xvariant_type_t *parameter_type,
                                                                         xvariant_t           *state);

XPL_AVAILABLE_IN_ALL
void                    g_simple_action_set_enabled                     (xsimple_action_t      *simple,
                                                                         xboolean_t            enabled);

XPL_AVAILABLE_IN_2_30
void                    g_simple_action_set_state                       (xsimple_action_t      *simple,
                                                                         xvariant_t           *value);

XPL_AVAILABLE_IN_2_44
void                    g_simple_action_set_state_hint                  (xsimple_action_t      *simple,
                                                                         xvariant_t           *state_hint);

G_END_DECLS

#endif /* __G_SIMPLE_ACTION_H__ */
