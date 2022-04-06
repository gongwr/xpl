/* GLib testing framework runner
 * Copyright (C) 2007 Sven Herzberg
 * Copyright (C) 2007 Tim Janik
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
#include "config.h"

#include <glib.h>
#include <glib-unix.h>
#include <gstdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

/* the read buffer size in bytes */
#define READ_BUFFER_SIZE 4096

/* --- prototypes --- */
static int      main_selftest   (int    argc,
                                 char **argv);
static void     parse_args      (xint_t           *argc_p,
                                 xchar_t        ***argv_p);

/* --- variables --- */
static xio_channel_t  *ioc_report = NULL;
static xboolean_t     gtester_quiet = FALSE;
static xboolean_t     gtester_verbose = FALSE;
static xboolean_t     gtester_list_tests = FALSE;
static xboolean_t     gtester_selftest = FALSE;
static xboolean_t     gtester_ignore_deprecation = FALSE;
static xboolean_t     subtest_running = FALSE;
static xint_t         subtest_exitstatus = 0;
static xboolean_t     subtest_io_pending = FALSE;
static xboolean_t     subtest_quiet = TRUE;
static xboolean_t     subtest_verbose = FALSE;
static xboolean_t     subtest_mode_fatal = TRUE;
static xboolean_t     subtest_mode_perf = FALSE;
static xboolean_t     subtest_mode_quick = TRUE;
static xboolean_t     subtest_mode_undefined = TRUE;
static const xchar_t *subtest_seedstr = NULL;
static xchar_t       *subtest_last_seed = NULL;
static xslist_t      *subtest_paths = NULL;
static xslist_t      *skipped_paths = NULL;
static xslist_t      *subtest_args = NULL;
static xboolean_t     testcase_open = FALSE;
static xuint_t        testcase_count = 0;
static xuint_t        testcase_fail_count = 0;
static const xchar_t *output_filename = NULL;
static xuint_t        log_indent = 0;
static xint_t         log_fd = -1;

/* --- functions --- */
static const char*
sindent (xuint_t n)
{
  static const char spaces[] = "                                                                                                    ";
  xsize_t l = sizeof (spaces) - 1;
  n = MIN (n, l);
  return spaces + l - n;
}

static void G_GNUC_PRINTF (1, 2)
test_log_printfe (const char *format,
                  ...)
{
  char *result;
  int r;
  va_list args;
  va_start (args, format);
  result = g_markup_vprintf_escaped (format, args);
  va_end (args);
  do
    r = write (log_fd, result, strlen (result));
  while (r < 0 && errno == EINTR);
  g_free (result);
}

static void
terminate (void)
{
  kill (getpid(), SIGTERM);
  g_abort();
}

static void
testcase_close (long double duration,
                xint_t        exit_status,
                xuint_t       n_forks)
{
  xboolean_t success;

  g_return_if_fail (testcase_open > 0);
  test_log_printfe ("%s<duration>%.6Lf</duration>\n", sindent (log_indent), duration);
  success = exit_status == G_TEST_RUN_SUCCESS || exit_status == G_TEST_RUN_SKIPPED;
  test_log_printfe ("%s<status exit-status=\"%d\" n-forks=\"%d\" result=\"%s\"/>\n",
                    sindent (log_indent), exit_status, n_forks,
                    success ? "success" : "failed");
  log_indent -= 2;
  test_log_printfe ("%s</testcase>\n", sindent (log_indent));
  testcase_open--;
  if (gtester_verbose)
    {
      switch (exit_status)
        {
        case G_TEST_RUN_SUCCESS:
          g_print ("OK\n");
          break;
        case G_TEST_RUN_SKIPPED:
          g_print ("SKIP\n");
          break;
        default:
          g_print ("FAIL\n");
          break;
        }
    }
  if (!success && subtest_last_seed)
    g_print ("GTester: last random seed: %s\n", subtest_last_seed);
  if (!success)
    testcase_fail_count += 1;
  if (subtest_mode_fatal && !success)
    terminate();
}

