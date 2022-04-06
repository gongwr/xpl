/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2019 Red Hat, Inc.
 * Copyrgith 2021 Igalia S.L.
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
#include "gpowerprofilemonitordbus.h"
#include "gioerror.h"
#include "ginitable.h"
#include "giomodule-priv.h"
#include "glibintl.h"
#include "glib/gstdio.h"
#include "gcancellable.h"
#include "gdbusproxy.h"
#include "gdbusnamewatching.h"

#define G_POWER_PROFILE_MONITOR_DBUS_GET_INITABLE_IFACE(o) (XTYPE_INSTANCE_GET_INTERFACE ((o), XTYPE_INITABLE, xinitable_t))

static void g_power_profile_monitor_dbus_iface_init (xpower_profile_monitor_tInterface *iface);
static void g_power_profile_monitor_dbus_initable_iface_init (xinitable_iface_t *iface);

struct _xpower_profile_monitor_dbus_t
{
  xobject_t parent_instance;

  xuint_t watch_id;
  xcancellable_t *cancellable;
  xdbus_proxy_t *proxy;
  gulong signal_id;

  xboolean_t power_saver_enabled;
};

typedef enum
{
  PROP_POWER_SAVER_ENABLED = 1,
} xpower_profile_monitor_dbus_tProperty;

#define POWERPROFILES_DBUS_NAME "net.hadess.PowerProfiles"
#define POWERPROFILES_DBUS_IFACE "net.hadess.PowerProfiles"
#define POWERPROFILES_DBUS_PATH "/net/hadess/PowerProfiles"

G_DEFINE_TYPE_WITH_CODE (xpower_profile_monitor_dbus_t, g_power_profile_monitor_dbus, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                g_power_profile_monitor_dbus_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_POWER_PROFILE_MONITOR,
                                                g_power_profile_monitor_dbus_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_POWER_PROFILE_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "dbus",
                                                         30))

static void
g_power_profile_monitor_dbus_init (xpower_profile_monitor_dbus_t *dbus)
{
  dbus->power_saver_enabled = FALSE;
}

static void
ppd_properties_changed_cb (xdbus_proxy_t *proxy,
                           xvariant_t   *changed_properties,
                           xstrv_t      *invalidated_properties,
                           xpointer_t    user_data)
{
  xpower_profile_monitor_dbus_t *dbus = user_data;
  const char *active_profile;
  xboolean_t enabled;

  if (!xvariant_lookup (changed_properties, "ActiveProfile", "&s", &active_profile))
    return;

  enabled = xstrcmp0 (active_profile, "power-saver") == 0;
  if (enabled == dbus->power_saver_enabled)
    return;

  dbus->power_saver_enabled = enabled;
  xobject_notify (G_OBJECT (dbus), "power-saver-enabled");
}

static void
ppd_proxy_cb (xobject_t      *source_object,
              xasync_result_t *res,
              xpointer_t      user_data)
{
  xpower_profile_monitor_dbus_t *dbus = user_data;
  xvariant_t *active_profile_variant;
  xdbus_proxy_t *proxy;
  xerror_t *error = NULL;
  const char *active_profile;
  xboolean_t power_saver_enabled;

  proxy = g_dbus_proxy_new_finish (res, &error);
  if (!proxy)
    {
      g_debug ("xpower_profile_monitor_dbus_t: Failed to create PowerProfiles D-Bus proxy: %s",
               error->message);
      xerror_free (error);
      return;
    }

  active_profile_variant = g_dbus_proxy_get_cached_property (proxy, "ActiveProfile");
  if (active_profile_variant != NULL &&
      xvariant_is_of_type (active_profile_variant, G_VARIANT_TYPE_STRING))
    {
      active_profile = xvariant_get_string (active_profile_variant, NULL);
      power_saver_enabled = xstrcmp0 (active_profile, "power-saver") == 0;
      if (power_saver_enabled != dbus->power_saver_enabled)
        {
          dbus->power_saver_enabled = power_saver_enabled;
          xobject_notify (G_OBJECT (dbus), "power-saver-enabled");
        }
    }
  g_clear_pointer (&active_profile_variant, xvariant_unref);

  dbus->signal_id = g_signal_connect (G_OBJECT (proxy), "g-properties-changed",
                                      G_CALLBACK (ppd_properties_changed_cb), dbus);
  dbus->proxy = g_steal_pointer (&proxy);
}

