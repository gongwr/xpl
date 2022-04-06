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

#ifndef __G_APPLICATION_H__
#define __G_APPLICATION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_APPLICATION                                  (xapplication_get_type ())
#define G_APPLICATION(inst)                                 (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_APPLICATION, xapplication_t))
#define G_APPLICATION_CLASS(class)                          (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_APPLICATION, xapplication_class_t))
#define X_IS_APPLICATION(inst)                              (XTYPE_CHECK_INSTANCE_TYPE ((inst), XTYPE_APPLICATION))
#define X_IS_APPLICATION_CLASS(class)                       (XTYPE_CHECK_CLASS_TYPE ((class), XTYPE_APPLICATION))
#define G_APPLICATION_GET_CLASS(inst)                       (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_APPLICATION, xapplication_class_t))

typedef struct _xapplication_private                         xapplication_private_t;
typedef struct _xapplication_class                           xapplication_class_t;

struct _xapplication
{
  /*< private >*/
  xobject_t parent_instance;

  xapplication_private_t *priv;
};

struct _xapplication_class
{
  /*< private >*/
  xobject_class_t parent_class;

  /*< public >*/
  /* signals */
  void                      (* startup)             (xapplication_t              *application);

  void                      (* activate)            (xapplication_t              *application);

  void                      (* open)                (xapplication_t              *application,
                                                     xfile_t                    **files,
                                                     xint_t                       n_files,
                                                     const xchar_t               *hint);

  int                       (* command_line)        (xapplication_t              *application,
                                                     xapplication_command_line_t   *command_line);

  /* vfuncs */

  /**
   * xapplication_class_t::local_command_line:
   * @application: a #xapplication_t
   * @arguments: (inout) (array zero-terminated=1): array of command line arguments
   * @exit_status: (out): exit status to fill after processing the command line.
   *
   * This virtual function is always invoked in the local instance. It
   * gets passed a pointer to a %NULL-terminated copy of @argv and is
   * expected to remove arguments that it handled (shifting up remaining
   * arguments).
   *
   * The last argument to local_command_line() is a pointer to the @status
   * variable which can used to set the exit status that is returned from
   * xapplication_run().
   *
   * See xapplication_run() for more details on #xapplication_t startup.
   *
   * Returns: %TRUE if the commandline has been completely handled
   */
  xboolean_t                  (* local_command_line)  (xapplication_t              *application,
                                                     xchar_t                   ***arguments,
                                                     int                       *exit_status);

  void                      (* before_emit)         (xapplication_t              *application,
                                                     xvariant_t                  *platform_data);
  void                      (* after_emit)          (xapplication_t              *application,
                                                     xvariant_t                  *platform_data);
  void                      (* add_platform_data)   (xapplication_t              *application,
                                                     xvariant_builder_t           *builder);
  void                      (* quit_mainloop)       (xapplication_t              *application);
  void                      (* run_mainloop)        (xapplication_t              *application);
  void                      (* shutdown)            (xapplication_t              *application);

  xboolean_t                  (* dbus_register)       (xapplication_t              *application,
                                                     xdbus_connection_t           *connection,
                                                     const xchar_t               *object_path,
                                                     xerror_t                   **error);
  void                      (* dbus_unregister)     (xapplication_t              *application,
                                                     xdbus_connection_t           *connection,
                                                     const xchar_t               *object_path);
  xint_t                      (* handle_local_options)(xapplication_t              *application,
                                                     xvariant_dict_t              *options);
  xboolean_t                  (* name_lost)           (xapplication_t              *application);

  /*< private >*/
  xpointer_t padding[7];
};

