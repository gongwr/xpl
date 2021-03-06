/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2012, 2013 Red Hat, Inc.
 * Copyright © 2012, 2013 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 *
 * Authors: Colin Walters <walters@verbum.org>
 *          Ryan Lortie <desrt@desrt.ca>
 */

/**
 * SECTION:gsubprocess
 * @title: xsubprocess_t
 * @short_description: Child processes
 * @include: gio/gio.h
 * @see_also: #xsubprocess_launcher_t
 *
 * #xsubprocess_t allows the creation of and interaction with child
 * processes.
 *
 * Processes can be communicated with using standard GIO-style APIs (ie:
 * #xinput_stream_t, #xoutput_stream_t).  There are GIO-style APIs to wait for
 * process termination (ie: cancellable and with an asynchronous
 * variant).
 *
 * There is an API to force a process to terminate, as well as a
 * race-free API for sending UNIX signals to a subprocess.
 *
 * One major advantage that GIO brings over the core GLib library is
 * comprehensive API for asynchronous I/O, such
 * xoutput_stream_splice_async().  This makes xsubprocess_t
 * significantly more powerful and flexible than equivalent APIs in
 * some other languages such as the `subprocess.py`
 * included with Python.  For example, using #xsubprocess_t one could
 * create two child processes, reading standard output from the first,
 * processing it, and writing to the input stream of the second, all
 * without blocking the main loop.
 *
 * A powerful xsubprocess_communicate() API is provided similar to the
 * `communicate()` method of `subprocess.py`. This enables very easy
 * interaction with a subprocess that has been opened with pipes.
 *
 * #xsubprocess_t defaults to tight control over the file descriptors open
 * in the child process, avoiding dangling-fd issues that are caused by
 * a simple fork()/exec().  The only open file descriptors in the
 * spawned process are ones that were explicitly specified by the
 * #xsubprocess_t API (unless %G_SUBPROCESS_FLAGS_INHERIT_FDS was
 * specified).
 *
 * #xsubprocess_t will quickly reap all child processes as they exit,
 * avoiding "zombie processes" remaining around for long periods of
 * time.  xsubprocess_wait() can be used to wait for this to happen,
 * but it will happen even without the call being explicitly made.
 *
 * As a matter of principle, #xsubprocess_t has no API that accepts
 * shell-style space-separated strings.  It will, however, match the
 * typical shell behaviour of searching the PATH for executables that do
 * not contain a directory separator in their name. By default, the `PATH`
 * of the current process is used.  You can specify
 * %G_SUBPROCESS_FLAGS_SEARCH_PATH_FROM_ENVP to use the `PATH` of the
 * launcher environment instead.
 *
 * #xsubprocess_t attempts to have a very simple API for most uses (ie:
 * spawning a subprocess with arguments and support for most typical
 * kinds of input and output redirection).  See xsubprocess_new(). The
 * #xsubprocess_launcher_t API is provided for more complicated cases
 * (advanced types of redirection, environment variable manipulation,
 * change of working directory, child setup functions, etc).
 *
 * A typical use of #xsubprocess_t will involve calling
 * xsubprocess_new(), followed by xsubprocess_wait_async() or
 * xsubprocess_wait().  After the process exits, the status can be
 * checked using functions such as xsubprocess_get_if_exited() (which
 * are similar to the familiar WIFEXITED-style POSIX macros).
 *
 * Since: 2.40
 **/

#include "config.h"

#include "gsubprocess.h"
#include "gsubprocesslauncher-private.h"
#include "gasyncresult.h"
#include "giostream.h"
#include "gmemoryinputstream.h"
#include "glibintl.h"
#include "glib-private.h"

#include <string.h>
#ifdef G_OS_UNIX
#include <gio/gunixoutputstream.h>
#include <gio/gfiledescriptorbased.h>
#include <gio/gunixinputstream.h>
#include <gstdio.h>
#include <glib-unix.h>
#include <fcntl.h>
#endif
#ifdef G_OS_WIN32
#include <windows.h>
#include <io.h>
#include "giowin32-priv.h"
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#else
#define HAVE_O_CLOEXEC 1
#endif

#define COMMUNICATE_READ_SIZE 4096

/* A xsubprocess_t can have two possible states: running and not.
 *
 * These two states are reflected by the value of 'pid'.  If it is
 * non-zero then the process is running, with that pid.
 *
 * When a xsubprocess_t is first created with xobject_new() it is not
 * running.  When it is finalized, it is also not running.
 *
 * During initable_init(), if the g_spawn() is successful then we
 * immediately register a child watch and take an extra ref on the
 * subprocess.  That reference doesn't drop until the child has quit,
 * which is why finalize can only happen in the non-running state.  In
 * the event that the g_spawn() failed we will still be finalizing a
 * non-running xsubprocess_t (before returning from xsubprocess_new())
 * with NULL.
 *
 * We make extensive use of the glib worker thread to guarantee
 * race-free operation.  As with all child watches, glib calls waitpid()
 * in the worker thread.  It reports the child exiting to us via the
 * worker thread (which means that we can do synchronous waits without
 * running a separate loop).  We also send signals to the child process
 * via the worker thread so that we don't race with waitpid() and
 * accidentally send a signal to an already-reaped child.
 */
static void initable_iface_init (xinitable_iface_t         *initable_iface);

typedef xobject_class_t GSubprocessClass;

struct _xsubprocess
{
  xobject_t parent;

  /* only used during construction */
  xsubprocess_launcher_t *launcher;
  xsubprocess_flags_t flags;
  xchar_t **argv;

  /* state tracking variables */
  xchar_t identifier[24];
  int status;
  xpid_t pid;

  /* list of xtask_t */
  xmutex_t pending_waits_lock;
  xslist_t *pending_waits;

  /* These are the streams created if a pipe is requested via flags. */
  xoutput_stream_t *stdin_pipe;
  xinput_stream_t  *stdout_pipe;
  xinput_stream_t  *stderr_pipe;
};

G_DEFINE_TYPE_WITH_CODE (xsubprocess_t, xsubprocess, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_INITABLE, initable_iface_init))

enum
{
  PROP_0,
  PROP_FLAGS,
  PROP_ARGV,
  N_PROPS
};

static xinput_stream_t *
platform_input_stream_from_spawn_fd (xint_t fd)
{
  if (fd < 0)
    return NULL;

#ifdef G_OS_UNIX
  return g_unix_input_stream_new (fd, TRUE);
#else
  return g_win32_input_stream_new_from_fd (fd, TRUE);
#endif
}

static xoutput_stream_t *
platform_output_stream_from_spawn_fd (xint_t fd)
{
  if (fd < 0)
    return NULL;

#ifdef G_OS_UNIX
  return g_unix_output_stream_new (fd, TRUE);
#else
  return g_win32_output_stream_new_from_fd (fd, TRUE);
#endif
}

#ifdef G_OS_UNIX
static xint_t
unix_open_file (const char  *filename,
                xint_t         mode,
                xerror_t     **error)
{
  xint_t my_fd;

  my_fd = g_open (filename, mode | O_BINARY | O_CLOEXEC, 0666);

  /* If we return -1 we should also set the error */
  if (my_fd < 0)
    {
      xint_t saved_errno = errno;
      char *display_name;

      display_name = xfilename_display_name (filename);
      g_set_error (error, G_IO_ERROR, g_io_error_from_errno (saved_errno),
                   _("Error opening file “%s”: %s"), display_name,
                   xstrerror (saved_errno));
      g_free (display_name);
      /* fall through... */
    }
#ifndef HAVE_O_CLOEXEC
  else
    fcntl (my_fd, F_SETFD, FD_CLOEXEC);
#endif

  return my_fd;
}
#endif

