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

#include <glib-object.h>
#include "gmarshal-internal.h"

#ifdef G_ENABLE_DEBUG
#define g_marshal_value_peek_boolean(v)  xvalue_get_boolean (v)
#define g_marshal_value_peek_char(v)     xvalue_get_schar (v)
#define g_marshal_value_peek_uchar(v)    xvalue_get_uchar (v)
#define g_marshal_value_peek_int(v)      xvalue_get_int (v)
#define g_marshal_value_peek_uint(v)     xvalue_get_uint (v)
#define g_marshal_value_peek_long(v)     xvalue_get_long (v)
#define g_marshal_value_peek_ulong(v)    xvalue_get_ulong (v)
#define g_marshal_value_peek_int64(v)    xvalue_get_int64 (v)
#define g_marshal_value_peek_uint64(v)   xvalue_get_uint64 (v)
#define g_marshal_value_peek_enum(v)     xvalue_get_enum (v)
#define g_marshal_value_peek_flags(v)    xvalue_get_flags (v)
#define g_marshal_value_peek_float(v)    xvalue_get_float (v)
#define g_marshal_value_peek_double(v)   xvalue_get_double (v)
#define g_marshal_value_peek_string(v)   (char*) xvalue_get_string (v)
#define g_marshal_value_peek_param(v)    xvalue_get_param (v)
#define g_marshal_value_peek_boxed(v)    xvalue_get_boxed (v)
#define g_marshal_value_peek_pointer(v)  xvalue_get_pointer (v)
#define g_marshal_value_peek_object(v)   xvalue_get_object (v)
#define g_marshal_value_peek_variant(v)  xvalue_get_variant (v)
#else /* !G_ENABLE_DEBUG */
/* WARNING: This code accesses GValues directly, which is UNSUPPORTED API.
 *          Do not access GValues directly in your code. Instead, use the
 *          xvalue_get_*() functions
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

/* BOOLEAN:OBJECT */
void
_g_cclosure_marshal_BOOLEAN__OBJECT (xclosure_t     *closure,
                                     xvalue_t       *return_value,
                                     xuint_t         n_param_values,
                                     const xvalue_t *param_values,
                                     xpointer_t      invocation_hint G_GNUC_UNUSED,
                                     xpointer_t      marshal_data)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__OBJECT) (xpointer_t data1,
                                                    xpointer_t arg1,
                                                    xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__OBJECT callback;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__OBJECT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_object (param_values + 1),
                       data2);

  xvalue_set_boolean (return_value, v_return);
}

void
_g_cclosure_marshal_BOOLEAN__OBJECTv (xclosure_t *closure,
                                      xvalue_t   *return_value,
                                      xpointer_t  instance,
                                      va_list   args,
                                      xpointer_t  marshal_data,
                                      int       n_params,
                                      xtype_t    *param_types)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__OBJECT) (xpointer_t data1,
                                                    xpointer_t arg1,
                                                    xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__OBJECT callback;
  xboolean_t v_return;
  xpointer_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg0 != NULL)
    arg0 = xobject_ref (arg0);
  va_end (args_copy);

  g_return_if_fail (return_value != NULL);

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
  callback = (GMarshalFunc_BOOLEAN__OBJECT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       arg0,
                       data2);
  if (arg0 != NULL)
    xobject_unref (arg0);

  xvalue_set_boolean (return_value, v_return);
}

/* BOOLEAN:OBJECT,FLAGS */
void
_g_cclosure_marshal_BOOLEAN__OBJECT_FLAGS (xclosure_t     *closure,
                                           xvalue_t       *return_value,
                                           xuint_t         n_param_values,
                                           const xvalue_t *param_values,
                                           xpointer_t      invocation_hint G_GNUC_UNUSED,
                                           xpointer_t      marshal_data)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__OBJECT_FLAGS) (xpointer_t data1,
                                                          xpointer_t arg1,
                                                          xuint_t arg2,
                                                          xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__OBJECT_FLAGS callback;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__OBJECT_FLAGS) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_object (param_values + 1),
                       g_marshal_value_peek_flags (param_values + 2),
                       data2);

  xvalue_set_boolean (return_value, v_return);
}

void
_g_cclosure_marshal_BOOLEAN__OBJECT_FLAGSv (xclosure_t *closure,
                                            xvalue_t   *return_value,
                                            xpointer_t  instance,
                                            va_list   args,
                                            xpointer_t  marshal_data,
                                            int       n_params,
                                            xtype_t    *param_types)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__OBJECT_FLAGS) (xpointer_t data1,
                                                          xpointer_t arg1,
                                                          xuint_t arg2,
                                                          xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__OBJECT_FLAGS callback;
  xboolean_t v_return;
  xpointer_t arg0;
  xuint_t arg1;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg0 != NULL)
    arg0 = xobject_ref (arg0);
  arg1 = (xuint_t) va_arg (args_copy, xuint_t);
  va_end (args_copy);

  g_return_if_fail (return_value != NULL);

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
  callback = (GMarshalFunc_BOOLEAN__OBJECT_FLAGS) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       arg0,
                       arg1,
                       data2);
  if (arg0 != NULL)
    xobject_unref (arg0);

  xvalue_set_boolean (return_value, v_return);
}

/* BOOLEAN:OBJECT,OBJECT */
void
_g_cclosure_marshal_BOOLEAN__OBJECT_OBJECT (xclosure_t     *closure,
                                            xvalue_t       *return_value,
                                            xuint_t         n_param_values,
                                            const xvalue_t *param_values,
                                            xpointer_t      invocation_hint G_GNUC_UNUSED,
                                            xpointer_t      marshal_data)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__OBJECT_OBJECT) (xpointer_t data1,
                                                           xpointer_t arg1,
                                                           xpointer_t arg2,
                                                           xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__OBJECT_OBJECT callback;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__OBJECT_OBJECT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_object (param_values + 1),
                       g_marshal_value_peek_object (param_values + 2),
                       data2);

  xvalue_set_boolean (return_value, v_return);
}

