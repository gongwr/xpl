/* GLib testing framework examples and tests
 *
 * Copyright 2014 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <gio/gio.h>

static void
event_cb (xsocket_listener_t      *listener,
          GSocketListenerEvent  event,
          xsocket_t              *socket,
          xpointer_t              data)
{
  static GSocketListenerEvent expected_event = XSOCKET_LISTENER_BINDING;
  xboolean_t *success = (xboolean_t *)data;

  g_assert (X_IS_SOCKET_LISTENER (listener));
  g_assert (X_IS_SOCKET (socket));
  g_assert (event == expected_event);

  switch (event)
    {
      case XSOCKET_LISTENER_BINDING:
        expected_event = XSOCKET_LISTENER_BOUND;
        break;
      case XSOCKET_LISTENER_BOUND:
        expected_event = XSOCKET_LISTENER_LISTENING;
        break;
      case XSOCKET_LISTENER_LISTENING:
        expected_event = XSOCKET_LISTENER_LISTENED;
        break;
      case XSOCKET_LISTENER_LISTENED:
        *success = TRUE;
        break;
    }
}

static void
test_event_signal (void)
{
  xboolean_t success = FALSE;
  xinet_address_t *iaddr;
  xsocket_address_t *saddr;
  xsocket_listener_t *listener;
  xerror_t *error = NULL;

  iaddr = xinet_address_new_loopback (XSOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (iaddr, 0);
  xobject_unref (iaddr);

  listener = xsocket_listener_new ();

  g_signal_connect (listener, "event", G_CALLBACK (event_cb), &success);

  xsocket_listener_add_address (listener,
                                 saddr,
                                 XSOCKET_TYPE_STREAM,
                                 XSOCKET_PROTOCOL_TCP,
                                 NULL,
                                 NULL,
                                 &error);
  g_assert_no_error (error);
  g_assert_true (success);

  xobject_unref (saddr);
  xobject_unref (listener);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/socket-listener/event-signal", test_event_signal);

  return g_test_run();
}
