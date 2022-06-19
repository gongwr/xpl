#include "config.h"
#include "mock-resolver.h"

#include <gio/gio.h>
#include <gio/gnetworking.h>

static void
test_basic (void)
{
  xnetwork_address_t *address;
  xuint_t port;
  xchar_t *hostname;
  xchar_t *scheme;

  address = (xnetwork_address_t*)g_network_address_new ("www.gnome.org", 8080);

  g_assert_cmpstr (g_network_address_get_hostname (address), ==, "www.gnome.org");
  g_assert_cmpint (g_network_address_get_port (address), ==, 8080);

  xobject_get (address, "hostname", &hostname, "port", &port, "scheme", &scheme, NULL);
  g_assert_cmpstr (hostname, ==, "www.gnome.org");
  g_assert_cmpint (port, ==, 8080);
  g_assert_null (scheme);
  g_free (hostname);

  xobject_unref (address);
}

typedef struct {
  const xchar_t *input;
  const xchar_t *scheme;
  const xchar_t *hostname;
  xuint16_t port;
  xint_t error_code;
} ParseTest;

static ParseTest uri_tests[] = {
  { "http://www.gnome.org:2020/start", "http", "www.gnome.org", 2020, -1 },
  { "ftp://joe~:(*)%46@ftp.gnome.org:2020/start", "ftp", "ftp.gnome.org", 2020, -1 },
  { "ftp://[fec0::abcd]/start", "ftp", "fec0::abcd", 8080, -1 },
  { "ftp://[fec0::abcd]:999/start", "ftp", "fec0::abcd", 999, -1 },
  { "ftp://joe%x-@ftp.gnome.org:2020/start", NULL, NULL, 0, G_IO_ERROR_INVALID_ARGUMENT },
  { "http://[fec0::abcd%em1]/start", NULL, NULL, 0, G_IO_ERROR_INVALID_ARGUMENT },
  { "http://[fec0::abcd%25em1]/start", "http", "fec0::abcd%em1", 8080, -1 },
  { "http://[fec0::abcd%10]/start", NULL, NULL, 0, G_IO_ERROR_INVALID_ARGUMENT },
  { "http://[fec0::abcd%25em%31]/start", "http", "fec0::abcd%em1", 8080, -1 },
  { "ftp://ftp.gnome.org/start?foo=bar@baz", "ftp", "ftp.gnome.org", 8080, -1 }
};

static void
test_parse_uri (xconstpointer d)
{
  const ParseTest *test = d;
  xnetwork_address_t *address;
  xerror_t *error;

  error = NULL;
  address = (xnetwork_address_t*)g_network_address_parse_uri (test->input, 8080, &error);

  if (address)
    {
      g_assert_cmpstr (g_network_address_get_scheme (address), ==, test->scheme);
      g_assert_cmpstr (g_network_address_get_hostname (address), ==, test->hostname);
      g_assert_cmpint (g_network_address_get_port (address), ==, test->port);
      g_assert_no_error (error);
    }
  else
    g_assert_error (error, G_IO_ERROR, test->error_code);

  if (address)
    xobject_unref (address);
  if (error)
    xerror_free (error);
}

static ParseTest host_tests[] =
{
  { "www.gnome.org", NULL, "www.gnome.org", 1234, -1 },
  { "www.gnome.org:8080", NULL, "www.gnome.org", 8080, -1 },
  { "[2001:db8::1]", NULL, "2001:db8::1", 1234, -1 },
  { "[2001:db8::1]:888", NULL, "2001:db8::1", 888, -1 },
  { "[2001:db8::1%em1]", NULL, "2001:db8::1%em1", 1234, -1 },
  { "[2001:db8::1%25em1]", NULL, "2001:db8::1%25em1", 1234, -1 },
  { "[hostname", NULL, NULL, 0, G_IO_ERROR_INVALID_ARGUMENT },
  { "[hostnam]e", NULL, NULL, 0, G_IO_ERROR_INVALID_ARGUMENT },
  { "hostname:", NULL, NULL, 0, G_IO_ERROR_INVALID_ARGUMENT },
  { "hostname:-1", NULL, NULL, 0, G_IO_ERROR_INVALID_ARGUMENT },
  { "hostname:9999999", NULL, NULL, 0, G_IO_ERROR_INVALID_ARGUMENT }
};

static void
test_parse_host (xconstpointer d)
{
  const ParseTest *test = d;
  xnetwork_address_t *address;
  xerror_t *error;

  error = NULL;
  address = (xnetwork_address_t*)g_network_address_parse (test->input, 1234, &error);

  if (address)
    {
      g_assert_null (g_network_address_get_scheme (address));
      g_assert_cmpstr (g_network_address_get_hostname (address), ==, test->hostname);
      g_assert_cmpint (g_network_address_get_port (address), ==, test->port);
      g_assert_no_error (error);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, test->error_code);
    }

  if (address)
    xobject_unref (address);
  if (error)
    xerror_free (error);
}

typedef struct {
  const xchar_t *input;
  xboolean_t valid_parse, valid_resolve, valid_ip;
} ResolveTest;

static ResolveTest address_tests[] = {
  { "192.168.1.2",         TRUE,  TRUE,  TRUE },
  { "fe80::42",            TRUE,  TRUE,  TRUE },

  /* g_network_address_parse() accepts these, but they are not
   * (just) IP addresses.
   */
  { "192.168.1.2:80",      TRUE,  FALSE, FALSE },
  { "[fe80::42]",          TRUE,  FALSE, FALSE },
  { "[fe80::42]:80",       TRUE,  FALSE, FALSE },

  /* These should not be considered IP addresses by anyone. */
  { "192.168.258",         FALSE, FALSE, FALSE },
  { "192.11010306",        FALSE, FALSE, FALSE },
  { "3232235778",          FALSE, FALSE, FALSE },
  { "0300.0250.0001.0001", FALSE, FALSE, FALSE },
  { "0xC0.0xA8.0x01.0x02", FALSE, FALSE, FALSE },
  { "0xc0.0xa8.0x01.0x02", FALSE, FALSE, FALSE },
  { "0xc0a80102",          FALSE, FALSE, FALSE }
};

