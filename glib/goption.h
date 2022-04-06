/* goption.h - Option parser
 *
 *  Copyright (C) 2004  Anders Carlsson <andersca@gnome.org>
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

#ifndef __G_OPTION_H__
#define __G_OPTION_H__

#if !defined (__XPL_H_INSIDE__) && !defined (XPL_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gerror.h>
#include <glib/gquark.h>

G_BEGIN_DECLS

/**
 * xoption_context_t:
 *
 * A `xoption_context_t` struct defines which options
 * are accepted by the commandline option parser. The struct has only private
 * fields and should not be directly accessed.
 */
typedef struct _GOptionContext xoption_context_t;

/**
 * xoption_group_t:
 *
 * A `xoption_group_t` struct defines the options in a single
 * group. The struct has only private fields and should not be directly accessed.
 *
 * All options in a group share the same translation function. Libraries which
 * need to parse commandline options are expected to provide a function for
 * getting a `xoption_group_t` holding their options, which
 * the application can then add to its #xoption_context_t.
 */
typedef struct _GOptionGroup   xoption_group_t;
typedef struct _GOptionEntry   GOptionEntry;

/**
 * GOptionFlags:
 * @G_OPTION_FLAG_NONE: No flags. Since: 2.42.
 * @G_OPTION_FLAG_HIDDEN: The option doesn't appear in `--help` output.
 * @G_OPTION_FLAG_IN_MAIN: The option appears in the main section of the
 *     `--help` output, even if it is defined in a group.
 * @G_OPTION_FLAG_REVERSE: For options of the %G_OPTION_ARG_NONE kind, this
 *     flag indicates that the sense of the option is reversed.
 * @G_OPTION_FLAG_NO_ARG: For options of the %G_OPTION_ARG_CALLBACK kind,
 *     this flag indicates that the callback does not take any argument
 *     (like a %G_OPTION_ARG_NONE option). Since 2.8
 * @G_OPTION_FLAXFILENAME: For options of the %G_OPTION_ARG_CALLBACK
 *     kind, this flag indicates that the argument should be passed to the
 *     callback in the GLib filename encoding rather than UTF-8. Since 2.8
 * @G_OPTION_FLAG_OPTIONAL_ARG: For options of the %G_OPTION_ARG_CALLBACK
 *     kind, this flag indicates that the argument supply is optional.
 *     If no argument is given then data of %GOptionParseFunc will be
 *     set to NULL. Since 2.8
 * @G_OPTION_FLAG_NOALIAS: This flag turns off the automatic conflict
 *     resolution which prefixes long option names with `groupname-` if
 *     there is a conflict. This option should only be used in situations
 *     where aliasing is necessary to model some legacy commandline interface.
 *     It is not safe to use this option, unless all option groups are under
 *     your direct control. Since 2.8.
 *
 * Flags which modify individual options.
 */
typedef enum
{
  G_OPTION_FLAG_NONE            = 0,
  G_OPTION_FLAG_HIDDEN		= 1 << 0,
  G_OPTION_FLAG_IN_MAIN		= 1 << 1,
  G_OPTION_FLAG_REVERSE		= 1 << 2,
  G_OPTION_FLAG_NO_ARG		= 1 << 3,
  G_OPTION_FLAXFILENAME	= 1 << 4,
  G_OPTION_FLAG_OPTIONAL_ARG    = 1 << 5,
  G_OPTION_FLAG_NOALIAS	        = 1 << 6
} GOptionFlags;

/**
 * GOptionArg:
 * @G_OPTION_ARG_NONE: No extra argument. This is useful for simple flags.
 * @G_OPTION_ARG_STRING: The option takes a UTF-8 string argument.
 * @G_OPTION_ARG_INT: The option takes an integer argument.
 * @G_OPTION_ARG_CALLBACK: The option provides a callback (of type
 *     #GOptionArgFunc) to parse the extra argument.
 * @G_OPTION_ARXFILENAME: The option takes a filename as argument, which will
       be in the GLib filename encoding rather than UTF-8.
 * @G_OPTION_ARG_STRING_ARRAY: The option takes a string argument, multiple
 *     uses of the option are collected into an array of strings.
 * @G_OPTION_ARXFILENAME_ARRAY: The option takes a filename as argument,
 *     multiple uses of the option are collected into an array of strings.
 * @G_OPTION_ARG_DOUBLE: The option takes a double argument. The argument
 *     can be formatted either for the user's locale or for the "C" locale.
 *     Since 2.12
 * @G_OPTION_ARG_INT64: The option takes a 64-bit integer. Like
 *     %G_OPTION_ARG_INT but for larger numbers. The number can be in
 *     decimal base, or in hexadecimal (when prefixed with `0x`, for
 *     example, `0xffffffff`). Since 2.12
 *
 * The #GOptionArg enum values determine which type of extra argument the
 * options expect to find. If an option expects an extra argument, it can
 * be specified in several ways; with a short option: `-x arg`, with a long
 * option: `--name arg` or combined in a single argument: `--name=arg`.
 */