void
_g_cclosure_marshal_BOOLEAN__OBJECT_OBJECTv (xclosure_t *closure,
                                             xvalue_t   *return_value,
                                             xpointer_t  instance,
                                             va_list   args,
                                             xpointer_t  marshal_data,
                                             int       n_params,
                                             xtype_t    *param_types)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__OBJECT_OBJECT) (xpointer_t data1,
                                                           xpointer_t arg1,
                                                           xpointer_t arg2,
                                                           xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__OBJECT_OBJECT callback;
  xboolean_t v_return;
  xpointer_t arg0;
  xpointer_t arg1;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg0 != NULL)
    arg0 = xobject_ref (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg1 != NULL)
    arg1 = xobject_ref (arg1);
  va_end (args_copy);

  g_return_if_fail (return_value != NULL);

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
  callback = (GMarshalFunc_BOOLEAN__OBJECT_OBJECT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       arg0,
                       arg1,
                       data2);
  if (arg0 != NULL)
    xobject_unref (arg0);
  if (arg1 != NULL)
    xobject_unref (arg1);

  xvalue_set_boolean (return_value, v_return);
}

/* BOOLEAN:POINTER,INT */
void
_g_cclosure_marshal_BOOLEAN__POINTER_INT (xclosure_t     *closure,
                                          xvalue_t       *return_value,
                                          xuint_t         n_param_values,
                                          const xvalue_t *param_values,
                                          xpointer_t      invocation_hint G_GNUC_UNUSED,
                                          xpointer_t      marshal_data)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__POINTER_INT) (xpointer_t data1,
                                                         xpointer_t arg1,
                                                         xint_t arg2,
                                                         xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__POINTER_INT callback;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__POINTER_INT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_pointer (param_values + 1),
                       g_marshal_value_peek_int (param_values + 2),
                       data2);

  xvalue_set_boolean (return_value, v_return);
}

void
_g_cclosure_marshal_BOOLEAN__POINTER_INTv (xclosure_t *closure,
                                           xvalue_t   *return_value,
                                           xpointer_t  instance,
                                           va_list   args,
                                           xpointer_t  marshal_data,
                                           int       n_params,
                                           xtype_t    *param_types)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__POINTER_INT) (xpointer_t data1,
                                                         xpointer_t arg1,
                                                         xint_t arg2,
                                                         xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__POINTER_INT callback;
  xboolean_t v_return;
  xpointer_t arg0;
  xint_t arg1;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  arg1 = (xint_t) va_arg (args_copy, xint_t);
  va_end (args_copy);

  g_return_if_fail (return_value != NULL);

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
  callback = (GMarshalFunc_BOOLEAN__POINTER_INT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       arg0,
                       arg1,
                       data2);


  xvalue_set_boolean (return_value, v_return);
}

/* BOOLEAN:STRING */
void
_g_cclosure_marshal_BOOLEAN__STRING (xclosure_t     *closure,
                                     xvalue_t       *return_value,
                                     xuint_t         n_param_values,
                                     const xvalue_t *param_values,
                                     xpointer_t      invocation_hint G_GNUC_UNUSED,
                                     xpointer_t      marshal_data)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__STRING) (xpointer_t data1,
                                                    xpointer_t arg1,
                                                    xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__STRING callback;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__STRING) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_string (param_values + 1),
                       data2);

  xvalue_set_boolean (return_value, v_return);
}

void
_g_cclosure_marshal_BOOLEAN__STRINGv (xclosure_t *closure,
                                      xvalue_t   *return_value,
                                      xpointer_t  instance,
                                      va_list   args,
                                      xpointer_t  marshal_data,
                                      int       n_params,
                                      xtype_t    *param_types)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__STRING) (xpointer_t data1,
                                                    xpointer_t arg1,
                                                    xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__STRING callback;
  xboolean_t v_return;
  xpointer_t arg0;
  va_list args_copy;

  g_return_if_fail (return_value != NULL);

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = xstrdup (arg0);
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
  callback = (GMarshalFunc_BOOLEAN__STRING) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       arg0,
                       data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_free (arg0);

  xvalue_set_boolean (return_value, v_return);
}

/* BOOLEAN:UINT */
void
_g_cclosure_marshal_BOOLEAN__UINT (xclosure_t     *closure,
                                   xvalue_t       *return_value,
                                   xuint_t         n_param_values,
                                   const xvalue_t *param_values,
                                   xpointer_t      invocation_hint G_GNUC_UNUSED,
                                   xpointer_t      marshal_data)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__UINT) (xpointer_t data1,
                                                  xuint_t arg1,
                                                  xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__UINT callback;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__UINT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_uint (param_values + 1),
                       data2);

  xvalue_set_boolean (return_value, v_return);
}

void
_g_cclosure_marshal_BOOLEAN__UINTv (xclosure_t *closure,
                                    xvalue_t   *return_value,
                                    xpointer_t  instance,
                                    va_list   args,
                                    xpointer_t  marshal_data,
                                    int       n_params,
                                    xtype_t    *param_types)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__UINT) (xpointer_t data1,
                                                  xuint_t arg1,
                                                  xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__UINT callback;
  xboolean_t v_return;
  xuint_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xuint_t) va_arg (args_copy, xuint_t);
  va_end (args_copy);

  g_return_if_fail (return_value != NULL);

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
  callback = (GMarshalFunc_BOOLEAN__UINT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       arg0,
                       data2);


  xvalue_set_boolean (return_value, v_return);
}

/* BOOLEAN:VOID */
void
_g_cclosure_marshal_BOOLEAN__VOID (xclosure_t     *closure,
                                   xvalue_t       *return_value,
                                   xuint_t         n_param_values,
                                   const xvalue_t *param_values,
                                   xpointer_t      invocation_hint G_GNUC_UNUSED,
                                   xpointer_t      marshal_data)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__VOID) (xpointer_t data1,
                                                  xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__VOID callback;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 1);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__VOID) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       data2);

  xvalue_set_boolean (return_value, v_return);
}

void
_g_cclosure_marshal_BOOLEAN__VOIDv (xclosure_t *closure,
                                    xvalue_t   *return_value,
                                    xpointer_t  instance,
                                    va_list   args,
                                    xpointer_t  marshal_data,
                                    int       n_params,
                                    xtype_t    *param_types)
{
  typedef xboolean_t (*GMarshalFunc_BOOLEAN__VOID) (xpointer_t data1,
                                                  xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_BOOLEAN__VOID callback;
  xboolean_t v_return;

  g_return_if_fail (return_value != NULL);

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
  callback = (GMarshalFunc_BOOLEAN__VOID) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       data2);


  xvalue_set_boolean (return_value, v_return);
}

