/* gcharset.c - Charset information
 *
 * Copyright (C) 2011 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gcharset.h"
#include "gcharsetprivate.h"

#include "garray.h"
#include "genviron.h"
#include "ghash.h"
#include "gmessages.h"
#include "gstrfuncs.h"
#include "gthread.h"
#include "gthreadprivate.h"
#ifdef G_OS_WIN32
#include "gwin32.h"
#endif

#include "libcharset/libcharset.h"

#include <string.h>
#include <stdio.h>

#if (HAVE_LANGINFO_TIME_CODESET || HAVE_LANGINFO_CODESET)
#include <langinfo.h>
#endif

#include <locale.h>
#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

G_LOCK_DEFINE_STATIC (aliases);

static xhashtable_t *
get_alias_hash (void)
{
  static xhashtable_t *alias_hash = NULL;
  const char *aliases;

  G_LOCK (aliases);

  if (!alias_hash)
    {
      alias_hash = xhash_table_new (xstr_hash, xstr_equal);

      aliases = _g_locale_get_charset_aliases ();
      while (*aliases != '\0')
        {
          const char *canonical;
          const char *alias;
          const char **alias_array;
          int count = 0;

          alias = aliases;
          aliases += strlen (aliases) + 1;
          canonical = aliases;
          aliases += strlen (aliases) + 1;

          alias_array = xhash_table_lookup (alias_hash, canonical);
          if (alias_array)
            {
              while (alias_array[count])
                count++;
            }

          alias_array = g_renew (const char *, alias_array, count + 2);
          alias_array[count] = alias;
          alias_array[count + 1] = NULL;

          xhash_table_insert (alias_hash, (char *)canonical, alias_array);
        }
    }

  G_UNLOCK (aliases);

  return alias_hash;
}

/* As an abuse of the alias table, the following routines gets
 * the charsets that are aliases for the canonical name.
 */
const char **
_g_charset_get_aliases (const char *canonical_name)
{
  xhashtable_t *alias_hash = get_alias_hash ();

  return xhash_table_lookup (alias_hash, canonical_name);
}

static xboolean_t
xutf8_get_charset_internal (const char  *raw_data,
                             const char **a)
{
  /* Allow CHARSET to override the charset of any locale category. Users should
   * probably never be setting this — instead, just add the charset after a `.`
   * in `LANGUAGE`/`LC_ALL`/`LC_*`/`LANG`. I can’t find any reference (in
   * `git log`, code comments, or man pages) to this environment variable being
   * standardised or documented or even used anywhere outside GLib. Perhaps it
   * should eventually be removed. */
  const char *charset = g_getenv ("CHARSET");

  if (charset && *charset)
    {
      *a = charset;

      if (charset && strstr (charset, "UTF-8"))
        return TRUE;
      else
        return FALSE;
    }

  /* The libcharset code tries to be thread-safe without
   * a lock, but has a memory leak and a missing memory
   * barrier, so we lock for it
   */
  G_LOCK (aliases);
  charset = _g_locale_charset_unalias (raw_data);
  G_UNLOCK (aliases);

  if (charset && *charset)
    {
      *a = charset;

      if (charset && strstr (charset, "UTF-8"))
        return TRUE;
      else
        return FALSE;
    }

  /* Assume this for compatibility at present.  */
  *a = "US-ASCII";

  return FALSE;
}

typedef struct _GCharsetCache GCharsetCache;

struct _GCharsetCache {
  xboolean_t is_utf8;
  xchar_t *raw;
  xchar_t *charset;
};

static void
charset_cache_free (xpointer_t data)
{
  GCharsetCache *cache = data;
  g_free (cache->raw);
  g_free (cache->charset);
  g_free (cache);
}

