/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2009 Codethink Limited
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

#ifndef __G_UNIX_FD_LIST_H__
#define __G_UNIX_FD_LIST_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define XTYPE_UNIX_FD_LIST                                 (g_unix_fd_list_get_type ())
#define G_UNIX_FD_LIST(inst)                                (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_UNIX_FD_LIST, xunix_fd_list))
#define G_UNIX_FD_LIST_CLASS(class)                         (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_UNIX_FD_LIST, GUnixFDListClass))
#define X_IS_UNIX_FD_LIST(inst)                             (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_UNIX_FD_LIST))
#define X_IS_UNIX_FD_LIST_CLASS(class)                      (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_UNIX_FD_LIST))
#define G_UNIX_FD_LIST_GET_CLASS(inst)                      (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_UNIX_FD_LIST, GUnixFDListClass))
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xunix_fd_list, xobject_unref)

typedef struct _GUnixFDListPrivate                       GUnixFDListPrivate;
typedef struct _GUnixFDListClass                         GUnixFDListClass;

struct _GUnixFDListClass
{
  xobject_class_t parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

struct _GUnixFDList
{
  xobject_t parent_instance;
  GUnixFDListPrivate *priv;
};

XPL_AVAILABLE_IN_ALL
xtype_t                   g_unix_fd_list_get_type                         (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xunix_fd_list_t *           g_unix_fd_list_new                              (void);
XPL_AVAILABLE_IN_ALL
xunix_fd_list_t *           g_unix_fd_list_new_from_array                   (const xint_t   *fds,
                                                                         xint_t          n_fds);

XPL_AVAILABLE_IN_ALL
xint_t                    g_unix_fd_list_append                           (xunix_fd_list_t  *list,
                                                                         xint_t          fd,
                                                                         xerror_t      **error);

XPL_AVAILABLE_IN_ALL
xint_t                    g_unix_fd_list_get_length                       (xunix_fd_list_t  *list);

XPL_AVAILABLE_IN_ALL
xint_t                    g_unix_fd_list_get                              (xunix_fd_list_t  *list,
                                                                         xint_t          index_,
                                                                         xerror_t      **error);

XPL_AVAILABLE_IN_ALL
const xint_t *            g_unix_fd_list_peek_fds                         (xunix_fd_list_t  *list,
                                                                         xint_t         *length);

XPL_AVAILABLE_IN_ALL
xint_t *                  g_unix_fd_list_steal_fds                        (xunix_fd_list_t  *list,
                                                                         xint_t         *length);

G_END_DECLS

#endif /* __G_UNIX_FD_LIST_H__ */
