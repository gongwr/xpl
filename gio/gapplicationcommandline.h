/* GIO - GLib Input, Output and Streaming Library
 *
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

#ifndef __G_APPLICATION_COMMAND_LINE_H__
#define __G_APPLICATION_COMMAND_LINE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_APPLICATION_COMMAND_LINE                     (g_application_command_line_get_type ())
#define G_APPLICATION_COMMAND_LINE(inst)                    (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_APPLICATION_COMMAND_LINE,                        \
                                                             GApplicationCommandLine))
#define G_APPLICATION_COMMAND_LINE_CLASS(class)             (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_APPLICATION_COMMAND_LINE,                        \
                                                             GApplicationCommandLineClass))
#define X_IS_APPLICATION_COMMAND_LINE(inst)                 (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_APPLICATION_COMMAND_LINE))
#define X_IS_APPLICATION_COMMAND_LINE_CLASS(class)          (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_APPLICATION_COMMAND_LINE))
#define G_APPLICATION_COMMAND_LINE_GET_CLASS(inst)          (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_APPLICATION_COMMAND_LINE,                        \
                                                             GApplicationCommandLineClass))

typedef struct _GApplicationCommandLinePrivate               GApplicationCommandLinePrivate;
typedef struct _GApplicationCommandLineClass                 GApplicationCommandLineClass;

struct _GApplicationCommandLine
{
  /*< private >*/
  xobject_t parent_instance;

  GApplicationCommandLinePrivate *priv;
};

struct _GApplicationCommandLineClass
{
  /*< private >*/
  xobject_class_t parent_class;

  void                  (* print_literal)       (GApplicationCommandLine *cmdline,
                                                 const xchar_t             *message);
  void                  (* printerr_literal)    (GApplicationCommandLine *cmdline,
                                                 const xchar_t             *message);
  xinput_stream_t *        (* get_stdin)           (GApplicationCommandLine *cmdline);

  xpointer_t padding[11];
};

XPL_AVAILABLE_IN_ALL
xtype_t                   g_application_command_line_get_type             (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xchar_t **                g_application_command_line_get_arguments        (GApplicationCommandLine   *cmdline,
                                                                         int                       *argc);

XPL_AVAILABLE_IN_2_40
GVariantDict *          g_application_command_line_get_options_dict     (GApplicationCommandLine   *cmdline);

XPL_AVAILABLE_IN_2_36
xinput_stream_t *          g_application_command_line_get_stdin            (GApplicationCommandLine   *cmdline);

XPL_AVAILABLE_IN_ALL
const xchar_t * const *   g_application_command_line_get_environ          (GApplicationCommandLine   *cmdline);

XPL_AVAILABLE_IN_ALL
const xchar_t *           g_application_command_line_getenv               (GApplicationCommandLine   *cmdline,
                                                                         const xchar_t               *name);

XPL_AVAILABLE_IN_ALL
const xchar_t *           g_application_command_line_get_cwd              (GApplicationCommandLine   *cmdline);

XPL_AVAILABLE_IN_ALL
xboolean_t                g_application_command_line_get_is_remote        (GApplicationCommandLine   *cmdline);

XPL_AVAILABLE_IN_ALL
void                    g_application_command_line_print                (GApplicationCommandLine   *cmdline,
                                                                         const xchar_t               *format,
                                                                         ...) G_GNUC_PRINTF(2, 3);
XPL_AVAILABLE_IN_ALL
void                    g_application_command_line_printerr             (GApplicationCommandLine   *cmdline,
                                                                         const xchar_t               *format,
                                                                         ...) G_GNUC_PRINTF(2, 3);

XPL_AVAILABLE_IN_ALL
int                     g_application_command_line_get_exit_status      (GApplicationCommandLine   *cmdline);
XPL_AVAILABLE_IN_ALL
void                    g_application_command_line_set_exit_status      (GApplicationCommandLine   *cmdline,
                                                                         int                        exit_status);

XPL_AVAILABLE_IN_ALL
xvariant_t *              g_application_command_line_get_platform_data    (GApplicationCommandLine   *cmdline);

XPL_AVAILABLE_IN_2_36
xfile_t *                 g_application_command_line_create_file_for_arg  (GApplicationCommandLine   *cmdline,
                                                                         const xchar_t               *arg);

G_END_DECLS

#endif /* __G_APPLICATION_COMMAND_LINE_H__ */