/**
 * g_get_charset:
 * @charset: (out) (optional) (transfer none): return location for character set
 *   name, or %NULL.
 *
 * Obtains the character set for the [current locale][setlocale]; you
 * might use this character set as an argument to g_convert(), to convert
 * from the current locale's encoding to some other encoding. (Frequently
 * g_locale_to_utf8() and g_locale_from_utf8() are nice shortcuts, though.)
 *
 * On Windows the character set returned by this function is the
 * so-called system default ANSI code-page. That is the character set
 * used by the "narrow" versions of C library and Win32 functions that
 * handle file names. It might be different from the character set
 * used by the C library's current locale.
 *
 * On Linux, the character set is found by consulting nl_langinfo() if
 * available. If not, the environment variables `LC_ALL`, `LC_CTYPE`, `LANG`
 * and `CHARSET` are queried in order.
 *
 * The return value is %TRUE if the locale's encoding is UTF-8, in that
 * case you can perhaps avoid calling g_convert().
 *
 * The string returned in @charset is not allocated, and should not be
 * freed.
 *
 * Returns: %TRUE if the returned charset is UTF-8
 */
xboolean_t
g_get_charset (const char **charset)
{
  static GPrivate cache_private = G_PRIVATE_INIT (charset_cache_free);
  GCharsetCache *cache = g_private_get (&cache_private);
  const xchar_t *raw;

  if (!cache)
    cache = g_private_set_alloc0 (&cache_private, sizeof (GCharsetCache));

  G_LOCK (aliases);
  raw = _g_locale_charset_raw ();
  G_UNLOCK (aliases);

  if (cache->raw == NULL || strcmp (cache->raw, raw) != 0)
    {
      const xchar_t *new_charset;

      g_free (cache->raw);
      g_free (cache->charset);
      cache->raw = xstrdup (raw);
      cache->is_utf8 = xutf8_get_charset_internal (raw, &new_charset);
      cache->charset = xstrdup (new_charset);
    }

  if (charset)
    *charset = cache->charset;

  return cache->is_utf8;
}

/*
 * Do the same as g_get_charset() but it temporarily set locale (LC_ALL to
 * LC_TIME) to correctly check for charset about time conversion relatives.
 *
 * Returns: %TRUE if the returned charset is UTF-8
 */
xboolean_t
_g_get_time_charset (const char **charset)
{
  static GPrivate cache_private = G_PRIVATE_INIT (charset_cache_free);
  GCharsetCache *cache = g_private_get (&cache_private);
  const xchar_t *raw;

  if (!cache)
    cache = g_private_set_alloc0 (&cache_private, sizeof (GCharsetCache));

#ifdef HAVE_LANGINFO_TIME_CODESET
  raw = nl_langinfo (_NL_TIME_CODESET);
#else
  G_LOCK (aliases);
  raw = _g_locale_charset_raw ();
  G_UNLOCK (aliases);
#endif

  if (cache->raw == NULL || strcmp (cache->raw, raw) != 0)
    {
      const xchar_t *new_charset;

      g_free (cache->raw);
      g_free (cache->charset);
      cache->raw = xstrdup (raw);
      cache->is_utf8 = xutf8_get_charset_internal (raw, &new_charset);
      cache->charset = xstrdup (new_charset);
    }

  if (charset)
    *charset = cache->charset;

  return cache->is_utf8;
}
/*
 * Do the same as g_get_charset() but it temporarily set locale (LC_ALL to
 * LC_CTYPE) to correctly check for charset about CTYPE conversion relatives.
 *
 * Returns: %TRUE if the returned charset is UTF-8
 */
xboolean_t
_g_get_ctype_charset (const char **charset)
{
  static GPrivate cache_private = G_PRIVATE_INIT (charset_cache_free);
  GCharsetCache *cache = g_private_get (&cache_private);
  const xchar_t *raw;

  if (!cache)
    cache = g_private_set_alloc0 (&cache_private, sizeof (GCharsetCache));

#ifdef HAVE_LANGINFO_CODESET
  raw = nl_langinfo (CODESET);
#else
  G_LOCK (aliases);
  raw = _g_locale_charset_raw ();
  G_UNLOCK (aliases);
#endif

  if (cache->raw == NULL || strcmp (cache->raw, raw) != 0)
    {
      const xchar_t *new_charset;

      g_free (cache->raw);
      g_free (cache->charset);
      cache->raw = xstrdup (raw);
      cache->is_utf8 = xutf8_get_charset_internal (raw, &new_charset);
      cache->charset = xstrdup (new_charset);
    }

  if (charset)
    *charset = cache->charset;

  return cache->is_utf8;
}

