/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2012 Red Hat, Inc.
 * Copyright © 2012-2013 Canonical Limited
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
 * SECTION:gsubprocesslauncher
 * @title: xsubprocess_t Launcher
 * @short_description: Environment options for launching a child process
 * @include: gio/gio.h
 *
 * This class contains a set of options for launching child processes,
 * such as where its standard input and output will be directed, the
 * argument list, the environment, and more.
 *
 * While the #xsubprocess_t class has high level functions covering
 * popular cases, use of this class allows access to more advanced
 * options.  It can also be used to launch multiple subprocesses with
 * a similar configuration.
 *
 * Since: 2.40
 */

#define ALL_STDIN_FLAGS         (G_SUBPROCESS_FLAGS_STDIN_PIPE |        \
                                 G_SUBPROCESS_FLAGS_STDIN_INHERIT)
#define ALL_STDOUT_FLAGS        (G_SUBPROCESS_FLAGS_STDOUT_PIPE |       \
                                 G_SUBPROCESS_FLAGS_STDOUT_SILENCE)
#define ALL_STDERR_FLAGS        (G_SUBPROCESS_FLAGS_STDERR_PIPE |       \
                                 G_SUBPROCESS_FLAGS_STDERR_SILENCE |    \
                                 G_SUBPROCESS_FLAGS_STDERR_MERGE)

#include "config.h"

#include "gsubprocesslauncher-private.h"
#include "gioenumtypes.h"
#include "gsubprocess.h"
#include "ginitable.h"
#include "gioerror.h"

#ifdef G_OS_UNIX
#include <unistd.h>
#include <fcntl.h>
#endif

typedef xobject_class_t GSubprocessLauncherClass;

G_DEFINE_TYPE (xsubprocess_launcher, xsubprocess_launcher, XTYPE_OBJECT)

static xboolean_t
verify_disposition (const xchar_t      *stream_name,
                    xsubprocess_flags_t  filtered_flags,
                    xint_t              fd,
                    const xchar_t      *filename)
{
  xuint_t n_bits;

  if (!filtered_flags)
    n_bits = 0;
  else if (((filtered_flags - 1) & filtered_flags) == 0)
    n_bits = 1;
  else
    n_bits = 2; /* ...or more */

  if (n_bits + (fd >= 0) + (filename != NULL) > 1)
    {
      xstring_t *err;

      err = xstring_new (NULL);
      if (n_bits)
        {
          xflags_class_t *class;
          xuint_t i;

          class = xtype_class_peek (XTYPE_SUBPROCESS_FLAGS);

          for (i = 0; i < class->n_values; i++)
            {
              const xflags_value_t *value = &class->values[i];

              if (filtered_flags & value->value)
                xstring_append_printf (err, " %s", value->value_name);
            }

          xtype_class_unref (class);
        }

      if (fd >= 0)
        xstring_append_printf (err, " xsubprocess_launcher_take_%s_fd()", stream_name);

      if (filename)
        xstring_append_printf (err, " xsubprocess_launcher_set_%s_file_path()", stream_name);

      g_critical ("You may specify at most one disposition for the %s stream, but you specified:%s.",
                  stream_name, err->str);
      xstring_free (err, TRUE);

      return FALSE;
    }

  return TRUE;
}

static xboolean_t
verify_flags (xsubprocess_flags_t flags)
{
  return verify_disposition ("stdin", flags & ALL_STDIN_FLAGS, -1, NULL) &&
         verify_disposition ("stdout", flags & ALL_STDOUT_FLAGS, -1, NULL) &&
         verify_disposition ("stderr", flags & ALL_STDERR_FLAGS, -1, NULL);
}

static void
xsubprocess_launcher_set_property (xobject_t *object, xuint_t prop_id,
                                    const xvalue_t *value, xparam_spec_t *pspec)
{
  xsubprocess_launcher_t *launcher = G_SUBPROCESS_LAUNCHER (object);

  g_assert (prop_id == 1);

  if (verify_flags (xvalue_get_flags (value)))
    launcher->flags = xvalue_get_flags (value);
}

