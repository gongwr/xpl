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

#include "config.h"

#include "gsimpleaction.h"
#include "gactiongroup.h"
#include "gactionmap.h"
#include "gaction.h"

/**
 * SECTION:gremoteactiongroup
 * @title: xremote_action_group_t
 * @short_description: A xaction_group_t that interacts with other processes
 * @include: gio/gio.h
 *
 * The xremote_action_group_t interface is implemented by #xaction_group_t
 * instances that either transmit action invocations to other processes
 * or receive action invocations in the local process from other
 * processes.
 *
 * The interface has `_full` variants of the two
 * methods on #xaction_group_t used to activate actions:
 * xaction_group_activate_action() and
 * xaction_group_change_action_state(). These variants allow a
 * "platform data" #xvariant_t to be specified: a dictionary providing
 * context for the action invocation (for example: timestamps, startup
 * notification IDs, etc).
 *
 * #xdbus_action_group_t implements #xremote_action_group_t.  This provides a
 * mechanism to send platform data for action invocations over D-Bus.
 *
 * Additionally, xdbus_connection_export_action_group() will check if
 * the exported #xaction_group_t implements #xremote_action_group_t and use the
 * `_full` variants of the calls if available.  This
 * provides a mechanism by which to receive platform data for action
 * invocations that arrive by way of D-Bus.
 *
 * Since: 2.32
 **/

/**
 * xremote_action_group_t:
 *
 * #xremote_action_group_t is an opaque data structure and can only be accessed
 * using the following functions.
 **/

/**
 * xremote_action_group_interface_t:
 * @activate_action_full: the virtual function pointer for xremote_action_group_activate_action_full()
 * @change_action_state_full: the virtual function pointer for xremote_action_group_change_action_state_full()
 *
 * The virtual function table for #xremote_action_group_t.
 *
 * Since: 2.32
 **/

#include "config.h"

#include "gremoteactiongroup.h"

G_DEFINE_INTERFACE (xremote_action_group, g_remote_action_group, XTYPE_ACTION_GROUP)

static void
xremote_action_group_default_init (xremote_action_group_interface_t *iface)
{
}

/**
 * xremote_action_group_activate_action_full:
 * @remote: a #xdbus_action_group_t
 * @action_name: the name of the action to activate
 * @parameter: (nullable): the optional parameter to the activation
 * @platform_data: the platform data to send
 *
 * Activates the remote action.
 *
 * This is the same as xaction_group_activate_action() except that it
 * allows for provision of "platform data" to be sent along with the
 * activation request.  This typically contains details such as the user
 * interaction timestamp or startup notification information.
 *
 * @platform_data must be non-%NULL and must have the type
 * %G_VARIANT_TYPE_VARDICT.  If it is floating, it will be consumed.
 *
 * Since: 2.32
 **/
void
xremote_action_group_activate_action_full (xremote_action_group_t *remote,
                                            const xchar_t        *action_name,
                                            xvariant_t           *parameter,
                                            xvariant_t           *platform_data)
{
  G_REMOTE_ACTION_GROUP_GET_IFACE (remote)
    ->activate_action_full (remote, action_name, parameter, platform_data);
}

/**
 * xremote_action_group_change_action_state_full:
 * @remote: a #xremote_action_group_t
 * @action_name: the name of the action to change the state of
 * @value: the new requested value for the state
 * @platform_data: the platform data to send
 *
 * Changes the state of a remote action.
 *
 * This is the same as xaction_group_change_action_state() except that
 * it allows for provision of "platform data" to be sent along with the
 * state change request.  This typically contains details such as the
 * user interaction timestamp or startup notification information.
 *
 * @platform_data must be non-%NULL and must have the type
 * %G_VARIANT_TYPE_VARDICT.  If it is floating, it will be consumed.
 *
 * Since: 2.32
 **/
void
xremote_action_group_change_action_state_full (xremote_action_group_t *remote,
                                                const xchar_t        *action_name,
                                                xvariant_t           *value,
                                                xvariant_t           *platform_data)
{
  G_REMOTE_ACTION_GROUP_GET_IFACE (remote)
    ->change_action_state_full (remote, action_name, value, platform_data);
}