static void
test_log_msg (GTestLogMsg *msg)
{
  switch (msg->log_type)
    {
      xuint_t i;
      xchar_t **strv;
    case G_TEST_LOG_NONE:
    case G_TEST_LOG_START_SUITE:
    case G_TEST_LOG_STOP_SUITE:
      break;
    case G_TEST_LOG_ERROR:
      strv = xstrsplit (msg->strings[0], "\n", -1);
      for (i = 0; strv[i]; i++)
        test_log_printfe ("%s<error>%s</error>\n", sindent (log_indent), strv[i]);
      xstrfreev (strv);
      break;
    case G_TEST_LOG_START_BINARY:
      test_log_printfe ("%s<binary file=\"%s\"/>\n", sindent (log_indent), msg->strings[0]);
      subtest_last_seed = xstrdup (msg->strings[1]);
      test_log_printfe ("%s<random-seed>%s</random-seed>\n", sindent (log_indent), subtest_last_seed);
      break;
    case G_TEST_LOXLIST_CASE:
      g_print ("%s\n", msg->strings[0]);
      break;
    case G_TEST_LOG_START_CASE:
      testcase_count++;
      if (gtester_verbose)
        {
          xchar_t *sc = xstrconcat (msg->strings[0], ":", NULL);
          xchar_t *sleft = xstrdup_printf ("%-68s", sc);
          g_free (sc);
          g_print ("%70s ", sleft);
          g_free (sleft);
        }
      g_return_if_fail (testcase_open == 0);
      testcase_open++;
      test_log_printfe ("%s<testcase path=\"%s\">\n", sindent (log_indent), msg->strings[0]);
      log_indent += 2;
      break;
    case G_TEST_LOG_SKIP_CASE:
      if (FALSE && gtester_verbose) /* enable to debug test case skipping logic */
        {
          xchar_t *sc = xstrconcat (msg->strings[0], ":", NULL);
          xchar_t *sleft = xstrdup_printf ("%-68s", sc);
          g_free (sc);
          g_print ("%70s SKIPPED\n", sleft);
          g_free (sleft);
        }
      test_log_printfe ("%s<testcase path=\"%s\" skipped=\"1\"/>\n", sindent (log_indent), msg->strings[0]);
      break;
    case G_TEST_LOG_STOP_CASE:
      testcase_close (msg->nums[2], (int) msg->nums[0], (int) msg->nums[1]);
      break;
    case G_TEST_LOG_MIN_RESULT:
    case G_TEST_LOG_MAX_RESULT:
      test_log_printfe ("%s<performance minimize=\"%d\" maximize=\"%d\" value=\"%.16Lg\">\n",
                        sindent (log_indent), msg->log_type == G_TEST_LOG_MIN_RESULT, msg->log_type == G_TEST_LOG_MAX_RESULT, msg->nums[0]);
      test_log_printfe ("%s%s\n", sindent (log_indent + 2), msg->strings[0]);
      test_log_printfe ("%s</performance>\n", sindent (log_indent));
      break;
    case G_TEST_LOG_MESSAGE:
      test_log_printfe ("%s<message>\n%s\n%s</message>\n", sindent (log_indent), msg->strings[0], sindent (log_indent));
      break;
    }
}

static xboolean_t
child_report_cb (xio_channel_t  *source,
                 xio_condition_t condition,
                 xpointer_t     data)
{
  GTestLogBuffer *tlb = data;
  GIOStatus status = G_IO_STATUS_NORMAL;
  xboolean_t first_read_eof = FALSE, first_read = TRUE;
  xsize_t length = 0;
  do
    {
      xuint8_t buffer[READ_BUFFER_SIZE];
      xerror_t *error = NULL;
      status = g_io_channel_read_chars (source, (xchar_t*) buffer, sizeof (buffer), &length, &error);
      if (first_read && (condition & G_IO_IN))
        {
          /* on some unixes (MacOS) we need to detect non-blocking fd EOF
           * by an IO_IN select/poll followed by read()==0.
           */
          first_read_eof = length == 0;
        }
      first_read = FALSE;
      if (length)
        {
          GTestLogMsg *msg;
          g_test_log_buffer_push (tlb, length, buffer);
          do
            {
              msg = g_test_log_buffer_pop (tlb);
              if (msg)
                {
                  test_log_msg (msg);
                  g_test_log_msg_free (msg);
                }
            }
          while (msg);
        }
      g_clear_error (&error);
      /* ignore the io channel status, which will report intermediate EOFs for non blocking fds */
      (void) status;
    }
  while (length > 0);
  /* g_print ("LASTIOSTATE: first_read_eof=%d condition=%d\n", first_read_eof, condition); */
  if (first_read_eof || (condition & (G_IO_ERR | G_IO_HUP)))
    {
      /* if there's no data to read and select() reports an error or hangup,
       * the fd must have been closed remotely
       */
      subtest_io_pending = FALSE;
      return FALSE;
    }
  return TRUE; /* keep polling */
}

