/*
 * Copyright 2020 Xavier Claessens
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

#include <glib.h>

typedef struct
{
  int dummy;
} xobject_t;

static void
test_typeof (void)
{
#if __cplusplus >= 201103L
  // test_t that with C++11 we don't get those kind of errors:
  // error: invalid conversion from ‘xpointer_t’ {aka ‘void*’} to ‘xobject_t*’ [-fpermissive]
  xobject_t *obj = g_rc_box_new0 (xobject_t);
  xobject_t *obj2 = g_rc_box_acquire (obj);
  g_assert_true (obj2 == obj);

  xobject_t *obj3 = g_atomic_pointer_get (&obj2);
  g_assert_true (obj3 == obj);

  xobject_t *obj4 = nullptr;
  g_atomic_pointer_set (&obj4, obj3);
  g_assert_true (obj4 == obj);

  xobject_t *obj5 = nullptr;
  g_atomic_pointer_compare_and_exchange (&obj5, nullptr, obj4);
  g_assert_true (obj5 == obj);

  xobject_t *obj6 = g_steal_pointer (&obj5);
  g_assert_true (obj6 == obj);

  g_clear_pointer (&obj6, g_rc_box_release);
  g_rc_box_release (obj);
#else
  g_test_skip ("This test requires C++11 compiler");
#endif
}

int
main (int argc, char *argv[])
{
#if __cplusplus >= 201103L
  g_test_init (&argc, &argv, nullptr);
#else
  g_test_init (&argc, &argv, NULL);
#endif

  g_test_add_func ("/C++/typeof", test_typeof);

  return g_test_run ();
}
