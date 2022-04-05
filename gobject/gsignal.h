/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2000-2001 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __G_SIGNAL_H__
#define __G_SIGNAL_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include	<gobject/gclosure.h>
#include	<gobject/gvalue.h>
#include	<gobject/gparam.h>
#include	<gobject/gmarshal.h>

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GSignalQuery		 GSignalQuery;
typedef struct _GSignalInvocationHint	 GSignalInvocationHint;
/**
 * GSignalCMarshaller:
 *
 * This is the signature of marshaller functions, required to marshall
 * arrays of parameter values to signal emissions into C language callback
 * invocations.
 *
 * It is merely an alias to #GClosureMarshal since the #GClosure mechanism
 * takes over responsibility of actual function invocation for the signal
 * system.
 */
typedef GClosureMarshal			 GSignalCMarshaller;
/**
 * GSignalCVaMarshaller:
 *
 * This is the signature of va_list marshaller functions, an optional
 * marshaller that can be used in some situations to avoid
 * marshalling the signal argument into GValues.
 */
typedef GVaClosureMarshal		 GSignalCVaMarshaller;
/**
 * GSignalEmissionHook:
 * @ihint: Signal invocation hint, see #GSignalInvocationHint.
 * @n_param_values: the number of parameters to the function, including
 *  the instance on which the signal was emitted.
 * @param_values: (array length=n_param_values): the instance on which
 *  the signal was emitted, followed by the parameters of the emission.
 * @data: user data associated with the hook.
 *
 * A simple function pointer to get invoked when the signal is emitted.
 *
 * Emission hooks allow you to tie a hook to the signal type, so that it will
 * trap all emissions of that signal, from any object.
 *
 * You may not attach these to signals created with the %G_SIGNAL_NO_HOOKS flag.
 *
 * Returns: whether it wants to stay connected. If it returns %FALSE, the signal
 *  hook is disconnected (and destroyed).
 */
typedef xboolean_t (*GSignalEmissionHook) (GSignalInvocationHint *ihint,
					 xuint_t			n_param_values,
					 const GValue	       *param_values,
					 xpointer_t		data);
/**
 * GSignalAccumulator:
 * @ihint: Signal invocation hint, see #GSignalInvocationHint.
 * @return_accu: Accumulator to collect callback return values in, this
 *  is the return value of the current signal emission.
 * @handler_return: A #GValue holding the return value of the signal handler.
 * @data: Callback data that was specified when creating the signal.
 *
 * The signal accumulator is a special callback function that can be used
 * to collect return values of the various callbacks that are called
 * during a signal emission.
 *
 * The signal accumulator is specified at signal creation time, if it is
 * left %NULL, no accumulation of callback return values is performed.
 * The return value of signal emissions is then the value returned by the
 * last callback.
 *
 * Returns: The accumulator function returns whether the signal emission
 *  should be aborted. Returning %TRUE will continue with
 *  the signal emission. Returning %FALSE will abort the current emission.
 *  Since 2.62, returning %FALSE will skip to the CLEANUP stage. In this case,
 *  emission will occur as normal in the CLEANUP stage and the handler's
 *  return value will be accumulated.
 */
typedef xboolean_t (*GSignalAccumulator)	(GSignalInvocationHint *ihint,
					 GValue		       *return_accu,
					 const GValue	       *handler_return,
					 xpointer_t               data);


/* --- run, match and connect types --- */
/**
 * GSignalFlags:
 * @G_SIGNAL_RUN_FIRST: Invoke the object method handler in the first emission stage.
 * @G_SIGNAL_RUN_LAST: Invoke the object method handler in the third emission stage.
 * @G_SIGNAL_RUN_CLEANUP: Invoke the object method handler in the last emission stage.
 * @G_SIGNAL_NO_RECURSE: Signals being emitted for an object while currently being in
 *  emission for this very object will not be emitted recursively,
 *  but instead cause the first emission to be restarted.
 * @G_SIGNAL_DETAILED: This signal supports "::detail" appendices to the signal name
 *  upon handler connections and emissions.
 * @G_SIGNAL_ACTION: Action signals are signals that may freely be emitted on alive
 *  objects from user code via g_signal_emit() and friends, without
 *  the need of being embedded into extra code that performs pre or
 *  post emission adjustments on the object. They can also be thought
 *  of as object methods which can be called generically by
 *  third-party code.
 * @G_SIGNAL_NO_HOOKS: No emissions hooks are supported for this signal.
 * @G_SIGNAL_MUST_COLLECT: Varargs signal emission will always collect the
 *   arguments, even if there are no signal handlers connected.  Since 2.30.
 * @G_SIGNAL_DEPRECATED: The signal is deprecated and will be removed
 *   in a future version. A warning will be generated if it is connected while
 *   running with G_ENABLE_DIAGNOSTIC=1.  Since 2.32.
 * @G_SIGNAL_ACCUMULATOR_FIRST_RUN: Only used in #GSignalAccumulator accumulator
 *   functions for the #GSignalInvocationHint::run_type field to mark the first
 *   call to the accumulator function for a signal emission.  Since 2.68.
 *
 * The signal flags are used to specify a signal's behaviour.
 */
