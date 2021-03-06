/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2011 Red Hat, Inc.
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
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "gnetworkmonitornetlink.h"
#include "gcredentials.h"
#include "ginetaddressmask.h"
#include "ginitable.h"
#include "giomodule-priv.h"
#include "glibintl.h"
#include "glib/gstdio.h"
#include "gnetworkingprivate.h"
#include "gnetworkmonitor.h"
#include "gsocket.h"
#include "gunixcredentialsmessage.h"

/* must come at the end to pick system includes from
 * gnetworkingprivate.h */
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

static xinitable_iface_t *initable_parent_iface;
static void xnetwork_monitor_netlink_iface_init (GNetworkMonitorInterface *iface);
static void xnetwork_monitor_netlink_initable_iface_init (xinitable_iface_t *iface);

struct _xnetwork_monitor_netlink_private
{
  xsocket_t *sock;
  xsource_t *source, *dump_source;
  xmain_context_t *context;

  xptr_array_t *dump_networks;
};

static xboolean_t read_netlink_messages (xnetwork_monitor_netlink_t  *nl,
                                       xerror_t                 **error);
static xboolean_t read_netlink_messages_callback (xsocket_t             *socket,
                                                xio_condition_t         condition,
                                                xpointer_t             user_data);
static xboolean_t request_dump (xnetwork_monitor_netlink_t  *nl,
                              xerror_t                 **error);

#define xnetwork_monitor_netlink_get_type _xnetwork_monitor_netlink_get_type
G_DEFINE_TYPE_WITH_CODE (xnetwork_monitor_netlink, xnetwork_monitor_netlink, XTYPE_NETWORK_MONITOR_BASE,
                         G_ADD_PRIVATE (xnetwork_monitor_netlink)
                         G_IMPLEMENT_INTERFACE (XTYPE_NETWORK_MONITOR,
                                                xnetwork_monitor_netlink_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE,
                                                xnetwork_monitor_netlink_initable_iface_init)
                         _xio_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_NETWORK_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "netlink",
                                                         20))

static void
xnetwork_monitor_netlink_init (xnetwork_monitor_netlink_t *nl)
{
  nl->priv = xnetwork_monitor_netlink_get_instance_private (nl);
}

static xboolean_t
xnetwork_monitor_netlink_initable_init (xinitable_t     *initable,
                                         xcancellable_t  *cancellable,
                                         xerror_t       **error)
{
  xnetwork_monitor_netlink_t *nl = XNETWORK_MONITOR_NETLINK (initable);
  xint_t sockfd;
  struct sockaddr_nl snl;

  /* We create the socket the old-school way because sockaddr_netlink
   * can't be represented as a xsocket_address_t
   */
  sockfd = g_socket (PF_NETLINK, SOCK_RAW, NETLINK_ROUTE, NULL);
  if (sockfd == -1)
    {
      int errsv = errno;
      g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errsv),
                   _("Could not create network monitor: %s"),
                   xstrerror (errsv));
      return FALSE;
    }

  snl.nl_family = AF_NETLINK;
  snl.nl_pid = snl.nl_pad = 0;
  snl.nl_groups = RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_ROUTE;
  if (bind (sockfd, (struct sockaddr *)&snl, sizeof (snl)) != 0)
    {
      int errsv = errno;
      g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errsv),
                   _("Could not create network monitor: %s"),
                   xstrerror (errsv));
      (void) g_close (sockfd, NULL);
      return FALSE;
    }

  nl->priv->sock = xsocket_new_from_fd (sockfd, error);
  if (!nl->priv->sock)
    {
      g_prefix_error (error, "%s", _("Could not create network monitor: "));
      (void) g_close (sockfd, NULL);
      return FALSE;
    }

  if (!xsocket_set_option (nl->priv->sock, SOL_SOCKET, SO_PASSCRED,
			    TRUE, NULL))
    {
      int errsv = errno;
      g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errsv),
                   _("Could not create network monitor: %s"),
                   xstrerror (errsv));
      return FALSE;
    }

  /* Request the current state */
  if (!request_dump (nl, error))
    return FALSE;

  /* And read responses; since we haven't yet marked the socket
   * non-blocking, each call will block until a message is received.
   */
  while (nl->priv->dump_networks)
    {
      xerror_t *local_error = NULL;
      if (!read_netlink_messages (nl, &local_error))
        {
          g_warning ("%s", local_error->message);
          g_clear_error (&local_error);
          break;
        }
    }

  xsocket_set_blocking (nl->priv->sock, FALSE);
  nl->priv->context = xmain_context_ref_thread_default ();
  nl->priv->source = xsocket_create_source (nl->priv->sock, G_IO_IN, NULL);
  xsource_set_callback (nl->priv->source,
                         (xsource_func_t) read_netlink_messages_callback, nl, NULL);
  xsource_attach (nl->priv->source, nl->priv->context);

  return initable_parent_iface->init (initable, cancellable, error);
}

