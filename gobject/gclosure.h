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
#ifndef __G_CLOSURE_H__
#define __G_CLOSURE_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include        <gobject/gtype.h>

G_BEGIN_DECLS

/* --- defines --- */
/**
 * G_CLOSURE_NEEDS_MARSHAL:
 * @closure: a #GClosure
 *
 * Check if the closure still needs a marshaller. See g_closure_set_marshal().
 *
 * Returns: %TRUE if a #GClosureMarshal marshaller has not yet been set on
 * @closure.
 */
#define	G_CLOSURE_NEEDS_MARSHAL(closure) (((GClosure*) (closure))->marshal == NULL)
/**
 * G_CLOSURE_N_NOTIFIERS:
 * @cl: a #GClosure
 *
 * Get the total number of notifiers connected with the closure @cl.
 *
 * The count includes the meta marshaller, the finalize and invalidate notifiers
 * and the marshal guards. Note that each guard counts as two notifiers.
 * See g_closure_set_meta_marshal(), g_closure_add_finalize_notifier(),
 * g_closure_add_invalidate_notifier() and g_closure_add_marshal_guards().
 *
 * Returns: number of notifiers
 */
#define	G_CLOSURE_N_NOTIFIERS(cl)	 (((cl)->n_guards << 1L) + \
                                          (cl)->n_fnotifiers + (cl)->n_inotifiers)
/**
 * G_CCLOSURE_SWAP_DATA:
 * @cclosure: a #GCClosure
 *
 * Checks whether the user data of the #GCClosure should be passed as the
 * first parameter to the callback. See g_cclosure_new_swap().
 *
 * Returns: %TRUE if data has to be swapped.
 */
#define	G_CCLOSURE_SWAP_DATA(cclosure)	 (((GClosure*) (cclosure))->derivative_flag)
/**
 * G_CALLBACK:
 * @f: a function pointer.
 *
 * Cast a function pointer to a #GCallback.
 */
#define	G_CALLBACK(f)			 ((GCallback) (f))


/* -- typedefs --- */
typedef struct _GClosure		 GClosure;
typedef struct _GClosureNotifyData	 GClosureNotifyData;

/**
 * GCallback:
 *
 * The type used for callback functions in structure definitions and function
 * signatures.
 *
 * This doesn't mean that all callback functions must take no  parameters and
 * return void. The required signature of a callback function is determined by
 * the context in which is used (e.g. the signal to which it is connected).
 *
 * Use G_CALLBACK() to cast the callback function to a #GCallback.
 */
typedef void  (*GCallback)              (void);
/**
 * GClosureNotify:
 * @data: data specified when registering the notification callback
 * @closure: the #GClosure on which the notification is emitted
 *
 * The type used for the various notification callbacks which can be registered
 * on closures.
 */
typedef void  (*GClosureNotify)		(xpointer_t	 data,
					 GClosure	*closure);
/**
 * GClosureMarshal:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @n_param_values: the length of the @param_values array
 * @param_values: (array length=n_param_values): an array of
 *  #GValues holding the arguments on which to invoke the
 *  callback of @closure
 * @invocation_hint: (nullable): the invocation hint given as the
 *  last argument to g_closure_invoke()
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 *
 * The type used for marshaller functions.
 */
typedef void  (*GClosureMarshal)	(GClosure	*closure,
					 GValue         *return_value,
					 xuint_t           n_param_values,
					 const GValue   *param_values,
					 xpointer_t        invocation_hint,
					 xpointer_t	 marshal_data);

/**
 * GVaClosureMarshal:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is
 *  invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * This is the signature of va_list marshaller functions, an optional
 * marshaller that can be used in some situations to avoid
 * marshalling the signal argument into GValues.
 */
typedef void (* GVaClosureMarshal) (GClosure *closure,
				    GValue   *return_value,
				    xpointer_t  instance,
				    va_list   args,
				    xpointer_t  marshal_data,
				    int       n_params,
				    xtype_t    *param_types);

/**
 * GCClosure:
 * @closure: the #GClosure
 * @callback: the callback function
 *
 * A #GCClosure is a specialization of #GClosure for C function callbacks.
 */
typedef struct _GCClosure		 GCClosure;


/* --- structures --- */
struct _GClosureNotifyData
{
  xpointer_t       data;
  GClosureNotify notify;
};
/**
 * GClosure:
 * @in_marshal: Indicates whether the closure is currently being invoked with
 *  g_closure_invoke()
 * @is_invalid: Indicates whether the closure has been invalidated by
 *  g_closure_invalidate()
 *
 * A #GClosure represents a callback supplied by the programmer.
 */
struct _GClosure
{
  /*< private >*/
  xuint_t ref_count : 15;  /* (atomic) */
  /* meta_marshal is not used anymore but must be zero for historical reasons
     as it was exposed in the G_CLOSURE_N_NOTIFIERS macro */
  xuint_t meta_marshal_nouse : 1;  /* (atomic) */
  xuint_t n_guards : 1;  /* (atomic) */
  xuint_t n_fnotifiers : 2;  /* finalization notifiers (atomic) */
  xuint_t n_inotifiers : 8;  /* invalidation notifiers (atomic) */
  xuint_t in_inotify : 1;  /* (atomic) */
  xuint_t floating : 1;  /* (atomic) */
  /*< protected >*/
  xuint_t derivative_flag : 1;  /* (atomic) */
  /*< public >*/
  xuint_t in_marshal : 1;  /* (atomic) */
  xuint_t is_invalid : 1;  /* (atomic) */

