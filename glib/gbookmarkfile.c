/* gbookmarkfile.c: parsing and building desktop bookmarks
 *
 * Copyright (C) 2005-2006 Emmanuele Bassi
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

#include "config.h"

#include "gbookmarkfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <time.h>
#include <stdarg.h>

#include "gconvert.h"
#include "gdataset.h"
#include "gdatetime.h"
#include "gerror.h"
#include "gfileutils.h"
#include "ghash.h"
#include "glibintl.h"
#include "glist.h"
#include "gmain.h"
#include "gmarkup.h"
#include "gmem.h"
#include "gmessages.h"
#include "gshell.h"
#include "gslice.h"
#include "gstdio.h"
#include "gstring.h"
#include "gstrfuncs.h"
#include "gtimer.h"
#include "gutils.h"


/**
 * SECTION:bookmarkfile
 * @title: Bookmark file parser
 * @short_description: parses files containing bookmarks
 *
 * xbookmark_file_t lets you parse, edit or create files containing bookmarks
 * to URI, along with some meta-data about the resource pointed by the URI
 * like its MIME type, the application that is registering the bookmark and
 * the icon that should be used to represent the bookmark. The data is stored
 * using the
 * [Desktop Bookmark Specification](http://www.gnome.org/~ebassi/bookmark-spec).
 *
 * The syntax of the bookmark files is described in detail inside the
 * Desktop Bookmark Specification, here is a quick summary: bookmark
 * files use a sub-class of the XML Bookmark Exchange Language
 * specification, consisting of valid UTF-8 encoded XML, under the
 * <xbel> root element; each bookmark is stored inside a
 * <bookmark> element, using its URI: no relative paths can
 * be used inside a bookmark file. The bookmark may have a user defined
 * title and description, to be used instead of the URI. Under the
 * <metadata> element, with its owner attribute set to
 * `http://freedesktop.org`, is stored the meta-data about a resource
 * pointed by its URI. The meta-data consists of the resource's MIME
 * type; the applications that have registered a bookmark; the groups
 * to which a bookmark belongs to; a visibility flag, used to set the
 * bookmark as "private" to the applications and groups that has it
 * registered; the URI and MIME type of an icon, to be used when
 * displaying the bookmark inside a GUI.
 *
 * Here is an example of a bookmark file:
 * [bookmarks.xbel](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/glib/tests/bookmarks.xbel)
 *
 * A bookmark file might contain more than one bookmark; each bookmark
 * is accessed through its URI.
 *
 * The important caveat of bookmark files is that when you add a new
 * bookmark you must also add the application that is registering it, using
 * g_bookmark_file_add_application() or g_bookmark_file_set_application_info().
 * If a bookmark has no applications then it won't be dumped when creating
 * the on disk representation, using g_bookmark_file_to_data() or
 * g_bookmark_file_to_file().
 *
 * The #xbookmark_file_t parser was added in GLib 2.12.
 */

/* XBEL 1.0 standard entities */
#define XBEL_VERSION		"1.0"
#define XBEL_DTD_NICK		"xbel"
#define XBEL_DTD_SYSTEM		"+//IDN python.org//DTD XML Bookmark " \
				"Exchange Language 1.0//EN//XML"

#define XBEL_DTD_URI		"http://www.python.org/topics/xml/dtds/xbel-1.0.dtd"

#define XBEL_ROOT_ELEMENT	"xbel"
#define XBEL_FOLDER_ELEMENT	"folder" 	/* unused */
#define XBEL_BOOKMARK_ELEMENT	"bookmark"
#define XBEL_ALIAS_ELEMENT	"alias"		/* unused */
#define XBEL_SEPARATOR_ELEMENT	"separator" 	/* unused */
#define XBEL_TITLE_ELEMENT	"title"
#define XBEL_DESC_ELEMENT	"desc"
#define XBEL_INFO_ELEMENT	"info"
#define XBEL_METADATA_ELEMENT	"metadata"

#define XBEL_VERSION_ATTRIBUTE	"version"
#define XBEL_FOLDED_ATTRIBUTE	"folded" 	/* unused */
#define XBEL_OWNER_ATTRIBUTE	"owner"
#define XBEL_ADDED_ATTRIBUTE	"added"
#define XBEL_VISITED_ATTRIBUTE	"visited"
#define XBEL_MODIFIED_ATTRIBUTE "modified"
#define XBEL_ID_ATTRIBUTE	"id"
#define XBEL_HREF_ATTRIBUTE	"href"
#define XBEL_REF_ATTRIBUTE	"ref" 		/* unused */

#define XBEL_YES_VALUE		"yes"
#define XBEL_NO_VALUE		"no"

/* Desktop bookmark spec entities */
#define BOOKMARK_METADATA_OWNER 	"http://freedesktop.org"

#define BOOKMARK_NAMESPACE_NAME 	"bookmark"
#define BOOKMARK_NAMESPACE_URI		"http://www.freedesktop.org/standards/desktop-bookmarks"

#define BOOKMARK_GROUPS_ELEMENT		"groups"
#define BOOKMARK_GROUP_ELEMENT		"group"
#define BOOKMARK_APPLICATIONS_ELEMENT	"applications"
#define BOOKMARK_APPLICATION_ELEMENT	"application"
#define BOOKMARK_ICON_ELEMENT 		"icon"
#define BOOKMARK_PRIVATE_ELEMENT	"private"

#define BOOKMARK_NAME_ATTRIBUTE		"name"
#define BOOKMARK_EXEC_ATTRIBUTE		"exec"
#define BOOKMARK_COUNT_ATTRIBUTE 	"count"
#define BOOKMARK_TIMESTAMP_ATTRIBUTE	"timestamp"     /* deprecated by "modified" */
#define BOOKMARK_MODIFIED_ATTRIBUTE     "modified"
#define BOOKMARK_HREF_ATTRIBUTE 	"href"
#define BOOKMARK_TYPE_ATTRIBUTE 	"type"

/* Shared MIME Info entities */
#define MIME_NAMESPACE_NAME 		"mime"
#define MIME_NAMESPACE_URI 		"http://www.freedesktop.org/standards/shared-mime-info"
#define MIME_TYPE_ELEMENT 		"mime-type"
#define MIME_TYPE_ATTRIBUTE 		"type"


typedef struct _BookmarkAppInfo  BookmarkAppInfo;
typedef struct _BookmarkMetadata BookmarkMetadata;
typedef struct _BookmarkItem     BookmarkItem;
typedef struct _ParseData        ParseData;

struct _BookmarkAppInfo
{
  xchar_t *name;
  xchar_t *exec;

  xuint_t count;

  xdatetime_t *stamp;  /* (owned) */
};

struct _BookmarkMetadata
{
  xchar_t *mime_type;

  xlist_t *groups;

  xlist_t *applications;
  xhashtable_t *apps_by_name;

  xchar_t *icon_href;
  xchar_t *icon_mime;

  xuint_t is_private : 1;
};

struct _BookmarkItem
{
  xchar_t *uri;

  xchar_t *title;
  xchar_t *description;

  xdatetime_t *added;  /* (owned) */
  xdatetime_t *modified;  /* (owned) */
  xdatetime_t *visited;  /* (owned) */

  BookmarkMetadata *metadata;
};

struct _GBookmarkFile
{
  xchar_t *title;
  xchar_t *description;

  /* we store our items in a list and keep a copy inside
   * a hash table for faster lookup performances
   */
  xlist_t *items;
  xhashtable_t *items_by_uri;
};

/* parser state machine */
typedef enum
{
  STATE_STARTED        = 0,

  STATE_ROOT,
  STATE_BOOKMARK,
  STATE_TITLE,
  STATE_DESC,
  STATE_INFO,
  STATE_METADATA,
  STATE_APPLICATIONS,
  STATE_APPLICATION,
  STATE_GROUPS,
  STATE_GROUP,
  STATE_MIME,
  STATE_ICON,

  STATE_FINISHED
} ParserState;

static void          g_bookmark_file_init        (xbookmark_file_t  *bookmark);
static void          g_bookmark_file_clear       (xbookmark_file_t  *bookmark);
static xboolean_t      g_bookmark_file_parse       (xbookmark_file_t  *bookmark,
						  const xchar_t    *buffer,
						  xsize_t           length,
						  xerror_t        **error);
static xchar_t *       g_bookmark_file_dump        (xbookmark_file_t  *bookmark,
						  xsize_t          *length,
						  xerror_t        **error);
static BookmarkItem *g_bookmark_file_lookup_item (xbookmark_file_t  *bookmark,
						  const xchar_t    *uri);
static void          g_bookmark_file_add_item    (xbookmark_file_t  *bookmark,
						  BookmarkItem   *item,
						  xerror_t        **error);

static xboolean_t timestamp_from_iso8601 (const xchar_t  *iso_date,
                                        xdatetime_t   **out_date_time,
                                        xerror_t      **error);

/********************************
 * BookmarkAppInfo              *
 *                              *
 * Application metadata storage *
 ********************************/
static BookmarkAppInfo *
bookmark_app_info_new (const xchar_t *name)
{
  BookmarkAppInfo *retval;

  g_warn_if_fail (name != NULL);

  retval = g_slice_new (BookmarkAppInfo);

  retval->name = xstrdup (name);
  retval->exec = NULL;
  retval->count = 0;
  retval->stamp = NULL;

  return retval;
}

static void
bookmark_app_info_free (BookmarkAppInfo *app_info)
{
  if (!app_info)
    return;

  g_free (app_info->name);
  g_free (app_info->exec);
  g_clear_pointer (&app_info->stamp, xdate_time_unref);

  g_slice_free (BookmarkAppInfo, app_info);
}

static xchar_t *
bookmark_app_info_dump (BookmarkAppInfo *app_info)
{
  xchar_t *retval;
  xchar_t *name, *exec, *modified, *count;

  g_warn_if_fail (app_info != NULL);

  if (app_info->count == 0)
    return NULL;

  name = g_markup_escape_text (app_info->name, -1);
  exec = g_markup_escape_text (app_info->exec, -1);
  modified = xdate_time_format_iso8601 (app_info->stamp);
  count = xstrdup_printf ("%u", app_info->count);

  retval = xstrconcat ("          "
                        "<" BOOKMARK_NAMESPACE_NAME ":" BOOKMARK_APPLICATION_ELEMENT
                        " " BOOKMARK_NAME_ATTRIBUTE "=\"", name, "\""
                        " " BOOKMARK_EXEC_ATTRIBUTE "=\"", exec, "\""
                        " " BOOKMARK_MODIFIED_ATTRIBUTE "=\"", modified, "\""
                        " " BOOKMARK_COUNT_ATTRIBUTE "=\"", count, "\"/>\n",
                        NULL);

  g_free (name);
  g_free (exec);
  g_free (modified);
  g_free (count);

  return retval;
}


/***********************
 * BookmarkMetadata    *
 *                     *
 * Metadata storage    *
 ***********************/
static BookmarkMetadata *
bookmark_metadata_new (void)
{
  BookmarkMetadata *retval;

  retval = g_slice_new (BookmarkMetadata);

  retval->mime_type = NULL;

  retval->groups = NULL;

  retval->applications = NULL;
  retval->apps_by_name = xhash_table_new_full (xstr_hash,
                                                xstr_equal,
                                                NULL,
                                                NULL);

  retval->is_private = FALSE;

  retval->icon_href = NULL;
  retval->icon_mime = NULL;

  return retval;
}

static void
bookmark_metadata_free (BookmarkMetadata *metadata)
{
  if (!metadata)
    return;

  g_free (metadata->mime_type);

  xlist_free_full (metadata->groups, g_free);
  xlist_free_full (metadata->applications, (xdestroy_notify_t) bookmark_app_info_free);

  xhash_table_destroy (metadata->apps_by_name);

  g_free (metadata->icon_href);
  g_free (metadata->icon_mime);

  g_slice_free (BookmarkMetadata, metadata);
}

