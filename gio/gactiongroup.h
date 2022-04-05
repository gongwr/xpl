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

#ifndef __XACTION_GROUP_H__
#define __XACTION_GROUP_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS


#define XTYPE_ACTION_GROUP                                 (xaction_group_get_type ())
#define XACTION_GROUP(inst)                                (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_ACTION_GROUP, xaction_group_t))
#define X_IS_ACTION_GROUP(inst)                             (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_ACTION_GROUP))
#define XACTION_GROUP_GET_IFACE(inst)                      (XTYPE_INSTANCE_GET_INTERFACE ((inst),                  \
                                                             XTYPE_ACTION_GROUP, xaction_group_interface_t))

typedef struct _xaction_group_interface                       xaction_group_interface_t;

struct _xaction_group_interface
{
  xtype_interface_t x_iface;

  /* virtual functions */
  xboolean_t              (* has_action)                 (xaction_group_t  *action_group,
                                                        const xchar_t   *action_name);

  xchar_t **              (* list_actions)               (xaction_group_t  *action_group);

  xboolean_t              (* get_action_enabled)         (xaction_group_t  *action_group,
                                                        const xchar_t   *action_name);

  const xvariant_type_t *  (* get_action_parameter_type)  (xaction_group_t  *action_group,
                                                        const xchar_t   *action_name);

  const xvariant_type_t *  (* get_action_state_type)      (xaction_group_t  *action_group,
                                                        const xchar_t   *action_name);

  xvariant_t *            (* get_action_state_hint)      (xaction_group_t  *action_group,
                                                        const xchar_t   *action_name);

  xvariant_t *            (* get_action_state)           (xaction_group_t  *action_group,
                                                        const xchar_t   *action_name);

  void                  (* change_action_state)        (xaction_group_t  *action_group,
                                                        const xchar_t   *action_name,
                                                        xvariant_t      *value);

  void                  (* activate_action)            (xaction_group_t  *action_group,
                                                        const xchar_t   *action_name,
                                                        xvariant_t      *parameter);

  /* signals */
  void                  (* action_added)               (xaction_group_t  *action_group,
                                                        const xchar_t   *action_name);
  void                  (* action_removed)             (xaction_group_t  *action_group,
                                                        const xchar_t   *action_name);
  void                  (* action_enabled_changed)     (xaction_group_t  *action_group,
                                                        const xchar_t   *action_name,
                                                        xboolean_t       enabled);
  void                  (* action_state_changed)       (xaction_group_t   *action_group,
                                                        const xchar_t    *action_name,
                                                        xvariant_t       *state);

  /* more virtual functions */
  xboolean_t              (* query_action)               (xaction_group_t        *action_group,
                                                        const xchar_t         *action_name,
                                                        xboolean_t            *enabled,
                                                        const xvariant_type_t **parameter_type,
                                                        const xvariant_type_t **state_type,
                                                        xvariant_t           **state_hint,
                                                        xvariant_t           **state);
};

XPL_AVAILABLE_IN_ALL
xtype_t                   xaction_group_get_type                         (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xboolean_t                xaction_group_has_action                       (xaction_group_t *action_group,
                                                                         const xchar_t  *action_name);
XPL_AVAILABLE_IN_ALL
xchar_t **                xaction_group_list_actions                     (xaction_group_t *action_group);

XPL_AVAILABLE_IN_ALL
const xvariant_type_t *    xaction_group_get_action_parameter_type        (xaction_group_t *action_group,
                                                                         const xchar_t  *action_name);
XPL_AVAILABLE_IN_ALL
const xvariant_type_t *    xaction_group_get_action_state_type            (xaction_group_t *action_group,
                                                                         const xchar_t  *action_name);
XPL_AVAILABLE_IN_ALL
xvariant_t *              xaction_group_get_action_state_hint            (xaction_group_t *action_group,
                                                                         const xchar_t  *action_name);

XPL_AVAILABLE_IN_ALL
xboolean_t                xaction_group_get_action_enabled               (xaction_group_t *action_group,
                                                                         const xchar_t  *action_name);

XPL_AVAILABLE_IN_ALL
xvariant_t *              xaction_group_get_action_state                 (xaction_group_t *action_group,
                                                                         const xchar_t  *action_name);
XPL_AVAILABLE_IN_ALL
void                    xaction_group_change_action_state              (xaction_group_t *action_group,
                                                                         const xchar_t  *action_name,
                                                                         xvariant_t     *value);

XPL_AVAILABLE_IN_ALL
void                    xaction_group_activate_action                  (xaction_group_t *action_group,
                                                                         const xchar_t  *action_name,
                                                                         xvariant_t     *parameter);

/* signals */
XPL_AVAILABLE_IN_ALL
void                    xaction_group_action_added                     (xaction_group_t *action_group,
                                                                         const xchar_t  *action_name);
XPL_AVAILABLE_IN_ALL
void                    xaction_group_action_removed                   (xaction_group_t *action_group,
                                                                         const xchar_t  *action_name);
XPL_AVAILABLE_IN_ALL
void                    xaction_group_action_enabled_changed           (xaction_group_t *action_group,
                                                                         const xchar_t  *action_name,
                                                                         xboolean_t      enabled);

XPL_AVAILABLE_IN_ALL
void                    xaction_group_action_state_changed             (xaction_group_t *action_group,
                                                                         const xchar_t  *action_name,
                                                                         xvariant_t     *state);

XPL_AVAILABLE_IN_2_32
xboolean_t                xaction_group_query_action                     (xaction_group_t        *action_group,
                                                                         const xchar_t         *action_name,
                                                                         xboolean_t            *enabled,
                                                                         const xvariant_type_t **parameter_type,
                                                                         const xvariant_type_t **state_type,
                                                                         xvariant_t           **state_hint,
                                                                         xvariant_t           **state);

G_END_DECLS

#endif /* __XACTION_GROUP_H__ */