typedef enum
{
  G_OPTION_ARG_NONE,
  G_OPTION_ARG_STRING,
  G_OPTION_ARG_INT,
  G_OPTION_ARG_CALLBACK,
  G_OPTION_ARXFILENAME,
  G_OPTION_ARG_STRING_ARRAY,
  G_OPTION_ARXFILENAME_ARRAY,
  G_OPTION_ARG_DOUBLE,
  G_OPTION_ARG_INT64
} GOptionArg;

/**
 * GOptionArgFunc:
 * @option_name: The name of the option being parsed. This will be either a
 *  single dash followed by a single letter (for a short name) or two dashes
 *  followed by a long option name.
 * @value: The value to be parsed.
 * @data: User data added to the #xoption_group_t containing the option when it
 *  was created with xoption_group_new()
 * @error: A return location for errors. The error code %G_OPTION_ERROR_FAILED
 *  is intended to be used for errors in #GOptionArgFunc callbacks.
 *
 * The type of function to be passed as callback for %G_OPTION_ARG_CALLBACK
 * options.
 *
 * Returns: %TRUE if the option was successfully parsed, %FALSE if an error
 *  occurred, in which case @error should be set with g_set_error()
 */
typedef xboolean_t (*GOptionArgFunc) (const xchar_t    *option_name,
				    const xchar_t    *value,
				    xpointer_t        data,
				    xerror_t        **error);

/**
 * GOptionParseFunc:
 * @context: The active #xoption_context_t
 * @group: The group to which the function belongs
 * @data: User data added to the #xoption_group_t containing the option when it
 *  was created with xoption_group_new()
 * @error: A return location for error details
 *
 * The type of function that can be called before and after parsing.
 *
 * Returns: %TRUE if the function completed successfully, %FALSE if an error
 *  occurred, in which case @error should be set with g_set_error()
 */
typedef xboolean_t (*GOptionParseFunc) (xoption_context_t *context,
				      xoption_group_t   *group,
				      xpointer_t	      data,
				      xerror_t        **error);

/**
 * GOptionErrorFunc:
 * @context: The active #xoption_context_t
 * @group: The group to which the function belongs
 * @data: User data added to the #xoption_group_t containing the option when it
 *  was created with xoption_group_new()
 * @error: The #xerror_t containing details about the parse error
 *
 * The type of function to be used as callback when a parse error occurs.
 */
typedef void (*GOptionErrorFunc) (xoption_context_t *context,
				  xoption_group_t   *group,
				  xpointer_t        data,
				  xerror_t        **error);

/**
 * G_OPTION_ERROR:
 *
 * Error domain for option parsing. Errors in this domain will
 * be from the #GOptionError enumeration. See #xerror_t for information on
 * error domains.
 */
#define G_OPTION_ERROR (g_option_error_quark ())

/**
 * GOptionError:
 * @G_OPTION_ERROR_UNKNOWN_OPTION: An option was not known to the parser.
 *  This error will only be reported, if the parser hasn't been instructed
 *  to ignore unknown options, see g_option_context_set_ignore_unknown_options().
 * @G_OPTION_ERROR_BAD_VALUE: A value couldn't be parsed.
 * @G_OPTION_ERROR_FAILED: A #GOptionArgFunc callback failed.
 *
 * Error codes returned by option parsing.
 */
typedef enum
{
  G_OPTION_ERROR_UNKNOWN_OPTION,
  G_OPTION_ERROR_BAD_VALUE,
  G_OPTION_ERROR_FAILED
} GOptionError;

XPL_AVAILABLE_IN_ALL
xquark g_option_error_quark (void);

