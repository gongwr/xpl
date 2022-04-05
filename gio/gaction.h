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

#ifndef __G_ACTION_H__
#define __G_ACTION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_ACTION                                       (g_action_get_type ())
#define G_ACTION(inst)                                      (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_ACTION, GAction))
#define X_IS_ACTION(inst)                                   (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_ACTION))
#define G_ACTION_GET_IFACE(inst)                            (XTYPE_INSTANCE_GET_INTERFACE ((inst),                  \
                                                             XTYPE_ACTION, GActionInterface))

typedef struct _GActionInterface                            GActionInterface;

struct _GActionInterface
{
  xtype_interface_t x_iface;

  /* virtual functions */
  const xchar_t *        (* get_name)             (GAction  *action);
  const xvariant_type_t * (* get_parameter_type)   (GAction  *action);
  const xvariant_type_t * (* get_state_type)       (GAction  *action);
  xvariant_t *           (* get_state_hint)       (GAction  *action);

  xboolean_t             (* get_enabled)          (GAction  *action);
  xvariant_t *           (* get_state)            (GAction  *action);

  void                 (* change_state)         (GAction  *action,
                                                 xvariant_t *value);
  void                 (* activate)             (GAction  *action,
                                                 xvariant_t *parameter);
};

XPL_AVAILABLE_IN_2_30
xtype_t                   g_action_get_type                               (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
const xchar_t *           g_action_get_name                               (GAction            *action);
XPL_AVAILABLE_IN_ALL
const xvariant_type_t *    g_action_get_parameter_type                     (GAction            *action);
XPL_AVAILABLE_IN_ALL
const xvariant_type_t *    g_action_get_state_type                         (GAction            *action);
XPL_AVAILABLE_IN_ALL
xvariant_t *              g_action_get_state_hint                         (GAction            *action);

XPL_AVAILABLE_IN_ALL
xboolean_t                g_action_get_enabled                            (GAction            *action);
XPL_AVAILABLE_IN_ALL
xvariant_t *              g_action_get_state                              (GAction            *action);

XPL_AVAILABLE_IN_ALL
void                    g_action_change_state                           (GAction            *action,
                                                                         xvariant_t           *value);
XPL_AVAILABLE_IN_ALL
void                    g_action_activate                               (GAction            *action,
                                                                         xvariant_t           *parameter);

XPL_AVAILABLE_IN_2_28
xboolean_t                g_action_name_is_valid                          (const xchar_t        *action_name);

XPL_AVAILABLE_IN_2_38
xboolean_t                g_action_parse_detailed_name                    (const xchar_t        *detailed_name,
                                                                         xchar_t             **action_name,
                                                                         xvariant_t          **target_value,
                                                                         xerror_t            **error);

XPL_AVAILABLE_IN_2_38
xchar_t *                 g_action_print_detailed_name                    (const xchar_t        *action_name,
                                                                         xvariant_t           *target_value);

G_END_DECLS

#endif /* __G_ACTION_H__ */