/* INT:BOXED */
void
_g_cclosure_marshal_INT__BOXED (xclosure_t     *closure,
                                xvalue_t       *return_value,
                                xuint_t         n_param_values,
                                const xvalue_t *param_values,
                                xpointer_t      invocation_hint G_GNUC_UNUSED,
                                xpointer_t      marshal_data)
{
  typedef xint_t (*GMarshalFunc_INT__BOXED) (xpointer_t data1,
                                           xpointer_t arg1,
                                           xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_INT__BOXED callback;
  xint_t v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_INT__BOXED) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_boxed (param_values + 1),
                       data2);

  xvalue_set_int (return_value, v_return);
}

void
_g_cclosure_marshal_INT__BOXEDv (xclosure_t *closure,
                                 xvalue_t   *return_value,
                                 xpointer_t  instance,
                                 va_list   args,
                                 xpointer_t  marshal_data,
                                 int       n_params,
                                 xtype_t    *param_types)
{
  typedef xint_t (*GMarshalFunc_INT__BOXED) (xpointer_t data1,
                                           xpointer_t arg1,
                                           xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_INT__BOXED callback;
  xint_t v_return;
  xpointer_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = xboxed_copy (param_types[0] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg0);
  va_end (args_copy);

  g_return_if_fail (return_value != NULL);

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
  callback = (GMarshalFunc_INT__BOXED) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       arg0,
                       data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    xboxed_free (param_types[0] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg0);

  xvalue_set_int (return_value, v_return);
}

/* INT:OBJECT */
void
_g_cclosure_marshal_INT__OBJECT (xclosure_t     *closure,
                                 xvalue_t       *return_value,
                                 xuint_t         n_param_values,
                                 const xvalue_t *param_values,
                                 xpointer_t      invocation_hint G_GNUC_UNUSED,
                                 xpointer_t      marshal_data)
{
  typedef xint_t (*GMarshalFunc_INT__OBJECT) (xpointer_t data1,
                                            xpointer_t arg1,
                                            xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_INT__OBJECT callback;
  xint_t v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_INT__OBJECT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_object (param_values + 1),
                       data2);

  xvalue_set_int (return_value, v_return);
}

void
_g_cclosure_marshal_INT__OBJECTv (xclosure_t *closure,
                                  xvalue_t   *return_value,
                                  xpointer_t  instance,
                                  va_list   args,
                                  xpointer_t  marshal_data,
                                  int       n_params,
                                  xtype_t    *param_types)
{
  typedef xint_t (*GMarshalFunc_INT__OBJECT) (xpointer_t data1,
                                            xpointer_t arg1,
                                            xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_INT__OBJECT callback;
  xint_t v_return;
  xpointer_t arg0;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg0 != NULL)
    arg0 = xobject_ref (arg0);
  va_end (args_copy);

  g_return_if_fail (return_value != NULL);

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
  callback = (GMarshalFunc_INT__OBJECT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       arg0,
                       data2);
  if (arg0 != NULL)
    xobject_unref (arg0);

  xvalue_set_int (return_value, v_return);
}

/* VOID:BOOLEAN,BOXED */
void
_g_cclosure_marshal_VOID__BOOLEAN_BOXED (xclosure_t     *closure,
                                         xvalue_t       *return_value G_GNUC_UNUSED,
                                         xuint_t         n_param_values,
                                         const xvalue_t *param_values,
                                         xpointer_t      invocation_hint G_GNUC_UNUSED,
                                         xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__BOOLEAN_BOXED) (xpointer_t data1,
                                                    xboolean_t arg1,
                                                    xpointer_t arg2,
                                                    xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__BOOLEAN_BOXED callback;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__BOOLEAN_BOXED) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_boolean (param_values + 1),
            g_marshal_value_peek_boxed (param_values + 2),
            data2);
}

void
_g_cclosure_marshal_VOID__BOOLEAN_BOXEDv (xclosure_t *closure,
                                          xvalue_t   *return_value G_GNUC_UNUSED,
                                          xpointer_t  instance,
                                          va_list   args,
                                          xpointer_t  marshal_data,
                                          int       n_params,
                                          xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__BOOLEAN_BOXED) (xpointer_t data1,
                                                    xboolean_t arg1,
                                                    xpointer_t arg2,
                                                    xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__BOOLEAN_BOXED callback;
  xboolean_t arg0;
  xpointer_t arg1;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xboolean_t) va_arg (args_copy, xboolean_t);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    arg1 = xboxed_copy (param_types[1] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg1);
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
  callback = (GMarshalFunc_VOID__BOOLEAN_BOXED) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            data2);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    xboxed_free (param_types[1] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg1);
}

/* VOID:ENUM,OBJECT */
void
_g_cclosure_marshal_VOID__ENUM_OBJECT (xclosure_t     *closure,
                                       xvalue_t       *return_value G_GNUC_UNUSED,
                                       xuint_t         n_param_values,
                                       const xvalue_t *param_values,
                                       xpointer_t      invocation_hint G_GNUC_UNUSED,
                                       xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__ENUM_OBJECT) (xpointer_t data1,
                                                  xint_t arg1,
                                                  xpointer_t arg2,
                                                  xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__ENUM_OBJECT callback;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__ENUM_OBJECT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_enum (param_values + 1),
            g_marshal_value_peek_object (param_values + 2),
            data2);
}

void
_g_cclosure_marshal_VOID__ENUM_OBJECTv (xclosure_t *closure,
                                        xvalue_t   *return_value G_GNUC_UNUSED,
                                        xpointer_t  instance,
                                        va_list   args,
                                        xpointer_t  marshal_data,
                                        int       n_params,
                                        xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__ENUM_OBJECT) (xpointer_t data1,
                                                  xint_t arg1,
                                                  xpointer_t arg2,
                                                  xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__ENUM_OBJECT callback;
  xint_t arg0;
  xpointer_t arg1;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xint_t) va_arg (args_copy, xint_t);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg1 != NULL)
    arg1 = xobject_ref (arg1);
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
  callback = (GMarshalFunc_VOID__ENUM_OBJECT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            data2);
  if (arg1 != NULL)
    xobject_unref (arg1);
}

