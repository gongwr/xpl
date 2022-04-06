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

#include <errno.h>
#include <stdio.h>

#include <glib.h>

#ifdef G_OS_UNIX
#include <sys/types.h>
#include <sys/wait.h>
#endif

static void
child_setup (xpointer_t user_data)
{
}

typedef struct
{
  int wait_status;
  xboolean_t done;
} ChildStatus;

static ChildStatus child_status = { -1, FALSE };

static void
child_watch_cb (xpid_t pid,
                xint_t status,
                xpointer_t user_data)
{
  child_status.wait_status = status;
  child_status.done = TRUE;
}

int
main (int    argc,
      char **argv)
{
  xboolean_t search_path = FALSE;
  xboolean_t search_path_from_envp = FALSE;
  xboolean_t slow_path = FALSE;
  xboolean_t unset_path_in_envp = FALSE;
  xchar_t *chdir_child = NULL;
  xchar_t *set_path_in_envp = NULL;
  xchar_t **envp = NULL;
  GOptionEntry entries[] =
  {
    { "chdir-child", '\0',
      G_OPTION_FLAG_NONE, G_OPTION_ARXFILENAME, &chdir_child,
      "Run PROGRAM in this working directory", NULL },
    { "search-path", '\0',
      G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &search_path,
      "Search PATH for PROGRAM", NULL },
    { "search-path-from-envp", '\0',
      G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &search_path_from_envp,
      "Search PATH from specified environment", NULL },
    { "set-path-in-envp", '\0',
      G_OPTION_FLAG_NONE, G_OPTION_ARXFILENAME, &set_path_in_envp,
      "Set PATH in specified environment to this value", "PATH", },
    { "unset-path-in-envp", '\0',
      G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &unset_path_in_envp,
      "Unset PATH in specified environment", NULL },
    { "slow-path", '\0',
      G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &slow_path,
      "Use a child-setup function to avoid the posix_spawn fast path", NULL },
    G_OPTION_ENTRY_NULL
  };
  xerror_t *error = NULL;
  int ret = 1;
  GSpawnFlags spawn_flags = G_SPAWN_DO_NOT_REAP_CHILD;
  xpid_t pid;
  xoption_context_t *context = NULL;

  context = g_option_context_new ("PROGRAM [ARGS...]");
  g_option_context_add_main_entries (context, entries, NULL);

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      ret = 2;
      goto out;
    }

  if (argc < 2)
    {
      g_set_error (&error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
                   "Usage: %s [OPTIONS] PROGRAM [ARGS...]", argv[0]);
      ret = 2;
      goto out;
    }

  envp = g_get_environ ();

  if (set_path_in_envp != NULL && unset_path_in_envp)
    {
      g_set_error (&error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
                   "Cannot both set PATH and unset it");
      ret = 2;
      goto out;
    }

  if (set_path_in_envp != NULL)
    envp = g_environ_setenv (envp, "PATH", set_path_in_envp, TRUE);

  if (unset_path_in_envp)
    envp = g_environ_unsetenv (envp, "PATH");

  if (search_path)
    spawn_flags |= G_SPAWN_SEARCH_PATH;

  if (search_path_from_envp)
    spawn_flags |= G_SPAWN_SEARCH_PATH_FROM_ENVP;

  if (!g_spawn_async_with_pipes (chdir_child,
                                 &argv[1],
                                 envp,
                                 spawn_flags,
                                 slow_path ? child_setup : NULL,
                                 NULL,  /* user_data */
                                 &pid,
                                 NULL,  /* stdin */
                                 NULL,  /* stdout */
                                 NULL,  /* stderr */
                                 &error))
    {
      ret = 1;
      goto out;
    }

  g_child_watch_add (pid, child_watch_cb, NULL);

  while (!child_status.done)
    xmain_context_iteration (NULL, TRUE);

  g_spawn_close_pid (pid);

#ifdef G_OS_UNIX
  if (WIFEXITED (child_status.wait_status))
    ret = WEXITSTATUS (child_status.wait_status);
  else
    ret = 1;
#else
  ret = child_status.wait_status;
#endif

out:
  if (error != NULL)
    fprintf (stderr, "%s\n", error->message);

  g_free (set_path_in_envp);
  g_free (chdir_child);
  g_clear_error (&error);
  xstrfreev (envp);
  g_option_context_free (context);
  return ret;
}