static void
ppd_appeared_cb (xdbus_connection_t *connection,
                 const xchar_t     *name,
                 const xchar_t     *name_owner,
                 xpointer_t         user_data)
{
  xpower_profile_monitor_dbus_t *dbus = user_data;

  g_dbus_proxy_new (connection,
                    G_DBUS_PROXY_FLAGS_NONE,
                    NULL,
                    POWERPROFILES_DBUS_NAME,
                    POWERPROFILES_DBUS_PATH,
                    POWERPROFILES_DBUS_IFACE,
                    dbus->cancellable,
                    ppd_proxy_cb,
                    dbus);
}

static void
ppd_vanished_cb (xdbus_connection_t *connection,
                 const xchar_t     *name,
                 xpointer_t         user_data)
{
  xpower_profile_monitor_dbus_t *dbus = user_data;

  g_clear_signal_handler (&dbus->signal_id, dbus->proxy);
  g_clear_object (&dbus->proxy);

  dbus->power_saver_enabled = FALSE;
  xobject_notify (G_OBJECT (dbus), "power-saver-enabled");
}

static void
g_power_profile_monitor_dbus_get_property (xobject_t    *object,
                                           xuint_t       prop_id,
                                           xvalue_t     *value,
                                           xparam_spec_t *pspec)
{
  xpower_profile_monitor_dbus_t *dbus = G_POWER_PROFILE_MONITOR_DBUS (object);

  switch ((xpower_profile_monitor_dbus_tProperty) prop_id)
    {
    case PROP_POWER_SAVER_ENABLED:
      xvalue_set_boolean (value, dbus->power_saver_enabled);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static xboolean_t
g_power_profile_monitor_dbus_initable_init (xinitable_t     *initable,
                                            xcancellable_t  *cancellable,
                                            xerror_t       **error)
{
  xpower_profile_monitor_dbus_t *dbus = G_POWER_PROFILE_MONITOR_DBUS (initable);

  dbus->cancellable = g_cancellable_new ();
  dbus->watch_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                                     POWERPROFILES_DBUS_NAME,
                                     G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
                                     ppd_appeared_cb,
                                     ppd_vanished_cb,
                                     dbus,
                                     NULL);

  return TRUE;
}

static void
g_power_profile_monitor_dbus_finalize (xobject_t *object)
{
  xpower_profile_monitor_dbus_t *dbus = G_POWER_PROFILE_MONITOR_DBUS (object);

  g_cancellable_cancel (dbus->cancellable);
  g_clear_object (&dbus->cancellable);
  g_clear_signal_handler (&dbus->signal_id, dbus->proxy);
  g_clear_object (&dbus->proxy);
  g_clear_handle_id (&dbus->watch_id, g_bus_unwatch_name);

  G_OBJECT_CLASS (g_power_profile_monitor_dbus_parent_class)->finalize (object);
}

static void
g_power_profile_monitor_dbus_class_init (xpower_profile_monitor_dbus_tClass *nl_class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (nl_class);

  gobject_class->get_property = g_power_profile_monitor_dbus_get_property;
  gobject_class->finalize = g_power_profile_monitor_dbus_finalize;

  xobject_class_override_property (gobject_class, PROP_POWER_SAVER_ENABLED, "power-saver-enabled");
}

static void
g_power_profile_monitor_dbus_iface_init (xpower_profile_monitor_tInterface *monitor_iface)
{
}

static void
g_power_profile_monitor_dbus_initable_iface_init (xinitable_iface_t *iface)
{
  iface->init = g_power_profile_monitor_dbus_initable_init;
}
