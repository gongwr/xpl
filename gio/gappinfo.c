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

#include "config.h"

#include "gappinfo.h"
#include "gappinfoprivate.h"
#include "gcontextspecificgroup.h"
#include "gtask.h"
#include "gcancellable.h"

#include "glibintl.h"
#include "gmarshal-internal.h"
#include <gioerror.h>
#include <gfile.h>

#ifdef G_OS_UNIX
#include "gdbusconnection.h"
#include "gdbusmessage.h"
#include "gportalsupport.h"
#include "gunixfdlist.h"
#include "gopenuriportal.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

/**
 * SECTION:gappinfo
 * @short_description: Application information and launch contexts
 * @include: gio/gio.h
 * @see_also: #xapp_info_monitor_t
 *
 * #xapp_info_t and #xapp_launch_context_t are used for describing and launching
 * applications installed on the system.
 *
 * As of GLib 2.20, URIs will always be converted to POSIX paths
 * (using xfile_get_path()) when using xapp_info_launch() even if
 * the application requested an URI and not a POSIX path. For example
 * for a desktop-file based application with Exec key `totem
 * %U` and a single URI, `sftp://foo/file.avi`, then
 * `/home/user/.gvfs/sftp on foo/file.avi` will be passed. This will
 * only work if a set of suitable GIO extensions (such as gvfs 2.26
 * compiled with FUSE support), is available and operational; if this
 * is not the case, the URI will be passed unmodified to the application.
 * Some URIs, such as `mailto:`, of course cannot be mapped to a POSIX
 * path (in gvfs there's no FUSE mount for it); such URIs will be
 * passed unmodified to the application.
 *
 * Specifically for gvfs 2.26 and later, the POSIX URI will be mapped
 * back to the GIO URI in the #xfile_t constructors (since gvfs
 * implements the #xvfs_t extension point). As such, if the application
 * needs to examine the URI, it needs to use xfile_get_uri() or
 * similar on #xfile_t. In other words, an application cannot assume
 * that the URI passed to e.g. xfile_new_for_commandline_arg() is
 * equal to the result of xfile_get_uri(). The following snippet
 * illustrates this:
 *
 * |[
 * xfile_t *f;
 * char *uri;
 *
 * file = xfile_new_for_commandline_arg (uri_from_commandline);
 *
 * uri = xfile_get_uri (file);
 * strcmp (uri, uri_from_commandline) == 0;
 * g_free (uri);
 *
 * if (xfile_has_uri_scheme (file, "cdda"))
 *   {
 *     // do something special with uri
 *   }
 * xobject_unref (file);
 * ]|
 *
 * This code will work when both `cdda://sr0/Track 1.wav` and
 * `/home/user/.gvfs/cdda on sr0/Track 1.wav` is passed to the
 * application. It should be noted that it's generally not safe
 * for applications to rely on the format of a particular URIs.
 * Different launcher applications (e.g. file managers) may have
 * different ideas of what a given URI means.
 */

struct _xapp_launch_context_private {
  char **envp;
};

typedef xapp_info_iface_t xapp_info_interface_t;
G_DEFINE_INTERFACE (xapp_info, xapp_info, XTYPE_OBJECT)

static void
xapp_info_default_init (xapp_info_interface_t *iface)
{
}


/**
 * xapp_info_dup:
 * @appinfo: a #xapp_info_t.
 *
 * Creates a duplicate of a #xapp_info_t.
 *
 * Returns: (transfer full): a duplicate of @appinfo.
 **/
xapp_info_t *
xapp_info_dup (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->dup) (appinfo);
}

/**
 * xapp_info_equal:
 * @appinfo1: the first #xapp_info_t.
 * @appinfo2: the second #xapp_info_t.
 *
 * Checks if two #GAppInfos are equal.
 *
 * Note that the check *may not* compare each individual
 * field, and only does an identity check. In case detecting changes in the
 * contents is needed, program code must additionally compare relevant fields.
 *
 * Returns: %TRUE if @appinfo1 is equal to @appinfo2. %FALSE otherwise.
 **/
xboolean_t
xapp_info_equal (xapp_info_t *appinfo1,
		  xapp_info_t *appinfo2)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo1), FALSE);
  xreturn_val_if_fail (X_IS_APP_INFO (appinfo2), FALSE);

  if (XTYPE_FROM_INSTANCE (appinfo1) != XTYPE_FROM_INSTANCE (appinfo2))
    return FALSE;

  iface = G_APP_INFO_GET_IFACE (appinfo1);

  return (* iface->equal) (appinfo1, appinfo2);
}

/**
 * xapp_info_get_id:
 * @appinfo: a #xapp_info_t.
 *
 * Gets the ID of an application. An id is a string that
 * identifies the application. The exact format of the id is
 * platform dependent. For instance, on Unix this is the
 * desktop file id from the xdg menu specification.
 *
 * Note that the returned ID may be %NULL, depending on how
 * the @appinfo has been constructed.
 *
 * Returns: (nullable): a string containing the application's ID.
 **/
const char *
xapp_info_get_id (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->get_id) (appinfo);
}

/**
 * xapp_info_get_name:
 * @appinfo: a #xapp_info_t.
 *
 * Gets the installed name of the application.
 *
 * Returns: the name of the application for @appinfo.
 **/
const char *
xapp_info_get_name (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->get_name) (appinfo);
}

