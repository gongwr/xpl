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

#include <string.h>

#include "gmountoperation.h"
#include "gioenumtypes.h"
#include "glibintl.h"
#include "gmarshal-internal.h"


/**
 * SECTION:gmountoperation
 * @short_description: Object used for authentication and user interaction
 * @include: gio/gio.h
 *
 * #xmount_operation_t provides a mechanism for interacting with the user.
 * It can be used for authenticating mountable operations, such as loop
 * mounting files, hard drive partitions or server locations. It can
 * also be used to ask the user questions or show a list of applications
 * preventing unmount or eject operations from completing.
 *
 * Note that #xmount_operation_t is used for more than just #xmount_t
 * objects – for example it is also used in xdrive_start() and
 * xdrive_stop().
 *
 * Users should instantiate a subclass of this that implements all the
 * various callbacks to show the required dialogs, such as
 * #GtkMountOperation. If no user interaction is desired (for example
 * when automounting filesystems at login time), usually %NULL can be
 * passed, see each method taking a #xmount_operation_t for details.
 *
 * The term ‘TCRYPT’ is used to mean ‘compatible with TrueCrypt and VeraCrypt’.
 * [TrueCrypt](https://en.wikipedia.org/wiki/TrueCrypt) is a discontinued system for
 * encrypting file containers, partitions or whole disks, typically used with Windows.
 * [VeraCrypt](https://www.veracrypt.fr/) is a maintained fork of TrueCrypt with various
 * improvements and auditing fixes.
 */

enum {
  ASK_PASSWORD,
  ASK_QUESTION,
  REPLY,
  ABORTED,
  SHOW_PROCESSES,
  SHOW_UNMOUNT_PROGRESS,
  LAST_SIGNAL
};

static xuint_t signals[LAST_SIGNAL] = { 0 };

struct _GMountOperationPrivate {
  char *password;
  char *user;
  char *domain;
  xboolean_t anonymous;
  GPasswordSave password_save;
  int choice;
  xboolean_t hidden_volume;
  xboolean_t system_volume;
  xuint_t pim;
};

enum {
  PROP_0,
  PROP_USERNAME,
  PROP_PASSWORD,
  PROP_ANONYMOUS,
  PROP_DOMAIN,
  PROP_PASSWORD_SAVE,
  PROP_CHOICE,
  PROP_IS_TCRYPT_HIDDEN_VOLUME,
  PROP_IS_TCRYPT_SYSTEM_VOLUME,
  PROP_PIM
};

G_DEFINE_TYPE_WITH_PRIVATE (xmount_operation_t, g_mount_operation, XTYPE_OBJECT)

