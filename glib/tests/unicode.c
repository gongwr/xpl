/* Unit tests for utilities
 * Copyright (C) 2010 Red Hat, Inc.
 * Copyright (C) 2011 Google, Inc.
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
 * Author: Matthias Clasen, Behdad Esfahbod
 */

/* We are testing some deprecated APIs here */
#ifndef XPL_DISABLE_DEPRECATION_WARNINGS
#define XPL_DISABLE_DEPRECATION_WARNINGS
#endif

#include <locale.h>
#include <stdio.h>

#include "glib.h"

#include "glib/gunidecomp.h"

/* test_t that xunichar_validate() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_unichar_validate (void)
{
  g_assert_true (xunichar_validate ('j'));
  g_assert_true (xunichar_validate (8356));
  g_assert_true (xunichar_validate (8356));
  g_assert_true (xunichar_validate (0xFDD1));
  g_assert_true (xunichar_validate (917760));
  g_assert_false (xunichar_validate (0x110000));
}

/* test_t that xunichar_type() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_unichar_character_type (void)
{
  xuint_t i;
  struct {
    xunicode_type_t type;
    xunichar_t     c;
  } examples[] = {
    { XUNICODE_CONTROL,              0x000D },
    { XUNICODE_FORMAT,               0x200E },
     /* XUNICODE_UNASSIGNED */
    { XUNICODE_PRIVATE_USE,          0xE000 },
    { XUNICODE_SURROGATE,            0xD800 },
    { XUNICODE_LOWERCASE_LETTER,     0x0061 },
    { XUNICODE_MODIFIER_LETTER,      0x02B0 },
    { XUNICODE_OTHER_LETTER,         0x3400 },
    { XUNICODE_TITLECASE_LETTER,     0x01C5 },
    { XUNICODE_UPPERCASE_LETTER,     0xFF21 },
    { XUNICODE_SPACING_MARK,         0x0903 },
    { XUNICODE_ENCLOSING_MARK,       0x20DD },
    { XUNICODE_NON_SPACING_MARK,     0xA806 },
    { XUNICODE_DECIMAL_NUMBER,       0xFF10 },
    { XUNICODE_LETTER_NUMBER,        0x16EE },
    { XUNICODE_OTHER_NUMBER,         0x17F0 },
    { XUNICODE_CONNECT_PUNCTUATION,  0x005F },
    { XUNICODE_DASH_PUNCTUATION,     0x058A },
    { XUNICODE_CLOSE_PUNCTUATION,    0x0F3B },
    { XUNICODE_FINAL_PUNCTUATION,    0x2019 },
    { XUNICODE_INITIAL_PUNCTUATION,  0x2018 },
    { XUNICODE_OTHER_PUNCTUATION,    0x2016 },
    { XUNICODE_OPEN_PUNCTUATION,     0x0F3A },
    { XUNICODE_CURRENCY_SYMBOL,      0x20A0 },
    { XUNICODE_MODIFIER_SYMBOL,      0x309B },
    { XUNICODE_MATH_SYMBOL,          0xFB29 },
    { XUNICODE_OTHER_SYMBOL,         0x00A6 },
    { XUNICODE_LINE_SEPARATOR,       0x2028 },
    { XUNICODE_PARAGRAPH_SEPARATOR,  0x2029 },
    { XUNICODE_SPACE_SEPARATOR,      0x202F },
  };

  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    {
      g_assert_cmpint (xunichar_type (examples[i].c), ==, examples[i].type);
    }

  /*** Testing TYPE() border cases ***/
  g_assert_cmpint (xunichar_type (0x3FF5), ==, 0x07);
  /* U+FFEFF Plane 15 Private Use */
  g_assert_cmpint (xunichar_type (0xFFEFF), ==, 0x03);
  /* U+E0001 Language Tag */
  g_assert_cmpint (xunichar_type (0xE0001), ==, 0x01);
  g_assert_cmpint (xunichar_type (XUNICODE_LAST_CHAR), ==, 0x02);
  g_assert_cmpint (xunichar_type (XUNICODE_LAST_CHAR + 1), ==, 0x02);
  g_assert_cmpint (xunichar_type (XUNICODE_LAST_CHAR_PART1), ==, 0x02);
  g_assert_cmpint (xunichar_type (XUNICODE_LAST_CHAR_PART1 + 1), ==, 0x02);
}

