/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2021 Red Hat, Inc.
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
 */

#include "config.h"

#include "gpowerprofilemonitor.h"
#include "gpowerprofilemonitorportal.h"
#include "gdbuserror.h"
#include "gdbusproxy.h"
#include "ginitable.h"
#include "gioerror.h"
#include "giomodule-priv.h"
#include "gportalsupport.h"

#define G_POWER_PROFILE_MONITOR_PORTAL_GET_INITABLE_IFACE(o) (XTYPE_INSTANCE_GET_INTERFACE ((o), XTYPE_INITABLE, xinitable_t))

static void g_power_profile_monitor_portal_iface_init (xpower_profile_monitor_tInterface *iface);
static void g_power_profile_monitor_portal_initable_iface_init (xinitable_iface_t *iface);

typedef enum
{
  PROP_POWER_SAVER_ENABLED = 1,
} xpower_profile_monitor_portal_tProperty;

struct _xpower_profile_monitor_portal_t
{
  xobject_t parent_instance;

  xdbus_proxy_t *proxy;
  gulong signal_id;
  xboolean_t power_saver_enabled;
};

G_DEFINE_TYPE_WITH_CODE (xpower_profile_monitor_portal_t, g_power_profile_monitor_portal, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                g_power_profile_monitor_portal_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_POWER_PROFILE_MONITOR,
                                                g_power_profile_monitor_portal_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_POWER_PROFILE_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "portal",
                                                         40))

static void
g_power_profile_monitor_portal_init (xpower_profile_monitor_portal_t *portal)
{
}

static void
proxy_properties_changed (xdbus_proxy_t *proxy,
                          xvariant_t   *changed_properties,
                          xstrv_t       invalidated_properties,
                          xpointer_t    user_data)
{
  xpower_profile_monitor_portal_t *ppm = user_data;
  xboolean_t power_saver_enabled;

  if (!xvariant_lookup (changed_properties, "power-saver-enabled", "b", &power_saver_enabled))
    return;

  if (power_saver_enabled == ppm->power_saver_enabled)
    return;

  ppm->power_saver_enabled = power_saver_enabled;
  xobject_notify (G_OBJECT (ppm), "power-saver-enabled");
}

static void
g_power_profile_monitor_portal_get_property (xobject_t    *object,
                                             xuint_t       prop_id,
                                             xvalue_t     *value,
                                             xparam_spec_t *pspec)
{
  xpower_profile_monitor_portal_t *ppm = G_POWER_PROFILE_MONITOR_PORTAL (object);

  switch ((xpower_profile_monitor_portal_tProperty) prop_id)
    {
    case PROP_POWER_SAVER_ENABLED:
      xvalue_set_boolean (value, ppm->power_saver_enabled);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static xboolean_t
g_power_profile_monitor_portal_initable_init (xinitable_t     *initable,
                                              xcancellable_t  *cancellable,
                                              xerror_t       **error)
{
  xpower_profile_monitor_portal_t *ppm = G_POWER_PROFILE_MONITOR_PORTAL (initable);
  xdbus_proxy_t *proxy;
  xchar_t *name_owner;
  xvariant_t *power_saver_enabled_v = NULL;

  if (!glib_should_use_portal ())
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Not using portals");
      return FALSE;
    }

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         "org.freedesktop.portal.Desktop",
                                         "/org/freedesktop/portal/desktop",
                                         "org.freedesktop.portal.PowerProfileMonitor",
                                         cancellable,
                                         error);
  if (!proxy)
    return FALSE;

  name_owner = g_dbus_proxy_get_name_owner (proxy);

  if (name_owner == NULL)
    {
      xobject_unref (proxy);
      g_set_error (error,
                   G_DBUS_ERROR,
                   G_DBUS_ERROR_NAME_HAS_NO_OWNER,
                   "Desktop portal not found");
      return FALSE;
    }

  g_free (name_owner);

  ppm->signal_id = g_signal_connect (proxy, "g-properties-changed",
                                     G_CALLBACK (proxy_properties_changed), ppm);

  power_saver_enabled_v = g_dbus_proxy_get_cached_property (proxy, "power-saver-enabled");
  if (power_saver_enabled_v != NULL &&
      xvariant_is_of_type (power_saver_enabled_v, G_VARIANT_TYPE_BOOLEAN))
    ppm->power_saver_enabled = xvariant_get_boolean (power_saver_enabled_v);
  g_clear_pointer (&power_saver_enabled_v, xvariant_unref);

  ppm->proxy = g_steal_pointer (&proxy);

  return TRUE;
}

static void
g_power_profile_monitor_portal_finalize (xobject_t *object)
{
  xpower_profile_monitor_portal_t *ppm = G_POWER_PROFILE_MONITOR_PORTAL (object);

  g_clear_signal_handler (&ppm->signal_id, ppm->proxy);
  g_clear_object (&ppm->proxy);

  G_OBJECT_CLASS (g_power_profile_monitor_portal_parent_class)->finalize (object);
}

static void
g_power_profile_monitor_portal_class_init (xpower_profile_monitor_portal_tClass *nl_class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (nl_class);

  gobject_class->get_property = g_power_profile_monitor_portal_get_property;
  gobject_class->finalize  = g_power_profile_monitor_portal_finalize;

  xobject_class_override_property (gobject_class, PROP_POWER_SAVER_ENABLED, "power-saver-enabled");
}

static void
g_power_profile_monitor_portal_iface_init (xpower_profile_monitor_tInterface *monitor_iface)
{
}

static void
g_power_profile_monitor_portal_initable_iface_init (xinitable_iface_t *iface)
{
  iface->init = g_power_profile_monitor_portal_initable_init;
}
