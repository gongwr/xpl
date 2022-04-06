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

#include "gdbusauth.h"

#include "gdbusauthmechanismanon.h"
#include "gdbusauthmechanismexternal.h"
#include "gdbusauthmechanismsha1.h"
#include "gdbusauthobserver.h"

#include "gdbuserror.h"
#include "gdbusutils.h"
#include "gioenumtypes.h"
#include "gcredentials.h"
#include "gcredentialsprivate.h"
#include "gdbusprivate.h"
#include "giostream.h"
#include "gdatainputstream.h"
#include "gdataoutputstream.h"

#ifdef G_OS_UNIX
#include "gnetworking.h"
#include "gunixconnection.h"
#include "gunixcredentialsmessage.h"
#endif

#include "glibintl.h"

G_GNUC_PRINTF(1, 2)
static void
debug_print (const xchar_t *message, ...)
{
  if (G_UNLIKELY (_g_dbus_debug_authentication ()))
    {
      xchar_t *s;
      xstring_t *str;
      va_list var_args;
      xuint_t n;

      _g_dbus_debug_print_lock ();

      va_start (var_args, message);
      s = xstrdup_vprintf (message, var_args);
      va_end (var_args);

      str = xstring_new (NULL);
      for (n = 0; s[n] != '\0'; n++)
        {
          if (G_UNLIKELY (s[n] == '\r'))
            xstring_append (str, "\\r");
          else if (G_UNLIKELY (s[n] == '\n'))
            xstring_append (str, "\\n");
          else
            xstring_append_c (str, s[n]);
        }
      g_print ("GDBus-debug:Auth: %s\n", str->str);
      xstring_free (str, TRUE);
      g_free (s);

      _g_dbus_debug_print_unlock ();
    }
}

typedef struct
{
  const xchar_t *name;
  xint_t priority;
  xtype_t gtype;
} Mechanism;

static void mechanism_free (Mechanism *m);

struct _GDBusAuthPrivate
{
  xio_stream_t *stream;

  /* A list of available Mechanism, sorted according to priority  */
  xlist_t *available_mechanisms;
};

enum
{
  PROP_0,
  PROP_STREAM
};

G_DEFINE_TYPE_WITH_PRIVATE (GDBusAuth, _g_dbus_auth, XTYPE_OBJECT)

/* ---------------------------------------------------------------------------------------------------- */

static void
_g_dbus_auth_finalize (xobject_t *object)
{
  GDBusAuth *auth = G_DBUS_AUTH (object);

  if (auth->priv->stream != NULL)
    xobject_unref (auth->priv->stream);
  xlist_free_full (auth->priv->available_mechanisms, (xdestroy_notify_t) mechanism_free);

  if (G_OBJECT_CLASS (_g_dbus_auth_parent_class)->finalize != NULL)
    G_OBJECT_CLASS (_g_dbus_auth_parent_class)->finalize (object);
}

