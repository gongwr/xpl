/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
 * Copyright (C) 2018 Igalia S.L.
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
#include <glib.h>
#include "glibintl.h"

#include <stdio.h>
#include <string.h>

#include "gthreadedresolver.h"
#include "gnetworkingprivate.h"

#include "gcancellable.h"
#include "ginetaddress.h"
#include "ginetsocketaddress.h"
#include "gtask.h"
#include "gsocketaddress.h"
#include "gsrvtarget.h"


XDEFINE_TYPE (GThreadedResolver, xthreaded_resolver, XTYPE_RESOLVER)

static void
xthreaded_resolver_init (GThreadedResolver *gtr)
{
}

static GResolverError
g_resolver_error_from_addrinfo_error (xint_t err)
{
  switch (err)
    {
    case EAI_FAIL:
#if defined(EAI_NODATA) && (EAI_NODATA != EAI_NONAME)
    case EAI_NODATA:
#endif
    case EAI_NONAME:
      return G_RESOLVER_ERROR_NOT_FOUND;

    case EAI_AGAIN:
      return G_RESOLVER_ERROR_TEMPORARY_FAILURE;

    default:
      return G_RESOLVER_ERROR_INTERNAL;
    }
}

typedef struct {
  char *hostname;
  int address_family;
} LookupData;

static LookupData *
lookup_data_new (const char *hostname,
                 int         address_family)
{
  LookupData *data = g_new (LookupData, 1);
  data->hostname = xstrdup (hostname);
  data->address_family = address_family;
  return data;
}

static void
lookup_data_free (LookupData *data)
{
  g_free (data->hostname);
  g_free (data);
}

static void
do_lookup_by_name (xtask_t         *task,
                   xpointer_t       source_object,
                   xpointer_t       task_data,
                   xcancellable_t  *cancellable)
{
  LookupData *lookup_data = task_data;
  const char *hostname = lookup_data->hostname;
  struct addrinfo *res = NULL;
  xlist_t *addresses;
  xint_t retval;
  struct addrinfo addrinfo_hints = { 0 };

#ifdef AI_ADDRCONFIG
  addrinfo_hints.ai_flags = AI_ADDRCONFIG;
#endif
  /* socktype and protocol don't actually matter, they just get copied into the
  * returned addrinfo structures (and then we ignore them). But if
  * we leave them unset, we'll get back duplicate answers.
  */
  addrinfo_hints.ai_socktype = SOCK_STREAM;
  addrinfo_hints.ai_protocol = IPPROTO_TCP;

  addrinfo_hints.ai_family = lookup_data->address_family;
  retval = getaddrinfo (hostname, NULL, &addrinfo_hints, &res);

  if (retval == 0)
    {
      struct addrinfo *ai;
      xsocket_address_t *sockaddr;
      xinet_address_t *addr;

      addresses = NULL;
      for (ai = res; ai; ai = ai->ai_next)
        {
          sockaddr = xsocket_address_new_from_native (ai->ai_addr, ai->ai_addrlen);
          if (!sockaddr)
            continue;
          if (!X_IS_INET_SOCKET_ADDRESS (sockaddr))
            {
              g_clear_object (&sockaddr);
              continue;
            }

          addr = xobject_ref (g_inet_socket_address_get_address ((xinet_socket_address_t *)sockaddr));
          addresses = xlist_prepend (addresses, addr);
          xobject_unref (sockaddr);
        }

      if (addresses != NULL)
        {
          addresses = xlist_reverse (addresses);
          xtask_return_pointer (task, addresses,
                                 (xdestroy_notify_t)g_resolver_free_addresses);
        }
      else
        {
          /* All addresses failed to be converted to GSocketAddresses. */
          xtask_return_new_error (task,
                                   G_RESOLVER_ERROR,
                                   G_RESOLVER_ERROR_NOT_FOUND,
                                   _("Error resolving “%s”: %s"),
                                   hostname,
                                   _("No valid addresses were found"));
        }
    }
  else
    {
#ifdef G_OS_WIN32
      xchar_t *error_message = g_win32_error_message (WSAGetLastError ());
#else
      xchar_t *error_message = g_locale_to_utf8 (gai_strerror (retval), -1, NULL, NULL, NULL);
      if (error_message == NULL)
        error_message = xstrdup ("[Invalid UTF-8]");
#endif

      xtask_return_new_error (task,
                               G_RESOLVER_ERROR,
                               g_resolver_error_from_addrinfo_error (retval),
                               _("Error resolving “%s”: %s"),
                               hostname, error_message);
      g_free (error_message);
    }

  if (res)
    freeaddrinfo (res);
}