static void
xsubprocess_set_property (xobject_t      *object,
                           xuint_t         prop_id,
                           const xvalue_t *value,
                           xparam_spec_t   *pspec)
{
  xsubprocess_t *self = G_SUBPROCESS (object);

  switch (prop_id)
    {
    case PROP_FLAGS:
      self->flags = xvalue_get_flags (value);
      break;

    case PROP_ARGV:
      self->argv = xvalue_dup_boxed (value);
      break;

    default:
      g_assert_not_reached ();
    }
}

static xboolean_t
xsubprocess_exited (xpid_t     pid,
                     xint_t     status,
                     xpointer_t user_data)
{
  xsubprocess_t *self = user_data;
  xslist_t *tasks;

  xassert (self->pid == pid);

  g_mutex_lock (&self->pending_waits_lock);
  self->status = status;
  tasks = self->pending_waits;
  self->pending_waits = NULL;
  self->pid = 0;
  g_mutex_unlock (&self->pending_waits_lock);

  /* Signal anyone in xsubprocess_wait_async() to wake up now */
  while (tasks)
    {
      xtask_return_boolean (tasks->data, TRUE);
      xobject_unref (tasks->data);
      tasks = xslist_delete_link (tasks, tasks);
    }

  g_spawn_close_pid (pid);

  return FALSE;
}

static xboolean_t
initable_init (xinitable_t     *initable,
               xcancellable_t  *cancellable,
               xerror_t       **error)
{
  xsubprocess_t *self = G_SUBPROCESS (initable);
  xint_t *pipe_ptrs[3] = { NULL, NULL, NULL };
  xint_t pipe_fds[3] = { -1, -1, -1 };
  xint_t close_fds[3] = { -1, -1, -1 };
#ifdef G_OS_UNIX
  xint_t stdin_fd = -1, stdout_fd = -1, stderr_fd = -1;
#endif
  GSpawnFlags spawn_flags = 0;
  xboolean_t success = FALSE;
  xint_t i;

  /* this is a programmer error */
  if (!self->argv || !self->argv[0] || !self->argv[0][0])
    return FALSE;

  if (xcancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  /* We must setup the three fds that will end up in the child as stdin,
   * stdout and stderr.
   *
   * First, stdin.
   */
  if (self->flags & G_SUBPROCESS_FLAGS_STDIN_INHERIT)
    spawn_flags |= G_SPAWN_CHILD_INHERITS_STDIN;
  else if (self->flags & G_SUBPROCESS_FLAGS_STDIN_PIPE)
    pipe_ptrs[0] = &pipe_fds[0];
#ifdef G_OS_UNIX
  else if (self->launcher)
    {
      if (self->launcher->stdin_fd != -1)
        stdin_fd = self->launcher->stdin_fd;
      else if (self->launcher->stdin_path != NULL)
        {
          stdin_fd = close_fds[0] = unix_open_file (self->launcher->stdin_path, O_RDONLY, error);
          if (stdin_fd == -1)
            goto out;
        }
    }
#endif

  /* Next, stdout. */
  if (self->flags & G_SUBPROCESS_FLAGS_STDOUT_SILENCE)
    spawn_flags |= G_SPAWN_STDOUT_TO_DEV_NULL;
  else if (self->flags & G_SUBPROCESS_FLAGS_STDOUT_PIPE)
    pipe_ptrs[1] = &pipe_fds[1];
#ifdef G_OS_UNIX
  else if (self->launcher)
    {
      if (self->launcher->stdout_fd != -1)
        stdout_fd = self->launcher->stdout_fd;
      else if (self->launcher->stdout_path != NULL)
        {
          stdout_fd = close_fds[1] = unix_open_file (self->launcher->stdout_path, O_CREAT | O_WRONLY, error);
          if (stdout_fd == -1)
            goto out;
        }
    }
#endif

  /* Finally, stderr. */
  if (self->flags & G_SUBPROCESS_FLAGS_STDERR_SILENCE)
    spawn_flags |= G_SPAWN_STDERR_TO_DEV_NULL;
  else if (self->flags & G_SUBPROCESS_FLAGS_STDERR_PIPE)
    pipe_ptrs[2] = &pipe_fds[2];
#ifdef G_OS_UNIX
  else if (self->flags & G_SUBPROCESS_FLAGS_STDERR_MERGE)
    /* This will work because stderr gets set up after stdout. */
    stderr_fd = 1;
  else if (self->launcher)
    {
      if (self->launcher->stderr_fd != -1)
        stderr_fd = self->launcher->stderr_fd;
      else if (self->launcher->stderr_path != NULL)
        {
          stderr_fd = close_fds[2] = unix_open_file (self->launcher->stderr_path, O_CREAT | O_WRONLY, error);
          if (stderr_fd == -1)
            goto out;
        }
    }
#endif

  /* argv0 has no '/' in it?  We better do a PATH lookup. */
  if (strchr (self->argv[0], G_DIR_SEPARATOR) == NULL)
    {
      if (self->launcher && self->launcher->flags & G_SUBPROCESS_FLAGS_SEARCH_PATH_FROM_ENVP)
        spawn_flags |= G_SPAWN_SEARCH_PATH_FROM_ENVP;
      else
        spawn_flags |= G_SPAWN_SEARCH_PATH;
    }

  if (self->flags & G_SUBPROCESS_FLAGS_INHERIT_FDS)
    spawn_flags |= G_SPAWN_LEAVE_DESCRIPTORS_OPEN;

  spawn_flags |= G_SPAWN_DO_NOT_REAP_CHILD;
  spawn_flags |= G_SPAWN_CLOEXEC_PIPES;

  success = g_spawn_async_with_pipes_and_fds (self->launcher ? self->launcher->cwd : NULL,
                                              (const xchar_t * const *) self->argv,
                                              (const xchar_t * const *) (self->launcher ? self->launcher->envp : NULL),
                                              spawn_flags,
#ifdef G_OS_UNIX
                                              self->launcher ? self->launcher->child_setup_func : NULL,
                                              self->launcher ? self->launcher->child_setup_user_data : NULL,
                                              stdin_fd, stdout_fd, stderr_fd,
                                              self->launcher ? (const xint_t *) self->launcher->source_fds->data : NULL,
                                              self->launcher ? (const xint_t *) self->launcher->target_fds->data : NULL,
                                              self->launcher ? self->launcher->source_fds->len : 0,
#else
                                              NULL, NULL,
                                              -1, -1, -1,
                                              NULL, NULL, 0,
#endif
                                              &self->pid,
                                              pipe_ptrs[0], pipe_ptrs[1], pipe_ptrs[2],
                                              error);
  xassert (success == (self->pid != 0));

  {
    xuint64_t identifier;
    xint_t s G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;

#ifdef G_OS_WIN32
    identifier = (xuint64_t) GetProcessId (self->pid);
#else
    identifier = (xuint64_t) self->pid;
#endif

    s = g_snprintf (self->identifier, sizeof self->identifier, "%"G_GUINT64_FORMAT, identifier);
    xassert (0 < s && (xsize_t) s < sizeof self->identifier);
  }

  /* Start attempting to reap the child immediately */
  if (success)
    {
      xmain_context_t *worker_context;
      xsource_t *source;

      worker_context = XPL_PRIVATE_CALL (g_get_worker_context) ();
      source = g_child_watch_source_new (self->pid);
      xsource_set_callback (source, (xsource_func_t) xsubprocess_exited, xobject_ref (self), xobject_unref);
      xsource_attach (source, worker_context);
      xsource_unref (source);
    }

#ifdef G_OS_UNIX
out:
#endif
  /* we don't need this past init... */
  self->launcher = NULL;

  for (i = 0; i < 3; i++)
    if (close_fds[i] != -1)
      close (close_fds[i]);

  self->stdin_pipe = platform_output_stream_from_spawn_fd (pipe_fds[0]);
  self->stdout_pipe = platform_input_stream_from_spawn_fd (pipe_fds[1]);
  self->stderr_pipe = platform_input_stream_from_spawn_fd (pipe_fds[2]);

  return success;
}

static void
xsubprocess_finalize (xobject_t *object)
{
  xsubprocess_t *self = G_SUBPROCESS (object);

  xassert (self->pending_waits == NULL);
  xassert (self->pid == 0);

  g_clear_object (&self->stdin_pipe);
  g_clear_object (&self->stdout_pipe);
  g_clear_object (&self->stderr_pipe);
  xstrfreev (self->argv);

  g_mutex_clear (&self->pending_waits_lock);

  XOBJECT_CLASS (xsubprocess_parent_class)->finalize (object);
}

static void
xsubprocess_init (xsubprocess_t  *self)
{
  g_mutex_init (&self->pending_waits_lock);
}

static void
initable_iface_init (xinitable_iface_t *initable_iface)
{
  initable_iface->init = initable_init;
}

static void
xsubprocess_class_init (GSubprocessClass *class)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (class);

  xobject_class->finalize = xsubprocess_finalize;
  xobject_class->set_property = xsubprocess_set_property;

  xobject_class_install_property (xobject_class, PROP_FLAGS,
                                   xparam_spec_flags ("flags", P_("Flags"), P_("Subprocess flags"),
                                                       XTYPE_SUBPROCESS_FLAGS, 0, XPARAM_WRITABLE |
                                                       XPARAM_CONSTRUCT_ONLY | XPARAM_STATIC_STRINGS));
  xobject_class_install_property (xobject_class, PROP_ARGV,
                                   xparam_spec_boxed ("argv", P_("Arguments"), P_("Argument vector"),
                                                       XTYPE_STRV, XPARAM_WRITABLE |
                                                       XPARAM_CONSTRUCT_ONLY | XPARAM_STATIC_STRINGS));
}

