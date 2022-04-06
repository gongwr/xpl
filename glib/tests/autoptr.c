#include <glib.h>
#include <string.h>

typedef struct _HNVC has_non_void_cleanup_t;
has_non_void_cleanup_t * non_void_cleanup (has_non_void_cleanup_t *);

/* Should not cause any warnings with -Wextra */
G_DEFINE_AUTOPTR_CLEANUP_FUNC(has_non_void_cleanup, non_void_cleanup)

static void
test_autofree (void)
{
#ifdef __clang_analyzer__
  g_test_skip ("autofree tests aren’t understood by the clang analyser");
#else
  g_autofree xchar_t *p = NULL;
  g_autofree xchar_t *p2 = NULL;
  g_autofree xchar_t *alwaysnull = NULL;

  p = g_malloc (10);
  p2 = g_malloc (42);

  if (TRUE)
    {
      g_autofree xuint8_t *buf = g_malloc (128);
      g_autofree xchar_t *alwaysnull_again = NULL;

      buf[0] = 1;

      g_assert_null (alwaysnull_again);
    }

  if (TRUE)
    {
      g_autofree xuint8_t *buf2 = g_malloc (256);

      buf2[255] = 42;
    }

  g_assert_null (alwaysnull);
#endif  /* __clang_analyzer__ */
}

static void
test_g_async_queue (void)
{
  x_autoptr(xasync_queue) val = g_async_queue_new ();
  g_assert_nonnull (val);
}

static void
test_g_bookmark_file (void)
{
  x_autoptr(xbookmark_file) val = g_bookmark_file_new ();
  g_assert_nonnull (val);
}

static void
test_xbytes (void)
{
  x_autoptr(xbytes) val = xbytes_new ("foo", 3);
  g_assert_nonnull (val);
}

static void
test_xchecksum (void)
{
  x_autoptr(xchecksum) val = xchecksum_new (G_CHECKSUM_SHA256);
  g_assert_nonnull (val);
}

static void
test_xdate (void)
{
  x_autoptr(xdate) val = xdate_new ();
  g_assert_nonnull (val);
}

static void
test_xdate_time (void)
{
  x_autoptr(xdatetime) val = xdate_time_new_now_utc ();
  g_assert_nonnull (val);
}

static void
test_g_dir (void)
{
  x_autoptr(xdir) val = g_dir_open (".", 0, NULL);
  g_assert_nonnull (val);
}

static void
test_xerror (void)
{
  x_autoptr(xerror) val = xerror_new_literal (XFILE_ERROR, XFILE_ERROR_FAILED, "oops");
  g_assert_nonnull (val);
}

static void
test_xhash_table (void)
{
  x_autoptr(xhashtable) val = xhash_table_new (NULL, NULL);
  g_assert_nonnull (val);
}

static void
test_g_hmac (void)
{
  x_autoptr(xhmac) val = g_hmac_new (G_CHECKSUM_SHA256, (xuint8_t*)"hello", 5);
  g_assert_nonnull (val);
}

static void
test_xio_channel (void)
{
#ifdef G_OS_WIN32
  const xchar_t *devnull = "nul";
#else
  const xchar_t *devnull = "/dev/null";
#endif

  x_autoptr(xio_channel) val = g_io_channel_new_file (devnull, "r", NULL);
  g_assert_nonnull (val);
}

static void
test_xkey_file (void)
{
  x_autoptr(xkey_file) val = xkey_file_new ();
  g_assert_nonnull (val);
}

static void
test_g_list (void)
{
  x_autoptr(xlist) val = NULL;
  x_autoptr(xlist) val2 = xlist_prepend (NULL, "foo");
  g_assert_null (val);
  g_assert_nonnull (val2);
}

static void
test_g_array (void)
{
  x_autoptr(xarray) val = g_array_new (0, 0, sizeof (xpointer_t));
  g_assert_nonnull (val);
}

static void
test_xptr_array (void)
{
  x_autoptr(xptr_array) val = xptr_array_new ();
  g_assert_nonnull (val);
}