typedef enum
{
  G_SIGNAL_RUN_FIRST	= 1 << 0,
  G_SIGNAL_RUN_LAST	= 1 << 1,
  G_SIGNAL_RUN_CLEANUP	= 1 << 2,
  G_SIGNAL_NO_RECURSE	= 1 << 3,
  G_SIGNAL_DETAILED	= 1 << 4,
  G_SIGNAL_ACTION	= 1 << 5,
  G_SIGNAL_NO_HOOKS	= 1 << 6,
  G_SIGNAL_MUST_COLLECT = 1 << 7,
  G_SIGNAL_DEPRECATED   = 1 << 8,
  /* normal signal flags until 1 << 16 */
  G_SIGNAL_ACCUMULATOR_FIRST_RUN    = 1 << 17,
} GSignalFlags;
/**
 * G_SIGNAL_FLAGS_MASK:
 *
 * A mask for all #GSignalFlags bits.
 */
#define G_SIGNAL_FLAGS_MASK  0x1ff
/**
 * GConnectFlags:
 * @G_CONNECT_AFTER: whether the handler should be called before or after the
 *  default handler of the signal.
 * @G_CONNECT_SWAPPED: whether the instance and data should be swapped when
 *  calling the handler; see g_signal_connect_swapped() for an example.
 *
 * The connection flags are used to specify the behaviour of a signal's
 * connection.
 */
typedef enum
{
  G_CONNECT_AFTER	= 1 << 0,
  G_CONNECT_SWAPPED	= 1 << 1
} GConnectFlags;
/**
 * GSignalMatchType:
 * @G_SIGNAL_MATCH_ID: The signal id must be equal.
 * @G_SIGNAL_MATCH_DETAIL: The signal detail must be equal.
 * @G_SIGNAL_MATCH_CLOSURE: The closure must be the same.
 * @G_SIGNAL_MATCH_FUNC: The C closure callback must be the same.
 * @G_SIGNAL_MATCH_DATA: The closure data must be the same.
 * @G_SIGNAL_MATCH_UNBLOCKED: Only unblocked signals may be matched.
 *
 * The match types specify what g_signal_handlers_block_matched(),
 * g_signal_handlers_unblock_matched() and g_signal_handlers_disconnect_matched()
 * match signals by.
 */
typedef enum
{
  G_SIGNAL_MATCH_ID	   = 1 << 0,
  G_SIGNAL_MATCH_DETAIL	   = 1 << 1,
  G_SIGNAL_MATCH_CLOSURE   = 1 << 2,
  G_SIGNAL_MATCH_FUNC	   = 1 << 3,
  G_SIGNAL_MATCH_DATA	   = 1 << 4,
  G_SIGNAL_MATCH_UNBLOCKED = 1 << 5
} GSignalMatchType;
/**
 * G_SIGNAL_MATCH_MASK:
 *
 * A mask for all #GSignalMatchType bits.
 */
#define G_SIGNAL_MATCH_MASK  0x3f
/**
 * G_SIGNAL_TYPE_STATIC_SCOPE:
 *
 * This macro flags signal argument types for which the signal system may
 * assume that instances thereof remain persistent across all signal emissions
 * they are used in. This is only useful for non ref-counted, value-copy types.
 *
 * To flag a signal argument in this way, add `| G_SIGNAL_TYPE_STATIC_SCOPE`
 * to the corresponding argument of g_signal_new().
 * |[
 * g_signal_new ("size_request",
 *   XTYPE_FROM_CLASS (gobject_class),
 * 	 G_SIGNAL_RUN_FIRST,
 * 	 G_STRUCT_OFFSET (GtkWidgetClass, size_request),
 * 	 NULL, NULL,
 * 	 _gtk_marshal_VOID__BOXED,
 * 	 XTYPE_NONE, 1,
 * 	 GTK_TYPE_REQUISITION | G_SIGNAL_TYPE_STATIC_SCOPE);
 * ]|
 */
