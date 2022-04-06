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

#ifndef __G_MOUNT_OPERATION_H__
#define __G_MOUNT_OPERATION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_MOUNT_OPERATION         (g_mount_operation_get_type ())
#define G_MOUNT_OPERATION(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_MOUNT_OPERATION, xmount_operation))
#define G_MOUNT_OPERATION_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_MOUNT_OPERATION, GMountOperationClass))
#define X_IS_MOUNT_OPERATION(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_MOUNT_OPERATION))
#define X_IS_MOUNT_OPERATION_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_MOUNT_OPERATION))
#define G_MOUNT_OPERATION_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_MOUNT_OPERATION, GMountOperationClass))

/**
 * xmount_operation_t:
 *
 * Class for providing authentication methods for mounting operations,
 * such as mounting a file locally, or authenticating with a server.
 **/
typedef struct _GMountOperationClass   GMountOperationClass;
typedef struct _GMountOperationPrivate GMountOperationPrivate;

struct _GMountOperation
{
  xobject_t parent_instance;

  GMountOperationPrivate *priv;
};

struct _GMountOperationClass
{
  xobject_class_t parent_class;

  /* signals: */

  void (* ask_password) (xmount_operation_t       *op,
			 const char            *message,
			 const char            *default_user,
			 const char            *default_domain,
			 GAskPasswordFlags      flags);

  /**
   * GMountOperationClass::ask_question:
   * @op: a #xmount_operation_t
   * @message: string containing a message to display to the user
   * @choices: (array zero-terminated=1) (element-type utf8): an array of
   *    strings for each possible choice
   *
   * Virtual implementation of #xmount_operation_t::ask-question.
   */
  void (* ask_question) (xmount_operation_t       *op,
			 const char            *message,
			 const char            *choices[]);

  void (* reply)        (xmount_operation_t       *op,
			 GMountOperationResult  result);

  void (* aborted)      (xmount_operation_t       *op);

  /**
   * GMountOperationClass::show_processes:
   * @op: a #xmount_operation_t
   * @message: string containing a message to display to the user
   * @processes: (element-type xpid_t): an array of #xpid_t for processes blocking
   *    the operation
   * @choices: (array zero-terminated=1) (element-type utf8): an array of
   *    strings for each possible choice
   *
   * Virtual implementation of #xmount_operation_t::show-processes.
   *
   * Since: 2.22
   */
  void (* show_processes) (xmount_operation_t      *op,
                           const xchar_t          *message,
                           xarray_t               *processes,
                           const xchar_t          *choices[]);

  void (* show_unmount_progress) (xmount_operation_t *op,
                                  const xchar_t     *message,
                                  sint64_t           time_left,
                                  sint64_t           bytes_left);

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
  void (*_g_reserved6) (void);
  void (*_g_reserved7) (void);
  void (*_g_reserved8) (void);
  void (*_g_reserved9) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t             g_mount_operation_get_type      (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xmount_operation_t * g_mount_operation_new           (void);

XPL_AVAILABLE_IN_ALL
const char *  g_mount_operation_get_username      (xmount_operation_t *op);
XPL_AVAILABLE_IN_ALL
void          g_mount_operation_set_username      (xmount_operation_t *op,
						   const char      *username);
XPL_AVAILABLE_IN_ALL
const char *  g_mount_operation_get_password      (xmount_operation_t *op);
XPL_AVAILABLE_IN_ALL
void          g_mount_operation_set_password      (xmount_operation_t *op,
						   const char      *password);
XPL_AVAILABLE_IN_ALL
xboolean_t      g_mount_operation_get_anonymous     (xmount_operation_t *op);
XPL_AVAILABLE_IN_ALL
void          g_mount_operation_set_anonymous     (xmount_operation_t *op,
						   xboolean_t         anonymous);
XPL_AVAILABLE_IN_ALL
const char *  g_mount_operation_get_domain        (xmount_operation_t *op);
XPL_AVAILABLE_IN_ALL
void          g_mount_operation_set_domain        (xmount_operation_t *op,
						   const char      *domain);
XPL_AVAILABLE_IN_ALL
GPasswordSave g_mount_operation_get_password_save (xmount_operation_t *op);
XPL_AVAILABLE_IN_ALL
void          g_mount_operation_set_password_save (xmount_operation_t *op,
						   GPasswordSave    save);
XPL_AVAILABLE_IN_ALL
int           g_mount_operation_get_choice        (xmount_operation_t *op);
XPL_AVAILABLE_IN_ALL
void          g_mount_operation_set_choice        (xmount_operation_t *op,
						   int              choice);
XPL_AVAILABLE_IN_ALL
void          g_mount_operation_reply             (xmount_operation_t *op,
						   GMountOperationResult result);
XPL_AVAILABLE_IN_2_58
xboolean_t      g_mount_operation_get_is_tcrypt_hidden_volume (xmount_operation_t *op);
XPL_AVAILABLE_IN_2_58
void          g_mount_operation_set_is_tcrypt_hidden_volume (xmount_operation_t *op,
                                                             xboolean_t hidden_volume);
XPL_AVAILABLE_IN_2_58
xboolean_t      g_mount_operation_get_is_tcrypt_system_volume (xmount_operation_t *op);
XPL_AVAILABLE_IN_2_58
void          g_mount_operation_set_is_tcrypt_system_volume (xmount_operation_t *op,
                                                             xboolean_t system_volume);
XPL_AVAILABLE_IN_2_58
xuint_t  g_mount_operation_get_pim           (xmount_operation_t *op);
XPL_AVAILABLE_IN_2_58
void          g_mount_operation_set_pim           (xmount_operation_t *op,
                                                   xuint_t pim);

G_END_DECLS

#endif /* __G_MOUNT_OPERATION_H__ */