/* test_t that xunichar_break_type() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_unichar_break_type (void)
{
  xuint_t i;
  struct {
    GUnicodeBreakType type;
    xunichar_t          c;
  } examples[] = {
    { XUNICODE_BREAK_MANDATORY,           0x2028 },
    { XUNICODE_BREAK_CARRIAGE_RETURN,     0x000D },
    { XUNICODE_BREAK_LINE_FEED,           0x000A },
    { XUNICODE_BREAK_COMBINING_MARK,      0x0300 },
    { XUNICODE_BREAK_SURROGATE,           0xD800 },
    { XUNICODE_BREAK_ZERO_WIDTH_SPACE,    0x200B },
    { XUNICODE_BREAK_INSEPARABLE,         0x2024 },
    { XUNICODE_BREAK_NON_BREAKING_GLUE,   0x00A0 },
    { XUNICODE_BREAK_CONTINGENT,          0xFFFC },
    { XUNICODE_BREAK_SPACE,               0x0020 },
    { XUNICODE_BREAK_AFTER,               0x05BE },
    { XUNICODE_BREAK_BEFORE,              0x02C8 },
    { XUNICODE_BREAK_BEFORE_AND_AFTER,    0x2014 },
    { XUNICODE_BREAK_HYPHEN,              0x002D },
    { XUNICODE_BREAK_NON_STARTER,         0x17D6 },
    { XUNICODE_BREAK_OPEN_PUNCTUATION,    0x0028 },
    { XUNICODE_BREAK_CLOSE_PARENTHESIS,   0x0029 },
    { XUNICODE_BREAK_CLOSE_PUNCTUATION,   0x007D },
    { XUNICODE_BREAK_QUOTATION,           0x0022 },
    { XUNICODE_BREAK_EXCLAMATION,         0x0021 },
    { XUNICODE_BREAK_IDEOGRAPHIC,         0x2E80 },
    { XUNICODE_BREAK_NUMERIC,             0x0030 },
    { XUNICODE_BREAK_INFIX_SEPARATOR,     0x002C },
    { XUNICODE_BREAK_SYMBOL,              0x002F },
    { XUNICODE_BREAK_ALPHABETIC,          0x0023 },
    { XUNICODE_BREAK_PREFIX,              0x0024 },
    { XUNICODE_BREAK_POSTFIX,             0x0025 },
    { XUNICODE_BREAK_COMPLEX_CONTEXT,     0x0E01 },
    { XUNICODE_BREAK_AMBIGUOUS,           0x00F7 },
    { XUNICODE_BREAK_UNKNOWN,             0xE000 },
    { XUNICODE_BREAK_NEXT_LINE,           0x0085 },
    { XUNICODE_BREAK_WORD_JOINER,         0x2060 },
    { XUNICODE_BREAK_HANGUL_L_JAMO,       0x1100 },
    { XUNICODE_BREAK_HANGUL_V_JAMO,       0x1160 },
    { XUNICODE_BREAK_HANGUL_T_JAMO,       0x11A8 },
    { XUNICODE_BREAK_HANGUL_LV_SYLLABLE,  0xAC00 },
    { XUNICODE_BREAK_HANGUL_LVT_SYLLABLE, 0xAC01 },
    { XUNICODE_BREAK_CONDITIONAL_JAPANESE_STARTER, 0x3041 },
    { XUNICODE_BREAK_HEBREW_LETTER,                0x05D0 },
    { XUNICODE_BREAK_REGIONAL_INDICATOR,           0x1F1F6 },
    { XUNICODE_BREAK_EMOJI_BASE,          0x1F466 },
    { XUNICODE_BREAK_EMOJI_MODIFIER,      0x1F3FB },
    { XUNICODE_BREAK_ZERO_WIDTH_JOINER,   0x200D },
  };

  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    {
      g_assert_cmpint (xunichar_break_type (examples[i].c), ==, examples[i].type);
    }
}

/* test_t that xunichar_get_script() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_unichar_script (void)
{
  xuint_t i;
  struct {
    xunicode_script_t script;
    xunichar_t          c;
  } examples[] = {
    { XUNICODE_SCRIPT_COMMON,                  0x002A },
    { XUNICODE_SCRIPT_INHERITED,               0x1CED },
    { XUNICODE_SCRIPT_INHERITED,               0x0670 },
    { XUNICODE_SCRIPT_ARABIC,                  0x060D },
    { XUNICODE_SCRIPT_ARMENIAN,                0x0559 },
    { XUNICODE_SCRIPT_BENGALI,                 0x09CD },
    { XUNICODE_SCRIPT_BOPOMOFO,                0x31B6 },
    { XUNICODE_SCRIPT_CHEROKEE,                0x13A2 },
    { XUNICODE_SCRIPT_COPTIC,                  0x2CFD },
    { XUNICODE_SCRIPT_CYRILLIC,                0x0482 },
    { XUNICODE_SCRIPT_DESERET,                0x10401 },
    { XUNICODE_SCRIPT_DEVANAGARI,              0x094D },
    { XUNICODE_SCRIPT_ETHIOPIC,                0x1258 },
    { XUNICODE_SCRIPT_GEORGIAN,                0x10FC },
    { XUNICODE_SCRIPT_GOTHIC,                 0x10341 },
    { XUNICODE_SCRIPT_GREEK,                   0x0375 },
    { XUNICODE_SCRIPT_GUJARATI,                0x0A83 },
    { XUNICODE_SCRIPT_GURMUKHI,                0x0A3C },
    { XUNICODE_SCRIPT_HAN,                     0x3005 },
    { XUNICODE_SCRIPT_HANGUL,                  0x1100 },
    { XUNICODE_SCRIPT_HEBREW,                  0x05BF },
    { XUNICODE_SCRIPT_HIRAGANA,                0x309F },
    { XUNICODE_SCRIPT_KANNADA,                 0x0CBC },
    { XUNICODE_SCRIPT_KATAKANA,                0x30FF },
    { XUNICODE_SCRIPT_KHMER,                   0x17DD },
    { XUNICODE_SCRIPT_LAO,                     0x0EDD },
    { XUNICODE_SCRIPT_LATIN,                   0x0061 },
    { XUNICODE_SCRIPT_MALAYALAM,               0x0D3D },
    { XUNICODE_SCRIPT_MONGOLIAN,               0x1843 },
    { XUNICODE_SCRIPT_MYANMAR,                 0x1031 },
    { XUNICODE_SCRIPT_OGHAM,                   0x169C },
    { XUNICODE_SCRIPT_OLD_ITALIC,             0x10322 },
    { XUNICODE_SCRIPT_ORIYA,                   0x0B3C },
    { XUNICODE_SCRIPT_RUNIC,                   0x16EF },
    { XUNICODE_SCRIPT_SINHALA,                 0x0DBD },
    { XUNICODE_SCRIPT_SYRIAC,                  0x0711 },
    { XUNICODE_SCRIPT_TAMIL,                   0x0B82 },
    { XUNICODE_SCRIPT_TELUGU,                  0x0C03 },
    { XUNICODE_SCRIPT_THAANA,                  0x07B1 },
    { XUNICODE_SCRIPT_THAI,                    0x0E31 },
    { XUNICODE_SCRIPT_TIBETAN,                 0x0FD4 },
    { XUNICODE_SCRIPT_CANADIAN_ABORIGINAL,     0x1400 },
    { XUNICODE_SCRIPT_CANADIAN_ABORIGINAL,     0x1401 },
    { XUNICODE_SCRIPT_YI,                      0xA015 },
    { XUNICODE_SCRIPT_TAGALOG,                 0x1700 },
    { XUNICODE_SCRIPT_HANUNOO,                 0x1720 },
    { XUNICODE_SCRIPT_BUHID,                   0x1740 },
    { XUNICODE_SCRIPT_TAGBANWA,                0x1760 },
    { XUNICODE_SCRIPT_BRAILLE,                 0x2800 },
    { XUNICODE_SCRIPT_CYPRIOT,                0x10808 },
    { XUNICODE_SCRIPT_LIMBU,                   0x1932 },
    { XUNICODE_SCRIPT_OSMANYA,                0x10480 },
    { XUNICODE_SCRIPT_SHAVIAN,                0x10450 },
    { XUNICODE_SCRIPT_LINEAR_B,               0x10000 },
    { XUNICODE_SCRIPT_TAI_LE,                  0x1950 },
    { XUNICODE_SCRIPT_UGARITIC,               0x1039F },
    { XUNICODE_SCRIPT_NEW_TAI_LUE,             0x1980 },
    { XUNICODE_SCRIPT_BUGINESE,                0x1A1F },
    { XUNICODE_SCRIPT_GLAGOLITIC,              0x2C00 },
    { XUNICODE_SCRIPT_TIFINAGH,                0x2D6F },
    { XUNICODE_SCRIPT_SYLOTI_NAGRI,            0xA800 },
    { XUNICODE_SCRIPT_OLD_PERSIAN,            0x103D0 },
    { XUNICODE_SCRIPT_KHAROSHTHI,             0x10A3F },
    { XUNICODE_SCRIPT_UNKNOWN,              0x1111111 },
    { XUNICODE_SCRIPT_BALINESE,                0x1B04 },
    { XUNICODE_SCRIPT_CUNEIFORM,              0x12000 },
    { XUNICODE_SCRIPT_PHOENICIAN,             0x10900 },
    { XUNICODE_SCRIPT_PHAGS_PA,                0xA840 },
    { XUNICODE_SCRIPT_NKO,                     0x07C0 },
    { XUNICODE_SCRIPT_KAYAH_LI,                0xA900 },
    { XUNICODE_SCRIPT_LEPCHA,                  0x1C00 },
    { XUNICODE_SCRIPT_REJANG,                  0xA930 },
    { XUNICODE_SCRIPT_SUNDANESE,               0x1B80 },
    { XUNICODE_SCRIPT_SAURASHTRA,              0xA880 },
    { XUNICODE_SCRIPT_CHAM,                    0xAA00 },
    { XUNICODE_SCRIPT_OL_CHIKI,                0x1C50 },
    { XUNICODE_SCRIPT_VAI,                     0xA500 },
    { XUNICODE_SCRIPT_CARIAN,                 0x102A0 },
    { XUNICODE_SCRIPT_LYCIAN,                 0x10280 },
    { XUNICODE_SCRIPT_LYDIAN,                 0x1093F },
    { XUNICODE_SCRIPT_AVESTAN,                0x10B00 },
    { XUNICODE_SCRIPT_BAMUM,                   0xA6A0 },
    { XUNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS,   0x13000 },
    { XUNICODE_SCRIPT_IMPERIAL_ARAMAIC,       0x10840 },
    { XUNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI,  0x10B60 },
    { XUNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN, 0x10B40 },
    { XUNICODE_SCRIPT_JAVANESE,                0xA980 },
    { XUNICODE_SCRIPT_KAITHI,                 0x11082 },
    { XUNICODE_SCRIPT_LISU,                    0xA4D0 },
    { XUNICODE_SCRIPT_MEETEI_MAYEK,            0xABE5 },
    { XUNICODE_SCRIPT_OLD_SOUTH_ARABIAN,      0x10A60 },
    { XUNICODE_SCRIPT_OLD_TURKIC,             0x10C00 },
    { XUNICODE_SCRIPT_SAMARITAN,               0x0800 },
    { XUNICODE_SCRIPT_TAI_THAM,                0x1A20 },
    { XUNICODE_SCRIPT_TAI_VIET,                0xAA80 },
    { XUNICODE_SCRIPT_BATAK,                   0x1BC0 },
    { XUNICODE_SCRIPT_BRAHMI,                 0x11000 },
    { XUNICODE_SCRIPT_MANDAIC,                 0x0840 },
    { XUNICODE_SCRIPT_CHAKMA,                 0x11100 },
    { XUNICODE_SCRIPT_MEROITIC_CURSIVE,       0x109A0 },
    { XUNICODE_SCRIPT_MEROITIC_HIEROGLYPHS,   0x10980 },
    { XUNICODE_SCRIPT_MIAO,                   0x16F00 },
    { XUNICODE_SCRIPT_SHARADA,                0x11180 },
    { XUNICODE_SCRIPT_SORA_SOMPENG,           0x110D0 },
    { XUNICODE_SCRIPT_TAKRI,                  0x11680 },
    { XUNICODE_SCRIPT_BASSA_VAH,              0x16AD0 },
    { XUNICODE_SCRIPT_CAUCASIAN_ALBANIAN,     0x10530 },
    { XUNICODE_SCRIPT_DUPLOYAN,               0x1BC00 },
    { XUNICODE_SCRIPT_ELBASAN,                0x10500 },
    { XUNICODE_SCRIPT_GRANTHA,                0x11301 },
    { XUNICODE_SCRIPT_KHOJKI,                 0x11200 },
    { XUNICODE_SCRIPT_KHUDAWADI,              0x112B0 },
    { XUNICODE_SCRIPT_LINEAR_A,               0x10600 },
    { XUNICODE_SCRIPT_MAHAJANI,               0x11150 },
    { XUNICODE_SCRIPT_MANICHAEAN,             0x10AC0 },
    { XUNICODE_SCRIPT_MENDE_KIKAKUI,          0x1E800 },
    { XUNICODE_SCRIPT_MODI,                   0x11600 },
    { XUNICODE_SCRIPT_MRO,                    0x16A40 },
    { XUNICODE_SCRIPT_NABATAEAN,              0x10880 },
    { XUNICODE_SCRIPT_OLD_NORTH_ARABIAN,      0x10A80 },
    { XUNICODE_SCRIPT_OLD_PERMIC,             0x10350 },
    { XUNICODE_SCRIPT_PAHAWH_HMONG,           0x16B00 },
    { XUNICODE_SCRIPT_PALMYRENE,              0x10860 },
    { XUNICODE_SCRIPT_PAU_CIN_HAU,            0x11AC0 },
    { XUNICODE_SCRIPT_PSALTER_PAHLAVI,        0x10B80 },
    { XUNICODE_SCRIPT_SIDDHAM,                0x11580 },
    { XUNICODE_SCRIPT_TIRHUTA,                0x11480 },
    { XUNICODE_SCRIPT_WARANG_CITI,            0x118A0 },
    { XUNICODE_SCRIPT_CHEROKEE,               0x0AB71 },
    { XUNICODE_SCRIPT_HATRAN,                 0x108E0 },
    { XUNICODE_SCRIPT_OLD_HUNGARIAN,          0x10C80 },
    { XUNICODE_SCRIPT_MULTANI,                0x11280 },
    { XUNICODE_SCRIPT_AHOM,                   0x11700 },
    { XUNICODE_SCRIPT_CUNEIFORM,              0x12480 },
    { XUNICODE_SCRIPT_ANATOLIAN_HIEROGLYPHS,  0x14400 },
    { XUNICODE_SCRIPT_SIGNWRITING,            0x1D800 },
    { XUNICODE_SCRIPT_ADLAM,                  0x1E900 },
    { XUNICODE_SCRIPT_BHAIKSUKI,              0x11C00 },
    { XUNICODE_SCRIPT_MARCHEN,                0x11C70 },
    { XUNICODE_SCRIPT_NEWA,                   0x11400 },
    { XUNICODE_SCRIPT_OSAGE,                  0x104B0 },
    { XUNICODE_SCRIPT_TANGUT,                 0x16FE0 },
    { XUNICODE_SCRIPT_MASARAM_GONDI,          0x11D00 },
    { XUNICODE_SCRIPT_NUSHU,                  0x1B170 },
    { XUNICODE_SCRIPT_SOYOMBO,                0x11A50 },
    { XUNICODE_SCRIPT_ZANABAZAR_SQUARE,       0x11A00 },
    { XUNICODE_SCRIPT_DOGRA,                  0x11800 },
    { XUNICODE_SCRIPT_GUNJALA_GONDI,          0x11D60 },
    { XUNICODE_SCRIPT_HANIFI_ROHINGYA,        0x10D00 },
    { XUNICODE_SCRIPT_MAKASAR,                0x11EE0 },
    { XUNICODE_SCRIPT_MEDEFAIDRIN,            0x16E40 },
    { XUNICODE_SCRIPT_OLD_SOGDIAN,            0x10F00 },
    { XUNICODE_SCRIPT_SOGDIAN,                0x10F30 },
    { XUNICODE_SCRIPT_ELYMAIC,                0x10FE0 },
    { XUNICODE_SCRIPT_NANDINAGARI,            0x119A0 },
    { XUNICODE_SCRIPT_NYIAKENG_PUACHUE_HMONG, 0x1E100 },
    { XUNICODE_SCRIPT_WANCHO,                 0x1E2C0 },
    { XUNICODE_SCRIPT_CHORASMIAN,             0x10FB0 },
    { XUNICODE_SCRIPT_DIVES_AKURU,            0x11900 },
    { XUNICODE_SCRIPT_KHITAN_SMALL_SCRIPT,    0x18B00 },
    { XUNICODE_SCRIPT_YEZIDI,                 0x10E80 },
    { XUNICODE_SCRIPT_CYPRO_MINOAN,           0x12F90 },
    { XUNICODE_SCRIPT_OLD_UYGHUR,             0x10F70 },
    { XUNICODE_SCRIPT_TANGSA,                 0x16A70 },
    { XUNICODE_SCRIPT_TOTO,                   0x1E290 },
    { XUNICODE_SCRIPT_VITHKUQI,               0x10570 }
  };
  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    g_assert_cmpint (xunichar_get_script (examples[i].c), ==, examples[i].script);
}

/* test_t that xunichar_combining_class() returns the correct value for
 * various ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_combining_class (void)
{
  xuint_t i;
  struct {
    xint_t class;
    xunichar_t          c;
  } examples[] = {
    {   0, 0x0020 },
    {   1, 0x0334 },
    {   7, 0x093C },
    {   8, 0x3099 },
    {   9, 0x094D },
    {  10, 0x05B0 },
    {  11, 0x05B1 },
    {  12, 0x05B2 },
    {  13, 0x05B3 },
    {  14, 0x05B4 },
    {  15, 0x05B5 },
    {  16, 0x05B6 },
    {  17, 0x05B7 },
    {  18, 0x05B8 },
    {  19, 0x05B9 },
    {  20, 0x05BB },
    {  21, 0x05BC },
    {  22, 0x05BD },
    {  23, 0x05BF },
    {  24, 0x05C1 },
    {  25, 0x05C2 },
    {  26, 0xFB1E },
    {  27, 0x064B },
    {  28, 0x064C },
    {  29, 0x064D },
    /* ... */
    { 228, 0x05AE },
    { 230, 0x0300 },
    { 232, 0x302C },
    { 233, 0x0362 },
    { 234, 0x0360 },
    { 234, 0x1DCD },
    { 240, 0x0345 }
  };
  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    {
      g_assert_cmpint (xunichar_combining_class (examples[i].c), ==, examples[i].class);
    }
}

