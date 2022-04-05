/* xobject_t - GLib Type, Object, Parameter and Signal Library
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

#include "gobject.h"
#include "genums.h"
#include "gboxed.h"
#include "gvaluetypes.h"


#ifdef G_ENABLE_DEBUG
#define g_marshal_value_peek_boolean(v)  g_value_get_boolean (v)
#define g_marshal_value_peek_char(v)     g_value_get_schar (v)
#define g_marshal_value_peek_uchar(v)    g_value_get_uchar (v)
#define g_marshal_value_peek_int(v)      g_value_get_int (v)
#define g_marshal_value_peek_uint(v)     g_value_get_uint (v)
#define g_marshal_value_peek_long(v)     g_value_get_long (v)
#define g_marshal_value_peek_ulong(v)    g_value_get_ulong (v)
#define g_marshal_value_peek_int64(v)    g_value_get_int64 (v)
#define g_marshal_value_peek_uint64(v)   g_value_get_uint64 (v)
#define g_marshal_value_peek_enum(v)     g_value_get_enum (v)
#define g_marshal_value_peek_flags(v)    g_value_get_flags (v)
#define g_marshal_value_peek_float(v)    g_value_get_float (v)
#define g_marshal_value_peek_double(v)   g_value_get_double (v)
#define g_marshal_value_peek_string(v)   (char*) g_value_get_string (v)
#define g_marshal_value_peek_param(v)    g_value_get_param (v)
#define g_marshal_value_peek_boxed(v)    g_value_get_boxed (v)
#define g_marshal_value_peek_pointer(v)  g_value_get_pointer (v)
#define g_marshal_value_peek_object(v)   g_value_get_object (v)
#define g_marshal_value_peek_variant(v)  g_value_get_variant (v)
#else /* !G_ENABLE_DEBUG */
/* WARNING: This code accesses GValues directly, which is UNSUPPORTED API.
 *          Do not access GValues directly in your code. Instead, use the
 *          g_value_get_*() functions
 */
#define g_marshal_value_peek_boolean(v)  (v)->data[0].v_int
#define g_marshal_value_peek_char(v)     (v)->data[0].v_int
#define g_marshal_value_peek_uchar(v)    (v)->data[0].v_uint
#define g_marshal_value_peek_int(v)      (v)->data[0].v_int
#define g_marshal_value_peek_uint(v)     (v)->data[0].v_uint
#define g_marshal_value_peek_long(v)     (v)->data[0].v_long
#define g_marshal_value_peek_ulong(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_int64(v)    (v)->data[0].v_int64
#define g_marshal_value_peek_uint64(v)   (v)->data[0].v_uint64
#define g_marshal_value_peek_enum(v)     (v)->data[0].v_long
#define g_marshal_value_peek_flags(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_float(v)    (v)->data[0].v_float
#define g_marshal_value_peek_double(v)   (v)->data[0].v_double
#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_param(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_boxed(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer
#define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_variant(v)  (v)->data[0].v_pointer
#endif /* !G_ENABLE_DEBUG */


/**
 * g_cclosure_marshal_VOID__VOID:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with no arguments.
 */
/* VOID:VOID */
void
g_cclosure_marshal_VOID__VOID (GClosure     *closure,
                               GValue       *return_value G_GNUC_UNUSED,
                               xuint_t         n_param_values,
                               const GValue *param_values,
                               xpointer_t      invocation_hint G_GNUC_UNUSED,
                               xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__VOID) (xpointer_t     data1,
                                           xpointer_t     data2);
  GMarshalFunc_VOID__VOID callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 1);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__VOID) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            data2);
}

/**
 * g_cclosure_marshal_VOID__VOIDv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__VOID().
 */
void
g_cclosure_marshal_VOID__VOIDv (GClosure     *closure,
                                GValue       *return_value,
                                xpointer_t      instance,
                                va_list       args,
                                xpointer_t      marshal_data,
                                int           n_params,
                                xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__VOID) (xpointer_t     instance,
                                           xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__VOID callback;

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__VOID) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            data2);
}

/**
 * g_cclosure_marshal_VOID__BOOLEAN:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single
 * boolean argument.
 */