/**
 * xsubprocess_new: (skip)
 * @flags: flags that define the behaviour of the subprocess
 * @error: (nullable): return location for an error, or %NULL
 * @argv0: first commandline argument to pass to the subprocess
 * @...:   more commandline arguments, followed by %NULL
 *
 * Create a new process with the given flags and varargs argument
 * list.  By default, matching the g_spawn_async() defaults, the
 * child's stdin will be set to the system null device, and
 * stdout/stderr will be inherited from the parent.  You can use
 * @flags to control this behavior.
 *
 * The argument list must be terminated with %NULL.
 *
 * Returns: A newly created #xsubprocess_t, or %NULL on error (and @error
 *   will be set)
 *
 * Since: 2.40
 */
xsubprocess_t *
xsubprocess_new (xsubprocess_flags_t   flags,
                  xerror_t           **error,
                  const xchar_t       *argv0,
                  ...)
{
  xsubprocess_t *result;
  xptr_array_t *args;
  const xchar_t *arg;
  va_list ap;

  xreturn_val_if_fail (argv0 != NULL && argv0[0] != '\0', NULL);
  xreturn_val_if_fail (error == NULL || *error == NULL, NULL);

  args = xptr_array_new ();

  va_start (ap, argv0);
  xptr_array_add (args, (xchar_t *) argv0);
  while ((arg = va_arg (ap, const xchar_t *)))
    xptr_array_add (args, (xchar_t *) arg);
  xptr_array_add (args, NULL);
  va_end (ap);

  result = xsubprocess_newv ((const xchar_t * const *) args->pdata, flags, error);

  xptr_array_free (args, TRUE);

  return result;
}

/**
 * xsubprocess_newv: (rename-to xsubprocess_new)
 * @argv: (array zero-terminated=1) (element-type filename): commandline arguments for the subprocess
 * @flags: flags that define the behaviour of the subprocess
 * @error: (nullable): return location for an error, or %NULL
 *
 * Create a new process with the given flags and argument list.
 *
 * The argument list is expected to be %NULL-terminated.
 *
 * Returns: A newly created #xsubprocess_t, or %NULL on error (and @error
 *   will be set)
 *
 * Since: 2.40
 */
xsubprocess_t *
xsubprocess_newv (const xchar_t * const  *argv,
                   xsubprocess_flags_t      flags,
                   xerror_t              **error)
{
  xreturn_val_if_fail (argv != NULL && argv[0] != NULL && argv[0][0] != '\0', NULL);

  return xinitable_new (XTYPE_SUBPROCESS, NULL, error,
                         "argv", argv,
                         "flags", flags,
                         NULL);
}

/**
 * xsubprocess_get_identifier:
 * @subprocess: a #xsubprocess_t
 *
 * On UNIX, returns the process ID as a decimal string.
 * On Windows, returns the result of GetProcessId() also as a string.
 * If the subprocess has terminated, this will return %NULL.
 *
 * Returns: (nullable): the subprocess identifier, or %NULL if the subprocess
 *    has terminated
 * Since: 2.40
 */
const xchar_t *
xsubprocess_get_identifier (xsubprocess_t *subprocess)
{
  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), NULL);

  if (subprocess->pid)
    return subprocess->identifier;
  else
    return NULL;
}

/**
 * xsubprocess_get_stdin_pipe:
 * @subprocess: a #xsubprocess_t
 *
 * Gets the #xoutput_stream_t that you can write to in order to give data
 * to the stdin of @subprocess.
 *
 * The process must have been created with %G_SUBPROCESS_FLAGS_STDIN_PIPE and
 * not %G_SUBPROCESS_FLAGS_STDIN_INHERIT, otherwise %NULL will be returned.
 *
 * Returns: (nullable) (transfer none): the stdout pipe
 *
 * Since: 2.40
 **/
xoutput_stream_t *
xsubprocess_get_stdin_pipe (xsubprocess_t *subprocess)
{
  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), NULL);

  return subprocess->stdin_pipe;
}

/**
 * xsubprocess_get_stdout_pipe:
 * @subprocess: a #xsubprocess_t
 *
 * Gets the #xinput_stream_t from which to read the stdout output of
 * @subprocess.
 *
 * The process must have been created with %G_SUBPROCESS_FLAGS_STDOUT_PIPE,
 * otherwise %NULL will be returned.
 *
 * Returns: (nullable) (transfer none): the stdout pipe
 *
 * Since: 2.40
 **/
xinput_stream_t *
xsubprocess_get_stdout_pipe (xsubprocess_t *subprocess)
{
  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), NULL);

  return subprocess->stdout_pipe;
}

/**
 * xsubprocess_get_stderr_pipe:
 * @subprocess: a #xsubprocess_t
 *
 * Gets the #xinput_stream_t from which to read the stderr output of
 * @subprocess.
 *
 * The process must have been created with %G_SUBPROCESS_FLAGS_STDERR_PIPE,
 * otherwise %NULL will be returned.
 *
 * Returns: (nullable) (transfer none): the stderr pipe
 *
 * Since: 2.40
 **/