static xlist_t *
lookup_by_name (xresolver_t     *resolver,
                const xchar_t   *hostname,
                xcancellable_t  *cancellable,
                xerror_t       **error)
{
  xtask_t *task;
  xlist_t *addresses;
  LookupData *data;

  data = lookup_data_new (hostname, AF_UNSPEC);
  task = xtask_new (resolver, cancellable, NULL, NULL);
  xtask_set_source_tag (task, lookup_by_name);
  xtask_set_name (task, "[gio] resolver lookup");
  xtask_set_task_data (task, data, (xdestroy_notify_t)lookup_data_free);
  xtask_set_return_on_cancel (task, TRUE);
  xtask_run_in_thread_sync (task, do_lookup_by_name);
  addresses = xtask_propagate_pointer (task, error);
  xobject_unref (task);

  return addresses;
}

static int
flags_to_family (GResolverNameLookupFlags flags)
{
  int address_family = AF_UNSPEC;

  if (flags & G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY)
    address_family = AF_INET;

  if (flags & G_RESOLVER_NAME_LOOKUP_FLAGS_IPV6_ONLY)
    {
      address_family = AF_INET6;
      /* You can only filter by one family at a time */
      xreturn_val_if_fail (!(flags & G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY), address_family);
    }

  return address_family;
}

static xlist_t *
lookup_by_name_with_flags (xresolver_t                 *resolver,
                           const xchar_t               *hostname,
                           GResolverNameLookupFlags   flags,
                           xcancellable_t              *cancellable,
                           xerror_t                   **error)
{
  xtask_t *task;
  xlist_t *addresses;
  LookupData *data;

  data = lookup_data_new (hostname, flags_to_family (flags));
  task = xtask_new (resolver, cancellable, NULL, NULL);
  xtask_set_source_tag (task, lookup_by_name_with_flags);
  xtask_set_name (task, "[gio] resolver lookup");
  xtask_set_task_data (task, data, (xdestroy_notify_t)lookup_data_free);
  xtask_set_return_on_cancel (task, TRUE);
  xtask_run_in_thread_sync (task, do_lookup_by_name);
  addresses = xtask_propagate_pointer (task, error);
  xobject_unref (task);

  return addresses;
}

static void
lookup_by_name_with_flags_async (xresolver_t                *resolver,
                                 const xchar_t              *hostname,
                                 GResolverNameLookupFlags  flags,
                                 xcancellable_t             *cancellable,
                                 xasync_ready_callback_t       callback,
                                 xpointer_t                  user_data)
{
  xtask_t *task;
  LookupData *data;

  data = lookup_data_new (hostname, flags_to_family (flags));
  task = xtask_new (resolver, cancellable, callback, user_data);
  xtask_set_source_tag (task, lookup_by_name_with_flags_async);
  xtask_set_name (task, "[gio] resolver lookup");
  xtask_set_task_data (task, data, (xdestroy_notify_t)lookup_data_free);
  xtask_set_return_on_cancel (task, TRUE);
  xtask_run_in_thread (task, do_lookup_by_name);
  xobject_unref (task);
}

static void
lookup_by_name_async (xresolver_t           *resolver,
                      const xchar_t         *hostname,
                      xcancellable_t        *cancellable,
                      xasync_ready_callback_t  callback,
                      xpointer_t             user_data)
{
  lookup_by_name_with_flags_async (resolver,
                                   hostname,
                                   G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT,
                                   cancellable,
                                   callback,
                                   user_data);
}

