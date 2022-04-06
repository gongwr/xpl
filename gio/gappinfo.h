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

#ifndef __G_APP_INFO_H__
#define __G_APP_INFO_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_APP_INFO            (xapp_info_get_type ())
#define G_APP_INFO(obj)            (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_APP_INFO, xapp_info_t))
#define X_IS_APP_INFO(obj)         (XTYPE_CHECK_INSTANCE_TYPE ((obj), XTYPE_APP_INFO))
#define G_APP_INFO_GET_IFACE(obj)  (XTYPE_INSTANCE_GET_INTERFACE ((obj), XTYPE_APP_INFO, xapp_info_iface_t))

#define XTYPE_APP_LAUNCH_CONTEXT         (xapp_launch_context_get_type ())
#define G_APP_LAUNCH_CONTEXT(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_APP_LAUNCH_CONTEXT, xapp_launch_context_t))
#define G_APP_LAUNCH_CONTEXT_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_APP_LAUNCH_CONTEXT, xapp_launch_context_class_t))
#define X_IS_APP_LAUNCH_CONTEXT(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_APP_LAUNCH_CONTEXT))
#define X_IS_APP_LAUNCH_CONTEXT_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_APP_LAUNCH_CONTEXT))
#define G_APP_LAUNCH_CONTEXT_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_APP_LAUNCH_CONTEXT, xapp_launch_context_class_t))

typedef struct _GAppLaunchContextClass   xapp_launch_context_class_t;
typedef struct _xapp_launch_context_private xapp_launch_context_private_t;

/**
 * xapp_info_t:
 *
 * Information about an installed application and methods to launch
 * it (with file arguments).
 */

/**
 * xapp_info_iface_t:
 * @x_iface: The parent interface.
 * @dup: Copies a #xapp_info_t.
 * @equal: Checks two #GAppInfos for equality.
 * @get_id: Gets a string identifier for a #xapp_info_t.
 * @get_name: Gets the name of the application for a #xapp_info_t.
 * @get_description: Gets a short description for the application described by the #xapp_info_t.
 * @get_executable: Gets the executable name for the #xapp_info_t.
 * @get_icon: Gets the #xicon_t for the #xapp_info_t.
 * @launch: Launches an application specified by the #xapp_info_t.
 * @supports_uris: Indicates whether the application specified supports launching URIs.
 * @supports_files: Indicates whether the application specified accepts filename arguments.
 * @launch_uris: Launches an application with a list of URIs.
 * @should_show: Returns whether an application should be shown (e.g. when getting a list of installed applications).
 * [FreeDesktop.Org Startup Notification Specification](http://standards.freedesktop.org/startup-notification-spec/startup-notification-latest.txt).
 * @set_as_default_for_type: Sets an application as default for a given content type.
 * @set_as_default_for_extension: Sets an application as default for a given file extension.
 * @add_supports_type: Adds to the #xapp_info_t information about supported file types.
 * @can_remove_supports_type: Checks for support for removing supported file types from a #xapp_info_t.
 * @remove_supports_type: Removes a supported application type from a #xapp_info_t.
 * @can_delete: Checks if a #xapp_info_t can be deleted. Since 2.20
 * @do_delete: Deletes a #xapp_info_t. Since 2.20
 * @get_commandline: Gets the commandline for the #xapp_info_t. Since 2.20
 * @get_display_name: Gets the display name for the #xapp_info_t. Since 2.24
 * @set_as_last_used_for_type: Sets the application as the last used. See xapp_info_set_as_last_used_for_type().
 * @get_supported_types: Retrieves the list of content types that @app_info claims to support.
 * @launch_uris_async: Asynchronously launches an application with a list of URIs. (Since: 2.60)
 * @launch_uris_finish: Finishes an operation started with @launch_uris_async. (Since: 2.60)

 * Application Information interface, for operating system portability.
 */
typedef struct _xapp_info_iface    xapp_info_iface_t;

struct _xapp_info_iface
{
  xtype_interface_t x_iface;

  /* Virtual Table */

  xapp_info_t *   (* dup)                          (xapp_info_t           *appinfo);
  xboolean_t     (* equal)                        (xapp_info_t           *appinfo1,
                                                 xapp_info_t           *appinfo2);
  const char * (* get_id)                       (xapp_info_t           *appinfo);
  const char * (* get_name)                     (xapp_info_t           *appinfo);
  const char * (* get_description)              (xapp_info_t           *appinfo);
  const char * (* get_executable)               (xapp_info_t           *appinfo);
  xicon_t *      (* get_icon)                     (xapp_info_t           *appinfo);
  xboolean_t     (* launch)                       (xapp_info_t           *appinfo,
                                                 xlist_t              *files,
                                                 xapp_launch_context_t  *context,
                                                 xerror_t            **error);
  xboolean_t     (* supports_uris)                (xapp_info_t           *appinfo);
  xboolean_t     (* supports_files)               (xapp_info_t           *appinfo);
  xboolean_t     (* launch_uris)                  (xapp_info_t           *appinfo,
                                                 xlist_t              *uris,
                                                 xapp_launch_context_t  *context,
                                                 xerror_t            **error);
  xboolean_t     (* should_show)                  (xapp_info_t           *appinfo);

