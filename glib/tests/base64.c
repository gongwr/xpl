#include <glib.h>
#include <string.h>
#include <stdlib.h>

#define DATA_SIZE 1024
#define BLOCK_SIZE 32
#define NUM_BLOCKS 32
static xuchar_t data[DATA_SIZE];

static void
test_incremental (xboolean_t line_break,
                  xsize_t    length)
{
  char *p;
  xsize_t len, decoded_len, max, input_len, block_size;
  int state, save;
  xuint_t decoder_save;
  char *text;
  xuchar_t *data2;

  data2 = g_malloc (length);
  text = g_malloc (length * 4);

  len = 0;
  state = 0;
  save = 0;
  input_len = 0;
  while (input_len < length)
    {
      block_size = MIN (BLOCK_SIZE, length - input_len);
      len += g_base64_encode_step (data + input_len, block_size,
                                   line_break, text + len, &state, &save);
      input_len += block_size;
    }
  len += g_base64_encode_close (line_break, text + len, &state, &save);

  if (line_break)
    max = length * 4 / 3 + length * 4 / (3 * 72) + 7;
  else
    max = length * 4 / 3 + 6;

  /* Check encoded length */
  g_assert_cmpint (len, <=, max);

  decoded_len = 0;
  state = 0;
  decoder_save = 0;
  p = text;
  while (len > 0)
    {
      int chunk_len = MIN (BLOCK_SIZE, len);
      decoded_len += g_base64_decode_step (p,
                                           chunk_len,
                                           data2 + decoded_len,
                                           &state, &decoder_save);
      p += chunk_len;
      len -= chunk_len;
    }

  g_assert_cmpmem (data, length, data2, decoded_len);

  g_free (text);
  g_free (data2);
}

static void
test_incremental_break (xconstpointer d)
{
  xint_t length = GPOINTER_TO_INT (d);

  test_incremental (TRUE, length);
}

static void
test_incremental_nobreak (xconstpointer d)
{
  xint_t length = GPOINTER_TO_INT (d);

  test_incremental (FALSE, length);
}

static void
test_full (xconstpointer d)
{
  xint_t length = GPOINTER_TO_INT (d);
  char *text;
  xuchar_t *data2;
  xsize_t len;

  text = g_base64_encode (data, length);
  data2 = g_base64_decode (text, &len);
  g_free (text);

  g_assert_cmpmem (data, length, data2, len);

  g_free (data2);
}

struct MyRawData
{
  xint_t length;   /* of data */
  xuchar_t data[DATA_SIZE];
};

/* 100 pre-encoded string from data[] buffer. Data length from 1..100
 */
static const char *ok_100_encode_strs[] = {
  "AA==",
  "AAE=",
  "AAEC",
  "AAECAw==",
  "AAECAwQ=",
  "AAECAwQF",
  "AAECAwQFBg==",
  "AAECAwQFBgc=",
  "AAECAwQFBgcI",
  "AAECAwQFBgcICQ==",
  "AAECAwQFBgcICQo=",
  "AAECAwQFBgcICQoL",
  "AAECAwQFBgcICQoLDA==",
  "AAECAwQFBgcICQoLDA0=",
  "AAECAwQFBgcICQoLDA0O",
  "AAECAwQFBgcICQoLDA0ODw==",
  "AAECAwQFBgcICQoLDA0ODxA=",
  "AAECAwQFBgcICQoLDA0ODxAR",
  "AAECAwQFBgcICQoLDA0ODxAREg==",
  "AAECAwQFBgcICQoLDA0ODxAREhM=",
  "AAECAwQFBgcICQoLDA0ODxAREhMU",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFQ==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRY=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYX",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGA==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBk=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBka",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGw==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxw=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwd",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHg==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8g",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gIQ==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISI=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIj",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJA==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCU=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUm",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJw==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJyg=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygp",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKg==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKis=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKiss",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLQ==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4v",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMA==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDE=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEy",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMw==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Ng==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OQ==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PA==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+Pw==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0A=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BB",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQg==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkM=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNE",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERQ==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUY=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZH",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSA==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSEk=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElK",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKSw==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0w=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xN",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTg==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk8=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9Q",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUQ==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVI=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJT",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVA==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFU=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVW",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWVw==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1g=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZ",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWg==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWls=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltc",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXQ==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV4=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5f",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYA==",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGE=",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFi",
  "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiYw==",
  NULL
};

