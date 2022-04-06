/*
 * Copyright 2015 Red Hat, Inc.
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
 * Author: Matthias Clasen <mclasen@redhat.com>
 */

#ifndef __GIO_TOOL_H__
#define __GIO_TOOL_H__

void print_error      (const xchar_t    *format,
                       ...) G_GNUC_PRINTF (1, 2);
void print_file_error (xfile_t          *file,
                       const xchar_t    *message);
void show_help        (xoption_context_t *context,
                       const char     *message);

const char         *file_type_to_string        (xfile_type_t                type);
const char         *attribute_type_to_string   (xfile_attribute_type_t       type);
xfile_attribute_type_t  attribute_type_from_string (const char              *str);
char               *attribute_flags_to_string  (xfile_attribute_info_flags_t  flags);

xboolean_t file_is_dir (xfile_t *file);

int handle_cat     (int argc, char *argv[], xboolean_t do_help);
int handle_copy    (int argc, char *argv[], xboolean_t do_help);
int handle_info    (int argc, char *argv[], xboolean_t do_help);
int handle_launch  (int argc, char *argv[], xboolean_t do_help);
int handle_list    (int argc, char *argv[], xboolean_t do_help);
int handle_mime    (int argc, char *argv[], xboolean_t do_help);
int handle_mkdir   (int argc, char *argv[], xboolean_t do_help);
int handle_monitor (int argc, char *argv[], xboolean_t do_help);
int handle_mount   (int argc, char *argv[], xboolean_t do_help);
int handle_move    (int argc, char *argv[], xboolean_t do_help);
int handle_open    (int argc, char *argv[], xboolean_t do_help);
int handle_rename  (int argc, char *argv[], xboolean_t do_help);
int handle_remove  (int argc, char *argv[], xboolean_t do_help);
int handle_save    (int argc, char *argv[], xboolean_t do_help);
int handle_set     (int argc, char *argv[], xboolean_t do_help);
int handle_trash   (int argc, char *argv[], xboolean_t do_help);
int handle_tree    (int argc, char *argv[], xboolean_t do_help);

#endif  /* __GIO_TOOL_H__ */
