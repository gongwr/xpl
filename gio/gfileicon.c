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

#include "gfileicon.h"
#include "gfile.h"
#include "gicon.h"
#include "glibintl.h"
#include "gloadableicon.h"
#include "ginputstream.h"
#include "gtask.h"
#include "gioerror.h"


/**
 * SECTION:gfileicon
 * @short_description: Icons pointing to an image file
 * @include: gio/gio.h
 * @see_also: #xicon_t, #xloadable_icon_t
 *
 * #xfile_icon_t specifies an icon by pointing to an image file
 * to be used as icon.
 *
 **/

static void xfile_icon_icon_iface_init          (xicon_iface_t          *iface);
static void xfile_icon_loadable_icon_iface_init (GLoadableIconIface  *iface);
static void xfile_icon_load_async               (xloadable_icon_t       *icon,
						  int                  size,
						  xcancellable_t        *cancellable,
						  xasync_ready_callback_t  callback,
						  xpointer_t             user_data);

struct _GFileIcon
{
  xobject_t parent_instance;

  xfile_t *file;
};

struct _GFileIconClass
{
  xobject_class_t parent_class;
};

enum
{
  PROP_0,
  PROP_FILE
};

G_DEFINE_TYPE_WITH_CODE (xfile_icon, xfile_icon, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_ICON,
                                                xfile_icon_icon_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_LOADABLE_ICON,
                                                xfile_icon_loadable_icon_iface_init))

