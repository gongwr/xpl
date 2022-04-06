/*
 * Copyright © 2011 William Hua
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
 *
 * Author: William Hua <william@attente.ca>
 */

#include "config.h"

#include "gsettingsbackendinternal.h"
#include "gsimplepermission.h"
#include "giomodule.h"

#import <Foundation/Foundation.h>

xtype_t g_nextstep_settings_backend_get_type (void);

#define G_NEXTSTEP_SETTINGS_BACKEND(obj) (XTYPE_CHECK_INSTANCE_CAST ((obj), g_nextstep_settings_backend_get_type (), GNextstepSettingsBackend))

typedef struct _GNextstepSettingsBackend GNextstepSettingsBackend;
typedef GSettingsBackendClass            GNextstepSettingsBackendClass;

struct _GNextstepSettingsBackend
{
  xsettings_backend_t  parent_instance;

  /*< private >*/
  NSUserDefaults   *user_defaults;
  xmutex_t            mutex;
};

G_DEFINE_TYPE_WITH_CODE (GNextstepSettingsBackend,
                         g_nextstep_settings_backend,
                         XTYPE_SETTINGS_BACKEND,
                         g_io_extension_point_implement (G_SETTINGS_BACKEND_EXTENSION_POINT_NAME,
                                                         g_define_type_id, "nextstep", 90));

static void          g_nextstep_settings_backend_finalize       (xobject_t            *backend);

static xvariant_t *    g_nextstep_settings_backend_read           (xsettings_backend_t   *backend,
                                                                 const xchar_t        *key,
                                                                 const xvariant_type_t *expected_type,
                                                                 xboolean_t            default_value);

static xboolean_t      g_nextstep_settings_backend_get_writable   (xsettings_backend_t   *backend,
                                                                 const xchar_t        *key);

static xboolean_t      g_nextstep_settings_backend_write          (xsettings_backend_t   *backend,
                                                                 const xchar_t        *key,
                                                                 xvariant_t           *value,
                                                                 xpointer_t            origin_tag);

static xboolean_t      g_nextstep_settings_backend_write_tree     (xsettings_backend_t   *backend,
                                                                 xtree_t              *tree,
                                                                 xpointer_t            origin_tag);

static void          g_nextstep_settings_backend_reset          (xsettings_backend_t   *backend,
                                                                 const xchar_t        *key,
                                                                 xpointer_t            origin_tag);

static void          g_nextstep_settings_backend_subscribe      (xsettings_backend_t   *backend,
                                                                 const xchar_t        *name);

static void          g_nextstep_settings_backend_unsubscribe    (xsettings_backend_t   *backend,
                                                                 const xchar_t        *name);

static void          g_nextstep_settings_backend_sync           (xsettings_backend_t   *backend);

static xpermission_t * g_nextstep_settings_backend_get_permission (xsettings_backend_t   *backend,
                                                                 const xchar_t        *path);

static xboolean_t      g_nextstep_settings_backend_write_pair     (xpointer_t            name,
                                                                 xpointer_t            value,
                                                                 xpointer_t            data);

static xvariant_t *    g_nextstep_settings_backend_get_g_variant  (id                  object,
                                                                 const xvariant_type_t *type);

static id            g_nextstep_settings_backend_get_ns_object  (xvariant_t           *variant);

static void
g_nextstep_settings_backend_class_init (GNextstepSettingsBackendClass *class)
{
  G_OBJECT_CLASS (class)->finalize = g_nextstep_settings_backend_finalize;
  class->read = g_nextstep_settings_backend_read;
  class->get_writable = g_nextstep_settings_backend_get_writable;
  class->write = g_nextstep_settings_backend_write;
  class->write_tree = g_nextstep_settings_backend_write_tree;
  class->reset = g_nextstep_settings_backend_reset;
  class->subscribe = g_nextstep_settings_backend_subscribe;
  class->unsubscribe = g_nextstep_settings_backend_unsubscribe;
  class->sync = g_nextstep_settings_backend_sync;
  class->get_permission = g_nextstep_settings_backend_get_permission;
}

