/* XPL - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_COMPLETION_H__
#define __G_COMPLETION_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/glist.h>

G_BEGIN_DECLS

typedef struct _GCompletion     GCompletion;

typedef xchar_t*          (*GCompletionFunc)      (xpointer_t);

/* GCompletion
 */

typedef xint_t (*GCompletionStrncmpFunc) (const xchar_t *s1,
                                        const xchar_t *s2,
                                        xsize_t        n);

struct _GCompletion
{
  xlist_t* items;
  GCompletionFunc func;

  xchar_t* prefix;
  xlist_t* cache;
  GCompletionStrncmpFunc strncmp_func;
};

XPL_DEPRECATED_IN_2_26
GCompletion* g_completion_new           (GCompletionFunc func);
XPL_DEPRECATED_IN_2_26
void         g_completion_add_items     (GCompletion*    cmp,
                                         xlist_t*          items);
XPL_DEPRECATED_IN_2_26
void         g_completion_remove_items  (GCompletion*    cmp,
                                         xlist_t*          items);
XPL_DEPRECATED_IN_2_26
void         g_completion_clear_items   (GCompletion*    cmp);
XPL_DEPRECATED_IN_2_26
xlist_t*       g_completion_complete      (GCompletion*    cmp,
                                         const xchar_t*    prefix,
                                         xchar_t**         new_prefix);
XPL_DEPRECATED_IN_2_26
xlist_t*       g_completion_complete_utf8 (GCompletion  *cmp,
                                         const xchar_t*    prefix,
                                         xchar_t**         new_prefix);
XPL_DEPRECATED_IN_2_26
void         g_completion_set_compare   (GCompletion *cmp,
                                         GCompletionStrncmpFunc strncmp_func);
XPL_DEPRECATED_IN_2_26
void         g_completion_free          (GCompletion*    cmp);

G_END_DECLS

#endif /* __G_COMPLETION_H__ */