/* VOID:ENUM,OBJECT,OBJECT */
void
_g_cclosure_marshal_VOID__ENUM_OBJECT_OBJECT (xclosure_t     *closure,
                                              xvalue_t       *return_value G_GNUC_UNUSED,
                                              xuint_t         n_param_values,
                                              const xvalue_t *param_values,
                                              xpointer_t      invocation_hint G_GNUC_UNUSED,
                                              xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__ENUM_OBJECT_OBJECT) (xpointer_t data1,
                                                         xint_t arg1,
                                                         xpointer_t arg2,
                                                         xpointer_t arg3,
                                                         xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__ENUM_OBJECT_OBJECT callback;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__ENUM_OBJECT_OBJECT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_enum (param_values + 1),
            g_marshal_value_peek_object (param_values + 2),
            g_marshal_value_peek_object (param_values + 3),
            data2);
}

void
_g_cclosure_marshal_VOID__ENUM_OBJECT_OBJECTv (xclosure_t *closure,
                                               xvalue_t   *return_value G_GNUC_UNUSED,
                                               xpointer_t  instance,
                                               va_list   args,
                                               xpointer_t  marshal_data,
                                               int       n_params,
                                               xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__ENUM_OBJECT_OBJECT) (xpointer_t data1,
                                                         xint_t arg1,
                                                         xpointer_t arg2,
                                                         xpointer_t arg3,
                                                         xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__ENUM_OBJECT_OBJECT callback;
  xint_t arg0;
  xpointer_t arg1;
  xpointer_t arg2;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xint_t) va_arg (args_copy, xint_t);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg1 != NULL)
    arg1 = xobject_ref (arg1);
  arg2 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg2 != NULL)
    arg2 = xobject_ref (arg2);
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
  callback = (GMarshalFunc_VOID__ENUM_OBJECT_OBJECT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            arg2,
            data2);
  if (arg1 != NULL)
    xobject_unref (arg1);
  if (arg2 != NULL)
    xobject_unref (arg2);
}

/* VOID:INT,INT,INT */
void
_g_cclosure_marshal_VOID__INT_INT_INT (xclosure_t     *closure,
                                       xvalue_t       *return_value G_GNUC_UNUSED,
                                       xuint_t         n_param_values,
                                       const xvalue_t *param_values,
                                       xpointer_t      invocation_hint G_GNUC_UNUSED,
                                       xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_INT_INT) (xpointer_t data1,
                                                  xint_t arg1,
                                                  xint_t arg2,
                                                  xint_t arg3,
                                                  xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__INT_INT_INT callback;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_INT_INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_int (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            g_marshal_value_peek_int (param_values + 3),
            data2);
}

void
_g_cclosure_marshal_VOID__INT_INT_INTv (xclosure_t *closure,
                                        xvalue_t   *return_value G_GNUC_UNUSED,
                                        xpointer_t  instance,
                                        va_list   args,
                                        xpointer_t  marshal_data,
                                        int       n_params,
                                        xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__INT_INT_INT) (xpointer_t data1,
                                                  xint_t arg1,
                                                  xint_t arg2,
                                                  xint_t arg3,
                                                  xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__INT_INT_INT callback;
  xint_t arg0;
  xint_t arg1;
  xint_t arg2;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xint_t) va_arg (args_copy, xint_t);
  arg1 = (xint_t) va_arg (args_copy, xint_t);
  arg2 = (xint_t) va_arg (args_copy, xint_t);
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
  callback = (GMarshalFunc_VOID__INT_INT_INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            arg2,
            data2);

}

/* VOID:OBJECT,OBJECT */
void
_g_cclosure_marshal_VOID__OBJECT_OBJECT (xclosure_t     *closure,
                                         xvalue_t       *return_value G_GNUC_UNUSED,
                                         xuint_t         n_param_values,
                                         const xvalue_t *param_values,
                                         xpointer_t      invocation_hint G_GNUC_UNUSED,
                                         xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT) (xpointer_t data1,
                                                    xpointer_t arg1,
                                                    xpointer_t arg2,
                                                    xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__OBJECT_OBJECT callback;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__OBJECT_OBJECT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_object (param_values + 1),
            g_marshal_value_peek_object (param_values + 2),
            data2);
}

void
_g_cclosure_marshal_VOID__OBJECT_OBJECTv (xclosure_t *closure,
                                          xvalue_t   *return_value G_GNUC_UNUSED,
                                          xpointer_t  instance,
                                          va_list   args,
                                          xpointer_t  marshal_data,
                                          int       n_params,
                                          xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT) (xpointer_t data1,
                                                    xpointer_t arg1,
                                                    xpointer_t arg2,
                                                    xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__OBJECT_OBJECT callback;
  xpointer_t arg0;
  xpointer_t arg1;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg0 != NULL)
    arg0 = xobject_ref (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg1 != NULL)
    arg1 = xobject_ref (arg1);
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
  callback = (GMarshalFunc_VOID__OBJECT_OBJECT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            data2);
  if (arg0 != NULL)
    xobject_unref (arg0);
  if (arg1 != NULL)
    xobject_unref (arg1);
}

/* VOID:OBJECT,OBJECT,ENUM */
void
_g_cclosure_marshal_VOID__OBJECT_OBJECT_ENUM (xclosure_t     *closure,
                                              xvalue_t       *return_value G_GNUC_UNUSED,
                                              xuint_t         n_param_values,
                                              const xvalue_t *param_values,
                                              xpointer_t      invocation_hint G_GNUC_UNUSED,
                                              xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT_ENUM) (xpointer_t data1,
                                                         xpointer_t arg1,
                                                         xpointer_t arg2,
                                                         xint_t arg3,
                                                         xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__OBJECT_OBJECT_ENUM callback;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__OBJECT_OBJECT_ENUM) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_object (param_values + 1),
            g_marshal_value_peek_object (param_values + 2),
            g_marshal_value_peek_enum (param_values + 3),
            data2);
}

