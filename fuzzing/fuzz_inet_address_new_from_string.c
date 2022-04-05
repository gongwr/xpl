#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;
  xinet_address_t *addr = NULL;

  fuzz_set_logging_func ();

  /* ignore @size (the function doesn’t support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) g_strndup ((const xchar_t *) data, size);
  addr = xinet_address_new_from_string ((const xchar_t *) nul_terminated_data);
  g_free (nul_terminated_data);

  if (addr != NULL)
    {
      xchar_t *text = xinet_address_to_string (addr);
      g_free (text);
    }

  g_clear_object (&addr);

  return 0;
}
