/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2001 Red Hat, Inc.
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

#include "gsourceclosure.h"
#include "gboxed.h"
#include "genums.h"
#include "gmarshal.h"
#include "gvalue.h"
#include "gvaluetypes.h"
#ifdef G_OS_UNIX
#include "glib-unix.h"
#endif

G_DEFINE_BOXED_TYPE (xio_channel_t, g_io_channel, g_io_channel_ref, g_io_channel_unref)

xtype_t
g_io_condition_get_type (void)
{
  static xtype_t etype = 0;

  if (g_once_init_enter (&etype))
    {
      static const xflags_value_t values[] = {
	{ G_IO_IN,   "G_IO_IN",   "in" },
	{ G_IO_OUT,  "G_IO_OUT",  "out" },
	{ G_IO_PRI,  "G_IO_PRI",  "pri" },
	{ G_IO_ERR,  "G_IO_ERR",  "err" },
	{ G_IO_HUP,  "G_IO_HUP",  "hup" },
	{ G_IO_NVAL, "G_IO_NVAL", "nval" },
	{ 0, NULL, NULL }
      };
      xtype_t type_id = xflags_register_static ("xio_condition_t", values);
      g_once_init_leave (&etype, type_id);
    }
  return etype;
}

/* We need to hand-write this marshaler, since it doesn't have an
 * instance object.
 */
static void
source_closure_marshal_BOOLEAN__VOID (xclosure_t     *closure,
				      xvalue_t       *return_value,
				      xuint_t         n_param_values,
				      const xvalue_t *param_values,
				      xpointer_t      invocation_hint,
				      xpointer_t      marshal_data)
{
  xsource_func_t callback;
  GCClosure *cc = (GCClosure*) closure;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 0);

  callback = (xsource_func_t) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (closure->data);

  xvalue_set_boolean (return_value, v_return);
}

static xboolean_t
io_watch_closure_callback (xio_channel_t   *channel,
			   xio_condition_t  condition,
			   xpointer_t      data)
{
  xclosure_t *closure = data;

  xvalue_t params[2] = { G_VALUE_INIT, G_VALUE_INIT };
  xvalue_t result_value = G_VALUE_INIT;
  xboolean_t result;

  xvalue_init (&result_value, XTYPE_BOOLEAN);
  xvalue_init (&params[0], XTYPE_IO_CHANNEL);
  xvalue_set_boxed (&params[0], channel);

  xvalue_init (&params[1], XTYPE_IO_CONDITION);
  xvalue_set_flags (&params[1], condition);

  xclosure_invoke (closure, &result_value, 2, params, NULL);

  result = xvalue_get_boolean (&result_value);
  xvalue_unset (&result_value);
  xvalue_unset (&params[0]);
  xvalue_unset (&params[1]);

  return result;
}

static xboolean_t
g_child_watch_closure_callback (xpid_t     pid,
                                xint_t     status,
                                xpointer_t data)
{
  xclosure_t *closure = data;

  xvalue_t params[2] = { G_VALUE_INIT, G_VALUE_INIT };
  xvalue_t result_value = G_VALUE_INIT;
  xboolean_t result;

  xvalue_init (&result_value, XTYPE_BOOLEAN);

#ifdef G_OS_UNIX
  xvalue_init (&params[0], XTYPE_ULONG);
  xvalue_set_ulong (&params[0], pid);
#endif
#ifdef G_OS_WIN32
  xvalue_init (&params[0], XTYPE_POINTER);
  xvalue_set_pointer (&params[0], pid);
#endif

  xvalue_init (&params[1], XTYPE_INT);
  xvalue_set_int (&params[1], status);

  xclosure_invoke (closure, &result_value, 2, params, NULL);

  result = xvalue_get_boolean (&result_value);
  xvalue_unset (&result_value);
  xvalue_unset (&params[0]);
  xvalue_unset (&params[1]);

  return result;
}

#ifdef G_OS_UNIX
static xboolean_t
g_unix_fd_source_closure_callback (int           fd,
                                   xio_condition_t  condition,
                                   xpointer_t      data)
{
  xclosure_t *closure = data;

  xvalue_t params[2] = { G_VALUE_INIT, G_VALUE_INIT };
  xvalue_t result_value = G_VALUE_INIT;
  xboolean_t result;

  xvalue_init (&result_value, XTYPE_BOOLEAN);

  xvalue_init (&params[0], XTYPE_INT);
  xvalue_set_int (&params[0], fd);

  xvalue_init (&params[1], XTYPE_IO_CONDITION);
  xvalue_set_flags (&params[1], condition);

  xclosure_invoke (closure, &result_value, 2, params, NULL);

  result = xvalue_get_boolean (&result_value);
  xvalue_unset (&result_value);
  xvalue_unset (&params[0]);
  xvalue_unset (&params[1]);

  return result;
}
#endif

