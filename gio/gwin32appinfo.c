/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
 * Copyright (C) 2014 Руслан Ижбулатов
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
 * Authors: Alexander Larsson <alexl@redhat.com>
 *          Руслан Ижбулатов  <lrn1986@gmail.com>
 */

#include "config.h"

#define COBJMACROS

#include <string.h>

#include "gcontenttype.h"
#include "gwin32appinfo.h"
#include "gappinfo.h"
#include "gioerror.h"
#include "gfile.h"
#include <glib/gstdio.h>
#include "glibintl.h"
#include <gio/gwin32registrykey.h>
#include <shlobj.h>
/* Contains the definitions from shlobj.h that are
 * guarded as Windows8-or-newer and are unavailable
 * to GLib, being only Windows7-or-newer.
 */
#include "gwin32api-application-activation-manager.h"

#include <windows.h>
/* For SHLoadIndirectString() */
#include <shlwapi.h>

#include <glib/gstdioprivate.h>
#include "giowin32-priv.h"
#include "glib-private.h"

/* We need to watch 8 places:
 * 0) HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations
 *    (anything below that key)
 *    On change: re-enumerate subkeys, read their values.
 * 1) HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts
 *    (anything below that key)
 *    On change: re-enumerate subkeys
 * 2) HKEY_CURRENT_USER\\Software\\Clients (anything below that key)
 *    On change: re-read the whole hierarchy of handlers
 * 3) HKEY_LOCAL_MACHINE\\Software\\Clients (anything below that key)
 *    On change: re-read the whole hierarchy of handlers
 * 4) HKEY_LOCAL_MACHINE\\Software\\RegisteredApplications (values of that key)
 *    On change: re-read the value list of registered applications
 * 5) HKEY_CURRENT_USER\\Software\\RegisteredApplications (values of that key)
 *    On change: re-read the value list of registered applications
 * 6) HKEY_CLASSES_ROOT\\Applications (anything below that key)
 *    On change: re-read the whole hierarchy of apps
 * 7) HKEY_CLASSES_ROOT (only its subkeys)
 *    On change: re-enumerate subkeys, try to filter out wrong names.
 *
 *
 * About verbs. A registry key (the name of that key is known as ProgID)
 * can contain a "shell" subkey, which can then contain a number of verb
 * subkeys (the most common being the "open" verb), and each of these
 * contains a "command" subkey, which has a default string value that
 * is the command to be run.
 * Most ProgIDs are in HKEY_CLASSES_ROOT, but some are nested deeper in
 * the registry (such as HKEY_CURRENT_USER\\Software\\<softwarename>).
 *
 * Verb selection works like this (according to https://docs.microsoft.com/en-us/windows/win32/shell/context ):
 * 1) If "open" verb is available, that verb is used.
 * 2) If the Shell subkey has a default string value, and if a verb subkey
 *    with that name exists, that verb is used.
 * 3) The first subkey found in the list of verb subkeys is used.
 * 4) The "openwith" verb is used
 *
 * Testing suggests that Windows never reaches the point 4 in any realistic
 * circumstances. If a "command" subkey is missing for a verb, or if it has
 * an empty string as its default value, the app launch fails
 * (the "openwith" verb is not used, even if it's present).
 * If the command is present, but is not valid (runs nonexisting executable,
 * for example), then other verbs are not checked.
 * It seems that when the documentation said "openwith verb", it meant
 * that Windows invokes the default "Open with..." dialog (it does not
 * look at the "openwith" verb subkey, even if it's there).
 * If a verb subkey that is supposed to be used is present, but it lacks
 * a command subkey, an error message is shown and nothing else happens.
 */

#define _verb_idx(array,index) ((GWin32AppInfoShellVerb *) xptr_array_index (array, index))

#define _lookup_by_verb(array, verb, dst, itemtype) do { \
  xsize_t _index; \
  itemtype *_v; \
  for (_index = 0; array && _index < array->len; _index++) \
    { \
      _v = (itemtype *) xptr_array_index (array, _index); \
      if (_wcsicmp (_v->verb_name, (verb)) == 0) \
        { \
          *(dst) = _v; \
          break; \
        } \
    } \
  if (array == NULL || _index >= array->len) \
    *(dst) = NULL; \
} while (0)

#define _verb_lookup(array, verb, dst) _lookup_by_verb (array, verb, dst, GWin32AppInfoShellVerb)

/* Because with subcommands a verb would have
 * a name like "foo\\bar", but the key its command
 * should be looked for is "shell\\foo\\shell\\bar\\command"
 */
typedef struct _reg_verb {
  xunichar2_t *name;
  xunichar2_t *shellpath;
} reg_verb;

typedef struct _GWin32AppInfoURLSchema GWin32AppInfoURLSchema;
typedef struct _GWin32AppInfoFileExtension GWin32AppInfoFileExtension;
typedef struct _GWin32AppInfoShellVerb GWin32AppInfoShellVerb;
typedef struct _GWin32AppInfoHandler GWin32AppInfoHandler;
typedef struct _GWin32AppInfoApplication GWin32AppInfoApplication;

typedef struct _GWin32AppInfoURLSchemaClass GWin32AppInfoURLSchemaClass;
typedef struct _GWin32AppInfoFileExtensionClass GWin32AppInfoFileExtensionClass;
typedef struct _GWin32AppInfoShellVerbClass GWin32AppInfoShellVerbClass;
typedef struct _GWin32AppInfoHandlerClass GWin32AppInfoHandlerClass;
typedef struct _GWin32AppInfoApplicationClass GWin32AppInfoApplicationClass;

struct _GWin32AppInfoURLSchemaClass
{
  xobject_class_t parent_class;
};

struct _GWin32AppInfoFileExtensionClass
{
  xobject_class_t parent_class;
};

struct _GWin32AppInfoHandlerClass
{
  xobject_class_t parent_class;
};

struct _GWin32AppInfoApplicationClass
{
  xobject_class_t parent_class;
};

struct _GWin32AppInfoShellVerbClass
{
  xobject_class_t parent_class;
};

struct _GWin32AppInfoURLSchema {
  xobject_t parent_instance;

  /* url schema (stuff before ':') */
  xunichar2_t *schema;

  /* url schema (stuff before ':'), in UTF-8 */
  xchar_t *schema_u8;

  /* url schema (stuff before ':'), in UTF-8, folded */
  xchar_t *schema_u8_folded;

  /* Handler currently selected for this schema. Can be NULL. */
  GWin32AppInfoHandler *chosen_handler;

  /* Maps folded handler IDs -> to GWin32AppInfoHandlers for this schema.
   * Includes the chosen handler, if any.
   */
  xhashtable_t *handlers;
};

struct _GWin32AppInfoHandler {
  xobject_t parent_instance;

  /* Usually a class name in HKCR */
  xunichar2_t *handler_id;

  /* Registry object obtained by opening @handler_id.
   * Can be used to watch this handler.
   * May be %NULL (for fake handlers that we made up).
   */
  GWin32RegistryKey *key;

  /* @handler_id, in UTF-8, folded */
  xchar_t *handler_id_folded;

  /* Icon of the application for this handler */
  xicon_t *icon;

  /* Verbs that this handler supports */
  xptr_array_t *verbs; /* of GWin32AppInfoShellVerb */

  /* AppUserModelID for a UWP application. When this is not NULL,
   * this handler launches a UWP application.
   * UWP applications are launched using a COM interface and have no commandlines,
   * and the verbs will reflect that too.
   */
  xunichar2_t *uwp_aumid;
};

struct _GWin32AppInfoShellVerb {
  xobject_t parent_instance;

  /* The verb that is used to invoke this handler. */
  xunichar2_t *verb_name;

  /* User-friendly (localized) verb name. */
  xchar_t *verb_displayname;

  /* %TRUE if this verb is for a UWP app.
   * It means that @command, @executable and @dll_function are %NULL.
   */
  xboolean_t is_uwp;

  /* shell/verb/command */
  xunichar2_t *command;

  /* Same as @command, but in UTF-8 */
  xchar_t *command_utf8;

  /* Executable of the program (UTF-8) */
  xchar_t *executable;

  /* Executable of the program (for matching, in folded form; UTF-8) */
  xchar_t *executable_folded;

  /* Pointer to a location within @executable */
  xchar_t *executable_basename;

  /* If not NULL, then @executable and its derived fields contain the name
   * of a DLL file (without the name of the function that rundll32.exe should
   * invoke), and this field contains the name of the function to be invoked.
   * The application is then invoked as 'rundll32.exe "dll_path",dll_function other_arguments...'.
   */
  xchar_t *dll_function;

  /* The application that is linked to this verb. */
  GWin32AppInfoApplication *app;
};

struct _GWin32AppInfoFileExtension {
  xobject_t parent_instance;

  /* File extension (with leading '.') */
  xunichar2_t *extension;

  /* File extension (with leading '.'), in UTF-8 */
  xchar_t *extension_u8;

  /* handler currently selected for this extension. Can be NULL. */
  GWin32AppInfoHandler *chosen_handler;

  /* Maps folded handler IDs -> to GWin32AppInfoHandlers for this extension.
   * Includes the chosen handler, if any.
   */
  xhashtable_t *handlers;
};

struct _GWin32AppInfoApplication {
  xobject_t parent_instance;

  /* Canonical name (used for key names).
   * For applications tracked by id this is the root registry
   * key path for the application.
   * For applications tracked by executable name this is the
   * basename of the executable.
   * For UWP apps this is the AppUserModelID.
   * For fake applications this is the full filename of the
   * executable (as far as it can be inferred from a command line,
   * meaning that it can also be a basename, if that's
   * all that a commandline happen to give us).
   */
  xunichar2_t *canonical_name;

  /* @canonical_name, in UTF-8 */
  xchar_t *canonical_name_u8;

  /* @canonical_name, in UTF-8, folded */
  xchar_t *canonical_name_folded;

  /* Human-readable name in English. Can be NULL */
  xunichar2_t *pretty_name;

  /* Human-readable name in English, UTF-8. Can be NULL */
  xchar_t *pretty_name_u8;

  /* Human-readable name in user's language. Can be NULL  */
  xunichar2_t *localized_pretty_name;

  /* Human-readable name in user's language, UTF-8. Can be NULL  */
  xchar_t *localized_pretty_name_u8;

  /* Description, could be in user's language. Can be NULL */
  xunichar2_t *description;

  /* Description, could be in user's language, UTF-8. Can be NULL */
  xchar_t *description_u8;

  /* Verbs that this application supports */
  xptr_array_t *verbs; /* of GWin32AppInfoShellVerb */

  /* Explicitly supported URLs, hashmap from map-owned xchar_t ptr (schema,
   * UTF-8, folded) -> to a GWin32AppInfoHandler
   * Schema can be used as a key in the urls hashmap.
   */
  xhashtable_t *supported_urls;

  /* Explicitly supported extensions, hashmap from map-owned xchar_t ptr
   * (.extension, UTF-8, folded) -> to a GWin32AppInfoHandler
   * Extension can be used as a key in the extensions hashmap.
   */
  xhashtable_t *supported_exts;

  /* Icon of the application (remember, handler can have its own icon too) */
  xicon_t *icon;

  /* Set to TRUE to prevent this app from appearing in lists of apps for
   * opening files. This will not prevent it from appearing in lists of apps
   * just for running, or lists of apps for opening exts/urls for which this
   * app reports explicit support.
   */
  xboolean_t no_open_with;

  /* Set to TRUE for applications from HKEY_CURRENT_USER.
   * Give them priority over applications from HKEY_LOCAL_MACHINE, when all
   * other things are equal.
   */
  xboolean_t user_specific;

  /* Set to TRUE for applications that are machine-wide defaults (i.e. default
   * browser) */
  xboolean_t default_app;

  /* Set to TRUE for UWP applications */
  xboolean_t is_uwp;
};

#define XTYPE_WIN32_APPINFO_URL_SCHEMA           (g_win32_appinfo_url_schema_get_type ())
#define G_WIN32_APPINFO_URL_SCHEMA(obj)           (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_WIN32_APPINFO_URL_SCHEMA, GWin32AppInfoURLSchema))

#define XTYPE_WIN32_APPINFO_FILE_EXTENSION       (g_win32_appinfo_file_extension_get_type ())
#define G_WIN32_APPINFO_FILE_EXTENSION(obj)       (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_WIN32_APPINFO_FILE_EXTENSION, GWin32AppInfoFileExtension))

#define XTYPE_WIN32_APPINFO_HANDLER              (g_win32_appinfo_handler_get_type ())
#define G_WIN32_APPINFO_HANDLER(obj)              (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_WIN32_APPINFO_HANDLER, GWin32AppInfoHandler))

#define XTYPE_WIN32_APPINFO_APPLICATION          (g_win32_appinfo_application_get_type ())
#define G_WIN32_APPINFO_APPLICATION(obj)          (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_WIN32_APPINFO_APPLICATION, GWin32AppInfoApplication))

#define XTYPE_WIN32_APPINFO_SHELL_VERB           (g_win32_appinfo_shell_verb_get_type ())
#define G_WIN32_APPINFO_SHELL_VERB(obj)          (XTYPE_CHECK_INSTANCE_CAST ((obj), XTYPE_WIN32_APPINFO_SHELL_VERB, GWin32AppInfoShellVerb))

xtype_t g_win32_appinfo_url_schema_get_type (void) G_GNUC_CONST;
xtype_t g_win32_appinfo_file_extension_get_type (void) G_GNUC_CONST;
xtype_t g_win32_appinfo_shell_verb_get_type (void) G_GNUC_CONST;
xtype_t g_win32_appinfo_handler_get_type (void) G_GNUC_CONST;
xtype_t g_win32_appinfo_application_get_type (void) G_GNUC_CONST;

XDEFINE_TYPE (GWin32AppInfoURLSchema, g_win32_appinfo_url_schema, XTYPE_OBJECT)
XDEFINE_TYPE (GWin32AppInfoFileExtension, g_win32_appinfo_file_extension, XTYPE_OBJECT)
XDEFINE_TYPE (GWin32AppInfoShellVerb, g_win32_appinfo_shell_verb, XTYPE_OBJECT)
XDEFINE_TYPE (GWin32AppInfoHandler, g_win32_appinfo_handler, XTYPE_OBJECT)
XDEFINE_TYPE (GWin32AppInfoApplication, g_win32_appinfo_application, XTYPE_OBJECT)

static void
g_win32_appinfo_url_schema_dispose (xobject_t *object)
{
  GWin32AppInfoURLSchema *url = G_WIN32_APPINFO_URL_SCHEMA (object);

  g_clear_pointer (&url->schema, g_free);
  g_clear_pointer (&url->schema_u8, g_free);
  g_clear_pointer (&url->schema_u8_folded, g_free);
  g_clear_object (&url->chosen_handler);
  g_clear_pointer (&url->handlers, xhash_table_destroy);
  XOBJECT_CLASS (g_win32_appinfo_url_schema_parent_class)->dispose (object);
}


static void
g_win32_appinfo_handler_dispose (xobject_t *object)
{
  GWin32AppInfoHandler *handler = G_WIN32_APPINFO_HANDLER (object);

  g_clear_pointer (&handler->handler_id, g_free);
  g_clear_pointer (&handler->handler_id_folded, g_free);
  g_clear_object (&handler->key);
  g_clear_object (&handler->icon);
  g_clear_pointer (&handler->verbs, xptr_array_unref);
  g_clear_pointer (&handler->uwp_aumid, g_free);
  XOBJECT_CLASS (g_win32_appinfo_handler_parent_class)->dispose (object);
}

static void
g_win32_appinfo_file_extension_dispose (xobject_t *object)
{
  GWin32AppInfoFileExtension *ext = G_WIN32_APPINFO_FILE_EXTENSION (object);

  g_clear_pointer (&ext->extension, g_free);
  g_clear_pointer (&ext->extension_u8, g_free);
  g_clear_object (&ext->chosen_handler);
  g_clear_pointer (&ext->handlers, xhash_table_destroy);
  XOBJECT_CLASS (g_win32_appinfo_file_extension_parent_class)->dispose (object);
}

static void
g_win32_appinfo_shell_verb_dispose (xobject_t *object)
{
  GWin32AppInfoShellVerb *shverb = G_WIN32_APPINFO_SHELL_VERB (object);

  g_clear_pointer (&shverb->verb_name, g_free);
  g_clear_pointer (&shverb->verb_displayname, g_free);
  g_clear_pointer (&shverb->command, g_free);
  g_clear_pointer (&shverb->command_utf8, g_free);
  g_clear_pointer (&shverb->executable_folded, g_free);
  g_clear_pointer (&shverb->executable, g_free);
  g_clear_pointer (&shverb->dll_function, g_free);
  g_clear_object (&shverb->app);
  XOBJECT_CLASS (g_win32_appinfo_shell_verb_parent_class)->dispose (object);
}

static void
g_win32_appinfo_application_dispose (xobject_t *object)
{
  GWin32AppInfoApplication *app = G_WIN32_APPINFO_APPLICATION (object);

  g_clear_pointer (&app->canonical_name_u8, g_free);
  g_clear_pointer (&app->canonical_name_folded, g_free);
  g_clear_pointer (&app->canonical_name, g_free);
  g_clear_pointer (&app->pretty_name, g_free);
  g_clear_pointer (&app->localized_pretty_name, g_free);
  g_clear_pointer (&app->description, g_free);
  g_clear_pointer (&app->pretty_name_u8, g_free);
  g_clear_pointer (&app->localized_pretty_name_u8, g_free);
  g_clear_pointer (&app->description_u8, g_free);
  g_clear_pointer (&app->supported_urls, xhash_table_destroy);
  g_clear_pointer (&app->supported_exts, xhash_table_destroy);
  g_clear_object (&app->icon);
  g_clear_pointer (&app->verbs, xptr_array_unref);
  XOBJECT_CLASS (g_win32_appinfo_application_parent_class)->dispose (object);
}

static const xchar_t *
g_win32_appinfo_application_get_some_name (GWin32AppInfoApplication *app)
{
  if (app->localized_pretty_name_u8)
    return app->localized_pretty_name_u8;

  if (app->pretty_name_u8)
    return app->pretty_name_u8;

  return app->canonical_name_u8;
}

static void
g_win32_appinfo_url_schema_class_init (GWin32AppInfoURLSchemaClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->dispose = g_win32_appinfo_url_schema_dispose;
}

static void
g_win32_appinfo_file_extension_class_init (GWin32AppInfoFileExtensionClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->dispose = g_win32_appinfo_file_extension_dispose;
}

static void
g_win32_appinfo_shell_verb_class_init (GWin32AppInfoShellVerbClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->dispose = g_win32_appinfo_shell_verb_dispose;
}

static void
g_win32_appinfo_handler_class_init (GWin32AppInfoHandlerClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->dispose = g_win32_appinfo_handler_dispose;
}

static void
g_win32_appinfo_application_class_init (GWin32AppInfoApplicationClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->dispose = g_win32_appinfo_application_dispose;
}

static void
g_win32_appinfo_url_schema_init (GWin32AppInfoURLSchema *self)
{
  self->handlers = xhash_table_new_full (xstr_hash,
                                          xstr_equal,
                                          g_free,
                                          xobject_unref);
}

static void
g_win32_appinfo_shell_verb_init (GWin32AppInfoShellVerb *self)
{
}

static void
g_win32_appinfo_file_extension_init (GWin32AppInfoFileExtension *self)
{
  self->handlers = xhash_table_new_full (xstr_hash,
                                          xstr_equal,
                                          g_free,
                                          xobject_unref);
}

static void
g_win32_appinfo_handler_init (GWin32AppInfoHandler *self)
{
  self->verbs = xptr_array_new_with_free_func (xobject_unref);
}

static void
g_win32_appinfo_application_init (GWin32AppInfoApplication *self)
{
  self->supported_urls = xhash_table_new_full (xstr_hash,
                                                xstr_equal,
                                                g_free,
                                                xobject_unref);
  self->supported_exts = xhash_table_new_full (xstr_hash,
                                                xstr_equal,
                                                g_free,
                                                xobject_unref);
  self->verbs = xptr_array_new_with_free_func (xobject_unref);
}

/* The AppInfo threadpool that does asynchronous AppInfo tree rebuilds */
static GThreadPool *gio_win32_appinfo_threadpool;

/* This mutex is held by a thread that reads or writes the AppInfo tree.
 * (tree object references can be obtained and later read without
 *  holding this mutex, since objects are practically immutable).
 */
static xmutex_t gio_win32_appinfo_mutex;

/* Any thread wanting to access AppInfo can wait on this condition */
static xcond_t gio_win32_appinfo_cond;

/* Increased to indicate that AppInfo tree does needs to be rebuilt.
 * AppInfo thread checks this to see if it needs to
 * do a tree re-build. If the value changes during a rebuild,
 * another rebuild is triggered after that.
 * Other threads check this to see if they need
 * to wait for a tree re-build to finish.
 */
static xint_t gio_win32_appinfo_update_counter = 0;

/* Map of owned ".ext" (with '.', UTF-8, folded)
 * to GWin32AppInfoFileExtension ptr
 */
static xhashtable_t *extensions = NULL;

/* Map of owned "schema" (without ':', UTF-8, folded)
 * to GWin32AppInfoURLSchema ptr
 */
static xhashtable_t *urls = NULL;

/* Map of owned "appID" (UTF-8, folded) to
 * a GWin32AppInfoApplication
 */
static xhashtable_t *apps_by_id = NULL;

/* Map of owned "app.exe" (UTF-8, folded) to
 * a GWin32AppInfoApplication.
 * This map and its values are separate from apps_by_id. The fact that an app
 * with known ID has the same executable [base]name as an app in this map does
 * not mean that they are the same application.
 */
static xhashtable_t *apps_by_exe = NULL;