  /* For changing associations */
  xboolean_t     (* set_as_default_for_type)      (xapp_info_t           *appinfo,
                                                 const char         *content_type,
                                                 xerror_t            **error);
  xboolean_t     (* set_as_default_for_extension) (xapp_info_t           *appinfo,
                                                 const char         *extension,
                                                 xerror_t            **error);
  xboolean_t     (* add_supports_type)            (xapp_info_t           *appinfo,
                                                 const char         *content_type,
                                                 xerror_t            **error);
  xboolean_t     (* can_remove_supports_type)     (xapp_info_t           *appinfo);
  xboolean_t     (* remove_supports_type)         (xapp_info_t           *appinfo,
                                                 const char         *content_type,
                                                 xerror_t            **error);
  xboolean_t     (* can_delete)                   (xapp_info_t           *appinfo);
  xboolean_t     (* do_delete)                    (xapp_info_t           *appinfo);
  const char * (* get_commandline)              (xapp_info_t           *appinfo);
  const char * (* get_display_name)             (xapp_info_t           *appinfo);
  xboolean_t     (* set_as_last_used_for_type)    (xapp_info_t           *appinfo,
                                                 const char         *content_type,
                                                 xerror_t            **error);
  const char ** (* get_supported_types)         (xapp_info_t           *appinfo);
  void         (* launch_uris_async)            (xapp_info_t           *appinfo,
                                                 xlist_t              *uris,
                                                 xapp_launch_context_t  *context,
                                                 xcancellable_t       *cancellable,
                                                 xasync_ready_callback_t callback,
                                                 xpointer_t            user_data);
  xboolean_t     (* launch_uris_finish)           (xapp_info_t           *appinfo,
                                                 xasync_result_t       *result,
                                                 xerror_t            **error);
};