/**
 * g_get_codeset:
 *
 * Gets the character set for the current locale.
 *
 * Returns: a newly allocated string containing the name
 *     of the character set. This string must be freed with g_free().
 */
xchar_t *
g_get_codeset (void)
{
  const xchar_t *charset;

  g_get_charset (&charset);

  return xstrdup (charset);
}

/**
 * g_get_console_charset:
 * @charset: (out) (optional) (transfer none): return location for character set
 *   name, or %NULL.
 *
 * Obtains the character set used by the console attached to the process,
 * which is suitable for printing output to the terminal.
 *
 * Usually this matches the result returned by g_get_charset(), but in
 * environments where the locale's character set does not match the encoding
 * of the console this function tries to guess a more suitable value instead.
 *
 * On Windows the character set returned by this function is the
 * output code page used by the console associated with the calling process.
 * If the codepage can't be determined (for example because there is no
 * console attached) UTF-8 is assumed.
 *
 * The return value is %TRUE if the locale's encoding is UTF-8, in that
 * case you can perhaps avoid calling g_convert().
 *
 * The string returned in @charset is not allocated, and should not be
 * freed.
 *
 * Returns: %TRUE if the returned charset is UTF-8
 *
 * Since: 2.62
 */
xboolean_t
g_get_console_charset (const char **charset)
{
#ifdef G_OS_WIN32
  static GPrivate cache_private = G_PRIVATE_INIT (charset_cache_free);
  GCharsetCache *cache = g_private_get (&cache_private);
  const xchar_t *locale;
  unsigned int cp;
  char buf[2 + 20 + 1]; /* "CP" + G_MAXUINT64 (to be safe) in decimal form (20 bytes) + "\0" */
  const xchar_t *raw = NULL;

  if (!cache)
    cache = g_private_set_alloc0 (&cache_private, sizeof (GCharsetCache));

  /* first try to query $LANG (works for Cygwin/MSYS/MSYS2 and others using mintty) */
  locale = g_getenv ("LANG");
  if (locale != NULL && locale[0] != '\0')
    {
      /* If the locale name contains an encoding after the dot, return it.  */
      const char *dot = strchr (locale, '.');

      if (dot != NULL)
        {
          const char *modifier;

          dot++;
          /* Look for the possible @... trailer and remove it, if any.  */
          modifier = strchr (dot, '@');
          if (modifier == NULL)
            raw = dot;
          else if ((xsize_t) (modifier - dot) < sizeof (buf))
            {
              memcpy (buf, dot, modifier - dot);
              buf[modifier - dot] = '\0';
              raw = buf;
            }
        }
    }
  /* next try querying console codepage using native win32 API */
  if (raw == NULL)
    {
      cp = GetConsoleOutputCP ();
      if (cp)
        {
          sprintf (buf, "CP%u", cp);
          raw = buf;
        }
      else if (GetLastError () != ERROR_INVALID_HANDLE)
        {
          xchar_t *emsg = g_win32_error_message (GetLastError ());
          g_warning ("Failed to determine console output code page: %s. "
                     "Falling back to UTF-8", emsg);
          g_free (emsg);
        }
    }
  /* fall-back to UTF-8 if the rest failed (it's a universal default) */
  if (raw == NULL)
    raw = "UTF-8";

  if (cache->raw == NULL || strcmp (cache->raw, raw) != 0)
    {
      const xchar_t *new_charset;

      g_free (cache->raw);
      g_free (cache->charset);
      cache->raw = xstrdup (raw);
      cache->is_utf8 = xutf8_get_charset_internal (raw, &new_charset);
      cache->charset = xstrdup (new_charset);
    }

  if (charset)
    *charset = cache->charset;

  return cache->is_utf8;
#else
  /* assume the locale settings match the console encoding on non-Windows OSs */
  return g_get_charset (charset);
#endif
}

#ifndef G_OS_WIN32

