#include "fuzz.h"

static void
test_bytes (const xuint8_t *data,
            xsize_t         size)
{
  xerror_t *error = NULL;
  xbytes_t *unescaped_bytes = NULL;
  xchar_t *escaped_string = NULL;

  if (size > G_MAXSSIZE)
    return;

  unescaped_bytes = xuri_unescape_bytes ((const xchar_t *) data, (xssize_t) size, NULL, &error);
  if (unescaped_bytes == NULL)
    {
      g_assert_nonnull (error);
      g_clear_error (&error);
      return;
    }

  escaped_string = xuri_escape_bytes (xbytes_get_data (unescaped_bytes, NULL),
                                       xbytes_get_size (unescaped_bytes),
                                       NULL);
  xbytes_unref (unescaped_bytes);

  if (escaped_string == NULL)
    return;

  g_free (escaped_string);
}

static void
test_string (const xuint8_t *data,
             xsize_t         size)
{
  xchar_t *unescaped_string = NULL;
  xchar_t *escaped_string = NULL;

  unescaped_string = xuri_unescape_segment ((const xchar_t *) data, (const xchar_t *) data + size, NULL);
  if (unescaped_string == NULL)
    return;

  escaped_string = xuri_escape_string (unescaped_string, NULL, TRUE);
  g_free (unescaped_string);

  if (escaped_string == NULL)
    return;

  g_free (escaped_string);
}

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  fuzz_set_logging_func ();

  /* Bytes form */
  test_bytes (data, size);

  /* String form (doesn’t do %-decoding) */
  test_string (data, size);

  return 0;
}
