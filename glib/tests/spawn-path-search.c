/*
 * Copyright 2021 Collabora Ltd.
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
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <glib.h>

#ifdef G_OS_UNIX
#include <sys/types.h>
#include <sys/wait.h>
#endif

static xboolean_t
skip_win32 (void)
{
#ifdef G_OS_WIN32
  g_test_skip ("The test manipulate PATH, and breaks DLL lookups.");
  return TRUE;
#else
  return FALSE;
#endif
}

static void
test_do_not_search (void)
{
  xptr_array_t *argv = xptr_array_new_with_free_func (g_free);
  xchar_t *here = g_test_build_filename (G_TEST_BUILT, ".", NULL);
  xchar_t *subdir = g_test_build_filename (G_TEST_BUILT, "path-test-subdir", NULL);
  xchar_t **envp = g_get_environ ();
  xchar_t *out = NULL;
  xchar_t *err = NULL;
  xerror_t *error = NULL;
  int wait_status = -1;

  g_test_summary ("Without G_SPAWN_SEARCH_PATH, spawn-test-helper "
                  "means ./spawn-test-helper.");

  if (skip_win32 ())
    return;

  envp = g_environ_setenv (envp, "PATH", subdir, TRUE);

  xptr_array_add (argv,
                   g_test_build_filename (G_TEST_BUILT, "spawn-path-search-helper", NULL));
  xptr_array_add (argv, xstrdup ("--"));
  xptr_array_add (argv, xstrdup ("spawn-test-helper"));
  xptr_array_add (argv, NULL);

  g_spawn_sync (here,
                (char **) argv->pdata,
                envp,
                G_SPAWN_DEFAULT,
                NULL,  /* child setup */
                NULL,  /* user data */
                &out,
                &err,
                &wait_status,
                &error);
  g_assert_no_error (error);

  g_test_message ("%s", out);
  g_test_message ("%s", err);
  g_assert_nonnull (strstr (err, "this is spawn-test-helper from glib/tests"));

#ifdef G_OS_UNIX
  g_assert_true (WIFEXITED (wait_status));
  g_assert_cmpint (WEXITSTATUS (wait_status), ==, 0);
#endif

  xstrfreev (envp);
  g_free (here);
  g_free (subdir);
  g_free (out);
  g_free (err);
  xptr_array_unref (argv);
}

static void
test_search_path (void)
{
  xptr_array_t *argv = xptr_array_new_with_free_func (g_free);
  xchar_t *here = g_test_build_filename (G_TEST_BUILT, ".", NULL);
  xchar_t *subdir = g_test_build_filename (G_TEST_BUILT, "path-test-subdir", NULL);
  xchar_t **envp = g_get_environ ();
  xchar_t *out = NULL;
  xchar_t *err = NULL;
  xerror_t *error = NULL;
  int wait_status = -1;

  g_test_summary ("With G_SPAWN_SEARCH_PATH, spawn-test-helper "
                  "means $PATH/spawn-test-helper.");

  if (skip_win32 ())
    return;

  envp = g_environ_setenv (envp, "PATH", subdir, TRUE);

  xptr_array_add (argv,
                   g_test_build_filename (G_TEST_BUILT, "spawn-path-search-helper", NULL));
  xptr_array_add (argv, xstrdup ("--search-path"));
  xptr_array_add (argv, xstrdup ("--"));
  xptr_array_add (argv, xstrdup ("spawn-test-helper"));
  xptr_array_add (argv, NULL);

  g_spawn_sync (here,
                (char **) argv->pdata,
                envp,
                G_SPAWN_DEFAULT,
                NULL,  /* child setup */
                NULL,  /* user data */
                &out,
                &err,
                &wait_status,
                &error);
  g_assert_no_error (error);

  g_test_message ("%s", out);
  g_test_message ("%s", err);
  g_assert_nonnull (strstr (err, "this is spawn-test-helper from path-test-subdir"));

#ifdef G_OS_UNIX
  g_assert_true (WIFEXITED (wait_status));
  g_assert_cmpint (WEXITSTATUS (wait_status), ==, 5);
#endif

  xstrfreev (envp);
  g_free (here);
  g_free (subdir);
  g_free (out);
  g_free (err);
  xptr_array_unref (argv);
}