/* Map of owned "path:\to\app.exe" (UTF-8, folded) to
 * a GWin32AppInfoApplication.
 * The app objects in this map are fake - they are linked to
 * handlers that do not have any apps associated with them.
 */
static xhashtable_t *fake_apps = NULL;

/* Map of owned "handler id" (UTF-8, folded)
 * to a GWin32AppInfoHandler
 */
static xhashtable_t *handlers = NULL;

/* Temporary (only exists while the registry is being scanned) table
 * that maps GWin32RegistryKey objects (keeps a ref) to owned AUMId wchar strings.
 */
static xhashtable_t *uwp_handler_table = NULL;

/* Watch this whole subtree */
static GWin32RegistryKey *url_associations_key;

/* Watch this whole subtree */
static GWin32RegistryKey *file_exts_key;

/* Watch this whole subtree */
static GWin32RegistryKey *user_clients_key;

/* Watch this whole subtree */
static GWin32RegistryKey *system_clients_key;

/* Watch this key */
static GWin32RegistryKey *user_registered_apps_key;

/* Watch this key */
static GWin32RegistryKey *system_registered_apps_key;

/* Watch this whole subtree */
static GWin32RegistryKey *applications_key;

/* Watch this key */
static GWin32RegistryKey *classes_root_key;

#define URL_ASSOCIATIONS L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\"
#define USER_CHOICE L"\\UserChoice"
#define OPEN_WITH_PROGIDS L"\\OpenWithProgids"
#define FILE_EXTS L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\"
#define HKCR L"HKEY_CLASSES_ROOT\\"
#define HKCU L"HKEY_CURRENT_USER\\"
#define HKLM L"HKEY_LOCAL_MACHINE\\"
#define REG_PATH_MAX 256
#define REG_PATH_MAX_SIZE (REG_PATH_MAX * sizeof (xunichar2_t))

/* for g_wcsdup(),
 *     _g_win32_extract_executable(),
 *     _g_win32_fixup_broken_microsoft_rundll_commandline()
 */
#include "giowin32-private.c"

/* for g_win32_package_parser_enum_packages() */
#include "gwin32packageparser.h"

static void
read_handler_icon (GWin32RegistryKey  *key,
                   xicon_t             **icon_out)
{
  GWin32RegistryKey *icon_key;
  GWin32RegistryValueType default_type;
  xchar_t *default_value;

  xassert (icon_out);

  *icon_out = NULL;

  icon_key = g_win32_registry_key_get_child_w (key, L"DefaultIcon", NULL);

  if (icon_key == NULL)
    return;

  if (g_win32_registry_key_get_value (icon_key,
                                      NULL,
                                      TRUE,
                                      "",
                                      &default_type,
                                      (xpointer_t *) &default_value,
                                      NULL,
                                      NULL))
    {
      /* TODO: For UWP handlers this string is usually in @{...} form,
       * see grab_registry_string() below. Right now this
       * string is read as-is and the icon would silently fail to load.
       * Also, right now handler icon is not used anywhere
       * (only app icon is used).
       */
      if (default_type == G_WIN32_REGISTRY_VALUE_STR &&
          default_value[0] != '\0')
        *icon_out = g_themed_icon_new (default_value);

      g_clear_pointer (&default_value, g_free);
    }

  xobject_unref (icon_key);
}

static void
reg_verb_free (xpointer_t p)
{
  if (p == NULL)
    return;

  g_free (((reg_verb *) p)->name);
  g_free (((reg_verb *) p)->shellpath);
  g_free (p);
}

#define is_open(x) ( \
  ((x)[0] == L'o' || (x)[0] == L'O') && \
  ((x)[1] == L'p' || (x)[1] == L'P') && \
  ((x)[2] == L'e' || (x)[2] == L'E') && \
  ((x)[3] == L'n' || (x)[3] == L'N') && \
  ((x)[4] == L'\0') \
)

/* default verb (if any) comes first,
 * then "open", then the rest of the verbs
 * are sorted alphabetically
 */
static xint_t
compare_verbs (xconstpointer a,
               xconstpointer b,
               xpointer_t user_data)
{
  const reg_verb *ca = (const reg_verb *) a;
  const reg_verb *cb = (const reg_verb *) b;
  const xunichar2_t *def = (const xunichar2_t *) user_data;
  xboolean_t is_open_ca;
  xboolean_t is_open_cb;

  if (def != NULL)
    {
      if (_wcsicmp (ca->name, def) == 0)
        return -1;
      else if (_wcsicmp (cb->name, def) == 0)
        return 1;
    }

  is_open_ca = is_open (ca->name);
  is_open_cb = is_open (cb->name);

  if (is_open_ca && !is_open_cb)
    return -1;
  else if (is_open_ca && !is_open_cb)
    return 1;

  return _wcsicmp (ca->name, cb->name);
}

static xboolean_t build_registry_path (xunichar2_t *output, xsize_t output_size, ...) G_GNUC_NULL_TERMINATED;
static xboolean_t build_registry_pathv (xunichar2_t *output, xsize_t output_size, va_list components);

static GWin32RegistryKey *_g_win32_registry_key_build_and_new_w (xerror_t **error, ...) G_GNUC_NULL_TERMINATED;

/* Called by process_verbs_commands.
 * @verb is a verb name
 * @command_line is the commandline of that verb
 * @command_line_utf8 is the UTF-8 version of @command_line
 * @verb_displayname is the prettier display name of the verb (might be NULL)
 * @verb_is_preferred is TRUE if the verb is the preferred one
 * @invent_new_verb_name is TRUE when the verb should be added
 *                       even if a verb with such
 *                       name already exists (in which case
 *                       a new name is invented), unless
 *                       the existing verb runs exactly the same
 *                       commandline.
 */
typedef void (*verb_command_func) (xpointer_t         handler_data1,
                                   xpointer_t         handler_data2,
                                   const xunichar2_t *verb,
                                   const xunichar2_t *command_line,
                                   const xchar_t     *command_line_utf8,
                                   const xchar_t     *verb_displayname,
                                   xboolean_t         verb_is_preferred,
                                   xboolean_t         invent_new_verb_name);

static xunichar2_t *                 decide_which_id_to_use (const xunichar2_t    *program_id,
                                                           GWin32RegistryKey **return_key,
                                                           xchar_t             **return_handler_id_u8,
                                                           xchar_t             **return_handler_id_u8_folded,
                                                           xunichar2_t         **return_uwp_aumid);

static GWin32AppInfoURLSchema *    get_schema_object      (const xunichar2_t *schema,
                                                           const xchar_t     *schema_u8,
                                                           const xchar_t     *schema_u8_folded);

static GWin32AppInfoHandler *      get_handler_object     (const xchar_t       *handler_id_u8_folded,
                                                           GWin32RegistryKey *handler_key,
                                                           const xunichar2_t   *handler_id,
                                                           const xunichar2_t   *uwp_aumid);

static GWin32AppInfoFileExtension *get_ext_object         (const xunichar2_t *ext,
                                                           const xchar_t     *ext_u8,
                                                           const xchar_t     *ext_u8_folded);


static void                        process_verbs_commands (xlist_t             *verbs,
                                                           const reg_verb    *preferred_verb,
                                                           const xunichar2_t   *path_to_progid,
                                                           const xunichar2_t   *progid,
                                                           xboolean_t           autoprefer_first_verb,
                                                           verb_command_func  handler,
                                                           xpointer_t           handler_data1,
                                                           xpointer_t           handler_data2);

static void                        handler_add_verb       (xpointer_t           handler_data1,
                                                           xpointer_t           handler_data2,
                                                           const xunichar2_t   *verb,
                                                           const xunichar2_t   *command_line,
                                                           const xchar_t       *command_line_utf8,
                                                           const xchar_t       *verb_displayname,
                                                           xboolean_t           verb_is_preferred,
                                                           xboolean_t           invent_new_verb_name);

static void                        process_uwp_verbs      (xlist_t                    *verbs,
                                                           const reg_verb           *preferred_verb,
                                                           const xunichar2_t          *path_to_progid,
                                                           const xunichar2_t          *progid,
                                                           xboolean_t                  autoprefer_first_verb,
                                                           GWin32AppInfoHandler     *handler_rec,
                                                           GWin32AppInfoApplication *app);

static void                        uwp_handler_add_verb   (GWin32AppInfoHandler     *handler_rec,
                                                           GWin32AppInfoApplication *app,
                                                           const xunichar2_t          *verb,
                                                           const xchar_t              *verb_displayname,
                                                           xboolean_t                  verb_is_preferred);

/* output_size is in *bytes*, not gunichar2s! */
static xboolean_t
build_registry_path (xunichar2_t *output, xsize_t output_size, ...)
{
  va_list ap;
  xboolean_t result;

  va_start (ap, output_size);

  result = build_registry_pathv (output, output_size, ap);

  va_end (ap);

  return result;
}

/* output_size is in *bytes*, not gunichar2s! */
static xboolean_t
build_registry_pathv (xunichar2_t *output, xsize_t output_size, va_list components)
{
  va_list lentest;
  xunichar2_t *p;
  xunichar2_t *component;
  xsize_t length;

  if (output == NULL)
    return FALSE;

  G_VA_COPY (lentest, components);

  for (length = 0, component = va_arg (lentest, xunichar2_t *);
       component != NULL;
       component = va_arg (lentest, xunichar2_t *))
    {
      length += wcslen (component);
    }

  va_end (lentest);

  if ((length >= REG_PATH_MAX_SIZE) ||
      (length * sizeof (xunichar2_t) >= output_size))
    return FALSE;

  output[0] = L'\0';

  for (p = output, component = va_arg (components, xunichar2_t *);
       component != NULL;
       component = va_arg (components, xunichar2_t *))
    {
      length = wcslen (component);
      wcscat (p, component);
      p += length;
    }

  return TRUE;
}


static GWin32RegistryKey *
_g_win32_registry_key_build_and_new_w (xerror_t **error, ...)
{
  va_list ap;
  xunichar2_t key_path[REG_PATH_MAX_SIZE + 1];
  GWin32RegistryKey *key;

  va_start (ap, error);

  key = NULL;

  if (build_registry_pathv (key_path, sizeof (key_path), ap))
    key = g_win32_registry_key_new_w (key_path, error);

  va_end (ap);

  return key;
}

/* Gets the list of shell verbs (a xlist_t of reg_verb, put into @verbs)
 * from the @program_id_key.
 * If one of the verbs should be preferred,
 * a pointer to this verb (in the xlist_t) will be
 * put into @preferred_verb.
 * Does not automatically assume that the first verb
 * is preferred (when no other preferences exist).
 * @verbname_prefix is prefixed to the name of the verb
 * (this is used for subcommands) and is initially an
 * empty string.
 * @verbshell_prefix is the subkey of @program_id_key
 * that contains the verbs. It is "Shell" initially,
 * but grows with recursive invocations (for subcommands).
 * @is_uwp points to a boolean, which
 * indicates whether the function is being called for a UWP app.
 * It might be switched from %TRUE to %FALSE on return,
 * if the application turns out to not to be UWP on closer inspection.
 * If the application is already known not to be UWP before the
 * call, this pointer can be %NULL instead.
 * Returns TRUE on success, FALSE on failure.
 */
static xboolean_t
get_verbs (GWin32RegistryKey  *program_id_key,
           const reg_verb    **preferred_verb,
           xlist_t             **verbs,
           const xunichar2_t    *verbname_prefix,
           const xunichar2_t    *verbshell_prefix,
           xboolean_t           *is_uwp)
{
  GWin32RegistrySubkeyIter iter;
  GWin32RegistryKey *key;
  GWin32RegistryValueType val_type;
  xunichar2_t *default_verb;
  xsize_t verbshell_prefix_len;
  xsize_t verbname_prefix_len;
  xlist_t *i;

  xassert (program_id_key && verbs && preferred_verb);

  *verbs = NULL;
  *preferred_verb = NULL;

  key = g_win32_registry_key_get_child_w (program_id_key,
                                          verbshell_prefix,
                                          NULL);

  if (key == NULL)
    return FALSE;

  if (!g_win32_registry_subkey_iter_init (&iter, key, NULL))
    {
      xobject_unref (key);

      return FALSE;
    }

  verbshell_prefix_len = xutf16_len (verbshell_prefix);
  verbname_prefix_len = xutf16_len (verbname_prefix);

  while (g_win32_registry_subkey_iter_next (&iter, TRUE, NULL))
    {
      const xunichar2_t *name;
      xsize_t name_len;
      GWin32RegistryKey *subkey;
      xboolean_t has_subcommands;
      const reg_verb *tmp;
      GWin32RegistryValueType subc_type;
      reg_verb *rverb;
      const xunichar2_t *shell = L"Shell";
      const xsize_t shell_len = xutf16_len (shell);

      if (!g_win32_registry_subkey_iter_get_name_w (&iter, &name, &name_len, NULL))
        continue;

      subkey = g_win32_registry_key_get_child_w (key,
                                                 name,
                                                 NULL);

      /* We may not have the required access rights to open the child key */
      if (subkey == NULL)
        continue;

      /* The key we're looking at is "<some_root>/Shell/<this_key>",
       * where "Shell" is verbshell_prefix.
       * If it has a value named 'Subcommands' (doesn't matter what its data is),
       * it means that this key has its own Shell subkey, the subkeys
       * of which are shell commands (i.e. <some_root>/Shell/<this_key>/Shell/<some_other_keys>).
       * To handle that, create new, extended nameprefix and shellprefix,
       * and call the function recursively.
       * name prefix "" -> "<this_key_name>\\"
       * shell prefix "Shell" -> "Shell\\<this_key_name>\\Shell"
       * The root, program_id_key, remains the same in all invocations.
       * Essentially, we're flattening the command tree into a list.
       */
      has_subcommands = FALSE;
      if ((is_uwp == NULL || !(*is_uwp)) && /* Assume UWP apps don't have subcommands */
          g_win32_registry_key_get_value_w (subkey,
                                            NULL,
                                            TRUE,
                                            L"Subcommands",
                                            &subc_type,
                                            NULL,
                                            NULL,
                                            NULL) &&
          subc_type == G_WIN32_REGISTRY_VALUE_STR)
        {
          xboolean_t dummy = FALSE;
          xunichar2_t *new_nameprefix = g_new (xunichar2_t, verbname_prefix_len + name_len + 1 + 1);
          xunichar2_t *new_shellprefix = g_new (xunichar2_t, verbshell_prefix_len + 1 + name_len + 1 + shell_len + 1);
          memcpy (&new_shellprefix[0], verbshell_prefix, verbshell_prefix_len * sizeof (xunichar2_t));
          new_shellprefix[verbshell_prefix_len] = L'\\';
          memcpy (&new_shellprefix[verbshell_prefix_len + 1], name, name_len * sizeof (xunichar2_t));
          new_shellprefix[verbshell_prefix_len + 1 + name_len] = L'\\';
          memcpy (&new_shellprefix[verbshell_prefix_len + 1 + name_len + 1], shell, shell_len * sizeof (xunichar2_t));
          new_shellprefix[verbshell_prefix_len + 1 + name_len + 1 + shell_len] = 0;

          memcpy (&new_nameprefix[0], verbname_prefix, verbname_prefix_len * sizeof (xunichar2_t));
          memcpy (&new_nameprefix[verbname_prefix_len], name, (name_len) * sizeof (xunichar2_t));
          new_nameprefix[verbname_prefix_len + name_len] = L'\\';
          new_nameprefix[verbname_prefix_len + name_len + 1] = 0;
          has_subcommands = get_verbs (program_id_key, &tmp, verbs, new_nameprefix, new_shellprefix, &dummy);
          g_free (new_shellprefix);
          g_free (new_nameprefix);
        }

      /* Presence of subcommands means that this key itself is not a command-key */
      if (has_subcommands)
        {
          g_clear_object (&subkey);
          continue;
        }

      if (is_uwp != NULL && *is_uwp &&
          !g_win32_registry_key_get_value_w (subkey,
                                             NULL,
                                             TRUE,
                                             L"ActivatableClassId",
                                             &subc_type,
                                             NULL,
                                             NULL,
                                             NULL))
        {
          /* We expected a UWP app, but it lacks ActivatableClassId
           * on a verb, which means that it does not behave like
           * a UWP app should (msedge being an example - it's UWP,
           * but has its own launchable exe file and a simple ID),
           * so we have to treat it like a normal app.
           */
           *is_uwp = FALSE;
        }

      g_clear_object (&subkey);

      /* We don't look at the command sub-key and its value (the actual command line) here.
       * We save the registry path instead, and use it later in process_verbs_commands().
       * The name of the verb is also saved.
       * verbname_prefix is prefixed to the verb name (it's either an empty string
       * or already ends with a '\\', so no extra separators needed).
       * verbshell_prefix is prefixed to the verb key path (this one needs a separator,
       * because it never has one - all verbshell prefixes end with "Shell", not "Shell\\")
       */
      rverb = g_new0 (reg_verb, 1);
      rverb->name = g_new (xunichar2_t, verbname_prefix_len + name_len + 1);
      memcpy (&rverb->name[0], verbname_prefix, verbname_prefix_len * sizeof (xunichar2_t));
      memcpy (&rverb->name[verbname_prefix_len], name, name_len * sizeof (xunichar2_t));
      rverb->name[verbname_prefix_len + name_len] = 0;
      rverb->shellpath = g_new (xunichar2_t, verbshell_prefix_len + 1 + name_len + 1);
      memcpy (&rverb->shellpath[0], verbshell_prefix, verbshell_prefix_len * sizeof (xunichar2_t));
      memcpy (&rverb->shellpath[verbshell_prefix_len], L"\\", sizeof (xunichar2_t));
      memcpy (&rverb->shellpath[verbshell_prefix_len + 1], name, name_len * sizeof (xunichar2_t));
      rverb->shellpath[verbshell_prefix_len + 1 + name_len] = 0;
      *verbs = xlist_append (*verbs, rverb);
    }

  g_win32_registry_subkey_iter_clear (&iter);

  if (*verbs == NULL)
    {
      xobject_unref (key);

      return FALSE;
    }

  default_verb = NULL;

  if (g_win32_registry_key_get_value_w (key,
                                        NULL,
                                        TRUE,
                                        L"",
                                        &val_type,
                                        (void **) &default_verb,
                                        NULL,
                                        NULL) &&
      (val_type != G_WIN32_REGISTRY_VALUE_STR ||
       xutf16_len (default_verb) <= 0))
    g_clear_pointer (&default_verb, g_free);

  xobject_unref (key);

  /* Only sort at the top level */
  if (verbname_prefix[0] == 0)
    {
      *verbs = xlist_sort_with_data (*verbs, compare_verbs, default_verb);

      for (i = *verbs; default_verb && *preferred_verb == NULL && i; i = i->next)
        if (_wcsicmp (default_verb, ((const reg_verb *) i->data)->name) == 0)
          *preferred_verb = (const reg_verb *) i->data;
    }

  g_clear_pointer (&default_verb, g_free);

  return TRUE;
}

/* Grabs a URL association (from HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\
 * or from an application with Capabilities, or just a schema subkey in HKCR).
 * @program_id is a ProgID of the handler for the URL.
 * @schema is the schema for the URL.
 * @schema_u8 and @schema_u8_folded are UTF-8 and folded UTF-8
 * respectively.
 * @app is the app to which the URL handler belongs (can be NULL).
 * @is_user_choice is TRUE if this association is clearly preferred
 */
static void
get_url_association (const xunichar2_t          *program_id,
                     const xunichar2_t          *schema,
                     const xchar_t              *schema_u8,
                     const xchar_t              *schema_u8_folded,
                     GWin32AppInfoApplication *app,
                     xboolean_t                  is_user_choice)
{
  GWin32AppInfoURLSchema *schema_rec;
  GWin32AppInfoHandler *handler_rec;
  xunichar2_t *handler_id;
  xlist_t *verbs;
  const reg_verb *preferred_verb;
  xchar_t *handler_id_u8;
  xchar_t *handler_id_u8_folded;
  xunichar2_t *uwp_aumid;
  xboolean_t is_uwp;
  GWin32RegistryKey *handler_key;

  if ((handler_id = decide_which_id_to_use (program_id,
                                            &handler_key,
                                            &handler_id_u8,
                                            &handler_id_u8_folded,
                                            &uwp_aumid)) == NULL)
    return;

  is_uwp = uwp_aumid != NULL;

  if (!get_verbs (handler_key, &preferred_verb, &verbs, L"", L"Shell", &is_uwp))
    {
      g_clear_pointer (&handler_id, g_free);
      g_clear_pointer (&handler_id_u8, g_free);
      g_clear_pointer (&handler_id_u8_folded, g_free);
      g_clear_object (&handler_key);
      g_clear_pointer (&uwp_aumid, g_free);

      return;
    }

  if (!is_uwp && uwp_aumid != NULL)
    g_clear_pointer (&uwp_aumid, g_free);

  schema_rec = get_schema_object (schema,
                                  schema_u8,
                                  schema_u8_folded);

  handler_rec = get_handler_object (handler_id_u8_folded,
                                    handler_key,
                                    handler_id,
                                    uwp_aumid);

  if (is_user_choice || schema_rec->chosen_handler == NULL)
    g_set_object (&schema_rec->chosen_handler, handler_rec);

  xhash_table_insert (schema_rec->handlers,
                       xstrdup (handler_id_u8_folded),
                       xobject_ref (handler_rec));

  g_clear_object (&handler_key);

  if (app)
    xhash_table_insert (app->supported_urls,
                         xstrdup (schema_rec->schema_u8_folded),
                         xobject_ref (handler_rec));

  if (uwp_aumid == NULL)
    process_verbs_commands (g_steal_pointer (&verbs),
                            preferred_verb,
                            HKCR,
                            handler_id,
                            TRUE,
                            handler_add_verb,
                            handler_rec,
                            app);
  else
    process_uwp_verbs (g_steal_pointer (&verbs),
                       preferred_verb,
                       HKCR,
                       handler_id,
                       TRUE,
                       handler_rec,
                       app);


  g_clear_pointer (&handler_id_u8, g_free);
  g_clear_pointer (&handler_id_u8_folded, g_free);
  g_clear_pointer (&handler_id, g_free);
  g_clear_pointer (&uwp_aumid, g_free);
}