static void
test_resolve_address (xconstpointer d)
{
  const ResolveTest *test = d;
  xsocket_connectable_t *connectable;
  xsocket_address_enumerator_t *addr_enum;
  xsocket_address_t *addr;
  xerror_t *error = NULL;

  g_test_message ("Input: %s", test->input);

  g_assert_cmpint (test->valid_ip, ==, g_hostname_is_ip_address (test->input));

  connectable = g_network_address_parse (test->input, 1234, &error);
  g_assert_no_error (error);

  addr_enum = xsocket_connectable_enumerate (connectable);
  addr = xsocket_address_enumerator_next (addr_enum, NULL, &error);
  xobject_unref (addr_enum);
  xobject_unref (connectable);

  if (addr)
    {
      g_assert_true (test->valid_parse);
      g_assert_true (X_IS_INET_SOCKET_ADDRESS (addr));
      xobject_unref (addr);
    }
  else
    {
      g_assert_false (test->valid_parse);
      g_assert_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND);
      xerror_free (error);
      return;
    }
}

/* Technically this should be in a xresolver_t test program, but we don't
 * have one of those since it's mostly impossible to test programmatically.
 * So it goes here so it can share the tests.
 */
static void
test_resolve_address_gresolver (xconstpointer d)
{
  const ResolveTest *test = d;
  xresolver_t *resolver;
  xlist_t *addrs;
  xinet_address_t *iaddr;
  xerror_t *error = NULL;

  g_test_message ("Input: %s", test->input);

  resolver = g_resolver_get_default ();
  addrs = g_resolver_lookup_by_name (resolver, test->input, NULL, &error);
  xobject_unref (resolver);

  if (addrs)
    {
      g_assert_true (test->valid_resolve);
      g_assert_cmpint (xlist_length (addrs), ==, 1);

      iaddr = addrs->data;
      g_assert_true (X_IS_INET_ADDRESS (iaddr));

      xobject_unref (iaddr);
      xlist_free (addrs);
    }
  else
    {
      g_assert_nonnull (error);
      g_test_message ("Error: %s", error->message);
      g_assert_false (test->valid_resolve);

      if (!test->valid_parse)
        {
          /* xresolver_t should have rejected the address internally, in
           * which case we're guaranteed to get G_RESOLVER_ERROR_NOT_FOUND.
           */
          g_assert_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND);
        }
      else
        {
          /* If xresolver_t didn't reject the string itself, then we
           * might have attempted to send it over the network. If that
           * attempt succeeded, we'd get back NOT_FOUND, but if
           * there's no network available we might have gotten some
           * other error instead.
           */
        }

      xerror_free (error);
      return;
    }
}

#define SCOPE_ID_TEST_ADDR "fe80::42"
#define SCOPE_ID_TEST_PORT 99

#if defined (HAVE_IF_INDEXTONAME) && defined (HAVE_IF_NAMETOINDEX)
static char SCOPE_ID_TEST_IFNAME[IF_NAMESIZE];
static int SCOPE_ID_TEST_INDEX;
#else
#define SCOPE_ID_TEST_IFNAME "1"
#define SCOPE_ID_TEST_INDEX 1
#endif

static void
find_ifname_and_index (void)
{
  if (SCOPE_ID_TEST_INDEX != 0)
    return;

#if defined (HAVE_IF_INDEXTONAME) && defined (HAVE_IF_NAMETOINDEX)
  SCOPE_ID_TEST_INDEX = if_nametoindex ("lo");
  if (SCOPE_ID_TEST_INDEX != 0)
    {
      xstrlcpy (SCOPE_ID_TEST_IFNAME, "lo", sizeof (SCOPE_ID_TEST_IFNAME));
      return;
    }

  for (SCOPE_ID_TEST_INDEX = 1; SCOPE_ID_TEST_INDEX < 1024; SCOPE_ID_TEST_INDEX++) {
    if (if_indextoname (SCOPE_ID_TEST_INDEX, SCOPE_ID_TEST_IFNAME))
      break;
  }
  g_assert_cmpstr (SCOPE_ID_TEST_IFNAME, !=, "");
#endif
}

static void
test_scope_id (xsocket_connectable_t *addr)
{
#ifndef G_OS_WIN32
  xsocket_address_enumerator_t *addr_enum;
  xsocket_address_t *saddr;
  xinet_socket_address_t *isaddr;
  xinet_address_t *iaddr;
  char *tostring;
  xerror_t *error = NULL;

  addr_enum = xsocket_connectable_enumerate (addr);
  saddr = xsocket_address_enumerator_next (addr_enum, NULL, &error);
  g_assert_no_error (error);

  g_assert_nonnull (saddr);
  g_assert_true (X_IS_INET_SOCKET_ADDRESS (saddr));

  isaddr = G_INET_SOCKET_ADDRESS (saddr);
  g_assert_cmpint (g_inet_socket_address_get_scope_id (isaddr), ==, SCOPE_ID_TEST_INDEX);
  g_assert_cmpint (g_inet_socket_address_get_port (isaddr), ==, SCOPE_ID_TEST_PORT);

  iaddr = g_inet_socket_address_get_address (isaddr);
  tostring = xinet_address_to_string (iaddr);
  g_assert_cmpstr (tostring, ==, SCOPE_ID_TEST_ADDR);
  g_free (tostring);

  xobject_unref (saddr);
  saddr = xsocket_address_enumerator_next (addr_enum, NULL, &error);
  g_assert_no_error (error);
  g_assert_null (saddr);

  xobject_unref (addr_enum);
#else
  g_test_skip ("winsock2 getaddrinfo() can’t understand scope IDs");
#endif
}

