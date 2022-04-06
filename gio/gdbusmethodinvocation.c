/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include "config.h"

#include <stdlib.h>

#include "gdbusutils.h"
#include "gdbusconnection.h"
#include "gdbusmessage.h"
#include "gdbusmethodinvocation.h"
#include "gdbusintrospection.h"
#include "gdbuserror.h"
#include "gdbusprivate.h"
#include "gioerror.h"

#ifdef G_OS_UNIX
#include "gunixfdlist.h"
#endif

#include "glibintl.h"

/**
 * SECTION:gdbusmethodinvocation
 * @short_description: Object for handling remote calls
 * @include: gio/gio.h
 *
 * Instances of the #xdbus_method_invocation_t class are used when
 * handling D-Bus method calls. It provides a way to asynchronously
 * return results and errors.
 *
 * The normal way to obtain a #xdbus_method_invocation_t object is to receive
 * it as an argument to the handle_method_call() function in a
 * #xdbus_interface_vtable_t that was passed to xdbus_connection_register_object().
 */

typedef struct _GDBusMethodInvocationClass GDBusMethodInvocationClass;

/**
 * GDBusMethodInvocationClass:
 *
 * Class structure for #xdbus_method_invocation_t.
 *
 * Since: 2.26
 */
struct _GDBusMethodInvocationClass
{
  /*< private >*/
  xobject_class_t parent_class;
};

/**
 * xdbus_method_invocation_t:
 *
 * The #xdbus_method_invocation_t structure contains only private data and
 * should only be accessed using the provided API.
 *
 * Since: 2.26
 */
struct _GDBusMethodInvocation
{
  /*< private >*/
  xobject_t parent_instance;

  /* construct-only properties */
  xchar_t           *sender;
  xchar_t           *object_path;
  xchar_t           *interface_name;
  xchar_t           *method_name;
  xdbus_method_info_t *method_info;
  xdbus_property_info_t *property_info;
  xdbus_connection_t *connection;
  xdbus_message_t    *message;
  xvariant_t        *parameters;
  xpointer_t         user_data;
};

G_DEFINE_TYPE (xdbus_method_invocation_t, xdbus_method_invocation, XTYPE_OBJECT)

static void
xdbus_method_invocation_finalize (xobject_t *object)
{
  xdbus_method_invocation_t *invocation = G_DBUS_METHOD_INVOCATION (object);

  g_free (invocation->sender);
  g_free (invocation->object_path);
  g_free (invocation->interface_name);
  g_free (invocation->method_name);
  if (invocation->method_info)
      g_dbus_method_info_unref (invocation->method_info);
  if (invocation->property_info)
      g_dbus_property_info_unref (invocation->property_info);
  xobject_unref (invocation->connection);
  xobject_unref (invocation->message);
  xvariant_unref (invocation->parameters);

  G_OBJECT_CLASS (xdbus_method_invocation_parent_class)->finalize (object);
}

static void
xdbus_method_invocation_class_init (GDBusMethodInvocationClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = xdbus_method_invocation_finalize;
}

static void
xdbus_method_invocation_init (xdbus_method_invocation_t *invocation)
{
}