static void
xsubprocess_launcher_dispose (xobject_t *object)
{
  xsubprocess_launcher_t *self = G_SUBPROCESS_LAUNCHER (object);

#ifdef G_OS_UNIX
  g_clear_pointer (&self->stdin_path, g_free);
  g_clear_pointer (&self->stdout_path, g_free);
  g_clear_pointer (&self->stderr_path, g_free);

  xsubprocess_launcher_close (self);

  if (self->child_setup_destroy_notify)
    (* self->child_setup_destroy_notify) (self->child_setup_user_data);
  self->child_setup_destroy_notify = NULL;
  self->child_setup_user_data = NULL;
#endif

  g_clear_pointer (&self->envp, xstrfreev);
  g_clear_pointer (&self->cwd, g_free);

  G_OBJECT_CLASS (xsubprocess_launcher_parent_class)->dispose (object);
}

static void
xsubprocess_launcher_init (xsubprocess_launcher_t  *self)
{
  self->envp = g_get_environ ();

#ifdef G_OS_UNIX
  self->stdin_fd = -1;
  self->stdout_fd = -1;
  self->stderr_fd = -1;
  self->source_fds = g_array_new (FALSE, 0, sizeof (int));
  self->target_fds = g_array_new (FALSE, 0, sizeof (int));
#endif
}

static void
xsubprocess_launcher_class_init (GSubprocessLauncherClass *class)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->set_property = xsubprocess_launcher_set_property;
  gobject_class->dispose = xsubprocess_launcher_dispose;

  xobject_class_install_property (gobject_class, 1,
                                   g_param_spec_flags ("flags", "Flags", "xsubprocess_flags_t for launched processes",
                                                       XTYPE_SUBPROCESS_FLAGS, 0, G_PARAM_WRITABLE |
                                                       G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY));
}

/**
 * xsubprocess_launcher_new:
 * @flags: #xsubprocess_flags_t
 *
 * Creates a new #xsubprocess_launcher_t.
 *
 * The launcher is created with the default options.  A copy of the
 * environment of the calling process is made at the time of this call
 * and will be used as the environment that the process is launched in.
 *
 * Since: 2.40
 **/
xsubprocess_launcher_t *
xsubprocess_launcher_new (xsubprocess_flags_t flags)
{
  if (!verify_flags (flags))
    return NULL;

  return xobject_new (XTYPE_SUBPROCESS_LAUNCHER,
                       "flags", flags,
                       NULL);
}

/**
 * xsubprocess_launcher_set_environ:
 * @self: a #xsubprocess_launcher_t
 * @env: (array zero-terminated=1) (element-type filename) (transfer none):
 *     the replacement environment
 *
 * Replace the entire environment of processes launched from this
 * launcher with the given 'environ' variable.
 *
 * Typically you will build this variable by using g_listenv() to copy
 * the process 'environ' and using the functions g_environ_setenv(),
 * g_environ_unsetenv(), etc.
 *
 * As an alternative, you can use xsubprocess_launcher_setenv(),
 * xsubprocess_launcher_unsetenv(), etc.
 *
 * Pass an empty array to set an empty environment. Pass %NULL to inherit the
 * parent process’ environment. As of GLib 2.54, the parent process’ environment
 * will be copied when xsubprocess_launcher_set_environ() is called.
 * Previously, it was copied when the subprocess was executed. This means the
 * copied environment may now be modified (using xsubprocess_launcher_setenv(),
 * etc.) before launching the subprocess.
 *
 * On UNIX, all strings in this array can be arbitrary byte strings.
 * On Windows, they should be in UTF-8.
 *
 * Since: 2.40
 **/
void
xsubprocess_launcher_set_environ (xsubprocess_launcher_t  *self,
                                   xchar_t               **env)
{
  xstrfreev (self->envp);
  self->envp = xstrdupv (env);

  if (self->envp == NULL)
    self->envp = g_get_environ ();
}

/**
 * xsubprocess_launcher_setenv:
 * @self: a #xsubprocess_launcher_t
 * @variable: (type filename): the environment variable to set,
 *     must not contain '='
 * @value: (type filename): the new value for the variable
 * @overwrite: whether to change the variable if it already exists
 *
 * Sets the environment variable @variable in the environment of
 * processes launched from this launcher.
 *
 * On UNIX, both the variable's name and value can be arbitrary byte
 * strings, except that the variable's name cannot contain '='.
 * On Windows, they should be in UTF-8.
 *
 * Since: 2.40
 **/
void
xsubprocess_launcher_setenv (xsubprocess_launcher_t *self,
                              const xchar_t         *variable,
                              const xchar_t         *value,
                              xboolean_t             overwrite)
{
  self->envp = g_environ_setenv (self->envp, variable, value, overwrite);
}

