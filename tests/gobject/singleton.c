/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2006 Imendio AB
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
 */
#undef  G_LOG_DOMAIN
#define G_LOG_DOMAIN "TestSingleton"
#include <glib-object.h>
#include <string.h>

/* --- MySingleton class --- */
typedef struct {
  xobject_t parent_instance;
} MySingleton;
typedef struct {
  xobject_class_t parent_class;
} MySingletonClass;

static xtype_t my_singleton_get_type (void);
#define MY_TYPE_SINGLETON         (my_singleton_get_type ())
#define MY_SINGLETON(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), MY_TYPE_SINGLETON, MySingleton))
#define MY_IS_SINGLETON(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), MY_TYPE_SINGLETON))
#define MY_SINGLETON_CLASS(c)     (XTYPE_CHECK_CLASS_CAST ((c), MY_TYPE_SINGLETON, MySingletonClass))
#define MY_IS_SINGLETON_CLASS(c)  (XTYPE_CHECK_CLASS_TYPE ((c), MY_TYPE_SINGLETON))
#define MY_SINGLETON_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), MY_TYPE_SINGLETON, MySingletonClass))

G_DEFINE_TYPE (MySingleton, my_singleton, XTYPE_OBJECT)

static MySingleton *the_one_and_only = NULL;

/* --- methods --- */
static xobject_t*
my_singleton_constructor (xtype_t                  type,
                          xuint_t                  n_construct_properties,
                          GObjectConstructParam *construct_properties)
{
  if (the_one_and_only)
    return xobject_ref (G_OBJECT (the_one_and_only));
  else
    return G_OBJECT_CLASS (my_singleton_parent_class)->constructor (type, n_construct_properties, construct_properties);
}

static void
my_singleton_init (MySingleton *self)
{
  g_assert (the_one_and_only == NULL);
  the_one_and_only = self;
}

static void
my_singleton_class_init (MySingletonClass *klass)
{
  G_OBJECT_CLASS (klass)->constructor = my_singleton_constructor;
}

/* --- test program --- */
int
main (int   argc,
      char *argv[])
{
  MySingleton *singleton, *obj;

  /* create the singleton */
  singleton = xobject_new (MY_TYPE_SINGLETON, NULL);
  g_assert (singleton != NULL);
  /* assert _singleton_ creation */
  obj = xobject_new (MY_TYPE_SINGLETON, NULL);
  g_assert (singleton == obj);
  xobject_unref (obj);
  /* shutdown */
  xobject_unref (singleton);
  return 0;
}
