/* GIO - GLib Input, Output and Streaming Library
 *
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

#include "mock-resolver.h"

struct _MockResolver
{
  xresolver_t parent_instance;
  xuint_t ipv4_delay_ms;
  xuint_t ipv6_delay_ms;
  xlist_t *ipv4_results;
  xlist_t *ipv6_results;
  xerror_t *ipv4_error;
  xerror_t *ipv6_error;
};

XDEFINE_TYPE (MockResolver, mock_resolver, XTYPE_RESOLVER)

MockResolver *
mock_resolver_new (void)
{
  return xobject_new (MOCK_TYPE_RESOLVER, NULL);
}

void
mock_resolver_set_ipv4_delay_ms (MockResolver *self, xuint_t delay_ms)
{
  self->ipv4_delay_ms = delay_ms;
}

static xpointer_t
copy_object (xconstpointer obj, xpointer_t user_data)
{
  return xobject_ref (G_OBJECT (obj));
}

void
mock_resolver_set_ipv4_results (MockResolver *self, xlist_t *results)
{
  if (self->ipv4_results)
    xlist_free_full (self->ipv4_results, xobject_unref);
  self->ipv4_results = xlist_copy_deep (results, copy_object, NULL);
}

void
mock_resolver_set_ipv4_error (MockResolver *self, xerror_t *error)
{
  g_clear_error (&self->ipv4_error);
  if (error)
    self->ipv4_error = xerror_copy (error);
}

void
mock_resolver_set_ipv6_delay_ms (MockResolver *self, xuint_t delay_ms)
{
  self->ipv6_delay_ms = delay_ms;
}

void
mock_resolver_set_ipv6_results (MockResolver *self, xlist_t *results)
{
  if (self->ipv6_results)
    xlist_free_full (self->ipv6_results, xobject_unref);
  self->ipv6_results = xlist_copy_deep (results, copy_object, NULL);
}

void
mock_resolver_set_ipv6_error (MockResolver *self, xerror_t *error)
{
  g_clear_error (&self->ipv6_error);
  if (error)
    self->ipv6_error = xerror_copy (error);
}

static xboolean_t lookup_by_name_cb (xpointer_t user_data);

/* Core of the implementation of `lookup_by_name()` in the mock resolver.
 *
 * It creates a #xsource_t which will become ready with the resolver results. It
 * will become ready either after a timeout, or as an idle callback. This
 * simulates doing some actual network-based resolution work.
 *
 * A previous implementation of this did the work in a thread, but that made it
 * hard to synchronise the timeouts with the #xresolver_t failure timeouts in the
 * calling thread, as spawning a worker thread could be subject to non-trivial
 * delays. */
static void
do_lookup_by_name (MockResolver             *self,
                   xtask_t                    *task,
                   GResolverNameLookupFlags  flags)
{
  xsource_t *source = NULL;

  xtask_set_task_data (task, GINT_TO_POINTER (flags), NULL);

  if (flags == G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY)
    source = g_timeout_source_new (self->ipv4_delay_ms);
  else if (flags == G_RESOLVER_NAME_LOOKUP_FLAGS_IPV6_ONLY)
    source = g_timeout_source_new (self->ipv6_delay_ms);
  else if (flags == G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT)
    source = g_idle_source_new ();
  else
    g_assert_not_reached ();

  xsource_set_callback (source, lookup_by_name_cb, xobject_ref (task), xobject_unref);
  xsource_attach (source, xmain_context_get_thread_default ());
  xsource_unref (source);
}