xinput_stream_t *
xsubprocess_get_stderr_pipe (xsubprocess_t *subprocess)
{
  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), NULL);

  return subprocess->stderr_pipe;
}

/* Remove the first list element containing @data, and return %TRUE. If no
 * such element is found, return %FALSE. */
static xboolean_t
slist_remove_if_present (xslist_t        **list,
                         xconstpointer   data)
{
  xslist_t *l, *prev;

  for (l = *list, prev = NULL; l != NULL; prev = l, l = prev->next)
    {
      if (l->data == data)
        {
          if (prev != NULL)
            prev->next = l->next;
          else
            *list = l->next;

          xslist_free_1 (l);

          return TRUE;
        }
    }

  return FALSE;
}

static void
xsubprocess_wait_cancelled (xcancellable_t *cancellable,
                             xpointer_t      user_data)
{
  xtask_t *task = user_data;
  xsubprocess_t *self;
  xboolean_t task_was_pending;

  self = xtask_get_source_object (task);

  g_mutex_lock (&self->pending_waits_lock);
  task_was_pending = slist_remove_if_present (&self->pending_waits, task);
  g_mutex_unlock (&self->pending_waits_lock);

  if (task_was_pending)
    {
      xtask_return_boolean (task, FALSE);
      xobject_unref (task);  /* ref from pending_waits */
    }
}

/**
 * xsubprocess_wait_async:
 * @subprocess: a #xsubprocess_t
 * @cancellable: a #xcancellable_t, or %NULL
 * @callback: a #xasync_ready_callback_t to call when the operation is complete
 * @user_data: user_data for @callback
 *
 * Wait for the subprocess to terminate.
 *
 * This is the asynchronous version of xsubprocess_wait().
 *
 * Since: 2.40
 */
void
xsubprocess_wait_async (xsubprocess_t         *subprocess,
                         xcancellable_t        *cancellable,
                         xasync_ready_callback_t  callback,
                         xpointer_t             user_data)
{
  xtask_t *task;

  task = xtask_new (subprocess, cancellable, callback, user_data);
  xtask_set_source_tag (task, xsubprocess_wait_async);

  g_mutex_lock (&subprocess->pending_waits_lock);
  if (subprocess->pid)
    {
      /* Only bother with cancellable if we're putting it in the list.
       * If not, it's going to dispatch immediately anyway and we will
       * see the cancellation in the _finish().
       */
      if (cancellable)
        xsignal_connect_object (cancellable, "cancelled", G_CALLBACK (xsubprocess_wait_cancelled), task, 0);

      subprocess->pending_waits = xslist_prepend (subprocess->pending_waits, task);
      task = NULL;
    }
  g_mutex_unlock (&subprocess->pending_waits_lock);

  /* If we still have task then it's because did_exit is already TRUE */
  if (task != NULL)
    {
      xtask_return_boolean (task, TRUE);
      xobject_unref (task);
    }
}

/**
 * xsubprocess_wait_finish:
 * @subprocess: a #xsubprocess_t
 * @result: the #xasync_result_t passed to your #xasync_ready_callback_t
 * @error: a pointer to a %NULL #xerror_t, or %NULL
 *
 * Collects the result of a previous call to
 * xsubprocess_wait_async().
 *
 * Returns: %TRUE if successful, or %FALSE with @error set
 *
 * Since: 2.40
 */
xboolean_t
xsubprocess_wait_finish (xsubprocess_t   *subprocess,
                          xasync_result_t  *result,
                          xerror_t       **error)
{
  return xtask_propagate_boolean (XTASK (result), error);
}

/* Some generic helpers for emulating synchronous operations using async
 * operations.
 */
static void
xsubprocess_sync_setup (void)
{
  xmain_context_push_thread_default (xmain_context_new ());
}

static void
xsubprocess_sync_done (xobject_t      *source_object,
                        xasync_result_t *result,
                        xpointer_t      user_data)
{
  xasync_result_t **result_ptr = user_data;

  *result_ptr = xobject_ref (result);
}

static void
xsubprocess_sync_complete (xasync_result_t **result)
{
  xmain_context_t *context = xmain_context_get_thread_default ();

  while (!*result)
    xmain_context_iteration (context, TRUE);

  xmain_context_pop_thread_default (context);
  xmain_context_unref (context);
}

/**
 * xsubprocess_wait:
 * @subprocess: a #xsubprocess_t
 * @cancellable: a #xcancellable_t
 * @error: a #xerror_t
 *
 * Synchronously wait for the subprocess to terminate.
 *
 * After the process terminates you can query its exit status with
 * functions such as xsubprocess_get_if_exited() and
 * xsubprocess_get_exit_status().
 *
 * This function does not fail in the case of the subprocess having
 * abnormal termination.  See xsubprocess_wait_check() for that.
 *
 * Cancelling @cancellable doesn't kill the subprocess.  Call
 * xsubprocess_force_exit() if it is desirable.
 *
 * Returns: %TRUE on success, %FALSE if @cancellable was cancelled
 *
 * Since: 2.40
 */
xboolean_t
xsubprocess_wait (xsubprocess_t   *subprocess,
                   xcancellable_t  *cancellable,
                   xerror_t       **error)
{
  xasync_result_t *result = NULL;
  xboolean_t success;

  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), FALSE);

  /* Synchronous waits are actually the 'more difficult' case because we
   * need to deal with the possibility of cancellation.  That more or
   * less implies that we need a main context (to dispatch either of the
   * possible reasons for the operation ending).
   *
   * So we make one and then do this async...
   */

  if (xcancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  /* We can shortcut in the case that the process already quit (but only
   * after we checked the cancellable).
   */
  if (subprocess->pid == 0)
    return TRUE;

  /* Otherwise, we need to do this the long way... */
  xsubprocess_sync_setup ();
  xsubprocess_wait_async (subprocess, cancellable, xsubprocess_sync_done, &result);
  xsubprocess_sync_complete (&result);
  success = xsubprocess_wait_finish (subprocess, result, error);
  xobject_unref (result);

  return success;
}

/**
 * xsubprocess_wait_check:
 * @subprocess: a #xsubprocess_t
 * @cancellable: a #xcancellable_t
 * @error: a #xerror_t
 *
 * Combines xsubprocess_wait() with g_spawn_check_wait_status().
 *
 * Returns: %TRUE on success, %FALSE if process exited abnormally, or
 * @cancellable was cancelled
 *
 * Since: 2.40
 */
xboolean_t
xsubprocess_wait_check (xsubprocess_t   *subprocess,
                         xcancellable_t  *cancellable,
                         xerror_t       **error)
{
  return xsubprocess_wait (subprocess, cancellable, error) &&
         g_spawn_check_wait_status (subprocess->status, error);
}

/**
 * xsubprocess_wait_check_async:
 * @subprocess: a #xsubprocess_t
 * @cancellable: a #xcancellable_t, or %NULL
 * @callback: a #xasync_ready_callback_t to call when the operation is complete
 * @user_data: user_data for @callback
 *
 * Combines xsubprocess_wait_async() with g_spawn_check_wait_status().
 *
 * This is the asynchronous version of xsubprocess_wait_check().
 *
 * Since: 2.40
 */