static void
test_xbyte_array (void)
{
  x_autoptr(xbyte_array) val = xbyte_array_new ();
  g_assert_nonnull (val);
}

static void
test_xmain_context (void)
{
  x_autoptr(xmain_context) val = xmain_context_new ();
  g_assert_nonnull (val);
}

static void
test_xmain_context_pusher (void)
{
  xmain_context_t *context, *old_thread_default;

  context = xmain_context_new ();
  old_thread_default = xmain_context_get_thread_default ();
  g_assert_false (old_thread_default == context);

  if (TRUE)
    {
      x_autoptr(xmain_context_pusher) val = xmain_context_pusher_new (context);
      g_assert_nonnull (val);

      /* Check it’s now the thread-default main context */
      g_assert_true (xmain_context_get_thread_default () == context);
    }

  /* Check it’s now the old thread-default main context */
  g_assert_false (xmain_context_get_thread_default () == context);
  g_assert_true (xmain_context_get_thread_default () == old_thread_default);

  xmain_context_unref (context);
}

static void
test_xmain_loop (void)
{
  x_autoptr(xmain_loop) val = xmain_loop_new (NULL, TRUE);
  g_assert_nonnull (val);
}

static void
test_xsource (void)
{
  x_autoptr(xsource) val = g_timeout_source_new_seconds (2);
  g_assert_nonnull (val);
}

static void
test_xmapped_file (void)
{
  x_autoptr(xmapped_file) val = xmapped_file_new (g_test_get_filename (G_TEST_DIST, "keyfiletest.ini", NULL), FALSE, NULL);
  g_assert_nonnull (val);
}

static void
parser_start (xmarkup_parse_context_t  *context,
              const xchar_t          *element_name,
              const xchar_t         **attribute_names,
              const xchar_t         **attribute_values,
              xpointer_t              user_data,
              xerror_t              **error)
{
}

static void
parser_end (xmarkup_parse_context_t  *context,
            const xchar_t          *element_name,
            xpointer_t              user_data,
            xerror_t              **error)
{
}

static GMarkupParser parser = {
  .start_element = parser_start,
  .end_element = parser_end
};

static void
test_xmarkup_parse_context (void)
{
  x_autoptr(xmarkup_parse_context) val = xmarkup_parse_context_new (&parser,  0, NULL, NULL);
  g_assert_nonnull (val);
}

static void
test_g_node (void)
{
  x_autoptr(xnode) val = g_node_new ("hello");
  g_assert_nonnull (val);
}

static void
test_g_option_context (void)
{
  x_autoptr(xoption_context) val = g_option_context_new ("hello");
  g_assert_nonnull (val);
}

static void
test_xoption_group (void)
{
  x_autoptr(xoption_group) val = xoption_group_new ("hello", "world", "helpme", NULL, NULL);
  g_assert_nonnull (val);
}

static void
test_xpattern_spec (void)
{
  x_autoptr(xpattern_spec) val = xpattern_spec_new ("plaid");
  g_assert_nonnull (val);
}

static void
test_g_queue (void)
{
  x_autoptr(xqueue) val = g_queue_new ();
  x_auto(xqueue) stackval = G_QUEUE_INIT;
  g_assert_nonnull (val);
  g_assert_null (stackval.head);
}

static void
test_g_rand (void)
{
  x_autoptr(xrand) val = g_rand_new ();
  g_assert_nonnull (val);
}

static void
test_xregex (void)
{
  x_autoptr(xregex) val = xregex_new (".*", 0, 0, NULL);
  g_assert_nonnull (val);
}

static void
test_xmatch_info (void)
{
  x_autoptr(xregex) regex = xregex_new (".*", 0, 0, NULL);
  x_autoptr(xmatch_info) match = NULL;

  if (!xregex_match (regex, "hello", 0, &match))
    g_assert_not_reached ();
}

static void
test_g_scanner (void)
{
  GScannerConfig config = { 0, };
  x_autoptr(xscanner) val = g_scanner_new (&config);
  g_assert_nonnull (val);
}