/* Grabs a file extension association (from HKCR\.ext or similar).
 * @program_id is a ProgID of the handler for the extension.
 * @file_extension is the extension (with the leading '.')
 * @app is the app to which the extension handler belongs (can be NULL).
 * @is_user_choice is TRUE if this is clearly the preferred association
 */
static void
get_file_ext (const xunichar2_t            *program_id,
              const xunichar2_t            *file_extension,
              GWin32AppInfoApplication   *app,
              xboolean_t                    is_user_choice)
{
  GWin32AppInfoHandler *handler_rec;
  xunichar2_t *handler_id;
  const reg_verb *preferred_verb;
  xlist_t *verbs;
  xchar_t *handler_id_u8;
  xchar_t *handler_id_u8_folded;
  xunichar2_t *uwp_aumid;
  xboolean_t is_uwp;
  GWin32RegistryKey *handler_key;
  GWin32AppInfoFileExtension *file_extn;
  xchar_t *file_extension_u8;
  xchar_t *file_extension_u8_folded;

  if ((handler_id = decide_which_id_to_use (program_id,
                                            &handler_key,
                                            &handler_id_u8,
                                            &handler_id_u8_folded,
                                            &uwp_aumid)) == NULL)
    return;

  if (!xutf16_to_utf8_and_fold (file_extension,
                                 -1,
                                 &file_extension_u8,
                                 &file_extension_u8_folded))
    {
      g_clear_pointer (&handler_id, g_free);
      g_clear_pointer (&handler_id_u8, g_free);
      g_clear_pointer (&handler_id_u8_folded, g_free);
      g_clear_pointer (&uwp_aumid, g_free);
      g_clear_object (&handler_key);

      return;
    }

  is_uwp = uwp_aumid != NULL;

  if (!get_verbs (handler_key, &preferred_verb, &verbs, L"", L"Shell", &is_uwp))
    {
      g_clear_pointer (&handler_id, g_free);
      g_clear_pointer (&handler_id_u8, g_free);
      g_clear_pointer (&handler_id_u8_folded, g_free);
      g_clear_object (&handler_key);
      g_clear_pointer (&file_extension_u8, g_free);
      g_clear_pointer (&file_extension_u8_folded, g_free);
      g_clear_pointer (&uwp_aumid, g_free);

      return;
    }

  if (!is_uwp && uwp_aumid != NULL)
    g_clear_pointer (&uwp_aumid, g_free);

  file_extn = get_ext_object (file_extension, file_extension_u8, file_extension_u8_folded);

  handler_rec = get_handler_object (handler_id_u8_folded,
                                    handler_key,
                                    handler_id,
                                    uwp_aumid);

  if (is_user_choice || file_extn->chosen_handler == NULL)
    g_set_object (&file_extn->chosen_handler, handler_rec);

  xhash_table_insert (file_extn->handlers,
                       xstrdup (handler_id_u8_folded),
                       xobject_ref (handler_rec));

  if (app)
    xhash_table_insert (app->supported_exts,
                         xstrdup (file_extension_u8_folded),
                         xobject_ref (handler_rec));

  g_clear_pointer (&file_extension_u8, g_free);
  g_clear_pointer (&file_extension_u8_folded, g_free);
  g_clear_object (&handler_key);

  if (uwp_aumid == NULL)
    process_verbs_commands (g_steal_pointer (&verbs),
                            preferred_verb,
                            HKCR,
                            handler_id,
                            TRUE,
                            handler_add_verb,
                            handler_rec,
                            app);
  else
    process_uwp_verbs (g_steal_pointer (&verbs),
                       preferred_verb,
                       HKCR,
                       handler_id,
                       TRUE,
                       handler_rec,
                       app);

  g_clear_pointer (&handler_id, g_free);
  g_clear_pointer (&handler_id_u8, g_free);
  g_clear_pointer (&handler_id_u8_folded, g_free);
  g_clear_pointer (&uwp_aumid, g_free);
}

/* Returns either a @program_id or the string from
 * the default value of the program_id key (which is a name
 * of a proxy class), or NULL.
 * Does not check that proxy represents a valid
 * record, just checks that it exists.
 * Can return the class key (HKCR/program_id or HKCR/proxy_id).
 * Can convert returned value to UTF-8 and fold it.
 */
static xunichar2_t *
decide_which_id_to_use (const xunichar2_t    *program_id,
                        GWin32RegistryKey **return_key,
                        xchar_t             **return_handler_id_u8,
                        xchar_t             **return_handler_id_u8_folded,
                        xunichar2_t         **return_uwp_aumid)
{
  GWin32RegistryKey *key;
  GWin32RegistryKey *uwp_key;
  GWin32RegistryValueType val_type;
  xunichar2_t *proxy_id;
  xunichar2_t *return_id;
  xunichar2_t *uwp_aumid;
  xboolean_t got_value;
  xchar_t *handler_id_u8;
  xchar_t *handler_id_u8_folded;
  xassert (program_id);

  if (return_key)
    *return_key = NULL;

  if (return_uwp_aumid)
    *return_uwp_aumid = NULL;

  key = g_win32_registry_key_get_child_w (classes_root_key, program_id, NULL);

  if (key == NULL)
    return NULL;

  /* Check for UWP first */
  uwp_aumid = NULL;
  uwp_key = g_win32_registry_key_get_child_w (key, L"Application", NULL);

  if (uwp_key != NULL)
    {
      got_value = g_win32_registry_key_get_value_w (uwp_key,
                                                    NULL,
                                                    TRUE,
                                                    L"AppUserModelID",
                                                    &val_type,
                                                    (void **) &uwp_aumid,
                                                    NULL,
                                                    NULL);
      if (got_value && val_type != G_WIN32_REGISTRY_VALUE_STR)
        g_clear_pointer (&uwp_aumid, g_free);

      /* Other values in the Application key contain useful information
       * (description, name, icon), but it's inconvenient to read
       * it here (we don't have an app object *yet*). Store the key
       * in a table instead, and look at it later.
       */
      if (uwp_aumid == NULL)
        g_debug ("ProgramID %S looks like a UWP application, but isn't",
                 program_id);
      else
        xhash_table_insert (uwp_handler_table, xobject_ref (uwp_key), g_wcsdup (uwp_aumid, -1));

      xobject_unref (uwp_key);
    }

  /* Then check for proxy */
  proxy_id = NULL;

  if (uwp_aumid == NULL)
    {
      got_value = g_win32_registry_key_get_value_w (key,
                                                    NULL,
                                                    TRUE,
                                                    L"",
                                                    &val_type,
                                                    (void **) &proxy_id,
                                                    NULL,
                                                    NULL);
      if (got_value && val_type != G_WIN32_REGISTRY_VALUE_STR)
        g_clear_pointer (&proxy_id, g_free);
    }

  return_id = NULL;

  if (proxy_id)
    {
      GWin32RegistryKey *proxy_key;
      proxy_key = g_win32_registry_key_get_child_w (classes_root_key, proxy_id, NULL);

      if (proxy_key)
        {
          if (return_key)
            *return_key = g_steal_pointer (&proxy_key);
          g_clear_object (&proxy_key);

          return_id = g_steal_pointer (&proxy_id);
        }

      g_clear_pointer (&proxy_id, g_free);
    }

  if ((return_handler_id_u8 ||
       return_handler_id_u8_folded) &&
      !xutf16_to_utf8_and_fold (return_id == NULL ? program_id : return_id,
                                 -1,
                                 &handler_id_u8,
                                 &handler_id_u8_folded))
    {
      g_clear_object (&key);
      if (return_key)
        g_clear_object (return_key);
      g_clear_pointer (&return_id, g_free);

      return NULL;
    }

  if (return_handler_id_u8)
    *return_handler_id_u8 = g_steal_pointer (&handler_id_u8);
  g_clear_pointer (&handler_id_u8, g_free);
  if (return_handler_id_u8_folded)
    *return_handler_id_u8_folded = g_steal_pointer (&handler_id_u8_folded);
  g_clear_pointer (&handler_id_u8_folded, g_free);
  if (return_uwp_aumid)
    *return_uwp_aumid = g_steal_pointer (&uwp_aumid);
  g_clear_pointer (&uwp_aumid, g_free);

  if (return_id == NULL && return_key)
    *return_key = g_steal_pointer (&key);
  g_clear_object (&key);

  if (return_id == NULL)
    return g_wcsdup (program_id, -1);

  return return_id;
}

/* Grabs the command for each verb from @verbs,
 * and invokes @handler for it. Consumes @verbs.
 * @path_to_progid and @progid are concatenated to
 * produce a path to the key where Shell/verb/command
 * subkeys are looked up.
 * @preferred_verb, if not NULL, will be used to inform
 * the @handler that a verb is preferred.
 * @autoprefer_first_verb will automatically make the first
 * verb to be preferred, if @preferred_verb is NULL.
 * @handler_data1 and @handler_data2 are passed to @handler as-is.
 */
static void
process_verbs_commands (xlist_t             *verbs,
                        const reg_verb    *preferred_verb,
                        const xunichar2_t   *path_to_progid,
                        const xunichar2_t   *progid,
                        xboolean_t           autoprefer_first_verb,
                        verb_command_func  handler,
                        xpointer_t           handler_data1,
                        xpointer_t           handler_data2)
{
  xlist_t *i;
  xboolean_t got_value;

  xassert (handler != NULL);
  xassert (verbs != NULL);
  xassert (progid != NULL);

  for (i = verbs; i; i = i->next)
    {
      const reg_verb *verb = (const reg_verb *) i->data;
      GWin32RegistryKey *key;
      GWin32RegistryKey *verb_key;
      xunichar2_t *command_value;
      xchar_t *command_value_utf8;
      GWin32RegistryValueType val_type;
      xunichar2_t *verb_displayname;
      xchar_t *verb_displayname_u8;

      key = _g_win32_registry_key_build_and_new_w (NULL, path_to_progid, progid,
                                                   L"\\", verb->shellpath, L"\\command", NULL);

      if (key == NULL)
        {
          g_debug ("%S%S\\shell\\%S does not have a \"command\" subkey",
                   path_to_progid, progid, verb->shellpath);
          continue;
        }

      command_value = NULL;
      got_value = g_win32_registry_key_get_value_w (key,
                                                    NULL,
                                                    TRUE,
                                                    L"",
                                                    &val_type,
                                                    (void **) &command_value,
                                                    NULL,
                                                    NULL);
      g_clear_object (&key);

      if (!got_value ||
          val_type != G_WIN32_REGISTRY_VALUE_STR ||
          (command_value_utf8 = xutf16_to_utf8 (command_value,
                                                 -1,
                                                 NULL,
                                                 NULL,
                                                 NULL)) == NULL)
        {
          g_clear_pointer (&command_value, g_free);
          continue;
        }

      verb_displayname = NULL;
      verb_displayname_u8 = NULL;
      verb_key = _g_win32_registry_key_build_and_new_w (NULL, path_to_progid, progid,
                                                        L"\\", verb->shellpath, NULL);

      if (verb_key)
        {
          xsize_t verb_displayname_len;

          got_value = g_win32_registry_key_get_value_w (verb_key,
                                                        g_win32_registry_get_os_dirs_w (),
                                                        TRUE,
                                                        L"MUIVerb",
                                                        &val_type,
                                                        (void **) &verb_displayname,
                                                        &verb_displayname_len,
                                                        NULL);

          if (got_value &&
              val_type == G_WIN32_REGISTRY_VALUE_STR &&
              verb_displayname_len > sizeof (xunichar2_t))
            verb_displayname_u8 = xutf16_to_utf8 (verb_displayname, -1, NULL, NULL, NULL);

          g_clear_pointer (&verb_displayname, g_free);

          if (verb_displayname_u8 == NULL)
            {
              got_value = g_win32_registry_key_get_value_w (verb_key,
                                                            NULL,
                                                            TRUE,
                                                            L"",
                                                            &val_type,
                                                            (void **) &verb_displayname,
                                                            &verb_displayname_len,
                                                            NULL);

              if (got_value &&
                  val_type == G_WIN32_REGISTRY_VALUE_STR &&
                  verb_displayname_len > sizeof (xunichar2_t))
                verb_displayname_u8 = xutf16_to_utf8 (verb_displayname, -1, NULL, NULL, NULL);
            }

          g_clear_pointer (&verb_displayname, g_free);
          g_clear_object (&verb_key);
        }

      handler (handler_data1, handler_data2, verb->name, command_value, command_value_utf8,
               verb_displayname_u8,
               (preferred_verb && _wcsicmp (verb->name, preferred_verb->name) == 0) ||
               (!preferred_verb && autoprefer_first_verb && i == verbs),
               FALSE);

      g_clear_pointer (&command_value, g_free);
      g_clear_pointer (&command_value_utf8, g_free);
      g_clear_pointer (&verb_displayname_u8, g_free);
    }

  xlist_free_full (verbs, reg_verb_free);
}

static void
process_uwp_verbs (xlist_t                    *verbs,
                   const reg_verb           *preferred_verb,
                   const xunichar2_t          *path_to_progid,
                   const xunichar2_t          *progid,
                   xboolean_t                  autoprefer_first_verb,
                   GWin32AppInfoHandler     *handler_rec,
                   GWin32AppInfoApplication *app)
{
  xlist_t *i;

  xassert (verbs != NULL);

  for (i = verbs; i; i = i->next)
    {
      const reg_verb *verb = (const reg_verb *) i->data;
      GWin32RegistryKey *key;
      xboolean_t got_value;
      GWin32RegistryValueType val_type;
      xunichar2_t *acid;
      xsize_t acid_len;

      key = _g_win32_registry_key_build_and_new_w (NULL, path_to_progid, progid,
                                                   L"\\", verb->shellpath, NULL);

      if (key == NULL)
        {
          g_debug ("%S%S\\%S does not exist",
                   path_to_progid, progid, verb->shellpath);
          continue;
        }

      acid = NULL;
      got_value = g_win32_registry_key_get_value_w (key,
                                                    g_win32_registry_get_os_dirs_w (),
                                                    TRUE,
                                                    L"ActivatableClassId",
                                                    &val_type,
                                                    (void **) &acid,
                                                    &acid_len,
                                                    NULL);

      if (got_value &&
          val_type == G_WIN32_REGISTRY_VALUE_STR &&
          acid_len > sizeof (xunichar2_t))
        {
          /* TODO: default value of a shell subkey, if not empty,
           * migh contain something like @{Some.Identifier_1234.456.678.789_some_words?ms-resource://Arbitrary.Path/Pointing/Somewhere}
           * and it might be possible to turn it into a nice displayname.
           */
          uwp_handler_add_verb (handler_rec,
                                app,
                                verb->name,
                                NULL,
                                (preferred_verb && _wcsicmp (verb->name, preferred_verb->name) == 0) ||
                                (!preferred_verb && autoprefer_first_verb && i == verbs));
        }
      else
        {
          g_debug ("%S%S\\%S does not have an ActivatableClassId string value",
                   path_to_progid, progid, verb->shellpath);
        }

      g_clear_pointer (&acid, g_free);
      g_clear_object (&key);
    }

  xlist_free_full (verbs, reg_verb_free);
}

/* Looks up a schema object identified by
 * @schema_u8_folded in the urls hash table.
 * If such object doesn't exist,
 * creates it and puts it into the urls hash table.
 * Returns the object.
 */
static GWin32AppInfoURLSchema *
get_schema_object (const xunichar2_t *schema,
                   const xchar_t     *schema_u8,
                   const xchar_t     *schema_u8_folded)
{
  GWin32AppInfoURLSchema *schema_rec;

  schema_rec = xhash_table_lookup (urls, schema_u8_folded);

  if (schema_rec != NULL)
    return schema_rec;

  schema_rec = xobject_new (XTYPE_WIN32_APPINFO_URL_SCHEMA, NULL);
  schema_rec->schema = g_wcsdup (schema, -1);
  schema_rec->schema_u8 = xstrdup (schema_u8);
  schema_rec->schema_u8_folded = xstrdup (schema_u8_folded);
  xhash_table_insert (urls, xstrdup (schema_rec->schema_u8_folded), schema_rec);

  return schema_rec;
}

/* Looks up a handler object identified by
 * @handler_id_u8_folded in the handlers hash table.
 * If such object doesn't exist,
 * creates it and puts it into the handlers hash table.
 * Returns the object.
 */
static GWin32AppInfoHandler *
get_handler_object (const xchar_t       *handler_id_u8_folded,
                    GWin32RegistryKey *handler_key,
                    const xunichar2_t   *handler_id,
                    const xunichar2_t   *uwp_aumid)
{
  GWin32AppInfoHandler *handler_rec;

  handler_rec = xhash_table_lookup (handlers, handler_id_u8_folded);

  if (handler_rec != NULL)
    return handler_rec;

  handler_rec = xobject_new (XTYPE_WIN32_APPINFO_HANDLER, NULL);
  if (handler_key)
    handler_rec->key = xobject_ref (handler_key);
  handler_rec->handler_id = g_wcsdup (handler_id, -1);
  handler_rec->handler_id_folded = xstrdup (handler_id_u8_folded);
  if (uwp_aumid)
    handler_rec->uwp_aumid = g_wcsdup (uwp_aumid, -1);
  if (handler_key)
    read_handler_icon (handler_key, &handler_rec->icon);
  xhash_table_insert (handlers, xstrdup (handler_id_u8_folded), handler_rec);

  return handler_rec;
}

static void
handler_add_verb (xpointer_t           handler_data1,
                  xpointer_t           handler_data2,
                  const xunichar2_t   *verb,
                  const xunichar2_t   *command_line,
                  const xchar_t       *command_line_utf8,
                  const xchar_t       *verb_displayname,
                  xboolean_t           verb_is_preferred,
                  xboolean_t           invent_new_verb_name)
{
  GWin32AppInfoHandler *handler_rec = (GWin32AppInfoHandler *) handler_data1;
  GWin32AppInfoApplication *app_rec = (GWin32AppInfoApplication *) handler_data2;
  GWin32AppInfoShellVerb *shverb;

  _verb_lookup (handler_rec->verbs, verb, &shverb);

  if (shverb != NULL)
    return;

  shverb = xobject_new (XTYPE_WIN32_APPINFO_SHELL_VERB, NULL);
  shverb->verb_name = g_wcsdup (verb, -1);
  shverb->verb_displayname = xstrdup (verb_displayname);
  shverb->command = g_wcsdup (command_line, -1);
  shverb->command_utf8 = xstrdup (command_line_utf8);
  shverb->is_uwp = FALSE; /* This function is for non-UWP verbs only */
  if (app_rec)
    shverb->app = xobject_ref (app_rec);

  _g_win32_extract_executable (shverb->command,
                               &shverb->executable,
                               &shverb->executable_basename,
                               &shverb->executable_folded,
                               NULL,
                               &shverb->dll_function);

  if (shverb->dll_function != NULL)
    _g_win32_fixup_broken_microsoft_rundll_commandline (shverb->command);

  if (!verb_is_preferred)
    xptr_array_add (handler_rec->verbs, shverb);
  else
    xptr_array_insert (handler_rec->verbs, 0, shverb);
}

/* Tries to generate a new name for a verb that looks
 * like "verb (%x)", where %x is an integer in range of [0;255).
 * On success puts new verb (and new verb displayname) into
 * @new_verb and @new_displayname and return TRUE.
 * On failure puts NULL into both and returns FALSE.
 */
static xboolean_t
generate_new_verb_name (xptr_array_t        *verbs,
                        const xunichar2_t  *verb,
                        const xchar_t      *verb_displayname,
                        xunichar2_t       **new_verb,
                        xchar_t           **new_displayname)
{
  xsize_t counter;
  GWin32AppInfoShellVerb *shverb;
  xsize_t orig_len = xutf16_len (verb);
  xsize_t new_verb_name_len = orig_len + strlen (" ()") + 2 + 1;
  xunichar2_t *new_verb_name = g_new (xunichar2_t, new_verb_name_len);

  *new_verb = NULL;
  *new_displayname = NULL;

  memcpy (new_verb_name, verb, orig_len * sizeof (xunichar2_t));
  for (counter = 0; counter < 255; counter++)
  {
    _snwprintf (&new_verb_name[orig_len], new_verb_name_len, L" (%zx)", counter);
    _verb_lookup (verbs, new_verb_name, &shverb);

    if (shverb == NULL)
      {
        *new_verb = new_verb_name;
        if (verb_displayname != NULL)
          *new_displayname = xstrdup_printf ("%s (%zx)", verb_displayname, counter);

        return TRUE;
      }
  }

  return FALSE;
}