/* VOID:BOOLEAN */
void
g_cclosure_marshal_VOID__BOOLEAN (GClosure     *closure,
                                  GValue       *return_value G_GNUC_UNUSED,
                                  xuint_t         n_param_values,
                                  const GValue *param_values,
                                  xpointer_t      invocation_hint G_GNUC_UNUSED,
                                  xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__BOOLEAN) (xpointer_t     data1,
                                              xboolean_t     arg_1,
                                              xpointer_t     data2);
  GMarshalFunc_VOID__BOOLEAN callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__BOOLEAN) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_boolean (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__BOOLEANv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__BOOLEAN().
 */
void
g_cclosure_marshal_VOID__BOOLEANv (GClosure     *closure,
                                   GValue       *return_value,
                                   xpointer_t      instance,
                                   va_list       args,
                                   xpointer_t      marshal_data,
                                   int           n_params,
                                   xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__BOOLEAN) (xpointer_t     instance,
                                              xboolean_t     arg_0,
                                              xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__BOOLEAN callback;
  xboolean_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xboolean_t) va_arg (args_copy, xboolean_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__BOOLEAN) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
}

/**
 * g_cclosure_marshal_VOID__CHAR:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single
 * character argument.
 */
/* VOID:CHAR */
void
g_cclosure_marshal_VOID__CHAR (GClosure     *closure,
                               GValue       *return_value G_GNUC_UNUSED,
                               xuint_t         n_param_values,
                               const GValue *param_values,
                               xpointer_t      invocation_hint G_GNUC_UNUSED,
                               xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__CHAR) (xpointer_t     data1,
                                           xchar_t        arg_1,
                                           xpointer_t     data2);
  GMarshalFunc_VOID__CHAR callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__CHAR) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_char (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__CHARv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__CHAR().
 */
void
g_cclosure_marshal_VOID__CHARv (GClosure     *closure,
                                GValue       *return_value,
                                xpointer_t      instance,
                                va_list       args,
                                xpointer_t      marshal_data,
                                int           n_params,
                                xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__CHAR) (xpointer_t     instance,
                                           xchar_t        arg_0,
                                           xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__CHAR callback;
  xchar_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xchar_t) va_arg (args_copy, xint_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__CHAR) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
}

/**
 * g_cclosure_marshal_VOID__UCHAR:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single
 * unsigned character argument.
 */