static xlist_t *
lookup_by_name_finish (xresolver_t     *resolver,
                       xasync_result_t  *result,
                       xerror_t       **error)
{
  xreturn_val_if_fail (xtask_is_valid (result, resolver), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}

static xlist_t *
lookup_by_name_with_flags_finish (xresolver_t     *resolver,
                                  xasync_result_t  *result,
                                  xerror_t       **error)
{
  xreturn_val_if_fail (xtask_is_valid (result, resolver), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}

static void
do_lookup_by_address (xtask_t         *task,
                      xpointer_t       source_object,
                      xpointer_t       task_data,
                      xcancellable_t  *cancellable)
{
  xinet_address_t *address = task_data;
  struct sockaddr_storage sockaddr;
  xsize_t sockaddr_size;
  xsocket_address_t *gsockaddr;
  xchar_t name[NI_MAXHOST];
  xint_t retval;

  gsockaddr = g_inet_socket_address_new (address, 0);
  xsocket_address_to_native (gsockaddr, (struct sockaddr *)&sockaddr,
                              sizeof (sockaddr), NULL);
  sockaddr_size = xsocket_address_get_native_size (gsockaddr);
  xobject_unref (gsockaddr);

  retval = getnameinfo ((struct sockaddr *)&sockaddr, sockaddr_size,
                        name, sizeof (name), NULL, 0, NI_NAMEREQD);
  if (retval == 0)
    xtask_return_pointer (task, xstrdup (name), g_free);
  else
    {
      xchar_t *phys;

#ifdef G_OS_WIN32
      xchar_t *error_message = g_win32_error_message (WSAGetLastError ());
#else
      xchar_t *error_message = g_locale_to_utf8 (gai_strerror (retval), -1, NULL, NULL, NULL);
      if (error_message == NULL)
        error_message = xstrdup ("[Invalid UTF-8]");
#endif

      phys = xinet_address_to_string (address);
      xtask_return_new_error (task,
                               G_RESOLVER_ERROR,
                               g_resolver_error_from_addrinfo_error (retval),
                               _("Error reverse-resolving “%s”: %s"),
                               phys ? phys : "(unknown)",
                               error_message);
      g_free (phys);
      g_free (error_message);
    }
}

static xchar_t *
lookup_by_address (xresolver_t        *resolver,
                   xinet_address_t     *address,
                   xcancellable_t     *cancellable,
                   xerror_t          **error)
{
  xtask_t *task;
  xchar_t *name;

  task = xtask_new (resolver, cancellable, NULL, NULL);
  xtask_set_source_tag (task, lookup_by_address);
  xtask_set_name (task, "[gio] resolver lookup");
  xtask_set_task_data (task, xobject_ref (address), xobject_unref);
  xtask_set_return_on_cancel (task, TRUE);
  xtask_run_in_thread_sync (task, do_lookup_by_address);
  name = xtask_propagate_pointer (task, error);
  xobject_unref (task);

  return name;
}

static void
lookup_by_address_async (xresolver_t           *resolver,
                         xinet_address_t        *address,
                         xcancellable_t        *cancellable,
                         xasync_ready_callback_t  callback,
                         xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (resolver, cancellable, callback, user_data);
  xtask_set_source_tag (task, lookup_by_address_async);
  xtask_set_name (task, "[gio] resolver lookup");
  xtask_set_task_data (task, xobject_ref (address), xobject_unref);
  xtask_set_return_on_cancel (task, TRUE);
  xtask_run_in_thread (task, do_lookup_by_address);
  xobject_unref (task);
}

static xchar_t *
lookup_by_address_finish (xresolver_t     *resolver,
                          xasync_result_t  *result,
                          xerror_t       **error)
{
  xreturn_val_if_fail (xtask_is_valid (result, resolver), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}


#if defined(G_OS_UNIX)

#if defined __BIONIC__ && !defined BIND_4_COMPAT
/* Copy from bionic/libc/private/arpa_nameser_compat.h
 * and bionic/libc/private/arpa_nameser.h */
typedef struct {
	unsigned	id :16;		/* query identification number */
#if BYTE_ORDER == BIG_ENDIAN
			/* fields in third byte */
	unsigned	qr: 1;		/* response flag */
	unsigned	opcode: 4;	/* purpose of message */
	unsigned	aa: 1;		/* authoritative answer */
	unsigned	tc: 1;		/* truncated message */
	unsigned	rd: 1;		/* recursion desired */
			/* fields in fourth byte */
	unsigned	ra: 1;		/* recursion available */
	unsigned	unused :1;	/* unused bits (MBZ as of 4.9.3a3) */
	unsigned	ad: 1;		/* authentic data from named */
	unsigned	cd: 1;		/* checking disabled by resolver */
	unsigned	rcode :4;	/* response code */
#endif
#if BYTE_ORDER == LITTLE_ENDIAN || BYTE_ORDER == PDP_ENDIAN
			/* fields in third byte */
	unsigned	rd :1;		/* recursion desired */
	unsigned	tc :1;		/* truncated message */
	unsigned	aa :1;		/* authoritative answer */
	unsigned	opcode :4;	/* purpose of message */
	unsigned	qr :1;		/* response flag */
			/* fields in fourth byte */
	unsigned	rcode :4;	/* response code */
	unsigned	cd: 1;		/* checking disabled by resolver */
	unsigned	ad: 1;		/* authentic data from named */
	unsigned	unused :1;	/* unused bits (MBZ as of 4.9.3a3) */
	unsigned	ra :1;		/* recursion available */
#endif
			/* remaining bytes */
	unsigned	qdcount :16;	/* number of question entries */
	unsigned	ancount :16;	/* number of answer entries */
	unsigned	nscount :16;	/* number of authority entries */
	unsigned	arcount :16;	/* number of resource entries */
} HEADER;

#define NS_INT32SZ	4	/* #/bytes of data in a uint32_t */
#define NS_INT16SZ	2	/* #/bytes of data in a uint16_t */

#define NS_GET16(s, cp) do { \
	const u_char *t_cp = (const u_char *)(cp); \
	(s) = ((uint16_t)t_cp[0] << 8) \
	    | ((uint16_t)t_cp[1]) \
	    ; \
	(cp) += NS_INT16SZ; \
} while (/*CONSTCOND*/0)

#define NS_GET32(l, cp) do { \
	const u_char *t_cp = (const u_char *)(cp); \
	(l) = ((uint32_t)t_cp[0] << 24) \
	    | ((uint32_t)t_cp[1] << 16) \
	    | ((uint32_t)t_cp[2] << 8) \
	    | ((uint32_t)t_cp[3]) \
	    ; \
	(cp) += NS_INT32SZ; \
} while (/*CONSTCOND*/0)

#define	GETSHORT		NS_GET16
#define	GETLONG			NS_GET32

#define C_IN 1

/* From bionic/libc/private/resolv_private.h */
int dn_expand(const u_char *, const u_char *, const u_char *, char *, int);
#define dn_skipname __dn_skipname
int dn_skipname(const u_char *, const u_char *);

/* From bionic/libc/private/arpa_nameser_compat.h */
#define T_MX		ns_t_mx
#define T_TXT		ns_t_txt
#define T_SOA		ns_t_soa
#define T_NS		ns_t_ns

/* From bionic/libc/private/arpa_nameser.h */
typedef enum __ns_type {
	ns_t_invalid = 0,	/* Cookie. */
	ns_t_a = 1,		/* Host address. */
	ns_t_ns = 2,		/* Authoritative server. */
	ns_t_md = 3,		/* Mail destination. */
	ns_t_mf = 4,		/* Mail forwarder. */
	ns_t_cname = 5,		/* Canonical name. */
	ns_t_soa = 6,		/* Start of authority zone. */
	ns_t_mb = 7,		/* Mailbox domain name. */
	ns_t_mg = 8,		/* Mail group member. */
	ns_t_mr = 9,		/* Mail rename name. */
	ns_t_null = 10,		/* Null resource record. */
	ns_t_wks = 11,		/* Well known service. */
	ns_t_ptr = 12,		/* Domain name pointer. */
	ns_t_hinfo = 13,	/* Host information. */
	ns_t_minfo = 14,	/* Mailbox information. */
	ns_t_mx = 15,		/* Mail routing information. */
	ns_t_txt = 16,		/* Text strings. */
	ns_t_rp = 17,		/* Responsible person. */
	ns_t_afsdb = 18,	/* AFS cell database. */
	ns_t_x25 = 19,		/* X_25 calling address. */
	ns_t_isdn = 20,		/* ISDN calling address. */
	ns_t_rt = 21,		/* Router. */
	ns_t_nsap = 22,		/* NSAP address. */
	ns_t_nsap_ptr = 23,	/* Reverse NSAP lookup (deprecated). */
	ns_t_sig = 24,		/* Security signature. */
	ns_t_key = 25,		/* Security key. */
	ns_t_px = 26,		/* X.400 mail mapping. */
	ns_t_gpos = 27,		/* Geographical position (withdrawn). */
	ns_t_aaaa = 28,		/* Ip6 Address. */
	ns_t_loc = 29,		/* Location Information. */
	ns_t_nxt = 30,		/* Next domain (security). */
	ns_t_eid = 31,		/* Endpoint identifier. */
	ns_t_nimloc = 32,	/* Nimrod Locator. */
	ns_t_srv = 33,		/* Server Selection. */
	ns_t_atma = 34,		/* ATM Address */
	ns_t_naptr = 35,	/* Naming Authority PoinTeR */
	ns_t_kx = 36,		/* Key Exchange */
	ns_t_cert = 37,		/* Certification record */
	ns_t_a6 = 38,		/* IPv6 address (deprecates AAAA) */
	ns_t_dname = 39,	/* Non-terminal DNAME (for IPv6) */
	ns_t_sink = 40,		/* Kitchen sink (experimental) */
	ns_t_opt = 41,		/* EDNS0 option (meta-RR) */
	ns_t_apl = 42,		/* Address prefix list (RFC 3123) */
	ns_t_tkey = 249,	/* Transaction key */
	ns_t_tsig = 250,	/* Transaction signature. */
	ns_t_ixfr = 251,	/* Incremental zone transfer. */
	ns_t_axfr = 252,	/* Transfer zone of authority. */
	ns_t_mailb = 253,	/* Transfer mailbox records. */
	ns_t_maila = 254,	/* Transfer mail agent records. */
	ns_t_any = 255,		/* Wildcard match. */
	ns_t_zxfr = 256,	/* BIND-specific, nonstandard. */
	ns_t_max = 65536
} ns_type;

#endif /* __BIONIC__ */

static xvariant_t *
parse_res_srv (const xuint8_t  *answer,
               const xuint8_t  *end,
               const xuint8_t **p)
{
  xchar_t namebuf[1024];
  xuint16_t priority, weight, port;

  GETSHORT (priority, *p);
  GETSHORT (weight, *p);
  GETSHORT (port, *p);
  *p += dn_expand (answer, end, *p, namebuf, sizeof (namebuf));

  return xvariant_new ("(qqqs)",
                        priority,
                        weight,
                        port,
                        namebuf);
}

static xvariant_t *
parse_res_soa (const xuint8_t  *answer,
               const xuint8_t  *end,
               const xuint8_t **p)
{
  xchar_t mnamebuf[1024];
  xchar_t rnamebuf[1024];
  xuint32_t serial, refresh, retry, expire, ttl;

  *p += dn_expand (answer, end, *p, mnamebuf, sizeof (mnamebuf));
  *p += dn_expand (answer, end, *p, rnamebuf, sizeof (rnamebuf));

  GETLONG (serial, *p);
  GETLONG (refresh, *p);
  GETLONG (retry, *p);
  GETLONG (expire, *p);
  GETLONG (ttl, *p);

  return xvariant_new ("(ssuuuuu)",
                        mnamebuf,
                        rnamebuf,
                        serial,
                        refresh,
                        retry,
                        expire,
                        ttl);
}

static xvariant_t *
parse_res_ns (const xuint8_t  *answer,
              const xuint8_t  *end,
              const xuint8_t **p)
{
  xchar_t namebuf[1024];

  *p += dn_expand (answer, end, *p, namebuf, sizeof (namebuf));

  return xvariant_new ("(s)", namebuf);
}

static xvariant_t *
parse_res_mx (const xuint8_t  *answer,
              const xuint8_t  *end,
              const xuint8_t **p)
{
  xchar_t namebuf[1024];
  xuint16_t preference;

  GETSHORT (preference, *p);

  *p += dn_expand (answer, end, *p, namebuf, sizeof (namebuf));

  return xvariant_new ("(qs)",
                        preference,
                        namebuf);
}

static xvariant_t *
parse_res_txt (const xuint8_t  *answer,
               const xuint8_t  *end,
               const xuint8_t **p)
{
  xvariant_t *record;
  xptr_array_t *array;
  const xuint8_t *at = *p;
  xsize_t len;

  array = xptr_array_new_with_free_func (g_free);
  while (at < end)
    {
      len = *(at++);
      if (len > (xsize_t) (end - at))
        break;
      xptr_array_add (array, xstrndup ((xchar_t *)at, len));
      at += len;
    }

  *p = at;
  record = xvariant_new ("(@as)",
                          xvariant_new_strv ((const xchar_t **)array->pdata, array->len));
  xptr_array_free (array, TRUE);
  return record;
}

static xint_t
g_resolver_record_type_to_rrtype (GResolverRecordType type)
{
  switch (type)
  {
    case G_RESOLVER_RECORD_SRV:
      return T_SRV;
    case G_RESOLVER_RECORD_TXT:
      return T_TXT;
    case G_RESOLVER_RECORD_SOA:
      return T_SOA;
    case G_RESOLVER_RECORD_NS:
      return T_NS;
    case G_RESOLVER_RECORD_MX:
      return T_MX;
  }
  xreturn_val_if_reached (-1);
}

xlist_t *
g_resolver_records_from_res_query (const xchar_t      *rrname,
                                   xint_t              rrtype,
                                   const xuint8_t     *answer,
                                   xssize_t            len,
                                   xint_t              herr,
                                   xerror_t          **error)
{
  xint_t count;
  xchar_t namebuf[1024];
  const xuint8_t *end, *p;
  xuint16_t type, qclass, rdlength;
  const HEADER *header;
  xlist_t *records;
  xvariant_t *record;

  if (len <= 0)
    {
      if (len == 0 || herr == HOST_NOT_FOUND || herr == NO_DATA)
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND,
                       _("No DNS record of the requested type for “%s”"), rrname);
        }
      else if (herr == TRY_AGAIN)
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_TEMPORARY_FAILURE,
                       _("Temporarily unable to resolve “%s”"), rrname);
        }
      else
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                       _("Error resolving “%s”"), rrname);
        }

      return NULL;
    }

  records = NULL;

  header = (HEADER *)answer;
  p = answer + sizeof (HEADER);
  end = answer + len;

  /* Skip query */
  count = ntohs (header->qdcount);
  while (count-- && p < end)
    {
      p += dn_expand (answer, end, p, namebuf, sizeof (namebuf));
      p += 4;

      /* To silence gcc warnings */
      namebuf[0] = namebuf[1];
    }

  /* Read answers */
  count = ntohs (header->ancount);
  while (count-- && p < end)
    {
      p += dn_expand (answer, end, p, namebuf, sizeof (namebuf));
      GETSHORT (type, p);
      GETSHORT (qclass, p);
      p += 4; /* ignore the ttl (type=long) value */
      GETSHORT (rdlength, p);

      if (type != rrtype || qclass != C_IN)
        {
          p += rdlength;
          continue;
        }

      switch (rrtype)
        {
        case T_SRV:
          record = parse_res_srv (answer, end, &p);
          break;
        case T_MX:
          record = parse_res_mx (answer, end, &p);
          break;
        case T_SOA:
          record = parse_res_soa (answer, end, &p);
          break;
        case T_NS:
          record = parse_res_ns (answer, end, &p);
          break;
        case T_TXT:
          record = parse_res_txt (answer, p + rdlength, &p);
          break;
        default:
          g_warn_if_reached ();
          record = NULL;
          break;
        }

      if (record != NULL)
        records = xlist_prepend (records, record);
    }

  if (records == NULL)
    {
      g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND,
                   _("No DNS record of the requested type for “%s”"), rrname);

      return NULL;
    }
  else
    return records;
}

