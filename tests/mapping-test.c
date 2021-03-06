/* XPL - Library of useful routines for C programming
 * Copyright (C) 2005 Matthias Clasen
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "glib.h"
#include "gstdio.h"

#ifdef G_OS_UNIX
#include <unistd.h>
#endif
#ifdef G_OS_WIN32
#include <process.h>
#endif

static xchar_t *dir, *global_filename, *global_displayname, *childname;

static xboolean_t stop = FALSE;

static xint_t parent_pid;

#ifndef G_OS_WIN32

static void
handle_usr1 (int signum)
{
  stop = TRUE;
}

#endif

static xboolean_t
check_stop (xpointer_t data)
{
  xmain_loop_t *loop = data;

#ifdef G_OS_WIN32
  stop = xfile_test ("STOP", XFILE_TEST_EXISTS);
#endif

  if (stop)
    xmain_loop_quit (loop);

  return TRUE;
}

static void
write_or_die (const xchar_t *filename,
	      const xchar_t *contents,
	      xssize_t       length)
{
  xerror_t *error = NULL;
  xchar_t *displayname;

  if (!xfile_set_contents (filename, contents, length, &error))
    {
      displayname = xfilename_display_name (childname);
      g_print ("failed to write '%s': %s\n",
	       displayname, error->message);
      exit (1);
    }
}

static xmapped_file_t *
map_or_die (const xchar_t *filename,
	    xboolean_t     writable)
{
  xerror_t *error = NULL;
  xmapped_file_t *map;
  xchar_t *displayname;

  map = xmapped_file_new (filename, writable, &error);
  if (!map)
    {
      displayname = xfilename_display_name (childname);
      g_print ("failed to map '%s' non-writable, shared: %s\n",
	       displayname, error->message);
      exit (1);
    }

  return map;
}

static xboolean_t
signal_parent (xpointer_t data)
{
#ifndef G_OS_WIN32
  kill (parent_pid, SIGUSR1);
#endif
  return XSOURCE_REMOVE;
}

static int
child_main (int argc, char *argv[])
{
  xmapped_file_t *map;
  xmain_loop_t *loop;

  parent_pid = atoi (argv[2]);
  map = map_or_die (global_filename, FALSE);

#ifndef G_OS_WIN32
  signal (SIGUSR1, handle_usr1);
#endif
  loop = xmain_loop_new (NULL, FALSE);
  g_idle_add (check_stop, loop);
  g_idle_add (signal_parent, NULL);
  xmain_loop_run (loop);

 g_message ("test_child_private: received parent signal");

  write_or_die (childname,
		xmapped_file_get_contents (map),
		xmapped_file_get_length (map));

  signal_parent (NULL);

  return 0;
}

static void
test_mapping (void)
{
  xmapped_file_t *map;

  write_or_die (global_filename, "ABC", -1);

  map = map_or_die (global_filename, FALSE);
  xassert (xmapped_file_get_length (map) == 3);
  xmapped_file_free (map);

  map = map_or_die (global_filename, TRUE);
  xassert (xmapped_file_get_length (map) == 3);
  xmapped_file_free (map);
  g_message ("test_mapping: ok");
}

static void
test_private (void)
{
  xerror_t *error = NULL;
  xmapped_file_t *map;
  xchar_t *buffer;
  xsize_t len;

  write_or_die (global_filename, "ABC", -1);
  map = map_or_die (global_filename, TRUE);

  buffer = (xchar_t *)xmapped_file_get_contents (map);
  buffer[0] = '1';
  buffer[1] = '2';
  buffer[2] = '3';
  xmapped_file_free (map);

  if (!xfile_get_contents (global_filename, &buffer, &len, &error))
    {
      g_print ("failed to read '%s': %s\n",
               global_displayname, error->message);
      exit (1);

    }
  xassert (len == 3);
  xassert (strcmp (buffer, "ABC") == 0);
  g_free (buffer);

  g_message ("test_private: ok");
}

static void
test_child_private (xchar_t *argv0)
{
  xerror_t *error = NULL;
  xmapped_file_t *map;
  xchar_t *buffer;
  xsize_t len;
  xchar_t *child_argv[4];
  xpid_t  child_pid;
#ifndef G_OS_WIN32
  xmain_loop_t *loop;
#endif
  xchar_t pid[100];

#ifdef G_OS_WIN32
  g_remove ("STOP");
  xassert (!xfile_test ("STOP", XFILE_TEST_EXISTS));
#endif

  write_or_die (global_filename, "ABC", -1);
  map = map_or_die (global_filename, TRUE);

#ifndef G_OS_WIN32
  signal (SIGUSR1, handle_usr1);
#endif

  g_snprintf (pid, sizeof(pid), "%d", getpid ());
  child_argv[0] = argv0;
  child_argv[1] = "mapchild";
  child_argv[2] = pid;
  child_argv[3] = NULL;
  if (!g_spawn_async (dir, child_argv, NULL,
		      0, NULL, NULL, &child_pid, &error))
    {
      g_print ("failed to spawn child: %s\n",
	       error->message);
      exit (1);
    }
 g_message ("test_child_private: child spawned");

#ifndef G_OS_WIN32
  loop = xmain_loop_new (NULL, FALSE);
  g_idle_add (check_stop, loop);
  xmain_loop_run (loop);
  stop = FALSE;
#else
  g_usleep (2000000);
#endif

 g_message ("test_child_private: received first child signal");

  buffer = (xchar_t *)xmapped_file_get_contents (map);
  buffer[0] = '1';
  buffer[1] = '2';
  buffer[2] = '3';
  xmapped_file_free (map);

#ifndef G_OS_WIN32
  kill (child_pid, SIGUSR1);
#else
  xfile_set_contents ("STOP", "Hey there\n", -1, NULL);
#endif

#ifndef G_OS_WIN32
  g_idle_add (check_stop, loop);
  xmain_loop_run (loop);
#else
  g_usleep (2000000);
#endif

 g_message ("test_child_private: received second child signal");

  if (!xfile_get_contents (childname, &buffer, &len, &error))
    {
      xchar_t *name;

      name = xfilename_display_name (childname);
      g_print ("failed to read '%s': %s\n", name, error->message);
      exit (1);
    }
  xassert (len == 3);
  xassert (strcmp (buffer, "ABC") == 0);
  g_free (buffer);

  g_message ("test_child_private: ok");
}

static int
parent_main (int   argc,
	     char *argv[])
{
  /* test mapping with various flag combinations */
  test_mapping ();

  /* test private modification */
  test_private ();

  /* test multiple clients, non-shared */
  test_child_private (argv[0]);

  return 0;
}

int
main (int argc,
      char *argv[])
{
  int ret;
#ifndef G_OS_WIN32
  sigset_t sig_mask, old_mask;

  sigemptyset (&sig_mask);
  sigaddset (&sig_mask, SIGUSR1);
  if (sigprocmask (SIG_UNBLOCK, &sig_mask, &old_mask) == 0)
    {
      if (sigismember (&old_mask, SIGUSR1))
        g_message ("SIGUSR1 was blocked, unblocking it");
    }
#endif

  dir = g_get_current_dir ();
  global_filename = g_build_filename (dir, "maptest", NULL);
  global_displayname = xfilename_display_name (global_filename);
  childname = g_build_filename (dir, "mapchild", NULL);

  if (argc > 1)
    ret = child_main (argc, argv);
  else
    ret = parent_main (argc, argv);

  g_free (childname);
  g_free (global_filename);
  g_free (global_displayname);
  g_free (dir);

  return ret;
}
