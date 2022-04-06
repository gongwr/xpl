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

#ifndef __XFILENAME_COMPLETER_H__
#define __XFILENAME_COMPLETER_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_FILENAME_COMPLETER         (xfilename_completer_get_type ())
#define XFILENAME_COMPLETER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_FILENAME_COMPLETER, xfilename_completer))
#define XFILENAME_COMPLETER_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_FILENAME_COMPLETER, xfilename_completer_class))
#define XFILENAME_COMPLETER_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_FILENAME_COMPLETER, xfilename_completer_class))
#define X_IS_FILENAME_COMPLETER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_FILENAME_COMPLETER))
#define X_IS_FILENAME_COMPLETER_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_FILENAME_COMPLETER))

/**
 * xfilename_completer_t:
 *
 * Completes filenames based on files that exist within the file system.
 **/
typedef struct _xfilename_completer_class xfilename_completer_class_t;

struct _xfilename_completer_class
{
  xobject_class_t parent_class;

  /*< public >*/
  /* signals */
  void (* got_completion_data) (xfilename_completer_t *filename_completer);

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t               xfilename_completer_get_type              (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xfilename_completer_t *xfilename_completer_new                   (void);

XPL_AVAILABLE_IN_ALL
char *              xfilename_completer_get_completion_suffix (xfilename_completer_t *completer,
                                                                const char *initial_text);
XPL_AVAILABLE_IN_ALL
char **             xfilename_completer_get_completions       (xfilename_completer_t *completer,
                                                                const char *initial_text);
XPL_AVAILABLE_IN_ALL
void                xfilename_completer_set_dirs_only         (xfilename_completer_t *completer,
                                                                xboolean_t dirs_only);

G_END_DECLS

#endif /* __XFILENAME_COMPLETER_H__ */
