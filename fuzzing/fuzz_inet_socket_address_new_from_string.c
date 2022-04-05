#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;
  xsocket_address_t *addr = NULL;

  fuzz_set_logging_func ();

  /* ignore @size (the function doesn’t support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) g_strndup ((const xchar_t *) data, size);
  addr = g_inet_socket_address_new_from_string ((const xchar_t *) nul_terminated_data, 1);
  g_free (nul_terminated_data);

  if (addr != NULL)
    {
      xchar_t *text = xsocket_connectable_to_string (XSOCKET_CONNECTABLE (addr));
      g_free (text);
    }

  g_clear_object (&addr);

  return 0;
}
