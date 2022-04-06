#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  xvariant_t *variant = NULL, *normal_variant = NULL;

  fuzz_set_logging_func ();

  variant = xvariant_new_from_data (G_VARIANT_TYPE_VARIANT, data, size, FALSE,
                                     NULL, NULL);
  if (variant == NULL)
    return 0;

  normal_variant = xvariant_take_ref (xvariant_get_normal_form (variant));
  xvariant_get_data (variant);

  xvariant_unref (normal_variant);
  xvariant_unref (variant);
  return 0;
}
