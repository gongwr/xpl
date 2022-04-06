/* giowin32-private.c - private glib-gio functions for W32 xapp_info_t
 *
 * Copyright 2019 Руслан Ижбулатов
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


static xsize_t
xutf16_len (const xunichar2_t *str)
{
  xsize_t result;

  for (result = 0; str[0] != 0; str++, result++)
    ;

  return result;
}

static xunichar2_t *
g_wcsdup (const xunichar2_t *str, xssize_t str_len)
{
  xsize_t str_len_unsigned;
  xsize_t str_size;

  g_return_val_if_fail (str != NULL, NULL);

  if (str_len < 0)
    str_len_unsigned = xutf16_len (str);
  else
    str_len_unsigned = (xsize_t) str_len;

  g_assert (str_len_unsigned <= G_MAXSIZE / sizeof (xunichar2_t) - 1);
  str_size = (str_len_unsigned + 1) * sizeof (xunichar2_t);

  return g_memdup2 (str, str_size);
}

static const xunichar2_t *
xutf16_wchr (const xunichar2_t *str, const wchar_t wchr)
{
  for (; str != NULL && str[0] != 0; str++)
    if ((wchar_t) str[0] == wchr)
      return str;

  return NULL;
}

static xboolean_t
xutf16_to_utf8_and_fold (const xunichar2_t  *str,
                          xssize_t            length,
                          xchar_t           **str_u8,
                          xchar_t           **str_u8_folded)
{
  xchar_t *u8;
  xchar_t *folded;
  u8 = xutf16_to_utf8 (str, length, NULL, NULL, NULL);

  if (u8 == NULL)
    return FALSE;

  folded = xutf8_casefold (u8, -1);

  if (str_u8)
    *str_u8 = g_steal_pointer (&u8);

  g_free (u8);

  if (str_u8_folded)
    *str_u8_folded = g_steal_pointer (&folded);

  g_free (folded);

  return TRUE;
}

/* Finds the last directory separator in @filename,
 * returns a pointer to the position after that separator.
 * If the string ends with a separator, returned value
 * will be pointing at the NUL terminator.
 * If the string does not contain separators, returns the
 * string itself.
 */
static const xunichar2_t *
xutf16_find_basename (const xunichar2_t *filename,
                       xssize_t           len)
{
  const xunichar2_t *result;

  if (len < 0)
    len = xutf16_len (filename);
  if (len == 0)
    return filename;

  result = &filename[len - 1];

  while (result > filename)
    {
      if ((wchar_t) result[0] == L'/' ||
          (wchar_t) result[0] == L'\\')
        {
          result += 1;
          break;
        }

      result -= 1;
    }

  return result;
}

/* Finds the last directory separator in @filename,
 * returns a pointer to the position after that separator.
 * If the string ends with a separator, returned value
 * will be pointing at the NUL terminator.
 * If the string does not contain separators, returns the
 * string itself.
 */
static const xchar_t *
xutf8_find_basename (const xchar_t *filename,
                      xssize_t       len)
{
  const xchar_t *result;

  if (len < 0)
    len = strlen (filename);
  if (len == 0)
    return filename;

  result = &filename[len - 1];

  while (result > filename)
    {
      if (result[0] == '/' ||
          result[0] == '\\')
        {
          result += 1;
          break;
        }

      result -= 1;
    }

  return result;
}

/**
 * Parses @commandline, figuring out what the filename being invoked
 * is. All returned strings are pointers into @commandline.
 * @commandline must be a valid UTF-16 string and not be NULL.
 * @after_executable is the first character after executable
 * (usually a space, but not always).
 * If @comma_separator is TRUE, accepts ',' as a separator between
 * the filename and the following argument.
 */