static void
test_g_sequence (void)
{
  x_autoptr(xsequence) val = g_sequence_new (NULL);
  g_assert_nonnull (val);
}

static void
test_g_slist (void)
{
  x_autoptr(xslist) val = NULL;
  x_autoptr(xslist) nonempty_val = xslist_prepend (NULL, "hello");
  g_assert_null (val);
  g_assert_nonnull (nonempty_val);
}

static void
test_xstring (void)
{
  x_autoptr(xstring) val = xstring_new ("");
  g_assert_nonnull (val);
}

static void
test_xstring_chunk (void)
{
  x_autoptr(xstring_chunk) val = xstring_chunk_new (42);
  g_assert_nonnull (val);
}

static xpointer_t
mythread (xpointer_t data)
{
  g_usleep (G_USEC_PER_SEC);
  return NULL;
}

static void
test_xthread (void)
{
  x_autoptr(xthread) val = xthread_new ("bob", mythread, NULL);
  g_assert_nonnull (val);
}

static void
test_g_mutex (void)
{
  x_auto(xmutex) val;

  g_mutex_init (&val);
}

/* Thread function to check that a mutex given in @data is locked */
static xpointer_t
mutex_locked_thread (xpointer_t data)
{
  xmutex_t *mutex = (xmutex_t *) data;
  g_assert_false (g_mutex_trylock (mutex));
  return NULL;
}

/* Thread function to check that a mutex given in @data is unlocked */
static xpointer_t
mutex_unlocked_thread (xpointer_t data)
{
  xmutex_t *mutex = (xmutex_t *) data;
  g_assert_true (g_mutex_trylock (mutex));
  g_mutex_unlock (mutex);
  return NULL;
}

static void
test_g_mutex_locker (void)
{
  xmutex_t mutex;
  xthread_t *thread;

  g_mutex_init (&mutex);

  if (TRUE)
    {
      x_autoptr(xmutex_locker) val = g_mutex_locker_new (&mutex);

      g_assert_nonnull (val);

      /* Verify that the mutex is actually locked */
      thread = xthread_new ("mutex locked", mutex_locked_thread, &mutex);
      xthread_join (thread);
    }

    /* Verify that the mutex is unlocked again */
    thread = xthread_new ("mutex unlocked", mutex_unlocked_thread, &mutex);
    xthread_join (thread);
}

/* Thread function to check that a recursive mutex given in @data is locked */
static xpointer_t
rec_mutex_locked_thread (xpointer_t data)
{
  GRecMutex *rec_mutex = (GRecMutex *) data;
  g_assert_false (g_rec_mutex_trylock (rec_mutex));
  return NULL;
}

/* Thread function to check that a recursive mutex given in @data is unlocked */
static xpointer_t
rec_mutex_unlocked_thread (xpointer_t data)
{
  GRecMutex *rec_mutex = (GRecMutex *) data;
  g_assert_true (g_rec_mutex_trylock (rec_mutex));
  return NULL;
}

static void
test_g_rec_mutex_locker (void)
{
  GRecMutex rec_mutex;
  xthread_t *thread;

  g_rec_mutex_init (&rec_mutex);

  if (TRUE)
    {
      x_autoptr(xrec_mutex_locker) val = g_rec_mutex_locker_new (&rec_mutex);

      g_assert_nonnull (val);

      /* Verify that the mutex is actually locked */
      thread = xthread_new ("rec mutex locked", rec_mutex_locked_thread, &rec_mutex);
      xthread_join (thread);
    }

  /* Verify that the mutex is unlocked again */
  thread = xthread_new ("rec mutex unlocked", rec_mutex_unlocked_thread, &rec_mutex);
  xthread_join (thread);

  g_rec_mutex_clear (&rec_mutex);
}