static xboolean_t
request_dump (xnetwork_monitor_netlink_t  *nl,
              xerror_t                 **error)
{
  struct nlmsghdr *n;
  struct rtgenmsg *gen;
  xchar_t buf[NLMSG_SPACE (sizeof (*gen))];

  memset (buf, 0, sizeof (buf));
  n = (struct nlmsghdr*) buf;
  n->nlmsg_len = NLMSG_LENGTH (sizeof (*gen));
  n->nlmsg_type = RTM_GETROUTE;
  n->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  n->nlmsg_pid = 0;
  gen = NLMSG_DATA (n);
  gen->rtgen_family = AF_UNSPEC;

  if (xsocket_send (nl->priv->sock, buf, sizeof (buf),
                     NULL, error) < 0)
    {
      g_prefix_error (error, "%s", _("Could not get network status: "));
      return FALSE;
    }

  nl->priv->dump_networks = xptr_array_new_with_free_func (xobject_unref);
  return TRUE;
}

static xboolean_t
timeout_request_dump (xpointer_t user_data)
{
  xnetwork_monitor_netlink_t *nl = user_data;

  xsource_destroy (nl->priv->dump_source);
  xsource_unref (nl->priv->dump_source);
  nl->priv->dump_source = NULL;

  request_dump (nl, NULL);

  return FALSE;
}

static void
queue_request_dump (xnetwork_monitor_netlink_t *nl)
{
  if (nl->priv->dump_networks)
    return;

  if (nl->priv->dump_source)
    {
      xsource_destroy (nl->priv->dump_source);
      xsource_unref (nl->priv->dump_source);
    }

  nl->priv->dump_source = g_timeout_source_new_seconds (1);
  xsource_set_callback (nl->priv->dump_source,
                         (xsource_func_t) timeout_request_dump, nl, NULL);
  xsource_attach (nl->priv->dump_source, nl->priv->context);
}

static xinet_address_mask_t *
create_inet_address_mask (xsocket_family_t  family,
                          const xuint8_t  *dest,
                          xsize_t          dest_len)
{
  xinet_address_t *dest_addr;
  xinet_address_mask_t *network;

  if (dest)
    dest_addr = xinet_address_new_from_bytes (dest, family);
  else
    dest_addr = xinet_address_new_any (family);
  network = xinet_address_mask_new (dest_addr, dest_len, NULL);
  xobject_unref (dest_addr);

  return network;
}

static void
add_network (xnetwork_monitor_netlink_t *nl,
             xsocket_family_t           family,
             const xuint8_t           *dest,
             xsize_t                   dest_len)
{
  xinet_address_mask_t *network = create_inet_address_mask (family, dest, dest_len);
  g_return_if_fail (network != NULL);

  if (nl->priv->dump_networks)
    xptr_array_add (nl->priv->dump_networks, xobject_ref (network));
  else
    xnetwork_monitor_base_add_network (G_NETWORK_MONITOR_BASE (nl), network);

  xobject_unref (network);
}