/**
 * xsubprocess_launcher_unsetenv:
 * @self: a #xsubprocess_launcher_t
 * @variable: (type filename): the environment variable to unset,
 *     must not contain '='
 *
 * Removes the environment variable @variable from the environment of
 * processes launched from this launcher.
 *
 * On UNIX, the variable's name can be an arbitrary byte string not
 * containing '='. On Windows, it should be in UTF-8.
 *
 * Since: 2.40
 **/
void
xsubprocess_launcher_unsetenv (xsubprocess_launcher_t *self,
                                const xchar_t         *variable)
{
  self->envp = g_environ_unsetenv (self->envp, variable);
}

/**
 * xsubprocess_launcher_getenv:
 * @self: a #xsubprocess_launcher_t
 * @variable: (type filename): the environment variable to get
 *
 * Returns the value of the environment variable @variable in the
 * environment of processes launched from this launcher.
 *
 * On UNIX, the returned string can be an arbitrary byte string.
 * On Windows, it will be UTF-8.
 *
 * Returns: (nullable) (type filename): the value of the environment variable,
 *     %NULL if unset
 *
 * Since: 2.40
 **/
const xchar_t *
xsubprocess_launcher_getenv (xsubprocess_launcher_t *self,
                              const xchar_t         *variable)
{
  return g_environ_getenv (self->envp, variable);
}

/**
 * xsubprocess_launcher_set_cwd:
 * @self: a #xsubprocess_launcher_t
 * @cwd: (type filename): the cwd for launched processes
 *
 * Sets the current working directory that processes will be launched
 * with.
 *
 * By default processes are launched with the current working directory
 * of the launching process at the time of launch.
 *
 * Since: 2.40
 **/
void
xsubprocess_launcher_set_cwd (xsubprocess_launcher_t *self,
                               const xchar_t         *cwd)
{
  g_free (self->cwd);
  self->cwd = xstrdup (cwd);
}

/**
 * xsubprocess_launcher_set_flags:
 * @self: a #xsubprocess_launcher_t
 * @flags: #xsubprocess_flags_t
 *
 * Sets the flags on the launcher.
 *
 * The default flags are %G_SUBPROCESS_FLAGS_NONE.
 *
 * You may not set flags that specify conflicting options for how to
 * handle a particular stdio stream (eg: specifying both
 * %G_SUBPROCESS_FLAGS_STDIN_PIPE and
 * %G_SUBPROCESS_FLAGS_STDIN_INHERIT).
 *
 * You may also not set a flag that conflicts with a previous call to a
 * function like xsubprocess_launcher_set_stdin_file_path() or
 * xsubprocess_launcher_take_stdout_fd().
 *
 * Since: 2.40
 **/
void
xsubprocess_launcher_set_flags (xsubprocess_launcher_t *self,
                                 xsubprocess_flags_t     flags)
{
  const xchar_t *stdin_path = NULL, *stdout_path = NULL, *stderr_path = NULL;
  xint_t stdin_fd = -1, stdout_fd = -1, stderr_fd = -1;

#ifdef G_OS_UNIX
  stdin_fd = self->stdin_fd;
  stdout_fd = self->stdout_fd;
  stderr_fd = self->stderr_fd;
  stdin_path = self->stdin_path;
  stdout_path = self->stdout_path;
  stderr_path = self->stderr_path;
#endif

  if (verify_disposition ("stdin", flags & ALL_STDIN_FLAGS, stdin_fd, stdin_path) &&
      verify_disposition ("stdout", flags & ALL_STDOUT_FLAGS, stdout_fd, stdout_path) &&
      verify_disposition ("stderr", flags & ALL_STDERR_FLAGS, stderr_fd, stderr_path))
    self->flags = flags;
}

#ifdef G_OS_UNIX
static void
assign_fd (xint_t *fd_ptr, xint_t fd)
{
  xint_t flags;

  if (*fd_ptr != -1)
    close (*fd_ptr);

  *fd_ptr = fd;

  if (fd != -1)
    {
      /* best effort */
      flags = fcntl (fd, F_GETFD);
      if (~flags & FD_CLOEXEC)
        fcntl (fd, F_SETFD, flags | FD_CLOEXEC);
    }
}

