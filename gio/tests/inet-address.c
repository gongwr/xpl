/* Unit tests for xinet_address_t
 * Copyright (C) 2012 Red Hat, Inc
 * Author: Matthias Clasen
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include "config.h"

#include <gio/gio.h>
#include <gio/gnetworking.h>

static void
test_parse (void)
{
  xinet_address_t *addr;

  addr = xinet_address_new_from_string ("0:0:0:0:0:0:0:0");
  xassert (addr != NULL);
  xobject_unref (addr);
  addr = xinet_address_new_from_string ("1:0:0:0:0:0:0:8");
  xassert (addr != NULL);
  xobject_unref (addr);
  addr = xinet_address_new_from_string ("0:0:0:0:0:FFFF:204.152.189.116");
  xassert (addr != NULL);
  xobject_unref (addr);
  addr = xinet_address_new_from_string ("::1");
  xassert (addr != NULL);
  xobject_unref (addr);
  addr = xinet_address_new_from_string ("::");
  xassert (addr != NULL);
  xobject_unref (addr);
  addr = xinet_address_new_from_string ("::FFFF:204.152.189.116");
  xassert (addr != NULL);
  xobject_unref (addr);
  addr = xinet_address_new_from_string ("204.152.189.116");
  xassert (addr != NULL);
  xobject_unref (addr);

  addr = xinet_address_new_from_string ("::1::2");
  xassert (addr == NULL);
  addr = xinet_address_new_from_string ("2001:1:2:3:4:5:6:7]");
  xassert (addr == NULL);
  addr = xinet_address_new_from_string ("[2001:1:2:3:4:5:6:7");
  xassert (addr == NULL);
  addr = xinet_address_new_from_string ("[2001:1:2:3:4:5:6:7]");
  xassert (addr == NULL);
  addr = xinet_address_new_from_string ("[2001:1:2:3:4:5:6:7]:80");
  xassert (addr == NULL);
  addr = xinet_address_new_from_string ("0:1:2:3:4:5:6:7:8:9");
  xassert (addr == NULL);
  addr = xinet_address_new_from_string ("::FFFFFFF");
  xassert (addr == NULL);
  addr = xinet_address_new_from_string ("204.152.189.116:80");
  xassert (addr == NULL);
}

static void
test_any (void)
{
  xinet_address_t *addr;
  xsocket_family_t family[2] = { XSOCKET_FAMILY_IPV4, XSOCKET_FAMILY_IPV6 };
  xsize_t size[2] = { 4, 16 };
  xint_t i;

  for (i = 0; i < 2; i++)
    {
      addr = xinet_address_new_any (family[i]);

      xassert (xinet_address_get_is_any (addr));
      xassert (xinet_address_get_family (addr) == family[i]);
      xassert (xinet_address_get_native_size (addr) == size[i]);
      xassert (!xinet_address_get_is_loopback (addr));
      xassert (!xinet_address_get_is_link_local (addr));
      xassert (!xinet_address_get_is_site_local (addr));
      xassert (!xinet_address_get_is_multicast (addr));
      xassert (!xinet_address_get_is_mc_global (addr));
      xassert (!xinet_address_get_is_mc_link_local (addr));
      xassert (!xinet_address_get_is_mc_node_local (addr));
      xassert (!xinet_address_get_is_mc_org_local (addr));
      xassert (!xinet_address_get_is_mc_site_local (addr));

      xobject_unref (addr);
   }
}

static void
test_loopback (void)
{
  xinet_address_t *addr;

  addr = xinet_address_new_from_string ("::1");
  xassert (xinet_address_get_family (addr) == XSOCKET_FAMILY_IPV6);
  xassert (xinet_address_get_is_loopback (addr));
  xobject_unref (addr);

  addr = xinet_address_new_from_string ("127.0.0.0");
  xassert (xinet_address_get_family (addr) == XSOCKET_FAMILY_IPV4);
  xassert (xinet_address_get_is_loopback (addr));
  xobject_unref (addr);
}

static void
test_bytes (void)
{
  xinet_address_t *addr1, *addr2, *addr3;
  const xuint8_t *bytes;

  addr1 = xinet_address_new_from_string ("192.168.0.100");
  addr2 = xinet_address_new_from_string ("192.168.0.101");
  bytes = xinet_address_to_bytes (addr1);
  addr3 = xinet_address_new_from_bytes (bytes, XSOCKET_FAMILY_IPV4);

  xassert (!xinet_address_equal (addr1, addr2));
  xassert (xinet_address_equal (addr1, addr3));

  xobject_unref (addr1);
  xobject_unref (addr2);
  xobject_unref (addr3);
}

static void
test_property (void)
{
  xinet_address_t *addr;
  xsocket_family_t family;
  const xuint8_t *bytes;
  xboolean_t any;
  xboolean_t loopback;
  xboolean_t link_local;
  xboolean_t site_local;
  xboolean_t multicast;
  xboolean_t mc_global;
  xboolean_t mc_link_local;
  xboolean_t mc_node_local;
  xboolean_t mc_org_local;
  xboolean_t mc_site_local;

  addr = xinet_address_new_from_string ("ff85::");
  xobject_get (addr,
                "family", &family,
                "bytes", &bytes,
                "is-any", &any,
                "is-loopback", &loopback,
                "is-link-local", &link_local,
                "is-site-local", &site_local,
                "is-multicast", &multicast,
                "is-mc-global", &mc_global,
                "is-mc-link-local", &mc_link_local,
                "is-mc-node-local", &mc_node_local,
                "is-mc-org-local", &mc_org_local,
                "is-mc-site-local", &mc_site_local,
                NULL);

  xassert (family == XSOCKET_FAMILY_IPV6);
  xassert (!any);
  xassert (!loopback);
  xassert (!link_local);
  xassert (!site_local);
  xassert (multicast);
  xassert (!mc_global);
  xassert (!mc_link_local);
  xassert (!mc_node_local);
  xassert (!mc_org_local);
  xassert (mc_site_local);

  xobject_unref (addr);
}

static void
test_socket_address (void)
{
  xinet_address_t *addr;
  xinet_socket_address_t *saddr;
  xuint_t port;
  xuint32_t flowinfo;
  xuint32_t scope_id;
  xsocket_family_t family;

  addr = xinet_address_new_from_string ("::ffff:125.1.15.5");
  saddr = G_INET_SOCKET_ADDRESS (g_inet_socket_address_new (addr, 308));

  xassert (xinet_address_equal (addr, g_inet_socket_address_get_address (saddr)));
  xobject_unref (addr);

  xassert (g_inet_socket_address_get_port (saddr) == 308);
  xassert (g_inet_socket_address_get_flowinfo (saddr) == 0);
  xassert (g_inet_socket_address_get_scope_id (saddr) == 0);

  xobject_unref (saddr);

  addr = xinet_address_new_from_string ("::1");
  saddr = G_INET_SOCKET_ADDRESS (xobject_new (XTYPE_INET_SOCKET_ADDRESS,
                                               "address", addr,
                                               "port", 308,
                                               "flowinfo", 10,
                                               "scope-id", 25,
                                               NULL));
  xobject_unref (addr);

  xassert (g_inet_socket_address_get_port (saddr) == 308);
  xassert (g_inet_socket_address_get_flowinfo (saddr) == 10);
  xassert (g_inet_socket_address_get_scope_id (saddr) == 25);

  xobject_get (saddr,
                "family", &family,
                "address", &addr,
                "port", &port,
                "flowinfo", &flowinfo,
                "scope-id", &scope_id,
                NULL);

  xassert (family == XSOCKET_FAMILY_IPV6);
  xassert (addr != NULL);
  xassert (port == 308);
  xassert (flowinfo == 10);
  xassert (scope_id == 25);

  xobject_unref (addr);
  xobject_unref (saddr);
}

static void
test_socket_address_to_string (void)
{
  xsocket_address_t *sa = NULL;
  xinet_address_t *ia = NULL;
  xchar_t *str = NULL;

  /* IPv4. */
  ia = xinet_address_new_from_string ("123.1.123.1");
  sa = g_inet_socket_address_new (ia, 80);
  str = xsocket_connectable_to_string (XSOCKET_CONNECTABLE (sa));
  g_assert_cmpstr (str, ==, "123.1.123.1:80");
  g_free (str);
  xobject_unref (sa);
  xobject_unref (ia);

  /* IPv6. */
  ia = xinet_address_new_from_string ("fe80::80");
  sa = g_inet_socket_address_new (ia, 80);
  str = xsocket_connectable_to_string (XSOCKET_CONNECTABLE (sa));
  g_assert_cmpstr (str, ==, "[fe80::80]:80");
  g_free (str);
  xobject_unref (sa);
  xobject_unref (ia);

  /* IPv6 without port. */
  ia = xinet_address_new_from_string ("fe80::80");
  sa = g_inet_socket_address_new (ia, 0);
  str = xsocket_connectable_to_string (XSOCKET_CONNECTABLE (sa));
  g_assert_cmpstr (str, ==, "fe80::80");
  g_free (str);
  xobject_unref (sa);
  xobject_unref (ia);

  /* IPv6 with scope. */
  ia = xinet_address_new_from_string ("::1");
  sa = XSOCKET_ADDRESS (xobject_new (XTYPE_INET_SOCKET_ADDRESS,
                                       "address", ia,
                                       "port", 123,
                                       "flowinfo", 10,
                                       "scope-id", 25,
                                       NULL));
  str = xsocket_connectable_to_string (XSOCKET_CONNECTABLE (sa));
  g_assert_cmpstr (str, ==, "[::1%25]:123");
  g_free (str);
  xobject_unref (sa);
  xobject_unref (ia);
}

