/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __G_DBUS_INTROSPECTION_H__
#define __G_DBUS_INTROSPECTION_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

/**
 * xdbus_annotation_info_t:
 * @ref_count: The reference count or -1 if statically allocated.
 * @key: The name of the annotation, e.g. "org.freedesktop.DBus.Deprecated".
 * @value: The value of the annotation.
 * @annotations: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_annotation_info_t structures or %NULL if there are no annotations.
 *
 * Information about an annotation.
 *
 * Since: 2.26
 */
struct _GDBusAnnotationInfo
{
  /*< public >*/
  xint_t                  ref_count;  /* (atomic) */
  xchar_t                *key;
  xchar_t                *value;
  xdbus_annotation_info_t **annotations;
};

/**
 * xdbus_arg_info_t:
 * @ref_count: The reference count or -1 if statically allocated.
 * @name: Name of the argument, e.g. @unix_user_id.
 * @signature: D-Bus signature of the argument (a single complete type).
 * @annotations: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_annotation_info_t structures or %NULL if there are no annotations.
 *
 * Information about an argument for a method or a signal.
 *
 * Since: 2.26
 */
struct _GDBusArgInfo
{
  /*< public >*/
  xint_t                  ref_count;  /* (atomic) */
  xchar_t                *name;
  xchar_t                *signature;
  xdbus_annotation_info_t **annotations;
};

/**
 * xdbus_method_info_t:
 * @ref_count: The reference count or -1 if statically allocated.
 * @name: The name of the D-Bus method, e.g. @RequestName.
 * @in_args: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_arg_info_t structures or %NULL if there are no in arguments.
 * @out_args: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_arg_info_t structures or %NULL if there are no out arguments.
 * @annotations: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_annotation_info_t structures or %NULL if there are no annotations.
 *
 * Information about a method on an D-Bus interface.
 *
 * Since: 2.26
 */
struct _GDBusMethodInfo
{
  /*< public >*/
  xint_t                  ref_count;  /* (atomic) */
  xchar_t                *name;
  xdbus_arg_info_t        **in_args;
  xdbus_arg_info_t        **out_args;
  xdbus_annotation_info_t **annotations;
};

/**
 * xdbus_signalInfo_t:
 * @ref_count: The reference count or -1 if statically allocated.
 * @name: The name of the D-Bus signal, e.g. "NameOwnerChanged".
 * @args: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_arg_info_t structures or %NULL if there are no arguments.
 * @annotations: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_annotation_info_t structures or %NULL if there are no annotations.
 *
 * Information about a signal on a D-Bus interface.
 *
 * Since: 2.26
 */
struct _GDBusSignalInfo
{
  /*< public >*/
  xint_t                  ref_count;  /* (atomic) */
  xchar_t                *name;
  xdbus_arg_info_t        **args;
  xdbus_annotation_info_t **annotations;
};

/**
 * xdbus_property_info_t:
 * @ref_count: The reference count or -1 if statically allocated.
 * @name: The name of the D-Bus property, e.g. "SupportedFilesystems".
 * @signature: The D-Bus signature of the property (a single complete type).
 * @flags: Access control flags for the property.
 * @annotations: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_annotation_info_t structures or %NULL if there are no annotations.
 *
 * Information about a D-Bus property on a D-Bus interface.
 *
 * Since: 2.26
 */
struct _GDBusPropertyInfo
{
  /*< public >*/
  xint_t                      ref_count;  /* (atomic) */
  xchar_t                    *name;
  xchar_t                    *signature;
  GDBusPropertyInfoFlags    flags;
  xdbus_annotation_info_t     **annotations;
};

/**
 * xdbus_interface_info_t:
 * @ref_count: The reference count or -1 if statically allocated.
 * @name: The name of the D-Bus interface, e.g. "org.freedesktop.DBus.Properties".
 * @methods: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_method_info_t structures or %NULL if there are no methods.
 * @signals: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_signalInfo_t structures or %NULL if there are no signals.
 * @properties: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_property_info_t structures or %NULL if there are no properties.
 * @annotations: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_annotation_info_t structures or %NULL if there are no annotations.
 *
 * Information about a D-Bus interface.
 *
 * Since: 2.26
 */
