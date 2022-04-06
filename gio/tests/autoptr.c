#include <gio/gio.h>

static void
test_autoptr (void)
{
  x_autoptr(xfile) p = xfile_new_for_path ("/blah");
  x_autoptr(xinet_address) a = xinet_address_new_from_string ("127.0.0.1");
  g_autofree xchar_t *path = xfile_get_path (p);
  g_autofree xchar_t *istr = xinet_address_to_string (a);

  g_assert_cmpstr (path, ==, G_DIR_SEPARATOR_S "blah");
  g_assert_cmpstr (istr, ==, "127.0.0.1");
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/autoptr/autoptr", test_autoptr);

  return g_test_run ();
}