static void
test_host_scope_id (void)
{
  xsocket_connectable_t *addr;
  char *str;

  find_ifname_and_index ();

  str = xstrdup_printf ("%s%%%s", SCOPE_ID_TEST_ADDR, SCOPE_ID_TEST_IFNAME);
  addr = g_network_address_new (str, SCOPE_ID_TEST_PORT);
  g_free (str);

  test_scope_id (addr);
  xobject_unref (addr);
}

static void
test_uri_scope_id (void)
{
  xsocket_connectable_t *addr;
  char *uri;
  xerror_t *error = NULL;

  find_ifname_and_index ();

  uri = xstrdup_printf ("http://[%s%%%s]:%d/foo",
                         SCOPE_ID_TEST_ADDR,
                         SCOPE_ID_TEST_IFNAME,
                         SCOPE_ID_TEST_PORT);
  addr = g_network_address_parse_uri (uri, 0, &error);
  g_free (uri);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
  g_assert_null (addr);
  g_clear_error (&error);

  uri = xstrdup_printf ("http://[%s%%25%s]:%d/foo",
                         SCOPE_ID_TEST_ADDR,
                         SCOPE_ID_TEST_IFNAME,
                         SCOPE_ID_TEST_PORT);
  addr = g_network_address_parse_uri (uri, 0, &error);
  g_free (uri);
  g_assert_no_error (error);

  test_scope_id (addr);
  xobject_unref (addr);
}

static void
test_loopback_basic (void)
{
  xnetwork_address_t *addr;  /* owned */

  addr = G_NETWORK_ADDRESS (g_network_address_new_loopback (666));

  /* test_t basic properties. */
  g_assert_cmpstr (g_network_address_get_hostname (addr), ==, "localhost");
  g_assert_cmpuint (g_network_address_get_port (addr), ==, 666);
  g_assert_null (g_network_address_get_scheme (addr));

  xobject_unref (addr);
}

static void
assert_socket_address_matches (xsocket_address_t *a,
                               const xchar_t    *expected_address,
                               xuint16_t         expected_port)
{
  xinet_socket_address_t *sa;
  xchar_t *str;  /* owned */

  g_assert_true (X_IS_INET_SOCKET_ADDRESS (a));

  sa = G_INET_SOCKET_ADDRESS (a);
  g_assert_cmpint (g_inet_socket_address_get_port (sa), ==, expected_port);

  str = xinet_address_to_string (g_inet_socket_address_get_address (sa));
  g_assert_cmpstr (str, ==, expected_address);
  g_free (str);
}

static void
test_loopback_sync (void)
{
  xsocket_connectable_t *addr;  /* owned */
  xsocket_address_enumerator_t *enumerator;  /* owned */
  xsocket_address_t *a;  /* owned */
  xerror_t *error = NULL;

  addr = g_network_address_new_loopback (616);
  enumerator = xsocket_connectable_enumerate (addr);

  /* IPv6 address. */
  a = xsocket_address_enumerator_next (enumerator, NULL, &error);
  g_assert_no_error (error);
  assert_socket_address_matches (a, "::1", 616);
  xobject_unref (a);

  /* IPv4 address. */
  a = xsocket_address_enumerator_next (enumerator, NULL, &error);
  g_assert_no_error (error);
  assert_socket_address_matches (a, "127.0.0.1", 616);
  xobject_unref (a);

  /* End of results. */
  g_assert_null (xsocket_address_enumerator_next (enumerator, NULL, &error));
  g_assert_no_error (error);

  xobject_unref (enumerator);
  xobject_unref (addr);
}