/**
 * xapp_info_get_display_name:
 * @appinfo: a #xapp_info_t.
 *
 * Gets the display name of the application. The display name is often more
 * descriptive to the user than the name itself.
 *
 * Returns: the display name of the application for @appinfo, or the name if
 * no display name is available.
 *
 * Since: 2.24
 **/
const char *
xapp_info_get_display_name (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->get_display_name == NULL)
    return (* iface->get_name) (appinfo);

  return (* iface->get_display_name) (appinfo);
}

/**
 * xapp_info_get_description:
 * @appinfo: a #xapp_info_t.
 *
 * Gets a human-readable description of an installed application.
 *
 * Returns: (nullable): a string containing a description of the
 * application @appinfo, or %NULL if none.
 **/
const char *
xapp_info_get_description (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->get_description) (appinfo);
}

/**
 * xapp_info_get_executable: (virtual get_executable)
 * @appinfo: a #xapp_info_t
 *
 * Gets the executable's name for the installed application.
 *
 * Returns: (type filename): a string containing the @appinfo's application
 * binaries name
 **/
const char *
xapp_info_get_executable (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->get_executable) (appinfo);
}


/**
 * xapp_info_get_commandline: (virtual get_commandline)
 * @appinfo: a #xapp_info_t
 *
 * Gets the commandline with which the application will be
 * started.
 *
 * Returns: (nullable) (type filename): a string containing the @appinfo's commandline,
 *     or %NULL if this information is not available
 *
 * Since: 2.20
 **/
const char *
xapp_info_get_commandline (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->get_commandline)
    return (* iface->get_commandline) (appinfo);

  return NULL;
}

/**
 * xapp_info_set_as_default_for_type:
 * @appinfo: a #xapp_info_t.
 * @content_type: the content type.
 * @error: a #xerror_t.
 *
 * Sets the application as the default handler for a given type.
 *
 * Returns: %TRUE on success, %FALSE on error.
 **/
xboolean_t
xapp_info_set_as_default_for_type (xapp_info_t    *appinfo,
				    const char  *content_type,
				    xerror_t     **error)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);
  xreturn_val_if_fail (content_type != NULL, FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->set_as_default_for_type)
    return (* iface->set_as_default_for_type) (appinfo, content_type, error);

  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       _("Setting default applications not supported yet"));
  return FALSE;
}

/**
 * xapp_info_set_as_last_used_for_type:
 * @appinfo: a #xapp_info_t.
 * @content_type: the content type.
 * @error: a #xerror_t.
 *
 * Sets the application as the last used application for a given type.
 * This will make the application appear as first in the list returned
 * by xapp_info_get_recommended_for_type(), regardless of the default
 * application for that content type.
 *
 * Returns: %TRUE on success, %FALSE on error.
 **/
xboolean_t
xapp_info_set_as_last_used_for_type (xapp_info_t    *appinfo,
                                      const char  *content_type,
                                      xerror_t     **error)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);
  xreturn_val_if_fail (content_type != NULL, FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->set_as_last_used_for_type)
    return (* iface->set_as_last_used_for_type) (appinfo, content_type, error);

  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       _("Setting application as last used for type not supported yet"));
  return FALSE;
}

/**
 * xapp_info_set_as_default_for_extension:
 * @appinfo: a #xapp_info_t.
 * @extension: (type filename): a string containing the file extension
 *     (without the dot).
 * @error: a #xerror_t.
 *
 * Sets the application as the default handler for the given file extension.
 *
 * Returns: %TRUE on success, %FALSE on error.
 **/
xboolean_t
xapp_info_set_as_default_for_extension (xapp_info_t    *appinfo,
					 const char  *extension,
					 xerror_t     **error)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);
  xreturn_val_if_fail (extension != NULL, FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->set_as_default_for_extension)
    return (* iface->set_as_default_for_extension) (appinfo, extension, error);

  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                       "xapp_info_set_as_default_for_extension not supported yet");
  return FALSE;
}


/**
 * xapp_info_add_supports_type:
 * @appinfo: a #xapp_info_t.
 * @content_type: a string.
 * @error: a #xerror_t.
 *
 * Adds a content type to the application information to indicate the
 * application is capable of opening files with the given content type.
 *
 * Returns: %TRUE on success, %FALSE on error.
 **/
xboolean_t
xapp_info_add_supports_type (xapp_info_t    *appinfo,
			      const char  *content_type,
			      xerror_t     **error)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);
  xreturn_val_if_fail (content_type != NULL, FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->add_supports_type)
    return (* iface->add_supports_type) (appinfo, content_type, error);

  g_set_error_literal (error, G_IO_ERROR,
                       G_IO_ERROR_NOT_SUPPORTED,
                       "xapp_info_add_supports_type not supported yet");

  return FALSE;
}


/**
 * xapp_info_can_remove_supports_type:
 * @appinfo: a #xapp_info_t.
 *
 * Checks if a supported content type can be removed from an application.
 *
 * Returns: %TRUE if it is possible to remove supported
 *     content types from a given @appinfo, %FALSE if not.
 **/
xboolean_t
xapp_info_can_remove_supports_type (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->can_remove_supports_type)
    return (* iface->can_remove_supports_type) (appinfo);

  return FALSE;
}


/**
 * xapp_info_remove_supports_type:
 * @appinfo: a #xapp_info_t.
 * @content_type: a string.
 * @error: a #xerror_t.
 *
 * Removes a supported type from an application, if possible.
 *
 * Returns: %TRUE on success, %FALSE on error.
 **/