#define	G_SIGNAL_TYPE_STATIC_SCOPE (XTYPE_FLAG_RESERVED_ID_BIT)


/* --- signal information --- */
/**
 * GSignalInvocationHint:
 * @signal_id: The signal id of the signal invoking the callback
 * @detail: The detail passed on for this emission
 * @run_type: The stage the signal emission is currently in, this
 *  field will contain one of %G_SIGNAL_RUN_FIRST,
 *  %G_SIGNAL_RUN_LAST or %G_SIGNAL_RUN_CLEANUP and %G_SIGNAL_ACCUMULATOR_FIRST_RUN.
 *  %G_SIGNAL_ACCUMULATOR_FIRST_RUN is only set for the first run of the accumulator
 *  function for a signal emission.
 *
 * The #GSignalInvocationHint structure is used to pass on additional information
 * to callbacks during a signal emission.
 */
struct _GSignalInvocationHint
{
  xuint_t		signal_id;
  GQuark	detail;
  GSignalFlags	run_type;
};
/**
 * GSignalQuery:
 * @signal_id: The signal id of the signal being queried, or 0 if the
 *  signal to be queried was unknown.
 * @signal_name: The signal name.
 * @itype: The interface/instance type that this signal can be emitted for.
 * @signal_flags: The signal flags as passed in to g_signal_new().
 * @return_type: The return type for user callbacks.
 * @n_params: The number of parameters that user callbacks take.
 * @param_types: (array length=n_params): The individual parameter types for
 *  user callbacks, note that the effective callback signature is:
 *  |[<!-- language="C" -->
 *  @return_type callback (#xpointer_t     data1,
 *  [param_types param_names,]
 *  xpointer_t     data2);
 *  ]|
 *
 * A structure holding in-depth information for a specific signal.
 *
 * See also: g_signal_query()
 */
struct _GSignalQuery
{
  xuint_t		signal_id;
  const xchar_t  *signal_name;
  xtype_t		itype;
  GSignalFlags	signal_flags;
  xtype_t		return_type; /* mangled with G_SIGNAL_TYPE_STATIC_SCOPE flag */
  xuint_t		n_params;
  const xtype_t  *param_types; /* mangled with G_SIGNAL_TYPE_STATIC_SCOPE flag */
};


/* --- signals --- */
XPL_AVAILABLE_IN_ALL
xuint_t                 g_signal_newv         (const xchar_t        *signal_name,
					     xtype_t               itype,
					     GSignalFlags        signal_flags,
					     GClosure           *class_closure,
					     GSignalAccumulator	 accumulator,
					     xpointer_t		 accu_data,
					     GSignalCMarshaller  c_marshaller,
					     xtype_t               return_type,
					     xuint_t               n_params,
					     xtype_t              *param_types);
XPL_AVAILABLE_IN_ALL
xuint_t                 g_signal_new_valist   (const xchar_t        *signal_name,
					     xtype_t               itype,
					     GSignalFlags        signal_flags,
					     GClosure           *class_closure,
					     GSignalAccumulator	 accumulator,
					     xpointer_t		 accu_data,
					     GSignalCMarshaller  c_marshaller,
					     xtype_t               return_type,
					     xuint_t               n_params,
					     va_list             args);
XPL_AVAILABLE_IN_ALL
xuint_t                 g_signal_new          (const xchar_t        *signal_name,
					     xtype_t               itype,
					     GSignalFlags        signal_flags,
					     xuint_t               class_offset,
					     GSignalAccumulator	 accumulator,
					     xpointer_t		 accu_data,
					     GSignalCMarshaller  c_marshaller,
					     xtype_t               return_type,
					     xuint_t               n_params,
					     ...);
XPL_AVAILABLE_IN_ALL
xuint_t            g_signal_new_class_handler (const xchar_t        *signal_name,
                                             xtype_t               itype,
                                             GSignalFlags        signal_flags,
                                             GCallback           class_handler,
                                             GSignalAccumulator  accumulator,
                                             xpointer_t            accu_data,
                                             GSignalCMarshaller  c_marshaller,
                                             xtype_t               return_type,
                                             xuint_t               n_params,
                                             ...);