/**
 * xdbus_method_invocation_get_sender:
 * @invocation: A #xdbus_method_invocation_t.
 *
 * Gets the bus name that invoked the method.
 *
 * Returns: A string. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_method_invocation_get_sender (xdbus_method_invocation_t *invocation)
{
  g_return_val_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->sender;
}

/**
 * xdbus_method_invocation_get_object_path:
 * @invocation: A #xdbus_method_invocation_t.
 *
 * Gets the object path the method was invoked on.
 *
 * Returns: A string. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_method_invocation_get_object_path (xdbus_method_invocation_t *invocation)
{
  g_return_val_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->object_path;
}

/**
 * xdbus_method_invocation_get_interface_name:
 * @invocation: A #xdbus_method_invocation_t.
 *
 * Gets the name of the D-Bus interface the method was invoked on.
 *
 * If this method call is a property Get, Set or GetAll call that has
 * been redirected to the method call handler then
 * "org.freedesktop.DBus.Properties" will be returned.  See
 * #xdbus_interface_vtable_t for more information.
 *
 * Returns: A string. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_method_invocation_get_interface_name (xdbus_method_invocation_t *invocation)
{
  g_return_val_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->interface_name;
}

/**
 * xdbus_method_invocation_get_method_info:
 * @invocation: A #xdbus_method_invocation_t.
 *
 * Gets information about the method call, if any.
 *
 * If this method invocation is a property Get, Set or GetAll call that
 * has been redirected to the method call handler then %NULL will be
 * returned.  See xdbus_method_invocation_get_property_info() and
 * #xdbus_interface_vtable_t for more information.
 *
 * Returns: (nullable): A #xdbus_method_info_t or %NULL. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
const xdbus_method_info_t *
xdbus_method_invocation_get_method_info (xdbus_method_invocation_t *invocation)
{
  g_return_val_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->method_info;
}

/**
 * xdbus_method_invocation_get_property_info:
 * @invocation: A #xdbus_method_invocation_t
 *
 * Gets information about the property that this method call is for, if
 * any.
 *
 * This will only be set in the case of an invocation in response to a
 * property Get or Set call that has been directed to the method call
 * handler for an object on account of its property_get() or
 * property_set() vtable pointers being unset.
 *
 * See #xdbus_interface_vtable_t for more information.
 *
 * If the call was GetAll, %NULL will be returned.
 *
 * Returns: (nullable) (transfer none): a #xdbus_property_info_t or %NULL
 *
 * Since: 2.38
 */
const xdbus_property_info_t *
xdbus_method_invocation_get_property_info (xdbus_method_invocation_t *invocation)
{
  g_return_val_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->property_info;
}

/**
 * xdbus_method_invocation_get_method_name:
 * @invocation: A #xdbus_method_invocation_t.
 *
 * Gets the name of the method that was invoked.
 *
 * Returns: A string. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
const xchar_t *
xdbus_method_invocation_get_method_name (xdbus_method_invocation_t *invocation)
{
  g_return_val_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->method_name;
}

/**
 * xdbus_method_invocation_get_connection:
 * @invocation: A #xdbus_method_invocation_t.
 *
 * Gets the #xdbus_connection_t the method was invoked on.
 *
 * Returns: (transfer none):A #xdbus_connection_t. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
xdbus_connection_t *
xdbus_method_invocation_get_connection (xdbus_method_invocation_t *invocation)
{
  g_return_val_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->connection;
}

/**
 * xdbus_method_invocation_get_message:
 * @invocation: A #xdbus_method_invocation_t.
 *
 * Gets the #xdbus_message_t for the method invocation. This is useful if
 * you need to use low-level protocol features, such as UNIX file
 * descriptor passing, that cannot be properly expressed in the
 * #xvariant_t API.
 *
 * See this [server][gdbus-server] and [client][gdbus-unix-fd-client]
 * for an example of how to use this low-level API to send and receive
 * UNIX file descriptors.
 *
 * Returns: (transfer none): #xdbus_message_t. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
xdbus_message_t *
xdbus_method_invocation_get_message (xdbus_method_invocation_t *invocation)
{
  g_return_val_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->message;
}

/**
 * xdbus_method_invocation_get_parameters:
 * @invocation: A #xdbus_method_invocation_t.
 *
 * Gets the parameters of the method invocation. If there are no input
 * parameters then this will return a xvariant_t with 0 children rather than NULL.
 *
 * Returns: (transfer none): A #xvariant_t tuple. Do not unref this because it is owned by @invocation.
 *
 * Since: 2.26
 */
xvariant_t *
xdbus_method_invocation_get_parameters (xdbus_method_invocation_t *invocation)
{
  g_return_val_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->parameters;
}

/**
 * xdbus_method_invocation_get_user_data: (skip)
 * @invocation: A #xdbus_method_invocation_t.
 *
 * Gets the @user_data #xpointer_t passed to xdbus_connection_register_object().
 *
 * Returns: A #xpointer_t.
 *
 * Since: 2.26
 */
xpointer_t
xdbus_method_invocation_get_user_data (xdbus_method_invocation_t *invocation)
{
  g_return_val_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->user_data;
}

