/* xobject_t - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1998-1999, 2000-2001 Tim Janik and Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MT safe with regards to reference counting.
 */

#include "config.h"

#include <string.h>
#include <signal.h>

#include "../glib/glib-private.h"

#include "gobject.h"
#include "gtype-private.h"
#include "gvaluecollector.h"
#include "gsignal.h"
#include "gparamspecs.h"
#include "gvaluetypes.h"
#include "gobject_trace.h"
#include "gconstructor.h"

/**
 * SECTION:objects
 * @title: xobject_t
 * @short_description: The base object type
 * @see_also: #GParamSpecObject, xparam_spec_object()
 *
 * xobject_t is the fundamental type providing the common attributes and
 * methods for all object types in GTK+, Pango and other libraries
 * based on xobject_t.  The xobject_t class provides methods for object
 * construction and destruction, property access methods, and signal
 * support.  Signals are described in detail [here][gobject-Signals].
 *
 * For a tutorial on implementing a new xobject_t class, see [How to define and
 * implement a new xobject_t][howto-gobject]. For a list of naming conventions for
 * GObjects and their methods, see the [xtype_t conventions][gtype-conventions].
 * For the high-level concepts behind xobject_t, read [Instantiatable classed types:
 * Objects][gtype-instantiatable-classed].
 *
 * ## Floating references # {#floating-ref}
 *
 * **Note**: Floating references are a C convenience API and should not be
 * used in modern xobject_t code. Language bindings in particular find the
 * concept highly problematic, as floating references are not identifiable
 * through annotations, and neither are deviations from the floating reference
 * behavior, like types that inherit from #xinitially_unowned_t and still return
 * a full reference from xobject_new().
 *
 * xinitially_unowned_t is derived from xobject_t. The only difference between
 * the two is that the initial reference of a xinitially_unowned_t is flagged
 * as a "floating" reference. This means that it is not specifically
 * claimed to be "owned" by any code portion. The main motivation for
 * providing floating references is C convenience. In particular, it
 * allows code to be written as:
 *
 * |[<!-- language="C" -->
 * container = create_container ();
 * container_add_child (container, create_child());
 * ]|
 *
 * If container_add_child() calls xobject_ref_sink() on the passed-in child,
 * no reference of the newly created child is leaked. Without floating
 * references, container_add_child() can only xobject_ref() the new child,
 * so to implement this code without reference leaks, it would have to be
 * written as:
 *
 * |[<!-- language="C" -->
 * Child *child;
 * container = create_container ();
 * child = create_child ();
 * container_add_child (container, child);
 * xobject_unref (child);
 * ]|
 *
 * The floating reference can be converted into an ordinary reference by
 * calling xobject_ref_sink(). For already sunken objects (objects that
 * don't have a floating reference anymore), xobject_ref_sink() is equivalent
 * to xobject_ref() and returns a new reference.
 *
 * Since floating references are useful almost exclusively for C convenience,
 * language bindings that provide automated reference and memory ownership
 * maintenance (such as smart pointers or garbage collection) should not
 * expose floating references in their API. The best practice for handling
 * types that have initially floating references is to immediately sink those
 * references after xobject_new() returns, by checking if the #xtype_t
 * inherits from #xinitially_unowned_t. For instance:
 *
 * |[<!-- language="C" -->
 * xobject_t *res = xobject_new_with_properties (gtype,
 *                                              n_props,
 *                                              prop_names,
 *                                              prop_values);
 *
 * // or: if (xtype_is_a (gtype, XTYPE_INITIALLY_UNOWNED))
 * if (X_IS_INITIALLY_UNOWNED (res))
 *   xobject_ref_sink (res);
 *
 * return res;
 * ]|
 *
 * Some object implementations may need to save an objects floating state
 * across certain code portions (an example is #GtkMenu), to achieve this,
 * the following sequence can be used:
 *
 * |[<!-- language="C" -->
 * // save floating state
 * xboolean_t was_floating = xobject_is_floating (object);
 * xobject_ref_sink (object);
 * // protected code portion
 *
 * ...
 *
 * // restore floating state
 * if (was_floating)
 *   xobject_force_floating (object);
 * else
 *   xobject_unref (object); // release previously acquired reference
 * ]|
 */


/* --- macros --- */
#define PARAM_SPEC_PARAM_ID(pspec)		((pspec)->param_id)
#define	PARAM_SPEC_SET_PARAM_ID(pspec, id)	((pspec)->param_id = (id))

#define OBJECT_HAS_TOGGLE_REF_FLAG 0x1
#define OBJECT_HAS_TOGGLE_REF(object) \
    ((g_datalist_get_flags (&(object)->qdata) & OBJECT_HAS_TOGGLE_REF_FLAG) != 0)
#define OBJECT_FLOATING_FLAG 0x2

#define CLASS_HAS_PROPS_FLAG 0x1
#define CLASS_HAS_PROPS(class) \
    ((class)->flags & CLASS_HAS_PROPS_FLAG)
#define CLASS_HAS_CUSTOM_CONSTRUCTOR(class) \
    ((class)->constructor != xobject_constructor)
#define CLASS_HAS_CUSTOM_CONSTRUCTED(class) \
    ((class)->constructed != xobject_constructed)

#define CLASS_HAS_DERIVED_CLASS_FLAG 0x2
#define CLASS_HAS_DERIVED_CLASS(class) \
    ((class)->flags & CLASS_HAS_DERIVED_CLASS_FLAG)

/* --- signals --- */
enum {
  NOTIFY,
  LAST_SIGNAL
};


/* --- properties --- */
enum {
  PROP_NONE
};

#define OPTIONAL_FLAG_IN_CONSTRUCTION 1<<0
#define OPTIONAL_FLAG_HAS_SIGNAL_HANDLER 1<<1 /* Set if object ever had a signal handler */

#if SIZEOF_INT == 4 && XPL_SIZEOF_VOID_P == 8
#define HAVE_OPTIONAL_FLAGS
#endif

typedef struct
{
  GTypeInstance  xtype_instance;

  /*< private >*/
  xuint_t          ref_count;  /* (atomic) */
#ifdef HAVE_OPTIONAL_FLAGS
  xuint_t          optional_flags;  /* (atomic) */
#endif
  GData         *qdata;
} xobject_real_t;

G_STATIC_ASSERT(sizeof(xobject_t) == sizeof(xobject_real_t));
G_STATIC_ASSERT(G_STRUCT_OFFSET(xobject_t, ref_count) == G_STRUCT_OFFSET(xobject_real_t, ref_count));
G_STATIC_ASSERT(G_STRUCT_OFFSET(xobject_t, qdata) == G_STRUCT_OFFSET(xobject_real_t, qdata));


/* --- prototypes --- */
static void	xobject_base_class_init		(xobject_class_t	*class);
static void	xobject_base_class_finalize		(xobject_class_t	*class);
static void	xobject_do_class_init			(xobject_class_t	*class);
static void	xobject_init				(xobject_t	*object,
							 xobject_class_t	*class);
static xobject_t*	xobject_constructor			(xtype_t                  type,
							 xuint_t                  n_construct_properties,
							 GObjectConstructParam *construct_params);
static void     xobject_constructed                    (xobject_t        *object);
static void	xobject_real_dispose			(xobject_t	*object);
static void	xobject_finalize			(xobject_t	*object);
static void	xobject_do_set_property		(xobject_t        *object,
							 xuint_t           property_id,
							 const xvalue_t   *value,
							 xparam_spec_t     *pspec);
static void	xobject_do_get_property		(xobject_t        *object,
							 xuint_t           property_id,
							 xvalue_t         *value,
							 xparam_spec_t     *pspec);
static void	xvalue_object_init			(xvalue_t		*value);
static void	xvalue_object_free_value		(xvalue_t		*value);
static void	xvalue_object_copy_value		(const xvalue_t	*src_value,
							 xvalue_t		*dest_value);
static void	xvalue_object_transform_value		(const xvalue_t	*src_value,
							 xvalue_t		*dest_value);
static xpointer_t xvalue_object_peek_pointer             (const xvalue_t   *value);
static xchar_t*	xvalue_object_collect_value		(xvalue_t		*value,
							 xuint_t           n_collect_values,
							 xtype_c_value_t    *collect_values,
							 xuint_t           collect_flags);
static xchar_t*	xvalue_object_lcopy_value		(const xvalue_t	*value,
							 xuint_t           n_collect_values,
							 xtype_c_value_t    *collect_values,
							 xuint_t           collect_flags);
static void	xobject_dispatch_properties_changed	(xobject_t	*object,
							 xuint_t		 n_pspecs,
							 xparam_spec_t    **pspecs);
static xuint_t               object_floating_flag_handler (xobject_t        *object,
                                                         xint_t            job);

static void object_interface_check_properties           (xpointer_t        check_data,
							 xpointer_t        x_iface);
static void                weak_locations_free_unlocked (xslist_t **weak_locations);

/* --- typedefs --- */
typedef struct _xobject_notify_queue            xobject_notify_queue_t;

struct _xobject_notify_queue
{
  xslist_t  *pspecs;
  xuint16_t  n_pspecs;
  xuint16_t  freeze_count;
};

/* --- variables --- */
G_LOCK_DEFINE_STATIC (closure_array_mutex);
G_LOCK_DEFINE_STATIC (weak_refs_mutex);
G_LOCK_DEFINE_STATIC (toggle_refs_mutex);
static xquark	            quark_closure_array = 0;
static xquark	            quark_weak_refs = 0;
static xquark	            quark_toggle_refs = 0;
static xquark               quark_notify_queue;
static xquark               quark_in_construction;
static GParamSpecPool      *pspec_pool = NULL;
static xulong_t	            gobject_signals[LAST_SIGNAL] = { 0, };
static xuint_t (*floating_flag_handler) (xobject_t*, xint_t) = object_floating_flag_handler;
/* qdata pointing to xslist_t<GWeakRef *>, protected by weak_locations_lock */
static xquark	            quark_weak_locations = 0;
static GRWLock              weak_locations_lock;

G_LOCK_DEFINE_STATIC(notify_lock);

/* --- functions --- */
static void
xobject_notify_queue_free (xpointer_t data)
{
  xobject_notify_queue_t *nqueue = data;

  xslist_free (nqueue->pspecs);
  g_slice_free (xobject_notify_queue_t, nqueue);
}

static xobject_notify_queue_t*
xobject_notify_queue_freeze (xobject_t  *object,
                              xboolean_t  conditional)
{
  xobject_notify_queue_t *nqueue;

  G_LOCK(notify_lock);
  nqueue = g_datalist_id_get_data (&object->qdata, quark_notify_queue);
  if (!nqueue)
    {
      if (conditional)
        {
          G_UNLOCK(notify_lock);
          return NULL;
        }

      nqueue = g_slice_new0 (xobject_notify_queue_t);
      g_datalist_id_set_data_full (&object->qdata, quark_notify_queue,
                                   nqueue, xobject_notify_queue_free);
    }

  if (nqueue->freeze_count >= 65535)
    g_critical("Free queue for %s (%p) is larger than 65535,"
               " called xobject_freeze_notify() too often."
               " Forgot to call xobject_thaw_notify() or infinite loop",
               G_OBJECT_TYPE_NAME (object), object);
  else
    nqueue->freeze_count++;

  G_UNLOCK(notify_lock);

  return nqueue;
}

static void
xobject_notify_queue_thaw (xobject_t            *object,
                            xobject_notify_queue_t *nqueue)
{
  xparam_spec_t *pspecs_mem[16], **pspecs, **free_me = NULL;
  xslist_t *slist;
  xuint_t n_pspecs = 0;

  g_return_if_fail (g_atomic_int_get(&object->ref_count) > 0);

  G_LOCK(notify_lock);

  /* Just make sure we never get into some nasty race condition */
  if (G_UNLIKELY(nqueue->freeze_count == 0)) {
    G_UNLOCK(notify_lock);
    g_warning ("%s: property-changed notification for %s(%p) is not frozen",
               G_STRFUNC, G_OBJECT_TYPE_NAME (object), object);
    return;
  }

  nqueue->freeze_count--;
  if (nqueue->freeze_count) {
    G_UNLOCK(notify_lock);
    return;
  }

  pspecs = nqueue->n_pspecs > 16 ? free_me = g_new (xparam_spec_t*, nqueue->n_pspecs) : pspecs_mem;

  for (slist = nqueue->pspecs; slist; slist = slist->next)
    {
      pspecs[n_pspecs++] = slist->data;
    }
  g_datalist_id_set_data (&object->qdata, quark_notify_queue, NULL);

  G_UNLOCK(notify_lock);

  if (n_pspecs)
    G_OBJECT_GET_CLASS (object)->dispatch_properties_changed (object, n_pspecs, pspecs);
  g_free (free_me);
}

static void
xobject_notify_queue_add (xobject_t            *object,
                           xobject_notify_queue_t *nqueue,
                           xparam_spec_t         *pspec)
{
  G_LOCK(notify_lock);

  xassert (nqueue->n_pspecs < 65535);

  if (xslist_find (nqueue->pspecs, pspec) == NULL)
    {
      nqueue->pspecs = xslist_prepend (nqueue->pspecs, pspec);
      nqueue->n_pspecs++;
    }

  G_UNLOCK(notify_lock);
}

#ifdef	G_ENABLE_DEBUG
G_LOCK_DEFINE_STATIC     (debug_objects);
static xuint_t		 debug_objects_count = 0;
static xhashtable_t	*debug_objects_ht = NULL;

static void
debug_objects_foreach (xpointer_t key,
		       xpointer_t value,
		       xpointer_t user_data)
{
  xobject_t *object = value;

  g_message ("[%p] stale %s\tref_count=%u",
	     object,
	     G_OBJECT_TYPE_NAME (object),
	     object->ref_count);
}

#ifdef G_HAS_CONSTRUCTORS
#ifdef G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(debug_objects_atexit)
#endif
G_DEFINE_DESTRUCTOR(debug_objects_atexit)
#endif /* G_HAS_CONSTRUCTORS */

static void
debug_objects_atexit (void)
{
  GOBJECT_IF_DEBUG (OBJECTS,
    {
      G_LOCK (debug_objects);
      g_message ("stale GObjects: %u", debug_objects_count);
      xhash_table_foreach (debug_objects_ht, debug_objects_foreach, NULL);
      G_UNLOCK (debug_objects);
    });
}
#endif	/* G_ENABLE_DEBUG */

void
_xobject_type_init (void)
{
  static xboolean_t initialized = FALSE;
  static const GTypeFundamentalInfo finfo = {
    XTYPE_FLAG_CLASSED | XTYPE_FLAG_INSTANTIATABLE | XTYPE_FLAG_DERIVABLE | XTYPE_FLAG_DEEP_DERIVABLE,
  };
  xtype_info_t info = {
    sizeof (xobject_class_t),
    (xbase_init_func_t) xobject_base_class_init,
    (xbase_finalize_func_t) xobject_base_class_finalize,
    (xclass_init_func_t) xobject_do_class_init,
    NULL	/* class_destroy */,
    NULL	/* class_data */,
    sizeof (xobject_t),
    0		/* n_preallocs */,
    (xinstance_init_func_t) xobject_init,
    NULL,	/* value_table */
  };
  static const xtype_value_table_t value_table = {
    xvalue_object_init,	  /* value_init */
    xvalue_object_free_value,	  /* value_free */
    xvalue_object_copy_value,	  /* value_copy */
    xvalue_object_peek_pointer,  /* value_peek_pointer */
    "p",			  /* collect_format */
    xvalue_object_collect_value, /* collect_value */
    "p",			  /* lcopy_format */
    xvalue_object_lcopy_value,	  /* lcopy_value */
  };
  xtype_t type G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;

  g_return_if_fail (initialized == FALSE);
  initialized = TRUE;

  /* XTYPE_OBJECT
   */
  info.value_table = &value_table;
  type = xtype_register_fundamental (XTYPE_OBJECT, g_intern_static_string ("xobject_t"), &info, &finfo, 0);
  xassert (type == XTYPE_OBJECT);
  xvalue_register_transform_func (XTYPE_OBJECT, XTYPE_OBJECT, xvalue_object_transform_value);

#if G_ENABLE_DEBUG
  /* We cannot use GOBJECT_IF_DEBUG here because of the G_HAS_CONSTRUCTORS
   * conditional in between, as the C spec leaves conditionals inside macro
   * expansions as undefined behavior. Only GCC and Clang are known to work
   * but compilation breaks on MSVC.
   *
   * See: https://bugzilla.gnome.org/show_bug.cgi?id=769504
   */
  if (_xtype_debug_flags & XTYPE_DEBUG_OBJECTS) \
    {
      debug_objects_ht = xhash_table_new (g_direct_hash, NULL);
# ifndef G_HAS_CONSTRUCTORS
      g_atexit (debug_objects_atexit);
# endif /* G_HAS_CONSTRUCTORS */
    }
#endif /* G_ENABLE_DEBUG */
}

static void
xobject_base_class_init (xobject_class_t *class)
{
  xobject_class_t *pclass = xtype_class_peek_parent (class);

  /* Don't inherit HAS_DERIVED_CLASS flag from parent class */
  class->flags &= ~CLASS_HAS_DERIVED_CLASS_FLAG;

  if (pclass)
    pclass->flags |= CLASS_HAS_DERIVED_CLASS_FLAG;

  /* reset instance specific fields and methods that don't get inherited */
  class->construct_properties = pclass ? xslist_copy (pclass->construct_properties) : NULL;
  class->get_property = NULL;
  class->set_property = NULL;
}

static void
xobject_base_class_finalize (xobject_class_t *class)
{
  xlist_t *list, *node;

  _g_signals_destroy (G_OBJECT_CLASS_TYPE (class));

  xslist_free (class->construct_properties);
  class->construct_properties = NULL;
  list = xparam_spec_pool_list_owned (pspec_pool, G_OBJECT_CLASS_TYPE (class));
  for (node = list; node; node = node->next)
    {
      xparam_spec_t *pspec = node->data;

      xparam_spec_pool_remove (pspec_pool, pspec);
      PARAM_SPEC_SET_PARAM_ID (pspec, 0);
      xparam_spec_unref (pspec);
    }
  xlist_free (list);
}

static void
xobject_do_class_init (xobject_class_t *class)
{
  /* read the comment about typedef struct CArray; on why not to change this quark */
  quark_closure_array = g_quark_from_static_string ("xobject-closure-array");

  quark_weak_refs = g_quark_from_static_string ("xobject-weak-references");
  quark_weak_locations = g_quark_from_static_string ("xobject-weak-locations");
  quark_toggle_refs = g_quark_from_static_string ("xobject-toggle-references");
  quark_notify_queue = g_quark_from_static_string ("xobject-notify-queue");
  quark_in_construction = g_quark_from_static_string ("xobject-in-construction");
  pspec_pool = xparam_spec_pool_new (TRUE);

  class->constructor = xobject_constructor;
  class->constructed = xobject_constructed;
  class->set_property = xobject_do_set_property;
  class->get_property = xobject_do_get_property;
  class->dispose = xobject_real_dispose;
  class->finalize = xobject_finalize;
  class->dispatch_properties_changed = xobject_dispatch_properties_changed;
  class->notify = NULL;

  /**
   * xobject_t::notify:
   * @gobject: the object which received the signal.
   * @pspec: the #xparam_spec_t of the property which changed.
   *
   * The notify signal is emitted on an object when one of its properties has
   * its value set through xobject_set_property(), xobject_set(), et al.
   *
   * Note that getting this signal doesn???t itself guarantee that the value of
   * the property has actually changed. When it is emitted is determined by the
   * derived xobject_t class. If the implementor did not create the property with
   * %XPARAM_EXPLICIT_NOTIFY, then any call to xobject_set_property() results
   * in ::notify being emitted, even if the new value is the same as the old.
   * If they did pass %XPARAM_EXPLICIT_NOTIFY, then this signal is emitted only
   * when they explicitly call xobject_notify() or xobject_notify_by_pspec(),
   * and common practice is to do that only when the value has actually changed.
   *
   * This signal is typically used to obtain change notification for a
   * single property, by specifying the property name as a detail in the
   * xsignal_connect() call, like this:
   *
   * |[<!-- language="C" -->
   * xsignal_connect (text_view->buffer, "notify::paste-target-list",
   *                   G_CALLBACK (gtk_text_view_target_list_notify),
   *                   text_view)
   * ]|
   *
   * It is important to note that you must use
   * [canonical parameter names][canonical-parameter-names] as
   * detail strings for the notify signal.
   */
  gobject_signals[NOTIFY] =
    xsignal_new (g_intern_static_string ("notify"),
		  XTYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED | G_SIGNAL_NO_HOOKS | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (xobject_class_t, notify),
		  NULL, NULL,
		  NULL,
		  XTYPE_NONE,
		  1, XTYPE_PARAM);

  /* Install a check function that we'll use to verify that classes that
   * implement an interface implement all properties for that interface
   */
  xtype_add_interface_check (NULL, object_interface_check_properties);
}