XPL_AVAILABLE_IN_ALL
void             g_signal_set_va_marshaller (xuint_t              signal_id,
					     xtype_t              instance_type,
					     GSignalCVaMarshaller va_marshaller);

XPL_AVAILABLE_IN_ALL
void                  g_signal_emitv        (const GValue       *instance_and_params,
					     xuint_t               signal_id,
					     GQuark              detail,
					     GValue             *return_value);
XPL_AVAILABLE_IN_ALL
void                  g_signal_emit_valist  (xpointer_t            instance,
					     xuint_t               signal_id,
					     GQuark              detail,
					     va_list             var_args);
XPL_AVAILABLE_IN_ALL
void                  g_signal_emit         (xpointer_t            instance,
					     xuint_t               signal_id,
					     GQuark              detail,
					     ...);
XPL_AVAILABLE_IN_ALL
void                  g_signal_emit_by_name (xpointer_t            instance,
					     const xchar_t        *detailed_signal,
					     ...);
XPL_AVAILABLE_IN_ALL
xuint_t                 g_signal_lookup       (const xchar_t        *name,
					     xtype_t               itype);
XPL_AVAILABLE_IN_ALL
const xchar_t *         g_signal_name         (xuint_t               signal_id);
XPL_AVAILABLE_IN_ALL
void                  g_signal_query        (xuint_t               signal_id,
					     GSignalQuery       *query);
XPL_AVAILABLE_IN_ALL
xuint_t*                g_signal_list_ids     (xtype_t               itype,
					     xuint_t              *n_ids);
XPL_AVAILABLE_IN_2_66
xboolean_t              g_signal_is_valid_name (const xchar_t      *name);
XPL_AVAILABLE_IN_ALL
xboolean_t	      g_signal_parse_name   (const xchar_t	*detailed_signal,
					     xtype_t		 itype,
					     xuint_t		*signal_id_p,
					     GQuark		*detail_p,
					     xboolean_t		 force_detail_quark);
XPL_AVAILABLE_IN_ALL
GSignalInvocationHint* g_signal_get_invocation_hint (xpointer_t    instance);


/* --- signal emissions --- */
XPL_AVAILABLE_IN_ALL
void	g_signal_stop_emission		    (xpointer_t		  instance,
					     xuint_t		  signal_id,
					     GQuark		  detail);
XPL_AVAILABLE_IN_ALL
void	g_signal_stop_emission_by_name	    (xpointer_t		  instance,
					     const xchar_t	 *detailed_signal);
XPL_AVAILABLE_IN_ALL
gulong	g_signal_add_emission_hook	    (xuint_t		  signal_id,
					     GQuark		  detail,
					     GSignalEmissionHook  hook_func,
					     xpointer_t	       	  hook_data,
					     GDestroyNotify	  data_destroy);
XPL_AVAILABLE_IN_ALL
void	g_signal_remove_emission_hook	    (xuint_t		  signal_id,
					     gulong		  hook_id);


/* --- signal handlers --- */
XPL_AVAILABLE_IN_ALL
xboolean_t g_signal_has_handler_pending	      (xpointer_t		  instance,
					       xuint_t		  signal_id,
					       GQuark		  detail,
					       xboolean_t		  may_be_blocked);
XPL_AVAILABLE_IN_ALL
gulong	 g_signal_connect_closure_by_id	      (xpointer_t		  instance,
					       xuint_t		  signal_id,
					       GQuark		  detail,
					       GClosure		 *closure,
					       xboolean_t		  after);
XPL_AVAILABLE_IN_ALL
gulong	 g_signal_connect_closure	      (xpointer_t		  instance,
					       const xchar_t       *detailed_signal,
					       GClosure		 *closure,
					       xboolean_t		  after);
XPL_AVAILABLE_IN_ALL
gulong	 g_signal_connect_data		      (xpointer_t		  instance,
					       const xchar_t	 *detailed_signal,
					       GCallback	  c_handler,
					       xpointer_t		  data,
					       GClosureNotify	  destroy_data,
					       GConnectFlags	  connect_flags);
XPL_AVAILABLE_IN_ALL
void	 g_signal_handler_block		      (xpointer_t		  instance,
					       gulong		  handler_id);
XPL_AVAILABLE_IN_ALL
void	 g_signal_handler_unblock	      (xpointer_t		  instance,
					       gulong		  handler_id);