static void
generate_databuffer_for_base64 (struct MyRawData *p)
{
  int i;
  for (i = 0; i < DATA_SIZE; i++)
    p->data[i] = i;
}

static void
test_base64_encode (void)
{
  int i;
  xint_t length = 1;
  char *text;
  struct MyRawData myraw;

  generate_databuffer_for_base64 (&myraw);

  for (i = 0; ok_100_encode_strs[i]; i++)
    {
      length = i + 1;
      text = g_base64_encode (myraw.data, length);
      g_assert_cmpstr (text, ==, ok_100_encode_strs[i]);
      /* printf ("\"%s\",\n",text); */
      g_free (text);
    }
}

/* test_t that incremental and all-in-one encoding of strings of a length which
 * is not a multiple of 3 bytes behave the same, as the state carried over
 * between g_base64_encode_step() calls varies depending on how the input is
 * split up. This is like the test_base64_decode_smallblock() test, but for
 * encoding. */
static void
test_base64_encode_incremental_small_block (xconstpointer block_size_p)
{
  xsize_t i;
  struct MyRawData myraw;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=780066");

  generate_databuffer_for_base64 (&myraw);

  for (i = 0; ok_100_encode_strs[i] != NULL; i++)
    {
      const xuint_t block_size = GPOINTER_TO_UINT (block_size_p);
      xchar_t *encoded_complete = NULL;
      xchar_t encoded_stepped[1024];
      xint_t state = 0, save = 0;
      xsize_t len_written, len_read, len_to_read, input_length;

      input_length = i + 1;

      /* Do it all at once. */
      encoded_complete = g_base64_encode (myraw.data, input_length);

      /* Split the data up so some number of bits remain after each step. */
      for (len_written = 0, len_read = 0; len_read < input_length; len_read += len_to_read)
        {
          len_to_read = MIN (block_size, input_length - len_read);
          len_written += g_base64_encode_step (myraw.data + len_read, len_to_read,
                                               FALSE,
                                               encoded_stepped + len_written,
                                               &state, &save);
        }

      len_written += g_base64_encode_close (FALSE, encoded_stepped + len_written,
                                            &state, &save);
      g_assert_cmpuint (len_written, <, G_N_ELEMENTS (encoded_stepped));

      /* Nul-terminate to make string comparison easier. */
      encoded_stepped[len_written] = '\0';

      /* Compare results. They should be the same. */
      g_assert_cmpstr (encoded_complete, ==, ok_100_encode_strs[i]);
      g_assert_cmpstr (encoded_stepped, ==, encoded_complete);

      g_free (encoded_complete);
    }
}

static void
decode_and_compare (const xchar_t            *datap,
                    const struct MyRawData *p)
{
  xuchar_t *data2;
  xsize_t len;

  data2 = g_base64_decode (datap, &len);
  g_assert_cmpmem (p->data, p->length, data2, len);
  g_free (data2);
}

static void
decode_inplace_and_compare (const xchar_t            *datap,
                            const struct MyRawData *p)
{
  xchar_t *data;
  xuchar_t *data2;
  xsize_t len;

  data = xstrdup (datap);
  data2 = g_base64_decode_inplace (data, &len);
  g_assert_cmpmem (p->data, p->length, data2, len);
  g_free (data2);
}

static void
test_base64_decode (void)
{
  int i;
  struct MyRawData myraw;

  generate_databuffer_for_base64 (&myraw);

  for (i = 0; ok_100_encode_strs[i]; i++)
    {
      myraw.length = i + 1;
      decode_and_compare (ok_100_encode_strs[i], &myraw);
    }
}