static void
g_mount_operation_set_property (xobject_t      *object,
                                xuint_t         prop_id,
                                const xvalue_t *value,
                                xparam_spec_t   *pspec)
{
  xmount_operation_t *operation;

  operation = G_MOUNT_OPERATION (object);

  switch (prop_id)
    {
    case PROP_USERNAME:
      g_mount_operation_set_username (operation,
                                      xvalue_get_string (value));
      break;

    case PROP_PASSWORD:
      g_mount_operation_set_password (operation,
                                      xvalue_get_string (value));
      break;

    case PROP_ANONYMOUS:
      g_mount_operation_set_anonymous (operation,
                                       xvalue_get_boolean (value));
      break;

    case PROP_DOMAIN:
      g_mount_operation_set_domain (operation,
                                    xvalue_get_string (value));
      break;

    case PROP_PASSWORD_SAVE:
      g_mount_operation_set_password_save (operation,
                                           xvalue_get_enum (value));
      break;

    case PROP_CHOICE:
      g_mount_operation_set_choice (operation,
                                    xvalue_get_int (value));
      break;

    case PROP_IS_TCRYPT_HIDDEN_VOLUME:
      g_mount_operation_set_is_tcrypt_hidden_volume (operation,
                                                     xvalue_get_boolean (value));
      break;

    case PROP_IS_TCRYPT_SYSTEM_VOLUME:
      g_mount_operation_set_is_tcrypt_system_volume (operation,
                                                     xvalue_get_boolean (value));
      break;

    case PROP_PIM:
        g_mount_operation_set_pim (operation,
                                   xvalue_get_uint (value));
        break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
g_mount_operation_get_property (xobject_t    *object,
                                xuint_t       prop_id,
                                xvalue_t     *value,
                                xparam_spec_t *pspec)
{
  xmount_operation_t *operation;
  GMountOperationPrivate *priv;

  operation = G_MOUNT_OPERATION (object);
  priv = operation->priv;

  switch (prop_id)
    {
    case PROP_USERNAME:
      xvalue_set_string (value, priv->user);
      break;

    case PROP_PASSWORD:
      xvalue_set_string (value, priv->password);
      break;

    case PROP_ANONYMOUS:
      xvalue_set_boolean (value, priv->anonymous);
      break;

    case PROP_DOMAIN:
      xvalue_set_string (value, priv->domain);
      break;

    case PROP_PASSWORD_SAVE:
      xvalue_set_enum (value, priv->password_save);
      break;

    case PROP_CHOICE:
      xvalue_set_int (value, priv->choice);
      break;

    case PROP_IS_TCRYPT_HIDDEN_VOLUME:
      xvalue_set_boolean (value, priv->hidden_volume);
      break;

    case PROP_IS_TCRYPT_SYSTEM_VOLUME:
      xvalue_set_boolean (value, priv->system_volume);
      break;

    case PROP_PIM:
      xvalue_set_uint (value, priv->pim);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
g_mount_operation_finalize (xobject_t *object)
{
  xmount_operation_t *operation;
  GMountOperationPrivate *priv;

  operation = G_MOUNT_OPERATION (object);

  priv = operation->priv;

  g_free (priv->password);
  g_free (priv->user);
  g_free (priv->domain);

  XOBJECT_CLASS (g_mount_operation_parent_class)->finalize (object);
}

static xboolean_t
reply_non_handled_in_idle (xpointer_t data)
{
  xmount_operation_t *op = data;

  g_mount_operation_reply (op, G_MOUNT_OPERATION_UNHANDLED);
  return XSOURCE_REMOVE;
}

static void
ask_password (xmount_operation_t *op,
	      const char      *message,
	      const char      *default_user,
	      const char      *default_domain,
	      GAskPasswordFlags flags)
{
  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
		   reply_non_handled_in_idle,
		   xobject_ref (op),
		   xobject_unref);
}

static void
ask_question (xmount_operation_t *op,
	      const char      *message,
	      const char      *choices[])
{
  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
		   reply_non_handled_in_idle,
		   xobject_ref (op),
		   xobject_unref);
}

static void
show_processes (xmount_operation_t      *op,
                const xchar_t          *message,
                xarray_t               *processes,
                const xchar_t          *choices[])
{
  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
		   reply_non_handled_in_idle,
		   xobject_ref (op),
		   xobject_unref);
}

static void
show_unmount_progress (xmount_operation_t *op,
                       const xchar_t     *message,
                       sint64_t           time_left,
                       sint64_t           bytes_left)
{
  /* nothing to do */
}

static void
g_mount_operation_class_init (GMountOperationClass *klass)
{
  xobject_class_t *object_class;

  object_class = XOBJECT_CLASS (klass);
  object_class->finalize = g_mount_operation_finalize;
  object_class->get_property = g_mount_operation_get_property;
  object_class->set_property = g_mount_operation_set_property;

  klass->ask_password = ask_password;
  klass->ask_question = ask_question;
  klass->show_processes = show_processes;
  klass->show_unmount_progress = show_unmount_progress;

  /**
   * xmount_operation_t::ask-password:
   * @op: a #xmount_operation_t requesting a password.
   * @message: string containing a message to display to the user.
   * @default_user: string containing the default user name.
   * @default_domain: string containing the default domain.
   * @flags: a set of #GAskPasswordFlags.
   *
   * Emitted when a mount operation asks the user for a password.
   *
   * If the message contains a line break, the first line should be
   * presented as a heading. For example, it may be used as the
   * primary text in a #GtkMessageDialog.
   */
  signals[ASK_PASSWORD] =
    xsignal_new (I_("ask-password"),
		  XTYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GMountOperationClass, ask_password),
		  NULL, NULL,
		  _g_cclosure_marshal_VOID__STRING_STRING_STRING_FLAGS,
		  XTYPE_NONE, 4,
		  XTYPE_STRING, XTYPE_STRING, XTYPE_STRING, XTYPE_ASK_PASSWORD_FLAGS);
  xsignal_set_va_marshaller (signals[ASK_PASSWORD],
                              XTYPE_FROM_CLASS (object_class),
                              _g_cclosure_marshal_VOID__STRING_STRING_STRING_FLAGSv);

  /**
   * xmount_operation_t::ask-question:
   * @op: a #xmount_operation_t asking a question.
   * @message: string containing a message to display to the user.
   * @choices: an array of strings for each possible choice.
   *
   * Emitted when asking the user a question and gives a list of
   * choices for the user to choose from.
   *
   * If the message contains a line break, the first line should be
   * presented as a heading. For example, it may be used as the
   * primary text in a #GtkMessageDialog.
   */
  signals[ASK_QUESTION] =
    xsignal_new (I_("ask-question"),
		  XTYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GMountOperationClass, ask_question),
		  NULL, NULL,
		  _g_cclosure_marshal_VOID__STRING_BOXED,
		  XTYPE_NONE, 2,
		  XTYPE_STRING, XTYPE_STRV);
  xsignal_set_va_marshaller (signals[ASK_QUESTION],
                              XTYPE_FROM_CLASS (object_class),
                              _g_cclosure_marshal_VOID__STRING_BOXEDv);

  /**
   * xmount_operation_t::reply:
   * @op: a #xmount_operation_t.
   * @result: a #GMountOperationResult indicating how the request was handled
   *
   * Emitted when the user has replied to the mount operation.
   */
  signals[REPLY] =
    xsignal_new (I_("reply"),
		  XTYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GMountOperationClass, reply),
		  NULL, NULL,
		  NULL,
		  XTYPE_NONE, 1,
		  XTYPE_MOUNT_OPERATION_RESULT);

  /**
   * xmount_operation_t::aborted:
   *
   * Emitted by the backend when e.g. a device becomes unavailable
   * while a mount operation is in progress.
   *
   * Implementations of xmount_operation_t should handle this signal
   * by dismissing open password dialogs.
   *
   * Since: 2.20
   */
  signals[ABORTED] =
    xsignal_new (I_("aborted"),
		  XTYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GMountOperationClass, aborted),
		  NULL, NULL,
		  NULL,
		  XTYPE_NONE, 0);

  /**
   * xmount_operation_t::show-processes:
   * @op: a #xmount_operation_t.
   * @message: string containing a message to display to the user.
   * @processes: (element-type xpid_t): an array of #xpid_t for processes
   *   blocking the operation.
   * @choices: an array of strings for each possible choice.
   *
   * Emitted when one or more processes are blocking an operation
   * e.g. unmounting/ejecting a #xmount_t or stopping a #xdrive_t.
   *
   * Note that this signal may be emitted several times to update the
   * list of blocking processes as processes close files. The
   * application should only respond with g_mount_operation_reply() to
   * the latest signal (setting #xmount_operation_t:choice to the choice
   * the user made).
   *
   * If the message contains a line break, the first line should be
   * presented as a heading. For example, it may be used as the
   * primary text in a #GtkMessageDialog.
   *
   * Since: 2.22
   */
  signals[SHOW_PROCESSES] =
    xsignal_new (I_("show-processes"),
		  XTYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GMountOperationClass, show_processes),
		  NULL, NULL,
		  _g_cclosure_marshal_VOID__STRING_BOXED_BOXED,
		  XTYPE_NONE, 3,
		  XTYPE_STRING, XTYPE_ARRAY, XTYPE_STRV);
  xsignal_set_va_marshaller (signals[SHOW_PROCESSES],
                              XTYPE_FROM_CLASS (object_class),
                              _g_cclosure_marshal_VOID__STRING_BOXED_BOXEDv);

  /**
   * xmount_operation_t::show-unmount-progress:
   * @op: a #xmount_operation_t:
   * @message: string containing a message to display to the user
   * @time_left: the estimated time left before the operation completes,
   *     in microseconds, or -1
   * @bytes_left: the amount of bytes to be written before the operation
   *     completes (or -1 if such amount is not known), or zero if the operation
   *     is completed
   *
   * Emitted when an unmount operation has been busy for more than some time
   * (typically 1.5 seconds).
   *
   * When unmounting or ejecting a volume, the kernel might need to flush
   * pending data in its buffers to the volume stable storage, and this operation
   * can take a considerable amount of time. This signal may be emitted several
   * times as long as the unmount operation is outstanding, and then one
   * last time when the operation is completed, with @bytes_left set to zero.
   *
   * Implementations of xmount_operation_t should handle this signal by
   * showing an UI notification, and then dismiss it, or show another notification
   * of completion, when @bytes_left reaches zero.
   *
   * If the message contains a line break, the first line should be
   * presented as a heading. For example, it may be used as the
   * primary text in a #GtkMessageDialog.
   *
   * Since: 2.34
   */
  signals[SHOW_UNMOUNT_PROGRESS] =
    xsignal_new (I_("show-unmount-progress"),
                  XTYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GMountOperationClass, show_unmount_progress),
                  NULL, NULL,
                  _g_cclosure_marshal_VOID__STRING_INT64_INT64,
                  XTYPE_NONE, 3,
                  XTYPE_STRING, XTYPE_INT64, XTYPE_INT64);
  xsignal_set_va_marshaller (signals[SHOW_UNMOUNT_PROGRESS],
                              XTYPE_FROM_CLASS (object_class),
                              _g_cclosure_marshal_VOID__STRING_INT64_INT64v);

  /**
   * xmount_operation_t:username:
   *
   * The user name that is used for authentication when carrying out
   * the mount operation.
   */
  xobject_class_install_property (object_class,
                                   PROP_USERNAME,
                                   xparam_spec_string ("username",
                                                        P_("Username"),
                                                        P_("The user name"),
                                                        NULL,
                                                        XPARAM_READWRITE|
                                                        XPARAM_STATIC_NAME|XPARAM_STATIC_NICK|XPARAM_STATIC_BLURB));

  /**
   * xmount_operation_t:password:
   *
   * The password that is used for authentication when carrying out
   * the mount operation.
   */
  xobject_class_install_property (object_class,
                                   PROP_PASSWORD,
                                   xparam_spec_string ("password",
                                                        P_("Password"),
                                                        P_("The password"),
                                                        NULL,
                                                        XPARAM_READWRITE|
                                                        XPARAM_STATIC_NAME|XPARAM_STATIC_NICK|XPARAM_STATIC_BLURB));

  /**
   * xmount_operation_t:anonymous:
   *
   * Whether to use an anonymous user when authenticating.
   */
  xobject_class_install_property (object_class,
                                   PROP_ANONYMOUS,
                                   xparam_spec_boolean ("anonymous",
                                                         P_("Anonymous"),
                                                         P_("Whether to use an anonymous user"),
                                                         FALSE,
                                                         XPARAM_READWRITE|
                                                         XPARAM_STATIC_NAME|XPARAM_STATIC_NICK|XPARAM_STATIC_BLURB));

  /**
   * xmount_operation_t:domain:
   *
   * The domain to use for the mount operation.
   */
  xobject_class_install_property (object_class,
                                   PROP_DOMAIN,
                                   xparam_spec_string ("domain",
                                                        P_("Domain"),
                                                        P_("The domain of the mount operation"),
                                                        NULL,
                                                        XPARAM_READWRITE|
                                                        XPARAM_STATIC_NAME|XPARAM_STATIC_NICK|XPARAM_STATIC_BLURB));

  /**
   * xmount_operation_t:password-save:
   *
   * Determines if and how the password information should be saved.
   */
  xobject_class_install_property (object_class,
                                   PROP_PASSWORD_SAVE,
                                   xparam_spec_enum ("password-save",
                                                      P_("Password save"),
                                                      P_("How passwords should be saved"),
                                                      XTYPE_PASSWORD_SAVE,
                                                      G_PASSWORD_SAVE_NEVER,
                                                      XPARAM_READWRITE|
                                                      XPARAM_STATIC_NAME|XPARAM_STATIC_NICK|XPARAM_STATIC_BLURB));

  /**
   * xmount_operation_t:choice:
   *
   * The index of the user's choice when a question is asked during the
   * mount operation. See the #xmount_operation_t::ask-question signal.
   */
  xobject_class_install_property (object_class,
                                   PROP_CHOICE,
                                   xparam_spec_int ("choice",
                                                     P_("Choice"),
                                                     P_("The users choice"),
                                                     0, G_MAXINT, 0,
                                                     XPARAM_READWRITE|
                                                     XPARAM_STATIC_NAME|XPARAM_STATIC_NICK|XPARAM_STATIC_BLURB));

  /**
   * xmount_operation_t:is-tcrypt-hidden-volume:
   *
   * Whether the device to be unlocked is a TCRYPT hidden volume.
   * See [the VeraCrypt documentation](https://www.veracrypt.fr/en/Hidden%20Volume.html).
   *
   * Since: 2.58
   */
  xobject_class_install_property (object_class,
                                   PROP_IS_TCRYPT_HIDDEN_VOLUME,
                                   xparam_spec_boolean ("is-tcrypt-hidden-volume",
                                                         P_("TCRYPT Hidden Volume"),
                                                         P_("Whether to unlock a TCRYPT hidden volume. See https://www.veracrypt.fr/en/Hidden%20Volume.html."),
                                                         FALSE,
                                                         XPARAM_READWRITE|
                                                         XPARAM_STATIC_NAME|XPARAM_STATIC_NICK|XPARAM_STATIC_BLURB));

  /**
  * xmount_operation_t:is-tcrypt-system-volume:
  *
  * Whether the device to be unlocked is a TCRYPT system volume.
  * In this context, a system volume is a volume with a bootloader
  * and operating system installed. This is only supported for Windows
  * operating systems. For further documentation, see
  * [the VeraCrypt documentation](https://www.veracrypt.fr/en/System%20Encryption.html).
  *
  * Since: 2.58
  */
  xobject_class_install_property (object_class,
                                   PROP_IS_TCRYPT_SYSTEM_VOLUME,
                                   xparam_spec_boolean ("is-tcrypt-system-volume",
                                                         P_("TCRYPT System Volume"),
                                                         P_("Whether to unlock a TCRYPT system volume. Only supported for unlocking Windows system volumes. See https://www.veracrypt.fr/en/System%20Encryption.html."),
                                                         FALSE,
                                                         XPARAM_READWRITE|
                                                         XPARAM_STATIC_NAME|XPARAM_STATIC_NICK|XPARAM_STATIC_BLURB));

  /**
  * xmount_operation_t:pim:
  *
  * The VeraCrypt PIM value, when unlocking a VeraCrypt volume. See
  * [the VeraCrypt documentation](https://www.veracrypt.fr/en/Personal%20Iterations%20Multiplier%20(PIM).html).
  *
  * Since: 2.58
  */
  xobject_class_install_property (object_class,
                                   PROP_PIM,
                                   xparam_spec_uint ("pim",
                                                      P_("PIM"),
                                                      P_("The VeraCrypt PIM value"),
                                                      0, G_MAXUINT, 0,
                                                      XPARAM_READWRITE|
                                                      XPARAM_STATIC_NAME|XPARAM_STATIC_NICK|XPARAM_STATIC_BLURB));
}