static void
test_search_path_from_envp (void)
{
  xptr_array_t *argv = xptr_array_new_with_free_func (g_free);
  xchar_t *here = g_test_build_filename (G_TEST_BUILT, ".", NULL);
  xchar_t *subdir = g_test_build_filename (G_TEST_BUILT, "path-test-subdir", NULL);
  xchar_t **envp = g_get_environ ();
  xchar_t *out = NULL;
  xchar_t *err = NULL;
  xerror_t *error = NULL;
  int wait_status = -1;

  g_test_summary ("With G_SPAWN_SEARCH_PATH_FROM_ENVP, spawn-test-helper "
                  "means $PATH/spawn-test-helper with $PATH from envp.");

  if (skip_win32 ())
    return;

  envp = g_environ_setenv (envp, "PATH", here, TRUE);

  xptr_array_add (argv,
                   g_test_build_filename (G_TEST_BUILT, "spawn-path-search-helper", NULL));
  xptr_array_add (argv, xstrdup ("--search-path-from-envp"));
  xptr_array_add (argv, xstrdup ("--set-path-in-envp"));
  xptr_array_add (argv, xstrdup (subdir));
  xptr_array_add (argv, xstrdup ("--"));
  xptr_array_add (argv, xstrdup ("spawn-test-helper"));
  xptr_array_add (argv, NULL);

  g_spawn_sync (here,
                (char **) argv->pdata,
                envp,
                G_SPAWN_DEFAULT,
                NULL,  /* child setup */
                NULL,  /* user data */
                &out,
                &err,
                &wait_status,
                &error);
  g_assert_no_error (error);

  g_test_message ("%s", out);
  g_test_message ("%s", err);
  g_assert_nonnull (strstr (err, "this is spawn-test-helper from path-test-subdir"));

#ifdef G_OS_UNIX
  g_assert_true (WIFEXITED (wait_status));
  g_assert_cmpint (WEXITSTATUS (wait_status), ==, 5);
#endif

  xstrfreev (envp);
  g_free (here);
  g_free (subdir);
  g_free (out);
  g_free (err);
  xptr_array_unref (argv);
}

static void
test_search_path_ambiguous (void)
{
  xptr_array_t *argv = xptr_array_new_with_free_func (g_free);
  xchar_t *here = g_test_build_filename (G_TEST_BUILT, ".", NULL);
  xchar_t *subdir = g_test_build_filename (G_TEST_BUILT, "path-test-subdir", NULL);
  xchar_t **envp = g_get_environ ();
  xchar_t *out = NULL;
  xchar_t *err = NULL;
  xerror_t *error = NULL;
  int wait_status = -1;

  g_test_summary ("With G_SPAWN_SEARCH_PATH and G_SPAWN_SEARCH_PATH_FROM_ENVP, "
                  "the latter wins.");

  if (skip_win32 ())
    return;

  envp = g_environ_setenv (envp, "PATH", here, TRUE);

  xptr_array_add (argv,
                   g_test_build_filename (G_TEST_BUILT, "spawn-path-search-helper", NULL));
  xptr_array_add (argv, xstrdup ("--search-path"));
  xptr_array_add (argv, xstrdup ("--search-path-from-envp"));
  xptr_array_add (argv, xstrdup ("--set-path-in-envp"));
  xptr_array_add (argv, xstrdup (subdir));
  xptr_array_add (argv, xstrdup ("--"));
  xptr_array_add (argv, xstrdup ("spawn-test-helper"));
  xptr_array_add (argv, NULL);

  g_spawn_sync (here,
                (char **) argv->pdata,
                envp,
                G_SPAWN_DEFAULT,
                NULL,  /* child setup */
                NULL,  /* user data */
                &out,
                &err,
                &wait_status,
                &error);
  g_assert_no_error (error);

  g_test_message ("%s", out);
  g_test_message ("%s", err);
  g_assert_nonnull (strstr (err, "this is spawn-test-helper from path-test-subdir"));

#ifdef G_OS_UNIX
  g_assert_true (WIFEXITED (wait_status));
  g_assert_cmpint (WEXITSTATUS (wait_status), ==, 5);
#endif

  xstrfreev (envp);
  g_free (here);
  g_free (subdir);
  g_free (out);
  g_free (err);
  xptr_array_unref (argv);
}