struct _GDBusInterfaceInfo
{
  /*< public >*/
  xint_t                  ref_count;  /* (atomic) */
  xchar_t                *name;
  xdbus_method_info_t     **methods;
  xdbus_signalInfo_t     **signals;
  xdbus_property_info_t   **properties;
  xdbus_annotation_info_t **annotations;
};

/**
 * xdbus_node_info_t:
 * @ref_count: The reference count or -1 if statically allocated.
 * @path: The path of the node or %NULL if omitted. Note that this may be a relative path. See the D-Bus specification for more details.
 * @interfaces: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_interface_info_t structures or %NULL if there are no interfaces.
 * @nodes: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_node_info_t structures or %NULL if there are no nodes.
 * @annotations: (array zero-terminated=1): A pointer to a %NULL-terminated array of pointers to #xdbus_annotation_info_t structures or %NULL if there are no annotations.
 *
 * Information about nodes in a remote object hierarchy.
 *
 * Since: 2.26
 */
struct _GDBusNodeInfo
{
  /*< public >*/
  xint_t                  ref_count;  /* (atomic) */
  xchar_t                *path;
  xdbus_interface_info_t  **interfaces;
  xdbus_node_info_t       **nodes;
  xdbus_annotation_info_t **annotations;
};

XPL_AVAILABLE_IN_ALL
const xchar_t        *g_dbus_annotation_info_lookup          (xdbus_annotation_info_t **annotations,
                                                            const xchar_t          *name);
XPL_AVAILABLE_IN_ALL
xdbus_method_info_t    *g_dbus_interface_info_lookup_method    (xdbus_interface_info_t   *info,
                                                            const xchar_t          *name);
XPL_AVAILABLE_IN_ALL
xdbus_signalInfo_t    *g_dbus_interface_info_lookup_signal    (xdbus_interface_info_t   *info,
                                                            const xchar_t          *name);
XPL_AVAILABLE_IN_ALL
xdbus_property_info_t  *g_dbus_interface_info_lookup_property  (xdbus_interface_info_t   *info,
                                                            const xchar_t          *name);
XPL_AVAILABLE_IN_ALL
void                g_dbus_interface_info_cache_build      (xdbus_interface_info_t   *info);
XPL_AVAILABLE_IN_ALL
void                g_dbus_interface_info_cache_release    (xdbus_interface_info_t   *info);

XPL_AVAILABLE_IN_ALL
void                g_dbus_interface_info_generate_xml     (xdbus_interface_info_t   *info,
                                                            xuint_t                 indent,
                                                            xstring_t              *string_builder);

XPL_AVAILABLE_IN_ALL
xdbus_node_info_t      *g_dbus_node_info_new_for_xml           (const xchar_t          *xml_data,
                                                            xerror_t              **error);
XPL_AVAILABLE_IN_ALL
xdbus_interface_info_t *g_dbus_node_info_lookup_interface      (xdbus_node_info_t        *info,
                                                            const xchar_t          *name);
XPL_AVAILABLE_IN_ALL
void                g_dbus_node_info_generate_xml          (xdbus_node_info_t        *info,
                                                            xuint_t                 indent,
                                                            xstring_t              *string_builder);

XPL_AVAILABLE_IN_ALL
xdbus_node_info_t       *g_dbus_node_info_ref                  (xdbus_node_info_t        *info);
XPL_AVAILABLE_IN_ALL
xdbus_interface_info_t  *g_dbus_interface_info_ref             (xdbus_interface_info_t   *info);
XPL_AVAILABLE_IN_ALL
xdbus_method_info_t     *g_dbus_method_info_ref                (xdbus_method_info_t      *info);
XPL_AVAILABLE_IN_ALL
xdbus_signalInfo_t     *g_dbus_signal_info_ref                (xdbus_signalInfo_t      *info);
XPL_AVAILABLE_IN_ALL
xdbus_property_info_t   *g_dbus_property_info_ref              (xdbus_property_info_t    *info);
XPL_AVAILABLE_IN_ALL
xdbus_arg_info_t        *g_dbus_arg_info_ref                   (xdbus_arg_info_t         *info);
XPL_AVAILABLE_IN_ALL
xdbus_annotation_info_t *g_dbus_annotation_info_ref            (xdbus_annotation_info_t  *info);