#elif defined(G_OS_WIN32)

static xvariant_t *
parse_dns_srv (DNS_RECORD *rec)
{
  return xvariant_new ("(qqqs)",
                        (xuint16_t)rec->Data.SRV.wPriority,
                        (xuint16_t)rec->Data.SRV.wWeight,
                        (xuint16_t)rec->Data.SRV.wPort,
                        rec->Data.SRV.pNameTarget);
}

static xvariant_t *
parse_dns_soa (DNS_RECORD *rec)
{
  return xvariant_new ("(ssuuuuu)",
                        rec->Data.SOA.pNamePrimaryServer,
                        rec->Data.SOA.pNameAdministrator,
                        (xuint32_t)rec->Data.SOA.dwSerialNo,
                        (xuint32_t)rec->Data.SOA.dwRefresh,
                        (xuint32_t)rec->Data.SOA.dwRetry,
                        (xuint32_t)rec->Data.SOA.dwExpire,
                        (xuint32_t)rec->Data.SOA.dwDefaultTtl);
}

static xvariant_t *
parse_dns_ns (DNS_RECORD *rec)
{
  return xvariant_new ("(s)", rec->Data.NS.pNameHost);
}

static xvariant_t *
parse_dns_mx (DNS_RECORD *rec)
{
  return xvariant_new ("(qs)",
                        (xuint16_t)rec->Data.MX.wPreference,
                        rec->Data.MX.pNameExchange);
}

