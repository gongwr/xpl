/*
 * Copyright © 2018 Endless Mobile, Inc.
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
 * Author: Philip Withnall <withnall@endlessm.com>
 */

#ifndef __G_UTILS_PRIVATE_H__
#define __G_UTILS_PRIVATE_H__

#include "glibconfig.h"
#include "gtypes.h"
#include "gtestutils.h"

G_BEGIN_DECLS

XPL_AVAILABLE_IN_2_60
void g_set_user_dirs (const xchar_t *first_dir_type,
                      ...) G_GNUC_NULL_TERMINATED;

/* Returns the smallest power of 2 greater than or equal to n,
 * or 0 if such power does not fit in a xsize_t
 */
static inline xsize_t
g_nearest_pow (xsize_t num)
{
  xsize_t n = num - 1;

  g_assert (num > 0 && num <= G_MAXSIZE / 2);

  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
#if XPL_SIZEOF_SIZE_T == 8
  n |= n >> 32;
#endif

  return n + 1;
}

G_END_DECLS

#endif /* __G_UTILS_PRIVATE_H__ */