XPL_AVAILABLE_IN_ALL
void                 g_dbus_node_info_unref                (xdbus_node_info_t        *info);
XPL_AVAILABLE_IN_ALL
void                 g_dbus_interface_info_unref           (xdbus_interface_info_t   *info);
XPL_AVAILABLE_IN_ALL
void                 g_dbus_method_info_unref              (xdbus_method_info_t      *info);
XPL_AVAILABLE_IN_ALL
void                 g_dbus_signal_info_unref              (xdbus_signalInfo_t      *info);
XPL_AVAILABLE_IN_ALL
void                 g_dbus_property_info_unref            (xdbus_property_info_t    *info);
XPL_AVAILABLE_IN_ALL
void                 g_dbus_arg_info_unref                 (xdbus_arg_info_t         *info);
XPL_AVAILABLE_IN_ALL
void                 g_dbus_annotation_info_unref          (xdbus_annotation_info_t  *info);

/**
 * XTYPE_DBUS_NODE_INFO:
 *
 * The #xtype_t for a boxed type holding a #xdbus_node_info_t.
 *
 * Since: 2.26
 */
#define XTYPE_DBUS_NODE_INFO       (g_dbus_node_info_get_type ())

/**
 * XTYPE_DBUS_INTERFACE_INFO:
 *
 * The #xtype_t for a boxed type holding a #xdbus_interface_info_t.
 *
 * Since: 2.26
 */
#define XTYPE_DBUS_INTERFACE_INFO  (g_dbus_interface_info_get_type ())

/**
 * XTYPE_DBUS_METHOD_INFO:
 *
 * The #xtype_t for a boxed type holding a #xdbus_method_info_t.
 *
 * Since: 2.26
 */
#define XTYPE_DBUS_METHOD_INFO     (g_dbus_method_info_get_type ())

/**
 * XTYPE_DBUS_SIGNAL_INFO:
 *
 * The #xtype_t for a boxed type holding a #xdbus_signalInfo_t.
 *
 * Since: 2.26
 */
#define XTYPE_DBUS_SIGNAL_INFO     (g_dbus_signal_info_get_type ())

/**
 * XTYPE_DBUS_PROPERTY_INFO:
 *
 * The #xtype_t for a boxed type holding a #xdbus_property_info_t.
 *
 * Since: 2.26
 */
#define XTYPE_DBUS_PROPERTY_INFO   (g_dbus_property_info_get_type ())

/**
 * XTYPE_DBUS_ARG_INFO:
 *
 * The #xtype_t for a boxed type holding a #xdbus_arg_info_t.
 *
 * Since: 2.26
 */
#define XTYPE_DBUS_ARG_INFO        (g_dbus_arg_info_get_type ())

/**
 * XTYPE_DBUS_ANNOTATION_INFO:
 *
 * The #xtype_t for a boxed type holding a #xdbus_annotation_info_t.
 *
 * Since: 2.26
 */
#define XTYPE_DBUS_ANNOTATION_INFO (g_dbus_annotation_info_get_type ())

XPL_AVAILABLE_IN_ALL
xtype_t g_dbus_node_info_get_type       (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t g_dbus_interface_info_get_type  (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t g_dbus_method_info_get_type     (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t g_dbus_signal_info_get_type     (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t g_dbus_property_info_get_type   (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t g_dbus_arg_info_get_type        (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xtype_t g_dbus_annotation_info_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __G_DBUS_INTROSPECTION_H__ */