static void
child_watch_cb (xpid_t     pid,
		xint_t     status,
		xpointer_t data)
{
  g_spawn_close_pid (pid);
  if (WIFEXITED (status)) /* normal exit */
    subtest_exitstatus = WEXITSTATUS (status);
  else /* signal or core dump, etc */
    subtest_exitstatus = 0xffffffff;
  subtest_running = FALSE;
}

static xchar_t*
queue_gfree (xslist_t **slistp,
             xchar_t   *string)
{
  *slistp = xslist_prepend (*slistp, string);
  return string;
}

static void
unset_cloexec_fdp (xpointer_t fdp_data)
{
  int r, *fdp = fdp_data;
  do
    r = fcntl (*fdp, F_SETFD, 0 /* FD_CLOEXEC */);
  while (r < 0 && errno == EINTR);
}

static xboolean_t
launch_test_binary (const char *binary,
                    xuint_t       skip_tests)
{
  GTestLogBuffer *tlb;
  xslist_t *slist, *free_list = NULL;
  xerror_t *error = NULL;
  int argc = 0;
  const xchar_t **argv;
  xpid_t pid = 0;
  xint_t report_pipe[2] = { -1, -1 };
  xuint_t child_report_cb_id = 0;
  xboolean_t loop_pending;
  xint_t i = 0;

  if (!g_unix_open_pipe (report_pipe, FD_CLOEXEC, &error))
    {
      if (subtest_mode_fatal)
        xerror ("Failed to open pipe for test binary: %s: %s", binary, error->message);
      else
        g_warning ("Failed to open pipe for test binary: %s: %s", binary, error->message);
      g_clear_error (&error);
      return FALSE;
    }

  /* setup argc */
  for (slist = subtest_args; slist; slist = slist->next)
    argc++;
  /* argc++; */
  if (subtest_quiet)
    argc++;
  if (subtest_verbose)
    argc++;
  if (!subtest_mode_fatal)
    argc++;
  /* Either -m=quick or -m=slow is always appended. */
  argc++;
  if (subtest_mode_perf)
    argc++;
  if (!subtest_mode_undefined)
    argc++;
  if (gtester_list_tests)
    argc++;
  if (subtest_seedstr)
    argc++;
  argc++;
  if (skip_tests)
    argc++;
  for (slist = subtest_paths; slist; slist = slist->next)
    argc++;
  for (slist = skipped_paths; slist; slist = slist->next)
    argc++;

  /* setup argv */
  argv = g_malloc ((argc + 2) * sizeof(xchar_t *));
  argv[i++] = binary;
  for (slist = subtest_args; slist; slist = slist->next)
    argv[i++] = (xchar_t*) slist->data;
  /* argv[i++] = "--debug-log"; */
  if (subtest_quiet)
    argv[i++] = "--quiet";
  if (subtest_verbose)
    argv[i++] = "--verbose";
  if (!subtest_mode_fatal)
    argv[i++] = "--keep-going";
  if (subtest_mode_quick)
    argv[i++] = "-m=quick";
  else
    argv[i++] = "-m=slow";
  if (subtest_mode_perf)
    argv[i++] = "-m=perf";
  if (!subtest_mode_undefined)
    argv[i++] = "-m=no-undefined";
  if (gtester_list_tests)
    argv[i++] = "-l";
  if (subtest_seedstr)
    argv[i++] = queue_gfree (&free_list, xstrdup_printf ("--seed=%s", subtest_seedstr));
  argv[i++] = queue_gfree (&free_list, xstrdup_printf ("--GTestLogFD=%u", report_pipe[1]));
  if (skip_tests)
    argv[i++] = queue_gfree (&free_list, xstrdup_printf ("--GTestSkipCount=%u", skip_tests));
  for (slist = subtest_paths; slist; slist = slist->next)
    argv[i++] = queue_gfree (&free_list, xstrdup_printf ("-p=%s", (xchar_t*) slist->data));
  for (slist = skipped_paths; slist; slist = slist->next)
    argv[i++] = queue_gfree (&free_list, xstrdup_printf ("-s=%s", (xchar_t*) slist->data));
  argv[i++] = NULL;

  g_spawn_async_with_pipes (NULL, /* g_get_current_dir() */
                            (xchar_t**) argv,
                            NULL, /* envp */
                            G_SPAWN_DO_NOT_REAP_CHILD, /* G_SPAWN_SEARCH_PATH */
                            unset_cloexec_fdp, &report_pipe[1], /* pre-exec callback */
                            &pid,
                            NULL,       /* standard_input */
                            NULL,       /* standard_output */
                            NULL,       /* standard_error */
                            &error);
  xslist_foreach (free_list, (void(*)(void*,void*)) g_free, NULL);
  xslist_free (free_list);
  free_list = NULL;
  close (report_pipe[1]);

  if (!gtester_quiet)
    g_print ("(pid=%lu)\n", (unsigned long) pid);

  if (error)
    {
      close (report_pipe[0]);
      if (subtest_mode_fatal)
        xerror ("Failed to execute test binary: %s: %s", argv[0], error->message);
      else
        g_warning ("Failed to execute test binary: %s: %s", argv[0], error->message);
      g_clear_error (&error);
      g_free (argv);
      return FALSE;
    }
  g_free (argv);

  subtest_running = TRUE;
  subtest_io_pending = TRUE;
  tlb = g_test_log_buffer_new();
  if (report_pipe[0] >= 0)
    {
      ioc_report = g_io_channel_unix_new (report_pipe[0]);
      g_io_channel_set_flags (ioc_report, G_IO_FLAG_NONBLOCK, NULL);
      g_io_channel_set_encoding (ioc_report, NULL, NULL);
      g_io_channel_set_buffered (ioc_report, FALSE);
      child_report_cb_id = g_io_add_watch_full (ioc_report, G_PRIORITY_DEFAULT - 1, G_IO_IN | G_IO_ERR | G_IO_HUP, child_report_cb, tlb, NULL);
      g_io_channel_unref (ioc_report);
    }
  g_child_watch_add_full (G_PRIORITY_DEFAULT + 1, pid, child_watch_cb, NULL, NULL);

  loop_pending = xmain_context_pending (NULL);
  while (subtest_running ||     /* FALSE once child exits */
         subtest_io_pending ||  /* FALSE once ioc_report closes */
         loop_pending)          /* TRUE while idler, etc are running */
    {
      /* g_print ("LOOPSTATE: subtest_running=%d subtest_io_pending=%d\n", subtest_running, subtest_io_pending); */
      /* check for unexpected hangs that are not signalled on report_pipe */
      if (!subtest_running &&   /* child exited */
          subtest_io_pending && /* no EOF detected on report_pipe */
          !loop_pending)        /* no IO events pending however */
        break;
      xmain_context_iteration (NULL, TRUE);
      loop_pending = xmain_context_pending (NULL);
    }

  if (subtest_io_pending)
    xsource_remove (child_report_cb_id);

  close (report_pipe[0]);
  g_test_log_buffer_free (tlb);

  return TRUE;
}

