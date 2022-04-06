
#include "gdbus-object-manager-example/objectmanager-gen.h"

/* ---------------------------------------------------------------------------------------------------- */

/* The fixture contains a xtest_dbus_t object and
 * a proxy to the service we're going to be testing.
 */
typedef struct {
  xtest_dbus_t *dbus;
  xdbus_object_manager_t *manager;
} TestFixture;

static void
fixture_setup (TestFixture *fixture, xconstpointer unused)
{
  xerror_t *error = NULL;
  xchar_t *relative, *servicesdir;

  /* Create the global dbus-daemon for this test suite
   */
  fixture->dbus = g_test_dbus_new (G_TEST_DBUS_NONE);

  /* Add the private directory with our in-tree service files.
   */
  relative = g_test_build_filename (G_TEST_BUILT, "services", NULL);
  servicesdir = g_canonicalize_filename (relative, NULL);
  g_free (relative);

  g_test_dbus_add_service_dir (fixture->dbus, servicesdir);
  g_free (servicesdir);

  /* Start the private D-Bus daemon
   */
  g_test_dbus_up (fixture->dbus);

  /* Create the proxy that we're going to test
   */
  fixture->manager =
    example_object_manager_client_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                    G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                    "org.gtk.GDBus.Examples.ObjectManager",
                                                    "/example/Animals",
                                                    NULL, /* xcancellable_t */
                                                    &error);
  if (fixture->manager == NULL)
    xerror ("Error getting object manager client: %s", error->message);
}

static void
fixture_teardown (TestFixture *fixture, xconstpointer unused)
{
  /* Tear down the proxy
   */
  if (fixture->manager)
    xobject_unref (fixture->manager);

  /* Stop the private D-Bus daemon
   */
  g_test_dbus_down (fixture->dbus);
  xobject_unref (fixture->dbus);
}

/* The gdbus-example-objectmanager-server exports 10 objects,
 * to test the server has actually activated, let's ensure
 * that 10 objects exist.
 */
static void
test_gtest_dbus (TestFixture *fixture, xconstpointer unused)
{
  xlist_t *objects;

  objects = g_dbus_object_manager_get_objects (fixture->manager);

  g_assert_cmpint (xlist_length (objects), ==, 10);
  xlist_free_full (objects, xobject_unref);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  /* This test simply ensures that we can bring the xtest_dbus_t up and down a hand
   * full of times in a row, each time successfully activating the in-tree service
   */
  g_test_add ("/xtest_dbus_t/Cycle1", TestFixture, NULL,
  	      fixture_setup, test_gtest_dbus, fixture_teardown);
  g_test_add ("/xtest_dbus_t/Cycle2", TestFixture, NULL,
  	      fixture_setup, test_gtest_dbus, fixture_teardown);
  g_test_add ("/xtest_dbus_t/Cycle3", TestFixture, NULL,
  	      fixture_setup, test_gtest_dbus, fixture_teardown);
  g_test_add ("/xtest_dbus_t/Cycle4", TestFixture, NULL,
  	      fixture_setup, test_gtest_dbus, fixture_teardown);
  g_test_add ("/xtest_dbus_t/Cycle5", TestFixture, NULL,
  	      fixture_setup, test_gtest_dbus, fixture_teardown);

  return g_test_run ();
}