xboolean_t
xapp_info_remove_supports_type (xapp_info_t    *appinfo,
				 const char  *content_type,
				 xerror_t     **error)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);
  xreturn_val_if_fail (content_type != NULL, FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->remove_supports_type)
    return (* iface->remove_supports_type) (appinfo, content_type, error);

  g_set_error_literal (error, G_IO_ERROR,
                       G_IO_ERROR_NOT_SUPPORTED,
                       "xapp_info_remove_supports_type not supported yet");

  return FALSE;
}

/**
 * xapp_info_get_supported_types:
 * @appinfo: a #xapp_info_t that can handle files
 *
 * Retrieves the list of content types that @app_info claims to support.
 * If this information is not provided by the environment, this function
 * will return %NULL.
 * This function does not take in consideration associations added with
 * xapp_info_add_supports_type(), but only those exported directly by
 * the application.
 *
 * Returns: (transfer none) (array zero-terminated=1) (element-type utf8):
 *    a list of content types.
 *
 * Since: 2.34
 */
const char **
xapp_info_get_supported_types (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->get_supported_types)
    return iface->get_supported_types (appinfo);
  else
    return NULL;
}


/**
 * xapp_info_get_icon:
 * @appinfo: a #xapp_info_t.
 *
 * Gets the icon for the application.
 *
 * Returns: (nullable) (transfer none): the default #xicon_t for @appinfo or %NULL
 * if there is no default icon.
 **/
xicon_t *
xapp_info_get_icon (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), NULL);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->get_icon) (appinfo);
}


/**
 * xapp_info_launch:
 * @appinfo: a #xapp_info_t
 * @files: (nullable) (element-type xfile_t): a #xlist_t of #xfile_t objects
 * @context: (nullable): a #xapp_launch_context_t or %NULL
 * @error: a #xerror_t
 *
 * Launches the application. Passes @files to the launched application
 * as arguments, using the optional @context to get information
 * about the details of the launcher (like what screen it is on).
 * On error, @error will be set accordingly.
 *
 * To launch the application without arguments pass a %NULL @files list.
 *
 * Note that even if the launch is successful the application launched
 * can fail to start if it runs into problems during startup. There is
 * no way to detect this.
 *
 * Some URIs can be changed when passed through a xfile_t (for instance
 * unsupported URIs with strange formats like mailto:), so if you have
 * a textual URI you want to pass in as argument, consider using
 * xapp_info_launch_uris() instead.
 *
 * The launched application inherits the environment of the launching
 * process, but it can be modified with xapp_launch_context_setenv()
 * and xapp_launch_context_unsetenv().
 *
 * On UNIX, this function sets the `GIO_LAUNCHED_DESKTOP_FILE`
 * environment variable with the path of the launched desktop file and
 * `GIO_LAUNCHED_DESKTOP_FILE_PID` to the process id of the launched
 * process. This can be used to ignore `GIO_LAUNCHED_DESKTOP_FILE`,
 * should it be inherited by further processes. The `DISPLAY` and
 * `DESKTOP_STARTUP_ID` environment variables are also set, based
 * on information provided in @context.
 *
 * Returns: %TRUE on successful launch, %FALSE otherwise.
 **/
xboolean_t
xapp_info_launch (xapp_info_t           *appinfo,
		   xlist_t              *files,
		   xapp_launch_context_t  *launch_context,
		   xerror_t            **error)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->launch) (appinfo, files, launch_context, error);
}


/**
 * xapp_info_supports_uris:
 * @appinfo: a #xapp_info_t.
 *
 * Checks if the application supports reading files and directories from URIs.
 *
 * Returns: %TRUE if the @appinfo supports URIs.
 **/
xboolean_t
xapp_info_supports_uris (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->supports_uris) (appinfo);
}


/**
 * xapp_info_supports_files:
 * @appinfo: a #xapp_info_t.
 *
 * Checks if the application accepts files as arguments.
 *
 * Returns: %TRUE if the @appinfo supports files.
 **/
xboolean_t
xapp_info_supports_files (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->supports_files) (appinfo);
}


/**
 * xapp_info_launch_uris:
 * @appinfo: a #xapp_info_t
 * @uris: (nullable) (element-type utf8): a #xlist_t containing URIs to launch.
 * @context: (nullable): a #xapp_launch_context_t or %NULL
 * @error: a #xerror_t
 *
 * Launches the application. This passes the @uris to the launched application
 * as arguments, using the optional @context to get information
 * about the details of the launcher (like what screen it is on).
 * On error, @error will be set accordingly.
 *
 * To launch the application without arguments pass a %NULL @uris list.
 *
 * Note that even if the launch is successful the application launched
 * can fail to start if it runs into problems during startup. There is
 * no way to detect this.
 *
 * Returns: %TRUE on successful launch, %FALSE otherwise.
 **/
xboolean_t
xapp_info_launch_uris (xapp_info_t           *appinfo,
			xlist_t              *uris,
			xapp_launch_context_t  *launch_context,
			xerror_t            **error)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->launch_uris) (appinfo, uris, launch_context, error);
}

