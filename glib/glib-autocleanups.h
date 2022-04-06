/*
 * Copyright © 2015 Canonical Limited
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
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

static inline void
g_autoptr_cleanup_generic_gfree (void *p)
{
  void **pp = (void**)p;
  g_free (*pp);
}

static inline void
g_autoptr_cleanup_xstring_free (xstring_t *string)
{
  if (string)
    xstring_free (string, TRUE);
}

/* Ignore deprecations in case we refer to a type which was added in a more
 * recent GLib version than the user’s #XPL_VERSION_MAX_ALLOWED definition. */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/* If adding a cleanup here, please also add a test case to
 * glib/tests/autoptr.c
 */
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xasync_queue, g_async_queue_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xbookmark_file, g_bookmark_file_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xbytes, xbytes_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xchecksum, xchecksum_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xdatetime, xdate_time_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xdate, xdate_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xdir, g_dir_close)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xerror, xerror_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xhashtable, xhash_table_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xhmac, g_hmac_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xio_channel, g_io_channel_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xkey_file, xkey_file_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xlist, xlist_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xarray, g_array_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xptr_array, xptr_array_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xbyte_array, xbyte_array_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xmain_context, xmain_context_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xmain_context_pusher, xmain_context_pusher_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xmain_loop, xmain_loop_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xsource, xsource_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xmapped_file, xmapped_file_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xmarkup_parse_context, xmarkup_parse_context_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xnode, g_node_destroy)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xoption_context, g_option_context_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xoption_group, xoption_group_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xpattern_spec, xpattern_spec_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xqueue, g_queue_free)
G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(xqueue, g_queue_clear)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xrand, g_rand_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xregex, xregex_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xmatch_info, xmatch_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xscanner, g_scanner_destroy)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xsequence, g_sequence_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xslist, xslist_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xstring, g_autoptr_cleanup_xstring_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xstring_chunk, xstring_chunk_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xstrv_builder, xstrv_builder_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xthread, xthread_unref)
G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(xmutex, g_mutex_clear)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xmutex_locker, g_mutex_locker_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xrec_mutex_locker, g_rec_mutex_locker_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xrwlock_writer_locker, g_rw_lock_writer_locker_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xrwlock_reader_locker, g_rw_lock_reader_locker_free)
G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(xcond, g_cond_clear)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xtimer, g_timer_destroy)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xtimezone, xtime_zone_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xtree, xtree_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xvariant, xvariant_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xvariant_builder, xvariant_builder_unref)
G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(xvariant_builder, xvariant_builder_clear)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xvariant_iter, xvariant_iter_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xvariant_dict, xvariant_dict_unref)
G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(xvariant_dict, xvariant_dict_clear)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xvariant_type, xvariant_type_free)
G_DEFINE_AUTO_CLEANUP_FREE_FUNC(xstrv, xstrfreev, NULL)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xref_string, g_ref_string_release)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(xuri, xuri_unref)

G_GNUC_END_IGNORE_DEPRECATIONS
