/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __XFILE_ATTRIBUTE_H__
#define __XFILE_ATTRIBUTE_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/giotypes.h>

G_BEGIN_DECLS

/**
 * xfile_attribute_info_t:
 * @name: the name of the attribute.
 * @type: the #xfile_attribute_type_t type of the attribute.
 * @flags: a set of #xfile_attribute_info_flags_t.
 *
 * Information about a specific attribute.
 **/
struct _GFileAttributeInfo
{
  char                    *name;
  xfile_attribute_type_t       type;
  xfile_attribute_info_flags_t  flags;
};

/**
 * xfile_attribute_info_list_t:
 * @infos: an array of #GFileAttributeInfos.
 * @n_infos: the number of values in the array.
 *
 * Acts as a lightweight registry for possible valid file attributes.
 * The registry stores Key-Value pair formats as #GFileAttributeInfos.
 **/
struct _GFileAttributeInfoList
{
  xfile_attribute_info_t *infos;
  int                 n_infos;
};

#define XTYPE_FILE_ATTRIBUTE_INFO_LIST (xfile_attribute_info_list_get_type ())
XPL_AVAILABLE_IN_ALL
xtype_t xfile_attribute_info_list_get_type (void);

XPL_AVAILABLE_IN_ALL
xfile_attribute_info_list_t *  xfile_attribute_info_list_new    (void);
XPL_AVAILABLE_IN_ALL
xfile_attribute_info_list_t *  xfile_attribute_info_list_ref    (xfile_attribute_info_list_t *list);
XPL_AVAILABLE_IN_ALL
void                      xfile_attribute_info_list_unref  (xfile_attribute_info_list_t *list);
XPL_AVAILABLE_IN_ALL
xfile_attribute_info_list_t *  xfile_attribute_info_list_dup    (xfile_attribute_info_list_t *list);
XPL_AVAILABLE_IN_ALL
const xfile_attribute_info_t *xfile_attribute_info_list_lookup (xfile_attribute_info_list_t *list,
							     const char             *name);
XPL_AVAILABLE_IN_ALL
void                      xfile_attribute_info_list_add    (xfile_attribute_info_list_t *list,
							     const char             *name,
							     xfile_attribute_type_t      type,
							     xfile_attribute_info_flags_t flags);

G_END_DECLS

#endif /* __XFILE_INFO_H__ */