static void
test_base64_decode_inplace (void)
{
  int i;
  struct MyRawData myraw;

  generate_databuffer_for_base64 (&myraw);

  for (i = 0; ok_100_encode_strs[i]; i++)
    {
      myraw.length = i + 1;
      decode_inplace_and_compare (ok_100_encode_strs[i], &myraw);
    }
}

static void
test_base64_encode_decode (void)
{
  int i;
  char *text;
  struct MyRawData myraw;

  generate_databuffer_for_base64 (&myraw);

  for (i = 0; i < DATA_SIZE; i++)
    {
      myraw.length = i + 1;
      text = g_base64_encode (myraw.data, myraw.length);

      decode_and_compare (text, &myraw);

      g_free (text);
    }
}

static void
test_base64_decode_smallblock (xconstpointer blocksize_p)
{
  const xuint_t blocksize = GPOINTER_TO_UINT (blocksize_p);
  xuint_t i;

  for (i = 0; ok_100_encode_strs[i]; i++)
    {
      const char *str = ok_100_encode_strs[i];
      const char *p;
      xsize_t len = strlen (str);
      xint_t state = 0;
      xuint_t save = 0;
      xuchar_t *decoded;
      xsize_t decoded_size = 0;
      xuchar_t *decoded_atonce;
      xsize_t decoded_atonce_size = 0;

      decoded = g_malloc (len / 4 * 3 + 3);

      p = str;
      while (len > 0)
        {
          int chunk_len = MIN (blocksize, len);
          xsize_t size = g_base64_decode_step (p, chunk_len,
                                             decoded + decoded_size,
                                             &state, &save);
          decoded_size += size;
          len -= chunk_len;
          p += chunk_len;
        }

      decoded_atonce = g_base64_decode (str, &decoded_atonce_size);

      g_assert_cmpmem (decoded, decoded_size, decoded_atonce, decoded_atonce_size);

      g_free (decoded);
      g_free (decoded_atonce);
    }
}

/* test_t that calling g_base64_encode (NULL, 0) returns correct output. This is
 * as per the first test vector in RFC 4648 ??10.
 * https://tools.ietf.org/html/rfc4648#section-10 */
static void
test_base64_encode_empty (void)
{
  xchar_t *encoded = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/1698");

  encoded = g_base64_encode (NULL, 0);
  g_assert_cmpstr (encoded, ==, "");
  g_free (encoded);

  encoded = g_base64_encode ((const xuchar_t *) "", 0);
  g_assert_cmpstr (encoded, ==, "");
  g_free (encoded);
}

/* test_t that calling g_base64_decode ("", *) returns correct output. This is
 * as per the first test vector in RFC 4648 ??10. Note that calling
 * g_base64_decode (NULL, *) is not allowed.
 * https://tools.ietf.org/html/rfc4648#section-10 */
static void
test_base64_decode_empty (void)
{
  xuchar_t *decoded = NULL;
  xsize_t decoded_len;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/1698");

  decoded = g_base64_decode ("", &decoded_len);
  g_assert_cmpstr ((xchar_t *) decoded, ==, "");
  g_assert_cmpuint (decoded_len, ==, 0);
  g_free (decoded);
}

/* Check all the RFC 4648 test vectors for base 64 encoding from ??10.
 * https://tools.ietf.org/html/rfc4648#section-10 */
static void
test_base64_encode_decode_rfc4648 (void)
{
  const struct
    {
      const xchar_t *decoded;  /* technically this should be a byte array, but all the test vectors are ASCII strings */
      const xchar_t *encoded;
    }
  vectors[] =
    {
      { "", "" },
      { "f", "Zg==" },
      { "fo", "Zm8=" },
      { "foo", "Zm9v" },
      { "foob", "Zm9vYg==" },
      { "fooba", "Zm9vYmE=" },
      { "foobar", "Zm9vYmFy" },
    };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      xchar_t *encoded = NULL;
      xuchar_t *decoded = NULL;
      xsize_t expected_decoded_len = strlen (vectors[i].decoded);
      xsize_t decoded_len;

      g_test_message ("Vector %" G_GSIZE_FORMAT ": %s", i, vectors[i].decoded);

      encoded = g_base64_encode ((const xuchar_t *) vectors[i].decoded, expected_decoded_len);
      g_assert_cmpstr (encoded, ==, vectors[i].encoded);

      decoded = g_base64_decode (encoded, &decoded_len);
      g_assert_cmpstr ((xchar_t *) decoded, ==, vectors[i].decoded);
      g_assert_cmpuint (decoded_len, ==, expected_decoded_len);

      g_free (encoded);
      g_free (decoded);
    }
}

