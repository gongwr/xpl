#include "fuzz.h"

static void
test_parse (const xchar_t   *data,
            size_t         size,
            GKeyFileFlags  flags)
{
  xkey_file_t *key = NULL;

  key = xkey_file_new ();
  xkey_file_load_from_data (key, (const xchar_t*) data, size, G_KEY_FILE_NONE,
                             NULL);

  xkey_file_free (key);
}

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  fuzz_set_logging_func ();

  test_parse ((const xchar_t *) data, size, G_KEY_FILE_NONE);
  test_parse ((const xchar_t *) data, size, G_KEY_FILE_KEEP_COMMENTS);
  test_parse ((const xchar_t *) data, size, G_KEY_FILE_KEEP_TRANSLATIONS);

  return 0;
}