void
_g_cclosure_marshal_VOID__OBJECT_OBJECT_ENUMv (xclosure_t *closure,
                                               xvalue_t   *return_value G_GNUC_UNUSED,
                                               xpointer_t  instance,
                                               va_list   args,
                                               xpointer_t  marshal_data,
                                               int       n_params,
                                               xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT_ENUM) (xpointer_t data1,
                                                         xpointer_t arg1,
                                                         xpointer_t arg2,
                                                         xint_t arg3,
                                                         xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__OBJECT_OBJECT_ENUM callback;
  xpointer_t arg0;
  xpointer_t arg1;
  xint_t arg2;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg0 != NULL)
    arg0 = xobject_ref (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg1 != NULL)
    arg1 = xobject_ref (arg1);
  arg2 = (xint_t) va_arg (args_copy, xint_t);
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
  callback = (GMarshalFunc_VOID__OBJECT_OBJECT_ENUM) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            arg2,
            data2);
  if (arg0 != NULL)
    xobject_unref (arg0);
  if (arg1 != NULL)
    xobject_unref (arg1);
}

/* VOID:OBJECT,OBJECT,STRING,STRING,VARIANT */
void
_g_cclosure_marshal_VOID__OBJECT_OBJECT_STRING_STRING_VARIANT (xclosure_t     *closure,
                                                               xvalue_t       *return_value G_GNUC_UNUSED,
                                                               xuint_t         n_param_values,
                                                               const xvalue_t *param_values,
                                                               xpointer_t      invocation_hint G_GNUC_UNUSED,
                                                               xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT_STRING_STRING_VARIANT) (xpointer_t data1,
                                                                          xpointer_t arg1,
                                                                          xpointer_t arg2,
                                                                          xpointer_t arg3,
                                                                          xpointer_t arg4,
                                                                          xpointer_t arg5,
                                                                          xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__OBJECT_OBJECT_STRING_STRING_VARIANT callback;

  g_return_if_fail (n_param_values == 6);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__OBJECT_OBJECT_STRING_STRING_VARIANT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_object (param_values + 1),
            g_marshal_value_peek_object (param_values + 2),
            g_marshal_value_peek_string (param_values + 3),
            g_marshal_value_peek_string (param_values + 4),
            g_marshal_value_peek_variant (param_values + 5),
            data2);
}

void
_g_cclosure_marshal_VOID__OBJECT_OBJECT_STRING_STRING_VARIANTv (xclosure_t *closure,
                                                                xvalue_t   *return_value G_GNUC_UNUSED,
                                                                xpointer_t  instance,
                                                                va_list   args,
                                                                xpointer_t  marshal_data,
                                                                int       n_params,
                                                                xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT_STRING_STRING_VARIANT) (xpointer_t data1,
                                                                          xpointer_t arg1,
                                                                          xpointer_t arg2,
                                                                          xpointer_t arg3,
                                                                          xpointer_t arg4,
                                                                          xpointer_t arg5,
                                                                          xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__OBJECT_OBJECT_STRING_STRING_VARIANT callback;
  xpointer_t arg0;
  xpointer_t arg1;
  xpointer_t arg2;
  xpointer_t arg3;
  xpointer_t arg4;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg0 != NULL)
    arg0 = xobject_ref (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg1 != NULL)
    arg1 = xobject_ref (arg1);
  arg2 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[2] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg2 != NULL)
    arg2 = xstrdup (arg2);
  arg3 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[3] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg3 != NULL)
    arg3 = xstrdup (arg3);
  arg4 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[4] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg4 != NULL)
    arg4 = xvariant_ref_sink (arg4);
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
  callback = (GMarshalFunc_VOID__OBJECT_OBJECT_STRING_STRING_VARIANT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            arg2,
            arg3,
            arg4,
            data2);
  if (arg0 != NULL)
    xobject_unref (arg0);
  if (arg1 != NULL)
    xobject_unref (arg1);
  if ((param_types[2] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg2 != NULL)
    g_free (arg2);
  if ((param_types[3] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg3 != NULL)
    g_free (arg3);
  if ((param_types[4] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg4 != NULL)
    xvariant_unref (arg4);
}

/* VOID:OBJECT,OBJECT,VARIANT,BOXED */
void
_g_cclosure_marshal_VOID__OBJECT_OBJECT_VARIANT_BOXED (xclosure_t     *closure,
                                                       xvalue_t       *return_value G_GNUC_UNUSED,
                                                       xuint_t         n_param_values,
                                                       const xvalue_t *param_values,
                                                       xpointer_t      invocation_hint G_GNUC_UNUSED,
                                                       xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT_VARIANT_BOXED) (xpointer_t data1,
                                                                  xpointer_t arg1,
                                                                  xpointer_t arg2,
                                                                  xpointer_t arg3,
                                                                  xpointer_t arg4,
                                                                  xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__OBJECT_OBJECT_VARIANT_BOXED callback;

  g_return_if_fail (n_param_values == 5);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__OBJECT_OBJECT_VARIANT_BOXED) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_object (param_values + 1),
            g_marshal_value_peek_object (param_values + 2),
            g_marshal_value_peek_variant (param_values + 3),
            g_marshal_value_peek_boxed (param_values + 4),
            data2);
}

void
_g_cclosure_marshal_VOID__OBJECT_OBJECT_VARIANT_BOXEDv (xclosure_t *closure,
                                                        xvalue_t   *return_value G_GNUC_UNUSED,
                                                        xpointer_t  instance,
                                                        va_list   args,
                                                        xpointer_t  marshal_data,
                                                        int       n_params,
                                                        xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT_VARIANT_BOXED) (xpointer_t data1,
                                                                  xpointer_t arg1,
                                                                  xpointer_t arg2,
                                                                  xpointer_t arg3,
                                                                  xpointer_t arg4,
                                                                  xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__OBJECT_OBJECT_VARIANT_BOXED callback;
  xpointer_t arg0;
  xpointer_t arg1;
  xpointer_t arg2;
  xpointer_t arg3;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg0 != NULL)
    arg0 = xobject_ref (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg1 != NULL)
    arg1 = xobject_ref (arg1);
  arg2 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[2] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg2 != NULL)
    arg2 = xvariant_ref_sink (arg2);
  arg3 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[3] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg3 != NULL)
    arg3 = xboxed_copy (param_types[3] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg3);
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
  callback = (GMarshalFunc_VOID__OBJECT_OBJECT_VARIANT_BOXED) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            arg2,
            arg3,
            data2);
  if (arg0 != NULL)
    xobject_unref (arg0);
  if (arg1 != NULL)
    xobject_unref (arg1);
  if ((param_types[2] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg2 != NULL)
    xvariant_unref (arg2);
  if ((param_types[3] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg3 != NULL)
    xboxed_free (param_types[3] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg3);
}

/* VOID:OBJECT,VARIANT */
void
_g_cclosure_marshal_VOID__OBJECT_VARIANT (xclosure_t     *closure,
                                          xvalue_t       *return_value G_GNUC_UNUSED,
                                          xuint_t         n_param_values,
                                          const xvalue_t *param_values,
                                          xpointer_t      invocation_hint G_GNUC_UNUSED,
                                          xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_VARIANT) (xpointer_t data1,
                                                     xpointer_t arg1,
                                                     xpointer_t arg2,
                                                     xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__OBJECT_VARIANT callback;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__OBJECT_VARIANT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_object (param_values + 1),
            g_marshal_value_peek_variant (param_values + 2),
            data2);
}

