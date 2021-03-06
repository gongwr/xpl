/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2011 Collabora, Ltd.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Stef Walter <stefw@collabora.co.uk>
 */

#include "config.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <string.h>

#ifdef G_OS_WIN32
#include <conio.h>
#endif

#include "gtlsconsoleinteraction.h"

/*
 * WARNING: This is not the example you're looking for [slow hand wave]. This
 * is not industrial strength, it's just for testing. It uses embarrassing
 * functions like getpass() and does lazy things with threads.
 */

XDEFINE_TYPE (GTlsConsoleInteraction, xtls_console_interaction, XTYPE_TLS_INTERACTION)

#if defined(G_OS_WIN32) || defined(__BIONIC__)
/* win32 doesn't have getpass() */
#include <stdio.h>
#ifndef BUFSIZ
#define BUFSIZ 8192
#endif
static xchar_t *
getpass (const xchar_t *prompt)
{
  static xchar_t buf[BUFSIZ];
  xint_t i;

  g_printf ("%s", prompt);
  fflush (stdout);

  for (i = 0; i < BUFSIZ - 1; ++i)
    {
#ifdef __BIONIC__
      buf[i] = getc (stdin);
#else
      buf[i] = _getch ();
#endif
      if (buf[i] == '\r')
        break;
    }
  buf[i] = '\0';

  g_printf ("\n");

  return &buf[0];
}
#endif

static GTlsInteractionResult
xtls_console_interaction_ask_password (xtls_interaction_t    *interaction,
                                        xtls_password_t       *password,
                                        xcancellable_t       *cancellable,
                                        xerror_t            **error)
{
  const xchar_t *value;
  xchar_t *prompt;

  prompt = xstrdup_printf ("Password \"%s\"': ", xtls_password_get_description (password));
  value = getpass (prompt);
  g_free (prompt);

  if (xcancellable_set_error_if_cancelled (cancellable, error))
    return G_TLS_INTERACTION_FAILED;

  xtls_password_set_value (password, (xuchar_t *)value, -1);
  return G_TLS_INTERACTION_HANDLED;
}

static void
ask_password_with_getpass (xtask_t        *task,
                           xpointer_t      object,
                           xpointer_t      task_data,
                           xcancellable_t *cancellable)
{
  xtls_password_t *password = task_data;
  xerror_t *error = NULL;

  xtls_console_interaction_ask_password (G_TLS_INTERACTION (object), password,
                                          cancellable, &error);
  if (error != NULL)
    xtask_return_error (task, error);
  else
    xtask_return_int (task, G_TLS_INTERACTION_HANDLED);
}

static void
xtls_console_interaction_ask_password_async (xtls_interaction_t    *interaction,
                                              xtls_password_t       *password,
                                              xcancellable_t       *cancellable,
                                              xasync_ready_callback_t callback,
                                              xpointer_t            user_data)
{
  xtask_t *task;

  task = xtask_new (interaction, cancellable, callback, user_data);
  xtask_set_task_data (task, xobject_ref (password), xobject_unref);
  xtask_run_in_thread (task, ask_password_with_getpass);
  xobject_unref (task);
}

static GTlsInteractionResult
xtls_console_interaction_ask_password_finish (xtls_interaction_t    *interaction,
                                               xasync_result_t       *result,
                                               xerror_t            **error)
{
  GTlsInteractionResult ret;

  xreturn_val_if_fail (xtask_is_valid (result, interaction),
                        G_TLS_INTERACTION_FAILED);

  ret = xtask_propagate_int (XTASK (result), error);
  if (ret == (GTlsInteractionResult)-1)
    return G_TLS_INTERACTION_FAILED;
  else
    return ret;
}

static void
xtls_console_interaction_init (GTlsConsoleInteraction *interaction)
{

}

static void
xtls_console_interaction_class_init (GTlsConsoleInteractionClass *klass)
{
  GTlsInteractionClass *interaction_class = G_TLS_INTERACTION_CLASS (klass);
  interaction_class->ask_password = xtls_console_interaction_ask_password;
  interaction_class->ask_password_async = xtls_console_interaction_ask_password_async;
  interaction_class->ask_password_finish = xtls_console_interaction_ask_password_finish;
}

xtls_interaction_t *
xtls_console_interaction_new (void)
{
  return xobject_new (XTYPE_TLS_CONSOLE_INTERACTION, NULL);
}
