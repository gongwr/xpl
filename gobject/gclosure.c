/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2000-2001 Red Hat, Inc.
 * Copyright (C) 2005 Imendio AB
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

/*
 * MT safe with regards to reference counting.
 */

#include "config.h"

#include "../glib/gvalgrind.h"
#include <string.h>

#include <ffi.h>

#include "gclosure.h"
#include "gboxed.h"
#include "gobject.h"
#include "genums.h"
#include "gvalue.h"
#include "gvaluetypes.h"
#include "gtype-private.h"


/**
 * SECTION:gclosure
 * @short_description: Functions as first-class objects
 * @title: Closures
 *
 * A #xclosure_t represents a callback supplied by the programmer.
 *
 * It will generally comprise a function of some kind and a marshaller
 * used to call it. It is the responsibility of the marshaller to
 * convert the arguments for the invocation from #GValues into
 * a suitable form, perform the callback on the converted arguments,
 * and transform the return value back into a #xvalue_t.
 *
 * In the case of C programs, a closure usually just holds a pointer
 * to a function and maybe a data argument, and the marshaller
 * converts between #xvalue_t and native C types. The xobject_t
 * library provides the #GCClosure type for this purpose. Bindings for
 * other languages need marshallers which convert between #GValues
 * and suitable representations in the runtime of the language in
 * order to use functions written in that language as callbacks. Use
 * xclosure_set_marshal() to set the marshaller on such a custom
 * closure implementation.
 *
 * Within xobject_t, closures play an important role in the
 * implementation of signals. When a signal is registered, the
 * @c_marshaller argument to xsignal_new() specifies the default C
 * marshaller for any closure which is connected to this
 * signal. xobject_t provides a number of C marshallers for this
 * purpose, see the g_cclosure_marshal_*() functions. Additional C
 * marshallers can be generated with the [glib-genmarshal][glib-genmarshal]
 * utility.  Closures can be explicitly connected to signals with
 * xsignal_connect_closure(), but it usually more convenient to let
 * xobject_t create a closure automatically by using one of the
 * xsignal_connect_*() functions which take a callback function/user
 * data pair.
 *
 * Using closures has a number of important advantages over a simple
 * callback function/data pointer combination:
 *
 * - Closures allow the callee to get the types of the callback parameters,
 *   which means that language bindings don't have to write individual glue
 *   for each callback type.
 *
 * - The reference counting of #xclosure_t makes it easy to handle reentrancy
 *   right; if a callback is removed while it is being invoked, the closure
 *   and its parameters won't be freed until the invocation finishes.
 *
 * - xclosure_invalidate() and invalidation notifiers allow callbacks to be
 *   automatically removed when the objects they point to go away.
 */

#define	CLOSURE_MAX_REF_COUNT		((1 << 15) - 1)
#define	CLOSURE_MAX_N_GUARDS		((1 << 1) - 1)
#define	CLOSURE_MAX_N_FNOTIFIERS	((1 << 2) - 1)
#define	CLOSURE_MAX_N_INOTIFIERS	((1 << 8) - 1)
#define	CLOSURE_N_MFUNCS(cl)		(((cl)->n_guards << 1L))
/* same as G_CLOSURE_N_NOTIFIERS() (keep in sync) */
#define	CLOSURE_N_NOTIFIERS(cl)		(CLOSURE_N_MFUNCS (cl) + \
                                         (cl)->n_fnotifiers + \
                                         (cl)->n_inotifiers)

typedef union {
  xclosure_t closure;
  xint_t vint;
} ClosureInt;

#define CHANGE_FIELD(_closure, _field, _OP, _value, _must_set, _SET_OLD, _SET_NEW)      \
G_STMT_START {                                                                          \
  ClosureInt *cunion = (ClosureInt*) _closure;                 		                \
  xint_t new_int, old_int, success;                              		                \
  do                                                    		                \
    {                                                   		                \
      ClosureInt tmp;                                   		                \
      tmp.vint = old_int = cunion->vint;                		                \
      _SET_OLD tmp.closure._field;                                                      \
      tmp.closure._field _OP _value;                      		                \
      _SET_NEW tmp.closure._field;                                                      \
      new_int = tmp.vint;                               		                \
      success = g_atomic_int_compare_and_exchange (&cunion->vint, old_int, new_int);    \
    }                                                   		                \
  while (!success && _must_set);                                                        \
} G_STMT_END

#define SWAP(_closure, _field, _value, _oldv)   CHANGE_FIELD (_closure, _field, =, _value, TRUE, *(_oldv) =,     (void) )
#define SET(_closure, _field, _value)           CHANGE_FIELD (_closure, _field, =, _value, TRUE,     (void),     (void) )
#define INC(_closure, _field)                   CHANGE_FIELD (_closure, _field, +=,     1, TRUE,     (void),     (void) )
#define INC_ASSIGN(_closure, _field, _newv)     CHANGE_FIELD (_closure, _field, +=,     1, TRUE,     (void), *(_newv) = )
#define DEC(_closure, _field)                   CHANGE_FIELD (_closure, _field, -=,     1, TRUE,     (void),     (void) )
#define DEC_ASSIGN(_closure, _field, _newv)     CHANGE_FIELD (_closure, _field, -=,     1, TRUE,     (void), *(_newv) = )

#if 0   /* for non-thread-safe closures */
#define SWAP(cl,f,v,o)     (void) (*(o) = cl->f, cl->f = v)
#define SET(cl,f,v)        (void) (cl->f = v)
#define INC(cl,f)          (void) (cl->f += 1)
#define INC_ASSIGN(cl,f,n) (void) (cl->f += 1, *(n) = cl->f)
#define DEC(cl,f)          (void) (cl->f -= 1)
#define DEC_ASSIGN(cl,f,n) (void) (cl->f -= 1, *(n) = cl->f)
#endif

enum {
  FNOTIFY,
  INOTIFY,
  PRE_NOTIFY,
  POST_NOTIFY
};


/* --- functions --- */
/**
 * xclosure_new_simple:
 * @sizeof_closure: the size of the structure to allocate, must be at least
 *                  `sizeof (xclosure_t)`
 * @data: data to store in the @data field of the newly allocated #xclosure_t
 *
 * Allocates a struct of the given size and initializes the initial
 * part as a #xclosure_t.
 *
 * This function is mainly useful when implementing new types of closures:
 *
 * |[<!-- language="C" -->
 * typedef struct _MyClosure MyClosure;
 * struct _MyClosure
 * {
 *   xclosure_t closure;
 *   // extra data goes here
 * };
 *
 * static void
 * my_closure_finalize (xpointer_t  notify_data,
 *                      xclosure_t *closure)
 * {
 *   MyClosure *my_closure = (MyClosure *)closure;
 *
 *   // free extra data here
 * }
 *
 * MyClosure *my_closure_new (xpointer_t data)
 * {
 *   xclosure_t *closure;
 *   MyClosure *my_closure;
 *
 *   closure = xclosure_new_simple (sizeof (MyClosure), data);
 *   my_closure = (MyClosure *) closure;
 *
 *   // initialize extra data here
 *
 *   xclosure_add_finalize_notifier (closure, notify_data,
 *                                    my_closure_finalize);
 *   return my_closure;
 * }
 * ]|
 *
 * Returns: (transfer none): a floating reference to a new #xclosure_t
 */