static xvariant_t *
parse_dns_txt (DNS_RECORD *rec)
{
  xvariant_t *record;
  xptr_array_t *array;
  DWORD i;

  array = xptr_array_new ();
  for (i = 0; i < rec->Data.TXT.dwStringCount; i++)
    xptr_array_add (array, rec->Data.TXT.pStringArray[i]);
  record = xvariant_new ("(@as)",
                          xvariant_new_strv ((const xchar_t **)array->pdata, array->len));
  xptr_array_free (array, TRUE);
  return record;
}

static WORD
g_resolver_record_type_to_dnstype (GResolverRecordType type)
{
  switch (type)
  {
    case G_RESOLVER_RECORD_SRV:
      return DNS_TYPE_SRV;
    case G_RESOLVER_RECORD_TXT:
      return DNS_TYPE_TEXT;
    case G_RESOLVER_RECORD_SOA:
      return DNS_TYPE_SOA;
    case G_RESOLVER_RECORD_NS:
      return DNS_TYPE_NS;
    case G_RESOLVER_RECORD_MX:
      return DNS_TYPE_MX;
  }
  xreturn_val_if_reached (-1);
}

static xlist_t *
g_resolver_records_from_DnsQuery (const xchar_t  *rrname,
                                  WORD          dnstype,
                                  DNS_STATUS    status,
                                  DNS_RECORD   *results,
                                  xerror_t      **error)
{
  DNS_RECORD *rec;
  xpointer_t record;
  xlist_t *records;

  if (status != ERROR_SUCCESS)
    {
      if (status == DNS_ERROR_RCODE_NAME_ERROR)
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND,
                       _("No DNS record of the requested type for “%s”"), rrname);
        }
      else if (status == DNS_ERROR_RCODE_SERVER_FAILURE)
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_TEMPORARY_FAILURE,
                       _("Temporarily unable to resolve “%s”"), rrname);
        }
      else
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                       _("Error resolving “%s”"), rrname);
        }

      return NULL;
    }

  records = NULL;
  for (rec = results; rec; rec = rec->pNext)
    {
      if (rec->wType != dnstype)
        continue;
      switch (dnstype)
        {
        case DNS_TYPE_SRV:
          record = parse_dns_srv (rec);
          break;
        case DNS_TYPE_SOA:
          record = parse_dns_soa (rec);
          break;
        case DNS_TYPE_NS:
          record = parse_dns_ns (rec);
          break;
        case DNS_TYPE_MX:
          record = parse_dns_mx (rec);
          break;
        case DNS_TYPE_TEXT:
          record = parse_dns_txt (rec);
          break;
        default:
          g_warn_if_reached ();
          record = NULL;
          break;
        }
      if (record != NULL)
        records = xlist_prepend (records, xvariant_ref_sink (record));
    }

  if (records == NULL)
    {
      g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND,
                   _("No DNS record of the requested type for “%s”"), rrname);

      return NULL;
    }
  else
    return records;
}