int
main (int argc, char *argv[])
{
  xint_t i;

  g_test_init (&argc, &argv, NULL);

  for (i = 0; i < DATA_SIZE; i++)
    data[i] = (xuchar_t)i;

  g_test_add_data_func ("/base64/full/1", GINT_TO_POINTER (DATA_SIZE), test_full);
  g_test_add_data_func ("/base64/full/2", GINT_TO_POINTER (1), test_full);
  g_test_add_data_func ("/base64/full/3", GINT_TO_POINTER (2), test_full);
  g_test_add_data_func ("/base64/full/4", GINT_TO_POINTER (3), test_full);

  g_test_add_data_func ("/base64/encode/incremental/small-block/1", GINT_TO_POINTER (1), test_base64_encode_incremental_small_block);
  g_test_add_data_func ("/base64/encode/incremental/small-block/2", GINT_TO_POINTER (2), test_base64_encode_incremental_small_block);
  g_test_add_data_func ("/base64/encode/incremental/small-block/3", GINT_TO_POINTER (3), test_base64_encode_incremental_small_block);
  g_test_add_data_func ("/base64/encode/incremental/small-block/4", GINT_TO_POINTER (4), test_base64_encode_incremental_small_block);

  g_test_add_data_func ("/base64/incremental/nobreak/1", GINT_TO_POINTER (DATA_SIZE), test_incremental_nobreak);
  g_test_add_data_func ("/base64/incremental/break/1", GINT_TO_POINTER (DATA_SIZE), test_incremental_break);

  g_test_add_data_func ("/base64/incremental/nobreak/2", GINT_TO_POINTER (DATA_SIZE - 1), test_incremental_nobreak);
  g_test_add_data_func ("/base64/incremental/break/2", GINT_TO_POINTER (DATA_SIZE - 1), test_incremental_break);

  g_test_add_data_func ("/base64/incremental/nobreak/3", GINT_TO_POINTER (DATA_SIZE - 2), test_incremental_nobreak);
  g_test_add_data_func ("/base64/incremental/break/3", GINT_TO_POINTER (DATA_SIZE - 2), test_incremental_break);

  g_test_add_data_func ("/base64/incremental/nobreak/4-a", GINT_TO_POINTER (1), test_incremental_nobreak);
  g_test_add_data_func ("/base64/incremental/nobreak/4-b", GINT_TO_POINTER (2), test_incremental_nobreak);
  g_test_add_data_func ("/base64/incremental/nobreak/4-c", GINT_TO_POINTER (3), test_incremental_nobreak);

  g_test_add_func ("/base64/encode", test_base64_encode);
  g_test_add_func ("/base64/decode", test_base64_decode);
  g_test_add_func ("/base64/decode-inplace", test_base64_decode_inplace);
  g_test_add_func ("/base64/encode-decode", test_base64_encode_decode);

  g_test_add_data_func ("/base64/incremental/smallblock/1", GINT_TO_POINTER(1),
                        test_base64_decode_smallblock);
  g_test_add_data_func ("/base64/incremental/smallblock/2", GINT_TO_POINTER(2),
                        test_base64_decode_smallblock);
  g_test_add_data_func ("/base64/incremental/smallblock/3", GINT_TO_POINTER(3),
                        test_base64_decode_smallblock);
  g_test_add_data_func ("/base64/incremental/smallblock/4", GINT_TO_POINTER(4),
                        test_base64_decode_smallblock);

  g_test_add_func ("/base64/encode/empty", test_base64_encode_empty);
  g_test_add_func ("/base64/decode/empty", test_base64_decode_empty);

  g_test_add_func ("/base64/encode-decode/rfc4648", test_base64_encode_decode_rfc4648);

  return g_test_run ();
}