/* test_t that xunichar_get_mirror() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_mirror (void)
{
  xunichar_t mirror;

  g_assert_true (xunichar_get_mirror_char ('(', &mirror));
  g_assert_cmpint (mirror, ==, ')');
  g_assert_true (xunichar_get_mirror_char (')', &mirror));
  g_assert_cmpint (mirror, ==, '(');
  g_assert_true (xunichar_get_mirror_char ('{', &mirror));
  g_assert_cmpint (mirror, ==, '}');
  g_assert_true (xunichar_get_mirror_char ('}', &mirror));
  g_assert_cmpint (mirror, ==, '{');
  g_assert_true (xunichar_get_mirror_char (0x208D, &mirror));
  g_assert_cmpint (mirror, ==, 0x208E);
  g_assert_true (xunichar_get_mirror_char (0x208E, &mirror));
  g_assert_cmpint (mirror, ==, 0x208D);
  g_assert_false (xunichar_get_mirror_char ('a', &mirror));
}

/* test_t that xutf8_strup() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_strup (void)
{
  char *str_up = NULL;
  const char *str = "AaZz09x;\x03\x45"
    "\xEF\xBD\x81"  /* Unichar 'A' (U+FF21) */
    "\xEF\xBC\xA1"; /* Unichar 'a' (U+FF41) */

  /* Testing degenerated cases */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str_up = xutf8_strup (NULL, 0);
      g_test_assert_expected_messages ();
    }

  str_up = xutf8_strup (str, strlen (str));
  /* Tricky, comparing two unicode strings with an ASCII function */
  g_assert_cmpstr (str_up, ==, "AAZZ09X;\003E\357\274\241\357\274\241");
  g_free (str_up);
}

/* test_t that xutf8_strdown() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_strdown (void)
{
  char *str_down = NULL;
  const char *str = "AaZz09x;\x03\x07"
    "\xEF\xBD\x81"  /* Unichar 'A' (U+FF21) */
    "\xEF\xBC\xA1"; /* Unichar 'a' (U+FF41) */

  /* Testing degenerated cases */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str_down = xutf8_strdown (NULL, 0);
      g_test_assert_expected_messages ();
    }

  str_down = xutf8_strdown (str, strlen (str));
  /* Tricky, comparing two unicode strings with an ASCII function */
  g_assert_cmpstr (str_down, ==, "aazz09x;\003\007\357\275\201\357\275\201");
  g_free (str_down);
}

/* test_t that xutf8_strup() and xutf8_strdown() return the correct
 * value for Turkish 'i' with and without dot above. */
static void
test_turkish_strupdown (void)
{
  char *str_up = NULL;
  char *str_down = NULL;
  const char *str = "iII"
                    "\xcc\x87"  /* COMBINING DOT ABOVE (U+307) */
                    "\xc4\xb1"  /* LATIN SMALL LETTER DOTLESS I (U+131) */
                    "\xc4\xb0"; /* LATIN CAPITAL LETTER I WITH DOT ABOVE (U+130) */

  char *oldlocale = xstrdup (setlocale (LC_ALL, "tr_TR"));

  if (oldlocale == NULL)
    {
      g_test_skip ("locale tr_TR not available");
      return;
    }

  str_up = xutf8_strup (str, strlen (str));
  str_down = xutf8_strdown (str, strlen (str));
  /* i => LATIN CAPITAL LETTER I WITH DOT ABOVE,
   * I => I,
   * I + COMBINING DOT ABOVE => I + COMBINING DOT ABOVE,
   * LATIN SMALL LETTER DOTLESS I => I,
   * LATIN CAPITAL LETTER I WITH DOT ABOVE => LATIN CAPITAL LETTER I WITH DOT ABOVE */
  g_assert_cmpstr (str_up, ==, "\xc4\xb0II\xcc\x87I\xc4\xb0");
  /* i => i,
   * I => LATIN SMALL LETTER DOTLESS I,
   * I + COMBINING DOT ABOVE => i,
   * LATIN SMALL LETTER DOTLESS I => LATIN SMALL LETTER DOTLESS I,
   * LATIN CAPITAL LETTER I WITH DOT ABOVE => i */
  g_assert_cmpstr (str_down, ==, "i\xc4\xb1i\xc4\xb1i");
  g_free (str_up);
  g_free (str_down);

  setlocale (LC_ALL, oldlocale);
  g_free (oldlocale);
}

/* test_t that xutf8_casefold() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_casefold (void)
{
  char *str_casefold = NULL;
  const char *str = "AaZz09x;"
    "\xEF\xBD\x81"  /* Unichar 'A' (U+FF21) */
    "\xEF\xBC\xA1"; /* Unichar 'a' (U+FF41) */

  /* Testing degenerated cases */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*!= NULL*");
      str_casefold = xutf8_casefold (NULL, 0);
      g_test_assert_expected_messages ();
    }

  str_casefold = xutf8_casefold (str, strlen (str));
  /* Tricky, comparing two unicode strings with an ASCII function */
  g_assert_cmpstr (str_casefold, ==, "aazz09x;\357\275\201\357\275\201");
  g_free (str_casefold);
}

static void
test_casemap_and_casefold (void)
{
  FILE *infile;
  char buffer[1024];
  char **strings;
  char *filename;
  const char *locale;
  const char *test;
  const char *expected;
  char *convert;
  char *current_locale = setlocale (LC_CTYPE, NULL);

  filename = g_test_build_filename (G_TEST_DIST, "casemap.txt", NULL);
  infile = fopen (filename, "r");
  g_assert (infile != NULL);

  while (fgets (buffer, sizeof (buffer), infile))
    {
      if (buffer[0] == '#')
        continue;

      strings = xstrsplit (buffer, "\t", -1);
      locale = strings[0];
      if (!locale[0])
        locale = "C";

      if (strcmp (locale, current_locale) != 0)
        {
          setlocale (LC_CTYPE, locale);
          current_locale = setlocale (LC_CTYPE, NULL);

          if (strncmp (current_locale, locale, 2) != 0)
            {
              g_test_message ("Cannot set locale to %s, skipping", locale);
              goto next;
            }
        }

      test = strings[1];

      /* gen-casemap-txt.py uses an empty string when a single
       * character doesn't have an equivalent in a particular case;
       * since that behavior is nonsense for multicharacter strings,
       * it would make more sense to put the expected result ... the
       * original character unchanged. But for now, we just work
       * around it here and take the empty string to mean "same as
       * original"
       */

      convert = xutf8_strup (test, -1);
      expected = strings[4][0] ? strings[4] : test;
      g_assert_cmpstr (convert, ==, expected);
      g_free (convert);

      convert = xutf8_strdown (test, -1);
      expected = strings[2][0] ? strings[2] : test;
      g_assert_cmpstr (convert, ==, expected);
      g_free (convert);

    next:
      xstrfreev (strings);
    }

  fclose (infile);

  g_free (filename);
  filename = g_test_build_filename (G_TEST_DIST, "casefold.txt", NULL);

  infile = fopen (filename, "r");
  g_assert (infile != NULL);

  while (fgets (buffer, sizeof (buffer), infile))
    {
      if (buffer[0] == '#')
        continue;

      buffer[strlen (buffer) - 1] = '\0';
      strings = xstrsplit (buffer, "\t", -1);

      test = strings[0];

      convert = xutf8_casefold (test, -1);
      g_assert_cmpstr (convert, ==, strings[1]);
      g_free (convert);

      xstrfreev (strings);
    }

  fclose (infile);
  g_free (filename);
}