XPL_AVAILABLE_IN_ALL
xtype_t                   xapplication_get_type                          (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xboolean_t                xapplication_id_is_valid                       (const xchar_t              *application_id);

XPL_AVAILABLE_IN_ALL
xapplication_t *          xapplication_new                               (const xchar_t              *application_id,
                                                                         GApplicationFlags         flags);

XPL_AVAILABLE_IN_ALL
const xchar_t *           xapplication_get_application_id                (xapplication_t             *application);
XPL_AVAILABLE_IN_ALL
void                    xapplication_set_application_id                (xapplication_t             *application,
                                                                         const xchar_t              *application_id);

XPL_AVAILABLE_IN_2_34
xdbus_connection_t *       xapplication_get_dbus_connection               (xapplication_t             *application);
XPL_AVAILABLE_IN_2_34
const xchar_t *           xapplication_get_dbus_object_path              (xapplication_t             *application);

XPL_AVAILABLE_IN_ALL
xuint_t                   xapplication_get_inactivity_timeout            (xapplication_t             *application);
XPL_AVAILABLE_IN_ALL
void                    xapplication_set_inactivity_timeout            (xapplication_t             *application,
                                                                         xuint_t                     inactivity_timeout);

XPL_AVAILABLE_IN_ALL
GApplicationFlags       xapplication_get_flags                         (xapplication_t             *application);
XPL_AVAILABLE_IN_ALL
void                    xapplication_set_flags                         (xapplication_t             *application,
                                                                         GApplicationFlags         flags);

XPL_AVAILABLE_IN_2_42
const xchar_t *           xapplication_get_resource_base_path            (xapplication_t             *application);
XPL_AVAILABLE_IN_2_42
void                    xapplication_set_resource_base_path            (xapplication_t             *application,
                                                                         const xchar_t              *resource_path);

XPL_DEPRECATED
void                    xapplication_set_action_group                  (xapplication_t             *application,
                                                                         xaction_group_t             *action_group);

XPL_AVAILABLE_IN_2_40
void                    xapplication_add_main_option_entries           (xapplication_t             *application,
                                                                         const GOptionEntry       *entries);

XPL_AVAILABLE_IN_2_42
void                    xapplication_add_main_option                   (xapplication_t             *application,
                                                                         const char               *long_name,
                                                                         char                      short_name,
                                                                         GOptionFlags              flags,
                                                                         GOptionArg                arg,
                                                                         const char               *description,
                                                                         const char               *arg_description);
XPL_AVAILABLE_IN_2_40
void                    xapplication_add_option_group                  (xapplication_t             *application,
                                                                         xoption_group_t             *group);
XPL_AVAILABLE_IN_2_56
void                    xapplication_set_option_context_parameter_string (xapplication_t             *application,
                                                                           const xchar_t              *parameter_string);
XPL_AVAILABLE_IN_2_56
void                    xapplication_set_option_context_summary        (xapplication_t             *application,
                                                                         const xchar_t              *summary);
XPL_AVAILABLE_IN_2_56
void                    xapplication_set_option_context_description    (xapplication_t             *application,
                                                                         const xchar_t              *description);
XPL_AVAILABLE_IN_ALL
xboolean_t                xapplication_get_is_registered                 (xapplication_t             *application);
XPL_AVAILABLE_IN_ALL
xboolean_t                xapplication_get_is_remote                     (xapplication_t             *application);

XPL_AVAILABLE_IN_ALL
xboolean_t                xapplication_register                          (xapplication_t             *application,
                                                                         xcancellable_t             *cancellable,
                                                                         xerror_t                  **error);

XPL_AVAILABLE_IN_ALL
void                    xapplication_hold                              (xapplication_t             *application);
XPL_AVAILABLE_IN_ALL
void                    xapplication_release                           (xapplication_t             *application);

XPL_AVAILABLE_IN_ALL
void                    xapplication_activate                          (xapplication_t             *application);

XPL_AVAILABLE_IN_ALL
void                    xapplication_open                              (xapplication_t             *application,
                                                                         xfile_t                   **files,
                                                                         xint_t                      n_files,
                                                                         const xchar_t              *hint);

XPL_AVAILABLE_IN_ALL
int                     xapplication_run                               (xapplication_t             *application,
                                                                         int                       argc,
                                                                         char                    **argv);

XPL_AVAILABLE_IN_2_32
void                    xapplication_quit                              (xapplication_t             *application);

XPL_AVAILABLE_IN_2_32
xapplication_t *          xapplication_get_default                       (void);
XPL_AVAILABLE_IN_2_32
void                    xapplication_set_default                       (xapplication_t             *application);

XPL_AVAILABLE_IN_2_38
void                    xapplication_mark_busy                         (xapplication_t             *application);
XPL_AVAILABLE_IN_2_38
void                    xapplication_unmark_busy                       (xapplication_t             *application);
XPL_AVAILABLE_IN_2_44
xboolean_t                xapplication_get_is_busy                       (xapplication_t             *application);

XPL_AVAILABLE_IN_2_40
void                    xapplication_send_notification                 (xapplication_t             *application,
                                                                         const xchar_t              *id,
                                                                         xnotification_t            *notification);
XPL_AVAILABLE_IN_2_40
void                    xapplication_withdraw_notification             (xapplication_t             *application,
                                                                         const xchar_t              *id);

XPL_AVAILABLE_IN_2_44
void                    xapplication_bind_busy_property                (xapplication_t             *application,
                                                                         xpointer_t                  object,
                                                                         const xchar_t              *property);

XPL_AVAILABLE_IN_2_44
void                    xapplication_unbind_busy_property              (xapplication_t             *application,
                                                                         xpointer_t                  object,
                                                                         const xchar_t              *property);

G_END_DECLS

#endif /* __G_APPLICATION_H__ */