static void
test_search_path_fallback_in_environ (void)
{
  xptr_array_t *argv = xptr_array_new_with_free_func (g_free);
  xchar_t *here = g_test_build_filename (G_TEST_BUILT, ".", NULL);
  xchar_t *subdir = g_test_build_filename (G_TEST_BUILT, "path-test-subdir", NULL);
  xchar_t **envp = g_get_environ ();
  xchar_t *out = NULL;
  xchar_t *err = NULL;
  xerror_t *error = NULL;
  int wait_status = -1;

  g_test_summary ("With G_SPAWN_SEARCH_PATH but no PATH, a fallback is used.");

  if (skip_win32 ())
    return;

  /* We can't make a meaningful assertion about what the fallback *is*,
   * but we can assert that it *includes* the current working directory. */

  if (xfile_test ("/usr/bin/spawn-test-helper", XFILE_TEST_IS_EXECUTABLE) ||
      xfile_test ("/bin/spawn-test-helper", XFILE_TEST_IS_EXECUTABLE))
    {
      g_test_skip ("Not testing fallback with unknown spawn-test-helper "
                   "executable in /usr/bin:/bin");
      return;
    }

  envp = g_environ_unsetenv (envp, "PATH");

  xptr_array_add (argv,
                   g_test_build_filename (G_TEST_BUILT, "spawn-path-search-helper", NULL));
  xptr_array_add (argv, xstrdup ("--search-path"));
  xptr_array_add (argv, xstrdup ("--set-path-in-envp"));
  xptr_array_add (argv, xstrdup (subdir));
  xptr_array_add (argv, xstrdup ("--"));
  xptr_array_add (argv, xstrdup ("spawn-test-helper"));
  xptr_array_add (argv, NULL);

  g_spawn_sync (here,
                (char **) argv->pdata,
                envp,
                G_SPAWN_DEFAULT,
                NULL,  /* child setup */
                NULL,  /* user data */
                &out,
                &err,
                &wait_status,
                &error);
  g_assert_no_error (error);

  g_test_message ("%s", out);
  g_test_message ("%s", err);
  g_assert_nonnull (strstr (err, "this is spawn-test-helper from glib/tests"));

#ifdef G_OS_UNIX
  g_assert_true (WIFEXITED (wait_status));
  g_assert_cmpint (WEXITSTATUS (wait_status), ==, 0);
#endif

  xstrfreev (envp);
  g_free (here);
  g_free (subdir);
  g_free (out);
  g_free (err);
  xptr_array_unref (argv);
}

static void
test_search_path_fallback_in_envp (void)
{
  xptr_array_t *argv = xptr_array_new_with_free_func (g_free);
  xchar_t *here = g_test_build_filename (G_TEST_BUILT, ".", NULL);
  xchar_t *subdir = g_test_build_filename (G_TEST_BUILT, "path-test-subdir", NULL);
  xchar_t **envp = g_get_environ ();
  xchar_t *out = NULL;
  xchar_t *err = NULL;
  xerror_t *error = NULL;
  int wait_status = -1;

  g_test_summary ("With G_SPAWN_SEARCH_PATH_FROM_ENVP but no PATH, a fallback is used.");
  /* We can't make a meaningful assertion about what the fallback *is*,
   * but we can assert that it *includes* the current working directory. */

  if (skip_win32 ())
    return;

  if (xfile_test ("/usr/bin/spawn-test-helper", XFILE_TEST_IS_EXECUTABLE) ||
      xfile_test ("/bin/spawn-test-helper", XFILE_TEST_IS_EXECUTABLE))
    {
      g_test_skip ("Not testing fallback with unknown spawn-test-helper "
                   "executable in /usr/bin:/bin");
      return;
    }

  envp = g_environ_setenv (envp, "PATH", subdir, TRUE);

  xptr_array_add (argv,
                   g_test_build_filename (G_TEST_BUILT, "spawn-path-search-helper", NULL));
  xptr_array_add (argv, xstrdup ("--search-path-from-envp"));
  xptr_array_add (argv, xstrdup ("--unset-path-in-envp"));
  xptr_array_add (argv, xstrdup ("--"));
  xptr_array_add (argv, xstrdup ("spawn-test-helper"));
  xptr_array_add (argv, NULL);

  g_spawn_sync (here,
                (char **) argv->pdata,
                envp,
                G_SPAWN_DEFAULT,
                NULL,  /* child setup */
                NULL,  /* user data */
                &out,
                &err,
                &wait_status,
                &error);
  g_assert_no_error (error);

  g_test_message ("%s", out);
  g_test_message ("%s", err);
  g_assert_nonnull (strstr (err, "this is spawn-test-helper from glib/tests"));

#ifdef G_OS_UNIX
  g_assert_true (WIFEXITED (wait_status));
  g_assert_cmpint (WEXITSTATUS (wait_status), ==, 0);
#endif

  xstrfreev (envp);
  g_free (here);
  g_free (subdir);
  g_free (out);
  g_free (err);
  xptr_array_unref (argv);
}