void
_g_cclosure_marshal_VOID__OBJECT_VARIANTv (xclosure_t *closure,
                                           xvalue_t   *return_value G_GNUC_UNUSED,
                                           xpointer_t  instance,
                                           va_list   args,
                                           xpointer_t  marshal_data,
                                           int       n_params,
                                           xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_VARIANT) (xpointer_t data1,
                                                     xpointer_t arg1,
                                                     xpointer_t arg2,
                                                     xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__OBJECT_VARIANT callback;
  xpointer_t arg0;
  xpointer_t arg1;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if (arg0 != NULL)
    arg0 = xobject_ref (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    arg1 = xvariant_ref_sink (arg1);
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
  callback = (GMarshalFunc_VOID__OBJECT_VARIANT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            data2);
  if (arg0 != NULL)
    xobject_unref (arg0);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    xvariant_unref (arg1);
}

/* VOID:POINTER,INT,STRING */
void
_g_cclosure_marshal_VOID__POINTER_INT_STRING (xclosure_t     *closure,
                                              xvalue_t       *return_value G_GNUC_UNUSED,
                                              xuint_t         n_param_values,
                                              const xvalue_t *param_values,
                                              xpointer_t      invocation_hint G_GNUC_UNUSED,
                                              xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_INT_STRING) (xpointer_t data1,
                                                         xpointer_t arg1,
                                                         xint_t arg2,
                                                         xpointer_t arg3,
                                                         xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__POINTER_INT_STRING callback;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_INT_STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_pointer (param_values + 1),
            g_marshal_value_peek_int (param_values + 2),
            g_marshal_value_peek_string (param_values + 3),
            data2);
}

void
_g_cclosure_marshal_VOID__POINTER_INT_STRINGv (xclosure_t *closure,
                                               xvalue_t   *return_value G_GNUC_UNUSED,
                                               xpointer_t  instance,
                                               va_list   args,
                                               xpointer_t  marshal_data,
                                               int       n_params,
                                               xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__POINTER_INT_STRING) (xpointer_t data1,
                                                         xpointer_t arg1,
                                                         xint_t arg2,
                                                         xpointer_t arg3,
                                                         xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__POINTER_INT_STRING callback;
  xpointer_t arg0;
  xint_t arg1;
  xpointer_t arg2;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  arg1 = (xint_t) va_arg (args_copy, xint_t);
  arg2 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[2] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg2 != NULL)
    arg2 = xstrdup (arg2);
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
  callback = (GMarshalFunc_VOID__POINTER_INT_STRING) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            arg2,
            data2);
  if ((param_types[2] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg2 != NULL)
    g_free (arg2);
}

/* VOID:STRING,BOOLEAN */
void
_g_cclosure_marshal_VOID__STRING_BOOLEAN (xclosure_t     *closure,
                                          xvalue_t       *return_value G_GNUC_UNUSED,
                                          xuint_t         n_param_values,
                                          const xvalue_t *param_values,
                                          xpointer_t      invocation_hint G_GNUC_UNUSED,
                                          xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_BOOLEAN) (xpointer_t data1,
                                                     xpointer_t arg1,
                                                     xboolean_t arg2,
                                                     xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_BOOLEAN callback;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_BOOLEAN) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_boolean (param_values + 2),
            data2);
}

void
_g_cclosure_marshal_VOID__STRING_BOOLEANv (xclosure_t *closure,
                                           xvalue_t   *return_value G_GNUC_UNUSED,
                                           xpointer_t  instance,
                                           va_list   args,
                                           xpointer_t  marshal_data,
                                           int       n_params,
                                           xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__STRING_BOOLEAN) (xpointer_t data1,
                                                     xpointer_t arg1,
                                                     xboolean_t arg2,
                                                     xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_BOOLEAN callback;
  xpointer_t arg0;
  xboolean_t arg1;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = xstrdup (arg0);
  arg1 = (xboolean_t) va_arg (args_copy, xboolean_t);
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
  callback = (GMarshalFunc_VOID__STRING_BOOLEAN) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_free (arg0);
}

/* VOID:STRING,BOXED */
void
_g_cclosure_marshal_VOID__STRING_BOXED (xclosure_t     *closure,
                                        xvalue_t       *return_value G_GNUC_UNUSED,
                                        xuint_t         n_param_values,
                                        const xvalue_t *param_values,
                                        xpointer_t      invocation_hint G_GNUC_UNUSED,
                                        xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_BOXED) (xpointer_t data1,
                                                   xpointer_t arg1,
                                                   xpointer_t arg2,
                                                   xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_BOXED callback;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_BOXED) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_boxed (param_values + 2),
            data2);
}

void
_g_cclosure_marshal_VOID__STRING_BOXEDv (xclosure_t *closure,
                                         xvalue_t   *return_value G_GNUC_UNUSED,
                                         xpointer_t  instance,
                                         va_list   args,
                                         xpointer_t  marshal_data,
                                         int       n_params,
                                         xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__STRING_BOXED) (xpointer_t data1,
                                                   xpointer_t arg1,
                                                   xpointer_t arg2,
                                                   xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_BOXED callback;
  xpointer_t arg0;
  xpointer_t arg1;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = xstrdup (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    arg1 = xboxed_copy (param_types[1] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg1);
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
  callback = (GMarshalFunc_VOID__STRING_BOXED) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_free (arg0);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    xboxed_free (param_types[1] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg1);
}

/* VOID:STRING,BOXED,BOXED */
void
_g_cclosure_marshal_VOID__STRING_BOXED_BOXED (xclosure_t     *closure,
                                              xvalue_t       *return_value G_GNUC_UNUSED,
                                              xuint_t         n_param_values,
                                              const xvalue_t *param_values,
                                              xpointer_t      invocation_hint G_GNUC_UNUSED,
                                              xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_BOXED_BOXED) (xpointer_t data1,
                                                         xpointer_t arg1,
                                                         xpointer_t arg2,
                                                         xpointer_t arg3,
                                                         xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_BOXED_BOXED callback;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_BOXED_BOXED) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_boxed (param_values + 2),
            g_marshal_value_peek_boxed (param_values + 3),
            data2);
}

