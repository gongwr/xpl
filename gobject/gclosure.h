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
 * @closure: a #xclosure_t
 *
 * Check if the closure still needs a marshaller. See xclosure_set_marshal().
 *
 * Returns: %TRUE if a #GClosureMarshal marshaller has not yet been set on
 * @closure.
 */
#define	G_CLOSURE_NEEDS_MARSHAL(closure) (((xclosure_t*) (closure))->marshal == NULL)
/**
 * G_CLOSURE_N_NOTIFIERS:
 * @cl: a #xclosure_t
 *
 * Get the total number of notifiers connected with the closure @cl.
 *
 * The count includes the meta marshaller, the finalize and invalidate notifiers
 * and the marshal guards. Note that each guard counts as two notifiers.
 * See xclosure_set_meta_marshal(), xclosure_add_finalize_notifier(),
 * xclosure_add_invalidate_notifier() and xclosure_add_marshal_guards().
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
#define	G_CCLOSURE_SWAP_DATA(cclosure)	 (((xclosure_t*) (cclosure))->derivative_flag)
/**
 * G_CALLBACK:
 * @f: a function pointer.
 *
 * Cast a function pointer to a #xcallback_t.
 */
#define	G_CALLBACK(f)			 ((xcallback_t) (f))


/* -- typedefs --- */
typedef struct _xclosure		 xclosure_t;
typedef struct _xclosure_notify_data	 xclosure_notify_data_t;

/**
 * xcallback_t:
 *
 * The type used for callback functions in structure definitions and function
 * signatures.
 *
 * This doesn't mean that all callback functions must take no  parameters and
 * return void. The required signature of a callback function is determined by
 * the context in which is used (e.g. the signal to which it is connected).
 *
 * Use G_CALLBACK() to cast the callback function to a #xcallback_t.
 */
typedef void  (*xcallback_t)              (void);
/**
 * xclosure_notify_t:
 * @data: data specified when registering the notification callback
 * @closure: the #xclosure_t on which the notification is emitted
 *
 * The type used for the various notification callbacks which can be registered
 * on closures.
 */
typedef void  (*xclosure_notify_t)		(xpointer_t	 data,
					 xclosure_t	*closure);
/**
 * GClosureMarshal:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: (nullable): a #xvalue_t to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @n_param_values: the length of the @param_values array
 * @param_values: (array length=n_param_values): an array of
 *  #GValues holding the arguments on which to invoke the
 *  callback of @closure
 * @invocation_hint: (nullable): the invocation hint given as the
 *  last argument to xclosure_invoke()
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see xclosure_set_marshal() and
 *  xclosure_set_meta_marshal()
 *
 * The type used for marshaller functions.
 */
typedef void  (*GClosureMarshal)	(xclosure_t	*closure,
					 xvalue_t         *return_value,
					 xuint_t           n_param_values,
					 const xvalue_t   *param_values,
					 xpointer_t        invocation_hint,
					 xpointer_t	 marshal_data);

/**
 * GVaClosureMarshal:
 * @closure: the #xclosure_t to which the marshaller belongs
 * @return_value: (nullable): a #xvalue_t to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is
 *  invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see xclosure_set_marshal() and
 *  xclosure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * This is the signature of va_list marshaller functions, an optional
 * marshaller that can be used in some situations to avoid
 * marshalling the signal argument into GValues.
 */
typedef void (* GVaClosureMarshal) (xclosure_t *closure,
				    xvalue_t   *return_value,
				    xpointer_t  instance,
				    va_list   args,
				    xpointer_t  marshal_data,
				    int       n_params,
				    xtype_t    *param_types);

/**
 * GCClosure:
 * @closure: the #xclosure_t
 * @callback: the callback function
 *
 * A #GCClosure is a specialization of #xclosure_t for C function callbacks.
 */
typedef struct _GCClosure		 GCClosure;


/* --- structures --- */
struct _xclosure_notify_data
{
  xpointer_t       data;
  xclosure_notify_t notify;
};
/**
 * xclosure_t:
 * @in_marshal: Indicates whether the closure is currently being invoked with
 *  xclosure_invoke()
 * @is_invalid: Indicates whether the closure has been invalidated by
 *  xclosure_invalidate()
 *
 * A #xclosure_t represents a callback supplied by the programmer.
 */
