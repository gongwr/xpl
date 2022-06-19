/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include "gdummyfile.h"
#include "gfile.h"


static void xdummy_file_file_iface_init (xfile_iface_t *iface);

typedef struct {
  char *scheme;
  char *userinfo;
  char *host;
  int port; /* -1 => not in uri */
  char *path;
  char *query;
  char *fragment;
} xdecoded_uri_t;

struct _GDummyFile
{
  xobject_t parent_instance;

  xdecoded_uri_t *decoded_uri;
  char *text_uri;
};

#define xdummy_file_get_type _xdummy_file_get_type
G_DEFINE_TYPE_WITH_CODE (xdummy_file_t, xdummy_file, XTYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (XTYPE_FILE,
						xdummy_file_file_iface_init))

#define SUB_DELIM_CHARS  "!$&'()*+,;="

static char *       _xencode_uri       (xdecoded_uri_t *decoded);
static void         _xdecoded_uri_free (xdecoded_uri_t *decoded);
static xdecoded_uri_t *_xdecoded_uri       (const char  *uri);
static xdecoded_uri_t *_xdecoded_uri_new  (void);

static char * unescape_string (const xchar_t *escaped_string,
			       const xchar_t *escaped_string_end,
			       const xchar_t *illegal_characters);

static void xstring_append_encoded (xstring_t    *string,
                                     const char *encoded,
				     const char *reserved_chars_allowed);

static void
xdummy_file_finalize (xobject_t *object)
{
  xdummy_file_t *dummy;

  dummy = G_DUMMY_FILE (object);

  if (dummy->decoded_uri)
    _xdecoded_uri_free (dummy->decoded_uri);

  g_free (dummy->text_uri);

  XOBJECT_CLASS (xdummy_file_parent_class)->finalize (object);
}

static void
xdummy_file_class_init (xdummy_file_class_t *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->finalize = xdummy_file_finalize;
}

static void
xdummy_file_init (xdummy_file_t *dummy)
{
}

xfile_t *
_xdummy_file_new (const char *uri)
{
  xdummy_file_t *dummy;

  xreturn_val_if_fail (uri != NULL, NULL);

  dummy = xobject_new (XTYPE_DUMMY_FILE, NULL);
  dummy->text_uri = xstrdup (uri);
  dummy->decoded_uri = _xdecoded_uri (uri);

  return XFILE (dummy);
}

static xboolean_t
xdummy_file_is_native (xfile_t *file)
{
  return FALSE;
}

static char *
xdummy_file_get_basename (xfile_t *file)
{
  xdummy_file_t *dummy = G_DUMMY_FILE (file);

  if (dummy->decoded_uri)
    return g_path_get_basename (dummy->decoded_uri->path);
  return NULL;
}

static char *
xdummy_file_get_path (xfile_t *file)
{
  return NULL;
}

static char *
xdummy_file_get_uri (xfile_t *file)
{
  return xstrdup (G_DUMMY_FILE (file)->text_uri);
}

static char *
xdummy_file_get_parse_name (xfile_t *file)
{
  return xstrdup (G_DUMMY_FILE (file)->text_uri);
}

static xfile_t *
xdummy_file_get_parent (xfile_t *file)
{
  xdummy_file_t *dummy = G_DUMMY_FILE (file);
  xfile_t *parent;
  char *dirname;
  char *uri;
  xdecoded_uri_t new_decoded_uri;

  if (dummy->decoded_uri == NULL ||
      xstrcmp0 (dummy->decoded_uri->path, "/") == 0)
    return NULL;

  dirname = g_path_get_dirname (dummy->decoded_uri->path);

  if (strcmp (dirname, ".") == 0)
    {
      g_free (dirname);
      return NULL;
    }

  new_decoded_uri = *dummy->decoded_uri;
  new_decoded_uri.path = dirname;
  uri = _xencode_uri (&new_decoded_uri);
  g_free (dirname);

  parent = _xdummy_file_new (uri);
  g_free (uri);

  return parent;
}

static xfile_t *
xdummy_file_dup (xfile_t *file)
{
  xdummy_file_t *dummy = G_DUMMY_FILE (file);

  return _xdummy_file_new (dummy->text_uri);
}

static xuint_t
xdummy_file_hash (xfile_t *file)
{
  xdummy_file_t *dummy = G_DUMMY_FILE (file);

  return xstr_hash (dummy->text_uri);
}

static xboolean_t
xdummy_file_equal (xfile_t *file1,
		    xfile_t *file2)
{
  xdummy_file_t *dummy1 = G_DUMMY_FILE (file1);
  xdummy_file_t *dummy2 = G_DUMMY_FILE (file2);

  return xstr_equal (dummy1->text_uri, dummy2->text_uri);
}

