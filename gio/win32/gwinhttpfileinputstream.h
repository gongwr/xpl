/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
 * Copyright (C) 2008 Novell, Inc.
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
 * Author: Tor Lillqvist <tml@novell.com>
 */

#ifndef __XWINHTTP_FILE_INPUT_STREAM_H__
#define __XWINHTTP_FILE_INPUT_STREAM_H__

#include <gio/gfileinputstream.h>

#include "gwinhttpfile.h"

G_BEGIN_DECLS

#define XTYPE_WINHTTP_FILE_INPUT_STREAM         (_g_winhttp_file_input_stream_get_type ())
#define XWINHTTP_FILE_INPUT_STREAM(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_WINHTTP_FILE_INPUT_STREAM, GWinHttpFileInputStream))
#define XWINHTTP_FILE_INPUT_STREAM_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), XTYPE_WINHTTP_FILE_INPUT_STREAM, GWinHttpFileInputStreamClass))
#define X_IS_WINHTTP_FILE_INPUT_STREAM(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_WINHTTP_FILE_INPUT_STREAM))
#define X_IS_WINHTTP_FILE_INPUT_STREAM_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_WINHTTP_FILE_INPUT_STREAM))
#define XWINHTTP_FILE_INPUT_STREAM_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_WINHTTP_FILE_INPUT_STREAM, GWinHttpFileInputStreamClass))

typedef struct _GWinHttpFileInputStream         GWinHttpFileInputStream;
typedef struct _GWinHttpFileInputStreamClass    GWinHttpFileInputStreamClass;

xtype_t _g_winhttp_file_input_stream_get_type (void) G_GNUC_CONST;

xfile_input_stream_t *_g_winhttp_file_input_stream_new (xwin_http_file_t *file,
                                                    HINTERNET     connection,
                                                    HINTERNET     request);

G_END_DECLS

#endif /* __XWINHTTP_FILE_INPUT_STREAM_H__ */
