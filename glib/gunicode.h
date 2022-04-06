/* gunicode.h - Unicode manipulation functions
 *
 *  Copyright (C) 1999, 2000 Tom Tromey
 *  Copyright 2000, 2005 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __XUNICODE_H__
#define __XUNICODE_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gerror.h>
#include <glib/gtypes.h>

G_BEGIN_DECLS

/**
 * xunichar_t:
 *
 * A type which can hold any UTF-32 or UCS-4 character code,
 * also known as a Unicode code point.
 *
 * If you want to produce the UTF-8 representation of a #xunichar_t,
 * use g_ucs4_to_utf8(). See also xutf8_to_ucs4() for the reverse
 * process.
 *
 * To print/scan values of this type as integer, use
 * %G_GINT32_MODIFIER and/or %G_GUINT32_FORMAT.
 *
 * The notation to express a Unicode code point in running text is
 * as a hexadecimal number with four to six digits and uppercase
 * letters, prefixed by the string "U+". Leading zeros are omitted,
 * unless the code point would have fewer than four hexadecimal digits.
 * For example, "U+0041 LATIN CAPITAL LETTER A". To print a code point
 * in the U+-notation, use the format string "U+\%04"G_GINT32_FORMAT"X".
 * To scan, use the format string "U+\%06"G_GINT32_FORMAT"X".
 *
 * |[
 * xunichar_t c;
 * sscanf ("U+0041", "U+%06"G_GINT32_FORMAT"X", &amp;c)
 * g_print ("Read U+%04"G_GINT32_FORMAT"X", c);
 * ]|
 */
typedef xuint32_t xunichar_t;

/**
 * xunichar2_t:
 *
 * A type which can hold any UTF-16 code
 * point<footnote id="utf16_surrogate_pairs">UTF-16 also has so called
 * <firstterm>surrogate pairs</firstterm> to encode characters beyond
 * the BMP as pairs of 16bit numbers. Surrogate pairs cannot be stored
 * in a single xunichar2_t field, but all GLib functions accepting xunichar2_t
 * arrays will correctly interpret surrogate pairs.</footnote>.
 *
 * To print/scan values of this type to/from text you need to convert
 * to/from UTF-8, using xutf16_to_utf8()/xutf8_to_utf16().
 *
 * To print/scan values of this type as integer, use
 * %G_GINT16_MODIFIER and/or %G_GUINT16_FORMAT.
 */
typedef xuint16_t xunichar2_t;

/**
 * xunicode_type_t:
 * @XUNICODE_CONTROL: General category "Other, Control" (Cc)
 * @XUNICODE_FORMAT: General category "Other, Format" (Cf)
 * @XUNICODE_UNASSIGNED: General category "Other, Not Assigned" (Cn)
 * @XUNICODE_PRIVATE_USE: General category "Other, Private Use" (Co)
 * @XUNICODE_SURROGATE: General category "Other, Surrogate" (Cs)
 * @XUNICODE_LOWERCASE_LETTER: General category "Letter, Lowercase" (Ll)
 * @XUNICODE_MODIFIER_LETTER: General category "Letter, Modifier" (Lm)
 * @XUNICODE_OTHER_LETTER: General category "Letter, Other" (Lo)
 * @XUNICODE_TITLECASE_LETTER: General category "Letter, Titlecase" (Lt)
 * @XUNICODE_UPPERCASE_LETTER: General category "Letter, Uppercase" (Lu)
 * @XUNICODE_SPACING_MARK: General category "Mark, Spacing" (Mc)
 * @XUNICODE_ENCLOSING_MARK: General category "Mark, Enclosing" (Me)
 * @XUNICODE_NON_SPACING_MARK: General category "Mark, Nonspacing" (Mn)
 * @XUNICODE_DECIMAL_NUMBER: General category "Number, Decimal Digit" (Nd)
 * @XUNICODE_LETTER_NUMBER: General category "Number, Letter" (Nl)
 * @XUNICODE_OTHER_NUMBER: General category "Number, Other" (No)
 * @XUNICODE_CONNECT_PUNCTUATION: General category "Punctuation, Connector" (Pc)
 * @XUNICODE_DASH_PUNCTUATION: General category "Punctuation, Dash" (Pd)
 * @XUNICODE_CLOSE_PUNCTUATION: General category "Punctuation, Close" (Pe)
 * @XUNICODE_FINAL_PUNCTUATION: General category "Punctuation, Final quote" (Pf)
 * @XUNICODE_INITIAL_PUNCTUATION: General category "Punctuation, Initial quote" (Pi)
 * @XUNICODE_OTHER_PUNCTUATION: General category "Punctuation, Other" (Po)
 * @XUNICODE_OPEN_PUNCTUATION: General category "Punctuation, Open" (Ps)
 * @XUNICODE_CURRENCY_SYMBOL: General category "Symbol, Currency" (Sc)
 * @XUNICODE_MODIFIER_SYMBOL: General category "Symbol, Modifier" (Sk)
 * @XUNICODE_MATH_SYMBOL: General category "Symbol, Math" (Sm)
 * @XUNICODE_OTHER_SYMBOL: General category "Symbol, Other" (So)
 * @XUNICODE_LINE_SEPARATOR: General category "Separator, Line" (Zl)
 * @XUNICODE_PARAGRAPH_SEPARATOR: General category "Separator, Paragraph" (Zp)
 * @XUNICODE_SPACE_SEPARATOR: General category "Separator, Space" (Zs)
 *
 * These are the possible character classifications from the
 * Unicode specification.
 * See [Unicode Character Database](http://www.unicode.org/reports/tr44/#General_Category_Values).
 */
typedef enum
{
  XUNICODE_CONTROL,
  XUNICODE_FORMAT,
  XUNICODE_UNASSIGNED,
  XUNICODE_PRIVATE_USE,
  XUNICODE_SURROGATE,
  XUNICODE_LOWERCASE_LETTER,
  XUNICODE_MODIFIER_LETTER,
  XUNICODE_OTHER_LETTER,
  XUNICODE_TITLECASE_LETTER,
  XUNICODE_UPPERCASE_LETTER,
  XUNICODE_SPACING_MARK,
  XUNICODE_ENCLOSING_MARK,
  XUNICODE_NON_SPACING_MARK,
  XUNICODE_DECIMAL_NUMBER,
  XUNICODE_LETTER_NUMBER,
  XUNICODE_OTHER_NUMBER,
  XUNICODE_CONNECT_PUNCTUATION,
  XUNICODE_DASH_PUNCTUATION,
  XUNICODE_CLOSE_PUNCTUATION,
  XUNICODE_FINAL_PUNCTUATION,
  XUNICODE_INITIAL_PUNCTUATION,
  XUNICODE_OTHER_PUNCTUATION,
  XUNICODE_OPEN_PUNCTUATION,
  XUNICODE_CURRENCY_SYMBOL,
  XUNICODE_MODIFIER_SYMBOL,
  XUNICODE_MATH_SYMBOL,
  XUNICODE_OTHER_SYMBOL,
  XUNICODE_LINE_SEPARATOR,
  XUNICODE_PARAGRAPH_SEPARATOR,
  XUNICODE_SPACE_SEPARATOR
} xunicode_type_t;