static int
safe_strcmp (const char *a,
             const char *b)
{
  if (a == NULL)
    a = "";
  if (b == NULL)
    b = "";

  return strcmp (a, b);
}

static xboolean_t
uri_same_except_path (xdecoded_uri_t *a,
		      xdecoded_uri_t *b)
{
  if (safe_strcmp (a->scheme, b->scheme) != 0)
    return FALSE;
  if (safe_strcmp (a->userinfo, b->userinfo) != 0)
    return FALSE;
  if (safe_strcmp (a->host, b->host) != 0)
    return FALSE;
  if (a->port != b->port)
    return FALSE;

  return TRUE;
}

static const char *
match_prefix (const char *path,
              const char *prefix)
{
  int prefix_len;

  prefix_len = strlen (prefix);
  if (strncmp (path, prefix, prefix_len) != 0)
    return NULL;
  return path + prefix_len;
}

static xboolean_t
xdummy_file_prefix_matches (xfile_t *parent, xfile_t *descendant)
{
  xdummy_file_t *parent_dummy = G_DUMMY_FILE (parent);
  xdummy_file_t *descendant_dummy = G_DUMMY_FILE (descendant);
  const char *remainder;

  if (parent_dummy->decoded_uri != NULL &&
      descendant_dummy->decoded_uri != NULL)
    {
      if (uri_same_except_path (parent_dummy->decoded_uri,
				descendant_dummy->decoded_uri))
        {
	  remainder = match_prefix (descendant_dummy->decoded_uri->path,
                                    parent_dummy->decoded_uri->path);
          if (remainder != NULL && *remainder == '/')
	    {
	      while (*remainder == '/')
	        remainder++;
	      if (*remainder != 0)
	        return TRUE;
	    }
        }
    }
  else
    {
      remainder = match_prefix (descendant_dummy->text_uri,
				parent_dummy->text_uri);
      if (remainder != NULL && *remainder == '/')
	  {
	    while (*remainder == '/')
	      remainder++;
	    if (*remainder != 0)
	      return TRUE;
	  }
    }

  return FALSE;
}

static char *
xdummy_file_get_relative_path (xfile_t *parent,
				xfile_t *descendant)
{
  xdummy_file_t *parent_dummy = G_DUMMY_FILE (parent);
  xdummy_file_t *descendant_dummy = G_DUMMY_FILE (descendant);
  const char *remainder;

  if (parent_dummy->decoded_uri != NULL &&
      descendant_dummy->decoded_uri != NULL)
    {
      if (uri_same_except_path (parent_dummy->decoded_uri,
				descendant_dummy->decoded_uri))
        {
          remainder = match_prefix (descendant_dummy->decoded_uri->path,
                                    parent_dummy->decoded_uri->path);
          if (remainder != NULL && *remainder == '/')
	    {
	      while (*remainder == '/')
	        remainder++;
	      if (*remainder != 0)
	        return xstrdup (remainder);
	    }
        }
    }
  else
    {
      remainder = match_prefix (descendant_dummy->text_uri,
				parent_dummy->text_uri);
      if (remainder != NULL && *remainder == '/')
	  {
	    while (*remainder == '/')
	      remainder++;
	    if (*remainder != 0)
	      return unescape_string (remainder, NULL, "/");
	  }
    }

  return NULL;
}


static xfile_t *
xdummy_file_resolve_relative_path (xfile_t      *file,
				    const char *relative_path)
{
  xdummy_file_t *dummy = G_DUMMY_FILE (file);
  xfile_t *child;
  char *uri;
  xdecoded_uri_t new_decoded_uri;
  xstring_t *str;

  if (dummy->decoded_uri == NULL)
    {
      str = xstring_new (dummy->text_uri);
      xstring_append (str, "/");
      xstring_append_encoded (str, relative_path, SUB_DELIM_CHARS ":@/");
      child = _xdummy_file_new (str->str);
      xstring_free (str, TRUE);
    }
  else
    {
      new_decoded_uri = *dummy->decoded_uri;

      if (g_path_is_absolute (relative_path))
	new_decoded_uri.path = xstrdup (relative_path);
      else
	new_decoded_uri.path = g_build_filename (new_decoded_uri.path, relative_path, NULL);

      uri = _xencode_uri (&new_decoded_uri);
      g_free (new_decoded_uri.path);

      child = _xdummy_file_new (uri);
      g_free (uri);
    }

  return child;
}

static xfile_t *
xdummy_file_get_child_for_display_name (xfile_t        *file,
					 const char   *display_name,
					 xerror_t      **error)
{
  return xfile_get_child (file, display_name);
}

static xboolean_t
xdummy_file_has_uri_scheme (xfile_t *file,
			     const char *uri_scheme)
{
  xdummy_file_t *dummy = G_DUMMY_FILE (file);

  if (dummy->decoded_uri)
    return g_ascii_strcasecmp (uri_scheme, dummy->decoded_uri->scheme) == 0;
  return FALSE;
}

static char *
xdummy_file_get_uri_scheme (xfile_t *file)
{
  xdummy_file_t *dummy = G_DUMMY_FILE (file);

  if (dummy->decoded_uri)
    return xstrdup (dummy->decoded_uri->scheme);

  return NULL;
}


static void
xdummy_file_file_iface_init (xfile_iface_t *iface)
{
  iface->dup = xdummy_file_dup;
  iface->hash = xdummy_file_hash;
  iface->equal = xdummy_file_equal;
  iface->is_native = xdummy_file_is_native;
  iface->has_uri_scheme = xdummy_file_has_uri_scheme;
  iface->get_uri_scheme = xdummy_file_get_uri_scheme;
  iface->get_basename = xdummy_file_get_basename;
  iface->get_path = xdummy_file_get_path;
  iface->get_uri = xdummy_file_get_uri;
  iface->get_parse_name = xdummy_file_get_parse_name;
  iface->get_parent = xdummy_file_get_parent;
  iface->prefix_matches = xdummy_file_prefix_matches;
  iface->get_relative_path = xdummy_file_get_relative_path;
  iface->resolve_relative_path = xdummy_file_resolve_relative_path;
  iface->get_child_for_display_name = xdummy_file_get_child_for_display_name;

  iface->supports_thread_contexts = TRUE;
}

/* Uri handling helper functions: */

static int
unescape_character (const char *scanner)
{
  int first_digit;
  int second_digit;

  first_digit = g_ascii_xdigit_value (*scanner++);
  if (first_digit < 0)
    return -1;

  second_digit = g_ascii_xdigit_value (*scanner++);
  if (second_digit < 0)
    return -1;

  return (first_digit << 4) | second_digit;
}

static char *
unescape_string (const xchar_t *escaped_string,
		 const xchar_t *escaped_string_end,
		 const xchar_t *illegal_characters)
{
  const xchar_t *in;
  xchar_t *out, *result;
  xint_t character;

  if (escaped_string == NULL)
    return NULL;

  if (escaped_string_end == NULL)
    escaped_string_end = escaped_string + strlen (escaped_string);

  result = g_malloc (escaped_string_end - escaped_string + 1);

  out = result;
  for (in = escaped_string; in < escaped_string_end; in++)
    {
      character = *in;
      if (*in == '%')
        {
          in++;
          if (escaped_string_end - in < 2)
	    {
	      g_free (result);
	      return NULL;
	    }

          character = unescape_character (in);

          /* Check for an illegal character. We consider '\0' illegal here. */
          if (character <= 0 ||
	      (illegal_characters != NULL &&
	       strchr (illegal_characters, (char)character) != NULL))
	    {
	      g_free (result);
	      return NULL;
	    }
          in++; /* The other char will be eaten in the loop header */
        }
      *out++ = (char)character;
    }

  *out = '\0';
  g_warn_if_fail ((xsize_t) (out - result) <= strlen (escaped_string));
  return result;
}

void
_xdecoded_uri_free (xdecoded_uri_t *decoded)
{
  if (decoded == NULL)
    return;

  g_free (decoded->scheme);
  g_free (decoded->query);
  g_free (decoded->fragment);
  g_free (decoded->userinfo);
  g_free (decoded->host);
  g_free (decoded->path);
  g_free (decoded);
}

xdecoded_uri_t *
_xdecoded_uri_new (void)
{
  xdecoded_uri_t *uri;

  uri = g_new0 (xdecoded_uri_t, 1);
  uri->port = -1;

  return uri;
}

xdecoded_uri_t *
_xdecoded_uri (const char *uri)
{
  xdecoded_uri_t *decoded;
  const char *p, *in, *hier_part_start, *hier_part_end, *query_start, *fragment_start;
  char *out;
  char c;

  /* From RFC 3986 Decodes:
   * URI         = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
   */

  p = uri;

  /* Decode scheme:
     scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
  */

  if (!g_ascii_isalpha (*p))
    return NULL;

  while (1)
    {
      c = *p++;

      if (c == ':')
	break;

      if (!(g_ascii_isalnum(c) ||
	    c == '+' ||
	    c == '-' ||
	    c == '.'))
	return NULL;
    }

  decoded = _xdecoded_uri_new ();

  decoded->scheme = g_malloc (p - uri);
  out = decoded->scheme;
  for (in = uri; in < p - 1; in++)
    *out++ = g_ascii_tolower (*in);
  *out = 0;

  hier_part_start = p;

  query_start = strchr (p, '?');
  if (query_start)
    {
      hier_part_end = query_start++;
      fragment_start = strchr (query_start, '#');
      if (fragment_start)
	{
	  decoded->query = xstrndup (query_start, fragment_start - query_start);
	  decoded->fragment = xstrdup (fragment_start+1);
	}
      else
	{
	  decoded->query = xstrdup (query_start);
	  decoded->fragment = NULL;
	}
    }
  else
    {
      /* No query */
      decoded->query = NULL;
      fragment_start = strchr (p, '#');
      if (fragment_start)
	{
	  hier_part_end = fragment_start++;
	  decoded->fragment = xstrdup (fragment_start);
	}
      else
	{
	  hier_part_end = p + strlen (p);
	  decoded->fragment = NULL;
	}
    }

  /*  3:
      hier-part   = "//" authority path-abempty
                  / path-absolute
                  / path-rootless
                  / path-empty

  */

  if (hier_part_start[0] == '/' &&
      hier_part_start[1] == '/')
    {
      const char *authority_start, *authority_end;
      const char *userinfo_start, *userinfo_end;
      const char *host_start, *host_end;
      const char *port_start;

      authority_start = hier_part_start + 2;
      /* authority is always followed by / or nothing */
      authority_end = memchr (authority_start, '/', hier_part_end - authority_start);
      if (authority_end == NULL)
	authority_end = hier_part_end;

      /* 3.2:
	      authority   = [ userinfo "@" ] host [ ":" port ]
      */

      userinfo_end = memchr (authority_start, '@', authority_end - authority_start);
      if (userinfo_end)
	{
	  userinfo_start = authority_start;
	  decoded->userinfo = unescape_string (userinfo_start, userinfo_end, NULL);
	  if (decoded->userinfo == NULL)
	    {
	      _xdecoded_uri_free (decoded);
	      return NULL;
	    }
	  host_start = userinfo_end + 1;
	}
      else
	host_start = authority_start;

      port_start = memchr (host_start, ':', authority_end - host_start);
      if (port_start)
	{
	  host_end = port_start++;

	  decoded->port = atoi(port_start);
	}
      else
	{
	  host_end = authority_end;
	  decoded->port = -1;
	}

      decoded->host = xstrndup (host_start, host_end - host_start);

      hier_part_start = authority_end;
    }

  decoded->path = unescape_string (hier_part_start, hier_part_end, "/");

  if (decoded->path == NULL)
    {
      _xdecoded_uri_free (decoded);
      return NULL;
    }

  return decoded;
}

static xboolean_t
is_valid (char c, const char *reserved_chars_allowed)
{
  if (g_ascii_isalnum (c) ||
      c == '-' ||
      c == '.' ||
      c == '_' ||
      c == '~')
    return TRUE;

  if (reserved_chars_allowed &&
      strchr (reserved_chars_allowed, c) != NULL)
    return TRUE;

  return FALSE;
}

static void
xstring_append_encoded (xstring_t    *string,
                         const char *encoded,
			 const char *reserved_chars_allowed)
{
  unsigned char c;
  static const xchar_t hex[16] = "0123456789ABCDEF";

  while ((c = *encoded) != 0)
    {
      if (is_valid (c, reserved_chars_allowed))
	{
	  xstring_append_c (string, c);
	  encoded++;
	}
      else
	{
	  xstring_append_c (string, '%');
	  xstring_append_c (string, hex[((xuchar_t)c) >> 4]);
	  xstring_append_c (string, hex[((xuchar_t)c) & 0xf]);
	  encoded++;
	}
    }
}

static char *
_xencode_uri (xdecoded_uri_t *decoded)
{
  xstring_t *uri;

  uri = xstring_new (NULL);

  xstring_append (uri, decoded->scheme);
  xstring_append (uri, "://");

  if (decoded->host != NULL)
    {
      if (decoded->userinfo)
	{
	  /* userinfo    = *( unreserved / pct-encoded / sub-delims / ":" ) */
	  xstring_append_encoded (uri, decoded->userinfo, SUB_DELIM_CHARS ":");
	  xstring_append_c (uri, '@');
	}

      xstring_append (uri, decoded->host);

      if (decoded->port != -1)
	{
	  xstring_append_c (uri, ':');
	  xstring_append_printf (uri, "%d", decoded->port);
	}
    }

  xstring_append_encoded (uri, decoded->path, SUB_DELIM_CHARS ":@/");

  if (decoded->query)
    {
      xstring_append_c (uri, '?');
      xstring_append (uri, decoded->query);
    }

  if (decoded->fragment)
    {
      xstring_append_c (uri, '#');
      xstring_append (uri, decoded->fragment);
    }

  return xstring_free (uri, FALSE);
}