XPL_AVAILABLE_IN_ALL
void	 g_signal_handler_disconnect	      (xpointer_t		  instance,
					       gulong		  handler_id);
XPL_AVAILABLE_IN_ALL
xboolean_t g_signal_handler_is_connected	      (xpointer_t		  instance,
					       gulong		  handler_id);
XPL_AVAILABLE_IN_ALL
gulong	 g_signal_handler_find		      (xpointer_t		  instance,
					       GSignalMatchType	  mask,
					       xuint_t		  signal_id,
					       GQuark		  detail,
					       GClosure		 *closure,
					       xpointer_t		  func,
					       xpointer_t		  data);
XPL_AVAILABLE_IN_ALL
xuint_t	 g_signal_handlers_block_matched      (xpointer_t		  instance,
					       GSignalMatchType	  mask,
					       xuint_t		  signal_id,
					       GQuark		  detail,
					       GClosure		 *closure,
					       xpointer_t		  func,
					       xpointer_t		  data);
XPL_AVAILABLE_IN_ALL
xuint_t	 g_signal_handlers_unblock_matched    (xpointer_t		  instance,
					       GSignalMatchType	  mask,
					       xuint_t		  signal_id,
					       GQuark		  detail,
					       GClosure		 *closure,
					       xpointer_t		  func,
					       xpointer_t		  data);
XPL_AVAILABLE_IN_ALL
xuint_t	 g_signal_handlers_disconnect_matched (xpointer_t		  instance,
					       GSignalMatchType	  mask,
					       xuint_t		  signal_id,
					       GQuark		  detail,
					       GClosure		 *closure,
					       xpointer_t		  func,
					       xpointer_t		  data);

XPL_AVAILABLE_IN_2_62
void	 g_clear_signal_handler		      (gulong            *handler_id_ptr,
					       xpointer_t           instance);

#define  g_clear_signal_handler(handler_id_ptr, instance)           \
  G_STMT_START {                                                    \
    xpointer_t const _instance      = (instance);                     \
    gulong *const _handler_id_ptr = (handler_id_ptr);               \
    const gulong _handler_id      = *_handler_id_ptr;               \
                                                                    \
    if (_handler_id > 0)                                            \
      {                                                             \
        *_handler_id_ptr = 0;                                       \
        g_signal_handler_disconnect (_instance, _handler_id);       \
      }                                                             \
  } G_STMT_END                                                      \
  XPL_AVAILABLE_MACRO_IN_2_62

/* --- overriding and chaining --- */
XPL_AVAILABLE_IN_ALL
void    g_signal_override_class_closure       (xuint_t              signal_id,
                                               xtype_t              instance_type,
                                               GClosure          *class_closure);
XPL_AVAILABLE_IN_ALL
void    g_signal_override_class_handler       (const xchar_t       *signal_name,
                                               xtype_t              instance_type,
                                               GCallback          class_handler);
XPL_AVAILABLE_IN_ALL
void    g_signal_chain_from_overridden        (const GValue      *instance_and_params,
                                               GValue            *return_value);
XPL_AVAILABLE_IN_ALL
void   g_signal_chain_from_overridden_handler (xpointer_t           instance,
                                               ...);


/* --- convenience --- */
/**
 * g_signal_connect:
 * @instance: the instance to connect to.
 * @detailed_signal: a string of the form "signal-name::detail".
 * @c_handler: the #GCallback to connect.
 * @data: data to pass to @c_handler calls.
 *
 * Connects a #GCallback function to a signal for a particular object.
 *
 * The handler will be called synchronously, before the default handler of the signal. g_signal_emit() will not return control until all handlers are called.
 *
 * See [memory management of signal handlers][signal-memory-management] for
 * details on how to handle the return value and memory management of @data.
 *
 * Returns: the handler ID, of type #gulong (always greater than 0 for successful connections)
 */
#define g_signal_connect(instance, detailed_signal, c_handler, data) \
    g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, (GConnectFlags) 0)
/**
 * g_signal_connect_after:
 * @instance: the instance to connect to.
 * @detailed_signal: a string of the form "signal-name::detail".
 * @c_handler: the #GCallback to connect.
 * @data: data to pass to @c_handler calls.
 *
 * Connects a #GCallback function to a signal for a particular object.
 *
 * The handler will be called synchronously, after the default handler of the signal.
 *
 * Returns: the handler ID, of type #gulong (always greater than 0 for successful connections)
 */