static void
remove_network (xnetwork_monitor_netlink_t *nl,
                xsocket_family_t           family,
                const xuint8_t           *dest,
                xsize_t                   dest_len)
{
  xinet_address_mask_t *network = create_inet_address_mask (family, dest, dest_len);
  g_return_if_fail (network != NULL);

  if (nl->priv->dump_networks)
    {
      xinet_address_mask_t **dump_networks = (xinet_address_mask_t **)nl->priv->dump_networks->pdata;
      xuint_t i;

      for (i = 0; i < nl->priv->dump_networks->len; i++)
        {
          if (xinet_address_mask_equal (network, dump_networks[i]))
            xptr_array_remove_index_fast (nl->priv->dump_networks, i--);
        }
    }
  else
    {
      xnetwork_monitor_base_remove_network (G_NETWORK_MONITOR_BASE (nl), network);
    }

  xobject_unref (network);
}

static void
finish_dump (xnetwork_monitor_netlink_t *nl)
{
  xnetwork_monitor_base_set_networks (G_NETWORK_MONITOR_BASE (nl),
                                       (xinet_address_mask_t **)nl->priv->dump_networks->pdata,
                                       nl->priv->dump_networks->len);
  xptr_array_free (nl->priv->dump_networks, TRUE);
  nl->priv->dump_networks = NULL;
}

static xboolean_t
read_netlink_messages (xnetwork_monitor_netlink_t  *nl,
                       xerror_t                 **error)
{
  xinput_vector_t iv;
  xssize_t len;
  xint_t flags;
  xerror_t *local_error = NULL;
  xsocket_address_t *addr = NULL;
  struct nlmsghdr *msg;
  struct rtmsg *rtmsg;
  struct rtattr *attr;
  struct sockaddr_nl source_sockaddr;
  xsize_t attrlen;
  xuint8_t *dest, *gateway, *oif;
  xboolean_t retval = TRUE;

  iv.buffer = NULL;
  iv.size = 0;

  flags = MSG_PEEK | MSG_TRUNC;
  len = xsocket_receive_message (nl->priv->sock, NULL, &iv, 1,
                                  NULL, NULL, &flags, NULL, &local_error);
  if (len < 0)
    {
      retval = FALSE;
      goto done;
    }

  iv.buffer = g_malloc (len);
  iv.size = len;
  len = xsocket_receive_message (nl->priv->sock, &addr, &iv, 1,
                                  NULL, NULL, NULL, NULL, &local_error);
  if (len < 0)
    {
      retval = FALSE;
      goto done;
    }

  if (!xsocket_address_to_native (addr, &source_sockaddr, sizeof (source_sockaddr), &local_error))
    {
      retval = FALSE;
      goto done;
    }

  /* If the sender port id is 0 (not fakeable) then the message is from the kernel */
  if (source_sockaddr.nl_pid != 0)
    goto done;

  msg = (struct nlmsghdr *) iv.buffer;
  for (; len > 0; msg = NLMSG_NEXT (msg, len))
    {
      if (!NLMSG_OK (msg, (size_t) len))
        {
          g_set_error_literal (&local_error,
                               G_IO_ERROR,
                               G_IO_ERROR_PARTIAL_INPUT,
                               "netlink message was truncated; shouldn't happen...");
          retval = FALSE;
          goto done;
        }

      switch (msg->nlmsg_type)
        {
        case RTM_NEWROUTE:
        case RTM_DELROUTE:
          rtmsg = NLMSG_DATA (msg);

          if (rtmsg->rtm_family != AF_INET && rtmsg->rtm_family != AF_INET6)
            continue;
          if (rtmsg->rtm_type == RTN_UNREACHABLE)
            continue;

          attrlen = NLMSG_PAYLOAD (msg, sizeof (struct rtmsg));
          attr = RTM_RTA (rtmsg);
          dest = gateway = oif = NULL;
          while (RTA_OK (attr, attrlen))
            {
              if (attr->rta_type == RTA_DST)
                dest = RTA_DATA (attr);
              else if (attr->rta_type == RTA_GATEWAY)
                gateway = RTA_DATA (attr);
              else if (attr->rta_type == RTA_OIF)
                oif = RTA_DATA (attr);
              attr = RTA_NEXT (attr, attrlen);
            }

          if (dest || gateway || oif)
            {
              /* Unless we're processing the results of a dump, ignore
               * IPv6 link-local multicast routes, which are added and
               * removed all the time for some reason.
               */
#define UNALIGNED_IN6_IS_ADDR_MC_LINKLOCAL(a)           \
              ((a[0] == 0xff) && ((a[1] & 0xf) == 0x2))

              if (!nl->priv->dump_networks &&
                  rtmsg->rtm_family == AF_INET6 &&
                  rtmsg->rtm_dst_len != 0 &&
                  (dest && UNALIGNED_IN6_IS_ADDR_MC_LINKLOCAL (dest)))
                continue;

              if (msg->nlmsg_type == RTM_NEWROUTE)
                add_network (nl, rtmsg->rtm_family, dest, rtmsg->rtm_dst_len);
              else
                remove_network (nl, rtmsg->rtm_family, dest, rtmsg->rtm_dst_len);
              queue_request_dump (nl);
            }
          break;

        case NLMSG_DONE:
          finish_dump (nl);
          goto done;

        case NLMSG_ERROR:
          {
            struct nlmsgerr *e = NLMSG_DATA (msg);

            g_set_error (&local_error,
                         G_IO_ERROR,
                         g_io_error_from_errno (-e->error),
                         "netlink error: %s",
                         xstrerror (-e->error));
          }
          retval = FALSE;
          goto done;

        default:
          g_set_error (&local_error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_DATA,
                       "unexpected netlink message %d",
                       msg->nlmsg_type);
          retval = FALSE;
          goto done;
        }
    }

 done:
  g_free (iv.buffer);
  g_clear_object (&addr);

  if (!retval && nl->priv->dump_networks)
    finish_dump (nl);

  if (local_error)
    g_propagate_prefixed_error (error, local_error, "Error on netlink socket: ");

  return retval;
}