/* VOID:UCHAR */
void
g_cclosure_marshal_VOID__UCHAR (GClosure     *closure,
                                GValue       *return_value G_GNUC_UNUSED,
                                xuint_t         n_param_values,
                                const GValue *param_values,
                                xpointer_t      invocation_hint G_GNUC_UNUSED,
                                xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__UCHAR) (xpointer_t     data1,
                                            guchar       arg_1,
                                            xpointer_t     data2);
  GMarshalFunc_VOID__UCHAR callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UCHAR) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_uchar (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__UCHARv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__UCHAR().
 */
void
g_cclosure_marshal_VOID__UCHARv (GClosure     *closure,
                                 GValue       *return_value,
                                 xpointer_t      instance,
                                 va_list       args,
                                 xpointer_t      marshal_data,
                                 int           n_params,
                                 xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__UCHAR) (xpointer_t     instance,
                                            guchar       arg_0,
                                            xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__UCHAR callback;
  guchar arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (guchar) va_arg (args_copy, xuint_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UCHAR) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
}

/**
 * g_cclosure_marshal_VOID__INT:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single
 * integer argument.
 */
/* VOID:INT */
void
g_cclosure_marshal_VOID__INT (GClosure     *closure,
                              GValue       *return_value G_GNUC_UNUSED,
                              xuint_t         n_param_values,
                              const GValue *param_values,
                              xpointer_t      invocation_hint G_GNUC_UNUSED,
                              xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT) (xpointer_t     data1,
                                          xint_t         arg_1,
                                          xpointer_t     data2);
  GMarshalFunc_VOID__INT callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_int (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__INTv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__INT().
 */
void
g_cclosure_marshal_VOID__INTv (GClosure     *closure,
                               GValue       *return_value,
                               xpointer_t      instance,
                               va_list       args,
                               xpointer_t      marshal_data,
                               int           n_params,
                               xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__INT) (xpointer_t     instance,
                                          xint_t         arg_0,
                                          xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__INT callback;
  xint_t arg0;
  va_list args_copy;

 G_VA_COPY (args_copy, args);
  arg0 = (xint_t) va_arg (args_copy, xint_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
}

/**
 * g_cclosure_marshal_VOID__UINT:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with with a single
 * unsigned integer argument.
 */
/* VOID:UINT */
void
g_cclosure_marshal_VOID__UINT (GClosure     *closure,
                               GValue       *return_value G_GNUC_UNUSED,
                               xuint_t         n_param_values,
                               const GValue *param_values,
                               xpointer_t      invocation_hint G_GNUC_UNUSED,
                               xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__UINT) (xpointer_t     data1,
                                           xuint_t        arg_1,
                                           xpointer_t     data2);
  GMarshalFunc_VOID__UINT callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UINT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_uint (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__UINTv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__UINT().
 */
void
g_cclosure_marshal_VOID__UINTv (GClosure     *closure,
                                GValue       *return_value,
                                xpointer_t      instance,
                                va_list       args,
                                xpointer_t      marshal_data,
                                int           n_params,
                                xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__UINT) (xpointer_t     instance,
                                           xuint_t        arg_0,
                                           xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__UINT callback;
  xuint_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xuint_t) va_arg (args_copy, xuint_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UINT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
}

/**
 * g_cclosure_marshal_VOID__LONG:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with with a single
 * long integer argument.
 */
/* VOID:LONG */
void
g_cclosure_marshal_VOID__LONG (GClosure     *closure,
                               GValue       *return_value G_GNUC_UNUSED,
                               xuint_t         n_param_values,
                               const GValue *param_values,
                               xpointer_t      invocation_hint G_GNUC_UNUSED,
                               xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__LONG) (xpointer_t     data1,
                                           glong        arg_1,
                                           xpointer_t     data2);
  GMarshalFunc_VOID__LONG callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__LONG) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_long (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__LONGv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__LONG().
 */
void
g_cclosure_marshal_VOID__LONGv (GClosure     *closure,
                                GValue       *return_value,
                                xpointer_t      instance,
                                va_list       args,
                                xpointer_t      marshal_data,
                                int           n_params,
                                xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__LONG) (xpointer_t     instance,
                                           glong        arg_0,
                                           xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__LONG callback;
  glong arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (glong) va_arg (args_copy, glong);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__LONG) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
}

/**
 * g_cclosure_marshal_VOID__ULONG:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single
 * unsigned long integer argument.
 */
/* VOID:ULONG */
void
g_cclosure_marshal_VOID__ULONG (GClosure     *closure,
                                GValue       *return_value G_GNUC_UNUSED,
                                xuint_t         n_param_values,
                                const GValue *param_values,
                                xpointer_t      invocation_hint G_GNUC_UNUSED,
                                xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__ULONG) (xpointer_t     data1,
                                            gulong       arg_1,
                                            xpointer_t     data2);
  GMarshalFunc_VOID__ULONG callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__ULONG) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_ulong (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__ULONGv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__ULONG().
 */
void
g_cclosure_marshal_VOID__ULONGv (GClosure     *closure,
                                 GValue       *return_value,
                                 xpointer_t      instance,
                                 va_list       args,
                                 xpointer_t      marshal_data,
                                 int           n_params,
                                 xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__ULONG) (xpointer_t     instance,
                                            gulong       arg_0,
                                            xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__ULONG callback;
  gulong arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (gulong) va_arg (args_copy, gulong);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__ULONG) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
}

/**
 * g_cclosure_marshal_VOID__ENUM:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single
 * argument with an enumerated type.
 */
/* VOID:ENUM */
void
g_cclosure_marshal_VOID__ENUM (GClosure     *closure,
                               GValue       *return_value G_GNUC_UNUSED,
                               xuint_t         n_param_values,
                               const GValue *param_values,
                               xpointer_t      invocation_hint G_GNUC_UNUSED,
                               xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__ENUM) (xpointer_t     data1,
                                           xint_t         arg_1,
                                           xpointer_t     data2);
  GMarshalFunc_VOID__ENUM callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__ENUM) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_enum (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__ENUMv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__ENUM().
 */
void
g_cclosure_marshal_VOID__ENUMv (GClosure     *closure,
                                GValue       *return_value,
                                xpointer_t      instance,
                                va_list       args,
                                xpointer_t      marshal_data,
                                int           n_params,
                                xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__ENUM) (xpointer_t     instance,
                                           xint_t         arg_0,
                                           xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__ENUM callback;
  xint_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xint_t) va_arg (args_copy, xint_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__ENUM) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
}

/**
 * g_cclosure_marshal_VOID__FLAGS:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single
 * argument with a flags types.
 */
/* VOID:FLAGS */
void
g_cclosure_marshal_VOID__FLAGS (GClosure     *closure,
                                GValue       *return_value G_GNUC_UNUSED,
                                xuint_t         n_param_values,
                                const GValue *param_values,
                                xpointer_t      invocation_hint G_GNUC_UNUSED,
                                xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__FLAGS) (xpointer_t     data1,
                                            xuint_t        arg_1,
                                            xpointer_t     data2);
  GMarshalFunc_VOID__FLAGS callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__FLAGS) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_flags (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__FLAGSv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__FLAGS().
 */
void
g_cclosure_marshal_VOID__FLAGSv (GClosure     *closure,
                                 GValue       *return_value,
                                 xpointer_t      instance,
                                 va_list       args,
                                 xpointer_t      marshal_data,
                                 int           n_params,
                                 xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__FLAGS) (xpointer_t     instance,
                                            xuint_t        arg_0,
                                            xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__FLAGS callback;
  xuint_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xuint_t) va_arg (args_copy, xuint_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__FLAGS) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
}