/* read an alias file for the locales */
static void
read_aliases (const xchar_t *file,
              xhashtable_t  *alias_table)
{
  FILE *fp;
  char buf[256];

  fp = fopen (file,"r");
  if (!fp)
    return;
  while (fgets (buf, 256, fp))
    {
      char *p, *q;

      xstrstrip (buf);

      /* Line is a comment */
      if ((buf[0] == '#') || (buf[0] == '\0'))
        continue;

      /* Reads first column */
      for (p = buf, q = NULL; *p; p++) {
        if ((*p == '\t') || (*p == ' ') || (*p == ':')) {
          *p = '\0';
          q = p+1;
          while ((*q == '\t') || (*q == ' ')) {
            q++;
          }
          break;
        }
      }
      /* The line only had one column */
      if (!q || *q == '\0')
        continue;

      /* Read second column */
      for (p = q; *p; p++) {
        if ((*p == '\t') || (*p == ' ')) {
          *p = '\0';
          break;
        }
      }

      /* Add to alias table if necessary */
      if (!xhash_table_lookup (alias_table, buf)) {
        xhash_table_insert (alias_table, xstrdup (buf), xstrdup (q));
      }
    }
  fclose (fp);
}

#endif

static char *
unalias_lang (char *lang)
{
#ifndef G_OS_WIN32
  static xhashtable_t *alias_table = NULL;
  char *p;
  int i;

  if (g_once_init_enter (&alias_table))
    {
      xhashtable_t *table = xhash_table_new (xstr_hash, xstr_equal);
      read_aliases ("/usr/share/locale/locale.alias", table);
      g_once_init_leave (&alias_table, table);
    }

  i = 0;
  while ((p = xhash_table_lookup (alias_table, lang)) && (strcmp (p, lang) != 0))
    {
      lang = p;
      if (i++ == 30)
        {
          static xboolean_t said_before = FALSE;
          if (!said_before)
            g_warning ("Too many alias levels for a locale, "
                       "may indicate a loop");
          said_before = TRUE;
          return lang;
        }
    }
#endif
  return lang;
}

/* Mask for components of locale spec. The ordering here is from
 * least significant to most significant
 */
enum
{
  COMPONENT_CODESET =   1 << 0,
  COMPONENT_TERRITORY = 1 << 1,
  COMPONENT_MODIFIER =  1 << 2
};

/* Break an X/Open style locale specification into components
 */
static xuint_t
explode_locale (const xchar_t *locale,
                xchar_t      **language,
                xchar_t      **territory,
                xchar_t      **codeset,
                xchar_t      **modifier)
{
  const xchar_t *uscore_pos;
  const xchar_t *at_pos;
  const xchar_t *dot_pos;

  xuint_t mask = 0;

  uscore_pos = strchr (locale, '_');
  dot_pos = strchr (uscore_pos ? uscore_pos : locale, '.');
  at_pos = strchr (dot_pos ? dot_pos : (uscore_pos ? uscore_pos : locale), '@');

  if (at_pos)
    {
      mask |= COMPONENT_MODIFIER;
      *modifier = xstrdup (at_pos);
    }
  else
    at_pos = locale + strlen (locale);

  if (dot_pos)
    {
      mask |= COMPONENT_CODESET;
      *codeset = xstrndup (dot_pos, at_pos - dot_pos);
    }
  else
    dot_pos = at_pos;

  if (uscore_pos)
    {
      mask |= COMPONENT_TERRITORY;
      *territory = xstrndup (uscore_pos, dot_pos - uscore_pos);
    }
  else
    uscore_pos = dot_pos;

  *language = xstrndup (locale, uscore_pos - locale);

  return mask;
}

/*
 * Compute all interesting variants for a given locale name -
 * by stripping off different components of the value.
 *
 * For simplicity, we assume that the locale is in
 * X/Open format: language[_territory][.codeset][@modifier]
 *
 * TODO: Extend this to handle the CEN format (see the GNUlibc docs)
 *       as well. We could just copy the code from glibc wholesale
 *       but it is big, ugly, and complicated, so I'm reluctant
 *       to do so when this should handle 99% of the time...
 */