static void
app_add_verb (xpointer_t           handler_data1,
              xpointer_t           handler_data2,
              const xunichar2_t   *verb,
              const xunichar2_t   *command_line,
              const xchar_t       *command_line_utf8,
              const xchar_t       *verb_displayname,
              xboolean_t           verb_is_preferred,
              xboolean_t           invent_new_verb_name)
{
  xunichar2_t *new_verb = NULL;
  xchar_t *new_displayname = NULL;
  GWin32AppInfoApplication *app_rec = (GWin32AppInfoApplication *) handler_data2;
  GWin32AppInfoShellVerb *shverb;

  _verb_lookup (app_rec->verbs, verb, &shverb);

  /* Special logic for fake apps - do our best to
   * collate all possible verbs in the app,
   * including the verbs that have the same name but
   * different commandlines, in which case a new
   * verb name has to be invented.
   */
  if (shverb != NULL)
    {
      xsize_t vi;

      if (!invent_new_verb_name)
        return;

      for (vi = 0; vi < app_rec->verbs->len; vi++)
        {
          GWin32AppInfoShellVerb *app_verb;

          app_verb = _verb_idx (app_rec->verbs, vi);

          if (_wcsicmp (command_line, app_verb->command) == 0)
            break;
        }

      if (vi < app_rec->verbs->len ||
          !generate_new_verb_name (app_rec->verbs,
                                   verb,
                                   verb_displayname,
                                   &new_verb,
                                   &new_displayname))
        return;
    }

  shverb = xobject_new (XTYPE_WIN32_APPINFO_SHELL_VERB, NULL);
  if (new_verb == NULL)
    shverb->verb_name = g_wcsdup (verb, -1);
  else
    shverb->verb_name = g_steal_pointer (&new_verb);
  if (new_displayname == NULL)
    shverb->verb_displayname = xstrdup (verb_displayname);
  else
    shverb->verb_displayname = g_steal_pointer (&new_displayname);

  shverb->command = g_wcsdup (command_line, -1);
  shverb->command_utf8 = xstrdup (command_line_utf8);
  shverb->app = xobject_ref (app_rec);

  _g_win32_extract_executable (shverb->command,
                               &shverb->executable,
                               &shverb->executable_basename,
                               &shverb->executable_folded,
                               NULL,
                               &shverb->dll_function);

  if (shverb->dll_function != NULL)
    _g_win32_fixup_broken_microsoft_rundll_commandline (shverb->command);

  if (!verb_is_preferred)
    xptr_array_add (app_rec->verbs, shverb);
  else
    xptr_array_insert (app_rec->verbs, 0, shverb);
}

static void
uwp_app_add_verb (GWin32AppInfoApplication *app_rec,
                  const xunichar2_t          *verb,
                  const xchar_t              *verb_displayname)
{
  GWin32AppInfoShellVerb *shverb;

  _verb_lookup (app_rec->verbs, verb, &shverb);

  if (shverb != NULL)
    return;

  shverb = xobject_new (XTYPE_WIN32_APPINFO_SHELL_VERB, NULL);
  shverb->verb_name = g_wcsdup (verb, -1);
  shverb->app = xobject_ref (app_rec);
  shverb->verb_displayname = xstrdup (verb_displayname);

  shverb->is_uwp = TRUE;

  /* Strictly speaking, this is unnecessary, but
   * let's make it clear that UWP verbs have no
   * commands and executables.
   */
  shverb->command = NULL;
  shverb->command_utf8 = NULL;
  shverb->executable = NULL;
  shverb->executable_basename = NULL;
  shverb->executable_folded = NULL;
  shverb->dll_function = NULL;

  xptr_array_add (app_rec->verbs, shverb);
}

static void
uwp_handler_add_verb (GWin32AppInfoHandler     *handler_rec,
                      GWin32AppInfoApplication *app,
                      const xunichar2_t          *verb,
                      const xchar_t              *verb_displayname,
                      xboolean_t                  verb_is_preferred)
{
  GWin32AppInfoShellVerb *shverb;

  _verb_lookup (handler_rec->verbs, verb, &shverb);

  if (shverb != NULL)
    return;

  shverb = xobject_new (XTYPE_WIN32_APPINFO_SHELL_VERB, NULL);
  shverb->verb_name = g_wcsdup (verb, -1);
  shverb->verb_displayname = xstrdup (verb_displayname);

  shverb->is_uwp = TRUE;

  if (app)
    shverb->app = xobject_ref (app);

  shverb->command = NULL;
  shverb->command_utf8 = NULL;
  shverb->executable = NULL;
  shverb->executable_basename = NULL;
  shverb->executable_folded = NULL;
  shverb->dll_function = NULL;

  if (!verb_is_preferred)
    xptr_array_add (handler_rec->verbs, shverb);
  else
    xptr_array_insert (handler_rec->verbs, 0, shverb);
}

/* Looks up a file extension object identified by
 * @ext_u8_folded in the extensions hash table.
 * If such object doesn't exist,
 * creates it and puts it into the extensions hash table.
 * Returns the object.
 */
static GWin32AppInfoFileExtension *
get_ext_object (const xunichar2_t *ext,
                const xchar_t     *ext_u8,
                const xchar_t     *ext_u8_folded)
{
  GWin32AppInfoFileExtension *file_extn;

  if (xhash_table_lookup_extended (extensions,
                                    ext_u8_folded,
                                    NULL,
                                    (void **) &file_extn))
    return file_extn;

  file_extn = xobject_new (XTYPE_WIN32_APPINFO_FILE_EXTENSION, NULL);
  file_extn->extension = g_wcsdup (ext, -1);
  file_extn->extension_u8 = xstrdup (ext_u8);
  xhash_table_insert (extensions, xstrdup (ext_u8_folded), file_extn);

  return file_extn;
}

/* Iterates over HKCU\\Software\\Clients or HKLM\\Software\\Clients,
 * (depending on @user_registry being TRUE or FALSE),
 * collecting applications listed there.
 * Puts the path to the client key for each client into @priority_capable_apps
 * (only for clients with file or URL associations).
 */
static void
collect_capable_apps_from_clients (xptr_array_t *capable_apps,
                                   xptr_array_t *priority_capable_apps,
                                   xboolean_t   user_registry)
{
  GWin32RegistryKey *clients;
  GWin32RegistrySubkeyIter clients_iter;

  const xunichar2_t *client_type_name;
  xsize_t client_type_name_len;


  if (user_registry)
    clients =
        g_win32_registry_key_new_w (L"HKEY_CURRENT_USER\\Software\\Clients",
                                     NULL);
  else
    clients =
        g_win32_registry_key_new_w (L"HKEY_LOCAL_MACHINE\\Software\\Clients",
                                     NULL);

  if (clients == NULL)
    return;

  if (!g_win32_registry_subkey_iter_init (&clients_iter, clients, NULL))
    {
      xobject_unref (clients);
      return;
    }

  while (g_win32_registry_subkey_iter_next (&clients_iter, TRUE, NULL))
    {
      GWin32RegistrySubkeyIter subkey_iter;
      GWin32RegistryKey *system_client_type;
      GWin32RegistryValueType default_type;
      xunichar2_t *default_value = NULL;
      const xunichar2_t *client_name;
      xsize_t client_name_len;

      if (!g_win32_registry_subkey_iter_get_name_w (&clients_iter,
                                                    &client_type_name,
                                                    &client_type_name_len,
                                                    NULL))
        continue;

      system_client_type = g_win32_registry_key_get_child_w (clients,
                                                             client_type_name,
                                                             NULL);

      if (system_client_type == NULL)
        continue;

      if (g_win32_registry_key_get_value_w (system_client_type,
                                            NULL,
                                            TRUE,
                                            L"",
                                            &default_type,
                                            (xpointer_t *) &default_value,
                                            NULL,
                                            NULL))
        {
          if (default_type != G_WIN32_REGISTRY_VALUE_STR ||
              default_value[0] == L'\0')
            g_clear_pointer (&default_value, g_free);
        }

      if (!g_win32_registry_subkey_iter_init (&subkey_iter,
                                              system_client_type,
                                              NULL))
        {
          g_clear_pointer (&default_value, g_free);
          xobject_unref (system_client_type);
          continue;
        }

      while (g_win32_registry_subkey_iter_next (&subkey_iter, TRUE, NULL))
        {
          GWin32RegistryKey *system_client;
          GWin32RegistryKey *system_client_assoc;
          xboolean_t add;
          xunichar2_t *keyname;

          if (!g_win32_registry_subkey_iter_get_name_w (&subkey_iter,
                                                        &client_name,
                                                        &client_name_len,
                                                        NULL))
            continue;

          system_client = g_win32_registry_key_get_child_w (system_client_type,
                                                            client_name,
                                                            NULL);

          if (system_client == NULL)
            continue;

          add = FALSE;

          system_client_assoc = g_win32_registry_key_get_child_w (system_client,
                                                                  L"Capabilities\\FileAssociations",
                                                                  NULL);

          if (system_client_assoc != NULL)
            {
              add = TRUE;
              xobject_unref (system_client_assoc);
            }
          else
            {
              system_client_assoc = g_win32_registry_key_get_child_w (system_client,
                                                                      L"Capabilities\\UrlAssociations",
                                                                      NULL);

              if (system_client_assoc != NULL)
                {
                  add = TRUE;
                  xobject_unref (system_client_assoc);
                }
            }

          if (add)
            {
              keyname = g_wcsdup (g_win32_registry_key_get_path_w (system_client), -1);

              if (default_value && wcscmp (default_value, client_name) == 0)
                xptr_array_add (priority_capable_apps, keyname);
              else
                xptr_array_add (capable_apps, keyname);
            }

          xobject_unref (system_client);
        }

      g_win32_registry_subkey_iter_clear (&subkey_iter);
      g_clear_pointer (&default_value, g_free);
      xobject_unref (system_client_type);
    }

  g_win32_registry_subkey_iter_clear (&clients_iter);
  xobject_unref (clients);
}

/* Iterates over HKCU\\Software\\RegisteredApplications or HKLM\\Software\\RegisteredApplications,
 * (depending on @user_registry being TRUE or FALSE),
 * collecting applications listed there.
 * Puts the path to the app key for each app into @capable_apps.
 */
static void
collect_capable_apps_from_registered_apps (xptr_array_t *capable_apps,
                                           xboolean_t   user_registry)
{
  GWin32RegistryValueIter iter;
  const xunichar2_t *reg_path;

  xunichar2_t *value_data;
  xsize_t      value_data_size;
  GWin32RegistryValueType value_type;
  GWin32RegistryKey *registered_apps;

  if (user_registry)
    reg_path = L"HKEY_CURRENT_USER\\Software\\RegisteredApplications";
  else
    reg_path = L"HKEY_LOCAL_MACHINE\\Software\\RegisteredApplications";

  registered_apps =
      g_win32_registry_key_new_w (reg_path, NULL);

  if (!registered_apps)
    return;

  if (!g_win32_registry_value_iter_init (&iter, registered_apps, NULL))
    {
      xobject_unref (registered_apps);

      return;
    }

  while (g_win32_registry_value_iter_next (&iter, TRUE, NULL))
    {
      xunichar2_t possible_location[REG_PATH_MAX_SIZE + 1];
      GWin32RegistryKey *location;
      xunichar2_t *p;

      if ((!g_win32_registry_value_iter_get_value_type (&iter,
                                                        &value_type,
                                                        NULL)) ||
          (value_type != G_WIN32_REGISTRY_VALUE_STR) ||
          (!g_win32_registry_value_iter_get_data_w (&iter, TRUE,
                                                    (void **) &value_data,
                                                    &value_data_size,
                                                    NULL)) ||
          (value_data_size < sizeof (xunichar2_t)) ||
          (value_data[0] == L'\0'))
        continue;

      if (!build_registry_path (possible_location, sizeof (possible_location),
                                user_registry ? HKCU : HKLM, value_data, NULL))
        continue;

      location = g_win32_registry_key_new_w (possible_location, NULL);

      if (location == NULL)
        continue;

      p = wcsrchr (possible_location, L'\\');

      if (p)
        {
          *p = L'\0';
          xptr_array_add (capable_apps, g_wcsdup (possible_location, -1));
        }

      xobject_unref (location);
    }

  g_win32_registry_value_iter_clear (&iter);
  xobject_unref (registered_apps);
}

/* Looks up an app object identified by
 * @canonical_name_folded in the @app_hashmap.
 * If such object doesn't exist,
 * creates it and puts it into the @app_hashmap.
 * Returns the object.
 */
static GWin32AppInfoApplication *
get_app_object (xhashtable_t      *app_hashmap,
                const xunichar2_t *canonical_name,
                const xchar_t     *canonical_name_u8,
                const xchar_t     *canonical_name_folded,
                xboolean_t         user_specific,
                xboolean_t         default_app,
                xboolean_t         is_uwp)
{
  GWin32AppInfoApplication *app;

  app = xhash_table_lookup (app_hashmap, canonical_name_folded);

  if (app != NULL)
    return app;

  app = xobject_new (XTYPE_WIN32_APPINFO_APPLICATION, NULL);
  app->canonical_name = g_wcsdup (canonical_name, -1);
  app->canonical_name_u8 = xstrdup (canonical_name_u8);
  app->canonical_name_folded = xstrdup (canonical_name_folded);
  app->no_open_with = FALSE;
  app->user_specific = user_specific;
  app->default_app = default_app;
  app->is_uwp = is_uwp;
  xhash_table_insert (app_hashmap,
                       xstrdup (canonical_name_folded),
                       app);

  return app;
}

/* Grabs an application that has Capabilities.
 * @app_key_path is the path to the application key
 * (which must have a "Capabilities" subkey).
 * @default_app is TRUE if the app has priority
 */
static void
read_capable_app (const xunichar2_t *app_key_path,
                  xboolean_t         user_specific,
                  xboolean_t         default_app)
{
  GWin32AppInfoApplication *app;
  xchar_t *canonical_name_u8 = NULL;
  xchar_t *canonical_name_folded = NULL;
  xchar_t *app_key_path_u8 = NULL;
  xchar_t *app_key_path_u8_folded = NULL;
  GWin32RegistryKey *appkey = NULL;
  xunichar2_t *fallback_friendly_name;
  GWin32RegistryValueType vtype;
  xboolean_t success;
  xunichar2_t *friendly_name;
  xunichar2_t *description;
  xunichar2_t *narrow_application_name;
  xunichar2_t *icon_source;
  GWin32RegistryKey *capabilities;
  GWin32RegistryKey *default_icon_key;
  GWin32RegistryKey *associations;
  const reg_verb *preferred_verb;
  xlist_t *verbs = NULL;
  xboolean_t verbs_in_root_key = TRUE;

  appkey = NULL;
  capabilities = NULL;

  if (!xutf16_to_utf8_and_fold (app_key_path,
                                 -1,
                                 &canonical_name_u8,
                                 &canonical_name_folded) ||
      !xutf16_to_utf8_and_fold (app_key_path,
                                 -1,
                                 &app_key_path_u8,
                                 &app_key_path_u8_folded) ||
      (appkey = g_win32_registry_key_new_w (app_key_path, NULL)) == NULL ||
      (capabilities = g_win32_registry_key_get_child_w (appkey, L"Capabilities", NULL)) == NULL ||
      !(get_verbs (appkey, &preferred_verb, &verbs, L"", L"Shell", NULL) ||
        (verbs_in_root_key = FALSE) ||
        get_verbs (capabilities, &preferred_verb, &verbs, L"", L"Shell", NULL)))
    {
      g_clear_pointer (&canonical_name_u8, g_free);
      g_clear_pointer (&canonical_name_folded, g_free);
      g_clear_object (&appkey);
      g_clear_object (&capabilities);
      g_clear_pointer (&app_key_path_u8, g_free);
      g_clear_pointer (&app_key_path_u8_folded, g_free);

      return;
    }

  app = get_app_object (apps_by_id,
                        app_key_path,
                        canonical_name_u8,
                        canonical_name_folded,
                        user_specific,
                        default_app,
                        FALSE);

  process_verbs_commands (g_steal_pointer (&verbs),
                          preferred_verb,
                          L"", /* [ab]use the fact that two strings are simply concatenated */
                          verbs_in_root_key ? app_key_path : g_win32_registry_key_get_path_w (capabilities),
                          FALSE,
                          app_add_verb,
                          app,
                          app);

  fallback_friendly_name = NULL;
  success = g_win32_registry_key_get_value_w (appkey,
                                              NULL,
                                              TRUE,
                                              L"",
                                              &vtype,
                                              (void **) &fallback_friendly_name,
                                              NULL,
                                              NULL);

  if (success && vtype != G_WIN32_REGISTRY_VALUE_STR)
    g_clear_pointer (&fallback_friendly_name, g_free);

  if (fallback_friendly_name &&
      app->pretty_name == NULL)
    {
      app->pretty_name = g_wcsdup (fallback_friendly_name, -1);
      g_clear_pointer (&app->pretty_name_u8, g_free);
      app->pretty_name_u8 = xutf16_to_utf8 (fallback_friendly_name,
                                             -1,
                                             NULL,
                                             NULL,
                                             NULL);
    }

  friendly_name = NULL;
  success = g_win32_registry_key_get_value_w (capabilities,
                                              g_win32_registry_get_os_dirs_w (),
                                              TRUE,
                                              L"LocalizedString",
                                              &vtype,
                                              (void **) &friendly_name,
                                              NULL,
                                              NULL);

  if (success &&
      vtype != G_WIN32_REGISTRY_VALUE_STR)
    g_clear_pointer (&friendly_name, g_free);

  if (friendly_name &&
      app->localized_pretty_name == NULL)
    {
      app->localized_pretty_name = g_wcsdup (friendly_name, -1);
      g_clear_pointer (&app->localized_pretty_name_u8, g_free);
      app->localized_pretty_name_u8 = xutf16_to_utf8 (friendly_name,
                                                       -1,
                                                       NULL,
                                                       NULL,
                                                       NULL);
    }

  description = NULL;
  success = g_win32_registry_key_get_value_w (capabilities,
                                              g_win32_registry_get_os_dirs_w (),
                                              TRUE,
                                              L"ApplicationDescription",
                                              &vtype,
                                              (void **) &description,
                                              NULL,
                                              NULL);

  if (success && vtype != G_WIN32_REGISTRY_VALUE_STR)
    g_clear_pointer (&description, g_free);

  if (description && app->description == NULL)
    {
      app->description = g_wcsdup (description, -1);
      g_clear_pointer (&app->description_u8, g_free);
      app->description_u8 = xutf16_to_utf8 (description, -1, NULL, NULL, NULL);
    }

  default_icon_key = g_win32_registry_key_get_child_w (appkey,
                                                       L"DefaultIcon",
                                                       NULL);

  icon_source = NULL;

  if (default_icon_key != NULL)
    {
      success = g_win32_registry_key_get_value_w (default_icon_key,
                                                  NULL,
                                                  TRUE,
                                                  L"",
                                                  &vtype,
                                                  (void **) &icon_source,
                                                  NULL,
                                                  NULL);

      if (success &&
          vtype != G_WIN32_REGISTRY_VALUE_STR)
        g_clear_pointer (&icon_source, g_free);

      xobject_unref (default_icon_key);
    }

  if (icon_source == NULL)
    {
      success = g_win32_registry_key_get_value_w (capabilities,
                                                  NULL,
                                                  TRUE,
                                                  L"ApplicationIcon",
                                                  &vtype,
                                                  (void **) &icon_source,
                                                  NULL,
                                                  NULL);

      if (success &&
          vtype != G_WIN32_REGISTRY_VALUE_STR)
        g_clear_pointer (&icon_source, g_free);
    }

  if (icon_source &&
      app->icon == NULL)
    {
      xchar_t *name = xutf16_to_utf8 (icon_source, -1, NULL, NULL, NULL);
      app->icon = g_themed_icon_new (name);
      g_free (name);
    }

  narrow_application_name = NULL;
  success = g_win32_registry_key_get_value_w (capabilities,
                                              g_win32_registry_get_os_dirs_w (),
                                              TRUE,
                                              L"ApplicationName",
                                              &vtype,
                                              (void **) &narrow_application_name,
                                              NULL,
                                              NULL);

  if (success && vtype != G_WIN32_REGISTRY_VALUE_STR)
    g_clear_pointer (&narrow_application_name, g_free);

  if (narrow_application_name &&
      app->localized_pretty_name == NULL)
    {
      app->localized_pretty_name = g_wcsdup (narrow_application_name, -1);
      g_clear_pointer (&app->localized_pretty_name_u8, g_free);
      app->localized_pretty_name_u8 = xutf16_to_utf8 (narrow_application_name,
                                                       -1,
                                                       NULL,
                                                       NULL,
                                                       NULL);
    }

  associations = g_win32_registry_key_get_child_w (capabilities,
                                                   L"FileAssociations",
                                                   NULL);

  if (associations != NULL)
    {
      GWin32RegistryValueIter iter;

      if (g_win32_registry_value_iter_init (&iter, associations, NULL))
        {
          xunichar2_t *file_extension;
          xunichar2_t *extension_handler;
          xsize_t      file_extension_len;
          xsize_t      extension_handler_size;
          GWin32RegistryValueType value_type;

          while (g_win32_registry_value_iter_next (&iter, TRUE, NULL))
            {
              if ((!g_win32_registry_value_iter_get_value_type (&iter,
                                                                &value_type,
                                                                NULL)) ||
                  (value_type != G_WIN32_REGISTRY_VALUE_STR) ||
                  (!g_win32_registry_value_iter_get_name_w (&iter,
                                                            &file_extension,
                                                            &file_extension_len,
                                                            NULL)) ||
                  (file_extension_len <= 0) ||
                  (file_extension[0] != L'.') ||
                  (!g_win32_registry_value_iter_get_data_w (&iter, TRUE,
                                                            (void **) &extension_handler,
                                                            &extension_handler_size,
                                                            NULL)) ||
                  (extension_handler_size < sizeof (xunichar2_t)) ||
                  (extension_handler[0] == L'\0'))
                continue;

              get_file_ext (extension_handler, file_extension, app, FALSE);
            }

          g_win32_registry_value_iter_clear (&iter);
        }

      xobject_unref (associations);
    }

  associations = g_win32_registry_key_get_child_w (capabilities, L"URLAssociations", NULL);

  if (associations != NULL)
    {
      GWin32RegistryValueIter iter;

      if (g_win32_registry_value_iter_init (&iter, associations, NULL))
        {
          xunichar2_t *url_schema;
          xunichar2_t *schema_handler;
          xsize_t      url_schema_len;
          xsize_t      schema_handler_size;
          GWin32RegistryValueType value_type;

          while (g_win32_registry_value_iter_next (&iter, TRUE, NULL))
            {
              xchar_t *schema_u8;
              xchar_t *schema_u8_folded;

              if ((!g_win32_registry_value_iter_get_value_type (&iter,
                                                                &value_type,
                                                                NULL)) ||
                  ((value_type != G_WIN32_REGISTRY_VALUE_STR) &&
                   (value_type != G_WIN32_REGISTRY_VALUE_EXPAND_STR)) ||
                  (!g_win32_registry_value_iter_get_name_w (&iter,
                                                            &url_schema,
                                                            &url_schema_len,
                                                            NULL)) ||
                  (url_schema_len <= 0) ||
                  (url_schema[0] == L'\0') ||
                  (!g_win32_registry_value_iter_get_data_w (&iter, TRUE,
                                                            (void **) &schema_handler,
                                                            &schema_handler_size,
                                                            NULL)) ||
                  (schema_handler_size < sizeof (xunichar2_t)) ||
                  (schema_handler[0] == L'\0'))
                continue;



              if (xutf16_to_utf8_and_fold (url_schema,
                                            url_schema_len,
                                            &schema_u8,
                                            &schema_u8_folded))
                get_url_association (schema_handler, url_schema, schema_u8, schema_u8_folded, app, FALSE);

              g_clear_pointer (&schema_u8, g_free);
              g_clear_pointer (&schema_u8_folded, g_free);
            }

          g_win32_registry_value_iter_clear (&iter);
        }

      xobject_unref (associations);
    }

  g_clear_pointer (&fallback_friendly_name, g_free);
  g_clear_pointer (&description, g_free);
  g_clear_pointer (&icon_source, g_free);
  g_clear_pointer (&narrow_application_name, g_free);

  xobject_unref (appkey);
  xobject_unref (capabilities);
  g_clear_pointer (&app_key_path_u8, g_free);
  g_clear_pointer (&app_key_path_u8_folded, g_free);
  g_clear_pointer (&canonical_name_u8, g_free);
  g_clear_pointer (&canonical_name_folded, g_free);
}