xclosure_t*
xclosure_new_simple (xuint_t           sizeof_closure,
		      xpointer_t        data)
{
  xclosure_t *closure;
  xint_t private_size;
  xchar_t *allocated;

  g_return_val_if_fail (sizeof_closure >= sizeof (xclosure_t), NULL);

  private_size = sizeof (GRealClosure) - sizeof (xclosure_t);

#ifdef ENABLE_VALGRIND
  /* See comments in gtype.c about what's going on here... */
  if (RUNNING_ON_VALGRIND)
    {
      private_size += sizeof (xpointer_t);

      allocated = g_malloc0 (private_size + sizeof_closure + sizeof (xpointer_t));

      *(xpointer_t *) (allocated + private_size + sizeof_closure) = allocated + sizeof (xpointer_t);

      VALGRIND_MALLOCLIKE_BLOCK (allocated + private_size, sizeof_closure + sizeof (xpointer_t), 0, TRUE);
      VALGRIND_MALLOCLIKE_BLOCK (allocated + sizeof (xpointer_t), private_size - sizeof (xpointer_t), 0, TRUE);
    }
  else
#endif
    allocated = g_malloc0 (private_size + sizeof_closure);

  closure = (xclosure_t *) (allocated + private_size);

  SET (closure, ref_count, 1);
  SET (closure, floating, TRUE);
  closure->data = data;

  return closure;
}

static inline void
closure_invoke_notifiers (xclosure_t *closure,
			  xuint_t     notify_type)
{
  /* notifier layout:
   *     n_guards    n_guards     n_fnotif.  n_inotifiers
   * ->[[pre_guards][post_guards][fnotifiers][inotifiers]]
   *
   * CLOSURE_N_MFUNCS(cl)    = n_guards + n_guards;
   * CLOSURE_N_NOTIFIERS(cl) = CLOSURE_N_MFUNCS(cl) + n_fnotifiers + n_inotifiers
   *
   * constrains/catches:
   * - closure->notifiers may be reloacted during callback
   * - closure->n_fnotifiers and closure->n_inotifiers may change during callback
   * - i.e. callbacks can be removed/added during invocation
   * - must prepare for callback removal during FNOTIFY and INOTIFY (done via ->marshal= & ->data=)
   * - must distinguish (->marshal= & ->data=) for INOTIFY vs. FNOTIFY (via ->in_inotify)
   * + closure->n_guards is const during PRE_NOTIFY & POST_NOTIFY
   * + none of the callbacks can cause recursion
   * + closure->n_inotifiers is const 0 during FNOTIFY
   */
  switch (notify_type)
    {
      xclosure_notify_data_t *ndata;
      xuint_t i, offs;
    case FNOTIFY:
      while (closure->n_fnotifiers)
	{
          xuint_t n;
	  DEC_ASSIGN (closure, n_fnotifiers, &n);

	  ndata = closure->notifiers + CLOSURE_N_MFUNCS (closure) + n;
	  closure->marshal = (GClosureMarshal) ndata->notify;
	  closure->data = ndata->data;
	  ndata->notify (ndata->data, closure);
	}
      closure->marshal = NULL;
      closure->data = NULL;
      break;
    case INOTIFY:
      SET (closure, in_inotify, TRUE);
      while (closure->n_inotifiers)
	{
          xuint_t n;
          DEC_ASSIGN (closure, n_inotifiers, &n);

	  ndata = closure->notifiers + CLOSURE_N_MFUNCS (closure) + closure->n_fnotifiers + n;
	  closure->marshal = (GClosureMarshal) ndata->notify;
	  closure->data = ndata->data;
	  ndata->notify (ndata->data, closure);
	}
      closure->marshal = NULL;
      closure->data = NULL;
      SET (closure, in_inotify, FALSE);
      break;
    case PRE_NOTIFY:
      i = closure->n_guards;
      offs = 0;
      while (i--)
	{
	  ndata = closure->notifiers + offs + i;
	  ndata->notify (ndata->data, closure);
	}
      break;
    case POST_NOTIFY:
      i = closure->n_guards;
      offs = i;
      while (i--)
	{
	  ndata = closure->notifiers + offs + i;
	  ndata->notify (ndata->data, closure);
	}
      break;
    }
}

static void
xclosure_set_meta_va_marshal (xclosure_t       *closure,
			       GVaClosureMarshal va_meta_marshal)
{
  GRealClosure *real_closure;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (va_meta_marshal != NULL);
  g_return_if_fail (closure->is_invalid == FALSE);
  g_return_if_fail (closure->in_marshal == FALSE);

  real_closure = G_REAL_CLOSURE (closure);

  g_return_if_fail (real_closure->meta_marshal != NULL);

  real_closure->va_meta_marshal = va_meta_marshal;
}

/**
 * xclosure_set_meta_marshal: (skip)
 * @closure: a #xclosure_t
 * @marshal_data: (closure meta_marshal): context-dependent data to pass
 *  to @meta_marshal
 * @meta_marshal: a #GClosureMarshal function
 *
 * Sets the meta marshaller of @closure.
 *
 * A meta marshaller wraps the @closure's marshal and modifies the way
 * it is called in some fashion. The most common use of this facility
 * is for C callbacks.
 *
 * The same marshallers (generated by [glib-genmarshal][glib-genmarshal]),
 * are used everywhere, but the way that we get the callback function
 * differs. In most cases we want to use the @closure's callback, but in
 * other cases we want to use some different technique to retrieve the
 * callback function.
 *
 * For example, class closures for signals (see
 * xsignal_type_cclosure_new()) retrieve the callback function from a
 * fixed offset in the class structure.  The meta marshaller retrieves
 * the right callback and passes it to the marshaller as the
 * @marshal_data argument.
 */
void
xclosure_set_meta_marshal (xclosure_t       *closure,
			    xpointer_t        marshal_data,
			    GClosureMarshal meta_marshal)
{
  GRealClosure *real_closure;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (meta_marshal != NULL);
  g_return_if_fail (closure->is_invalid == FALSE);
  g_return_if_fail (closure->in_marshal == FALSE);

  real_closure = G_REAL_CLOSURE (closure);

  g_return_if_fail (real_closure->meta_marshal == NULL);

  real_closure->meta_marshal = meta_marshal;
  real_closure->meta_marshal_data = marshal_data;
}

/**
 * xclosure_add_marshal_guards: (skip)
 * @closure: a #xclosure_t
 * @pre_marshal_data: (closure pre_marshal_notify): data to pass
 *  to @pre_marshal_notify
 * @pre_marshal_notify: a function to call before the closure callback
 * @post_marshal_data: (closure post_marshal_notify): data to pass
 *  to @post_marshal_notify
 * @post_marshal_notify: a function to call after the closure callback
 *
 * Adds a pair of notifiers which get invoked before and after the
 * closure callback, respectively.
 *
 * This is typically used to protect the extra arguments for the
 * duration of the callback. See xobject_watch_closure() for an
 * example of marshal guards.
 */
void
xclosure_add_marshal_guards (xclosure_t      *closure,
			      xpointer_t       pre_marshal_data,
			      xclosure_notify_t pre_marshal_notify,
			      xpointer_t       post_marshal_data,
			      xclosure_notify_t post_marshal_notify)
{
  xuint_t i;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (pre_marshal_notify != NULL);
  g_return_if_fail (post_marshal_notify != NULL);
  g_return_if_fail (closure->is_invalid == FALSE);
  g_return_if_fail (closure->in_marshal == FALSE);
  g_return_if_fail (closure->n_guards < CLOSURE_MAX_N_GUARDS);

  closure->notifiers = g_renew (xclosure_notify_data_t, closure->notifiers, CLOSURE_N_NOTIFIERS (closure) + 2);
  if (closure->n_inotifiers)
    closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
			closure->n_fnotifiers +
			closure->n_inotifiers + 1)] = closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
									  closure->n_fnotifiers + 0)];
  if (closure->n_inotifiers > 1)
    closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
			closure->n_fnotifiers +
			closure->n_inotifiers)] = closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
								      closure->n_fnotifiers + 1)];
  if (closure->n_fnotifiers)
    closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
			closure->n_fnotifiers + 1)] = closure->notifiers[CLOSURE_N_MFUNCS (closure) + 0];
  if (closure->n_fnotifiers > 1)
    closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
			closure->n_fnotifiers)] = closure->notifiers[CLOSURE_N_MFUNCS (closure) + 1];
  if (closure->n_guards)
    closure->notifiers[(closure->n_guards +
			closure->n_guards + 1)] = closure->notifiers[closure->n_guards];
  i = closure->n_guards;
  closure->notifiers[i].data = pre_marshal_data;
  closure->notifiers[i].notify = pre_marshal_notify;
  closure->notifiers[i + 1].data = post_marshal_data;
  closure->notifiers[i + 1].notify = post_marshal_notify;
  INC (closure, n_guards);
}