static xchar_t *
bookmark_metadata_dump (BookmarkMetadata *metadata)
{
  xstring_t *retval;
  xchar_t *buffer;

  if (!metadata->applications)
    return NULL;

  retval = xstring_sized_new (1024);

  /* metadata container */
  xstring_append (retval,
		   "      "
		   "<" XBEL_METADATA_ELEMENT
		   " " XBEL_OWNER_ATTRIBUTE "=\"" BOOKMARK_METADATA_OWNER
		   "\">\n");

  /* mime type */
  if (metadata->mime_type) {
    buffer = xstrconcat ("        "
			  "<" MIME_NAMESPACE_NAME ":" MIME_TYPE_ELEMENT " "
			  MIME_TYPE_ATTRIBUTE "=\"", metadata->mime_type, "\"/>\n",
			  NULL);
    xstring_append (retval, buffer);
    g_free (buffer);
  }

  if (metadata->groups)
    {
      xlist_t *l;

      /* open groups container */
      xstring_append (retval,
		       "        "
		       "<" BOOKMARK_NAMESPACE_NAME
		       ":" BOOKMARK_GROUPS_ELEMENT ">\n");

      for (l = xlist_last (metadata->groups); l != NULL; l = l->prev)
        {
          xchar_t *group_name;

	  group_name = g_markup_escape_text ((xchar_t *) l->data, -1);
	  buffer = xstrconcat ("          "
				"<" BOOKMARK_NAMESPACE_NAME
				":" BOOKMARK_GROUP_ELEMENT ">",
				group_name,
				"</" BOOKMARK_NAMESPACE_NAME
				":"  BOOKMARK_GROUP_ELEMENT ">\n", NULL);
	  xstring_append (retval, buffer);

	  g_free (buffer);
	  g_free (group_name);
        }

      /* close groups container */
      xstring_append (retval,
		       "        "
		       "</" BOOKMARK_NAMESPACE_NAME
		       ":" BOOKMARK_GROUPS_ELEMENT ">\n");
    }

  if (metadata->applications)
    {
      xlist_t *l;

      /* open applications container */
      xstring_append (retval,
		       "        "
		       "<" BOOKMARK_NAMESPACE_NAME
		       ":" BOOKMARK_APPLICATIONS_ELEMENT ">\n");

      for (l = xlist_last (metadata->applications); l != NULL; l = l->prev)
        {
          BookmarkAppInfo *app_info = (BookmarkAppInfo *) l->data;
          xchar_t *app_data;

	  g_warn_if_fail (app_info != NULL);

          app_data = bookmark_app_info_dump (app_info);

	  if (app_data)
            {
              retval = xstring_append (retval, app_data);

	      g_free (app_data);
	    }
        }

      /* close applications container */
      xstring_append (retval,
		       "        "
		       "</" BOOKMARK_NAMESPACE_NAME
		       ":" BOOKMARK_APPLICATIONS_ELEMENT ">\n");
    }

  /* icon */
  if (metadata->icon_href)
    {
      if (!metadata->icon_mime)
        metadata->icon_mime = xstrdup ("application/octet-stream");

      buffer = xstrconcat ("       "
			    "<" BOOKMARK_NAMESPACE_NAME
			    ":" BOOKMARK_ICON_ELEMENT
			    " " BOOKMARK_HREF_ATTRIBUTE "=\"", metadata->icon_href,
			    "\" " BOOKMARK_TYPE_ATTRIBUTE "=\"", metadata->icon_mime, "\"/>\n", NULL);
      xstring_append (retval, buffer);

      g_free (buffer);
    }

  /* private hint */
  if (metadata->is_private)
    xstring_append (retval,
		     "        "
		     "<" BOOKMARK_NAMESPACE_NAME
		     ":" BOOKMARK_PRIVATE_ELEMENT "/>\n");

  /* close metadata container */
  xstring_append (retval,
		   "      "
		   "</" XBEL_METADATA_ELEMENT ">\n");

  return xstring_free (retval, FALSE);
}

/******************************************************
 * BookmarkItem                                       *
 *                                                    *
 * Storage for a single bookmark item inside the list *
 ******************************************************/
static BookmarkItem *
bookmark_item_new (const xchar_t *uri)
{
  BookmarkItem *item;

  g_warn_if_fail (uri != NULL);

  item = g_slice_new (BookmarkItem);
  item->uri = xstrdup (uri);

  item->title = NULL;
  item->description = NULL;

  item->added = NULL;
  item->modified = NULL;
  item->visited = NULL;

  item->metadata = NULL;

  return item;
}

static void
bookmark_item_free (BookmarkItem *item)
{
  if (!item)
    return;

  g_free (item->uri);
  g_free (item->title);
  g_free (item->description);

  if (item->metadata)
    bookmark_metadata_free (item->metadata);

  g_clear_pointer (&item->added, xdate_time_unref);
  g_clear_pointer (&item->modified, xdate_time_unref);
  g_clear_pointer (&item->visited, xdate_time_unref);

  g_slice_free (BookmarkItem, item);
}

static void
bookmark_item_touch_modified (BookmarkItem *item)
{
  g_clear_pointer (&item->modified, xdate_time_unref);
  item->modified = xdate_time_new_now_utc ();
}

static xchar_t *
bookmark_item_dump (BookmarkItem *item)
{
  xstring_t *retval;
  xchar_t *escaped_uri;

  /* at this point, we must have at least a registered application; if we don't
   * we don't screw up the bookmark file, and just skip this item
   */
  if (!item->metadata || !item->metadata->applications)
    {
      g_warning ("Item for URI '%s' has no registered applications: skipping.", item->uri);
      return NULL;
    }

  retval = xstring_sized_new (4096);

  xstring_append (retval, "  <" XBEL_BOOKMARK_ELEMENT " ");

  escaped_uri = g_markup_escape_text (item->uri, -1);

  xstring_append (retval, XBEL_HREF_ATTRIBUTE "=\"");
  xstring_append (retval, escaped_uri);
  xstring_append (retval , "\" ");

  g_free (escaped_uri);

  if (item->added)
    {
      char *added;

      added = xdate_time_format_iso8601 (item->added);
      xstring_append (retval, XBEL_ADDED_ATTRIBUTE "=\"");
      xstring_append (retval, added);
      xstring_append (retval, "\" ");
      g_free (added);
    }

  if (item->modified)
    {
      char *modified;

      modified = xdate_time_format_iso8601 (item->modified);
      xstring_append (retval, XBEL_MODIFIED_ATTRIBUTE "=\"");
      xstring_append (retval, modified);
      xstring_append (retval, "\" ");
      g_free (modified);
    }

  if (item->visited)
    {
      char *visited;

      visited = xdate_time_format_iso8601 (item->visited);
      xstring_append (retval, XBEL_VISITED_ATTRIBUTE "=\"");
      xstring_append (retval, visited);
      xstring_append (retval, "\" ");
      g_free (visited);
    }

  if (retval->str[retval->len - 1] == ' ')
    xstring_truncate (retval, retval->len - 1);
  xstring_append (retval, ">\n");

  if (item->title)
    {
      xchar_t *escaped_title;

      escaped_title = g_markup_escape_text (item->title, -1);
      xstring_append (retval, "    " "<" XBEL_TITLE_ELEMENT ">");
      xstring_append (retval, escaped_title);
      xstring_append (retval, "</" XBEL_TITLE_ELEMENT ">\n");

      g_free (escaped_title);
    }

  if (item->description)
    {
      xchar_t *escaped_desc;

      escaped_desc = g_markup_escape_text (item->description, -1);
      xstring_append (retval, "    " "<" XBEL_DESC_ELEMENT ">");
      xstring_append (retval, escaped_desc);
      xstring_append (retval, "</" XBEL_DESC_ELEMENT ">\n");

      g_free (escaped_desc);
    }

  if (item->metadata)
    {
      xchar_t *metadata;

      metadata = bookmark_metadata_dump (item->metadata);
      if (metadata)
        {
          xstring_append (retval, "    " "<" XBEL_INFO_ELEMENT ">\n");
          xstring_append (retval, metadata);
          xstring_append (retval, "    " "</" XBEL_INFO_ELEMENT ">\n");

          g_free (metadata);
        }
    }

  xstring_append (retval, "  </" XBEL_BOOKMARK_ELEMENT ">\n");

  return xstring_free (retval, FALSE);
}

static BookmarkAppInfo *
bookmark_item_lookup_app_info (BookmarkItem *item,
			       const xchar_t  *app_name)
{
  g_warn_if_fail (item != NULL && app_name != NULL);

  if (!item->metadata)
    return NULL;

  return xhash_table_lookup (item->metadata->apps_by_name, app_name);
}

/*************************
 *    xbookmark_file_t    *
 *************************/

static void
g_bookmark_file_init (xbookmark_file_t *bookmark)
{
  bookmark->title = NULL;
  bookmark->description = NULL;

  bookmark->items = NULL;
  bookmark->items_by_uri = xhash_table_new_full (xstr_hash,
                                                  xstr_equal,
                                                  NULL,
                                                  NULL);
}

static void
g_bookmark_file_clear (xbookmark_file_t *bookmark)
{
  g_free (bookmark->title);
  g_free (bookmark->description);

  xlist_free_full (bookmark->items, (xdestroy_notify_t) bookmark_item_free);
  bookmark->items = NULL;

  if (bookmark->items_by_uri)
    {
      xhash_table_destroy (bookmark->items_by_uri);

      bookmark->items_by_uri = NULL;
    }
}

struct _ParseData
{
  ParserState state;

  xhashtable_t *namespaces;

  xbookmark_file_t *bookmark_file;
  BookmarkItem *current_item;
};

static ParseData *
parse_data_new (void)
{
  ParseData *retval;

  retval = g_new (ParseData, 1);

  retval->state = STATE_STARTED;
  retval->namespaces = xhash_table_new_full (xstr_hash, xstr_equal,
  					      (xdestroy_notify_t) g_free,
  					      (xdestroy_notify_t) g_free);
  retval->bookmark_file = NULL;
  retval->current_item = NULL;

  return retval;
}

static void
parse_data_free (ParseData *parse_data)
{
  xhash_table_destroy (parse_data->namespaces);

  g_free (parse_data);
}

#define IS_ATTRIBUTE(s,a)	((0 == strcmp ((s), (a))))

static void
parse_bookmark_element (xmarkup_parse_context_t  *context,
			ParseData            *parse_data,
			const xchar_t         **attribute_names,
			const xchar_t         **attribute_values,
			xerror_t              **error)
{
  const xchar_t *uri, *added, *modified, *visited;
  const xchar_t *attr;
  xint_t i;
  BookmarkItem *item;
  xerror_t *add_error;

  g_warn_if_fail ((parse_data != NULL) && (parse_data->state == STATE_BOOKMARK));

  i = 0;
  uri = added = modified = visited = NULL;
  for (attr = attribute_names[i]; attr != NULL; attr = attribute_names[++i])
    {
      if (IS_ATTRIBUTE (attr, XBEL_HREF_ATTRIBUTE))
        uri = attribute_values[i];
      else if (IS_ATTRIBUTE (attr, XBEL_ADDED_ATTRIBUTE))
        added = attribute_values[i];
      else if (IS_ATTRIBUTE (attr, XBEL_MODIFIED_ATTRIBUTE))
        modified = attribute_values[i];
      else if (IS_ATTRIBUTE (attr, XBEL_VISITED_ATTRIBUTE))
        visited = attribute_values[i];
      else
        {
          /* bookmark is defined by the XBEL spec, so we need
           * to error out if the element has different or
           * missing attributes
           */
          g_set_error (error, G_MARKUP_ERROR,
		       G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
          	       _("Unexpected attribute “%s” for element “%s”"),
          	       attr,
          	       XBEL_BOOKMARK_ELEMENT);
          return;
        }
    }

  if (!uri)
    {
      g_set_error (error, G_MARKUP_ERROR,
      		   G_MARKUP_ERROR_INVALID_CONTENT,
      		   _("Attribute “%s” of element “%s” not found"),
      		   XBEL_HREF_ATTRIBUTE,
      		   XBEL_BOOKMARK_ELEMENT);
      return;
    }

  g_warn_if_fail (parse_data->current_item == NULL);

  item = bookmark_item_new (uri);

  if (added != NULL && !timestamp_from_iso8601 (added, &item->added, error))
    {
      bookmark_item_free (item);
      return;
    }

  if (modified != NULL && !timestamp_from_iso8601 (modified, &item->modified, error))
    {
      bookmark_item_free (item);
      return;
    }

  if (visited != NULL && !timestamp_from_iso8601 (visited, &item->visited, error))
    {
      bookmark_item_free (item);
      return;
    }

  add_error = NULL;
  g_bookmark_file_add_item (parse_data->bookmark_file,
  			    item,
  			    &add_error);
  if (add_error)
    {
      bookmark_item_free (item);

      g_propagate_error (error, add_error);

      return;
    }

  parse_data->current_item = item;
}

static void
parse_application_element (xmarkup_parse_context_t  *context,
			   ParseData            *parse_data,
			   const xchar_t         **attribute_names,
			   const xchar_t         **attribute_values,
			   xerror_t              **error)
{
  const xchar_t *name, *exec, *count, *stamp, *modified;
  const xchar_t *attr;
  xint_t i;
  BookmarkItem *item;
  BookmarkAppInfo *ai;

  g_warn_if_fail ((parse_data != NULL) && (parse_data->state == STATE_APPLICATION));

  i = 0;
  name = exec = count = stamp = modified = NULL;
  for (attr = attribute_names[i]; attr != NULL; attr = attribute_names[++i])
    {
      if (IS_ATTRIBUTE (attr, BOOKMARK_NAME_ATTRIBUTE))
        name = attribute_values[i];
      else if (IS_ATTRIBUTE (attr, BOOKMARK_EXEC_ATTRIBUTE))
        exec = attribute_values[i];
      else if (IS_ATTRIBUTE (attr, BOOKMARK_COUNT_ATTRIBUTE))
        count = attribute_values[i];
      else if (IS_ATTRIBUTE (attr, BOOKMARK_TIMESTAMP_ATTRIBUTE))
        stamp = attribute_values[i];
      else if (IS_ATTRIBUTE (attr, BOOKMARK_MODIFIED_ATTRIBUTE))
        modified = attribute_values[i];
    }

  /* the "name" and "exec" attributes are mandatory */
  if (!name)
    {
      g_set_error (error, G_MARKUP_ERROR,
      		   G_MARKUP_ERROR_INVALID_CONTENT,
      		   _("Attribute “%s” of element “%s” not found"),
      		   BOOKMARK_NAME_ATTRIBUTE,
      		   BOOKMARK_APPLICATION_ELEMENT);
      return;
    }

  if (!exec)
    {
      g_set_error (error, G_MARKUP_ERROR,
      		   G_MARKUP_ERROR_INVALID_CONTENT,
      		   _("Attribute “%s” of element “%s” not found"),
      		   BOOKMARK_EXEC_ATTRIBUTE,
      		   BOOKMARK_APPLICATION_ELEMENT);
      return;
    }

  g_warn_if_fail (parse_data->current_item != NULL);
  item = parse_data->current_item;

  ai = bookmark_item_lookup_app_info (item, name);
  if (!ai)
    {
      ai = bookmark_app_info_new (name);

      if (!item->metadata)
	item->metadata = bookmark_metadata_new ();

      item->metadata->applications = xlist_prepend (item->metadata->applications, ai);
      xhash_table_replace (item->metadata->apps_by_name, ai->name, ai);
    }

  g_free (ai->exec);
  ai->exec = xstrdup (exec);

  if (count)
    ai->count = atoi (count);
  else
    ai->count = 1;

  g_clear_pointer (&ai->stamp, xdate_time_unref);
  if (modified != NULL)
    {
      if (!timestamp_from_iso8601 (modified, &ai->stamp, error))
        return;
    }
  else
    {
      /* the timestamp attribute has been deprecated but we still parse
       * it for backward compatibility
       */
      if (stamp)
        ai->stamp = xdate_time_new_from_unix_utc (atol (stamp));
      else
        ai->stamp = xdate_time_new_now_utc ();
    }
}

