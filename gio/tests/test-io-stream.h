/* Simple I/O stream. This is a utility module for tests, not a test.
 *
 * Copyright © 2008-2010 Red Hat, Inc.
 * Copyright © 2011 Nokia Corporation
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
 * Author: Simon McVittie <simon.mcvittie@collabora.co.uk>
 */

#ifndef TEST_IO_STREAM_H
#define TEST_IO_STREAM_H

#include <gio/gio.h>

typedef struct
{
  xio_stream_t parent_instance;
  xinput_stream_t *input_stream;
  xoutput_stream_t *output_stream;
} TestIOStream;

typedef struct
{
  xio_stream_class_t parent_class;
} TestIOStreamClass;

xtype_t test_io_stream_get_type (void) G_GNUC_CONST;

#define TEST_TYPE_IO_STREAM  (test_io_stream_get_type ())
#define TEST_IO_STREAM(o)    (XTYPE_CHECK_INSTANCE_CAST ((o), \
                              TEST_TYPE_IO_STREAM, TestIOStream))
#define TEST_IS_IO_STREAM(o) (XTYPE_CHECK_INSTANCE_TYPE ((o), \
                                                          TEST_TYPE_IO_STREAM))

xio_stream_t *test_io_stream_new (xinput_stream_t  *input_stream,
                               xoutput_stream_t *output_stream);

#endif /* guard */
