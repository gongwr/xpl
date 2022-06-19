#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  xerror_t *error = NULL;
  xhashtable_t *parsed_params = NULL;

  fuzz_set_logging_func ();

  if (size > G_MAXSSIZE)
    return 0;

  parsed_params = xuri_parse_params ((const xchar_t *) data, (xssize_t) size,
                                      "&", XURI_PARAMS_NONE, &error);
  if (parsed_params == NULL)
    {
      xassert (error);
      g_clear_error (&error);
      return 0;
    }


  g_assert_no_error (error);
  xhash_table_unref (parsed_params);

  return 0;
}
