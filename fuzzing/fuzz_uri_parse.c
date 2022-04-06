#include "fuzz.h"

static void
test_with_flags (const xchar_t *data,
                 xuri_flags_t    flags)
{
  xuri_t *uri = NULL;
  xchar_t *uri_string = NULL;

  uri = xuri_parse (data, flags, NULL);

  if (uri == NULL)
    return;

  uri_string = xuri_to_string (uri);
  xuri_unref (uri);

  if (uri_string == NULL)
    return;

  g_free (uri_string);
}

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;

  fuzz_set_logging_func ();

  /* ignore @size (xuri_parse() doesn’t support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) xstrndup ((const xchar_t *) data, size);
  test_with_flags ((const xchar_t *) nul_terminated_data, XURI_FLAGS_NONE);
  test_with_flags ((const xchar_t *) nul_terminated_data, XURI_FLAGS_PARSE_RELAXED);
  test_with_flags ((const xchar_t *) nul_terminated_data, XURI_FLAGS_NON_DNS);
  test_with_flags ((const xchar_t *) nul_terminated_data, XURI_FLAGS_HAS_AUTH_PARAMS);
  test_with_flags ((const xchar_t *) nul_terminated_data, XURI_FLAGS_HAS_PASSWORD);
  test_with_flags ((const xchar_t *) nul_terminated_data, XURI_FLAGS_ENCODED_QUERY);
  test_with_flags ((const xchar_t *) nul_terminated_data, XURI_FLAGS_ENCODED_PATH);
  test_with_flags ((const xchar_t *) nul_terminated_data, XURI_FLAGS_SCHEME_NORMALIZE);
  g_free (nul_terminated_data);

  return 0;
}
