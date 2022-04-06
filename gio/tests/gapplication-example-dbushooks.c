#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

static void
activate (xapplication_t *application)
{
  g_print ("activated\n");

  /* Note: when doing a longer-lasting action here that returns
   * to the mainloop, you should use xapplication_hold() and
   * xapplication_release() to keep the application alive until
   * the action is completed.
   */
}

typedef xapplication_t TestApplication;
typedef xapplication_class_t TestApplicationClass;

static xtype_t test_application_get_type (void);
G_DEFINE_TYPE (TestApplication, test_application, XTYPE_APPLICATION)

static xboolean_t
test_application_dbus_register (xapplication_t    *application,
                                xdbus_connection_t *connection,
                                const xchar_t     *object_path,
                                xerror_t         **error)
{
  /* We must chain up to the parent class */
  if (!G_APPLICATION_CLASS (test_application_parent_class)->dbus_register (application,
                                                                           connection,
                                                                           object_path,
                                                                           error))
    return FALSE;

  /* Now we can do our own stuff here. For example, we could export some D-Bus objects */
  return TRUE;
}

static void
test_application_dbus_unregister (xapplication_t    *application,
                                  xdbus_connection_t *connection,
                                  const xchar_t     *object_path)
{
  /* Do our own stuff here, e.g. unexport any D-Bus objects we exported in the dbus_register
   * hook above. Be sure to check that we actually did export them, since the hook
   * above might have returned early due to the parent class' hook returning FALSE!
   */

  /* Lastly, we must chain up to the parent class */
  G_APPLICATION_CLASS (test_application_parent_class)->dbus_unregister (application,
                                                                        connection,
                                                                        object_path);
}

static void
test_application_init (TestApplication *app)
{
}

static void
test_application_class_init (TestApplicationClass *class)
{
  xapplication_class_t *xapplication_class = G_APPLICATION_CLASS (class);

  xapplication_class->dbus_register = test_application_dbus_register;
  xapplication_class->dbus_unregister = test_application_dbus_unregister;
}

static xapplication_t *
test_application_new (const xchar_t       *application_id,
                      GApplicationFlags  flags)
{
  g_return_val_if_fail (xapplication_id_is_valid (application_id), NULL);

  return xobject_new (test_application_get_type (),
                       "application-id", application_id,
                       "flags", flags,
                       NULL);
}

int
main (int argc, char **argv)
{
  xapplication_t *app;
  int status;

  app = test_application_new ("org.gtk.TestApplication", 0);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  xapplication_set_inactivity_timeout (app, 10000);

  status = xapplication_run (app, argc, argv);

  xobject_unref (app);

  return status;
}