static void
xfile_icon_get_property (xobject_t    *object,
                          xuint_t       prop_id,
                          xvalue_t     *value,
                          xparam_spec_t *pspec)
{
  xfile_icon_t *icon = XFILE_ICON (object);

  switch (prop_id)
    {
      case PROP_FILE:
        xvalue_set_object (value, icon->file);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xfile_icon_set_property (xobject_t      *object,
                          xuint_t         prop_id,
                          const xvalue_t *value,
                          xparam_spec_t   *pspec)
{
  xfile_icon_t *icon = XFILE_ICON (object);

  switch (prop_id)
    {
      case PROP_FILE:
        icon->file = XFILE (xvalue_dup_object (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xfile_icon_constructed (xobject_t *object)
{
#ifndef G_DISABLE_ASSERT
  xfile_icon_t *icon = XFILE_ICON (object);
#endif

  G_OBJECT_CLASS (xfile_icon_parent_class)->constructed (object);

  /* Must have be set during construction */
  g_assert (icon->file != NULL);
}

static void
xfile_icon_finalize (xobject_t *object)
{
  xfile_icon_t *icon;

  icon = XFILE_ICON (object);

  if (icon->file)
    xobject_unref (icon->file);

  G_OBJECT_CLASS (xfile_icon_parent_class)->finalize (object);
}

static void
xfile_icon_class_init (GFileIconClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = xfile_icon_get_property;
  gobject_class->set_property = xfile_icon_set_property;
  gobject_class->finalize = xfile_icon_finalize;
  gobject_class->constructed = xfile_icon_constructed;

  /**
   * xfile_icon_t:file:
   *
   * The file containing the icon.
   */
  xobject_class_install_property (gobject_class, PROP_FILE,
                                   g_param_spec_object ("file",
                                                        P_("file"),
                                                        P_("The file containing the icon"),
                                                        XTYPE_FILE,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK));
}

static void
xfile_icon_init (xfile_icon_t *file)
{
}

/**
 * xfile_icon_new:
 * @file: a #xfile_t.
 *
 * Creates a new icon for a file.
 *
 * Returns: (transfer full) (type xfile_icon_t): a #xicon_t for the given
 *   @file, or %NULL on error.
 **/
xicon_t *
xfile_icon_new (xfile_t *file)
{
  g_return_val_if_fail (X_IS_FILE (file), NULL);

  return XICON (xobject_new (XTYPE_FILE_ICON, "file", file, NULL));
}

/**
 * xfile_icon_get_file:
 * @icon: a #xicon_t.
 *
 * Gets the #xfile_t associated with the given @icon.
 *
 * Returns: (transfer none): a #xfile_t.
 **/
xfile_t *
xfile_icon_get_file (xfile_icon_t *icon)
{
  g_return_val_if_fail (X_IS_FILE_ICON (icon), NULL);

  return icon->file;
}

static xuint_t
xfile_icon_hash (xicon_t *icon)
{
  xfile_icon_t *file_icon = XFILE_ICON (icon);

  return xfile_hash (file_icon->file);
}

static xboolean_t
xfile_icon_equal (xicon_t *icon1,
		   xicon_t *icon2)
{
  xfile_icon_t *file1 = XFILE_ICON (icon1);
  xfile_icon_t *file2 = XFILE_ICON (icon2);

  return xfile_equal (file1->file, file2->file);
}

static xboolean_t
xfile_icon_to_tokens (xicon_t *icon,
		       xptr_array_t *tokens,
                       xint_t  *out_version)
{
  xfile_icon_t *file_icon = XFILE_ICON (icon);

  g_return_val_if_fail (out_version != NULL, FALSE);

  *out_version = 0;

  xptr_array_add (tokens, xfile_get_uri (file_icon->file));
  return TRUE;
}

static xicon_t *
xfile_icon_from_tokens (xchar_t  **tokens,
                         xint_t     num_tokens,
                         xint_t     version,
                         xerror_t **error)
{
  xicon_t *icon;
  xfile_t *file;

  icon = NULL;

  if (version != 0)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   _("Can’t handle version %d of xfile_icon_t encoding"),
                   version);
      goto out;
    }

  if (num_tokens != 1)
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           _("Malformed input data for xfile_icon_t"));
      goto out;
    }

  file = xfile_new_for_uri (tokens[0]);
  icon = xfile_icon_new (file);
  xobject_unref (file);

 out:
  return icon;
}

static xvariant_t *
xfile_icon_serialize (xicon_t *icon)
{
  xfile_icon_t *file_icon = XFILE_ICON (icon);

  return xvariant_new ("(sv)", "file", xvariant_new_take_string (xfile_get_uri (file_icon->file)));
}

static void
xfile_icon_icon_iface_init (xicon_iface_t *iface)
{
  iface->hash = xfile_icon_hash;
  iface->equal = xfile_icon_equal;
  iface->to_tokens = xfile_icon_to_tokens;
  iface->from_tokens = xfile_icon_from_tokens;
  iface->serialize = xfile_icon_serialize;
}


static xinput_stream_t *
xfile_icon_load (xloadable_icon_t  *icon,
		  int            size,
		  char          **type,
		  xcancellable_t   *cancellable,
		  xerror_t        **error)
{
  xfile_input_stream_t *stream;
  xfile_icon_t *file_icon = XFILE_ICON (icon);

  stream = xfile_read (file_icon->file,
			cancellable,
			error);

  if (stream && type)
    *type = NULL;

  return G_INPUT_STREAM (stream);
}

static void
load_async_callback (xobject_t      *source_object,
		     xasync_result_t *res,
		     xpointer_t      user_data)
{
  xfile_input_stream_t *stream;
  xerror_t *error = NULL;
  xtask_t *task = user_data;

  stream = xfile_read_finish (XFILE (source_object), res, &error);
  if (stream == NULL)
    xtask_return_error (task, error);
  else
    xtask_return_pointer (task, stream, xobject_unref);
  xobject_unref (task);
}

static void
xfile_icon_load_async (xloadable_icon_t       *icon,
                        int                  size,
                        xcancellable_t        *cancellable,
                        xasync_ready_callback_t  callback,
                        xpointer_t             user_data)
{
  xfile_icon_t *file_icon = XFILE_ICON (icon);
  xtask_t *task;

  task = xtask_new (icon, cancellable, callback, user_data);
  xtask_set_source_tag (task, xfile_icon_load_async);

  xfile_read_async (file_icon->file, 0,
                     cancellable,
                     load_async_callback, task);
}

static xinput_stream_t *
xfile_icon_load_finish (xloadable_icon_t  *icon,
			 xasync_result_t   *res,
			 char          **type,
			 xerror_t        **error)
{
  g_return_val_if_fail (xtask_is_valid (res, icon), NULL);

  if (type)
    *type = NULL;

  return xtask_propagate_pointer (XTASK (res), error);
}

static void
xfile_icon_loadable_icon_iface_init (GLoadableIconIface *iface)
{
  iface->load = xfile_icon_load;
  iface->load_async = xfile_icon_load_async;
  iface->load_finish = xfile_icon_load_finish;
}