/* test_t that xunichar_ismark() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_mark (void)
{
  g_assert_true (xunichar_ismark (0x0903));
  g_assert_true (xunichar_ismark (0x20DD));
  g_assert_true (xunichar_ismark (0xA806));
  g_assert_false (xunichar_ismark ('a'));

  /*** Testing TYPE() border cases ***/
  g_assert_false (xunichar_ismark (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_false (xunichar_ismark (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_ismark (0xE0001));
  g_assert_false (xunichar_ismark (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_ismark (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_ismark (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_ismark (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_isspace() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_space (void)
{
  g_assert_false (xunichar_isspace ('a'));
  g_assert_true (xunichar_isspace (' '));
  g_assert_true (xunichar_isspace ('\t'));
  g_assert_true (xunichar_isspace ('\n'));
  g_assert_true (xunichar_isspace ('\r'));
  g_assert_true (xunichar_isspace ('\f'));
  g_assert_false (xunichar_isspace (0xff41)); /* Unicode fullwidth 'a' */
  g_assert_true (xunichar_isspace (0x202F)); /* Unicode space separator */
  g_assert_true (xunichar_isspace (0x2028)); /* Unicode line separator */
  g_assert_true (xunichar_isspace (0x2029)); /* Unicode paragraph separator */

  /*** Testing TYPE() border cases ***/
  g_assert_false (xunichar_isspace (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_false (xunichar_isspace (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_isspace (0xE0001));
  g_assert_false (xunichar_isspace (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_isspace (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_isspace (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_isspace (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_isalnum() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_alnum (void)
{
  g_assert_false (xunichar_isalnum (' '));
  g_assert_true (xunichar_isalnum ('a'));
  g_assert_true (xunichar_isalnum ('z'));
  g_assert_true (xunichar_isalnum ('0'));
  g_assert_true (xunichar_isalnum ('9'));
  g_assert_true (xunichar_isalnum ('A'));
  g_assert_true (xunichar_isalnum ('Z'));
  g_assert_false (xunichar_isalnum ('-'));
  g_assert_false (xunichar_isalnum ('*'));
  g_assert_true (xunichar_isalnum (0xFF21));  /* Unichar fullwidth 'A' */
  g_assert_true (xunichar_isalnum (0xFF3A));  /* Unichar fullwidth 'Z' */
  g_assert_true (xunichar_isalnum (0xFF41));  /* Unichar fullwidth 'a' */
  g_assert_true (xunichar_isalnum (0xFF5A));  /* Unichar fullwidth 'z' */
  g_assert_true (xunichar_isalnum (0xFF10));  /* Unichar fullwidth '0' */
  g_assert_true (xunichar_isalnum (0xFF19));  /* Unichar fullwidth '9' */
  g_assert_false (xunichar_isalnum (0xFF0A)); /* Unichar fullwidth '*' */

  /*** Testing TYPE() border cases ***/
  g_assert_true (xunichar_isalnum (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_false (xunichar_isalnum (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_isalnum (0xE0001));
  g_assert_false (xunichar_isalnum (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_isalnum (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_isalnum (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_isalnum (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_isalpha() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_alpha (void)
{
  g_assert_false (xunichar_isalpha (' '));
  g_assert_true (xunichar_isalpha ('a'));
  g_assert_true (xunichar_isalpha ('z'));
  g_assert_false (xunichar_isalpha ('0'));
  g_assert_false (xunichar_isalpha ('9'));
  g_assert_true (xunichar_isalpha ('A'));
  g_assert_true (xunichar_isalpha ('Z'));
  g_assert_false (xunichar_isalpha ('-'));
  g_assert_false (xunichar_isalpha ('*'));
  g_assert_true (xunichar_isalpha (0xFF21));  /* Unichar fullwidth 'A' */
  g_assert_true (xunichar_isalpha (0xFF3A));  /* Unichar fullwidth 'Z' */
  g_assert_true (xunichar_isalpha (0xFF41));  /* Unichar fullwidth 'a' */
  g_assert_true (xunichar_isalpha (0xFF5A));  /* Unichar fullwidth 'z' */
  g_assert_false (xunichar_isalpha (0xFF10)); /* Unichar fullwidth '0' */
  g_assert_false (xunichar_isalpha (0xFF19)); /* Unichar fullwidth '9' */
  g_assert_false (xunichar_isalpha (0xFF0A)); /* Unichar fullwidth '*' */

  /*** Testing TYPE() border cases ***/
  g_assert_true (xunichar_isalpha (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_false (xunichar_isalpha (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_isalpha (0xE0001));
  g_assert_false (xunichar_isalpha (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_isalpha (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_isalpha (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_isalpha (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_isdigit() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_digit (void)
{
  g_assert_false (xunichar_isdigit (' '));
  g_assert_false (xunichar_isdigit ('a'));
  g_assert_true (xunichar_isdigit ('0'));
  g_assert_true (xunichar_isdigit ('9'));
  g_assert_false (xunichar_isdigit ('A'));
  g_assert_false (xunichar_isdigit ('-'));
  g_assert_false (xunichar_isdigit ('*'));
  g_assert_false (xunichar_isdigit (0xFF21)); /* Unichar fullwidth 'A' */
  g_assert_false (xunichar_isdigit (0xFF3A)); /* Unichar fullwidth 'Z' */
  g_assert_false (xunichar_isdigit (0xFF41)); /* Unichar fullwidth 'a' */
  g_assert_false (xunichar_isdigit (0xFF5A)); /* Unichar fullwidth 'z' */
  g_assert_true (xunichar_isdigit (0xFF10));  /* Unichar fullwidth '0' */
  g_assert_true (xunichar_isdigit (0xFF19));  /* Unichar fullwidth '9' */
  g_assert_false (xunichar_isdigit (0xFF0A)); /* Unichar fullwidth '*' */

  /*** Testing TYPE() border cases ***/
  g_assert_false (xunichar_isdigit (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_false (xunichar_isdigit (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_isdigit (0xE0001));
  g_assert_false (xunichar_isdigit (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_isdigit (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_isdigit (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_isdigit (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_digit_value() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_digit_value (void)
{
  g_assert_cmpint (xunichar_digit_value (' '), ==, -1);
  g_assert_cmpint (xunichar_digit_value ('a'), ==, -1);
  g_assert_cmpint (xunichar_digit_value ('0'), ==, 0);
  g_assert_cmpint (xunichar_digit_value ('9'), ==, 9);
  g_assert_cmpint (xunichar_digit_value ('A'), ==, -1);
  g_assert_cmpint (xunichar_digit_value ('-'), ==, -1);
  g_assert_cmpint (xunichar_digit_value (0xFF21), ==, -1); /* Unichar 'A' */
  g_assert_cmpint (xunichar_digit_value (0xFF3A), ==, -1); /* Unichar 'Z' */
  g_assert_cmpint (xunichar_digit_value (0xFF41), ==, -1); /* Unichar 'a' */
  g_assert_cmpint (xunichar_digit_value (0xFF5A), ==, -1); /* Unichar 'z' */
  g_assert_cmpint (xunichar_digit_value (0xFF10), ==, 0);  /* Unichar '0' */
  g_assert_cmpint (xunichar_digit_value (0xFF19), ==, 9);  /* Unichar '9' */
  g_assert_cmpint (xunichar_digit_value (0xFF0A), ==, -1); /* Unichar '*' */

  /*** Testing TYPE() border cases ***/
  g_assert_cmpint (xunichar_digit_value (0x3FF5), ==, -1);
   /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_cmpint (xunichar_digit_value (0xFFEFF), ==, -1);
  /* U+E0001 Language Tag */
  g_assert_cmpint (xunichar_digit_value (0xE0001), ==, -1);
  g_assert_cmpint (xunichar_digit_value (XUNICODE_LAST_CHAR), ==, -1);
  g_assert_cmpint (xunichar_digit_value (XUNICODE_LAST_CHAR + 1), ==, -1);
  g_assert_cmpint (xunichar_digit_value (XUNICODE_LAST_CHAR_PART1), ==, -1);
  g_assert_cmpint (xunichar_digit_value (XUNICODE_LAST_CHAR_PART1 + 1), ==, -1);
}

/* test_t that xunichar_isxdigit() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_xdigit (void)
{
  g_assert_false (xunichar_isxdigit (' '));
  g_assert_true (xunichar_isxdigit ('a'));
  g_assert_true (xunichar_isxdigit ('f'));
  g_assert_false (xunichar_isxdigit ('g'));
  g_assert_false (xunichar_isxdigit ('z'));
  g_assert_true (xunichar_isxdigit ('0'));
  g_assert_true (xunichar_isxdigit ('9'));
  g_assert_true (xunichar_isxdigit ('A'));
  g_assert_true (xunichar_isxdigit ('F'));
  g_assert_false (xunichar_isxdigit ('G'));
  g_assert_false (xunichar_isxdigit ('Z'));
  g_assert_false (xunichar_isxdigit ('-'));
  g_assert_false (xunichar_isxdigit ('*'));
  g_assert_true (xunichar_isxdigit (0xFF21));  /* Unichar fullwidth 'A' */
  g_assert_true (xunichar_isxdigit (0xFF26));  /* Unichar fullwidth 'F' */
  g_assert_false (xunichar_isxdigit (0xFF27)); /* Unichar fullwidth 'G' */
  g_assert_false (xunichar_isxdigit (0xFF3A)); /* Unichar fullwidth 'Z' */
  g_assert_true (xunichar_isxdigit (0xFF41));  /* Unichar fullwidth 'a' */
  g_assert_true (xunichar_isxdigit (0xFF46));  /* Unichar fullwidth 'f' */
  g_assert_false (xunichar_isxdigit (0xFF47)); /* Unichar fullwidth 'g' */
  g_assert_false (xunichar_isxdigit (0xFF5A)); /* Unichar fullwidth 'z' */
  g_assert_true (xunichar_isxdigit (0xFF10));  /* Unichar fullwidth '0' */
  g_assert_true (xunichar_isxdigit (0xFF19));  /* Unichar fullwidth '9' */
  g_assert_false (xunichar_isxdigit (0xFF0A)); /* Unichar fullwidth '*' */

  /*** Testing TYPE() border cases ***/
  g_assert_false (xunichar_isxdigit (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_false (xunichar_isxdigit (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_isxdigit (0xE0001));
  g_assert_false (xunichar_isxdigit (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_isxdigit (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_isxdigit (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_isxdigit (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_xdigit_value() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_xdigit_value (void)
{
  g_assert_cmpint (xunichar_xdigit_value (' '), ==, -1);
  g_assert_cmpint (xunichar_xdigit_value ('a'), ==, 10);
  g_assert_cmpint (xunichar_xdigit_value ('f'), ==, 15);
  g_assert_cmpint (xunichar_xdigit_value ('g'), ==, -1);
  g_assert_cmpint (xunichar_xdigit_value ('0'), ==, 0);
  g_assert_cmpint (xunichar_xdigit_value ('9'), ==, 9);
  g_assert_cmpint (xunichar_xdigit_value ('A'), ==, 10);
  g_assert_cmpint (xunichar_xdigit_value ('F'), ==, 15);
  g_assert_cmpint (xunichar_xdigit_value ('G'), ==, -1);
  g_assert_cmpint (xunichar_xdigit_value ('-'), ==, -1);
  g_assert_cmpint (xunichar_xdigit_value (0xFF21), ==, 10); /* Unichar 'A' */
  g_assert_cmpint (xunichar_xdigit_value (0xFF26), ==, 15); /* Unichar 'F' */
  g_assert_cmpint (xunichar_xdigit_value (0xFF27), ==, -1); /* Unichar 'G' */
  g_assert_cmpint (xunichar_xdigit_value (0xFF3A), ==, -1); /* Unichar 'Z' */
  g_assert_cmpint (xunichar_xdigit_value (0xFF41), ==, 10); /* Unichar 'a' */
  g_assert_cmpint (xunichar_xdigit_value (0xFF46), ==, 15); /* Unichar 'f' */
  g_assert_cmpint (xunichar_xdigit_value (0xFF47), ==, -1); /* Unichar 'g' */
  g_assert_cmpint (xunichar_xdigit_value (0xFF5A), ==, -1); /* Unichar 'z' */
  g_assert_cmpint (xunichar_xdigit_value (0xFF10), ==, 0);  /* Unichar '0' */
  g_assert_cmpint (xunichar_xdigit_value (0xFF19), ==, 9);  /* Unichar '9' */
  g_assert_cmpint (xunichar_xdigit_value (0xFF0A), ==, -1); /* Unichar '*' */

  /*** Testing TYPE() border cases ***/
  g_assert_cmpint (xunichar_xdigit_value (0x3FF5), ==, -1);
   /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_cmpint (xunichar_xdigit_value (0xFFEFF), ==, -1);
  /* U+E0001 Language Tag */
  g_assert_cmpint (xunichar_xdigit_value (0xE0001), ==, -1);
  g_assert_cmpint (xunichar_xdigit_value (XUNICODE_LAST_CHAR), ==, -1);
  g_assert_cmpint (xunichar_xdigit_value (XUNICODE_LAST_CHAR + 1), ==, -1);
  g_assert_cmpint (xunichar_xdigit_value (XUNICODE_LAST_CHAR_PART1), ==, -1);
  g_assert_cmpint (xunichar_xdigit_value (XUNICODE_LAST_CHAR_PART1 + 1), ==, -1);
}

/* test_t that xunichar_ispunct() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_punctuation (void)
{
  g_assert_false (xunichar_ispunct (' '));
  g_assert_false (xunichar_ispunct ('a'));
  g_assert_true (xunichar_ispunct ('.'));
  g_assert_true (xunichar_ispunct (','));
  g_assert_true (xunichar_ispunct (';'));
  g_assert_true (xunichar_ispunct (':'));
  g_assert_true (xunichar_ispunct ('-'));

  g_assert_false (xunichar_ispunct (0xFF21)); /* Unichar fullwidth 'A' */
  g_assert_true (xunichar_ispunct (0x005F));  /* Unichar fullwidth '.' */
  g_assert_true (xunichar_ispunct (0x058A));  /* Unichar fullwidth '-' */

  /*** Testing TYPE() border cases ***/
  g_assert_false (xunichar_ispunct (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_false (xunichar_ispunct (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_ispunct (0xE0001));
  g_assert_false (xunichar_ispunct (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_ispunct (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_ispunct (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_ispunct (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_iscntrl() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_cntrl (void)
{
  g_assert_true (xunichar_iscntrl (0x08));
  g_assert_false (xunichar_iscntrl ('a'));
  g_assert_true (xunichar_iscntrl (0x007F)); /* Unichar fullwidth <del> */
  g_assert_true (xunichar_iscntrl (0x009F)); /* Unichar fullwidth control */

  /*** Testing TYPE() border cases ***/
  g_assert_false (xunichar_iscntrl (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_false (xunichar_iscntrl (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_iscntrl (0xE0001));
  g_assert_false (xunichar_iscntrl (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_iscntrl (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_iscntrl (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_iscntrl (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_isgraph() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_graph (void)
{
  g_assert_false (xunichar_isgraph (0x08));
  g_assert_false (xunichar_isgraph (' '));
  g_assert_true (xunichar_isgraph ('a'));
  g_assert_true (xunichar_isgraph ('0'));
  g_assert_true (xunichar_isgraph ('9'));
  g_assert_true (xunichar_isgraph ('A'));
  g_assert_true (xunichar_isgraph ('-'));
  g_assert_true (xunichar_isgraph ('*'));
  g_assert_true (xunichar_isgraph (0xFF21));  /* Unichar fullwidth 'A' */
  g_assert_true (xunichar_isgraph (0xFF3A));  /* Unichar fullwidth 'Z' */
  g_assert_true (xunichar_isgraph (0xFF41));  /* Unichar fullwidth 'a' */
  g_assert_true (xunichar_isgraph (0xFF5A));  /* Unichar fullwidth 'z' */
  g_assert_true (xunichar_isgraph (0xFF10));  /* Unichar fullwidth '0' */
  g_assert_true (xunichar_isgraph (0xFF19));  /* Unichar fullwidth '9' */
  g_assert_true (xunichar_isgraph (0xFF0A));  /* Unichar fullwidth '*' */
  g_assert_false (xunichar_isgraph (0x007F)); /* Unichar fullwidth <del> */
  g_assert_false (xunichar_isgraph (0x009F)); /* Unichar fullwidth control */

  /*** Testing TYPE() border cases ***/
  g_assert_true (xunichar_isgraph (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_true (xunichar_isgraph (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_isgraph (0xE0001));
  g_assert_false (xunichar_isgraph (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_isgraph (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_isgraph (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_isgraph (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_iszerowidth() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_zerowidth (void)
{
  g_assert_false (xunichar_iszerowidth (0x00AD));
  g_assert_false (xunichar_iszerowidth (0x115F));
  g_assert_true (xunichar_iszerowidth (0x1160));
  g_assert_true (xunichar_iszerowidth (0x11AA));
  g_assert_true (xunichar_iszerowidth (0x11FF));
  g_assert_false (xunichar_iszerowidth (0x1200));
  g_assert_false (xunichar_iszerowidth (0x200A));
  g_assert_true (xunichar_iszerowidth (0x200B));
  g_assert_true (xunichar_iszerowidth (0x200C));
  g_assert_true (xunichar_iszerowidth (0x591));

  /*** Testing TYPE() border cases ***/
  g_assert_false (xunichar_iszerowidth (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_false (xunichar_iszerowidth (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_true (xunichar_iszerowidth (0xE0001));
  g_assert_false (xunichar_iszerowidth (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_iszerowidth (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_iszerowidth (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_iszerowidth (XUNICODE_LAST_CHAR_PART1 + 1));

  /* Hangul Jamo Extended-B block, containing jungseong and jongseong for
   * Old Korean */
  g_assert_true (xunichar_iszerowidth (0xD7B0));
  g_assert_true (xunichar_iszerowidth (0xD7FB));
}

/* test_t that xunichar_istitle() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_title (void)
{
  g_assert_true (xunichar_istitle (0x01c5));
  g_assert_true (xunichar_istitle (0x1f88));
  g_assert_true (xunichar_istitle (0x1fcc));
  g_assert_false (xunichar_istitle ('a'));
  g_assert_false (xunichar_istitle ('A'));
  g_assert_false (xunichar_istitle (';'));

  /*** Testing TYPE() border cases ***/
  g_assert_false (xunichar_istitle (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_false (xunichar_istitle (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_istitle (0xE0001));
  g_assert_false (xunichar_istitle (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_istitle (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_istitle (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_istitle (XUNICODE_LAST_CHAR_PART1 + 1));

  g_assert_cmphex (xunichar_totitle (0x0000), ==, 0x0000);
  g_assert_cmphex (xunichar_totitle (0x01c6), ==, 0x01c5);
  g_assert_cmphex (xunichar_totitle (0x01c4), ==, 0x01c5);
  g_assert_cmphex (xunichar_totitle (0x01c5), ==, 0x01c5);
  g_assert_cmphex (xunichar_totitle (0x1f80), ==, 0x1f88);
  g_assert_cmphex (xunichar_totitle (0x1f88), ==, 0x1f88);
  g_assert_cmphex (xunichar_totitle ('a'), ==, 'A');
  g_assert_cmphex (xunichar_totitle ('A'), ==, 'A');

  /*** Testing TYPE() border cases ***/
  g_assert_cmphex (xunichar_totitle (0x3FF5), ==, 0x3FF5);
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_cmphex (xunichar_totitle (0xFFEFF), ==, 0xFFEFF);
  g_assert_cmphex (xunichar_totitle (0xDFFFF), ==, 0xDFFFF);
  /* U+E0001 Language Tag */
  g_assert_cmphex (xunichar_totitle (0xE0001), ==, 0xE0001);
  g_assert_cmphex (xunichar_totitle (XUNICODE_LAST_CHAR), ==,
                   XUNICODE_LAST_CHAR);
  g_assert_cmphex (xunichar_totitle (XUNICODE_LAST_CHAR + 1), ==,
                   (XUNICODE_LAST_CHAR + 1));
  g_assert_cmphex (xunichar_totitle (XUNICODE_LAST_CHAR_PART1), ==,
                   (XUNICODE_LAST_CHAR_PART1));
  g_assert_cmphex (xunichar_totitle (XUNICODE_LAST_CHAR_PART1 + 1), ==,
                   (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_isupper() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_upper (void)
{
  g_assert_false (xunichar_isupper (' '));
  g_assert_false (xunichar_isupper ('0'));
  g_assert_false (xunichar_isupper ('a'));
  g_assert_true (xunichar_isupper ('A'));
  g_assert_false (xunichar_isupper (0xff41)); /* Unicode fullwidth 'a' */
  g_assert_true (xunichar_isupper (0xff21)); /* Unicode fullwidth 'A' */

  /*** Testing TYPE() border cases ***/
  g_assert_false (xunichar_isupper (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_false (xunichar_isupper (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_isupper (0xE0001));
  g_assert_false (xunichar_isupper (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_isupper (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_isupper (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_isupper (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_islower() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_lower (void)
{
  g_assert_false (xunichar_islower (' '));
  g_assert_false (xunichar_islower ('0'));
  g_assert_true (xunichar_islower ('a'));
  g_assert_false (xunichar_islower ('A'));
  g_assert_true (xunichar_islower (0xff41)); /* Unicode fullwidth 'a' */
  g_assert_false (xunichar_islower (0xff21)); /* Unicode fullwidth 'A' */

  /*** Testing TYPE() border cases ***/
  g_assert_false (xunichar_islower (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_false (xunichar_islower (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_islower (0xE0001));
  g_assert_false (xunichar_islower (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_islower (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_islower (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_islower (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_isprint() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_print (void)
{
  g_assert_true (xunichar_isprint (' '));
  g_assert_true (xunichar_isprint ('0'));
  g_assert_true (xunichar_isprint ('a'));
  g_assert_true (xunichar_isprint ('A'));
  g_assert_true (xunichar_isprint (0xff41)); /* Unicode fullwidth 'a' */
  g_assert_true (xunichar_isprint (0xff21)); /* Unicode fullwidth 'A' */

  /*** Testing TYPE() border cases ***/
  g_assert_true (xunichar_isprint (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_true (xunichar_isprint (0xFFEFF));
  /* U+E0001 Language Tag */
  g_assert_false (xunichar_isprint (0xE0001));
  g_assert_false (xunichar_isprint (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_isprint (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_isprint (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_isprint (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_toupper() and xunichar_tolower() return the
 * correct values for various ASCII and Unicode alphabetic, numeric,
 * and other, codepoints. */
static void
test_cases (void)
{
  g_assert_cmphex (xunichar_toupper (0x0), ==, 0x0);
  g_assert_cmphex (xunichar_tolower (0x0), ==, 0x0);
  g_assert_cmphex (xunichar_toupper ('a'), ==, 'A');
  g_assert_cmphex (xunichar_toupper ('A'), ==, 'A');
  /* Unicode fullwidth 'a' == 'A' */
  g_assert_cmphex (xunichar_toupper (0xff41), ==, 0xff21);
  /* Unicode fullwidth 'A' == 'A' */
  g_assert_cmphex (xunichar_toupper (0xff21), ==, 0xff21);
  g_assert_cmphex (xunichar_toupper (0x01C5), ==, 0x01C4);
  g_assert_cmphex (xunichar_toupper (0x01C6), ==, 0x01C4);
  g_assert_cmphex (xunichar_tolower ('A'), ==, 'a');
  g_assert_cmphex (xunichar_tolower ('a'), ==, 'a');
  /* Unicode fullwidth 'A' == 'a' */
  g_assert_cmphex (xunichar_tolower (0xff21), ==, 0xff41);
  /* Unicode fullwidth 'a' == 'a' */
  g_assert_cmphex (xunichar_tolower (0xff41), ==, 0xff41);
  g_assert_cmphex (xunichar_tolower (0x01C4), ==, 0x01C6);
  g_assert_cmphex (xunichar_tolower (0x01C5), ==, 0x01C6);
  g_assert_cmphex (xunichar_tolower (0x1F8A), ==, 0x1F82);
  g_assert_cmphex (xunichar_totitle (0x1F8A), ==, 0x1F8A);
  g_assert_cmphex (xunichar_toupper (0x1F8A), ==, 0x1F8A);
  g_assert_cmphex (xunichar_tolower (0x1FB2), ==, 0x1FB2);
  g_assert_cmphex (xunichar_toupper (0x1FB2), ==, 0x1FB2);

  /* U+130 is a special case, it's a 'I' with a dot on top */
  g_assert_cmphex (xunichar_tolower (0x130), ==, 0x69);

  /* Testing ATTTABLE() border cases */
  g_assert_cmphex (xunichar_toupper (0x1D6FE), ==, 0x1D6FE);

  /*** Testing TYPE() border cases ***/
  g_assert_cmphex (xunichar_toupper (0x3FF5), ==, 0x3FF5);
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_cmphex (xunichar_toupper (0xFFEFF), ==, 0xFFEFF);
  g_assert_cmphex (xunichar_toupper (0xDFFFF), ==, 0xDFFFF);
  /* U+E0001 Language Tag */
  g_assert_cmphex (xunichar_toupper (0xE0001), ==, 0xE0001);
  g_assert_cmphex (xunichar_toupper (XUNICODE_LAST_CHAR), ==,
                   XUNICODE_LAST_CHAR);
  g_assert_cmphex (xunichar_toupper (XUNICODE_LAST_CHAR + 1), ==,
                   (XUNICODE_LAST_CHAR + 1));
  g_assert_cmphex (xunichar_toupper (XUNICODE_LAST_CHAR_PART1), ==,
                   (XUNICODE_LAST_CHAR_PART1));
  g_assert_cmphex (xunichar_toupper (XUNICODE_LAST_CHAR_PART1 + 1), ==,
                   (XUNICODE_LAST_CHAR_PART1 + 1));

  /* Testing ATTTABLE() border cases */
  g_assert_cmphex (xunichar_tolower (0x1D6FA), ==, 0x1D6FA);

  /*** Testing TYPE() border cases ***/
  g_assert_cmphex (xunichar_tolower (0x3FF5), ==, 0x3FF5);
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_cmphex (xunichar_tolower (0xFFEFF), ==, 0xFFEFF);
  g_assert_cmphex (xunichar_tolower (0xDFFFF), ==, 0xDFFFF);
  /* U+E0001 Language Tag */
  g_assert_cmphex (xunichar_tolower (0xE0001), ==, 0xE0001);
  g_assert_cmphex (xunichar_tolower (XUNICODE_LAST_CHAR), ==,
                   XUNICODE_LAST_CHAR);
  g_assert_cmphex (xunichar_tolower (XUNICODE_LAST_CHAR + 1), ==,
                   (XUNICODE_LAST_CHAR + 1));
  g_assert_cmphex (xunichar_tolower (XUNICODE_LAST_CHAR_PART1), ==,
                   XUNICODE_LAST_CHAR_PART1);
  g_assert_cmphex (xunichar_tolower (XUNICODE_LAST_CHAR_PART1 + 1), ==,
                   (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_isdefined() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_defined (void)
{
  g_assert_true (xunichar_isdefined (0x0903));
  g_assert_true (xunichar_isdefined (0x20DD));
  g_assert_true (xunichar_isdefined (0x20BA));
  g_assert_true (xunichar_isdefined (0xA806));
  g_assert_true (xunichar_isdefined ('a'));
  g_assert_false (xunichar_isdefined (0x10C49));
  g_assert_false (xunichar_isdefined (0x169D));

  /*** Testing TYPE() border cases ***/
  g_assert_true (xunichar_isdefined (0x3FF5));
  /* U+FFEFF Plane 15 Private Use (needed to be > XUNICODE_MAX_TABLE_INDEX) */
  g_assert_true (xunichar_isdefined (0xFFEFF));
  g_assert_false (xunichar_isdefined (0xDFFFF));
  /* U+E0001 Language Tag */
  g_assert_true (xunichar_isdefined (0xE0001));
  g_assert_false (xunichar_isdefined (XUNICODE_LAST_CHAR));
  g_assert_false (xunichar_isdefined (XUNICODE_LAST_CHAR + 1));
  g_assert_false (xunichar_isdefined (XUNICODE_LAST_CHAR_PART1));
  g_assert_false (xunichar_isdefined (XUNICODE_LAST_CHAR_PART1 + 1));
}

/* test_t that xunichar_iswide() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_wide (void)
{
  xuint_t i;
  struct {
    xunichar_t c;
    enum {
      NOT_WIDE,
      WIDE_CJK,
      WIDE
    } wide;
  } examples[] = {
    /* Neutral */
    {   0x0000, NOT_WIDE },
    {   0x0483, NOT_WIDE },
    {   0x0641, NOT_WIDE },
    {   0xFFFC, NOT_WIDE },
    {  0x10000, NOT_WIDE },
    {  0xE0001, NOT_WIDE },
    {  0x2FFFE, NOT_WIDE },
    {  0x3FFFE, NOT_WIDE },

    /* Narrow */
    {   0x0020, NOT_WIDE },
    {   0x0041, NOT_WIDE },
    {   0x27E6, NOT_WIDE },

    /* Halfwidth */
    {   0x20A9, NOT_WIDE },
    {   0xFF61, NOT_WIDE },
    {   0xFF69, NOT_WIDE },
    {   0xFFEE, NOT_WIDE },

    /* Ambiguous */
    {   0x00A1, WIDE_CJK },
    {   0x00BE, WIDE_CJK },
    {   0x02DD, WIDE_CJK },
    {   0x2020, WIDE_CJK },
    {   0xFFFD, WIDE_CJK },
    {   0x00A1, WIDE_CJK },
    {  0x1F100, WIDE_CJK },
    {  0xE0100, WIDE_CJK },
    { 0x100000, WIDE_CJK },
    { 0x10FFFD, WIDE_CJK },

    /* Fullwidth */
    {   0x3000, WIDE },
    {   0xFF60, WIDE },

    /* Wide */
    {   0x2329, WIDE },
    {   0x3001, WIDE },
    {   0xFE69, WIDE },
    {  0x30000, WIDE },
    {  0x3FFFD, WIDE },

    /* Default Wide blocks */
    {   0x4DBF, WIDE },
    {   0x9FFF, WIDE },
    {   0xFAFF, WIDE },
    {  0x2A6DF, WIDE },
    {  0x2B73F, WIDE },
    {  0x2B81F, WIDE },
    {  0x2FA1F, WIDE },

    /* Uniode-5.2 character additions */
    /* Wide */
    {   0x115F, WIDE },

    /* Uniode-6.0 character additions */
    /* Wide */
    {  0x2B740, WIDE },
    {  0x1B000, WIDE },

    { 0x111111, NOT_WIDE }
  };

  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    {
      g_assert_cmpint (xunichar_iswide (examples[i].c), ==,
                       (examples[i].wide == WIDE));
      g_assert_cmpint (xunichar_iswide_cjk (examples[i].c), ==,
                       (examples[i].wide != NOT_WIDE));
    }
};

/* test_t that xunichar_compose() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_compose (void)
{
  xunichar_t ch;

  /* Not composable */
  g_assert_false (xunichar_compose (0x0041, 0x0042, &ch) && ch == 0);
  g_assert_false (xunichar_compose (0x0041, 0, &ch) && ch == 0);
  g_assert_false (xunichar_compose (0x0066, 0x0069, &ch) && ch == 0);

  /* Tricky non-composable */
  g_assert_false (xunichar_compose (0x0308, 0x0301, &ch) && ch == 0); /* !0x0344 */
  g_assert_false (xunichar_compose (0x0F71, 0x0F72, &ch) && ch == 0); /* !0x0F73 */

  /* Singletons should not compose */
  g_assert_false (xunichar_compose (0x212B, 0, &ch) && ch == 0);
  g_assert_false (xunichar_compose (0x00C5, 0, &ch) && ch == 0);
  g_assert_false (xunichar_compose (0x2126, 0, &ch) && ch == 0);
  g_assert_false (xunichar_compose (0x03A9, 0, &ch) && ch == 0);

  /* Pairs */
  g_assert_true (xunichar_compose (0x0041, 0x030A, &ch) && ch == 0x00C5);
  g_assert_true (xunichar_compose (0x006F, 0x0302, &ch) && ch == 0x00F4);
  g_assert_true (xunichar_compose (0x1E63, 0x0307, &ch) && ch == 0x1E69);
  g_assert_true (xunichar_compose (0x0073, 0x0323, &ch) && ch == 0x1E63);
  g_assert_true (xunichar_compose (0x0064, 0x0307, &ch) && ch == 0x1E0B);
  g_assert_true (xunichar_compose (0x0064, 0x0323, &ch) && ch == 0x1E0D);

  /* Hangul */
  g_assert_true (xunichar_compose (0xD4CC, 0x11B6, &ch) && ch == 0xD4DB);
  g_assert_true (xunichar_compose (0x1111, 0x1171, &ch) && ch == 0xD4CC);
  g_assert_true (xunichar_compose (0xCE20, 0x11B8, &ch) && ch == 0xCE31);
  g_assert_true (xunichar_compose (0x110E, 0x1173, &ch) && ch == 0xCE20);
}

/* test_t that xunichar_decompose() returns the correct value for various
 * ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_decompose (void)
{
  xunichar_t a, b;

  /* Not decomposable */
  g_assert_false (xunichar_decompose (0x0041, &a, &b) && a == 0x0041 && b == 0);
  g_assert_false (xunichar_decompose (0xFB01, &a, &b) && a == 0xFB01 && b == 0);

  /* Singletons */
  g_assert_true (xunichar_decompose (0x212B, &a, &b) && a == 0x00C5 && b == 0);
  g_assert_true (xunichar_decompose (0x2126, &a, &b) && a == 0x03A9 && b == 0);

  /* Tricky pairs */
  g_assert_true (xunichar_decompose (0x0344, &a, &b) && a == 0x0308 && b == 0x0301);
  g_assert_true (xunichar_decompose (0x0F73, &a, &b) && a == 0x0F71 && b == 0x0F72);

  /* Pairs */
  g_assert_true (xunichar_decompose (0x00C5, &a, &b) && a == 0x0041 && b == 0x030A);
  g_assert_true (xunichar_decompose (0x00F4, &a, &b) && a == 0x006F && b == 0x0302);
  g_assert_true (xunichar_decompose (0x1E69, &a, &b) && a == 0x1E63 && b == 0x0307);
  g_assert_true (xunichar_decompose (0x1E63, &a, &b) && a == 0x0073 && b == 0x0323);
  g_assert_true (xunichar_decompose (0x1E0B, &a, &b) && a == 0x0064 && b == 0x0307);
  g_assert_true (xunichar_decompose (0x1E0D, &a, &b) && a == 0x0064 && b == 0x0323);

  /* Hangul */
  g_assert_true (xunichar_decompose (0xD4DB, &a, &b) && a == 0xD4CC && b == 0x11B6);
  g_assert_true (xunichar_decompose (0xD4CC, &a, &b) && a == 0x1111 && b == 0x1171);
  g_assert_true (xunichar_decompose (0xCE31, &a, &b) && a == 0xCE20 && b == 0x11B8);
  g_assert_true (xunichar_decompose (0xCE20, &a, &b) && a == 0x110E && b == 0x1173);
}

/* test_t that xunichar_fully_decompose() returns the correct value for
 * various ASCII and Unicode alphabetic, numeric, and other, codepoints. */
static void
test_fully_decompose_canonical (void)
{
  xunichar_t decomp[5];
  xsize_t len;

#define TEST_DECOMP(ch, expected_len, a, b, c, d) \
  len = xunichar_fully_decompose (ch, FALSE, decomp, G_N_ELEMENTS (decomp)); \
  g_assert_cmpint (expected_len, ==, len); \
  if (expected_len >= 1) g_assert_cmphex (decomp[0], ==, a); \
  if (expected_len >= 2) g_assert_cmphex (decomp[1], ==, b); \
  if (expected_len >= 3) g_assert_cmphex (decomp[2], ==, c); \
  if (expected_len >= 4) g_assert_cmphex (decomp[3], ==, d); \

#define TEST0(ch)		TEST_DECOMP (ch, 1, ch, 0, 0, 0)
#define TEST1(ch, a)		TEST_DECOMP (ch, 1, a, 0, 0, 0)
#define TEST2(ch, a, b)		TEST_DECOMP (ch, 2, a, b, 0, 0)
#define TEST3(ch, a, b, c)	TEST_DECOMP (ch, 3, a, b, c, 0)
#define TEST4(ch, a, b, c, d)	TEST_DECOMP (ch, 4, a, b, c, d)

  /* Not decomposable */
  TEST0 (0x0041);
  TEST0 (0xFB01);

  /* Singletons */
  TEST2 (0x212B, 0x0041, 0x030A);
  TEST1 (0x2126, 0x03A9);

  /* Tricky pairs */
  TEST2 (0x0344, 0x0308, 0x0301);
  TEST2 (0x0F73, 0x0F71, 0x0F72);

  /* General */
  TEST2 (0x00C5, 0x0041, 0x030A);
  TEST2 (0x00F4, 0x006F, 0x0302);
  TEST3 (0x1E69, 0x0073, 0x0323, 0x0307);
  TEST2 (0x1E63, 0x0073, 0x0323);
  TEST2 (0x1E0B, 0x0064, 0x0307);
  TEST2 (0x1E0D, 0x0064, 0x0323);

  /* Hangul */
  TEST3 (0xD4DB, 0x1111, 0x1171, 0x11B6);
  TEST2 (0xD4CC, 0x1111, 0x1171);
  TEST3 (0xCE31, 0x110E, 0x1173, 0x11B8);
  TEST2 (0xCE20, 0x110E, 0x1173);

#undef TEST_DECOMP
}

/* test_t that xunicode_canonical_decomposition() returns the correct
 * value for various ASCII and Unicode alphabetic, numeric, and other,
 * codepoints. */
static void
test_canonical_decomposition (void)
{
  xunichar_t *decomp;
  xsize_t len;

#define TEST_DECOMP(ch, expected_len, a, b, c, d) \
  decomp = xunicode_canonical_decomposition (ch, &len); \
  g_assert_cmpint (expected_len, ==, len); \
  if (expected_len >= 1) g_assert_cmphex (decomp[0], ==, a); \
  if (expected_len >= 2) g_assert_cmphex (decomp[1], ==, b); \
  if (expected_len >= 3) g_assert_cmphex (decomp[2], ==, c); \
  if (expected_len >= 4) g_assert_cmphex (decomp[3], ==, d); \
  g_free (decomp);

#define TEST0(ch)		TEST_DECOMP (ch, 1, ch, 0, 0, 0)
#define TEST1(ch, a)		TEST_DECOMP (ch, 1, a, 0, 0, 0)
#define TEST2(ch, a, b)		TEST_DECOMP (ch, 2, a, b, 0, 0)
#define TEST3(ch, a, b, c)	TEST_DECOMP (ch, 3, a, b, c, 0)
#define TEST4(ch, a, b, c, d)	TEST_DECOMP (ch, 4, a, b, c, d)

  /* Not decomposable */
  TEST0 (0x0041);
  TEST0 (0xFB01);

  /* Singletons */
  TEST2 (0x212B, 0x0041, 0x030A);
  TEST1 (0x2126, 0x03A9);

  /* Tricky pairs */
  TEST2 (0x0344, 0x0308, 0x0301);
  TEST2 (0x0F73, 0x0F71, 0x0F72);

  /* General */
  TEST2 (0x00C5, 0x0041, 0x030A);
  TEST2 (0x00F4, 0x006F, 0x0302);
  TEST3 (0x1E69, 0x0073, 0x0323, 0x0307);
  TEST2 (0x1E63, 0x0073, 0x0323);
  TEST2 (0x1E0B, 0x0064, 0x0307);
  TEST2 (0x1E0D, 0x0064, 0x0323);

  /* Hangul */
  TEST3 (0xD4DB, 0x1111, 0x1171, 0x11B6);
  TEST2 (0xD4CC, 0x1111, 0x1171);
  TEST3 (0xCE31, 0x110E, 0x1173, 0x11B8);
  TEST2 (0xCE20, 0x110E, 0x1173);

#undef TEST_DECOMP
}

/* test_t that xunichar_decompose() whenever encouttering a char ch
 * decomposes into a and b, b itself won't decompose any further. */
static void
test_decompose_tail (void)
{
  xunichar_t ch, a, b, c, d;

  /* test_t that whenever a char ch decomposes into a and b, b itself
   * won't decompose any further. */

  for (ch = 0; ch < 0x110000; ch++)
    if (xunichar_decompose (ch, &a, &b))
      g_assert_false (xunichar_decompose (b, &c, &d));
    else
      {
        g_assert_cmpuint (a, ==, ch);
        g_assert_cmpuint (b, ==, 0);
      }
}

/* test_t that all canonical decompositions of xunichar_fully_decompose()
 * are at most 4 in length, and compatibility decompositions are
 * at most 18 in length. */
static void
test_fully_decompose_len (void)
{
  xunichar_t ch;

  /* test_t that all canonical decompositions are at most 4 in length,
   * and compatibility decompositions are at most 18 in length.
   */

  for (ch = 0; ch < 0x110000; ch++) {
    g_assert_cmpint (xunichar_fully_decompose (ch, FALSE, NULL, 0), <=, 4);
    g_assert_cmpint (xunichar_fully_decompose (ch, TRUE,  NULL, 0), <=, 18);
  }
}

/* Check various examples from Unicode Annex #15 for NFD and NFC
 * normalization.
 */
static void
test_normalization (void)
{
  const struct {
    const char *source;
    const char *nfd;
    const char *nfc;
  } tests[] = {
    // Singletons
    { "\xe2\x84\xab", "A\xcc\x8a", "Å" }, // U+212B ANGSTROM SIGN
    { "\xe2\x84\xa6", "Ω", "Ω" }, // U+2126 OHM SIGN
    // Canonical Composites
    { "Å", "A\xcc\x8a", "Å" }, // U+00C5 LATIN CAPITAL LETTER A WITH RING ABOVE
    { "ô", "o\xcc\x82", "ô" }, // U+00F4 LATIN SMALL LETTER O WITH CIRCUMFLEX
    // Multiple Combining Marks
    { "\xe1\xb9\xa9", "s\xcc\xa3\xcc\x87", "ṩ" }, // U+1E69 LATIN SMALL LETTER S WITH DOT BELOW AND DOT ABOVE
    { "\xe1\xb8\x8b\xcc\xa3", "d\xcc\xa3\xcc\x87", "ḍ̇" },
    { "q\xcc\x87\xcc\xa3", "q\xcc\xa3\xcc\x87", "q̣̇" },
    // Compatibility Composites
    { "ﬁ", "ﬁ", "ﬁ" }, // U+FB01 LATIN SMALL LIGATURE FI
    { "2\xe2\x81\xb5", "2\xe2\x81\xb5", "2⁵" },
    { "\xe1\xba\x9b\xcc\xa3", "\xc5\xbf\xcc\xa3\xcc\x87", "ẛ̣" },

    // Tests for behavior with reordered marks
    { "s\xcc\x87\xcc\xa3", "s\xcc\xa3\xcc\x87", "ṩ" },
    { "α\xcc\x94\xcd\x82", "α\xcc\x94\xcd\x82", "ἇ" },
    { "α\xcd\x82\xcc\x94", "α\xcd\x82\xcc\x94", "ᾶ\xcc\x94" },
  };
  xsize_t i;

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      char *nfd, *nfc;

      nfd = xutf8_normalize (tests[i].source, -1, XNORMALIZE_NFD);
      g_assert_cmpstr (nfd, ==, tests[i].nfd);

      nfc = xutf8_normalize (tests[i].nfd, -1, XNORMALIZE_NFC);
      g_assert_cmpstr (nfc, ==, tests[i].nfc);

      g_free (nfd);
      g_free (nfc);
    }
}

static void
test_iso15924 (void)
{
  const struct {
    xunicode_script_t script;
    char four_letter_code[5];
  } data[] = {
    { XUNICODE_SCRIPT_COMMON,             "Zyyy" },
    { XUNICODE_SCRIPT_INHERITED,          "Zinh" },
    { XUNICODE_SCRIPT_MATH,               "Zmth" },
    { XUNICODE_SCRIPT_ARABIC,             "Arab" },
    { XUNICODE_SCRIPT_ARMENIAN,           "Armn" },
    { XUNICODE_SCRIPT_BENGALI,            "Beng" },
    { XUNICODE_SCRIPT_BOPOMOFO,           "Bopo" },
    { XUNICODE_SCRIPT_CHEROKEE,           "Cher" },
    { XUNICODE_SCRIPT_COPTIC,             "Copt" },
    { XUNICODE_SCRIPT_CYRILLIC,           "Cyrl" },
    { XUNICODE_SCRIPT_DESERET,            "Dsrt" },
    { XUNICODE_SCRIPT_DEVANAGARI,         "Deva" },
    { XUNICODE_SCRIPT_ETHIOPIC,           "Ethi" },
    { XUNICODE_SCRIPT_GEORGIAN,           "Geor" },
    { XUNICODE_SCRIPT_GOTHIC,             "Goth" },
    { XUNICODE_SCRIPT_GREEK,              "Grek" },
    { XUNICODE_SCRIPT_GUJARATI,           "Gujr" },
    { XUNICODE_SCRIPT_GURMUKHI,           "Guru" },
    { XUNICODE_SCRIPT_HAN,                "Hani" },
    { XUNICODE_SCRIPT_HANGUL,             "Hang" },
    { XUNICODE_SCRIPT_HEBREW,             "Hebr" },
    { XUNICODE_SCRIPT_HIRAGANA,           "Hira" },
    { XUNICODE_SCRIPT_KANNADA,            "Knda" },
    { XUNICODE_SCRIPT_KATAKANA,           "Kana" },
    { XUNICODE_SCRIPT_KHMER,              "Khmr" },
    { XUNICODE_SCRIPT_LAO,                "Laoo" },
    { XUNICODE_SCRIPT_LATIN,              "Latn" },
    { XUNICODE_SCRIPT_MALAYALAM,          "Mlym" },
    { XUNICODE_SCRIPT_MONGOLIAN,          "Mong" },
    { XUNICODE_SCRIPT_MYANMAR,            "Mymr" },
    { XUNICODE_SCRIPT_OGHAM,              "Ogam" },
    { XUNICODE_SCRIPT_OLD_ITALIC,         "Ital" },
    { XUNICODE_SCRIPT_ORIYA,              "Orya" },
    { XUNICODE_SCRIPT_RUNIC,              "Runr" },
    { XUNICODE_SCRIPT_SINHALA,            "Sinh" },
    { XUNICODE_SCRIPT_SYRIAC,             "Syrc" },
    { XUNICODE_SCRIPT_TAMIL,              "Taml" },
    { XUNICODE_SCRIPT_TELUGU,             "Telu" },
    { XUNICODE_SCRIPT_THAANA,             "Thaa" },
    { XUNICODE_SCRIPT_THAI,               "Thai" },
    { XUNICODE_SCRIPT_TIBETAN,            "Tibt" },
    { XUNICODE_SCRIPT_CANADIAN_ABORIGINAL, "Cans" },
    { XUNICODE_SCRIPT_YI,                 "Yiii" },
    { XUNICODE_SCRIPT_TAGALOG,            "Tglg" },
    { XUNICODE_SCRIPT_HANUNOO,            "Hano" },
    { XUNICODE_SCRIPT_BUHID,              "Buhd" },
    { XUNICODE_SCRIPT_TAGBANWA,           "Tagb" },

    /* Unicode-4.0 additions */
    { XUNICODE_SCRIPT_BRAILLE,            "Brai" },
    { XUNICODE_SCRIPT_CYPRIOT,            "Cprt" },
    { XUNICODE_SCRIPT_LIMBU,              "Limb" },
    { XUNICODE_SCRIPT_OSMANYA,            "Osma" },
    { XUNICODE_SCRIPT_SHAVIAN,            "Shaw" },
    { XUNICODE_SCRIPT_LINEAR_B,           "Linb" },
    { XUNICODE_SCRIPT_TAI_LE,             "Tale" },
    { XUNICODE_SCRIPT_UGARITIC,           "Ugar" },

    /* Unicode-4.1 additions */
    { XUNICODE_SCRIPT_NEW_TAI_LUE,        "Talu" },
    { XUNICODE_SCRIPT_BUGINESE,           "Bugi" },
    { XUNICODE_SCRIPT_GLAGOLITIC,         "Glag" },
    { XUNICODE_SCRIPT_TIFINAGH,           "Tfng" },
    { XUNICODE_SCRIPT_SYLOTI_NAGRI,       "Sylo" },
    { XUNICODE_SCRIPT_OLD_PERSIAN,        "Xpeo" },
    { XUNICODE_SCRIPT_KHAROSHTHI,         "Khar" },

    /* Unicode-5.0 additions */
    { XUNICODE_SCRIPT_UNKNOWN,            "Zzzz" },
    { XUNICODE_SCRIPT_BALINESE,           "Bali" },
    { XUNICODE_SCRIPT_CUNEIFORM,          "Xsux" },
    { XUNICODE_SCRIPT_PHOENICIAN,         "Phnx" },
    { XUNICODE_SCRIPT_PHAGS_PA,           "Phag" },
    { XUNICODE_SCRIPT_NKO,                "Nkoo" },

    /* Unicode-5.1 additions */
    { XUNICODE_SCRIPT_KAYAH_LI,           "Kali" },
    { XUNICODE_SCRIPT_LEPCHA,             "Lepc" },
    { XUNICODE_SCRIPT_REJANG,             "Rjng" },
    { XUNICODE_SCRIPT_SUNDANESE,          "Sund" },
    { XUNICODE_SCRIPT_SAURASHTRA,         "Saur" },
    { XUNICODE_SCRIPT_CHAM,               "Cham" },
    { XUNICODE_SCRIPT_OL_CHIKI,           "Olck" },
    { XUNICODE_SCRIPT_VAI,                "Vaii" },
    { XUNICODE_SCRIPT_CARIAN,             "Cari" },
    { XUNICODE_SCRIPT_LYCIAN,             "Lyci" },
    { XUNICODE_SCRIPT_LYDIAN,             "Lydi" },

    /* Unicode-5.2 additions */
    { XUNICODE_SCRIPT_AVESTAN,                "Avst" },
    { XUNICODE_SCRIPT_BAMUM,                  "Bamu" },
    { XUNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS,   "Egyp" },
    { XUNICODE_SCRIPT_IMPERIAL_ARAMAIC,       "Armi" },
    { XUNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI,  "Phli" },
    { XUNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN, "Prti" },
    { XUNICODE_SCRIPT_JAVANESE,               "Java" },
    { XUNICODE_SCRIPT_KAITHI,                 "Kthi" },
    { XUNICODE_SCRIPT_LISU,                   "Lisu" },
    { XUNICODE_SCRIPT_MEETEI_MAYEK,           "Mtei" },
    { XUNICODE_SCRIPT_OLD_SOUTH_ARABIAN,      "Sarb" },
    { XUNICODE_SCRIPT_OLD_TURKIC,             "Orkh" },
    { XUNICODE_SCRIPT_SAMARITAN,              "Samr" },
    { XUNICODE_SCRIPT_TAI_THAM,               "Lana" },
    { XUNICODE_SCRIPT_TAI_VIET,               "Tavt" },

    /* Unicode-6.0 additions */
    { XUNICODE_SCRIPT_BATAK,                  "Batk" },
    { XUNICODE_SCRIPT_BRAHMI,                 "Brah" },
    { XUNICODE_SCRIPT_MANDAIC,                "Mand" },

    /* Unicode-6.1 additions */
    { XUNICODE_SCRIPT_CHAKMA,                 "Cakm" },
    { XUNICODE_SCRIPT_MEROITIC_CURSIVE,       "Merc" },
    { XUNICODE_SCRIPT_MEROITIC_HIEROGLYPHS,   "Mero" },
    { XUNICODE_SCRIPT_MIAO,                   "Plrd" },
    { XUNICODE_SCRIPT_SHARADA,                "Shrd" },
    { XUNICODE_SCRIPT_SORA_SOMPENG,           "Sora" },
    { XUNICODE_SCRIPT_TAKRI,                  "Takr" },

    /* Unicode 7.0 additions */
    { XUNICODE_SCRIPT_BASSA_VAH,              "Bass" },
    { XUNICODE_SCRIPT_CAUCASIAN_ALBANIAN,     "Aghb" },
    { XUNICODE_SCRIPT_DUPLOYAN,               "Dupl" },
    { XUNICODE_SCRIPT_ELBASAN,                "Elba" },
    { XUNICODE_SCRIPT_GRANTHA,                "Gran" },
    { XUNICODE_SCRIPT_KHOJKI,                 "Khoj" },
    { XUNICODE_SCRIPT_KHUDAWADI,              "Sind" },
    { XUNICODE_SCRIPT_LINEAR_A,               "Lina" },
    { XUNICODE_SCRIPT_MAHAJANI,               "Mahj" },
    { XUNICODE_SCRIPT_MANICHAEAN,             "Mani" },
    { XUNICODE_SCRIPT_MENDE_KIKAKUI,          "Mend" },
    { XUNICODE_SCRIPT_MODI,                   "Modi" },
    { XUNICODE_SCRIPT_MRO,                    "Mroo" },
    { XUNICODE_SCRIPT_NABATAEAN,              "Nbat" },
    { XUNICODE_SCRIPT_OLD_NORTH_ARABIAN,      "Narb" },
    { XUNICODE_SCRIPT_OLD_PERMIC,             "Perm" },
    { XUNICODE_SCRIPT_PAHAWH_HMONG,           "Hmng" },
    { XUNICODE_SCRIPT_PALMYRENE,              "Palm" },
    { XUNICODE_SCRIPT_PAU_CIN_HAU,            "Pauc" },
    { XUNICODE_SCRIPT_PSALTER_PAHLAVI,        "Phlp" },
    { XUNICODE_SCRIPT_SIDDHAM,                "Sidd" },
    { XUNICODE_SCRIPT_TIRHUTA,                "Tirh" },
    { XUNICODE_SCRIPT_WARANG_CITI,            "Wara" },

    /* Unicode 8.0 additions */
    { XUNICODE_SCRIPT_AHOM,                   "Ahom" },
    { XUNICODE_SCRIPT_ANATOLIAN_HIEROGLYPHS,  "Hluw" },
    { XUNICODE_SCRIPT_HATRAN,                 "Hatr" },
    { XUNICODE_SCRIPT_MULTANI,                "Mult" },
    { XUNICODE_SCRIPT_OLD_HUNGARIAN,          "Hung" },
    { XUNICODE_SCRIPT_SIGNWRITING,            "Sgnw" },

    /* Unicode 9.0 additions */
    { XUNICODE_SCRIPT_ADLAM,                  "Adlm" },
    { XUNICODE_SCRIPT_BHAIKSUKI,              "Bhks" },
    { XUNICODE_SCRIPT_MARCHEN,                "Marc" },
    { XUNICODE_SCRIPT_NEWA,                   "Newa" },
    { XUNICODE_SCRIPT_OSAGE,                  "Osge" },
    { XUNICODE_SCRIPT_TANGUT,                 "Tang" },

    /* Unicode 10.0 additions */
    { XUNICODE_SCRIPT_MASARAM_GONDI,          "Gonm" },
    { XUNICODE_SCRIPT_NUSHU,                  "Nshu" },
    { XUNICODE_SCRIPT_SOYOMBO,                "Soyo" },
    { XUNICODE_SCRIPT_ZANABAZAR_SQUARE,       "Zanb" },

    /* Unicode 11.0 additions */
    { XUNICODE_SCRIPT_DOGRA,                  "Dogr" },
    { XUNICODE_SCRIPT_GUNJALA_GONDI,          "Gong" },
    { XUNICODE_SCRIPT_HANIFI_ROHINGYA,        "Rohg" },
    { XUNICODE_SCRIPT_MAKASAR,                "Maka" },
    { XUNICODE_SCRIPT_MEDEFAIDRIN,            "Medf" },
    { XUNICODE_SCRIPT_OLD_SOGDIAN,            "Sogo" },
    { XUNICODE_SCRIPT_SOGDIAN,                "Sogd" },

    /* Unicode 12.0 additions */
    { XUNICODE_SCRIPT_ELYMAIC,                "Elym" },
    { XUNICODE_SCRIPT_NANDINAGARI,            "Nand" },
    { XUNICODE_SCRIPT_NYIAKENG_PUACHUE_HMONG, "Hmnp" },
    { XUNICODE_SCRIPT_WANCHO,                 "Wcho" },

    /* Unicode 13.0 additions */
    { XUNICODE_SCRIPT_CHORASMIAN,             "Chrs" },
    { XUNICODE_SCRIPT_DIVES_AKURU,            "Diak" },
    { XUNICODE_SCRIPT_KHITAN_SMALL_SCRIPT,    "Kits" },
    { XUNICODE_SCRIPT_YEZIDI,                 "Yezi" },

    /* Unicode 14.0 additions */
    { XUNICODE_SCRIPT_CYPRO_MINOAN,           "Cpmn" },
    { XUNICODE_SCRIPT_OLD_UYGHUR,             "Ougr" },
    { XUNICODE_SCRIPT_TANGSA,                 "Tnsa" },
    { XUNICODE_SCRIPT_TOTO,                   "Toto" },
    { XUNICODE_SCRIPT_VITHKUQI,               "Vith" }
  };
  xuint_t i;

  g_assert_cmphex (0, ==,
                   xunicode_script_to_iso15924 (XUNICODE_SCRIPT_INVALID_CODE));
  g_assert_cmphex (0x5A7A7A7A, ==, xunicode_script_to_iso15924 (1000));
  g_assert_cmphex (0x41726162, ==,
                   xunicode_script_to_iso15924 (XUNICODE_SCRIPT_ARABIC));

  g_assert_cmphex (XUNICODE_SCRIPT_INVALID_CODE, ==,
                   xunicode_script_from_iso15924 (0));
  g_assert_cmphex (XUNICODE_SCRIPT_UNKNOWN, ==,
                   xunicode_script_from_iso15924 (0x12345678));

#define PACK(a,b,c,d) \
  ((xuint32_t)((((xuint8_t)(a))<<24)|(((xuint8_t)(b))<<16)|(((xuint8_t)(c))<<8)|((xuint8_t)(d))))

  for (i = 0; i < G_N_ELEMENTS (data); i++)
    {
      xuint32_t code = PACK (data[i].four_letter_code[0],
                           data[i].four_letter_code[1],
                           data[i].four_letter_code[2],
                           data[i].four_letter_code[3]);

      g_assert_cmphex (xunicode_script_to_iso15924 (data[i].script), ==, code);
      g_assert_cmpint (xunicode_script_from_iso15924 (code), ==, data[i].script);
    }

#undef PACK
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/unicode/alnum", test_alnum);
  g_test_add_func ("/unicode/alpha", test_alpha);
  g_test_add_func ("/unicode/break-type", test_unichar_break_type);
  g_test_add_func ("/unicode/canonical-decomposition", test_canonical_decomposition);
  g_test_add_func ("/unicode/casefold", test_casefold);
  g_test_add_func ("/unicode/casemap_and_casefold", test_casemap_and_casefold);
  g_test_add_func ("/unicode/cases", test_cases);
  g_test_add_func ("/unicode/character-type", test_unichar_character_type);
  g_test_add_func ("/unicode/cntrl", test_cntrl);
  g_test_add_func ("/unicode/combining-class", test_combining_class);
  g_test_add_func ("/unicode/compose", test_compose);
  g_test_add_func ("/unicode/decompose", test_decompose);
  g_test_add_func ("/unicode/decompose-tail", test_decompose_tail);
  g_test_add_func ("/unicode/defined", test_defined);
  g_test_add_func ("/unicode/digit", test_digit);
  g_test_add_func ("/unicode/digit-value", test_digit_value);
  g_test_add_func ("/unicode/fully-decompose-canonical", test_fully_decompose_canonical);
  g_test_add_func ("/unicode/fully-decompose-len", test_fully_decompose_len);
  g_test_add_func ("/unicode/normalization", test_normalization);
  g_test_add_func ("/unicode/graph", test_graph);
  g_test_add_func ("/unicode/iso15924", test_iso15924);
  g_test_add_func ("/unicode/lower", test_lower);
  g_test_add_func ("/unicode/mark", test_mark);
  g_test_add_func ("/unicode/mirror", test_mirror);
  g_test_add_func ("/unicode/print", test_print);
  g_test_add_func ("/unicode/punctuation", test_punctuation);
  g_test_add_func ("/unicode/script", test_unichar_script);
  g_test_add_func ("/unicode/space", test_space);
  g_test_add_func ("/unicode/strdown", test_strdown);
  g_test_add_func ("/unicode/strup", test_strup);
  g_test_add_func ("/unicode/turkish-strupdown", test_turkish_strupdown);
  g_test_add_func ("/unicode/title", test_title);
  g_test_add_func ("/unicode/upper", test_upper);
  g_test_add_func ("/unicode/validate", test_unichar_validate);
  g_test_add_func ("/unicode/wide", test_wide);
  g_test_add_func ("/unicode/xdigit", test_xdigit);
  g_test_add_func ("/unicode/xdigit-value", test_xdigit_value);
  g_test_add_func ("/unicode/zero-width", test_zerowidth);

  return g_test_run();
}