#endif

typedef struct {
  char *rrname;
  GResolverRecordType record_type;
} LookupRecordsData;

static void
free_lookup_records_data (LookupRecordsData *lrd)
{
  g_free (lrd->rrname);
  g_slice_free (LookupRecordsData, lrd);
}

static void
free_records (xlist_t *records)
{
  xlist_free_full (records, (xdestroy_notify_t) xvariant_unref);
}

#if defined(G_OS_UNIX)
#ifdef __BIONIC__
#ifndef C_IN
#define C_IN 1
#endif
int res_query(const char *, int, int, u_char *, int);
#endif
#endif

static void
do_lookup_records (xtask_t         *task,
                   xpointer_t       source_object,
                   xpointer_t       task_data,
                   xcancellable_t  *cancellable)
{
  LookupRecordsData *lrd = task_data;
  xlist_t *records;
  xerror_t *error = NULL;

#if defined(G_OS_UNIX)
  xint_t len = 512;
  xint_t herr;
  xbyte_array_t *answer;
  xint_t rrtype;

#ifdef HAVE_RES_NQUERY
  /* Load the resolver state. This is done once per worker thread, and the
   * #xresolver_t::reload signal is ignored (since we always reload). This could
   * be improved by having an explicit worker thread pool, with each thread
   * containing some state which is initialised at thread creation time and
   * updated in response to #xresolver_t::reload.
   *
   * What we have currently is not particularly worse than using res_query() in
   * worker threads, since it would transparently call res_init() for each new
   * worker thread. (Although the workers would get reused by the
   * #GThreadPool.)
   *
   * FreeBSD requires the state to be zero-filled before calling res_ninit(). */
  struct __res_state res = { 0, };
  if (res_ninit (&res) != 0)
    {
      xtask_return_new_error (task, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                               _("Error resolving “%s”"), lrd->rrname);
      return;
    }
#endif

  rrtype = g_resolver_record_type_to_rrtype (lrd->record_type);
  answer = xbyte_array_new ();
  for (;;)
    {
      xbyte_array_set_size (answer, len * 2);
#if defined(HAVE_RES_NQUERY)
      len = res_nquery (&res, lrd->rrname, C_IN, rrtype, answer->data, answer->len);
#else
      len = res_query (lrd->rrname, C_IN, rrtype, answer->data, answer->len);
#endif

      /* If answer fit in the buffer then we're done */
      if (len < 0 || len < (xint_t)answer->len)
        break;

      /*
       * On overflow some res_query's return the length needed, others
       * return the full length entered. This code works in either case.
       */
    }

  herr = h_errno;
  records = g_resolver_records_from_res_query (lrd->rrname, rrtype, answer->data, len, herr, &error);
  xbyte_array_free (answer, TRUE);

#ifdef HAVE_RES_NQUERY

#if defined(HAVE_RES_NDESTROY)
  res_ndestroy (&res);
#elif defined(HAVE_RES_NCLOSE)
  res_nclose (&res);
#elif defined(HAVE_RES_NINIT)
#error "Your platform has res_ninit() but not res_nclose() or res_ndestroy(). Please file a bug at https://gitlab.gnome.org/GNOME/glib/issues/new"
#endif

#endif  /* HAVE_RES_NQUERY */

#else

  DNS_STATUS status;
  DNS_RECORD *results = NULL;
  WORD dnstype;

  dnstype = g_resolver_record_type_to_dnstype (lrd->record_type);
  status = DnsQuery_A (lrd->rrname, dnstype, DNS_QUERY_STANDARD, NULL, &results, NULL);
  records = g_resolver_records_from_DnsQuery (lrd->rrname, dnstype, status, results, &error);
  if (results != NULL)
    DnsRecordListFree (results, DnsFreeRecordList);

#endif

  if (records)
    xtask_return_pointer (task, records, (xdestroy_notify_t) free_records);
  else
    xtask_return_error (task, error);
}

