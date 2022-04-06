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

#include "config.h"

#include "gsimplepermission.h"
#include "gpermission.h"


/**
 * SECTION:gsimplepermission
 * @title: xsimple_permission_t
 * @short_description: A xpermission_t that doesn't change value
 * @include: gio/gio.h
 *
 * #xsimple_permission_t is a trivial implementation of #xpermission_t that
 * represents a permission that is either always or never allowed.  The
 * value is given at construction and doesn't change.
 *
 * Calling request or release will result in errors.
 **/

/**
 * xsimple_permission_t:
 *
 * #xsimple_permission_t is an opaque data structure.  There are no methods
 * except for those defined by #xpermission_t.
 **/

typedef GPermissionClass GSimplePermissionClass;

struct _GSimplePermission
{
  xpermission_t parent_instance;
};

G_DEFINE_TYPE (xsimple_permission, g_simple_permission, XTYPE_PERMISSION)

static void
g_simple_permission_init (xsimple_permission_t *simple)
{
}

static void
g_simple_permission_class_init (GSimplePermissionClass *class)
{
}

/**
 * g_simple_permission_new:
 * @allowed: %TRUE if the action is allowed
 *
 * Creates a new #xpermission_t instance that represents an action that is
 * either always or never allowed.
 *
 * Returns: the #xsimple_permission_t, as a #xpermission_t
 *
 * Since: 2.26
 **/
xpermission_t *
g_simple_permission_new (xboolean_t allowed)
{
  xpermission_t *permission = xobject_new (XTYPE_SIMPLE_PERMISSION, NULL);

  g_permission_impl_update (permission, allowed, FALSE, FALSE);

  return permission;
}
