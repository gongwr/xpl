#include <gio/gio.h>
#include <gio/gnetworking.h>
#include <gio/gunixfdmessage.h>
#include <gio/gunixsocketaddress.h>
#include <string.h>
#include <unistd.h>

/* ensures that no FDs are left open at the end */
static void
check_fd_list (const xint_t *fd_list)
{
  xint_t i;

  for (i = 0; i < 40; i++)
    {
      int my_fd;

      my_fd = dup (0);
      g_assert_cmpint (fd_list[i], ==, my_fd);
    }

  for (i = 0; i < 40; i++)
    close (fd_list[i]);
}

static void
create_fd_list (xint_t *fd_list)
{
  xint_t i;

  for (i = 0; i < 40; i++)
    {
      fd_list[i] = dup (0);
      g_assert_cmpint (fd_list[i], >, 0);
    }

  for (i = 0; i < 40; i++)
    close (fd_list[i]);
}

static void
test_fds (void)
{
  xerror_t *err = NULL;
  GUnixFDMessage *message;
  GUnixFDMessage **mv;
  xunix_fd_list_t *list, *l2;
  xsocket_t *sockets[2];
  xsocket_address_t *addr;
  const xint_t *peek;
  char buffer[1024];
  xint_t fd_list[40];
  xoutput_vector_t ov;
  xinput_vector_t iv;
  xint_t *stolen;
  xint_t sv[3];
  xint_t flags;
  xint_t nm;
  xint_t s;
  xchar_t *path;
  xbyte_array_t *array;
  xboolean_t abstract;
  GUnixSocketAddressType type;

  create_fd_list (fd_list);

  s = socketpair (PF_UNIX, SOCK_STREAM, 0, sv);
  g_assert_cmpint (s, ==, 0);
  sv[2] = -1;

  list = g_unix_fd_list_new_from_array (sv, -1);
  message = G_UNIX_FD_MESSAGE (g_unix_fd_message_new_with_fd_list (list));
  xobject_unref (list);

  g_assert (g_unix_fd_message_get_fd_list (message) == list);
  xobject_get (message, "fd-list", &l2, NULL);
  g_assert (l2 == list);
  g_assert_cmpint (g_unix_fd_list_get_length (list), ==, 2);

  peek = g_unix_fd_list_peek_fds (list, &s);
  g_assert_cmpint (s, ==, 2);
  stolen = g_unix_fd_message_steal_fds (message, &s);
  g_assert_cmpint (s, ==, 2);
  g_assert (stolen == peek);

  g_assert_cmpint (stolen[0], ==, sv[0]);
  g_assert_cmpint (stolen[1], ==, sv[1]);
  g_assert_cmpint (stolen[2], ==, sv[2]);
  g_free (stolen);

  g_unix_fd_message_append_fd (message, sv[0], &err);
  g_assert_no_error (err);
  s = close (sv[0]);
  g_assert_cmpint (s, ==, 0);
  g_unix_fd_message_append_fd (message, sv[1], &err);
  g_assert_no_error (err);
  s = close (sv[1]);
  g_assert_cmpint (s, ==, 0);

  s = close (g_unix_fd_list_get (list, 0, &err));
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 0);
  s = close (g_unix_fd_list_get (list, 1, &err));
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 0);
  s = close (g_unix_fd_list_get (list, 0, &err));
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 0);
  s = close (g_unix_fd_list_get (list, 1, &err));
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 0);
  s = close (g_unix_fd_list_get (list, 0, &err));
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 0);
  s = close (g_unix_fd_list_get (list, 1, &err));
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 0);

  xobject_unref (message);
  xobject_unref (list);

  message = G_UNIX_FD_MESSAGE (g_unix_fd_message_new ());
  list = g_unix_fd_message_get_fd_list (message);
  s = pipe (sv);
  g_assert_cmpint (s, ==, 0);

  s = g_unix_fd_list_append (list, sv[0], &err);
  g_assert_no_error (err);
  g_assert_cmpint (s, >=, 0);
  s = g_unix_fd_list_append (list, sv[1], &err);
  g_assert_no_error (err);
  g_assert_cmpint (s, >=, 0);

  s = close (sv[0]);
  g_assert_cmpint (s, ==, 0);
  s = close (sv[1]);
  g_assert_cmpint (s, ==, 0);
  s = close (g_unix_fd_list_get (list, 0, &err));
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 0);
  s = close (g_unix_fd_list_get (list, 1, &err));
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 0);

  s = socketpair (PF_UNIX, SOCK_STREAM, 0, sv);
  g_assert_cmpint (s, ==, 0);

  sockets[0] = xsocket_new_from_fd (sv[0], &err);
  g_assert_no_error (err);
  g_assert (X_IS_SOCKET (sockets[0]));
  sockets[1] = xsocket_new_from_fd (sv[1], &err);
  g_assert_no_error (err);
  g_assert (X_IS_SOCKET (sockets[1]));

  addr = xsocket_get_local_address (sockets[0], &err);
  g_assert_no_error (err);
  g_assert (X_IS_UNIX_SOCKET_ADDRESS (addr));
  g_assert_cmpint (g_unix_socket_address_get_address_type (G_UNIX_SOCKET_ADDRESS (addr)), ==, G_UNIX_SOCKET_ADDRESS_ANONYMOUS);
  g_assert_cmpint (g_unix_socket_address_get_path_len (G_UNIX_SOCKET_ADDRESS (addr)), ==, 0);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_assert (!g_unix_socket_address_get_is_abstract (G_UNIX_SOCKET_ADDRESS (addr)));