/**
 * xclosure_add_finalize_notifier: (skip)
 * @closure: a #xclosure_t
 * @notify_data: (closure notify_func): data to pass to @notify_func
 * @notify_func: the callback function to register
 *
 * Registers a finalization notifier which will be called when the
 * reference count of @closure goes down to 0.
 *
 * Multiple finalization notifiers on a single closure are invoked in
 * unspecified order. If a single call to xclosure_unref() results in
 * the closure being both invalidated and finalized, then the invalidate
 * notifiers will be run before the finalize notifiers.
 */
void
xclosure_add_finalize_notifier (xclosure_t      *closure,
				 xpointer_t       notify_data,
				 xclosure_notify_t notify_func)
{
  xuint_t i;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (notify_func != NULL);
  g_return_if_fail (closure->n_fnotifiers < CLOSURE_MAX_N_FNOTIFIERS);

  closure->notifiers = g_renew (xclosure_notify_data_t, closure->notifiers, CLOSURE_N_NOTIFIERS (closure) + 1);
  if (closure->n_inotifiers)
    closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
			closure->n_fnotifiers +
			closure->n_inotifiers)] = closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
								      closure->n_fnotifiers + 0)];
  i = CLOSURE_N_MFUNCS (closure) + closure->n_fnotifiers;
  closure->notifiers[i].data = notify_data;
  closure->notifiers[i].notify = notify_func;
  INC (closure, n_fnotifiers);
}

/**
 * xclosure_add_invalidate_notifier: (skip)
 * @closure: a #xclosure_t
 * @notify_data: (closure notify_func): data to pass to @notify_func
 * @notify_func: the callback function to register
 *
 * Registers an invalidation notifier which will be called when the
 * @closure is invalidated with xclosure_invalidate().
 *
 * Invalidation notifiers are invoked before finalization notifiers,
 * in an unspecified order.
 */
void
xclosure_add_invalidate_notifier (xclosure_t      *closure,
				   xpointer_t       notify_data,
				   xclosure_notify_t notify_func)
{
  xuint_t i;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (notify_func != NULL);
  g_return_if_fail (closure->is_invalid == FALSE);
  g_return_if_fail (closure->n_inotifiers < CLOSURE_MAX_N_INOTIFIERS);

  closure->notifiers = g_renew (xclosure_notify_data_t, closure->notifiers, CLOSURE_N_NOTIFIERS (closure) + 1);
  i = CLOSURE_N_MFUNCS (closure) + closure->n_fnotifiers + closure->n_inotifiers;
  closure->notifiers[i].data = notify_data;
  closure->notifiers[i].notify = notify_func;
  INC (closure, n_inotifiers);
}

static inline xboolean_t
closure_try_remove_inotify (xclosure_t       *closure,
			    xpointer_t       notify_data,
			    xclosure_notify_t notify_func)
{
  xclosure_notify_data_t *ndata, *nlast;

  nlast = closure->notifiers + CLOSURE_N_NOTIFIERS (closure) - 1;
  for (ndata = nlast + 1 - closure->n_inotifiers; ndata <= nlast; ndata++)
    if (ndata->notify == notify_func && ndata->data == notify_data)
      {
	DEC (closure, n_inotifiers);
	if (ndata < nlast)
	  *ndata = *nlast;

	return TRUE;
      }
  return FALSE;
}

static inline xboolean_t
closure_try_remove_fnotify (xclosure_t       *closure,
			    xpointer_t       notify_data,
			    xclosure_notify_t notify_func)
{
  xclosure_notify_data_t *ndata, *nlast;

  nlast = closure->notifiers + CLOSURE_N_NOTIFIERS (closure) - closure->n_inotifiers - 1;
  for (ndata = nlast + 1 - closure->n_fnotifiers; ndata <= nlast; ndata++)
    if (ndata->notify == notify_func && ndata->data == notify_data)
      {
	DEC (closure, n_fnotifiers);
	if (ndata < nlast)
	  *ndata = *nlast;
	if (closure->n_inotifiers)
	  closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
			      closure->n_fnotifiers)] = closure->notifiers[(CLOSURE_N_MFUNCS (closure) +
									    closure->n_fnotifiers +
									    closure->n_inotifiers)];
	return TRUE;
      }
  return FALSE;
}

/**
 * xclosure_ref:
 * @closure: #xclosure_t to increment the reference count on
 *
 * Increments the reference count on a closure to force it staying
 * alive while the caller holds a pointer to it.
 *
 * Returns: (transfer none): The @closure passed in, for convenience
 */
xclosure_t*
xclosure_ref (xclosure_t *closure)
{
  xuint_t new_ref_count;
  g_return_val_if_fail (closure != NULL, NULL);
  g_return_val_if_fail (closure->ref_count > 0, NULL);
  g_return_val_if_fail (closure->ref_count < CLOSURE_MAX_REF_COUNT, NULL);

  INC_ASSIGN (closure, ref_count, &new_ref_count);
  g_return_val_if_fail (new_ref_count > 1, NULL);

  return closure;
}

/**
 * xclosure_invalidate:
 * @closure: #xclosure_t to invalidate
 *
 * Sets a flag on the closure to indicate that its calling
 * environment has become invalid, and thus causes any future
 * invocations of xclosure_invoke() on this @closure to be
 * ignored.
 *
 * Also, invalidation notifiers installed on the closure will
 * be called at this point. Note that unless you are holding a
 * reference to the closure yourself, the invalidation notifiers may
 * unref the closure and cause it to be destroyed, so if you need to
 * access the closure after calling xclosure_invalidate(), make sure
 * that you've previously called xclosure_ref().
 *
 * Note that xclosure_invalidate() will also be called when the
 * reference count of a closure drops to zero (unless it has already
 * been invalidated before).
 */
void
xclosure_invalidate (xclosure_t *closure)
{
  g_return_if_fail (closure != NULL);

  if (!closure->is_invalid)
    {
      xboolean_t was_invalid;
      xclosure_ref (closure);           /* preserve floating flag */
      SWAP (closure, is_invalid, TRUE, &was_invalid);
      /* invalidate only once */
      if (!was_invalid)
        closure_invoke_notifiers (closure, INOTIFY);
      xclosure_unref (closure);
    }
}

/**
 * xclosure_unref:
 * @closure: #xclosure_t to decrement the reference count on
 *
 * Decrements the reference count of a closure after it was previously
 * incremented by the same caller.
 *
 * If no other callers are using the closure, then the closure will be
 * destroyed and freed.
 */