/* Iterates over subkeys in HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\
 * and calls get_url_association() for each one that has a user-chosen handler.
 */
static void
read_urls (GWin32RegistryKey *url_associations)
{
  GWin32RegistrySubkeyIter url_iter;

  if (url_associations == NULL)
    return;

  if (!g_win32_registry_subkey_iter_init (&url_iter, url_associations, NULL))
    return;

  while (g_win32_registry_subkey_iter_next (&url_iter, TRUE, NULL))
    {
      xchar_t *schema_u8 = NULL;
      xchar_t *schema_u8_folded = NULL;
      const xunichar2_t *url_schema = NULL;
      xunichar2_t *program_id = NULL;
      GWin32RegistryKey *user_choice = NULL;
      xsize_t url_schema_len;
      GWin32RegistryValueType val_type;

      if (g_win32_registry_subkey_iter_get_name_w (&url_iter,
                                                   &url_schema,
                                                   &url_schema_len,
                                                   NULL) &&
          xutf16_to_utf8_and_fold (url_schema,
                                    url_schema_len,
                                    &schema_u8,
                                    &schema_u8_folded) &&
          (user_choice = _g_win32_registry_key_build_and_new_w (NULL, URL_ASSOCIATIONS,
                                                                url_schema, USER_CHOICE,
                                                                NULL)) != NULL &&
          g_win32_registry_key_get_value_w (user_choice,
                                            NULL,
                                            TRUE,
                                            L"Progid",
                                            &val_type,
                                            (void **) &program_id,
                                            NULL,
                                            NULL) &&
          val_type == G_WIN32_REGISTRY_VALUE_STR)
        get_url_association (program_id, url_schema, schema_u8, schema_u8_folded, NULL, TRUE);

      g_clear_pointer (&program_id, g_free);
      g_clear_pointer (&user_choice, xobject_unref);
      g_clear_pointer (&schema_u8, g_free);
      g_clear_pointer (&schema_u8_folded, g_free);
    }

  g_win32_registry_subkey_iter_clear (&url_iter);
}

/* Reads an application that is only registered by the basename of its
 * executable (and doesn't have Capabilities subkey).
 * @incapable_app is the registry key for the app.
 * @app_exe_basename is the basename of its executable.
 */
static void
read_incapable_app (GWin32RegistryKey *incapable_app,
                    const xunichar2_t   *app_exe_basename,
                    const xchar_t       *app_exe_basename_u8,
                    const xchar_t       *app_exe_basename_u8_folded)
{
  GWin32RegistryValueIter sup_iter;
  GWin32AppInfoApplication *app;
  xlist_t *verbs;
  const reg_verb *preferred_verb;
  xunichar2_t *friendly_app_name;
  xboolean_t success;
  GWin32RegistryValueType vtype;
  xboolean_t no_open_with;
  GWin32RegistryKey *default_icon_key;
  xunichar2_t *icon_source;
  xicon_t *icon = NULL;
  GWin32RegistryKey *supported_key;

  if (!get_verbs (incapable_app, &preferred_verb, &verbs, L"", L"Shell", NULL))
    return;

  app = get_app_object (apps_by_exe,
                        app_exe_basename,
                        app_exe_basename_u8,
                        app_exe_basename_u8_folded,
                        FALSE,
                        FALSE,
                        FALSE);

  process_verbs_commands (g_steal_pointer (&verbs),
                          preferred_verb,
                          L"HKEY_CLASSES_ROOT\\Applications\\",
                          app_exe_basename,
                          TRUE,
                          app_add_verb,
                          app,
                          app);

  friendly_app_name = NULL;
  success = g_win32_registry_key_get_value_w (incapable_app,
                                              g_win32_registry_get_os_dirs_w (),
                                              TRUE,
                                              L"FriendlyAppName",
                                              &vtype,
                                              (void **) &friendly_app_name,
                                              NULL,
                                              NULL);

  if (success && vtype != G_WIN32_REGISTRY_VALUE_STR)
    g_clear_pointer (&friendly_app_name, g_free);

  no_open_with = g_win32_registry_key_get_value_w (incapable_app,
                                                   NULL,
                                                   TRUE,
                                                   L"NoOpenWith",
                                                   &vtype,
                                                   NULL,
                                                   NULL,
                                                   NULL);

  default_icon_key =
      g_win32_registry_key_get_child_w (incapable_app,
                                        L"DefaultIcon",
                                        NULL);

  icon_source = NULL;

  if (default_icon_key != NULL)
    {
      success =
          g_win32_registry_key_get_value_w (default_icon_key,
                                            NULL,
                                            TRUE,
                                            L"",
                                            &vtype,
                                            (void **) &icon_source,
                                            NULL,
                                            NULL);

      if (success && vtype != G_WIN32_REGISTRY_VALUE_STR)
        g_clear_pointer (&icon_source, g_free);

      xobject_unref (default_icon_key);
    }

  if (icon_source)
    {
      xchar_t *name = xutf16_to_utf8 (icon_source, -1, NULL, NULL, NULL);
      if (name != NULL)
        icon = g_themed_icon_new (name);
      g_free (name);
    }

  app->no_open_with = no_open_with;

  if (friendly_app_name &&
      app->localized_pretty_name == NULL)
    {
      app->localized_pretty_name = g_wcsdup (friendly_app_name, -1);
      g_clear_pointer (&app->localized_pretty_name_u8, g_free);
      app->localized_pretty_name_u8 =
          xutf16_to_utf8 (friendly_app_name, -1, NULL, NULL, NULL);
    }

  if (icon && app->icon == NULL)
    app->icon = xobject_ref (icon);

  supported_key =
      g_win32_registry_key_get_child_w (incapable_app,
                                        L"SupportedTypes",
                                        NULL);

  if (supported_key &&
      g_win32_registry_value_iter_init (&sup_iter, supported_key, NULL))
    {
      xunichar2_t *ext_name;
      xsize_t      ext_name_len;

      while (g_win32_registry_value_iter_next (&sup_iter, TRUE, NULL))
        {
          if ((!g_win32_registry_value_iter_get_name_w (&sup_iter,
                                                        &ext_name,
                                                        &ext_name_len,
                                                        NULL)) ||
              (ext_name_len <= 0) ||
              (ext_name[0] != L'.'))
            continue;

          get_file_ext (ext_name, ext_name, app, FALSE);
        }

      g_win32_registry_value_iter_clear (&sup_iter);
    }

  g_clear_object (&supported_key);
  g_free (friendly_app_name);
  g_free (icon_source);

  g_clear_object (&icon);
}

/* Iterates over subkeys of HKEY_CLASSES_ROOT\\Applications
 * and calls read_incapable_app() for each one.
 */
static void
read_exeapps (void)
{
  GWin32RegistryKey *applications_key;
  GWin32RegistrySubkeyIter app_iter;

  applications_key =
      g_win32_registry_key_new_w (L"HKEY_CLASSES_ROOT\\Applications", NULL);

  if (applications_key == NULL)
    return;

  if (!g_win32_registry_subkey_iter_init (&app_iter, applications_key, NULL))
    {
      xobject_unref (applications_key);
      return;
    }

  while (g_win32_registry_subkey_iter_next (&app_iter, TRUE, NULL))
    {
      const xunichar2_t *app_exe_basename;
      xsize_t app_exe_basename_len;
      GWin32RegistryKey *incapable_app;
      xchar_t *app_exe_basename_u8;
      xchar_t *app_exe_basename_u8_folded;

      if (!g_win32_registry_subkey_iter_get_name_w (&app_iter,
                                                    &app_exe_basename,
                                                    &app_exe_basename_len,
                                                    NULL) ||
          !xutf16_to_utf8_and_fold (app_exe_basename,
                                     app_exe_basename_len,
                                     &app_exe_basename_u8,
                                     &app_exe_basename_u8_folded))
        continue;

      incapable_app =
          g_win32_registry_key_get_child_w (applications_key,
                                            app_exe_basename,
                                            NULL);

      if (incapable_app != NULL)
        read_incapable_app (incapable_app,
                            app_exe_basename,
                            app_exe_basename_u8,
                            app_exe_basename_u8_folded);

      g_clear_object (&incapable_app);
      g_clear_pointer (&app_exe_basename_u8, g_free);
      g_clear_pointer (&app_exe_basename_u8_folded, g_free);
    }

  g_win32_registry_subkey_iter_clear (&app_iter);
  xobject_unref (applications_key);
}

/* Iterates over subkeys of HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\
 * and calls get_file_ext() for each associated handler
 * (starting with user-chosen handler, if any)
 */
static void
read_exts (GWin32RegistryKey *file_exts)
{
  GWin32RegistrySubkeyIter ext_iter;
  const xunichar2_t *file_extension;
  xsize_t file_extension_len;

  if (file_exts == NULL)
    return;

  if (!g_win32_registry_subkey_iter_init (&ext_iter, file_exts, NULL))
    return;

  while (g_win32_registry_subkey_iter_next (&ext_iter, TRUE, NULL))
    {
      GWin32RegistryKey *open_with_progids;
      xunichar2_t *program_id;
      GWin32RegistryValueIter iter;
      xunichar2_t *value_name;
      xsize_t      value_name_len;
      GWin32RegistryValueType value_type;
      GWin32RegistryKey *user_choice;

      if (!g_win32_registry_subkey_iter_get_name_w (&ext_iter,
                                                    &file_extension,
                                                    &file_extension_len,
                                                    NULL))
        continue;

      program_id = NULL;
      user_choice = _g_win32_registry_key_build_and_new_w (NULL, FILE_EXTS, file_extension,
                                                           USER_CHOICE, NULL);
      if (user_choice &&
          g_win32_registry_key_get_value_w (user_choice,
                                            NULL,
                                            TRUE,
                                            L"Progid",
                                            &value_type,
                                            (void **) &program_id,
                                            NULL,
                                            NULL) &&
          value_type == G_WIN32_REGISTRY_VALUE_STR)
        {
          /* Note: program_id could be "ProgramID" or "Applications\\program.exe".
           * The code still works, but handler_id might have a backslash
           * in it - that might trip us up later on.
           * Even though in that case this is logically an "application"
           * registry entry, we don't treat it in any special way.
           * We do scan that registry branch anyway, just not here.
           */
          get_file_ext (program_id, file_extension, NULL, TRUE);
        }

      g_clear_object (&user_choice);
      g_clear_pointer (&program_id, g_free);

      open_with_progids = _g_win32_registry_key_build_and_new_w (NULL, FILE_EXTS,
                                                                 file_extension,
                                                                 OPEN_WITH_PROGIDS,
                                                                 NULL);

      if (open_with_progids == NULL)
        continue;

      if (!g_win32_registry_value_iter_init (&iter, open_with_progids, NULL))
        {
          g_clear_object (&open_with_progids);
          continue;
        }

      while (g_win32_registry_value_iter_next (&iter, TRUE, NULL))
        {
          if (!g_win32_registry_value_iter_get_name_w (&iter, &value_name,
                                                       &value_name_len,
                                                       NULL) ||
              (value_name_len == 0))
            continue;

          get_file_ext (value_name, file_extension, NULL, FALSE);
        }

      g_win32_registry_value_iter_clear (&iter);
      g_clear_object (&open_with_progids);
    }

  g_win32_registry_subkey_iter_clear (&ext_iter);
}

/* Iterates over subkeys in HKCR, calls
 * get_file_ext() for any subkey that starts with ".",
 * or get_url_association() for any subkey that could
 * be a URL schema and has a "URL Protocol" value.
 */
static void
read_classes (GWin32RegistryKey *classes_root)
{
  GWin32RegistrySubkeyIter class_iter;
  const xunichar2_t *class_name;
  xsize_t class_name_len;

  if (classes_root == NULL)
    return;

  if (!g_win32_registry_subkey_iter_init (&class_iter, classes_root, NULL))
    return;

  while (g_win32_registry_subkey_iter_next (&class_iter, TRUE, NULL))
    {
      if ((!g_win32_registry_subkey_iter_get_name_w (&class_iter,
                                                     &class_name,
                                                     &class_name_len,
                                                     NULL)) ||
          (class_name_len <= 1))
        continue;

      if (class_name[0] == L'.')
        {
          GWin32RegistryKey *class_key;
          GWin32RegistryValueIter iter;
          GWin32RegistryKey *open_with_progids;
          xunichar2_t *value_name;
          xsize_t      value_name_len;

          /* Read the data from the HKCR\\.ext (usually proxied
           * to another HKCR subkey)
           */
          get_file_ext (class_name, class_name, NULL, FALSE);

          class_key = g_win32_registry_key_get_child_w (classes_root, class_name, NULL);

          if (class_key == NULL)
            continue;

          open_with_progids = g_win32_registry_key_get_child_w (class_key, L"OpenWithProgids", NULL);
          g_clear_object (&class_key);

          if (open_with_progids == NULL)
            continue;

          if (!g_win32_registry_value_iter_init (&iter, open_with_progids, NULL))
            {
              g_clear_object (&open_with_progids);
              continue;
            }

          /* Read the data for other handlers for this extension */
          while (g_win32_registry_value_iter_next (&iter, TRUE, NULL))
            {
              if (!g_win32_registry_value_iter_get_name_w (&iter, &value_name,
                                                           &value_name_len,
                                                           NULL) ||
                  (value_name_len == 0))
                continue;

              get_file_ext (value_name, class_name, NULL, FALSE);
            }

          g_win32_registry_value_iter_clear (&iter);
          g_clear_object (&open_with_progids);
        }
      else
        {
          xsize_t i;
          GWin32RegistryKey *class_key;
          xboolean_t success;
          GWin32RegistryValueType vtype;
          xchar_t *schema_u8;
          xchar_t *schema_u8_folded;

          for (i = 0; i < class_name_len; i++)
            if (!iswalpha (class_name[i]))
              break;

          if (i != class_name_len)
            continue;

          class_key = g_win32_registry_key_get_child_w (classes_root, class_name, NULL);

          if (class_key == NULL)
            continue;

          success = g_win32_registry_key_get_value_w (class_key,
                                                      NULL,
                                                      TRUE,
                                                      L"URL Protocol",
                                                      &vtype,
                                                      NULL,
                                                      NULL,
                                                      NULL);
          g_clear_object (&class_key);

          if (!success ||
              vtype != G_WIN32_REGISTRY_VALUE_STR)
            continue;

          if (!xutf16_to_utf8_and_fold (class_name, -1, &schema_u8, &schema_u8_folded))
            continue;

          get_url_association (class_name, class_name, schema_u8, schema_u8_folded, NULL, FALSE);

          g_clear_pointer (&schema_u8, g_free);
          g_clear_pointer (&schema_u8_folded, g_free);
        }
    }

  g_win32_registry_subkey_iter_clear (&class_iter);
}

/* Iterates over all handlers and over all apps,
 * and links handler verbs to apps if a handler
 * runs the same executable as one of the app verbs.
 */
static void
link_handlers_to_unregistered_apps (void)
{
  xhash_table_iter_t iter;
  xhash_table_iter_t app_iter;
  GWin32AppInfoHandler *handler;
  xchar_t *handler_id_fld;
  GWin32AppInfoApplication *app;
  xchar_t *canonical_name_fld;
  xchar_t *appexe_fld_basename;

  xhash_table_iter_init (&iter, handlers);
  while (xhash_table_iter_next (&iter,
                                 (xpointer_t *) &handler_id_fld,
                                 (xpointer_t *) &handler))
    {
      xsize_t vi;

      if (handler->uwp_aumid != NULL)
        continue;

      for (vi = 0; vi < handler->verbs->len; vi++)
        {
          GWin32AppInfoShellVerb *handler_verb;
          const xchar_t *handler_exe_basename;
          enum
            {
              SH_UNKNOWN,
              GOT_SH_INFO,
              ERROR_GETTING_SH_INFO,
            } have_stat_handler = SH_UNKNOWN;
          GWin32PrivateStat handler_verb_exec_info;

          handler_verb = _verb_idx (handler->verbs, vi);

          if (handler_verb->app != NULL)
            continue;

          handler_exe_basename = xutf8_find_basename (handler_verb->executable_folded, -1);
          xhash_table_iter_init (&app_iter, apps_by_id);

          while (xhash_table_iter_next (&app_iter,
                                         (xpointer_t *) &canonical_name_fld,
                                         (xpointer_t *) &app))
            {
              GWin32AppInfoShellVerb *app_verb;
              xsize_t ai;

              if (app->is_uwp)
                continue;

              for (ai = 0; ai < app->verbs->len; ai++)
                {
                  GWin32PrivateStat app_verb_exec_info;
                  const xchar_t *app_exe_basename;
                  app_verb = _verb_idx (app->verbs, ai);

                  app_exe_basename = xutf8_find_basename (app_verb->executable_folded, -1);

                  /* First check that the executable paths are identical */
                  if (xstrcmp0 (app_verb->executable_folded, handler_verb->executable_folded) != 0)
                    {
                      /* If not, check the basenames. If they are different, don't bother
                       * with further checks.
                       */
                      if (xstrcmp0 (app_exe_basename, handler_exe_basename) != 0)
                        continue;

                      /* Get filesystem IDs for both files.
                       * For the handler that is attempted only once.
                       */
                      if (have_stat_handler == SH_UNKNOWN)
                        {
                          if (XPL_PRIVATE_CALL (g_win32_stat_utf8) (handler_verb->executable_folded,
                                                                     &handler_verb_exec_info) == 0)
                            have_stat_handler = GOT_SH_INFO;
                          else
                            have_stat_handler = ERROR_GETTING_SH_INFO;
                        }

                      if (have_stat_handler != GOT_SH_INFO ||
                          (XPL_PRIVATE_CALL (g_win32_stat_utf8) (app_verb->executable_folded,
                                                                  &app_verb_exec_info) != 0) ||
                          app_verb_exec_info.file_index != handler_verb_exec_info.file_index)
                        continue;
                    }

                  handler_verb->app = xobject_ref (app);
                  break;
                }
            }

          if (handler_verb->app != NULL)
            continue;

          xhash_table_iter_init (&app_iter, apps_by_exe);

          while (xhash_table_iter_next (&app_iter,
                                         (xpointer_t *) &appexe_fld_basename,
                                         (xpointer_t *) &app))
            {
              if (app->is_uwp)
                continue;

              /* Use basename because apps_by_exe only has basenames */
              if (xstrcmp0 (handler_exe_basename, appexe_fld_basename) != 0)
                continue;

              handler_verb->app = xobject_ref (app);
              break;
            }
        }
    }
}

/* Finds all .ext and schema: handler verbs that have no app linked to them,
 * creates a "fake app" object and links these verbs to these
 * objects. Objects are identified by the full path to
 * the executable being run, thus multiple different invocations
 * get grouped in a more-or-less natural way.
 * The iteration goes separately over .ext and schema: handlers
 * (instead of the global handlers hashmap) to allow us to
 * put the handlers into supported_urls or supported_exts as
 * needed (handler objects themselves have no knowledge of extensions
 * and/or URLs they are associated with).
 */