/* Thread function to check that an rw lock given in @data cannot be writer locked */
static xpointer_t
rw_lock_cannot_take_writer_lock_thread (xpointer_t data)
{
  GRWLock *lock = (GRWLock *) data;
  g_assert_false (g_rw_lock_writer_trylock (lock));
  return NULL;
}

/* Thread function to check that an rw lock given in @data can be reader locked */
static xpointer_t
rw_lock_can_take_reader_lock_thread (xpointer_t data)
{
  GRWLock *lock = (GRWLock *) data;
  g_assert_true (g_rw_lock_reader_trylock (lock));
  g_rw_lock_reader_unlock (lock);
  return NULL;
}

static void
test_g_rw_lock_lockers (void)
{
  GRWLock lock;
  xthread_t *thread;

  g_rw_lock_init (&lock);

  if (TRUE)
    {
      x_autoptr(xrwlock_writer_locker) val = g_rw_lock_writer_locker_new (&lock);

      g_assert_nonnull (val);

      /* Verify that we cannot take another writer lock as a writer lock is currently held */
      thread = xthread_new ("rw lock cannot take writer lock", rw_lock_cannot_take_writer_lock_thread, &lock);
      xthread_join (thread);

      /* Verify that we cannot take a reader lock as a writer lock is currently held */
      g_assert_false (g_rw_lock_reader_trylock (&lock));
    }

  if (TRUE)
    {
      x_autoptr(xrwlock_reader_locker) val = g_rw_lock_reader_locker_new (&lock);

      g_assert_nonnull (val);

      /* Verify that we can take another reader lock from another thread */
      thread = xthread_new ("rw lock can take reader lock", rw_lock_can_take_reader_lock_thread, &lock);
      xthread_join (thread);

      /* ... and also that recursive reader locking from the same thread works */
      g_assert_true (g_rw_lock_reader_trylock (&lock));
      g_rw_lock_reader_unlock (&lock);

      /* Verify that we cannot take a writer lock as a reader lock is currently held */
      thread = xthread_new ("rw lock cannot take writer lock", rw_lock_cannot_take_writer_lock_thread, &lock);
      xthread_join (thread);
    }

  /* Verify that we can take a writer lock again: this can only work if all of
   * the locks taken above have been correctly released. */
  g_assert_true (g_rw_lock_writer_trylock (&lock));
  g_rw_lock_writer_unlock (&lock);

  g_rw_lock_clear (&lock);
}

static void
test_xcond (void)
{
  x_auto(xcond) val;
  g_cond_init (&val);
}

static void
test_xtimer (void)
{
  x_autoptr(xtimer) val = g_timer_new ();
  g_assert_nonnull (val);
}

static void
test_xtimezone (void)
{
  x_autoptr(xtimezone) val = xtime_zone_new_utc ();
  g_assert_nonnull (val);
}

static void
test_xtree (void)
{
  x_autoptr(xtree) val = xtree_new ((GCompareFunc)strcmp);
  g_assert_nonnull (val);
}

static void
test_xvariant (void)
{
  x_autoptr(xvariant) val = xvariant_new_string ("hello");
  g_assert_nonnull (val);
}

static void
test_xvariant_builder (void)
{
  x_autoptr(xvariant_builder) val = xvariant_builder_new (G_VARIANT_TYPE ("as"));
  x_auto(xvariant_builder) stackval;

  g_assert_nonnull (val);
  xvariant_builder_init (&stackval, G_VARIANT_TYPE ("as"));
}

static void
test_xvariant_iter (void)
{
  x_autoptr(xvariant) var = xvariant_new_fixed_array (G_VARIANT_TYPE_UINT32, "", 0, sizeof(xuint32_t));
  x_autoptr(xvariant_iter) val = xvariant_iter_new (var);
  g_assert_nonnull (val);
}

static void
test_xvariant_dict (void)
{
  x_autoptr(xvariant) data = xvariant_new_from_data (G_VARIANT_TYPE ("a{sv}"), "", 0, FALSE, NULL, NULL);
  x_auto(xvariant_dict) stackval;
  x_autoptr(xvariant_dict) val = xvariant_dict_new (data);

  xvariant_dict_init (&stackval, data);
  g_assert_nonnull (val);
}