static void
launch_test (const char *binary)
{
  xboolean_t success = TRUE;
  xtimer_t *btimer = g_timer_new();
  xboolean_t need_restart;

  testcase_count = 0;
  if (!gtester_quiet)
    g_print ("TEST: %s... ", binary);

 retry:
  test_log_printfe ("%s<testbinary path=\"%s\">\n", sindent (log_indent), binary);
  log_indent += 2;
  g_timer_start (btimer);
  subtest_exitstatus = 0;
  success &= launch_test_binary (binary, testcase_count);
  success &= subtest_exitstatus == 0;
  need_restart = testcase_open != 0;
  if (testcase_open)
    testcase_close (0, -256, 0);
  g_timer_stop (btimer);
  test_log_printfe ("%s<duration>%.6f</duration>\n", sindent (log_indent), g_timer_elapsed (btimer, NULL));
  log_indent -= 2;
  test_log_printfe ("%s</testbinary>\n", sindent (log_indent));
  g_free (subtest_last_seed);
  subtest_last_seed = NULL;
  if (need_restart)
    {
      /* restart test binary, skipping processed test cases */
      goto retry;
    }

  /* count the inability to run a test as a failure */
  if (!success && testcase_count == 0)
    testcase_fail_count++;

  if (!gtester_quiet)
    g_print ("%s: %s\n", !success ? "FAIL" : "PASS", binary);
  g_timer_destroy (btimer);
  if (subtest_mode_fatal && !success)
    terminate();
}