static void
link_handlers_to_fake_apps (void)
{
  xhash_table_iter_t iter;
  xhash_table_iter_t handler_iter;
  xchar_t *extension_utf8_folded;
  GWin32AppInfoFileExtension *file_extn;
  xchar_t *handler_id_fld;
  GWin32AppInfoHandler *handler;
  xchar_t *url_utf8_folded;
  GWin32AppInfoURLSchema *schema;

  xhash_table_iter_init (&iter, extensions);
  while (xhash_table_iter_next (&iter,
                                 (xpointer_t *) &extension_utf8_folded,
                                 (xpointer_t *) &file_extn))
    {
      xhash_table_iter_init (&handler_iter, file_extn->handlers);
      while (xhash_table_iter_next (&handler_iter,
                                     (xpointer_t *) &handler_id_fld,
                                     (xpointer_t *) &handler))
        {
          xsize_t vi;

          if (handler->uwp_aumid != NULL)
            continue;

          for (vi = 0; vi < handler->verbs->len; vi++)
            {
              GWin32AppInfoShellVerb *handler_verb;
              GWin32AppInfoApplication *app;
              xunichar2_t *exename_utf16;
              handler_verb = _verb_idx (handler->verbs, vi);

              if (handler_verb->app != NULL)
                continue;

              exename_utf16 = xutf8_to_utf16 (handler_verb->executable, -1, NULL, NULL, NULL);
              if (exename_utf16 == NULL)
                continue;

              app = get_app_object (fake_apps,
                                    exename_utf16,
                                    handler_verb->executable,
                                    handler_verb->executable_folded,
                                    FALSE,
                                    FALSE,
                                    FALSE);
              g_clear_pointer (&exename_utf16, g_free);
              handler_verb->app = xobject_ref (app);

              app_add_verb (app,
                            app,
                            handler_verb->verb_name,
                            handler_verb->command,
                            handler_verb->command_utf8,
                            handler_verb->verb_displayname,
                            TRUE,
                            TRUE);
              xhash_table_insert (app->supported_exts,
                                   xstrdup (extension_utf8_folded),
                                   xobject_ref (handler));
            }
        }
    }

  xhash_table_iter_init (&iter, urls);
  while (xhash_table_iter_next (&iter,
                                 (xpointer_t *) &url_utf8_folded,
                                 (xpointer_t *) &schema))
    {
      xhash_table_iter_init (&handler_iter, schema->handlers);
      while (xhash_table_iter_next (&handler_iter,
                                     (xpointer_t *) &handler_id_fld,
                                     (xpointer_t *) &handler))
        {
          xsize_t vi;

          if (handler->uwp_aumid != NULL)
            continue;

          for (vi = 0; vi < handler->verbs->len; vi++)
            {
              GWin32AppInfoShellVerb *handler_verb;
              GWin32AppInfoApplication *app;
              xchar_t *command_utf8_folded;
              handler_verb = _verb_idx (handler->verbs, vi);

              if (handler_verb->app != NULL)
                continue;

              command_utf8_folded = xutf8_casefold (handler_verb->command_utf8, -1);
              app = get_app_object (fake_apps,
                                    handler_verb->command,
                                    handler_verb->command_utf8,
                                    command_utf8_folded,
                                    FALSE,
                                    FALSE,
                                    FALSE);
              g_clear_pointer (&command_utf8_folded, g_free);
              handler_verb->app = xobject_ref (app);

              app_add_verb (app,
                            app,
                            handler_verb->verb_name,
                            handler_verb->command,
                            handler_verb->command_utf8,
                            handler_verb->verb_displayname,
                            TRUE,
                            TRUE);
              xhash_table_insert (app->supported_urls,
                                   xstrdup (url_utf8_folded),
                                   xobject_ref (handler));
            }
        }
    }
}

static GWin32AppInfoHandler *
find_uwp_handler_for_ext (GWin32AppInfoFileExtension *file_extn,
                          const xunichar2_t            *app_user_model_id)
{
  xhash_table_iter_t handler_iter;
  xchar_t *handler_id_fld;
  GWin32AppInfoHandler *handler;

  xhash_table_iter_init (&handler_iter, file_extn->handlers);
  while (xhash_table_iter_next (&handler_iter,
                                 (xpointer_t *) &handler_id_fld,
                                 (xpointer_t *) &handler))
    {
      if (handler->uwp_aumid == NULL)
        continue;

      if (_wcsicmp (handler->uwp_aumid, app_user_model_id) == 0)
        return handler;
    }

  return NULL;
}

static GWin32AppInfoHandler *
find_uwp_handler_for_schema (GWin32AppInfoURLSchema *schema,
                             const xunichar2_t        *app_user_model_id)
{
  xhash_table_iter_t handler_iter;
  xchar_t *handler_id_fld;
  GWin32AppInfoHandler *handler;

  xhash_table_iter_init (&handler_iter, schema->handlers);
  while (xhash_table_iter_next (&handler_iter,
                                 (xpointer_t *) &handler_id_fld,
                                 (xpointer_t *) &handler))
    {
      if (handler->uwp_aumid == NULL)
        continue;

      if (_wcsicmp (handler->uwp_aumid, app_user_model_id) == 0)
        return handler;
    }

  return NULL;
}

static xboolean_t
uwp_package_cb (xpointer_t         user_data,
                const xunichar2_t *full_package_name,
                const xunichar2_t *package_name,
                const xunichar2_t *app_user_model_id,
                xboolean_t         show_in_applist,
                xptr_array_t       *supported_extgroups,
                xptr_array_t       *supported_protocols)
{
  xuint_t i, i_verb, i_ext;
  xint_t extensions_considered;
  GWin32AppInfoApplication *app;
  xchar_t *app_user_model_id_u8;
  xchar_t *app_user_model_id_u8_folded;
  xhash_table_iter_t iter;
  GWin32AppInfoHandler *ext;
  GWin32AppInfoHandler *url;

  if (!xutf16_to_utf8_and_fold (app_user_model_id,
                                 -1,
                                 &app_user_model_id_u8,
                                 &app_user_model_id_u8_folded))
    return TRUE;

  app = get_app_object (apps_by_id,
                        app_user_model_id,
                        app_user_model_id_u8,
                        app_user_model_id_u8_folded,
                        TRUE,
                        FALSE,
                        TRUE);

  extensions_considered = 0;

  for (i = 0; i < supported_extgroups->len; i++)
    {
      GWin32PackageExtGroup *grp = (GWin32PackageExtGroup *) xptr_array_index (supported_extgroups, i);

      extensions_considered += grp->extensions->len;

      for (i_ext = 0; i_ext < grp->extensions->len; i_ext++)
        {
          wchar_t *ext = (wchar_t *) xptr_array_index (grp->extensions, i_ext);
          xchar_t *ext_u8;
          xchar_t *ext_u8_folded;
          GWin32AppInfoFileExtension *file_extn;
          GWin32AppInfoHandler *handler_rec;

          if (!xutf16_to_utf8_and_fold (ext,
                                         -1,
                                         &ext_u8,
                                         &ext_u8_folded))
            continue;

          file_extn = get_ext_object (ext, ext_u8, ext_u8_folded);
          g_free (ext_u8);
          handler_rec = find_uwp_handler_for_ext (file_extn, app_user_model_id);

          if (handler_rec == NULL)
            {
              /* Use AppUserModelId as the ID of the new fake handler */
              handler_rec = get_handler_object (app_user_model_id_u8_folded,
                                                NULL,
                                                app_user_model_id,
                                                app_user_model_id);
              xhash_table_insert (file_extn->handlers,
                                   xstrdup (app_user_model_id_u8_folded),
                                   xobject_ref (handler_rec));
            }

          if (file_extn->chosen_handler == NULL)
            g_set_object (&file_extn->chosen_handler, handler_rec);

          /* This is somewhat wasteful, but for 100% correct handling
           * we need to remember which extensions (handlers) support
           * which verbs, and each handler gets its own copy of the
           * verb object, since our design is handler-centric,
           * not verb-centric. The app also gets a list of verbs,
           * but without handlers it would have no idea which
           * verbs can be used with which extensions.
           */
          for (i_verb = 0; i_verb < grp->verbs->len; i_verb++)
            {
              wchar_t *verb = NULL;

              verb = (wchar_t *) xptr_array_index (grp->verbs, i_verb);
              /* *_add_verb() functions are no-ops when a verb already exists,
               * so we're free to call them as many times as we want.
               */
              uwp_handler_add_verb (handler_rec,
                                    app,
                                    verb,
                                    NULL,
                                    FALSE);
            }

          xhash_table_insert (app->supported_exts,
                               g_steal_pointer (&ext_u8_folded),
                               xobject_ref (handler_rec));
        }
    }

  xhash_table_iter_init (&iter, app->supported_exts);

  /* Pile up all handler verbs into the app too,
   * for cases when we don't have a ref to a handler.
   */
  while (xhash_table_iter_next (&iter, NULL, (xpointer_t *) &ext))
    {
      xuint_t i_hverb;

      if (!ext)
        continue;

      for (i_hverb = 0; i_hverb < ext->verbs->len; i_hverb++)
        {
          GWin32AppInfoShellVerb *handler_verb;

          handler_verb = _verb_idx (ext->verbs, i_hverb);
          uwp_app_add_verb (app, handler_verb->verb_name, handler_verb->verb_displayname);
          if (handler_verb->app == NULL && handler_verb->is_uwp)
            handler_verb->app = xobject_ref (app);
        }
    }

  if (app->verbs->len == 0 && extensions_considered > 0)
    g_warning ("Unexpectedly, UWP app `%S' (AUMId `%s') supports %d extensions but has no verbs",
               full_package_name, app_user_model_id_u8, extensions_considered);

  for (i = 0; i < supported_protocols->len; i++)
    {
      wchar_t *proto = (wchar_t *) xptr_array_index (supported_protocols, i);
      xchar_t *proto_u8;
      xchar_t *proto_u8_folded;
      GWin32AppInfoURLSchema *schema_rec;
      GWin32AppInfoHandler *handler_rec;

      if (!xutf16_to_utf8_and_fold (proto,
                                     -1,
                                     &proto_u8,
                                     &proto_u8_folded))
        continue;

      schema_rec = get_schema_object (proto,
                                      proto_u8,
                                      proto_u8_folded);

      g_free (proto_u8);

      handler_rec = find_uwp_handler_for_schema (schema_rec, app_user_model_id);

      if (handler_rec == NULL)
        {
          /* Use AppUserModelId as the ID of the new fake handler */
          handler_rec = get_handler_object (app_user_model_id_u8_folded,
                                            NULL,
                                            app_user_model_id,
                                            app_user_model_id);

          xhash_table_insert (schema_rec->handlers,
                               xstrdup (app_user_model_id_u8_folded),
                               xobject_ref (handler_rec));
        }

      if (schema_rec->chosen_handler == NULL)
        g_set_object (&schema_rec->chosen_handler, handler_rec);

      /* Technically, UWP apps don't use verbs for URIs,
       * but we only store an app field in verbs,
       * so each UWP URI handler has to have one.
       * Let's call it "open".
       */
      uwp_handler_add_verb (handler_rec,
                            app,
                            L"open",
                            NULL,
                            TRUE);

      xhash_table_insert (app->supported_urls,
                           g_steal_pointer (&proto_u8_folded),
                           xobject_ref (handler_rec));
    }

  xhash_table_iter_init (&iter, app->supported_urls);

  while (xhash_table_iter_next (&iter, NULL, (xpointer_t *) &url))
    {
      xuint_t i_hverb;

      if (!url)
        continue;

      for (i_hverb = 0; i_hverb < url->verbs->len; i_hverb++)
        {
          GWin32AppInfoShellVerb *handler_verb;

          handler_verb = _verb_idx (url->verbs, i_hverb);
          uwp_app_add_verb (app, handler_verb->verb_name, handler_verb->verb_displayname);
          if (handler_verb->app == NULL && handler_verb->is_uwp)
            handler_verb->app = xobject_ref (app);
        }
    }

  g_free (app_user_model_id_u8);
  g_free (app_user_model_id_u8_folded);

  return TRUE;
}

/* Calls SHLoadIndirectString() in a loop to resolve
 * a string in @{...} format (also supports other indirect
 * strings, but we aren't using it for those).
 * Consumes the input, but may return it unmodified
 * (not an indirect string). May return %NULL (the string
 * is indirect, but the OS failed to load it).
 */
static xunichar2_t *
resolve_string (xunichar2_t *at_string)
{
  HRESULT hr;
  xunichar2_t *result = NULL;
  xsize_t result_size;
  /* This value is arbitrary */
  const xsize_t reasonable_size_limit = 8192;

  if (at_string == NULL || at_string[0] != L'@')
    return at_string;

  /* In case of a no-op @at_string will be copied into the output,
   * buffer so allocate at least that much.
   */
  result_size = wcslen (at_string) + 1;

  while (TRUE)
    {
      result = g_renew (xunichar2_t, result, result_size);
      /* Since there's no built-in way to detect too small buffer size,
       * we do so by putting a sentinel at the end of the buffer.
       * If it's 0 (result is always 0-terminated, even if the buffer
       * is too small), then try larger buffer.
       */
      result[result_size - 1] = 0xff;
      /* This string accepts size in characters, not bytes. */
      hr = SHLoadIndirectString (at_string, result, result_size, NULL);
      if (!SUCCEEDED (hr))
        {
          g_free (result);
          g_free (at_string);
          return NULL;
        }
      else if (result[result_size - 1] != 0 ||
               result_size >= reasonable_size_limit)
        {
          /* Now that the length is known, allocate the exact amount */
          xunichar2_t *copy = g_wcsdup (result, -1);
          g_free (result);
          g_free (at_string);
          return copy;
        }

      result_size *= 2;
    }

  g_assert_not_reached ();

  return at_string;
}

static void
grab_registry_string (GWin32RegistryKey  *handler_appkey,
                      const xunichar2_t    *value_name,
                      xunichar2_t         **destination,
                      xchar_t             **destination_u8)
{
  xunichar2_t *value;
  xsize_t value_size;
  GWin32RegistryValueType vtype;
  const xunichar2_t *ms_resource_prefix = L"ms-resource:";
  xsize_t ms_resource_prefix_len = wcslen (ms_resource_prefix);

  /* Right now this function is not used without destination,
   * enforce this. destination_u8 is optional.
   */
  xassert (destination != NULL);

  if (*destination != NULL)
    return;

  value = NULL;
  if (g_win32_registry_key_get_value_w (handler_appkey,
                                        NULL,
                                        TRUE,
                                        value_name,
                                        &vtype,
                                        (void **) &value,
                                        &value_size,
                                        NULL) &&
      vtype != G_WIN32_REGISTRY_VALUE_STR)
    g_clear_pointer (&value, g_free);

  /* There's no way for us to resolve "ms-resource:..." strings */
  if (value != NULL &&
      value_size >= ms_resource_prefix_len &&
      memcmp (value,
              ms_resource_prefix,
              ms_resource_prefix_len * sizeof (xunichar2_t)) == 0)
    g_clear_pointer (&value, g_free);

  if (value == NULL)
    return;

  *destination = resolve_string (g_steal_pointer (&value));

  if (*destination == NULL)
    return;

  if (destination_u8)
    *destination_u8 = xutf16_to_utf8 (*destination, -1, NULL, NULL, NULL);
}

static void
read_uwp_handler_info (void)
{
  xhash_table_iter_t iter;
  GWin32RegistryKey *handler_appkey;
  xunichar2_t *aumid;

  xhash_table_iter_init (&iter, uwp_handler_table);

  while (xhash_table_iter_next (&iter, (xpointer_t *) &handler_appkey, (xpointer_t *) &aumid))
    {
      xchar_t *aumid_u8_folded;
      GWin32AppInfoApplication *app;

      if (!xutf16_to_utf8_and_fold (aumid,
                                     -1,
                                     NULL,
                                     &aumid_u8_folded))
        continue;

      app = xhash_table_lookup (apps_by_id, aumid_u8_folded);
      g_clear_pointer (&aumid_u8_folded, g_free);

      if (app == NULL)
        continue;

      grab_registry_string (handler_appkey, L"ApplicationDescription", &app->description, &app->description_u8);
      grab_registry_string (handler_appkey, L"ApplicationName", &app->localized_pretty_name, &app->localized_pretty_name_u8);
      /* TODO: ApplicationIcon value (usually also @{...}) resolves into
       * an image (PNG only?) with implicit multiple variants (scale, size, etc).
       */
    }
}

static void
update_registry_data (void)
{
  xuint_t i;
  xptr_array_t *capable_apps_keys;
  xptr_array_t *user_capable_apps_keys;
  xptr_array_t *priority_capable_apps_keys;
  GWin32RegistryKey *url_associations;
  GWin32RegistryKey *file_exts;
  GWin32RegistryKey *classes_root;
  DWORD collect_start, collect_end, alloc_end, capable_end, url_end, ext_end, exeapp_end, classes_end, uwp_end, postproc_end;
  xerror_t *error = NULL;

  url_associations =
      g_win32_registry_key_new_w (L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations",
                                   NULL);
  file_exts =
      g_win32_registry_key_new_w (L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts",
                                   NULL);
  classes_root = g_win32_registry_key_new_w (L"HKEY_CLASSES_ROOT", NULL);

  capable_apps_keys = xptr_array_new_with_free_func (g_free);
  user_capable_apps_keys = xptr_array_new_with_free_func (g_free);
  priority_capable_apps_keys = xptr_array_new_with_free_func (g_free);

  g_clear_pointer (&apps_by_id, xhash_table_destroy);
  g_clear_pointer (&apps_by_exe, xhash_table_destroy);
  g_clear_pointer (&fake_apps, xhash_table_destroy);
  g_clear_pointer (&urls, xhash_table_destroy);
  g_clear_pointer (&extensions, xhash_table_destroy);
  g_clear_pointer (&handlers, xhash_table_destroy);

  collect_start = GetTickCount ();
  collect_capable_apps_from_clients (capable_apps_keys,
                                     priority_capable_apps_keys,
                                     FALSE);
  collect_capable_apps_from_clients (user_capable_apps_keys,
                                     priority_capable_apps_keys,
                                     TRUE);
  collect_capable_apps_from_registered_apps (user_capable_apps_keys,
                                             TRUE);
  collect_capable_apps_from_registered_apps (capable_apps_keys,
                                             FALSE);
  collect_end = GetTickCount ();

  apps_by_id =
      xhash_table_new_full (xstr_hash, xstr_equal, g_free, xobject_unref);
  apps_by_exe =
      xhash_table_new_full (xstr_hash, xstr_equal, g_free, xobject_unref);
  fake_apps =
      xhash_table_new_full (xstr_hash, xstr_equal, g_free, xobject_unref);
  urls =
      xhash_table_new_full (xstr_hash, xstr_equal, g_free, xobject_unref);
  extensions =
      xhash_table_new_full (xstr_hash, xstr_equal, g_free, xobject_unref);
  handlers =
      xhash_table_new_full (xstr_hash, xstr_equal, g_free, xobject_unref);
  uwp_handler_table =
      xhash_table_new_full (g_direct_hash, g_direct_equal, xobject_unref, g_free);
  alloc_end = GetTickCount ();

  for (i = 0; i < priority_capable_apps_keys->len; i++)
    read_capable_app (xptr_array_index (priority_capable_apps_keys, i),
                      TRUE,
                      TRUE);
  for (i = 0; i < user_capable_apps_keys->len; i++)
    read_capable_app (xptr_array_index (user_capable_apps_keys, i),
                      TRUE,
                      FALSE);
  for (i = 0; i < capable_apps_keys->len; i++)
    read_capable_app (xptr_array_index (capable_apps_keys, i),
                      FALSE,
                      FALSE);
  capable_end = GetTickCount ();

  read_urls (url_associations);
  url_end = GetTickCount ();
  read_exts (file_exts);
  ext_end = GetTickCount ();
  read_exeapps ();
  exeapp_end = GetTickCount ();
  read_classes (classes_root);
  classes_end = GetTickCount ();

  if (!g_win32_package_parser_enum_packages (uwp_package_cb, NULL, &error))
    {
      g_debug ("Unable to get UWP apps: %s", error->message);
      g_clear_error (&error);
    }

  read_uwp_handler_info ();

  uwp_end = GetTickCount ();
  link_handlers_to_unregistered_apps ();
  link_handlers_to_fake_apps ();
  postproc_end = GetTickCount ();

  g_debug ("Collecting capable appnames: %lums\n"
           "Allocating hashtables:...... %lums\n"
           "Reading capable apps:        %lums\n"
           "Reading URL associations:... %lums\n"
           "Reading extension assocs:    %lums\n"
           "Reading exe-only apps:...... %lums\n"
           "Reading classes:             %lums\n"
           "Reading UWP apps:            %lums\n"
           "Postprocessing:..............%lums\n"
           "TOTAL:                       %lums",
           collect_end - collect_start,
           alloc_end - collect_end,
           capable_end - alloc_end,
           url_end - capable_end,
           ext_end - url_end,
           exeapp_end - ext_end,
           classes_end - exeapp_end,
           uwp_end - classes_end,
           postproc_end - uwp_end,
           postproc_end - collect_start);

  g_clear_object (&classes_root);
  g_clear_object (&url_associations);
  g_clear_object (&file_exts);
  xptr_array_free (capable_apps_keys, TRUE);
  xptr_array_free (user_capable_apps_keys, TRUE);
  xptr_array_free (priority_capable_apps_keys, TRUE);
  xhash_table_unref (uwp_handler_table);

  return;
}

static void
watch_keys (void);

/* This function is called when any of our registry watchers detect
 * changes in the registry.
 */
static void
keys_updated (GWin32RegistryKey  *key,
              xpointer_t            user_data)
{
  watch_keys ();
  /* Indicate the tree as not up-to-date, push a new job for the AppInfo thread */
  g_atomic_int_inc (&gio_win32_appinfo_update_counter);
  /* We don't use the data pointer, but it must be non-NULL */
  xthread_pool_push (gio_win32_appinfo_threadpool, (xpointer_t) keys_updated, NULL);
}