static void
test_xvariant_type (void)
{
  x_autoptr(xvariant_type) val = xvariant_type_new ("s");
  g_assert_nonnull (val);
}

static void
test_strv (void)
{
  x_auto(xstrv) val = xstrsplit("a:b:c", ":", -1);
  g_assert_nonnull (val);
}

static void
test_refstring (void)
{
  x_autoptr(xref_string) str = g_ref_string_new ("hello, world");
  g_assert_nonnull (str);
}

static void
mark_freed (xpointer_t ptr)
{
  xboolean_t *freed = ptr;
  *freed = TRUE;
}

static void
test_autolist (void)
{
  char data[1] = {0};
  xboolean_t freed1 = FALSE;
  xboolean_t freed2 = FALSE;
  xboolean_t freed3 = FALSE;
  xbytes_t *b1 = xbytes_new_with_free_func (data, sizeof(data), mark_freed, &freed1);
  xbytes_t *b2 = xbytes_new_with_free_func (data, sizeof(data), mark_freed, &freed2);
  xbytes_t *b3 = xbytes_new_with_free_func (data, sizeof(data), mark_freed, &freed3);

  {
    g_autolist(xbytes) l = NULL;

    l = xlist_prepend (l, b1);
    l = xlist_prepend (l, b3);

    /* Squash warnings about dead stores */
    (void) l;
  }

  /* Only assert if autoptr works */
#ifdef __GNUC__
  g_assert_true (freed1);
  g_assert_true (freed3);
#endif
  g_assert_false (freed2);

  xbytes_unref (b2);
  g_assert_true (freed2);
}

static void
test_autoslist (void)
{
  char data[1] = {0};
  xboolean_t freed1 = FALSE;
  xboolean_t freed2 = FALSE;
  xboolean_t freed3 = FALSE;
  xbytes_t *b1 = xbytes_new_with_free_func (data, sizeof(data), mark_freed, &freed1);
  xbytes_t *b2 = xbytes_new_with_free_func (data, sizeof(data), mark_freed, &freed2);
  xbytes_t *b3 = xbytes_new_with_free_func (data, sizeof(data), mark_freed, &freed3);

  {
    g_autoslist(xbytes) l = NULL;

    l = xslist_prepend (l, b1);
    l = xslist_prepend (l, b3);
  }

  /* Only assert if autoptr works */
#ifdef __GNUC__
  g_assert_true (freed1);
  g_assert_true (freed3);
#endif
  g_assert_false (freed2);

  xbytes_unref (b2);
  g_assert_true (freed2);
}

static void
test_autoqueue (void)
{
  char data[1] = {0};
  xboolean_t freed1 = FALSE;
  xboolean_t freed2 = FALSE;
  xboolean_t freed3 = FALSE;
  xbytes_t *b1 = xbytes_new_with_free_func (data, sizeof(data), mark_freed, &freed1);
  xbytes_t *b2 = xbytes_new_with_free_func (data, sizeof(data), mark_freed, &freed2);
  xbytes_t *b3 = xbytes_new_with_free_func (data, sizeof(data), mark_freed, &freed3);

  {
    g_autoqueue(xbytes) q = g_queue_new ();

    g_queue_push_head (q, b1);
    g_queue_push_tail (q, b3);
  }

  /* Only assert if autoptr works */
#ifdef __GNUC__
  g_assert_true (freed1);
  g_assert_true (freed3);
#endif
  g_assert_false (freed2);

  xbytes_unref (b2);
  g_assert_true (freed2);
}

