#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;
  xsocket_connectable_t *connectable = NULL;

  fuzz_set_logging_func ();

  /* ignore @size (g_network_address_parse_uri() doesn’t support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) xstrndup ((const xchar_t *) data, size);
  connectable = g_network_address_parse_uri ((const xchar_t *) nul_terminated_data, 1, NULL);
  g_free (nul_terminated_data);

  if (connectable != NULL)
    {
      xchar_t *text = xsocket_connectable_to_string (connectable);
      g_free (text);
    }

  g_clear_object (&connectable);

  return 0;
}
