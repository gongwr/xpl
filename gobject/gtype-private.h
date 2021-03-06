/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1998-1999, 2000-2001 Tim Janik and Red Hat, Inc.
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
#ifndef __XTYPE_PRIVATE_H__
#define __XTYPE_PRIVATE_H__

#if !defined (__XPL_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include "gboxed.h"
#include "gclosure.h"
#include "gobject.h"

/*< private >
 * GOBJECT_IF_DEBUG:
 * @debug_type: Currently only OBJECTS and SIGNALS are supported.
 * @code_block: Custom debug code.
 *
 * A convenience macro for debugging xobject_t.
 * This macro is only used internally.
 */
#ifdef G_ENABLE_DEBUG
#define GOBJECT_IF_DEBUG(debug_type, code_block) \
G_STMT_START { \
    if (_xtype_debug_flags & XTYPE_DEBUG_ ## debug_type) \
      { code_block; } \
} G_STMT_END
#else   /* !G_ENABLE_DEBUG */
#define GOBJECT_IF_DEBUG(debug_type, code_block)
#endif  /* G_ENABLE_DEBUG */

G_BEGIN_DECLS

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
extern GTypeDebugFlags _xtype_debug_flags;
G_GNUC_END_IGNORE_DEPRECATIONS

typedef struct _GRealClosure  GRealClosure;
struct _GRealClosure
{
  GClosureMarshal meta_marshal;
  xpointer_t meta_marshal_data;
  GVaClosureMarshal va_meta_marshal;
  GVaClosureMarshal va_marshal;
  xclosure_t closure;
};

#define G_REAL_CLOSURE(_c) \
  ((GRealClosure *)G_STRUCT_MEMBER_P ((_c), -G_STRUCT_OFFSET (GRealClosure, closure)))

void    _xvalue_c_init          (void); /* sync with gvalue.c */
void    _xvalue_types_init      (void); /* sync with gvaluetypes.c */
void    _xenum_types_init       (void); /* sync with genums.c */
void    _g_param_type_init       (void); /* sync with gparam.c */
void    _xboxed_type_init       (void); /* sync with gboxed.c */
void    _xobject_type_init      (void); /* sync with gobject.c */
void    _xparam_spec_types_init (void); /* sync with gparamspecs.c */
void    _xvalue_transforms_init (void); /* sync with gvaluetransform.c */
void    _xsignal_init           (void); /* sync with gsignal.c */

/* for gboxed.c */
xpointer_t        _xtype_boxed_copy      (xtype_t          type,
                                         xpointer_t       value);
void            _xtype_boxed_free      (xtype_t          type,
                                         xpointer_t       value);
void            _xtype_boxed_init      (xtype_t          type,
                                         GBoxedCopyFunc copy_func,
                                         GBoxedFreeFunc free_func);

xboolean_t    _xclosure_is_void (xclosure_t       *closure,
				xpointer_t        instance);
xboolean_t    _xclosure_supports_invoke_va (xclosure_t       *closure);
void        _xclosure_set_va_marshal (xclosure_t       *closure,
				       GVaClosureMarshal marshal);
void        _xclosure_invoke_va (xclosure_t       *closure,
				  xvalue_t /*out*/ *return_value,
				  xpointer_t        instance,
				  va_list         args,
				  int             n_params,
				  xtype_t          *param_types);

xboolean_t    _xobject_has_signal_handler     (xobject_t     *object);
void        _xobject_set_has_signal_handler (xobject_t     *object);

/**
 * _G_DEFINE_TYPE_EXTENDED_WITH_PRELUDE:
 *
 * See also G_DEFINE_TYPE_EXTENDED().  This macro is generally only
 * necessary as a workaround for classes which have properties of
 * object types that may be initialized in distinct threads.  See:
 * https://bugzilla.gnome.org/show_bug.cgi?id=674885
 *
 * Currently private.
 */
#define _G_DEFINE_TYPE_EXTENDED_WITH_PRELUDE(TN, t_n, T_P, _f_, _P_, _C_)	    _G_DEFINE_TYPE_EXTENDED_BEGIN_PRE (TN, t_n, T_P) {_P_;} _G_DEFINE_TYPE_EXTENDED_BEGIN_REGISTER (TN, t_n, T_P, _f_){_C_;} _G_DEFINE_TYPE_EXTENDED_END()

G_END_DECLS

#endif /* __XTYPE_PRIVATE_H__ */