static void
test_localhost_sync (void)
{
  xsocket_connectable_t *addr;  /* owned */
  xsocket_address_enumerator_t *enumerator;  /* owned */
  xsocket_address_t *a;  /* owned */
  xerror_t *error = NULL;
  xresolver_t *original_resolver; /* owned */
  MockResolver *mock_resolver; /* owned */
  xlist_t *ipv4_results = NULL; /* owned */

  /* This test ensures that variations of the "localhost" hostname always resolve to a loopback address */

  /* Set up a DNS resolver that returns nonsense for "localhost" */
  original_resolver = g_resolver_get_default ();
  mock_resolver = mock_resolver_new ();
  g_resolver_set_default (G_RESOLVER (mock_resolver));
  ipv4_results = xlist_append (ipv4_results, xinet_address_new_from_string ("123.123.123.123"));
  mock_resolver_set_ipv4_results (mock_resolver, ipv4_results);

  addr = g_network_address_new ("localhost.", 616);
  enumerator = xsocket_connectable_enumerate (addr);

  /* IPv6 address. */
  a = xsocket_address_enumerator_next (enumerator, NULL, &error);
  g_assert_no_error (error);
  assert_socket_address_matches (a, "::1", 616);
  xobject_unref (a);

  /* IPv4 address. */
  a = xsocket_address_enumerator_next (enumerator, NULL, &error);
  g_assert_no_error (error);
  assert_socket_address_matches (a, "127.0.0.1", 616);
  xobject_unref (a);

  /* End of results. */
  g_assert_null (xsocket_address_enumerator_next (enumerator, NULL, &error));
  g_assert_no_error (error);
  xobject_unref (enumerator);
  xobject_unref (addr);

  addr = g_network_address_new (".localhost", 616);
  enumerator = xsocket_connectable_enumerate (addr);

  /* IPv6 address. */
  a = xsocket_address_enumerator_next (enumerator, NULL, &error);
  g_assert_no_error (error);
  assert_socket_address_matches (a, "::1", 616);
  xobject_unref (a);

  /* IPv4 address. */
  a = xsocket_address_enumerator_next (enumerator, NULL, &error);
  g_assert_no_error (error);
  assert_socket_address_matches (a, "127.0.0.1", 616);
  xobject_unref (a);

  /* End of results. */
  g_assert_null (xsocket_address_enumerator_next (enumerator, NULL, &error));
  g_assert_no_error (error);
  xobject_unref (enumerator);
  xobject_unref (addr);

  addr = g_network_address_new ("foo.localhost", 616);
  enumerator = xsocket_connectable_enumerate (addr);

  /* IPv6 address. */
  a = xsocket_address_enumerator_next (enumerator, NULL, &error);
  g_assert_no_error (error);
  assert_socket_address_matches (a, "::1", 616);
  xobject_unref (a);

  /* IPv4 address. */
  a = xsocket_address_enumerator_next (enumerator, NULL, &error);
  g_assert_no_error (error);
  assert_socket_address_matches (a, "127.0.0.1", 616);
  xobject_unref (a);

  /* End of results. */
  g_assert_null (xsocket_address_enumerator_next (enumerator, NULL, &error));
  g_assert_no_error (error);
  xobject_unref (enumerator);
  xobject_unref (addr);

  addr = g_network_address_new (".localhost.", 616);
  enumerator = xsocket_connectable_enumerate (addr);

  /* IPv6 address. */
  a = xsocket_address_enumerator_next (enumerator, NULL, &error);
  g_assert_no_error (error);
  assert_socket_address_matches (a, "::1", 616);
  xobject_unref (a);

  /* IPv4 address. */
  a = xsocket_address_enumerator_next (enumerator, NULL, &error);
  g_assert_no_error (error);
  assert_socket_address_matches (a, "127.0.0.1", 616);
  xobject_unref (a);

  /* End of results. */
  g_assert_null (xsocket_address_enumerator_next (enumerator, NULL, &error));
  g_assert_no_error (error);
  xobject_unref (enumerator);
  xobject_unref (addr);

  addr = g_network_address_new ("invalid", 616);
  enumerator = xsocket_connectable_enumerate (addr);

  /* IPv4 address. */
  a = xsocket_address_enumerator_next (enumerator, NULL, &error);
  g_assert_no_error (error);
  assert_socket_address_matches (a, "123.123.123.123", 616);
  xobject_unref (a);

  /* End of results. */
  g_assert_null (xsocket_address_enumerator_next (enumerator, NULL, &error));
  g_assert_no_error (error);
  xobject_unref (enumerator);
  xobject_unref (addr);

  g_resolver_set_default (original_resolver);
  xlist_free_full (ipv4_results, (xdestroy_notify_t) xobject_unref);
  xobject_unref (original_resolver);
  xobject_unref (mock_resolver);
}

typedef struct {
  xlist_t/*<owned xsocket_address_t> */ *addrs;  /* owned */
  xmain_loop_t *loop;  /* owned */
  xsocket_address_enumerator_t *enumerator; /* unowned */
  xuint_t delay_ms;
  xint_t expected_error_code;
} AsyncData;

static void got_addr (xobject_t *source_object, xasync_result_t *result, xpointer_t user_data);

static int
on_delayed_get_addr (xpointer_t user_data)
{
  AsyncData *data = user_data;
  xsocket_address_enumerator_next_async (data->enumerator, NULL,
                                          got_addr, user_data);
  return XSOURCE_REMOVE;
}

static void
got_addr (xobject_t      *source_object,
          xasync_result_t *result,
          xpointer_t      user_data)
{
  xsocket_address_enumerator_t *enumerator;
  AsyncData *data;
  xsocket_address_t *a;  /* owned */
  xerror_t *error = NULL;

  enumerator = XSOCKET_ADDRESS_ENUMERATOR (source_object);
  data = user_data;

  a = xsocket_address_enumerator_next_finish (enumerator, result, &error);

  if (data->expected_error_code)
    {
      g_assert_error (error, G_IO_ERROR, data->expected_error_code);
      g_clear_error (&error);
    }
  else
    g_assert_no_error (error);

  if (a == NULL)
    {
      /* End of results. */
      data->addrs = xlist_reverse (data->addrs);
      xmain_loop_quit (data->loop);
    }
  else
    {
      g_assert_true (X_IS_INET_SOCKET_ADDRESS (a));
      data->addrs = xlist_prepend (data->addrs, a);

      if (!data->delay_ms)
        xsocket_address_enumerator_next_async (enumerator, NULL,
                                                got_addr, user_data);
      else
        {
          data->enumerator = enumerator;
          g_timeout_add (data->delay_ms, on_delayed_get_addr, data);
        }
    }
}

static void
got_addr_ignored (xobject_t      *source_object,
                  xasync_result_t *result,
                  xpointer_t      user_data)
{
  xsocket_address_enumerator_t *enumerator;
  xsocket_address_t *a;  /* owned */
  xerror_t *error = NULL;

  /* This function simply ignores the returned addresses but keeps enumerating */

  enumerator = XSOCKET_ADDRESS_ENUMERATOR (source_object);

  a = xsocket_address_enumerator_next_finish (enumerator, result, &error);
  g_assert_no_error (error);
  if (a != NULL)
    {
      xobject_unref (a);
      xsocket_address_enumerator_next_async (enumerator, NULL,
                                              got_addr_ignored, user_data);
    }
}