static void
watch_keys (void)
{
  if (url_associations_key)
    g_win32_registry_key_watch (url_associations_key,
                                TRUE,
                                G_WIN32_REGISTRY_WATCH_NAME |
                                G_WIN32_REGISTRY_WATCH_ATTRIBUTES |
                                G_WIN32_REGISTRY_WATCH_VALUES,
                                keys_updated,
                                NULL,
                                NULL);

  if (file_exts_key)
    g_win32_registry_key_watch (file_exts_key,
                                TRUE,
                                G_WIN32_REGISTRY_WATCH_NAME |
                                G_WIN32_REGISTRY_WATCH_ATTRIBUTES |
                                G_WIN32_REGISTRY_WATCH_VALUES,
                                keys_updated,
                                NULL,
                                NULL);

  if (user_clients_key)
    g_win32_registry_key_watch (user_clients_key,
                                TRUE,
                                G_WIN32_REGISTRY_WATCH_NAME |
                                G_WIN32_REGISTRY_WATCH_ATTRIBUTES |
                                G_WIN32_REGISTRY_WATCH_VALUES,
                                keys_updated,
                                NULL,
                                NULL);

  if (system_clients_key)
    g_win32_registry_key_watch (system_clients_key,
                                TRUE,
                                G_WIN32_REGISTRY_WATCH_NAME |
                                G_WIN32_REGISTRY_WATCH_ATTRIBUTES |
                                G_WIN32_REGISTRY_WATCH_VALUES,
                                keys_updated,
                                NULL,
                                NULL);

  if (applications_key)
    g_win32_registry_key_watch (applications_key,
                                TRUE,
                                G_WIN32_REGISTRY_WATCH_NAME |
                                G_WIN32_REGISTRY_WATCH_ATTRIBUTES |
                                G_WIN32_REGISTRY_WATCH_VALUES,
                                keys_updated,
                                NULL,
                                NULL);

  if (user_registered_apps_key)
    g_win32_registry_key_watch (user_registered_apps_key,
                                TRUE,
                                G_WIN32_REGISTRY_WATCH_NAME |
                                G_WIN32_REGISTRY_WATCH_ATTRIBUTES |
                                G_WIN32_REGISTRY_WATCH_VALUES,
                                keys_updated,
                                NULL,
                                NULL);

  if (system_registered_apps_key)
    g_win32_registry_key_watch (system_registered_apps_key,
                                TRUE,
                                G_WIN32_REGISTRY_WATCH_NAME |
                                G_WIN32_REGISTRY_WATCH_ATTRIBUTES |
                                G_WIN32_REGISTRY_WATCH_VALUES,
                                keys_updated,
                                NULL,
                                NULL);

  if (classes_root_key)
    g_win32_registry_key_watch (classes_root_key,
                                FALSE,
                                G_WIN32_REGISTRY_WATCH_NAME |
                                G_WIN32_REGISTRY_WATCH_ATTRIBUTES |
                                G_WIN32_REGISTRY_WATCH_VALUES,
                                keys_updated,
                                NULL,
                                NULL);
}

/* This is the main function of the AppInfo thread */
static void
gio_win32_appinfo_thread_func (xpointer_t data,
                               xpointer_t user_data)
{
  xint_t saved_counter;
  g_mutex_lock (&gio_win32_appinfo_mutex);
  saved_counter = g_atomic_int_get (&gio_win32_appinfo_update_counter);

  if (saved_counter > 0)
    update_registry_data ();
  /* If the counter didn't change while we were working, then set it to zero.
   * Otherwise we need to rebuild the tree again, so keep it greater than zero.
   * Numeric value doesn't matter - even if we're asked to rebuild N times,
   * we just need to rebuild once, and as long as there were no new rebuild
   * requests while we were working, we're done.
   */
  if (g_atomic_int_compare_and_exchange  (&gio_win32_appinfo_update_counter,
                                          saved_counter,
                                          0))
    g_cond_broadcast (&gio_win32_appinfo_cond);

  g_mutex_unlock (&gio_win32_appinfo_mutex);
}

/* Initializes Windows AppInfo. Creates the registry watchers,
 * the AppInfo thread, and initiates an update of the AppInfo tree.
 * Called with do_wait = `FALSE` at startup to prevent it from
 * blocking until the tree is updated. All subsequent calls
 * from everywhere else are made with do_wait = `TRUE`, blocking
 * until the tree is re-built (if needed).
 */
void
gio_win32_appinfo_init (xboolean_t do_wait)
{
  static xsize_t initialized;

  if (g_once_init_enter (&initialized))
    {
      HMODULE gio_dll_extra;

      url_associations_key =
          g_win32_registry_key_new_w (L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations",
                                       NULL);
      file_exts_key =
          g_win32_registry_key_new_w (L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts",
                                       NULL);
      user_clients_key =
          g_win32_registry_key_new_w (L"HKEY_CURRENT_USER\\Software\\Clients",
                                       NULL);
      system_clients_key =
          g_win32_registry_key_new_w (L"HKEY_LOCAL_MACHINE\\Software\\Clients",
                                       NULL);
      applications_key =
          g_win32_registry_key_new_w (L"HKEY_CLASSES_ROOT\\Applications",
                                       NULL);
      user_registered_apps_key =
          g_win32_registry_key_new_w (L"HKEY_CURRENT_USER\\Software\\RegisteredApplications",
                                       NULL);
      system_registered_apps_key =
          g_win32_registry_key_new_w (L"HKEY_LOCAL_MACHINE\\Software\\RegisteredApplications",
                                       NULL);
      classes_root_key =
          g_win32_registry_key_new_w (L"HKEY_CLASSES_ROOT",
                                       NULL);

      watch_keys ();

      /* We don't really require an exclusive pool, but the implementation
       * details might cause the xthread_pool_push() call below to block
       * if the pool is not exclusive (specifically - for POSIX threads backend
       * lacking thread scheduler settings).
       */
      gio_win32_appinfo_threadpool = xthread_pool_new (gio_win32_appinfo_thread_func,
                                                        NULL,
                                                        1,
                                                        TRUE,
                                                        NULL);
      g_mutex_init (&gio_win32_appinfo_mutex);
      g_cond_init (&gio_win32_appinfo_cond);
      g_atomic_int_set (&gio_win32_appinfo_update_counter, 1);
      /* Trigger initial tree build. Fake data pointer. */
      xthread_pool_push (gio_win32_appinfo_threadpool, (xpointer_t) keys_updated, NULL);
      /* Increment the DLL refcount */
      GetModuleHandleExA (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                          (const char *) gio_win32_appinfo_init,
                          &gio_dll_extra);
      /* gio DLL cannot be unloaded now */

      g_once_init_leave (&initialized, TRUE);
    }

  if (!do_wait)
    return;

  /* Previously, we checked each of the watched keys here.
   * Now we just look at the update counter, because each key
   * has a change callback keys_updated, which increments this counter.
   */
  if (g_atomic_int_get (&gio_win32_appinfo_update_counter) > 0)
    {
      g_mutex_lock (&gio_win32_appinfo_mutex);
      while (g_atomic_int_get (&gio_win32_appinfo_update_counter) > 0)
        g_cond_wait (&gio_win32_appinfo_cond, &gio_win32_appinfo_mutex);
      g_mutex_unlock (&gio_win32_appinfo_mutex);
    }
}


static void g_win32_app_info_iface_init (xapp_info_iface_t *iface);

struct _GWin32AppInfo
{
  xobject_t parent_instance;

  /*<private>*/
  xchar_t **supported_types;

  GWin32AppInfoApplication *app;

  GWin32AppInfoHandler *handler;

  xuint_t startup_notify : 1;
};

G_DEFINE_TYPE_WITH_CODE (GWin32AppInfo, g_win32_app_info, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_APP_INFO,
                                                g_win32_app_info_iface_init))


static void
g_win32_app_info_finalize (xobject_t *object)
{
  GWin32AppInfo *info;

  info = G_WIN32_APP_INFO (object);

  g_clear_pointer (&info->supported_types, xstrfreev);
  g_clear_object (&info->app);
  g_clear_object (&info->handler);

  XOBJECT_CLASS (g_win32_app_info_parent_class)->finalize (object);
}

static void
g_win32_app_info_class_init (GWin32AppInfoClass *klass)
{
  xobject_class_t *xobject_class = XOBJECT_CLASS (klass);

  xobject_class->finalize = g_win32_app_info_finalize;
}

static void
g_win32_app_info_init (GWin32AppInfo *local)
{
}

static xapp_info_t *
g_win32_app_info_new_from_app (GWin32AppInfoApplication *app,
                               GWin32AppInfoHandler     *handler)
{
  GWin32AppInfo *new_info;
  xhash_table_iter_t iter;
  xpointer_t ext;
  int i;

  new_info = xobject_new (XTYPE_WIN32_APP_INFO, NULL);

  new_info->app = xobject_ref (app);

  gio_win32_appinfo_init (TRUE);
  g_mutex_lock (&gio_win32_appinfo_mutex);

  i = 0;
  xhash_table_iter_init (&iter, new_info->app->supported_exts);

  while (xhash_table_iter_next (&iter, &ext, NULL))
    {
      if (ext)
        i += 1;
    }

  new_info->supported_types = g_new (xchar_t *, i + 1);

  i = 0;
  xhash_table_iter_init (&iter, new_info->app->supported_exts);

  while (xhash_table_iter_next (&iter, &ext, NULL))
    {
      if (!ext)
        continue;

      new_info->supported_types[i] = xstrdup ((xchar_t *) ext);
      i += 1;
    }

  g_mutex_unlock (&gio_win32_appinfo_mutex);

  new_info->supported_types[i] = NULL;

  new_info->handler = handler ? xobject_ref (handler) : NULL;

  return G_APP_INFO (new_info);
}


static xapp_info_t *
g_win32_app_info_dup (xapp_info_t *appinfo)
{
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);
  GWin32AppInfo *new_info;

  new_info = xobject_new (XTYPE_WIN32_APP_INFO, NULL);

  if (info->app)
    new_info->app = xobject_ref (info->app);

  if (info->handler)
    new_info->handler = xobject_ref (info->handler);

  new_info->startup_notify = info->startup_notify;

  if (info->supported_types)
    {
      int i;

      for (i = 0; info->supported_types[i]; i++)
        break;

      new_info->supported_types = g_new (xchar_t *, i + 1);

      for (i = 0; info->supported_types[i]; i++)
        new_info->supported_types[i] = xstrdup (info->supported_types[i]);

      new_info->supported_types[i] = NULL;
    }

  return G_APP_INFO (new_info);
}

static xboolean_t
g_win32_app_info_equal (xapp_info_t *appinfo1,
                        xapp_info_t *appinfo2)
{
  GWin32AppInfoShellVerb *shverb1 = NULL;
  GWin32AppInfoShellVerb *shverb2 = NULL;
  GWin32AppInfo *info1 = G_WIN32_APP_INFO (appinfo1);
  GWin32AppInfo *info2 = G_WIN32_APP_INFO (appinfo2);
  GWin32AppInfoApplication *app1 = info1->app;
  GWin32AppInfoApplication *app2 = info2->app;

  if (app1 == NULL ||
      app2 == NULL)
    return info1 == info2;

  if (app1->canonical_name_folded != NULL &&
      app2->canonical_name_folded != NULL)
    return (xstrcmp0 (app1->canonical_name_folded,
                       app2->canonical_name_folded)) == 0;

  if (app1->verbs->len > 0 &&
      app2->verbs->len > 0)
    {
      shverb1 = _verb_idx (app1->verbs, 0);
      shverb2 = _verb_idx (app2->verbs, 0);
      if (shverb1->executable_folded != NULL &&
          shverb2->executable_folded != NULL)
        return (xstrcmp0 (shverb1->executable_folded,
                           shverb2->executable_folded)) == 0;
    }

  return app1 == app2;
}

static const char *
g_win32_app_info_get_id (xapp_info_t *appinfo)
{
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);
  GWin32AppInfoShellVerb *shverb;

  if (info->app == NULL)
    return NULL;

  if (info->app->canonical_name_u8)
    return info->app->canonical_name_u8;

  if (info->app->verbs->len > 0 &&
      (shverb = _verb_idx (info->app->verbs, 0))->executable_basename != NULL)
    return shverb->executable_basename;

  return NULL;
}

static const char *
g_win32_app_info_get_name (xapp_info_t *appinfo)
{
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);

  if (info->app && info->app->pretty_name_u8)
    return info->app->pretty_name_u8;
  else if (info->app && info->app->canonical_name_u8)
    return info->app->canonical_name_u8;
  else
    return P_("Unnamed");
}

static const char *
g_win32_app_info_get_display_name (xapp_info_t *appinfo)
{
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);

  if (info->app)
    {
      if (info->app->localized_pretty_name_u8)
        return info->app->localized_pretty_name_u8;
      else if (info->app->pretty_name_u8)
        return info->app->pretty_name_u8;
    }

  return g_win32_app_info_get_name (appinfo);
}

static const char *
g_win32_app_info_get_description (xapp_info_t *appinfo)
{
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);

  if (info->app == NULL)
    return NULL;

  return info->app->description_u8;
}

static const char *
g_win32_app_info_get_executable (xapp_info_t *appinfo)
{
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);

  if (info->app == NULL)
    return NULL;

  if (info->app->verbs->len > 0 && !info->app->is_uwp)
    return _verb_idx (info->app->verbs, 0)->executable;

  return NULL;
}

static const char *
g_win32_app_info_get_commandline (xapp_info_t *appinfo)
{
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);

  if (info->app == NULL)
    return NULL;

  if (info->app->verbs->len > 0 && !info->app->is_uwp)
    return _verb_idx (info->app->verbs, 0)->command_utf8;

  return NULL;
}

static xicon_t *
g_win32_app_info_get_icon (xapp_info_t *appinfo)
{
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);

  if (info->app == NULL)
    return NULL;

  return info->app->icon;
}

typedef struct _file_or_uri {
  xchar_t *uri;
  xchar_t *file;
} file_or_uri;

static char *
expand_macro_single (char macro, file_or_uri *obj)
{
  char *result = NULL;

  switch (macro)
    {
    case '*':
    case '0':
    case '1':
    case 'l':
    case 'd':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      /* TODO: handle 'l' and 'd' differently (longname and desktop name) */
      if (obj->uri)
        result = xstrdup (obj->uri);
      else if (obj->file)
        result = xstrdup (obj->file);
      break;
    case 'u':
    case 'U':
      if (obj->uri)
        result = g_shell_quote (obj->uri);
      break;
    case 'f':
    case 'F':
      if (obj->file)
        result = g_shell_quote (obj->file);
      break;
    }

  return result;
}

static xboolean_t
expand_macro (char               macro,
              xstring_t           *exec,
              GWin32AppInfo     *info,
              xlist_t            **stat_obj_list,
              xlist_t            **obj_list)
{
  xlist_t *objs = *obj_list;
  char *expanded;
  xboolean_t result = FALSE;

  xreturn_val_if_fail (exec != NULL, FALSE);

/*
Legend: (from http://msdn.microsoft.com/en-us/library/windows/desktop/cc144101%28v=vs.85%29.aspx)
%* - replace with all parameters
%~ - replace with all parameters starting with and following the second parameter
%0 or %1 the first file parameter. For example "C:\\Users\\Eric\\Destop\\New Text Document.txt". Generally this should be in quotes and the applications command line parsing should accept quotes to disambiguate files with spaces in the name and different command line parameters (this is a security best practice and I believe mentioned in MSDN).
%<n> (where N is 2 - 9), replace with the nth parameter
%s - show command
%h - hotkey value
%i - IDList stored in a shared memory handle is passed here.
%l - long file name form of the first parameter. Note win32 applications will be passed the long file name, win16 applications get the short file name. Specifying %L is preferred as it avoids the need to probe for the application type.
%d - desktop absolute parsing name of the first parameter (for items that don't have file system paths)
%v - for verbs that are none implies all, if there is no parameter passed this is the working directory
%w - the working directory
*/

  switch (macro)
    {
    case '*':
    case '~':
      if (*stat_obj_list)
        {
          xint_t i;
          xlist_t *o;

          for (o = *stat_obj_list, i = 0;
               macro == '~' && o && i < 2;
               o = o->next, i++);

          for (; o; o = o->next)
            {
              expanded = expand_macro_single (macro, o->data);

              if (expanded)
                {
                  if (o != *stat_obj_list)
                    xstring_append (exec, " ");

                  xstring_append (exec, expanded);
                  g_free (expanded);
                }
            }

          objs = NULL;
          result = TRUE;
        }
      break;
    case '0':
    case '1':
    case 'l':
    case 'd':
      if (*stat_obj_list)
        {
          xlist_t *o;

          o = *stat_obj_list;

          if (o)
            {
              expanded = expand_macro_single (macro, o->data);

              if (expanded)
                {
                  if (o != *stat_obj_list)
                    xstring_append (exec, " ");

                  xstring_append (exec, expanded);
                  g_free (expanded);
                }
            }

          if (objs)
            objs = objs->next;

          result = TRUE;
        }
      break;
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      if (*stat_obj_list)
        {
          xint_t i;
          xlist_t *o;
          xint_t n;

          switch (macro)
            {
            case '2':
              n = 2;
              break;
            case '3':
              n = 3;
              break;
            case '4':
              n = 4;
              break;
            case '5':
              n = 5;
              break;
            case '6':
              n = 6;
              break;
            case '7':
              n = 7;
              break;
            case '8':
              n = 8;
              break;
            case '9':
              n = 9;
              break;
            }

          for (o = *stat_obj_list, i = 0; o && i < n; o = o->next, i++);

          if (o)
            {
              expanded = expand_macro_single (macro, o->data);

              if (expanded)
                {
                  if (o != *stat_obj_list)
                    xstring_append (exec, " ");

                  xstring_append (exec, expanded);
                  g_free (expanded);
                }
            }
          result = TRUE;

          if (objs)
            objs = NULL;
        }
      break;
    case 's':
      break;
    case 'h':
      break;
    case 'i':
      break;
    case 'v':
      break;
    case 'w':
      expanded = g_get_current_dir ();
      xstring_append (exec, expanded);
      g_free (expanded);
      break;
    case 'u':
    case 'f':
      if (objs)
        {
          expanded = expand_macro_single (macro, objs->data);

          if (expanded)
            {
              xstring_append (exec, expanded);
              g_free (expanded);
            }
          objs = objs->next;
          result = TRUE;
        }

      break;

    case 'U':
    case 'F':
      while (objs)
        {
          expanded = expand_macro_single (macro, objs->data);

          if (expanded)
            {
              xstring_append (exec, expanded);
              g_free (expanded);
            }

          objs = objs->next;
          result = TRUE;

          if (objs != NULL && expanded)
            xstring_append_c (exec, ' ');
        }

      break;

    case 'c':
      if (info->app && info->app->localized_pretty_name_u8)
        {
          expanded = g_shell_quote (info->app->localized_pretty_name_u8);
          xstring_append (exec, expanded);
          g_free (expanded);
        }
      break;

    case 'm': /* deprecated */
    case 'n': /* deprecated */
    case 'N': /* deprecated */
    /*case 'd': *//* deprecated */
    case 'D': /* deprecated */
      break;

    case '%':
      xstring_append_c (exec, '%');
      break;
    }

  *obj_list = objs;

  return result;
}

static xboolean_t
expand_application_parameters (GWin32AppInfo   *info,
                               const xchar_t     *exec_line,
                               xlist_t          **objs,
                               int             *argc,
                               char          ***argv,
                               xerror_t         **error)
{
  xlist_t *obj_list = *objs;
  xlist_t **stat_obj_list = objs;
  const char *p = exec_line;
  xstring_t *expanded_exec;
  xboolean_t res;
  xchar_t *a_char;

  expanded_exec = xstring_new (NULL);
  res = FALSE;

  while (*p)
    {
      if (p[0] == '%' && p[1] != '\0')
        {
          if (expand_macro (p[1],
                            expanded_exec,
                            info, stat_obj_list,
                            objs))
            res = TRUE;

          p++;
        }
      else
        xstring_append_c (expanded_exec, *p);

      p++;
    }

  /* No file substitutions */
  if (obj_list == *objs && obj_list != NULL && !res)
    {
      /* If there is no macro default to %f. This is also what KDE does */
      xstring_append_c (expanded_exec, ' ');
      expand_macro ('f', expanded_exec, info, stat_obj_list, objs);
    }

  /* Replace '\\' with '/', because g_shell_parse_argv considers them
   * to be escape sequences.
   */
  for (a_char = expanded_exec->str;
       a_char <= &expanded_exec->str[expanded_exec->len];
       a_char++)
    {
      if (*a_char == '\\')
        *a_char = '/';
    }

  res = g_shell_parse_argv (expanded_exec->str, argc, argv, error);
  xstring_free (expanded_exec, TRUE);
  return res;
}


static xchar_t *
get_appath_for_exe (const xchar_t *exe_basename)
{
  GWin32RegistryKey *apppath_key = NULL;
  GWin32RegistryValueType val_type;
  xchar_t *appath = NULL;
  xboolean_t got_value;
  xchar_t *key_path = xstrdup_printf ("HKEY_LOCAL_MACHINE\\"
                                     "SOFTWARE\\"
                                     "Microsoft\\"
                                     "Windows\\"
                                     "CurrentVersion\\"
                                     "App Paths\\"
                                     "%s", exe_basename);

  apppath_key = g_win32_registry_key_new (key_path, NULL);
  g_clear_pointer (&key_path, g_free);

  if (apppath_key == NULL)
    return NULL;

  got_value = g_win32_registry_key_get_value (apppath_key,
                                              NULL,
                                              TRUE,
                                              "Path",
                                              &val_type,
                                              (void **) &appath,
                                              NULL,
                                              NULL);

  xobject_unref (apppath_key);

  if (got_value &&
      val_type == G_WIN32_REGISTRY_VALUE_STR)
    return appath;

  g_clear_pointer (&appath, g_free);

  return appath;
}


