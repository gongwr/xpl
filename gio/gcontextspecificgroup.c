/*
 * Copyright © 2015 Canonical Limited
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gcontextspecificgroup.h"

#include <glib-object.h>
#include "glib-private.h"

typedef struct
{
  xsource_t   source;

  xmutex_t    lock;
  xpointer_t  instance;
  xqueue_t    pending;
} GContextSpecificSource;

static xboolean_t
g_context_specific_source_dispatch (xsource_t     *source,
                                    xsource_func_t  callback,
                                    xpointer_t     user_data)
{
  GContextSpecificSource *css = (GContextSpecificSource *) source;
  xuint_t signal_id;

  g_mutex_lock (&css->lock);

  g_assert (!g_queue_is_empty (&css->pending));
  signal_id = GPOINTER_TO_UINT (g_queue_pop_head (&css->pending));

  if (g_queue_is_empty (&css->pending))
    xsource_set_ready_time (source, -1);

  g_mutex_unlock (&css->lock);

  xsignal_emit (css->instance, signal_id, 0);

  return TRUE;
}

static void
g_context_specific_source_finalize (xsource_t *source)
{
  GContextSpecificSource *css = (GContextSpecificSource *) source;

  g_mutex_clear (&css->lock);
  g_queue_clear (&css->pending);
}

static GContextSpecificSource *
g_context_specific_source_new (const xchar_t *name,
                               xpointer_t     instance)
{
  static xsource_funcs_t source_funcs = {
    NULL,
    NULL,
    g_context_specific_source_dispatch,
    g_context_specific_source_finalize,
    NULL, NULL
  };
  GContextSpecificSource *css;
  xsource_t *source;

  source = xsource_new (&source_funcs, sizeof (GContextSpecificSource));
  css = (GContextSpecificSource *) source;

  xsource_set_name (source, name);

  g_mutex_init (&css->lock);
  g_queue_init (&css->pending);
  css->instance = instance;

  return css;
}

static xboolean_t
g_context_specific_group_change_state (xpointer_t user_data)
{
  GContextSpecificGroup *group = user_data;

  g_mutex_lock (&group->lock);

  if (group->requested_state != group->effective_state)
    {
      (* group->requested_func) ();

      group->effective_state = group->requested_state;
      group->requested_func = NULL;

      g_cond_broadcast (&group->cond);
    }

  g_mutex_unlock (&group->lock);

  return FALSE;
}

/* this is not the most elegant way to deal with this, but it's probably
 * the best.  there are only two other things we could do, really:
 *
 *  - run the start function (but not the stop function) from the user's
 *    thread under some sort of lock.  we don't run the stop function
 *    from the user's thread to avoid the destroy-while-emitting problem
 *
 *  - have some check-and-compare functionality similar to what
 *    gsettings does where we send an artificial event in case we notice
 *    a change during the potential race period (using stat, for
 *    example)
 */
static void
g_context_specific_group_request_state (GContextSpecificGroup *group,
                                        xboolean_t               requested_state,
                                        xcallback_t              requested_func)
{
  if (requested_state != group->requested_state)
    {
      if (group->effective_state != group->requested_state)
        {
          /* abort the currently pending state transition */
          g_assert (group->effective_state == requested_state);

          group->requested_state = requested_state;
          group->requested_func = NULL;
        }
      else
        {
          /* start a new state transition */
          group->requested_state = requested_state;
          group->requested_func = requested_func;

          xmain_context_invoke (XPL_PRIVATE_CALL(g_get_worker_context) (),
                                 g_context_specific_group_change_state, group);
        }
    }

  /* we only block for positive transitions */
  if (requested_state)
    {
      while (group->requested_state != group->effective_state)
        g_cond_wait (&group->cond, &group->lock);

      /* there is no way this could go back to FALSE because the object
       * that we just created in this thread would have to have been
       * destroyed again (from this thread) before that could happen.
       */
      g_assert (group->effective_state);
    }
}

xpointer_t
g_context_specific_group_get (GContextSpecificGroup *group,
                              xtype_t                  type,
                              xoffset_t                context_offset,
                              xcallback_t              start_func)
{
  GContextSpecificSource *css;
  xmain_context_t *context;

  context = xmain_context_get_thread_default ();
  if (!context)
    context = xmain_context_default ();

  g_mutex_lock (&group->lock);

  if (!group->table)
    group->table = xhash_table_new (NULL, NULL);

  css = xhash_table_lookup (group->table, context);

  if (!css)
    {
      xpointer_t instance;

      instance = xobject_new (type, NULL);
      css = g_context_specific_source_new (xtype_name (type), instance);
      G_STRUCT_MEMBER (xmain_context_t *, instance, context_offset) = xmain_context_ref (context);
      xsource_attach ((xsource_t *) css, context);

      xhash_table_insert (group->table, context, css);
    }
  else
    xobject_ref (css->instance);

  if (start_func)
    g_context_specific_group_request_state (group, TRUE, start_func);

  g_mutex_unlock (&group->lock);

  return css->instance;
}

void
g_context_specific_group_remove (GContextSpecificGroup *group,
                                 xmain_context_t          *context,
                                 xpointer_t               instance,
                                 xcallback_t              stop_func)
{
  GContextSpecificSource *css;

  if (!context)
    {
      g_critical ("Removing %s with NULL context.  This object was probably directly constructed from a "
                  "dynamic language.  This is not a valid use of the API.", G_OBJECT_TYPE_NAME (instance));
      return;
    }

  g_mutex_lock (&group->lock);
  css = xhash_table_lookup (group->table, context);
  xhash_table_remove (group->table, context);
  g_assert (css);

  /* stop only if we were the last one */
  if (stop_func && xhash_table_size (group->table) == 0)
    g_context_specific_group_request_state (group, FALSE, stop_func);

  g_mutex_unlock (&group->lock);

  g_assert (css->instance == instance);

  xsource_destroy ((xsource_t *) css);
  xsource_unref ((xsource_t *) css);
  xmain_context_unref (context);
}

void
g_context_specific_group_emit (GContextSpecificGroup *group,
                               xuint_t                  signal_id)
{
  g_mutex_lock (&group->lock);

  if (group->table)
    {
      xhash_table_iter_t iter;
      xpointer_t value;
      xpointer_t ptr;

      ptr = GUINT_TO_POINTER (signal_id);

      xhash_table_iter_init (&iter, group->table);
      while (xhash_table_iter_next (&iter, NULL, &value))
        {
          GContextSpecificSource *css = value;

          g_mutex_lock (&css->lock);

          g_queue_remove (&css->pending, ptr);
          g_queue_push_tail (&css->pending, ptr);

          xsource_set_ready_time ((xsource_t *) css, 0);

          g_mutex_unlock (&css->lock);
        }
    }

  g_mutex_unlock (&group->lock);
}