static void
test_loopback_async (void)
{
  xsocket_connectable_t *addr;  /* owned */
  xsocket_address_enumerator_t *enumerator;  /* owned */
  AsyncData data = { 0, };

  addr = g_network_address_new_loopback (610);
  enumerator = xsocket_connectable_enumerate (addr);

  /* Get all the addresses. */
  data.addrs = NULL;
  data.loop = xmain_loop_new (NULL, FALSE);

  xsocket_address_enumerator_next_async (enumerator, NULL, got_addr, &data);

  xmain_loop_run (data.loop);
  xmain_loop_unref (data.loop);

  /* Check results. */
  g_assert_cmpuint (xlist_length (data.addrs), ==, 2);
  assert_socket_address_matches (data.addrs->data, "::1", 610);
  assert_socket_address_matches (data.addrs->next->data, "127.0.0.1", 610);

  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);

  xobject_unref (enumerator);
  xobject_unref (addr);
}

static void
test_localhost_async (void)
{
  xsocket_connectable_t *addr;  /* owned */
  xsocket_address_enumerator_t *enumerator;  /* owned */
  AsyncData data = { 0, };
  xresolver_t *original_resolver; /* owned */
  MockResolver *mock_resolver; /* owned */
  xlist_t *ipv4_results = NULL; /* owned */

  /* This test ensures that variations of the "localhost" hostname always resolve to a loopback address */

  /* Set up a DNS resolver that returns nonsense for "localhost" */
  original_resolver = g_resolver_get_default ();
  mock_resolver = mock_resolver_new ();
  g_resolver_set_default (G_RESOLVER (mock_resolver));
  ipv4_results = xlist_append (ipv4_results, xinet_address_new_from_string ("123.123.123.123"));
  mock_resolver_set_ipv4_results (mock_resolver, ipv4_results);

  addr = g_network_address_new ("localhost", 610);
  enumerator = xsocket_connectable_enumerate (addr);

  /* Get all the addresses. */
  data.addrs = NULL;
  data.delay_ms = 1;
  data.loop = xmain_loop_new (NULL, FALSE);

  xsocket_address_enumerator_next_async (enumerator, NULL, got_addr, &data);
  xmain_loop_run (data.loop);

  /* Check results. */
  g_assert_cmpuint (xlist_length (data.addrs), ==, 2);
  assert_socket_address_matches (data.addrs->data, "::1", 610);
  assert_socket_address_matches (data.addrs->next->data, "127.0.0.1", 610);

  g_resolver_set_default (original_resolver);
  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
  xlist_free_full (ipv4_results, (xdestroy_notify_t) xobject_unref);
  xobject_unref (original_resolver);
  xobject_unref (mock_resolver);
  xobject_unref (enumerator);
  xobject_unref (addr);
  xmain_loop_unref (data.loop);
}

static void
test_to_string (void)
{
  xsocket_connectable_t *addr = NULL;
  xchar_t *str = NULL;
  xerror_t *error = NULL;

  /* Without port. */
  addr = g_network_address_new ("some-hostname", 0);
  str = xsocket_connectable_to_string (addr);
  g_assert_cmpstr (str, ==, "some-hostname");
  g_free (str);
  xobject_unref (addr);

  /* With port. */
  addr = g_network_address_new ("some-hostname", 123);
  str = xsocket_connectable_to_string (addr);
  g_assert_cmpstr (str, ==, "some-hostname:123");
  g_free (str);
  xobject_unref (addr);

  /* With scheme and port. */
  addr = g_network_address_parse_uri ("http://some-hostname:123", 80, &error);
  g_assert_no_error (error);
  str = xsocket_connectable_to_string (addr);
  g_assert_cmpstr (str, ==, "http:some-hostname:123");
  g_free (str);
  xobject_unref (addr);

  /* Loopback. */
  addr = g_network_address_new ("localhost", 456);
  str = xsocket_connectable_to_string (addr);
  g_assert_cmpstr (str, ==, "localhost:456");
  g_free (str);
  xobject_unref (addr);
}

static int
sort_addresses (xconstpointer a, xconstpointer b)
{
  xsocket_family_t family_a = xinet_address_get_family (G_INET_ADDRESS (a));
  xsocket_family_t family_b = xinet_address_get_family (G_INET_ADDRESS (b));

  if (family_a == family_b)
    return 0;
  else if (family_a == XSOCKET_FAMILY_IPV4)
    return -1;
  else
    return 1;
}

static int
sort_socket_addresses (xconstpointer a, xconstpointer b)
{
  xinet_address_t *addr_a = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (a));
  xinet_address_t *addr_b = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (b));
  return sort_addresses (addr_a, addr_b);
}

static void
assert_list_matches_expected (xlist_t *result, xlist_t *expected)
{
  xlist_t *result_copy = NULL;

  g_assert_cmpint (xlist_length (result), ==, xlist_length (expected));

  /* Sort by ipv4 first which matches the expected list. Do this on a copy of
   * @result to avoid modifying the original. */
  result_copy = xlist_copy (result);
  result = result_copy = xlist_sort (result_copy, sort_socket_addresses);

  for (; result != NULL; result = result->next, expected = expected->next)
    {
      xinet_address_t *address = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (result->data));
      g_assert_true (xinet_address_equal (address, expected->data));
    }

  xlist_free (result_copy);
}

typedef struct {
  MockResolver *mock_resolver;
  xresolver_t *original_resolver;
  xlist_t *input_ipv4_results;
  xlist_t *input_ipv6_results;
  xlist_t *input_all_results;
  xsocket_connectable_t *addr;
  xsocket_address_enumerator_t *enumerator;
  xmain_loop_t *loop;
} HappyEyeballsFixture;

