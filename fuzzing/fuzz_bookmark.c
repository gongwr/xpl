#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  xbookmark_file_t *bookmark = NULL;

  fuzz_set_logging_func ();

  bookmark = g_bookmark_file_new ();
  g_bookmark_file_load_from_data (bookmark, (const xchar_t*) data, size, NULL);

  g_bookmark_file_free (bookmark);
  return 0;
}
