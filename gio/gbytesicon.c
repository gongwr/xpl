/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2013 Canonical Limited
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
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gbytesicon.h"
#include "gbytes.h"
#include "gicon.h"
#include "glibintl.h"
#include "gloadableicon.h"
#include "gmemoryinputstream.h"
#include "gtask.h"
#include "gioerror.h"


/**
 * SECTION:gbytesicon
 * @short_description: An icon stored in memory as a xbytes_t
 * @include: gio/gio.h
 * @see_also: #xicon_t, #xloadable_icon_t, #xbytes_t
 *
 * #xbytes_icon_t specifies an image held in memory in a common format (usually
 * png) to be used as icon.
 *
 * Since: 2.38
 **/

typedef xobject_class_t GBytesIconClass;

struct _GBytesIcon
{
  xobject_t parent_instance;

  xbytes_t *bytes;
};

enum
{
  PROP_0,
  PROP_BYTES
};

static void xbytes_icon_icon_iface_init          (xicon_iface_t          *iface);
static void xbytes_icon_loadable_icon_iface_init (GLoadableIconIface  *iface);
G_DEFINE_TYPE_WITH_CODE (xbytes_icon, xbytes_icon, XTYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (XTYPE_ICON, xbytes_icon_icon_iface_init)
                         G_IMPLEMENT_INTERFACE (XTYPE_LOADABLE_ICON, xbytes_icon_loadable_icon_iface_init))

static void
xbytes_icon_get_property (xobject_t    *object,
                           xuint_t       prop_id,
                           xvalue_t     *value,
                           xparam_spec_t *pspec)
{
  xbytes_icon_t *icon = G_BYTES_ICON (object);

  switch (prop_id)
    {
      case PROP_BYTES:
        xvalue_set_boxed (value, icon->bytes);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xbytes_icon_set_property (xobject_t      *object,
                          xuint_t         prop_id,
                          const xvalue_t *value,
                          xparam_spec_t   *pspec)
{
  xbytes_icon_t *icon = G_BYTES_ICON (object);

  switch (prop_id)
    {
      case PROP_BYTES:
        icon->bytes = xvalue_dup_boxed (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xbytes_icon_finalize (xobject_t *object)
{
  xbytes_icon_t *icon;

  icon = G_BYTES_ICON (object);

  xbytes_unref (icon->bytes);

  G_OBJECT_CLASS (xbytes_icon_parent_class)->finalize (object);
}

static void
xbytes_icon_class_init (GBytesIconClass *klass)
{
  xobject_class_t *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = xbytes_icon_get_property;
  gobject_class->set_property = xbytes_icon_set_property;
  gobject_class->finalize = xbytes_icon_finalize;

  /**
   * xbytes_icon_t:bytes:
   *
   * The bytes containing the icon.
   */
  xobject_class_install_property (gobject_class, PROP_BYTES,
                                   g_param_spec_boxed ("bytes",
                                                       P_("bytes"),
                                                       P_("The bytes containing the icon"),
                                                       XTYPE_BYTES,
                                                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
xbytes_icon_init (xbytes_icon_t *bytes)
{
}

/**
 * xbytes_icon_new:
 * @bytes: a #xbytes_t.
 *
 * Creates a new icon for a bytes.
 *
 * This cannot fail, but loading and interpreting the bytes may fail later on
 * (for example, if g_loadable_icon_load() is called) if the image is invalid.
 *
 * Returns: (transfer full) (type xbytes_icon_t): a #xicon_t for the given
 *   @bytes.
 *
 * Since: 2.38
 **/
xicon_t *
xbytes_icon_new (xbytes_t *bytes)
{
  g_return_val_if_fail (bytes != NULL, NULL);

  return xobject_new (XTYPE_BYTES_ICON, "bytes", bytes, NULL);
}

/**
 * xbytes_icon_get_bytes:
 * @icon: a #xicon_t.
 *
 * Gets the #xbytes_t associated with the given @icon.
 *
 * Returns: (transfer none): a #xbytes_t.
 *
 * Since: 2.38
 **/
xbytes_t *
xbytes_icon_get_bytes (xbytes_icon_t *icon)
{
  g_return_val_if_fail (X_IS_BYTES_ICON (icon), NULL);

  return icon->bytes;
}

static xuint_t
xbytes_icon_hash (xicon_t *icon)
{
  xbytes_icon_t *bytes_icon = G_BYTES_ICON (icon);

  return xbytes_hash (bytes_icon->bytes);
}

static xboolean_t
xbytes_icon_equal (xicon_t *icon1,
                    xicon_t *icon2)
{
  xbytes_icon_t *bytes1 = G_BYTES_ICON (icon1);
  xbytes_icon_t *bytes2 = G_BYTES_ICON (icon2);

  return xbytes_equal (bytes1->bytes, bytes2->bytes);
}

static xvariant_t *
xbytes_icon_serialize (xicon_t *icon)
{
  xbytes_icon_t *bytes_icon = G_BYTES_ICON (icon);

  return xvariant_new ("(sv)", "bytes",
                        xvariant_new_from_bytes (G_VARIANT_TYPE_BYTESTRING, bytes_icon->bytes, TRUE));
}

static void
xbytes_icon_icon_iface_init (xicon_iface_t *iface)
{
  iface->hash = xbytes_icon_hash;
  iface->equal = xbytes_icon_equal;
  iface->serialize = xbytes_icon_serialize;
}

static xinput_stream_t *
xbytes_icon_load (xloadable_icon_t  *icon,
                   int            size,
                   char          **type,
                   xcancellable_t   *cancellable,
                   xerror_t        **error)
{
  xbytes_icon_t *bytes_icon = G_BYTES_ICON (icon);

  if (type)
    *type = NULL;

  return g_memory_input_stream_new_from_bytes (bytes_icon->bytes);
}

static void
xbytes_icon_load_async (xloadable_icon_t       *icon,
                         int                  size,
                         xcancellable_t        *cancellable,
                         xasync_ready_callback_t  callback,
                         xpointer_t             user_data)
{
  xbytes_icon_t *bytes_icon = G_BYTES_ICON (icon);
  xtask_t *task;

  task = xtask_new (icon, cancellable, callback, user_data);
  xtask_set_source_tag (task, xbytes_icon_load_async);
  xtask_return_pointer (task, g_memory_input_stream_new_from_bytes (bytes_icon->bytes), xobject_unref);
  xobject_unref (task);
}

static xinput_stream_t *
xbytes_icon_load_finish (xloadable_icon_t  *icon,
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
xbytes_icon_loadable_icon_iface_init (GLoadableIconIface *iface)
{
  iface->load = xbytes_icon_load;
  iface->load_async = xbytes_icon_load_async;
  iface->load_finish = xbytes_icon_load_finish;
}