static void
happy_eyeballs_setup (HappyEyeballsFixture *fixture,
                      xconstpointer         data)
{
  static const char * const ipv4_address_strings[] = { "1.1.1.1", "2.2.2.2" };
  static const char * const ipv6_address_strings[] = { "ff::11", "ff::22" };
  xsize_t i;

  fixture->original_resolver = g_resolver_get_default ();
  fixture->mock_resolver = mock_resolver_new ();
  g_resolver_set_default (G_RESOLVER (fixture->mock_resolver));

  for (i = 0; i < G_N_ELEMENTS (ipv4_address_strings); ++i)
    {
      xinet_address_t *ipv4_addr = xinet_address_new_from_string (ipv4_address_strings[i]);
      xinet_address_t *ipv6_addr = xinet_address_new_from_string (ipv6_address_strings[i]);
      fixture->input_ipv4_results = xlist_append (fixture->input_ipv4_results, ipv4_addr);
      fixture->input_ipv6_results = xlist_append (fixture->input_ipv6_results, ipv6_addr);
      fixture->input_all_results = xlist_append (fixture->input_all_results, ipv4_addr);
      fixture->input_all_results = xlist_append (fixture->input_all_results, ipv6_addr);
    }
  fixture->input_all_results = xlist_sort (fixture->input_all_results, sort_addresses);
  mock_resolver_set_ipv4_results (fixture->mock_resolver, fixture->input_ipv4_results);
  mock_resolver_set_ipv6_results (fixture->mock_resolver, fixture->input_ipv6_results);

  fixture->addr = g_network_address_new ("test.fake", 80);
  fixture->enumerator = xsocket_connectable_enumerate (fixture->addr);

  fixture->loop = xmain_loop_new (NULL, FALSE);
}

static void
happy_eyeballs_teardown (HappyEyeballsFixture *fixture,
                         xconstpointer         data)
{
  xobject_unref (fixture->addr);
  xobject_unref (fixture->enumerator);
  g_resolver_free_addresses (fixture->input_all_results);
  xlist_free (fixture->input_ipv4_results);
  xlist_free (fixture->input_ipv6_results);
  g_resolver_set_default (fixture->original_resolver);
  xobject_unref (fixture->original_resolver);
  xobject_unref (fixture->mock_resolver);
  xmain_loop_unref (fixture->loop);
}

static const xuint_t FAST_DELAY_LESS_THAN_TIMEOUT = 25;
static const xuint_t SLOW_DELAY_MORE_THAN_TIMEOUT = 100;

static void
test_happy_eyeballs_basic (HappyEyeballsFixture *fixture,
                           xconstpointer         user_data)
{
  AsyncData data = { 0 };

  data.delay_ms = FAST_DELAY_LESS_THAN_TIMEOUT;
  data.loop = fixture->loop;

  /* This just tests in the common case it gets all results */

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  assert_list_matches_expected (data.addrs, fixture->input_all_results);

  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
}

