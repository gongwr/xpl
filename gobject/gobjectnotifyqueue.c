/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1998-1999, 2000-2001 Tim Janik and Red Hat, Inc.
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

/* WARNING:
 *
 *    This file is INSTALLED and other projects (outside of glib)
 *    #include its contents.
 */

#ifndef __G_OBJECT_NOTIFY_QUEUE_H__
#define __G_OBJECT_NOTIFY_QUEUE_H__

#include <string.h> /* memset */

#include <glib-object.h>

G_BEGIN_DECLS


/* --- typedefs --- */
typedef struct _GObjectNotifyContext          GObjectNotifyContext;
typedef struct _xobject_notify_queue            xobject_notify_queue_t;
typedef void (*xobject_notify_queue_dispatcher_t) (xobject_t     *object,
					      xuint_t        n_pspecs,
					      xparam_spec_t **pspecs);


/* --- structures --- */
struct _GObjectNotifyContext
{
  xquark                       quark_notify_queue;
  xobject_notify_queue_dispatcher_t dispatcher;
  GTrashStack                 *_nqueue_trash; /* unused */
};
struct _xobject_notify_queue
{
  GObjectNotifyContext *context;
  xslist_t               *pspecs;
  xuint16_t               n_pspecs;
  xuint16_t               freeze_count;
};

G_LOCK_DEFINE_STATIC(notify_lock);

/* --- functions --- */
static void
xobject_notify_queue_free (xpointer_t data)
{
  xobject_notify_queue_t *nqueue = data;

  xslist_free (nqueue->pspecs);
  g_slice_free (xobject_notify_queue_t, nqueue);
}

static inline xobject_notify_queue_t*
xobject_notify_queue_freeze (xobject_t		   *object,
			      GObjectNotifyContext *context)
{
  xobject_notify_queue_t *nqueue;

  G_LOCK(notify_lock);
  nqueue = g_datalist_id_get_data (&object->qdata, context->quark_notify_queue);
  if (!nqueue)
    {
      nqueue = g_slice_new0 (xobject_notify_queue_t);
      nqueue->context = context;
      g_datalist_id_set_data_full (&object->qdata, context->quark_notify_queue,
				   nqueue, xobject_notify_queue_free);
    }

  if (nqueue->freeze_count >= 65535)
    g_critical("Free queue for %s (%p) is larger than 65535,"
               " called xobject_freeze_notify() too often."
               " Forgot to call xobject_thaw_notify() or infinite loop",
               G_OBJECT_TYPE_NAME (object), object);
  else
    nqueue->freeze_count++;
  G_UNLOCK(notify_lock);

  return nqueue;
}

static inline void
xobject_notify_queue_thaw (xobject_t            *object,
			    xobject_notify_queue_t *nqueue)
{
  GObjectNotifyContext *context = nqueue->context;
  xparam_spec_t *pspecs_mem[16], **pspecs, **free_me = NULL;
  xslist_t *slist;
  xuint_t n_pspecs = 0;

  g_return_if_fail (nqueue->freeze_count > 0);
  g_return_if_fail (g_atomic_int_get(&object->ref_count) > 0);

  G_LOCK(notify_lock);

  /* Just make sure we never get into some nasty race condition */
  if (G_UNLIKELY(nqueue->freeze_count == 0)) {
    G_UNLOCK(notify_lock);
    g_warning ("%s: property-changed notification for %s(%p) is not frozen",
	       G_STRFUNC, G_OBJECT_TYPE_NAME (object), object);
    return;
  }

  nqueue->freeze_count--;
  if (nqueue->freeze_count) {
    G_UNLOCK(notify_lock);
    return;
  }

  pspecs = nqueue->n_pspecs > 16 ? free_me = g_new (xparam_spec_t*, nqueue->n_pspecs) : pspecs_mem;

  for (slist = nqueue->pspecs; slist; slist = slist->next)
    {
      pspecs[n_pspecs++] = slist->data;
    }
  g_datalist_id_set_data (&object->qdata, context->quark_notify_queue, NULL);

  G_UNLOCK(notify_lock);

  if (n_pspecs)
    context->dispatcher (object, n_pspecs, pspecs);
  g_free (free_me);
}

static inline void
xobject_notify_queue_clear (xobject_t            *object,
			     xobject_notify_queue_t *nqueue)
{
  g_return_if_fail (nqueue->freeze_count > 0);

  G_LOCK(notify_lock);

  xslist_free (nqueue->pspecs);
  nqueue->pspecs = NULL;
  nqueue->n_pspecs = 0;

  G_UNLOCK(notify_lock);
}

static inline void
xobject_notify_queue_add (xobject_t            *object,
			   xobject_notify_queue_t *nqueue,
			   xparam_spec_t	      *pspec)
{
  if (pspec->flags & G_PARAM_READABLE)
    {
      xparam_spec_t *redirect;

      G_LOCK(notify_lock);

      g_return_if_fail (nqueue->n_pspecs < 65535);

      redirect = g_param_spec_get_redirect_target (pspec);
      if (redirect)
	pspec = redirect;

      /* we do the deduping in _thaw */
      if (xslist_find (nqueue->pspecs, pspec) == NULL)
        {
          nqueue->pspecs = xslist_prepend (nqueue->pspecs, pspec);
          nqueue->n_pspecs++;
        }

      G_UNLOCK(notify_lock);
    }
}

/* NB: This function is not threadsafe, do not ever use it if
 * you need a threadsafe notify queue.
 * Use xobject_notify_queue_freeze() to acquire the queue and
 * xobject_notify_queue_thaw() after you are done instead.
 */
static inline xobject_notify_queue_t*
xobject_notify_queue_from_object (xobject_t              *object,
                                   GObjectNotifyContext *context)
{
  return g_datalist_id_get_data (&object->qdata, context->quark_notify_queue);
}

G_END_DECLS

#endif /* __G_OBJECT_NOTIFY_QUEUE_H__ */