static void
append_locale_variants (xptr_array_t *array,
                        const xchar_t *locale)
{
  xchar_t *language = NULL;
  xchar_t *territory = NULL;
  xchar_t *codeset = NULL;
  xchar_t *modifier = NULL;

  xuint_t mask;
  xuint_t i, j;

  g_return_if_fail (locale != NULL);

  mask = explode_locale (locale, &language, &territory, &codeset, &modifier);

  /* Iterate through all possible combinations, from least attractive
   * to most attractive.
   */
  for (j = 0; j <= mask; ++j)
    {
      i = mask - j;

      if ((i & ~mask) == 0)
        {
          xchar_t *val = xstrconcat (language,
                                    (i & COMPONENT_TERRITORY) ? territory : "",
                                    (i & COMPONENT_CODESET) ? codeset : "",
                                    (i & COMPONENT_MODIFIER) ? modifier : "",
                                    NULL);
          xptr_array_add (array, val);
        }
    }

  g_free (language);
  if (mask & COMPONENT_CODESET)
    g_free (codeset);
  if (mask & COMPONENT_TERRITORY)
    g_free (territory);
  if (mask & COMPONENT_MODIFIER)
    g_free (modifier);
}

/**
 * g_get_locale_variants:
 * @locale: a locale identifier
 *
 * Returns a list of derived variants of @locale, which can be used to
 * e.g. construct locale-dependent filenames or search paths. The returned
 * list is sorted from most desirable to least desirable.
 * This function handles territory, charset and extra locale modifiers. See
 * [`setlocale(3)`](man:setlocale) for information about locales and their format.
 *
 * @locale itself is guaranteed to be returned in the output.
 *
 * For example, if @locale is `fr_BE`, then the returned list
 * is `fr_BE`, `fr`. If @locale is `en_GB.UTF-8@euro`, then the returned list
 * is `en_GB.UTF-8@euro`, `en_GB.UTF-8`, `en_GB@euro`, `en_GB`, `en.UTF-8@euro`,
 * `en.UTF-8`, `en@euro`, `en`.
 *
 * If you need the list of variants for the current locale,
 * use g_get_language_names().
 *
 * Returns: (transfer full) (array zero-terminated=1) (element-type utf8): a newly
 *   allocated array of newly allocated strings with the locale variants. Free with
 *   xstrfreev().
 *
 * Since: 2.28
 */
xchar_t **
g_get_locale_variants (const xchar_t *locale)
{
  xptr_array_t *array;

  g_return_val_if_fail (locale != NULL, NULL);

  array = xptr_array_sized_new (8);
  append_locale_variants (array, locale);
  xptr_array_add (array, NULL);

  return (xchar_t **) xptr_array_free (array, FALSE);
}

/* The following is (partly) taken from the gettext package.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.  */

static const xchar_t *
guess_category_value (const xchar_t *category_name)
{
  const xchar_t *retval;

  /* The highest priority value is the 'LANGUAGE' environment
     variable.  This is a GNU extension.  */
  retval = g_getenv ("LANGUAGE");
  if ((retval != NULL) && (retval[0] != '\0'))
    return retval;

  /* 'LANGUAGE' is not set.  So we have to proceed with the POSIX
     methods of looking to 'LC_ALL', 'LC_xxx', and 'LANG'.  On some
     systems this can be done by the 'setlocale' function itself.  */

  /* Setting of LC_ALL overwrites all other.  */
  retval = g_getenv ("LC_ALL");
  if ((retval != NULL) && (retval[0] != '\0'))
    return retval;

  /* Next comes the name of the desired category.  */
  retval = g_getenv (category_name);
  if ((retval != NULL) && (retval[0] != '\0'))
    return retval;

  /* Last possibility is the LANG environment variable.  */
  retval = g_getenv ("LANG");
  if ((retval != NULL) && (retval[0] != '\0'))
    return retval;

#ifdef XPLATFORM_WIN32
  /* g_win32_getlocale() first checks for LC_ALL, LC_MESSAGES and
   * LANG, which we already did above. Oh well. The main point of
   * calling g_win32_getlocale() is to get the thread's locale as used
   * by Windows and the Microsoft C runtime (in the "English_United
   * States" format) translated into the Unixish format.
   */
  {
    char *locale = g_win32_getlocale ();
    retval = g_intern_string (locale);
    g_free (locale);
    return retval;
  }
#endif

  return NULL;
}