static inline xboolean_t
install_property_internal (xtype_t       g_type,
			   xuint_t       property_id,
			   xparam_spec_t *pspec)
{
  if (xparam_spec_pool_lookup (pspec_pool, pspec->name, g_type, FALSE))
    {
      g_warning ("When installing property: type '%s' already has a property named '%s'",
		 xtype_name (g_type),
		 pspec->name);
      return FALSE;
    }

  xparam_spec_ref_sink (pspec);
  PARAM_SPEC_SET_PARAM_ID (pspec, property_id);
  xparam_spec_pool_insert (pspec_pool, pspec, g_type);
  return TRUE;
}

static xboolean_t
validate_pspec_to_install (xparam_spec_t *pspec)
{
  xreturn_val_if_fail (X_IS_PARAM_SPEC (pspec), FALSE);
  xreturn_val_if_fail (PARAM_SPEC_PARAM_ID (pspec) == 0, FALSE);	/* paranoid */

  xreturn_val_if_fail (pspec->flags & (XPARAM_READABLE | XPARAM_WRITABLE), FALSE);

  if (pspec->flags & XPARAM_CONSTRUCT)
    xreturn_val_if_fail ((pspec->flags & XPARAM_CONSTRUCT_ONLY) == 0, FALSE);

  if (pspec->flags & (XPARAM_CONSTRUCT | XPARAM_CONSTRUCT_ONLY))
    xreturn_val_if_fail (pspec->flags & XPARAM_WRITABLE, FALSE);

  return TRUE;
}

static xboolean_t
validate_and_install_class_property (xobject_class_t *class,
                                     xtype_t         oclass_type,
                                     xtype_t         parent_type,
                                     xuint_t         property_id,
                                     xparam_spec_t   *pspec)
{
  if (!validate_pspec_to_install (pspec))
    return FALSE;

  if (pspec->flags & XPARAM_WRITABLE)
    xreturn_val_if_fail (class->set_property != NULL, FALSE);
  if (pspec->flags & XPARAM_READABLE)
    xreturn_val_if_fail (class->get_property != NULL, FALSE);

  class->flags |= CLASS_HAS_PROPS_FLAG;
  if (install_property_internal (oclass_type, property_id, pspec))
    {
      if (pspec->flags & (XPARAM_CONSTRUCT | XPARAM_CONSTRUCT_ONLY))
        class->construct_properties = xslist_append (class->construct_properties, pspec);

      /* for property overrides of construct properties, we have to get rid
       * of the overridden inherited construct property
       */
      pspec = xparam_spec_pool_lookup (pspec_pool, pspec->name, parent_type, TRUE);
      if (pspec && pspec->flags & (XPARAM_CONSTRUCT | XPARAM_CONSTRUCT_ONLY))
        class->construct_properties = xslist_remove (class->construct_properties, pspec);

      return TRUE;
    }
  else
    return FALSE;
}

/**
 * xobject_class_install_property:
 * @oclass: a #xobject_class_t
 * @property_id: the id for the new property
 * @pspec: the #xparam_spec_t for the new property
 *
 * Installs a new property.
 *
 * All properties should be installed during the class initializer.  It
 * is possible to install properties after that, but doing so is not
 * recommend, and specifically, is not guaranteed to be thread-safe vs.
 * use of properties on the same type on other threads.
 *
 * Note that it is possible to redefine a property in a derived class,
 * by installing a property with the same name. This can be useful at times,
 * e.g. to change the range of allowed values or the default value.
 */
void
xobject_class_install_property (xobject_class_t *class,
				 xuint_t	       property_id,
				 xparam_spec_t   *pspec)
{
  xtype_t oclass_type, parent_type;

  g_return_if_fail (X_IS_OBJECT_CLASS (class));
  g_return_if_fail (property_id > 0);

  oclass_type = G_OBJECT_CLASS_TYPE (class);
  parent_type = xtype_parent (oclass_type);

  if (CLASS_HAS_DERIVED_CLASS (class))
    xerror ("Attempt to add property %s::%s to class after it was derived", G_OBJECT_CLASS_NAME (class), pspec->name);

  (void) validate_and_install_class_property (class,
                                              oclass_type,
                                              parent_type,
                                              property_id,
                                              pspec);
}

/**
 * xobject_class_install_properties:
 * @oclass: a #xobject_class_t
 * @n_pspecs: the length of the #GParamSpecs array
 * @pspecs: (array length=n_pspecs): the #GParamSpecs array
 *   defining the new properties
 *
 * Installs new properties from an array of #GParamSpecs.
 *
 * All properties should be installed during the class initializer.  It
 * is possible to install properties after that, but doing so is not
 * recommend, and specifically, is not guaranteed to be thread-safe vs.
 * use of properties on the same type on other threads.
 *
 * The property id of each property is the index of each #xparam_spec_t in
 * the @pspecs array.
 *
 * The property id of 0 is treated specially by #xobject_t and it should not
 * be used to store a #xparam_spec_t.
 *
 * This function should be used if you plan to use a static array of
 * #GParamSpecs and xobject_notify_by_pspec(). For instance, this
 * class initialization:
 *
 * |[<!-- language="C" -->
 * enum {
 *   PROP_0, PROP_FOO, PROP_BAR, N_PROPERTIES
 * };
 *
 * static xparam_spec_t *obj_properties[N_PROPERTIES] = { NULL, };
 *
 * static void
 * my_object_class_init (xobject_class_t *klass)
 * {
 *   xobject_class_t *xobject_class = XOBJECT_CLASS (klass);
 *
 *   obj_properties[PROP_FOO] =
 *     xparam_spec_int ("foo", "foo_t", "foo_t",
 *                       -1, G_MAXINT,
 *                       0,
 *                       XPARAM_READWRITE);
 *
 *   obj_properties[PROP_BAR] =
 *     xparam_spec_string ("bar", "Bar", "Bar",
 *                          NULL,
 *                          XPARAM_READWRITE);
 *
 *   xobject_class->set_property = my_object_set_property;
 *   xobject_class->get_property = my_object_get_property;
 *   xobject_class_install_properties (xobject_class,
 *                                      N_PROPERTIES,
 *                                      obj_properties);
 * }
 * ]|
 *
 * allows calling xobject_notify_by_pspec() to notify of property changes:
 *
 * |[<!-- language="C" -->
 * void
 * my_object_set_foo (xobject_t *self, xint_t foo)
 * {
 *   if (self->foo != foo)
 *     {
 *       self->foo = foo;
 *       xobject_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_FOO]);
 *     }
 *  }
 * ]|
 *
 * Since: 2.26
 */
void
xobject_class_install_properties (xobject_class_t  *oclass,
                                   xuint_t          n_pspecs,
                                   xparam_spec_t   **pspecs)
{
  xtype_t oclass_type, parent_type;
  xuint_t i;

  g_return_if_fail (X_IS_OBJECT_CLASS (oclass));
  g_return_if_fail (n_pspecs > 1);
  g_return_if_fail (pspecs[0] == NULL);

  if (CLASS_HAS_DERIVED_CLASS (oclass))
    xerror ("Attempt to add properties to %s after it was derived",
             G_OBJECT_CLASS_NAME (oclass));

  oclass_type = G_OBJECT_CLASS_TYPE (oclass);
  parent_type = xtype_parent (oclass_type);

  /* we skip the first element of the array as it would have a 0 prop_id */
  for (i = 1; i < n_pspecs; i++)
    {
      xparam_spec_t *pspec = pspecs[i];

      if (!validate_and_install_class_property (oclass,
                                                oclass_type,
                                                parent_type,
                                                i,
                                                pspec))
        {
          break;
        }
    }
}

/**
 * xobject_interface_install_property:
 * @x_iface: (type xobject_t.TypeInterface): any interface vtable for the
 *    interface, or the default
 *  vtable for the interface.
 * @pspec: the #xparam_spec_t for the new property
 *
 * Add a property to an interface; this is only useful for interfaces
 * that are added to xobject-derived types. Adding a property to an
 * interface forces all objects classes with that interface to have a
 * compatible property. The compatible property could be a newly
 * created #xparam_spec_t, but normally
 * xobject_class_override_property() will be used so that the object
 * class only needs to provide an implementation and inherits the
 * property description, default value, bounds, and so forth from the
 * interface property.
 *
 * This function is meant to be called from the interface's default
 * vtable initialization function (the @class_init member of
 * #xtype_info_t.) It must not be called after after @class_init has
 * been called for any object types implementing this interface.
 *
 * If @pspec is a floating reference, it will be consumed.
 *
 * Since: 2.4
 */
void
xobject_interface_install_property (xpointer_t      x_iface,
				     xparam_spec_t   *pspec)
{
  xtype_interface_t *iface_class = x_iface;

  g_return_if_fail (XTYPE_IS_INTERFACE (iface_class->g_type));
  g_return_if_fail (!X_IS_PARAM_SPEC_OVERRIDE (pspec)); /* paranoid */

  if (!validate_pspec_to_install (pspec))
    return;

  (void) install_property_internal (iface_class->g_type, 0, pspec);
}

/**
 * xobject_class_find_property:
 * @oclass: a #xobject_class_t
 * @property_name: the name of the property to look up
 *
 * Looks up the #xparam_spec_t for a property of a class.
 *
 * Returns: (transfer none): the #xparam_spec_t for the property, or
 *          %NULL if the class doesn't have a property of that name
 */
xparam_spec_t*
xobject_class_find_property (xobject_class_t *class,
			      const xchar_t  *property_name)
{
  xparam_spec_t *pspec;
  xparam_spec_t *redirect;

  xreturn_val_if_fail (X_IS_OBJECT_CLASS (class), NULL);
  xreturn_val_if_fail (property_name != NULL, NULL);

  pspec = xparam_spec_pool_lookup (pspec_pool,
				    property_name,
				    G_OBJECT_CLASS_TYPE (class),
				    TRUE);
  if (pspec)
    {
      redirect = xparam_spec_get_redirect_target (pspec);
      if (redirect)
	return redirect;
      else
	return pspec;
    }
  else
    return NULL;
}

/**
 * xobject_interface_find_property:
 * @x_iface: (type xobject_t.TypeInterface): any interface vtable for the
 *  interface, or the default vtable for the interface
 * @property_name: name of a property to look up.
 *
 * Find the #xparam_spec_t with the given name for an
 * interface. Generally, the interface vtable passed in as @x_iface
 * will be the default vtable from xtype_default_interface_ref(), or,
 * if you know the interface has already been loaded,
 * xtype_default_interface_peek().
 *
 * Since: 2.4
 *
 * Returns: (transfer none): the #xparam_spec_t for the property of the
 *          interface with the name @property_name, or %NULL if no
 *          such property exists.
 */
xparam_spec_t*
xobject_interface_find_property (xpointer_t      x_iface,
				  const xchar_t  *property_name)
{
  xtype_interface_t *iface_class = x_iface;

  xreturn_val_if_fail (XTYPE_IS_INTERFACE (iface_class->g_type), NULL);
  xreturn_val_if_fail (property_name != NULL, NULL);

  return xparam_spec_pool_lookup (pspec_pool,
				   property_name,
				   iface_class->g_type,
				   FALSE);
}

/**
 * xobject_class_override_property:
 * @oclass: a #xobject_class_t
 * @property_id: the new property ID
 * @name: the name of a property registered in a parent class or
 *  in an interface of this class.
 *
 * Registers @property_id as referring to a property with the name
 * @name in a parent class or in an interface implemented by @oclass.
 * This allows this class to "override" a property implementation in
 * a parent class or to provide the implementation of a property from
 * an interface.
 *
 * Internally, overriding is implemented by creating a property of type
 * #GParamSpecOverride; generally operations that query the properties of
 * the object class, such as xobject_class_find_property() or
 * xobject_class_list_properties() will return the overridden
 * property. However, in one case, the @construct_properties argument of
 * the @constructor virtual function, the #GParamSpecOverride is passed
 * instead, so that the @param_id field of the #xparam_spec_t will be
 * correct.  For virtually all uses, this makes no difference. If you
 * need to get the overridden property, you can call
 * xparam_spec_get_redirect_target().
 *
 * Since: 2.4
 */
void
xobject_class_override_property (xobject_class_t *oclass,
				  xuint_t         property_id,
				  const xchar_t  *name)
{
  xparam_spec_t *overridden = NULL;
  xparam_spec_t *new;
  xtype_t parent_type;

  g_return_if_fail (X_IS_OBJECT_CLASS (oclass));
  g_return_if_fail (property_id > 0);
  g_return_if_fail (name != NULL);

  /* Find the overridden property; first check parent types
   */
  parent_type = xtype_parent (G_OBJECT_CLASS_TYPE (oclass));
  if (parent_type != XTYPE_NONE)
    overridden = xparam_spec_pool_lookup (pspec_pool,
					   name,
					   parent_type,
					   TRUE);
  if (!overridden)
    {
      xtype_t *ifaces;
      xuint_t n_ifaces;

      /* Now check interfaces
       */
      ifaces = xtype_interfaces (G_OBJECT_CLASS_TYPE (oclass), &n_ifaces);
      while (n_ifaces-- && !overridden)
	{
	  overridden = xparam_spec_pool_lookup (pspec_pool,
						 name,
						 ifaces[n_ifaces],
						 FALSE);
	}

      g_free (ifaces);
    }

  if (!overridden)
    {
      g_warning ("%s: Can't find property to override for '%s::%s'",
		 G_STRFUNC, G_OBJECT_CLASS_NAME (oclass), name);
      return;
    }

  new = xparam_spec_override (name, overridden);
  xobject_class_install_property (oclass, property_id, new);
}

/**
 * xobject_class_list_properties:
 * @oclass: a #xobject_class_t
 * @n_properties: (out): return location for the length of the returned array
 *
 * Get an array of #xparam_spec_t* for all properties of a class.
 *
 * Returns: (array length=n_properties) (transfer container): an array of
 *          #xparam_spec_t* which should be freed after use
 */
xparam_spec_t** /* free result */
xobject_class_list_properties (xobject_class_t *class,
				xuint_t        *n_properties_p)
{
  xparam_spec_t **pspecs;
  xuint_t n;

  xreturn_val_if_fail (X_IS_OBJECT_CLASS (class), NULL);

  pspecs = xparam_spec_pool_list (pspec_pool,
				   G_OBJECT_CLASS_TYPE (class),
				   &n);
  if (n_properties_p)
    *n_properties_p = n;

  return pspecs;
}

/**
 * xobject_interface_list_properties:
 * @x_iface: (type xobject_t.TypeInterface): any interface vtable for the
 *  interface, or the default vtable for the interface
 * @n_properties_p: (out): location to store number of properties returned.
 *
 * Lists the properties of an interface.Generally, the interface
 * vtable passed in as @x_iface will be the default vtable from
 * xtype_default_interface_ref(), or, if you know the interface has
 * already been loaded, xtype_default_interface_peek().
 *
 * Since: 2.4
 *
 * Returns: (array length=n_properties_p) (transfer container): a
 *          pointer to an array of pointers to #xparam_spec_t
 *          structures. The paramspecs are owned by GLib, but the
 *          array should be freed with g_free() when you are done with
 *          it.
 */
xparam_spec_t**
xobject_interface_list_properties (xpointer_t      x_iface,
				    xuint_t        *n_properties_p)
{
  xtype_interface_t *iface_class = x_iface;
  xparam_spec_t **pspecs;
  xuint_t n;

  xreturn_val_if_fail (XTYPE_IS_INTERFACE (iface_class->g_type), NULL);

  pspecs = xparam_spec_pool_list (pspec_pool,
				   iface_class->g_type,
				   &n);
  if (n_properties_p)
    *n_properties_p = n;

  return pspecs;
}

static inline xuint_t
object_get_optional_flags (xobject_t *object)
{
#ifdef HAVE_OPTIONAL_FLAGS
  xobject_real_t *real = (xobject_real_t *)object;
  return (xuint_t)g_atomic_int_get (&real->optional_flags);
#else
  return 0;
#endif
}

static inline void
object_set_optional_flags (xobject_t *object,
                          xuint_t flags)
{
#ifdef HAVE_OPTIONAL_FLAGS
  xobject_real_t *real = (xobject_real_t *)object;
  g_atomic_int_or (&real->optional_flags, flags);
#endif
}

static inline void
object_unset_optional_flags (xobject_t *object,
                            xuint_t flags)
{
#ifdef HAVE_OPTIONAL_FLAGS
  xobject_real_t *real = (xobject_real_t *)object;
  g_atomic_int_and (&real->optional_flags, ~flags);
#endif
}

xboolean_t
_xobject_has_signal_handler  (xobject_t *object)
{
#ifdef HAVE_OPTIONAL_FLAGS
  return (object_get_optional_flags (object) & OPTIONAL_FLAG_HAS_SIGNAL_HANDLER) != 0;
#else
  return TRUE;
#endif
}

void
_xobject_set_has_signal_handler (xobject_t     *object)
{
#ifdef HAVE_OPTIONAL_FLAGS
  object_set_optional_flags (object, OPTIONAL_FLAG_HAS_SIGNAL_HANDLER);
#endif
}

static inline xboolean_t
object_in_construction (xobject_t *object)
{
#ifdef HAVE_OPTIONAL_FLAGS
  return (object_get_optional_flags (object) & OPTIONAL_FLAG_IN_CONSTRUCTION) != 0;
#else
  return g_datalist_id_get_data (&object->qdata, quark_in_construction) != NULL;
#endif
}

static inline void
set_object_in_construction (xobject_t *object)
{
#ifdef HAVE_OPTIONAL_FLAGS
  object_set_optional_flags (object, OPTIONAL_FLAG_IN_CONSTRUCTION);
#else
  g_datalist_id_set_data (&object->qdata, quark_in_construction, object);
#endif
}

static inline void
unset_object_in_construction (xobject_t *object)
{
#ifdef HAVE_OPTIONAL_FLAGS
  object_unset_optional_flags (object, OPTIONAL_FLAG_IN_CONSTRUCTION);
#else
  g_datalist_id_set_data (&object->qdata, quark_in_construction, NULL);
#endif
}

static void
xobject_init (xobject_t		*object,
	       xobject_class_t	*class)
{
  object->ref_count = 1;
  object->qdata = NULL;

  if (CLASS_HAS_PROPS (class))
    {
      /* freeze object's notification queue, xobject_newv() preserves pairedness */
      xobject_notify_queue_freeze (object, FALSE);
    }

  if (CLASS_HAS_CUSTOM_CONSTRUCTOR (class))
    {
      /* mark object in-construction for notify_queue_thaw() and to allow construct-only properties */
      set_object_in_construction (object);
    }

  GOBJECT_IF_DEBUG (OBJECTS,
    {
      G_LOCK (debug_objects);
      debug_objects_count++;
      xhash_table_add (debug_objects_ht, object);
      G_UNLOCK (debug_objects);
    });
}