static void
_g_dbus_auth_get_property (xobject_t    *object,
                           xuint_t       prop_id,
                           xvalue_t     *value,
                           xparam_spec_t *pspec)
{
  GDBusAuth *auth = G_DBUS_AUTH (object);

  switch (prop_id)
    {
    case PROP_STREAM:
      xvalue_set_object (value, auth->priv->stream);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_g_dbus_auth_set_property (xobject_t      *object,
                           xuint_t         prop_id,
                           const xvalue_t *value,
                           xparam_spec_t   *pspec)
{
  GDBusAuth *auth = G_DBUS_AUTH (object);

  switch (prop_id)
    {
    case PROP_STREAM:
      auth->priv->stream = xvalue_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_g_dbus_auth_class_init (GDBusAuthClass *klass)
{
  xobject_class_t *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = _g_dbus_auth_get_property;
  gobject_class->set_property = _g_dbus_auth_set_property;
  gobject_class->finalize     = _g_dbus_auth_finalize;

  xobject_class_install_property (gobject_class,
                                   PROP_STREAM,
                                   g_param_spec_object ("stream",
                                                        P_("IO Stream"),
                                                        P_("The underlying xio_stream_t used for I/O"),
                                                        XTYPE_IO_STREAM,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));
}

static void
mechanism_free (Mechanism *m)
{
  g_free (m);
}

static void
add_mechanism (GDBusAuth         *auth,
               xdbus_auth_observer_t *observer,
               xtype_t              mechanism_type)
{
  const xchar_t *name;

  name = _g_dbus_auth_mechanism_get_name (mechanism_type);
  if (observer == NULL || xdbus_auth_observer_allow_mechanism (observer, name))
    {
      Mechanism *m;
      m = g_new0 (Mechanism, 1);
      m->name = name;
      m->priority = _g_dbus_auth_mechanism_get_priority (mechanism_type);
      m->gtype = mechanism_type;
      auth->priv->available_mechanisms = xlist_prepend (auth->priv->available_mechanisms, m);
    }
}

static xint_t
mech_compare_func (Mechanism *a, Mechanism *b)
{
  xint_t ret;
  /* ensure deterministic order */
  ret = b->priority - a->priority;
  if (ret == 0)
    ret = xstrcmp0 (b->name, a->name);
  return ret;
}

static void
_g_dbus_auth_init (GDBusAuth *auth)
{
  auth->priv = _g_dbus_auth_get_instance_private (auth);
}

static void
_g_dbus_auth_add_mechs (GDBusAuth         *auth,
                        xdbus_auth_observer_t *observer)
{
  /* TODO: trawl extension points */
  add_mechanism (auth, observer, XTYPE_DBUS_AUTH_MECHANISM_ANON);
  add_mechanism (auth, observer, XTYPE_DBUS_AUTH_MECHANISM_SHA1);
  add_mechanism (auth, observer, XTYPE_DBUS_AUTH_MECHANISM_EXTERNAL);

  auth->priv->available_mechanisms = xlist_sort (auth->priv->available_mechanisms,
                                                  (GCompareFunc) mech_compare_func);
}

static xtype_t
find_mech_by_name (GDBusAuth *auth,
                   const xchar_t *name)
{
  xtype_t ret;
  xlist_t *l;

  ret = (xtype_t) 0;

  for (l = auth->priv->available_mechanisms; l != NULL; l = l->next)
    {
      Mechanism *m = l->data;
      if (xstrcmp0 (name, m->name) == 0)
        {
          ret = m->gtype;
          goto out;
        }
    }

 out:
  return ret;
}

GDBusAuth  *
_g_dbus_auth_new (xio_stream_t *stream)
{
  return xobject_new (XTYPE_DBUS_AUTH,
                       "stream", stream,
                       NULL);
}

/* ---------------------------------------------------------------------------------------------------- */
/* like g_data_input_stream_read_line() but sets error if there's no content to read */
static xchar_t *
_my_g_data_input_stream_read_line (xdata_input_stream_t  *dis,
                                   xsize_t             *out_line_length,
                                   xcancellable_t      *cancellable,
                                   xerror_t           **error)
{
  xchar_t *ret;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  ret = g_data_input_stream_read_line (dis,
                                       out_line_length,
                                       cancellable,
                                       error);
  if (ret == NULL && error != NULL && *error == NULL)
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           _("Unexpected lack of content trying to read a line"));
    }

  return ret;
}

/* This function is to avoid situations like this
 *
 * BEGIN\r\nl\0\0\1...
 *
 * e.g. where we read into the first D-Bus message while waiting for
 * the final line from the client (TODO: file bug against gio for
 * this)
 */
static xchar_t *
_my_xinput_stream_read_line_safe (xinput_stream_t  *i,
                                   xsize_t         *out_line_length,
                                   xcancellable_t  *cancellable,
                                   xerror_t       **error)
{
  xstring_t *str;
  xchar_t c;
  xssize_t num_read;
  xboolean_t last_was_cr;

  str = xstring_new (NULL);

  last_was_cr = FALSE;
  while (TRUE)
    {
      num_read = xinput_stream_read (i,
                                      &c,
                                      1,
                                      cancellable,
                                      error);
      if (num_read == -1)
        goto fail;
      if (num_read == 0)
        {
          if (error != NULL && *error == NULL)
            {
              g_set_error_literal (error,
                                   G_IO_ERROR,
                                   G_IO_ERROR_FAILED,
                                   _("Unexpected lack of content trying to (safely) read a line"));
            }
          goto fail;
        }

      xstring_append_c (str, (xint_t) c);
      if (last_was_cr)
        {
          if (c == 0x0a)
            {
              g_assert (str->len >= 2);
              xstring_set_size (str, str->len - 2);
              goto out;
            }
        }
      last_was_cr = (c == 0x0d);
    }

 out:
  if (out_line_length != NULL)
    *out_line_length = str->len;
  return xstring_free (str, FALSE);

 fail:
  g_assert (error == NULL || *error != NULL);
  xstring_free (str, TRUE);
  return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

static xchar_t *
hexdecode (const xchar_t  *str,
           xsize_t        *out_len,
           xerror_t      **error)
{
  xchar_t *ret;
  xstring_t *s;
  xuint_t n;

  ret = NULL;
  s = xstring_new (NULL);

  for (n = 0; str[n] != '\0'; n += 2)
    {
      xint_t upper_nibble;
      xint_t lower_nibble;
      xuint_t value;

      upper_nibble = g_ascii_xdigit_value (str[n]);
      lower_nibble = g_ascii_xdigit_value (str[n + 1]);
      if (upper_nibble == -1 || lower_nibble == -1)
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_FAILED,
                       "Error hexdecoding string '%s' around position %d",
                       str, n);
          goto out;
        }
      value = (upper_nibble<<4) | lower_nibble;
      xstring_append_c (s, value);
    }

  *out_len = s->len;
  ret = xstring_free (s, FALSE);
  s = NULL;

 out:
  if (s != NULL)
    {
      *out_len = 0;
      xstring_free (s, TRUE);
    }
   return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static GDBusAuthMechanism *
client_choose_mech_and_send_initial_response (GDBusAuth           *auth,
                                              xcredentials_t        *credentials_that_were_sent,
                                              const xchar_t* const  *supported_auth_mechs,
                                              xptr_array_t           *attempted_auth_mechs,
                                              xdata_output_stream_t   *dos,
                                              xcancellable_t        *cancellable,
                                              xerror_t             **error)
{
  GDBusAuthMechanism *mech;
  xtype_t auth_mech_to_use_gtype;
  xuint_t n;
  xuint_t m;
  xchar_t *initial_response;
  xsize_t initial_response_len;
  xchar_t *encoded;
  xchar_t *s;

 again:
  mech = NULL;

  debug_print ("CLIENT: Trying to choose mechanism");

  /* find an authentication mechanism to try, if any */
  auth_mech_to_use_gtype = (xtype_t) 0;
  for (n = 0; supported_auth_mechs[n] != NULL; n++)
    {
      xboolean_t attempted_already;
      attempted_already = FALSE;
      for (m = 0; m < attempted_auth_mechs->len; m++)
        {
          if (xstrcmp0 (supported_auth_mechs[n], attempted_auth_mechs->pdata[m]) == 0)
            {
              attempted_already = TRUE;
              break;
            }
        }
      if (!attempted_already)
        {
          auth_mech_to_use_gtype = find_mech_by_name (auth, supported_auth_mechs[n]);
          if (auth_mech_to_use_gtype != (xtype_t) 0)
            break;
        }
    }

  if (auth_mech_to_use_gtype == (xtype_t) 0)
    {
      xuint_t n;
      xchar_t *available;
      xstring_t *tried_str;

      debug_print ("CLIENT: Exhausted all available mechanisms");

      available = xstrjoinv (", ", (xchar_t **) supported_auth_mechs);

      tried_str = xstring_new (NULL);
      for (n = 0; n < attempted_auth_mechs->len; n++)
        {
          if (n > 0)
            xstring_append (tried_str, ", ");
          xstring_append (tried_str, attempted_auth_mechs->pdata[n]);
        }
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   _("Exhausted all available authentication mechanisms (tried: %s) (available: %s)"),
                   tried_str->str,
                   available);
      xstring_free (tried_str, TRUE);
      g_free (available);
      goto out;
    }

  /* OK, decided on a mechanism - let's do this thing */
  mech = xobject_new (auth_mech_to_use_gtype,
                       "stream", auth->priv->stream,
                       "credentials", credentials_that_were_sent,
                       NULL);
  debug_print ("CLIENT: Trying mechanism '%s'", _g_dbus_auth_mechanism_get_name (auth_mech_to_use_gtype));
  xptr_array_add (attempted_auth_mechs, (xpointer_t) _g_dbus_auth_mechanism_get_name (auth_mech_to_use_gtype));

  /* the auth mechanism may not be supported
   * (for example, EXTERNAL only works if credentials were exchanged)
   */
  if (!_g_dbus_auth_mechanism_is_supported (mech))
    {
      debug_print ("CLIENT: Mechanism '%s' says it is not supported", _g_dbus_auth_mechanism_get_name (auth_mech_to_use_gtype));
      xobject_unref (mech);
      mech = NULL;
      goto again;
    }

  initial_response_len = 0;
  initial_response = _g_dbus_auth_mechanism_client_initiate (mech,
                                                             &initial_response_len);
#if 0
  g_printerr ("using auth mechanism with name '%s' of type '%s' with initial response '%s'\n",
              _g_dbus_auth_mechanism_get_name (auth_mech_to_use_gtype),
              xtype_name (XTYPE_FROM_INSTANCE (mech)),
              initial_response);
#endif
  if (initial_response != NULL)
    {
      //g_printerr ("initial_response = '%s'\n", initial_response);
      encoded = _g_dbus_hexencode (initial_response, initial_response_len);
      s = xstrdup_printf ("AUTH %s %s\r\n",
                           _g_dbus_auth_mechanism_get_name (auth_mech_to_use_gtype),
                           encoded);
      g_free (initial_response);
      g_free (encoded);
    }
  else
    {
      s = xstrdup_printf ("AUTH %s\r\n", _g_dbus_auth_mechanism_get_name (auth_mech_to_use_gtype));
    }
  debug_print ("CLIENT: writing '%s'", s);
  if (!g_data_output_stream_put_string (dos, s, cancellable, error))
    {
      xobject_unref (mech);
      mech = NULL;
      g_free (s);
      goto out;
    }
  g_free (s);

 out:
  return mech;
}


/* ---------------------------------------------------------------------------------------------------- */

typedef enum
{
  CLIENT_STATE_WAITING_FOR_DATA,
  CLIENT_STATE_WAITING_FOR_OK,
  CLIENT_STATE_WAITING_FOR_REJECT,
  CLIENT_STATE_WAITING_FOR_AGREE_UNIX_FD
} ClientState;

xchar_t *
_g_dbus_auth_run_client (GDBusAuth     *auth,
                         xdbus_auth_observer_t     *observer,
                         GDBusCapabilityFlags offered_capabilities,
                         GDBusCapabilityFlags *out_negotiated_capabilities,
                         xcancellable_t  *cancellable,
                         xerror_t       **error)
{
  xchar_t *s;
  xdata_input_stream_t *dis;
  xdata_output_stream_t *dos;
  xcredentials_t *credentials;
  xchar_t *ret_guid;
  xchar_t *line;
  xsize_t line_length;
  xchar_t **supported_auth_mechs;
  xptr_array_t *attempted_auth_mechs;
  GDBusAuthMechanism *mech;
  ClientState state;
  GDBusCapabilityFlags negotiated_capabilities;

  debug_print ("CLIENT: initiating");

  _g_dbus_auth_add_mechs (auth, observer);

  ret_guid = NULL;
  supported_auth_mechs = NULL;
  attempted_auth_mechs = xptr_array_new ();
  mech = NULL;
  negotiated_capabilities = 0;
  credentials = NULL;

  dis = G_DATA_INPUT_STREAM (g_data_input_stream_new (g_io_stream_get_input_stream (auth->priv->stream)));
  dos = G_DATA_OUTPUT_STREAM (g_data_output_stream_new (g_io_stream_get_output_stream (auth->priv->stream)));
  g_filter_input_stream_set_close_base_stream (G_FILTER_INPUT_STREAM (dis), FALSE);
  g_filter_output_stream_set_close_base_stream (G_FILTER_OUTPUT_STREAM (dos), FALSE);

  g_data_input_stream_set_newline_type (dis, G_DATA_STREAM_NEWLINE_TYPE_CR_LF);

#ifdef G_OS_UNIX
  if (X_IS_UNIX_CONNECTION (auth->priv->stream))
    {
      credentials = xcredentials_new ();
      if (!g_unix_connection_send_credentials (G_UNIX_CONNECTION (auth->priv->stream),
                                               cancellable,
                                               error))
        goto out;
    }
  else
    {
      if (!g_data_output_stream_put_byte (dos, '\0', cancellable, error))
        goto out;
    }
#else
  if (!g_data_output_stream_put_byte (dos, '\0', cancellable, error))
    goto out;
#endif

  if (credentials != NULL)
    {
      if (G_UNLIKELY (_g_dbus_debug_authentication ()))
        {
          s = xcredentials_to_string (credentials);
          debug_print ("CLIENT: sent credentials '%s'", s);
          g_free (s);
        }
    }
  else
    {
      debug_print ("CLIENT: didn't send any credentials");
    }

  /* TODO: to reduce roundtrips, try to pick an auth mechanism to start with */

  /* Get list of supported authentication mechanisms */
  s = "AUTH\r\n";
  debug_print ("CLIENT: writing '%s'", s);
  if (!g_data_output_stream_put_string (dos, s, cancellable, error))
    goto out;
  state = CLIENT_STATE_WAITING_FOR_REJECT;

  while (TRUE)
    {
      switch (state)
        {
        case CLIENT_STATE_WAITING_FOR_REJECT:
          debug_print ("CLIENT: WaitingForReject");
          line = _my_g_data_input_stream_read_line (dis, &line_length, cancellable, error);
          if (line == NULL)
            goto out;
          debug_print ("CLIENT: WaitingForReject, read '%s'", line);

        choose_mechanism:
          if (!xstr_has_prefix (line, "REJECTED "))
            {
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           "In WaitingForReject: Expected 'REJECTED am1 am2 ... amN', got '%s'",
                           line);
              g_free (line);
              goto out;
            }
          if (supported_auth_mechs == NULL)
            {
              supported_auth_mechs = xstrsplit (line + sizeof ("REJECTED ") - 1, " ", 0);
#if 0
              for (n = 0; supported_auth_mechs != NULL && supported_auth_mechs[n] != NULL; n++)
                g_printerr ("supported_auth_mechs[%d] = '%s'\n", n, supported_auth_mechs[n]);
#endif
            }
          g_free (line);
          mech = client_choose_mech_and_send_initial_response (auth,
                                                               credentials,
                                                               (const xchar_t* const *) supported_auth_mechs,
                                                               attempted_auth_mechs,
                                                               dos,
                                                               cancellable,
                                                               error);
          if (mech == NULL)
            goto out;
          if (_g_dbus_auth_mechanism_client_get_state (mech) == G_DBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA)
            state = CLIENT_STATE_WAITING_FOR_DATA;
          else
            state = CLIENT_STATE_WAITING_FOR_OK;
          break;

        case CLIENT_STATE_WAITING_FOR_OK:
          debug_print ("CLIENT: WaitingForOK");
          line = _my_g_data_input_stream_read_line (dis, &line_length, cancellable, error);
          if (line == NULL)
            goto out;
          debug_print ("CLIENT: WaitingForOK, read '%s'", line);
          if (xstr_has_prefix (line, "OK "))
            {
              if (!g_dbus_is_guid (line + 3))
                {
                  g_set_error (error,
                               G_IO_ERROR,
                               G_IO_ERROR_FAILED,
                               "Invalid OK response '%s'",
                               line);
                  g_free (line);
                  goto out;
                }
              ret_guid = xstrdup (line + 3);
              g_free (line);

              if (offered_capabilities & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING)
                {
                  s = "NEGOTIATE_UNIX_FD\r\n";
                  debug_print ("CLIENT: writing '%s'", s);
                  if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                    goto out;
                  state = CLIENT_STATE_WAITING_FOR_AGREE_UNIX_FD;
                }
              else
                {
                  s = "BEGIN\r\n";
                  debug_print ("CLIENT: writing '%s'", s);
                  if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                    goto out;
                  /* and we're done! */
                  goto out;
                }
            }
          else if (xstr_has_prefix (line, "REJECTED "))
            {
              goto choose_mechanism;
            }
          else
            {
              /* TODO: handle other valid responses */
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           "In WaitingForOk: unexpected response '%s'",
                           line);
              g_free (line);
              goto out;
            }
          break;

        case CLIENT_STATE_WAITING_FOR_AGREE_UNIX_FD:
          debug_print ("CLIENT: WaitingForAgreeUnixFD");
          line = _my_g_data_input_stream_read_line (dis, &line_length, cancellable, error);
          if (line == NULL)
            goto out;
          debug_print ("CLIENT: WaitingForAgreeUnixFD, read='%s'", line);
          if (xstrcmp0 (line, "AGREE_UNIX_FD") == 0)
            {
              g_free (line);
              negotiated_capabilities |= G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING;
              s = "BEGIN\r\n";
              debug_print ("CLIENT: writing '%s'", s);
              if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                goto out;
              /* and we're done! */
              goto out;
            }
          else if (xstr_has_prefix (line, "ERROR") && (line[5] == 0 || g_ascii_isspace (line[5])))
            {
              //xstrstrip (line + 5); g_debug ("bah, no unix_fd: '%s'", line + 5);
              g_free (line);
              s = "BEGIN\r\n";
              debug_print ("CLIENT: writing '%s'", s);
              if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                goto out;
              /* and we're done! */
              goto out;
            }
          else
            {
              /* TODO: handle other valid responses */
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           "In WaitingForAgreeUnixFd: unexpected response '%s'",
                           line);
              g_free (line);
              goto out;
            }
          break;

        case CLIENT_STATE_WAITING_FOR_DATA:
          debug_print ("CLIENT: WaitingForData");
          line = _my_g_data_input_stream_read_line (dis, &line_length, cancellable, error);
          if (line == NULL)
            goto out;
          debug_print ("CLIENT: WaitingForData, read='%s'", line);
          if (xstr_has_prefix (line, "DATA "))
            {
              xchar_t *encoded;
              xchar_t *decoded_data;
              xsize_t decoded_data_len = 0;

              encoded = xstrdup (line + 5);
              g_free (line);
              xstrstrip (encoded);
              decoded_data = hexdecode (encoded, &decoded_data_len, error);
              g_free (encoded);
              if (decoded_data == NULL)
                {
                  g_prefix_error (error, "DATA response is malformed: ");
                  /* invalid encoding, disconnect! */
                  goto out;
                }
              _g_dbus_auth_mechanism_client_data_receive (mech, decoded_data, decoded_data_len);
              g_free (decoded_data);

              if (_g_dbus_auth_mechanism_client_get_state (mech) == G_DBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND)
                {
                  xchar_t *data;
                  xsize_t data_len;
                  xchar_t *encoded_data;
                  data = _g_dbus_auth_mechanism_client_data_send (mech, &data_len);
                  encoded_data = _g_dbus_hexencode (data, data_len);
                  s = xstrdup_printf ("DATA %s\r\n", encoded_data);
                  g_free (encoded_data);
                  g_free (data);
                  debug_print ("CLIENT: writing '%s'", s);
                  if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                    {
                      g_free (s);
                      goto out;
                    }
                  g_free (s);
                }
              state = CLIENT_STATE_WAITING_FOR_OK;
            }
          else if (xstr_has_prefix (line, "REJECTED "))
            {
              /* could be the chosen authentication method just doesn't work. Try
               * another one...
               */
              goto choose_mechanism;
            }
          else
            {
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           "In WaitingForData: unexpected response '%s'",
                           line);
              g_free (line);
              goto out;
            }
          break;

        default:
          g_assert_not_reached ();
          break;
        }

    }; /* main authentication client loop */

 out:
  if (mech != NULL)
    xobject_unref (mech);
  xptr_array_unref (attempted_auth_mechs);
  xstrfreev (supported_auth_mechs);
  xobject_unref (dis);
  xobject_unref (dos);

  /* ensure return value is NULL if error is set */
  if (error != NULL && *error != NULL)
    {
      g_free (ret_guid);
      ret_guid = NULL;
    }

  if (ret_guid != NULL)
    {
      if (out_negotiated_capabilities != NULL)
        *out_negotiated_capabilities = negotiated_capabilities;
    }

  if (credentials != NULL)
    xobject_unref (credentials);

  debug_print ("CLIENT: Done, authenticated=%d", ret_guid != NULL);

  return ret_guid;
}