void
xclosure_unref (xclosure_t *closure)
{
  xuint_t new_ref_count;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (closure->ref_count > 0);

  if (closure->ref_count == 1)	/* last unref, invalidate first */
    xclosure_invalidate (closure);

  DEC_ASSIGN (closure, ref_count, &new_ref_count);

  if (new_ref_count == 0)
    {
      closure_invoke_notifiers (closure, FNOTIFY);
      g_free (closure->notifiers);

#ifdef ENABLE_VALGRIND
      /* See comments in gtype.c about what's going on here... */
      if (RUNNING_ON_VALGRIND)
        {
          xchar_t *allocated;

          allocated = (xchar_t *) G_REAL_CLOSURE (closure);
          allocated -= sizeof (xpointer_t);

          g_free (allocated);

          VALGRIND_FREELIKE_BLOCK (allocated + sizeof (xpointer_t), 0);
          VALGRIND_FREELIKE_BLOCK (closure, 0);
        }
      else
#endif
        g_free (G_REAL_CLOSURE (closure));
    }
}

/**
 * xclosure_sink:
 * @closure: #xclosure_t to decrement the initial reference count on, if it's
 *           still being held
 *
 * Takes over the initial ownership of a closure.
 *
 * Each closure is initially created in a "floating" state, which means
 * that the initial reference count is not owned by any caller.
 *
 * This function checks to see if the object is still floating, and if so,
 * unsets the floating state and decreases the reference count. If the
 * closure is not floating, xclosure_sink() does nothing.
 *
 * The reason for the existence of the floating state is to prevent
 * cumbersome code sequences like:
 *
 * |[<!-- language="C" -->
 * closure = g_cclosure_new (cb_func, cb_data);
 * xsource_set_closure (source, closure);
 * xclosure_unref (closure); // xobject_t doesn't really need this
 * ]|
 *
 * Because xsource_set_closure() (and similar functions) take ownership of the
 * initial reference count, if it is unowned, we instead can write:
 *
 * |[<!-- language="C" -->
 * xsource_set_closure (source, g_cclosure_new (cb_func, cb_data));
 * ]|
 *
 * Generally, this function is used together with xclosure_ref(). An example
 * of storing a closure for later notification looks like:
 *
 * |[<!-- language="C" -->
 * static xclosure_t *notify_closure = NULL;
 * void
 * foo_notify_set_closure (xclosure_t *closure)
 * {
 *   if (notify_closure)
 *     xclosure_unref (notify_closure);
 *   notify_closure = closure;
 *   if (notify_closure)
 *     {
 *       xclosure_ref (notify_closure);
 *       xclosure_sink (notify_closure);
 *     }
 * }
 * ]|
 *
 * Because xclosure_sink() may decrement the reference count of a closure
 * (if it hasn't been called on @closure yet) just like xclosure_unref(),
 * xclosure_ref() should be called prior to this function.
 */
void
xclosure_sink (xclosure_t *closure)
{
  g_return_if_fail (closure != NULL);
  g_return_if_fail (closure->ref_count > 0);

  /* floating is basically a kludge to avoid creating closures
   * with a ref_count of 0. so the initial ref_count a closure has
   * is unowned. with invoking xclosure_sink() code may
   * indicate that it takes over that initial ref_count.
   */
  if (closure->floating)
    {
      xboolean_t was_floating;
      SWAP (closure, floating, FALSE, &was_floating);
      /* unref floating flag only once */
      if (was_floating)
        xclosure_unref (closure);
    }
}

/**
 * xclosure_remove_invalidate_notifier: (skip)
 * @closure: a #xclosure_t
 * @notify_data: data which was passed to xclosure_add_invalidate_notifier()
 *               when registering @notify_func
 * @notify_func: the callback function to remove
 *
 * Removes an invalidation notifier.
 *
 * Notice that notifiers are automatically removed after they are run.
 */
void
xclosure_remove_invalidate_notifier (xclosure_t      *closure,
				      xpointer_t       notify_data,
				      xclosure_notify_t notify_func)
{
  g_return_if_fail (closure != NULL);
  g_return_if_fail (notify_func != NULL);

  if (closure->is_invalid && closure->in_inotify && /* account removal of notify_func() while it's called */
      ((xpointer_t) closure->marshal) == ((xpointer_t) notify_func) &&
      closure->data == notify_data)
    closure->marshal = NULL;
  else if (!closure_try_remove_inotify (closure, notify_data, notify_func))
    g_warning (G_STRLOC ": unable to remove uninstalled invalidation notifier: %p (%p)",
	       notify_func, notify_data);
}

/**
 * xclosure_remove_finalize_notifier: (skip)
 * @closure: a #xclosure_t
 * @notify_data: data which was passed to xclosure_add_finalize_notifier()
 *  when registering @notify_func
 * @notify_func: the callback function to remove
 *
 * Removes a finalization notifier.
 *
 * Notice that notifiers are automatically removed after they are run.
 */
void
xclosure_remove_finalize_notifier (xclosure_t      *closure,
				    xpointer_t       notify_data,
				    xclosure_notify_t notify_func)
{
  g_return_if_fail (closure != NULL);
  g_return_if_fail (notify_func != NULL);

  if (closure->is_invalid && !closure->in_inotify && /* account removal of notify_func() while it's called */
      ((xpointer_t) closure->marshal) == ((xpointer_t) notify_func) &&
      closure->data == notify_data)
    closure->marshal = NULL;
  else if (!closure_try_remove_fnotify (closure, notify_data, notify_func))
    g_warning (G_STRLOC ": unable to remove uninstalled finalization notifier: %p (%p)",
               notify_func, notify_data);
}

/**
 * xclosure_invoke:
 * @closure: a #xclosure_t
 * @return_value: (optional) (out): a #xvalue_t to store the return
 *                value. May be %NULL if the callback of @closure
 *                doesn't return a value.
 * @n_param_values: the length of the @param_values array
 * @param_values: (array length=n_param_values): an array of
 *                #GValues holding the arguments on which to
 *                invoke the callback of @closure
 * @invocation_hint: (nullable): a context-dependent invocation hint
 *
 * Invokes the closure, i.e. executes the callback represented by the @closure.
 */
void
xclosure_invoke (xclosure_t       *closure,
		  xvalue_t /*out*/ *return_value,
		  xuint_t           n_param_values,
		  const xvalue_t   *param_values,
		  xpointer_t        invocation_hint)
{
  GRealClosure *real_closure;

  g_return_if_fail (closure != NULL);

  real_closure = G_REAL_CLOSURE (closure);

  xclosure_ref (closure);      /* preserve floating flag */
  if (!closure->is_invalid)
    {
      GClosureMarshal marshal;
      xpointer_t marshal_data;
      xboolean_t in_marshal = closure->in_marshal;

      g_return_if_fail (closure->marshal || real_closure->meta_marshal);

      SET (closure, in_marshal, TRUE);
      if (real_closure->meta_marshal)
	{
	  marshal_data = real_closure->meta_marshal_data;
	  marshal = real_closure->meta_marshal;
	}
      else
	{
	  marshal_data = NULL;
	  marshal = closure->marshal;
	}
      if (!in_marshal)
	closure_invoke_notifiers (closure, PRE_NOTIFY);
      marshal (closure,
	       return_value,
	       n_param_values, param_values,
	       invocation_hint,
	       marshal_data);
      if (!in_marshal)
	closure_invoke_notifiers (closure, POST_NOTIFY);
      SET (closure, in_marshal, in_marshal);
    }
  xclosure_unref (closure);
}

xboolean_t
_xclosure_supports_invoke_va (xclosure_t       *closure)
{
  GRealClosure *real_closure;

  g_return_val_if_fail (closure != NULL, FALSE);

  real_closure = G_REAL_CLOSURE (closure);

  return
    real_closure->va_marshal != NULL &&
    (real_closure->meta_marshal == NULL ||
     real_closure->va_meta_marshal != NULL);
}