static void
xobject_do_set_property (xobject_t      *object,
			  xuint_t         property_id,
			  const xvalue_t *value,
			  xparam_spec_t   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
xobject_do_get_property (xobject_t     *object,
			  xuint_t        property_id,
			  xvalue_t      *value,
			  xparam_spec_t  *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
xobject_real_dispose (xobject_t *object)
{
  xsignal_handlers_destroy (object);
  g_datalist_id_set_data (&object->qdata, quark_closure_array, NULL);
  g_datalist_id_set_data (&object->qdata, quark_weak_refs, NULL);
  g_datalist_id_set_data (&object->qdata, quark_weak_locations, NULL);
}

#ifdef G_ENABLE_DEBUG
static xboolean_t
floating_check (xobject_t *object)
{
  static const char *g_enable_diagnostic = NULL;

  if (G_UNLIKELY (g_enable_diagnostic == NULL))
    {
      g_enable_diagnostic = g_getenv ("G_ENABLE_DIAGNOSTIC");
      if (g_enable_diagnostic == NULL)
        g_enable_diagnostic = "0";
    }

  if (g_enable_diagnostic[0] == '1')
    return xobject_is_floating (object);

  return FALSE;
}
#endif

static void
xobject_finalize (xobject_t *object)
{
  if (object_in_construction (object))
    {
      g_critical ("object %s %p finalized while still in-construction",
                  G_OBJECT_TYPE_NAME (object), object);
    }

#ifdef G_ENABLE_DEBUG
 if (floating_check (object))
   {
      g_critical ("A floating object %s %p was finalized. This means that someone\n"
                  "called xobject_unref() on an object that had only a floating\n"
                  "reference; the initial floating reference is not owned by anyone\n"
                  "and must be removed with xobject_ref_sink().",
                  G_OBJECT_TYPE_NAME (object), object);
   }
#endif

  g_datalist_clear (&object->qdata);

  GOBJECT_IF_DEBUG (OBJECTS,
    {
      G_LOCK (debug_objects);
      xassert (xhash_table_contains (debug_objects_ht, object));
      xhash_table_remove (debug_objects_ht, object);
      debug_objects_count--;
      G_UNLOCK (debug_objects);
    });
}

static void
xobject_dispatch_properties_changed (xobject_t     *object,
				      xuint_t        n_pspecs,
				      xparam_spec_t **pspecs)
{
  xuint_t i;

  for (i = 0; i < n_pspecs; i++)
    xsignal_emit (object, gobject_signals[NOTIFY], xparam_spec_get_name_quark (pspecs[i]), pspecs[i]);
}

/**
 * xobject_run_dispose:
 * @object: a #xobject_t
 *
 * Releases all references to other objects. This can be used to break
 * reference cycles.
 *
 * This function should only be called from object system implementations.
 */
void
xobject_run_dispose (xobject_t *object)
{
  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (g_atomic_int_get (&object->ref_count) > 0);

  xobject_ref (object);
  TRACE (GOBJECT_OBJECT_DISPOSE(object,XTYPE_FROM_INSTANCE(object), 0));
  G_OBJECT_GET_CLASS (object)->dispose (object);
  TRACE (GOBJECT_OBJECT_DISPOSE_END(object,XTYPE_FROM_INSTANCE(object), 0));
  xobject_unref (object);
}

/**
 * xobject_freeze_notify:
 * @object: a #xobject_t
 *
 * Increases the freeze count on @object. If the freeze count is
 * non-zero, the emission of "notify" signals on @object is
 * stopped. The signals are queued until the freeze count is decreased
 * to zero. Duplicate notifications are squashed so that at most one
 * #xobject_t::notify signal is emitted for each property modified while the
 * object is frozen.
 *
 * This is necessary for accessors that modify multiple properties to prevent
 * premature notification while the object is still being modified.
 */
void
xobject_freeze_notify (xobject_t *object)
{
  g_return_if_fail (X_IS_OBJECT (object));

  if (g_atomic_int_get (&object->ref_count) == 0)
    return;

  xobject_ref (object);
  xobject_notify_queue_freeze (object, FALSE);
  xobject_unref (object);
}

static xparam_spec_t *
get_notify_pspec (xparam_spec_t *pspec)
{
  xparam_spec_t *redirected;

  /* we don't notify on non-READABLE parameters */
  if (~pspec->flags & XPARAM_READABLE)
    return NULL;

  /* if the paramspec is redirected, notify on the target */
  redirected = xparam_spec_get_redirect_target (pspec);
  if (redirected != NULL)
    return redirected;

  /* else, notify normally */
  return pspec;
}

static inline void
xobject_notify_by_spec_internal (xobject_t    *object,
				  xparam_spec_t *pspec)
{
  xparam_spec_t *notify_pspec;

  notify_pspec = get_notify_pspec (pspec);

  if (notify_pspec != NULL)
    {
      xobject_notify_queue_t *nqueue;

      /* conditional freeze: only increase freeze count if already frozen */
      nqueue = xobject_notify_queue_freeze (object, TRUE);

      if (nqueue != NULL)
        {
          /* we're frozen, so add to the queue and release our freeze */
          xobject_notify_queue_add (object, nqueue, notify_pspec);
          xobject_notify_queue_thaw (object, nqueue);
        }
      else
        /* not frozen, so just dispatch the notification directly */
        G_OBJECT_GET_CLASS (object)
          ->dispatch_properties_changed (object, 1, &notify_pspec);
    }
}

/**
 * xobject_notify:
 * @object: a #xobject_t
 * @property_name: the name of a property installed on the class of @object.
 *
 * Emits a "notify" signal for the property @property_name on @object.
 *
 * When possible, eg. when signaling a property change from within the class
 * that registered the property, you should use xobject_notify_by_pspec()
 * instead.
 *
 * Note that emission of the notify signal may be blocked with
 * xobject_freeze_notify(). In this case, the signal emissions are queued
 * and will be emitted (in reverse order) when xobject_thaw_notify() is
 * called.
 */
void
xobject_notify (xobject_t     *object,
		 const xchar_t *property_name)
{
  xparam_spec_t *pspec;

  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);
  if (g_atomic_int_get (&object->ref_count) == 0)
    return;

  xobject_ref (object);
  /* We don't need to get the redirect target
   * (by, e.g. calling xobject_class_find_property())
   * because xobject_notify_queue_add() does that
   */
  pspec = xparam_spec_pool_lookup (pspec_pool,
				    property_name,
				    G_OBJECT_TYPE (object),
				    TRUE);

  if (!pspec)
    g_warning ("%s: object class '%s' has no property named '%s'",
	       G_STRFUNC,
	       G_OBJECT_TYPE_NAME (object),
	       property_name);
  else
    xobject_notify_by_spec_internal (object, pspec);
  xobject_unref (object);
}

/**
 * xobject_notify_by_pspec:
 * @object: a #xobject_t
 * @pspec: the #xparam_spec_t of a property installed on the class of @object.
 *
 * Emits a "notify" signal for the property specified by @pspec on @object.
 *
 * This function omits the property name lookup, hence it is faster than
 * xobject_notify().
 *
 * One way to avoid using xobject_notify() from within the
 * class that registered the properties, and using xobject_notify_by_pspec()
 * instead, is to store the xparam_spec_t used with
 * xobject_class_install_property() inside a static array, e.g.:
 *
 *|[<!-- language="C" -->
 *   enum
 *   {
 *     PROP_0,
 *     PROP_FOO,
 *     PROP_LAST
 *   };
 *
 *   static xparam_spec_t *properties[PROP_LAST];
 *
 *   static void
 *   my_object_class_init (xobject_class_t *klass)
 *   {
 *     properties[PROP_FOO] = xparam_spec_int ("foo", "foo_t", "The foo",
 *                                              0, 100,
 *                                              50,
 *                                              XPARAM_READWRITE);
 *     xobject_class_install_property (xobject_class,
 *                                      PROP_FOO,
 *                                      properties[PROP_FOO]);
 *   }
 * ]|
 *
 * and then notify a change on the "foo" property with:
 *
 * |[<!-- language="C" -->
 *   xobject_notify_by_pspec (self, properties[PROP_FOO]);
 * ]|
 *
 * Since: 2.26
 */
void
xobject_notify_by_pspec (xobject_t    *object,
			  xparam_spec_t *pspec)
{

  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (X_IS_PARAM_SPEC (pspec));

  if (g_atomic_int_get (&object->ref_count) == 0)
    return;

  xobject_ref (object);
  xobject_notify_by_spec_internal (object, pspec);
  xobject_unref (object);
}

/**
 * xobject_thaw_notify:
 * @object: a #xobject_t
 *
 * Reverts the effect of a previous call to
 * xobject_freeze_notify(). The freeze count is decreased on @object
 * and when it reaches zero, queued "notify" signals are emitted.
 *
 * Duplicate notifications for each property are squashed so that at most one
 * #xobject_t::notify signal is emitted for each property, in the reverse order
 * in which they have been queued.
 *
 * It is an error to call this function when the freeze count is zero.
 */
void
xobject_thaw_notify (xobject_t *object)
{
  xobject_notify_queue_t *nqueue;

  g_return_if_fail (X_IS_OBJECT (object));
  if (g_atomic_int_get (&object->ref_count) == 0)
    return;

  xobject_ref (object);

  /* FIXME: Freezing is the only way to get at the notify queue.
   * So we freeze once and then thaw twice.
   */
  nqueue = xobject_notify_queue_freeze (object, FALSE);
  xobject_notify_queue_thaw (object, nqueue);
  xobject_notify_queue_thaw (object, nqueue);

  xobject_unref (object);
}

static void
consider_issuing_property_deprecation_warning (const xparam_spec_t *pspec)
{
  static xhashtable_t *already_warned_table;
  static const xchar_t *enable_diagnostic;
  static xmutex_t already_warned_lock;
  xboolean_t already;

  if (!(pspec->flags & XPARAM_DEPRECATED))
    return;

  if (g_once_init_enter (&enable_diagnostic))
    {
      const xchar_t *value = g_getenv ("G_ENABLE_DIAGNOSTIC");

      if (!value)
        value = "0";

      g_once_init_leave (&enable_diagnostic, value);
    }

  if (enable_diagnostic[0] == '0')
    return;

  /* We hash only on property names: this means that we could end up in
   * a situation where we fail to emit a warning about a pair of
   * same-named deprecated properties used on two separate types.
   * That's pretty unlikely to occur, and even if it does, you'll still
   * have seen the warning for the first one...
   *
   * Doing it this way lets us hash directly on the (interned) property
   * name pointers.
   */
  g_mutex_lock (&already_warned_lock);

  if (already_warned_table == NULL)
    already_warned_table = xhash_table_new (NULL, NULL);

  already = xhash_table_contains (already_warned_table, (xpointer_t) pspec->name);
  if (!already)
    xhash_table_add (already_warned_table, (xpointer_t) pspec->name);

  g_mutex_unlock (&already_warned_lock);

  if (!already)
    g_warning ("The property %s:%s is deprecated and shouldn't be used "
               "anymore. It will be removed in a future version.",
               xtype_name (pspec->owner_type), pspec->name);
}

static inline void
object_get_property (xobject_t     *object,
		     xparam_spec_t  *pspec,
		     xvalue_t      *value)
{
  xobject_class_t *class = xtype_class_peek (pspec->owner_type);
  xuint_t param_id = PARAM_SPEC_PARAM_ID (pspec);
  xparam_spec_t *redirect;

  if (class == NULL)
    {
      g_warning ("'%s::%s' is not a valid property name; '%s' is not a xobject_t subtype",
                 xtype_name (pspec->owner_type), pspec->name, xtype_name (pspec->owner_type));
      return;
    }

  redirect = xparam_spec_get_redirect_target (pspec);
  if (redirect)
    pspec = redirect;

  consider_issuing_property_deprecation_warning (pspec);

  class->get_property (object, param_id, value, pspec);
}

static inline void
object_set_property (xobject_t             *object,
		     xparam_spec_t          *pspec,
		     const xvalue_t        *value,
		     xobject_notify_queue_t  *nqueue)
{
  xvalue_t tmp_value = G_VALUE_INIT;
  xobject_class_t *class = xtype_class_peek (pspec->owner_type);
  xuint_t param_id = PARAM_SPEC_PARAM_ID (pspec);
  xparam_spec_t *redirect;

  if (class == NULL)
    {
      g_warning ("'%s::%s' is not a valid property name; '%s' is not a xobject_t subtype",
                 xtype_name (pspec->owner_type), pspec->name, xtype_name (pspec->owner_type));
      return;
    }

  redirect = xparam_spec_get_redirect_target (pspec);
  if (redirect)
    pspec = redirect;

  /* provide a copy to work from, convert (if necessary) and validate */
  xvalue_init (&tmp_value, pspec->value_type);
  if (!xvalue_transform (value, &tmp_value))
    g_warning ("unable to set property '%s' of type '%s' from value of type '%s'",
	       pspec->name,
	       xtype_name (pspec->value_type),
	       G_VALUE_TYPE_NAME (value));
  else if (g_param_value_validate (pspec, &tmp_value) && !(pspec->flags & XPARAM_LAX_VALIDATION))
    {
      xchar_t *contents = xstrdup_value_contents (value);

      g_warning ("value \"%s\" of type '%s' is invalid or out of range for property '%s' of type '%s'",
		 contents,
		 G_VALUE_TYPE_NAME (value),
		 pspec->name,
		 xtype_name (pspec->value_type));
      g_free (contents);
    }
  else
    {
      class->set_property (object, param_id, &tmp_value, pspec);

      if (~pspec->flags & XPARAM_EXPLICIT_NOTIFY &&
          pspec->flags & XPARAM_READABLE)
        xobject_notify_queue_add (object, nqueue, pspec);
    }
  xvalue_unset (&tmp_value);
}

static void
object_interface_check_properties (xpointer_t check_data,
				   xpointer_t x_iface)
{
  xtype_interface_t *iface_class = x_iface;
  xobject_class_t *class;
  xtype_t iface_type = iface_class->g_type;
  xparam_spec_t **pspecs;
  xuint_t n;

  class = xtype_class_ref (iface_class->g_instance_type);

  if (class == NULL)
    return;

  if (!X_IS_OBJECT_CLASS (class))
    goto out;

  pspecs = xparam_spec_pool_list (pspec_pool, iface_type, &n);

  while (n--)
    {
      xparam_spec_t *class_pspec = xparam_spec_pool_lookup (pspec_pool,
							  pspecs[n]->name,
							  G_OBJECT_CLASS_TYPE (class),
							  TRUE);

      if (!class_pspec)
	{
	  g_critical ("Object class %s doesn't implement property "
		      "'%s' from interface '%s'",
		      xtype_name (G_OBJECT_CLASS_TYPE (class)),
		      pspecs[n]->name,
		      xtype_name (iface_type));

	  continue;
	}

      /* We do a number of checks on the properties of an interface to
       * make sure that all classes implementing the interface are
       * overriding the properties correctly.
       *
       * We do the checks in order of importance so that we can give
       * more useful error messages first.
       *
       * First, we check that the implementation doesn't remove the
       * basic functionality (readability, writability) advertised by
       * the interface.  Next, we check that it doesn't introduce
       * additional restrictions (such as construct-only).  Finally, we
       * make sure the types are compatible.
       */

#define SUBSET(a,b,mask) (((a) & ~(b) & (mask)) == 0)
      /* If the property on the interface is readable then the
       * implementation must be readable.  If the interface is writable
       * then the implementation must be writable.
       */
      if (!SUBSET (pspecs[n]->flags, class_pspec->flags, XPARAM_READABLE | XPARAM_WRITABLE))
        {
          g_critical ("Flags for property '%s' on class '%s' remove functionality compared with the "
                      "property on interface '%s'\n", pspecs[n]->name,
                      xtype_name (G_OBJECT_CLASS_TYPE (class)), xtype_name (iface_type));
          continue;
        }

      /* If the property on the interface is writable then we need to
       * make sure the implementation doesn't introduce new restrictions
       * on that writability (ie: construct-only).
       *
       * If the interface was not writable to begin with then we don't
       * really have any problems here because "writable at construct
       * time only" is still more permissive than "read only".
       */
      if (pspecs[n]->flags & XPARAM_WRITABLE)
        {
          if (!SUBSET (class_pspec->flags, pspecs[n]->flags, XPARAM_CONSTRUCT_ONLY))
            {
              g_critical ("Flags for property '%s' on class '%s' introduce additional restrictions on "
                          "writability compared with the property on interface '%s'\n", pspecs[n]->name,
                          xtype_name (G_OBJECT_CLASS_TYPE (class)), xtype_name (iface_type));
              continue;
            }
        }
#undef SUBSET

      /* If the property on the interface is readable then we are
       * effectively advertising that reading the property will return a
       * value of a specific type.  All implementations of the interface
       * need to return items of this type -- but may be more
       * restrictive.  For example, it is legal to have:
       *
       *   GtkWidget *get_item();
       *
       * that is implemented by a function that always returns a
       * GtkEntry.  In short: readability implies that the
       * implementation  value type must be equal or more restrictive.
       *
       * Similarly, if the property on the interface is writable then
       * must be able to accept the property being set to any value of
       * that type, including subclasses.  In this case, we may also be
       * less restrictive.  For example, it is legal to have:
       *
       *   set_item (GtkEntry *);
       *
       * that is implemented by a function that will actually work with
       * any GtkWidget.  In short: writability implies that the
       * implementation value type must be equal or less restrictive.
       *
       * In the case that the property is both readable and writable
       * then the only way that both of the above can be satisfied is
       * with a type that is exactly equal.
       */
      switch (pspecs[n]->flags & (XPARAM_READABLE | XPARAM_WRITABLE))
        {
        case XPARAM_READABLE | XPARAM_WRITABLE:
          /* class pspec value type must have exact equality with interface */
          if (pspecs[n]->value_type != class_pspec->value_type)
            g_critical ("Read/writable property '%s' on class '%s' has type '%s' which is not exactly equal to the "
                        "type '%s' of the property on the interface '%s'\n", pspecs[n]->name,
                        xtype_name (G_OBJECT_CLASS_TYPE (class)), xtype_name (XPARAM_SPEC_VALUE_TYPE (class_pspec)),
                        xtype_name (XPARAM_SPEC_VALUE_TYPE (pspecs[n])), xtype_name (iface_type));
          break;

        case XPARAM_READABLE:
          /* class pspec value type equal or more restrictive than interface */
          if (!xtype_is_a (class_pspec->value_type, pspecs[n]->value_type))
            g_critical ("Read-only property '%s' on class '%s' has type '%s' which is not equal to or more "
                        "restrictive than the type '%s' of the property on the interface '%s'\n", pspecs[n]->name,
                        xtype_name (G_OBJECT_CLASS_TYPE (class)), xtype_name (XPARAM_SPEC_VALUE_TYPE (class_pspec)),
                        xtype_name (XPARAM_SPEC_VALUE_TYPE (pspecs[n])), xtype_name (iface_type));
          break;

        case XPARAM_WRITABLE:
          /* class pspec value type equal or less restrictive than interface */
          if (!xtype_is_a (pspecs[n]->value_type, class_pspec->value_type))
            g_critical ("Write-only property '%s' on class '%s' has type '%s' which is not equal to or less "
                        "restrictive than the type '%s' of the property on the interface '%s' \n", pspecs[n]->name,
                        xtype_name (G_OBJECT_CLASS_TYPE (class)), xtype_name (XPARAM_SPEC_VALUE_TYPE (class_pspec)),
                        xtype_name (XPARAM_SPEC_VALUE_TYPE (pspecs[n])), xtype_name (iface_type));
          break;

        default:
          g_assert_not_reached ();
        }
    }

  g_free (pspecs);

 out:
  xtype_class_unref (class);
}

xtype_t
xobject_get_type (void)
{
    return XTYPE_OBJECT;
}

/**
 * xobject_new: (skip)
 * @object_type: the type id of the #xobject_t subtype to instantiate
 * @first_property_name: the name of the first property
 * @...: the value of the first property, followed optionally by more
 *  name/value pairs, followed by %NULL
 *
 * Creates a new instance of a #xobject_t subtype and sets its properties.
 *
 * Construction parameters (see %XPARAM_CONSTRUCT, %XPARAM_CONSTRUCT_ONLY)
 * which are not explicitly specified are set to their default values. Any
 * private data for the object is guaranteed to be initialized with zeros, as
 * per xtype_create_instance().
 *
 * Note that in C, small integer types in variable argument lists are promoted
 * up to #xint_t or #xuint_t as appropriate, and read back accordingly. #xint_t is 32
 * bits on every platform on which GLib is currently supported. This means that
 * you can use C expressions of type #xint_t with xobject_new() and properties of
 * type #xint_t or #xuint_t or smaller. Specifically, you can use integer literals
 * with these property types.
 *
 * When using property types of #sint64_t or #xuint64_t, you must ensure that the
 * value that you provide is 64 bit. This means that you should use a cast or
 * make use of the %G_GINT64_CONSTANT or %G_GUINT64_CONSTANT macros.
 *
 * Similarly, #gfloat is promoted to #xdouble_t, so you must ensure that the value
 * you provide is a #xdouble_t, even for a property of type #gfloat.
 *
 * Since GLib 2.72, all #GObjects are guaranteed to be aligned to at least the
 * alignment of the largest basic GLib type (typically this is #xuint64_t or
 * #xdouble_t). If you need larger alignment for an element in a #xobject_t, you
 * should allocate it on the heap (aligned), or arrange for your #xobject_t to be
 * appropriately padded.
 *
 * Returns: (transfer full) (type xobject_t.Object): a new instance of
 *   @object_type
 */
xpointer_t
xobject_new (xtype_t	   object_type,
	      const xchar_t *first_property_name,
	      ...)
{
  xobject_t *object;
  va_list var_args;

  /* short circuit for calls supplying no properties */
  if (!first_property_name)
    return xobject_new_with_properties (object_type, 0, NULL, NULL);

  va_start (var_args, first_property_name);
  object = xobject_new_valist (object_type, first_property_name, var_args);
  va_end (var_args);

  return object;
}

/* Check alignment. (See https://gitlab.gnome.org/GNOME/glib/-/issues/1231.)
 * This should never fail, since xtype_create_instance() uses g_slice_alloc0().
 * The GSlice allocator always aligns to the next power of 2 greater than the
 * allocation size. The allocation size for a xobject_t is
 *   sizeof(GTypeInstance) + sizeof(xuint_t) + sizeof(GData*)
 * which is 12B on 32-bit platforms, and larger on 64-bit systems. In both
 * cases, that???s larger than the 8B needed for a xuint64_t or xdouble_t.
 *
 * If GSlice falls back to malloc(), it???s documented to return something
 * suitably aligned for any basic type. */
static inline xboolean_t
xobject_is_aligned (xobject_t *object)
{
  return ((((guintptr) (void *) object) %
             MAX (G_ALIGNOF (xdouble_t),
                  MAX (G_ALIGNOF (xuint64_t),
                       MAX (G_ALIGNOF (xint_t),
                            G_ALIGNOF (xlong_t))))) == 0);
}

static xpointer_t
xobject_new_with_custom_constructor (xobject_class_t          *class,
                                      GObjectConstructParam *params,
                                      xuint_t                  n_params)
{
  xobject_notify_queue_t *nqueue = NULL;
  xboolean_t newly_constructed;
  GObjectConstructParam *cparams;
  xobject_t *object;
  xvalue_t *cvalues;
  xint_t n_cparams;
  xint_t cvals_used;
  xslist_t *node;
  xuint_t i;

  /* If we have ->constructed() then we have to do a lot more work.
   * It's possible that this is a singleton and it's also possible
   * that the user's constructor() will attempt to modify the values
   * that we pass in, so we'll need to allocate copies of them.
   * It's also possible that the user may attempt to call
   * xobject_set() from inside of their constructor, so we need to
   * add ourselves to a list of objects for which that is allowed
   * while their constructor() is running.
   */

  /* Create the array of GObjectConstructParams for constructor() */
  n_cparams = xslist_length (class->construct_properties);
  cparams = g_new (GObjectConstructParam, n_cparams);
  cvalues = g_new0 (xvalue_t, n_cparams);
  cvals_used = 0;
  i = 0;

  /* As above, we may find the value in the passed-in params list.
   *
   * If we have the value passed in then we can use the xvalue_t from
   * it directly because it is safe to modify.  If we use the
   * default value from the class, we had better not pass that in
   * and risk it being modified, so we create a new one.
   * */
  for (node = class->construct_properties; node; node = node->next)
    {
      xparam_spec_t *pspec;
      xvalue_t *value;
      xuint_t j;

      pspec = node->data;
      value = NULL; /* to silence gcc... */

      for (j = 0; j < n_params; j++)
        if (params[j].pspec == pspec)
          {
            consider_issuing_property_deprecation_warning (pspec);
            value = params[j].value;
            break;
          }

      if (value == NULL)
        {
          value = &cvalues[cvals_used++];
          xvalue_init (value, pspec->value_type);
          g_param_value_set_default (pspec, value);
        }

      cparams[i].pspec = pspec;
      cparams[i].value = value;
      i++;
    }

  /* construct object from construction parameters */
  object = class->constructor (class->xtype_class.g_type, n_cparams, cparams);
  /* free construction values */
  g_free (cparams);
  while (cvals_used--)
    xvalue_unset (&cvalues[cvals_used]);
  g_free (cvalues);

  /* There is code in the wild that relies on being able to return NULL
   * from its custom constructor.  This was never a supported operation,
   * but since the code is already out there...
   */
  if (object == NULL)
    {
      g_critical ("Custom constructor for class %s returned NULL (which is invalid). "
                  "Please use xinitable_t instead.", G_OBJECT_CLASS_NAME (class));
      return NULL;
    }

  if (!xobject_is_aligned (object))
    {
      g_critical ("Custom constructor for class %s returned a non-aligned "
                  "xobject_t (which is invalid since GLib 2.72). Assuming any "
                  "code using this object doesn???t require it to be aligned. "
                  "Please fix your constructor to align to the largest GLib "
                  "basic type (typically xdouble_t or xuint64_t).",
                  G_OBJECT_CLASS_NAME (class));
    }

  /* xobject_init() will have marked the object as being in-construction.
   * Check if the returned object still is so marked, or if this is an
   * already-existing singleton (in which case we should not do 'constructed').
   */
  newly_constructed = object_in_construction (object);
  if (newly_constructed)
    unset_object_in_construction (object);

  if (CLASS_HAS_PROPS (class))
    {
      /* If this object was newly_constructed then xobject_init()
       * froze the queue.  We need to freeze it here in order to get
       * the handle so that we can thaw it below (otherwise it will
       * be frozen forever).
       *
       * We also want to do a freeze if we have any params to set,
       * even on a non-newly_constructed object.
       *
       * It's possible that we have the case of non-newly created
       * singleton and all of the passed-in params were construct
       * properties so n_params > 0 but we will actually set no
       * properties.  This is a pretty lame case to optimise, so
       * just ignore it and freeze anyway.
       */
      if (newly_constructed || n_params)
        nqueue = xobject_notify_queue_freeze (object, FALSE);

      /* Remember: if it was newly_constructed then xobject_init()
       * already did a freeze, so we now have two.  Release one.
       */
      if (newly_constructed)
        xobject_notify_queue_thaw (object, nqueue);
    }

  /* run 'constructed' handler if there is a custom one */
  if (newly_constructed && CLASS_HAS_CUSTOM_CONSTRUCTED (class))
    class->constructed (object);

  /* set remaining properties */
  for (i = 0; i < n_params; i++)
    if (!(params[i].pspec->flags & (XPARAM_CONSTRUCT | XPARAM_CONSTRUCT_ONLY)))
      {
        consider_issuing_property_deprecation_warning (params[i].pspec);
        object_set_property (object, params[i].pspec, params[i].value, nqueue);
      }

  /* If nqueue is non-NULL then we are frozen.  Thaw it. */
  if (nqueue)
    xobject_notify_queue_thaw (object, nqueue);

  return object;
}

static xpointer_t
xobject_new_internal (xobject_class_t          *class,
                       GObjectConstructParam *params,
                       xuint_t                  n_params)
{
  xobject_notify_queue_t *nqueue = NULL;
  xobject_t *object;

  if G_UNLIKELY (CLASS_HAS_CUSTOM_CONSTRUCTOR (class))
    return xobject_new_with_custom_constructor (class, params, n_params);

  object = (xobject_t *) xtype_create_instance (class->xtype_class.g_type);

  xassert (xobject_is_aligned (object));

  if (CLASS_HAS_PROPS (class))
    {
      xslist_t *node;

      /* This will have been setup in xobject_init() */
      nqueue = g_datalist_id_get_data (&object->qdata, quark_notify_queue);
      xassert (nqueue != NULL);

      /* We will set exactly n_construct_properties construct
       * properties, but they may come from either the class default
       * values or the passed-in parameter list.
       */
      for (node = class->construct_properties; node; node = node->next)
        {
          const xvalue_t *value;
          xparam_spec_t *pspec;
          xuint_t j;

          pspec = node->data;
          value = NULL; /* to silence gcc... */

          for (j = 0; j < n_params; j++)
            if (params[j].pspec == pspec)
              {
                consider_issuing_property_deprecation_warning (pspec);
                value = params[j].value;
                break;
              }

          if (value == NULL)
            value = xparam_spec_get_default_value (pspec);

          object_set_property (object, pspec, value, nqueue);
        }
    }

  /* run 'constructed' handler if there is a custom one */
  if (CLASS_HAS_CUSTOM_CONSTRUCTED (class))
    class->constructed (object);

  if (nqueue)
    {
      xuint_t i;

      /* Set remaining properties.  The construct properties will
       * already have been taken, so set only the non-construct
       * ones.
       */
      for (i = 0; i < n_params; i++)
        if (!(params[i].pspec->flags & (XPARAM_CONSTRUCT | XPARAM_CONSTRUCT_ONLY)))
          {
            consider_issuing_property_deprecation_warning (params[i].pspec);
            object_set_property (object, params[i].pspec, params[i].value, nqueue);
          }

      xobject_notify_queue_thaw (object, nqueue);
    }

  return object;
}


static inline xboolean_t
xobject_new_is_valid_property (xtype_t                  object_type,
                                xparam_spec_t            *pspec,
                                const char            *name,
                                GObjectConstructParam *params,
                                xuint_t                  n_params)
{
  xuint_t i;

  if (G_UNLIKELY (pspec == NULL))
    {
      g_critical ("%s: object class '%s' has no property named '%s'",
                  G_STRFUNC, xtype_name (object_type), name);
      return FALSE;
    }

  if (G_UNLIKELY (~pspec->flags & XPARAM_WRITABLE))
    {
      g_critical ("%s: property '%s' of object class '%s' is not writable",
                  G_STRFUNC, pspec->name, xtype_name (object_type));
      return FALSE;
    }

  if (G_UNLIKELY (pspec->flags & (XPARAM_CONSTRUCT | XPARAM_CONSTRUCT_ONLY)))
    {
      for (i = 0; i < n_params; i++)
        if (params[i].pspec == pspec)
          break;
      if (G_UNLIKELY (i != n_params))
        {
          g_critical ("%s: property '%s' for type '%s' cannot be set twice",
                      G_STRFUNC, name, xtype_name (object_type));
          return FALSE;
        }
    }
  return TRUE;
}


/**
 * xobject_new_with_properties: (skip)
 * @object_type: the object type to instantiate
 * @n_properties: the number of properties
 * @names: (array length=n_properties): the names of each property to be set
 * @values: (array length=n_properties): the values of each property to be set
 *
 * Creates a new instance of a #xobject_t subtype and sets its properties using
 * the provided arrays. Both arrays must have exactly @n_properties elements,
 * and the names and values correspond by index.
 *
 * Construction parameters (see %XPARAM_CONSTRUCT, %XPARAM_CONSTRUCT_ONLY)
 * which are not explicitly specified are set to their default values.
 *
 * Returns: (type xobject_t.Object) (transfer full): a new instance of
 * @object_type
 *
 * Since: 2.54
 */
xobject_t *
xobject_new_with_properties (xtype_t          object_type,
                              xuint_t          n_properties,
                              const char    *names[],
                              const xvalue_t   values[])
{
  xobject_class_t *class, *unref_class = NULL;
  xobject_t *object;

  xreturn_val_if_fail (XTYPE_IS_OBJECT (object_type), NULL);

  /* Try to avoid thrashing the ref_count if we don't need to (since
   * it's a locked operation).
   */
  class = xtype_class_peek_static (object_type);

  if (class == NULL)
    class = unref_class = xtype_class_ref (object_type);

  if (n_properties > 0)
    {
      xuint_t i, count = 0;
      GObjectConstructParam *params;

      params = g_newa (GObjectConstructParam, n_properties);
      for (i = 0; i < n_properties; i++)
        {
          xparam_spec_t *pspec;
          pspec = xparam_spec_pool_lookup (pspec_pool, names[i], object_type, TRUE);
          if (!xobject_new_is_valid_property (object_type, pspec, names[i], params, count))
            continue;
          params[count].pspec = pspec;

          /* Init xvalue_t */
          params[count].value = g_newa0 (xvalue_t, 1);
          xvalue_init (params[count].value, G_VALUE_TYPE (&values[i]));

          xvalue_copy (&values[i], params[count].value);
          count++;
        }
      object = xobject_new_internal (class, params, count);

      while (count--)
        xvalue_unset (params[count].value);
    }
  else
    object = xobject_new_internal (class, NULL, 0);

  if (unref_class != NULL)
    xtype_class_unref (unref_class);

  return object;
}

/**
 * xobject_newv:
 * @object_type: the type id of the #xobject_t subtype to instantiate
 * @n_parameters: the length of the @parameters array
 * @parameters: (array length=n_parameters): an array of #GParameter
 *
 * Creates a new instance of a #xobject_t subtype and sets its properties.
 *
 * Construction parameters (see %XPARAM_CONSTRUCT, %XPARAM_CONSTRUCT_ONLY)
 * which are not explicitly specified are set to their default values.
 *
 * Returns: (type xobject_t.Object) (transfer full): a new instance of
 * @object_type
 *
 * Deprecated: 2.54: Use xobject_new_with_properties() instead.
 * deprecated. See #GParameter for more information.
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
xpointer_t
xobject_newv (xtype_t       object_type,
               xuint_t       n_parameters,
               GParameter *parameters)
{
  xobject_class_t *class, *unref_class = NULL;
  xobject_t *object;

  xreturn_val_if_fail (XTYPE_IS_OBJECT (object_type), NULL);
  xreturn_val_if_fail (n_parameters == 0 || parameters != NULL, NULL);

  /* Try to avoid thrashing the ref_count if we don't need to (since
   * it's a locked operation).
   */
  class = xtype_class_peek_static (object_type);

  if (!class)
    class = unref_class = xtype_class_ref (object_type);

  if (n_parameters)
    {
      GObjectConstructParam *cparams;
      xuint_t i, j;

      cparams = g_newa (GObjectConstructParam, n_parameters);
      j = 0;

      for (i = 0; i < n_parameters; i++)
        {
          xparam_spec_t *pspec;

          pspec = xparam_spec_pool_lookup (pspec_pool, parameters[i].name, object_type, TRUE);
          if (!xobject_new_is_valid_property (object_type, pspec, parameters[i].name, cparams, j))
            continue;

          cparams[j].pspec = pspec;
          cparams[j].value = &parameters[i].value;
          j++;
        }

      object = xobject_new_internal (class, cparams, j);
    }
  else
    /* Fast case: no properties passed in. */
    object = xobject_new_internal (class, NULL, 0);

  if (unref_class)
    xtype_class_unref (unref_class);

  return object;
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * xobject_new_valist: (skip)
 * @object_type: the type id of the #xobject_t subtype to instantiate
 * @first_property_name: the name of the first property
 * @var_args: the value of the first property, followed optionally by more
 *  name/value pairs, followed by %NULL
 *
 * Creates a new instance of a #xobject_t subtype and sets its properties.
 *
 * Construction parameters (see %XPARAM_CONSTRUCT, %XPARAM_CONSTRUCT_ONLY)
 * which are not explicitly specified are set to their default values.
 *
 * Returns: a new instance of @object_type
 */
xobject_t*
xobject_new_valist (xtype_t        object_type,
                     const xchar_t *first_property_name,
                     va_list      var_args)
{
  xobject_class_t *class, *unref_class = NULL;
  xobject_t *object;

  xreturn_val_if_fail (XTYPE_IS_OBJECT (object_type), NULL);

  /* Try to avoid thrashing the ref_count if we don't need to (since
   * it's a locked operation).
   */
  class = xtype_class_peek_static (object_type);

  if (!class)
    class = unref_class = xtype_class_ref (object_type);

  if (first_property_name)
    {
      GObjectConstructParam params_stack[16];
      xvalue_t values_stack[G_N_ELEMENTS (params_stack)];
      const xchar_t *name;
      GObjectConstructParam *params = params_stack;
      xvalue_t *values = values_stack;
      xuint_t n_params = 0;
      xuint_t n_params_alloc = G_N_ELEMENTS (params_stack);

      name = first_property_name;

      do
        {
          xchar_t *error = NULL;
          xparam_spec_t *pspec;

          pspec = xparam_spec_pool_lookup (pspec_pool, name, object_type, TRUE);

          if (!xobject_new_is_valid_property (object_type, pspec, name, params, n_params))
            break;

          if (G_UNLIKELY (n_params == n_params_alloc))
            {
              xuint_t i;

              if (n_params_alloc == G_N_ELEMENTS (params_stack))
                {
                  n_params_alloc = G_N_ELEMENTS (params_stack) * 2u;
                  params = g_new (GObjectConstructParam, n_params_alloc);
                  values = g_new (xvalue_t, n_params_alloc);
                  memcpy (params, params_stack, sizeof (GObjectConstructParam) * n_params);
                  memcpy (values, values_stack, sizeof (xvalue_t) * n_params);
                }
              else
                {
                  n_params_alloc *= 2u;
                  params = g_realloc (params, sizeof (GObjectConstructParam) * n_params_alloc);
                  values = g_realloc (values, sizeof (xvalue_t) * n_params_alloc);
                }

              for (i = 0; i < n_params; i++)
                params[i].value = &values[i];
            }

          params[n_params].pspec = pspec;
          params[n_params].value = &values[n_params];
          memset (&values[n_params], 0, sizeof (xvalue_t));

          G_VALUE_COLLECT_INIT (&values[n_params], pspec->value_type, var_args, 0, &error);

          if (error)
            {
              g_critical ("%s: %s", G_STRFUNC, error);
              xvalue_unset (&values[n_params]);
              g_free (error);
              break;
            }

          n_params++;
        }
      while ((name = va_arg (var_args, const xchar_t *)));

      object = xobject_new_internal (class, params, n_params);

      while (n_params--)
        xvalue_unset (params[n_params].value);

      if (G_UNLIKELY (n_params_alloc != G_N_ELEMENTS (params_stack)))
        {
          g_free (params);
          g_free (values);
        }
    }
  else
    /* Fast case: no properties passed in. */
    object = xobject_new_internal (class, NULL, 0);

  if (unref_class)
    xtype_class_unref (unref_class);

  return object;
}

static xobject_t*
xobject_constructor (xtype_t                  type,
		      xuint_t                  n_construct_properties,
		      GObjectConstructParam *construct_params)
{
  xobject_t *object;

  /* create object */
  object = (xobject_t*) xtype_create_instance (type);

  /* set construction parameters */
  if (n_construct_properties)
    {
      xobject_notify_queue_t *nqueue = xobject_notify_queue_freeze (object, FALSE);

      /* set construct properties */
      while (n_construct_properties--)
	{
	  xvalue_t *value = construct_params->value;
	  xparam_spec_t *pspec = construct_params->pspec;

	  construct_params++;
	  object_set_property (object, pspec, value, nqueue);
	}
      xobject_notify_queue_thaw (object, nqueue);
      /* the notification queue is still frozen from xobject_init(), so
       * we don't need to handle it here, xobject_newv() takes
       * care of that
       */
    }

  return object;
}

static void
xobject_constructed (xobject_t *object)
{
  /* empty default impl to allow unconditional upchaining */
}

static inline xboolean_t
xobject_set_is_valid_property (xobject_t         *object,
                                xparam_spec_t      *pspec,
                                const char      *property_name)
{
  if (G_UNLIKELY (pspec == NULL))
    {
      g_warning ("%s: object class '%s' has no property named '%s'",
                 G_STRFUNC, G_OBJECT_TYPE_NAME (object), property_name);
      return FALSE;
    }
  if (G_UNLIKELY (!(pspec->flags & XPARAM_WRITABLE)))
    {
      g_warning ("%s: property '%s' of object class '%s' is not writable",
                 G_STRFUNC, pspec->name, G_OBJECT_TYPE_NAME (object));
      return FALSE;
    }
  if (G_UNLIKELY (((pspec->flags & XPARAM_CONSTRUCT_ONLY) && !object_in_construction (object))))
    {
      g_warning ("%s: construct property \"%s\" for object '%s' can't be set after construction",
                 G_STRFUNC, pspec->name, G_OBJECT_TYPE_NAME (object));
      return FALSE;
    }
  return TRUE;
}

/**
 * xobject_setv: (skip)
 * @object: a #xobject_t
 * @n_properties: the number of properties
 * @names: (array length=n_properties): the names of each property to be set
 * @values: (array length=n_properties): the values of each property to be set
 *
 * Sets @n_properties properties for an @object.
 * Properties to be set will be taken from @values. All properties must be
 * valid. Warnings will be emitted and undefined behaviour may result if invalid
 * properties are passed in.
 *
 * Since: 2.54
 */
void
xobject_setv (xobject_t       *object,
               xuint_t          n_properties,
               const xchar_t   *names[],
               const xvalue_t   values[])
{
  xuint_t i;
  xobject_notify_queue_t *nqueue;
  xparam_spec_t *pspec;
  xtype_t obj_type;

  g_return_if_fail (X_IS_OBJECT (object));

  if (n_properties == 0)
    return;

  xobject_ref (object);
  obj_type = G_OBJECT_TYPE (object);
  nqueue = xobject_notify_queue_freeze (object, FALSE);
  for (i = 0; i < n_properties; i++)
    {
      pspec = xparam_spec_pool_lookup (pspec_pool, names[i], obj_type, TRUE);

      if (!xobject_set_is_valid_property (object, pspec, names[i]))
        break;

      consider_issuing_property_deprecation_warning (pspec);
      object_set_property (object, pspec, &values[i], nqueue);
    }

  xobject_notify_queue_thaw (object, nqueue);
  xobject_unref (object);
}

/**
 * xobject_set_valist: (skip)
 * @object: a #xobject_t
 * @first_property_name: name of the first property to set
 * @var_args: value for the first property, followed optionally by more
 *  name/value pairs, followed by %NULL
 *
 * Sets properties on an object.
 */
void
xobject_set_valist (xobject_t	 *object,
		     const xchar_t *first_property_name,
		     va_list	  var_args)
{
  xobject_notify_queue_t *nqueue;
  const xchar_t *name;

  g_return_if_fail (X_IS_OBJECT (object));

  xobject_ref (object);
  nqueue = xobject_notify_queue_freeze (object, FALSE);

  name = first_property_name;
  while (name)
    {
      xvalue_t value = G_VALUE_INIT;
      xparam_spec_t *pspec;
      xchar_t *error = NULL;

      pspec = xparam_spec_pool_lookup (pspec_pool,
					name,
					G_OBJECT_TYPE (object),
					TRUE);

      if (!xobject_set_is_valid_property (object, pspec, name))
        break;

      G_VALUE_COLLECT_INIT (&value, pspec->value_type, var_args,
			    0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRFUNC, error);
	  g_free (error);
          xvalue_unset (&value);
	  break;
	}

      consider_issuing_property_deprecation_warning (pspec);
      object_set_property (object, pspec, &value, nqueue);
      xvalue_unset (&value);

      name = va_arg (var_args, xchar_t*);
    }

  xobject_notify_queue_thaw (object, nqueue);
  xobject_unref (object);
}