static void
parse_mime_type_element (xmarkup_parse_context_t  *context,
			 ParseData            *parse_data,
			 const xchar_t         **attribute_names,
			 const xchar_t         **attribute_values,
			 xerror_t              **error)
{
  const xchar_t *type;
  const xchar_t *attr;
  xint_t i;
  BookmarkItem *item;

  g_warn_if_fail ((parse_data != NULL) && (parse_data->state == STATE_MIME));

  i = 0;
  type = NULL;
  for (attr = attribute_names[i]; attr != NULL; attr = attribute_names[++i])
    {
      if (IS_ATTRIBUTE (attr, MIME_TYPE_ATTRIBUTE))
        type = attribute_values[i];
    }

  if (!type)
    type = "application/octet-stream";

  g_warn_if_fail (parse_data->current_item != NULL);
  item = parse_data->current_item;

  if (!item->metadata)
    item->metadata = bookmark_metadata_new ();

  g_free (item->metadata->mime_type);
  item->metadata->mime_type = xstrdup (type);
}

static void
parse_icon_element (xmarkup_parse_context_t  *context,
		    ParseData            *parse_data,
		    const xchar_t         **attribute_names,
		    const xchar_t         **attribute_values,
		    xerror_t              **error)
{
  const xchar_t *href;
  const xchar_t *type;
  const xchar_t *attr;
  xint_t i;
  BookmarkItem *item;

  g_warn_if_fail ((parse_data != NULL) && (parse_data->state == STATE_ICON));

  i = 0;
  href = NULL;
  type = NULL;
  for (attr = attribute_names[i]; attr != NULL; attr = attribute_names[++i])
    {
      if (IS_ATTRIBUTE (attr, BOOKMARK_HREF_ATTRIBUTE))
        href = attribute_values[i];
      else if (IS_ATTRIBUTE (attr, BOOKMARK_TYPE_ATTRIBUTE))
        type = attribute_values[i];
    }

  /* the "href" attribute is mandatory */
  if (!href)
    {
      g_set_error (error, G_MARKUP_ERROR,
      		   G_MARKUP_ERROR_INVALID_CONTENT,
      		   _("Attribute “%s” of element “%s” not found"),
      		   BOOKMARK_HREF_ATTRIBUTE,
      		   BOOKMARK_ICON_ELEMENT);
      return;
    }

  if (!type)
    type = "application/octet-stream";

  g_warn_if_fail (parse_data->current_item != NULL);
  item = parse_data->current_item;

  if (!item->metadata)
    item->metadata = bookmark_metadata_new ();

  g_free (item->metadata->icon_href);
  g_free (item->metadata->icon_mime);
  item->metadata->icon_href = xstrdup (href);
  item->metadata->icon_mime = xstrdup (type);
}

/* scans through the attributes of an element for the "xmlns" pragma, and
 * adds any resulting namespace declaration to a per-parser hashtable, using
 * the namespace name as a key for the namespace URI; if no key was found,
 * the namespace is considered as default, and stored under the "default" key.
 *
 * FIXME: this works on the assumption that the generator of the XBEL file
 * is either this code or is smart enough to place the namespace declarations
 * inside the main root node or inside the metadata node and does not redefine
 * a namespace inside an inner node; this does *not* conform to the
 * XML-NS standard, although is a close approximation.  In order to make this
 * conformant to the XML-NS specification we should use a per-element
 * namespace table inside GMarkup and ask it to resolve the namespaces for us.
 */
static void
map_namespace_to_name (ParseData    *parse_data,
                       const xchar_t **attribute_names,
		       const xchar_t **attribute_values)
{
  const xchar_t *attr;
  xint_t i;

  g_warn_if_fail (parse_data != NULL);

  if (!attribute_names || !attribute_names[0])
    return;

  i = 0;
  for (attr = attribute_names[i]; attr; attr = attribute_names[++i])
    {
      if (xstr_has_prefix (attr, "xmlns"))
        {
          xchar_t *namespace_name, *namespace_uri;
          xchar_t *p;

          p = xutf8_strchr (attr, -1, ':');
          if (p)
            p = xutf8_next_char (p);
          else
            p = "default";

          namespace_name = xstrdup (p);
          namespace_uri = xstrdup (attribute_values[i]);

          xhash_table_replace (parse_data->namespaces,
                                namespace_name,
                                namespace_uri);
        }
     }
}

/* checks whether @element_full is equal to @element.
 *
 * if @namespace is set, it tries to resolve the namespace to a known URI,
 * and if found is prepended to the element name, from which is separated
 * using the character specified in the @sep parameter.
 */
static xboolean_t
is_element_full (ParseData   *parse_data,
                 const xchar_t *element_full,
                 const xchar_t *namespace,
                 const xchar_t *element,
                 const xchar_t  sep)
{
  xchar_t *ns_uri, *ns_name;
  const xchar_t *p, *element_name;
  xboolean_t retval;

  g_warn_if_fail (parse_data != NULL);
  g_warn_if_fail (element_full != NULL);

  if (!element)
    return FALSE;

  /* no namespace requested: dumb element compare */
  if (!namespace)
    return (0 == strcmp (element_full, element));

  /* search for namespace separator; if none found, assume we are under the
   * default namespace, and set ns_name to our "default" marker; if no default
   * namespace has been set, just do a plain comparison between @full_element
   * and @element.
   */
  p = xutf8_strchr (element_full, -1, ':');
  if (p)
    {
      ns_name = xstrndup (element_full, p - element_full);
      element_name = xutf8_next_char (p);
    }
  else
    {
      ns_name = xstrdup ("default");
      element_name = element_full;
    }

  ns_uri = xhash_table_lookup (parse_data->namespaces, ns_name);
  if (!ns_uri)
    {
      /* no default namespace found */
      g_free (ns_name);

      return (0 == strcmp (element_full, element));
    }

  retval = (0 == strcmp (ns_uri, namespace) &&
            0 == strcmp (element_name, element));

  g_free (ns_name);

  return retval;
}

#define IS_ELEMENT(p,s,e)	(is_element_full ((p), (s), NULL, (e), '\0'))
#define IS_ELEMENT_NS(p,s,n,e)	(is_element_full ((p), (s), (n), (e), '|'))

static const xchar_t *
parser_state_to_element_name (ParserState state)
{
  switch (state)
    {
    case STATE_STARTED:
    case STATE_FINISHED:
      return "(top-level)";
    case STATE_ROOT:
      return XBEL_ROOT_ELEMENT;
    case STATE_BOOKMARK:
      return XBEL_BOOKMARK_ELEMENT;
    case STATE_TITLE:
      return XBEL_TITLE_ELEMENT;
    case STATE_DESC:
      return XBEL_DESC_ELEMENT;
    case STATE_INFO:
      return XBEL_INFO_ELEMENT;
    case STATE_METADATA:
      return XBEL_METADATA_ELEMENT;
    case STATE_APPLICATIONS:
      return BOOKMARK_APPLICATIONS_ELEMENT;
    case STATE_APPLICATION:
      return BOOKMARK_APPLICATION_ELEMENT;
    case STATE_GROUPS:
      return BOOKMARK_GROUPS_ELEMENT;
    case STATE_GROUP:
      return BOOKMARK_GROUP_ELEMENT;
    case STATE_MIME:
      return MIME_TYPE_ELEMENT;
    case STATE_ICON:
      return BOOKMARK_ICON_ELEMENT;
    default:
      g_assert_not_reached ();
    }
}