static void
_g_win32_parse_filename (const xunichar2_t  *commandline,
                         xboolean_t          comma_separator,
                         const xunichar2_t **executable_start,
                         xssize_t           *executable_len,
                         const xunichar2_t **executable_basename,
                         const xunichar2_t **after_executable)
{
  const xunichar2_t *p;
  const xunichar2_t *first_argument;
  xboolean_t quoted;
  xssize_t len;
  xssize_t execlen;
  xboolean_t found;

  while ((wchar_t) commandline[0] == L' ')
    commandline++;

  quoted = FALSE;
  execlen = 0;
  found = FALSE;
  first_argument = NULL;

  if ((wchar_t) commandline[0] == L'"')
    {
      quoted = TRUE;
      commandline += 1;
    }

  len = xutf16_len (commandline);
  p = commandline;

  while (p < &commandline[len])
    {
      switch ((wchar_t) p[0])
        {
        case L'"':
          if (quoted)
            {
              first_argument = p + 1;
              /* Note: this is a valid commandline for opening "c:/file.txt":
               * > "notepad"c:/file.txt
               */
              p = &commandline[len];
              found = TRUE;
            }
          else
            execlen += 1;
          break;
        case L' ':
          if (!quoted)
            {
              first_argument = p;
              p = &commandline[len];
              found = TRUE;
            }
          else
            execlen += 1;
          break;
        case L',':
          if (!quoted && comma_separator)
            {
              first_argument = p;
              p = &commandline[len];
              found = TRUE;
            }
          else
            execlen += 1;
          break;
        default:
          execlen += 1;
          break;
        }
      p += 1;
    }

  if (!found)
    first_argument = &commandline[len];

  if (executable_start)
    *executable_start = commandline;

  if (executable_len)
    *executable_len = execlen;

  if (executable_basename)
    *executable_basename = xutf16_find_basename (commandline, execlen);

  if (after_executable)
    *after_executable = first_argument;
}

/* Make sure @commandline is a valid UTF-16 string before
 * calling this function!
 * follow_class_chain_to_handler() does perform such validation.
 */
static void
_g_win32_extract_executable (const xunichar2_t  *commandline,
                             xchar_t           **ex_out,
                             xchar_t           **ex_basename_out,
                             xchar_t           **ex_folded_out,
                             xchar_t           **ex_folded_basename_out,
                             xchar_t           **dll_function_out)
{
  xchar_t *ex;
  xchar_t *ex_folded;
  const xunichar2_t *first_argument;
  const xunichar2_t *executable;
  const xunichar2_t *executable_basename;
  xboolean_t quoted;
  xboolean_t folded;
  xssize_t execlen;

  _g_win32_parse_filename (commandline, FALSE, &executable, &execlen, &executable_basename, &first_argument);

  commandline = executable;

  while ((wchar_t) first_argument[0] == L' ')
    first_argument++;

  folded = xutf16_to_utf8_and_fold (executable, (xssize_t) execlen, &ex, &ex_folded);
  /* This should never fail as @executable has to be valid UTF-16. */
  g_assert (folded);

  if (dll_function_out)
    *dll_function_out = NULL;

  /* See if the executable basename is "rundll32.exe". If so, then
   * parse the rest of the commandline as r'"?path-to-dll"?[ ]*,*[ ]*dll_function_to_invoke'
   */
  /* Using just "rundll32.exe", without an absolute path, seems
   * very exploitable, but MS does that sometimes, so we have
   * to accept that.
   */
  if ((xstrcmp0 (ex_folded, "rundll32.exe") == 0 ||
       xstr_has_suffix (ex_folded, "\\rundll32.exe") ||
       xstr_has_suffix (ex_folded, "/rundll32.exe")) &&
      first_argument[0] != 0 &&
      dll_function_out != NULL)
    {
      /* Corner cases:
       * > rundll32.exe c:\some,file,with,commas.dll,some_function
       * is treated by rundll32 as:
       * dll=c:\some
       * function=file,with,commas.dll,some_function
       * unless the dll name is surrounded by double quotation marks:
       * > rundll32.exe "c:\some,file,with,commas.dll",some_function
       * in which case everything works normally.
       * Also, quoting only works if it surrounds the file name, i.e:
       * > rundll32.exe "c:\some,file"",with,commas.dll",some_function
       * will not work.
       * Also, comma is optional when filename is quoted or when function
       * name is separated from the filename by space(s):
       * > rundll32.exe "c:\some,file,with,commas.dll"some_function
       * will work,
       * > rundll32.exe c:\some_dll_without_commas_or_spaces.dll some_function
       * will work too.
       * Also, any number of commas is accepted:
       * > rundll32.exe c:\some_dll_without_commas_or_spaces.dll , , ,,, , some_function
       * works just fine.
       * And the ultimate example is:
       * > "rundll32.exe""c:\some,file,with,commas.dll"some_function
       * and it also works.
       * Good job, Microsoft!
       */
      const xunichar2_t *filename_end = NULL;
      xssize_t filename_len = 0;
      xssize_t function_len = 0;
      const xunichar2_t *dllpart;

      quoted = FALSE;

      if ((wchar_t) first_argument[0] == L'"')
        quoted = TRUE;

      _g_win32_parse_filename (first_argument, TRUE, &dllpart, &filename_len, NULL, &filename_end);

      if (filename_end[0] != 0 && filename_len > 0)
        {
          const xunichar2_t *function_begin = filename_end;

          while ((wchar_t) function_begin[0] == L',' || (wchar_t) function_begin[0] == L' ')
            function_begin += 1;

          if (function_begin[0] != 0)
            {
              xchar_t *dllpart_utf8;
              xchar_t *dllpart_utf8_folded;
              xchar_t *function_utf8;
              xboolean_t folded;
              const xunichar2_t *space = xutf16_wchr (function_begin, L' ');

              if (space)
                function_len = space - function_begin;
              else
                function_len = xutf16_len (function_begin);

              if (quoted)
                first_argument += 1;

              folded = xutf16_to_utf8_and_fold (first_argument, filename_len, &dllpart_utf8, &dllpart_utf8_folded);
              g_assert (folded);

              function_utf8 = xutf16_to_utf8 (function_begin, function_len, NULL, NULL, NULL);

              /* We only take this branch when dll_function_out is not NULL */
              *dll_function_out = g_steal_pointer (&function_utf8);

              g_free (function_utf8);

              /*
               * Free our previous output candidate (rundll32) and replace it with the DLL path,
               * then proceed forward as if nothing has changed.
               */
              g_free (ex);
              g_free (ex_folded);

              ex = dllpart_utf8;
              ex_folded = dllpart_utf8_folded;
            }
        }
    }

  if (ex_out)
    {
      if (ex_basename_out)
        *ex_basename_out = (xchar_t *) xutf8_find_basename (ex, -1);

      *ex_out = g_steal_pointer (&ex);
    }

  g_free (ex);

  if (ex_folded_out)
    {
      if (ex_folded_basename_out)
        *ex_folded_basename_out = (xchar_t *) xutf8_find_basename (ex_folded, -1);

      *ex_folded_out = g_steal_pointer (&ex_folded);
    }

  g_free (ex_folded);
}