static xboolean_t
g_win32_app_info_launch_uwp_internal (GWin32AppInfo           *info,
                                      xboolean_t                 for_files,
                                      IShellItemArray         *items,
                                      GWin32AppInfoShellVerb  *shverb,
                                      xerror_t                 **error)
{
  DWORD pid;
  IApplicationActivationManager* paam = NULL;
  xboolean_t result = TRUE;
  HRESULT hr;

  hr = CoCreateInstance (&CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER, &IID_IApplicationActivationManager, (void **) &paam);
  if (FAILED (hr))
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Failed to create ApplicationActivationManager: 0x%lx", hr);
      return FALSE;
    }

  if (items == NULL)
    hr = IApplicationActivationManager_ActivateApplication (paam, (const wchar_t *) info->app->canonical_name, NULL, AO_NONE, &pid);
  else if (for_files)
    hr = IApplicationActivationManager_ActivateForFile (paam, (const wchar_t *) info->app->canonical_name, items, shverb->verb_name, &pid);
  else
    hr = IApplicationActivationManager_ActivateForProtocol (paam, (const wchar_t *) info->app->canonical_name, items, &pid);

  if (FAILED (hr))
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "The app %s failed to launch: 0x%lx",
                   g_win32_appinfo_application_get_some_name (info->app), hr);
      result = FALSE;
    }

  IApplicationActivationManager_Release (paam);

  return result;
}


static xboolean_t
g_win32_app_info_launch_internal (GWin32AppInfo      *info,
                                  xlist_t              *objs, /* non-UWP only */
                                  xboolean_t            for_files, /* UWP only */
                                  IShellItemArray    *items, /* UWP only */
                                  xapp_launch_context_t  *launch_context,
                                  GSpawnFlags         spawn_flags,
                                  xerror_t            **error)
{
  xboolean_t completed = FALSE;
  char **argv, **envp;
  int argc;
  const xchar_t *command;
  xchar_t *apppath;
  GWin32AppInfoShellVerb *shverb;

  xreturn_val_if_fail (info != NULL, FALSE);
  xreturn_val_if_fail (info->app != NULL, FALSE);

  argv = NULL;
  shverb = NULL;

  if (!info->app->is_uwp &&
      info->handler != NULL &&
      info->handler->verbs->len > 0)
    shverb = _verb_idx (info->handler->verbs, 0);
  else if (info->app->verbs->len > 0)
    shverb = _verb_idx (info->app->verbs, 0);

  if (shverb == NULL)
    {
      if (info->app->is_uwp || info->handler == NULL)
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     P_("The app ‘%s’ in the application object has no verbs"),
                     g_win32_appinfo_application_get_some_name (info->app));
      else if (info->handler->verbs->len == 0)
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     P_("The app ‘%s’ and the handler ‘%s’ in the application object have no verbs"),
                     g_win32_appinfo_application_get_some_name (info->app),
                     info->handler->handler_id_folded);

      return FALSE;
    }

  if (info->app->is_uwp)
    return g_win32_app_info_launch_uwp_internal (info,
                                                 for_files,
                                                 items,
                                                 shverb,
                                                 error);

  if (launch_context)
    envp = xapp_launch_context_get_environment (launch_context);
  else
    envp = g_get_environ ();

  xassert (shverb->command_utf8 != NULL);
  command = shverb->command_utf8;
  apppath = get_appath_for_exe (shverb->executable_basename);

  if (apppath)
    {
      xchar_t **p;
      xsize_t p_index;

      for (p = envp, p_index = 0; p[0]; p++, p_index++)
        if ((p[0][0] == 'p' || p[0][0] == 'P') &&
            (p[0][1] == 'a' || p[0][1] == 'A') &&
            (p[0][2] == 't' || p[0][2] == 'T') &&
            (p[0][3] == 'h' || p[0][3] == 'H') &&
            (p[0][4] == '='))
          break;

      if (p[0] == NULL)
        {
          xchar_t **new_envp;
          new_envp = g_new (char *, xstrv_length (envp) + 2);
          new_envp[0] = xstrdup_printf ("PATH=%s", apppath);

          for (p_index = 0; p_index <= xstrv_length (envp); p_index++)
            new_envp[1 + p_index] = envp[p_index];

          g_free (envp);
          envp = new_envp;
        }
      else
        {
          xchar_t *p_path;

          p_path = &p[0][5];

          if (p_path[0] != '\0')
            envp[p_index] = xstrdup_printf ("PATH=%s%c%s",
                                             apppath,
                                             G_SEARCHPATH_SEPARATOR,
                                             p_path);
          else
            envp[p_index] = xstrdup_printf ("PATH=%s", apppath);

          g_free (&p_path[-5]);
        }
    }

  do
    {
      xpid_t pid;

      if (!expand_application_parameters (info,
                                          command,
                                          &objs,
                                          &argc,
                                          &argv,
                                          error))
        goto out;

      if (!g_spawn_async (NULL,
                          argv,
                          envp,
                          spawn_flags,
                          NULL,
                          NULL,
                          &pid,
                          error))
          goto out;

      if (launch_context != NULL)
        {
          xvariant_builder_t builder;
          xvariant_t *platform_data;

          xvariant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);
          /* pid handles are never bigger than 2^24 as per
           * https://docs.microsoft.com/en-us/windows/win32/sysinfo/kernel-objects,
           * so truncating to `int32` is valid.
           * The xsize_t cast is to silence a compiler warning
           * about conversion from pointer to integer of
           * different size. */
          xvariant_builder_add (&builder, "{sv}", "pid", xvariant_new_int32 ((xsize_t) pid));

          platform_data = xvariant_ref_sink (xvariant_builder_end (&builder));
          xsignal_emit_by_name (launch_context, "launched", info, platform_data);
          xvariant_unref (platform_data);
        }

      xstrfreev (argv);
      argv = NULL;
    }
  while (objs != NULL);

  completed = TRUE;

 out:
  xstrfreev (argv);
  xstrfreev (envp);

  return completed;
}

static void
free_file_or_uri (xpointer_t ptr)
{
  file_or_uri *obj = ptr;
  g_free (obj->file);
  g_free (obj->uri);
  g_free (obj);
}


static xboolean_t
g_win32_app_supports_uris (GWin32AppInfoApplication *app)
{
  xssize_t num_of_uris_supported;

  if (app == NULL)
    return FALSE;

  num_of_uris_supported = (xssize_t) xhash_table_size (app->supported_urls);

  if (xhash_table_lookup (app->supported_urls, "file"))
    num_of_uris_supported -= 1;

  return num_of_uris_supported > 0;
}


static xboolean_t
g_win32_app_info_supports_uris (xapp_info_t *appinfo)
{
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);

  if (info->app == NULL)
    return FALSE;

  return g_win32_app_supports_uris (info->app);
}


static xboolean_t
g_win32_app_info_supports_files (xapp_info_t *appinfo)
{
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);

  if (info->app == NULL)
    return FALSE;

  return xhash_table_size (info->app->supported_exts) > 0;
}


static IShellItemArray *
make_item_array (xboolean_t   for_files,
                 xlist_t     *files_or_uris,
                 xerror_t   **error)
{
  ITEMIDLIST **item_ids;
  IShellItemArray *items;
  xlist_t *p;
  xsize_t count;
  xsize_t i;
  HRESULT hr;

  count = xlist_length (files_or_uris);

  items = NULL;
  item_ids = g_new (ITEMIDLIST*, count);

  for (i = 0, p = files_or_uris; p != NULL; p = p->next, i++)
    {
      wchar_t *file_or_uri_utf16;

      if (!for_files)
        file_or_uri_utf16 = xutf8_to_utf16 ((xchar_t *) p->data, -1, NULL, NULL, error);
      else
        file_or_uri_utf16 = xutf8_to_utf16 (xfile_peek_path (XFILE (p->data)), -1, NULL, NULL, error);

      if (file_or_uri_utf16 == NULL)
        break;

      if (for_files)
        {
          wchar_t *c;
          xsize_t len;
          xsize_t len_tail;

          len = wcslen (file_or_uri_utf16);
          /* Filenames *MUST* use single backslashes, else the call
           * will fail. First convert all slashes to backslashes,
           * then remove duplicates.
           */
          for (c = file_or_uri_utf16; for_files && *c != 0; c++)
            {
              if (*c == L'/')
                *c = L'\\';
            }
          for (len_tail = 0, c = &file_or_uri_utf16[len - 1];
               for_files && c > file_or_uri_utf16;
               c--, len_tail++)
            {
              if (c[0] != L'\\' || c[-1] != L'\\')
                continue;

              memmove (&c[-1], &c[0], len_tail * sizeof (wchar_t));
            }
        }

      hr = SHParseDisplayName (file_or_uri_utf16, NULL, &item_ids[i], 0, NULL);
      g_free (file_or_uri_utf16);

      if (FAILED (hr))
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "File or URI `%S' cannot be parsed by SHParseDisplayName: 0x%lx", file_or_uri_utf16, hr);
          break;
        }
    }

  if (i == count)
    {
      hr = SHCreateShellItemArrayFromIDLists (count, (const ITEMIDLIST **) item_ids, &items);
      if (FAILED (hr))
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "SHCreateShellItemArrayFromIDLists() failed: 0x%lx", hr);
          items = NULL;
        }
    }

  count = i;

  for (i = 0; i < count; i++)
    CoTaskMemFree (item_ids[i]);

  g_free (item_ids);

  return items;
}


static xboolean_t
g_win32_app_info_launch_uris (xapp_info_t           *appinfo,
                              xlist_t              *uris,
                              xapp_launch_context_t  *launch_context,
                              xerror_t            **error)
{
  xboolean_t res = FALSE;
  xboolean_t do_files;
  xlist_t *objs;
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);

  if (info->app != NULL && info->app->is_uwp)
    {
      IShellItemArray *items = NULL;

      if (uris)
        {
          items = make_item_array (FALSE, uris, error);
          if (items == NULL)
            return res;
        }

      res = g_win32_app_info_launch_internal (info, NULL, FALSE, items, launch_context, 0, error);

      if (items != NULL)
        IShellItemArray_Release (items);

      return res;
    }

  do_files = g_win32_app_info_supports_files (appinfo);

  objs = NULL;
  while (uris)
    {
      file_or_uri *obj;
      obj = g_new0 (file_or_uri, 1);

      if (do_files)
        {
          xfile_t *file;
          xchar_t *path;

          file = xfile_new_for_uri (uris->data);
          path = xfile_get_path (file);
          obj->file = path;
          xobject_unref (file);
        }

      obj->uri = xstrdup (uris->data);

      objs = xlist_prepend (objs, obj);
      uris = uris->next;
    }

  objs = xlist_reverse (objs);

  res = g_win32_app_info_launch_internal (info,
                                          objs,
                                          FALSE,
                                          NULL,
                                          launch_context,
                                          G_SPAWN_SEARCH_PATH,
                                          error);

  xlist_free_full (objs, free_file_or_uri);

  return res;
}

static xboolean_t
g_win32_app_info_should_show (xapp_info_t *appinfo)
{
  /* FIXME: This is a placeholder implementation to avoid crashes
   * for now. It can be made more specific to @appinfo in future. */

  return TRUE;
}

static xboolean_t
g_win32_app_info_launch (xapp_info_t           *appinfo,
                         xlist_t              *files,
                         xapp_launch_context_t  *launch_context,
                         xerror_t            **error)
{
  xboolean_t res = FALSE;
  xboolean_t do_uris;
  xlist_t *objs;
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);

  if (info->app != NULL && info->app->is_uwp)
    {
      IShellItemArray *items = NULL;

      if (files)
        {
          items = make_item_array (TRUE, files, error);
          if (items == NULL)
            return res;
        }

      res = g_win32_app_info_launch_internal (info, NULL, TRUE, items, launch_context, 0, error);

      if (items != NULL)
        IShellItemArray_Release (items);

      return res;
    }

  do_uris = g_win32_app_info_supports_uris (appinfo);

  objs = NULL;
  while (files)
    {
      file_or_uri *obj;
      obj = g_new0 (file_or_uri, 1);
      obj->file = xfile_get_path (XFILE (files->data));

      if (do_uris)
        obj->uri = xfile_get_uri (XFILE (files->data));

      objs = xlist_prepend (objs, obj);
      files = files->next;
    }

  objs = xlist_reverse (objs);

  res = g_win32_app_info_launch_internal (info,
                                          objs,
                                          TRUE,
                                          NULL,
                                          launch_context,
                                          G_SPAWN_SEARCH_PATH,
                                          error);

  xlist_free_full (objs, free_file_or_uri);

  return res;
}

static const char **
g_win32_app_info_get_supported_types (xapp_info_t *appinfo)
{
  GWin32AppInfo *info = G_WIN32_APP_INFO (appinfo);

  return (const char**) info->supported_types;
}

xapp_info_t *
xapp_info_create_from_commandline (const char           *commandline,
                                    const char           *application_name,
                                    GAppInfoCreateFlags   flags,
                                    xerror_t              **error)
{
  GWin32AppInfo *info;
  GWin32AppInfoApplication *app;
  xunichar2_t *app_command;

  xreturn_val_if_fail (commandline, NULL);

  app_command = xutf8_to_utf16 (commandline, -1, NULL, NULL, NULL);

  if (app_command == NULL)
    return NULL;

  info = xobject_new (XTYPE_WIN32_APP_INFO, NULL);
  app = xobject_new (XTYPE_WIN32_APPINFO_APPLICATION, NULL);

  app->no_open_with = FALSE;
  app->user_specific = FALSE;
  app->default_app = FALSE;

  if (application_name)
    {
      app->canonical_name = xutf8_to_utf16 (application_name,
                                             -1,
                                             NULL,
                                             NULL,
                                             NULL);
      app->canonical_name_u8 = xstrdup (application_name);
      app->canonical_name_folded = xutf8_casefold (application_name, -1);
    }

  app_add_verb (app,
                app,
                L"open",
                app_command,
                commandline,
                "open",
                TRUE,
                FALSE);

  g_clear_pointer (&app_command, g_free);
  info->app = app;
  info->handler = NULL;

  return G_APP_INFO (info);
}

/* xapp_info_t interface init */

static void
g_win32_app_info_iface_init (xapp_info_iface_t *iface)
{
  iface->dup = g_win32_app_info_dup;
  iface->equal = g_win32_app_info_equal;
  iface->get_id = g_win32_app_info_get_id;
  iface->get_name = g_win32_app_info_get_name;
  iface->get_description = g_win32_app_info_get_description;
  iface->get_executable = g_win32_app_info_get_executable;
  iface->get_icon = g_win32_app_info_get_icon;
  iface->launch = g_win32_app_info_launch;
  iface->supports_uris = g_win32_app_info_supports_uris;
  iface->supports_files = g_win32_app_info_supports_files;
  iface->launch_uris = g_win32_app_info_launch_uris;
  iface->should_show = g_win32_app_info_should_show;
/*  iface->set_as_default_for_type = g_win32_app_info_set_as_default_for_type;*/
/*  iface->set_as_default_for_extension = g_win32_app_info_set_as_default_for_extension;*/
/*  iface->add_supports_type = g_win32_app_info_add_supports_type;*/
/*  iface->can_remove_supports_type = g_win32_app_info_can_remove_supports_type;*/
/*  iface->remove_supports_type = g_win32_app_info_remove_supports_type;*/
/*  iface->can_delete = g_win32_app_info_can_delete;*/
/*  iface->do_delete = g_win32_app_info_delete;*/
  iface->get_commandline = g_win32_app_info_get_commandline;
  iface->get_display_name = g_win32_app_info_get_display_name;
/*  iface->set_as_last_used_for_type = g_win32_app_info_set_as_last_used_for_type;*/
  iface->get_supported_types = g_win32_app_info_get_supported_types;
}

xapp_info_t *
xapp_info_get_default_for_uri_scheme (const char *uri_scheme)
{
  GWin32AppInfoURLSchema *scheme = NULL;
  char *scheme_down;
  xapp_info_t *result;
  GWin32AppInfoShellVerb *shverb;

  scheme_down = xutf8_casefold (uri_scheme, -1);

  if (!scheme_down)
    return NULL;

  if (strcmp (scheme_down, "file") == 0)
    {
      g_free (scheme_down);

      return NULL;
    }

  gio_win32_appinfo_init (TRUE);
  g_mutex_lock (&gio_win32_appinfo_mutex);

  g_set_object (&scheme, xhash_table_lookup (urls, scheme_down));
  g_free (scheme_down);

  g_mutex_unlock (&gio_win32_appinfo_mutex);

  result = NULL;

  if (scheme != NULL &&
      scheme->chosen_handler != NULL &&
      scheme->chosen_handler->verbs->len > 0 &&
      (shverb = _verb_idx (scheme->chosen_handler->verbs, 0))->app != NULL)
    result = g_win32_app_info_new_from_app (shverb->app,
                                            scheme->chosen_handler);

  g_clear_object (&scheme);

  return result;
}

xapp_info_t *
xapp_info_get_default_for_type (const char *content_type,
                                 xboolean_t    must_support_uris)
{
  GWin32AppInfoFileExtension *ext = NULL;
  char *ext_down;
  xapp_info_t *result;
  GWin32AppInfoShellVerb *shverb;

  ext_down = xutf8_casefold (content_type, -1);

  if (!ext_down)
    return NULL;

  gio_win32_appinfo_init (TRUE);
  g_mutex_lock (&gio_win32_appinfo_mutex);

  /* Assuming that "content_type" is a file extension, not a MIME type */
  g_set_object (&ext, xhash_table_lookup (extensions, ext_down));
  g_free (ext_down);

  g_mutex_unlock (&gio_win32_appinfo_mutex);

  if (ext == NULL)
    return NULL;

  result = NULL;

  if (ext->chosen_handler != NULL &&
      ext->chosen_handler->verbs->len > 0 &&
      (shverb = _verb_idx (ext->chosen_handler->verbs, 0))->app != NULL &&
      (!must_support_uris ||
       g_win32_app_supports_uris (shverb->app)))
    result = g_win32_app_info_new_from_app (shverb->app,
                                            ext->chosen_handler);
  else
    {
      xhash_table_iter_t iter;
      GWin32AppInfoHandler *handler;

      xhash_table_iter_init (&iter, ext->handlers);

      while (result == NULL &&
             xhash_table_iter_next (&iter, NULL, (xpointer_t *) &handler))
        {
          if (handler->verbs->len == 0)
            continue;

          shverb = _verb_idx (handler->verbs, 0);

          if (shverb->app &&
              (!must_support_uris ||
               g_win32_app_supports_uris (shverb->app)))
            result = g_win32_app_info_new_from_app (shverb->app, handler);
        }
    }

  g_clear_object (&ext);

  return result;
}

xlist_t *
xapp_info_get_all (void)
{
  xhash_table_iter_t iter;
  xpointer_t value;
  xlist_t *infos;
  xlist_t *apps;
  xlist_t *apps_i;

  gio_win32_appinfo_init (TRUE);
  g_mutex_lock (&gio_win32_appinfo_mutex);

  apps = NULL;
  xhash_table_iter_init (&iter, apps_by_id);
  while (xhash_table_iter_next (&iter, NULL, &value))
    apps = xlist_prepend (apps, xobject_ref (G_OBJECT (value)));

  g_mutex_unlock (&gio_win32_appinfo_mutex);

  infos = NULL;
  for (apps_i = apps; apps_i; apps_i = apps_i->next)
    infos = xlist_prepend (infos,
                            g_win32_app_info_new_from_app (apps_i->data, NULL));

  xlist_free_full (apps, xobject_unref);

  return infos;
}

xlist_t *
xapp_info_get_all_for_type (const char *content_type)
{
  GWin32AppInfoFileExtension *ext = NULL;
  char *ext_down;
  GWin32AppInfoHandler *handler;
  xhash_table_iter_t iter;
  xhashtable_t *apps = NULL;
  xlist_t *result;
  GWin32AppInfoShellVerb *shverb;

  ext_down = xutf8_casefold (content_type, -1);

  if (!ext_down)
    return NULL;

  gio_win32_appinfo_init (TRUE);
  g_mutex_lock (&gio_win32_appinfo_mutex);

  /* Assuming that "content_type" is a file extension, not a MIME type */
  g_set_object (&ext, xhash_table_lookup (extensions, ext_down));
  g_free (ext_down);

  g_mutex_unlock (&gio_win32_appinfo_mutex);

  if (ext == NULL)
    return NULL;

  result = NULL;
  /* Used as a set to ensure uniqueness */
  apps = xhash_table_new (g_direct_hash, g_direct_equal);

  if (ext->chosen_handler != NULL &&
      ext->chosen_handler->verbs->len > 0 &&
      (shverb = _verb_idx (ext->chosen_handler->verbs, 0))->app != NULL)
    {
      xhash_table_add (apps, shverb->app);
      result = xlist_prepend (result,
                               g_win32_app_info_new_from_app (shverb->app,
                                                              ext->chosen_handler));
    }

  xhash_table_iter_init (&iter, ext->handlers);

  while (xhash_table_iter_next (&iter, NULL, (xpointer_t *) &handler))
    {
      xsize_t vi;

      for (vi = 0; vi < handler->verbs->len; vi++)
        {
          shverb = _verb_idx (handler->verbs, vi);

          if (shverb->app == NULL ||
              xhash_table_contains (apps, shverb->app))
            continue;

          xhash_table_add (apps, shverb->app);
          result = xlist_prepend (result,
                                   g_win32_app_info_new_from_app (shverb->app,
                                                                  handler));
        }
    }

  g_clear_object (&ext);
  result = xlist_reverse (result);
  xhash_table_unref (apps);

  return result;
}

xlist_t *
xapp_info_get_fallback_for_type (const xchar_t *content_type)
{
  /* TODO: fix this once gcontenttype support is improved */
  return xapp_info_get_all_for_type (content_type);
}

xlist_t *
xapp_info_get_recommended_for_type (const xchar_t *content_type)
{
  /* TODO: fix this once gcontenttype support is improved */
  return xapp_info_get_all_for_type (content_type);
}

void
xapp_info_reset_type_associations (const char *content_type)
{
  /* nothing to do */
}