void
xsubprocess_wait_check_async (xsubprocess_t         *subprocess,
                               xcancellable_t        *cancellable,
                               xasync_ready_callback_t  callback,
                               xpointer_t             user_data)
{
  xsubprocess_wait_async (subprocess, cancellable, callback, user_data);
}

/**
 * xsubprocess_wait_check_finish:
 * @subprocess: a #xsubprocess_t
 * @result: the #xasync_result_t passed to your #xasync_ready_callback_t
 * @error: a pointer to a %NULL #xerror_t, or %NULL
 *
 * Collects the result of a previous call to
 * xsubprocess_wait_check_async().
 *
 * Returns: %TRUE if successful, or %FALSE with @error set
 *
 * Since: 2.40
 */
xboolean_t
xsubprocess_wait_check_finish (xsubprocess_t   *subprocess,
                                xasync_result_t  *result,
                                xerror_t       **error)
{
  return xsubprocess_wait_finish (subprocess, result, error) &&
         g_spawn_check_wait_status (subprocess->status, error);
}

#ifdef G_OS_UNIX
typedef struct
{
  xsubprocess_t *subprocess;
  xint_t signalnum;
} SignalRecord;

static xboolean_t
xsubprocess_actually_send_signal (xpointer_t user_data)
{
  SignalRecord *signal_record = user_data;

  /* The pid is set to zero from the worker thread as well, so we don't
   * need to take a lock in order to prevent it from changing under us.
   */
  if (signal_record->subprocess->pid)
    kill (signal_record->subprocess->pid, signal_record->signalnum);

  xobject_unref (signal_record->subprocess);

  g_slice_free (SignalRecord, signal_record);

  return FALSE;
}

static void
xsubprocess_dispatch_signal (xsubprocess_t *subprocess,
                              xint_t         signalnum)
{
  SignalRecord signal_record = { xobject_ref (subprocess), signalnum };

  g_return_if_fail (X_IS_SUBPROCESS (subprocess));

  /* This MUST be a lower priority than the priority that the child
   * watch source uses in initable_init().
   *
   * Reaping processes, reporting the results back to xsubprocess_t and
   * sending signals is all done in the glib worker thread.  We cannot
   * have a kill() done after the reap and before the report without
   * risking killing a process that's no longer there so the kill()
   * needs to have the lower priority.
   *
   * G_PRIORITY_HIGH_IDLE is lower priority than G_PRIORITY_DEFAULT.
   */
  xmain_context_invoke_full (XPL_PRIVATE_CALL (g_get_worker_context) (),
                              G_PRIORITY_HIGH_IDLE,
                              xsubprocess_actually_send_signal,
                              g_slice_dup (SignalRecord, &signal_record),
                              NULL);
}

/**
 * xsubprocess_send_signal:
 * @subprocess: a #xsubprocess_t
 * @signal_num: the signal number to send
 *
 * Sends the UNIX signal @signal_num to the subprocess, if it is still
 * running.
 *
 * This API is race-free.  If the subprocess has terminated, it will not
 * be signalled.
 *
 * This API is not available on Windows.
 *
 * Since: 2.40
 **/
void
xsubprocess_send_signal (xsubprocess_t *subprocess,
                          xint_t         signal_num)
{
  g_return_if_fail (X_IS_SUBPROCESS (subprocess));

  xsubprocess_dispatch_signal (subprocess, signal_num);
}
#endif

/**
 * xsubprocess_force_exit:
 * @subprocess: a #xsubprocess_t
 *
 * Use an operating-system specific method to attempt an immediate,
 * forceful termination of the process.  There is no mechanism to
 * determine whether or not the request itself was successful;
 * however, you can use xsubprocess_wait() to monitor the status of
 * the process after calling this function.
 *
 * On Unix, this function sends %SIGKILL.
 *
 * Since: 2.40
 **/
void
xsubprocess_force_exit (xsubprocess_t *subprocess)
{
  g_return_if_fail (X_IS_SUBPROCESS (subprocess));

#ifdef G_OS_UNIX
  xsubprocess_dispatch_signal (subprocess, SIGKILL);
#else
  TerminateProcess (subprocess->pid, 1);
#endif
}

/**
 * xsubprocess_get_status:
 * @subprocess: a #xsubprocess_t
 *
 * Gets the raw status code of the process, as from waitpid().
 *
 * This value has no particular meaning, but it can be used with the
 * macros defined by the system headers such as WIFEXITED.  It can also
 * be used with g_spawn_check_wait_status().
 *
 * It is more likely that you want to use xsubprocess_get_if_exited()
 * followed by xsubprocess_get_exit_status().
 *
 * It is an error to call this function before xsubprocess_wait() has
 * returned.
 *
 * Returns: the (meaningless) waitpid() exit status from the kernel
 *
 * Since: 2.40
 **/
xint_t
xsubprocess_get_status (xsubprocess_t *subprocess)
{
  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), FALSE);
  xreturn_val_if_fail (subprocess->pid == 0, FALSE);

  return subprocess->status;
}

/**
 * xsubprocess_get_successful:
 * @subprocess: a #xsubprocess_t
 *
 * Checks if the process was "successful".  A process is considered
 * successful if it exited cleanly with an exit status of 0, either by
 * way of the exit() system call or return from main().
 *
 * It is an error to call this function before xsubprocess_wait() has
 * returned.
 *
 * Returns: %TRUE if the process exited cleanly with a exit status of 0
 *
 * Since: 2.40
 **/
xboolean_t
xsubprocess_get_successful (xsubprocess_t *subprocess)
{
  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), FALSE);
  xreturn_val_if_fail (subprocess->pid == 0, FALSE);

#ifdef G_OS_UNIX
  return WIFEXITED (subprocess->status) && WEXITSTATUS (subprocess->status) == 0;
#else
  return subprocess->status == 0;
#endif
}

/**
 * xsubprocess_get_if_exited:
 * @subprocess: a #xsubprocess_t
 *
 * Check if the given subprocess exited normally (ie: by way of exit()
 * or return from main()).
 *
 * This is equivalent to the system WIFEXITED macro.
 *
 * It is an error to call this function before xsubprocess_wait() has
 * returned.
 *
 * Returns: %TRUE if the case of a normal exit
 *
 * Since: 2.40
 **/
xboolean_t
xsubprocess_get_if_exited (xsubprocess_t *subprocess)
{
  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), FALSE);
  xreturn_val_if_fail (subprocess->pid == 0, FALSE);

#ifdef G_OS_UNIX
  return WIFEXITED (subprocess->status);
#else
  return TRUE;
#endif
}

/**
 * xsubprocess_get_exit_status:
 * @subprocess: a #xsubprocess_t
 *
 * Check the exit status of the subprocess, given that it exited
 * normally.  This is the value passed to the exit() system call or the
 * return value from main.
 *
 * This is equivalent to the system WEXITSTATUS macro.
 *
 * It is an error to call this function before xsubprocess_wait() and
 * unless xsubprocess_get_if_exited() returned %TRUE.
 *
 * Returns: the exit status
 *
 * Since: 2.40
 **/
xint_t
xsubprocess_get_exit_status (xsubprocess_t *subprocess)
{
  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), 1);
  xreturn_val_if_fail (subprocess->pid == 0, 1);

#ifdef G_OS_UNIX
  xreturn_val_if_fail (WIFEXITED (subprocess->status), 1);

  return WEXITSTATUS (subprocess->status);
#else
  return subprocess->status;
#endif
}