typedef struct _GLanguageNamesCache GLanguageNamesCache;

struct _GLanguageNamesCache {
  xchar_t *languages;
  xchar_t **language_names;
};

static void
language_names_cache_free (xpointer_t data)
{
  GLanguageNamesCache *cache = data;
  g_free (cache->languages);
  xstrfreev (cache->language_names);
  g_free (cache);
}

/**
 * g_get_language_names:
 *
 * Computes a list of applicable locale names, which can be used to
 * e.g. construct locale-dependent filenames or search paths. The returned
 * list is sorted from most desirable to least desirable and always contains
 * the default locale "C".
 *
 * For example, if LANGUAGE=de:en_US, then the returned list is
 * "de", "en_US", "en", "C".
 *
 * This function consults the environment variables `LANGUAGE`, `LC_ALL`,
 * `LC_MESSAGES` and `LANG` to find the list of locales specified by the
 * user.
 *
 * Returns: (array zero-terminated=1) (transfer none): a %NULL-terminated array of strings owned by GLib
 *    that must not be modified or freed.
 *
 * Since: 2.6
 */
const xchar_t * const *
g_get_language_names (void)
{
  return g_get_language_names_with_category ("LC_MESSAGES");
}

/**
 * g_get_language_names_with_category:
 * @category_name: a locale category name
 *
 * Computes a list of applicable locale names with a locale category name,
 * which can be used to construct the fallback locale-dependent filenames
 * or search paths. The returned list is sorted from most desirable to
 * least desirable and always contains the default locale "C".
 *
 * This function consults the environment variables `LANGUAGE`, `LC_ALL`,
 * @category_name, and `LANG` to find the list of locales specified by the
 * user.
 *
 * g_get_language_names() returns g_get_language_names_with_category("LC_MESSAGES").
 *
 * Returns: (array zero-terminated=1) (transfer none): a %NULL-terminated array of strings owned by
 *    the thread g_get_language_names_with_category was called from.
 *    It must not be modified or freed. It must be copied if planned to be used in another thread.
 *
 * Since: 2.58
 */
const xchar_t * const *
g_get_language_names_with_category (const xchar_t *category_name)
{
  static GPrivate cache_private = G_PRIVATE_INIT ((void (*)(xpointer_t)) xhash_table_unref);
  xhashtable_t *cache = g_private_get (&cache_private);
  const xchar_t *languages;
  GLanguageNamesCache *name_cache;

  g_return_val_if_fail (category_name != NULL, NULL);

  if (!cache)
    {
      cache = xhash_table_new_full (xstr_hash, xstr_equal,
                                     g_free, language_names_cache_free);
      g_private_set (&cache_private, cache);
    }

  languages = guess_category_value (category_name);
  if (!languages)
    languages = "C";

  name_cache = (GLanguageNamesCache *) xhash_table_lookup (cache, category_name);
  if (!(name_cache && name_cache->languages &&
        strcmp (name_cache->languages, languages) == 0))
    {
      xptr_array_t *array;
      xchar_t **alist, **a;

      xhash_table_remove (cache, category_name);

      array = xptr_array_sized_new (8);

      alist = xstrsplit (languages, ":", 0);
      for (a = alist; *a; a++)
        append_locale_variants (array, unalias_lang (*a));
      xstrfreev (alist);
      xptr_array_add (array, xstrdup ("C"));
      xptr_array_add (array, NULL);

      name_cache = g_new0 (GLanguageNamesCache, 1);
      name_cache->languages = xstrdup (languages);
      name_cache->language_names = (xchar_t **) xptr_array_free (array, FALSE);
      xhash_table_insert (cache, xstrdup (category_name), name_cache);
    }

  return (const xchar_t * const *) name_cache->language_names;
}