G_GNUC_END_IGNORE_DEPRECATIONS

  xobject_get (addr,
                "path", &path,
                "path-as-array", &array,
                "abstract", &abstract,
                "address-type", &type,
                NULL);
  g_assert_cmpstr (path, ==, "");
  g_assert_cmpint (array->len, ==, 0);
  g_assert (!abstract);
  g_assert (type == G_UNIX_SOCKET_ADDRESS_ANONYMOUS);
  g_free (path);
  xbyte_array_free (array, TRUE);

  xobject_unref (addr);

  buffer[0] = 0xff;
  ov.buffer = buffer;
  ov.size = 1;
  s = xsocket_send_message (sockets[0], NULL, &ov, 1,
                             (xsocket_control_message_t **) &message,
                             1, 0, NULL, &err);
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 1);
  xobject_unref (message);

  message = NULL;

  flags = 0;
  iv.buffer = buffer;
  iv.size = 1;
  s = xsocket_receive_message (sockets[1], NULL, &iv, 1,
                                (xsocket_control_message_t ***) &mv,
                                &nm, &flags, NULL, &err);
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 1);
  xobject_unref (sockets[0]);
  xobject_unref (sockets[1]);

  g_assert_cmpint (nm, ==, 1);
  message = mv[0];
  g_free (mv);

  g_assert (X_IS_UNIX_FD_MESSAGE (message));
  list = xobject_ref (g_unix_fd_message_get_fd_list (message));
  xobject_unref (message);

  peek = g_unix_fd_list_peek_fds (list, &s);
  g_assert_cmpint (s, ==, 2);
  sv[0] = g_unix_fd_list_get (list, 1, &err);
  g_assert_no_error (err);

  strcpy (buffer, "failure to say failure to say 'i love gnome-panel!'.");
  s = write (sv[0], buffer, strlen (buffer) + 1);
  g_assert_cmpint (s, ==, strlen (buffer) + 1);

  close (sv[0]);
  memset (buffer, 0xff, sizeof buffer);

  s = read (peek[0], buffer, sizeof buffer);
  g_assert_cmpint (s, ==, 53);
  g_assert_cmpstr (buffer, ==,
                   "failure to say failure to say 'i love gnome-panel!'.");

  xobject_unref (list);

  check_fd_list (fd_list);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/unix-streams/file-descriptors", test_fds);

  return g_test_run();

}