void
_xclosure_invoke_va (xclosure_t       *closure,
		      xvalue_t /*out*/ *return_value,
		      xpointer_t        instance,
		      va_list         args,
		      int             n_params,
		      xtype_t          *param_types)
{
  GRealClosure *real_closure;

  g_return_if_fail (closure != NULL);

  real_closure = G_REAL_CLOSURE (closure);

  xclosure_ref (closure);      /* preserve floating flag */
  if (!closure->is_invalid)
    {
      GVaClosureMarshal marshal;
      xpointer_t marshal_data;
      xboolean_t in_marshal = closure->in_marshal;

      g_return_if_fail (closure->marshal || real_closure->meta_marshal);

      SET (closure, in_marshal, TRUE);
      if (real_closure->va_meta_marshal)
	{
	  marshal_data = real_closure->meta_marshal_data;
	  marshal = real_closure->va_meta_marshal;
	}
      else
	{
	  marshal_data = NULL;
	  marshal = real_closure->va_marshal;
	}
      if (!in_marshal)
	closure_invoke_notifiers (closure, PRE_NOTIFY);
      marshal (closure,
	       return_value,
	       instance, args,
	       marshal_data,
	       n_params, param_types);
      if (!in_marshal)
	closure_invoke_notifiers (closure, POST_NOTIFY);
      SET (closure, in_marshal, in_marshal);
    }
  xclosure_unref (closure);
}


/**
 * xclosure_set_marshal: (skip)
 * @closure: a #xclosure_t
 * @marshal: a #GClosureMarshal function
 *
 * Sets the marshaller of @closure.
 *
 * The `marshal_data` of @marshal provides a way for a meta marshaller to
 * provide additional information to the marshaller.
 *
 * For xobject_t's C predefined marshallers (the `g_cclosure_marshal_*()`
 * functions), what it provides is a callback function to use instead of
 * @closure->callback.
 *
 * See also: xclosure_set_meta_marshal()
 */
void
xclosure_set_marshal (xclosure_t       *closure,
		       GClosureMarshal marshal)
{
  g_return_if_fail (closure != NULL);
  g_return_if_fail (marshal != NULL);

  if (closure->marshal && closure->marshal != marshal)
    g_warning ("attempt to override closure->marshal (%p) with new marshal (%p)",
	       closure->marshal, marshal);
  else
    closure->marshal = marshal;
}

void
_xclosure_set_va_marshal (xclosure_t       *closure,
			   GVaClosureMarshal marshal)
{
  GRealClosure *real_closure;

  g_return_if_fail (closure != NULL);
  g_return_if_fail (marshal != NULL);

  real_closure = G_REAL_CLOSURE (closure);

  if (real_closure->va_marshal && real_closure->va_marshal != marshal)
    g_warning ("attempt to override closure->va_marshal (%p) with new marshal (%p)",
	       real_closure->va_marshal, marshal);
  else
    real_closure->va_marshal = marshal;
}

/**
 * g_cclosure_new: (skip)
 * @callback_func: the function to invoke
 * @user_data: (closure callback_func): user data to pass to @callback_func
 * @destroy_data: destroy notify to be called when @user_data is no longer used
 *
 * Creates a new closure which invokes @callback_func with @user_data as
 * the last parameter.
 *
 * @destroy_data will be called as a finalize notifier on the #xclosure_t.
 *
 * Returns: (transfer none): a floating reference to a new #GCClosure
 */
xclosure_t*
g_cclosure_new (xcallback_t      callback_func,
		xpointer_t       user_data,
		xclosure_notify_t destroy_data)
{
  xclosure_t *closure;

  g_return_val_if_fail (callback_func != NULL, NULL);

  closure = xclosure_new_simple (sizeof (GCClosure), user_data);
  if (destroy_data)
    xclosure_add_finalize_notifier (closure, user_data, destroy_data);
  ((GCClosure*) closure)->callback = (xpointer_t) callback_func;

  return closure;
}

/**
 * g_cclosure_new_swap: (skip)
 * @callback_func: the function to invoke
 * @user_data: (closure callback_func): user data to pass to @callback_func
 * @destroy_data: destroy notify to be called when @user_data is no longer used
 *
 * Creates a new closure which invokes @callback_func with @user_data as
 * the first parameter.
 *
 * @destroy_data will be called as a finalize notifier on the #xclosure_t.
 *
 * Returns: (transfer none): a floating reference to a new #GCClosure
 */
xclosure_t*
g_cclosure_new_swap (xcallback_t      callback_func,
		     xpointer_t       user_data,
		     xclosure_notify_t destroy_data)
{
  xclosure_t *closure;

  g_return_val_if_fail (callback_func != NULL, NULL);

  closure = xclosure_new_simple (sizeof (GCClosure), user_data);
  if (destroy_data)
    xclosure_add_finalize_notifier (closure, user_data, destroy_data);
  ((GCClosure*) closure)->callback = (xpointer_t) callback_func;
  SET (closure, derivative_flag, TRUE);

  return closure;
}

static void
xtype_class_meta_marshal (xclosure_t       *closure,
			   xvalue_t /*out*/ *return_value,
			   xuint_t           n_param_values,
			   const xvalue_t   *param_values,
			   xpointer_t        invocation_hint,
			   xpointer_t        marshal_data)
{
  xtype_class_t *class;
  xpointer_t callback;
  /* xtype_t itype = (xtype_t) closure->data; */
  xuint_t offset = GPOINTER_TO_UINT (marshal_data);

  class = XTYPE_INSTANCE_GET_CLASS (xvalue_peek_pointer (param_values + 0), itype, xtype_class_t);
  callback = G_STRUCT_MEMBER (xpointer_t, class, offset);
  if (callback)
    closure->marshal (closure,
		      return_value,
		      n_param_values, param_values,
		      invocation_hint,
		      callback);
}

static void
xtype_class_meta_marshalv (xclosure_t *closure,
			    xvalue_t   *return_value,
			    xpointer_t  instance,
			    va_list   args,
			    xpointer_t  marshal_data,
			    int       n_params,
			    xtype_t    *param_types)
{
  GRealClosure *real_closure;
  xtype_class_t *class;
  xpointer_t callback;
  /* xtype_t itype = (xtype_t) closure->data; */
  xuint_t offset = GPOINTER_TO_UINT (marshal_data);

  real_closure = G_REAL_CLOSURE (closure);

  class = XTYPE_INSTANCE_GET_CLASS (instance, itype, xtype_class_t);
  callback = G_STRUCT_MEMBER (xpointer_t, class, offset);
  if (callback)
    real_closure->va_marshal (closure,
			      return_value,
			      instance, args,
			      callback,
			      n_params,
			      param_types);
}

static void
xtype_iface_meta_marshal (xclosure_t       *closure,
			   xvalue_t /*out*/ *return_value,
			   xuint_t           n_param_values,
			   const xvalue_t   *param_values,
			   xpointer_t        invocation_hint,
			   xpointer_t        marshal_data)
{
  xtype_class_t *class;
  xpointer_t callback;
  xtype_t itype = (xtype_t) closure->data;
  xuint_t offset = GPOINTER_TO_UINT (marshal_data);

  class = XTYPE_INSTANCE_GET_INTERFACE (xvalue_peek_pointer (param_values + 0), itype, xtype_class_t);
  callback = G_STRUCT_MEMBER (xpointer_t, class, offset);
  if (callback)
    closure->marshal (closure,
		      return_value,
		      n_param_values, param_values,
		      invocation_hint,
		      callback);
}