/**
 * xapp_info_launch_uris_async:
 * @appinfo: a #xapp_info_t
 * @uris: (nullable) (element-type utf8): a #xlist_t containing URIs to launch.
 * @context: (nullable): a #xapp_launch_context_t or %NULL
 * @cancellable: (nullable): a #xcancellable_t
 * @callback: (nullable): a #xasync_ready_callback_t to call when the request is done
 * @user_data: (nullable): data to pass to @callback
 *
 * Async version of xapp_info_launch_uris().
 *
 * The @callback is invoked immediately after the application launch, but it
 * waits for activation in case of D-Bus–activated applications and also provides
 * extended error information for sandboxed applications, see notes for
 * xapp_info_launch_default_for_uri_async().
 *
 * Since: 2.60
 **/
void
xapp_info_launch_uris_async (xapp_info_t           *appinfo,
                              xlist_t              *uris,
                              xapp_launch_context_t  *context,
                              xcancellable_t       *cancellable,
                              xasync_ready_callback_t callback,
                              xpointer_t            user_data)
{
  xapp_info_iface_t *iface;

  g_return_if_fail (X_IS_APP_INFO (appinfo));
  g_return_if_fail (context == NULL || X_IS_APP_LAUNCH_CONTEXT (context));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  iface = G_APP_INFO_GET_IFACE (appinfo);
  if (iface->launch_uris_async == NULL)
    {
      xtask_t *task;

      task = xtask_new (appinfo, cancellable, callback, user_data);
      xtask_set_source_tag (task, xapp_info_launch_uris_async);
      xtask_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               "Operation not supported for the current backend.");
      xobject_unref (task);

      return;
    }

  (* iface->launch_uris_async) (appinfo, uris, context, cancellable, callback, user_data);
}

/**
 * xapp_info_launch_uris_finish:
 * @appinfo: a #xapp_info_t
 * @result: a #xasync_result_t
 * @error: (nullable): a #xerror_t
 *
 * Finishes a xapp_info_launch_uris_async() operation.
 *
 * Returns: %TRUE on successful launch, %FALSE otherwise.
 *
 * Since: 2.60
 */
xboolean_t
xapp_info_launch_uris_finish (xapp_info_t     *appinfo,
                               xasync_result_t *result,
                               xerror_t      **error)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);
  if (iface->launch_uris_finish == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                           "Operation not supported for the current backend.");
      return FALSE;
    }

  return (* iface->launch_uris_finish) (appinfo, result, error);
}

/**
 * xapp_info_should_show:
 * @appinfo: a #xapp_info_t.
 *
 * Checks if the application info should be shown in menus that
 * list available applications.
 *
 * Returns: %TRUE if the @appinfo should be shown, %FALSE otherwise.
 **/
xboolean_t
xapp_info_should_show (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  return (* iface->should_show) (appinfo);
}

/**
 * xapp_info_launch_default_for_uri:
 * @uri: the uri to show
 * @context: (nullable): an optional #xapp_launch_context_t
 * @error: (nullable): return location for an error, or %NULL
 *
 * Utility function that launches the default application
 * registered to handle the specified uri. Synchronous I/O
 * is done on the uri to detect the type of the file if
 * required.
 *
 * The D-Bus–activated applications don't have to be started if your application
 * terminates too soon after this function. To prevent this, use
 * xapp_info_launch_default_for_uri_async() instead.
 *
 * Returns: %TRUE on success, %FALSE on error.
 **/
xboolean_t
xapp_info_launch_default_for_uri (const char         *uri,
                                   xapp_launch_context_t  *launch_context,
                                   xerror_t            **error)
{
  char *uri_scheme;
  xapp_info_t *app_info = NULL;
  xboolean_t res = FALSE;

  /* xfile_query_default_handler() calls
   * xapp_info_get_default_for_uri_scheme() too, but we have to do it
   * here anyway in case xfile_t can't parse @uri correctly.
   */
  uri_scheme = xuri_parse_scheme (uri);
  if (uri_scheme && uri_scheme[0] != '\0')
    app_info = xapp_info_get_default_for_uri_scheme (uri_scheme);
  g_free (uri_scheme);

  if (!app_info)
    {
      xfile_t *file;

      file = xfile_new_for_uri (uri);
      app_info = xfile_query_default_handler (file, NULL, error);
      xobject_unref (file);
    }

  if (app_info)
    {
      xlist_t l;

      l.data = (char *)uri;
      l.next = l.prev = NULL;
      res = xapp_info_launch_uris (app_info, &l, launch_context, error);
      xobject_unref (app_info);
    }

#ifdef G_OS_UNIX
  if (!res && glib_should_use_portal ())
    {
      const char *parent_window = NULL;

      /* Reset any error previously set by launch_default_for_uri */
      g_clear_error (error);

      if (launch_context && launch_context->priv->envp)
        parent_window = g_environ_getenv (launch_context->priv->envp, "PARENT_WINDOW_ID");

      return g_openuri_portal_open_uri (uri, parent_window, error);
    }
#endif

  return res;
}

typedef struct
{
  xchar_t *uri;
  xapp_launch_context_t *context;
} LaunchDefaultForUriData;

static void
launch_default_for_uri_data_free (LaunchDefaultForUriData *data)
{
  g_free (data->uri);
  g_clear_object (&data->context);
  g_free (data);
}