/**
 * xsubprocess_get_if_signaled:
 * @subprocess: a #xsubprocess_t
 *
 * Check if the given subprocess terminated in response to a signal.
 *
 * This is equivalent to the system WIFSIGNALED macro.
 *
 * It is an error to call this function before xsubprocess_wait() has
 * returned.
 *
 * Returns: %TRUE if the case of termination due to a signal
 *
 * Since: 2.40
 **/
xboolean_t
xsubprocess_get_if_signaled (xsubprocess_t *subprocess)
{
  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), FALSE);
  xreturn_val_if_fail (subprocess->pid == 0, FALSE);

#ifdef G_OS_UNIX
  return WIFSIGNALED (subprocess->status);
#else
  return FALSE;
#endif
}

/**
 * xsubprocess_get_term_sig:
 * @subprocess: a #xsubprocess_t
 *
 * Get the signal number that caused the subprocess to terminate, given
 * that it terminated due to a signal.
 *
 * This is equivalent to the system WTERMSIG macro.
 *
 * It is an error to call this function before xsubprocess_wait() and
 * unless xsubprocess_get_if_signaled() returned %TRUE.
 *
 * Returns: the signal causing termination
 *
 * Since: 2.40
 **/
xint_t
xsubprocess_get_term_sig (xsubprocess_t *subprocess)
{
  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), 0);
  xreturn_val_if_fail (subprocess->pid == 0, 0);

#ifdef G_OS_UNIX
  xreturn_val_if_fail (WIFSIGNALED (subprocess->status), 0);

  return WTERMSIG (subprocess->status);
#else
  g_critical ("xsubprocess_get_term_sig() called on Windows, where "
              "xsubprocess_get_if_signaled() always returns FALSE...");
  return 0;
#endif
}

/*< private >*/
void
xsubprocess_set_launcher (xsubprocess_t         *subprocess,
                           xsubprocess_launcher_t *launcher)
{
  subprocess->launcher = launcher;
}


/* xsubprocess_communicate implementation below:
 *
 * This is a tough problem.  We have to watch 5 things at the same time:
 *
 *  - writing to stdin made progress
 *  - reading from stdout made progress
 *  - reading from stderr made progress
 *  - process terminated
 *  - cancellable being cancelled by caller
 *
 * We use a xmain_context_t for all of these (either as async function
 * calls or as a xsource_t (in the case of the cancellable).  That way at
 * least we don't have to worry about threading.
 *
 * For the sync case we use the usual trick of creating a private main
 * context and iterating it until completion.
 *
 * It's very possible that the process will dump a lot of data to stdout
 * just before it quits, so we can easily have data to read from stdout
 * and see the process has terminated at the same time.  We want to make
 * sure that we read all of the data from the pipes first, though, so we
 * do IO operations at a higher priority than the wait operation (which
 * is at G_IO_PRIORITY_DEFAULT).  Even in the case that we have to do
 * multiple reads to get this data, the pipe() will always be polling
 * as ready and with the async result for the read at a higher priority,
 * the main context will not dispatch the completion for the wait().
 *
 * We keep our own private xcancellable_t.  In the event that any of the
 * above suffers from an error condition (including the user cancelling
 * their cancellable) we immediately dispatch the xtask_t with the error
 * result and fire our cancellable to cleanup any pending operations.
 * In the case that the error is that the user's cancellable was fired,
 * it's vaguely wasteful to report an error because xtask_t will handle
 * this automatically, so we just return FALSE.
 *
 * We let each pending sub-operation take a ref on the xtask_t of the
 * communicate operation.  We have to be careful that we don't report
 * the task completion more than once, though, so we keep a flag for
 * that.
 */
typedef struct
{
  const xchar_t *stdin_data;
  xsize_t stdin_length;
  xsize_t stdin_offset;

  xboolean_t add_nul;

  xinput_stream_t *stdin_buf;
  xmemory_output_stream_t *stdout_buf;
  xmemory_output_stream_t *stderr_buf;

  xcancellable_t *cancellable;
  xsource_t      *cancellable_source;

  xuint_t         outstanding_ops;
  xboolean_t      reported_error;
} CommunicateState;

static void
xsubprocess_communicate_made_progress (xobject_t      *source_object,
                                        xasync_result_t *result,
                                        xpointer_t      user_data)
{
  CommunicateState *state;
  xsubprocess_t *subprocess;
  xerror_t *error = NULL;
  xpointer_t source;
  xtask_t *task;

  xassert (source_object != NULL);

  task = user_data;
  subprocess = xtask_get_source_object (task);
  state = xtask_get_task_data (task);
  source = source_object;

  state->outstanding_ops--;

  if (source == subprocess->stdin_pipe ||
      source == state->stdout_buf ||
      source == state->stderr_buf)
    {
      if (xoutput_stream_splice_finish ((xoutput_stream_t*) source, result, &error) == -1)
        goto out;

      if (source == state->stdout_buf ||
          source == state->stderr_buf)
        {
          /* This is a memory stream, so it can't be cancelled or return
           * an error really.
           */
          if (state->add_nul)
            {
              xsize_t bytes_written;
              if (!xoutput_stream_write_all (source, "\0", 1, &bytes_written,
                                              NULL, &error))
                goto out;
            }
          if (!xoutput_stream_close (source, NULL, &error))
            goto out;
        }
    }
  else if (source == subprocess)
    {
      (void) xsubprocess_wait_finish (subprocess, result, &error);
    }
  else
    g_assert_not_reached ();

 out:
  if (error)
    {
      /* Only report the first error we see.
       *
       * We might be seeing an error as a result of the cancellation
       * done when the process quits.
       */
      if (!state->reported_error)
        {
          state->reported_error = TRUE;
          xcancellable_cancel (state->cancellable);
          xtask_return_error (task, error);
        }
      else
        xerror_free (error);
    }
  else if (state->outstanding_ops == 0)
    {
      xtask_return_boolean (task, TRUE);
    }

  /* And drop the original ref */
  xobject_unref (task);
}

static xboolean_t
xsubprocess_communicate_cancelled (xcancellable_t *cancellable,
                                    xpointer_t      user_data)
{
  CommunicateState *state = user_data;

  xcancellable_cancel (state->cancellable);

  return FALSE;
}

static void
xsubprocess_communicate_state_free (xpointer_t data)
{
  CommunicateState *state = data;

  g_clear_object (&state->cancellable);
  g_clear_object (&state->stdin_buf);
  g_clear_object (&state->stdout_buf);
  g_clear_object (&state->stderr_buf);

  if (state->cancellable_source)
    {
      xsource_destroy (state->cancellable_source);
      xsource_unref (state->cancellable_source);
    }

  g_slice_free (CommunicateState, state);
}

