#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;
  xinet_address_mask_t *mask = NULL;

  fuzz_set_logging_func ();

  /* ignore @size (the function doesn’t support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) g_strndup ((const xchar_t *) data, size);
  mask = xinet_address_mask_new_from_string ((const xchar_t *) nul_terminated_data, NULL);
  g_free (nul_terminated_data);

  if (mask != NULL)
    {
      xchar_t *text = xinet_address_mask_to_string (mask);
      g_free (text);
    }

  g_clear_object (&mask);

  return 0;
}