static void
test_search_path_heap_allocation (void)
{
  xptr_array_t *argv = xptr_array_new_with_free_func (g_free);
  /* Must be longer than the arbitrary 4000 byte limit for stack allocation
   * in gspawn.c */
  char placeholder[4096];
  xchar_t *here = g_test_build_filename (G_TEST_BUILT, ".", NULL);
  xchar_t *subdir = g_test_build_filename (G_TEST_BUILT, "path-test-subdir", NULL);
  xchar_t *long_dir = NULL;
  xchar_t *long_path = NULL;
  xchar_t **envp = g_get_environ ();
  xchar_t *out = NULL;
  xchar_t *err = NULL;
  xerror_t *error = NULL;
  int wait_status = -1;
  xsize_t i;

  if (skip_win32 ())
    return;

  memset (placeholder, '_', sizeof (placeholder));
  /* Force search_path_buffer to be heap-allocated */
  long_dir = g_test_build_filename (G_TEST_BUILT, "path-test-subdir", placeholder, NULL);
  long_path = xstrjoin (G_SEARCHPATH_SEPARATOR_S, subdir, long_dir, NULL);
  envp = g_environ_setenv (envp, "PATH", long_path, TRUE);
  g_free (long_path);
  g_free (long_dir);

  xptr_array_add (argv,
                   g_test_build_filename (G_TEST_BUILT, "spawn-path-search-helper", NULL));
  xptr_array_add (argv, xstrdup ("--search-path"));
  xptr_array_add (argv, xstrdup ("--"));
  xptr_array_add (argv, xstrdup ("spawn-test-helper"));

  /* Add enough arguments to make argv longer than the arbitrary 4000 byte
   * limit for stack allocation in gspawn.c.
   * This assumes sizeof (char *) >= 4. */
  for (i = 0; i < 1001; i++)
    xptr_array_add (argv, xstrdup ("_"));

  xptr_array_add (argv, NULL);

  g_spawn_sync (here,
                (char **) argv->pdata,
                envp,
                G_SPAWN_DEFAULT,
                NULL,  /* child setup */
                NULL,  /* user data */
                &out,
                &err,
                &wait_status,
                &error);
  g_assert_no_error (error);

  g_test_message ("%s", out);
  g_test_message ("%s", err);
  g_assert_nonnull (strstr (err, "this is spawn-test-helper from path-test-subdir"));

#ifdef G_OS_UNIX
  g_assert_true (WIFEXITED (wait_status));
  g_assert_cmpint (WEXITSTATUS (wait_status), ==, 5);
#endif

  xstrfreev (envp);
  g_free (here);
  g_free (subdir);
  g_free (out);
  g_free (err);
  xptr_array_unref (argv);
}

int
main (int    argc,
      char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/spawn/do-not-search", test_do_not_search);
  g_test_add_func ("/spawn/search-path", test_search_path);
  g_test_add_func ("/spawn/search-path-from-envp", test_search_path_from_envp);
  g_test_add_func ("/spawn/search-path-ambiguous", test_search_path_ambiguous);
  g_test_add_func ("/spawn/search-path-heap-allocation",
                   test_search_path_heap_allocation);
  g_test_add_func ("/spawn/search-path-fallback-in-environ",
                   test_search_path_fallback_in_environ);
  g_test_add_func ("/spawn/search-path-fallback-in-envp",
                   test_search_path_fallback_in_envp);

  return g_test_run ();
}