void
_g_cclosure_marshal_VOID__STRING_BOXED_BOXEDv (xclosure_t *closure,
                                               xvalue_t   *return_value G_GNUC_UNUSED,
                                               xpointer_t  instance,
                                               va_list   args,
                                               xpointer_t  marshal_data,
                                               int       n_params,
                                               xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__STRING_BOXED_BOXED) (xpointer_t data1,
                                                         xpointer_t arg1,
                                                         xpointer_t arg2,
                                                         xpointer_t arg3,
                                                         xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_BOXED_BOXED callback;
  xpointer_t arg0;
  xpointer_t arg1;
  xpointer_t arg2;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = xstrdup (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    arg1 = xboxed_copy (param_types[1] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg1);
  arg2 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[2] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg2 != NULL)
    arg2 = xboxed_copy (param_types[2] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg2);
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
  callback = (GMarshalFunc_VOID__STRING_BOXED_BOXED) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            arg2,
            data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_free (arg0);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    xboxed_free (param_types[1] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg1);
  if ((param_types[2] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg2 != NULL)
    xboxed_free (param_types[2] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg2);
}

/* VOID:STRING,INT64,INT64 */
void
_g_cclosure_marshal_VOID__STRING_INT64_INT64 (xclosure_t     *closure,
                                              xvalue_t       *return_value G_GNUC_UNUSED,
                                              xuint_t         n_param_values,
                                              const xvalue_t *param_values,
                                              xpointer_t      invocation_hint G_GNUC_UNUSED,
                                              xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_INT64_INT64) (xpointer_t data1,
                                                         xpointer_t arg1,
                                                         sint64_t arg2,
                                                         sint64_t arg3,
                                                         xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_INT64_INT64 callback;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_INT64_INT64) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_int64 (param_values + 2),
            g_marshal_value_peek_int64 (param_values + 3),
            data2);
}

void
_g_cclosure_marshal_VOID__STRING_INT64_INT64v (xclosure_t *closure,
                                               xvalue_t   *return_value G_GNUC_UNUSED,
                                               xpointer_t  instance,
                                               va_list   args,
                                               xpointer_t  marshal_data,
                                               int       n_params,
                                               xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__STRING_INT64_INT64) (xpointer_t data1,
                                                         xpointer_t arg1,
                                                         sint64_t arg2,
                                                         sint64_t arg3,
                                                         xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_INT64_INT64 callback;
  xpointer_t arg0;
  sint64_t arg1;
  sint64_t arg2;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = xstrdup (arg0);
  arg1 = (sint64_t) va_arg (args_copy, sint64_t);
  arg2 = (sint64_t) va_arg (args_copy, sint64_t);
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
  callback = (GMarshalFunc_VOID__STRING_INT64_INT64) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            arg2,
            data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_free (arg0);
}

/* VOID:STRING,STRING,STRING,FLAGS */
void
_g_cclosure_marshal_VOID__STRING_STRING_STRING_FLAGS (xclosure_t     *closure,
                                                      xvalue_t       *return_value G_GNUC_UNUSED,
                                                      xuint_t         n_param_values,
                                                      const xvalue_t *param_values,
                                                      xpointer_t      invocation_hint G_GNUC_UNUSED,
                                                      xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_STRING_STRING_FLAGS) (xpointer_t data1,
                                                                 xpointer_t arg1,
                                                                 xpointer_t arg2,
                                                                 xpointer_t arg3,
                                                                 xuint_t arg4,
                                                                 xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_STRING_STRING_FLAGS callback;

  g_return_if_fail (n_param_values == 5);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_STRING_STRING_FLAGS) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_string (param_values + 2),
            g_marshal_value_peek_string (param_values + 3),
            g_marshal_value_peek_flags (param_values + 4),
            data2);
}

void
_g_cclosure_marshal_VOID__STRING_STRING_STRING_FLAGSv (xclosure_t *closure,
                                                       xvalue_t   *return_value G_GNUC_UNUSED,
                                                       xpointer_t  instance,
                                                       va_list   args,
                                                       xpointer_t  marshal_data,
                                                       int       n_params,
                                                       xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__STRING_STRING_STRING_FLAGS) (xpointer_t data1,
                                                                 xpointer_t arg1,
                                                                 xpointer_t arg2,
                                                                 xpointer_t arg3,
                                                                 xuint_t arg4,
                                                                 xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_STRING_STRING_FLAGS callback;
  xpointer_t arg0;
  xpointer_t arg1;
  xpointer_t arg2;
  xuint_t arg3;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = xstrdup (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    arg1 = xstrdup (arg1);
  arg2 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[2] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg2 != NULL)
    arg2 = xstrdup (arg2);
  arg3 = (xuint_t) va_arg (args_copy, xuint_t);
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
  callback = (GMarshalFunc_VOID__STRING_STRING_STRING_FLAGS) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            arg2,
            arg3,
            data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_free (arg0);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    g_free (arg1);
  if ((param_types[2] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg2 != NULL)
    g_free (arg2);
}

/* VOID:STRING,STRING,VARIANT */
void
_g_cclosure_marshal_VOID__STRING_STRING_VARIANT (xclosure_t     *closure,
                                                 xvalue_t       *return_value G_GNUC_UNUSED,
                                                 xuint_t         n_param_values,
                                                 const xvalue_t *param_values,
                                                 xpointer_t      invocation_hint G_GNUC_UNUSED,
                                                 xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_STRING_VARIANT) (xpointer_t data1,
                                                            xpointer_t arg1,
                                                            xpointer_t arg2,
                                                            xpointer_t arg3,
                                                            xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_STRING_VARIANT callback;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_STRING_VARIANT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_string (param_values + 2),
            g_marshal_value_peek_variant (param_values + 3),
            data2);
}