/**
 * g_cclosure_marshal_VOID__FLOAT:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with one
 * single-precision floating point argument.
 */
/* VOID:FLOAT */
void
g_cclosure_marshal_VOID__FLOAT (GClosure     *closure,
                                GValue       *return_value G_GNUC_UNUSED,
                                xuint_t         n_param_values,
                                const GValue *param_values,
                                xpointer_t      invocation_hint G_GNUC_UNUSED,
                                xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__FLOAT) (xpointer_t     data1,
                                            gfloat       arg_1,
                                            xpointer_t     data2);
  GMarshalFunc_VOID__FLOAT callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__FLOAT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_float (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__FLOATv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__FLOAT().
 */
void
g_cclosure_marshal_VOID__FLOATv (GClosure     *closure,
                                 GValue       *return_value,
                                 xpointer_t      instance,
                                 va_list       args,
                                 xpointer_t      marshal_data,
                                 int           n_params,
                                 xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__FLOAT) (xpointer_t     instance,
                                            gfloat       arg_0,
                                            xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__FLOAT callback;
  gfloat arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (gfloat) va_arg (args_copy, xdouble_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__FLOAT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
}

/**
 * g_cclosure_marshal_VOID__DOUBLE:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with one
 * double-precision floating point argument.
 */
/* VOID:DOUBLE */
void
g_cclosure_marshal_VOID__DOUBLE (GClosure     *closure,
                                 GValue       *return_value G_GNUC_UNUSED,
                                 xuint_t         n_param_values,
                                 const GValue *param_values,
                                 xpointer_t      invocation_hint G_GNUC_UNUSED,
                                 xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__DOUBLE) (xpointer_t     data1,
                                             xdouble_t      arg_1,
                                             xpointer_t     data2);
  GMarshalFunc_VOID__DOUBLE callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__DOUBLE) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_double (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__DOUBLEv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__DOUBLE().
 */
void
g_cclosure_marshal_VOID__DOUBLEv (GClosure     *closure,
                                  GValue       *return_value,
                                  xpointer_t      instance,
                                  va_list       args,
                                  xpointer_t      marshal_data,
                                  int           n_params,
                                  xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__DOUBLE) (xpointer_t     instance,
                                             xdouble_t      arg_0,
                                             xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__DOUBLE callback;
  xdouble_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xdouble_t) va_arg (args_copy, xdouble_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__DOUBLE) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
}

/**
 * g_cclosure_marshal_VOID__STRING:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single string
 * argument.
 */
/* VOID:STRING */
void
g_cclosure_marshal_VOID__STRING (GClosure     *closure,
                                 GValue       *return_value G_GNUC_UNUSED,
                                 xuint_t         n_param_values,
                                 const GValue *param_values,
                                 xpointer_t      invocation_hint G_GNUC_UNUSED,
                                 xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING) (xpointer_t     data1,
                                             xpointer_t     arg_1,
                                             xpointer_t     data2);
  GMarshalFunc_VOID__STRING callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__STRINGv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__STRING().
 */
void
g_cclosure_marshal_VOID__STRINGv (GClosure     *closure,
                                  GValue       *return_value,
                                  xpointer_t      instance,
                                  va_list       args,
                                  xpointer_t      marshal_data,
                                  int           n_params,
                                  xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__STRING) (xpointer_t     instance,
                                             xpointer_t     arg_0,
                                             xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING callback;
  xpointer_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = g_strdup (arg0);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_free (arg0);
}

/**
 * g_cclosure_marshal_VOID__PARAM:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single
 * argument of type #GParamSpec.
 */
/* VOID:PARAM */
void
g_cclosure_marshal_VOID__PARAM (GClosure     *closure,
                                GValue       *return_value G_GNUC_UNUSED,
                                xuint_t         n_param_values,
                                const GValue *param_values,
                                xpointer_t      invocation_hint G_GNUC_UNUSED,
                                xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__PARAM) (xpointer_t     data1,
                                            xpointer_t     arg_1,
                                            xpointer_t     data2);
  GMarshalFunc_VOID__PARAM callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__PARAM) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_param (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__PARAMv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__PARAM().
 */
void
g_cclosure_marshal_VOID__PARAMv (GClosure     *closure,
                                 GValue       *return_value,
                                 xpointer_t      instance,
                                 va_list       args,
                                 xpointer_t      marshal_data,
                                 int           n_params,
                                 xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__PARAM) (xpointer_t     instance,
                                            xpointer_t     arg_0,
                                            xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__PARAM callback;
  xpointer_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = g_param_spec_ref (arg0);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__PARAM) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_param_spec_unref (arg0);
}