static xlist_t *
lookup_records (xresolver_t              *resolver,
                const xchar_t            *rrname,
                GResolverRecordType     record_type,
                xcancellable_t           *cancellable,
                xerror_t                **error)
{
  xtask_t *task;
  xlist_t *records;
  LookupRecordsData *lrd;

  task = xtask_new (resolver, cancellable, NULL, NULL);
  xtask_set_source_tag (task, lookup_records);
  xtask_set_name (task, "[gio] resolver lookup records");

  lrd = g_slice_new (LookupRecordsData);
  lrd->rrname = xstrdup (rrname);
  lrd->record_type = record_type;
  xtask_set_task_data (task, lrd, (xdestroy_notify_t) free_lookup_records_data);

  xtask_set_return_on_cancel (task, TRUE);
  xtask_run_in_thread_sync (task, do_lookup_records);
  records = xtask_propagate_pointer (task, error);
  xobject_unref (task);

  return records;
}

static void
lookup_records_async (xresolver_t           *resolver,
                      const char          *rrname,
                      GResolverRecordType  record_type,
                      xcancellable_t        *cancellable,
                      xasync_ready_callback_t  callback,
                      xpointer_t             user_data)
{
  xtask_t *task;
  LookupRecordsData *lrd;

  task = xtask_new (resolver, cancellable, callback, user_data);
  xtask_set_source_tag (task, lookup_records_async);
  xtask_set_name (task, "[gio] resolver lookup records");

  lrd = g_slice_new (LookupRecordsData);
  lrd->rrname = xstrdup (rrname);
  lrd->record_type = record_type;
  xtask_set_task_data (task, lrd, (xdestroy_notify_t) free_lookup_records_data);

  xtask_set_return_on_cancel (task, TRUE);
  xtask_run_in_thread (task, do_lookup_records);
  xobject_unref (task);
}

