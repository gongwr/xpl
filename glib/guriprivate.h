/*
 * Copyright © 2020 Red Hat, Inc.
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
 *
 * Author: Marc-André Lureau <marcandre.lureau@redhat.com>
 */

#ifndef __XURI_PRIVATE_H__
#define __XURI_PRIVATE_H__

#include "gtypes.h"

G_BEGIN_DECLS

void
_uri_encoder (xstring_t      *out,
              const xuchar_t *start,
              xsize_t         length,
              const xchar_t  *reserved_chars_allowed,
              xboolean_t      allow_utf8);

G_END_DECLS

#endif /* __XURI_PRIVATE_H__ */