#define g_signal_connect_after(instance, detailed_signal, c_handler, data) \
    g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, G_CONNECT_AFTER)
/**
 * g_signal_connect_swapped:
 * @instance: the instance to connect to.
 * @detailed_signal: a string of the form "signal-name::detail".
 * @c_handler: the #GCallback to connect.
 * @data: data to pass to @c_handler calls.
 *
 * Connects a #GCallback function to a signal for a particular object.
 *
 * The instance on which the signal is emitted and @data will be swapped when
 * calling the handler. This is useful when calling pre-existing functions to
 * operate purely on the @data, rather than the @instance: swapping the
 * parameters avoids the need to write a wrapper function.
 *
 * For example, this allows the shorter code:
 * |[<!-- language="C" -->
 * g_signal_connect_swapped (button, "clicked",
 *                           (GCallback) gtk_widget_hide, other_widget);
 * ]|
 *
 * Rather than the cumbersome:
 * |[<!-- language="C" -->
 * static void
 * button_clicked_cb (GtkButton *button, GtkWidget *other_widget)
 * {
 *     gtk_widget_hide (other_widget);
 * }
 *
 * ...
 *
 * g_signal_connect (button, "clicked",
 *                   (GCallback) button_clicked_cb, other_widget);
 * ]|
 *
 * Returns: the handler ID, of type #gulong (always greater than 0 for successful connections)
 */
#define g_signal_connect_swapped(instance, detailed_signal, c_handler, data) \
    g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, G_CONNECT_SWAPPED)
/**
 * g_signal_handlers_disconnect_by_func:
 * @instance: The instance to remove handlers from.
 * @func: The C closure callback of the handlers (useless for non-C closures).
 * @data: The closure data of the handlers' closures.
 *
 * Disconnects all handlers on an instance that match @func and @data.
 *
 * Returns: The number of handlers that matched.
 */
#define	g_signal_handlers_disconnect_by_func(instance, func, data)						\
    g_signal_handlers_disconnect_matched ((instance),								\
					  (GSignalMatchType) (G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA),	\
					  0, 0, NULL, (func), (data))

/**
 * g_signal_handlers_disconnect_by_data:
 * @instance: The instance to remove handlers from
 * @data: the closure data of the handlers' closures
 *
 * Disconnects all handlers on an instance that match @data.
 *
 * Returns: The number of handlers that matched.
 *
 * Since: 2.32
 */
#define g_signal_handlers_disconnect_by_data(instance, data) \
  g_signal_handlers_disconnect_matched ((instance), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, (data))

/**
 * g_signal_handlers_block_by_func:
 * @instance: The instance to block handlers from.
 * @func: The C closure callback of the handlers (useless for non-C closures).
 * @data: The closure data of the handlers' closures.
 *
 * Blocks all handlers on an instance that match @func and @data.
 *
 * Returns: The number of handlers that matched.
 */
#define	g_signal_handlers_block_by_func(instance, func, data)							\
    g_signal_handlers_block_matched      ((instance),								\
				          (GSignalMatchType) (G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA),	\
				          0, 0, NULL, (func), (data))
/**
 * g_signal_handlers_unblock_by_func:
 * @instance: The instance to unblock handlers from.
 * @func: The C closure callback of the handlers (useless for non-C closures).
 * @data: The closure data of the handlers' closures.
 *
 * Unblocks all handlers on an instance that match @func and @data.
 *
 * Returns: The number of handlers that matched.
 */
#define	g_signal_handlers_unblock_by_func(instance, func, data)							\
    g_signal_handlers_unblock_matched    ((instance),								\
				          (GSignalMatchType) (G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA),	\
				          0, 0, NULL, (func), (data))


XPL_AVAILABLE_IN_ALL
xboolean_t g_signal_accumulator_true_handled (GSignalInvocationHint *ihint,
					    GValue                *return_accu,
					    const GValue          *handler_return,
					    xpointer_t               dummy);

XPL_AVAILABLE_IN_ALL
xboolean_t g_signal_accumulator_first_wins   (GSignalInvocationHint *ihint,
                                            GValue                *return_accu,
                                            const GValue          *handler_return,
                                            xpointer_t               dummy);

/*< private >*/
XPL_AVAILABLE_IN_ALL
void	 g_signal_handlers_destroy	      (xpointer_t		  instance);
void	 _g_signals_destroy		      (xtype_t		  itype);

G_END_DECLS

#endif /* __G_SIGNAL_H__ */