/**
 * xsubprocess_launcher_set_stdin_file_path:
 * @self: a #xsubprocess_launcher_t
 * @path: (type filename) (nullable: a filename or %NULL
 *
 * Sets the file path to use as the stdin for spawned processes.
 *
 * If @path is %NULL then any previously given path is unset.
 *
 * The file must exist or spawning the process will fail.
 *
 * You may not set a stdin file path if a stdin fd is already set or if
 * the launcher flags contain any flags directing stdin elsewhere.
 *
 * This feature is only available on UNIX.
 *
 * Since: 2.40
 **/
void
xsubprocess_launcher_set_stdin_file_path (xsubprocess_launcher_t *self,
                                           const xchar_t         *path)
{
  if (verify_disposition ("stdin", self->flags & ALL_STDIN_FLAGS, self->stdin_fd, path))
    {
      g_free (self->stdin_path);
      self->stdin_path = xstrdup (path);
    }
}

/**
 * xsubprocess_launcher_take_stdin_fd:
 * @self: a #xsubprocess_launcher_t
 * @fd: a file descriptor, or -1
 *
 * Sets the file descriptor to use as the stdin for spawned processes.
 *
 * If @fd is -1 then any previously given fd is unset.
 *
 * Note that if your intention is to have the stdin of the calling
 * process inherited by the child then %G_SUBPROCESS_FLAGS_STDIN_INHERIT
 * is a better way to go about doing that.
 *
 * The passed @fd is noted but will not be touched in the current
 * process.  It is therefore necessary that it be kept open by the
 * caller until the subprocess is spawned.  The file descriptor will
 * also not be explicitly closed on the child side, so it must be marked
 * O_CLOEXEC if that's what you want.
 *
 * You may not set a stdin fd if a stdin file path is already set or if
 * the launcher flags contain any flags directing stdin elsewhere.
 *
 * This feature is only available on UNIX.
 *
 * Since: 2.40
 **/
void
xsubprocess_launcher_take_stdin_fd (xsubprocess_launcher_t *self,
                                     xint_t                 fd)
{
  if (verify_disposition ("stdin", self->flags & ALL_STDIN_FLAGS, fd, self->stdin_path))
    assign_fd (&self->stdin_fd, fd);
}

/**
 * xsubprocess_launcher_set_stdout_file_path:
 * @self: a #xsubprocess_launcher_t
 * @path: (type filename) (nullable): a filename or %NULL
 *
 * Sets the file path to use as the stdout for spawned processes.
 *
 * If @path is %NULL then any previously given path is unset.
 *
 * The file will be created or truncated when the process is spawned, as
 * would be the case if using '>' at the shell.
 *
 * You may not set a stdout file path if a stdout fd is already set or
 * if the launcher flags contain any flags directing stdout elsewhere.
 *
 * This feature is only available on UNIX.
 *
 * Since: 2.40
 **/
void
xsubprocess_launcher_set_stdout_file_path (xsubprocess_launcher_t *self,
                                            const xchar_t         *path)
{
  if (verify_disposition ("stdout", self->flags & ALL_STDOUT_FLAGS, self->stdout_fd, path))
    {
      g_free (self->stdout_path);
      self->stdout_path = xstrdup (path);
    }
}

/**
 * xsubprocess_launcher_take_stdout_fd:
 * @self: a #xsubprocess_launcher_t
 * @fd: a file descriptor, or -1
 *
 * Sets the file descriptor to use as the stdout for spawned processes.
 *
 * If @fd is -1 then any previously given fd is unset.
 *
 * Note that the default behaviour is to pass stdout through to the
 * stdout of the parent process.
 *
 * The passed @fd is noted but will not be touched in the current
 * process.  It is therefore necessary that it be kept open by the
 * caller until the subprocess is spawned.  The file descriptor will
 * also not be explicitly closed on the child side, so it must be marked
 * O_CLOEXEC if that's what you want.
 *
 * You may not set a stdout fd if a stdout file path is already set or
 * if the launcher flags contain any flags directing stdout elsewhere.
 *
 * This feature is only available on UNIX.
 *
 * Since: 2.40
 **/
void
xsubprocess_launcher_take_stdout_fd (xsubprocess_launcher_t *self,
                                      xint_t                 fd)
{
  if (verify_disposition ("stdout", self->flags & ALL_STDOUT_FLAGS, fd, self->stdout_path))
    assign_fd (&self->stdout_fd, fd);
}