static inline xboolean_t
xobject_get_is_valid_property (xobject_t          *object,
                                xparam_spec_t       *pspec,
                                const char       *property_name)
{
  if (G_UNLIKELY (pspec == NULL))
    {
      g_warning ("%s: object class '%s' has no property named '%s'",
                 G_STRFUNC, G_OBJECT_TYPE_NAME (object), property_name);
      return FALSE;
    }
  if (G_UNLIKELY (!(pspec->flags & XPARAM_READABLE)))
    {
      g_warning ("%s: property '%s' of object class '%s' is not readable",
                 G_STRFUNC, pspec->name, G_OBJECT_TYPE_NAME (object));
      return FALSE;
    }
  return TRUE;
}

/**
 * xobject_getv:
 * @object: a #xobject_t
 * @n_properties: the number of properties
 * @names: (array length=n_properties): the names of each property to get
 * @values: (array length=n_properties): the values of each property to get
 *
 * Gets @n_properties properties for an @object.
 * Obtained properties will be set to @values. All properties must be valid.
 * Warnings will be emitted and undefined behaviour may result if invalid
 * properties are passed in.
 *
 * Since: 2.54
 */
void
xobject_getv (xobject_t      *object,
               xuint_t         n_properties,
               const xchar_t  *names[],
               xvalue_t        values[])
{
  xuint_t i;
  xparam_spec_t *pspec;
  xtype_t obj_type;

  g_return_if_fail (X_IS_OBJECT (object));

  if (n_properties == 0)
    return;

  xobject_ref (object);

  memset (values, 0, n_properties * sizeof (xvalue_t));

  obj_type = G_OBJECT_TYPE (object);
  for (i = 0; i < n_properties; i++)
    {
      pspec = xparam_spec_pool_lookup (pspec_pool, names[i], obj_type, TRUE);
      if (!xobject_get_is_valid_property (object, pspec, names[i]))
        break;
      xvalue_init (&values[i], pspec->value_type);
      object_get_property (object, pspec, &values[i]);
    }
  xobject_unref (object);
}