static void
g_mount_operation_init (xmount_operation_t *operation)
{
  operation->priv = g_mount_operation_get_instance_private (operation);
}

/**
 * g_mount_operation_new:
 *
 * Creates a new mount operation.
 *
 * Returns: a #xmount_operation_t.
 **/
xmount_operation_t *
g_mount_operation_new (void)
{
  return xobject_new (XTYPE_MOUNT_OPERATION, NULL);
}

/**
 * g_mount_operation_get_username:
 * @op: a #xmount_operation_t.
 *
 * Get the user name from the mount operation.
 *
 * Returns: (nullable): a string containing the user name.
 **/
const char *
g_mount_operation_get_username (xmount_operation_t *op)
{
  xreturn_val_if_fail (X_IS_MOUNT_OPERATION (op), NULL);
  return op->priv->user;
}

/**
 * g_mount_operation_set_username:
 * @op: a #xmount_operation_t.
 * @username: (nullable): input username.
 *
 * Sets the user name within @op to @username.
 **/
void
g_mount_operation_set_username (xmount_operation_t *op,
				const char      *username)
{
  g_return_if_fail (X_IS_MOUNT_OPERATION (op));
  g_free (op->priv->user);
  op->priv->user = xstrdup (username);
  xobject_notify (G_OBJECT (op), "username");
}