/* < internal >
 * _xdbus_method_invocation_new:
 * @sender: (nullable): The bus name that invoked the method or %NULL if @connection is not a bus connection.
 * @object_path: The object path the method was invoked on.
 * @interface_name: The name of the D-Bus interface the method was invoked on.
 * @method_name: The name of the method that was invoked.
 * @method_info: (nullable): Information about the method call or %NULL.
 * @property_info: (nullable): Information about the property or %NULL.
 * @connection: The #xdbus_connection_t the method was invoked on.
 * @message: The D-Bus message as a #xdbus_message_t.
 * @parameters: The parameters as a #xvariant_t tuple.
 * @user_data: The @user_data #xpointer_t passed to xdbus_connection_register_object().
 *
 * Creates a new #xdbus_method_invocation_t object.
 *
 * Returns: A #xdbus_method_invocation_t. Free with xobject_unref().
 *
 * Since: 2.26
 */
xdbus_method_invocation_t *
_xdbus_method_invocation_new (const xchar_t             *sender,
                               const xchar_t             *object_path,
                               const xchar_t             *interface_name,
                               const xchar_t             *method_name,
                               const xdbus_method_info_t   *method_info,
                               const xdbus_property_info_t *property_info,
                               xdbus_connection_t         *connection,
                               xdbus_message_t            *message,
                               xvariant_t                *parameters,
                               xpointer_t                 user_data)
{
  xdbus_method_invocation_t *invocation;

  g_return_val_if_fail (sender == NULL || g_dbus_is_name (sender), NULL);
  g_return_val_if_fail (xvariant_is_object_path (object_path), NULL);
  g_return_val_if_fail (interface_name == NULL || g_dbus_is_interface_name (interface_name), NULL);
  g_return_val_if_fail (g_dbus_is_member_name (method_name), NULL);
  g_return_val_if_fail (X_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (X_IS_DBUS_MESSAGE (message), NULL);
  g_return_val_if_fail (xvariant_is_of_type (parameters, G_VARIANT_TYPE_TUPLE), NULL);

  invocation = G_DBUS_METHOD_INVOCATION (xobject_new (XTYPE_DBUS_METHOD_INVOCATION, NULL));
  invocation->sender = xstrdup (sender);
  invocation->object_path = xstrdup (object_path);
  invocation->interface_name = xstrdup (interface_name);
  invocation->method_name = xstrdup (method_name);
  if (method_info)
    invocation->method_info = g_dbus_method_info_ref ((xdbus_method_info_t *)method_info);
  if (property_info)
    invocation->property_info = g_dbus_property_info_ref ((xdbus_property_info_t *)property_info);
  invocation->connection = xobject_ref (connection);
  invocation->message = xobject_ref (message);
  invocation->parameters = xvariant_ref (parameters);
  invocation->user_data = user_data;

  return invocation;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
xdbus_method_invocation_return_value_internal (xdbus_method_invocation_t *invocation,
                                                xvariant_t              *parameters,
                                                xunix_fd_list_t           *fd_list)
{
  xdbus_message_t *reply;
  xerror_t *error;

  g_return_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail ((parameters == NULL) || xvariant_is_of_type (parameters, G_VARIANT_TYPE_TUPLE));

  if (xdbus_message_get_flags (invocation->message) & G_DBUS_MESSAGE_FLAGS_NO_REPLY_EXPECTED)
    {
      if (parameters != NULL)
        {
          xvariant_ref_sink (parameters);
          xvariant_unref (parameters);
        }
      goto out;
    }

  if (parameters == NULL)
    parameters = xvariant_new_tuple (NULL, 0);

  /* if we have introspection data, check that the signature of @parameters is correct */
  if (invocation->method_info != NULL)
    {
      xvariant_type_t *type;

      type = _g_dbus_compute_complete_signature (invocation->method_info->out_args);

      if (!xvariant_is_of_type (parameters, type))
        {
          xchar_t *type_string = xvariant_type_dup_string (type);

          g_warning ("Type of return value is incorrect: expected '%s', got '%s''",
		     type_string, xvariant_get_type_string (parameters));
          xvariant_type_free (type);
          g_free (type_string);
          goto out;
        }
      xvariant_type_free (type);
    }

  /* property_info is only non-NULL if set that way from
   * xdbus_connection_t, so this must be the case of async property
   * handling on either 'Get', 'Set' or 'GetAll'.
   */
  if (invocation->property_info != NULL)
    {
      if (xstr_equal (invocation->method_name, "Get"))
        {
          xvariant_t *nested;

          if (!xvariant_is_of_type (parameters, G_VARIANT_TYPE ("(v)")))
            {
              g_warning ("Type of return value for property 'Get' call should be '(v)' but got '%s'",
                         xvariant_get_type_string (parameters));
              goto out;
            }

          /* Go deeper and make sure that the value inside of the
           * variant matches the property type.
           */
          xvariant_get (parameters, "(v)", &nested);
          if (!xstr_equal (xvariant_get_type_string (nested), invocation->property_info->signature))
            {
              g_warning ("Value returned from property 'Get' call for '%s' should be '%s' but is '%s'",
                         invocation->property_info->name, invocation->property_info->signature,
                         xvariant_get_type_string (nested));
              xvariant_unref (nested);
              goto out;
            }
          xvariant_unref (nested);
        }

      else if (xstr_equal (invocation->method_name, "GetAll"))
        {
          if (!xvariant_is_of_type (parameters, G_VARIANT_TYPE ("(a{sv})")))
            {
              g_warning ("Type of return value for property 'GetAll' call should be '(a{sv})' but got '%s'",
                         xvariant_get_type_string (parameters));
              goto out;
            }

          /* Could iterate the list of properties and make sure that all
           * of them are actually on the interface and with the correct
           * types, but let's not do that for now...
           */
        }

      else if (xstr_equal (invocation->method_name, "Set"))
        {
          if (!xvariant_is_of_type (parameters, G_VARIANT_TYPE_UNIT))
            {
              g_warning ("Type of return value for property 'Set' call should be '()' but got '%s'",
                         xvariant_get_type_string (parameters));
              goto out;
            }
        }

      else
        g_assert_not_reached ();
    }

  if (G_UNLIKELY (_g_dbus_debug_return ()))
    {
      _g_dbus_debug_print_lock ();
      g_print ("========================================================================\n"
               "GDBus-debug:Return:\n"
               " >>>> METHOD RETURN\n"
               "      in response to %s.%s()\n"
               "      on object %s\n"
               "      to name %s\n"
               "      reply-serial %d\n",
               invocation->interface_name, invocation->method_name,
               invocation->object_path,
               invocation->sender,
               xdbus_message_get_serial (invocation->message));
      _g_dbus_debug_print_unlock ();
    }

  reply = xdbus_message_new_method_reply (invocation->message);
  xdbus_message_set_body (reply, parameters);

#ifdef G_OS_UNIX
  if (fd_list != NULL)
    xdbus_message_set_unix_fd_list (reply, fd_list);
#endif

  error = NULL;
  if (!xdbus_connection_send_message (xdbus_method_invocation_get_connection (invocation), reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, &error))
    {
      if (!xerror_matches (error, G_IO_ERROR, G_IO_ERROR_CLOSED))
        g_warning ("Error sending message: %s", error->message);
      xerror_free (error);
    }
  xobject_unref (reply);

 out:
  xobject_unref (invocation);
}

/**
 * xdbus_method_invocation_return_value:
 * @invocation: (transfer full): A #xdbus_method_invocation_t.
 * @parameters: (nullable): A #xvariant_t tuple with out parameters for the method or %NULL if not passing any parameters.
 *
 * Finishes handling a D-Bus method call by returning @parameters.
 * If the @parameters xvariant_t is floating, it is consumed.
 *
 * It is an error if @parameters is not of the right format: it must be a tuple
 * containing the out-parameters of the D-Bus method. Even if the method has a
 * single out-parameter, it must be contained in a tuple. If the method has no
 * out-parameters, @parameters may be %NULL or an empty tuple.
 *
 * |[<!-- language="C" -->
 * xdbus_method_invocation_t *invocation = some_invocation;
 * g_autofree xchar_t *result_string = NULL;
 * x_autoptr (xerror) error = NULL;
 *
 * result_string = calculate_result (&error);
 *
 * if (error != NULL)
 *   xdbus_method_invocation_return_gerror (invocation, error);
 * else
 *   xdbus_method_invocation_return_value (invocation,
 *                                          xvariant_new ("(s)", result_string));
 *
 * // Do not free @invocation here; returning a value does that
 * ]|
 *
 * This method will take ownership of @invocation. See
 * #xdbus_interface_vtable_t for more information about the ownership of
 * @invocation.
 *
 * Since 2.48, if the method call requested for a reply not to be sent
 * then this call will sink @parameters and free @invocation, but
 * otherwise do nothing (as per the recommendations of the D-Bus
 * specification).
 *
 * Since: 2.26
 */
void
xdbus_method_invocation_return_value (xdbus_method_invocation_t *invocation,
                                       xvariant_t              *parameters)
{
  xdbus_method_invocation_return_value_internal (invocation, parameters, NULL);
}

#ifdef G_OS_UNIX
/**
 * xdbus_method_invocation_return_value_with_unix_fd_list:
 * @invocation: (transfer full): A #xdbus_method_invocation_t.
 * @parameters: (nullable): A #xvariant_t tuple with out parameters for the method or %NULL if not passing any parameters.
 * @fd_list: (nullable): A #xunix_fd_list_t or %NULL.
 *
 * Like xdbus_method_invocation_return_value() but also takes a #xunix_fd_list_t.
 *
 * This method is only available on UNIX.
 *
 * This method will take ownership of @invocation. See
 * #xdbus_interface_vtable_t for more information about the ownership of
 * @invocation.
 *
 * Since: 2.30
 */
void
xdbus_method_invocation_return_value_with_unix_fd_list (xdbus_method_invocation_t *invocation,
                                                         xvariant_t              *parameters,
                                                         xunix_fd_list_t           *fd_list)
{
  xdbus_method_invocation_return_value_internal (invocation, parameters, fd_list);
}
#endif

/* ---------------------------------------------------------------------------------------------------- */

/**
 * xdbus_method_invocation_return_error:
 * @invocation: (transfer full): A #xdbus_method_invocation_t.
 * @domain: A #xquark for the #xerror_t error domain.
 * @code: The error code.
 * @format: printf()-style format.
 * @...: Parameters for @format.
 *
 * Finishes handling a D-Bus method call by returning an error.
 *
 * See g_dbus_error_encode_gerror() for details about what error name
 * will be returned on the wire. In a nutshell, if the given error is
 * registered using g_dbus_error_register_error() the name given
 * during registration is used. Otherwise, a name of the form
 * `org.gtk.GDBus.UnmappedGError.Quark...` is used. This provides
 * transparent mapping of #xerror_t between applications using GDBus.
 *
 * If you are writing an application intended to be portable,
 * always register errors with g_dbus_error_register_error()
 * or use xdbus_method_invocation_return_dbus_error().
 *
 * This method will take ownership of @invocation. See
 * #xdbus_interface_vtable_t for more information about the ownership of
 * @invocation.
 *
 * Since 2.48, if the method call requested for a reply not to be sent
 * then this call will free @invocation but otherwise do nothing (as per
 * the recommendations of the D-Bus specification).
 *
 * Since: 2.26
 */
void
xdbus_method_invocation_return_error (xdbus_method_invocation_t *invocation,
                                       xquark                 domain,
                                       xint_t                   code,
                                       const xchar_t           *format,
                                       ...)
{
  va_list var_args;

  g_return_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail (format != NULL);

  va_start (var_args, format);
  xdbus_method_invocation_return_error_valist (invocation,
                                                domain,
                                                code,
                                                format,
                                                var_args);
  va_end (var_args);
}

/**
 * xdbus_method_invocation_return_error_valist:
 * @invocation: (transfer full): A #xdbus_method_invocation_t.
 * @domain: A #xquark for the #xerror_t error domain.
 * @code: The error code.
 * @format: printf()-style format.
 * @var_args: #va_list of parameters for @format.
 *
 * Like xdbus_method_invocation_return_error() but intended for
 * language bindings.
 *
 * This method will take ownership of @invocation. See
 * #xdbus_interface_vtable_t for more information about the ownership of
 * @invocation.
 *
 * Since: 2.26
 */
void
xdbus_method_invocation_return_error_valist (xdbus_method_invocation_t *invocation,
                                              xquark                 domain,
                                              xint_t                   code,
                                              const xchar_t           *format,
                                              va_list                var_args)
{
  xchar_t *literal_message;

  g_return_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail (format != NULL);

  literal_message = xstrdup_vprintf (format, var_args);
  xdbus_method_invocation_return_error_literal (invocation,
                                                 domain,
                                                 code,
                                                 literal_message);
  g_free (literal_message);
}

/**
 * xdbus_method_invocation_return_error_literal:
 * @invocation: (transfer full): A #xdbus_method_invocation_t.
 * @domain: A #xquark for the #xerror_t error domain.
 * @code: The error code.
 * @message: The error message.
 *
 * Like xdbus_method_invocation_return_error() but without printf()-style formatting.
 *
 * This method will take ownership of @invocation. See
 * #xdbus_interface_vtable_t for more information about the ownership of
 * @invocation.
 *
 * Since: 2.26
 */
void
xdbus_method_invocation_return_error_literal (xdbus_method_invocation_t *invocation,
                                               xquark                 domain,
                                               xint_t                   code,
                                               const xchar_t           *message)
{
  xerror_t *error;

  g_return_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail (message != NULL);

  error = xerror_new_literal (domain, code, message);
  xdbus_method_invocation_return_gerror (invocation, error);
  xerror_free (error);
}

/**
 * xdbus_method_invocation_return_gerror:
 * @invocation: (transfer full): A #xdbus_method_invocation_t.
 * @error: A #xerror_t.
 *
 * Like xdbus_method_invocation_return_error() but takes a #xerror_t
 * instead of the error domain, error code and message.
 *
 * This method will take ownership of @invocation. See
 * #xdbus_interface_vtable_t for more information about the ownership of
 * @invocation.
 *
 * Since: 2.26
 */
void
xdbus_method_invocation_return_gerror (xdbus_method_invocation_t *invocation,
                                        const xerror_t          *error)
{
  xchar_t *dbus_error_name;

  g_return_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail (error != NULL);

  dbus_error_name = g_dbus_error_encode_gerror (error);

  xdbus_method_invocation_return_dbus_error (invocation,
                                              dbus_error_name,
                                              error->message);
  g_free (dbus_error_name);
}

/**
 * xdbus_method_invocation_take_error: (skip)
 * @invocation: (transfer full): A #xdbus_method_invocation_t.
 * @error: (transfer full): A #xerror_t.
 *
 * Like xdbus_method_invocation_return_gerror() but takes ownership
 * of @error so the caller does not need to free it.
 *
 * This method will take ownership of @invocation. See
 * #xdbus_interface_vtable_t for more information about the ownership of
 * @invocation.
 *
 * Since: 2.30
 */
void
xdbus_method_invocation_take_error (xdbus_method_invocation_t *invocation,
                                     xerror_t                *error)
{
  g_return_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail (error != NULL);
  xdbus_method_invocation_return_gerror (invocation, error);
  xerror_free (error);
}

/**
 * xdbus_method_invocation_return_dbus_error:
 * @invocation: (transfer full): A #xdbus_method_invocation_t.
 * @error_name: A valid D-Bus error name.
 * @error_message: A valid D-Bus error message.
 *
 * Finishes handling a D-Bus method call by returning an error.
 *
 * This method will take ownership of @invocation. See
 * #xdbus_interface_vtable_t for more information about the ownership of
 * @invocation.
 *
 * Since: 2.26
 */
void
xdbus_method_invocation_return_dbus_error (xdbus_method_invocation_t *invocation,
                                            const xchar_t           *error_name,
                                            const xchar_t           *error_message)
{
  xdbus_message_t *reply;

  g_return_if_fail (X_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail (error_name != NULL && g_dbus_is_name (error_name));
  g_return_if_fail (error_message != NULL);

  if (xdbus_message_get_flags (invocation->message) & G_DBUS_MESSAGE_FLAGS_NO_REPLY_EXPECTED)
    goto out;

  if (G_UNLIKELY (_g_dbus_debug_return ()))
    {
      _g_dbus_debug_print_lock ();
      g_print ("========================================================================\n"
               "GDBus-debug:Return:\n"
               " >>>> METHOD ERROR %s\n"
               "      message '%s'\n"
               "      in response to %s.%s()\n"
               "      on object %s\n"
               "      to name %s\n"
               "      reply-serial %d\n",
               error_name,
               error_message,
               invocation->interface_name, invocation->method_name,
               invocation->object_path,
               invocation->sender,
               xdbus_message_get_serial (invocation->message));
      _g_dbus_debug_print_unlock ();
    }

  reply = xdbus_message_new_method_error_literal (invocation->message,
                                                   error_name,
                                                   error_message);
  xdbus_connection_send_message (xdbus_method_invocation_get_connection (invocation), reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
  xobject_unref (reply);

out:
  xobject_unref (invocation);
}