static void
g_nextstep_settings_backend_init (GNextstepSettingsBackend *self)
{
  NSAutoreleasePool *pool;

  pool = [[NSAutoreleasePool alloc] init];

  self->user_defaults = [[NSUserDefaults standardUserDefaults] retain];

  g_mutex_init (&self->mutex);

  [pool drain];
}

static void
g_nextstep_settings_backend_finalize (xobject_t *self)
{
  GNextstepSettingsBackend *backend = G_NEXTSTEP_SETTINGS_BACKEND (self);
  NSAutoreleasePool *pool;

  pool = [[NSAutoreleasePool alloc] init];

  g_mutex_clear (&backend->mutex);

  [backend->user_defaults release];

  [pool drain];

  G_OBJECT_CLASS (g_nextstep_settings_backend_parent_class)->finalize (self);
}

static xvariant_t *
g_nextstep_settings_backend_read (xsettings_backend_t   *backend,
                                  const xchar_t        *key,
                                  const xvariant_type_t *expected_type,
                                  xboolean_t            default_value)
{
  GNextstepSettingsBackend *self = G_NEXTSTEP_SETTINGS_BACKEND (backend);
  NSAutoreleasePool *pool;
  NSString *name;
  id value;
  xvariant_t *variant;

  if (default_value)
    return NULL;

  pool = [[NSAutoreleasePool alloc] init];
  name = [NSString stringWithUTF8String:key];

  g_mutex_lock (&self->mutex);
  value = [self->user_defaults objectForKey:name];
  g_mutex_unlock (&self->mutex);

  variant = g_nextstep_settings_backend_get_g_variant (value, expected_type);

  [pool drain];

  return variant;
}

static xboolean_t
g_nextstep_settings_backend_get_writable (xsettings_backend_t *backend,
                                          const xchar_t      *key)
{
  return TRUE;
}

static xboolean_t
g_nextstep_settings_backend_write (xsettings_backend_t *backend,
                                   const xchar_t      *key,
                                   xvariant_t         *value,
                                   xpointer_t          origin_tag)
{
  GNextstepSettingsBackend *self = G_NEXTSTEP_SETTINGS_BACKEND (backend);
  NSAutoreleasePool *pool;

  pool = [[NSAutoreleasePool alloc] init];

  g_mutex_lock (&self->mutex);
  g_nextstep_settings_backend_write_pair ((xpointer_t) key, value, self);
  g_mutex_unlock (&self->mutex);

  g_settings_backend_changed (backend, key, origin_tag);

  [pool drain];

  return TRUE;
}

static xboolean_t
g_nextstep_settings_backend_write_tree (xsettings_backend_t *backend,
                                        xtree_t            *tree,
                                        xpointer_t          origin_tag)
{
  GNextstepSettingsBackend *self = G_NEXTSTEP_SETTINGS_BACKEND (backend);
  NSAutoreleasePool *pool;

  pool = [[NSAutoreleasePool alloc] init];

  g_mutex_lock (&self->mutex);
  xtree_foreach (tree, g_nextstep_settings_backend_write_pair, self);
  g_mutex_unlock (&self->mutex);
  g_settings_backend_changed_tree (backend, tree, origin_tag);

  [pool drain];

  return TRUE;
}

static void
g_nextstep_settings_backend_reset (xsettings_backend_t *backend,
                                   const xchar_t      *key,
                                   xpointer_t          origin_tag)
{
  GNextstepSettingsBackend *self = G_NEXTSTEP_SETTINGS_BACKEND (backend);
  NSAutoreleasePool *pool;
  NSString *name;

  pool = [[NSAutoreleasePool alloc] init];
  name = [NSString stringWithUTF8String:key];

  g_mutex_lock (&self->mutex);
  [self->user_defaults removeObjectForKey:name];
  g_mutex_unlock (&self->mutex);

  g_settings_backend_changed (backend, key, origin_tag);

  [pool drain];
}