  /*< private >*/	void   (*marshal)  (GClosure       *closure,
					    GValue /*out*/ *return_value,
					    xuint_t           n_param_values,
					    const GValue   *param_values,
					    xpointer_t        invocation_hint,
					    xpointer_t	    marshal_data);
  /*< protected >*/	xpointer_t data;

  /*< private >*/	GClosureNotifyData *notifiers;

  /* invariants/constraints:
   * - ->marshal and ->data are _invalid_ as soon as ->is_invalid==TRUE
   * - invocation of all inotifiers occurs prior to fnotifiers
   * - order of inotifiers is random
   *   inotifiers may _not_ free/invalidate parameter values (e.g. ->data)
   * - order of fnotifiers is random
   * - each notifier may only be removed before or during its invocation
   * - reference counting may only happen prior to fnotify invocation
   *   (in that sense, fnotifiers are really finalization handlers)
   */
};
/* closure for C function calls, callback() is the user function
 */
struct _GCClosure
{
  GClosure	closure;
  xpointer_t	callback;
};


/* --- prototypes --- */
XPL_AVAILABLE_IN_ALL
GClosure* g_cclosure_new			(GCallback	callback_func,
						 xpointer_t	user_data,
						 GClosureNotify destroy_data);
XPL_AVAILABLE_IN_ALL
GClosure* g_cclosure_new_swap			(GCallback	callback_func,
						 xpointer_t	user_data,
						 GClosureNotify destroy_data);
XPL_AVAILABLE_IN_ALL
GClosure* g_signal_type_cclosure_new		(xtype_t          itype,
						 xuint_t          struct_offset);


/* --- prototypes --- */
XPL_AVAILABLE_IN_ALL
GClosure* g_closure_ref				(GClosure	*closure);
XPL_AVAILABLE_IN_ALL
void	  g_closure_sink			(GClosure	*closure);
XPL_AVAILABLE_IN_ALL
void	  g_closure_unref			(GClosure	*closure);
/* intimidating */
XPL_AVAILABLE_IN_ALL
GClosure* g_closure_new_simple			(xuint_t		 sizeof_closure,
						 xpointer_t	 data);
XPL_AVAILABLE_IN_ALL
void	  g_closure_add_finalize_notifier	(GClosure       *closure,
						 xpointer_t	 notify_data,
						 GClosureNotify	 notify_func);
XPL_AVAILABLE_IN_ALL
void	  g_closure_remove_finalize_notifier	(GClosure       *closure,
						 xpointer_t	 notify_data,
						 GClosureNotify	 notify_func);
XPL_AVAILABLE_IN_ALL
void	  g_closure_add_invalidate_notifier	(GClosure       *closure,
						 xpointer_t	 notify_data,
						 GClosureNotify	 notify_func);
XPL_AVAILABLE_IN_ALL
void	  g_closure_remove_invalidate_notifier	(GClosure       *closure,
						 xpointer_t	 notify_data,
						 GClosureNotify	 notify_func);
XPL_AVAILABLE_IN_ALL
void	  g_closure_add_marshal_guards		(GClosure	*closure,
						 xpointer_t        pre_marshal_data,
						 GClosureNotify	 pre_marshal_notify,
						 xpointer_t        post_marshal_data,
						 GClosureNotify	 post_marshal_notify);
XPL_AVAILABLE_IN_ALL
void	  g_closure_set_marshal			(GClosure	*closure,
						 GClosureMarshal marshal);
XPL_AVAILABLE_IN_ALL
void	  g_closure_set_meta_marshal		(GClosure       *closure,
						 xpointer_t	 marshal_data,
						 GClosureMarshal meta_marshal);
XPL_AVAILABLE_IN_ALL
void	  g_closure_invalidate			(GClosure	*closure);
XPL_AVAILABLE_IN_ALL
void	  g_closure_invoke			(GClosure 	*closure,
						 GValue	/*out*/	*return_value,
						 xuint_t		 n_param_values,
						 const GValue	*param_values,
						 xpointer_t	 invocation_hint);

/* FIXME:
   OK:  data_object::destroy		-> closure_invalidate();
   MIS:	closure_invalidate()		-> disconnect(closure);
   MIS:	disconnect(closure)		-> (unlink) closure_unref();
   OK:	closure_finalize()		-> g_free (data_string);

   random remarks:
   - need marshaller repo with decent aliasing to base types
   - provide marshaller collection, virtually covering anything out there
*/

XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_generic (GClosure     *closure,
                                 GValue       *return_gvalue,
                                 xuint_t         n_param_values,
                                 const GValue *param_values,
                                 xpointer_t      invocation_hint,
                                 xpointer_t      marshal_data);

XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_generic_va (GClosure *closure,
				    GValue   *return_value,
				    xpointer_t  instance,
				    va_list   args_list,
				    xpointer_t  marshal_data,
				    int       n_params,
				    xtype_t    *param_types);


G_END_DECLS

#endif /* __G_CLOSURE_H__ */