/**
 * GOptionEntry:
 * @long_name: The long name of an option can be used to specify it
 *     in a commandline as `--long_name`. Every option must have a
 *     long name. To resolve conflicts if multiple option groups contain
 *     the same long name, it is also possible to specify the option as
 *     `--groupname-long_name`.
 * @short_name: If an option has a short name, it can be specified
 *     `-short_name` in a commandline. @short_name must be  a printable
 *     ASCII character different from '-', or zero if the option has no
 *     short name.
 * @flags: Flags from #GOptionFlags
 * @arg: The type of the option, as a #GOptionArg
 * @arg_data: If the @arg type is %G_OPTION_ARG_CALLBACK, then @arg_data
 *     must point to a #GOptionArgFunc callback function, which will be
 *     called to handle the extra argument. Otherwise, @arg_data is a
 *     pointer to a location to store the value, the required type of
 *     the location depends on the @arg type:
 *     - %G_OPTION_ARG_NONE: %xboolean_t
 *     - %G_OPTION_ARG_STRING: %xchar_t*
 *     - %G_OPTION_ARG_INT: %xint_t
 *     - %G_OPTION_ARXFILENAME: %xchar_t*
 *     - %G_OPTION_ARG_STRING_ARRAY: %xchar_t**
 *     - %G_OPTION_ARXFILENAME_ARRAY: %xchar_t**
 *     - %G_OPTION_ARG_DOUBLE: %xdouble_t
 *     If @arg type is %G_OPTION_ARG_STRING or %G_OPTION_ARXFILENAME,
 *     the location will contain a newly allocated string if the option
 *     was given. That string needs to be freed by the callee using g_free().
 *     Likewise if @arg type is %G_OPTION_ARG_STRING_ARRAY or
 *     %G_OPTION_ARXFILENAME_ARRAY, the data should be freed using xstrfreev().
 * @description: the description for the option in `--help`
 *     output. The @description is translated using the @translate_func
 *     of the group, see xoption_group_set_translation_domain().
 * @arg_description: The placeholder to use for the extra argument parsed
 *     by the option in `--help` output. The @arg_description is translated
 *     using the @translate_func of the group, see
 *     xoption_group_set_translation_domain().
 *
 * A GOptionEntry struct defines a single option. To have an effect, they
 * must be added to a #xoption_group_t with g_option_context_add_main_entries()
 * or xoption_group_add_entries().
 */
struct _GOptionEntry
{
  const xchar_t *long_name;
  xchar_t        short_name;
  xint_t         flags;

  GOptionArg   arg;
  xpointer_t     arg_data;

  const xchar_t *description;
  const xchar_t *arg_description;
};

/**
 * G_OPTION_REMAINING:
 *
 * If a long option in the main group has this name, it is not treated as a
 * regular option. Instead it collects all non-option arguments which would
 * otherwise be left in `argv`. The option must be of type
 * %G_OPTION_ARG_CALLBACK, %G_OPTION_ARG_STRING_ARRAY
 * or %G_OPTION_ARXFILENAME_ARRAY.
 *
 *
 * Using %G_OPTION_REMAINING instead of simply scanning `argv`
 * for leftover arguments has the advantage that GOption takes care of
 * necessary encoding conversions for strings or filenames.
 *
 * Since: 2.6
 */
#define G_OPTION_REMAINING ""

/**
 * G_OPTION_ENTRY_NULL:
 *
 * A #GOptionEntry array requires a %NULL terminator, this macro can
 * be used as terminator instead of an explicit `{ 0 }` but it cannot
 * be assigned to a variable.
 *
 * |[
 *   GOptionEntry option[] = { G_OPTION_ENTRY_NULL };
 * ]|
 *
 * Since: 2.70
 */
#define G_OPTION_ENTRY_NULL    \
  XPL_AVAILABLE_MACRO_IN_2_70 \
  { NULL, 0, 0, 0, NULL, NULL, NULL }


XPL_AVAILABLE_IN_ALL
xoption_context_t *g_option_context_new              (const xchar_t         *parameter_string);
XPL_AVAILABLE_IN_ALL
void            g_option_context_set_summary      (xoption_context_t      *context,
                                                   const xchar_t         *summary);
XPL_AVAILABLE_IN_ALL
const xchar_t *   g_option_context_get_summary      (xoption_context_t     *context);
XPL_AVAILABLE_IN_ALL
void            g_option_context_set_description  (xoption_context_t      *context,
                                                   const xchar_t         *description);