static void
g_nextstep_settings_backend_subscribe (xsettings_backend_t *backend,
                                       const xchar_t      *name)
{
}

static void
g_nextstep_settings_backend_unsubscribe (xsettings_backend_t *backend,
                                         const xchar_t      *name)
{
}

static void
g_nextstep_settings_backend_sync (xsettings_backend_t *backend)
{
  GNextstepSettingsBackend *self = G_NEXTSTEP_SETTINGS_BACKEND (backend);
  NSAutoreleasePool *pool;

  pool = [[NSAutoreleasePool alloc] init];

  g_mutex_lock (&self->mutex);
  [self->user_defaults synchronize];
  g_mutex_unlock (&self->mutex);

  [pool drain];
}

static xpermission_t *
g_nextstep_settings_backend_get_permission (xsettings_backend_t *backend,
                                            const xchar_t      *path)
{
  return g_simple_permission_new (TRUE);
}

static xboolean_t
g_nextstep_settings_backend_write_pair (xpointer_t name,
                                        xpointer_t value,
                                        xpointer_t data)
{
  GNextstepSettingsBackend *backend = G_NEXTSTEP_SETTINGS_BACKEND (data);
  NSString *key;
  id object;

  key = [NSString stringWithUTF8String:name];
  object = g_nextstep_settings_backend_get_ns_object (value);

  [backend->user_defaults setObject:object forKey:key];

  return FALSE;
}

static xvariant_t *
g_nextstep_settings_backend_get_g_variant (id                  object,
                                           const xvariant_type_t *type)
{
  if ([object isKindOfClass:[NSData class]])
    return xvariant_parse (type, [[[[NSString alloc] initWithData:object encoding:NSUTF8StringEncoding] autorelease] UTF8String], NULL, NULL, NULL);
  else if ([object isKindOfClass:[NSNumber class]])
    {
      if (xvariant_type_equal (type, G_VARIANT_TYPE_BOOLEAN))
        return xvariant_new_boolean ([object boolValue]);
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_BYTE))
        return xvariant_new_byte ([object unsignedCharValue]);
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_INT16))
        return xvariant_new_int16 ([object shortValue]);
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_UINT16))
        return xvariant_new_uint16 ([object unsignedShortValue]);
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_INT32))
        return xvariant_new_int32 ([object longValue]);
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_UINT32))
        return xvariant_new_uint32 ([object unsignedLongValue]);
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_INT64))
        return xvariant_new_int64 ([object longLongValue]);
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_UINT64))
        return xvariant_new_uint64 ([object unsignedLongLongValue]);
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_HANDLE))
        return xvariant_new_handle ([object longValue]);
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_DOUBLE))
        return xvariant_new_double ([object doubleValue]);
    }
  else if ([object isKindOfClass:[NSString class]])
    {
      const char *string;

      string = [object UTF8String];

      if (xvariant_type_equal (type, G_VARIANT_TYPE_STRING))
        return xvariant_new_string (string);
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_OBJECT_PATH))
        return xvariant_is_object_path (string) ?
               xvariant_new_object_path (string) : NULL;
      else if (xvariant_type_equal (type, G_VARIANT_TYPE_SIGNATURE))
        return xvariant_is_signature (string) ?
               xvariant_new_signature (string) : NULL;
    }
  else if ([object isKindOfClass:[NSDictionary class]])
    {
      if (xvariant_type_is_subtype_of (type, G_VARIANT_TYPE ("a{s*}")))
        {
          const xvariant_type_t *value_type;
          xvariant_builder_t builder;
          NSString *key;

          value_type = xvariant_type_value (xvariant_type_element (type));

          xvariant_builder_init (&builder, type);

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1050
          for(key in object)
#else
          NSEnumerator *enumerator = [object objectEnumerator];
          while((key = [enumerator nextObject]))
#endif
            {
              xvariant_t *name;
              id value;
              xvariant_t *variant;
              xvariant_t *entry;

              name = xvariant_new_string ([key UTF8String]);
              value = [object objectForKey:key];
              variant = g_nextstep_settings_backend_get_g_variant (value, value_type);

              if (variant == NULL)
                {
                  xvariant_builder_clear (&builder);

                  return NULL;
                }

              entry = xvariant_new_dict_entry (name, variant);
              xvariant_builder_add_value (&builder, entry);
            }

          return xvariant_builder_end (&builder);
        }
    }
  else if ([object isKindOfClass:[NSArray class]])
    {
      if (xvariant_type_is_subtype_of (type, G_VARIANT_TYPE_ARRAY))
        {
          const xvariant_type_t *value_type;
          xvariant_builder_t builder;
          id value;

          value_type = xvariant_type_element (type);
          xvariant_builder_init (&builder, type);

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1050
          for(value in object)
#else
          NSEnumerator *enumerator = [object objectEnumerator];
          while((value = [enumerator nextObject]))
#endif
            {
              xvariant_t *variant = g_nextstep_settings_backend_get_g_variant (value, value_type);

              if (variant == NULL)
                {
                  xvariant_builder_clear (&builder);

                  return NULL;
                }

              xvariant_builder_add_value (&builder, variant);
            }

          return xvariant_builder_end (&builder);
        }
    }

  return NULL;
}