struct _xclosure
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

  /*< private >*/	void   (*marshal)  (xclosure_t       *closure,
					    xvalue_t /*out*/ *return_value,
					    xuint_t           n_param_values,
					    const xvalue_t   *param_values,
					    xpointer_t        invocation_hint,
					    xpointer_t	    marshal_data);
  /*< protected >*/	xpointer_t data;

  /*< private >*/	xclosure_notify_data_t *notifiers;

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
  xclosure_t	closure;
  xpointer_t	callback;
};


/* --- prototypes --- */
XPL_AVAILABLE_IN_ALL
xclosure_t* g_cclosure_new			(xcallback_t	callback_func,
						 xpointer_t	user_data,
						 xclosure_notify_t destroy_data);
XPL_AVAILABLE_IN_ALL
xclosure_t* g_cclosure_new_swap			(xcallback_t	callback_func,
						 xpointer_t	user_data,
						 xclosure_notify_t destroy_data);
XPL_AVAILABLE_IN_ALL
xclosure_t* g_signal_type_cclosure_new		(xtype_t          itype,
						 xuint_t          struct_offset);


/* --- prototypes --- */
XPL_AVAILABLE_IN_ALL
xclosure_t* xclosure_ref				(xclosure_t	*closure);
XPL_AVAILABLE_IN_ALL
void	  xclosure_sink			(xclosure_t	*closure);
XPL_AVAILABLE_IN_ALL
void	  xclosure_unref			(xclosure_t	*closure);
/* intimidating */
XPL_AVAILABLE_IN_ALL
xclosure_t* xclosure_new_simple			(xuint_t		 sizeof_closure,
						 xpointer_t	 data);
XPL_AVAILABLE_IN_ALL
void	  xclosure_add_finalize_notifier	(xclosure_t       *closure,
						 xpointer_t	 notify_data,
						 xclosure_notify_t	 notify_func);
XPL_AVAILABLE_IN_ALL
void	  xclosure_remove_finalize_notifier	(xclosure_t       *closure,
						 xpointer_t	 notify_data,
						 xclosure_notify_t	 notify_func);
XPL_AVAILABLE_IN_ALL
void	  xclosure_add_invalidate_notifier	(xclosure_t       *closure,
						 xpointer_t	 notify_data,
						 xclosure_notify_t	 notify_func);
XPL_AVAILABLE_IN_ALL
void	  xclosure_remove_invalidate_notifier	(xclosure_t       *closure,
						 xpointer_t	 notify_data,
						 xclosure_notify_t	 notify_func);
XPL_AVAILABLE_IN_ALL
void	  xclosure_add_marshal_guards		(xclosure_t	*closure,
						 xpointer_t        pre_marshal_data,
						 xclosure_notify_t	 pre_marshal_notify,
						 xpointer_t        post_marshal_data,
						 xclosure_notify_t	 post_marshal_notify);
XPL_AVAILABLE_IN_ALL
void	  xclosure_set_marshal			(xclosure_t	*closure,
						 GClosureMarshal marshal);
XPL_AVAILABLE_IN_ALL
void	  xclosure_set_meta_marshal		(xclosure_t       *closure,
						 xpointer_t	 marshal_data,
						 GClosureMarshal meta_marshal);
XPL_AVAILABLE_IN_ALL
void	  xclosure_invalidate			(xclosure_t	*closure);
XPL_AVAILABLE_IN_ALL
void	  xclosure_invoke			(xclosure_t 	*closure,
						 xvalue_t	/*out*/	*return_value,
						 xuint_t		 n_param_values,
						 const xvalue_t	*param_values,
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
void g_cclosure_marshal_generic (xclosure_t     *closure,
                                 xvalue_t       *return_gvalue,
                                 xuint_t         n_param_values,
                                 const xvalue_t *param_values,
                                 xpointer_t      invocation_hint,
                                 xpointer_t      marshal_data);

XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_generic_va (xclosure_t *closure,
				    xvalue_t   *return_value,
				    xpointer_t  instance,
				    va_list   args_list,
				    xpointer_t  marshal_data,
				    int       n_params,
				    xtype_t    *param_types);


G_END_DECLS

#endif /* __G_CLOSURE_H__ */