xboolean_t
_xclosure_is_void (xclosure_t *closure,
		    xpointer_t instance)
{
  GRealClosure *real_closure;
  xtype_class_t *class;
  xpointer_t callback;
  xtype_t itype;
  xuint_t offset;

  if (closure->is_invalid)
    return TRUE;

  real_closure = G_REAL_CLOSURE (closure);

  if (real_closure->meta_marshal == xtype_iface_meta_marshal)
    {
      itype = (xtype_t) closure->data;
      offset = GPOINTER_TO_UINT (real_closure->meta_marshal_data);

      class = XTYPE_INSTANCE_GET_INTERFACE (instance, itype, xtype_class_t);
      callback = G_STRUCT_MEMBER (xpointer_t, class, offset);
      return callback == NULL;
    }
  else if (real_closure->meta_marshal == xtype_class_meta_marshal)
    {
      offset = GPOINTER_TO_UINT (real_closure->meta_marshal_data);

      class = XTYPE_INSTANCE_GET_CLASS (instance, itype, xtype_class_t);
      callback = G_STRUCT_MEMBER (xpointer_t, class, offset);
      return callback == NULL;
    }

  return FALSE;
}

static void
xtype_iface_meta_marshalv (xclosure_t *closure,
			    xvalue_t   *return_value,
			    xpointer_t  instance,
			    va_list   args,
			    xpointer_t  marshal_data,
			    int       n_params,
			    xtype_t    *param_types)
{
  GRealClosure *real_closure;
  xtype_class_t *class;
  xpointer_t callback;
  xtype_t itype = (xtype_t) closure->data;
  xuint_t offset = GPOINTER_TO_UINT (marshal_data);

  real_closure = G_REAL_CLOSURE (closure);

  class = XTYPE_INSTANCE_GET_INTERFACE (instance, itype, xtype_class_t);
  callback = G_STRUCT_MEMBER (xpointer_t, class, offset);
  if (callback)
    real_closure->va_marshal (closure,
			      return_value,
			      instance, args,
			      callback,
			      n_params,
			      param_types);
}

/**
 * xsignal_type_cclosure_new:
 * @itype: the #xtype_t identifier of an interface or classed type
 * @struct_offset: the offset of the member function of @itype's class
 *  structure which is to be invoked by the new closure
 *
 * Creates a new closure which invokes the function found at the offset
 * @struct_offset in the class structure of the interface or classed type
 * identified by @itype.
 *
 * Returns: (transfer none): a floating reference to a new #GCClosure
 */
xclosure_t*
xsignal_type_cclosure_new (xtype_t    itype,
			    xuint_t    struct_offset)
{
  xclosure_t *closure;

  g_return_val_if_fail (XTYPE_IS_CLASSED (itype) || XTYPE_IS_INTERFACE (itype), NULL);
  g_return_val_if_fail (struct_offset >= sizeof (xtype_class_t), NULL);

  closure = xclosure_new_simple (sizeof (xclosure_t), (xpointer_t) itype);
  if (XTYPE_IS_INTERFACE (itype))
    {
      xclosure_set_meta_marshal (closure, GUINT_TO_POINTER (struct_offset), xtype_iface_meta_marshal);
      xclosure_set_meta_va_marshal (closure, xtype_iface_meta_marshalv);
    }
  else
    {
      xclosure_set_meta_marshal (closure, GUINT_TO_POINTER (struct_offset), xtype_class_meta_marshal);
      xclosure_set_meta_va_marshal (closure, xtype_class_meta_marshalv);
    }
  return closure;
}

#include <ffi.h>
static ffi_type *
value_to_ffi_type (const xvalue_t *gvalue,
                   xpointer_t *value,
                   xint_t *enum_tmpval,
                   xboolean_t *tmpval_used)
{
  ffi_type *rettype = NULL;
  xtype_t type = xtype_fundamental (G_VALUE_TYPE (gvalue));
  g_assert (type != XTYPE_INVALID);

  if (enum_tmpval)
    {
      g_assert (tmpval_used != NULL);
      *tmpval_used = FALSE;
    }

  switch (type)
    {
    case XTYPE_BOOLEAN:
    case XTYPE_CHAR:
    case XTYPE_INT:
      rettype = &ffi_type_sint;
      *value = (xpointer_t)&(gvalue->data[0].v_int);
      break;
    case XTYPE_ENUM:
      /* enums are stored in v_long even though they are integers, which makes
       * marshalling through libffi somewhat complicated.  They need to be
       * marshalled as signed ints, but we need to use a temporary int sized
       * value to pass to libffi otherwise it'll pull the wrong value on
       * BE machines with 32-bit integers when treating v_long as 32-bit int.
       */
      g_assert (enum_tmpval != NULL);
      rettype = &ffi_type_sint;
      *enum_tmpval = xvalue_get_enum (gvalue);
      *value = enum_tmpval;
      *tmpval_used = TRUE;
      break;
    case XTYPE_FLAGS:
      g_assert (enum_tmpval != NULL);
      rettype = &ffi_type_uint;
      *enum_tmpval = xvalue_get_flags (gvalue);
      *value = enum_tmpval;
      *tmpval_used = TRUE;
      break;
    case XTYPE_UCHAR:
    case XTYPE_UINT:
      rettype = &ffi_type_uint;
      *value = (xpointer_t)&(gvalue->data[0].v_uint);
      break;
    case XTYPE_STRING:
    case XTYPE_OBJECT:
    case XTYPE_BOXED:
    case XTYPE_PARAM:
    case XTYPE_POINTER:
    case XTYPE_INTERFACE:
    case XTYPE_VARIANT:
      rettype = &ffi_type_pointer;
      *value = (xpointer_t)&(gvalue->data[0].v_pointer);
      break;
    case XTYPE_FLOAT:
      rettype = &ffi_type_float;
      *value = (xpointer_t)&(gvalue->data[0].v_float);
      break;
    case XTYPE_DOUBLE:
      rettype = &ffi_type_double;
      *value = (xpointer_t)&(gvalue->data[0].v_double);
      break;
    case XTYPE_LONG:
      rettype = &ffi_type_slong;
      *value = (xpointer_t)&(gvalue->data[0].v_long);
      break;
    case XTYPE_ULONG:
      rettype = &ffi_type_ulong;
      *value = (xpointer_t)&(gvalue->data[0].v_ulong);
      break;
    case XTYPE_INT64:
      rettype = &ffi_type_sint64;
      *value = (xpointer_t)&(gvalue->data[0].v_int64);
      break;
    case XTYPE_UINT64:
      rettype = &ffi_type_uint64;
      *value = (xpointer_t)&(gvalue->data[0].v_uint64);
      break;
    default:
      rettype = &ffi_type_pointer;
      *value = NULL;
      g_warning ("value_to_ffi_type: Unsupported fundamental type: %s", xtype_name (type));
      break;
    }
  return rettype;
}