/* ---------------------------------------------------------------------------------------------------- */

static xchar_t *
get_auth_mechanisms (GDBusAuth     *auth,
                     xboolean_t       allow_anonymous,
                     const xchar_t   *prefix,
                     const xchar_t   *suffix,
                     const xchar_t   *separator)
{
  xlist_t *l;
  xstring_t *str;
  xboolean_t need_sep;

  str = xstring_new (prefix);
  need_sep = FALSE;
  for (l = auth->priv->available_mechanisms; l != NULL; l = l->next)
    {
      Mechanism *m = l->data;

      if (!allow_anonymous && xstrcmp0 (m->name, "ANONYMOUS") == 0)
        continue;

      if (need_sep)
        xstring_append (str, separator);
      xstring_append (str, m->name);
      need_sep = TRUE;
    }

  xstring_append (str, suffix);
  return xstring_free (str, FALSE);
}


typedef enum
{
  SERVER_STATE_WAITING_FOR_AUTH,
  SERVER_STATE_WAITING_FOR_DATA,
  SERVER_STATE_WAITING_FOR_BEGIN
} ServerState;

xboolean_t
_g_dbus_auth_run_server (GDBusAuth              *auth,
                         xdbus_auth_observer_t      *observer,
                         const xchar_t            *guid,
                         xboolean_t                allow_anonymous,
                         xboolean_t                require_same_user,
                         GDBusCapabilityFlags    offered_capabilities,
                         GDBusCapabilityFlags   *out_negotiated_capabilities,
                         xcredentials_t          **out_received_credentials,
                         xcancellable_t           *cancellable,
                         xerror_t                **error)
{
  xboolean_t ret;
  ServerState state;
  xdata_input_stream_t *dis;
  xdata_output_stream_t *dos;
  xerror_t *local_error;
  xchar_t *line;
  xsize_t line_length;
  GDBusAuthMechanism *mech;
  xchar_t *s;
  GDBusCapabilityFlags negotiated_capabilities;
  xcredentials_t *credentials;
  xcredentials_t *own_credentials = NULL;

  debug_print ("SERVER: initiating");

  _g_dbus_auth_add_mechs (auth, observer);

  ret = FALSE;
  dis = NULL;
  dos = NULL;
  mech = NULL;
  negotiated_capabilities = 0;
  credentials = NULL;

  if (!g_dbus_is_guid (guid))
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   "The given GUID '%s' is not valid",
                   guid);
      goto out;
    }

  dis = G_DATA_INPUT_STREAM (g_data_input_stream_new (g_io_stream_get_input_stream (auth->priv->stream)));
  dos = G_DATA_OUTPUT_STREAM (g_data_output_stream_new (g_io_stream_get_output_stream (auth->priv->stream)));
  g_filter_input_stream_set_close_base_stream (G_FILTER_INPUT_STREAM (dis), FALSE);
  g_filter_output_stream_set_close_base_stream (G_FILTER_OUTPUT_STREAM (dos), FALSE);

  g_data_input_stream_set_newline_type (dis, G_DATA_STREAM_NEWLINE_TYPE_CR_LF);

  /* read the NUL-byte, possibly with credentials attached */