static void
usage (xboolean_t just_version)
{
  if (just_version)
    {
      g_print ("gtester version %d.%d.%d\n", XPL_MAJOR_VERSION, XPL_MINOR_VERSION, XPL_MICRO_VERSION);
      return;
    }
  g_print ("Usage:\n");
  g_print ("gtester [OPTIONS] testprogram...\n\n");
  /*        12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
  g_print ("Help Options:\n");
  g_print ("  -h, --help                    Show this help message\n\n");
  g_print ("Utility Options:\n");
  g_print ("  -v, --version                 Print version information\n");
  g_print ("  --g-fatal-warnings            Make warnings fatal (abort)\n");
  g_print ("  -k, --keep-going              Continue running after tests failed\n");
  g_print ("  -l                            List paths of available test cases\n");
  g_print ("  -m {perf|slow|thorough|quick} Run test cases according to mode\n");
  g_print ("  -m {undefined|no-undefined}   Run test cases according to mode\n");
  g_print ("  -p=TESTPATH                   Only start test cases matching TESTPATH\n");
  g_print ("  -s=TESTPATH                   Skip test cases matching TESTPATH\n");
  g_print ("  --seed=SEEDSTRING             Start tests with random seed SEEDSTRING\n");
  g_print ("  -o=LOGFILE                    Write the test log to LOGFILE\n");
  g_print ("  -q, --quiet                   Suppress per test binary output\n");
  g_print ("  --verbose                     Report success per testcase\n");
}

static void
parse_args (xint_t    *argc_p,
            xchar_t ***argv_p)
{
  xuint_t argc = *argc_p;
  xchar_t **argv = *argv_p;
  xuint_t i, e;
  /* parse known args */
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--g-fatal-warnings") == 0)
        {
          GLogLevelFlags fatal_mask = (GLogLevelFlags) g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK);
          fatal_mask = (GLogLevelFlags) (fatal_mask | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
          g_log_set_always_fatal (fatal_mask);
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "--gtester-selftest") == 0)
        {
          gtester_selftest = TRUE;
          argv[i] = NULL;
          break;        /* stop parsing regular gtester arguments */
        }
      else if (strcmp (argv[i], "-h") == 0 || strcmp (argv[i], "--help") == 0)
        {
          usage (FALSE);
          exit (0);
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "-v") == 0 || strcmp (argv[i], "--version") == 0)
        {
          usage (TRUE);
          exit (0);
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "--keep-going") == 0 ||
               strcmp (argv[i], "-k") == 0)
        {
          subtest_mode_fatal = FALSE;
          argv[i] = NULL;
        }
      else if (strcmp ("-p", argv[i]) == 0 || strncmp ("-p=", argv[i], 3) == 0)
        {
          xchar_t *equal = argv[i] + 2;
          if (*equal == '=')
            subtest_paths = xslist_prepend (subtest_paths, equal + 1);
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              subtest_paths = xslist_prepend (subtest_paths, argv[i]);
            }
          argv[i] = NULL;
        }
      else if (strcmp ("-s", argv[i]) == 0 || strncmp ("-s=", argv[i], 3) == 0)
        {
          xchar_t *equal = argv[i] + 2;
          if (*equal == '=')
            skipped_paths = xslist_prepend (skipped_paths, equal + 1);
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              skipped_paths = xslist_prepend (skipped_paths, argv[i]);
            }
          argv[i] = NULL;
        }
      else if (strcmp ("--test-arg", argv[i]) == 0 || strncmp ("--test-arg=", argv[i], 11) == 0)
        {
          xchar_t *equal = argv[i] + 10;
          if (*equal == '=')
            subtest_args = xslist_prepend (subtest_args, equal + 1);
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              subtest_args = xslist_prepend (subtest_args, argv[i]);
            }
          argv[i] = NULL;
        }
      else if (strcmp ("-o", argv[i]) == 0 || strncmp ("-o=", argv[i], 3) == 0)
        {
          xchar_t *equal = argv[i] + 2;
          if (*equal == '=')
            output_filename = equal + 1;
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              output_filename = argv[i];
            }
          argv[i] = NULL;
        }
      else if (strcmp ("-m", argv[i]) == 0 || strncmp ("-m=", argv[i], 3) == 0)
        {
          xchar_t *equal = argv[i] + 2;
          const xchar_t *mode = "";
          if (*equal == '=')
            mode = equal + 1;
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              mode = argv[i];
            }
          if (strcmp (mode, "perf") == 0)
            subtest_mode_perf = TRUE;
          else if (strcmp (mode, "slow") == 0 || strcmp (mode, "thorough") == 0)
            subtest_mode_quick = FALSE;
          else if (strcmp (mode, "quick") == 0)
            {
              subtest_mode_quick = TRUE;
              subtest_mode_perf = FALSE;
            }
          else if (strcmp (mode, "undefined") == 0)
            subtest_mode_undefined = TRUE;
          else if (strcmp (mode, "no-undefined") == 0)
            subtest_mode_undefined = FALSE;
          else
            xerror ("unknown test mode: -m %s", mode);
          argv[i] = NULL;
        }
      else if (strcmp ("-q", argv[i]) == 0 || strcmp ("--quiet", argv[i]) == 0)
        {
          gtester_quiet = TRUE;
          gtester_verbose = FALSE;
          argv[i] = NULL;
        }
      else if (strcmp ("--verbose", argv[i]) == 0)
        {
          gtester_quiet = FALSE;
          gtester_verbose = TRUE;
          argv[i] = NULL;
        }
      else if (strcmp ("-l", argv[i]) == 0)
        {
          gtester_list_tests = TRUE;
          argv[i] = NULL;
        }
      else if (strcmp ("--seed", argv[i]) == 0 || strncmp ("--seed=", argv[i], 7) == 0)
        {
          xchar_t *equal = argv[i] + 6;
          if (*equal == '=')
            subtest_seedstr = equal + 1;
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              subtest_seedstr = argv[i];
            }
          argv[i] = NULL;
        }
      else if (strcmp ("--i-know-this-is-deprecated", argv[i]) == 0)
        {
          gtester_ignore_deprecation = TRUE;
          argv[i] = NULL;
        }
    }
  /* collapse argv */
  e = 0;
  for (i = 0; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
}

