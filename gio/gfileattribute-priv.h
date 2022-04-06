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

#ifndef __XFILE_ATTRIBUTE_PRIV_H__
#define __XFILE_ATTRIBUTE_PRIV_H__

#include "gfileattribute.h"
#include "gfileinfo.h"

#define XFILE_ATTRIBUTE_VALUE_INIT {0}

typedef struct  {
  xfile_attribute_type_t type : 8;
  xfile_attribute_status_t status : 8;
  union {
    xboolean_t boolean;
    gint32 int32;
    xuint32_t uint32;
    sint64_t int64;
    xuint64_t uint64;
    char *string;
    xobject_t *obj;
    char **stringv;
  } u;
} GFileAttributeValue;

GFileAttributeValue *_xfile_attribute_value_new             (void);
void                 _xfile_attribute_value_free            (GFileAttributeValue *attr);
void                 _xfile_attribute_value_clear           (GFileAttributeValue *attr);
void                 _xfile_attribute_value_set             (GFileAttributeValue *attr,
							      const GFileAttributeValue *new_value);
GFileAttributeValue *_xfile_attribute_value_dup             (const GFileAttributeValue *other);
xpointer_t             _xfile_attribute_value_peek_as_pointer (GFileAttributeValue *attr);

char *               _xfile_attribute_value_as_string       (const GFileAttributeValue *attr);

const char *         _xfile_attribute_value_get_string      (const GFileAttributeValue *attr);
const char *         _xfile_attribute_value_get_byte_string (const GFileAttributeValue *attr);
xboolean_t             _xfile_attribute_value_get_boolean     (const GFileAttributeValue *attr);
xuint32_t              _xfile_attribute_value_get_uint32      (const GFileAttributeValue *attr);
gint32               _xfile_attribute_value_get_int32       (const GFileAttributeValue *attr);
xuint64_t              _xfile_attribute_value_get_uint64      (const GFileAttributeValue *attr);
sint64_t               _xfile_attribute_value_get_int64       (const GFileAttributeValue *attr);
xobject_t *            _xfile_attribute_value_get_object      (const GFileAttributeValue *attr);
char **              _xfile_attribute_value_get_stringv     (const GFileAttributeValue *attr);

void                 _xfile_attribute_value_set_from_pointer(GFileAttributeValue *attr,
							      xfile_attribute_type_t   type,
							      xpointer_t             value_p,
							      xboolean_t             dup);
void                 _xfile_attribute_value_set_string      (GFileAttributeValue *attr,
							      const char          *string);
void                 _xfile_attribute_value_set_byte_string (GFileAttributeValue *attr,
							      const char          *string);
void                 _xfile_attribute_value_set_boolean     (GFileAttributeValue *attr,
							      xboolean_t             value);
void                 _xfile_attribute_value_set_uint32      (GFileAttributeValue *attr,
							      xuint32_t              value);
void                 _xfile_attribute_value_set_int32       (GFileAttributeValue *attr,
							      gint32               value);
void                 _xfile_attribute_value_set_uint64      (GFileAttributeValue *attr,
							      xuint64_t              value);
void                 _xfile_attribute_value_set_int64       (GFileAttributeValue *attr,
							      sint64_t               value);
void                 _xfile_attribute_value_set_object      (GFileAttributeValue *attr,
							      xobject_t             *obj);
void                 _xfile_attribute_value_set_stringv     (GFileAttributeValue *attr,
							      char               **value);


GFileAttributeValue *_xfile_info_get_attribute_value (xfile_info_t  *info,
						       const char *attribute);

#endif /* __XFILE_ATTRIBUTE_PRIV_H__ */
