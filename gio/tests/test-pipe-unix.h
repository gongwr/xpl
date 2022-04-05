/* Unix pipe-to-self. This is a utility module for tests, not a test.
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
 * Author: Simon McVittie <simon.mcvittie@collabora.co.uk>
 */

#ifndef TEST_PIPE_UNIX_H
#define TEST_PIPE_UNIX_H

#include <gio/gio.h>

xboolean_t test_pipe (xinput_stream_t  **is,
                    xoutput_stream_t **os,
                    xerror_t        **error);

xboolean_t test_bidi_pipe (xio_stream_t **left,
                         xio_stream_t **right,
                         xerror_t    **error);

#endif /* guard */
