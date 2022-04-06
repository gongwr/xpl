#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>

#include <time.h>

#include <gio/gio.h>
#include <gio/gunixfdlist.h>

/* see gdbus-example-server.c for the server implementation */
static xint_t
get_server_stdout (xdbus_connection_t  *connection,
                   const xchar_t      *name_owner,
                   xerror_t          **error)
{
  xdbus_message_t *method_call_message;
  xdbus_message_t *method_reply_message;
  xunix_fd_list_t *fd_list;
  xint_t fd;

  fd = -1;
  method_call_message = NULL;
  method_reply_message = NULL;

  method_call_message = xdbus_message_new_method_call (name_owner,
                                                        "/org/gtk/GDBus/test_object_t",
                                                        "org.gtk.GDBus.test_interface_t",
                                                        "GimmeStdout");
  method_reply_message = xdbus_connection_send_message_with_reply_sync (connection,
                                                                         method_call_message,
                                                                         G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                                                         -1,
                                                                         NULL, /* out_serial */
                                                                         NULL, /* cancellable */
                                                                         error);
  if (method_reply_message == NULL)
      goto out;

  if (xdbus_message_get_message_type (method_reply_message) == G_DBUS_MESSAGE_TYPE_ERROR)
    {
      xdbus_message_to_gerror (method_reply_message, error);
      goto out;
    }

  fd_list = xdbus_message_get_unix_fd_list (method_reply_message);
  fd = g_unix_fd_list_get (fd_list, 0, error);

 out:
  xobject_unref (method_call_message);
  xobject_unref (method_reply_message);

  return fd;
}

static void
on_name_appeared (xdbus_connection_t *connection,
                  const xchar_t     *name,
                  const xchar_t     *name_owner,
                  xpointer_t         user_data)
{
  xint_t fd;
  xerror_t *error;

  error = NULL;
  fd = get_server_stdout (connection, name_owner, &error);
  if (fd == -1)
    {
      g_printerr ("Error invoking GimmeStdout(): %s\n",
                  error->message);
      xerror_free (error);
      exit (1);
    }
  else
    {
      xchar_t *now_buf = NULL;
      xssize_t len;
      xchar_t *str;
      xdatetime_t *now = xdate_time_new_now_local ();

      g_assert_nonnull (now);
      now_buf = xdate_time_format (now, "%Y-%m-%d %H:%M:%S");
      xdate_time_unref (now);

      str = xstrdup_printf ("On %s, gdbus-example-unix-fd-client with pid %d was here!\n",
                             now_buf,
                             (xint_t) getpid ());
      len = strlen (str);
      g_warn_if_fail (write (fd, str, len) == len);
      close (fd);

      g_print ("Wrote the following on server's stdout:\n%s", str);

      g_free (str);
      g_free (now_buf);
      exit (0);
    }
}

static void
on_name_vanished (xdbus_connection_t *connection,
                  const xchar_t     *name,
                  xpointer_t         user_data)
{
  g_printerr ("Failed to get name owner for %s\n"
              "Is ./gdbus-example-server running?\n",
              name);
  exit (1);
}

int
main (int argc, char *argv[])
{
  xuint_t watcher_id;
  xmain_loop_t *loop;

  watcher_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                                 "org.gtk.GDBus.TestServer",
                                 G_BUS_NAME_WATCHER_FLAGS_NONE,
                                 on_name_appeared,
                                 on_name_vanished,
                                 NULL,
                                 NULL);

  loop = xmain_loop_new (NULL, FALSE);
  xmain_loop_run (loop);

  g_bus_unwatch_name (watcher_id);
  return 0;
}