/**
 * rundll32 accepts many different commandlines. Among them is this:
 * > rundll32.exe "c:/program files/foo/bar.dll",,, , ,,,, , function_name %1
 * rundll32 just reads the first argument as a potentially quoted
 * filename until the quotation ends (if quoted) or until a comma,
 * or until a space. Then ignores all subsequent spaces (if any) and commas (if any;
 * at least one comma is mandatory only if the filename is not quoted),
 * and then interprets the rest of the commandline (until a space or a NUL-byte)
 * as a name of a function.
 * When GLib tries to run a program, it attempts to correctly re-quote the arguments,
 * turning the first argument into "c:/program files/foo/bar.dll,,,".
 * This breaks rundll32 parsing logic.
 * Try to work around this by ensuring that the syntax is like this:
 * > rundll32.exe "c:/program files/foo/bar.dll" function_name
 * This syntax is valid for rundll32 *and* GLib spawn routines won't break it.
 *
 * @commandline must have at least 2 arguments, and the second argument
 * must contain a (possibly quoted) filename, followed by a space or
 * a comma. This can be checked for with an extract_executable() call -
 * it should return a non-null dll_function.
 */
static void
_g_win32_fixup_broken_microsoft_rundll_commandline (xunichar2_t *commandline)
{
  const xunichar2_t *first_argument;
  xunichar2_t *after_first_argument;

  _g_win32_parse_filename (commandline, FALSE, NULL, NULL, NULL, &first_argument);

  while ((wchar_t) first_argument[0] == L' ')
    first_argument++;

  _g_win32_parse_filename (first_argument, TRUE, NULL, NULL, NULL, (const xunichar2_t **) &after_first_argument);

  if ((wchar_t) after_first_argument[0] == L',')
    after_first_argument[0] = 0x0020;
  /* Else everything is ok (first char after filename is ' ' or the first char
   * of the function name - either way this will work).
   */
}