static void
start_element_raw_cb (xmarkup_parse_context_t *context,
                      const xchar_t         *element_name,
                      const xchar_t        **attribute_names,
                      const xchar_t        **attribute_values,
                      xpointer_t             user_data,
                      xerror_t             **error)
{
  ParseData *parse_data = (ParseData *) user_data;

  /* we must check for namespace declarations first
   *
   * XXX - we could speed up things by checking for namespace declarations
   * only on the root node, where they usually are; this would probably break
   * on streams not produced by us or by "smart" generators
   */
  map_namespace_to_name (parse_data, attribute_names, attribute_values);

  switch (parse_data->state)
    {
    case STATE_STARTED:
      if (IS_ELEMENT (parse_data, element_name, XBEL_ROOT_ELEMENT))
        {
          const xchar_t *attr;
          xint_t i;

          i = 0;
          for (attr = attribute_names[i]; attr; attr = attribute_names[++i])
            {
              if ((IS_ATTRIBUTE (attr, XBEL_VERSION_ATTRIBUTE)) &&
                  (0 == strcmp (attribute_values[i], XBEL_VERSION)))
                parse_data->state = STATE_ROOT;
            }
	}
      else
        g_set_error (error, G_MARKUP_ERROR,
		     G_MARKUP_ERROR_INVALID_CONTENT,
          	     _("Unexpected tag “%s”, tag “%s” expected"),
          	     element_name, XBEL_ROOT_ELEMENT);
      break;
    case STATE_ROOT:
      if (IS_ELEMENT (parse_data, element_name, XBEL_TITLE_ELEMENT))
        parse_data->state = STATE_TITLE;
      else if (IS_ELEMENT (parse_data, element_name, XBEL_DESC_ELEMENT))
        parse_data->state = STATE_DESC;
      else if (IS_ELEMENT (parse_data, element_name, XBEL_BOOKMARK_ELEMENT))
        {
          xerror_t *inner_error = NULL;

          parse_data->state = STATE_BOOKMARK;

          parse_bookmark_element (context,
          			  parse_data,
          			  attribute_names,
          			  attribute_values,
          			  &inner_error);
          if (inner_error)
            g_propagate_error (error, inner_error);
        }
      else
        g_set_error (error, G_MARKUP_ERROR,
        	     G_MARKUP_ERROR_INVALID_CONTENT,
        	     _("Unexpected tag “%s” inside “%s”"),
        	     element_name,
        	     XBEL_ROOT_ELEMENT);
      break;
    case STATE_BOOKMARK:
      if (IS_ELEMENT (parse_data, element_name, XBEL_TITLE_ELEMENT))
        parse_data->state = STATE_TITLE;
      else if (IS_ELEMENT (parse_data, element_name, XBEL_DESC_ELEMENT))
        parse_data->state = STATE_DESC;
      else if (IS_ELEMENT (parse_data, element_name, XBEL_INFO_ELEMENT))
        parse_data->state = STATE_INFO;
      else
        g_set_error (error, G_MARKUP_ERROR,
        	     G_MARKUP_ERROR_INVALID_CONTENT,
          	     _("Unexpected tag “%s” inside “%s”"),
          	     element_name,
          	     XBEL_BOOKMARK_ELEMENT);
      break;
    case STATE_INFO:
      if (IS_ELEMENT (parse_data, element_name, XBEL_METADATA_ELEMENT))
        {
          const xchar_t *attr;
          xint_t i;

          i = 0;
          for (attr = attribute_names[i]; attr; attr = attribute_names[++i])
            {
              if ((IS_ATTRIBUTE (attr, XBEL_OWNER_ATTRIBUTE)) &&
                  (0 == strcmp (attribute_values[i], BOOKMARK_METADATA_OWNER)))
                {
                  parse_data->state = STATE_METADATA;

                  if (!parse_data->current_item->metadata)
                    parse_data->current_item->metadata = bookmark_metadata_new ();
                }
            }
        }
      else
        g_set_error (error, G_MARKUP_ERROR,
        	     G_MARKUP_ERROR_INVALID_CONTENT,
        	     _("Unexpected tag “%s”, tag “%s” expected"),
        	     element_name,
        	     XBEL_METADATA_ELEMENT);
      break;
    case STATE_METADATA:
      if (IS_ELEMENT_NS (parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_APPLICATIONS_ELEMENT))
        parse_data->state = STATE_APPLICATIONS;
      else if (IS_ELEMENT_NS (parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_GROUPS_ELEMENT))
        parse_data->state = STATE_GROUPS;
      else if (IS_ELEMENT_NS (parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_PRIVATE_ELEMENT))
        parse_data->current_item->metadata->is_private = TRUE;
      else if (IS_ELEMENT_NS (parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_ICON_ELEMENT))
        {
          xerror_t *inner_error = NULL;

	  parse_data->state = STATE_ICON;

          parse_icon_element (context,
          		      parse_data,
          		      attribute_names,
          		      attribute_values,
          		      &inner_error);
          if (inner_error)
            g_propagate_error (error, inner_error);
        }
      else if (IS_ELEMENT_NS (parse_data, element_name, MIME_NAMESPACE_URI, MIME_TYPE_ELEMENT))
        {
          xerror_t *inner_error = NULL;

          parse_data->state = STATE_MIME;

          parse_mime_type_element (context,
          			   parse_data,
          			   attribute_names,
          			   attribute_values,
          			   &inner_error);
          if (inner_error)
            g_propagate_error (error, inner_error);
        }
      else
        g_set_error (error, G_MARKUP_ERROR,
        	     G_MARKUP_ERROR_UNKNOWN_ELEMENT,
        	     _("Unexpected tag “%s” inside “%s”"),
        	     element_name,
        	     XBEL_METADATA_ELEMENT);
      break;
    case STATE_APPLICATIONS:
      if (IS_ELEMENT_NS (parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_APPLICATION_ELEMENT))
        {
          xerror_t *inner_error = NULL;

          parse_data->state = STATE_APPLICATION;

          parse_application_element (context,
          			     parse_data,
          			     attribute_names,
          			     attribute_values,
          			     &inner_error);
          if (inner_error)
            g_propagate_error (error, inner_error);
        }
      else
        g_set_error (error, G_MARKUP_ERROR,
        	     G_MARKUP_ERROR_INVALID_CONTENT,
        	     _("Unexpected tag “%s”, tag “%s” expected"),
        	     element_name,
        	     BOOKMARK_APPLICATION_ELEMENT);
      break;
    case STATE_GROUPS:
      if (IS_ELEMENT_NS (parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_GROUP_ELEMENT))
        parse_data->state = STATE_GROUP;
      else
        g_set_error (error, G_MARKUP_ERROR,
        	     G_MARKUP_ERROR_INVALID_CONTENT,
        	     _("Unexpected tag “%s”, tag “%s” expected"),
        	     element_name,
        	     BOOKMARK_GROUP_ELEMENT);
      break;

    case STATE_TITLE:
    case STATE_DESC:
    case STATE_APPLICATION:
    case STATE_GROUP:
    case STATE_MIME:
    case STATE_ICON:
    case STATE_FINISHED:
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   _("Unexpected tag “%s” inside “%s”"),
                   element_name,
                   parser_state_to_element_name (parse_data->state));
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static void
end_element_raw_cb (xmarkup_parse_context_t *context,
                    const xchar_t         *element_name,
                    xpointer_t             user_data,
                    xerror_t             **error)
{
  ParseData *parse_data = (ParseData *) user_data;

  if (IS_ELEMENT (parse_data, element_name, XBEL_ROOT_ELEMENT))
    parse_data->state = STATE_FINISHED;
  else if (IS_ELEMENT (parse_data, element_name, XBEL_BOOKMARK_ELEMENT))
    {
      parse_data->current_item = NULL;

      parse_data->state = STATE_ROOT;
    }
  else if ((IS_ELEMENT (parse_data, element_name, XBEL_INFO_ELEMENT)) ||
           (IS_ELEMENT (parse_data, element_name, XBEL_TITLE_ELEMENT)) ||
           (IS_ELEMENT (parse_data, element_name, XBEL_DESC_ELEMENT)))
    {
      if (parse_data->current_item)
        parse_data->state = STATE_BOOKMARK;
      else
        parse_data->state = STATE_ROOT;
    }
  else if (IS_ELEMENT (parse_data, element_name, XBEL_METADATA_ELEMENT))
    parse_data->state = STATE_INFO;
  else if (IS_ELEMENT_NS (parse_data, element_name,
                          BOOKMARK_NAMESPACE_URI,
                          BOOKMARK_APPLICATION_ELEMENT))
    parse_data->state = STATE_APPLICATIONS;
  else if (IS_ELEMENT_NS (parse_data, element_name,
                          BOOKMARK_NAMESPACE_URI,
                          BOOKMARK_GROUP_ELEMENT))
    parse_data->state = STATE_GROUPS;
  else if ((IS_ELEMENT_NS (parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_APPLICATIONS_ELEMENT)) ||
           (IS_ELEMENT_NS (parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_GROUPS_ELEMENT)) ||
           (IS_ELEMENT_NS (parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_PRIVATE_ELEMENT)) ||
           (IS_ELEMENT_NS (parse_data, element_name, BOOKMARK_NAMESPACE_URI, BOOKMARK_ICON_ELEMENT)) ||
           (IS_ELEMENT_NS (parse_data, element_name, MIME_NAMESPACE_URI, MIME_TYPE_ELEMENT)))
    parse_data->state = STATE_METADATA;
}

static void
text_raw_cb (xmarkup_parse_context_t *context,
             const xchar_t         *text,
             xsize_t                length,
             xpointer_t             user_data,
             xerror_t             **error)
{
  ParseData *parse_data = (ParseData *) user_data;
  xchar_t *payload;

  payload = xstrndup (text, length);

  switch (parse_data->state)
    {
    case STATE_TITLE:
      if (parse_data->current_item)
        {
          g_free (parse_data->current_item->title);
          parse_data->current_item->title = xstrdup (payload);
        }
      else
        {
          g_free (parse_data->bookmark_file->title);
          parse_data->bookmark_file->title = xstrdup (payload);
        }
      break;
    case STATE_DESC:
      if (parse_data->current_item)
        {
          g_free (parse_data->current_item->description);
          parse_data->current_item->description = xstrdup (payload);
        }
      else
        {
          g_free (parse_data->bookmark_file->description);
          parse_data->bookmark_file->description = xstrdup (payload);
        }
      break;
    case STATE_GROUP:
      {
      xlist_t *groups;

      g_warn_if_fail (parse_data->current_item != NULL);

      if (!parse_data->current_item->metadata)
        parse_data->current_item->metadata = bookmark_metadata_new ();

      groups = parse_data->current_item->metadata->groups;
      parse_data->current_item->metadata->groups = xlist_prepend (groups, xstrdup (payload));
      }
      break;
    case STATE_ROOT:
    case STATE_BOOKMARK:
    case STATE_INFO:
    case STATE_METADATA:
    case STATE_APPLICATIONS:
    case STATE_APPLICATION:
    case STATE_GROUPS:
    case STATE_MIME:
    case STATE_ICON:
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  g_free (payload);
}

static const GMarkupParser markup_parser =
{
  start_element_raw_cb, /* start_element */
  end_element_raw_cb,   /* end_element */
  text_raw_cb,          /* text */
  NULL,                 /* passthrough */
  NULL
};

static xboolean_t
g_bookmark_file_parse (xbookmark_file_t  *bookmark,
			 const xchar_t  *buffer,
			 xsize_t         length,
			 xerror_t       **error)
{
  xmarkup_parse_context_t *context;
  ParseData *parse_data;
  xerror_t *parse_error, *end_error;
  xboolean_t retval;

  g_warn_if_fail (bookmark != NULL);

  if (!buffer)
    return FALSE;

  parse_error = NULL;
  end_error = NULL;

  if (length == (xsize_t) -1)
    length = strlen (buffer);

  parse_data = parse_data_new ();
  parse_data->bookmark_file = bookmark;

  context = xmarkup_parse_context_new (&markup_parser,
  					0,
  					parse_data,
  					(xdestroy_notify_t) parse_data_free);

  retval = xmarkup_parse_context_parse (context,
  					 buffer,
  					 length,
  					 &parse_error);
  if (!retval)
    g_propagate_error (error, parse_error);
  else
   {
     retval = xmarkup_parse_context_end_parse (context, &end_error);
      if (!retval)
        g_propagate_error (error, end_error);
   }

  xmarkup_parse_context_free (context);

  return retval;
}

static xchar_t *
g_bookmark_file_dump (xbookmark_file_t  *bookmark,
		      xsize_t          *length,
		      xerror_t        **error)
{
  xstring_t *retval;
  xchar_t *buffer;
  xlist_t *l;

  retval = xstring_sized_new (4096);

  xstring_append (retval,
		   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
#if 0
		   /* XXX - do we really need the doctype? */
		   "<!DOCTYPE " XBEL_DTD_NICK "\n"
		   "  PUBLIC \"" XBEL_DTD_SYSTEM "\"\n"
		   "         \"" XBEL_DTD_URI "\">\n"
#endif
		   "<" XBEL_ROOT_ELEMENT " " XBEL_VERSION_ATTRIBUTE "=\"" XBEL_VERSION "\"\n"
		   "      xmlns:" BOOKMARK_NAMESPACE_NAME "=\"" BOOKMARK_NAMESPACE_URI "\"\n"
		   "      xmlns:" MIME_NAMESPACE_NAME     "=\"" MIME_NAMESPACE_URI "\"\n>");

  if (bookmark->title)
    {
      xchar_t *escaped_title;

      escaped_title = g_markup_escape_text (bookmark->title, -1);

      buffer = xstrconcat ("  "
			    "<" XBEL_TITLE_ELEMENT ">",
			    escaped_title,
			    "</" XBEL_TITLE_ELEMENT ">\n", NULL);

      xstring_append (retval, buffer);

      g_free (buffer);
      g_free (escaped_title);
    }

  if (bookmark->description)
    {
      xchar_t *escaped_desc;

      escaped_desc = g_markup_escape_text (bookmark->description, -1);

      buffer = xstrconcat ("  "
			    "<" XBEL_DESC_ELEMENT ">",
			    escaped_desc,
			    "</" XBEL_DESC_ELEMENT ">\n", NULL);
      xstring_append (retval, buffer);

      g_free (buffer);
      g_free (escaped_desc);
    }

  if (!bookmark->items)
    goto out;
  else
    retval = xstring_append (retval, "\n");

  /* the items are stored in reverse order */
  for (l = xlist_last (bookmark->items);
       l != NULL;
       l = l->prev)
    {
      BookmarkItem *item = (BookmarkItem *) l->data;
      xchar_t *item_dump;

      item_dump = bookmark_item_dump (item);
      if (!item_dump)
        continue;

      retval = xstring_append (retval, item_dump);

      g_free (item_dump);
    }

out:
  xstring_append (retval, "</" XBEL_ROOT_ELEMENT ">");

  if (length)
    *length = retval->len;

  return xstring_free (retval, FALSE);
}

/**************
 *    Misc    *
 **************/

static xboolean_t
timestamp_from_iso8601 (const xchar_t  *iso_date,
                        xdatetime_t   **out_date_time,
                        xerror_t      **error)
{
  xdatetime_t *dt = xdate_time_new_from_iso8601 (iso_date, NULL);
  if (dt == NULL)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR, G_BOOKMARK_FILE_ERROR_READ,
                   _("Invalid date/time ‘%s’ in bookmark file"), iso_date);
      return FALSE;
    }

  *out_date_time = g_steal_pointer (&dt);
  return TRUE;
}

G_DEFINE_QUARK (g-bookmark-file-error-quark, g_bookmark_file_error)

/********************
 *    Public API    *
 ********************/

/**
 * g_bookmark_file_new: (constructor)
 *
 * Creates a new empty #xbookmark_file_t object.
 *
 * Use g_bookmark_file_load_from_file(), g_bookmark_file_load_from_data()
 * or g_bookmark_file_load_from_data_dirs() to read an existing bookmark
 * file.
 *
 * Returns: an empty #xbookmark_file_t
 *
 * Since: 2.12
 */
xbookmark_file_t *
g_bookmark_file_new (void)
{
  xbookmark_file_t *bookmark;

  bookmark = g_new (xbookmark_file_t, 1);

  g_bookmark_file_init (bookmark);

  return bookmark;
}

/**
 * g_bookmark_file_free:
 * @bookmark: a #xbookmark_file_t
 *
 * Frees a #xbookmark_file_t.
 *
 * Since: 2.12
 */
void
g_bookmark_file_free (xbookmark_file_t *bookmark)
{
  if (!bookmark)
    return;

  g_bookmark_file_clear (bookmark);

  g_free (bookmark);
}

/**
 * g_bookmark_file_load_from_data:
 * @bookmark: an empty #xbookmark_file_t struct
 * @data: (array length=length) (element-type xuint8_t): desktop bookmarks
 *    loaded in memory
 * @length: the length of @data in bytes
 * @error: return location for a #xerror_t, or %NULL
 *
 * Loads a bookmark file from memory into an empty #xbookmark_file_t
 * structure.  If the object cannot be created then @error is set to a
 * #GBookmarkFileError.
 *
 * Returns: %TRUE if a desktop bookmark could be loaded.
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_load_from_data (xbookmark_file_t  *bookmark,
				const xchar_t    *data,
				xsize_t           length,
				xerror_t        **error)
{
  xerror_t *parse_error;
  xboolean_t retval;

  g_return_val_if_fail (bookmark != NULL, FALSE);

  if (length == (xsize_t) -1)
    length = strlen (data);

  if (bookmark->items)
    {
      g_bookmark_file_clear (bookmark);
      g_bookmark_file_init (bookmark);
    }

  parse_error = NULL;
  retval = g_bookmark_file_parse (bookmark, data, length, &parse_error);

  if (!retval)
    g_propagate_error (error, parse_error);

  return retval;
}

/**
 * g_bookmark_file_load_from_file:
 * @bookmark: an empty #xbookmark_file_t struct
 * @filename: (type filename): the path of a filename to load, in the
 *     GLib file name encoding
 * @error: return location for a #xerror_t, or %NULL
 *
 * Loads a desktop bookmark file into an empty #xbookmark_file_t structure.
 * If the file could not be loaded then @error is set to either a #GFileError
 * or #GBookmarkFileError.
 *
 * Returns: %TRUE if a desktop bookmark file could be loaded
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_load_from_file (xbookmark_file_t  *bookmark,
				const xchar_t    *filename,
				xerror_t        **error)
{
  xboolean_t ret = FALSE;
  xchar_t *buffer = NULL;
  xsize_t len;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  if (!xfile_get_contents (filename, &buffer, &len, error))
    goto out;

  if (!g_bookmark_file_load_from_data (bookmark, buffer, len, error))
    goto out;

  ret = TRUE;
 out:
  g_free (buffer);
  return ret;
}


/* Iterates through all the directories in *dirs trying to
 * find file.  When it successfully locates file, returns a
 * string its absolute path.  It also leaves the unchecked
 * directories in *dirs.  You should free the returned string
 *
 * Adapted from gkeyfile.c
 */