static CommunicateState *
xsubprocess_communicate_internal (xsubprocess_t         *subprocess,
                                   xboolean_t             add_nul,
                                   xbytes_t              *stdin_buf,
                                   xcancellable_t        *cancellable,
                                   xasync_ready_callback_t  callback,
                                   xpointer_t             user_data)
{
  CommunicateState *state;
  xtask_t *task;

  task = xtask_new (subprocess, cancellable, callback, user_data);
  xtask_set_source_tag (task, xsubprocess_communicate_internal);

  state = g_slice_new0 (CommunicateState);
  xtask_set_task_data (task, state, xsubprocess_communicate_state_free);

  state->cancellable = xcancellable_new ();
  state->add_nul = add_nul;

  if (cancellable)
    {
      state->cancellable_source = xcancellable_source_new (cancellable);
      /* No ref held here, but we unref the source from state's free function */
      xsource_set_callback (state->cancellable_source,
                             G_SOURCE_FUNC (xsubprocess_communicate_cancelled),
                             state, NULL);
      xsource_attach (state->cancellable_source, xmain_context_get_thread_default ());
    }

  if (subprocess->stdin_pipe)
    {
      xassert (stdin_buf != NULL);

#ifdef G_OS_UNIX
      /* We're doing async writes to the pipe, and the async write mechanism assumes
       * that streams polling as writable do SOME progress (possibly partial) and then
       * stop, but never block.
       *
       * However, for blocking pipes, unix will return writable if there is *any* space left
       * but still block until the full buffer size is available before returning from write.
       * So, to avoid async blocking on the main loop we make this non-blocking here.
       *
       * It should be safe to change the fd because we're the only user at this point as
       * per the xsubprocess_communicate() docs, and all the code called by this function
       * properly handles non-blocking fds.
       */
      g_unix_set_fd_nonblocking (g_unix_output_stream_get_fd (G_UNIX_OUTPUT_STREAM (subprocess->stdin_pipe)), TRUE, NULL);
#endif

      state->stdin_buf = g_memory_input_stream_new_from_bytes (stdin_buf);
      xoutput_stream_splice_async (subprocess->stdin_pipe, (xinput_stream_t*)state->stdin_buf,
                                    G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                    G_PRIORITY_DEFAULT, state->cancellable,
                                    xsubprocess_communicate_made_progress, xobject_ref (task));
      state->outstanding_ops++;
    }

  if (subprocess->stdout_pipe)
    {
      state->stdout_buf = (xmemory_output_stream_t*)g_memory_output_stream_new_resizable ();
      xoutput_stream_splice_async ((xoutput_stream_t*)state->stdout_buf, subprocess->stdout_pipe,
                                    G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE,
                                    G_PRIORITY_DEFAULT, state->cancellable,
                                    xsubprocess_communicate_made_progress, xobject_ref (task));
      state->outstanding_ops++;
    }

  if (subprocess->stderr_pipe)
    {
      state->stderr_buf = (xmemory_output_stream_t*)g_memory_output_stream_new_resizable ();
      xoutput_stream_splice_async ((xoutput_stream_t*)state->stderr_buf, subprocess->stderr_pipe,
                                    G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE,
                                    G_PRIORITY_DEFAULT, state->cancellable,
                                    xsubprocess_communicate_made_progress, xobject_ref (task));
      state->outstanding_ops++;
    }

  xsubprocess_wait_async (subprocess, state->cancellable,
                           xsubprocess_communicate_made_progress, xobject_ref (task));
  state->outstanding_ops++;

  xobject_unref (task);
  return state;
}

/**
 * xsubprocess_communicate:
 * @subprocess: a #xsubprocess_t
 * @stdin_buf: (nullable): data to send to the stdin of the subprocess, or %NULL
 * @cancellable: a #xcancellable_t
 * @stdout_buf: (out) (nullable) (optional) (transfer full): data read from the subprocess stdout
 * @stderr_buf: (out) (nullable) (optional) (transfer full): data read from the subprocess stderr
 * @error: a pointer to a %NULL #xerror_t pointer, or %NULL
 *
 * Communicate with the subprocess until it terminates, and all input
 * and output has been completed.
 *
 * If @stdin_buf is given, the subprocess must have been created with
 * %G_SUBPROCESS_FLAGS_STDIN_PIPE.  The given data is fed to the
 * stdin of the subprocess and the pipe is closed (ie: EOF).
 *
 * At the same time (as not to cause blocking when dealing with large
 * amounts of data), if %G_SUBPROCESS_FLAGS_STDOUT_PIPE or
 * %G_SUBPROCESS_FLAGS_STDERR_PIPE were used, reads from those
 * streams.  The data that was read is returned in @stdout and/or
 * the @stderr.
 *
 * If the subprocess was created with %G_SUBPROCESS_FLAGS_STDOUT_PIPE,
 * @stdout_buf will contain the data read from stdout.  Otherwise, for
 * subprocesses not created with %G_SUBPROCESS_FLAGS_STDOUT_PIPE,
 * @stdout_buf will be set to %NULL.  Similar provisions apply to
 * @stderr_buf and %G_SUBPROCESS_FLAGS_STDERR_PIPE.
 *
 * As usual, any output variable may be given as %NULL to ignore it.
 *
 * If you desire the stdout and stderr data to be interleaved, create
 * the subprocess with %G_SUBPROCESS_FLAGS_STDOUT_PIPE and
 * %G_SUBPROCESS_FLAGS_STDERR_MERGE.  The merged result will be returned
 * in @stdout_buf and @stderr_buf will be set to %NULL.
 *
 * In case of any error (including cancellation), %FALSE will be
 * returned with @error set.  Some or all of the stdin data may have
 * been written.  Any stdout or stderr data that has been read will be
 * discarded. None of the out variables (aside from @error) will have
 * been set to anything in particular and should not be inspected.
 *
 * In the case that %TRUE is returned, the subprocess has exited and the
 * exit status inspection APIs (eg: xsubprocess_get_if_exited(),
 * xsubprocess_get_exit_status()) may be used.
 *
 * You should not attempt to use any of the subprocess pipes after
 * starting this function, since they may be left in strange states,
 * even if the operation was cancelled.  You should especially not
 * attempt to interact with the pipes while the operation is in progress
 * (either from another thread or if using the asynchronous version).
 *
 * Returns: %TRUE if successful
 *
 * Since: 2.40
 **/
xboolean_t
xsubprocess_communicate (xsubprocess_t   *subprocess,
                          xbytes_t        *stdin_buf,
                          xcancellable_t  *cancellable,
                          xbytes_t       **stdout_buf,
                          xbytes_t       **stderr_buf,
                          xerror_t       **error)
{
  xasync_result_t *result = NULL;
  xboolean_t success;

  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), FALSE);
  xreturn_val_if_fail (stdin_buf == NULL || (subprocess->flags & G_SUBPROCESS_FLAGS_STDIN_PIPE), FALSE);
  xreturn_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  xsubprocess_sync_setup ();
  xsubprocess_communicate_internal (subprocess, FALSE, stdin_buf, cancellable,
                                     xsubprocess_sync_done, &result);
  xsubprocess_sync_complete (&result);
  success = xsubprocess_communicate_finish (subprocess, result, stdout_buf, stderr_buf, error);
  xobject_unref (result);

  return success;
}

/**
 * xsubprocess_communicate_async:
 * @subprocess: Self
 * @stdin_buf: (nullable): Input data, or %NULL
 * @cancellable: (nullable): Cancellable
 * @callback: Callback
 * @user_data: User data
 *
 * Asynchronous version of xsubprocess_communicate().  Complete
 * invocation with xsubprocess_communicate_finish().
 */
void
xsubprocess_communicate_async (xsubprocess_t         *subprocess,
                                xbytes_t              *stdin_buf,
                                xcancellable_t        *cancellable,
                                xasync_ready_callback_t  callback,
                                xpointer_t             user_data)
{
  g_return_if_fail (X_IS_SUBPROCESS (subprocess));
  g_return_if_fail (stdin_buf == NULL || (subprocess->flags & G_SUBPROCESS_FLAGS_STDIN_PIPE));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  xsubprocess_communicate_internal (subprocess, FALSE, stdin_buf, cancellable, callback, user_data);
}