/**
 * xobject_get_valist: (skip)
 * @object: a #xobject_t
 * @first_property_name: name of the first property to get
 * @var_args: return location for the first property, followed optionally by more
 *  name/return location pairs, followed by %NULL
 *
 * Gets properties of an object.
 *
 * In general, a copy is made of the property contents and the caller
 * is responsible for freeing the memory in the appropriate manner for
 * the type, for instance by calling g_free() or xobject_unref().
 *
 * See xobject_get().
 */
void
xobject_get_valist (xobject_t	 *object,
		     const xchar_t *first_property_name,
		     va_list	  var_args)
{
  const xchar_t *name;

  g_return_if_fail (X_IS_OBJECT (object));

  xobject_ref (object);

  name = first_property_name;

  while (name)
    {
      xvalue_t value = G_VALUE_INIT;
      xparam_spec_t *pspec;
      xchar_t *error;

      pspec = xparam_spec_pool_lookup (pspec_pool,
					name,
					G_OBJECT_TYPE (object),
					TRUE);

      if (!xobject_get_is_valid_property (object, pspec, name))
        break;

      xvalue_init (&value, pspec->value_type);

      object_get_property (object, pspec, &value);

      G_VALUE_LCOPY (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRFUNC, error);
	  g_free (error);
	  xvalue_unset (&value);
	  break;
	}

      xvalue_unset (&value);

      name = va_arg (var_args, xchar_t*);
    }

  xobject_unref (object);
}

/**
 * xobject_set: (skip)
 * @object: (type xobject_t.Object): a #xobject_t
 * @first_property_name: name of the first property to set
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %NULL
 *
 * Sets properties on an object.
 *
 * The same caveats about passing integer literals as varargs apply as with
 * xobject_new(). In particular, any integer literals set as the values for
 * properties of type #sint64_t or #xuint64_t must be 64 bits wide, using the
 * %G_GINT64_CONSTANT or %G_GUINT64_CONSTANT macros.
 *
 * Note that the "notify" signals are queued and only emitted (in
 * reverse order) after all properties have been set. See
 * xobject_freeze_notify().
 */
void
xobject_set (xpointer_t     _object,
	      const xchar_t *first_property_name,
	      ...)
{
  xobject_t *object = _object;
  va_list var_args;

  g_return_if_fail (X_IS_OBJECT (object));

  va_start (var_args, first_property_name);
  xobject_set_valist (object, first_property_name, var_args);
  va_end (var_args);
}

/**
 * xobject_get: (skip)
 * @object: (type xobject_t.Object): a #xobject_t
 * @first_property_name: name of the first property to get
 * @...: return location for the first property, followed optionally by more
 *  name/return location pairs, followed by %NULL
 *
 * Gets properties of an object.
 *
 * In general, a copy is made of the property contents and the caller
 * is responsible for freeing the memory in the appropriate manner for
 * the type, for instance by calling g_free() or xobject_unref().
 *
 * Here is an example of using xobject_get() to get the contents
 * of three properties: an integer, a string and an object:
 * |[<!-- language="C" -->
 *  xint_t intval;
 *  xuint64_t uint64val;
 *  xchar_t *strval;
 *  xobject_t *objval;
 *
 *  xobject_get (my_object,
 *                "int-property", &intval,
 *                "uint64-property", &uint64val,
 *                "str-property", &strval,
 *                "obj-property", &objval,
 *                NULL);
 *
 *  // Do something with intval, uint64val, strval, objval
 *
 *  g_free (strval);
 *  xobject_unref (objval);
 * ]|
 */
void
xobject_get (xpointer_t     _object,
	      const xchar_t *first_property_name,
	      ...)
{
  xobject_t *object = _object;
  va_list var_args;

  g_return_if_fail (X_IS_OBJECT (object));

  va_start (var_args, first_property_name);
  xobject_get_valist (object, first_property_name, var_args);
  va_end (var_args);
}

/**
 * xobject_set_property:
 * @object: a #xobject_t
 * @property_name: the name of the property to set
 * @value: the value
 *
 * Sets a property on an object.
 */
void
xobject_set_property (xobject_t	    *object,
		       const xchar_t  *property_name,
		       const xvalue_t *value)
{
  xobject_setv (object, 1, &property_name, value);
}

/**
 * xobject_get_property:
 * @object: a #xobject_t
 * @property_name: the name of the property to get
 * @value: return location for the property value
 *
 * Gets a property of an object.
 *
 * The @value can be:
 *
 *  - an empty #xvalue_t initialized by %G_VALUE_INIT, which will be
 *    automatically initialized with the expected type of the property
 *    (since GLib 2.60)
 *  - a #xvalue_t initialized with the expected type of the property
 *  - a #xvalue_t initialized with a type to which the expected type
 *    of the property can be transformed
 *
 * In general, a copy is made of the property contents and the caller is
 * responsible for freeing the memory by calling xvalue_unset().
 *
 * Note that xobject_get_property() is really intended for language
 * bindings, xobject_get() is much more convenient for C programming.
 */
void
xobject_get_property (xobject_t	   *object,
		       const xchar_t *property_name,
		       xvalue_t	   *value)
{
  xparam_spec_t *pspec;

  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (value != NULL);

  xobject_ref (object);

  pspec = xparam_spec_pool_lookup (pspec_pool,
				    property_name,
				    G_OBJECT_TYPE (object),
				    TRUE);

  if (xobject_get_is_valid_property (object, pspec, property_name))
    {
      xvalue_t *prop_value, tmp_value = G_VALUE_INIT;

      if (G_VALUE_TYPE (value) == XTYPE_INVALID)
        {
          /* zero-initialized value */
          xvalue_init (value, pspec->value_type);
          prop_value = value;
        }
      else if (G_VALUE_TYPE (value) == pspec->value_type)
        {
          /* auto-conversion of the callers value type */
          xvalue_reset (value);
          prop_value = value;
        }
      else if (!xvalue_type_transformable (pspec->value_type, G_VALUE_TYPE (value)))
        {
          g_warning ("%s: can't retrieve property '%s' of type '%s' as value of type '%s'",
                     G_STRFUNC, pspec->name,
                     xtype_name (pspec->value_type),
                     G_VALUE_TYPE_NAME (value));
          xobject_unref (object);
          return;
        }
      else
        {
          xvalue_init (&tmp_value, pspec->value_type);
          prop_value = &tmp_value;
        }
      object_get_property (object, pspec, prop_value);
      if (prop_value != value)
        {
          xvalue_transform (prop_value, value);
          xvalue_unset (&tmp_value);
        }
    }

  xobject_unref (object);
}

/**
 * xobject_connect: (skip)
 * @object: (type xobject_t.Object): a #xobject_t
 * @signal_spec: the spec for the first signal
 * @...: #xcallback_t for the first signal, followed by data for the
 *       first signal, followed optionally by more signal
 *       spec/callback/data triples, followed by %NULL
 *
 * A convenience function to connect multiple signals at once.
 *
 * The signal specs expected by this function have the form
 * "modifier::signal_name", where modifier can be one of the following:
 * - signal: equivalent to xsignal_connect_data (..., NULL, 0)
 * - object-signal, object_signal: equivalent to xsignal_connect_object (..., 0)
 * - swapped-signal, swapped_signal: equivalent to xsignal_connect_data (..., NULL, G_CONNECT_SWAPPED)
 * - swapped_object_signal, swapped-object-signal: equivalent to xsignal_connect_object (..., G_CONNECT_SWAPPED)
 * - signal_after, signal-after: equivalent to xsignal_connect_data (..., NULL, G_CONNECT_AFTER)
 * - object_signal_after, object-signal-after: equivalent to xsignal_connect_object (..., G_CONNECT_AFTER)
 * - swapped_signal_after, swapped-signal-after: equivalent to xsignal_connect_data (..., NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER)
 * - swapped_object_signal_after, swapped-object-signal-after: equivalent to xsignal_connect_object (..., G_CONNECT_SWAPPED | G_CONNECT_AFTER)
 *
 * |[<!-- language="C" -->
 *   menu->toplevel = xobject_connect (xobject_new (GTK_TYPE_WINDOW,
 * 						   "type", GTK_WINDOW_POPUP,
 * 						   "child", menu,
 * 						   NULL),
 * 				     "signal::event", gtk_menu_window_event, menu,
 * 				     "signal::size_request", gtk_menu_window_size_request, menu,
 * 				     "signal::destroy", gtk_widget_destroyed, &menu->toplevel,
 * 				     NULL);
 * ]|
 *
 * Returns: (transfer none) (type xobject_t.Object): @object
 */
xpointer_t
xobject_connect (xpointer_t     _object,
		  const xchar_t *signal_spec,
		  ...)
{
  xobject_t *object = _object;
  va_list var_args;

  xreturn_val_if_fail (X_IS_OBJECT (object), NULL);
  xreturn_val_if_fail (object->ref_count > 0, object);

  va_start (var_args, signal_spec);
  while (signal_spec)
    {
      xcallback_t callback = va_arg (var_args, xcallback_t);
      xpointer_t data = va_arg (var_args, xpointer_t);

      if (strncmp (signal_spec, "signal::", 8) == 0)
	xsignal_connect_data (object, signal_spec + 8,
			       callback, data, NULL,
			       0);
      else if (strncmp (signal_spec, "object_signal::", 15) == 0 ||
               strncmp (signal_spec, "object-signal::", 15) == 0)
	xsignal_connect_object (object, signal_spec + 15,
				 callback, data,
				 0);
      else if (strncmp (signal_spec, "swapped_signal::", 16) == 0 ||
               strncmp (signal_spec, "swapped-signal::", 16) == 0)
	xsignal_connect_data (object, signal_spec + 16,
			       callback, data, NULL,
			       G_CONNECT_SWAPPED);
      else if (strncmp (signal_spec, "swapped_object_signal::", 23) == 0 ||
               strncmp (signal_spec, "swapped-object-signal::", 23) == 0)
	xsignal_connect_object (object, signal_spec + 23,
				 callback, data,
				 G_CONNECT_SWAPPED);
      else if (strncmp (signal_spec, "signal_after::", 14) == 0 ||
               strncmp (signal_spec, "signal-after::", 14) == 0)
	xsignal_connect_data (object, signal_spec + 14,
			       callback, data, NULL,
			       G_CONNECT_AFTER);
      else if (strncmp (signal_spec, "object_signal_after::", 21) == 0 ||
               strncmp (signal_spec, "object-signal-after::", 21) == 0)
	xsignal_connect_object (object, signal_spec + 21,
				 callback, data,
				 G_CONNECT_AFTER);
      else if (strncmp (signal_spec, "swapped_signal_after::", 22) == 0 ||
               strncmp (signal_spec, "swapped-signal-after::", 22) == 0)
	xsignal_connect_data (object, signal_spec + 22,
			       callback, data, NULL,
			       G_CONNECT_SWAPPED | G_CONNECT_AFTER);
      else if (strncmp (signal_spec, "swapped_object_signal_after::", 29) == 0 ||
               strncmp (signal_spec, "swapped-object-signal-after::", 29) == 0)
	xsignal_connect_object (object, signal_spec + 29,
				 callback, data,
				 G_CONNECT_SWAPPED | G_CONNECT_AFTER);
      else
	{
	  g_warning ("%s: invalid signal spec \"%s\"", G_STRFUNC, signal_spec);
	  break;
	}
      signal_spec = va_arg (var_args, xchar_t*);
    }
  va_end (var_args);

  return object;
}

/**
 * xobject_disconnect: (skip)
 * @object: (type xobject_t.Object): a #xobject_t
 * @signal_spec: the spec for the first signal
 * @...: #xcallback_t for the first signal, followed by data for the first signal,
 *  followed optionally by more signal spec/callback/data triples,
 *  followed by %NULL
 *
 * A convenience function to disconnect multiple signals at once.
 *
 * The signal specs expected by this function have the form
 * "any_signal", which means to disconnect any signal with matching
 * callback and data, or "any_signal::signal_name", which only
 * disconnects the signal named "signal_name".
 */
void
xobject_disconnect (xpointer_t     _object,
		     const xchar_t *signal_spec,
		     ...)
{
  xobject_t *object = _object;
  va_list var_args;

  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (object->ref_count > 0);

  va_start (var_args, signal_spec);
  while (signal_spec)
    {
      xcallback_t callback = va_arg (var_args, xcallback_t);
      xpointer_t data = va_arg (var_args, xpointer_t);
      xuint_t sid = 0, detail = 0, mask = 0;

      if (strncmp (signal_spec, "any_signal::", 12) == 0 ||
          strncmp (signal_spec, "any-signal::", 12) == 0)
	{
	  signal_spec += 12;
	  mask = G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA;
	}
      else if (strcmp (signal_spec, "any_signal") == 0 ||
               strcmp (signal_spec, "any-signal") == 0)
	{
	  signal_spec += 10;
	  mask = G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA;
	}
      else
	{
	  g_warning ("%s: invalid signal spec \"%s\"", G_STRFUNC, signal_spec);
	  break;
	}

      if ((mask & G_SIGNAL_MATCH_ID) &&
	  !xsignal_parse_name (signal_spec, G_OBJECT_TYPE (object), &sid, &detail, FALSE))
	g_warning ("%s: invalid signal name \"%s\"", G_STRFUNC, signal_spec);
      else if (!xsignal_handlers_disconnect_matched (object, mask | (detail ? G_SIGNAL_MATCH_DETAIL : 0),
						      sid, detail,
						      NULL, (xpointer_t)callback, data))
	g_warning ("%s: signal handler %p(%p) is not connected", G_STRFUNC, callback, data);
      signal_spec = va_arg (var_args, xchar_t*);
    }
  va_end (var_args);
}

typedef struct {
  xobject_t *object;
  xuint_t n_weak_refs;
  struct {
    GWeakNotify notify;
    xpointer_t    data;
  } weak_refs[1];  /* flexible array */
} WeakRefStack;

static void
weak_refs_notify (xpointer_t data)
{
  WeakRefStack *wstack = data;
  xuint_t i;

  for (i = 0; i < wstack->n_weak_refs; i++)
    wstack->weak_refs[i].notify (wstack->weak_refs[i].data, wstack->object);
  g_free (wstack);
}

