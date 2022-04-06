#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;
  xdatetime_t *dt = NULL;

  fuzz_set_logging_func ();

  /* ignore @size (the function doesn’t support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) xstrndup ((const xchar_t *) data, size);
  dt = xdate_time_new_from_iso8601 ((const xchar_t *) nul_terminated_data, NULL);
  g_free (nul_terminated_data);

  if (dt != NULL)
    {
      xchar_t *text = xdate_time_format_iso8601 (dt);
      g_free (text);
    }

  g_clear_pointer (&dt, xdate_time_unref);

  return 0;
}
