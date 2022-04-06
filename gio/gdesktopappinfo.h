/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __G_DESKTOP_APP_INFO_H__
#define __G_DESKTOP_APP_INFO_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define XTYPE_DESKTOP_APP_INFO         (g_desktop_app_info_get_type ())
#define G_DESKTOP_APP_INFO(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DESKTOP_APP_INFO, GDesktopAppInfo))
#define G_DESKTOP_APP_INFO_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_DESKTOP_APP_INFO, GDesktopAppInfoClass))
#define X_IS_DESKTOP_APP_INFO(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DESKTOP_APP_INFO))
#define X_IS_DESKTOP_APP_INFO_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DESKTOP_APP_INFO))
#define G_DESKTOP_APP_INFO_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DESKTOP_APP_INFO, GDesktopAppInfoClass))

typedef struct _GDesktopAppInfo        GDesktopAppInfo;
typedef struct _GDesktopAppInfoClass   GDesktopAppInfoClass;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(xdesktop_app_info, xobject_unref)

struct _GDesktopAppInfoClass
{
  xobject_class_t parent_class;
};


XPL_AVAILABLE_IN_ALL
xtype_t            g_desktop_app_info_get_type          (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
GDesktopAppInfo *g_desktop_app_info_new_from_filename (const char      *filename);
XPL_AVAILABLE_IN_ALL
GDesktopAppInfo *g_desktop_app_info_new_from_keyfile  (xkey_file_t        *key_file);

XPL_AVAILABLE_IN_ALL
const char *     g_desktop_app_info_get_filename      (GDesktopAppInfo *info);

XPL_AVAILABLE_IN_2_30
const char *     g_desktop_app_info_get_generic_name  (GDesktopAppInfo *info);
XPL_AVAILABLE_IN_2_30
const char *     g_desktop_app_info_get_categories    (GDesktopAppInfo *info);
XPL_AVAILABLE_IN_2_30
const char * const *g_desktop_app_info_get_keywords   (GDesktopAppInfo *info);
XPL_AVAILABLE_IN_2_30
xboolean_t         g_desktop_app_info_get_nodisplay     (GDesktopAppInfo *info);
XPL_AVAILABLE_IN_2_30
xboolean_t         g_desktop_app_info_get_show_in       (GDesktopAppInfo *info,
                                                       const xchar_t     *desktop_env);
XPL_AVAILABLE_IN_2_34
const char *     g_desktop_app_info_get_startup_wm_class (GDesktopAppInfo *info);

XPL_AVAILABLE_IN_ALL
GDesktopAppInfo *g_desktop_app_info_new               (const char      *desktop_id);
XPL_AVAILABLE_IN_ALL
xboolean_t         g_desktop_app_info_get_is_hidden     (GDesktopAppInfo *info);

XPL_DEPRECATED_IN_2_42
void             g_desktop_app_info_set_desktop_env   (const char      *desktop_env);

XPL_AVAILABLE_IN_2_36
xboolean_t         g_desktop_app_info_has_key           (GDesktopAppInfo *info,
                                                       const char      *key);
XPL_AVAILABLE_IN_2_36
char *           g_desktop_app_info_get_string        (GDesktopAppInfo *info,
                                                       const char      *key);
XPL_AVAILABLE_IN_2_56
char *           g_desktop_app_info_get_locale_string (GDesktopAppInfo *info,
                                                       const char      *key);
XPL_AVAILABLE_IN_2_36
xboolean_t         g_desktop_app_info_get_boolean       (GDesktopAppInfo *info,
                                                       const char      *key);

XPL_AVAILABLE_IN_2_60
xchar_t **         g_desktop_app_info_get_string_list (GDesktopAppInfo *info,
                                                     const char      *key,
                                                     xsize_t           *length);

XPL_AVAILABLE_IN_2_38
const xchar_t * const *   g_desktop_app_info_list_actions                 (GDesktopAppInfo   *info);

XPL_AVAILABLE_IN_2_38
void                    g_desktop_app_info_launch_action                (GDesktopAppInfo   *info,
                                                                         const xchar_t       *action_name,
                                                                         xapp_launch_context_t *launch_context);

XPL_AVAILABLE_IN_2_38
xchar_t *                 g_desktop_app_info_get_action_name              (GDesktopAppInfo   *info,
                                                                         const xchar_t       *action_name);

#define XTYPE_DESKTOP_APP_INFO_LOOKUP           (g_desktop_app_info_lookup_get_type ()) XPL_DEPRECATED_MACRO_IN_2_28
#define G_DESKTOP_APP_INFO_LOOKUP(obj)           (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_DESKTOP_APP_INFO_LOOKUP, GDesktopAppInfoLookup)) XPL_DEPRECATED_MACRO_IN_2_28
#define X_IS_DESKTOP_APP_INFO_LOOKUP(obj)	 (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_DESKTOP_APP_INFO_LOOKUP)) XPL_DEPRECATED_MACRO_IN_2_28
#define G_DESKTOP_APP_INFO_LOOKUP_GET_IFACE(obj) (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_DESKTOP_APP_INFO_LOOKUP, GDesktopAppInfoLookupIface)) XPL_DEPRECATED_MACRO_IN_2_28

/**
 * G_DESKTOP_APP_INFO_LOOKUP_EXTENSION_POINT_NAME:
 *
 * Extension point for default handler to URI association. See
 * [Extending GIO][extending-gio].
 *
 * Deprecated: 2.28: The #GDesktopAppInfoLookup interface is deprecated and
 *    unused by GIO.
 */
#define G_DESKTOP_APP_INFO_LOOKUP_EXTENSION_POINT_NAME "gio-desktop-app-info-lookup" XPL_DEPRECATED_MACRO_IN_2_28

/**
 * GDesktopAppInfoLookupIface:
 * @get_default_for_uri_scheme: Virtual method for
 *  g_desktop_app_info_lookup_get_default_for_uri_scheme().
 *
 * Interface that is used by backends to associate default
 * handlers with URI schemes.
 */
typedef struct _GDesktopAppInfoLookup GDesktopAppInfoLookup;
typedef struct _GDesktopAppInfoLookupIface GDesktopAppInfoLookupIface;

struct _GDesktopAppInfoLookupIface
{
  xtype_interface_t x_iface;

  xapp_info_t * (* get_default_for_uri_scheme) (GDesktopAppInfoLookup *lookup,
                                             const char            *uri_scheme);
};

XPL_DEPRECATED
xtype_t     g_desktop_app_info_lookup_get_type                   (void) G_GNUC_CONST;

XPL_DEPRECATED
xapp_info_t *g_desktop_app_info_lookup_get_default_for_uri_scheme (GDesktopAppInfoLookup *lookup,
                                                                const char            *uri_scheme);

/**
 * GDesktopAppLaunchCallback:
 * @appinfo: a #GDesktopAppInfo
 * @pid: Process identifier
 * @user_data: User data
 *
 * During invocation, g_desktop_app_info_launch_uris_as_manager() may
 * create one or more child processes.  This callback is invoked once
 * for each, providing the process ID.
 */
typedef void (*GDesktopAppLaunchCallback) (GDesktopAppInfo  *appinfo,
					   xpid_t              pid,
					   xpointer_t          user_data);

XPL_AVAILABLE_IN_2_28
xboolean_t    g_desktop_app_info_launch_uris_as_manager (GDesktopAppInfo            *appinfo,
						       xlist_t                      *uris,
						       xapp_launch_context_t          *launch_context,
						       GSpawnFlags                 spawn_flags,
						       GSpawnChildSetupFunc        user_setup,
						       xpointer_t                    user_setup_data,
						       GDesktopAppLaunchCallback   pid_callback,
						       xpointer_t                    pid_callback_data,
						       xerror_t                    **error);

XPL_AVAILABLE_IN_2_58
xboolean_t    g_desktop_app_info_launch_uris_as_manager_with_fds (GDesktopAppInfo            *appinfo,
								xlist_t                      *uris,
								xapp_launch_context_t          *launch_context,
								GSpawnFlags                 spawn_flags,
								GSpawnChildSetupFunc        user_setup,
								xpointer_t                    user_setup_data,
								GDesktopAppLaunchCallback   pid_callback,
								xpointer_t                    pid_callback_data,
								xint_t                        stdin_fd,
								xint_t                        stdout_fd,
								xint_t                        stderr_fd,
								xerror_t                    **error);

XPL_AVAILABLE_IN_2_40
xchar_t *** g_desktop_app_info_search (const xchar_t *search_string);

XPL_AVAILABLE_IN_2_42
xlist_t *g_desktop_app_info_get_implementations (const xchar_t *interface);

G_END_DECLS

#endif /* __G_DESKTOP_APP_INFO_H__ */