/**
 * g_cclosure_marshal_VOID__BOXED:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single
 * argument which is any boxed pointer type.
 */
/* VOID:BOXED */
void
g_cclosure_marshal_VOID__BOXED (GClosure     *closure,
                                GValue       *return_value G_GNUC_UNUSED,
                                xuint_t         n_param_values,
                                const GValue *param_values,
                                xpointer_t      invocation_hint G_GNUC_UNUSED,
                                xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__BOXED) (xpointer_t     data1,
                                            xpointer_t     arg_1,
                                            xpointer_t     data2);
  GMarshalFunc_VOID__BOXED callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__BOXED) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_boxed (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__BOXEDv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__BOXED().
 */
void
g_cclosure_marshal_VOID__BOXEDv (GClosure     *closure,
                                 GValue       *return_value,
                                 xpointer_t      instance,
                                 va_list       args,
                                 xpointer_t      marshal_data,
                                 int           n_params,
                                 xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__BOXED) (xpointer_t     instance,
                                            xpointer_t     arg_0,
                                            xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__BOXED callback;
  xpointer_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = g_boxed_copy (param_types[0] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg0);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__BOXED) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_boxed_free (param_types[0] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg0);
}

/**
 * g_cclosure_marshal_VOID__POINTER:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single raw
 * pointer argument type.
 *
 * If it is possible, it is better to use one of the more specific
 * functions such as g_cclosure_marshal_VOID__OBJECT() or
 * g_cclosure_marshal_VOID__OBJECT().
 */