/**
 * xobject_weak_ref: (skip)
 * @object: #xobject_t to reference weakly
 * @notify: callback to invoke before the object is freed
 * @data: extra data to pass to notify
 *
 * Adds a weak reference callback to an object. Weak references are
 * used for notification when an object is disposed. They are called
 * "weak references" because they allow you to safely hold a pointer
 * to an object without calling xobject_ref() (xobject_ref() adds a
 * strong reference, that is, forces the object to stay alive).
 *
 * Note that the weak references created by this method are not
 * thread-safe: they cannot safely be used in one thread if the
 * object's last xobject_unref() might happen in another thread.
 * Use #GWeakRef if thread-safety is required.
 */
void
xobject_weak_ref (xobject_t    *object,
		   GWeakNotify notify,
		   xpointer_t    data)
{
  WeakRefStack *wstack;
  xuint_t i;

  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (notify != NULL);
  g_return_if_fail (g_atomic_int_get (&object->ref_count) >= 1);

  G_LOCK (weak_refs_mutex);
  wstack = g_datalist_id_remove_no_notify (&object->qdata, quark_weak_refs);
  if (wstack)
    {
      i = wstack->n_weak_refs++;
      wstack = g_realloc (wstack, sizeof (*wstack) + sizeof (wstack->weak_refs[0]) * i);
    }
  else
    {
      wstack = g_renew (WeakRefStack, NULL, 1);
      wstack->object = object;
      wstack->n_weak_refs = 1;
      i = 0;
    }
  wstack->weak_refs[i].notify = notify;
  wstack->weak_refs[i].data = data;
  g_datalist_id_set_data_full (&object->qdata, quark_weak_refs, wstack, weak_refs_notify);
  G_UNLOCK (weak_refs_mutex);
}

/**
 * xobject_weak_unref: (skip)
 * @object: #xobject_t to remove a weak reference from
 * @notify: callback to search for
 * @data: data to search for
 *
 * Removes a weak reference callback to an object.
 */
void
xobject_weak_unref (xobject_t    *object,
		     GWeakNotify notify,
		     xpointer_t    data)
{
  WeakRefStack *wstack;
  xboolean_t found_one = FALSE;

  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (notify != NULL);

  G_LOCK (weak_refs_mutex);
  wstack = g_datalist_id_get_data (&object->qdata, quark_weak_refs);
  if (wstack)
    {
      xuint_t i;

      for (i = 0; i < wstack->n_weak_refs; i++)
	if (wstack->weak_refs[i].notify == notify &&
	    wstack->weak_refs[i].data == data)
	  {
	    found_one = TRUE;
	    wstack->n_weak_refs -= 1;
	    if (i != wstack->n_weak_refs)
	      wstack->weak_refs[i] = wstack->weak_refs[wstack->n_weak_refs];

	    break;
	  }
    }
  G_UNLOCK (weak_refs_mutex);
  if (!found_one)
    g_warning ("%s: couldn't find weak ref %p(%p)", G_STRFUNC, notify, data);
}

/**
 * xobject_add_weak_pointer: (skip)
 * @object: The object that should be weak referenced.
 * @weak_pointer_location: (inout) (not optional): The memory address
 *    of a pointer.
 *
 * Adds a weak reference from weak_pointer to @object to indicate that
 * the pointer located at @weak_pointer_location is only valid during
 * the lifetime of @object. When the @object is finalized,
 * @weak_pointer will be set to %NULL.
 *
 * Note that as with xobject_weak_ref(), the weak references created by
 * this method are not thread-safe: they cannot safely be used in one
 * thread if the object's last xobject_unref() might happen in another
 * thread. Use #GWeakRef if thread-safety is required.
 */
void
xobject_add_weak_pointer (xobject_t  *object,
                           xpointer_t *weak_pointer_location)
{
  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (weak_pointer_location != NULL);

  xobject_weak_ref (object,
                     (GWeakNotify) g_nullify_pointer,
                     weak_pointer_location);
}

/**
 * xobject_remove_weak_pointer: (skip)
 * @object: The object that is weak referenced.
 * @weak_pointer_location: (inout) (not optional): The memory address
 *    of a pointer.
 *
 * Removes a weak reference from @object that was previously added
 * using xobject_add_weak_pointer(). The @weak_pointer_location has
 * to match the one used with xobject_add_weak_pointer().
 */
void
xobject_remove_weak_pointer (xobject_t  *object,
                              xpointer_t *weak_pointer_location)
{
  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (weak_pointer_location != NULL);

  xobject_weak_unref (object,
                       (GWeakNotify) g_nullify_pointer,
                       weak_pointer_location);
}

static xuint_t
object_floating_flag_handler (xobject_t        *object,
                              xint_t            job)
{
  switch (job)
    {
      xpointer_t oldvalue;
    case +1:    /* force floating if possible */
      do
        oldvalue = g_atomic_pointer_get (&object->qdata);
      while (!g_atomic_pointer_compare_and_exchange ((void**) &object->qdata, oldvalue,
                                                     (xpointer_t) ((xsize_t) oldvalue | OBJECT_FLOATING_FLAG)));
      return (xsize_t) oldvalue & OBJECT_FLOATING_FLAG;
    case -1:    /* sink if possible */
      do
        oldvalue = g_atomic_pointer_get (&object->qdata);
      while (!g_atomic_pointer_compare_and_exchange ((void**) &object->qdata, oldvalue,
                                                     (xpointer_t) ((xsize_t) oldvalue & ~(xsize_t) OBJECT_FLOATING_FLAG)));
      return (xsize_t) oldvalue & OBJECT_FLOATING_FLAG;
    default:    /* check floating */
      return 0 != ((xsize_t) g_atomic_pointer_get (&object->qdata) & OBJECT_FLOATING_FLAG);
    }
}

/**
 * xobject_is_floating:
 * @object: (type xobject_t.Object): a #xobject_t
 *
 * Checks whether @object has a [floating][floating-ref] reference.
 *
 * Since: 2.10
 *
 * Returns: %TRUE if @object has a floating reference
 */
xboolean_t
xobject_is_floating (xpointer_t _object)
{
  xobject_t *object = _object;
  xreturn_val_if_fail (X_IS_OBJECT (object), FALSE);
  return floating_flag_handler (object, 0);
}

/**
 * xobject_ref_sink:
 * @object: (type xobject_t.Object): a #xobject_t
 *
 * Increase the reference count of @object, and possibly remove the
 * [floating][floating-ref] reference, if @object has a floating reference.
 *
 * In other words, if the object is floating, then this call "assumes
 * ownership" of the floating reference, converting it to a normal
 * reference by clearing the floating flag while leaving the reference
 * count unchanged.  If the object is not floating, then this call
 * adds a new normal reference increasing the reference count by one.
 *
 * Since GLib 2.56, the type of @object will be propagated to the return type
 * under the same conditions as for xobject_ref().
 *
 * Since: 2.10
 *
 * Returns: (type xobject_t.Object) (transfer none): @object
 */
xpointer_t
(xobject_ref_sink) (xpointer_t _object)
{
  xobject_t *object = _object;
  xboolean_t was_floating;
  xreturn_val_if_fail (X_IS_OBJECT (object), object);
  xreturn_val_if_fail (g_atomic_int_get (&object->ref_count) >= 1, object);
  xobject_ref (object);
  was_floating = floating_flag_handler (object, -1);
  if (was_floating)
    xobject_unref (object);
  return object;
}

/**
 * xobject_take_ref: (skip)
 * @object: (type xobject_t.Object): a #xobject_t
 *
 * If @object is floating, sink it.  Otherwise, do nothing.
 *
 * In other words, this function will convert a floating reference (if
 * present) into a full reference.
 *
 * Typically you want to use xobject_ref_sink() in order to
 * automatically do the correct thing with respect to floating or
 * non-floating references, but there is one specific scenario where
 * this function is helpful.
 *
 * The situation where this function is helpful is when creating an API
 * that allows the user to provide a callback function that returns a
 * xobject_t. We certainly want to allow the user the flexibility to
 * return a non-floating reference from this callback (for the case
 * where the object that is being returned already exists).
 *
 * At the same time, the API style of some popular xobject-based
 * libraries (such as Gtk) make it likely that for newly-created xobject_t
 * instances, the user can be saved some typing if they are allowed to
 * return a floating reference.
 *
 * Using this function on the return value of the user's callback allows
 * the user to do whichever is more convenient for them. The caller will
 * alway receives exactly one full reference to the value: either the
 * one that was returned in the first place, or a floating reference
 * that has been converted to a full reference.
 *
 * This function has an odd interaction when combined with
 * xobject_ref_sink() running at the same time in another thread on
 * the same #xobject_t instance. If xobject_ref_sink() runs first then
 * the result will be that the floating reference is converted to a hard
 * reference. If xobject_take_ref() runs first then the result will be
 * that the floating reference is converted to a hard reference and an
 * additional reference on top of that one is added. It is best to avoid
 * this situation.
 *
 * Since: 2.70
 *
 * Returns: (type xobject_t.Object) (transfer full): @object
 */
xpointer_t
xobject_take_ref (xpointer_t _object)
{
  xobject_t *object = _object;
  xreturn_val_if_fail (X_IS_OBJECT (object), object);
  xreturn_val_if_fail (g_atomic_int_get (&object->ref_count) >= 1, object);

  floating_flag_handler (object, -1);

  return object;
}

/**
 * xobject_force_floating:
 * @object: a #xobject_t
 *
 * This function is intended for #xobject_t implementations to re-enforce
 * a [floating][floating-ref] object reference. Doing this is seldom
 * required: all #GInitiallyUnowneds are created with a floating reference
 * which usually just needs to be sunken by calling xobject_ref_sink().
 *
 * Since: 2.10
 */
void
xobject_force_floating (xobject_t *object)
{
  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (g_atomic_int_get (&object->ref_count) >= 1);

  floating_flag_handler (object, +1);
}

typedef struct {
  xobject_t *object;
  xuint_t n_toggle_refs;
  struct {
    GToggleNotify notify;
    xpointer_t    data;
  } toggle_refs[1];  /* flexible array */
} ToggleRefStack;

static void
toggle_refs_notify (xobject_t *object,
		    xboolean_t is_last_ref)
{
  ToggleRefStack tstack, *tstackptr;

  G_LOCK (toggle_refs_mutex);
  /* If another thread removed the toggle reference on the object, while
   * we were waiting here, there's nothing to notify.
   * So let's check again if the object has toggle reference and in case return.
   */
  if (!OBJECT_HAS_TOGGLE_REF (object))
    {
      G_UNLOCK (toggle_refs_mutex);
      return;
    }

  tstackptr = g_datalist_id_get_data (&object->qdata, quark_toggle_refs);
  tstack = *tstackptr;
  G_UNLOCK (toggle_refs_mutex);

  /* Reentrancy here is not as tricky as it seems, because a toggle reference
   * will only be notified when there is exactly one of them.
   */
  xassert (tstack.n_toggle_refs == 1);
  tstack.toggle_refs[0].notify (tstack.toggle_refs[0].data, tstack.object, is_last_ref);
}

/**
 * xobject_add_toggle_ref: (skip)
 * @object: a #xobject_t
 * @notify: a function to call when this reference is the
 *  last reference to the object, or is no longer
 *  the last reference.
 * @data: data to pass to @notify
 *
 * Increases the reference count of the object by one and sets a
 * callback to be called when all other references to the object are
 * dropped, or when this is already the last reference to the object
 * and another reference is established.
 *
 * This functionality is intended for binding @object to a proxy
 * object managed by another memory manager. This is done with two
 * paired references: the strong reference added by
 * xobject_add_toggle_ref() and a reverse reference to the proxy
 * object which is either a strong reference or weak reference.
 *
 * The setup is that when there are no other references to @object,
 * only a weak reference is held in the reverse direction from @object
 * to the proxy object, but when there are other references held to
 * @object, a strong reference is held. The @notify callback is called
 * when the reference from @object to the proxy object should be
 * "toggled" from strong to weak (@is_last_ref true) or weak to strong
 * (@is_last_ref false).
 *
 * Since a (normal) reference must be held to the object before
 * calling xobject_add_toggle_ref(), the initial state of the reverse
 * link is always strong.
 *
 * Multiple toggle references may be added to the same gobject,
 * however if there are multiple toggle references to an object, none
 * of them will ever be notified until all but one are removed.  For
 * this reason, you should only ever use a toggle reference if there
 * is important state in the proxy object.
 *
 * Since: 2.8
 */
void
xobject_add_toggle_ref (xobject_t       *object,
			 GToggleNotify  notify,
			 xpointer_t       data)
{
  ToggleRefStack *tstack;
  xuint_t i;

  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (notify != NULL);
  g_return_if_fail (g_atomic_int_get (&object->ref_count) >= 1);

  xobject_ref (object);

  G_LOCK (toggle_refs_mutex);
  tstack = g_datalist_id_remove_no_notify (&object->qdata, quark_toggle_refs);
  if (tstack)
    {
      i = tstack->n_toggle_refs++;
      /* allocate i = tstate->n_toggle_refs - 1 positions beyond the 1 declared
       * in tstate->toggle_refs */
      tstack = g_realloc (tstack, sizeof (*tstack) + sizeof (tstack->toggle_refs[0]) * i);
    }
  else
    {
      tstack = g_renew (ToggleRefStack, NULL, 1);
      tstack->object = object;
      tstack->n_toggle_refs = 1;
      i = 0;
    }

  /* Set a flag for fast lookup after adding the first toggle reference */
  if (tstack->n_toggle_refs == 1)
    g_datalist_set_flags (&object->qdata, OBJECT_HAS_TOGGLE_REF_FLAG);

  tstack->toggle_refs[i].notify = notify;
  tstack->toggle_refs[i].data = data;
  g_datalist_id_set_data_full (&object->qdata, quark_toggle_refs, tstack,
			       (xdestroy_notify_t)g_free);
  G_UNLOCK (toggle_refs_mutex);
}

/**
 * xobject_remove_toggle_ref: (skip)
 * @object: a #xobject_t
 * @notify: a function to call when this reference is the
 *  last reference to the object, or is no longer
 *  the last reference.
 * @data: (nullable): data to pass to @notify, or %NULL to
 *  match any toggle refs with the @notify argument.
 *
 * Removes a reference added with xobject_add_toggle_ref(). The
 * reference count of the object is decreased by one.
 *
 * Since: 2.8
 */
void
xobject_remove_toggle_ref (xobject_t       *object,
			    GToggleNotify  notify,
			    xpointer_t       data)
{
  ToggleRefStack *tstack;
  xboolean_t found_one = FALSE;

  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (notify != NULL);

  G_LOCK (toggle_refs_mutex);
  tstack = g_datalist_id_get_data (&object->qdata, quark_toggle_refs);
  if (tstack)
    {
      xuint_t i;

      for (i = 0; i < tstack->n_toggle_refs; i++)
	if (tstack->toggle_refs[i].notify == notify &&
	    (tstack->toggle_refs[i].data == data || data == NULL))
	  {
	    found_one = TRUE;
	    tstack->n_toggle_refs -= 1;
	    if (i != tstack->n_toggle_refs)
	      tstack->toggle_refs[i] = tstack->toggle_refs[tstack->n_toggle_refs];

	    if (tstack->n_toggle_refs == 0)
	      g_datalist_unset_flags (&object->qdata, OBJECT_HAS_TOGGLE_REF_FLAG);

	    break;
	  }
    }
  G_UNLOCK (toggle_refs_mutex);

  if (found_one)
    xobject_unref (object);
  else
    g_warning ("%s: couldn't find toggle ref %p(%p)", G_STRFUNC, notify, data);
}

/**
 * xobject_ref:
 * @object: (type xobject_t.Object): a #xobject_t
 *
 * Increases the reference count of @object.
 *
 * Since GLib 2.56, if `XPL_VERSION_MAX_ALLOWED` is 2.56 or greater, the type
 * of @object will be propagated to the return type (using the GCC typeof()
 * extension), so any casting the caller needs to do on the return type must be
 * explicit.
 *
 * Returns: (type xobject_t.Object) (transfer none): the same @object
 */
xpointer_t
(xobject_ref) (xpointer_t _object)
{
  xobject_t *object = _object;
  xint_t old_val;
  xboolean_t object_already_finalized;

  xreturn_val_if_fail (X_IS_OBJECT (object), NULL);

  old_val = g_atomic_int_add (&object->ref_count, 1);
  object_already_finalized = (old_val <= 0);
  xreturn_val_if_fail (!object_already_finalized, NULL);

  if (old_val == 1 && OBJECT_HAS_TOGGLE_REF (object))
    toggle_refs_notify (object, FALSE);

  TRACE (GOBJECT_OBJECT_REF(object,XTYPE_FROM_INSTANCE(object),old_val));

  return object;
}

/**
 * xobject_unref:
 * @object: (type xobject_t.Object): a #xobject_t
 *
 * Decreases the reference count of @object. When its reference count
 * drops to 0, the object is finalized (i.e. its memory is freed).
 *
 * If the pointer to the #xobject_t may be reused in future (for example, if it is
 * an instance variable of another object), it is recommended to clear the
 * pointer to %NULL rather than retain a dangling pointer to a potentially
 * invalid #xobject_t instance. Use g_clear_object() for this.
 */