static void
value_from_ffi_type (xvalue_t *gvalue, xpointer_t *value)
{
  ffi_arg *int_val = (ffi_arg*) value;
  xtype_t type;

  type = G_VALUE_TYPE (gvalue);

restart:
  switch (xtype_fundamental (type))
    {
    case XTYPE_INT:
      xvalue_set_int (gvalue, (xint_t) *int_val);
      break;
    case XTYPE_FLOAT:
      xvalue_set_float (gvalue, *(gfloat*)value);
      break;
    case XTYPE_DOUBLE:
      xvalue_set_double (gvalue, *(xdouble_t*)value);
      break;
    case XTYPE_BOOLEAN:
      xvalue_set_boolean (gvalue, (xboolean_t) *int_val);
      break;
    case XTYPE_STRING:
      xvalue_take_string (gvalue, *(xchar_t**)value);
      break;
    case XTYPE_CHAR:
      xvalue_set_schar (gvalue, (gint8) *int_val);
      break;
    case XTYPE_UCHAR:
      xvalue_set_uchar (gvalue, (xuchar_t) *int_val);
      break;
    case XTYPE_UINT:
      xvalue_set_uint (gvalue, (xuint_t) *int_val);
      break;
    case XTYPE_POINTER:
      xvalue_set_pointer (gvalue, *(xpointer_t*)value);
      break;
    case XTYPE_LONG:
      xvalue_set_long (gvalue, (xlong_t) *int_val);
      break;
    case XTYPE_ULONG:
      xvalue_set_ulong (gvalue, (xulong_t) *int_val);
      break;
    case XTYPE_INT64:
      xvalue_set_int64 (gvalue, (sint64_t) *int_val);
      break;
    case XTYPE_UINT64:
      xvalue_set_uint64 (gvalue, (xuint64_t) *int_val);
      break;
    case XTYPE_BOXED:
      xvalue_take_boxed (gvalue, *(xpointer_t*)value);
      break;
    case XTYPE_ENUM:
      xvalue_set_enum (gvalue, (xint_t) *int_val);
      break;
    case XTYPE_FLAGS:
      xvalue_set_flags (gvalue, (xuint_t) *int_val);
      break;
    case XTYPE_PARAM:
      xvalue_take_param (gvalue, *(xpointer_t*)value);
      break;
    case XTYPE_OBJECT:
      xvalue_take_object (gvalue, *(xpointer_t*)value);
      break;
    case XTYPE_VARIANT:
      xvalue_take_variant (gvalue, *(xpointer_t*)value);
      break;
    case XTYPE_INTERFACE:
      type = xtype_interface_instantiatable_prerequisite (type);
      if (type)
        goto restart;
      G_GNUC_FALLTHROUGH;
    default:
      g_warning ("value_from_ffi_type: Unsupported fundamental type %s for type %s",
                 xtype_name (xtype_fundamental (G_VALUE_TYPE (gvalue))),
                 xtype_name (G_VALUE_TYPE (gvalue)));
    }
}

typedef union {
  xpointer_t _gpointer;
  float _float;
  double _double;
  xint_t _gint;
  xuint_t _guint;
  xlong_t _glong;
  xulong_t _gulong;
  sint64_t _gint64;
  xuint64_t _guint64;
} va_arg_storage;

static ffi_type *
va_to_ffi_type (xtype_t gtype,
		va_list *va,
		va_arg_storage *storage)
{
  ffi_type *rettype = NULL;
  xtype_t type = xtype_fundamental (gtype);
  g_assert (type != XTYPE_INVALID);

  switch (type)
    {
    case XTYPE_BOOLEAN:
    case XTYPE_CHAR:
    case XTYPE_INT:
    case XTYPE_ENUM:
      rettype = &ffi_type_sint;
      storage->_gint = va_arg (*va, xint_t);
      break;
    case XTYPE_UCHAR:
    case XTYPE_UINT:
    case XTYPE_FLAGS:
      rettype = &ffi_type_uint;
      storage->_guint = va_arg (*va, xuint_t);
      break;
    case XTYPE_STRING:
    case XTYPE_OBJECT:
    case XTYPE_BOXED:
    case XTYPE_PARAM:
    case XTYPE_POINTER:
    case XTYPE_INTERFACE:
    case XTYPE_VARIANT:
      rettype = &ffi_type_pointer;
      storage->_gpointer = va_arg (*va, xpointer_t);
      break;
    case XTYPE_FLOAT:
      /* Float args are passed as doubles in varargs */
      rettype = &ffi_type_float;
      storage->_float = (float)va_arg (*va, double);
      break;
    case XTYPE_DOUBLE:
      rettype = &ffi_type_double;
      storage->_double = va_arg (*va, double);
      break;
    case XTYPE_LONG:
      rettype = &ffi_type_slong;
      storage->_glong = va_arg (*va, xlong_t);
      break;
    case XTYPE_ULONG:
      rettype = &ffi_type_ulong;
      storage->_gulong = va_arg (*va, xulong_t);
      break;
    case XTYPE_INT64:
      rettype = &ffi_type_sint64;
      storage->_gint64 = va_arg (*va, sint64_t);
      break;
    case XTYPE_UINT64:
      rettype = &ffi_type_uint64;
      storage->_guint64 = va_arg (*va, xuint64_t);
      break;
    default:
      rettype = &ffi_type_pointer;
      storage->_guint64  = 0;
      g_warning ("va_to_ffi_type: Unsupported fundamental type: %s", xtype_name (type));
      break;
    }
  return rettype;
}

/**
 * g_cclosure_marshal_generic:
 * @closure: A #xclosure_t.
 * @return_gvalue: A #xvalue_t to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   xclosure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see xclosure_set_marshal() and
 *   xclosure_set_meta_marshal()
 *
 * A generic marshaller function implemented via
 * [libffi](http://sourceware.org/libffi/).
 *
 * Normally this function is not passed explicitly to xsignal_new(),
 * but used automatically by GLib when specifying a %NULL marshaller.
 *
 * Since: 2.30
 */
void
g_cclosure_marshal_generic (xclosure_t     *closure,
                            xvalue_t       *return_gvalue,
                            xuint_t         n_param_values,
                            const xvalue_t *param_values,
                            xpointer_t      invocation_hint,
                            xpointer_t      marshal_data)
{
  ffi_type *rtype;
  void *rvalue;
  int n_args;
  ffi_type **atypes;
  void **args;
  int i;
  ffi_cif cif;
  GCClosure *cc = (GCClosure*) closure;
  xint_t *enum_tmpval;
  xboolean_t tmpval_used = FALSE;

  enum_tmpval = g_alloca (sizeof (xint_t));
  if (return_gvalue && G_VALUE_TYPE (return_gvalue))
    {
      rtype = value_to_ffi_type (return_gvalue, &rvalue, enum_tmpval, &tmpval_used);
    }
  else
    {
      rtype = &ffi_type_void;
    }

  rvalue = g_alloca (MAX (rtype->size, sizeof (ffi_arg)));

  n_args = n_param_values + 1;
  atypes = g_alloca (sizeof (ffi_type *) * n_args);
  args =  g_alloca (sizeof (xpointer_t) * n_args);

  if (tmpval_used)
    enum_tmpval = g_alloca (sizeof (xint_t));

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      atypes[n_args-1] = value_to_ffi_type (param_values + 0,
                                            &args[n_args-1],
                                            enum_tmpval,
                                            &tmpval_used);
      atypes[0] = &ffi_type_pointer;
      args[0] = &closure->data;
    }
  else
    {
      atypes[0] = value_to_ffi_type (param_values + 0,
                                     &args[0],
                                     enum_tmpval,
                                     &tmpval_used);
      atypes[n_args-1] = &ffi_type_pointer;
      args[n_args-1] = &closure->data;
    }

  for (i = 1; i < n_args - 1; i++)
    {
      if (tmpval_used)
        enum_tmpval = g_alloca (sizeof (xint_t));

      atypes[i] = value_to_ffi_type (param_values + i,
                                     &args[i],
                                     enum_tmpval,
                                     &tmpval_used);
    }

  if (ffi_prep_cif (&cif, FFI_DEFAULT_ABI, n_args, rtype, atypes) != FFI_OK)
    return;

  ffi_call (&cif, marshal_data ? marshal_data : cc->callback, rvalue, args);

  if (return_gvalue && G_VALUE_TYPE (return_gvalue))
    value_from_ffi_type (return_gvalue, rvalue);
}

/**
 * g_cclosure_marshal_generic_va:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: (nullable): a #xvalue_t to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is
 *  invoked.
 * @args_list: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see xclosure_set_marshal() and
 *  xclosure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args_list.
 *
 * A generic #GVaClosureMarshal function implemented via
 * [libffi](http://sourceware.org/libffi/).
 *
 * Since: 2.30
 */