static void
test_happy_eyeballs_parallel (HappyEyeballsFixture *fixture,
                              xconstpointer         user_data)
{
  AsyncData data = { 0 };
  xsocket_address_enumerator_t *enumerator2;

  enumerator2 = xsocket_connectable_enumerate (fixture->addr);

  data.delay_ms = FAST_DELAY_LESS_THAN_TIMEOUT;
  data.loop = fixture->loop;

  /* We run multiple enumerations at once, the results shouldn't be affected. */

  xsocket_address_enumerator_next_async (enumerator2, NULL, got_addr_ignored, &data);
  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  assert_list_matches_expected (data.addrs, fixture->input_all_results);

  /* Run again to ensure the cache from the previous one is correct */

  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
  data.addrs = NULL;
  xobject_unref (enumerator2);

  enumerator2 = xsocket_connectable_enumerate (fixture->addr);
  xsocket_address_enumerator_next_async (enumerator2, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  assert_list_matches_expected (data.addrs, fixture->input_all_results);

  xobject_unref (enumerator2);
  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
}

static void
test_happy_eyeballs_slow_ipv4 (HappyEyeballsFixture *fixture,
                               xconstpointer         user_data)
{
  AsyncData data = { 0 };

  /* If ipv4 dns response is a bit slow we still get everything */

  data.loop = fixture->loop;
  mock_resolver_set_ipv4_delay_ms (fixture->mock_resolver, FAST_DELAY_LESS_THAN_TIMEOUT);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  assert_list_matches_expected (data.addrs, fixture->input_all_results);

  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
}

static void
test_happy_eyeballs_slow_ipv6 (HappyEyeballsFixture *fixture,
                               xconstpointer         user_data)
{
  AsyncData data = { 0 };

  /* If ipv6 is a bit slow it waits for them */

  data.loop = fixture->loop;
  mock_resolver_set_ipv6_delay_ms (fixture->mock_resolver, FAST_DELAY_LESS_THAN_TIMEOUT);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  assert_list_matches_expected (data.addrs, fixture->input_all_results);

  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
}

static void
test_happy_eyeballs_very_slow_ipv6 (HappyEyeballsFixture *fixture,
                                    xconstpointer         user_data)
{
  AsyncData data = { 0 };

  /* If ipv6 is very slow we still get everything */

  data.loop = fixture->loop;
  mock_resolver_set_ipv6_delay_ms (fixture->mock_resolver, SLOW_DELAY_MORE_THAN_TIMEOUT);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  assert_list_matches_expected (data.addrs, fixture->input_all_results);

  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
}

static void
test_happy_eyeballs_slow_connection_and_ipv4 (HappyEyeballsFixture *fixture,
                                              xconstpointer         user_data)
{
  AsyncData data = { 0 };

  /* Even if the dns response is slow we still get them if our connection attempts
   * take long enough. */

  data.loop = fixture->loop;
  data.delay_ms = SLOW_DELAY_MORE_THAN_TIMEOUT * 2;
  mock_resolver_set_ipv4_delay_ms (fixture->mock_resolver, SLOW_DELAY_MORE_THAN_TIMEOUT);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  assert_list_matches_expected (data.addrs, fixture->input_all_results);

  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
}

static void
test_happy_eyeballs_ipv6_error_ipv4_first (HappyEyeballsFixture *fixture,
                                           xconstpointer         user_data)
{
  AsyncData data = { 0 };
  xerror_t *ipv6_error;

  /* If ipv6 fails, ensuring that ipv4 finishes before ipv6 errors, we still get ipv4. */

  data.loop = fixture->loop;
  ipv6_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv6 Broken");
  mock_resolver_set_ipv6_error (fixture->mock_resolver, ipv6_error);
  mock_resolver_set_ipv6_delay_ms (fixture->mock_resolver, FAST_DELAY_LESS_THAN_TIMEOUT);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  assert_list_matches_expected (data.addrs, fixture->input_ipv4_results);

  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
  xerror_free (ipv6_error);
}

static void
test_happy_eyeballs_ipv6_error_ipv6_first (HappyEyeballsFixture *fixture,
                                           xconstpointer         user_data)
{
  AsyncData data = { 0 };
  xerror_t *ipv6_error;

  /* If ipv6 fails, ensuring that ipv6 errors before ipv4 finishes, we still get ipv4. */

  data.loop = fixture->loop;
  ipv6_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv6 Broken");
  mock_resolver_set_ipv6_error (fixture->mock_resolver, ipv6_error);
  mock_resolver_set_ipv4_delay_ms (fixture->mock_resolver, FAST_DELAY_LESS_THAN_TIMEOUT);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  assert_list_matches_expected (data.addrs, fixture->input_ipv4_results);

  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
  xerror_free (ipv6_error);
}

static void
test_happy_eyeballs_ipv6_error_ipv4_very_slow (HappyEyeballsFixture *fixture,
                                               xconstpointer         user_data)
{
  AsyncData data = { 0 };
  xerror_t *ipv6_error;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/merge_requests/865");
  g_test_summary ("Ensure that we successfully return IPv4 results even when they come significantly later than an IPv6 failure.");

  /* If ipv6 fails, ensuring that ipv6 errors before ipv4 finishes, we still get ipv4. */

  data.loop = fixture->loop;
  ipv6_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv6 Broken");
  mock_resolver_set_ipv6_error (fixture->mock_resolver, ipv6_error);
  mock_resolver_set_ipv4_delay_ms (fixture->mock_resolver, SLOW_DELAY_MORE_THAN_TIMEOUT);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  assert_list_matches_expected (data.addrs, fixture->input_ipv4_results);

  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
  xerror_free (ipv6_error);
}

static void
test_happy_eyeballs_ipv4_error_ipv4_first (HappyEyeballsFixture *fixture,
                                           xconstpointer         user_data)
{
  AsyncData data = { 0 };
  xerror_t *ipv4_error;

  /* If ipv4 fails, ensuring that ipv4 errors before ipv6 finishes, we still get ipv6. */

  data.loop = fixture->loop;
  ipv4_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv4 Broken");
  mock_resolver_set_ipv4_error (fixture->mock_resolver, ipv4_error);
  mock_resolver_set_ipv6_delay_ms (fixture->mock_resolver, FAST_DELAY_LESS_THAN_TIMEOUT);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  assert_list_matches_expected (data.addrs, fixture->input_ipv6_results);

  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
  xerror_free (ipv4_error);
}

static void
test_happy_eyeballs_ipv4_error_ipv6_first (HappyEyeballsFixture *fixture,
                                           xconstpointer         user_data)
{
  AsyncData data = { 0 };
  xerror_t *ipv4_error;

  /* If ipv4 fails, ensuring that ipv6 finishes before ipv4 errors, we still get ipv6. */

  data.loop = fixture->loop;
  ipv4_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv4 Broken");
  mock_resolver_set_ipv4_error (fixture->mock_resolver, ipv4_error);
  mock_resolver_set_ipv4_delay_ms (fixture->mock_resolver, FAST_DELAY_LESS_THAN_TIMEOUT);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  assert_list_matches_expected (data.addrs, fixture->input_ipv6_results);

  xlist_free_full (data.addrs, (xdestroy_notify_t) xobject_unref);
  xerror_free (ipv4_error);
}

static void
test_happy_eyeballs_both_error (HappyEyeballsFixture *fixture,
                                xconstpointer         user_data)
{
  AsyncData data = { 0 };
  xerror_t *ipv4_error, *ipv6_error;

  /* If both fail we get an error. */

  data.loop = fixture->loop;
  data.expected_error_code = G_IO_ERROR_TIMED_OUT;
  ipv4_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv4 Broken");
  ipv6_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv6 Broken");

  mock_resolver_set_ipv4_error (fixture->mock_resolver, ipv4_error);
  mock_resolver_set_ipv6_error (fixture->mock_resolver, ipv6_error);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  g_assert_null (data.addrs);

  xerror_free (ipv4_error);
  xerror_free (ipv6_error);
}

static void
test_happy_eyeballs_both_error_delays_1 (HappyEyeballsFixture *fixture,
                                         xconstpointer         user_data)
{
  AsyncData data = { 0 };
  xerror_t *ipv4_error, *ipv6_error;

  /* The same with some different timings */

  data.loop = fixture->loop;
  data.expected_error_code = G_IO_ERROR_TIMED_OUT;
  ipv4_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv4 Broken");
  ipv6_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv6 Broken");

  mock_resolver_set_ipv4_error (fixture->mock_resolver, ipv4_error);
  mock_resolver_set_ipv4_delay_ms (fixture->mock_resolver, FAST_DELAY_LESS_THAN_TIMEOUT);
  mock_resolver_set_ipv6_error (fixture->mock_resolver, ipv6_error);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  g_assert_null (data.addrs);

  xerror_free (ipv4_error);
  xerror_free (ipv6_error);
}

static void
test_happy_eyeballs_both_error_delays_2 (HappyEyeballsFixture *fixture,
                                         xconstpointer         user_data)
{
  AsyncData data = { 0 };
  xerror_t *ipv4_error, *ipv6_error;

  /* The same with some different timings */

  data.loop = fixture->loop;
  data.expected_error_code = G_IO_ERROR_TIMED_OUT;
  ipv4_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv4 Broken");
  ipv6_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv6 Broken");

  mock_resolver_set_ipv4_error (fixture->mock_resolver, ipv4_error);
  mock_resolver_set_ipv6_error (fixture->mock_resolver, ipv6_error);
  mock_resolver_set_ipv6_delay_ms (fixture->mock_resolver, FAST_DELAY_LESS_THAN_TIMEOUT);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  g_assert_null (data.addrs);

  xerror_free (ipv4_error);
  xerror_free (ipv6_error);
}

static void
test_happy_eyeballs_both_error_delays_3 (HappyEyeballsFixture *fixture,
                                         xconstpointer         user_data)
{
  AsyncData data = { 0 };
  xerror_t *ipv4_error, *ipv6_error;

  /* The same with some different timings */

  data.loop = fixture->loop;
  data.expected_error_code = G_IO_ERROR_TIMED_OUT;
  ipv4_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv4 Broken");
  ipv6_error = xerror_new_literal (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "IPv6 Broken");

  mock_resolver_set_ipv4_error (fixture->mock_resolver, ipv4_error);
  mock_resolver_set_ipv6_error (fixture->mock_resolver, ipv6_error);
  mock_resolver_set_ipv6_delay_ms (fixture->mock_resolver, SLOW_DELAY_MORE_THAN_TIMEOUT);

  xsocket_address_enumerator_next_async (fixture->enumerator, NULL, got_addr, &data);
  xmain_loop_run (fixture->loop);

  g_assert_null (data.addrs);

  xerror_free (ipv4_error);
  xerror_free (ipv6_error);
}

int
main (int argc, char *argv[])
{
  xsize_t i;
  xchar_t *path;

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/network-address/basic", test_basic);

  for (i = 0; i < G_N_ELEMENTS (host_tests); i++)
    {
      path = xstrdup_printf ("/network-address/parse-host/%" G_GSIZE_FORMAT, i);
      g_test_add_data_func (path, &host_tests[i], test_parse_host);
      g_free (path);
    }

  for (i = 0; i < G_N_ELEMENTS (uri_tests); i++)
    {
      path = xstrdup_printf ("/network-address/parse-uri/%" G_GSIZE_FORMAT, i);
      g_test_add_data_func (path, &uri_tests[i], test_parse_uri);
      g_free (path);
    }

  for (i = 0; i < G_N_ELEMENTS (address_tests); i++)
    {
      path = xstrdup_printf ("/network-address/resolve-address/%" G_GSIZE_FORMAT, i);
      g_test_add_data_func (path, &address_tests[i], test_resolve_address);
      g_free (path);
    }

  for (i = 0; i < G_N_ELEMENTS (address_tests); i++)
    {
      path = xstrdup_printf ("/gresolver/resolve-address/%" G_GSIZE_FORMAT, i);
      g_test_add_data_func (path, &address_tests[i], test_resolve_address_gresolver);
      g_free (path);
    }

  g_test_add_func ("/network-address/scope-id", test_host_scope_id);
  g_test_add_func ("/network-address/uri-scope-id", test_uri_scope_id);
  g_test_add_func ("/network-address/loopback/basic", test_loopback_basic);
  g_test_add_func ("/network-address/loopback/sync", test_loopback_sync);
  g_test_add_func ("/network-address/loopback/async", test_loopback_async);
  g_test_add_func ("/network-address/localhost/async", test_localhost_async);
  g_test_add_func ("/network-address/localhost/sync", test_localhost_sync);
  g_test_add_func ("/network-address/to-string", test_to_string);

  g_test_add ("/network-address/happy-eyeballs/basic", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_basic, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/parallel", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_parallel, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/slow-ipv4", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_slow_ipv4, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/slow-ipv6", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_slow_ipv6, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/very-slow-ipv6", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_very_slow_ipv6, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/slow-connection-and-ipv4", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_slow_connection_and_ipv4, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/ipv6-error-ipv4-first", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_ipv6_error_ipv4_first, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/ipv6-error-ipv6-first", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_ipv6_error_ipv6_first, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/ipv6-error-ipv4-very-slow", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_ipv6_error_ipv4_very_slow, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/ipv4-error-ipv6-first", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_ipv4_error_ipv6_first, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/ipv4-error-ipv4-first", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_ipv4_error_ipv4_first, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/both-error", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_both_error, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/both-error-delays-1", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_both_error_delays_1, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/both-error-delays-2", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_both_error_delays_2, happy_eyeballs_teardown);
  g_test_add ("/network-address/happy-eyeballs/both-error-delays-3", HappyEyeballsFixture, NULL,
              happy_eyeballs_setup, test_happy_eyeballs_both_error_delays_3, happy_eyeballs_teardown);

  return g_test_run ();
}
