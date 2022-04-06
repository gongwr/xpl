#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <locale.h>
#include <stdlib.h>

static void
print (const xchar_t *str)
{
  g_print ("%s\n", str ? str : "nil");
}

static void
print_app_list (xlist_t *list)
{
  while (list)
    {
      xapp_info_t *info = list->data;
      print (xapp_info_get_id (info));
      list = xlist_delete_link (list, list);
      xobject_unref (info);
    }
}

static void
quit (xpointer_t user_data)
{
  g_print ("appinfo database changed.\n");
  exit (0);
}

int
main (int argc, char **argv)
{
  setlocale (LC_ALL, "");

  if (argv[1] == NULL)
    ;
  else if (xstr_equal (argv[1], "list"))
    {
      xlist_t *all, *i;

      all = xapp_info_get_all ();
      for (i = all; i; i = i->next)
        g_print ("%s%s", xapp_info_get_id (i->data), i->next ? " " : "\n");
      xlist_free_full (all, xobject_unref);
    }
  else if (xstr_equal (argv[1], "search"))
    {
      xchar_t ***results;
      xint_t i, j;

      results = xdesktop_app_info_search (argv[2]);
      for (i = 0; results[i]; i++)
        {
          for (j = 0; results[i][j]; j++)
            g_print ("%s%s", j ? " " : "", results[i][j]);
          g_print ("\n");
          xstrfreev (results[i]);
        }
      g_free (results);
    }
  else if (xstr_equal (argv[1], "implementations"))
    {
      xlist_t *results;

      results = xdesktop_app_info_get_implementations (argv[2]);
      print_app_list (results);
    }
  else if (xstr_equal (argv[1], "show-info"))
    {
      xapp_info_t *info;

      info = (xapp_info_t *) xdesktop_app_info_new (argv[2]);
      if (info)
        {
          print (xapp_info_get_id (info));
          print (xapp_info_get_name (info));
          print (xapp_info_get_display_name (info));
          print (xapp_info_get_description (info));
          xobject_unref (info);
        }
    }
  else if (xstr_equal (argv[1], "default-for-type"))
    {
      xapp_info_t *info;

      info = xapp_info_get_default_for_type (argv[2], FALSE);

      if (info)
        {
          print (xapp_info_get_id (info));
          xobject_unref (info);
        }
    }
  else if (xstr_equal (argv[1], "recommended-for-type"))
    {
      xlist_t *list;

      list = xapp_info_get_recommended_for_type (argv[2]);
      print_app_list (list);
    }
  else if (xstr_equal (argv[1], "all-for-type"))
    {
      xlist_t *list;

      list = xapp_info_get_all_for_type (argv[2]);
      print_app_list (list);
    }

  else if (xstr_equal (argv[1], "fallback-for-type"))
    {
      xlist_t *list;

      list = xapp_info_get_fallback_for_type (argv[2]);
      print_app_list (list);
    }

  else if (xstr_equal (argv[1], "should-show"))
    {
      xapp_info_t *info;

      info = (xapp_info_t *) xdesktop_app_info_new (argv[2]);
      if (info)
        {
          g_print ("%s\n", xapp_info_should_show (info) ? "true" : "false");
          xobject_unref (info);
        }
    }

  else if (xstr_equal (argv[1], "monitor"))
    {
      xapp_info_monitor_t *monitor;
      xapp_info_t *info;

      monitor = xapp_info_monitor_get ();

      info = (xapp_info_t *) xdesktop_app_info_new ("this-desktop-file-does-not-exist");
      g_assert (!info);

      xsignal_connect (monitor, "changed", G_CALLBACK (quit), NULL);

      while (1)
        xmain_context_iteration (NULL, TRUE);
    }

  return 0;
}
