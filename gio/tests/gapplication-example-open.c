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

static void
open (xapplication_t  *application,
      xfile_t        **files,
      xint_t           n_files,
      const xchar_t   *hint)
{
  xint_t i;

  for (i = 0; i < n_files; i++)
    {
      xchar_t *uri = xfile_get_uri (files[i]);
      g_print ("open %s\n", uri);
      g_free (uri);
    }

  /* Note: when doing a longer-lasting action here that returns
   * to the mainloop, you should use xapplication_hold() and
   * xapplication_release() to keep the application alive until
   * the action is completed.
   */
}

int
main (int argc, char **argv)
{
  xapplication_t *app;
  int status;

  app = xapplication_new ("org.gtk.test_application_t",
                           G_APPLICATION_HANDLES_OPEN);
  xsignal_connect (app, "activate", G_CALLBACK (activate), NULL);
  xsignal_connect (app, "open", G_CALLBACK (open), NULL);
  xapplication_set_inactivity_timeout (app, 10000);

  status = xapplication_run (app, argc, argv);

  xobject_unref (app);

  return status;
}