int
main (int    argc,
      char **argv)
{
  xint_t ui;

  g_set_prgname (argv[0]);
  parse_args (&argc, &argv);
  if (gtester_selftest)
    return main_selftest (argc, argv);

  if (argc <= 1)
    {
      usage (FALSE);
      return 1;
    }

  if (!gtester_ignore_deprecation)
    g_warning ("Deprecated: Since GLib 2.62, gtester and gtester-report are "
               "deprecated. Port to TAP.");

  if (output_filename)
    {
      int errsv;
      log_fd = g_open (output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      errsv = errno;
      if (log_fd < 0)
        xerror ("Failed to open log file '%s': %s", output_filename, xstrerror (errsv));
    }

  test_log_printfe ("<?xml version=\"1.0\"?>\n");
  test_log_printfe ("<!-- Deprecated: Since GLib 2.62, gtester and "
                    "gtester-report are deprecated. Port to TAP. -->\n");
  test_log_printfe ("%s<gtester>\n", sindent (log_indent));
  log_indent += 2;
  for (ui = 1; ui < argc; ui++)
    {
      const char *binary = argv[ui];
      launch_test (binary);
      /* we only get here on success or if !subtest_mode_fatal */
    }
  log_indent -= 2;
  test_log_printfe ("%s</gtester>\n", sindent (log_indent));

  close (log_fd);

  return testcase_fail_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
fixture_setup (xuint_t        *fix,
               xconstpointer test_data)
{
  g_assert_cmphex (*fix, ==, 0);
  *fix = 0xdeadbeef;
}
static void
fixture_test (xuint_t        *fix,
              xconstpointer test_data)
{
  g_assert_cmphex (*fix, ==, 0xdeadbeef);
  g_test_message ("This is a test message API test message.");
  g_test_bug_base ("http://www.example.com/bugtracker/");
  g_test_bug ("123");
  g_test_bug_base ("http://www.example.com/bugtracker?bugnum=%s;cmd=showbug");
  g_test_bug ("456");
  g_test_bug ("https://example.com/no-base-used");
}
static void
fixture_teardown (xuint_t        *fix,
                  xconstpointer test_data)
{
  g_assert_cmphex (*fix, ==, 0xdeadbeef);
}

static int
main_selftest (int    argc,
               char **argv)
{
  /* gtester main() for --gtester-selftest invocations */
  g_test_init (&argc, &argv, NULL);
  g_test_add ("/gtester/fixture-test", xuint_t, NULL, fixture_setup, fixture_test, fixture_teardown);
  return g_test_run();
}
