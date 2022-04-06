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

#ifndef ___G_CCLOSURE_MARSHAL_MARSHAL_H__
#define ___G_CCLOSURE_MARSHAL_MARSHAL_H__

#include <glib-object.h>

G_BEGIN_DECLS

/* BOOLEAN:OBJECT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__OBJECT (xclosure_t     *closure,
                                          xvalue_t       *return_value,
                                          xuint_t         n_param_values,
                                          const xvalue_t *param_values,
                                          xpointer_t      invocation_hint,
                                          xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__OBJECTv (xclosure_t *closure,
                                           xvalue_t   *return_value,
                                           xpointer_t  instance,
                                           va_list   args,
                                           xpointer_t  marshal_data,
                                           int       n_params,
                                           xtype_t    *param_types);

/* BOOLEAN:OBJECT,FLAGS */
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__OBJECT_FLAGS (xclosure_t     *closure,
                                                xvalue_t       *return_value,
                                                xuint_t         n_param_values,
                                                const xvalue_t *param_values,
                                                xpointer_t      invocation_hint,
                                                xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__OBJECT_FLAGSv (xclosure_t *closure,
                                                 xvalue_t   *return_value,
                                                 xpointer_t  instance,
                                                 va_list   args,
                                                 xpointer_t  marshal_data,
                                                 int       n_params,
                                                 xtype_t    *param_types);

/* BOOLEAN:OBJECT,OBJECT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__OBJECT_OBJECT (xclosure_t     *closure,
                                                 xvalue_t       *return_value,
                                                 xuint_t         n_param_values,
                                                 const xvalue_t *param_values,
                                                 xpointer_t      invocation_hint,
                                                 xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__OBJECT_OBJECTv (xclosure_t *closure,
                                                  xvalue_t   *return_value,
                                                  xpointer_t  instance,
                                                  va_list   args,
                                                  xpointer_t  marshal_data,
                                                  int       n_params,
                                                  xtype_t    *param_types);

/* BOOLEAN:POINTER,INT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__POINTER_INT (xclosure_t     *closure,
                                               xvalue_t       *return_value,
                                               xuint_t         n_param_values,
                                               const xvalue_t *param_values,
                                               xpointer_t      invocation_hint,
                                               xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__POINTER_INTv (xclosure_t *closure,
                                                xvalue_t   *return_value,
                                                xpointer_t  instance,
                                                va_list   args,
                                                xpointer_t  marshal_data,
                                                int       n_params,
                                                xtype_t    *param_types);

/* BOOLEAN:STRING */
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__STRING (xclosure_t     *closure,
                                          xvalue_t       *return_value,
                                          xuint_t         n_param_values,
                                          const xvalue_t *param_values,
                                          xpointer_t      invocation_hint,
                                          xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__STRINGv (xclosure_t *closure,
                                           xvalue_t   *return_value,
                                           xpointer_t  instance,
                                           va_list   args,
                                           xpointer_t  marshal_data,
                                           int       n_params,
                                           xtype_t    *param_types);

/* BOOLEAN:UINT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__UINT (xclosure_t     *closure,
                                        xvalue_t       *return_value,
                                        xuint_t         n_param_values,
                                        const xvalue_t *param_values,
                                        xpointer_t      invocation_hint,
                                        xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__UINTv (xclosure_t *closure,
                                         xvalue_t   *return_value,
                                         xpointer_t  instance,
                                         va_list   args,
                                         xpointer_t  marshal_data,
                                         int       n_params,
                                         xtype_t    *param_types);

/* BOOLEAN:VOID */
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__VOID (xclosure_t     *closure,
                                        xvalue_t       *return_value,
                                        xuint_t         n_param_values,
                                        const xvalue_t *param_values,
                                        xpointer_t      invocation_hint,
                                        xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_BOOLEAN__VOIDv (xclosure_t *closure,
                                         xvalue_t   *return_value,
                                         xpointer_t  instance,
                                         va_list   args,
                                         xpointer_t  marshal_data,
                                         int       n_params,
                                         xtype_t    *param_types);

/* INT:BOXED */
G_GNUC_INTERNAL
void _g_cclosure_marshal_INT__BOXED (xclosure_t     *closure,
                                     xvalue_t       *return_value,
                                     xuint_t         n_param_values,
                                     const xvalue_t *param_values,
                                     xpointer_t      invocation_hint,
                                     xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_INT__BOXEDv (xclosure_t *closure,
                                      xvalue_t   *return_value,
                                      xpointer_t  instance,
                                      va_list   args,
                                      xpointer_t  marshal_data,
                                      int       n_params,
                                      xtype_t    *param_types);

/* INT:OBJECT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_INT__OBJECT (xclosure_t     *closure,
                                      xvalue_t       *return_value,
                                      xuint_t         n_param_values,
                                      const xvalue_t *param_values,
                                      xpointer_t      invocation_hint,
                                      xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_INT__OBJECTv (xclosure_t *closure,
                                       xvalue_t   *return_value,
                                       xpointer_t  instance,
                                       va_list   args,
                                       xpointer_t  marshal_data,
                                       int       n_params,
                                       xtype_t    *param_types);

/* VOID:BOOLEAN,BOXED */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__BOOLEAN_BOXED (xclosure_t     *closure,
                                              xvalue_t       *return_value,
                                              xuint_t         n_param_values,
                                              const xvalue_t *param_values,
                                              xpointer_t      invocation_hint,
                                              xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__BOOLEAN_BOXEDv (xclosure_t *closure,
                                               xvalue_t   *return_value,
                                               xpointer_t  instance,
                                               va_list   args,
                                               xpointer_t  marshal_data,
                                               int       n_params,
                                               xtype_t    *param_types);

/* VOID:ENUM,OBJECT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__ENUM_OBJECT (xclosure_t     *closure,
                                            xvalue_t       *return_value,
                                            xuint_t         n_param_values,
                                            const xvalue_t *param_values,
                                            xpointer_t      invocation_hint,
                                            xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__ENUM_OBJECTv (xclosure_t *closure,
                                             xvalue_t   *return_value,
                                             xpointer_t  instance,
                                             va_list   args,
                                             xpointer_t  marshal_data,
                                             int       n_params,
                                             xtype_t    *param_types);

/* VOID:ENUM,OBJECT,OBJECT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__ENUM_OBJECT_OBJECT (xclosure_t     *closure,
                                                   xvalue_t       *return_value,
                                                   xuint_t         n_param_values,
                                                   const xvalue_t *param_values,
                                                   xpointer_t      invocation_hint,
                                                   xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__ENUM_OBJECT_OBJECTv (xclosure_t *closure,
                                                    xvalue_t   *return_value,
                                                    xpointer_t  instance,
                                                    va_list   args,
                                                    xpointer_t  marshal_data,
                                                    int       n_params,
                                                    xtype_t    *param_types);

/* VOID:INT,INT,INT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__INT_INT_INT (xclosure_t     *closure,
                                            xvalue_t       *return_value,
                                            xuint_t         n_param_values,
                                            const xvalue_t *param_values,
                                            xpointer_t      invocation_hint,
                                            xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__INT_INT_INTv (xclosure_t *closure,
                                             xvalue_t   *return_value,
                                             xpointer_t  instance,
                                             va_list   args,
                                             xpointer_t  marshal_data,
                                             int       n_params,
                                             xtype_t    *param_types);

/* VOID:OBJECT,OBJECT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__OBJECT_OBJECT (xclosure_t     *closure,
                                              xvalue_t       *return_value,
                                              xuint_t         n_param_values,
                                              const xvalue_t *param_values,
                                              xpointer_t      invocation_hint,
                                              xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__OBJECT_OBJECTv (xclosure_t *closure,
                                               xvalue_t   *return_value,
                                               xpointer_t  instance,
                                               va_list   args,
                                               xpointer_t  marshal_data,
                                               int       n_params,
                                               xtype_t    *param_types);

/* VOID:OBJECT,OBJECT,ENUM */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__OBJECT_OBJECT_ENUM (xclosure_t     *closure,
                                                   xvalue_t       *return_value,
                                                   xuint_t         n_param_values,
                                                   const xvalue_t *param_values,
                                                   xpointer_t      invocation_hint,
                                                   xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__OBJECT_OBJECT_ENUMv (xclosure_t *closure,
                                                    xvalue_t   *return_value,
                                                    xpointer_t  instance,
                                                    va_list   args,
                                                    xpointer_t  marshal_data,
                                                    int       n_params,
                                                    xtype_t    *param_types);

/* VOID:OBJECT,OBJECT,STRING,STRING,VARIANT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__OBJECT_OBJECT_STRING_STRING_VARIANT (xclosure_t     *closure,
                                                                    xvalue_t       *return_value,
                                                                    xuint_t         n_param_values,
                                                                    const xvalue_t *param_values,
                                                                    xpointer_t      invocation_hint,
                                                                    xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__OBJECT_OBJECT_STRING_STRING_VARIANTv (xclosure_t *closure,
                                                                     xvalue_t   *return_value,
                                                                     xpointer_t  instance,
                                                                     va_list   args,
                                                                     xpointer_t  marshal_data,
                                                                     int       n_params,
                                                                     xtype_t    *param_types);

/* VOID:OBJECT,OBJECT,VARIANT,BOXED */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__OBJECT_OBJECT_VARIANT_BOXED (xclosure_t     *closure,
                                                            xvalue_t       *return_value,
                                                            xuint_t         n_param_values,
                                                            const xvalue_t *param_values,
                                                            xpointer_t      invocation_hint,
                                                            xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__OBJECT_OBJECT_VARIANT_BOXEDv (xclosure_t *closure,
                                                             xvalue_t   *return_value,
                                                             xpointer_t  instance,
                                                             va_list   args,
                                                             xpointer_t  marshal_data,
                                                             int       n_params,
                                                             xtype_t    *param_types);

/* VOID:OBJECT,VARIANT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__OBJECT_VARIANT (xclosure_t     *closure,
                                               xvalue_t       *return_value,
                                               xuint_t         n_param_values,
                                               const xvalue_t *param_values,
                                               xpointer_t      invocation_hint,
                                               xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__OBJECT_VARIANTv (xclosure_t *closure,
                                                xvalue_t   *return_value,
                                                xpointer_t  instance,
                                                va_list   args,
                                                xpointer_t  marshal_data,
                                                int       n_params,
                                                xtype_t    *param_types);

/* VOID:POINTER,INT,STRING */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__POINTER_INT_STRING (xclosure_t     *closure,
                                                   xvalue_t       *return_value,
                                                   xuint_t         n_param_values,
                                                   const xvalue_t *param_values,
                                                   xpointer_t      invocation_hint,
                                                   xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__POINTER_INT_STRINGv (xclosure_t *closure,
                                                    xvalue_t   *return_value,
                                                    xpointer_t  instance,
                                                    va_list   args,
                                                    xpointer_t  marshal_data,
                                                    int       n_params,
                                                    xtype_t    *param_types);

/* VOID:STRING,BOOLEAN */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_BOOLEAN (xclosure_t     *closure,
                                               xvalue_t       *return_value,
                                               xuint_t         n_param_values,
                                               const xvalue_t *param_values,
                                               xpointer_t      invocation_hint,
                                               xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_BOOLEANv (xclosure_t *closure,
                                                xvalue_t   *return_value,
                                                xpointer_t  instance,
                                                va_list   args,
                                                xpointer_t  marshal_data,
                                                int       n_params,
                                                xtype_t    *param_types);

/* VOID:STRING,BOXED */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_BOXED (xclosure_t     *closure,
                                             xvalue_t       *return_value,
                                             xuint_t         n_param_values,
                                             const xvalue_t *param_values,
                                             xpointer_t      invocation_hint,
                                             xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_BOXEDv (xclosure_t *closure,
                                              xvalue_t   *return_value,
                                              xpointer_t  instance,
                                              va_list   args,
                                              xpointer_t  marshal_data,
                                              int       n_params,
                                              xtype_t    *param_types);

/* VOID:STRING,BOXED,BOXED */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_BOXED_BOXED (xclosure_t     *closure,
                                                   xvalue_t       *return_value,
                                                   xuint_t         n_param_values,
                                                   const xvalue_t *param_values,
                                                   xpointer_t      invocation_hint,
                                                   xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_BOXED_BOXEDv (xclosure_t *closure,
                                                    xvalue_t   *return_value,
                                                    xpointer_t  instance,
                                                    va_list   args,
                                                    xpointer_t  marshal_data,
                                                    int       n_params,
                                                    xtype_t    *param_types);

/* VOID:STRING,INT64,INT64 */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_INT64_INT64 (xclosure_t     *closure,
                                                   xvalue_t       *return_value,
                                                   xuint_t         n_param_values,
                                                   const xvalue_t *param_values,
                                                   xpointer_t      invocation_hint,
                                                   xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_INT64_INT64v (xclosure_t *closure,
                                                    xvalue_t   *return_value,
                                                    xpointer_t  instance,
                                                    va_list   args,
                                                    xpointer_t  marshal_data,
                                                    int       n_params,
                                                    xtype_t    *param_types);

/* VOID:STRING,STRING,STRING,FLAGS */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_STRING_STRING_FLAGS (xclosure_t     *closure,
                                                           xvalue_t       *return_value,
                                                           xuint_t         n_param_values,
                                                           const xvalue_t *param_values,
                                                           xpointer_t      invocation_hint,
                                                           xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_STRING_STRING_FLAGSv (xclosure_t *closure,
                                                            xvalue_t   *return_value,
                                                            xpointer_t  instance,
                                                            va_list   args,
                                                            xpointer_t  marshal_data,
                                                            int       n_params,
                                                            xtype_t    *param_types);

/* VOID:STRING,STRING,VARIANT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_STRING_VARIANT (xclosure_t     *closure,
                                                      xvalue_t       *return_value,
                                                      xuint_t         n_param_values,
                                                      const xvalue_t *param_values,
                                                      xpointer_t      invocation_hint,
                                                      xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_STRING_VARIANTv (xclosure_t *closure,
                                                       xvalue_t   *return_value,
                                                       xpointer_t  instance,
                                                       va_list   args,
                                                       xpointer_t  marshal_data,
                                                       int       n_params,
                                                       xtype_t    *param_types);

/* VOID:STRING,VARIANT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_VARIANT (xclosure_t     *closure,
                                               xvalue_t       *return_value,
                                               xuint_t         n_param_values,
                                               const xvalue_t *param_values,
                                               xpointer_t      invocation_hint,
                                               xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__STRING_VARIANTv (xclosure_t *closure,
                                                xvalue_t   *return_value,
                                                xpointer_t  instance,
                                                va_list   args,
                                                xpointer_t  marshal_data,
                                                int       n_params,
                                                xtype_t    *param_types);

/* VOID:UINT,UINT,UINT */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__UINT_UINT_UINT (xclosure_t     *closure,
                                               xvalue_t       *return_value,
                                               xuint_t         n_param_values,
                                               const xvalue_t *param_values,
                                               xpointer_t      invocation_hint,
                                               xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__UINT_UINT_UINTv (xclosure_t *closure,
                                                xvalue_t   *return_value,
                                                xpointer_t  instance,
                                                va_list   args,
                                                xpointer_t  marshal_data,
                                                int       n_params,
                                                xtype_t    *param_types);

/* VOID:VARIANT,BOXED */
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__VARIANT_BOXED (xclosure_t     *closure,
                                              xvalue_t       *return_value,
                                              xuint_t         n_param_values,
                                              const xvalue_t *param_values,
                                              xpointer_t      invocation_hint,
                                              xpointer_t      marshal_data);
G_GNUC_INTERNAL
void _g_cclosure_marshal_VOID__VARIANT_BOXEDv (xclosure_t *closure,
                                               xvalue_t   *return_value,
                                               xpointer_t  instance,
                                               va_list   args,
                                               xpointer_t  marshal_data,
                                               int       n_params,
                                               xtype_t    *param_types);


G_END_DECLS

#endif /* ___G_CCLOSURE_MARSHAL_MARSHAL_H__ */