void
xobject_unref (xpointer_t _object)
{
  xobject_t *object = _object;
  xint_t old_ref;

  g_return_if_fail (X_IS_OBJECT (object));

  /* here we want to atomically do: if (ref_count>1) { ref_count--; return; } */
 retry_atomic_decrement1:
  old_ref = g_atomic_int_get (&object->ref_count);
  if (old_ref > 1)
    {
      /* valid if last 2 refs are owned by this call to unref and the toggle_ref */
      xboolean_t has_toggle_ref = OBJECT_HAS_TOGGLE_REF (object);

      if (!g_atomic_int_compare_and_exchange ((int *)&object->ref_count, old_ref, old_ref - 1))
	goto retry_atomic_decrement1;

      TRACE (GOBJECT_OBJECT_UNREF(object,XTYPE_FROM_INSTANCE(object),old_ref));

      /* if we went from 2->1 we need to notify toggle refs if any */
      if (old_ref == 2 && has_toggle_ref) /* The last ref being held in this case is owned by the toggle_ref */
	toggle_refs_notify (object, TRUE);
    }
  else
    {
      xslist_t **weak_locations;
      xobject_notify_queue_t *nqueue;

      /* The only way that this object can live at this point is if
       * there are outstanding weak references already established
       * before we got here.
       *
       * If there were not already weak references then no more can be
       * established at this time, because the other thread would have
       * to hold a strong ref in order to call
       * xobject_add_weak_pointer() and then we wouldn't be here.
       *
       * Other GWeakRef's (weak locations) instead may still be added
       * before the object is finalized, but in such case we'll unset
       * them as part of the qdata removal.
       */
      weak_locations = g_datalist_id_get_data (&object->qdata, quark_weak_locations);

      if (weak_locations != NULL)
        {
          g_rw_lock_writer_lock (&weak_locations_lock);

          /* It is possible that one of the weak references beat us to
           * the lock. Make sure the refcount is still what we expected
           * it to be.
           */
          old_ref = g_atomic_int_get (&object->ref_count);
          if (old_ref != 1)
            {
              g_rw_lock_writer_unlock (&weak_locations_lock);
              goto retry_atomic_decrement1;
            }

          /* We got the lock first, so the object will definitely die
           * now. Clear out all the weak references, if they're still set.
           */
          weak_locations = g_datalist_id_remove_no_notify (&object->qdata,
                                                           quark_weak_locations);
          g_clear_pointer (&weak_locations, weak_locations_free_unlocked);

          g_rw_lock_writer_unlock (&weak_locations_lock);
        }

      /* freeze the notification queue, so we don't accidentally emit
       * notifications during dispose() and finalize().
       *
       * The notification queue stays frozen unless the instance acquires
       * a reference during dispose(), in which case we thaw it and
       * dispatch all the notifications. If the instance gets through
       * to finalize(), the notification queue gets automatically
       * drained when xobject_finalize() is reached and
       * the qdata is cleared.
       */
      nqueue = xobject_notify_queue_freeze (object, FALSE);

      /* we are about to remove the last reference */
      TRACE (GOBJECT_OBJECT_DISPOSE(object,XTYPE_FROM_INSTANCE(object), 1));
      G_OBJECT_GET_CLASS (object)->dispose (object);
      TRACE (GOBJECT_OBJECT_DISPOSE_END(object,XTYPE_FROM_INSTANCE(object), 1));

      /* may have been re-referenced meanwhile */
    retry_atomic_decrement2:
      old_ref = g_atomic_int_get ((int *)&object->ref_count);
      if (old_ref > 1)
        {
          /* valid if last 2 refs are owned by this call to unref and the toggle_ref */
          xboolean_t has_toggle_ref = OBJECT_HAS_TOGGLE_REF (object);

          if (!g_atomic_int_compare_and_exchange ((int *)&object->ref_count, old_ref, old_ref - 1))
	    goto retry_atomic_decrement2;

          /* emit all notifications that have been queued during dispose() */
          xobject_notify_queue_thaw (object, nqueue);

	  TRACE (GOBJECT_OBJECT_UNREF(object,XTYPE_FROM_INSTANCE(object),old_ref));

          /* if we went from 2->1 we need to notify toggle refs if any */
          if (old_ref == 2 && has_toggle_ref) /* The last ref being held in this case is owned by the toggle_ref */
	    toggle_refs_notify (object, TRUE);

	  return;
	}

      /* we are still in the process of taking away the last ref */
      g_datalist_id_set_data (&object->qdata, quark_closure_array, NULL);
      xsignal_handlers_destroy (object);
      g_datalist_id_set_data (&object->qdata, quark_weak_refs, NULL);
      g_datalist_id_set_data (&object->qdata, quark_weak_locations, NULL);

      /* decrement the last reference */
      old_ref = g_atomic_int_add (&object->ref_count, -1);
      g_return_if_fail (old_ref > 0);

      TRACE (GOBJECT_OBJECT_UNREF(object,XTYPE_FROM_INSTANCE(object),old_ref));

      /* may have been re-referenced meanwhile */
      if (G_LIKELY (old_ref == 1))
	{
	  TRACE (GOBJECT_OBJECT_FINALIZE(object,XTYPE_FROM_INSTANCE(object)));
          G_OBJECT_GET_CLASS (object)->finalize (object);
	  TRACE (GOBJECT_OBJECT_FINALIZE_END(object,XTYPE_FROM_INSTANCE(object)));

          GOBJECT_IF_DEBUG (OBJECTS,
	    {
              xboolean_t was_present;

              /* catch objects not chaining finalize handlers */
              G_LOCK (debug_objects);
              was_present = xhash_table_remove (debug_objects_ht, object);
              G_UNLOCK (debug_objects);

              if (was_present)
                g_critical ("Object %p of type %s not finalized correctly.",
                            object, G_OBJECT_TYPE_NAME (object));
	    });
          xtype_free_instance ((GTypeInstance*) object);
	}
      else
        {
          /* The instance acquired a reference between dispose() and
           * finalize(), so we need to thaw the notification queue
           */
          xobject_notify_queue_thaw (object, nqueue);
        }
    }
}

/**
 * g_clear_object: (skip)
 * @object_ptr: a pointer to a #xobject_t reference
 *
 * Clears a reference to a #xobject_t.
 *
 * @object_ptr must not be %NULL.
 *
 * If the reference is %NULL then this function does nothing.
 * Otherwise, the reference count of the object is decreased and the
 * pointer is set to %NULL.
 *
 * A macro is also included that allows this function to be used without
 * pointer casts.
 *
 * Since: 2.28
 **/
#undef g_clear_object
void
g_clear_object (xobject_t **object_ptr)
{
  g_clear_pointer (object_ptr, xobject_unref);
}

/**
 * xobject_get_qdata:
 * @object: The xobject_t to get a stored user data pointer from
 * @quark: A #xquark, naming the user data pointer
 *
 * This function gets back user data pointers stored via
 * xobject_set_qdata().
 *
 * Returns: (transfer none) (nullable): The user data pointer set, or %NULL
 */
xpointer_t
xobject_get_qdata (xobject_t *object,
		    xquark   quark)
{
  xreturn_val_if_fail (X_IS_OBJECT (object), NULL);

  return quark ? g_datalist_id_get_data (&object->qdata, quark) : NULL;
}

/**
 * xobject_set_qdata: (skip)
 * @object: The xobject_t to set store a user data pointer
 * @quark: A #xquark, naming the user data pointer
 * @data: (nullable): An opaque user data pointer
 *
 * This sets an opaque, named pointer on an object.
 * The name is specified through a #xquark (retrieved e.g. via
 * g_quark_from_static_string()), and the pointer
 * can be gotten back from the @object with xobject_get_qdata()
 * until the @object is finalized.
 * Setting a previously set user data pointer, overrides (frees)
 * the old pointer set, using #NULL as pointer essentially
 * removes the data stored.
 */
void
xobject_set_qdata (xobject_t *object,
		    xquark   quark,
		    xpointer_t data)
{
  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (quark > 0);

  g_datalist_id_set_data (&object->qdata, quark, data);
}

/**
 * xobject_dup_qdata: (skip)
 * @object: the #xobject_t to store user data on
 * @quark: a #xquark, naming the user data pointer
 * @dup_func: (nullable): function to dup the value
 * @user_data: (nullable): passed as user_data to @dup_func
 *
 * This is a variant of xobject_get_qdata() which returns
 * a 'duplicate' of the value. @dup_func defines the
 * meaning of 'duplicate' in this context, it could e.g.
 * take a reference on a ref-counted object.
 *
 * If the @quark is not set on the object then @dup_func
 * will be called with a %NULL argument.
 *
 * Note that @dup_func is called while user data of @object
 * is locked.
 *
 * This function can be useful to avoid races when multiple
 * threads are using object data on the same key on the same
 * object.
 *
 * Returns: the result of calling @dup_func on the value
 *     associated with @quark on @object, or %NULL if not set.
 *     If @dup_func is %NULL, the value is returned
 *     unmodified.
 *
 * Since: 2.34
 */
xpointer_t
xobject_dup_qdata (xobject_t        *object,
                    xquark          quark,
                    GDuplicateFunc   dup_func,
                    xpointer_t         user_data)
{
  xreturn_val_if_fail (X_IS_OBJECT (object), NULL);
  xreturn_val_if_fail (quark > 0, NULL);

  return g_datalist_id_dup_data (&object->qdata, quark, dup_func, user_data);
}

/**
 * xobject_replace_qdata: (skip)
 * @object: the #xobject_t to store user data on
 * @quark: a #xquark, naming the user data pointer
 * @oldval: (nullable): the old value to compare against
 * @newval: (nullable): the new value
 * @destroy: (nullable): a destroy notify for the new value
 * @old_destroy: (out) (optional): destroy notify for the existing value
 *
 * Compares the user data for the key @quark on @object with
 * @oldval, and if they are the same, replaces @oldval with
 * @newval.
 *
 * This is like a typical atomic compare-and-exchange
 * operation, for user data on an object.
 *
 * If the previous value was replaced then ownership of the
 * old value (@oldval) is passed to the caller, including
 * the registered destroy notify for it (passed out in @old_destroy).
 * It???s up to the caller to free this as needed, which may
 * or may not include using @old_destroy as sometimes replacement
 * should not destroy the object in the normal way.
 *
 * Returns: %TRUE if the existing value for @quark was replaced
 *  by @newval, %FALSE otherwise.
 *
 * Since: 2.34
 */
xboolean_t
xobject_replace_qdata (xobject_t        *object,
                        xquark          quark,
                        xpointer_t        oldval,
                        xpointer_t        newval,
                        xdestroy_notify_t  destroy,
                        xdestroy_notify_t *old_destroy)
{
  xreturn_val_if_fail (X_IS_OBJECT (object), FALSE);
  xreturn_val_if_fail (quark > 0, FALSE);

  return g_datalist_id_replace_data (&object->qdata, quark,
                                     oldval, newval, destroy,
                                     old_destroy);
}

/**
 * xobject_set_qdata_full: (skip)
 * @object: The xobject_t to set store a user data pointer
 * @quark: A #xquark, naming the user data pointer
 * @data: (nullable): An opaque user data pointer
 * @destroy: (nullable): Function to invoke with @data as argument, when @data
 *           needs to be freed
 *
 * This function works like xobject_set_qdata(), but in addition,
 * a void (*destroy) (xpointer_t) function may be specified which is
 * called with @data as argument when the @object is finalized, or
 * the data is being overwritten by a call to xobject_set_qdata()
 * with the same @quark.
 */
void
xobject_set_qdata_full (xobject_t       *object,
			 xquark		quark,
			 xpointer_t	data,
			 xdestroy_notify_t destroy)
{
  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (quark > 0);

  g_datalist_id_set_data_full (&object->qdata, quark, data,
			       data ? destroy : (xdestroy_notify_t) NULL);
}

/**
 * xobject_steal_qdata:
 * @object: The xobject_t to get a stored user data pointer from
 * @quark: A #xquark, naming the user data pointer
 *
 * This function gets back user data pointers stored via
 * xobject_set_qdata() and removes the @data from object
 * without invoking its destroy() function (if any was
 * set).
 * Usually, calling this function is only required to update
 * user data pointers with a destroy notifier, for example:
 * |[<!-- language="C" -->
 * void
 * object_add_to_user_list (xobject_t     *object,
 *                          const xchar_t *new_string)
 * {
 *   // the quark, naming the object data
 *   xquark quark_string_list = g_quark_from_static_string ("my-string-list");
 *   // retrieve the old string list
 *   xlist_t *list = xobject_steal_qdata (object, quark_string_list);
 *
 *   // prepend new string
 *   list = xlist_prepend (list, xstrdup (new_string));
 *   // this changed 'list', so we need to set it again
 *   xobject_set_qdata_full (object, quark_string_list, list, free_string_list);
 * }
 * static void
 * free_string_list (xpointer_t data)
 * {
 *   xlist_t *node, *list = data;
 *
 *   for (node = list; node; node = node->next)
 *     g_free (node->data);
 *   xlist_free (list);
 * }
 * ]|
 * Using xobject_get_qdata() in the above example, instead of
 * xobject_steal_qdata() would have left the destroy function set,
 * and thus the partial string list would have been freed upon
 * xobject_set_qdata_full().
 *
 * Returns: (transfer full) (nullable): The user data pointer set, or %NULL
 */
xpointer_t
xobject_steal_qdata (xobject_t *object,
		      xquark   quark)
{
  xreturn_val_if_fail (X_IS_OBJECT (object), NULL);
  xreturn_val_if_fail (quark > 0, NULL);

  return g_datalist_id_remove_no_notify (&object->qdata, quark);
}

/**
 * xobject_get_data:
 * @object: #xobject_t containing the associations
 * @key: name of the key for that association
 *
 * Gets a named field from the objects table of associations (see xobject_set_data()).
 *
 * Returns: (transfer none) (nullable): the data if found,
 *          or %NULL if no such data exists.
 */
xpointer_t
xobject_get_data (xobject_t     *object,
                   const xchar_t *key)
{
  xreturn_val_if_fail (X_IS_OBJECT (object), NULL);
  xreturn_val_if_fail (key != NULL, NULL);

  return g_datalist_get_data (&object->qdata, key);
}

/**
 * xobject_set_data:
 * @object: #xobject_t containing the associations.
 * @key: name of the key
 * @data: (nullable): data to associate with that key
 *
 * Each object carries around a table of associations from
 * strings to pointers.  This function lets you set an association.
 *
 * If the object already had an association with that name,
 * the old association will be destroyed.
 *
 * Internally, the @key is converted to a #xquark using g_quark_from_string().
 * This means a copy of @key is kept permanently (even after @object has been
 * finalized) ??? so it is recommended to only use a small, bounded set of values
 * for @key in your program, to avoid the #xquark storage growing unbounded.
 */
void
xobject_set_data (xobject_t     *object,
                   const xchar_t *key,
                   xpointer_t     data)
{
  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_id_set_data (&object->qdata, g_quark_from_string (key), data);
}

/**
 * xobject_dup_data: (skip)
 * @object: the #xobject_t to store user data on
 * @key: a string, naming the user data pointer
 * @dup_func: (nullable): function to dup the value
 * @user_data: (nullable): passed as user_data to @dup_func
 *
 * This is a variant of xobject_get_data() which returns
 * a 'duplicate' of the value. @dup_func defines the
 * meaning of 'duplicate' in this context, it could e.g.
 * take a reference on a ref-counted object.
 *
 * If the @key is not set on the object then @dup_func
 * will be called with a %NULL argument.
 *
 * Note that @dup_func is called while user data of @object
 * is locked.
 *
 * This function can be useful to avoid races when multiple
 * threads are using object data on the same key on the same
 * object.
 *
 * Returns: the result of calling @dup_func on the value
 *     associated with @key on @object, or %NULL if not set.
 *     If @dup_func is %NULL, the value is returned
 *     unmodified.
 *
 * Since: 2.34
 */
xpointer_t
xobject_dup_data (xobject_t        *object,
                   const xchar_t    *key,
                   GDuplicateFunc   dup_func,
                   xpointer_t         user_data)
{
  xreturn_val_if_fail (X_IS_OBJECT (object), NULL);
  xreturn_val_if_fail (key != NULL, NULL);

  return g_datalist_id_dup_data (&object->qdata,
                                 g_quark_from_string (key),
                                 dup_func, user_data);
}

/**
 * xobject_replace_data: (skip)
 * @object: the #xobject_t to store user data on
 * @key: a string, naming the user data pointer
 * @oldval: (nullable): the old value to compare against
 * @newval: (nullable): the new value
 * @destroy: (nullable): a destroy notify for the new value
 * @old_destroy: (out) (optional): destroy notify for the existing value
 *
 * Compares the user data for the key @key on @object with
 * @oldval, and if they are the same, replaces @oldval with
 * @newval.
 *
 * This is like a typical atomic compare-and-exchange
 * operation, for user data on an object.
 *
 * If the previous value was replaced then ownership of the
 * old value (@oldval) is passed to the caller, including
 * the registered destroy notify for it (passed out in @old_destroy).
 * It???s up to the caller to free this as needed, which may
 * or may not include using @old_destroy as sometimes replacement
 * should not destroy the object in the normal way.
 *
 * See xobject_set_data() for guidance on using a small, bounded set of values
 * for @key.
 *
 * Returns: %TRUE if the existing value for @key was replaced
 *  by @newval, %FALSE otherwise.
 *
 * Since: 2.34
 */
xboolean_t
xobject_replace_data (xobject_t        *object,
                       const xchar_t    *key,
                       xpointer_t        oldval,
                       xpointer_t        newval,
                       xdestroy_notify_t  destroy,
                       xdestroy_notify_t *old_destroy)
{
  xreturn_val_if_fail (X_IS_OBJECT (object), FALSE);
  xreturn_val_if_fail (key != NULL, FALSE);

  return g_datalist_id_replace_data (&object->qdata,
                                     g_quark_from_string (key),
                                     oldval, newval, destroy,
                                     old_destroy);
}

/**
 * xobject_set_data_full: (skip)
 * @object: #xobject_t containing the associations
 * @key: name of the key
 * @data: (nullable): data to associate with that key
 * @destroy: (nullable): function to call when the association is destroyed
 *
 * Like xobject_set_data() except it adds notification
 * for when the association is destroyed, either by setting it
 * to a different value or when the object is destroyed.
 *
 * Note that the @destroy callback is not called if @data is %NULL.
 */
void
xobject_set_data_full (xobject_t       *object,
                        const xchar_t   *key,
                        xpointer_t       data,
                        xdestroy_notify_t destroy)
{
  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_id_set_data_full (&object->qdata, g_quark_from_string (key), data,
			       data ? destroy : (xdestroy_notify_t) NULL);
}

/**
 * xobject_steal_data:
 * @object: #xobject_t containing the associations
 * @key: name of the key
 *
 * Remove a specified datum from the object's data associations,
 * without invoking the association's destroy handler.
 *
 * Returns: (transfer full) (nullable): the data if found, or %NULL
 *          if no such data exists.
 */
xpointer_t
xobject_steal_data (xobject_t     *object,
                     const xchar_t *key)
{
  xquark quark;

  xreturn_val_if_fail (X_IS_OBJECT (object), NULL);
  xreturn_val_if_fail (key != NULL, NULL);

  quark = g_quark_try_string (key);

  return quark ? g_datalist_id_remove_no_notify (&object->qdata, quark) : NULL;
}

static void
xvalue_object_init (xvalue_t *value)
{
  value->data[0].v_pointer = NULL;
}

static void
xvalue_object_free_value (xvalue_t *value)
{
  if (value->data[0].v_pointer)
    xobject_unref (value->data[0].v_pointer);
}

