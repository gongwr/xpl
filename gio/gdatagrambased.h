/*
 * Copyright 2015 Collabora Ltd.
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
 * Authors: Philip Withnall <philip.withnall@collabora.co.uk>
 */

#ifndef __G_DATAGRAM_BASED_H__
#define __G_DATAGRAM_BASED_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_DATAGRAM_BASED             (g_datagram_based_get_type ())
#define G_DATAGRAM_BASED(inst)            (XTYPE_CHECK_INSTANCE_CAST ((inst), \
                                           XTYPE_DATAGRAM_BASED, xdatagram_based_t))
#define X_IS_DATAGRAM_BASED(inst)         (XTYPE_CHECK_INSTANCE_TYPE ((inst), \
                                           XTYPE_DATAGRAM_BASED))
#define G_DATAGRAM_BASED_GET_IFACE(inst)  (XTYPE_INSTANCE_GET_INTERFACE ((inst), \
                                           XTYPE_DATAGRAM_BASED, \
                                           GDatagramBasedInterface))
#define XTYPE_IS_DATAGRAM_BASED(type)    (xtype_is_a ((type), \
                                           XTYPE_DATAGRAM_BASED))

/**
 * xdatagram_based_t:
 *
 * Interface for socket-like objects with datagram semantics.
 *
 * Since: 2.48
 */
typedef struct _GDatagramBasedInterface GDatagramBasedInterface;

/**
 * GDatagramBasedInterface:
 * @x_iface: The parent interface.
 * @receive_messages: Virtual method for g_datagram_based_receive_messages().
 * @send_messages: Virtual method for g_datagram_based_send_messages().
 * @create_source: Virtual method for g_datagram_based_create_source().
 * @condition_check: Virtual method for g_datagram_based_condition_check().
 * @condition_wait: Virtual method for
 *   g_datagram_based_condition_wait().
 *
 * Provides an interface for socket-like objects which have datagram semantics,
 * following the Berkeley sockets API. The interface methods are thin wrappers
 * around the corresponding virtual methods, and no pre-processing of inputs is
 * implemented — so implementations of this API must handle all functionality
 * documented in the interface methods.
 *
 * Since: 2.48
 */
struct _GDatagramBasedInterface
{
  xtype_interface_t x_iface;

  /* Virtual table */
  xint_t          (*receive_messages)     (xdatagram_based_t       *datagram_based,
                                         xinput_message_t        *messages,
                                         xuint_t                 num_messages,
                                         xint_t                  flags,
                                         sint64_t                timeout,
                                         xcancellable_t         *cancellable,
                                         xerror_t              **error);
  xint_t          (*send_messages)        (xdatagram_based_t       *datagram_based,
                                         xoutput_message_t       *messages,
                                         xuint_t                 num_messages,
                                         xint_t                  flags,
                                         sint64_t                timeout,
                                         xcancellable_t         *cancellable,
                                         xerror_t              **error);

  xsource_t      *(*create_source)        (xdatagram_based_t       *datagram_based,
                                         xio_condition_t          condition,
                                         xcancellable_t         *cancellable);
  xio_condition_t  (*condition_check)      (xdatagram_based_t       *datagram_based,
                                         xio_condition_t          condition);
  xboolean_t      (*condition_wait)       (xdatagram_based_t       *datagram_based,
                                         xio_condition_t          condition,
                                         sint64_t                timeout,
                                         xcancellable_t         *cancellable,
                                         xerror_t              **error);
};

XPL_AVAILABLE_IN_2_48
xtype_t
g_datagram_based_get_type             (void);

XPL_AVAILABLE_IN_2_48
xint_t
g_datagram_based_receive_messages     (xdatagram_based_t       *datagram_based,
                                       xinput_message_t        *messages,
                                       xuint_t                 num_messages,
                                       xint_t                  flags,
                                       sint64_t                timeout,
                                       xcancellable_t         *cancellable,
                                       xerror_t              **error);

XPL_AVAILABLE_IN_2_48
xint_t
g_datagram_based_send_messages        (xdatagram_based_t       *datagram_based,
                                       xoutput_message_t       *messages,
                                       xuint_t                 num_messages,
                                       xint_t                  flags,
                                       sint64_t                timeout,
                                       xcancellable_t         *cancellable,
                                       xerror_t              **error);

XPL_AVAILABLE_IN_2_48
xsource_t *
g_datagram_based_create_source        (xdatagram_based_t       *datagram_based,
                                       xio_condition_t          condition,
                                       xcancellable_t         *cancellable);
XPL_AVAILABLE_IN_2_48
xio_condition_t
g_datagram_based_condition_check      (xdatagram_based_t       *datagram_based,
                                       xio_condition_t          condition);
XPL_AVAILABLE_IN_2_48
xboolean_t
g_datagram_based_condition_wait       (xdatagram_based_t       *datagram_based,
                                       xio_condition_t          condition,
                                       sint64_t                timeout,
                                       xcancellable_t         *cancellable,
                                       xerror_t              **error);

G_END_DECLS

#endif /* __G_DATAGRAM_BASED_H__ */