#ifdef G_OS_UNIX
static void
launch_default_for_uri_portal_open_uri_cb (xobject_t      *object,
                                           xasync_result_t *result,
                                           xpointer_t      user_data)
{
  xtask_t *task = XTASK (user_data);
  xerror_t *error = NULL;

  if (g_openuri_portal_open_uri_finish (result, &error))
    xtask_return_boolean (task, TRUE);
  else
    xtask_return_error (task, g_steal_pointer (&error));
  xobject_unref (task);
}
#endif

static void
launch_default_for_uri_portal_open_uri (xtask_t *task, xerror_t *error)
{
#ifdef G_OS_UNIX
  LaunchDefaultForUriData *data = xtask_get_task_data (task);
  xcancellable_t *cancellable = xtask_get_cancellable (task);

  if (glib_should_use_portal ())
    {
      const char *parent_window = NULL;

      /* Reset any error previously set by launch_default_for_uri */
      xerror_free (error);

      if (data->context && data->context->priv->envp)
        parent_window = g_environ_getenv (data->context->priv->envp,
                                          "PARENT_WINDOW_ID");

      g_openuri_portal_open_uri_async (data->uri,
                                       parent_window,
                                       cancellable,
                                       launch_default_for_uri_portal_open_uri_cb,
                                       g_steal_pointer (&task));
      return;
    }
#endif

  xtask_return_error (task, g_steal_pointer (&error));
  xobject_unref (task);
}

static void
launch_default_for_uri_launch_uris_cb (xobject_t      *object,
                                       xasync_result_t *result,
                                       xpointer_t      user_data)
{
  xapp_info_t *app_info = G_APP_INFO (object);
  xtask_t *task = XTASK (user_data);
  xerror_t *error = NULL;

  if (xapp_info_launch_uris_finish (app_info, result, &error))
    {
      xtask_return_boolean (task, TRUE);
      xobject_unref (task);
    }
  else
    launch_default_for_uri_portal_open_uri (g_steal_pointer (&task), g_steal_pointer (&error));
}

static void
launch_default_for_uri_launch_uris (xtask_t *task,
                                    xapp_info_t *app_info)
{
  xcancellable_t *cancellable = xtask_get_cancellable (task);
  xlist_t l;
  LaunchDefaultForUriData *data = xtask_get_task_data (task);

  l.data = (char *)data->uri;
  l.next = l.prev = NULL;
  xapp_info_launch_uris_async (app_info,
                                &l,
                                data->context,
                                cancellable,
                                launch_default_for_uri_launch_uris_cb,
                                g_steal_pointer (&task));
  xobject_unref (app_info);
}

static void
launch_default_for_uri_default_handler_cb (xobject_t      *object,
                                           xasync_result_t *result,
                                           xpointer_t      user_data)
{
  xfile_t *file = XFILE (object);
  xtask_t *task = XTASK (user_data);
  xapp_info_t *app_info = NULL;
  xerror_t *error = NULL;

  app_info = xfile_query_default_handler_finish (file, result, &error);
  if (app_info)
    launch_default_for_uri_launch_uris (g_steal_pointer (&task), g_steal_pointer (&app_info));
  else
    launch_default_for_uri_portal_open_uri (g_steal_pointer (&task), g_steal_pointer (&error));
}

/**
 * xapp_info_launch_default_for_uri_async:
 * @uri: the uri to show
 * @context: (nullable): an optional #xapp_launch_context_t
 * @cancellable: (nullable): a #xcancellable_t
 * @callback: (nullable): a #xasync_ready_callback_t to call when the request is done
 * @user_data: (nullable): data to pass to @callback
 *
 * Async version of xapp_info_launch_default_for_uri().
 *
 * This version is useful if you are interested in receiving
 * error information in the case where the application is
 * sandboxed and the portal may present an application chooser
 * dialog to the user.
 *
 * This is also useful if you want to be sure that the D-Bus–activated
 * applications are really started before termination and if you are interested
 * in receiving error information from their activation.
 *
 * Since: 2.50
 */
void
xapp_info_launch_default_for_uri_async (const char          *uri,
                                         xapp_launch_context_t   *context,
                                         xcancellable_t        *cancellable,
                                         xasync_ready_callback_t  callback,
                                         xpointer_t             user_data)
{
  xtask_t *task;
  char *uri_scheme;
  xapp_info_t *app_info = NULL;
  LaunchDefaultForUriData *data;

  g_return_if_fail (uri != NULL);

  task = xtask_new (NULL, cancellable, callback, user_data);
  xtask_set_source_tag (task, xapp_info_launch_default_for_uri_async);

  data = g_new (LaunchDefaultForUriData, 1);
  data->uri = xstrdup (uri);
  data->context = (context != NULL) ? xobject_ref (context) : NULL;
  xtask_set_task_data (task, g_steal_pointer (&data), (xdestroy_notify_t) launch_default_for_uri_data_free);

  /* xfile_query_default_handler_async() calls
   * xapp_info_get_default_for_uri_scheme() too, but we have to do it
   * here anyway in case xfile_t can't parse @uri correctly.
   */
  uri_scheme = xuri_parse_scheme (uri);
  if (uri_scheme && uri_scheme[0] != '\0')
    /* FIXME: The following still uses blocking calls. */
    app_info = xapp_info_get_default_for_uri_scheme (uri_scheme);
  g_free (uri_scheme);

  if (!app_info)
    {
      xfile_t *file;

      file = xfile_new_for_uri (uri);
      xfile_query_default_handler_async (file,
                                          G_PRIORITY_DEFAULT,
                                          cancellable,
                                          launch_default_for_uri_default_handler_cb,
                                          g_steal_pointer (&task));
      xobject_unref (file);
    }
  else
    launch_default_for_uri_launch_uris (g_steal_pointer (&task), g_steal_pointer (&app_info));
}