static void
xvalue_object_copy_value (const xvalue_t *src_value,
			   xvalue_t	*dest_value)
{
  if (src_value->data[0].v_pointer)
    dest_value->data[0].v_pointer = xobject_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static void
xvalue_object_transform_value (const xvalue_t *src_value,
				xvalue_t       *dest_value)
{
  if (src_value->data[0].v_pointer && xtype_is_a (G_OBJECT_TYPE (src_value->data[0].v_pointer), G_VALUE_TYPE (dest_value)))
    dest_value->data[0].v_pointer = xobject_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static xpointer_t
xvalue_object_peek_pointer (const xvalue_t *value)
{
  return value->data[0].v_pointer;
}

static xchar_t*
xvalue_object_collect_value (xvalue_t	  *value,
			      xuint_t        n_collect_values,
			      xtype_c_value_t *collect_values,
			      xuint_t        collect_flags)
{
  if (collect_values[0].v_pointer)
    {
      xobject_t *object = collect_values[0].v_pointer;

      if (object->xtype_instance.g_class == NULL)
	return xstrconcat ("invalid unclassed object pointer for value type '",
			    G_VALUE_TYPE_NAME (value),
			    "'",
			    NULL);
      else if (!xvalue_type_compatible (G_OBJECT_TYPE (object), G_VALUE_TYPE (value)))
	return xstrconcat ("invalid object type '",
			    G_OBJECT_TYPE_NAME (object),
			    "' for value type '",
			    G_VALUE_TYPE_NAME (value),
			    "'",
			    NULL);
      /* never honour G_VALUE_NOCOPY_CONTENTS for ref-counted types */
      value->data[0].v_pointer = xobject_ref (object);
    }
  else
    value->data[0].v_pointer = NULL;

  return NULL;
}

static xchar_t*
xvalue_object_lcopy_value (const xvalue_t *value,
			    xuint_t        n_collect_values,
			    xtype_c_value_t *collect_values,
			    xuint_t        collect_flags)
{
  xobject_t **object_p = collect_values[0].v_pointer;

  xreturn_val_if_fail (object_p != NULL, xstrdup_printf ("value location for '%s' passed as NULL", G_VALUE_TYPE_NAME (value)));

  if (!value->data[0].v_pointer)
    *object_p = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *object_p = value->data[0].v_pointer;
  else
    *object_p = xobject_ref (value->data[0].v_pointer);

  return NULL;
}

/**
 * xvalue_set_object:
 * @value: a valid #xvalue_t of %XTYPE_OBJECT derived type
 * @v_object: (type xobject_t.Object) (nullable): object value to be set
 *
 * Set the contents of a %XTYPE_OBJECT derived #xvalue_t to @v_object.
 *
 * xvalue_set_object() increases the reference count of @v_object
 * (the #xvalue_t holds a reference to @v_object).  If you do not wish
 * to increase the reference count of the object (i.e. you wish to
 * pass your current reference to the #xvalue_t because you no longer
 * need it), use xvalue_take_object() instead.
 *
 * It is important that your #xvalue_t holds a reference to @v_object (either its
 * own, or one it has taken) to ensure that the object won't be destroyed while
 * the #xvalue_t still exists).
 */
void
xvalue_set_object (xvalue_t   *value,
		    xpointer_t  v_object)
{
  xobject_t *old;

  g_return_if_fail (G_VALUE_HOLDS_OBJECT (value));

  old = value->data[0].v_pointer;

  if (v_object)
    {
      g_return_if_fail (X_IS_OBJECT (v_object));
      g_return_if_fail (xvalue_type_compatible (G_OBJECT_TYPE (v_object), G_VALUE_TYPE (value)));

      value->data[0].v_pointer = v_object;
      xobject_ref (value->data[0].v_pointer);
    }
  else
    value->data[0].v_pointer = NULL;

  if (old)
    xobject_unref (old);
}

/**
 * xvalue_set_object_take_ownership: (skip)
 * @value: a valid #xvalue_t of %XTYPE_OBJECT derived type
 * @v_object: (nullable): object value to be set
 *
 * This is an internal function introduced mainly for C marshallers.
 *
 * Deprecated: 2.4: Use xvalue_take_object() instead.
 */
void
xvalue_set_object_take_ownership (xvalue_t  *value,
				   xpointer_t v_object)
{
  xvalue_take_object (value, v_object);
}

/**
 * xvalue_take_object: (skip)
 * @value: a valid #xvalue_t of %XTYPE_OBJECT derived type
 * @v_object: (nullable): object value to be set
 *
 * Sets the contents of a %XTYPE_OBJECT derived #xvalue_t to @v_object
 * and takes over the ownership of the caller???s reference to @v_object;
 * the caller doesn???t have to unref it any more (i.e. the reference
 * count of the object is not increased).
 *
 * If you want the #xvalue_t to hold its own reference to @v_object, use
 * xvalue_set_object() instead.
 *
 * Since: 2.4
 */
void
xvalue_take_object (xvalue_t  *value,
		     xpointer_t v_object)
{
  g_return_if_fail (G_VALUE_HOLDS_OBJECT (value));

  if (value->data[0].v_pointer)
    {
      xobject_unref (value->data[0].v_pointer);
      value->data[0].v_pointer = NULL;
    }

  if (v_object)
    {
      g_return_if_fail (X_IS_OBJECT (v_object));
      g_return_if_fail (xvalue_type_compatible (G_OBJECT_TYPE (v_object), G_VALUE_TYPE (value)));

      value->data[0].v_pointer = v_object; /* we take over the reference count */
    }
}

/**
 * xvalue_get_object:
 * @value: a valid #xvalue_t of %XTYPE_OBJECT derived type
 *
 * Get the contents of a %XTYPE_OBJECT derived #xvalue_t.
 *
 * Returns: (type xobject_t.Object) (transfer none): object contents of @value
 */
xpointer_t
xvalue_get_object (const xvalue_t *value)
{
  xreturn_val_if_fail (G_VALUE_HOLDS_OBJECT (value), NULL);

  return value->data[0].v_pointer;
}

/**
 * xvalue_dup_object:
 * @value: a valid #xvalue_t whose type is derived from %XTYPE_OBJECT
 *
 * Get the contents of a %XTYPE_OBJECT derived #xvalue_t, increasing
 * its reference count. If the contents of the #xvalue_t are %NULL, then
 * %NULL will be returned.
 *
 * Returns: (type xobject_t.Object) (transfer full): object content of @value,
 *          should be unreferenced when no longer needed.
 */
xpointer_t
xvalue_dup_object (const xvalue_t *value)
{
  xreturn_val_if_fail (G_VALUE_HOLDS_OBJECT (value), NULL);

  return value->data[0].v_pointer ? xobject_ref (value->data[0].v_pointer) : NULL;
}

/**
 * xsignal_connect_object: (skip)
 * @instance: (type xobject_t.TypeInstance): the instance to connect to.
 * @detailed_signal: a string of the form "signal-name::detail".
 * @c_handler: the #xcallback_t to connect.
 * @gobject: (type xobject_t.Object) (nullable): the object to pass as data
 *    to @c_handler.
 * @connect_flags: a combination of #GConnectFlags.
 *
 * This is similar to xsignal_connect_data(), but uses a closure which
 * ensures that the @gobject stays alive during the call to @c_handler
 * by temporarily adding a reference count to @gobject.
 *
 * When the @gobject is destroyed the signal handler will be automatically
 * disconnected.  Note that this is not currently threadsafe (ie:
 * emitting a signal while @gobject is being destroyed in another thread
 * is not safe).
 *
 * Returns: the handler id.
 */
xulong_t
xsignal_connect_object (xpointer_t      instance,
			 const xchar_t  *detailed_signal,
			 xcallback_t     c_handler,
			 xpointer_t      gobject,
			 GConnectFlags connect_flags)
{
  xreturn_val_if_fail (XTYPE_CHECK_INSTANCE (instance), 0);
  xreturn_val_if_fail (detailed_signal != NULL, 0);
  xreturn_val_if_fail (c_handler != NULL, 0);

  if (gobject)
    {
      xclosure_t *closure;

      xreturn_val_if_fail (X_IS_OBJECT (gobject), 0);

      closure = ((connect_flags & G_CONNECT_SWAPPED) ? g_cclosure_new_object_swap : g_cclosure_new_object) (c_handler, gobject);

      return xsignal_connect_closure (instance, detailed_signal, closure, connect_flags & G_CONNECT_AFTER);
    }
  else
    return xsignal_connect_data (instance, detailed_signal, c_handler, NULL, NULL, connect_flags);
}

typedef struct {
  xobject_t  *object;
  xuint_t     n_closures;
  xclosure_t *closures[1]; /* flexible array */
} CArray;
/* don't change this structure without supplying an accessor for
 * watched closures, e.g.:
 * xslist_t* xobject_list_watched_closures (xobject_t *object)
 * {
 *   CArray *carray;
 *   xreturn_val_if_fail (X_IS_OBJECT (object), NULL);
 *   carray = xobject_get_data (object, "xobject-closure-array");
 *   if (carray)
 *     {
 *       xslist_t *slist = NULL;
 *       xuint_t i;
 *       for (i = 0; i < carray->n_closures; i++)
 *         slist = xslist_prepend (slist, carray->closures[i]);
 *       return slist;
 *     }
 *   return NULL;
 * }
 */

static void
object_remove_closure (xpointer_t  data,
		       xclosure_t *closure)
{
  xobject_t *object = data;
  CArray *carray;
  xuint_t i;

  G_LOCK (closure_array_mutex);
  carray = xobject_get_qdata (object, quark_closure_array);
  for (i = 0; i < carray->n_closures; i++)
    if (carray->closures[i] == closure)
      {
	carray->n_closures--;
	if (i < carray->n_closures)
	  carray->closures[i] = carray->closures[carray->n_closures];
	G_UNLOCK (closure_array_mutex);
	return;
      }
  G_UNLOCK (closure_array_mutex);
  g_assert_not_reached ();
}

static void
destroy_closure_array (xpointer_t data)
{
  CArray *carray = data;
  xobject_t *object = carray->object;
  xuint_t i, n = carray->n_closures;

  for (i = 0; i < n; i++)
    {
      xclosure_t *closure = carray->closures[i];

      /* removing object_remove_closure() upfront is probably faster than
       * letting it fiddle with quark_closure_array which is empty anyways
       */
      xclosure_remove_invalidate_notifier (closure, object, object_remove_closure);
      xclosure_invalidate (closure);
    }
  g_free (carray);
}

/**
 * xobject_watch_closure:
 * @object: #xobject_t restricting lifetime of @closure
 * @closure: #xclosure_t to watch
 *
 * This function essentially limits the life time of the @closure to
 * the life time of the object. That is, when the object is finalized,
 * the @closure is invalidated by calling xclosure_invalidate() on
 * it, in order to prevent invocations of the closure with a finalized
 * (nonexisting) object. Also, xobject_ref() and xobject_unref() are
 * added as marshal guards to the @closure, to ensure that an extra
 * reference count is held on @object during invocation of the
 * @closure.  Usually, this function will be called on closures that
 * use this @object as closure data.
 */
void
xobject_watch_closure (xobject_t  *object,
			xclosure_t *closure)
{
  CArray *carray;
  xuint_t i;

  g_return_if_fail (X_IS_OBJECT (object));
  g_return_if_fail (closure != NULL);
  g_return_if_fail (closure->is_invalid == FALSE);
  g_return_if_fail (closure->in_marshal == FALSE);
  g_return_if_fail (g_atomic_int_get (&object->ref_count) > 0);	/* this doesn't work on finalizing objects */

  xclosure_add_invalidate_notifier (closure, object, object_remove_closure);
  xclosure_add_marshal_guards (closure,
				object, (xclosure_notify_t) xobject_ref,
				object, (xclosure_notify_t) xobject_unref);
  G_LOCK (closure_array_mutex);
  carray = g_datalist_id_remove_no_notify (&object->qdata, quark_closure_array);
  if (!carray)
    {
      carray = g_renew (CArray, NULL, 1);
      carray->object = object;
      carray->n_closures = 1;
      i = 0;
    }
  else
    {
      i = carray->n_closures++;
      carray = g_realloc (carray, sizeof (*carray) + sizeof (carray->closures[0]) * i);
    }
  carray->closures[i] = closure;
  g_datalist_id_set_data_full (&object->qdata, quark_closure_array, carray, destroy_closure_array);
  G_UNLOCK (closure_array_mutex);
}

/**
 * xclosure_new_object:
 * @sizeof_closure: the size of the structure to allocate, must be at least
 *  `sizeof (xclosure_t)`
 * @object: a #xobject_t pointer to store in the @data field of the newly
 *  allocated #xclosure_t
 *
 * A variant of xclosure_new_simple() which stores @object in the
 * @data field of the closure and calls xobject_watch_closure() on
 * @object and the created closure. This function is mainly useful
 * when implementing new types of closures.
 *
 * Returns: (transfer floating): a newly allocated #xclosure_t
 */
xclosure_t *
xclosure_new_object (xuint_t    sizeof_closure,
		      xobject_t *object)
{
  xclosure_t *closure;

  xreturn_val_if_fail (X_IS_OBJECT (object), NULL);
  xreturn_val_if_fail (g_atomic_int_get (&object->ref_count) > 0, NULL);     /* this doesn't work on finalizing objects */

  closure = xclosure_new_simple (sizeof_closure, object);
  xobject_watch_closure (object, closure);

  return closure;
}

/**
 * g_cclosure_new_object: (skip)
 * @callback_func: the function to invoke
 * @object: a #xobject_t pointer to pass to @callback_func
 *
 * A variant of g_cclosure_new() which uses @object as @user_data and
 * calls xobject_watch_closure() on @object and the created
 * closure. This function is useful when you have a callback closely
 * associated with a #xobject_t, and want the callback to no longer run
 * after the object is is freed.
 *
 * Returns: (transfer floating): a new #GCClosure
 */
xclosure_t *
g_cclosure_new_object (xcallback_t callback_func,
		       xobject_t  *object)
{
  xclosure_t *closure;

  xreturn_val_if_fail (X_IS_OBJECT (object), NULL);
  xreturn_val_if_fail (g_atomic_int_get (&object->ref_count) > 0, NULL);     /* this doesn't work on finalizing objects */
  xreturn_val_if_fail (callback_func != NULL, NULL);

  closure = g_cclosure_new (callback_func, object, NULL);
  xobject_watch_closure (object, closure);

  return closure;
}

/**
 * g_cclosure_new_object_swap: (skip)
 * @callback_func: the function to invoke
 * @object: a #xobject_t pointer to pass to @callback_func
 *
 * A variant of g_cclosure_new_swap() which uses @object as @user_data
 * and calls xobject_watch_closure() on @object and the created
 * closure. This function is useful when you have a callback closely
 * associated with a #xobject_t, and want the callback to no longer run
 * after the object is is freed.
 *
 * Returns: (transfer floating): a new #GCClosure
 */
xclosure_t *
g_cclosure_new_object_swap (xcallback_t callback_func,
			    xobject_t  *object)
{
  xclosure_t *closure;

  xreturn_val_if_fail (X_IS_OBJECT (object), NULL);
  xreturn_val_if_fail (g_atomic_int_get (&object->ref_count) > 0, NULL);     /* this doesn't work on finalizing objects */
  xreturn_val_if_fail (callback_func != NULL, NULL);

  closure = g_cclosure_new_swap (callback_func, object, NULL);
  xobject_watch_closure (object, closure);

  return closure;
}

xsize_t
xobject_compat_control (xsize_t           what,
                         xpointer_t        data)
{
  switch (what)
    {
      xpointer_t *pp;
    case 1:     /* floating base type */
      return XTYPE_INITIALLY_UNOWNED;
    case 2:     /* FIXME: remove this once GLib/Gtk+ break ABI again */
      floating_flag_handler = (xuint_t(*)(xobject_t*,xint_t)) data;
      return 1;
    case 3:     /* FIXME: remove this once GLib/Gtk+ break ABI again */
      pp = data;
      *pp = floating_flag_handler;
      return 1;
    default:
      return 0;
    }
}

XDEFINE_TYPE (xinitially_unowned, xinitially_unowned, XTYPE_OBJECT)

static void
xinitially_unowned_init (xinitially_unowned_t *object)
{
  xobject_force_floating (object);
}

static void
xinitially_unowned_class_init (xinitially_unowned_class_t *klass)
{
}

/**
 * GWeakRef:
 *
 * A structure containing a weak reference to a #xobject_t.
 *
 * A `GWeakRef` can either be empty (i.e. point to %NULL), or point to an
 * object for as long as at least one "strong" reference to that object
 * exists. Before the object's #xobject_class_t.dispose method is called,
 * every #GWeakRef associated with becomes empty (i.e. points to %NULL).
 *
 * Like #xvalue_t, #GWeakRef can be statically allocated, stack- or
 * heap-allocated, or embedded in larger structures.
 *
 * Unlike xobject_weak_ref() and xobject_add_weak_pointer(), this weak
 * reference is thread-safe: converting a weak pointer to a reference is
 * atomic with respect to invalidation of weak pointers to destroyed
 * objects.
 *
 * If the object's #xobject_class_t.dispose method results in additional
 * references to the object being held (???re-referencing???), any #GWeakRefs taken
 * before it was disposed will continue to point to %NULL.  Any #GWeakRefs taken
 * during disposal and after re-referencing, or after disposal has returned due
 * to the re-referencing, will continue to point to the object until its refcount
 * goes back to zero, at which point they too will be invalidated.
 *
 * It is invalid to take a #GWeakRef on an object during #xobject_class_t.dispose
 * without first having or creating a strong reference to the object.
 */

/**
 * g_weak_ref_init: (skip)
 * @weak_ref: (inout): uninitialized or empty location for a weak
 *    reference
 * @object: (type xobject_t.Object) (nullable): a #xobject_t or %NULL
 *
 * Initialise a non-statically-allocated #GWeakRef.
 *
 * This function also calls g_weak_ref_set() with @object on the
 * freshly-initialised weak reference.
 *
 * This function should always be matched with a call to
 * g_weak_ref_clear().  It is not necessary to use this function for a
 * #GWeakRef in static storage because it will already be
 * properly initialised.  Just use g_weak_ref_set() directly.
 *
 * Since: 2.32
 */
void
g_weak_ref_init (GWeakRef *weak_ref,
                 xpointer_t  object)
{
  weak_ref->priv.p = NULL;

  g_weak_ref_set (weak_ref, object);
}

/**
 * g_weak_ref_clear: (skip)
 * @weak_ref: (inout): location of a weak reference, which
 *  may be empty
 *
 * Frees resources associated with a non-statically-allocated #GWeakRef.
 * After this call, the #GWeakRef is left in an undefined state.
 *
 * You should only call this on a #GWeakRef that previously had
 * g_weak_ref_init() called on it.
 *
 * Since: 2.32
 */
void
g_weak_ref_clear (GWeakRef *weak_ref)
{
  g_weak_ref_set (weak_ref, NULL);

  /* be unkind */
  weak_ref->priv.p = (void *) 0xccccccccu;
}

/**
 * g_weak_ref_get: (skip)
 * @weak_ref: (inout): location of a weak reference to a #xobject_t
 *
 * If @weak_ref is not empty, atomically acquire a strong
 * reference to the object it points to, and return that reference.
 *
 * This function is needed because of the potential race between taking
 * the pointer value and xobject_ref() on it, if the object was losing
 * its last reference at the same time in a different thread.
 *
 * The caller should release the resulting reference in the usual way,
 * by using xobject_unref().
 *
 * Returns: (transfer full) (type xobject_t.Object): the object pointed to
 *     by @weak_ref, or %NULL if it was empty
 *
 * Since: 2.32
 */
xpointer_t
g_weak_ref_get (GWeakRef *weak_ref)
{
  xpointer_t object_or_null;

  xreturn_val_if_fail (weak_ref!= NULL, NULL);

  g_rw_lock_reader_lock (&weak_locations_lock);

  object_or_null = weak_ref->priv.p;

  if (object_or_null != NULL)
    xobject_ref (object_or_null);

  g_rw_lock_reader_unlock (&weak_locations_lock);

  return object_or_null;
}

static void
weak_locations_free_unlocked (xslist_t **weak_locations)
{
  if (*weak_locations)
    {
      xslist_t *weak_location;

      for (weak_location = *weak_locations; weak_location;)
        {
          GWeakRef *weak_ref_location = weak_location->data;

          weak_ref_location->priv.p = NULL;
          weak_location = xslist_delete_link (weak_location, weak_location);
        }
    }

  g_free (weak_locations);
}

static void
weak_locations_free (xpointer_t data)
{
  xslist_t **weak_locations = data;

  g_rw_lock_writer_lock (&weak_locations_lock);
  weak_locations_free_unlocked (weak_locations);
  g_rw_lock_writer_unlock (&weak_locations_lock);
}

/**
 * g_weak_ref_set: (skip)
 * @weak_ref: location for a weak reference
 * @object: (type xobject_t.Object) (nullable): a #xobject_t or %NULL
 *
 * Change the object to which @weak_ref points, or set it to
 * %NULL.
 *
 * You must own a strong reference on @object while calling this
 * function.
 *
 * Since: 2.32
 */
void
g_weak_ref_set (GWeakRef *weak_ref,
                xpointer_t  object)
{
  xslist_t **weak_locations;
  xobject_t *new_object;
  xobject_t *old_object;

  g_return_if_fail (weak_ref != NULL);
  g_return_if_fail (object == NULL || X_IS_OBJECT (object));

  new_object = object;

  g_rw_lock_writer_lock (&weak_locations_lock);

  /* We use the extra level of indirection here so that if we have ever
   * had a weak pointer installed at any point in time on this object,
   * we can see that there is a non-NULL value associated with the
   * weak-pointer quark and know that this value will not change at any
   * point in the object's lifetime.
   *
   * Both properties are important for reducing the amount of times we
   * need to acquire locks and for decreasing the duration of time the
   * lock is held while avoiding some rather tricky races.
   *
   * Specifically: we can avoid having to do an extra unconditional lock
   * in xobject_unref() without worrying about some extremely tricky
   * races.
   */

  old_object = weak_ref->priv.p;
  if (new_object != old_object)
    {
      weak_ref->priv.p = new_object;

      /* Remove the weak ref from the old object */
      if (old_object != NULL)
        {
          weak_locations = g_datalist_id_get_data (&old_object->qdata, quark_weak_locations);
          /* for it to point to an object, the object must have had it added once */
          xassert (weak_locations != NULL);

          *weak_locations = xslist_remove (*weak_locations, weak_ref);

          if (!*weak_locations)
            {
              weak_locations_free_unlocked (weak_locations);
              g_datalist_id_remove_no_notify (&old_object->qdata, quark_weak_locations);
            }
        }

      /* Add the weak ref to the new object */
      if (new_object != NULL)
        {
          weak_locations = g_datalist_id_get_data (&new_object->qdata, quark_weak_locations);

          if (weak_locations == NULL)
            {
              weak_locations = g_new0 (xslist_t *, 1);
              g_datalist_id_set_data_full (&new_object->qdata, quark_weak_locations,
                                           weak_locations, weak_locations_free);
            }

          *weak_locations = xslist_prepend (*weak_locations, weak_ref);
        }
    }

  g_rw_lock_writer_unlock (&weak_locations_lock);
}