static xboolean_t
source_closure_callback (xpointer_t data)
{
  xclosure_t *closure = data;
  xvalue_t result_value = G_VALUE_INIT;
  xboolean_t result;

  xvalue_init (&result_value, XTYPE_BOOLEAN);

  xclosure_invoke (closure, &result_value, 0, NULL, NULL);

  result = xvalue_get_boolean (&result_value);
  xvalue_unset (&result_value);

  return result;
}

static void
closure_callback_get (xpointer_t     cb_data,
		      xsource_t     *source,
		      xsource_func_t *func,
		      xpointer_t    *data)
{
  xsource_func_t closure_callback = source->source_funcs->closure_callback;

  if (!closure_callback)
    {
      if (source->source_funcs == &g_io_watch_funcs)
        closure_callback = (xsource_func_t)io_watch_closure_callback;
      else if (source->source_funcs == &g_child_watch_funcs)
        closure_callback = (xsource_func_t)g_child_watch_closure_callback;
#ifdef G_OS_UNIX
      else if (source->source_funcs == &g_unix_fd_source_funcs)
        closure_callback = (xsource_func_t)g_unix_fd_source_closure_callback;
#endif
      else if (source->source_funcs == &g_timeout_funcs ||
#ifdef G_OS_UNIX
               source->source_funcs == &g_unix_signal_funcs ||
#endif
               source->source_funcs == &g_idle_funcs)
        closure_callback = source_closure_callback;
    }

  *func = closure_callback;
  *data = cb_data;
}

static xsource_callback_funcs_t closure_callback_funcs = {
  (void (*) (xpointer_t)) xclosure_ref,
  (void (*) (xpointer_t)) xclosure_unref,
  closure_callback_get
};

static void
closure_invalidated (xpointer_t  user_data,
                     xclosure_t *closure)
{
  xsource_destroy (user_data);
}

/**
 * xsource_set_closure:
 * @source: the source
 * @closure: a #xclosure_t
 *
 * Set the callback for a source as a #xclosure_t.
 *
 * If the source is not one of the standard GLib types, the @closure_callback
 * and @closure_marshal fields of the #xsource_funcs_t structure must have been
 * filled in with pointers to appropriate functions.
 */
void
xsource_set_closure (xsource_t  *source,
		      xclosure_t *closure)
{
  g_return_if_fail (source != NULL);
  g_return_if_fail (closure != NULL);

  if (!source->source_funcs->closure_callback &&
#ifdef G_OS_UNIX
      source->source_funcs != &g_unix_fd_source_funcs &&
      source->source_funcs != &g_unix_signal_funcs &&
#endif
      source->source_funcs != &g_child_watch_funcs &&
      source->source_funcs != &g_io_watch_funcs &&
      source->source_funcs != &g_timeout_funcs &&
      source->source_funcs != &g_idle_funcs)
    {
      g_critical (G_STRLOC ": closure cannot be set on xsource_t without xsource_funcs_t::closure_callback");
      return;
    }

  xclosure_ref (closure);
  xclosure_sink (closure);
  xsource_set_callback_indirect (source, closure, &closure_callback_funcs);

  xclosure_add_invalidate_notifier (closure, source, closure_invalidated);

  if (G_CLOSURE_NEEDS_MARSHAL (closure))
    {
      GClosureMarshal marshal = (GClosureMarshal)source->source_funcs->closure_marshal;
      if (marshal)
	xclosure_set_marshal (closure, marshal);
      else if (source->source_funcs == &g_idle_funcs ||
#ifdef G_OS_UNIX
               source->source_funcs == &g_unix_signal_funcs ||
#endif
               source->source_funcs == &g_timeout_funcs)
	xclosure_set_marshal (closure, source_closure_marshal_BOOLEAN__VOID);
      else
        xclosure_set_marshal (closure, g_cclosure_marshal_generic);
    }
}

static void
dummy_closure_marshal (xclosure_t     *closure,
		       xvalue_t       *return_value,
		       xuint_t         n_param_values,
		       const xvalue_t *param_values,
		       xpointer_t      invocation_hint,
		       xpointer_t      marshal_data)
{
  if (G_VALUE_HOLDS_BOOLEAN (return_value))
    xvalue_set_boolean (return_value, TRUE);
}

/**
 * xsource_set_dummy_callback:
 * @source: the source
 *
 * Sets a dummy callback for @source. The callback will do nothing, and
 * if the source expects a #xboolean_t return value, it will return %TRUE.
 * (If the source expects any other type of return value, it will return
 * a 0/%NULL value; whatever xvalue_init() initializes a #xvalue_t to for
 * that type.)
 *
 * If the source is not one of the standard GLib types, the
 * @closure_callback and @closure_marshal fields of the #xsource_funcs_t
 * structure must have been filled in with pointers to appropriate
 * functions.
 */
void
xsource_set_dummy_callback (xsource_t *source)
{
  xclosure_t *closure;

  closure = xclosure_new_simple (sizeof (xclosure_t), NULL);
  xclosure_set_meta_marshal (closure, NULL, dummy_closure_marshal);
  xsource_set_closure (source, closure);
}
