#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  const xchar_t *gdata = (const xchar_t*) data;
  xvariant_t *variant = NULL;
  xchar_t *text = NULL;

  fuzz_set_logging_func ();

  variant = g_variant_parse (NULL, gdata, gdata + size, NULL, NULL);
  if (variant == NULL)
    return 0;

  text = g_variant_print (variant, TRUE);

  g_free (text);
  g_variant_unref (variant);
  return 0;
}