/**
 * XUNICODE_COMBINING_MARK:
 *
 * Older name for %XUNICODE_SPACING_MARK.
 *
 * Deprecated: 2.30: Use %XUNICODE_SPACING_MARK.
 */
#define XUNICODE_COMBINING_MARK XUNICODE_SPACING_MARK XPL_DEPRECATED_MACRO_IN_2_30_FOR(XUNICODE_SPACING_MARK)

/**
 * GUnicodeBreakType:
 * @XUNICODE_BREAK_MANDATORY: Mandatory Break (BK)
 * @XUNICODE_BREAK_CARRIAGE_RETURN: Carriage Return (CR)
 * @XUNICODE_BREAK_LINE_FEED: Line Feed (LF)
 * @XUNICODE_BREAK_COMBINING_MARK: Attached Characters and Combining Marks (CM)
 * @XUNICODE_BREAK_SURROGATE: Surrogates (SG)
 * @XUNICODE_BREAK_ZERO_WIDTH_SPACE: Zero Width Space (ZW)
 * @XUNICODE_BREAK_INSEPARABLE: Inseparable (IN)
 * @XUNICODE_BREAK_NON_BREAKING_GLUE: Non-breaking ("Glue") (GL)
 * @XUNICODE_BREAK_CONTINGENT: Contingent Break Opportunity (CB)
 * @XUNICODE_BREAK_SPACE: Space (SP)
 * @XUNICODE_BREAK_AFTER: Break Opportunity After (BA)
 * @XUNICODE_BREAK_BEFORE: Break Opportunity Before (BB)
 * @XUNICODE_BREAK_BEFORE_AND_AFTER: Break Opportunity Before and After (B2)
 * @XUNICODE_BREAK_HYPHEN: Hyphen (HY)
 * @XUNICODE_BREAK_NON_STARTER: Nonstarter (NS)
 * @XUNICODE_BREAK_OPEN_PUNCTUATION: Opening Punctuation (OP)
 * @XUNICODE_BREAK_CLOSE_PUNCTUATION: Closing Punctuation (CL)
 * @XUNICODE_BREAK_QUOTATION: Ambiguous Quotation (QU)
 * @XUNICODE_BREAK_EXCLAMATION: Exclamation/Interrogation (EX)
 * @XUNICODE_BREAK_IDEOGRAPHIC: Ideographic (ID)
 * @XUNICODE_BREAK_NUMERIC: Numeric (NU)
 * @XUNICODE_BREAK_INFIX_SEPARATOR: Infix Separator (Numeric) (IS)
 * @XUNICODE_BREAK_SYMBOL: Symbols Allowing Break After (SY)
 * @XUNICODE_BREAK_ALPHABETIC: Ordinary Alphabetic and Symbol Characters (AL)
 * @XUNICODE_BREAK_PREFIX: Prefix (Numeric) (PR)
 * @XUNICODE_BREAK_POSTFIX: Postfix (Numeric) (PO)
 * @XUNICODE_BREAK_COMPLEX_CONTEXT: Complex Content Dependent (South East Asian) (SA)
 * @XUNICODE_BREAK_AMBIGUOUS: Ambiguous (Alphabetic or Ideographic) (AI)
 * @XUNICODE_BREAK_UNKNOWN: Unknown (XX)
 * @XUNICODE_BREAK_NEXT_LINE: Next Line (NL)
 * @XUNICODE_BREAK_WORD_JOINER: Word Joiner (WJ)
 * @XUNICODE_BREAK_HANGUL_L_JAMO: Hangul L Jamo (JL)
 * @XUNICODE_BREAK_HANGUL_V_JAMO: Hangul V Jamo (JV)
 * @XUNICODE_BREAK_HANGUL_T_JAMO: Hangul T Jamo (JT)
 * @XUNICODE_BREAK_HANGUL_LV_SYLLABLE: Hangul LV Syllable (H2)
 * @XUNICODE_BREAK_HANGUL_LVT_SYLLABLE: Hangul LVT Syllable (H3)
 * @XUNICODE_BREAK_CLOSE_PARANTHESIS: Closing Parenthesis (CP). Since 2.28. Deprecated: 2.70: Use %XUNICODE_BREAK_CLOSE_PARENTHESIS instead.
 * @XUNICODE_BREAK_CLOSE_PARENTHESIS: Closing Parenthesis (CP). Since 2.70
 * @XUNICODE_BREAK_CONDITIONAL_JAPANESE_STARTER: Conditional Japanese Starter (CJ). Since: 2.32
 * @XUNICODE_BREAK_HEBREW_LETTER: Hebrew Letter (HL). Since: 2.32
 * @XUNICODE_BREAK_REGIONAL_INDICATOR: Regional Indicator (RI). Since: 2.36
 * @XUNICODE_BREAK_EMOJI_BASE: Emoji Base (EB). Since: 2.50
 * @XUNICODE_BREAK_EMOJI_MODIFIER: Emoji Modifier (EM). Since: 2.50
 * @XUNICODE_BREAK_ZERO_WIDTH_JOINER: Zero Width Joiner (ZWJ). Since: 2.50
 *
 * These are the possible line break classifications.
 *
 * Since new unicode versions may add new types here, applications should be ready
 * to handle unknown values. They may be regarded as %XUNICODE_BREAK_UNKNOWN.
 *
 * See [Unicode Line Breaking Algorithm](http://www.unicode.org/unicode/reports/tr14/).
 */
typedef enum
{
  XUNICODE_BREAK_MANDATORY,
  XUNICODE_BREAK_CARRIAGE_RETURN,
  XUNICODE_BREAK_LINE_FEED,
  XUNICODE_BREAK_COMBINING_MARK,
  XUNICODE_BREAK_SURROGATE,
  XUNICODE_BREAK_ZERO_WIDTH_SPACE,
  XUNICODE_BREAK_INSEPARABLE,
  XUNICODE_BREAK_NON_BREAKING_GLUE,
  XUNICODE_BREAK_CONTINGENT,
  XUNICODE_BREAK_SPACE,
  XUNICODE_BREAK_AFTER,
  XUNICODE_BREAK_BEFORE,
  XUNICODE_BREAK_BEFORE_AND_AFTER,
  XUNICODE_BREAK_HYPHEN,
  XUNICODE_BREAK_NON_STARTER,
  XUNICODE_BREAK_OPEN_PUNCTUATION,
  XUNICODE_BREAK_CLOSE_PUNCTUATION,
  XUNICODE_BREAK_QUOTATION,
  XUNICODE_BREAK_EXCLAMATION,
  XUNICODE_BREAK_IDEOGRAPHIC,
  XUNICODE_BREAK_NUMERIC,
  XUNICODE_BREAK_INFIX_SEPARATOR,
  XUNICODE_BREAK_SYMBOL,
  XUNICODE_BREAK_ALPHABETIC,
  XUNICODE_BREAK_PREFIX,
  XUNICODE_BREAK_POSTFIX,
  XUNICODE_BREAK_COMPLEX_CONTEXT,
  XUNICODE_BREAK_AMBIGUOUS,
  XUNICODE_BREAK_UNKNOWN,
  XUNICODE_BREAK_NEXT_LINE,
  XUNICODE_BREAK_WORD_JOINER,
  XUNICODE_BREAK_HANGUL_L_JAMO,
  XUNICODE_BREAK_HANGUL_V_JAMO,
  XUNICODE_BREAK_HANGUL_T_JAMO,
  XUNICODE_BREAK_HANGUL_LV_SYLLABLE,
  XUNICODE_BREAK_HANGUL_LVT_SYLLABLE,
  XUNICODE_BREAK_CLOSE_PARANTHESIS,
  XUNICODE_BREAK_CLOSE_PARENTHESIS XPL_AVAILABLE_ENUMERATOR_IN_2_70 = XUNICODE_BREAK_CLOSE_PARANTHESIS,
  XUNICODE_BREAK_CONDITIONAL_JAPANESE_STARTER,
  XUNICODE_BREAK_HEBREW_LETTER,
  XUNICODE_BREAK_REGIONAL_INDICATOR,
  XUNICODE_BREAK_EMOJI_BASE,
  XUNICODE_BREAK_EMOJI_MODIFIER,
  XUNICODE_BREAK_ZERO_WIDTH_JOINER
} GUnicodeBreakType;