XPL_AVAILABLE_IN_ALL
xtype_t       xapp_info_get_type                     (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xapp_info_t *  xapp_info_create_from_commandline      (const char           *commandline,
                                                     const char           *application_name,
                                                     GAppInfoCreateFlags   flags,
                                                     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xapp_info_t *  xapp_info_dup                          (xapp_info_t             *appinfo);
XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_equal                        (xapp_info_t             *appinfo1,
                                                     xapp_info_t             *appinfo2);
XPL_AVAILABLE_IN_ALL
const char *xapp_info_get_id                       (xapp_info_t             *appinfo);
XPL_AVAILABLE_IN_ALL
const char *xapp_info_get_name                     (xapp_info_t             *appinfo);
XPL_AVAILABLE_IN_ALL
const char *xapp_info_get_display_name             (xapp_info_t             *appinfo);
XPL_AVAILABLE_IN_ALL
const char *xapp_info_get_description              (xapp_info_t             *appinfo);
XPL_AVAILABLE_IN_ALL
const char *xapp_info_get_executable               (xapp_info_t             *appinfo);
XPL_AVAILABLE_IN_ALL
const char *xapp_info_get_commandline              (xapp_info_t             *appinfo);
XPL_AVAILABLE_IN_ALL
xicon_t *     xapp_info_get_icon                     (xapp_info_t             *appinfo);
XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_launch                       (xapp_info_t             *appinfo,
                                                     xlist_t                *files,
                                                     xapp_launch_context_t    *context,
                                                     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_supports_uris                (xapp_info_t             *appinfo);
XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_supports_files               (xapp_info_t             *appinfo);
XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_launch_uris                  (xapp_info_t             *appinfo,
                                                     xlist_t                *uris,
                                                     xapp_launch_context_t    *context,
                                                     xerror_t              **error);
XPL_AVAILABLE_IN_2_60
void        xapp_info_launch_uris_async            (xapp_info_t             *appinfo,
                                                     xlist_t                *uris,
                                                     xapp_launch_context_t    *context,
                                                     xcancellable_t         *cancellable,
                                                     xasync_ready_callback_t   callback,
                                                     xpointer_t              user_data);
XPL_AVAILABLE_IN_2_60
xboolean_t    xapp_info_launch_uris_finish           (xapp_info_t             *appinfo,
                                                     xasync_result_t         *result,
                                                     xerror_t              **error);

XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_should_show                  (xapp_info_t             *appinfo);

XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_set_as_default_for_type      (xapp_info_t             *appinfo,
                                                     const char           *content_type,
                                                     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_set_as_default_for_extension (xapp_info_t             *appinfo,
                                                     const char           *extension,
                                                     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_add_supports_type            (xapp_info_t             *appinfo,
                                                     const char           *content_type,
                                                     xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_can_remove_supports_type     (xapp_info_t             *appinfo);
XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_remove_supports_type         (xapp_info_t             *appinfo,
                                                     const char           *content_type,
                                                     xerror_t              **error);
XPL_AVAILABLE_IN_2_34
const char **xapp_info_get_supported_types         (xapp_info_t             *appinfo);

XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_can_delete                   (xapp_info_t   *appinfo);
XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_delete                       (xapp_info_t   *appinfo);

XPL_AVAILABLE_IN_ALL
xboolean_t    xapp_info_set_as_last_used_for_type    (xapp_info_t             *appinfo,
                                                     const char           *content_type,
                                                     xerror_t              **error);

XPL_AVAILABLE_IN_ALL
xlist_t *   xapp_info_get_all                     (void);
XPL_AVAILABLE_IN_ALL
xlist_t *   xapp_info_get_all_for_type            (const char  *content_type);
XPL_AVAILABLE_IN_ALL
xlist_t *   xapp_info_get_recommended_for_type    (const xchar_t *content_type);
XPL_AVAILABLE_IN_ALL
xlist_t *   xapp_info_get_fallback_for_type       (const xchar_t *content_type);

XPL_AVAILABLE_IN_ALL
void      xapp_info_reset_type_associations     (const char  *content_type);
XPL_AVAILABLE_IN_ALL
xapp_info_t *xapp_info_get_default_for_type        (const char  *content_type,
                                                  xboolean_t     must_support_uris);
XPL_AVAILABLE_IN_ALL
xapp_info_t *xapp_info_get_default_for_uri_scheme  (const char  *uri_scheme);

XPL_AVAILABLE_IN_ALL
xboolean_t  xapp_info_launch_default_for_uri      (const char              *uri,
                                                  xapp_launch_context_t       *context,
                                                  xerror_t                 **error);

XPL_AVAILABLE_IN_2_50
void      xapp_info_launch_default_for_uri_async  (const char           *uri,
                                                    xapp_launch_context_t    *context,
                                                    xcancellable_t         *cancellable,
                                                    xasync_ready_callback_t   callback,
                                                    xpointer_t              user_data);
XPL_AVAILABLE_IN_2_50
xboolean_t  xapp_info_launch_default_for_uri_finish (xasync_result_t         *result,
                                                    xerror_t              **error);


/**
 * xapp_launch_context_t:
 *
 * Integrating the launch with the launching application. This is used to
 * handle for instance startup notification and launching the new application
 * on the same screen as the launching window.
 */
struct _GAppLaunchContext
{
  xobject_t parent_instance;

  /*< private >*/
  xapp_launch_context_private_t *priv;
};

struct _GAppLaunchContextClass
{
  xobject_class_t parent_class;

  char * (* get_display)           (xapp_launch_context_t *context,
                                    xapp_info_t          *info,
                                    xlist_t             *files);
  char * (* get_startup_notify_id) (xapp_launch_context_t *context,
                                    xapp_info_t          *info,
                                    xlist_t             *files);
  void   (* launch_failed)         (xapp_launch_context_t *context,
                                    const char        *startup_notify_id);
  void   (* launched)              (xapp_launch_context_t *context,
                                    xapp_info_t          *info,
                                    xvariant_t          *platform_data);
  void   (* launch_started)        (xapp_launch_context_t *context,
                                    xapp_info_t          *info,
                                    xvariant_t          *platform_data);

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t              xapp_launch_context_get_type              (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xapp_launch_context_t *xapp_launch_context_new                   (void);

XPL_AVAILABLE_IN_2_32
void               xapp_launch_context_setenv                (xapp_launch_context_t *context,
                                                               const char        *variable,
                                                               const char        *value);
XPL_AVAILABLE_IN_2_32
void               xapp_launch_context_unsetenv              (xapp_launch_context_t *context,
                                                               const char        *variable);
XPL_AVAILABLE_IN_2_32
char **            xapp_launch_context_get_environment       (xapp_launch_context_t *context);

XPL_AVAILABLE_IN_ALL
char *             xapp_launch_context_get_display           (xapp_launch_context_t *context,
                                                               xapp_info_t          *info,
                                                               xlist_t             *files);
XPL_AVAILABLE_IN_ALL
char *             xapp_launch_context_get_startup_notify_id (xapp_launch_context_t *context,
                                                               xapp_info_t          *info,
                                                               xlist_t             *files);
XPL_AVAILABLE_IN_ALL
void               xapp_launch_context_launch_failed         (xapp_launch_context_t *context,
                                                               const char *       startup_notify_id);

#define XTYPE_APP_INFO_MONITOR                             (xapp_info_monitor_get_type ())
#define G_APP_INFO_MONITOR(inst)                            (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_APP_INFO_MONITOR, xapp_info_monitor_t))
#define X_IS_APP_INFO_MONITOR(inst)                         (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_APP_INFO_MONITOR))

typedef struct _GAppInfoMonitor                             xapp_info_monitor_t;

XPL_AVAILABLE_IN_2_40
xtype_t                   xapp_info_monitor_get_type                     (void);

XPL_AVAILABLE_IN_2_40
xapp_info_monitor_t *       xapp_info_monitor_get                          (void);

G_END_DECLS

#endif /* __G_APP_INFO_H__ */
