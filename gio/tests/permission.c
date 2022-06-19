/* Unit tests for xpermission_t
 * Copyright (C) 2012 Red Hat, Inc
 * Author: Matthias Clasen
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <gio/gio.h>

static void
acquired (xobject_t      *source,
          xasync_result_t *res,
          xpointer_t      user_data)
{
  xpermission_t *p = G_PERMISSION (source);
  xmain_loop_t *loop = user_data;
  xerror_t *error = NULL;
  xboolean_t ret;

  ret = g_permission_acquire_finish (p, res, &error);
  xassert (!ret);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);

  xmain_loop_quit (loop);
}

static void
released (xobject_t      *source,
          xasync_result_t *res,
          xpointer_t      user_data)
{
  xpermission_t *p = G_PERMISSION (source);
  xmain_loop_t *loop = user_data;
  xerror_t *error = NULL;
  xboolean_t ret;

  ret = g_permission_release_finish (p, res, &error);
  xassert (!ret);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);

  xmain_loop_quit (loop);
}

static void
test_simple (void)
{
  xpermission_t *p;
  xboolean_t allowed;
  xboolean_t can_acquire;
  xboolean_t can_release;
  xboolean_t ret;
  xerror_t *error = NULL;
  xmain_loop_t *loop;

  p = g_simple_permission_new (TRUE);

  xassert (g_permission_get_allowed (p));
  xassert (!g_permission_get_can_acquire (p));
  xassert (!g_permission_get_can_release (p));

  xobject_get (p,
                "allowed", &allowed,
                "can-acquire", &can_acquire,
                "can-release", &can_release,
                NULL);

  xassert (allowed);
  xassert (!can_acquire);
  xassert (!can_release);

  ret = g_permission_acquire (p, NULL, &error);
  xassert (!ret);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);

  ret = g_permission_release (p, NULL, &error);
  xassert (!ret);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);

  loop = xmain_loop_new (NULL, FALSE);
  g_permission_acquire_async (p, NULL, acquired, loop);
  xmain_loop_run (loop);
  g_permission_release_async (p, NULL, released, loop);
  xmain_loop_run (loop);

  xmain_loop_unref (loop);

  xobject_unref (p);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/permission/simple", test_simple);

  return g_test_run ();
}