/**
 * xunicode_script_t:
 * @XUNICODE_SCRIPT_INVALID_CODE:
 *                               a value never returned from xunichar_get_script()
 * @XUNICODE_SCRIPT_COMMON:     a character used by multiple different scripts
 * @XUNICODE_SCRIPT_INHERITED:  a mark glyph that takes its script from the
 *                               base glyph to which it is attached
 * @XUNICODE_SCRIPT_ARABIC:     Arabic
 * @XUNICODE_SCRIPT_ARMENIAN:   Armenian
 * @XUNICODE_SCRIPT_BENGALI:    Bengali
 * @XUNICODE_SCRIPT_BOPOMOFO:   Bopomofo
 * @XUNICODE_SCRIPT_CHEROKEE:   Cherokee
 * @XUNICODE_SCRIPT_COPTIC:     Coptic
 * @XUNICODE_SCRIPT_CYRILLIC:   Cyrillic
 * @XUNICODE_SCRIPT_DESERET:    Deseret
 * @XUNICODE_SCRIPT_DEVANAGARI: Devanagari
 * @XUNICODE_SCRIPT_ETHIOPIC:   Ethiopic
 * @XUNICODE_SCRIPT_GEORGIAN:   Georgian
 * @XUNICODE_SCRIPT_GOTHIC:     Gothic
 * @XUNICODE_SCRIPT_GREEK:      Greek
 * @XUNICODE_SCRIPT_GUJARATI:   Gujarati
 * @XUNICODE_SCRIPT_GURMUKHI:   Gurmukhi
 * @XUNICODE_SCRIPT_HAN:        Han
 * @XUNICODE_SCRIPT_HANGUL:     Hangul
 * @XUNICODE_SCRIPT_HEBREW:     Hebrew
 * @XUNICODE_SCRIPT_HIRAGANA:   Hiragana
 * @XUNICODE_SCRIPT_KANNADA:    Kannada
 * @XUNICODE_SCRIPT_KATAKANA:   Katakana
 * @XUNICODE_SCRIPT_KHMER:      Khmer
 * @XUNICODE_SCRIPT_LAO:        Lao
 * @XUNICODE_SCRIPT_LATIN:      Latin
 * @XUNICODE_SCRIPT_MALAYALAM:  Malayalam
 * @XUNICODE_SCRIPT_MONGOLIAN:  Mongolian
 * @XUNICODE_SCRIPT_MYANMAR:    Myanmar
 * @XUNICODE_SCRIPT_OGHAM:      Ogham
 * @XUNICODE_SCRIPT_OLD_ITALIC: Old Italic
 * @XUNICODE_SCRIPT_ORIYA:      Oriya
 * @XUNICODE_SCRIPT_RUNIC:      Runic
 * @XUNICODE_SCRIPT_SINHALA:    Sinhala
 * @XUNICODE_SCRIPT_SYRIAC:     Syriac
 * @XUNICODE_SCRIPT_TAMIL:      Tamil
 * @XUNICODE_SCRIPT_TELUGU:     Telugu
 * @XUNICODE_SCRIPT_THAANA:     Thaana
 * @XUNICODE_SCRIPT_THAI:       Thai
 * @XUNICODE_SCRIPT_TIBETAN:    Tibetan
 * @XUNICODE_SCRIPT_CANADIAN_ABORIGINAL:
 *                               Canadian Aboriginal
 * @XUNICODE_SCRIPT_YI:         Yi
 * @XUNICODE_SCRIPT_TAGALOG:    Tagalog
 * @XUNICODE_SCRIPT_HANUNOO:    Hanunoo
 * @XUNICODE_SCRIPT_BUHID:      Buhid
 * @XUNICODE_SCRIPT_TAGBANWA:   Tagbanwa
 * @XUNICODE_SCRIPT_BRAILLE:    Braille
 * @XUNICODE_SCRIPT_CYPRIOT:    Cypriot
 * @XUNICODE_SCRIPT_LIMBU:      Limbu
 * @XUNICODE_SCRIPT_OSMANYA:    Osmanya
 * @XUNICODE_SCRIPT_SHAVIAN:    Shavian
 * @XUNICODE_SCRIPT_LINEAR_B:   Linear B
 * @XUNICODE_SCRIPT_TAI_LE:     Tai Le
 * @XUNICODE_SCRIPT_UGARITIC:   Ugaritic
 * @XUNICODE_SCRIPT_NEW_TAI_LUE:
 *                               New Tai Lue
 * @XUNICODE_SCRIPT_BUGINESE:   Buginese
 * @XUNICODE_SCRIPT_GLAGOLITIC: Glagolitic
 * @XUNICODE_SCRIPT_TIFINAGH:   Tifinagh
 * @XUNICODE_SCRIPT_SYLOTI_NAGRI:
 *                               Syloti Nagri
 * @XUNICODE_SCRIPT_OLD_PERSIAN:
 *                               Old Persian
 * @XUNICODE_SCRIPT_KHAROSHTHI: Kharoshthi
 * @XUNICODE_SCRIPT_UNKNOWN:    an unassigned code point
 * @XUNICODE_SCRIPT_BALINESE:   Balinese
 * @XUNICODE_SCRIPT_CUNEIFORM:  Cuneiform
 * @XUNICODE_SCRIPT_PHOENICIAN: Phoenician
 * @XUNICODE_SCRIPT_PHAGS_PA:   Phags-pa
 * @XUNICODE_SCRIPT_NKO:        N'Ko
 * @XUNICODE_SCRIPT_KAYAH_LI:   Kayah Li. Since 2.16.3
 * @XUNICODE_SCRIPT_LEPCHA:     Lepcha. Since 2.16.3
 * @XUNICODE_SCRIPT_REJANG:     Rejang. Since 2.16.3
 * @XUNICODE_SCRIPT_SUNDANESE:  Sundanese. Since 2.16.3
 * @XUNICODE_SCRIPT_SAURASHTRA: Saurashtra. Since 2.16.3
 * @XUNICODE_SCRIPT_CHAM:       Cham. Since 2.16.3
 * @XUNICODE_SCRIPT_OL_CHIKI:   Ol Chiki. Since 2.16.3
 * @XUNICODE_SCRIPT_VAI:        Vai. Since 2.16.3
 * @XUNICODE_SCRIPT_CARIAN:     Carian. Since 2.16.3
 * @XUNICODE_SCRIPT_LYCIAN:     Lycian. Since 2.16.3
 * @XUNICODE_SCRIPT_LYDIAN:     Lydian. Since 2.16.3
 * @XUNICODE_SCRIPT_AVESTAN:    Avestan. Since 2.26
 * @XUNICODE_SCRIPT_BAMUM:      Bamum. Since 2.26
 * @XUNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS:
 *                               Egyptian Hieroglpyhs. Since 2.26
 * @XUNICODE_SCRIPT_IMPERIAL_ARAMAIC:
 *                               Imperial Aramaic. Since 2.26
 * @XUNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI:
 *                               Inscriptional Pahlavi. Since 2.26
 * @XUNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN:
 *                               Inscriptional Parthian. Since 2.26
 * @XUNICODE_SCRIPT_JAVANESE:   Javanese. Since 2.26
 * @XUNICODE_SCRIPT_KAITHI:     Kaithi. Since 2.26
 * @XUNICODE_SCRIPT_LISU:       Lisu. Since 2.26
 * @XUNICODE_SCRIPT_MEETEI_MAYEK:
 *                               Meetei Mayek. Since 2.26
 * @XUNICODE_SCRIPT_OLD_SOUTH_ARABIAN:
 *                               Old South Arabian. Since 2.26
 * @XUNICODE_SCRIPT_OLD_TURKIC: Old Turkic. Since 2.28
 * @XUNICODE_SCRIPT_SAMARITAN:  Samaritan. Since 2.26
 * @XUNICODE_SCRIPT_TAI_THAM:   Tai Tham. Since 2.26
 * @XUNICODE_SCRIPT_TAI_VIET:   Tai Viet. Since 2.26
 * @XUNICODE_SCRIPT_BATAK:      Batak. Since 2.28
 * @XUNICODE_SCRIPT_BRAHMI:     Brahmi. Since 2.28
 * @XUNICODE_SCRIPT_MANDAIC:    Mandaic. Since 2.28
 * @XUNICODE_SCRIPT_CHAKMA:               Chakma. Since: 2.32
 * @XUNICODE_SCRIPT_MEROITIC_CURSIVE:     Meroitic Cursive. Since: 2.32
 * @XUNICODE_SCRIPT_MEROITIC_HIEROGLYPHS: Meroitic Hieroglyphs. Since: 2.32
 * @XUNICODE_SCRIPT_MIAO:                 Miao. Since: 2.32
 * @XUNICODE_SCRIPT_SHARADA:              Sharada. Since: 2.32
 * @XUNICODE_SCRIPT_SORA_SOMPENG:         Sora Sompeng. Since: 2.32
 * @XUNICODE_SCRIPT_TAKRI:                Takri. Since: 2.32
 * @XUNICODE_SCRIPT_BASSA_VAH:            Bassa. Since: 2.42
 * @XUNICODE_SCRIPT_CAUCASIAN_ALBANIAN:   Caucasian Albanian. Since: 2.42
 * @XUNICODE_SCRIPT_DUPLOYAN:             Duployan. Since: 2.42
 * @XUNICODE_SCRIPT_ELBASAN:              Elbasan. Since: 2.42
 * @XUNICODE_SCRIPT_GRANTHA:              Grantha. Since: 2.42
 * @XUNICODE_SCRIPT_KHOJKI:               Kjohki. Since: 2.42
 * @XUNICODE_SCRIPT_KHUDAWADI:            Khudawadi, Sindhi. Since: 2.42
 * @XUNICODE_SCRIPT_LINEAR_A:             Linear A. Since: 2.42
 * @XUNICODE_SCRIPT_MAHAJANI:             Mahajani. Since: 2.42
 * @XUNICODE_SCRIPT_MANICHAEAN:           Manichaean. Since: 2.42
 * @XUNICODE_SCRIPT_MENDE_KIKAKUI:        Mende Kikakui. Since: 2.42
 * @XUNICODE_SCRIPT_MODI:                 Modi. Since: 2.42
 * @XUNICODE_SCRIPT_MRO:                  Mro. Since: 2.42
 * @XUNICODE_SCRIPT_NABATAEAN:            Nabataean. Since: 2.42
 * @XUNICODE_SCRIPT_OLD_NORTH_ARABIAN:    Old North Arabian. Since: 2.42
 * @XUNICODE_SCRIPT_OLD_PERMIC:           Old Permic. Since: 2.42
 * @XUNICODE_SCRIPT_PAHAWH_HMONG:         Pahawh Hmong. Since: 2.42
 * @XUNICODE_SCRIPT_PALMYRENE:            Palmyrene. Since: 2.42
 * @XUNICODE_SCRIPT_PAU_CIN_HAU:          Pau Cin Hau. Since: 2.42
 * @XUNICODE_SCRIPT_PSALTER_PAHLAVI:      Psalter Pahlavi. Since: 2.42
 * @XUNICODE_SCRIPT_SIDDHAM:              Siddham. Since: 2.42
 * @XUNICODE_SCRIPT_TIRHUTA:              Tirhuta. Since: 2.42
 * @XUNICODE_SCRIPT_WARANG_CITI:          Warang Citi. Since: 2.42
 * @XUNICODE_SCRIPT_AHOM:                 Ahom. Since: 2.48
 * @XUNICODE_SCRIPT_ANATOLIAN_HIEROGLYPHS: Anatolian Hieroglyphs. Since: 2.48
 * @XUNICODE_SCRIPT_HATRAN:               Hatran. Since: 2.48
 * @XUNICODE_SCRIPT_MULTANI:              Multani. Since: 2.48
 * @XUNICODE_SCRIPT_OLD_HUNGARIAN:        Old Hungarian. Since: 2.48
 * @XUNICODE_SCRIPT_SIGNWRITING:          Signwriting. Since: 2.48
 * @XUNICODE_SCRIPT_ADLAM:                Adlam. Since: 2.50
 * @XUNICODE_SCRIPT_BHAIKSUKI:            Bhaiksuki. Since: 2.50
 * @XUNICODE_SCRIPT_MARCHEN:              Marchen. Since: 2.50
 * @XUNICODE_SCRIPT_NEWA:                 Newa. Since: 2.50
 * @XUNICODE_SCRIPT_OSAGE:                Osage. Since: 2.50
 * @XUNICODE_SCRIPT_TANGUT:               Tangut. Since: 2.50
 * @XUNICODE_SCRIPT_MASARAM_GONDI:        Masaram Gondi. Since: 2.54
 * @XUNICODE_SCRIPT_NUSHU:                Nushu. Since: 2.54
 * @XUNICODE_SCRIPT_SOYOMBO:              Soyombo. Since: 2.54
 * @XUNICODE_SCRIPT_ZANABAZAR_SQUARE:     Zanabazar Square. Since: 2.54
 * @XUNICODE_SCRIPT_DOGRA:                Dogra. Since: 2.58
 * @XUNICODE_SCRIPT_GUNJALA_GONDI:        Gunjala Gondi. Since: 2.58
 * @XUNICODE_SCRIPT_HANIFI_ROHINGYA:      Hanifi Rohingya. Since: 2.58
 * @XUNICODE_SCRIPT_MAKASAR:              Makasar. Since: 2.58
 * @XUNICODE_SCRIPT_MEDEFAIDRIN:          Medefaidrin. Since: 2.58
 * @XUNICODE_SCRIPT_OLD_SOGDIAN:          Old Sogdian. Since: 2.58
 * @XUNICODE_SCRIPT_SOGDIAN:              Sogdian. Since: 2.58
 * @XUNICODE_SCRIPT_ELYMAIC:              Elym. Since: 2.62
 * @XUNICODE_SCRIPT_NANDINAGARI:          Nand. Since: 2.62
 * @XUNICODE_SCRIPT_NYIAKENG_PUACHUE_HMONG: Rohg. Since: 2.62
 * @XUNICODE_SCRIPT_WANCHO:               Wcho. Since: 2.62
 * @XUNICODE_SCRIPT_CHORASMIAN:           Chorasmian. Since: 2.66
 * @XUNICODE_SCRIPT_DIVES_AKURU:          Dives Akuru. Since: 2.66
 * @XUNICODE_SCRIPT_KHITAN_SMALL_SCRIPT:  Khitan small script. Since: 2.66
 * @XUNICODE_SCRIPT_YEZIDI:               Yezidi. Since: 2.66
 * @XUNICODE_SCRIPT_CYPRO_MINOAN:         Cypro-Minoan. Since: 2.72
 * @XUNICODE_SCRIPT_OLD_UYGHUR:           Old Uyghur. Since: 2.72
 * @XUNICODE_SCRIPT_TANGSA:               Tangsa. Since: 2.72
 * @XUNICODE_SCRIPT_TOTO:                 Toto. Since: 2.72
 * @XUNICODE_SCRIPT_VITHKUQI:             Vithkuqi. Since: 2.72
 * @XUNICODE_SCRIPT_MATH:                 Mathematical notation. Since: 2.72
 *
 * The #xunicode_script_t enumeration identifies different writing
 * systems. The values correspond to the names as defined in the
 * Unicode standard. The enumeration has been added in GLib 2.14,
 * and is interchangeable with #PangoScript.
 *
 * Note that new types may be added in the future. Applications
 * should be ready to handle unknown values.
 * See [Unicode Standard Annex #24: Script names](http://www.unicode.org/reports/tr24/).
 */