#ifdef G_OS_UNIX
#ifndef G_CREDENTIALS_PREFER_MESSAGE_PASSING
  if (X_IS_SOCKET_CONNECTION (auth->priv->stream))
    {
      xsocket_t *sock = xsocket_connection_get_socket (XSOCKET_CONNECTION (auth->priv->stream));

      local_error = NULL;
      credentials = xsocket_get_credentials (sock, &local_error);

      if (credentials == NULL && !xerror_matches (local_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED))
        {
          g_propagate_error (error, local_error);
          goto out;
        }
      else
        {
          /* Clear the error indicator, so we can retry with
           * g_unix_connection_receive_credentials() if necessary */
          g_clear_error (&local_error);
        }
    }
#endif

  if (credentials == NULL && X_IS_UNIX_CONNECTION (auth->priv->stream))
    {
      local_error = NULL;
      credentials = g_unix_connection_receive_credentials (G_UNIX_CONNECTION (auth->priv->stream),
                                                           cancellable,
                                                           &local_error);
      if (credentials == NULL && !xerror_matches (local_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED))
        {
          g_propagate_error (error, local_error);
          goto out;
        }
      g_clear_error (&local_error);
    }
  else
    {
      local_error = NULL;
      (void)g_data_input_stream_read_byte (dis, cancellable, &local_error);
      if (local_error != NULL)
        {
          g_propagate_error (error, local_error);
          goto out;
        }
    }