static xchar_t *
find_file_in_data_dirs (const xchar_t   *file,
                        xchar_t       ***dirs,
                        xerror_t       **error)
{
  xchar_t **data_dirs, *data_dir, *path;

  path = NULL;

  if (dirs == NULL)
    return NULL;

  data_dirs = *dirs;
  path = NULL;
  while (data_dirs && (data_dir = *data_dirs) && !path)
    {
      xchar_t *candidate_file, *sub_dir;

      candidate_file = (xchar_t *) file;
      sub_dir = xstrdup ("");
      while (candidate_file != NULL && !path)
        {
          xchar_t *p;

          path = g_build_filename (data_dir, sub_dir,
                                   candidate_file, NULL);

          candidate_file = strchr (candidate_file, '-');

          if (candidate_file == NULL)
            break;

          candidate_file++;

          g_free (sub_dir);
          sub_dir = xstrndup (file, candidate_file - file - 1);

          for (p = sub_dir; *p != '\0'; p++)
            {
              if (*p == '-')
                *p = G_DIR_SEPARATOR;
            }
        }
      g_free (sub_dir);
      data_dirs++;
    }

  *dirs = data_dirs;

  if (!path)
    {
      g_set_error_literal (error, G_BOOKMARK_FILE_ERROR,
                           G_BOOKMARK_FILE_ERROR_FILE_NOT_FOUND,
                           _("No valid bookmark file found in data dirs"));

      return NULL;
    }

  return path;
}


/**
 * g_bookmark_file_load_from_data_dirs:
 * @bookmark: a #xbookmark_file_t
 * @file: (type filename): a relative path to a filename to open and parse
 * @full_path: (out) (optional) (type filename): return location for a string
 *    containing the full path of the file, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * This function looks for a desktop bookmark file named @file in the
 * paths returned from g_get_user_data_dir() and g_get_system_data_dirs(),
 * loads the file into @bookmark and returns the file's full path in
 * @full_path.  If the file could not be loaded then @error is
 * set to either a #GFileError or #GBookmarkFileError.
 *
 * Returns: %TRUE if a key file could be loaded, %FALSE otherwise
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_load_from_data_dirs (xbookmark_file_t  *bookmark,
				     const xchar_t    *file,
				     xchar_t         **full_path,
				     xerror_t        **error)
{
  xerror_t *file_error = NULL;
  xchar_t **all_data_dirs, **data_dirs;
  const xchar_t *user_data_dir;
  const xchar_t * const * system_data_dirs;
  xsize_t i, j;
  xchar_t *output_path;
  xboolean_t found_file;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (!g_path_is_absolute (file), FALSE);

  user_data_dir = g_get_user_data_dir ();
  system_data_dirs = g_get_system_data_dirs ();
  all_data_dirs = g_new0 (xchar_t *, xstrv_length ((xchar_t **)system_data_dirs) + 2);

  i = 0;
  all_data_dirs[i++] = xstrdup (user_data_dir);

  j = 0;
  while (system_data_dirs[j] != NULL)
    all_data_dirs[i++] = xstrdup (system_data_dirs[j++]);

  found_file = FALSE;
  data_dirs = all_data_dirs;
  output_path = NULL;
  while (*data_dirs != NULL && !found_file)
    {
      g_free (output_path);

      output_path = find_file_in_data_dirs (file, &data_dirs, &file_error);

      if (file_error)
        {
          g_propagate_error (error, file_error);
 	  break;
        }

      found_file = g_bookmark_file_load_from_file (bookmark,
      						   output_path,
      						   &file_error);
      if (file_error)
        {
	  g_propagate_error (error, file_error);
	  break;
        }
    }

  if (found_file && full_path)
    *full_path = output_path;
  else
    g_free (output_path);

  xstrfreev (all_data_dirs);

  return found_file;
}


/**
 * g_bookmark_file_to_data:
 * @bookmark: a #xbookmark_file_t
 * @length: (out) (optional): return location for the length of the returned string, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * This function outputs @bookmark as a string.
 *
 * Returns: (transfer full) (array length=length) (element-type xuint8_t):
 *   a newly allocated string holding the contents of the #xbookmark_file_t
 *
 * Since: 2.12
 */
xchar_t *
g_bookmark_file_to_data (xbookmark_file_t  *bookmark,
			 xsize_t          *length,
			 xerror_t        **error)
{
  xerror_t *write_error = NULL;
  xchar_t *retval;

  g_return_val_if_fail (bookmark != NULL, NULL);

  retval = g_bookmark_file_dump (bookmark, length, &write_error);
  if (write_error)
    {
      g_propagate_error (error, write_error);

      return NULL;
    }

  return retval;
}

/**
 * g_bookmark_file_to_file:
 * @bookmark: a #xbookmark_file_t
 * @filename: (type filename): path of the output file
 * @error: return location for a #xerror_t, or %NULL
 *
 * This function outputs @bookmark into a file.  The write process is
 * guaranteed to be atomic by using xfile_set_contents() internally.
 *
 * Returns: %TRUE if the file was successfully written.
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_to_file (xbookmark_file_t  *bookmark,
			 const xchar_t    *filename,
			 xerror_t        **error)
{
  xchar_t *data;
  xerror_t *data_error, *write_error;
  xsize_t len;
  xboolean_t retval;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  data_error = NULL;
  data = g_bookmark_file_to_data (bookmark, &len, &data_error);
  if (data_error)
    {
      g_propagate_error (error, data_error);

      return FALSE;
    }

  write_error = NULL;
  xfile_set_contents (filename, data, len, &write_error);
  if (write_error)
    {
      g_propagate_error (error, write_error);

      retval = FALSE;
    }
  else
    retval = TRUE;

  g_free (data);

  return retval;
}

static BookmarkItem *
g_bookmark_file_lookup_item (xbookmark_file_t *bookmark,
			     const xchar_t   *uri)
{
  g_warn_if_fail (bookmark != NULL && uri != NULL);

  return xhash_table_lookup (bookmark->items_by_uri, uri);
}

/* this function adds a new item to the list */
static void
g_bookmark_file_add_item (xbookmark_file_t  *bookmark,
			  BookmarkItem   *item,
			  xerror_t        **error)
{
  g_warn_if_fail (bookmark != NULL);
  g_warn_if_fail (item != NULL);

  /* this should never happen; and if it does, then we are
   * screwing up something big time.
   */
  if (G_UNLIKELY (g_bookmark_file_has_item (bookmark, item->uri)))
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_INVALID_URI,
		   _("A bookmark for URI “%s” already exists"),
		   item->uri);
      return;
    }

  bookmark->items = xlist_prepend (bookmark->items, item);

  xhash_table_replace (bookmark->items_by_uri,
			item->uri,
			item);

  if (item->added == NULL)
    item->added = xdate_time_new_now_utc ();

  if (item->modified == NULL)
    item->modified = xdate_time_new_now_utc ();

  if (item->visited == NULL)
    item->visited = xdate_time_new_now_utc ();
}

/**
 * g_bookmark_file_remove_item:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @error: return location for a #xerror_t, or %NULL
 *
 * Removes the bookmark for @uri from the bookmark file @bookmark.
 *
 * Returns: %TRUE if the bookmark was removed successfully.
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_remove_item (xbookmark_file_t  *bookmark,
			     const xchar_t    *uri,
			     xerror_t        **error)
{
  BookmarkItem *item;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  item = g_bookmark_file_lookup_item (bookmark, uri);

  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   uri);
      return FALSE;
    }

  bookmark->items = xlist_remove (bookmark->items, item);
  xhash_table_remove (bookmark->items_by_uri, item->uri);

  bookmark_item_free (item);

  return TRUE;
}

/**
 * g_bookmark_file_has_item:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 *
 * Looks whether the desktop bookmark has an item with its URI set to @uri.
 *
 * Returns: %TRUE if @uri is inside @bookmark, %FALSE otherwise
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_has_item (xbookmark_file_t *bookmark,
			  const xchar_t   *uri)
{
  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  return (NULL != xhash_table_lookup (bookmark->items_by_uri, uri));
}

/**
 * g_bookmark_file_get_uris:
 * @bookmark: a #xbookmark_file_t
 * @length: (out) (optional): return location for the number of returned URIs, or %NULL
 *
 * Returns all URIs of the bookmarks in the bookmark file @bookmark.
 * The array of returned URIs will be %NULL-terminated, so @length may
 * optionally be %NULL.
 *
 * Returns: (array length=length) (transfer full): a newly allocated %NULL-terminated array of strings.
 *   Use xstrfreev() to free it.
 *
 * Since: 2.12
 */
xchar_t **
g_bookmark_file_get_uris (xbookmark_file_t *bookmark,
			  xsize_t         *length)
{
  xlist_t *l;
  xchar_t **uris;
  xsize_t i, n_items;

  g_return_val_if_fail (bookmark != NULL, NULL);

  n_items = xlist_length (bookmark->items);
  uris = g_new0 (xchar_t *, n_items + 1);

  /* the items are stored in reverse order, so we walk the list backward */
  for (l = xlist_last (bookmark->items), i = 0; l != NULL; l = l->prev)
    {
      BookmarkItem *item = (BookmarkItem *) l->data;

      g_warn_if_fail (item != NULL);

      uris[i++] = xstrdup (item->uri);
    }
  uris[i] = NULL;

  if (length)
    *length = i;

  return uris;
}

/**
 * g_bookmark_file_set_title:
 * @bookmark: a #xbookmark_file_t
 * @uri: (nullable): a valid URI or %NULL
 * @title: a UTF-8 encoded string
 *
 * Sets @title as the title of the bookmark for @uri inside the
 * bookmark file @bookmark.
 *
 * If @uri is %NULL, the title of @bookmark is set.
 *
 * If a bookmark for @uri cannot be found then it is created.
 *
 * Since: 2.12
 */
void
g_bookmark_file_set_title (xbookmark_file_t *bookmark,
			   const xchar_t   *uri,
			   const xchar_t   *title)
{
  g_return_if_fail (bookmark != NULL);

  if (!uri)
    {
      g_free (bookmark->title);
      bookmark->title = xstrdup (title);
    }
  else
    {
      BookmarkItem *item;

      item = g_bookmark_file_lookup_item (bookmark, uri);
      if (!item)
        {
          item = bookmark_item_new (uri);
          g_bookmark_file_add_item (bookmark, item, NULL);
        }

      g_free (item->title);
      item->title = xstrdup (title);

      bookmark_item_touch_modified (item);
    }
}

/**
 * g_bookmark_file_get_title:
 * @bookmark: a #xbookmark_file_t
 * @uri: (nullable): a valid URI or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * Returns the title of the bookmark for @uri.
 *
 * If @uri is %NULL, the title of @bookmark is returned.
 *
 * In the event the URI cannot be found, %NULL is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: (transfer full): a newly allocated string or %NULL if the specified
 *   URI cannot be found.
 *
 * Since: 2.12
 */
xchar_t *
g_bookmark_file_get_title (xbookmark_file_t  *bookmark,
			   const xchar_t    *uri,
			   xerror_t        **error)
{
  BookmarkItem *item;

  g_return_val_if_fail (bookmark != NULL, NULL);

  if (!uri)
    return xstrdup (bookmark->title);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   uri);
      return NULL;
    }

  return xstrdup (item->title);
}

/**
 * g_bookmark_file_set_description:
 * @bookmark: a #xbookmark_file_t
 * @uri: (nullable): a valid URI or %NULL
 * @description: a string
 *
 * Sets @description as the description of the bookmark for @uri.
 *
 * If @uri is %NULL, the description of @bookmark is set.
 *
 * If a bookmark for @uri cannot be found then it is created.
 *
 * Since: 2.12
 */
void
g_bookmark_file_set_description (xbookmark_file_t *bookmark,
				 const xchar_t   *uri,
				 const xchar_t   *description)
{
  g_return_if_fail (bookmark != NULL);

  if (!uri)
    {
      g_free (bookmark->description);
      bookmark->description = xstrdup (description);
    }
  else
    {
      BookmarkItem *item;

      item = g_bookmark_file_lookup_item (bookmark, uri);
      if (!item)
        {
          item = bookmark_item_new (uri);
          g_bookmark_file_add_item (bookmark, item, NULL);
        }

      g_free (item->description);
      item->description = xstrdup (description);

      bookmark_item_touch_modified (item);
    }
}

/**
 * g_bookmark_file_get_description:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @error: return location for a #xerror_t, or %NULL
 *
 * Retrieves the description of the bookmark for @uri.
 *
 * In the event the URI cannot be found, %NULL is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: (transfer full): a newly allocated string or %NULL if the specified
 *   URI cannot be found.
 *
 * Since: 2.12
 */
xchar_t *
g_bookmark_file_get_description (xbookmark_file_t  *bookmark,
				 const xchar_t    *uri,
				 xerror_t        **error)
{
  BookmarkItem *item;

  g_return_val_if_fail (bookmark != NULL, NULL);

  if (!uri)
    return xstrdup (bookmark->description);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   uri);
      return NULL;
    }

  return xstrdup (item->description);
}

/**
 * g_bookmark_file_set_mime_type:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @mime_type: a MIME type
 *
 * Sets @mime_type as the MIME type of the bookmark for @uri.
 *
 * If a bookmark for @uri cannot be found then it is created.
 *
 * Since: 2.12
 */