typedef enum
{                         /* ISO 15924 code */
  XUNICODE_SCRIPT_INVALID_CODE = -1,
  XUNICODE_SCRIPT_COMMON       = 0,   /* Zyyy */
  XUNICODE_SCRIPT_INHERITED,          /* Zinh (Qaai) */
  XUNICODE_SCRIPT_ARABIC,             /* Arab */
  XUNICODE_SCRIPT_ARMENIAN,           /* Armn */
  XUNICODE_SCRIPT_BENGALI,            /* Beng */
  XUNICODE_SCRIPT_BOPOMOFO,           /* Bopo */
  XUNICODE_SCRIPT_CHEROKEE,           /* Cher */
  XUNICODE_SCRIPT_COPTIC,             /* Copt (Qaac) */
  XUNICODE_SCRIPT_CYRILLIC,           /* Cyrl (Cyrs) */
  XUNICODE_SCRIPT_DESERET,            /* Dsrt */
  XUNICODE_SCRIPT_DEVANAGARI,         /* Deva */
  XUNICODE_SCRIPT_ETHIOPIC,           /* Ethi */
  XUNICODE_SCRIPT_GEORGIAN,           /* Geor (Geon, Geoa) */
  XUNICODE_SCRIPT_GOTHIC,             /* Goth */
  XUNICODE_SCRIPT_GREEK,              /* Grek */
  XUNICODE_SCRIPT_GUJARATI,           /* Gujr */
  XUNICODE_SCRIPT_GURMUKHI,           /* Guru */
  XUNICODE_SCRIPT_HAN,                /* Hani */
  XUNICODE_SCRIPT_HANGUL,             /* Hang */
  XUNICODE_SCRIPT_HEBREW,             /* Hebr */
  XUNICODE_SCRIPT_HIRAGANA,           /* Hira */
  XUNICODE_SCRIPT_KANNADA,            /* Knda */
  XUNICODE_SCRIPT_KATAKANA,           /* Kana */
  XUNICODE_SCRIPT_KHMER,              /* Khmr */
  XUNICODE_SCRIPT_LAO,                /* Laoo */
  XUNICODE_SCRIPT_LATIN,              /* Latn (Latf, Latg) */
  XUNICODE_SCRIPT_MALAYALAM,          /* Mlym */
  XUNICODE_SCRIPT_MONGOLIAN,          /* Mong */
  XUNICODE_SCRIPT_MYANMAR,            /* Mymr */
  XUNICODE_SCRIPT_OGHAM,              /* Ogam */
  XUNICODE_SCRIPT_OLD_ITALIC,         /* Ital */
  XUNICODE_SCRIPT_ORIYA,              /* Orya */
  XUNICODE_SCRIPT_RUNIC,              /* Runr */
  XUNICODE_SCRIPT_SINHALA,            /* Sinh */
  XUNICODE_SCRIPT_SYRIAC,             /* Syrc (Syrj, Syrn, Syre) */
  XUNICODE_SCRIPT_TAMIL,              /* Taml */
  XUNICODE_SCRIPT_TELUGU,             /* Telu */
  XUNICODE_SCRIPT_THAANA,             /* Thaa */
  XUNICODE_SCRIPT_THAI,               /* Thai */
  XUNICODE_SCRIPT_TIBETAN,            /* Tibt */
  XUNICODE_SCRIPT_CANADIAN_ABORIGINAL, /* Cans */
  XUNICODE_SCRIPT_YI,                 /* Yiii */
  XUNICODE_SCRIPT_TAGALOG,            /* Tglg */
  XUNICODE_SCRIPT_HANUNOO,            /* Hano */
  XUNICODE_SCRIPT_BUHID,              /* Buhd */
  XUNICODE_SCRIPT_TAGBANWA,           /* Tagb */

  /* Unicode-4.0 additions */
  XUNICODE_SCRIPT_BRAILLE,            /* Brai */
  XUNICODE_SCRIPT_CYPRIOT,            /* Cprt */
  XUNICODE_SCRIPT_LIMBU,              /* Limb */
  XUNICODE_SCRIPT_OSMANYA,            /* Osma */
  XUNICODE_SCRIPT_SHAVIAN,            /* Shaw */
  XUNICODE_SCRIPT_LINEAR_B,           /* Linb */
  XUNICODE_SCRIPT_TAI_LE,             /* Tale */
  XUNICODE_SCRIPT_UGARITIC,           /* Ugar */

  /* Unicode-4.1 additions */
  XUNICODE_SCRIPT_NEW_TAI_LUE,        /* Talu */
  XUNICODE_SCRIPT_BUGINESE,           /* Bugi */
  XUNICODE_SCRIPT_GLAGOLITIC,         /* Glag */
  XUNICODE_SCRIPT_TIFINAGH,           /* Tfng */
  XUNICODE_SCRIPT_SYLOTI_NAGRI,       /* Sylo */
  XUNICODE_SCRIPT_OLD_PERSIAN,        /* Xpeo */
  XUNICODE_SCRIPT_KHAROSHTHI,         /* Khar */

  /* Unicode-5.0 additions */
  XUNICODE_SCRIPT_UNKNOWN,            /* Zzzz */
  XUNICODE_SCRIPT_BALINESE,           /* Bali */
  XUNICODE_SCRIPT_CUNEIFORM,          /* Xsux */
  XUNICODE_SCRIPT_PHOENICIAN,         /* Phnx */
  XUNICODE_SCRIPT_PHAGS_PA,           /* Phag */
  XUNICODE_SCRIPT_NKO,                /* Nkoo */

  /* Unicode-5.1 additions */
  XUNICODE_SCRIPT_KAYAH_LI,           /* Kali */
  XUNICODE_SCRIPT_LEPCHA,             /* Lepc */
  XUNICODE_SCRIPT_REJANG,             /* Rjng */
  XUNICODE_SCRIPT_SUNDANESE,          /* Sund */
  XUNICODE_SCRIPT_SAURASHTRA,         /* Saur */
  XUNICODE_SCRIPT_CHAM,               /* Cham */
  XUNICODE_SCRIPT_OL_CHIKI,           /* Olck */
  XUNICODE_SCRIPT_VAI,                /* Vaii */
  XUNICODE_SCRIPT_CARIAN,             /* Cari */
  XUNICODE_SCRIPT_LYCIAN,             /* Lyci */
  XUNICODE_SCRIPT_LYDIAN,             /* Lydi */

  /* Unicode-5.2 additions */
  XUNICODE_SCRIPT_AVESTAN,                /* Avst */
  XUNICODE_SCRIPT_BAMUM,                  /* Bamu */
  XUNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS,   /* Egyp */
  XUNICODE_SCRIPT_IMPERIAL_ARAMAIC,       /* Armi */
  XUNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI,  /* Phli */
  XUNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN, /* Prti */
  XUNICODE_SCRIPT_JAVANESE,               /* Java */
  XUNICODE_SCRIPT_KAITHI,                 /* Kthi */
  XUNICODE_SCRIPT_LISU,                   /* Lisu */
  XUNICODE_SCRIPT_MEETEI_MAYEK,           /* Mtei */
  XUNICODE_SCRIPT_OLD_SOUTH_ARABIAN,      /* Sarb */
  XUNICODE_SCRIPT_OLD_TURKIC,             /* Orkh */
  XUNICODE_SCRIPT_SAMARITAN,              /* Samr */
  XUNICODE_SCRIPT_TAI_THAM,               /* Lana */
  XUNICODE_SCRIPT_TAI_VIET,               /* Tavt */

  /* Unicode-6.0 additions */
  XUNICODE_SCRIPT_BATAK,                  /* Batk */
  XUNICODE_SCRIPT_BRAHMI,                 /* Brah */
  XUNICODE_SCRIPT_MANDAIC,                /* Mand */

  /* Unicode-6.1 additions */
  XUNICODE_SCRIPT_CHAKMA,                 /* Cakm */
  XUNICODE_SCRIPT_MEROITIC_CURSIVE,       /* Merc */
  XUNICODE_SCRIPT_MEROITIC_HIEROGLYPHS,   /* Mero */
  XUNICODE_SCRIPT_MIAO,                   /* Plrd */
  XUNICODE_SCRIPT_SHARADA,                /* Shrd */
  XUNICODE_SCRIPT_SORA_SOMPENG,           /* Sora */
  XUNICODE_SCRIPT_TAKRI,                  /* Takr */

  /* Unicode 7.0 additions */
  XUNICODE_SCRIPT_BASSA_VAH,              /* Bass */
  XUNICODE_SCRIPT_CAUCASIAN_ALBANIAN,     /* Aghb */
  XUNICODE_SCRIPT_DUPLOYAN,               /* Dupl */
  XUNICODE_SCRIPT_ELBASAN,                /* Elba */
  XUNICODE_SCRIPT_GRANTHA,                /* Gran */
  XUNICODE_SCRIPT_KHOJKI,                 /* Khoj */
  XUNICODE_SCRIPT_KHUDAWADI,              /* Sind */
  XUNICODE_SCRIPT_LINEAR_A,               /* Lina */
  XUNICODE_SCRIPT_MAHAJANI,               /* Mahj */
  XUNICODE_SCRIPT_MANICHAEAN,             /* Mani */
  XUNICODE_SCRIPT_MENDE_KIKAKUI,          /* Mend */
  XUNICODE_SCRIPT_MODI,                   /* Modi */
  XUNICODE_SCRIPT_MRO,                    /* Mroo */
  XUNICODE_SCRIPT_NABATAEAN,              /* Nbat */
  XUNICODE_SCRIPT_OLD_NORTH_ARABIAN,      /* Narb */
  XUNICODE_SCRIPT_OLD_PERMIC,             /* Perm */
  XUNICODE_SCRIPT_PAHAWH_HMONG,           /* Hmng */
  XUNICODE_SCRIPT_PALMYRENE,              /* Palm */
  XUNICODE_SCRIPT_PAU_CIN_HAU,            /* Pauc */
  XUNICODE_SCRIPT_PSALTER_PAHLAVI,        /* Phlp */
  XUNICODE_SCRIPT_SIDDHAM,                /* Sidd */
  XUNICODE_SCRIPT_TIRHUTA,                /* Tirh */
  XUNICODE_SCRIPT_WARANG_CITI,            /* Wara */

  /* Unicode 8.0 additions */
  XUNICODE_SCRIPT_AHOM,                   /* Ahom */
  XUNICODE_SCRIPT_ANATOLIAN_HIEROGLYPHS,  /* Hluw */
  XUNICODE_SCRIPT_HATRAN,                 /* Hatr */
  XUNICODE_SCRIPT_MULTANI,                /* Mult */
  XUNICODE_SCRIPT_OLD_HUNGARIAN,          /* Hung */
  XUNICODE_SCRIPT_SIGNWRITING,            /* Sgnw */

  /* Unicode 9.0 additions */
  XUNICODE_SCRIPT_ADLAM,                  /* Adlm */
  XUNICODE_SCRIPT_BHAIKSUKI,              /* Bhks */
  XUNICODE_SCRIPT_MARCHEN,                /* Marc */
  XUNICODE_SCRIPT_NEWA,                   /* Newa */
  XUNICODE_SCRIPT_OSAGE,                  /* Osge */
  XUNICODE_SCRIPT_TANGUT,                 /* Tang */

  /* Unicode 10.0 additions */
  XUNICODE_SCRIPT_MASARAM_GONDI,          /* Gonm */
  XUNICODE_SCRIPT_NUSHU,                  /* Nshu */
  XUNICODE_SCRIPT_SOYOMBO,                /* Soyo */
  XUNICODE_SCRIPT_ZANABAZAR_SQUARE,       /* Zanb */

  /* Unicode 11.0 additions */
  XUNICODE_SCRIPT_DOGRA,                  /* Dogr */
  XUNICODE_SCRIPT_GUNJALA_GONDI,          /* Gong */
  XUNICODE_SCRIPT_HANIFI_ROHINGYA,        /* Rohg */
  XUNICODE_SCRIPT_MAKASAR,                /* Maka */
  XUNICODE_SCRIPT_MEDEFAIDRIN,            /* Medf */
  XUNICODE_SCRIPT_OLD_SOGDIAN,            /* Sogo */
  XUNICODE_SCRIPT_SOGDIAN,                /* Sogd */

  /* Unicode 12.0 additions */
  XUNICODE_SCRIPT_ELYMAIC,                /* Elym */
  XUNICODE_SCRIPT_NANDINAGARI,            /* Nand */
  XUNICODE_SCRIPT_NYIAKENG_PUACHUE_HMONG, /* Rohg */
  XUNICODE_SCRIPT_WANCHO,                 /* Wcho */

  /* Unicode 13.0 additions */
  XUNICODE_SCRIPT_CHORASMIAN,             /* Chrs */
  XUNICODE_SCRIPT_DIVES_AKURU,            /* Diak */
  XUNICODE_SCRIPT_KHITAN_SMALL_SCRIPT,    /* Kits */
  XUNICODE_SCRIPT_YEZIDI,                 /* Yezi */

  /* Unicode 14.0 additions */
  XUNICODE_SCRIPT_CYPRO_MINOAN,           /* Cpmn */
  XUNICODE_SCRIPT_OLD_UYGHUR,             /* Ougr */
  XUNICODE_SCRIPT_TANGSA,                 /* Tnsa */
  XUNICODE_SCRIPT_TOTO,                   /* Toto */
  XUNICODE_SCRIPT_VITHKUQI,               /* Vith */

  /* not really a Unicode script, but part of ISO 15924 */
  XUNICODE_SCRIPT_MATH,                   /* Zmth */
} xunicode_script_t;

