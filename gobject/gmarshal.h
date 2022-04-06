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

#ifndef __G_MARSHAL_H__
#define __G_MARSHAL_H__

G_BEGIN_DECLS

/* VOID:VOID */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__VOID (xclosure_t     *closure,
                                    xvalue_t       *return_value,
                                    xuint_t         n_param_values,
                                    const xvalue_t *param_values,
                                    xpointer_t      invocation_hint,
                                    xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__VOIDv (xclosure_t *closure,
                                     xvalue_t   *return_value,
                                     xpointer_t  instance,
                                     va_list   args,
                                     xpointer_t  marshal_data,
                                     int       n_params,
                                     xtype_t    *param_types);

/* VOID:BOOLEAN */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__BOOLEAN (xclosure_t     *closure,
                                       xvalue_t       *return_value,
                                       xuint_t         n_param_values,
                                       const xvalue_t *param_values,
                                       xpointer_t      invocation_hint,
                                       xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__BOOLEANv (xclosure_t *closure,
                                        xvalue_t   *return_value,
                                        xpointer_t  instance,
                                        va_list   args,
                                        xpointer_t  marshal_data,
                                        int       n_params,
                                        xtype_t    *param_types);

/* VOID:CHAR */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__CHAR (xclosure_t     *closure,
                                    xvalue_t       *return_value,
                                    xuint_t         n_param_values,
                                    const xvalue_t *param_values,
                                    xpointer_t      invocation_hint,
                                    xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__CHARv (xclosure_t *closure,
                                     xvalue_t   *return_value,
                                     xpointer_t  instance,
                                     va_list   args,
                                     xpointer_t  marshal_data,
                                     int       n_params,
                                     xtype_t    *param_types);

/* VOID:UCHAR */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__UCHAR (xclosure_t     *closure,
                                     xvalue_t       *return_value,
                                     xuint_t         n_param_values,
                                     const xvalue_t *param_values,
                                     xpointer_t      invocation_hint,
                                     xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__UCHARv (xclosure_t *closure,
                                      xvalue_t   *return_value,
                                      xpointer_t  instance,
                                      va_list   args,
                                      xpointer_t  marshal_data,
                                      int       n_params,
                                      xtype_t    *param_types);

/* VOID:INT */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__INT (xclosure_t     *closure,
                                   xvalue_t       *return_value,
                                   xuint_t         n_param_values,
                                   const xvalue_t *param_values,
                                   xpointer_t      invocation_hint,
                                   xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__INTv (xclosure_t *closure,
                                    xvalue_t   *return_value,
                                    xpointer_t  instance,
                                    va_list   args,
                                    xpointer_t  marshal_data,
                                    int       n_params,
                                    xtype_t    *param_types);

/* VOID:UINT */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__UINT (xclosure_t     *closure,
                                    xvalue_t       *return_value,
                                    xuint_t         n_param_values,
                                    const xvalue_t *param_values,
                                    xpointer_t      invocation_hint,
                                    xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__UINTv (xclosure_t *closure,
                                     xvalue_t   *return_value,
                                     xpointer_t  instance,
                                     va_list   args,
                                     xpointer_t  marshal_data,
                                     int       n_params,
                                     xtype_t    *param_types);

/* VOID:LONG */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__LONG (xclosure_t     *closure,
                                    xvalue_t       *return_value,
                                    xuint_t         n_param_values,
                                    const xvalue_t *param_values,
                                    xpointer_t      invocation_hint,
                                    xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__LONGv (xclosure_t *closure,
                                     xvalue_t   *return_value,
                                     xpointer_t  instance,
                                     va_list   args,
                                     xpointer_t  marshal_data,
                                     int       n_params,
                                     xtype_t    *param_types);

/* VOID:ULONG */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__ULONG (xclosure_t     *closure,
                                     xvalue_t       *return_value,
                                     xuint_t         n_param_values,
                                     const xvalue_t *param_values,
                                     xpointer_t      invocation_hint,
                                     xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__ULONGv (xclosure_t *closure,
                                      xvalue_t   *return_value,
                                      xpointer_t  instance,
                                      va_list   args,
                                      xpointer_t  marshal_data,
                                      int       n_params,
                                      xtype_t    *param_types);

/* VOID:ENUM */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__ENUM (xclosure_t     *closure,
                                    xvalue_t       *return_value,
                                    xuint_t         n_param_values,
                                    const xvalue_t *param_values,
                                    xpointer_t      invocation_hint,
                                    xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__ENUMv (xclosure_t *closure,
                                     xvalue_t   *return_value,
                                     xpointer_t  instance,
                                     va_list   args,
                                     xpointer_t  marshal_data,
                                     int       n_params,
                                     xtype_t    *param_types);

/* VOID:FLAGS */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__FLAGS (xclosure_t     *closure,
                                     xvalue_t       *return_value,
                                     xuint_t         n_param_values,
                                     const xvalue_t *param_values,
                                     xpointer_t      invocation_hint,
                                     xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__FLAGSv (xclosure_t *closure,
                                      xvalue_t   *return_value,
                                      xpointer_t  instance,
                                      va_list   args,
                                      xpointer_t  marshal_data,
                                      int       n_params,
                                      xtype_t    *param_types);

/* VOID:FLOAT */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__FLOAT (xclosure_t     *closure,
                                     xvalue_t       *return_value,
                                     xuint_t         n_param_values,
                                     const xvalue_t *param_values,
                                     xpointer_t      invocation_hint,
                                     xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__FLOATv (xclosure_t *closure,
                                      xvalue_t   *return_value,
                                      xpointer_t  instance,
                                      va_list   args,
                                      xpointer_t  marshal_data,
                                      int       n_params,
                                      xtype_t    *param_types);

/* VOID:DOUBLE */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__DOUBLE (xclosure_t     *closure,
                                      xvalue_t       *return_value,
                                      xuint_t         n_param_values,
                                      const xvalue_t *param_values,
                                      xpointer_t      invocation_hint,
                                      xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__DOUBLEv (xclosure_t *closure,
                                       xvalue_t   *return_value,
                                       xpointer_t  instance,
                                       va_list   args,
                                       xpointer_t  marshal_data,
                                       int       n_params,
                                       xtype_t    *param_types);

/* VOID:STRING */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__STRING (xclosure_t     *closure,
                                      xvalue_t       *return_value,
                                      xuint_t         n_param_values,
                                      const xvalue_t *param_values,
                                      xpointer_t      invocation_hint,
                                      xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__STRINGv (xclosure_t *closure,
                                       xvalue_t   *return_value,
                                       xpointer_t  instance,
                                       va_list   args,
                                       xpointer_t  marshal_data,
                                       int       n_params,
                                       xtype_t    *param_types);

/* VOID:PARAM */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__PARAM (xclosure_t     *closure,
                                     xvalue_t       *return_value,
                                     xuint_t         n_param_values,
                                     const xvalue_t *param_values,
                                     xpointer_t      invocation_hint,
                                     xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__PARAMv (xclosure_t *closure,
                                      xvalue_t   *return_value,
                                      xpointer_t  instance,
                                      va_list   args,
                                      xpointer_t  marshal_data,
                                      int       n_params,
                                      xtype_t    *param_types);

/* VOID:BOXED */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__BOXED (xclosure_t     *closure,
                                     xvalue_t       *return_value,
                                     xuint_t         n_param_values,
                                     const xvalue_t *param_values,
                                     xpointer_t      invocation_hint,
                                     xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__BOXEDv (xclosure_t *closure,
                                      xvalue_t   *return_value,
                                      xpointer_t  instance,
                                      va_list   args,
                                      xpointer_t  marshal_data,
                                      int       n_params,
                                      xtype_t    *param_types);

/* VOID:POINTER */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__POINTER (xclosure_t     *closure,
                                       xvalue_t       *return_value,
                                       xuint_t         n_param_values,
                                       const xvalue_t *param_values,
                                       xpointer_t      invocation_hint,
                                       xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__POINTERv (xclosure_t *closure,
                                        xvalue_t   *return_value,
                                        xpointer_t  instance,
                                        va_list   args,
                                        xpointer_t  marshal_data,
                                        int       n_params,
                                        xtype_t    *param_types);

/* VOID:OBJECT */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__OBJECT (xclosure_t     *closure,
                                      xvalue_t       *return_value,
                                      xuint_t         n_param_values,
                                      const xvalue_t *param_values,
                                      xpointer_t      invocation_hint,
                                      xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__OBJECTv (xclosure_t *closure,
                                       xvalue_t   *return_value,
                                       xpointer_t  instance,
                                       va_list   args,
                                       xpointer_t  marshal_data,
                                       int       n_params,
                                       xtype_t    *param_types);

/* VOID:VARIANT */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__VARIANT (xclosure_t     *closure,
                                       xvalue_t       *return_value,
                                       xuint_t         n_param_values,
                                       const xvalue_t *param_values,
                                       xpointer_t      invocation_hint,
                                       xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__VARIANTv (xclosure_t *closure,
                                        xvalue_t   *return_value,
                                        xpointer_t  instance,
                                        va_list   args,
                                        xpointer_t  marshal_data,
                                        int       n_params,
                                        xtype_t    *param_types);

/* VOID:UINT,POINTER */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__UINT_POINTER (xclosure_t     *closure,
                                            xvalue_t       *return_value,
                                            xuint_t         n_param_values,
                                            const xvalue_t *param_values,
                                            xpointer_t      invocation_hint,
                                            xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_VOID__UINT_POINTERv (xclosure_t *closure,
                                             xvalue_t   *return_value,
                                             xpointer_t  instance,
                                             va_list   args,
                                             xpointer_t  marshal_data,
                                             int       n_params,
                                             xtype_t    *param_types);

/* BOOL:FLAGS */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_BOOLEAN__FLAGS (xclosure_t     *closure,
                                        xvalue_t       *return_value,
                                        xuint_t         n_param_values,
                                        const xvalue_t *param_values,
                                        xpointer_t      invocation_hint,
                                        xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_BOOLEAN__FLAGSv (xclosure_t *closure,
                                         xvalue_t   *return_value,
                                         xpointer_t  instance,
                                         va_list   args,
                                         xpointer_t  marshal_data,
                                         int       n_params,
                                         xtype_t    *param_types);

/**
 * g_cclosure_marshal_BOOL__FLAGS:
 * @closure: A #xclosure_t.
 * @return_value: A #xvalue_t to store the return value. May be %NULL
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
 * An old alias for g_cclosure_marshal_BOOLEAN__FLAGS().
 */
#define g_cclosure_marshal_BOOL__FLAGS	g_cclosure_marshal_BOOLEAN__FLAGS

/* STRING:OBJECT,POINTER */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_STRING__OBJECT_POINTER (xclosure_t     *closure,
                                                xvalue_t       *return_value,
                                                xuint_t         n_param_values,
                                                const xvalue_t *param_values,
                                                xpointer_t      invocation_hint,
                                                xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_STRING__OBJECT_POINTERv (xclosure_t *closure,
                                                 xvalue_t   *return_value,
                                                 xpointer_t  instance,
                                                 va_list   args,
                                                 xpointer_t  marshal_data,
                                                 int       n_params,
                                                 xtype_t    *param_types);

/* BOOL:BOXED,BOXED */
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_BOOLEAN__BOXED_BOXED (xclosure_t     *closure,
                                              xvalue_t       *return_value,
                                              xuint_t         n_param_values,
                                              const xvalue_t *param_values,
                                              xpointer_t      invocation_hint,
                                              xpointer_t      marshal_data);
XPL_AVAILABLE_IN_ALL
void g_cclosure_marshal_BOOLEAN__BOXED_BOXEDv (xclosure_t *closure,
                                               xvalue_t   *return_value,
                                               xpointer_t  instance,
                                               va_list   args,
                                               xpointer_t  marshal_data,
                                               int       n_params,
                                               xtype_t    *param_types);

/**
 * g_cclosure_marshal_BOOL__BOXED_BOXED:
 * @closure: A #xclosure_t.
 * @return_value: A #xvalue_t to store the return value. May be %NULL
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
 * An old alias for g_cclosure_marshal_BOOLEAN__BOXED_BOXED().
 */
#define g_cclosure_marshal_BOOL__BOXED_BOXED	g_cclosure_marshal_BOOLEAN__BOXED_BOXED

G_END_DECLS

#endif /* __G_MARSHAL_H__ */