/* VOID:POINTER */
void
g_cclosure_marshal_VOID__POINTER (GClosure     *closure,
                                  GValue       *return_value G_GNUC_UNUSED,
                                  xuint_t         n_param_values,
                                  const GValue *param_values,
                                  xpointer_t      invocation_hint G_GNUC_UNUSED,
                                  xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER) (xpointer_t     data1,
                                              xpointer_t     arg_1,
                                              xpointer_t     data2);
  GMarshalFunc_VOID__POINTER callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_pointer (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__POINTERv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__POINTER().
 */
void
g_cclosure_marshal_VOID__POINTERv (GClosure     *closure,
                                   GValue       *return_value,
                                   xpointer_t      instance,
                                   va_list       args,
                                   xpointer_t      marshal_data,
                                   int           n_params,
                                   xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__POINTER) (xpointer_t     instance,
                                              xpointer_t     arg_0,
                                              xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__POINTER callback;
  xpointer_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
}

/**
 * g_cclosure_marshal_VOID__OBJECT:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single
 * #xobject_t argument.
 */
/* VOID:OBJECT */
void
g_cclosure_marshal_VOID__OBJECT (GClosure     *closure,
                                 GValue       *return_value G_GNUC_UNUSED,
                                 xuint_t         n_param_values,
                                 const GValue *param_values,
                                 xpointer_t      invocation_hint G_GNUC_UNUSED,
                                 xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__OBJECT) (xpointer_t     data1,
                                             xpointer_t     arg_1,
                                             xpointer_t     data2);
  GMarshalFunc_VOID__OBJECT callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__OBJECT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_object (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__OBJECTv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__OBJECT().
 */
void
g_cclosure_marshal_VOID__OBJECTv (GClosure     *closure,
                                  GValue       *return_value,
                                  xpointer_t      instance,
                                  va_list       args,
                                  xpointer_t      marshal_data,
                                  int           n_params,
                                  xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__OBJECT) (xpointer_t     instance,
                                             xpointer_t     arg_0,
                                             xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__OBJECT callback;
  xpointer_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg0 != NULL)
    arg0 = g_object_ref (arg0);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__OBJECT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
  if (arg0 != NULL)
    g_object_unref (arg0);
}

/**
 * g_cclosure_marshal_VOID__VARIANT:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with a single
 * #xvariant_t argument.
 */
/* VOID:VARIANT */
void
g_cclosure_marshal_VOID__VARIANT (GClosure     *closure,
                                  GValue       *return_value G_GNUC_UNUSED,
                                  xuint_t         n_param_values,
                                  const GValue *param_values,
                                  xpointer_t      invocation_hint G_GNUC_UNUSED,
                                  xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__VARIANT) (xpointer_t     data1,
                                              xpointer_t     arg_1,
                                              xpointer_t     data2);
  GMarshalFunc_VOID__VARIANT callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__VARIANT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_variant (param_values + 1),
            data2);
}

/**
 * g_cclosure_marshal_VOID__VARIANTv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__VARIANT().
 */
void
g_cclosure_marshal_VOID__VARIANTv (GClosure     *closure,
                                   GValue       *return_value,
                                   xpointer_t      instance,
                                   va_list       args,
                                   xpointer_t      marshal_data,
                                   int           n_params,
                                   xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__VARIANT) (xpointer_t     instance,
                                              xpointer_t     arg_0,
                                              xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__VARIANT callback;
  xpointer_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = g_variant_ref_sink (arg0);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__VARIANT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_variant_unref (arg0);
}

/**
 * g_cclosure_marshal_VOID__UINT_POINTER:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with an unsigned int
 * and a pointer as arguments.
 */
/* VOID:UINT,POINTER */
void
g_cclosure_marshal_VOID__UINT_POINTER (GClosure     *closure,
                                       GValue       *return_value G_GNUC_UNUSED,
                                       xuint_t         n_param_values,
                                       const GValue *param_values,
                                       xpointer_t      invocation_hint G_GNUC_UNUSED,
                                       xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__UINT_POINTER) (xpointer_t     data1,
                                                   xuint_t        arg_1,
                                                   xpointer_t     arg_2,
                                                   xpointer_t     data2);
  GMarshalFunc_VOID__UINT_POINTER callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UINT_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_uint (param_values + 1),
            g_marshal_value_peek_pointer (param_values + 2),
            data2);
}