/**
 * g_mount_operation_get_password:
 * @op: a #xmount_operation_t.
 *
 * Gets a password from the mount operation.
 *
 * Returns: (nullable): a string containing the password within @op.
 **/
const char *
g_mount_operation_get_password (xmount_operation_t *op)
{
  xreturn_val_if_fail (X_IS_MOUNT_OPERATION (op), NULL);
  return op->priv->password;
}

/**
 * g_mount_operation_set_password:
 * @op: a #xmount_operation_t.
 * @password: (nullable): password to set.
 *
 * Sets the mount operation's password to @password.
 *
 **/
void
g_mount_operation_set_password (xmount_operation_t *op,
				const char      *password)
{
  g_return_if_fail (X_IS_MOUNT_OPERATION (op));
  g_free (op->priv->password);
  op->priv->password = xstrdup (password);
  xobject_notify (G_OBJECT (op), "password");
}

/**
 * g_mount_operation_get_anonymous:
 * @op: a #xmount_operation_t.
 *
 * Check to see whether the mount operation is being used
 * for an anonymous user.
 *
 * Returns: %TRUE if mount operation is anonymous.
 **/
xboolean_t
g_mount_operation_get_anonymous (xmount_operation_t *op)
{
  xreturn_val_if_fail (X_IS_MOUNT_OPERATION (op), FALSE);
  return op->priv->anonymous;
}