#else
  local_error = NULL;
  (void)g_data_input_stream_read_byte (dis, cancellable, &local_error);
  if (local_error != NULL)
    {
      g_propagate_error (error, local_error);
      goto out;
    }
#endif
  if (credentials != NULL)
    {
      if (G_UNLIKELY (_g_dbus_debug_authentication ()))
        {
          s = xcredentials_to_string (credentials);
          debug_print ("SERVER: received credentials '%s'", s);
          g_free (s);
        }
    }
  else
    {
      debug_print ("SERVER: didn't receive any credentials");
    }

  own_credentials = xcredentials_new ();

  state = SERVER_STATE_WAITING_FOR_AUTH;
  while (TRUE)
    {
      switch (state)
        {
        case SERVER_STATE_WAITING_FOR_AUTH:
          debug_print ("SERVER: WaitingForAuth");
          line = _my_g_data_input_stream_read_line (dis, &line_length, cancellable, error);
          debug_print ("SERVER: WaitingForAuth, read '%s'", line);
          if (line == NULL)
            goto out;
          if (xstrcmp0 (line, "AUTH") == 0)
            {
              s = get_auth_mechanisms (auth, allow_anonymous, "REJECTED ", "\r\n", " ");
              debug_print ("SERVER: writing '%s'", s);
              if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                {
                  g_free (s);
                  g_free (line);
                  goto out;
                }
              g_free (s);
              g_free (line);
            }
          else if (xstr_has_prefix (line, "AUTH "))
            {
              xchar_t **tokens;
              const xchar_t *encoded;
              const xchar_t *mech_name;
              xtype_t auth_mech_to_use_gtype;

              tokens = xstrsplit (line, " ", 0);

              switch (xstrv_length (tokens))
                {
                case 2:
                  /* no initial response */
                  mech_name = tokens[1];
                  encoded = NULL;
                  break;

                case 3:
                  /* initial response */
                  mech_name = tokens[1];
                  encoded = tokens[2];
                  break;

                default:
                  g_set_error (error,
                               G_IO_ERROR,
                               G_IO_ERROR_FAILED,
                               "Unexpected line '%s' while in WaitingForAuth state",
                               line);
                  xstrfreev (tokens);
                  g_free (line);
                  goto out;
                }

              g_free (line);

              /* TODO: record that the client has attempted to use this mechanism */
              //g_debug ("client is trying '%s'", mech_name);

              auth_mech_to_use_gtype = find_mech_by_name (auth, mech_name);
              if ((auth_mech_to_use_gtype == (xtype_t) 0) ||
                  (!allow_anonymous && xstrcmp0 (mech_name, "ANONYMOUS") == 0))
                {
                  /* We don't support this auth mechanism */
                  xstrfreev (tokens);
                  s = get_auth_mechanisms (auth, allow_anonymous, "REJECTED ", "\r\n", " ");
                  debug_print ("SERVER: writing '%s'", s);
                  if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                    {
                      g_free (s);
                      goto out;
                    }
                  g_free (s);

                  /* stay in WAITING FOR AUTH */
                  state = SERVER_STATE_WAITING_FOR_AUTH;
                }
              else
                {
                  xchar_t *initial_response;
                  xsize_t initial_response_len;

                  g_clear_object (&mech);
                  mech = xobject_new (auth_mech_to_use_gtype,
                                       "stream", auth->priv->stream,
                                       "credentials", credentials,
                                       NULL);

                  initial_response = NULL;
                  initial_response_len = 0;
                  if (encoded != NULL)
                    {
                      initial_response = hexdecode (encoded, &initial_response_len, error);
                      if (initial_response == NULL)
                        {
                          g_prefix_error (error, "Initial response is malformed: ");
                          /* invalid encoding, disconnect! */
                          xstrfreev (tokens);
                          goto out;
                        }
                    }

                  _g_dbus_auth_mechanism_server_initiate (mech,
                                                          initial_response,
                                                          initial_response_len);
                  g_free (initial_response);
                  xstrfreev (tokens);

                change_state:
                  switch (_g_dbus_auth_mechanism_server_get_state (mech))
                    {
                    case G_DBUS_AUTH_MECHANISM_STATE_ACCEPTED:
                      if (require_same_user &&
                          (credentials == NULL ||
                           !xcredentials_is_same_user (credentials, own_credentials, NULL)))
                        {
                          /* disconnect */
                          g_set_error_literal (error,
                                               G_IO_ERROR,
                                               G_IO_ERROR_FAILED,
                                               _("User IDs must be the same for peer and server"));
                          goto out;
                        }
                      else if (observer != NULL &&
                               !xdbus_auth_observer_authorize_authenticated_peer (observer,
                                                                                   auth->priv->stream,
                                                                                   credentials))
                        {
                          /* disconnect */
                          g_set_error_literal (error,
                                               G_IO_ERROR,
                                               G_IO_ERROR_FAILED,
                                               _("Cancelled via xdbus_auth_observer_t::authorize-authenticated-peer"));
                          goto out;
                        }
                      else
                        {
                          s = xstrdup_printf ("OK %s\r\n", guid);
                          debug_print ("SERVER: writing '%s'", s);
                          if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                            {
                              g_free (s);
                              goto out;
                            }
                          g_free (s);
                          state = SERVER_STATE_WAITING_FOR_BEGIN;
                        }
                      break;

                    case G_DBUS_AUTH_MECHANISM_STATE_REJECTED:
                      s = get_auth_mechanisms (auth, allow_anonymous, "REJECTED ", "\r\n", " ");
                      debug_print ("SERVER: writing '%s'", s);
                      if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                        {
                          g_free (s);
                          goto out;
                        }
                      g_free (s);
                      state = SERVER_STATE_WAITING_FOR_AUTH;
                      break;

                    case G_DBUS_AUTH_MECHANISM_STATE_WAITING_FOR_DATA:
                      state = SERVER_STATE_WAITING_FOR_DATA;
                      break;

                    case G_DBUS_AUTH_MECHANISM_STATE_HAVE_DATA_TO_SEND:
                      {
                        xchar_t *data;
                        xsize_t data_len;

                        data = _g_dbus_auth_mechanism_server_data_send (mech, &data_len);
                        if (data != NULL)
                          {
                            xchar_t *encoded_data;

                            encoded_data = _g_dbus_hexencode (data, data_len);
                            s = xstrdup_printf ("DATA %s\r\n", encoded_data);
                            g_free (encoded_data);
                            g_free (data);

                            debug_print ("SERVER: writing '%s'", s);
                            if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                              {
                                g_free (s);
                                goto out;
                              }
                            g_free (s);
                          }
                      }
                      goto change_state;
                      break;

                    default:
                      /* TODO */
                      g_assert_not_reached ();
                      break;
                    }
                }
            }
          else
            {
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           "Unexpected line '%s' while in WaitingForAuth state",
                           line);
              g_free (line);
              goto out;
            }
          break;

        case SERVER_STATE_WAITING_FOR_DATA:
          debug_print ("SERVER: WaitingForData");
          line = _my_g_data_input_stream_read_line (dis, &line_length, cancellable, error);
          debug_print ("SERVER: WaitingForData, read '%s'", line);
          if (line == NULL)
            goto out;
          if (xstr_has_prefix (line, "DATA "))
            {
              xchar_t *encoded;
              xchar_t *decoded_data;
              xsize_t decoded_data_len = 0;

              encoded = xstrdup (line + 5);
              g_free (line);
              xstrstrip (encoded);
              decoded_data = hexdecode (encoded, &decoded_data_len, error);
              g_free (encoded);
              if (decoded_data == NULL)
                {
                  g_prefix_error (error, "DATA response is malformed: ");
                  /* invalid encoding, disconnect! */
                  goto out;
                }
              _g_dbus_auth_mechanism_server_data_receive (mech, decoded_data, decoded_data_len);
              g_free (decoded_data);
              /* oh man, this goto-crap is so ugly.. really need to rewrite the state machine */
              goto change_state;
            }
          else
            {
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_FAILED,
                           "Unexpected line '%s' while in WaitingForData state",
                           line);
              g_free (line);
            }
          goto out;

        case SERVER_STATE_WAITING_FOR_BEGIN:
          debug_print ("SERVER: WaitingForBegin");
          /* Use extremely slow (but reliable) line reader - this basically
           * does a recvfrom() system call per character
           *
           * (the problem with using xdata_input_stream_t's read_line is that because of
           * buffering it might start reading into the first D-Bus message that
           * appears after "BEGIN\r\n"....)
           */
          line = _my_xinput_stream_read_line_safe (g_io_stream_get_input_stream (auth->priv->stream),
                                                    &line_length,
                                                    cancellable,
                                                    error);
          if (line == NULL)
            goto out;
          debug_print ("SERVER: WaitingForBegin, read '%s'", line);
          if (xstrcmp0 (line, "BEGIN") == 0)
            {
              /* YAY, done! */
              ret = TRUE;
              g_free (line);
              goto out;
            }
          else if (xstrcmp0 (line, "NEGOTIATE_UNIX_FD") == 0)
            {
              g_free (line);
              if (offered_capabilities & G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING)
                {
                  negotiated_capabilities |= G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING;
                  s = "AGREE_UNIX_FD\r\n";
                  debug_print ("SERVER: writing '%s'", s);
                  if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                    goto out;
                }
              else
                {
                  s = "ERROR \"fd passing not offered\"\r\n";
                  debug_print ("SERVER: writing '%s'", s);
                  if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                    goto out;
                }
            }
          else
            {
              g_debug ("Unexpected line '%s' while in WaitingForBegin state", line);
              g_free (line);
              s = "ERROR \"Unknown Command\"\r\n";
              debug_print ("SERVER: writing '%s'", s);
              if (!g_data_output_stream_put_string (dos, s, cancellable, error))
                goto out;
            }
          break;

        default:
          g_assert_not_reached ();
          break;
        }
    }


  g_set_error_literal (error,
                       G_IO_ERROR,
                       G_IO_ERROR_FAILED,
                       "Not implemented (server)");

 out:
  g_clear_object (&mech);
  g_clear_object (&dis);
  g_clear_object (&dos);
  g_clear_object (&own_credentials);

  /* ensure return value is FALSE if error is set */
  if (error != NULL && *error != NULL)
    {
      ret = FALSE;
    }

  if (ret)
    {
      if (out_negotiated_capabilities != NULL)
        *out_negotiated_capabilities = negotiated_capabilities;
      if (out_received_credentials != NULL)
        *out_received_credentials = credentials != NULL ? xobject_ref (credentials) : NULL;
    }

  if (credentials != NULL)
    xobject_unref (credentials);

  debug_print ("SERVER: Done, authenticated=%d", ret);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */
