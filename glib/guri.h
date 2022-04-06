/* XPL - Library of useful routines for C programming
 * Copyright © 2020 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _xuri xuri_t;

XPL_AVAILABLE_IN_2_66
xuri_t *       xuri_ref              (xuri_t *uri);
XPL_AVAILABLE_IN_2_66
void         xuri_unref            (xuri_t *uri);

/**
 * xuri_flags_t:
 * @XURI_FLAGS_NONE: No flags set.
 * @XURI_FLAGS_PARSE_RELAXED: Parse the URI more relaxedly than the
 *     [RFC 3986](https://tools.ietf.org/html/rfc3986) grammar specifies,
 *     fixing up or ignoring common mistakes in URIs coming from external
 *     sources. This is also needed for some obscure URI schemes where `;`
 *     separates the host from the path. Don’t use this flag unless you need to.
 * @XURI_FLAGS_HAS_PASSWORD: The userinfo field may contain a password,
 *     which will be separated from the username by `:`.
 * @XURI_FLAGS_HAS_AUTH_PARAMS: The userinfo may contain additional
 *     authentication-related parameters, which will be separated from
 *     the username and/or password by `;`.
 * @XURI_FLAGS_NON_DNS: The host component should not be assumed to be a
 *     DNS hostname or IP address (for example, for `smb` URIs with NetBIOS
 *     hostnames).
 * @XURI_FLAGS_ENCODED: When parsing a URI, this indicates that `%`-encoded
 *     characters in the userinfo, path, query, and fragment fields
 *     should not be decoded. (And likewise the host field if
 *     %XURI_FLAGS_NON_DNS is also set.) When building a URI, it indicates
 *     that you have already `%`-encoded the components, and so #xuri_t
 *     should not do any encoding itself.
 * @XURI_FLAGS_ENCODED_QUERY: Same as %XURI_FLAGS_ENCODED, for the query
 *     field only.
 * @XURI_FLAGS_ENCODED_PATH: Same as %XURI_FLAGS_ENCODED, for the path only.
 * @XURI_FLAGS_ENCODED_FRAGMENT: Same as %XURI_FLAGS_ENCODED, for the
 *     fragment only.
 * @XURI_FLAGS_SCHEME_NORMALIZE: A scheme-based normalization will be applied.
 *     For example, when parsing an HTTP URI changing omitted path to `/` and
 *     omitted port to `80`; and when building a URI, changing empty path to `/`
 *     and default port `80`). This only supports a subset of known schemes. (Since: 2.68)
 *
 * Flags that describe a URI.
 *
 * When parsing a URI, if you need to choose different flags based on
 * the type of URI, you can use xuri_peek_scheme() on the URI string
 * to check the scheme first, and use that to decide what flags to
 * parse it with.
 *
 * Since: 2.66
 */
XPL_AVAILABLE_TYPE_IN_2_66
typedef enum {
  XURI_FLAGS_NONE            = 0,
  XURI_FLAGS_PARSE_RELAXED   = 1 << 0,
  XURI_FLAGS_HAS_PASSWORD    = 1 << 1,
  XURI_FLAGS_HAS_AUTH_PARAMS = 1 << 2,
  XURI_FLAGS_ENCODED         = 1 << 3,
  XURI_FLAGS_NON_DNS         = 1 << 4,
  XURI_FLAGS_ENCODED_QUERY   = 1 << 5,
  XURI_FLAGS_ENCODED_PATH    = 1 << 6,
  XURI_FLAGS_ENCODED_FRAGMENT = 1 << 7,
  XURI_FLAGS_SCHEME_NORMALIZE XPL_AVAILABLE_ENUMERATOR_IN_2_68 = 1 << 8,
} xuri_flags_t;

XPL_AVAILABLE_IN_2_66
xboolean_t     xuri_split            (const xchar_t  *uri_ref,
                                     xuri_flags_t     flags,
                                     xchar_t       **scheme,
                                     xchar_t       **userinfo,
                                     xchar_t       **host,
                                     xint_t         *port,
                                     xchar_t       **path,
                                     xchar_t       **query,
                                     xchar_t       **fragment,
                                     xerror_t      **error);
XPL_AVAILABLE_IN_2_66
xboolean_t     xuri_split_with_user  (const xchar_t  *uri_ref,
                                     xuri_flags_t     flags,
                                     xchar_t       **scheme,
                                     xchar_t       **user,
                                     xchar_t       **password,
                                     xchar_t       **auth_params,
                                     xchar_t       **host,
                                     xint_t         *port,
                                     xchar_t       **path,
                                     xchar_t       **query,
                                     xchar_t       **fragment,
                                     xerror_t      **error);
XPL_AVAILABLE_IN_2_66
xboolean_t     xuri_split_network    (const xchar_t  *uri_string,
                                     xuri_flags_t     flags,
                                     xchar_t       **scheme,
                                     xchar_t       **host,
                                     xint_t         *port,
                                     xerror_t      **error);

XPL_AVAILABLE_IN_2_66
xboolean_t     xuri_is_valid         (const xchar_t  *uri_string,
                                     xuri_flags_t     flags,
                                     xerror_t      **error);

XPL_AVAILABLE_IN_2_66
xchar_t *      xuri_join             (xuri_flags_t     flags,
                                     const xchar_t  *scheme,
                                     const xchar_t  *userinfo,
                                     const xchar_t  *host,
                                     xint_t          port,
                                     const xchar_t  *path,
                                     const xchar_t  *query,
                                     const xchar_t  *fragment);
XPL_AVAILABLE_IN_2_66
xchar_t *      xuri_join_with_user   (xuri_flags_t     flags,
                                     const xchar_t  *scheme,
                                     const xchar_t  *user,
                                     const xchar_t  *password,
                                     const xchar_t  *auth_params,
                                     const xchar_t  *host,
                                     xint_t          port,
                                     const xchar_t  *path,
                                     const xchar_t  *query,
                                     const xchar_t  *fragment);

XPL_AVAILABLE_IN_2_66
xuri_t *       xuri_parse            (const xchar_t  *uri_string,
                                     xuri_flags_t     flags,
                                     xerror_t      **error);
XPL_AVAILABLE_IN_2_66
xuri_t *       xuri_parse_relative   (xuri_t         *base_uri,
                                     const xchar_t  *uri_ref,
                                     xuri_flags_t     flags,
                                     xerror_t      **error);

XPL_AVAILABLE_IN_2_66
xchar_t *      xuri_resolve_relative (const xchar_t  *base_uri_string,
                                     const xchar_t  *uri_ref,
                                     xuri_flags_t     flags,
                                     xerror_t      **error);

XPL_AVAILABLE_IN_2_66
xuri_t *       xuri_build            (xuri_flags_t     flags,
                                     const xchar_t  *scheme,
                                     const xchar_t  *userinfo,
                                     const xchar_t  *host,
                                     xint_t          port,
                                     const xchar_t  *path,
                                     const xchar_t  *query,
                                     const xchar_t  *fragment);
XPL_AVAILABLE_IN_2_66
xuri_t *       xuri_build_with_user  (xuri_flags_t     flags,
                                     const xchar_t  *scheme,
                                     const xchar_t  *user,
                                     const xchar_t  *password,
                                     const xchar_t  *auth_params,
                                     const xchar_t  *host,
                                     xint_t          port,
                                     const xchar_t  *path,
                                     const xchar_t  *query,
                                     const xchar_t  *fragment);

/**
 * xuri_hide_flags_t:
 * @XURI_HIDE_NONE: No flags set.
 * @XURI_HIDE_USERINFO: Hide the userinfo.
 * @XURI_HIDE_PASSWORD: Hide the password.
 * @XURI_HIDE_AUTH_PARAMS: Hide the auth_params.
 * @XURI_HIDE_QUERY: Hide the query.
 * @XURI_HIDE_FRAGMENT: Hide the fragment.
 *
 * Flags describing what parts of the URI to hide in
 * xuri_to_string_partial(). Note that %XURI_HIDE_PASSWORD and
 * %XURI_HIDE_AUTH_PARAMS will only work if the #xuri_t was parsed with
 * the corresponding flags.
 *
 * Since: 2.66
 */
XPL_AVAILABLE_TYPE_IN_2_66
typedef enum {
  XURI_HIDE_NONE        = 0,
  XURI_HIDE_USERINFO    = 1 << 0,
  XURI_HIDE_PASSWORD    = 1 << 1,
  XURI_HIDE_AUTH_PARAMS = 1 << 2,
  XURI_HIDE_QUERY       = 1 << 3,
  XURI_HIDE_FRAGMENT    = 1 << 4,
} xuri_hide_flags_t;

XPL_AVAILABLE_IN_2_66
char *       xuri_to_string         (xuri_t          *uri);
XPL_AVAILABLE_IN_2_66
char *       xuri_to_string_partial (xuri_t          *uri,
                                      xuri_hide_flags_t  flags);

XPL_AVAILABLE_IN_2_66
const xchar_t *xuri_get_scheme        (xuri_t          *uri);
XPL_AVAILABLE_IN_2_66
const xchar_t *xuri_get_userinfo      (xuri_t          *uri);
XPL_AVAILABLE_IN_2_66
const xchar_t *xuri_get_user          (xuri_t          *uri);
XPL_AVAILABLE_IN_2_66
const xchar_t *xuri_get_password      (xuri_t          *uri);
XPL_AVAILABLE_IN_2_66
const xchar_t *xuri_get_auth_params   (xuri_t          *uri);
XPL_AVAILABLE_IN_2_66
const xchar_t *xuri_get_host          (xuri_t          *uri);
XPL_AVAILABLE_IN_2_66
xint_t         xuri_get_port          (xuri_t          *uri);
XPL_AVAILABLE_IN_2_66
const xchar_t *xuri_get_path          (xuri_t          *uri);
XPL_AVAILABLE_IN_2_66
const xchar_t *xuri_get_query         (xuri_t          *uri);
XPL_AVAILABLE_IN_2_66
const xchar_t *xuri_get_fragment      (xuri_t          *uri);
XPL_AVAILABLE_IN_2_66
xuri_flags_t    xuri_get_flags         (xuri_t          *uri);

/**
 * xuri_params_flags_t:
 * @XURI_PARAMS_NONE: No flags set.
 * @XURI_PARAMS_CASE_INSENSITIVE: Parameter names are case insensitive.
 * @XURI_PARAMS_WWW_FORM: Replace `+` with space character. Only useful for
 *     URLs on the web, using the `https` or `http` schemas.
 * @XURI_PARAMS_PARSE_RELAXED: See %XURI_FLAGS_PARSE_RELAXED.
 *
 * Flags modifying the way parameters are handled by xuri_parse_params() and
 * #xuri_params_iter_t.
 *
 * Since: 2.66
 */
XPL_AVAILABLE_TYPE_IN_2_66
typedef enum {
  XURI_PARAMS_NONE             = 0,
  XURI_PARAMS_CASE_INSENSITIVE = 1 << 0,
  XURI_PARAMS_WWW_FORM         = 1 << 1,
  XURI_PARAMS_PARSE_RELAXED    = 1 << 2,
} xuri_params_flags_t;

XPL_AVAILABLE_IN_2_66
xhashtable_t *xuri_parse_params       (const xchar_t    *params,
                                      xssize_t          length,
                                      const xchar_t    *separators,
                                      xuri_params_flags_t flags,
                                      xerror_t        **error);

typedef struct _xuri_params_iter xuri_params_iter_t;

struct _xuri_params_iter
{
  /*< private >*/
  xint_t     dummy0;
  xpointer_t dummy1;
  xpointer_t dummy2;
  xuint8_t   dummy3[256];
};

XPL_AVAILABLE_IN_2_66
void        xuri_params_iter_init   (xuri_params_iter_t *iter,
                                      const xchar_t    *params,
                                      xssize_t          length,
                                      const xchar_t    *separators,
                                      xuri_params_flags_t flags);

XPL_AVAILABLE_IN_2_66
xboolean_t    xuri_params_iter_next   (xuri_params_iter_t *iter,
                                      xchar_t         **attribute,
                                      xchar_t         **value,
                                      xerror_t        **error);

/**
 * XURI_ERROR:
 *
 * Error domain for URI methods. Errors in this domain will be from
 * the #xuri_error_t enumeration. See #xerror_t for information on error
 * domains.
 *
 * Since: 2.66
 */
#define XURI_ERROR (xuri_error_quark ()) XPL_AVAILABLE_MACRO_IN_2_66
XPL_AVAILABLE_IN_2_66
xquark xuri_error_quark (void);

/**
 * xuri_error_t:
 * @XURI_ERROR_FAILED: Generic error if no more specific error is available.
 *     See the error message for details.
 * @XURI_ERROR_BAD_SCHEME: The scheme of a URI could not be parsed.
 * @XURI_ERROR_BAD_USER: The user/userinfo of a URI could not be parsed.
 * @XURI_ERROR_BAD_PASSWORD: The password of a URI could not be parsed.
 * @XURI_ERROR_BAD_AUTH_PARAMS: The authentication parameters of a URI could not be parsed.
 * @XURI_ERROR_BAD_HOST: The host of a URI could not be parsed.
 * @XURI_ERROR_BAD_PORT: The port of a URI could not be parsed.
 * @XURI_ERROR_BAD_PATH: The path of a URI could not be parsed.
 * @XURI_ERROR_BAD_QUERY: The query of a URI could not be parsed.
 * @XURI_ERROR_BAD_FRAGMENT: The fragment of a URI could not be parsed.
 *
 * Error codes returned by #xuri_t methods.
 *
 * Since: 2.66
 */
typedef enum {
  XURI_ERROR_FAILED,
  XURI_ERROR_BAD_SCHEME,
  XURI_ERROR_BAD_USER,
  XURI_ERROR_BAD_PASSWORD,
  XURI_ERROR_BAD_AUTH_PARAMS,
  XURI_ERROR_BAD_HOST,
  XURI_ERROR_BAD_PORT,
  XURI_ERROR_BAD_PATH,
  XURI_ERROR_BAD_QUERY,
  XURI_ERROR_BAD_FRAGMENT,
} xuri_error_t;

/**
 * XURI_RESERVED_CHARS_GENERIC_DELIMITERS:
 *
 * Generic delimiters characters as defined in
 * [RFC 3986](https://tools.ietf.org/html/rfc3986). Includes `:/?#[]@`.
 *
 * Since: 2.16
 **/
#define XURI_RESERVED_CHARS_GENERIC_DELIMITERS ":/?#[]@"

/**
 * XURI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS:
 *
 * Subcomponent delimiter characters as defined in
 * [RFC 3986](https://tools.ietf.org/html/rfc3986). Includes `!$&'()*+,;=`.
 *
 * Since: 2.16
 **/
#define XURI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS "!$&'()*+,;="

/**
 * XURI_RESERVED_CHARS_ALLOWED_IN_PATH_ELEMENT:
 *
 * Allowed characters in path elements. Includes `!$&'()*+,;=:@`.
 *
 * Since: 2.16
 **/
#define XURI_RESERVED_CHARS_ALLOWED_IN_PATH_ELEMENT XURI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS ":@"

/**
 * XURI_RESERVED_CHARS_ALLOWED_IN_PATH:
 *
 * Allowed characters in a path. Includes `!$&'()*+,;=:@/`.
 *
 * Since: 2.16
 **/
#define XURI_RESERVED_CHARS_ALLOWED_IN_PATH XURI_RESERVED_CHARS_ALLOWED_IN_PATH_ELEMENT "/"

/**
 * XURI_RESERVED_CHARS_ALLOWED_IN_USERINFO:
 *
 * Allowed characters in userinfo as defined in
 * [RFC 3986](https://tools.ietf.org/html/rfc3986). Includes `!$&'()*+,;=:`.
 *
 * Since: 2.16
 **/
#define XURI_RESERVED_CHARS_ALLOWED_IN_USERINFO XURI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS ":"

XPL_AVAILABLE_IN_ALL
char *      xuri_unescape_string  (const char *escaped_string,
                                    const char *illegal_characters);
XPL_AVAILABLE_IN_ALL
char *      xuri_unescape_segment (const char *escaped_string,
                                    const char *escaped_string_end,
                                    const char *illegal_characters);

XPL_AVAILABLE_IN_ALL
char *      xuri_parse_scheme     (const char *uri);
XPL_AVAILABLE_IN_2_66
const char *xuri_peek_scheme      (const char *uri);

XPL_AVAILABLE_IN_ALL
char *      xuri_escape_string    (const char *unescaped,
                                    const char *reserved_chars_allowed,
                                    xboolean_t    allow_utf8);

XPL_AVAILABLE_IN_2_66
xbytes_t *    xuri_unescape_bytes   (const char *escaped_string,
                                    xssize_t      length,
                                    const char *illegal_characters,
                                    xerror_t    **error);

XPL_AVAILABLE_IN_2_66
char *      xuri_escape_bytes     (const xuint8_t *unescaped,
                                    xsize_t         length,
                                    const char   *reserved_chars_allowed);

G_GNUC_END_IGNORE_DEPRECATIONS

G_END_DECLS