/**
 * xapp_info_launch_default_for_uri_finish:
 * @result: a #xasync_result_t
 * @error: (nullable): return location for an error, or %NULL
 *
 * Finishes an asynchronous launch-default-for-uri operation.
 *
 * Returns: %TRUE if the launch was successful, %FALSE if @error is set
 *
 * Since: 2.50
 */
xboolean_t
xapp_info_launch_default_for_uri_finish (xasync_result_t  *result,
                                          xerror_t       **error)
{
  xreturn_val_if_fail (xtask_is_valid (result, NULL), FALSE);

  return xtask_propagate_boolean (XTASK (result), error);
}

/**
 * xapp_info_can_delete:
 * @appinfo: a #xapp_info_t
 *
 * Obtains the information whether the #xapp_info_t can be deleted.
 * See xapp_info_delete().
 *
 * Returns: %TRUE if @appinfo can be deleted
 *
 * Since: 2.20
 */
xboolean_t
xapp_info_can_delete (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->can_delete)
    return (* iface->can_delete) (appinfo);

  return FALSE;
}


/**
 * xapp_info_delete:
 * @appinfo: a #xapp_info_t
 *
 * Tries to delete a #xapp_info_t.
 *
 * On some platforms, there may be a difference between user-defined
 * #GAppInfos which can be deleted, and system-wide ones which cannot.
 * See xapp_info_can_delete().
 *
 * Virtual: do_delete
 * Returns: %TRUE if @appinfo has been deleted
 *
 * Since: 2.20
 */
xboolean_t
xapp_info_delete (xapp_info_t *appinfo)
{
  xapp_info_iface_t *iface;

  xreturn_val_if_fail (X_IS_APP_INFO (appinfo), FALSE);

  iface = G_APP_INFO_GET_IFACE (appinfo);

  if (iface->do_delete)
    return (* iface->do_delete) (appinfo);

  return FALSE;
}


enum {
  LAUNCH_FAILED,
  LAUNCH_STARTED,
  LAUNCHED,
  LAST_SIGNAL
};

static xuint_t signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (xapp_launch_context_t, xapp_launch_context, XTYPE_OBJECT)

/**
 * xapp_launch_context_new:
 *
 * Creates a new application launch context. This is not normally used,
 * instead you instantiate a subclass of this, such as #GdkAppLaunchContext.
 *
 * Returns: a #xapp_launch_context_t.
 **/
xapp_launch_context_t *
xapp_launch_context_new (void)
{
  return xobject_new (XTYPE_APP_LAUNCH_CONTEXT, NULL);
}

static void
xapp_launch_context_finalize (xobject_t *object)
{
  xapp_launch_context_t *context = G_APP_LAUNCH_CONTEXT (object);

  xstrfreev (context->priv->envp);

  XOBJECT_CLASS (xapp_launch_context_parent_class)->finalize (object);
}

static void
xapp_launch_context_class_init (xapp_launch_context_class_t *klass)
{
  xobject_class_t *object_class = XOBJECT_CLASS (klass);

  object_class->finalize = xapp_launch_context_finalize;

  /**
   * xapp_launch_context_t::launch-failed:
   * @context: the object emitting the signal
   * @startup_notify_id: the startup notification id for the failed launch
   *
   * The #xapp_launch_context_t::launch-failed signal is emitted when a #xapp_info_t launch
   * fails. The startup notification id is provided, so that the launcher
   * can cancel the startup notification.
   *
   * Since: 2.36
   */
  signals[LAUNCH_FAILED] = xsignal_new (I_("launch-failed"),
                                         G_OBJECT_CLASS_TYPE (object_class),
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (xapp_launch_context_class_t, launch_failed),
                                         NULL, NULL, NULL,
                                         XTYPE_NONE, 1, XTYPE_STRING);

  /**
   * xapp_launch_context_t::launch-started:
   * @context: the object emitting the signal
   * @info: the #xapp_info_t that is about to be launched
   * @platform_data: (nullable): additional platform-specific data for this launch
   *
   * The #xapp_launch_context_t::launch-started signal is emitted when a #xapp_info_t is
   * about to be launched. If non-null the @platform_data is an
   * xvariant_t dictionary mapping strings to variants (ie `a{sv}`), which
   * contains additional, platform-specific data about this launch. On
   * UNIX, at least the `startup-notification-id` keys will be
   * present.
   *
   * The value of the `startup-notification-id` key (type `s`) is a startup
   * notification ID corresponding to the format from the [startup-notification
   * specification](https://specifications.freedesktop.org/startup-notification-spec/startup-notification-0.1.txt).
   * It allows tracking the progress of the launchee through startup.
   *
   * It is guaranteed that this signal is followed by either a #xapp_launch_context_t::launched or
   * #xapp_launch_context_t::launch-failed signal.
   *
   * Since: 2.72
   */
  signals[LAUNCH_STARTED] = xsignal_new (I_("launch-started"),
                                          G_OBJECT_CLASS_TYPE (object_class),
                                          G_SIGNAL_RUN_LAST,
                                          G_STRUCT_OFFSET (xapp_launch_context_class_t, launch_started),
                                          NULL, NULL,
                                          _g_cclosure_marshal_VOID__OBJECT_VARIANT,
                                          XTYPE_NONE, 2,
                                          XTYPE_APP_INFO, XTYPE_VARIANT);
  xsignal_set_va_marshaller (signals[LAUNCH_STARTED],
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_VOID__OBJECT_VARIANTv);

  /**
   * xapp_launch_context_t::launched:
   * @context: the object emitting the signal
   * @info: the #xapp_info_t that was just launched
   * @platform_data: additional platform-specific data for this launch
   *
   * The #xapp_launch_context_t::launched signal is emitted when a #xapp_info_t is successfully
   * launched. The @platform_data is an xvariant_t dictionary mapping
   * strings to variants (ie `a{sv}`), which contains additional,
   * platform-specific data about this launch. On UNIX, at least the
   * `pid` and `startup-notification-id` keys will be present.
   *
   * Since 2.72 the `pid` may be 0 if the process id wasn't known (for
   * example if the process was launched via D-Bus). The `pid` may not be
   * set at all in subsequent releases.
   *
   * Since: 2.36
   */
  signals[LAUNCHED] = xsignal_new (I_("launched"),
                                    G_OBJECT_CLASS_TYPE (object_class),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (xapp_launch_context_class_t, launched),
                                    NULL, NULL,
                                    _g_cclosure_marshal_VOID__OBJECT_VARIANT,
                                    XTYPE_NONE, 2,
                                    XTYPE_APP_INFO, XTYPE_VARIANT);
  xsignal_set_va_marshaller (signals[LAUNCHED],
                              XTYPE_FROM_CLASS (klass),
                              _g_cclosure_marshal_VOID__OBJECT_VARIANTv);
}