static xboolean_t
lookup_by_name_cb (xpointer_t user_data)
{
  xtask_t *task = XTASK (user_data);
  MockResolver *self = xtask_get_source_object (task);
  GResolverNameLookupFlags flags = GPOINTER_TO_INT (xtask_get_task_data (task));

  if (flags == G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY)
    {
      if (self->ipv4_error)
        xtask_return_error (task, xerror_copy (self->ipv4_error));
      else
        xtask_return_pointer (task, xlist_copy_deep (self->ipv4_results, copy_object, NULL), NULL);
    }
  else if (flags == G_RESOLVER_NAME_LOOKUP_FLAGS_IPV6_ONLY)
    {
      if (self->ipv6_error)
        xtask_return_error (task, xerror_copy (self->ipv6_error));
      else
        xtask_return_pointer (task, xlist_copy_deep (self->ipv6_results, copy_object, NULL), NULL);
    }
  else if (flags == G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT)
    {
      /* This is only the minimal implementation needed for some tests */
      xassert (self->ipv4_error == NULL && self->ipv6_error == NULL && self->ipv6_results == NULL);
      xtask_return_pointer (task, xlist_copy_deep (self->ipv4_results, copy_object, NULL), NULL);
    }
  else
    g_assert_not_reached ();

  return XSOURCE_REMOVE;
}

static void
lookup_by_name_with_flags_async (xresolver_t                *resolver,
                                 const xchar_t              *hostname,
                                 GResolverNameLookupFlags  flags,
                                 xcancellable_t             *cancellable,
                                 xasync_ready_callback_t       callback,
                                 xpointer_t                  user_data)
{
  MockResolver *self = MOCK_RESOLVER (resolver);
  xtask_t *task = NULL;

  task = xtask_new (resolver, cancellable, callback, user_data);
  xtask_set_source_tag (task, lookup_by_name_with_flags_async);

  do_lookup_by_name (self, task, flags);

  xobject_unref (task);
}

static void
async_result_cb (xobject_t      *source_object,
                 xasync_result_t *result,
                 xpointer_t      user_data)
{
  xasync_result_t **result_out = user_data;

  xassert (*result_out == NULL);
  *result_out = xobject_ref (result);

  xmain_context_wakeup (xmain_context_get_thread_default ());
}

static xlist_t *
lookup_by_name (xresolver_t     *resolver,
                const xchar_t   *hostname,
                xcancellable_t  *cancellable,
                xerror_t       **error)
{
  MockResolver *self = MOCK_RESOLVER (resolver);
  xmain_context_t *context = NULL;
  xlist_t *result = NULL;
  xasync_result_t *async_result = NULL;
  xtask_t *task = NULL;

  context = xmain_context_new ();
  xmain_context_push_thread_default (context);

  task = xtask_new (resolver, cancellable, async_result_cb, &async_result);
  xtask_set_source_tag (task, lookup_by_name);

  /* Set up the resolution job. */
  do_lookup_by_name (self, task, G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT);

  /* Wait for it to complete synchronously. */
  while (async_result == NULL)
    xmain_context_iteration (context, TRUE);

  result = xtask_propagate_pointer (XTASK (async_result), error);
  xobject_unref (async_result);

  g_assert_finalize_object (task);

  xmain_context_pop_thread_default (context);
  xmain_context_unref (context);

  return g_steal_pointer (&result);
}

static xlist_t *
lookup_by_name_with_flags_finish (xresolver_t     *resolver,
                                  xasync_result_t  *result,
                                  xerror_t       **error)
{
  return xtask_propagate_pointer (XTASK (result), error);
}

static void
mock_resolver_finalize (xobject_t *object)
{
  MockResolver *self = (MockResolver*)object;

  g_clear_error (&self->ipv4_error);
  g_clear_error (&self->ipv6_error);
  if (self->ipv6_results)
    xlist_free_full (self->ipv6_results, xobject_unref);
  if (self->ipv4_results)
    xlist_free_full (self->ipv4_results, xobject_unref);

  XOBJECT_CLASS (mock_resolver_parent_class)->finalize (object);
}

static void
mock_resolver_class_init (MockResolverClass *klass)
{
  GResolverClass *resolver_class = G_RESOLVER_CLASS (klass);
  xobject_class_t *object_class = XOBJECT_CLASS (klass);
  resolver_class->lookup_by_name_with_flags_async  = lookup_by_name_with_flags_async;
  resolver_class->lookup_by_name_with_flags_finish = lookup_by_name_with_flags_finish;
  resolver_class->lookup_by_name = lookup_by_name;
  object_class->finalize = mock_resolver_finalize;
}

static void
mock_resolver_init (MockResolver *self)
{
}
