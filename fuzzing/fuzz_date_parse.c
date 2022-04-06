#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;
  xdate_t *date = xdate_new ();

  fuzz_set_logging_func ();

  /* ignore @size (xdate_set_parse() doesn’t support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) xstrndup ((const xchar_t *) data, size);
  xdate_set_parse (date, (const xchar_t *) nul_terminated_data);
  g_free (nul_terminated_data);

  xdate_free (date);

  return 0;
}