void
g_bookmark_file_set_mime_type (xbookmark_file_t *bookmark,
			       const xchar_t   *uri,
			       const xchar_t   *mime_type)
{
  BookmarkItem *item;

  g_return_if_fail (bookmark != NULL);
  g_return_if_fail (uri != NULL);
  g_return_if_fail (mime_type != NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      item = bookmark_item_new (uri);
      g_bookmark_file_add_item (bookmark, item, NULL);
    }

  if (!item->metadata)
    item->metadata = bookmark_metadata_new ();

  g_free (item->metadata->mime_type);

  item->metadata->mime_type = xstrdup (mime_type);
  bookmark_item_touch_modified (item);
}

/**
 * g_bookmark_file_get_mime_type:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @error: return location for a #xerror_t, or %NULL
 *
 * Retrieves the MIME type of the resource pointed by @uri.
 *
 * In the event the URI cannot be found, %NULL is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.  In the
 * event that the MIME type cannot be found, %NULL is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_INVALID_VALUE.
 *
 * Returns: (transfer full): a newly allocated string or %NULL if the specified
 *   URI cannot be found.
 *
 * Since: 2.12
 */
xchar_t *
g_bookmark_file_get_mime_type (xbookmark_file_t  *bookmark,
			       const xchar_t    *uri,
			       xerror_t        **error)
{
  BookmarkItem *item;

  g_return_val_if_fail (bookmark != NULL, NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   uri);
      return NULL;
    }

  if (!item->metadata)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_INVALID_VALUE,
		   _("No MIME type defined in the bookmark for URI “%s”"),
		   uri);
      return NULL;
    }

  return xstrdup (item->metadata->mime_type);
}

/**
 * g_bookmark_file_set_is_private:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @is_private: %TRUE if the bookmark should be marked as private
 *
 * Sets the private flag of the bookmark for @uri.
 *
 * If a bookmark for @uri cannot be found then it is created.
 *
 * Since: 2.12
 */
void
g_bookmark_file_set_is_private (xbookmark_file_t *bookmark,
				const xchar_t   *uri,
				xboolean_t       is_private)
{
  BookmarkItem *item;

  g_return_if_fail (bookmark != NULL);
  g_return_if_fail (uri != NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      item = bookmark_item_new (uri);
      g_bookmark_file_add_item (bookmark, item, NULL);
    }

  if (!item->metadata)
    item->metadata = bookmark_metadata_new ();

  item->metadata->is_private = (is_private == TRUE);
  bookmark_item_touch_modified (item);
}

/**
 * g_bookmark_file_get_is_private:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @error: return location for a #xerror_t, or %NULL
 *
 * Gets whether the private flag of the bookmark for @uri is set.
 *
 * In the event the URI cannot be found, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.  In the
 * event that the private flag cannot be found, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_INVALID_VALUE.
 *
 * Returns: %TRUE if the private flag is set, %FALSE otherwise.
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_get_is_private (xbookmark_file_t  *bookmark,
				const xchar_t    *uri,
				xerror_t        **error)
{
  BookmarkItem *item;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   uri);
      return FALSE;
    }

  if (!item->metadata)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_INVALID_VALUE,
		   _("No private flag has been defined in bookmark for URI “%s”"),
		    uri);
      return FALSE;
    }

  return item->metadata->is_private;
}

/**
 * g_bookmark_file_set_added:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @added: a timestamp or -1 to use the current time
 *
 * Sets the time the bookmark for @uri was added into @bookmark.
 *
 * If no bookmark for @uri is found then it is created.
 *
 * Since: 2.12
 * Deprecated: 2.66: Use g_bookmark_file_set_added_date_time() instead, as
 *    `time_t` is deprecated due to the year 2038 problem.
 */
void
g_bookmark_file_set_added (xbookmark_file_t *bookmark,
			   const xchar_t   *uri,
			   time_t         added)
{
  xdatetime_t *added_dt = (added != (time_t) -1) ? xdate_time_new_from_unix_utc (added) : xdate_time_new_now_utc ();
  g_bookmark_file_set_added_date_time (bookmark, uri, added_dt);
  xdate_time_unref (added_dt);
}

/**
 * g_bookmark_file_set_added_date_time:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @added: a #xdatetime_t
 *
 * Sets the time the bookmark for @uri was added into @bookmark.
 *
 * If no bookmark for @uri is found then it is created.
 *
 * Since: 2.66
 */
void
g_bookmark_file_set_added_date_time (xbookmark_file_t *bookmark,
                                     const char    *uri,
                                     xdatetime_t     *added)
{
  BookmarkItem *item;

  g_return_if_fail (bookmark != NULL);
  g_return_if_fail (uri != NULL);
  g_return_if_fail (added != NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      item = bookmark_item_new (uri);
      g_bookmark_file_add_item (bookmark, item, NULL);
    }

  g_clear_pointer (&item->added, xdate_time_unref);
  item->added = xdate_time_ref (added);
  g_clear_pointer (&item->modified, xdate_time_unref);
  item->modified = xdate_time_ref (added);
}

/**
 * g_bookmark_file_get_added:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @error: return location for a #xerror_t, or %NULL
 *
 * Gets the time the bookmark for @uri was added to @bookmark
 *
 * In the event the URI cannot be found, -1 is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: a timestamp
 *
 * Since: 2.12
 * Deprecated: 2.66: Use g_bookmark_file_get_added_date_time() instead, as
 *    `time_t` is deprecated due to the year 2038 problem.
 */
time_t
g_bookmark_file_get_added (xbookmark_file_t  *bookmark,
			   const xchar_t    *uri,
			   xerror_t        **error)
{
  xdatetime_t *added = g_bookmark_file_get_added_date_time (bookmark, uri, error);
  return (added != NULL) ? xdate_time_to_unix (added) : (time_t) -1;
}

/**
 * g_bookmark_file_get_added_date_time:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @error: return location for a #xerror_t, or %NULL
 *
 * Gets the time the bookmark for @uri was added to @bookmark
 *
 * In the event the URI cannot be found, %NULL is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: (transfer none): a #xdatetime_t
 *
 * Since: 2.66
 */
xdatetime_t *
g_bookmark_file_get_added_date_time (xbookmark_file_t  *bookmark,
                                     const char     *uri,
                                     xerror_t        **error)
{
  BookmarkItem *item;

  g_return_val_if_fail (bookmark != NULL, NULL);
  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
                   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
                   _("No bookmark found for URI “%s”"),
                   uri);
      return NULL;
    }

  return item->added;
}

/**
 * g_bookmark_file_set_modified:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @modified: a timestamp or -1 to use the current time
 *
 * Sets the last time the bookmark for @uri was last modified.
 *
 * If no bookmark for @uri is found then it is created.
 *
 * The "modified" time should only be set when the bookmark's meta-data
 * was actually changed.  Every function of #xbookmark_file_t that
 * modifies a bookmark also changes the modification time, except for
 * g_bookmark_file_set_visited_date_time().
 *
 * Since: 2.12
 * Deprecated: 2.66: Use g_bookmark_file_set_modified_date_time() instead, as
 *    `time_t` is deprecated due to the year 2038 problem.
 */
void
g_bookmark_file_set_modified (xbookmark_file_t *bookmark,
			      const xchar_t   *uri,
			      time_t         modified)
{
  xdatetime_t *modified_dt = (modified != (time_t) -1) ? xdate_time_new_from_unix_utc (modified) : xdate_time_new_now_utc ();
  g_bookmark_file_set_modified_date_time (bookmark, uri, modified_dt);
  xdate_time_unref (modified_dt);
}

/**
 * g_bookmark_file_set_modified_date_time:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @modified: a #xdatetime_t
 *
 * Sets the last time the bookmark for @uri was last modified.
 *
 * If no bookmark for @uri is found then it is created.
 *
 * The "modified" time should only be set when the bookmark's meta-data
 * was actually changed.  Every function of #xbookmark_file_t that
 * modifies a bookmark also changes the modification time, except for
 * g_bookmark_file_set_visited_date_time().
 *
 * Since: 2.66
 */
void
g_bookmark_file_set_modified_date_time (xbookmark_file_t *bookmark,
                                        const char    *uri,
                                        xdatetime_t     *modified)
{
  BookmarkItem *item;

  g_return_if_fail (bookmark != NULL);
  g_return_if_fail (uri != NULL);
  g_return_if_fail (modified != NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      item = bookmark_item_new (uri);
      g_bookmark_file_add_item (bookmark, item, NULL);
    }

  g_clear_pointer (&item->modified, xdate_time_unref);
  item->modified = xdate_time_ref (modified);
}

/**
 * g_bookmark_file_get_modified:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @error: return location for a #xerror_t, or %NULL
 *
 * Gets the time when the bookmark for @uri was last modified.
 *
 * In the event the URI cannot be found, -1 is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: a timestamp
 *
 * Since: 2.12
 * Deprecated: 2.66: Use g_bookmark_file_get_modified_date_time() instead, as
 *    `time_t` is deprecated due to the year 2038 problem.
 */
time_t
g_bookmark_file_get_modified (xbookmark_file_t  *bookmark,
			      const xchar_t    *uri,
			      xerror_t        **error)
{
  xdatetime_t *modified = g_bookmark_file_get_modified_date_time (bookmark, uri, error);
  return (modified != NULL) ? xdate_time_to_unix (modified) : (time_t) -1;
}

/**
 * g_bookmark_file_get_modified_date_time:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @error: return location for a #xerror_t, or %NULL
 *
 * Gets the time when the bookmark for @uri was last modified.
 *
 * In the event the URI cannot be found, %NULL is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: (transfer none): a #xdatetime_t
 *
 * Since: 2.66
 */
xdatetime_t *
g_bookmark_file_get_modified_date_time (xbookmark_file_t  *bookmark,
                                        const char     *uri,
                                        xerror_t        **error)
{
  BookmarkItem *item;

  g_return_val_if_fail (bookmark != NULL, NULL);
  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
                   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
                   _("No bookmark found for URI “%s”"),
                   uri);
      return NULL;
    }

  return item->modified;
}

/**
 * g_bookmark_file_set_visited:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @visited: a timestamp or -1 to use the current time
 *
 * Sets the time the bookmark for @uri was last visited.
 *
 * If no bookmark for @uri is found then it is created.
 *
 * The "visited" time should only be set if the bookmark was launched,
 * either using the command line retrieved by g_bookmark_file_get_application_info()
 * or by the default application for the bookmark's MIME type, retrieved
 * using g_bookmark_file_get_mime_type().  Changing the "visited" time
 * does not affect the "modified" time.
 *
 * Since: 2.12
 * Deprecated: 2.66: Use g_bookmark_file_set_visited_date_time() instead, as
 *    `time_t` is deprecated due to the year 2038 problem.
 */
void
g_bookmark_file_set_visited (xbookmark_file_t *bookmark,
			     const xchar_t   *uri,
			     time_t         visited)
{
  xdatetime_t *visited_dt = (visited != (time_t) -1) ? xdate_time_new_from_unix_utc (visited) : xdate_time_new_now_utc ();
  g_bookmark_file_set_visited_date_time (bookmark, uri, visited_dt);
  xdate_time_unref (visited_dt);
}

/**
 * g_bookmark_file_set_visited_date_time:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @visited: a #xdatetime_t
 *
 * Sets the time the bookmark for @uri was last visited.
 *
 * If no bookmark for @uri is found then it is created.
 *
 * The "visited" time should only be set if the bookmark was launched,
 * either using the command line retrieved by g_bookmark_file_get_application_info()
 * or by the default application for the bookmark's MIME type, retrieved
 * using g_bookmark_file_get_mime_type().  Changing the "visited" time
 * does not affect the "modified" time.
 *
 * Since: 2.66
 */
void
g_bookmark_file_set_visited_date_time (xbookmark_file_t *bookmark,
                                       const char    *uri,
                                       xdatetime_t     *visited)
{
  BookmarkItem *item;

  g_return_if_fail (bookmark != NULL);
  g_return_if_fail (uri != NULL);
  g_return_if_fail (visited != NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      item = bookmark_item_new (uri);
      g_bookmark_file_add_item (bookmark, item, NULL);
    }

  g_clear_pointer (&item->visited, xdate_time_unref);
  item->visited = xdate_time_ref (visited);
}

/**
 * g_bookmark_file_get_visited:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @error: return location for a #xerror_t, or %NULL
 *
 * Gets the time the bookmark for @uri was last visited.
 *
 * In the event the URI cannot be found, -1 is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: a timestamp.
 *
 * Since: 2.12
 * Deprecated: 2.66: Use g_bookmark_file_get_visited_date_time() instead, as
 *    `time_t` is deprecated due to the year 2038 problem.
 */
time_t
g_bookmark_file_get_visited (xbookmark_file_t  *bookmark,
			     const xchar_t    *uri,
			     xerror_t        **error)
{
  xdatetime_t *visited = g_bookmark_file_get_visited_date_time (bookmark, uri, error);
  return (visited != NULL) ? xdate_time_to_unix (visited) : (time_t) -1;
}

/**
 * g_bookmark_file_get_visited_date_time:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @error: return location for a #xerror_t, or %NULL
 *
 * Gets the time the bookmark for @uri was last visited.
 *
 * In the event the URI cannot be found, %NULL is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: (transfer none): a #xdatetime_t
 *
 * Since: 2.66
 */
xdatetime_t *
g_bookmark_file_get_visited_date_time (xbookmark_file_t  *bookmark,
                                       const char     *uri,
                                       xerror_t        **error)
{
  BookmarkItem *item;

  g_return_val_if_fail (bookmark != NULL, NULL);
  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
                   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
                   _("No bookmark found for URI “%s”"),
                   uri);
      return NULL;
    }

  return item->visited;
}