/**
 * g_cclosure_marshal_VOID__UINT_POINTERv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_VOID__UINT_POINTER().
 */
void
g_cclosure_marshal_VOID__UINT_POINTERv (GClosure     *closure,
                                        GValue       *return_value,
                                        xpointer_t      instance,
                                        va_list       args,
                                        xpointer_t      marshal_data,
                                        int           n_params,
                                        xtype_t        *param_types)
{
  typedef void (*GMarshalFunc_VOID__UINT_POINTER) (xpointer_t     instance,
                                                   xuint_t        arg_0,
                                                   xpointer_t     arg_1,
                                                   xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__UINT_POINTER callback;
  xuint_t arg0;
  xpointer_t arg1;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xuint_t) va_arg (args_copy, xuint_t);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UINT_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            data2);
}

/**
 * g_cclosure_marshal_BOOLEAN__FLAGS:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with handlers that
 * take a flags type as an argument and return a boolean.  If you have
 * such a signal, you will probably also need to use an accumulator,
 * such as g_signal_accumulator_true_handled().
 */
/* BOOL:FLAGS */
void
g_cclosure_marshal_BOOLEAN__FLAGS (GClosure     *closure,
                                   GValue       *return_value G_GNUC_UNUSED,
                                   xuint_t         n_param_values,
                                   const GValue *param_values,
                                   xpointer_t      invocation_hint G_GNUC_UNUSED,
                                   xpointer_t      marshal_data)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__FLAGS) (xpointer_t     data1,
                                                   xuint_t        arg_1,
                                                   xpointer_t     data2);
  GMarshalFunc_BOOLEAN__FLAGS callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__FLAGS) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_flags (param_values + 1),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/**
 * g_cclosure_marshal_BOOLEAN__FLAGSv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_BOOLEAN__FLAGS().
 */
void
g_cclosure_marshal_BOOLEAN__FLAGSv (GClosure     *closure,
                                    GValue       *return_value,
                                    xpointer_t      instance,
                                    va_list       args,
                                    xpointer_t      marshal_data,
                                    int           n_params,
                                    xtype_t        *param_types)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__FLAGS) (xpointer_t     instance,
                                                   xuint_t        arg_0,
                                                   xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__FLAGS callback;
  xuint_t arg0;
  va_list args_copy;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);

  G_VA_COPY (args_copy, args);
  arg0 = (xuint_t) va_arg (args_copy, xuint_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__FLAGS) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       arg0,
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/**
 * g_cclosure_marshal_STRING__OBJECT_POINTER:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with handlers that
 * take a #xobject_t and a pointer and produce a string.  It is highly
 * unlikely that your signal handler fits this description.
 */
/* STRING:OBJECT,POINTER */
void
g_cclosure_marshal_STRING__OBJECT_POINTER (GClosure     *closure,
                                           GValue       *return_value G_GNUC_UNUSED,
                                           xuint_t         n_param_values,
                                           const GValue *param_values,
                                           xpointer_t      invocation_hint G_GNUC_UNUSED,
                                           xpointer_t      marshal_data)
{
  typedef xchar_t* (*GMarshalFunc_STRING__OBJECT_POINTER) (xpointer_t     data1,
                                                         xpointer_t     arg_1,
                                                         xpointer_t     arg_2,
                                                         xpointer_t     data2);
  GMarshalFunc_STRING__OBJECT_POINTER callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  xchar_t* v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_STRING__OBJECT_POINTER) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_object (param_values + 1),
                       g_marshal_value_peek_pointer (param_values + 2),
                       data2);

  g_value_take_string (return_value, v_return);
}

/**
 * g_cclosure_marshal_STRING__OBJECT_POINTERv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_STRING__OBJECT_POINTER().
 */