int
main (int argc, xchar_t *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/autoptr/autofree", test_autofree);
  g_test_add_func ("/autoptr/g_async_queue", test_g_async_queue);
  g_test_add_func ("/autoptr/g_bookmark_file", test_g_bookmark_file);
  g_test_add_func ("/autoptr/xbytes", test_xbytes);
  g_test_add_func ("/autoptr/xchecksum", test_xchecksum);
  g_test_add_func ("/autoptr/xdate", test_xdate);
  g_test_add_func ("/autoptr/xdate_time", test_xdate_time);
  g_test_add_func ("/autoptr/g_dir", test_g_dir);
  g_test_add_func ("/autoptr/xerror", test_xerror);
  g_test_add_func ("/autoptr/xhash_table", test_xhash_table);
  g_test_add_func ("/autoptr/g_hmac", test_g_hmac);
  g_test_add_func ("/autoptr/g_io_channel", test_xio_channel);
  g_test_add_func ("/autoptr/xkey_file", test_xkey_file);
  g_test_add_func ("/autoptr/g_list", test_g_list);
  g_test_add_func ("/autoptr/g_array", test_g_array);
  g_test_add_func ("/autoptr/xptr_array", test_xptr_array);
  g_test_add_func ("/autoptr/xbyte_array", test_xbyte_array);
  g_test_add_func ("/autoptr/xmain_context", test_xmain_context);
  g_test_add_func ("/autoptr/xmain_context_pusher", test_xmain_context_pusher);
  g_test_add_func ("/autoptr/xmain_loop", test_xmain_loop);
  g_test_add_func ("/autoptr/xsource", test_xsource);
  g_test_add_func ("/autoptr/xmapped_file", test_xmapped_file);
  g_test_add_func ("/autoptr/xmarkup_parse_context", test_xmarkup_parse_context);
  g_test_add_func ("/autoptr/g_node", test_g_node);
  g_test_add_func ("/autoptr/g_option_context", test_g_option_context);
  g_test_add_func ("/autoptr/xoption_group", test_xoption_group);
  g_test_add_func ("/autoptr/xpattern_spec", test_xpattern_spec);
  g_test_add_func ("/autoptr/g_queue", test_g_queue);
  g_test_add_func ("/autoptr/g_rand", test_g_rand);
  g_test_add_func ("/autoptr/xregex", test_xregex);
  g_test_add_func ("/autoptr/xmatch_info", test_xmatch_info);
  g_test_add_func ("/autoptr/g_scanner", test_g_scanner);
  g_test_add_func ("/autoptr/g_sequence", test_g_sequence);
  g_test_add_func ("/autoptr/g_slist", test_g_slist);
  g_test_add_func ("/autoptr/xstring", test_xstring);
  g_test_add_func ("/autoptr/xstring_chunk", test_xstring_chunk);
  g_test_add_func ("/autoptr/xthread", test_xthread);
  g_test_add_func ("/autoptr/g_mutex", test_g_mutex);
  g_test_add_func ("/autoptr/g_mutex_locker", test_g_mutex_locker);
  g_test_add_func ("/autoptr/g_rec_mutex_locker", test_g_rec_mutex_locker);
  g_test_add_func ("/autoptr/g_rw_lock_lockers", test_g_rw_lock_lockers);
  g_test_add_func ("/autoptr/g_cond", test_xcond);
  g_test_add_func ("/autoptr/g_timer", test_xtimer);
  g_test_add_func ("/autoptr/xtime_zone", test_xtimezone);
  g_test_add_func ("/autoptr/xtree", test_xtree);
  g_test_add_func ("/autoptr/g_variant", test_xvariant);
  g_test_add_func ("/autoptr/xvariant_builder", test_xvariant_builder);
  g_test_add_func ("/autoptr/xvariant_iter", test_xvariant_iter);
  g_test_add_func ("/autoptr/xvariant_dict", test_xvariant_dict);
  g_test_add_func ("/autoptr/xvariant_type", test_xvariant_type);
  g_test_add_func ("/autoptr/strv", test_strv);
  g_test_add_func ("/autoptr/refstring", test_refstring);
  g_test_add_func ("/autoptr/autolist", test_autolist);
  g_test_add_func ("/autoptr/autoslist", test_autoslist);
  g_test_add_func ("/autoptr/autoqueue", test_autoqueue);

  return g_test_run ();
}