/**
 * g_mount_operation_set_anonymous:
 * @op: a #xmount_operation_t.
 * @anonymous: boolean value.
 *
 * Sets the mount operation to use an anonymous user if @anonymous is %TRUE.
 **/
void
g_mount_operation_set_anonymous (xmount_operation_t *op,
				 xboolean_t         anonymous)
{
  GMountOperationPrivate *priv;
  g_return_if_fail (X_IS_MOUNT_OPERATION (op));
  priv = op->priv;

  if (priv->anonymous != anonymous)
    {
      priv->anonymous = anonymous;
      xobject_notify (G_OBJECT (op), "anonymous");
    }
}

/**
 * g_mount_operation_get_domain:
 * @op: a #xmount_operation_t.
 *
 * Gets the domain of the mount operation.
 *
 * Returns: (nullable): a string set to the domain.
 **/
const char *
g_mount_operation_get_domain (xmount_operation_t *op)
{
  xreturn_val_if_fail (X_IS_MOUNT_OPERATION (op), NULL);
  return op->priv->domain;
}

/**
 * g_mount_operation_set_domain:
 * @op: a #xmount_operation_t.
 * @domain: (nullable): the domain to set.
 *
 * Sets the mount operation's domain.
 **/
void
g_mount_operation_set_domain (xmount_operation_t *op,
			      const char      *domain)
{
  g_return_if_fail (X_IS_MOUNT_OPERATION (op));
  g_free (op->priv->domain);
  op->priv->domain = xstrdup (domain);
  xobject_notify (G_OBJECT (op), "domain");
}