static void
xnetwork_monitor_netlink_finalize (xobject_t *object)
{
  xnetwork_monitor_netlink_t *nl = XNETWORK_MONITOR_NETLINK (object);

  if (nl->priv->source)
    {
      xsource_destroy (nl->priv->source);
      xsource_unref (nl->priv->source);
    }

  if (nl->priv->dump_source)
    {
      xsource_destroy (nl->priv->dump_source);
      xsource_unref (nl->priv->dump_source);
    }

  if (nl->priv->sock)
    {
      xsocket_close (nl->priv->sock, NULL);
      xobject_unref (nl->priv->sock);
    }

  g_clear_pointer (&nl->priv->context, xmain_context_unref);
  g_clear_pointer (&nl->priv->dump_networks, xptr_array_unref);

  XOBJECT_CLASS (xnetwork_monitor_netlink_parent_class)->finalize (object);
}

static xboolean_t
read_netlink_messages_callback (xsocket_t      *socket,
                                xio_condition_t  condition,
                                xpointer_t      user_data)
{
  xerror_t *error = NULL;
  xnetwork_monitor_netlink_t *nl = XNETWORK_MONITOR_NETLINK (user_data);

  if (!read_netlink_messages (nl, &error))
    {
      g_warning ("Error reading netlink message: %s", error->message);
      g_clear_error (&error);
      return FALSE;
    }

  return TRUE;
}

static void
xnetwork_monitor_netlink_class_init (xnetwork_monitor_netlink_class_t *nl_class)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (nl_class);

  xobject_class->finalize = xnetwork_monitor_netlink_finalize;
}

static void
xnetwork_monitor_netlink_iface_init (GNetworkMonitorInterface *monitor_iface)
{
}

static void
xnetwork_monitor_netlink_initable_iface_init (xinitable_iface_t *iface)
{
  initable_parent_iface = xtype_interface_peek_parent (iface);

  iface->init = xnetwork_monitor_netlink_initable_init;
}