/**
 * g_bookmark_file_has_group:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @group: the group name to be searched
 * @error: return location for a #xerror_t, or %NULL
 *
 * Checks whether @group appears in the list of groups to which
 * the bookmark for @uri belongs to.
 *
 * In the event the URI cannot be found, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: %TRUE if @group was found.
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_has_group (xbookmark_file_t  *bookmark,
			   const xchar_t    *uri,
			   const xchar_t    *group,
			   xerror_t        **error)
{
  BookmarkItem *item;
  xlist_t *l;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   uri);
      return FALSE;
    }

  if (!item->metadata)
    return FALSE;

  for (l = item->metadata->groups; l != NULL; l = l->next)
    {
      if (strcmp (l->data, group) == 0)
        return TRUE;
    }

  return FALSE;

}

/**
 * g_bookmark_file_add_group:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @group: the group name to be added
 *
 * Adds @group to the list of groups to which the bookmark for @uri
 * belongs to.
 *
 * If no bookmark for @uri is found then it is created.
 *
 * Since: 2.12
 */
void
g_bookmark_file_add_group (xbookmark_file_t *bookmark,
			   const xchar_t   *uri,
			   const xchar_t   *group)
{
  BookmarkItem *item;

  g_return_if_fail (bookmark != NULL);
  g_return_if_fail (uri != NULL);
  g_return_if_fail (group != NULL && group[0] != '\0');

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      item = bookmark_item_new (uri);
      g_bookmark_file_add_item (bookmark, item, NULL);
    }

  if (!item->metadata)
    item->metadata = bookmark_metadata_new ();

  if (!g_bookmark_file_has_group (bookmark, uri, group, NULL))
    {
      item->metadata->groups = xlist_prepend (item->metadata->groups,
                                               xstrdup (group));

      bookmark_item_touch_modified (item);
    }
}

/**
 * g_bookmark_file_remove_group:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @group: the group name to be removed
 * @error: return location for a #xerror_t, or %NULL
 *
 * Removes @group from the list of groups to which the bookmark
 * for @uri belongs to.
 *
 * In the event the URI cannot be found, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 * In the event no group was defined, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_INVALID_VALUE.
 *
 * Returns: %TRUE if @group was successfully removed.
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_remove_group (xbookmark_file_t  *bookmark,
			      const xchar_t    *uri,
			      const xchar_t    *group,
			      xerror_t        **error)
{
  BookmarkItem *item;
  xlist_t *l;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   uri);
      return FALSE;
    }

  if (!item->metadata)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
                   G_BOOKMARK_FILE_ERROR_INVALID_VALUE,
                   _("No groups set in bookmark for URI “%s”"),
                   uri);
      return FALSE;
    }

  for (l = item->metadata->groups; l != NULL; l = l->next)
    {
      if (strcmp (l->data, group) == 0)
        {
          item->metadata->groups = xlist_remove_link (item->metadata->groups, l);
          g_free (l->data);
	  xlist_free_1 (l);

          bookmark_item_touch_modified (item);

          return TRUE;
        }
    }

  return FALSE;
}

/**
 * g_bookmark_file_set_groups:
 * @bookmark: a #xbookmark_file_t
 * @uri: an item's URI
 * @groups: (nullable) (array length=length) (element-type utf8): an array of
 *    group names, or %NULL to remove all groups
 * @length: number of group name values in @groups
 *
 * Sets a list of group names for the item with URI @uri.  Each previously
 * set group name list is removed.
 *
 * If @uri cannot be found then an item for it is created.
 *
 * Since: 2.12
 */
void
g_bookmark_file_set_groups (xbookmark_file_t  *bookmark,
			    const xchar_t    *uri,
			    const xchar_t   **groups,
			    xsize_t           length)
{
  BookmarkItem *item;
  xsize_t i;

  g_return_if_fail (bookmark != NULL);
  g_return_if_fail (uri != NULL);
  g_return_if_fail (groups != NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      item = bookmark_item_new (uri);
      g_bookmark_file_add_item (bookmark, item, NULL);
    }

  if (!item->metadata)
    item->metadata = bookmark_metadata_new ();

  xlist_free_full (item->metadata->groups, g_free);
  item->metadata->groups = NULL;

  if (groups)
    {
      for (i = 0; i < length && groups[i] != NULL; i++)
        item->metadata->groups = xlist_append (item->metadata->groups,
					        xstrdup (groups[i]));
    }

  bookmark_item_touch_modified (item);
}

/**
 * g_bookmark_file_get_groups:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @length: (out) (optional): return location for the length of the returned string, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * Retrieves the list of group names of the bookmark for @uri.
 *
 * In the event the URI cannot be found, %NULL is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * The returned array is %NULL terminated, so @length may optionally
 * be %NULL.
 *
 * Returns: (array length=length) (transfer full): a newly allocated %NULL-terminated array of group names.
 *   Use xstrfreev() to free it.
 *
 * Since: 2.12
 */
xchar_t **
g_bookmark_file_get_groups (xbookmark_file_t  *bookmark,
			    const xchar_t    *uri,
			    xsize_t          *length,
			    xerror_t        **error)
{
  BookmarkItem *item;
  xlist_t *l;
  xsize_t len, i;
  xchar_t **retval;

  g_return_val_if_fail (bookmark != NULL, NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   uri);
      return NULL;
    }

  if (!item->metadata)
    {
      if (length)
	*length = 0;

      return NULL;
    }

  len = xlist_length (item->metadata->groups);
  retval = g_new0 (xchar_t *, len + 1);
  for (l = xlist_last (item->metadata->groups), i = 0;
       l != NULL;
       l = l->prev)
    {
      xchar_t *group_name = (xchar_t *) l->data;

      g_warn_if_fail (group_name != NULL);

      retval[i++] = xstrdup (group_name);
    }
  retval[i] = NULL;

  if (length)
    *length = len;

  return retval;
}

/**
 * g_bookmark_file_add_application:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @name: (nullable): the name of the application registering the bookmark
 *   or %NULL
 * @exec: (nullable): command line to be used to launch the bookmark or %NULL
 *
 * Adds the application with @name and @exec to the list of
 * applications that have registered a bookmark for @uri into
 * @bookmark.
 *
 * Every bookmark inside a #xbookmark_file_t must have at least an
 * application registered.  Each application must provide a name, a
 * command line useful for launching the bookmark, the number of times
 * the bookmark has been registered by the application and the last
 * time the application registered this bookmark.
 *
 * If @name is %NULL, the name of the application will be the
 * same returned by g_get_application_name(); if @exec is %NULL, the
 * command line will be a composition of the program name as
 * returned by g_get_prgname() and the "\%u" modifier, which will be
 * expanded to the bookmark's URI.
 *
 * This function will automatically take care of updating the
 * registrations count and timestamping in case an application
 * with the same @name had already registered a bookmark for
 * @uri inside @bookmark.
 *
 * If no bookmark for @uri is found, one is created.
 *
 * Since: 2.12
 */
void
g_bookmark_file_add_application (xbookmark_file_t *bookmark,
				 const xchar_t   *uri,
				 const xchar_t   *name,
				 const xchar_t   *exec)
{
  BookmarkItem *item;
  xchar_t *app_name, *app_exec;
  xdatetime_t *stamp;

  g_return_if_fail (bookmark != NULL);
  g_return_if_fail (uri != NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      item = bookmark_item_new (uri);
      g_bookmark_file_add_item (bookmark, item, NULL);
    }

  if (name && name[0] != '\0')
    app_name = xstrdup (name);
  else
    app_name = xstrdup (g_get_application_name ());

  if (exec && exec[0] != '\0')
    app_exec = xstrdup (exec);
  else
    app_exec = xstrjoin (" ", g_get_prgname(), "%u", NULL);

  stamp = xdate_time_new_now_utc ();

  g_bookmark_file_set_application_info (bookmark, uri,
                                        app_name,
                                        app_exec,
                                        -1,
                                        stamp,
                                        NULL);

  xdate_time_unref (stamp);
  g_free (app_exec);
  g_free (app_name);
}

/**
 * g_bookmark_file_remove_application:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @name: the name of the application
 * @error: return location for a #xerror_t or %NULL
 *
 * Removes application registered with @name from the list of applications
 * that have registered a bookmark for @uri inside @bookmark.
 *
 * In the event the URI cannot be found, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 * In the event that no application with name @app_name has registered
 * a bookmark for @uri,  %FALSE is returned and error is set to
 * %G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED.
 *
 * Returns: %TRUE if the application was successfully removed.
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_remove_application (xbookmark_file_t  *bookmark,
				    const xchar_t    *uri,
				    const xchar_t    *name,
				    xerror_t        **error)
{
  xerror_t *set_error;
  xboolean_t retval;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  set_error = NULL;
  retval = g_bookmark_file_set_application_info (bookmark, uri,
                                                 name,
                                                 "",
                                                 0,
                                                 NULL,
                                                 &set_error);
  if (set_error)
    {
      g_propagate_error (error, set_error);

      return FALSE;
    }

  return retval;
}

/**
 * g_bookmark_file_has_application:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @name: the name of the application
 * @error: return location for a #xerror_t or %NULL
 *
 * Checks whether the bookmark for @uri inside @bookmark has been
 * registered by application @name.
 *
 * In the event the URI cannot be found, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: %TRUE if the application @name was found
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_has_application (xbookmark_file_t  *bookmark,
				 const xchar_t    *uri,
				 const xchar_t    *name,
				 xerror_t        **error)
{
  BookmarkItem *item;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   uri);
      return FALSE;
    }

  return (NULL != bookmark_item_lookup_app_info (item, name));
}

/**
 * g_bookmark_file_set_app_info:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @name: an application's name
 * @exec: an application's command line
 * @count: the number of registrations done for this application
 * @stamp: the time of the last registration for this application
 * @error: return location for a #xerror_t or %NULL
 *
 * Sets the meta-data of application @name inside the list of
 * applications that have registered a bookmark for @uri inside
 * @bookmark.
 *
 * You should rarely use this function; use g_bookmark_file_add_application()
 * and g_bookmark_file_remove_application() instead.
 *
 * @name can be any UTF-8 encoded string used to identify an
 * application.
 * @exec can have one of these two modifiers: "\%f", which will
 * be expanded as the local file name retrieved from the bookmark's
 * URI; "\%u", which will be expanded as the bookmark's URI.
 * The expansion is done automatically when retrieving the stored
 * command line using the g_bookmark_file_get_application_info() function.
 * @count is the number of times the application has registered the
 * bookmark; if is < 0, the current registration count will be increased
 * by one, if is 0, the application with @name will be removed from
 * the list of registered applications.
 * @stamp is the Unix time of the last registration; if it is -1, the
 * current time will be used.
 *
 * If you try to remove an application by setting its registration count to
 * zero, and no bookmark for @uri is found, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND; similarly,
 * in the event that no application @name has registered a bookmark
 * for @uri,  %FALSE is returned and error is set to
 * %G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED.  Otherwise, if no bookmark
 * for @uri is found, one is created.
 *
 * Returns: %TRUE if the application's meta-data was successfully
 *   changed.
 *
 * Since: 2.12
 * Deprecated: 2.66: Use g_bookmark_file_set_application_info() instead, as
 *    `time_t` is deprecated due to the year 2038 problem.
 */
xboolean_t
g_bookmark_file_set_app_info (xbookmark_file_t  *bookmark,
			      const xchar_t    *uri,
			      const xchar_t    *name,
			      const xchar_t    *exec,
			      xint_t            count,
			      time_t          stamp,
			      xerror_t        **error)
{
  xdatetime_t *stamp_dt = (stamp != (time_t) -1) ? xdate_time_new_from_unix_utc (stamp) : xdate_time_new_now_utc ();
  xboolean_t retval;
  retval = g_bookmark_file_set_application_info (bookmark, uri, name, exec, count,
                                                 stamp_dt, error);
  xdate_time_unref (stamp_dt);
  return retval;
}

/**
 * g_bookmark_file_set_application_info:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @name: an application's name
 * @exec: an application's command line
 * @count: the number of registrations done for this application
 * @stamp: (nullable): the time of the last registration for this application,
 *    which may be %NULL if @count is 0
 * @error: return location for a #xerror_t or %NULL
 *
 * Sets the meta-data of application @name inside the list of
 * applications that have registered a bookmark for @uri inside
 * @bookmark.
 *
 * You should rarely use this function; use g_bookmark_file_add_application()
 * and g_bookmark_file_remove_application() instead.
 *
 * @name can be any UTF-8 encoded string used to identify an
 * application.
 * @exec can have one of these two modifiers: "\%f", which will
 * be expanded as the local file name retrieved from the bookmark's
 * URI; "\%u", which will be expanded as the bookmark's URI.
 * The expansion is done automatically when retrieving the stored
 * command line using the g_bookmark_file_get_application_info() function.
 * @count is the number of times the application has registered the
 * bookmark; if is < 0, the current registration count will be increased
 * by one, if is 0, the application with @name will be removed from
 * the list of registered applications.
 * @stamp is the Unix time of the last registration.
 *
 * If you try to remove an application by setting its registration count to
 * zero, and no bookmark for @uri is found, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND; similarly,
 * in the event that no application @name has registered a bookmark
 * for @uri,  %FALSE is returned and error is set to
 * %G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED.  Otherwise, if no bookmark
 * for @uri is found, one is created.
 *
 * Returns: %TRUE if the application's meta-data was successfully
 *   changed.
 *
 * Since: 2.66
 */