static id
g_nextstep_settings_backend_get_ns_object (xvariant_t *variant)
{
  if (variant == NULL)
    return nil;
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_BOOLEAN))
    return [NSNumber numberWithBool:xvariant_get_boolean (variant)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_BYTE))
    return [NSNumber numberWithUnsignedChar:xvariant_get_byte (variant)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_INT16))
    return [NSNumber numberWithShort:xvariant_get_int16 (variant)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_UINT16))
    return [NSNumber numberWithUnsignedShort:xvariant_get_uint16 (variant)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_INT32))
    return [NSNumber numberWithLong:xvariant_get_int32 (variant)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_UINT32))
    return [NSNumber numberWithUnsignedLong:xvariant_get_uint32 (variant)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_INT64))
    return [NSNumber numberWithLongLong:xvariant_get_int64 (variant)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_UINT64))
    return [NSNumber numberWithUnsignedLongLong:xvariant_get_uint64 (variant)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_HANDLE))
    return [NSNumber numberWithLong:xvariant_get_handle (variant)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_DOUBLE))
    return [NSNumber numberWithDouble:xvariant_get_double (variant)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_STRING))
    return [NSString stringWithUTF8String:xvariant_get_string (variant, NULL)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_OBJECT_PATH))
    return [NSString stringWithUTF8String:xvariant_get_string (variant, NULL)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_SIGNATURE))
    return [NSString stringWithUTF8String:xvariant_get_string (variant, NULL)];
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE ("a{s*}")))
    {
      NSMutableDictionary *dictionary;
      xvariant_iter_t iter;
      const xchar_t *name;
      xvariant_t *value;

      dictionary = [NSMutableDictionary dictionaryWithCapacity:xvariant_iter_init (&iter, variant)];

      while (xvariant_iter_loop (&iter, "{&s*}", &name, &value))
        {
          NSString *key;
          id object;

          key = [NSString stringWithUTF8String:name];
          object = g_nextstep_settings_backend_get_ns_object (value);

          [dictionary setObject:object forKey:key];
        }

      return dictionary;
    }
  else if (xvariant_is_of_type (variant, G_VARIANT_TYPE_ARRAY))
    {
      NSMutableArray *array;
      xvariant_iter_t iter;
      xvariant_t *value;

      array = [NSMutableArray arrayWithCapacity:xvariant_iter_init (&iter, variant)];

      while ((value = xvariant_iter_next_value (&iter)) != NULL)
        [array addObject:g_nextstep_settings_backend_get_ns_object (value)];

      return array;
    }
  else
    return [[NSString stringWithUTF8String:xvariant_print (variant, TRUE)] dataUsingEncoding:NSUTF8StringEncoding];
}