void
_g_cclosure_marshal_VOID__STRING_STRING_VARIANTv (xclosure_t *closure,
                                                  xvalue_t   *return_value G_GNUC_UNUSED,
                                                  xpointer_t  instance,
                                                  va_list   args,
                                                  xpointer_t  marshal_data,
                                                  int       n_params,
                                                  xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__STRING_STRING_VARIANT) (xpointer_t data1,
                                                            xpointer_t arg1,
                                                            xpointer_t arg2,
                                                            xpointer_t arg3,
                                                            xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_STRING_VARIANT callback;
  xpointer_t arg0;
  xpointer_t arg1;
  xpointer_t arg2;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = xstrdup (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    arg1 = xstrdup (arg1);
  arg2 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[2] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg2 != NULL)
    arg2 = xvariant_ref_sink (arg2);
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
  callback = (GMarshalFunc_VOID__STRING_STRING_VARIANT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            arg2,
            data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_free (arg0);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    g_free (arg1);
  if ((param_types[2] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg2 != NULL)
    xvariant_unref (arg2);
}

/* VOID:STRING,VARIANT */
void
_g_cclosure_marshal_VOID__STRING_VARIANT (xclosure_t     *closure,
                                          xvalue_t       *return_value G_GNUC_UNUSED,
                                          xuint_t         n_param_values,
                                          const xvalue_t *param_values,
                                          xpointer_t      invocation_hint G_GNUC_UNUSED,
                                          xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_VARIANT) (xpointer_t data1,
                                                     xpointer_t arg1,
                                                     xpointer_t arg2,
                                                     xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_VARIANT callback;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_VARIANT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string (param_values + 1),
            g_marshal_value_peek_variant (param_values + 2),
            data2);
}

void
_g_cclosure_marshal_VOID__STRING_VARIANTv (xclosure_t *closure,
                                           xvalue_t   *return_value G_GNUC_UNUSED,
                                           xpointer_t  instance,
                                           va_list   args,
                                           xpointer_t  marshal_data,
                                           int       n_params,
                                           xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__STRING_VARIANT) (xpointer_t data1,
                                                     xpointer_t arg1,
                                                     xpointer_t arg2,
                                                     xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__STRING_VARIANT callback;
  xpointer_t arg0;
  xpointer_t arg1;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = xstrdup (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    arg1 = xvariant_ref_sink (arg1);
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
  callback = (GMarshalFunc_VOID__STRING_VARIANT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    g_free (arg0);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    xvariant_unref (arg1);
}

/* VOID:UINT,UINT,UINT */
void
_g_cclosure_marshal_VOID__UINT_UINT_UINT (xclosure_t     *closure,
                                          xvalue_t       *return_value G_GNUC_UNUSED,
                                          xuint_t         n_param_values,
                                          const xvalue_t *param_values,
                                          xpointer_t      invocation_hint G_GNUC_UNUSED,
                                          xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__UINT_UINT_UINT) (xpointer_t data1,
                                                     xuint_t arg1,
                                                     xuint_t arg2,
                                                     xuint_t arg3,
                                                     xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__UINT_UINT_UINT callback;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UINT_UINT_UINT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_uint (param_values + 1),
            g_marshal_value_peek_uint (param_values + 2),
            g_marshal_value_peek_uint (param_values + 3),
            data2);
}

void
_g_cclosure_marshal_VOID__UINT_UINT_UINTv (xclosure_t *closure,
                                           xvalue_t   *return_value G_GNUC_UNUSED,
                                           xpointer_t  instance,
                                           va_list   args,
                                           xpointer_t  marshal_data,
                                           int       n_params,
                                           xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__UINT_UINT_UINT) (xpointer_t data1,
                                                     xuint_t arg1,
                                                     xuint_t arg2,
                                                     xuint_t arg3,
                                                     xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__UINT_UINT_UINT callback;
  xuint_t arg0;
  xuint_t arg1;
  xuint_t arg2;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xuint_t) va_arg (args_copy, xuint_t);
  arg1 = (xuint_t) va_arg (args_copy, xuint_t);
  arg2 = (xuint_t) va_arg (args_copy, xuint_t);
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
  callback = (GMarshalFunc_VOID__UINT_UINT_UINT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            arg2,
            data2);

}

/* VOID:VARIANT,BOXED */
void
_g_cclosure_marshal_VOID__VARIANT_BOXED (xclosure_t     *closure,
                                         xvalue_t       *return_value G_GNUC_UNUSED,
                                         xuint_t         n_param_values,
                                         const xvalue_t *param_values,
                                         xpointer_t      invocation_hint G_GNUC_UNUSED,
                                         xpointer_t      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__VARIANT_BOXED) (xpointer_t data1,
                                                    xpointer_t arg1,
                                                    xpointer_t arg2,
                                                    xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__VARIANT_BOXED callback;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = xvalue_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = xvalue_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__VARIANT_BOXED) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_variant (param_values + 1),
            g_marshal_value_peek_boxed (param_values + 2),
            data2);
}

void
_g_cclosure_marshal_VOID__VARIANT_BOXEDv (xclosure_t *closure,
                                          xvalue_t   *return_value G_GNUC_UNUSED,
                                          xpointer_t  instance,
                                          va_list   args,
                                          xpointer_t  marshal_data,
                                          int       n_params,
                                          xtype_t    *param_types)
{
  typedef void (*GMarshalFunc_VOID__VARIANT_BOXED) (xpointer_t data1,
                                                    xpointer_t arg1,
                                                    xpointer_t arg2,
                                                    xpointer_t data2);
  GCClosure *cc = (GCClosure *) closure;
  xpointer_t data1, data2;
  GMarshalFunc_VOID__VARIANT_BOXED callback;
  xpointer_t arg0;
  xpointer_t arg1;
  va_list args_copy;

  G_VA_COPY (args_copy, args);
  arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    arg0 = xvariant_ref_sink (arg0);
  arg1 = (xpointer_t) va_arg (args_copy, xpointer_t);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    arg1 = xboxed_copy (param_types[1] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg1);
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
  callback = (GMarshalFunc_VOID__VARIANT_BOXED) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            arg0,
            arg1,
            data2);
  if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
    xvariant_unref (arg0);
  if ((param_types[1] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg1 != NULL)
    xboxed_free (param_types[1] & ~G_SIGNAL_TYPE_STATIC_SCOPE, arg1);
}