/**
 * g_mount_operation_get_password_save:
 * @op: a #xmount_operation_t.
 *
 * Gets the state of saving passwords for the mount operation.
 *
 * Returns: a #GPasswordSave flag.
 **/

GPasswordSave
g_mount_operation_get_password_save (xmount_operation_t *op)
{
  xreturn_val_if_fail (X_IS_MOUNT_OPERATION (op), G_PASSWORD_SAVE_NEVER);
  return op->priv->password_save;
}

/**
 * g_mount_operation_set_password_save:
 * @op: a #xmount_operation_t.
 * @save: a set of #GPasswordSave flags.
 *
 * Sets the state of saving passwords for the mount operation.
 *
 **/
void
g_mount_operation_set_password_save (xmount_operation_t *op,
				     GPasswordSave    save)
{
  GMountOperationPrivate *priv;
  g_return_if_fail (X_IS_MOUNT_OPERATION (op));
  priv = op->priv;

  if (priv->password_save != save)
    {
      priv->password_save = save;
      xobject_notify (G_OBJECT (op), "password-save");
    }
}

/**
 * g_mount_operation_get_choice:
 * @op: a #xmount_operation_t.
 *
 * Gets a choice from the mount operation.
 *
 * Returns: an integer containing an index of the user's choice from
 * the choice's list, or `0`.
 **/
int
g_mount_operation_get_choice (xmount_operation_t *op)
{
  xreturn_val_if_fail (X_IS_MOUNT_OPERATION (op), 0);
  return op->priv->choice;
}

/**
 * g_mount_operation_set_choice:
 * @op: a #xmount_operation_t.
 * @choice: an integer.
 *
 * Sets a default choice for the mount operation.
 **/
void
g_mount_operation_set_choice (xmount_operation_t *op,
			      int              choice)
{
  GMountOperationPrivate *priv;
  g_return_if_fail (X_IS_MOUNT_OPERATION (op));
  priv = op->priv;
  if (priv->choice != choice)
    {
      priv->choice = choice;
      xobject_notify (G_OBJECT (op), "choice");
    }
}

