#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  unsigned char *nul_terminated_data = NULL;
  const xchar_t *skipped_root G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;
  xchar_t *basename = NULL, *dirname = NULL;

  fuzz_set_logging_func ();

  /* ignore @size (none of the functions support it); ensure @data is nul-terminated */
  nul_terminated_data = (unsigned char *) xstrndup ((const xchar_t *) data, size);

  g_path_is_absolute ((const xchar_t *) nul_terminated_data);

  skipped_root = g_path_skip_root ((const xchar_t *) nul_terminated_data);
  xassert (skipped_root == NULL || skipped_root >= (const xchar_t *) nul_terminated_data);
  xassert (skipped_root == NULL || skipped_root <= (const xchar_t *) nul_terminated_data + size);

  basename = g_path_get_basename ((const xchar_t *) nul_terminated_data);
  xassert (strcmp (basename, ".") == 0 || strlen (basename) <= size);

  dirname = g_path_get_dirname ((const xchar_t *) nul_terminated_data);
  xassert (strcmp (dirname, ".") == 0 || strlen (dirname) <= size);

  g_free (nul_terminated_data);
  g_free (dirname);
  g_free (basename);

  return 0;
}