static xlist_t *
lookup_records_finish (xresolver_t     *resolver,
                       xasync_result_t  *result,
                       xerror_t       **error)
{
  xreturn_val_if_fail (xtask_is_valid (result, resolver), NULL);

  return xtask_propagate_pointer (XTASK (result), error);
}


static void
xthreaded_resolver_class_init (GThreadedResolverClass *threaded_class)
{
  GResolverClass *resolver_class = G_RESOLVER_CLASS (threaded_class);

  resolver_class->lookup_by_name                   = lookup_by_name;
  resolver_class->lookup_by_name_async             = lookup_by_name_async;
  resolver_class->lookup_by_name_finish            = lookup_by_name_finish;
  resolver_class->lookup_by_name_with_flags        = lookup_by_name_with_flags;
  resolver_class->lookup_by_name_with_flags_async  = lookup_by_name_with_flags_async;
  resolver_class->lookup_by_name_with_flags_finish = lookup_by_name_with_flags_finish;
  resolver_class->lookup_by_address                = lookup_by_address;
  resolver_class->lookup_by_address_async          = lookup_by_address_async;
  resolver_class->lookup_by_address_finish         = lookup_by_address_finish;
  resolver_class->lookup_records                   = lookup_records;
  resolver_class->lookup_records_async             = lookup_records_async;
  resolver_class->lookup_records_finish            = lookup_records_finish;
}