/**
 * g_mount_operation_get_is_tcrypt_hidden_volume:
 * @op: a #xmount_operation_t.
 *
 * Check to see whether the mount operation is being used
 * for a TCRYPT hidden volume.
 *
 * Returns: %TRUE if mount operation is for hidden volume.
 *
 * Since: 2.58
 **/
xboolean_t
g_mount_operation_get_is_tcrypt_hidden_volume (xmount_operation_t *op)
{
  xreturn_val_if_fail (X_IS_MOUNT_OPERATION (op), FALSE);
  return op->priv->hidden_volume;
}

/**
 * g_mount_operation_set_is_tcrypt_hidden_volume:
 * @op: a #xmount_operation_t.
 * @hidden_volume: boolean value.
 *
 * Sets the mount operation to use a hidden volume if @hidden_volume is %TRUE.
 *
 * Since: 2.58
 **/
void
g_mount_operation_set_is_tcrypt_hidden_volume (xmount_operation_t *op,
                                               xboolean_t hidden_volume)
{
  GMountOperationPrivate *priv;
  g_return_if_fail (X_IS_MOUNT_OPERATION (op));
  priv = op->priv;

  if (priv->hidden_volume != hidden_volume)
    {
      priv->hidden_volume = hidden_volume;
      xobject_notify (G_OBJECT (op), "is-tcrypt-hidden-volume");
    }
}

/**
 * g_mount_operation_get_is_tcrypt_system_volume:
 * @op: a #xmount_operation_t.
 *
 * Check to see whether the mount operation is being used
 * for a TCRYPT system volume.
 *
 * Returns: %TRUE if mount operation is for system volume.
 *
 * Since: 2.58
 **/
xboolean_t
g_mount_operation_get_is_tcrypt_system_volume (xmount_operation_t *op)
{
  xreturn_val_if_fail (X_IS_MOUNT_OPERATION (op), FALSE);
  return op->priv->system_volume;
}

/**
 * g_mount_operation_set_is_tcrypt_system_volume:
 * @op: a #xmount_operation_t.
 * @system_volume: boolean value.
 *
 * Sets the mount operation to use a system volume if @system_volume is %TRUE.
 *
 * Since: 2.58
 **/
void
g_mount_operation_set_is_tcrypt_system_volume (xmount_operation_t *op,
                                               xboolean_t system_volume)
{
  GMountOperationPrivate *priv;
  g_return_if_fail (X_IS_MOUNT_OPERATION (op));
  priv = op->priv;

  if (priv->system_volume != system_volume)
    {
      priv->system_volume = system_volume;
      xobject_notify (G_OBJECT (op), "is-tcrypt-system-volume");
    }
}

/**
 * g_mount_operation_get_pim:
 * @op: a #xmount_operation_t.
 *
 * Gets a PIM from the mount operation.
 *
 * Returns: The VeraCrypt PIM within @op.
 *
 * Since: 2.58
 **/
xuint_t
g_mount_operation_get_pim (xmount_operation_t *op)
{
  xreturn_val_if_fail (X_IS_MOUNT_OPERATION (op), 0);
  return op->priv->pim;
}

/**
 * g_mount_operation_set_pim:
 * @op: a #xmount_operation_t.
 * @pim: an unsigned integer.
 *
 * Sets the mount operation's PIM to @pim.
 *
 * Since: 2.58
 **/
void
g_mount_operation_set_pim (xmount_operation_t *op,
                           xuint_t pim)
{
  GMountOperationPrivate *priv;
  g_return_if_fail (X_IS_MOUNT_OPERATION (op));
  priv = op->priv;
  if (priv->pim != pim)
    {
      priv->pim = pim;
      xobject_notify (G_OBJECT (op), "pim");
    }
}

/**
 * g_mount_operation_reply:
 * @op: a #xmount_operation_t
 * @result: a #GMountOperationResult
 *
 * Emits the #xmount_operation_t::reply signal.
 **/
void
g_mount_operation_reply (xmount_operation_t *op,
			 GMountOperationResult result)
{
  g_return_if_fail (X_IS_MOUNT_OPERATION (op));
  xsignal_emit (op, signals[REPLY], 0, result);
}