static void
xapp_launch_context_init (xapp_launch_context_t *context)
{
  context->priv = xapp_launch_context_get_instance_private (context);
}

/**
 * xapp_launch_context_setenv:
 * @context: a #xapp_launch_context_t
 * @variable: (type filename): the environment variable to set
 * @value: (type filename): the value for to set the variable to.
 *
 * Arranges for @variable to be set to @value in the child's
 * environment when @context is used to launch an application.
 *
 * Since: 2.32
 */
void
xapp_launch_context_setenv (xapp_launch_context_t *context,
                             const char        *variable,
                             const char        *value)
{
  g_return_if_fail (X_IS_APP_LAUNCH_CONTEXT (context));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (value != NULL);

  if (!context->priv->envp)
    context->priv->envp = g_get_environ ();

  context->priv->envp =
    g_environ_setenv (context->priv->envp, variable, value, TRUE);
}

/**
 * xapp_launch_context_unsetenv:
 * @context: a #xapp_launch_context_t
 * @variable: (type filename): the environment variable to remove
 *
 * Arranges for @variable to be unset in the child's environment
 * when @context is used to launch an application.
 *
 * Since: 2.32
 */
void
xapp_launch_context_unsetenv (xapp_launch_context_t *context,
                               const char        *variable)
{
  g_return_if_fail (X_IS_APP_LAUNCH_CONTEXT (context));
  g_return_if_fail (variable != NULL);

  if (!context->priv->envp)
    context->priv->envp = g_get_environ ();

  context->priv->envp =
    g_environ_unsetenv (context->priv->envp, variable);
}

/**
 * xapp_launch_context_get_environment:
 * @context: a #xapp_launch_context_t
 *
 * Gets the complete environment variable list to be passed to
 * the child process when @context is used to launch an application.
 * This is a %NULL-terminated array of strings, where each string has
 * the form `KEY=VALUE`.
 *
 * Returns: (array zero-terminated=1) (element-type filename) (transfer full):
 *     the child's environment
 *
 * Since: 2.32
 */
char **
xapp_launch_context_get_environment (xapp_launch_context_t *context)
{
  xreturn_val_if_fail (X_IS_APP_LAUNCH_CONTEXT (context), NULL);

  if (!context->priv->envp)
    context->priv->envp = g_get_environ ();

  return xstrdupv (context->priv->envp);
}

/**
 * xapp_launch_context_get_display:
 * @context: a #xapp_launch_context_t
 * @info: a #xapp_info_t
 * @files: (element-type xfile_t): a #xlist_t of #xfile_t objects
 *
 * Gets the display string for the @context. This is used to ensure new
 * applications are started on the same display as the launching
 * application, by setting the `DISPLAY` environment variable.
 *
 * Returns: (nullable): a display string for the display.
 */
char *
xapp_launch_context_get_display (xapp_launch_context_t *context,
				  xapp_info_t          *info,
				  xlist_t             *files)
{
  xapp_launch_context_class_t *class;

  xreturn_val_if_fail (X_IS_APP_LAUNCH_CONTEXT (context), NULL);
  xreturn_val_if_fail (X_IS_APP_INFO (info), NULL);

  class = G_APP_LAUNCH_CONTEXT_GET_CLASS (context);

  if (class->get_display == NULL)
    return NULL;

  return class->get_display (context, info, files);
}

/**
 * xapp_launch_context_get_startup_notify_id:
 * @context: a #xapp_launch_context_t
 * @info: a #xapp_info_t
 * @files: (element-type xfile_t): a #xlist_t of of #xfile_t objects
 *
 * Initiates startup notification for the application and returns the
 * `DESKTOP_STARTUP_ID` for the launched operation, if supported.
 *
 * Startup notification IDs are defined in the
 * [FreeDesktop.Org Startup Notifications standard](http://standards.freedesktop.org/startup-notification-spec/startup-notification-latest.txt).
 *
 * Returns: (nullable): a startup notification ID for the application, or %NULL if
 *     not supported.
 **/