static void
test_mask_parse (void)
{
  xinet_address_mask_t *mask;
  xerror_t *error = NULL;

  mask = xinet_address_mask_new_from_string ("10.0.0.0/8", &error);
  g_assert_no_error (error);
  xassert (mask != NULL);
  xobject_unref (mask);

  mask = xinet_address_mask_new_from_string ("fe80::/10", &error);
  g_assert_no_error (error);
  xassert (mask != NULL);
  xobject_unref (mask);

  mask = xinet_address_mask_new_from_string ("::", &error);
  g_assert_no_error (error);
  xassert (mask != NULL);
  xobject_unref (mask);

  mask = xinet_address_mask_new_from_string ("::/abc", &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
  xassert (mask == NULL);
  g_clear_error (&error);

  mask = xinet_address_mask_new_from_string ("127.0.0.1/128", &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
  xassert (mask == NULL);
  g_clear_error (&error);
}

static void
test_mask_property (void)
{
  xinet_address_mask_t *mask;
  xinet_address_t *addr;
  xsocket_family_t family;
  xuint_t len;

  addr = xinet_address_new_from_string ("fe80::");
  mask = xinet_address_mask_new_from_string ("fe80::/10", NULL);
  xassert (xinet_address_mask_get_family (mask) == XSOCKET_FAMILY_IPV6);
  xassert (xinet_address_equal (addr, xinet_address_mask_get_address (mask)));
  xassert (xinet_address_mask_get_length (mask) == 10);
  xobject_unref (addr);

  xobject_get (mask,
                "family", &family,
                "address", &addr,
                "length", &len,
                NULL);
  xassert (family == XSOCKET_FAMILY_IPV6);
  xassert (addr != NULL);
  xassert (len == 10);
  xobject_unref (addr);

  xobject_unref (mask);
}

static void
test_mask_equal (void)
{
  xinet_address_mask_t *mask;
  xinet_address_mask_t *mask2;
  xchar_t *str;

  mask = xinet_address_mask_new_from_string ("fe80:0:0::/10", NULL);
  str = xinet_address_mask_to_string (mask);
  g_assert_cmpstr (str, ==, "fe80::/10");
  mask2 = xinet_address_mask_new_from_string (str, NULL);
  xassert (xinet_address_mask_equal (mask, mask2));
  xobject_unref (mask2);
  g_free (str);

  mask2 = xinet_address_mask_new_from_string ("fe80::/12", NULL);
  xassert (!xinet_address_mask_equal (mask, mask2));
  xobject_unref (mask2);

  mask2 = xinet_address_mask_new_from_string ("ff80::/10", NULL);
  xassert (!xinet_address_mask_equal (mask, mask2));
  xobject_unref (mask2);

  xobject_unref (mask);
}

static void
test_mask_match (void)
{
  xinet_address_mask_t *mask;
  xinet_address_t *addr;

  mask = xinet_address_mask_new_from_string ("1.2.0.0/16", NULL);

  addr = xinet_address_new_from_string ("1.2.0.0");
  xassert (xinet_address_mask_matches (mask, addr));
  xobject_unref (addr);
  addr = xinet_address_new_from_string ("1.2.3.4");
  xassert (xinet_address_mask_matches (mask, addr));
  xobject_unref (addr);
  addr = xinet_address_new_from_string ("1.3.1.1");
  xassert (!xinet_address_mask_matches (mask, addr));
  xobject_unref (addr);

  xobject_unref (mask);

  mask = xinet_address_mask_new_from_string ("1.2.0.0/24", NULL);

  addr = xinet_address_new_from_string ("1.2.0.0");
  xassert (xinet_address_mask_matches (mask, addr));
  xobject_unref (addr);
  addr = xinet_address_new_from_string ("1.2.3.4");
  xassert (!xinet_address_mask_matches (mask, addr));
  xobject_unref (addr);
  addr = xinet_address_new_from_string ("1.2.0.24");
  xassert (xinet_address_mask_matches (mask, addr));
  xobject_unref (addr);

  xobject_unref (mask);

}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/inet-address/parse", test_parse);
  g_test_add_func ("/inet-address/any", test_any);
  g_test_add_func ("/inet-address/loopback", test_loopback);
  g_test_add_func ("/inet-address/bytes", test_bytes);
  g_test_add_func ("/inet-address/property", test_property);
  g_test_add_func ("/socket-address/basic", test_socket_address);
  g_test_add_func ("/socket-address/to-string", test_socket_address_to_string);
  g_test_add_func ("/address-mask/parse", test_mask_parse);
  g_test_add_func ("/address-mask/property", test_mask_property);
  g_test_add_func ("/address-mask/equal", test_mask_equal);
  g_test_add_func ("/address-mask/match", test_mask_match);

  return g_test_run ();
}