/**
 * xsubprocess_launcher_set_stderr_file_path:
 * @self: a #xsubprocess_launcher_t
 * @path: (type filename) (nullable): a filename or %NULL
 *
 * Sets the file path to use as the stderr for spawned processes.
 *
 * If @path is %NULL then any previously given path is unset.
 *
 * The file will be created or truncated when the process is spawned, as
 * would be the case if using '2>' at the shell.
 *
 * If you want to send both stdout and stderr to the same file then use
 * %G_SUBPROCESS_FLAGS_STDERR_MERGE.
 *
 * You may not set a stderr file path if a stderr fd is already set or
 * if the launcher flags contain any flags directing stderr elsewhere.
 *
 * This feature is only available on UNIX.
 *
 * Since: 2.40
 **/
void
xsubprocess_launcher_set_stderr_file_path (xsubprocess_launcher_t *self,
                                            const xchar_t         *path)
{
  if (verify_disposition ("stderr", self->flags & ALL_STDERR_FLAGS, self->stderr_fd, path))
    {
      g_free (self->stderr_path);
      self->stderr_path = xstrdup (path);
    }
}

/**
 * xsubprocess_launcher_take_stderr_fd:
 * @self: a #xsubprocess_launcher_t
 * @fd: a file descriptor, or -1
 *
 * Sets the file descriptor to use as the stderr for spawned processes.
 *
 * If @fd is -1 then any previously given fd is unset.
 *
 * Note that the default behaviour is to pass stderr through to the
 * stderr of the parent process.
 *
 * The passed @fd belongs to the #xsubprocess_launcher_t.  It will be
 * automatically closed when the launcher is finalized.  The file
 * descriptor will also be closed on the child side when executing the
 * spawned process.
 *
 * You may not set a stderr fd if a stderr file path is already set or
 * if the launcher flags contain any flags directing stderr elsewhere.
 *
 * This feature is only available on UNIX.
 *
 * Since: 2.40
 **/
void
xsubprocess_launcher_take_stderr_fd (xsubprocess_launcher_t *self,
                                     xint_t                 fd)
{
  if (verify_disposition ("stderr", self->flags & ALL_STDERR_FLAGS, fd, self->stderr_path))
    assign_fd (&self->stderr_fd, fd);
}

/**
 * xsubprocess_launcher_take_fd:
 * @self: a #xsubprocess_launcher_t
 * @source_fd: File descriptor in parent process
 * @target_fd: Target descriptor for child process
 *
 * Transfer an arbitrary file descriptor from parent process to the
 * child.  This function takes ownership of the @source_fd; it will be closed
 * in the parent when @self is freed.
 *
 * By default, all file descriptors from the parent will be closed.
 * This function allows you to create (for example) a custom `pipe()` or
 * `socketpair()` before launching the process, and choose the target
 * descriptor in the child.
 *
 * An example use case is GNUPG, which has a command line argument
 * `--passphrase-fd` providing a file descriptor number where it expects
 * the passphrase to be written.
 */
void
xsubprocess_launcher_take_fd (xsubprocess_launcher_t   *self,
                               xint_t                   source_fd,
                               xint_t                   target_fd)
{
  if (self->source_fds != NULL && self->target_fds != NULL)
    {
      g_array_append_val (self->source_fds, source_fd);
      g_array_append_val (self->target_fds, target_fd);
    }
}

/**
 * xsubprocess_launcher_close:
 * @self: a #xsubprocess_launcher_t
 *
 * Closes all the file descriptors previously passed to the object with
 * xsubprocess_launcher_take_fd(), xsubprocess_launcher_take_stderr_fd(), etc.
 *
 * After calling this method, any subsequent calls to xsubprocess_launcher_spawn() or xsubprocess_launcher_spawnv() will
 * return %G_IO_ERROR_CLOSED. This method is idempotent if
 * called more than once.
 *
 * This function is called automatically when the #xsubprocess_launcher_t
 * is disposed, but is provided separately so that garbage collected
 * language bindings can call it earlier to guarantee when FDs are closed.
 *
 * Since: 2.68
 */
void
xsubprocess_launcher_close (xsubprocess_launcher_t *self)
{
  xuint_t i;

  g_return_if_fail (X_IS_SUBPROCESS_LAUNCHER (self));

  if (self->stdin_fd != -1)
    close (self->stdin_fd);
  self->stdin_fd = -1;

  if (self->stdout_fd != -1)
    close (self->stdout_fd);
  self->stdout_fd = -1;

  if (self->stderr_fd != -1)
    close (self->stderr_fd);
  self->stderr_fd = -1;

  if (self->source_fds)
    {
      g_assert (self->target_fds != NULL);
      g_assert (self->source_fds->len == self->target_fds->len);

      /* Note: Don’t close the target_fds, as they’re only valid FDs in the
       * child process. This code never executes in the child process. */
      for (i = 0; i < self->source_fds->len; i++)
        (void) close (g_array_index (self->source_fds, int, i));

      g_clear_pointer (&self->source_fds, g_array_unref);
      g_clear_pointer (&self->target_fds, g_array_unref);
    }

  self->closed_fd = TRUE;
}