void
g_cclosure_marshal_STRING__OBJECT_POINTERv (GClosure     *closure,
                                            GValue       *return_value,
                                            xpointer_t      instance,
                                            va_list       args,
                                            xpointer_t      marshal_data,
                                            int           n_params,
                                            xtype_t        *param_types)
{
  typedef xchar_t* (*GMarshalFunc_STRING__OBJECT_POINTER) (xpointer_t     instance,
                                                         xpointer_t     arg_0,
                                                         xpointer_t     arg_1,
                                                         xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_STRING__OBJECT_POINTER callback;
  xpointer_t arg0;
  xpointer_t arg1;
  va_list args_copy;
  xchar_t* v_return;

  g_return_if_fail (return_value != NULL);

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg0 != NULL)
    arg0 = g_object_ref (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_STRING__OBJECT_POINTER) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       arg0,
                       arg1,
                       data2);
  if (arg0 != NULL)
    g_object_unref (arg0);

  g_value_take_string (return_value, v_return);
}

/**
 * g_cclosure_marshal_BOOLEAN__BOXED_BOXED:
 * @closure: A #GClosure.
 * @return_value: A #GValue to store the return value. May be %NULL
 *   if the callback of closure doesn't return a value.
 * @n_param_values: The length of the @param_values array.
 * @param_values: An array of #GValues holding the arguments
 *   on which to invoke the callback of closure.
 * @invocation_hint: The invocation hint given as the last argument to
 *   g_closure_invoke().
 * @marshal_data: Additional data specified when registering the
 *   marshaller, see g_closure_set_marshal() and
 *   g_closure_set_meta_marshal()
 *
 * A #GClosureMarshal function for use with signals with handlers that
 * take two boxed pointers as arguments and return a boolean.  If you
 * have such a signal, you will probably also need to use an
 * accumulator, such as g_signal_accumulator_true_handled().
 */
/* BOOL:BOXED,BOXED */
void
g_cclosure_marshal_BOOLEAN__BOXED_BOXED (GClosure     *closure,
                                         GValue       *return_value G_GNUC_UNUSED,
                                         xuint_t         n_param_values,
                                         const GValue *param_values,
                                         xpointer_t      invocation_hint G_GNUC_UNUSED,
                                         xpointer_t      marshal_data)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__BOXED_BOXED) (xpointer_t     data1,
                                                         xpointer_t     arg_1,
                                                         xpointer_t     arg_2,
                                                         xpointer_t     data2);
  GMarshalFunc_BOOLEAN__BOXED_BOXED callback;
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__BOXED_BOXED) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_boxed (param_values + 1),
                       g_marshal_value_peek_boxed (param_values + 2),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/**
 * g_cclosure_marshal_BOOLEAN__BOXED_BOXEDv:
 * @closure: the #GClosure to which the marshaller belongs
 * @return_value: (nullable): a #GValue to store the return
 *  value. May be %NULL if the callback of @closure doesn't return a
 *  value.
 * @instance: (type xobject_t.TypeInstance): the instance on which the closure is invoked.
 * @args: va_list of arguments to be passed to the closure.
 * @marshal_data: (nullable): additional data specified when
 *  registering the marshaller, see g_closure_set_marshal() and
 *  g_closure_set_meta_marshal()
 * @n_params: the length of the @param_types array
 * @param_types: (array length=n_params): the #xtype_t of each argument from
 *  @args.
 *
 * The #GVaClosureMarshal equivalent to g_cclosure_marshal_BOOLEAN__BOXED_BOXED().
 */
void
g_cclosure_marshal_BOOLEAN__BOXED_BOXEDv (GClosure     *closure,
                                          GValue       *return_value,
                                          xpointer_t      instance,
                                          va_list       args,
                                          xpointer_t      marshal_data,
                                          int           n_params,
                                          xtype_t        *param_types)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__BOXED_BOXED) (xpointer_t     instance,
                                                         xpointer_t     arg_0,
                                                         xpointer_t     arg_1,
                                                         xpointer_t     data);
  GCClosure *cc = (GCClosure*) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__BOXED_BOXED callback;
  xpointer_t arg0;
  xpointer_t arg1;
  va_list args_copy;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = g_boxed_copy (param_types[0] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    arg1 = g_boxed_copy (param_types[1] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg1);
  va_end (args_copy);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = instance;
    }
  else
    {
      data1 = instance;
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__BOXED_BOXED) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       arg0,
                       arg1,
                       data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_boxed_free (param_types[0] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg0);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    g_boxed_free (param_types[1] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg1);

  g_value_set_boolean (return_value, v_return);
}