void
g_cclosure_marshal_generic_va (xclosure_t *closure,
			       xvalue_t   *return_value,
			       xpointer_t  instance,
			       va_list   args_list,
			       xpointer_t  marshal_data,
			       int       n_params,
			       xtype_t    *param_types)
{
  ffi_type *rtype;
  void *rvalue;
  int n_args;
  ffi_type **atypes;
  void **args;
  va_arg_storage *storage;
  int i;
  ffi_cif cif;
  GCClosure *cc = (GCClosure*) closure;
  xint_t *enum_tmpval;
  xboolean_t tmpval_used = FALSE;
  va_list args_copy;

  enum_tmpval = g_alloca (sizeof (xint_t));
  if (return_value && G_VALUE_TYPE (return_value))
    {
      rtype = value_to_ffi_type (return_value, &rvalue, enum_tmpval, &tmpval_used);
    }
  else
    {
      rtype = &ffi_type_void;
    }

  rvalue = g_alloca (MAX (rtype->size, sizeof (ffi_arg)));

  n_args = n_params + 2;
  atypes = g_alloca (sizeof (ffi_type *) * n_args);
  args =  g_alloca (sizeof (xpointer_t) * n_args);
  storage = g_alloca (sizeof (va_arg_storage) * n_params);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      atypes[n_args-1] = &ffi_type_pointer;
      args[n_args-1] = &instance;
      atypes[0] = &ffi_type_pointer;
      args[0] = &closure->data;
    }
  else
    {
      atypes[0] = &ffi_type_pointer;
      args[0] = &instance;
      atypes[n_args-1] = &ffi_type_pointer;
      args[n_args-1] = &closure->data;
    }

  G_VA_COPY (args_copy, args_list);

  /* Box non-primitive arguments */
  for (i = 0; i < n_params; i++)
    {
      xtype_t type = param_types[i]  & ~G_SIGNAL_TYPE_STATIC_SCOPE;
      xtype_t fundamental = XTYPE_FUNDAMENTAL (type);

      atypes[i+1] = va_to_ffi_type (type,
				    &args_copy,
				    &storage[i]);
      args[i+1] = &storage[i];

      if ((param_types[i]  & G_SIGNAL_TYPE_STATIC_SCOPE) == 0)
	{
	  if (fundamental == XTYPE_STRING && storage[i]._gpointer != NULL)
	    storage[i]._gpointer = xstrdup (storage[i]._gpointer);
	  else if (fundamental == XTYPE_PARAM && storage[i]._gpointer != NULL)
	    storage[i]._gpointer = g_param_spec_ref (storage[i]._gpointer);
	  else if (fundamental == XTYPE_BOXED && storage[i]._gpointer != NULL)
	    storage[i]._gpointer = xboxed_copy (type, storage[i]._gpointer);
	  else if (fundamental == XTYPE_VARIANT && storage[i]._gpointer != NULL)
	    storage[i]._gpointer = xvariant_ref_sink (storage[i]._gpointer);
	}
      if (fundamental == XTYPE_OBJECT && storage[i]._gpointer != NULL)
	storage[i]._gpointer = xobject_ref (storage[i]._gpointer);
    }

  va_end (args_copy);

  if (ffi_prep_cif (&cif, FFI_DEFAULT_ABI, n_args, rtype, atypes) != FFI_OK)
    return;

  ffi_call (&cif, marshal_data ? marshal_data : cc->callback, rvalue, args);

  /* Unbox non-primitive arguments */
  for (i = 0; i < n_params; i++)
    {
      xtype_t type = param_types[i]  & ~G_SIGNAL_TYPE_STATIC_SCOPE;
      xtype_t fundamental = XTYPE_FUNDAMENTAL (type);

      if ((param_types[i]  & G_SIGNAL_TYPE_STATIC_SCOPE) == 0)
	{
	  if (fundamental == XTYPE_STRING && storage[i]._gpointer != NULL)
	    g_free (storage[i]._gpointer);
	  else if (fundamental == XTYPE_PARAM && storage[i]._gpointer != NULL)
	    g_param_spec_unref (storage[i]._gpointer);
	  else if (fundamental == XTYPE_BOXED && storage[i]._gpointer != NULL)
	    xboxed_free (type, storage[i]._gpointer);
	  else if (fundamental == XTYPE_VARIANT && storage[i]._gpointer != NULL)
	    xvariant_unref (storage[i]._gpointer);
	}
      if (fundamental == XTYPE_OBJECT && storage[i]._gpointer != NULL)
	xobject_unref (storage[i]._gpointer);
    }

  if (return_value && G_VALUE_TYPE (return_value))
    value_from_ffi_type (return_value, rvalue);
}

/**
 * g_cclosure_marshal_VOID__VOID:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 1
 * @param_values: a #xvalue_t array holding only the instance
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__BOOLEAN:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xboolean_t parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xboolean_t arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__CHAR:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xchar_t parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xchar_t arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__UCHAR:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xuchar_t parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xuchar_t arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__INT:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xint_t parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xint_t arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__UINT:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xuint_t parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xuint_t arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__LONG:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xlong_t parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xlong_t arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__ULONG:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xulong_t parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xulong_t arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__ENUM:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the enumeration parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xint_t arg1, xpointer_t user_data)` where the #xint_t parameter denotes an enumeration type..
 */

/**
 * g_cclosure_marshal_VOID__FLAGS:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the flags parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xint_t arg1, xpointer_t user_data)` where the #xint_t parameter denotes a flags type.
 */

/**
 * g_cclosure_marshal_VOID__FLOAT:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #gfloat parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, gfloat arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__DOUBLE:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xdouble_t parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xdouble_t arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__STRING:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xchar_t* parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, const xchar_t *arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__PARAM:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xparam_spec_t* parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xparam_spec_t *arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__BOXED:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #GBoxed* parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, GBoxed *arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__POINTER:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xpointer_t parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xpointer_t arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__OBJECT:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xobject_t* parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xobject_t *arg1, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_VOID__VARIANT:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding the instance and the #xvariant_t* parameter
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xvariant_t *arg1, xpointer_t user_data)`.
 *
 * Since: 2.26
 */

/**
 * g_cclosure_marshal_VOID__UINT_POINTER:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: ignored
 * @n_param_values: 3
 * @param_values: a #xvalue_t array holding instance, arg1 and arg2
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `void (*callback) (xpointer_t instance, xuint_t arg1, xpointer_t arg2, xpointer_t user_data)`.
 */

/**
 * g_cclosure_marshal_BOOLEAN__FLAGS:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: a #xvalue_t which can store the returned #xboolean_t
 * @n_param_values: 2
 * @param_values: a #xvalue_t array holding instance and arg1
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `xboolean_t (*callback) (xpointer_t instance, xint_t arg1, xpointer_t user_data)` where the #xint_t parameter
 * denotes a flags type.
 */

/**
 * g_cclosure_marshal_BOOL__FLAGS:
 *
 * Another name for g_cclosure_marshal_BOOLEAN__FLAGS().
 */
/**
 * g_cclosure_marshal_STRING__OBJECT_POINTER:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: a #xvalue_t, which can store the returned string
 * @n_param_values: 3
 * @param_values: a #xvalue_t array holding instance, arg1 and arg2
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `xchar_t* (*callback) (xpointer_t instance, xobject_t *arg1, xpointer_t arg2, xpointer_t user_data)`.
 */
/**
 * g_cclosure_marshal_BOOLEAN__OBJECT_BOXED_BOXED:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: a #xvalue_t, which can store the returned string
 * @n_param_values: 3
 * @param_values: a #xvalue_t array holding instance, arg1 and arg2
 * @invocation_hint: the invocation hint given as the last argument
 *  to xclosure_invoke()
 * @marshal_data: additional data specified when registering the marshaller
 *
 * A marshaller for a #GCClosure with a callback of type
 * `xboolean_t (*callback) (xpointer_t instance, GBoxed *arg1, GBoxed *arg2, xpointer_t user_data)`.
 *
 * Since: 2.26
 */
