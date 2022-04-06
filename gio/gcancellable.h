/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __G_CANCELLABLE_H__
#define __G_CANCELLABLE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define XTYPE_CANCELLABLE         (g_cancellable_get_type ())
#define G_CANCELLABLE(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_CANCELLABLE, xcancellable))
#define G_CANCELLABLE_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_CANCELLABLE, GCancellableClass))
#define X_IS_CANCELLABLE(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_CANCELLABLE))
#define X_IS_CANCELLABLE_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_CANCELLABLE))
#define G_CANCELLABLE_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_CANCELLABLE, GCancellableClass))

/**
 * xcancellable_t:
 *
 * Allows actions to be cancelled.
 */
typedef struct _GCancellableClass   GCancellableClass;
typedef struct _GCancellablePrivate GCancellablePrivate;

struct _GCancellable
{
  xobject_t parent_instance;

  /*< private >*/
  GCancellablePrivate *priv;
};

struct _GCancellableClass
{
  xobject_class_t parent_class;

  void (* cancelled) (xcancellable_t *cancellable);

  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

XPL_AVAILABLE_IN_ALL
xtype_t         g_cancellable_get_type               (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xcancellable_t *g_cancellable_new                    (void);

/* These are only safe to call inside a cancellable op */
XPL_AVAILABLE_IN_ALL
xboolean_t      g_cancellable_is_cancelled           (xcancellable_t  *cancellable);
XPL_AVAILABLE_IN_ALL
xboolean_t      g_cancellable_set_error_if_cancelled (xcancellable_t  *cancellable,
						    xerror_t       **error);

XPL_AVAILABLE_IN_ALL
int           g_cancellable_get_fd                 (xcancellable_t  *cancellable);
XPL_AVAILABLE_IN_ALL
xboolean_t      g_cancellable_make_pollfd            (xcancellable_t  *cancellable,
						    xpollfd_t       *pollfd);
XPL_AVAILABLE_IN_ALL
void          g_cancellable_release_fd             (xcancellable_t  *cancellable);

XPL_AVAILABLE_IN_ALL
xsource_t *     g_cancellable_source_new             (xcancellable_t  *cancellable);

XPL_AVAILABLE_IN_ALL
xcancellable_t *g_cancellable_get_current            (void);
XPL_AVAILABLE_IN_ALL
void          g_cancellable_push_current           (xcancellable_t  *cancellable);
XPL_AVAILABLE_IN_ALL
void          g_cancellable_pop_current            (xcancellable_t  *cancellable);
XPL_AVAILABLE_IN_ALL
void          g_cancellable_reset                  (xcancellable_t  *cancellable);
XPL_AVAILABLE_IN_ALL
gulong        g_cancellable_connect                (xcancellable_t  *cancellable,
						    xcallback_t      callback,
						    xpointer_t       data,
						    xdestroy_notify_t data_destroy_func);
XPL_AVAILABLE_IN_ALL
void          g_cancellable_disconnect             (xcancellable_t  *cancellable,
						    gulong         handler_id);


/* This is safe to call from another thread */
XPL_AVAILABLE_IN_ALL
void          g_cancellable_cancel       (xcancellable_t  *cancellable);

G_END_DECLS

#endif /* __G_CANCELLABLE_H__ */