xboolean_t
g_bookmark_file_set_application_info (xbookmark_file_t  *bookmark,
                                      const char     *uri,
                                      const char     *name,
                                      const char     *exec,
                                      int             count,
                                      xdatetime_t      *stamp,
                                      xerror_t        **error)
{
  BookmarkItem *item;
  BookmarkAppInfo *ai;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);
  g_return_val_if_fail (name != NULL, FALSE);
  g_return_val_if_fail (exec != NULL, FALSE);
  g_return_val_if_fail (count == 0 || stamp != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      if (count == 0)
        {
          g_set_error (error, G_BOOKMARK_FILE_ERROR,
		       G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		       _("No bookmark found for URI “%s”"),
		       uri);
	  return FALSE;
	}
      else
        {
          item = bookmark_item_new (uri);
	  g_bookmark_file_add_item (bookmark, item, NULL);
	}
    }

  if (!item->metadata)
    item->metadata = bookmark_metadata_new ();

  ai = bookmark_item_lookup_app_info (item, name);
  if (!ai)
    {
      if (count == 0)
        {
          g_set_error (error, G_BOOKMARK_FILE_ERROR,
		       G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED,
		       _("No application with name “%s” registered a bookmark for “%s”"),
		       name,
		       uri);
          return FALSE;
        }
      else
        {
          ai = bookmark_app_info_new (name);

          item->metadata->applications = xlist_prepend (item->metadata->applications, ai);
          xhash_table_replace (item->metadata->apps_by_name, ai->name, ai);
        }
    }

  if (count == 0)
    {
      item->metadata->applications = xlist_remove (item->metadata->applications, ai);
      xhash_table_remove (item->metadata->apps_by_name, ai->name);
      bookmark_app_info_free (ai);

      bookmark_item_touch_modified (item);

      return TRUE;
    }
  else if (count > 0)
    ai->count = count;
  else
    ai->count += 1;

  g_clear_pointer (&ai->stamp, xdate_time_unref);
  ai->stamp = xdate_time_ref (stamp);

  if (exec && exec[0] != '\0')
    {
      g_free (ai->exec);
      ai->exec = g_shell_quote (exec);
    }

  bookmark_item_touch_modified (item);

  return TRUE;
}

/* expands the application's command line */
static xchar_t *
expand_exec_line (const xchar_t *exec_fmt,
		  const xchar_t *uri)
{
  xstring_t *exec;
  xchar_t ch;

  exec = xstring_sized_new (512);
  while ((ch = *exec_fmt++) != '\0')
   {
     if (ch != '%')
       {
         exec = xstring_append_c (exec, ch);
         continue;
       }

     ch = *exec_fmt++;
     switch (ch)
       {
       case '\0':
	 goto out;
       case 'U':
       case 'u':
         xstring_append (exec, uri);
         break;
       case 'F':
       case 'f':
         {
	   xchar_t *file = xfilename_from_uri (uri, NULL, NULL);
           if (file)
             {
	       xstring_append (exec, file);
	       g_free (file);
             }
           else
             {
               xstring_free (exec, TRUE);
               return NULL;
             }
         }
         break;
       case '%':
       default:
         exec = xstring_append_c (exec, ch);
         break;
       }
   }

 out:
  return xstring_free (exec, FALSE);
}

/**
 * g_bookmark_file_get_app_info:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @name: an application's name
 * @exec: (out) (optional): return location for the command line of the application, or %NULL
 * @count: (out) (optional): return location for the registration count, or %NULL
 * @stamp: (out) (optional): return location for the last registration time, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * Gets the registration information of @app_name for the bookmark for
 * @uri.  See g_bookmark_file_set_application_info() for more information about
 * the returned data.
 *
 * The string returned in @app_exec must be freed.
 *
 * In the event the URI cannot be found, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.  In the
 * event that no application with name @app_name has registered a bookmark
 * for @uri,  %FALSE is returned and error is set to
 * %G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED. In the event that unquoting
 * the command line fails, an error of the %G_SHELL_ERROR domain is
 * set and %FALSE is returned.
 *
 * Returns: %TRUE on success.
 *
 * Since: 2.12
 * Deprecated: 2.66: Use g_bookmark_file_get_application_info() instead, as
 *    `time_t` is deprecated due to the year 2038 problem.
 */
xboolean_t
g_bookmark_file_get_app_info (xbookmark_file_t  *bookmark,
			      const xchar_t    *uri,
			      const xchar_t    *name,
			      xchar_t         **exec,
			      xuint_t          *count,
			      time_t         *stamp,
			      xerror_t        **error)
{
  xdatetime_t *stamp_dt = NULL;
  xboolean_t retval;

  retval = g_bookmark_file_get_application_info (bookmark, uri, name, exec, count, &stamp_dt, error);
  if (!retval)
    return FALSE;

  if (stamp != NULL)
    *stamp = xdate_time_to_unix (stamp_dt);

  return TRUE;
}

/**
 * g_bookmark_file_get_application_info:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @name: an application's name
 * @exec: (out) (optional): return location for the command line of the application, or %NULL
 * @count: (out) (optional): return location for the registration count, or %NULL
 * @stamp: (out) (optional) (transfer none): return location for the last registration time, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * Gets the registration information of @app_name for the bookmark for
 * @uri.  See g_bookmark_file_set_application_info() for more information about
 * the returned data.
 *
 * The string returned in @app_exec must be freed.
 *
 * In the event the URI cannot be found, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.  In the
 * event that no application with name @app_name has registered a bookmark
 * for @uri,  %FALSE is returned and error is set to
 * %G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED. In the event that unquoting
 * the command line fails, an error of the %G_SHELL_ERROR domain is
 * set and %FALSE is returned.
 *
 * Returns: %TRUE on success.
 *
 * Since: 2.66
 */
xboolean_t
g_bookmark_file_get_application_info (xbookmark_file_t  *bookmark,
                                      const char     *uri,
                                      const char     *name,
                                      char          **exec,
                                      unsigned int   *count,
                                      xdatetime_t     **stamp,
                                      xerror_t        **error)
{
  BookmarkItem *item;
  BookmarkAppInfo *ai;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);
  g_return_val_if_fail (name != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   uri);
      return FALSE;
    }

  ai = bookmark_item_lookup_app_info (item, name);
  if (!ai)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_APP_NOT_REGISTERED,
		   _("No application with name “%s” registered a bookmark for “%s”"),
		   name,
		   uri);
      return FALSE;
    }

  if (exec)
    {
      xerror_t *unquote_error = NULL;
      xchar_t *command_line;

      command_line = g_shell_unquote (ai->exec, &unquote_error);
      if (unquote_error)
        {
          g_propagate_error (error, unquote_error);
          return FALSE;
        }

      *exec = expand_exec_line (command_line, uri);
      if (!*exec)
        {
          g_set_error (error, G_BOOKMARK_FILE_ERROR,
		       G_BOOKMARK_FILE_ERROR_INVALID_URI,
		       _("Failed to expand exec line “%s” with URI “%s”"),
		     ai->exec, uri);
          g_free (command_line);

          return FALSE;
        }
      else
        g_free (command_line);
    }

  if (count)
    *count = ai->count;

  if (stamp)
    *stamp = ai->stamp;

  return TRUE;
}

/**
 * g_bookmark_file_get_applications:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @length: (out) (optional): return location of the length of the returned list, or %NULL
 * @error: return location for a #xerror_t, or %NULL
 *
 * Retrieves the names of the applications that have registered the
 * bookmark for @uri.
 *
 * In the event the URI cannot be found, %NULL is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: (array length=length) (transfer full): a newly allocated %NULL-terminated array of strings.
 *   Use xstrfreev() to free it.
 *
 * Since: 2.12
 */
xchar_t **
g_bookmark_file_get_applications (xbookmark_file_t  *bookmark,
				  const xchar_t    *uri,
				  xsize_t          *length,
				  xerror_t        **error)
{
  BookmarkItem *item;
  xlist_t *l;
  xchar_t **apps;
  xsize_t i, n_apps;

  g_return_val_if_fail (bookmark != NULL, NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   uri);
      return NULL;
    }

  if (!item->metadata)
    {
      if (length)
	*length = 0;

      return NULL;
    }

  n_apps = xlist_length (item->metadata->applications);
  apps = g_new0 (xchar_t *, n_apps + 1);

  for (l = xlist_last (item->metadata->applications), i = 0;
       l != NULL;
       l = l->prev)
    {
      BookmarkAppInfo *ai;

      ai = (BookmarkAppInfo *) l->data;

      g_warn_if_fail (ai != NULL);
      g_warn_if_fail (ai->name != NULL);

      apps[i++] = xstrdup (ai->name);
    }
  apps[i] = NULL;

  if (length)
    *length = i;

  return apps;
}

/**
 * g_bookmark_file_get_size:
 * @bookmark: a #xbookmark_file_t
 *
 * Gets the number of bookmarks inside @bookmark.
 *
 * Returns: the number of bookmarks
 *
 * Since: 2.12
 */
xint_t
g_bookmark_file_get_size (xbookmark_file_t *bookmark)
{
  g_return_val_if_fail (bookmark != NULL, 0);

  return xlist_length (bookmark->items);
}

/**
 * g_bookmark_file_move_item:
 * @bookmark: a #xbookmark_file_t
 * @old_uri: a valid URI
 * @new_uri: (nullable): a valid URI, or %NULL
 * @error: return location for a #xerror_t or %NULL
 *
 * Changes the URI of a bookmark item from @old_uri to @new_uri.  Any
 * existing bookmark for @new_uri will be overwritten.  If @new_uri is
 * %NULL, then the bookmark is removed.
 *
 * In the event the URI cannot be found, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: %TRUE if the URI was successfully changed
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_move_item (xbookmark_file_t  *bookmark,
			   const xchar_t    *old_uri,
			   const xchar_t    *new_uri,
			   xerror_t        **error)
{
  BookmarkItem *item;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (old_uri != NULL, FALSE);

  item = g_bookmark_file_lookup_item (bookmark, old_uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   old_uri);
      return FALSE;
    }

  if (new_uri && new_uri[0] != '\0')
    {
      if (xstrcmp0 (old_uri, new_uri) == 0)
        return TRUE;

      if (g_bookmark_file_has_item (bookmark, new_uri))
        {
          if (!g_bookmark_file_remove_item (bookmark, new_uri, error))
            return FALSE;
        }

      xhash_table_steal (bookmark->items_by_uri, item->uri);

      g_free (item->uri);
      item->uri = xstrdup (new_uri);
      bookmark_item_touch_modified (item);

      xhash_table_replace (bookmark->items_by_uri, item->uri, item);

      return TRUE;
    }
  else
    {
      if (!g_bookmark_file_remove_item (bookmark, old_uri, error))
        return FALSE;

      return TRUE;
    }
}

/**
 * g_bookmark_file_set_icon:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @href: (nullable): the URI of the icon for the bookmark, or %NULL
 * @mime_type: the MIME type of the icon for the bookmark
 *
 * Sets the icon for the bookmark for @uri. If @href is %NULL, unsets
 * the currently set icon. @href can either be a full URL for the icon
 * file or the icon name following the Icon Naming specification.
 *
 * If no bookmark for @uri is found one is created.
 *
 * Since: 2.12
 */
void
g_bookmark_file_set_icon (xbookmark_file_t *bookmark,
			  const xchar_t   *uri,
			  const xchar_t   *href,
			  const xchar_t   *mime_type)
{
  BookmarkItem *item;

  g_return_if_fail (bookmark != NULL);
  g_return_if_fail (uri != NULL);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      item = bookmark_item_new (uri);
      g_bookmark_file_add_item (bookmark, item, NULL);
    }

  if (!item->metadata)
    item->metadata = bookmark_metadata_new ();

  g_free (item->metadata->icon_href);
  g_free (item->metadata->icon_mime);

  item->metadata->icon_href = xstrdup (href);

  if (mime_type && mime_type[0] != '\0')
    item->metadata->icon_mime = xstrdup (mime_type);
  else
    item->metadata->icon_mime = xstrdup ("application/octet-stream");

  bookmark_item_touch_modified (item);
}

/**
 * g_bookmark_file_get_icon:
 * @bookmark: a #xbookmark_file_t
 * @uri: a valid URI
 * @href: (out) (optional): return location for the icon's location or %NULL
 * @mime_type: (out) (optional): return location for the icon's MIME type or %NULL
 * @error: return location for a #xerror_t or %NULL
 *
 * Gets the icon of the bookmark for @uri.
 *
 * In the event the URI cannot be found, %FALSE is returned and
 * @error is set to %G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND.
 *
 * Returns: %TRUE if the icon for the bookmark for the URI was found.
 *   You should free the returned strings.
 *
 * Since: 2.12
 */
xboolean_t
g_bookmark_file_get_icon (xbookmark_file_t  *bookmark,
			  const xchar_t    *uri,
			  xchar_t         **href,
			  xchar_t         **mime_type,
			  xerror_t        **error)
{
  BookmarkItem *item;

  g_return_val_if_fail (bookmark != NULL, FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  item = g_bookmark_file_lookup_item (bookmark, uri);
  if (!item)
    {
      g_set_error (error, G_BOOKMARK_FILE_ERROR,
		   G_BOOKMARK_FILE_ERROR_URI_NOT_FOUND,
		   _("No bookmark found for URI “%s”"),
		   uri);
      return FALSE;
    }

  if ((!item->metadata) || (!item->metadata->icon_href))
    return FALSE;

  if (href)
    *href = xstrdup (item->metadata->icon_href);

  if (mime_type)
    *mime_type = xstrdup (item->metadata->icon_mime);

  return TRUE;
}