XPL_AVAILABLE_IN_ALL
xuint32_t        xunicode_script_to_iso15924   (xunicode_script_t script);
XPL_AVAILABLE_IN_ALL
xunicode_script_t xunicode_script_from_iso15924 (xuint32_t        iso15924);

/* These are all analogs of the <ctype.h> functions.
 */
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_isalnum   (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_isalpha   (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_iscntrl   (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_isdigit   (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_isgraph   (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_islower   (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_isprint   (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_ispunct   (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_isspace   (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_isupper   (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_isxdigit  (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_istitle   (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_isdefined (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_iswide    (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_iswide_cjk(xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_iszerowidth(xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_ismark    (xunichar_t c) G_GNUC_CONST;

/* More <ctype.h> functions.  These convert between the three cases.
 * See the Unicode book to understand title case.  */
XPL_AVAILABLE_IN_ALL
xunichar_t xunichar_toupper (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xunichar_t xunichar_tolower (xunichar_t c) G_GNUC_CONST;
XPL_AVAILABLE_IN_ALL
xunichar_t xunichar_totitle (xunichar_t c) G_GNUC_CONST;

/* If C is a digit (according to 'xunichar_isdigit'), then return its
   numeric value.  Otherwise return -1.  */
XPL_AVAILABLE_IN_ALL
xint_t xunichar_digit_value (xunichar_t c) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xint_t xunichar_xdigit_value (xunichar_t c) G_GNUC_CONST;

/* Return the Unicode character type of a given character.  */
XPL_AVAILABLE_IN_ALL
xunicode_type_t xunichar_type (xunichar_t c) G_GNUC_CONST;

/* Return the line break property for a given character */
XPL_AVAILABLE_IN_ALL
GUnicodeBreakType xunichar_break_type (xunichar_t c) G_GNUC_CONST;

/* Returns the combining class for a given character */
XPL_AVAILABLE_IN_ALL
xint_t xunichar_combining_class (xunichar_t uc) G_GNUC_CONST;

XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_get_mirror_char (xunichar_t ch,
                                    xunichar_t *mirrored_ch);

XPL_AVAILABLE_IN_ALL
xunicode_script_t xunichar_get_script (xunichar_t ch) G_GNUC_CONST;

/* Validate a Unicode character */
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_validate (xunichar_t ch) G_GNUC_CONST;

/* Pairwise canonical compose/decompose */
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_compose (xunichar_t  a,
                            xunichar_t  b,
                            xunichar_t *ch);
XPL_AVAILABLE_IN_ALL
xboolean_t xunichar_decompose (xunichar_t  ch,
                              xunichar_t *a,
                              xunichar_t *b);

XPL_AVAILABLE_IN_ALL
xsize_t xunichar_fully_decompose (xunichar_t  ch,
                                 xboolean_t  compat,
                                 xunichar_t *result,
                                 xsize_t     result_len);

/**
 * G_UNICHAR_MAX_DECOMPOSITION_LENGTH:
 *
 * The maximum length (in codepoints) of a compatibility or canonical
 * decomposition of a single Unicode character.
 *
 * This is as defined by Unicode 6.1.
 *
 * Since: 2.32
 */
#define G_UNICHAR_MAX_DECOMPOSITION_LENGTH 18 /* codepoints */

/* Compute canonical ordering of a string in-place.  This rearranges
   decomposed characters in the string according to their combining
   classes.  See the Unicode manual for more information.  */
XPL_AVAILABLE_IN_ALL
void xunicode_canonical_ordering (xunichar_t *string,
                                   xsize_t     len);


XPL_DEPRECATED_IN_2_30
xunichar_t *xunicode_canonical_decomposition (xunichar_t  ch,
                                             xsize_t    *result_len) G_GNUC_MALLOC;

/* Array of skip-bytes-per-initial character.
 */
XPL_VAR const xchar_t * const xutf8_skip;

/**
 * xutf8_next_char:
 * @p: Pointer to the start of a valid UTF-8 character
 *
 * Skips to the next character in a UTF-8 string.
 *
 * The string must be valid; this macro is as fast as possible, and has
 * no error-checking.
 *
 * You would use this macro to iterate over a string character by character.
 *
 * The macro returns the start of the next UTF-8 character.
 *
 * Before using this macro, use xutf8_validate() to validate strings
 * that may contain invalid UTF-8.
 */
#define xutf8_next_char(p) (char *)((p) + xutf8_skip[*(const xuchar_t *)(p)])

XPL_AVAILABLE_IN_ALL
xunichar_t xutf8_get_char           (const xchar_t  *p) G_GNUC_PURE;
XPL_AVAILABLE_IN_ALL
xunichar_t xutf8_get_char_validated (const  xchar_t *p,
                                    xssize_t        max_len) G_GNUC_PURE;

XPL_AVAILABLE_IN_ALL
xchar_t*   xutf8_offset_to_pointer (const xchar_t *str,
                                   xlong_t        offset) G_GNUC_PURE;
XPL_AVAILABLE_IN_ALL
xlong_t    xutf8_pointer_to_offset (const xchar_t *str,
                                   const xchar_t *pos) G_GNUC_PURE;
XPL_AVAILABLE_IN_ALL
xchar_t*   xutf8_prev_char         (const xchar_t *p) G_GNUC_PURE;
XPL_AVAILABLE_IN_ALL
xchar_t*   xutf8_find_next_char    (const xchar_t *p,
                                   const xchar_t *end) G_GNUC_PURE;
XPL_AVAILABLE_IN_ALL
xchar_t*   xutf8_find_prev_char    (const xchar_t *str,
                                   const xchar_t *p) G_GNUC_PURE;

XPL_AVAILABLE_IN_ALL
xlong_t    xutf8_strlen            (const xchar_t *p,
                                   xssize_t       max) G_GNUC_PURE;

XPL_AVAILABLE_IN_2_30
xchar_t   *xutf8_substring         (const xchar_t *str,
                                   xlong_t        start_pos,
                                   xlong_t        end_pos) G_GNUC_MALLOC;

XPL_AVAILABLE_IN_ALL
xchar_t   *xutf8_strncpy           (xchar_t       *dest,
                                   const xchar_t *src,
                                   xsize_t        n);

/* Find the UTF-8 character corresponding to ch, in string p. These
   functions are equivalants to strchr and strrchr */
XPL_AVAILABLE_IN_ALL
xchar_t* xutf8_strchr  (const xchar_t *p,
                       xssize_t       len,
                       xunichar_t     c);
XPL_AVAILABLE_IN_ALL
xchar_t* xutf8_strrchr (const xchar_t *p,
                       xssize_t       len,
                       xunichar_t     c);
XPL_AVAILABLE_IN_ALL
xchar_t* xutf8_strreverse (const xchar_t *str,
                          xssize_t len);

XPL_AVAILABLE_IN_ALL
xunichar2_t *xutf8_to_utf16     (const xchar_t      *str,
                                xlong_t             len,
                                xlong_t            *items_read,
                                xlong_t            *items_written,
                                xerror_t          **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xunichar_t * xutf8_to_ucs4      (const xchar_t      *str,
                                xlong_t             len,
                                xlong_t            *items_read,
                                xlong_t            *items_written,
                                xerror_t          **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xunichar_t * xutf8_to_ucs4_fast (const xchar_t      *str,
                                xlong_t             len,
                                xlong_t            *items_written) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xunichar_t * xutf16_to_ucs4     (const xunichar2_t  *str,
                                xlong_t             len,
                                xlong_t            *items_read,
                                xlong_t            *items_written,
                                xerror_t          **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t*     xutf16_to_utf8     (const xunichar2_t  *str,
                                xlong_t             len,
                                xlong_t            *items_read,
                                xlong_t            *items_written,
                                xerror_t          **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xunichar2_t *g_ucs4_to_utf16     (const xunichar_t   *str,
                                xlong_t             len,
                                xlong_t            *items_read,
                                xlong_t            *items_written,
                                xerror_t          **error) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t*     g_ucs4_to_utf8      (const xunichar_t   *str,
                                xlong_t             len,
                                xlong_t            *items_read,
                                xlong_t            *items_written,
                                xerror_t          **error) G_GNUC_MALLOC;

XPL_AVAILABLE_IN_ALL
xint_t      xunichar_to_utf8 (xunichar_t    c,
                             xchar_t      *outbuf);

XPL_AVAILABLE_IN_ALL
xboolean_t xutf8_validate (const xchar_t  *str,
                          xssize_t        max_len,
                          const xchar_t **end);
XPL_AVAILABLE_IN_2_60
xboolean_t xutf8_validate_len (const xchar_t  *str,
                              xsize_t         max_len,
                              const xchar_t **end);

XPL_AVAILABLE_IN_ALL
xchar_t *xutf8_strup   (const xchar_t *str,
                       xssize_t       len) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t *xutf8_strdown (const xchar_t *str,
                       xssize_t       len) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t *xutf8_casefold (const xchar_t *str,
                        xssize_t       len) G_GNUC_MALLOC;

/**
 * xnormalize_mode:
 * @XNORMALIZE_DEFAULT: standardize differences that do not affect the
 *     text content, such as the above-mentioned accent representation
 * @XNORMALIZE_NFD: another name for %XNORMALIZE_DEFAULT
 * @XNORMALIZE_DEFAULT_COMPOSE: like %XNORMALIZE_DEFAULT, but with
 *     composed forms rather than a maximally decomposed form
 * @XNORMALIZE_NFC: another name for %XNORMALIZE_DEFAULT_COMPOSE
 * @XNORMALIZE_ALL: beyond %XNORMALIZE_DEFAULT also standardize the
 *     "compatibility" characters in Unicode, such as SUPERSCRIPT THREE
 *     to the standard forms (in this case DIGIT THREE). Formatting
 *     information may be lost but for most text operations such
 *     characters should be considered the same
 * @XNORMALIZE_NFKD: another name for %XNORMALIZE_ALL
 * @XNORMALIZE_ALL_COMPOSE: like %XNORMALIZE_ALL, but with composed
 *     forms rather than a maximally decomposed form
 * @XNORMALIZE_NFKC: another name for %XNORMALIZE_ALL_COMPOSE
 *
 * Defines how a Unicode string is transformed in a canonical
 * form, standardizing such issues as whether a character with
 * an accent is represented as a base character and combining
 * accent or as a single precomposed character. Unicode strings
 * should generally be normalized before comparing them.
 */
typedef enum {
  XNORMALIZE_DEFAULT,
  XNORMALIZE_NFD = XNORMALIZE_DEFAULT,
  XNORMALIZE_DEFAULT_COMPOSE,
  XNORMALIZE_NFC = XNORMALIZE_DEFAULT_COMPOSE,
  XNORMALIZE_ALL,
  XNORMALIZE_NFKD = XNORMALIZE_ALL,
  XNORMALIZE_ALL_COMPOSE,
  XNORMALIZE_NFKC = XNORMALIZE_ALL_COMPOSE
} xnormalize_mode_t;

XPL_AVAILABLE_IN_ALL
xchar_t *xutf8_normalize (const xchar_t   *str,
                         xssize_t         len,
                         xnormalize_mode_t mode) G_GNUC_MALLOC;

XPL_AVAILABLE_IN_ALL
xint_t   xutf8_collate     (const xchar_t *str1,
                           const xchar_t *str2) G_GNUC_PURE;
XPL_AVAILABLE_IN_ALL
xchar_t *xutf8_collate_key (const xchar_t *str,
                           xssize_t       len) G_GNUC_MALLOC;
XPL_AVAILABLE_IN_ALL
xchar_t *xutf8_collate_key_for_filename (const xchar_t *str,
                                        xssize_t       len) G_GNUC_MALLOC;

XPL_AVAILABLE_IN_2_52
xchar_t *xutf8_make_valid (const xchar_t *str,
                          xssize_t       len) G_GNUC_MALLOC;

G_END_DECLS

#endif /* __XUNICODE_H__ */