XPL_AVAILABLE_IN_ALL
const xchar_t *   g_option_context_get_description  (xoption_context_t     *context);
XPL_AVAILABLE_IN_ALL
void            g_option_context_free             (xoption_context_t      *context);
XPL_AVAILABLE_IN_ALL
void		g_option_context_set_help_enabled (xoption_context_t      *context,
						   xboolean_t		help_enabled);
XPL_AVAILABLE_IN_ALL
xboolean_t	g_option_context_get_help_enabled (xoption_context_t      *context);
XPL_AVAILABLE_IN_ALL
void		g_option_context_set_ignore_unknown_options (xoption_context_t *context,
							     xboolean_t	     ignore_unknown);
XPL_AVAILABLE_IN_ALL
xboolean_t        g_option_context_get_ignore_unknown_options (xoption_context_t *context);

XPL_AVAILABLE_IN_2_44
void            g_option_context_set_strict_posix           (xoption_context_t *context,
                                                             xboolean_t        strict_posix);
XPL_AVAILABLE_IN_2_44
xboolean_t        g_option_context_get_strict_posix           (xoption_context_t *context);

XPL_AVAILABLE_IN_ALL
void            g_option_context_add_main_entries (xoption_context_t      *context,
						   const GOptionEntry  *entries,
						   const xchar_t         *translation_domain);
XPL_AVAILABLE_IN_ALL
xboolean_t        g_option_context_parse            (xoption_context_t      *context,
						   xint_t                *argc,
						   xchar_t             ***argv,
						   xerror_t             **error);
XPL_AVAILABLE_IN_2_40
xboolean_t        g_option_context_parse_strv       (xoption_context_t      *context,
                                                   xchar_t             ***arguments,
                                                   xerror_t             **error);
XPL_AVAILABLE_IN_ALL
void            g_option_context_set_translate_func (xoption_context_t     *context,
						     GTranslateFunc      func,
						     xpointer_t            data,
						     xdestroy_notify_t      destroy_notify);
XPL_AVAILABLE_IN_ALL
void            g_option_context_set_translation_domain (xoption_context_t  *context,
							 const xchar_t     *domain);

XPL_AVAILABLE_IN_ALL
void            g_option_context_add_group      (xoption_context_t *context,
						 xoption_group_t   *group);
XPL_AVAILABLE_IN_ALL
void          g_option_context_set_main_group (xoption_context_t *context,
					       xoption_group_t   *group);
XPL_AVAILABLE_IN_ALL
xoption_group_t *g_option_context_get_main_group (xoption_context_t *context);
XPL_AVAILABLE_IN_ALL
xchar_t        *g_option_context_get_help       (xoption_context_t *context,
                                               xboolean_t        main_help,
                                               xoption_group_t   *group);

XPL_AVAILABLE_IN_ALL
xoption_group_t *xoption_group_new                    (const xchar_t        *name,
						     const xchar_t        *description,
						     const xchar_t        *help_description,
						     xpointer_t            user_data,
						     xdestroy_notify_t      destroy);
XPL_AVAILABLE_IN_ALL
void	      xoption_group_set_parse_hooks	    (xoption_group_t       *group,
						     GOptionParseFunc    pre_parse_func,
						     GOptionParseFunc	 post_parse_func);
XPL_AVAILABLE_IN_ALL
void	      xoption_group_set_error_hook	    (xoption_group_t       *group,
						     GOptionErrorFunc	 error_func);
XPL_DEPRECATED_IN_2_44
void          xoption_group_free                   (xoption_group_t       *group);
XPL_AVAILABLE_IN_2_44
xoption_group_t *xoption_group_ref                    (xoption_group_t       *group);
XPL_AVAILABLE_IN_2_44
void          xoption_group_unref                  (xoption_group_t       *group);
XPL_AVAILABLE_IN_ALL
void          xoption_group_add_entries            (xoption_group_t       *group,
						     const GOptionEntry *entries);
XPL_AVAILABLE_IN_ALL
void          xoption_group_set_translate_func     (xoption_group_t       *group,
						     GTranslateFunc      func,
						     xpointer_t            data,
						     xdestroy_notify_t      destroy_notify);
XPL_AVAILABLE_IN_ALL
void          xoption_group_set_translation_domain (xoption_group_t       *group,
						     const xchar_t        *domain);

G_END_DECLS

#endif /* __G_OPTION_H__ */
