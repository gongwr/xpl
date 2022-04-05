/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2017 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __G_OPEN_URI_PORTAL_H__

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

xboolean_t g_openuri_portal_open_uri        (const char  *uri,
                                           const char  *parent_window,
                                           xerror_t     **error);

void     g_openuri_portal_open_uri_async  (const char          *uri,
                                           const char          *parent_window,
                                           xcancellable_t        *cancellable,
                                           xasync_ready_callback_t  callback,
                                           xpointer_t             user_data);

xboolean_t g_openuri_portal_open_uri_finish (xasync_result_t        *result,
                                           xerror_t             **error);

G_END_DECLS

#endif