/**
 * xsubprocess_communicate_finish:
 * @subprocess: Self
 * @result: Result
 * @stdout_buf: (out) (nullable) (optional) (transfer full): Return location for stdout data
 * @stderr_buf: (out) (nullable) (optional) (transfer full): Return location for stderr data
 * @error: Error
 *
 * Complete an invocation of xsubprocess_communicate_async().
 */
xboolean_t
xsubprocess_communicate_finish (xsubprocess_t   *subprocess,
                                 xasync_result_t  *result,
                                 xbytes_t       **stdout_buf,
                                 xbytes_t       **stderr_buf,
                                 xerror_t       **error)
{
  xboolean_t success;
  CommunicateState *state;

  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), FALSE);
  xreturn_val_if_fail (xtask_is_valid (result, subprocess), FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  xobject_ref (result);

  state = xtask_get_task_data ((xtask_t*)result);
  success = xtask_propagate_boolean ((xtask_t*)result, error);

  if (success)
    {
      if (stdout_buf)
        *stdout_buf = (state->stdout_buf != NULL) ? g_memory_output_stream_steal_as_bytes (state->stdout_buf) : NULL;
      if (stderr_buf)
        *stderr_buf = (state->stderr_buf != NULL) ? g_memory_output_stream_steal_as_bytes (state->stderr_buf) : NULL;
    }

  xobject_unref (result);
  return success;
}

/**
 * xsubprocess_communicate_utf8:
 * @subprocess: a #xsubprocess_t
 * @stdin_buf: (nullable): data to send to the stdin of the subprocess, or %NULL
 * @cancellable: a #xcancellable_t
 * @stdout_buf: (out) (nullable) (optional) (transfer full): data read from the subprocess stdout
 * @stderr_buf: (out) (nullable) (optional) (transfer full): data read from the subprocess stderr
 * @error: a pointer to a %NULL #xerror_t pointer, or %NULL
 *
 * Like xsubprocess_communicate(), but validates the output of the
 * process as UTF-8, and returns it as a regular NUL terminated string.
 *
 * On error, @stdout_buf and @stderr_buf will be set to undefined values and
 * should not be used.
 */
xboolean_t
xsubprocess_communicate_utf8 (xsubprocess_t   *subprocess,
                               const char    *stdin_buf,
                               xcancellable_t  *cancellable,
                               char         **stdout_buf,
                               char         **stderr_buf,
                               xerror_t       **error)
{
  xasync_result_t *result = NULL;
  xboolean_t success;
  xbytes_t *stdin_bytes;
  size_t stdin_buf_len = 0;

  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), FALSE);
  xreturn_val_if_fail (stdin_buf == NULL || (subprocess->flags & G_SUBPROCESS_FLAGS_STDIN_PIPE), FALSE);
  xreturn_val_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable), FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (stdin_buf != NULL)
    stdin_buf_len = strlen (stdin_buf);
  stdin_bytes = xbytes_new (stdin_buf, stdin_buf_len);

  xsubprocess_sync_setup ();
  xsubprocess_communicate_internal (subprocess, TRUE, stdin_bytes, cancellable,
                                     xsubprocess_sync_done, &result);
  xsubprocess_sync_complete (&result);
  success = xsubprocess_communicate_utf8_finish (subprocess, result, stdout_buf, stderr_buf, error);
  xobject_unref (result);

  xbytes_unref (stdin_bytes);
  return success;
}

/**
 * xsubprocess_communicate_utf8_async:
 * @subprocess: Self
 * @stdin_buf: (nullable): Input data, or %NULL
 * @cancellable: Cancellable
 * @callback: Callback
 * @user_data: User data
 *
 * Asynchronous version of xsubprocess_communicate_utf8().  Complete
 * invocation with xsubprocess_communicate_utf8_finish().
 */
void
xsubprocess_communicate_utf8_async (xsubprocess_t         *subprocess,
                                     const char          *stdin_buf,
                                     xcancellable_t        *cancellable,
                                     xasync_ready_callback_t  callback,
                                     xpointer_t             user_data)
{
  xbytes_t *stdin_bytes;
  size_t stdin_buf_len = 0;

  g_return_if_fail (X_IS_SUBPROCESS (subprocess));
  g_return_if_fail (stdin_buf == NULL || (subprocess->flags & G_SUBPROCESS_FLAGS_STDIN_PIPE));
  g_return_if_fail (cancellable == NULL || X_IS_CANCELLABLE (cancellable));

  if (stdin_buf != NULL)
    stdin_buf_len = strlen (stdin_buf);
  stdin_bytes = xbytes_new (stdin_buf, stdin_buf_len);

  xsubprocess_communicate_internal (subprocess, TRUE, stdin_bytes, cancellable, callback, user_data);

  xbytes_unref (stdin_bytes);
}

static xboolean_t
communicate_result_validate_utf8 (const char            *stream_name,
                                  char                 **return_location,
                                  xmemory_output_stream_t   *buffer,
                                  xerror_t               **error)
{
  if (return_location == NULL)
    return TRUE;

  if (buffer)
    {
      const char *end;
      *return_location = g_memory_output_stream_steal_data (buffer);
      if (!xutf8_validate (*return_location, -1, &end))
        {
          g_free (*return_location);
          *return_location = NULL;
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "Invalid UTF-8 in child %s at offset %lu",
                       stream_name,
                       (unsigned long) (end - *return_location));
          return FALSE;
        }
    }
  else
    *return_location = NULL;

  return TRUE;
}

/**
 * xsubprocess_communicate_utf8_finish:
 * @subprocess: Self
 * @result: Result
 * @stdout_buf: (out) (nullable) (optional) (transfer full): Return location for stdout data
 * @stderr_buf: (out) (nullable) (optional) (transfer full): Return location for stderr data
 * @error: Error
 *
 * Complete an invocation of xsubprocess_communicate_utf8_async().
 */
xboolean_t
xsubprocess_communicate_utf8_finish (xsubprocess_t   *subprocess,
                                      xasync_result_t  *result,
                                      char         **stdout_buf,
                                      char         **stderr_buf,
                                      xerror_t       **error)
{
  xboolean_t ret = FALSE;
  CommunicateState *state;
  xchar_t *local_stdout_buf = NULL, *local_stderr_buf = NULL;

  xreturn_val_if_fail (X_IS_SUBPROCESS (subprocess), FALSE);
  xreturn_val_if_fail (xtask_is_valid (result, subprocess), FALSE);
  xreturn_val_if_fail (error == NULL || *error == NULL, FALSE);

  xobject_ref (result);

  state = xtask_get_task_data ((xtask_t*)result);
  if (!xtask_propagate_boolean ((xtask_t*)result, error))
    goto out;

  /* TODO - validate UTF-8 while streaming, rather than all at once.
   */
  if (!communicate_result_validate_utf8 ("stdout", &local_stdout_buf,
                                         state->stdout_buf,
                                         error))
    goto out;
  if (!communicate_result_validate_utf8 ("stderr", &local_stderr_buf,
                                         state->stderr_buf,
                                         error))
    goto out;

  ret = TRUE;
 out:
  xobject_unref (result);

  if (ret && stdout_buf != NULL)
    *stdout_buf = g_steal_pointer (&local_stdout_buf);
  if (ret && stderr_buf != NULL)
    *stderr_buf = g_steal_pointer (&local_stderr_buf);

  g_free (local_stderr_buf);
  g_free (local_stdout_buf);

  return ret;
}