char *
xapp_launch_context_get_startup_notify_id (xapp_launch_context_t *context,
					    xapp_info_t          *info,
					    xlist_t             *files)
{
  xapp_launch_context_class_t *class;

  xreturn_val_if_fail (X_IS_APP_LAUNCH_CONTEXT (context), NULL);
  xreturn_val_if_fail (X_IS_APP_INFO (info), NULL);

  class = G_APP_LAUNCH_CONTEXT_GET_CLASS (context);

  if (class->get_startup_notify_id == NULL)
    return NULL;

  return class->get_startup_notify_id (context, info, files);
}


/**
 * xapp_launch_context_launch_failed:
 * @context: a #xapp_launch_context_t.
 * @startup_notify_id: the startup notification id that was returned by xapp_launch_context_get_startup_notify_id().
 *
 * Called when an application has failed to launch, so that it can cancel
 * the application startup notification started in xapp_launch_context_get_startup_notify_id().
 *
 **/
void
xapp_launch_context_launch_failed (xapp_launch_context_t *context,
				    const char        *startup_notify_id)
{
  g_return_if_fail (X_IS_APP_LAUNCH_CONTEXT (context));
  g_return_if_fail (startup_notify_id != NULL);

  xsignal_emit (context, signals[LAUNCH_FAILED], 0, startup_notify_id);
}


/**
 * SECTION:gappinfomonitor
 * @short_description: Monitor application information for changes
 *
 * #xapp_info_monitor_t is a very simple object used for monitoring the app
 * info database for changes (ie: newly installed or removed
 * applications).
 *
 * Call xapp_info_monitor_get() to get a #xapp_info_monitor_t and connect
 * to the "changed" signal.
 *
 * In the usual case, applications should try to make note of the change
 * (doing things like invalidating caches) but not act on it.  In
 * particular, applications should avoid making calls to #xapp_info_t APIs
 * in response to the change signal, deferring these until the time that
 * the data is actually required.  The exception to this case is when
 * application information is actually being displayed on the screen
 * (eg: during a search or when the list of all applications is shown).
 * The reason for this is that changes to the list of installed
 * applications often come in groups (like during system updates) and
 * rescanning the list on every change is pointless and expensive.
 *
 * Since: 2.40
 **/

/**
 * xapp_info_monitor_t:
 *
 * The only thing you can do with this is to get it via
 * xapp_info_monitor_get() and connect to the "changed" signal.
 *
 * Since: 2.40
 **/

typedef struct _GAppInfoMonitorClass GAppInfoMonitorClass;

struct _GAppInfoMonitor
{
  xobject_t parent_instance;
  xmain_context_t *context;
};

struct _GAppInfoMonitorClass
{
  xobject_class_t parent_class;
};

static xcontext_specific_group_t xapp_info_monitor_group;
static xuint_t                 xapp_info_monitor_changed_signal;

XDEFINE_TYPE (xapp_info_monitor, xapp_info_monitor, XTYPE_OBJECT)

static void
xapp_info_monitor_finalize (xobject_t *object)
{
  xapp_info_monitor_t *monitor = G_APP_INFO_MONITOR (object);

  xcontext_specific_group_remove (&xapp_info_monitor_group, monitor->context, monitor, NULL);

  XOBJECT_CLASS (xapp_info_monitor_parent_class)->finalize (object);
}

static void
xapp_info_monitor_init (xapp_info_monitor_t *monitor)
{
}

static void
xapp_info_monitor_class_init (GAppInfoMonitorClass *class)
{
  xobject_class_t *object_class = XOBJECT_CLASS (class);

  /**
   * xapp_info_monitor_t::changed:
   *
   * Signal emitted when the app info database for changes (ie: newly installed
   * or removed applications).
   **/
  xapp_info_monitor_changed_signal = xsignal_new (I_("changed"), XTYPE_APP_INFO_MONITOR, G_SIGNAL_RUN_FIRST,
                                                    0, NULL, NULL, g_cclosure_marshal_VOID__VOID, XTYPE_NONE, 0);

  object_class->finalize = xapp_info_monitor_finalize;
}

/**
 * xapp_info_monitor_get:
 *
 * Gets the #xapp_info_monitor_t for the current thread-default main
 * context.
 *
 * The #xapp_info_monitor_t will emit a "changed" signal in the
 * thread-default main context whenever the list of installed
 * applications (as reported by xapp_info_get_all()) may have changed.
 *
 * You must only call xobject_unref() on the return value from under
 * the same main context as you created it.
 *
 * Returns: (transfer full): a reference to a #xapp_info_monitor_t
 *
 * Since: 2.40
 **/
xapp_info_monitor_t *
xapp_info_monitor_get (void)
{
  return xcontext_specific_group_get (&xapp_info_monitor_group,
                                       XTYPE_APP_INFO_MONITOR,
                                       G_STRUCT_OFFSET (xapp_info_monitor_t, context),
                                       NULL);
}

void
xapp_info_monitor_fire (void)
{
  xcontext_specific_group_emit (&xapp_info_monitor_group, xapp_info_monitor_changed_signal);
}
