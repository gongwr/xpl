/* GLib testing framework examples and tests
 *
 * Copyright (C) 2013 Collabora, Ltd.
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 * Author: Philip Withnall <philip.withnall@collabora.co.uk>
 */

#define GIO_COMPILATION 1
#include "../thumbnail-verify.c"

static void
test_validity (void)
{
  struct
    {
      const xchar_t *filename;  /* name of a file in the tests/thumbnails dir */
      xuint64_t mtime;  /* asserted mtime of @filename */
      xuint64_t size;  /* asserted size of @filename */
      xboolean_t expected_validity;  /* should thumbnail_verify() succeed? */
    }
  tests[] =
    {
      /*
       * Tests with well-formed PNG files.
       *
       * Note that these files have all been brutally truncated to a reasonable
       * size, so aren't actually valid PNG files. Their headers are valid,
       * however, and that's all we care about.
       */

      /* test_t that validation succeeds against a valid PNG file with URI,
       * mtime and size which match the expected values. */
      { "valid.png", 1382429848, 93654, TRUE },
      /* test_t that validation succeeds with URI and mtime, but no size in the
       * tEXt data. */
      { "valid-no-size.png", 1382429848, 93633, TRUE },
      /* test_t that a missing file fails validation. */
      { "missing.png", 123456789, 12345, FALSE },
      /* test_t that an existing file with no tEXt data fails validation. */
      { "no-text-data.png", 123 /* invalid */, 26378, FALSE },
      /* test_t that a URI mismatch fails validation. */
      { "uri-mismatch.png" /* invalid */, 1382429848, 93654, FALSE },
      /* test_t that an mtime mismatch fails validation. */
      { "valid.png", 123 /* invalid */, 93654, FALSE },
      /* test_t that a valid URI and mtime, but a mismatched size, fails
       * validation. */
      { "valid.png", 1382429848, 123 /* invalid */, FALSE },
      /* test_t that validation succeeds with an mtime of 0. */
      { "mtime-zero.png", 0, 93621, TRUE },
      /* test_t that validation fails if the mtime is only a prefix match. */
      { "valid.png", 9848 /* invalid */, 93654, FALSE },

      /*
       * Tests with PNG files which have malicious or badly-formed headers.
       *
       * As above, the files have all been truncated to reduce their size.
       */

      /* Check a corrupted PNG header fails validation. */
      { "bad-header.png", 1382429848, 93654, FALSE },
      /* Check a PNG header by itself fails. */
      { "header-only.png", 1382429848, 8, FALSE },
      /* Check a PNG header and initial chunk size fails. */
      { "header-and-chunk-size.png", 1382429848, 20, FALSE },
      /* Check a huge chunk size fails. */
      { "huge-chunk-size.png", 1382429848, 93654, FALSE },
      /* Check that an empty key fails. */
      { "empty-key.png", 1382429848, 93654, FALSE },
      /* Check that an over-long value fails (even if nul-terminated). */
      { "overlong-value.png", 1382429848, 93660, FALSE },
    };
  xuint_t i;

  /* Run all the tests. */
  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      GLocalFileStat stat_buf;
      const xchar_t *thumbnail_path;
      xchar_t *file_uri;
      xboolean_t result;

      thumbnail_path = g_test_get_filename (G_TEST_DIST, "thumbnails",
                                            tests[i].filename, NULL);
      file_uri = xstrconcat ("file:///tmp/", tests[i].filename, NULL);
#ifdef HAVE_STATX
      stat_buf.stx_mtime.tv_sec = tests[i].mtime;
      stat_buf.stx_size = tests[i].size;
#else
#ifdef G_OS_WIN32
      stat_buf.st_mtim.tv_sec = tests[i].mtime;
#else
      stat_buf.st_mtime = tests[i].mtime;
#endif
      stat_buf.st_size = tests[i].size;
#endif

      result = thumbnail_verify (thumbnail_path, file_uri, &stat_buf);

      g_free (file_uri);

      xassert (result == tests[i].expected_validity);
    }
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/png-thumbs/validity", test_validity);

  return g_test_run ();
}
