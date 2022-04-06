#include <gio/gio.h>
#include <locale.h>

static xboolean_t option_use_async;
static xint_t     option_format_size;

static xint_t     outstanding_asyncs;

static void
print_result (const xchar_t *filename,
              xuint64_t      disk_usage,
              xuint64_t      num_dirs,
              xuint64_t      num_files,
              xerror_t      *error,
              xchar_t        nl)
{
  if (!error)
    {
      if (option_format_size)
        {
          GFormatSizeFlags format_flags;
          xchar_t *str;

          format_flags = (option_format_size > 1) ? G_FORMAT_SIZE_LONG_FORMAT : G_FORMAT_SIZE_DEFAULT;
          str = g_format_size_full (disk_usage, format_flags);
          g_print ("%s: %s (%"G_GUINT64_FORMAT" dirs, %"G_GUINT64_FORMAT" files)%c",
                   filename, str, num_dirs, num_files, nl);
          g_free (str);
        }
      else
        g_print ("%s: %"G_GUINT64_FORMAT" (%"G_GUINT64_FORMAT" dirs, %"G_GUINT64_FORMAT" files)%c",
                 filename, disk_usage, num_dirs, num_files, nl);
    }
  else
    {
      g_printerr ("%s: %s\n", filename, error->message);
      xerror_free (error);
    }
}

static void
async_ready_func (xobject_t      *source,
                  xasync_result_t *result,
                  xpointer_t      user_data)
{
  xchar_t *filename = user_data;
  xerror_t *error = NULL;
  xuint64_t disk_usage;
  xuint64_t num_dirs;
  xuint64_t num_files;

  xfile_measure_disk_usage_finish (XFILE (source), result, &disk_usage, &num_dirs, &num_files, &error);
  print_result (filename, disk_usage, num_dirs, num_files, error, '\n');
  outstanding_asyncs--;
  g_free (filename);
}

static void
report_progress (xboolean_t reporting,
                 xuint64_t  disk_usage,
                 xuint64_t  num_dirs,
                 xuint64_t  num_files,
                 xpointer_t user_data)
{
  const xchar_t *filename = user_data;

  if (!reporting)
    g_printerr ("%s: warning: does not support progress reporting\n", filename);

  print_result (filename, disk_usage, num_dirs, num_files, NULL, '\r');
}

int
main (int argc, char **argv)
{
  xfile_measure_progress_callback_t progress = NULL;
  xfile_measure_flags_t flags = 0;
  xint_t i;

#ifdef G_OS_WIN32
  argv = g_win32_get_command_line ();
#endif

  setlocale (LC_ALL, "");



  for (i = 1; argv[i] && argv[i][0] == '-'; i++)
    {
      if (xstr_equal (argv[i], "--"))
        break;

      if (xstr_equal (argv[i], "--help"))
        {
          g_print ("usage: du [--progress] [--async] [-x] [-h] [-h] [--apparent-size] [--any-error] [--] files...\n");
#ifdef G_OS_WIN32
          xstrfreev (argv);
#endif
          return 0;
        }
      else if (xstr_equal (argv[i], "-x"))
        flags |= XFILE_MEASURE_NO_XDEV;
      else if (xstr_equal (argv[i], "-h"))
        option_format_size++;
      else if (xstr_equal (argv[i], "--apparent-size"))
        flags |= XFILE_MEASURE_APPARENT_SIZE;
      else if (xstr_equal (argv[i], "--any-error"))
        flags |= XFILE_MEASURE_REPORT_ANY_ERROR;
      else if (xstr_equal (argv[i], "--async"))
        option_use_async = TRUE;
      else if (xstr_equal (argv[i], "--progress"))
        progress = report_progress;
      else
        {
          g_printerr ("unrecognised flag %s\n", argv[i]);
        }
    }

  if (!argv[i])
    {
      g_printerr ("usage: du [--progress] [--async] [-x] [-h] [-h] [--apparent-size] [--any-error] [--] files...\n");
#ifdef G_OS_WIN32
      xstrfreev (argv);
#endif
      return 1;
    }

  while (argv[i])
  {
    xfile_t *file = xfile_new_for_commandline_arg (argv[i]);

    if (option_use_async)
    {
      xfile_measure_disk_usage_async (file, flags, G_PRIORITY_DEFAULT, NULL,
                                       progress, argv[i], async_ready_func, argv[i]);
      outstanding_asyncs++;
    }
    else
    {
      xerror_t *error = NULL;
      xuint64_t disk_usage;
      xuint64_t num_dirs;
      xuint64_t num_files;

      xfile_measure_disk_usage (file, flags, NULL, progress, argv[i],
                                 &disk_usage, &num_dirs, &num_files, &error);
      print_result (argv[i], disk_usage, num_dirs, num_files, error, '\n');
    }

    xobject_unref (file);

    i++;
  }

  while (outstanding_asyncs)
    xmain_context_iteration (NULL, TRUE);

#ifdef G_OS_WIN32
  xstrfreev (argv);
#endif

  return 0;
}