/**
 * xsubprocess_launcher_set_child_setup: (skip)
 * @self: a #xsubprocess_launcher_t
 * @child_setup: a #GSpawnChildSetupFunc to use as the child setup function
 * @user_data: user data for @child_setup
 * @destroy_notify: a #xdestroy_notify_t for @user_data
 *
 * Sets up a child setup function.
 *
 * The child setup function will be called after fork() but before
 * exec() on the child's side.
 *
 * @destroy_notify will not be automatically called on the child's side
 * of the fork().  It will only be called when the last reference on the
 * #xsubprocess_launcher_t is dropped or when a new child setup function is
 * given.
 *
 * %NULL can be given as @child_setup to disable the functionality.
 *
 * Child setup functions are only available on UNIX.
 *
 * Since: 2.40
 **/
void
xsubprocess_launcher_set_child_setup (xsubprocess_launcher_t  *self,
                                       GSpawnChildSetupFunc  child_setup,
                                       xpointer_t              user_data,
                                       xdestroy_notify_t        destroy_notify)
{
  if (self->child_setup_destroy_notify)
    (* self->child_setup_destroy_notify) (self->child_setup_user_data);

  self->child_setup_func = child_setup;
  self->child_setup_user_data = user_data;
  self->child_setup_destroy_notify = destroy_notify;
}
#endif

/**
 * xsubprocess_launcher_spawn:
 * @self: a #xsubprocess_launcher_t
 * @error: Error
 * @argv0: Command line arguments
 * @...: Continued arguments, %NULL terminated
 *
 * Creates a #xsubprocess_t given a provided varargs list of arguments.
 *
 * Since: 2.40
 * Returns: (transfer full): A new #xsubprocess_t, or %NULL on error (and @error will be set)
 **/
xsubprocess_t *
xsubprocess_launcher_spawn (xsubprocess_launcher_t  *launcher,
                             xerror_t              **error,
                             const xchar_t          *argv0,
                             ...)
{
  xsubprocess_t *result;
  xptr_array_t *args;
  const xchar_t *arg;
  va_list ap;

  g_return_val_if_fail (argv0 != NULL && argv0[0] != '\0', NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  args = xptr_array_new ();

  va_start (ap, argv0);
  xptr_array_add (args, (xchar_t *) argv0);
  while ((arg = va_arg (ap, const xchar_t *)))
    xptr_array_add (args, (xchar_t *) arg);

  xptr_array_add (args, NULL);
  va_end (ap);

  result = xsubprocess_launcher_spawnv (launcher, (const xchar_t * const *) args->pdata, error);

  xptr_array_free (args, TRUE);

  return result;

}

/**
 * xsubprocess_launcher_spawnv:
 * @self: a #xsubprocess_launcher_t
 * @argv: (array zero-terminated=1) (element-type filename): Command line arguments
 * @error: Error
 *
 * Creates a #xsubprocess_t given a provided array of arguments.
 *
 * Since: 2.40
 * Returns: (transfer full): A new #xsubprocess_t, or %NULL on error (and @error will be set)
 **/
xsubprocess_t *
xsubprocess_launcher_spawnv (xsubprocess_launcher_t  *launcher,
                              const xchar_t * const  *argv,
                              xerror_t              **error)
{
  xsubprocess_t *subprocess;

  g_return_val_if_fail (argv != NULL && argv[0] != NULL && argv[0][0] != '\0', NULL);

#ifdef G_OS_UNIX
  if (launcher->closed_fd)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_CLOSED,
                   "Can't spawn a new child because a passed file descriptor has been closed.");
      return NULL;
    }
#endif

  subprocess = xobject_new (XTYPE_SUBPROCESS,
                             "argv", argv,
                             "flags", launcher->flags,
                             NULL);
  xsubprocess_set_launcher (subprocess, launcher);

  if (!xinitable_init (XINITABLE (subprocess), NULL, error))
    {
      xobject_unref (subprocess);
      return NULL;
    }

  return subprocess;
}
