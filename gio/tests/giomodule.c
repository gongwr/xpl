/* Unit tests for xio_module_t
 * Copyright (C) 2013 Red Hat, Inc
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
#include <glibconfig.h>

#ifdef _MSC_VER
# define MODULE_FILENAME_PREFIX ""
#else
# define MODULE_FILENAME_PREFIX "lib"
#endif

static void
test_extension_point (void)
{
  xio_extension_point_t *ep, *ep2;
  xio_extension_t *ext;
  xlist_t *list;
  xtype_t req;
  xtype_class_t *class;

  ep = g_io_extension_point_lookup ("test-extension-point");
  g_assert_null (ep);
  ep = g_io_extension_point_register ("test-extension-point");
  ep2 = g_io_extension_point_lookup ("test-extension-point");
  xassert (ep2 == ep);

  req = g_io_extension_point_get_required_type (ep);
  xassert (req == XTYPE_INVALID);
  g_io_extension_point_set_required_type (ep, XTYPE_OBJECT);
  req = g_io_extension_point_get_required_type (ep);
  xassert (req == XTYPE_OBJECT);

  list = g_io_extension_point_get_extensions (ep);
  g_assert_null (list);

  g_io_extension_point_implement ("test-extension-point",
                                  XTYPE_VFS,
                                  "extension1",
                                  10);

  g_io_extension_point_implement ("test-extension-point",
                                  XTYPE_OBJECT,
                                  "extension2",
                                  20);

  list = g_io_extension_point_get_extensions (ep);
  g_assert_cmpint (xlist_length (list), ==, 2);

  ext = list->data;
  g_assert_cmpstr (g_io_extension_get_name (ext), ==, "extension2");
  xassert (g_io_extension_get_type (ext) == XTYPE_OBJECT);
  xassert (g_io_extension_get_priority (ext) == 20);
  class = g_io_extension_ref_class (ext);
  xassert (class == xtype_class_peek (XTYPE_OBJECT));
  xtype_class_unref (class);

  ext = list->next->data;
  g_assert_cmpstr (g_io_extension_get_name (ext), ==, "extension1");
  xassert (g_io_extension_get_type (ext) == XTYPE_VFS);
  xassert (g_io_extension_get_priority (ext) == 10);
}

static void
test_module_scan_all (void)
{
#ifdef XPL_STATIC_COMPILATION
  /* The plugin module is statically linked with a separate copy
   * of GLib so g_io_extension_point_implement won't work. */
  g_test_skip ("xio_extension_point_t with dynamic modules isn't supported in static builds.");
  return;
#endif

  if (g_test_subprocess ())
    {
      xio_extension_point_t *ep;
      xio_extension_t *ext;
      xlist_t *list;
      ep = g_io_extension_point_register ("test-extension-point");
      g_io_modules_scan_all_in_directory (g_test_get_filename (G_TEST_BUILT, "modules", NULL));
      list = g_io_extension_point_get_extensions (ep);
      g_assert_cmpint (xlist_length (list), ==, 2);
      ext = list->data;
      g_assert_cmpstr (g_io_extension_get_name (ext), ==, "test-b");
      ext = list->next->data;
      g_assert_cmpstr (g_io_extension_get_name (ext), ==, "test-a");
      return;
    }
  g_test_trap_subprocess (NULL, 0, 7);
  g_test_trap_assert_passed ();
}

static void
test_module_scan_all_with_scope (void)
{
#ifdef XPL_STATIC_COMPILATION
  /* Disabled for the same reason as test_module_scan_all. */
  g_test_skip ("xio_extension_point_t with dynamic modules isn't supported in static builds.");
  return;
#endif

  if (g_test_subprocess ())
    {
      xio_extension_point_t *ep;
      GIOModuleScope *scope;
      xio_extension_t *ext;
      xlist_t *list;

      ep = g_io_extension_point_register ("test-extension-point");
      scope = xio_module_scope_new (G_IO_MODULE_SCOPE_BLOCK_DUPLICATES);
      xio_module_scope_block (scope, MODULE_FILENAME_PREFIX "testmoduleb." G_MODULE_SUFFIX);
      g_io_modules_scan_all_in_directory_with_scope (g_test_get_filename (G_TEST_BUILT, "modules", NULL), scope);
      list = g_io_extension_point_get_extensions (ep);
      g_assert_cmpint (xlist_length (list), ==, 1);
      ext = list->data;
      g_assert_cmpstr (g_io_extension_get_name (ext), ==, "test-a");
      xio_module_scope_free (scope);
      return;
    }
  g_test_trap_subprocess (NULL, 0, 7);
  g_test_trap_assert_passed ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/giomodule/extension-point", test_extension_point);
  g_test_add_func ("/giomodule/module-scan-all", test_module_scan_all);
  g_test_add_func ("/giomodule/module-scan-all-with-scope", test_module_scan_all_with_scope);

  return g_test_run ();
}
