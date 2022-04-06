#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;
  xchar_t *canonicalized = NULL;

  fuzz_set_logging_func ();

  /* ignore @size (g_canonicalize_filename() doesn’t support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) xstrndup ((const xchar_t *) data, size);
  canonicalized = g_canonicalize_filename ((const xchar_t *) nul_terminated_data, "/");
  g_free (nul_terminated_data);

  g_free (canonicalized);

  return 0;
}
