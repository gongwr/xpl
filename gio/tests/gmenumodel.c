#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>
#include <glib/gstdio.h>
#include <string.h>

#include "gdbus-sessionbus.h"

#include "glib/glib-private.h"

static xboolean_t
time_out (xpointer_t unused G_GNUC_UNUSED)
{
  xerror ("Timed out");
  /* not reached */
  return FALSE;
}

static xuint_t
add_timeout (xuint_t seconds)
{
#ifdef G_OS_UNIX
  /* Safety-catch against the main loop having blocked */
  alarm (seconds + 5);
#endif
  return g_timeout_add_seconds (seconds, time_out, NULL);
}

static void
cancel_timeout (xuint_t timeout_id)
{
#ifdef G_OS_UNIX
  alarm (0);
#endif
  xsource_remove (timeout_id);
}

/* Markup printing {{{1 */

/* This used to be part of GLib, but it was removed before the stable
 * release because it wasn't generally useful.  We want it here, though.
 */
static void
indent_string (xstring_t *string,
               xint_t     indent)
{
  while (indent--)
    xstring_append_c (string, ' ');
}

static xstring_t *
xmenu_markup_print_string (xstring_t    *string,
                            xmenu_model_t *model,
                            xint_t        indent,
                            xint_t        tabstop)
{
  xboolean_t need_nl = FALSE;
  xint_t i, n;

  if G_UNLIKELY (string == NULL)
    string = xstring_new (NULL);

  n = xmenu_model_get_n_items (model);

  for (i = 0; i < n; i++)
    {
      xmenu_attribute_iter_t *attr_iter;
      xmenu_link_iter_t *link_iter;
      xstring_t *contents;
      xstring_t *attrs;

      attr_iter = xmenu_model_iterate_item_attributes (model, i);
      link_iter = xmenu_model_iterate_item_links (model, i);
      contents = xstring_new (NULL);
      attrs = xstring_new (NULL);

      while (xmenu_attribute_iter_next (attr_iter))
        {
          const char *name = xmenu_attribute_iter_get_name (attr_iter);
          xvariant_t *value = xmenu_attribute_iter_get_value (attr_iter);

          if (xvariant_is_of_type (value, G_VARIANT_TYPE_STRING))
            {
              xchar_t *str;
              str = g_markup_printf_escaped (" %s='%s'", name, xvariant_get_string (value, NULL));
              xstring_append (attrs, str);
              g_free (str);
            }

          else
            {
              xchar_t *printed;
              xchar_t *str;
              const xchar_t *type;

              printed = xvariant_print (value, TRUE);
              type = xvariant_type_peek_string (xvariant_get_type (value));
              str = g_markup_printf_escaped ("<attribute name='%s' type='%s'>%s</attribute>\n", name, type, printed);
              indent_string (contents, indent + tabstop);
              xstring_append (contents, str);
              g_free (printed);
              g_free (str);
            }

          xvariant_unref (value);
        }
      xobject_unref (attr_iter);

      while (xmenu_link_iter_next (link_iter))
        {
          const xchar_t *name = xmenu_link_iter_get_name (link_iter);
          xmenu_model_t *menu = xmenu_link_iter_get_value (link_iter);
          xchar_t *str;

          if (contents->str[0])
            xstring_append_c (contents, '\n');

          str = g_markup_printf_escaped ("<link name='%s'>\n", name);
          indent_string (contents, indent + tabstop);
          xstring_append (contents, str);
          g_free (str);

          xmenu_markup_print_string (contents, menu, indent + 2 * tabstop, tabstop);

          indent_string (contents, indent + tabstop);
          xstring_append (contents, "</link>\n");
          xobject_unref (menu);
        }
      xobject_unref (link_iter);

      if (contents->str[0])
        {
          indent_string (string, indent);
          xstring_append_printf (string, "<item%s>\n", attrs->str);
          xstring_append (string, contents->str);
          indent_string (string, indent);
          xstring_append (string, "</item>\n");
          need_nl = TRUE;
        }

      else
        {
          if (need_nl)
            xstring_append_c (string, '\n');

          indent_string (string, indent);
          xstring_append_printf (string, "<item%s/>\n", attrs->str);
          need_nl = FALSE;
        }

      xstring_free (contents, TRUE);
      xstring_free (attrs, TRUE);
    }

  return string;
}

/* TestItem {{{1 */

/* This utility struct is used by both the RandomMenu and MirrorMenu
 * class implementations below.
 */
typedef struct {
  xhashtable_t *attributes;
  xhashtable_t *links;
} TestItem;

static TestItem *
test_item_new (xhashtable_t *attributes,
               xhashtable_t *links)
{
  TestItem *item;

  item = g_slice_new (TestItem);
  item->attributes = xhash_table_ref (attributes);
  item->links = xhash_table_ref (links);

  return item;
}

static void
test_item_free (xpointer_t data)
{
  TestItem *item = data;

  xhash_table_unref (item->attributes);
  xhash_table_unref (item->links);

  g_slice_free (TestItem, item);
}

/* RandomMenu {{{1 */
#define MAX_ITEMS 5
#define TOP_ORDER 4

typedef struct {
  xmenu_model_t parent_instance;

  xsequence_t *items;
  xint_t order;
} RandomMenu;

typedef xmenu_model_class_t RandomMenuClass;

static xtype_t random_menu_get_type (void);
G_DEFINE_TYPE (RandomMenu, random_menu, XTYPE_MENU_MODEL)

static xboolean_t
random_menu_is_mutable (xmenu_model_t *model)
{
  return TRUE;
}

static xint_t
random_menu_get_n_items (xmenu_model_t *model)
{
  RandomMenu *menu = (RandomMenu *) model;

  return g_sequence_get_length (menu->items);
}

static void
random_menu_get_item_attributes (xmenu_model_t  *model,
                                 xint_t         position,
                                 xhashtable_t **table)
{
  RandomMenu *menu = (RandomMenu *) model;
  TestItem *item;

  item = g_sequence_get (g_sequence_get_iter_at_pos (menu->items, position));
  *table = xhash_table_ref (item->attributes);
}

static void
random_menu_get_item_links (xmenu_model_t  *model,
                            xint_t         position,
                            xhashtable_t **table)
{
  RandomMenu *menu = (RandomMenu *) model;
  TestItem *item;

  item = g_sequence_get (g_sequence_get_iter_at_pos (menu->items, position));
  *table = xhash_table_ref (item->links);
}

static void
random_menu_finalize (xobject_t *object)
{
  RandomMenu *menu = (RandomMenu *) object;

  g_sequence_free (menu->items);

  G_OBJECT_CLASS (random_menu_parent_class)
    ->finalize (object);
}

static void
random_menu_init (RandomMenu *menu)
{
}

static void
random_menu_class_init (xmenu_model_class_t *class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (class);

  class->is_mutable = random_menu_is_mutable;
  class->get_n_items = random_menu_get_n_items;
  class->get_item_attributes = random_menu_get_item_attributes;
  class->get_item_links = random_menu_get_item_links;

  object_class->finalize = random_menu_finalize;
}

static RandomMenu * random_menu_new (xrand_t *rand, xint_t order);

static void
random_menu_change (RandomMenu *menu,
                    xrand_t      *rand)
{
  xint_t position, removes, adds;
  GSequenceIter *point;
  xint_t n_items;
  xint_t i;

  n_items = g_sequence_get_length (menu->items);

  do
    {
      position = g_rand_int_range (rand, 0, n_items + 1);
      removes = g_rand_int_range (rand, 0, n_items - position + 1);
      adds = g_rand_int_range (rand, 0, MAX_ITEMS - (n_items - removes) + 1);
    }
  while (removes == 0 && adds == 0);

  point = g_sequence_get_iter_at_pos (menu->items, position + removes);

  if (removes)
    {
      GSequenceIter *start;

      start = g_sequence_get_iter_at_pos (menu->items, position);
      g_sequence_remove_range (start, point);
    }

  for (i = 0; i < adds; i++)
    {
      const xchar_t *label;
      xhashtable_t *links;
      xhashtable_t *attributes;

      attributes = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t) xvariant_unref);
      links = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t) xobject_unref);

      if (menu->order > 0 && g_rand_boolean (rand))
        {
          RandomMenu *child;
	  const xchar_t *subtype;

          child = random_menu_new (rand, menu->order - 1);

          if (g_rand_boolean (rand))
            {
              subtype = XMENU_LINK_SECTION;
              /* label some section headers */
              if (g_rand_boolean (rand))
                label = "Section";
              else
                label = NULL;
            }
          else
            {
              /* label all submenus */
              subtype = XMENU_LINK_SUBMENU;
              label = "Submenu";
            }

          xhash_table_insert (links, xstrdup (subtype), child);
        }
      else
        /* label all terminals */
        label = "Menu Item";

      if (label)
        xhash_table_insert (attributes, xstrdup ("label"), xvariant_ref_sink (xvariant_new_string (label)));

      g_sequence_insert_before (point, test_item_new (attributes, links));
      xhash_table_unref (links);
      xhash_table_unref (attributes);
    }

  xmenu_model_items_changed (XMENU_MODEL (menu), position, removes, adds);
}

static RandomMenu *
random_menu_new (xrand_t *rand,
                 xint_t   order)
{
  RandomMenu *menu;

  menu = xobject_new (random_menu_get_type (), NULL);
  menu->items = g_sequence_new (test_item_free);
  menu->order = order;

  random_menu_change (menu, rand);

  return menu;
}

/* MirrorMenu {{{1 */
typedef struct {
  xmenu_model_t parent_instance;

  xmenu_model_t *clone_of;
  xsequence_t *items;
  gulong handler_id;
} MirrorMenu;

typedef xmenu_model_class_t MirrorMenuClass;

static xtype_t mirror_menu_get_type (void);
G_DEFINE_TYPE (MirrorMenu, mirror_menu, XTYPE_MENU_MODEL)

static xboolean_t
mirror_menu_is_mutable (xmenu_model_t *model)
{
  MirrorMenu *menu = (MirrorMenu *) model;

  return menu->handler_id != 0;
}

static xint_t
mirror_menu_get_n_items (xmenu_model_t *model)
{
  MirrorMenu *menu = (MirrorMenu *) model;

  return g_sequence_get_length (menu->items);
}

static void
mirror_menu_get_item_attributes (xmenu_model_t  *model,
                                 xint_t         position,
                                 xhashtable_t **table)
{
  MirrorMenu *menu = (MirrorMenu *) model;
  TestItem *item;

  item = g_sequence_get (g_sequence_get_iter_at_pos (menu->items, position));
  *table = xhash_table_ref (item->attributes);
}

static void
mirror_menu_get_item_links (xmenu_model_t  *model,
                            xint_t         position,
                            xhashtable_t **table)
{
  MirrorMenu *menu = (MirrorMenu *) model;
  TestItem *item;

  item = g_sequence_get (g_sequence_get_iter_at_pos (menu->items, position));
  *table = xhash_table_ref (item->links);
}

static void
mirror_menu_finalize (xobject_t *object)
{
  MirrorMenu *menu = (MirrorMenu *) object;

  if (menu->handler_id)
    g_signal_handler_disconnect (menu->clone_of, menu->handler_id);

  g_sequence_free (menu->items);
  xobject_unref (menu->clone_of);

  G_OBJECT_CLASS (mirror_menu_parent_class)
    ->finalize (object);
}

static void
mirror_menu_init (MirrorMenu *menu)
{
}

static void
mirror_menu_class_init (xmenu_model_class_t *class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (class);

  class->is_mutable = mirror_menu_is_mutable;
  class->get_n_items = mirror_menu_get_n_items;
  class->get_item_attributes = mirror_menu_get_item_attributes;
  class->get_item_links = mirror_menu_get_item_links;

  object_class->finalize = mirror_menu_finalize;
}

static MirrorMenu * mirror_menu_new (xmenu_model_t *clone_of);

static void
mirror_menu_changed (xmenu_model_t *model,
                     xint_t        position,
                     xint_t        removed,
                     xint_t        added,
                     xpointer_t    user_data)
{
  MirrorMenu *menu = user_data;
  GSequenceIter *point;
  xint_t i;

  g_assert (model == menu->clone_of);

  point = g_sequence_get_iter_at_pos (menu->items, position + removed);

  if (removed)
    {
      GSequenceIter *start;

      start = g_sequence_get_iter_at_pos (menu->items, position);
      g_sequence_remove_range (start, point);
    }

  for (i = position; i < position + added; i++)
    {
      xmenu_attribute_iter_t *attr_iter;
      xmenu_link_iter_t *link_iter;
      xhashtable_t *links;
      xhashtable_t *attributes;
      const xchar_t *name;
      xmenu_model_t *child;
      xvariant_t *value;

      attributes = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t) xvariant_unref);
      links = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t) xobject_unref);

      attr_iter = xmenu_model_iterate_item_attributes (model, i);
      while (xmenu_attribute_iter_get_next (attr_iter, &name, &value))
        {
          xhash_table_insert (attributes, xstrdup (name), value);
        }
      xobject_unref (attr_iter);

      link_iter = xmenu_model_iterate_item_links (model, i);
      while (xmenu_link_iter_get_next (link_iter, &name, &child))
        {
          xhash_table_insert (links, xstrdup (name), mirror_menu_new (child));
          xobject_unref (child);
        }
      xobject_unref (link_iter);

      g_sequence_insert_before (point, test_item_new (attributes, links));
      xhash_table_unref (attributes);
      xhash_table_unref (links);
    }

  xmenu_model_items_changed (XMENU_MODEL (menu), position, removed, added);
}

static MirrorMenu *
mirror_menu_new (xmenu_model_t *clone_of)
{
  MirrorMenu *menu;

  menu = xobject_new (mirror_menu_get_type (), NULL);
  menu->items = g_sequence_new (test_item_free);
  menu->clone_of = xobject_ref (clone_of);

  if (xmenu_model_is_mutable (clone_of))
    menu->handler_id = g_signal_connect (clone_of, "items-changed", G_CALLBACK (mirror_menu_changed), menu);
  mirror_menu_changed (clone_of, 0, 0, xmenu_model_get_n_items (clone_of), menu);

  return menu;
}

/* check_menus_equal(), assert_menus_equal() {{{1 */
static xboolean_t
check_menus_equal (xmenu_model_t *a,
                   xmenu_model_t *b)
{
  xboolean_t equal = TRUE;
  xint_t a_n, b_n;
  xint_t i;

  a_n = xmenu_model_get_n_items (a);
  b_n = xmenu_model_get_n_items (b);

  if (a_n != b_n)
    return FALSE;

  for (i = 0; i < a_n; i++)
    {
      xmenu_attribute_iter_t *attr_iter;
      xvariant_t *a_value, *b_value;
      xmenu_link_iter_t *link_iter;
      xmenu_model_t *a_menu, *b_menu;
      const xchar_t *name;

      attr_iter = xmenu_model_iterate_item_attributes (a, i);
      while (xmenu_attribute_iter_get_next (attr_iter, &name, &a_value))
        {
          b_value = xmenu_model_get_item_attribute_value (b, i, name, NULL);
          equal &= b_value && xvariant_equal (a_value, b_value);
          if (b_value)
            xvariant_unref (b_value);
          xvariant_unref (a_value);
        }
      xobject_unref (attr_iter);

      attr_iter = xmenu_model_iterate_item_attributes (b, i);
      while (xmenu_attribute_iter_get_next (attr_iter, &name, &b_value))
        {
          a_value = xmenu_model_get_item_attribute_value (a, i, name, NULL);
          equal &= a_value && xvariant_equal (a_value, b_value);
          if (a_value)
            xvariant_unref (a_value);
          xvariant_unref (b_value);
        }
      xobject_unref (attr_iter);

      link_iter = xmenu_model_iterate_item_links (a, i);
      while (xmenu_link_iter_get_next (link_iter, &name, &a_menu))
        {
          b_menu = xmenu_model_get_item_link (b, i, name);
          equal &= b_menu && check_menus_equal (a_menu, b_menu);
          if (b_menu)
            xobject_unref (b_menu);
          xobject_unref (a_menu);
        }
      xobject_unref (link_iter);

      link_iter = xmenu_model_iterate_item_links (b, i);
      while (xmenu_link_iter_get_next (link_iter, &name, &b_menu))
        {
          a_menu = xmenu_model_get_item_link (a, i, name);
          equal &= a_menu && check_menus_equal (a_menu, b_menu);
          if (a_menu)
            xobject_unref (a_menu);
          xobject_unref (b_menu);
        }
      xobject_unref (link_iter);
    }

  return equal;
}

static void
assert_menus_equal (xmenu_model_t *a,
                    xmenu_model_t *b)
{
  if (!check_menus_equal (a, b))
    {
      xstring_t *string;

      string = xstring_new ("\n  <a>\n");
      xmenu_markup_print_string (string, XMENU_MODEL (a), 4, 2);
      xstring_append (string, "  </a>\n\n-------------\n  <b>\n");
      xmenu_markup_print_string (string, XMENU_MODEL (b), 4, 2);
      xstring_append (string, "  </b>\n");
      xerror ("%s", string->str);
    }
}

static void
assert_menuitem_equal (xmenu_item_t  *item,
                       xmenu_model_t *model,
                       xint_t        index)
{
  xmenu_attribute_iter_t *attr_iter;
  xmenu_link_iter_t *link_iter;
  const xchar_t *name;
  xvariant_t *value;
  xmenu_model_t *linked_model;

  /* NOTE we can't yet test whether item has attributes or links that
   * are not in the model, because there's no iterator API for menu
   * items */

  attr_iter = xmenu_model_iterate_item_attributes (model, index);
  while (xmenu_attribute_iter_get_next (attr_iter, &name, &value))
    {
      xvariant_t *item_value;

      item_value = xmenu_item_get_attribute_value (item, name, xvariant_get_type (value));
      g_assert (item_value && xvariant_equal (item_value, value));

      xvariant_unref (item_value);
      xvariant_unref (value);
    }

  link_iter = xmenu_model_iterate_item_links (model, index);
  while (xmenu_link_iter_get_next (link_iter, &name, &linked_model))
    {
      xmenu_model_t *item_linked_model;

      item_linked_model = xmenu_item_get_link (item, name);
      g_assert (linked_model == item_linked_model);

      xobject_unref (item_linked_model);
      xobject_unref (linked_model);
    }

  xobject_unref (attr_iter);
  xobject_unref (link_iter);
}

/* Test cases {{{1 */
static void
test_equality (void)
{
  xrand_t *randa, *randb;
  xuint32_t seed;
  xint_t i;

  seed = g_test_rand_int ();

  randa = g_rand_new_with_seed (seed);
  randb = g_rand_new_with_seed (seed);

  for (i = 0; i < 500; i++)
    {
      RandomMenu *a, *b;

      a = random_menu_new (randa, TOP_ORDER);
      b = random_menu_new (randb, TOP_ORDER);
      assert_menus_equal (XMENU_MODEL (a), XMENU_MODEL (b));
      xobject_unref (b);
      xobject_unref (a);
    }

  g_rand_int (randa);

  for (i = 0; i < 500;)
    {
      RandomMenu *a, *b;

      a = random_menu_new (randa, TOP_ORDER);
      b = random_menu_new (randb, TOP_ORDER);
      if (check_menus_equal (XMENU_MODEL (a), XMENU_MODEL (b)))
        {
          /* by chance, they may really be equal.  double check. */
          xstring_t *as, *bs;

          as = xmenu_markup_print_string (NULL, XMENU_MODEL (a), 4, 2);
          bs = xmenu_markup_print_string (NULL, XMENU_MODEL (b), 4, 2);
          g_assert_cmpstr (as->str, ==, bs->str);
          xstring_free (bs, TRUE);
          xstring_free (as, TRUE);

          /* we're here because randa and randb just generated equal
           * menus.  they may do it again, so throw away randb and make
           * a fresh one.
           */
          g_rand_free (randb);
          randb = g_rand_new_with_seed (g_rand_int (randa));
        }
      else
        /* make sure we get enough unequals (ie: no xrand_t failure) */
        i++;

      xobject_unref (b);
      xobject_unref (a);
    }

  g_rand_free (randb);
  g_rand_free (randa);
}

static void
test_random (void)
{
  RandomMenu *random;
  MirrorMenu *mirror;
  xrand_t *rand;
  xint_t i;

  rand = g_rand_new_with_seed (g_test_rand_int ());
  random = random_menu_new (rand, TOP_ORDER);
  mirror = mirror_menu_new (XMENU_MODEL (random));

  for (i = 0; i < 500; i++)
    {
      assert_menus_equal (XMENU_MODEL (random), XMENU_MODEL (mirror));
      random_menu_change (random, rand);
    }

  xobject_unref (mirror);
  xobject_unref (random);

  g_rand_free (rand);
}

typedef struct
{
  xdbus_connection_t *client_connection;
  xdbus_connection_t *server_connection;
  xdbus_server_t *server;

  xthread_t *service_thread;
  /* Protects server_connection and service_loop. */
  xmutex_t service_loop_lock;
  xcond_t service_loop_cond;

  xmain_loop_t *service_loop;
} PeerConnection;

static xboolean_t
on_new_connection (xdbus_server_t *server,
                   xdbus_connection_t *connection,
                   xpointer_t user_data)
{
  PeerConnection *data = user_data;

  g_mutex_lock (&data->service_loop_lock);
  data->server_connection = xobject_ref (connection);
  g_cond_broadcast (&data->service_loop_cond);
  g_mutex_unlock (&data->service_loop_lock);

  return TRUE;
}

static void
create_service_loop (xmain_context_t   *service_context,
                     PeerConnection *data)
{
  g_assert (data->service_loop == NULL);
  g_mutex_lock (&data->service_loop_lock);
  data->service_loop = xmain_loop_new (service_context, FALSE);
  g_cond_broadcast (&data->service_loop_cond);
  g_mutex_unlock (&data->service_loop_lock);
}

static void
teardown_service_loop (PeerConnection *data)
{
  g_mutex_lock (&data->service_loop_lock);
  g_clear_pointer (&data->service_loop, xmain_loop_unref);
  g_mutex_unlock (&data->service_loop_lock);
}

static void
await_service_loop (PeerConnection *data)
{
  g_mutex_lock (&data->service_loop_lock);
  while (data->service_loop == NULL)
    g_cond_wait (&data->service_loop_cond, &data->service_loop_lock);
  g_mutex_unlock (&data->service_loop_lock);
}

static void
await_server_connection (PeerConnection *data)
{
  g_mutex_lock (&data->service_loop_lock);
  while (data->server_connection == NULL)
    g_cond_wait (&data->service_loop_cond, &data->service_loop_lock);
  g_mutex_unlock (&data->service_loop_lock);
}

static xpointer_t
service_thread_func (xpointer_t user_data)
{
  PeerConnection *data = user_data;
  xmain_context_t *service_context;
  xerror_t *error;
  xchar_t *address;
  xchar_t *tmpdir;
  GDBusServerFlags flags;
  xchar_t *guid;

  service_context = xmain_context_new ();
  xmain_context_push_thread_default (service_context);

  tmpdir = NULL;
  flags = G_DBUS_SERVER_FLAGS_NONE;

#ifdef G_OS_UNIX
  if (g_unix_socket_address_abstract_names_supported ())
    address = xstrdup ("unix:tmpdir=/tmp/test-dbus-peer");
  else
    {
      tmpdir = g_dir_make_tmp ("test-dbus-peer-XXXXXX", NULL);
      address = xstrdup_printf ("unix:tmpdir=%s", tmpdir);
    }
#else
  address = xstrdup ("nonce-tcp:");
  flags |= G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;
#endif

  guid = g_dbus_generate_guid ();

  error = NULL;
  data->server = g_dbus_server_new_sync (address,
                                         flags,
                                         guid,
                                         NULL,
                                         NULL,
                                         &error);
  g_assert_no_error (error);
  g_free (address);
  g_free (guid);

  g_signal_connect (data->server,
                    "new-connection",
                    G_CALLBACK (on_new_connection),
                    data);

  g_dbus_server_start (data->server);

  create_service_loop (service_context, data);
  xmain_loop_run (data->service_loop);

  xmain_context_pop_thread_default (service_context);

  teardown_service_loop (data);
  xmain_context_unref (service_context);

  if (tmpdir)
    {
      g_rmdir (tmpdir);
      g_free (tmpdir);
    }

  return NULL;
}

static void
peer_connection_up (PeerConnection *data)
{
  xerror_t *error;

  memset (data, '\0', sizeof (PeerConnection));

  g_mutex_init (&data->service_loop_lock);
  g_cond_init (&data->service_loop_cond);

  /* bring up a server - we run the server in a different thread to
     avoid deadlocks */
  data->service_thread = xthread_new ("test_dbus_peer",
                                       service_thread_func,
                                       data);
  await_service_loop (data);
  g_assert (data->server != NULL);

  /* bring up a connection and accept it */
  error = NULL;
  data->client_connection =
    g_dbus_connection_new_for_address_sync (g_dbus_server_get_client_address (data->server),
                                            G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                            NULL, /* xdbus_auth_observer_t */
                                            NULL, /* cancellable */
                                            &error);
  g_assert_no_error (error);
  g_assert (data->client_connection != NULL);
  await_server_connection (data);
}

static void
peer_connection_down (PeerConnection *data)
{
  xobject_unref (data->client_connection);
  xobject_unref (data->server_connection);

  g_dbus_server_stop (data->server);
  xobject_unref (data->server);

  xmain_loop_quit (data->service_loop);
  xthread_join (data->service_thread);

  g_mutex_clear (&data->service_loop_lock);
  g_cond_clear (&data->service_loop_cond);
}

struct roundtrip_state
{
  RandomMenu *random;
  MirrorMenu *proxy_mirror;
  xdbus_menu_model_t *proxy;
  xmain_loop_t *loop;
  xrand_t *rand;
  xint_t success;
  xint_t count;
};

static xboolean_t
roundtrip_step (xpointer_t data)
{
  struct roundtrip_state *state = data;

  if (check_menus_equal (XMENU_MODEL (state->random), XMENU_MODEL (state->proxy)) &&
      check_menus_equal (XMENU_MODEL (state->random), XMENU_MODEL (state->proxy_mirror)))
    {
      state->success++;
      state->count = 0;

      if (state->success < 100)
        random_menu_change (state->random, state->rand);
      else
        xmain_loop_quit (state->loop);
    }
  else if (state->count == 100)
    {
      assert_menus_equal (XMENU_MODEL (state->random), XMENU_MODEL (state->proxy));
      g_assert_not_reached ();
    }
  else
    state->count++;

  return G_SOURCE_CONTINUE;
}

static void
do_roundtrip (xdbus_connection_t *exporter_connection,
              xdbus_connection_t *proxy_connection)
{
  struct roundtrip_state state;
  xuint_t export_id;
  xuint_t id;

  state.rand = g_rand_new_with_seed (g_test_rand_int ());

  state.random = random_menu_new (state.rand, 2);
  export_id = g_dbus_connection_export_menu_model (exporter_connection,
                                                   "/",
                                                   XMENU_MODEL (state.random),
                                                   NULL);
  state.proxy = g_dbus_menu_model_get (proxy_connection,
                                       g_dbus_connection_get_unique_name (proxy_connection),
                                       "/");
  state.proxy_mirror = mirror_menu_new (XMENU_MODEL (state.proxy));
  state.count = 0;
  state.success = 0;

  id = g_timeout_add (10, roundtrip_step, &state);

  state.loop = xmain_loop_new (NULL, FALSE);
  xmain_loop_run (state.loop);

  xmain_loop_unref (state.loop);
  xsource_remove (id);
  xobject_unref (state.proxy);
  g_dbus_connection_unexport_menu_model (exporter_connection, export_id);
  xobject_unref (state.random);
  xobject_unref (state.proxy_mirror);
  g_rand_free (state.rand);
}

static void
test_dbus_roundtrip (void)
{
  xdbus_connection_t *bus;

  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  do_roundtrip (bus, bus);
  xobject_unref (bus);
}

static void
test_dbus_peer_roundtrip (void)
{
#ifdef _XPL_ADDRESS_SANITIZER
  g_test_incomplete ("FIXME: Leaks a GCancellableSource, see glib#2313");
  (void) peer_connection_up;
  (void) peer_connection_down;
#else
  PeerConnection peer;

  peer_connection_up (&peer);
  do_roundtrip (peer.server_connection, peer.client_connection);
  peer_connection_down (&peer);
#endif
}

static xint_t items_changed_count;

static void
items_changed (xmenu_model_t *model,
               xint_t        position,
               xint_t        removed,
               xint_t        added,
               xpointer_t    data)
{
  items_changed_count++;
}

static xboolean_t
stop_loop (xpointer_t data)
{
  xmain_loop_t *loop = data;

  xmain_loop_quit (loop);

  return G_SOURCE_REMOVE;
}

static void
do_subscriptions (xdbus_connection_t *exporter_connection,
                  xdbus_connection_t *proxy_connection)
{
  xmenu_t *menu;
  xdbus_menu_model_t *proxy;
  xmain_loop_t *loop;
  xerror_t *error = NULL;
  xuint_t export_id;
  xuint_t timeout_id;

  timeout_id = add_timeout (60);
  loop = xmain_loop_new (NULL, FALSE);

  menu = xmenu_new ();

  export_id = g_dbus_connection_export_menu_model (exporter_connection,
                                                   "/",
                                                   XMENU_MODEL (menu),
                                                   &error);
  g_assert_no_error (error);

  proxy = g_dbus_menu_model_get (proxy_connection,
                                 g_dbus_connection_get_unique_name (proxy_connection),
                                 "/");
  items_changed_count = 0;
  g_signal_connect (proxy, "items-changed",
                    G_CALLBACK (items_changed), NULL);

  xmenu_append (menu, "item1", NULL);
  xmenu_append (menu, "item2", NULL);
  xmenu_append (menu, "item3", NULL);

  g_assert_cmpint (items_changed_count, ==, 0);

  /* We don't subscribe to change-notification until we look at the items */
  g_timeout_add (100, stop_loop, loop);
  xmain_loop_run (loop);

  /* Looking at the items triggers subscription */
  xmenu_model_get_n_items (XMENU_MODEL (proxy));

  while (items_changed_count < 1)
    xmain_context_iteration (NULL, TRUE);

  /* We get all three items in one batch */
  g_assert_cmpint (items_changed_count, ==, 1);
  g_assert_cmpint (xmenu_model_get_n_items (XMENU_MODEL (proxy)), ==, 3);

  /* If we wait, we don't get any more */
  g_timeout_add (100, stop_loop, loop);
  xmain_loop_run (loop);
  g_assert_cmpint (items_changed_count, ==, 1);
  g_assert_cmpint (xmenu_model_get_n_items (XMENU_MODEL (proxy)), ==, 3);

  /* Now we're subscribed, we get changes individually */
  xmenu_append (menu, "item4", NULL);
  xmenu_append (menu, "item5", NULL);
  xmenu_append (menu, "item6", NULL);
  xmenu_remove (menu, 0);
  xmenu_remove (menu, 0);

  while (items_changed_count < 6)
    xmain_context_iteration (NULL, TRUE);

  g_assert_cmpint (items_changed_count, ==, 6);

  g_assert_cmpint (xmenu_model_get_n_items (XMENU_MODEL (proxy)), ==, 4);

  /* After destroying the proxy and waiting a bit, we don't get any more
   * items-changed signals */
  xobject_unref (proxy);

  g_timeout_add (100, stop_loop, loop);
  xmain_loop_run (loop);

  xmenu_remove (menu, 0);
  xmenu_remove (menu, 0);

  g_timeout_add (100, stop_loop, loop);
  xmain_loop_run (loop);

  g_assert_cmpint (items_changed_count, ==, 6);

  g_dbus_connection_unexport_menu_model (exporter_connection, export_id);
  xobject_unref (menu);

  xmain_loop_unref (loop);
  cancel_timeout (timeout_id);
}

static void
test_dbus_subscriptions (void)
{
  xdbus_connection_t *bus;

  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  do_subscriptions (bus, bus);
  xobject_unref (bus);
}

static void
test_dbus_peer_subscriptions (void)
{
#ifdef _XPL_ADDRESS_SANITIZER
  g_test_incomplete ("FIXME: Leaks a GCancellableSource, see glib#2313");
  (void) peer_connection_up;
  (void) peer_connection_down;
#else
  PeerConnection peer;

  peer_connection_up (&peer);
  do_subscriptions (peer.server_connection, peer.client_connection);
  peer_connection_down (&peer);
#endif
}

static xpointer_t
do_modify (xpointer_t data)
{
  RandomMenu *menu = data;
  xrand_t *rand;
  xint_t i;

  rand = g_rand_new_with_seed (g_test_rand_int ());

  for (i = 0; i < 10000; i++)
    {
      random_menu_change (menu, rand);
    }

  g_rand_free (rand);

  return NULL;
}

static xpointer_t
do_export (xpointer_t data)
{
  xmenu_model_t *menu = data;
  xint_t i;
  xdbus_connection_t *bus;
  xchar_t *path;
  xerror_t *error = NULL;
  xuint_t id;

  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  path = xstrdup_printf ("/%p", data);

  for (i = 0; i < 10000; i++)
    {
      id = g_dbus_connection_export_menu_model (bus, path, menu, &error);
      g_assert_no_error (error);
      g_dbus_connection_unexport_menu_model (bus, id);
      while (xmain_context_iteration (NULL, FALSE));
    }

  g_free (path);

  xobject_unref (bus);

  return NULL;
}

static void
test_dbus_threaded (void)
{
  RandomMenu *menu[10];
  xthread_t *call[10];
  xthread_t *export[10];
  xint_t i;

  for (i = 0; i < 10; i++)
    {
      xrand_t *rand = g_rand_new_with_seed (g_test_rand_int ());
      menu[i] = random_menu_new (rand, 2);
      call[i] = xthread_new ("call", do_modify, menu[i]);
      export[i] = xthread_new ("export", do_export, menu[i]);
      g_rand_free (rand);
    }

  for (i = 0; i < 10; i++)
    {
      xthread_join (call[i]);
      xthread_join (export[i]);
    }

  for (i = 0; i < 10; i++)
    xobject_unref (menu[i]);
}

static void
test_attributes (void)
{
  xmenu_t *menu;
  xmenu_item_t *item;
  xvariant_t *v;

  menu = xmenu_new ();

  item = xmenu_item_new ("test", NULL);
  xmenu_item_set_attribute_value (item, "boolean", xvariant_new_boolean (FALSE));
  xmenu_item_set_attribute_value (item, "string", xvariant_new_string ("bla"));

  xmenu_item_set_attribute (item, "double", "d", 1.5);
  v = xvariant_new_parsed ("[('one', 1), ('two', %i), (%s, 3)]", 2, "three");
  xmenu_item_set_attribute_value (item, "complex", v);
  xmenu_item_set_attribute_value (item, "test-123", xvariant_new_string ("test-123"));

  xmenu_append_item (menu, item);

  xmenu_item_set_attribute (item, "double", "d", G_PI);

  g_assert_cmpint (xmenu_model_get_n_items (XMENU_MODEL (menu)), ==, 1);

  v = xmenu_model_get_item_attribute_value (XMENU_MODEL (menu), 0, "boolean", NULL);
  g_assert (xvariant_is_of_type (v, G_VARIANT_TYPE_BOOLEAN));
  xvariant_unref (v);

  v = xmenu_model_get_item_attribute_value (XMENU_MODEL (menu), 0, "string", NULL);
  g_assert (xvariant_is_of_type (v, G_VARIANT_TYPE_STRING));
  xvariant_unref (v);

  v = xmenu_model_get_item_attribute_value (XMENU_MODEL (menu), 0, "double", NULL);
  g_assert (xvariant_is_of_type (v, G_VARIANT_TYPE_DOUBLE));
  xvariant_unref (v);

  v = xmenu_model_get_item_attribute_value (XMENU_MODEL (menu), 0, "complex", NULL);
  g_assert (xvariant_is_of_type (v, G_VARIANT_TYPE("a(si)")));
  xvariant_unref (v);

  xmenu_remove_all (menu);

  xobject_unref (menu);
  xobject_unref (item);
}

static void
test_attribute_iter (void)
{
  xmenu_t *menu;
  xmenu_item_t *item;
  const xchar_t *name;
  xvariant_t *v;
  xmenu_attribute_iter_t *iter;
  xhashtable_t *found;

  menu = xmenu_new ();

  item = xmenu_item_new ("test", NULL);
  xmenu_item_set_attribute_value (item, "boolean", xvariant_new_boolean (FALSE));
  xmenu_item_set_attribute_value (item, "string", xvariant_new_string ("bla"));

  xmenu_item_set_attribute (item, "double", "d", 1.5);
  v = xvariant_new_parsed ("[('one', 1), ('two', %i), (%s, 3)]", 2, "three");
  xmenu_item_set_attribute_value (item, "complex", v);
  xmenu_item_set_attribute_value (item, "test-123", xvariant_new_string ("test-123"));

  xmenu_append_item (menu, item);

  xmenu_item_set_attribute (item, "double", "d", G_PI);

  g_assert_cmpint (xmenu_model_get_n_items (XMENU_MODEL (menu)), ==, 1);

  found = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t)xvariant_unref);

  iter = xmenu_model_iterate_item_attributes (XMENU_MODEL (menu), 0);
  while (xmenu_attribute_iter_get_next (iter, &name, &v))
    xhash_table_insert (found, xstrdup (name), v);

  g_assert_cmpint (xhash_table_size (found), ==, 6);

  v = xhash_table_lookup (found, "label");
  g_assert (xvariant_is_of_type (v, G_VARIANT_TYPE_STRING));

  v = xhash_table_lookup (found, "boolean");
  g_assert (xvariant_is_of_type (v, G_VARIANT_TYPE_BOOLEAN));

  v = xhash_table_lookup (found, "string");
  g_assert (xvariant_is_of_type (v, G_VARIANT_TYPE_STRING));

  v = xhash_table_lookup (found, "double");
  g_assert (xvariant_is_of_type (v, G_VARIANT_TYPE_DOUBLE));

  v = xhash_table_lookup (found, "complex");
  g_assert (xvariant_is_of_type (v, G_VARIANT_TYPE("a(si)")));

  v = xhash_table_lookup (found, "test-123");
  g_assert (xvariant_is_of_type (v, G_VARIANT_TYPE_STRING));

  xhash_table_unref (found);

  xmenu_remove_all (menu);

  xobject_unref (menu);
  xobject_unref (item);
}

static void
test_links (void)
{
  xmenu_t *menu;
  xmenu_model_t *m;
  xmenu_model_t *x;
  xmenu_item_t *item;

  m = XMENU_MODEL (xmenu_new ());
  xmenu_append (XMENU (m), "test", NULL);

  menu = xmenu_new ();

  item = xmenu_item_new ("test2", NULL);
  xmenu_item_set_link (item, "submenu", m);
  xmenu_prepend_item (menu, item);

  item = xmenu_item_new ("test1", NULL);
  xmenu_item_set_link (item, "section", m);
  xmenu_insert_item (menu, 0, item);

  item = xmenu_item_new ("test3", NULL);
  xmenu_item_set_link (item, "wallet", m);
  xmenu_insert_item (menu, 1000, item);

  item = xmenu_item_new ("test4", NULL);
  xmenu_item_set_link (item, "purse", m);
  xmenu_item_set_link (item, "purse", NULL);
  xmenu_append_item (menu, item);

  g_assert_cmpint (xmenu_model_get_n_items (XMENU_MODEL (menu)), ==, 4);

  x = xmenu_model_get_item_link (XMENU_MODEL (menu), 0, "section");
  g_assert (x == m);
  xobject_unref (x);

  x = xmenu_model_get_item_link (XMENU_MODEL (menu), 1, "submenu");
  g_assert (x == m);
  xobject_unref (x);

  x = xmenu_model_get_item_link (XMENU_MODEL (menu), 2, "wallet");
  g_assert (x == m);
  xobject_unref (x);

  x = xmenu_model_get_item_link (XMENU_MODEL (menu), 3, "purse");
  g_assert (x == NULL);

  xobject_unref (m);
  xobject_unref (menu);
}

static void
test_mutable (void)
{
  xmenu_t *menu;

  menu = xmenu_new ();
  xmenu_append (menu, "test", "test");

  g_assert (xmenu_model_is_mutable (XMENU_MODEL (menu)));
  xmenu_freeze (menu);
  g_assert (!xmenu_model_is_mutable (XMENU_MODEL (menu)));

  xobject_unref (menu);
}

static void
test_convenience (void)
{
  xmenu_t *m1, *m2;
  xmenu_t *sub;
  xmenu_item_t *item;

  m1 = xmenu_new ();
  m2 = xmenu_new ();
  sub = xmenu_new ();

  xmenu_prepend (m1, "label1", "do::something");
  xmenu_insert (m2, 0, "label1", "do::something");

  xmenu_append (m1, "label2", "do::somethingelse");
  xmenu_insert (m2, -1, "label2", "do::somethingelse");

  xmenu_insert_section (m1, 10, "label3", XMENU_MODEL (sub));
  item = xmenu_item_new_section ("label3", XMENU_MODEL (sub));
  xmenu_insert_item (m2, 10, item);
  xobject_unref (item);

  xmenu_prepend_section (m1, "label4", XMENU_MODEL (sub));
  xmenu_insert_section (m2, 0, "label4", XMENU_MODEL (sub));

  xmenu_append_section (m1, "label5", XMENU_MODEL (sub));
  xmenu_insert_section (m2, -1, "label5", XMENU_MODEL (sub));

  xmenu_insert_submenu (m1, 5, "label6", XMENU_MODEL (sub));
  item = xmenu_item_new_submenu ("label6", XMENU_MODEL (sub));
  xmenu_insert_item (m2, 5, item);
  xobject_unref (item);

  xmenu_prepend_submenu (m1, "label7", XMENU_MODEL (sub));
  xmenu_insert_submenu (m2, 0, "label7", XMENU_MODEL (sub));

  xmenu_append_submenu (m1, "label8", XMENU_MODEL (sub));
  xmenu_insert_submenu (m2, -1, "label8", XMENU_MODEL (sub));

  assert_menus_equal (XMENU_MODEL (m1), XMENU_MODEL (m2));

  xobject_unref (m1);
  xobject_unref (m2);
}

static void
test_menuitem (void)
{
  xmenu_t *menu;
  xmenu_t *submenu;
  xmenu_item_t *item;
  xicon_t *icon;
  xboolean_t b;
  xchar_t *s;

  menu = xmenu_new ();
  submenu = xmenu_new ();

  item = xmenu_item_new ("label", "action");
  xmenu_item_set_attribute (item, "attribute", "b", TRUE);
  xmenu_item_set_link (item, XMENU_LINK_SUBMENU, XMENU_MODEL (submenu));
  xmenu_append_item (menu, item);

  icon = g_themed_icon_new ("bla");
  xmenu_item_set_icon (item, icon);
  xobject_unref (icon);

  g_assert (xmenu_item_get_attribute (item, "attribute", "b", &b));
  g_assert (b);

  xmenu_item_set_action_and_target (item, "action", "(bs)", TRUE, "string");
  g_assert (xmenu_item_get_attribute (item, "target", "(bs)", &b, &s));
  g_assert (b);
  g_assert_cmpstr (s, ==, "string");
  g_free (s);

  xobject_unref (item);

  item = xmenu_item_new_from_model (XMENU_MODEL (menu), 0);
  assert_menuitem_equal (item, XMENU_MODEL (menu), 0);
  xobject_unref (item);

  xobject_unref (menu);
  xobject_unref (submenu);
}

/* Epilogue {{{1 */
int
main (int argc, char **argv)
{
  xboolean_t ret;

  g_test_init (&argc, &argv, NULL);

  session_bus_up ();

  g_test_add_func ("/gmenu/equality", test_equality);
  g_test_add_func ("/gmenu/random", test_random);
  g_test_add_func ("/gmenu/dbus/roundtrip", test_dbus_roundtrip);
  g_test_add_func ("/gmenu/dbus/subscriptions", test_dbus_subscriptions);
  g_test_add_func ("/gmenu/dbus/threaded", test_dbus_threaded);
  g_test_add_func ("/gmenu/dbus/peer/roundtrip", test_dbus_peer_roundtrip);
  g_test_add_func ("/gmenu/dbus/peer/subscriptions", test_dbus_peer_subscriptions);
  g_test_add_func ("/gmenu/attributes", test_attributes);
  g_test_add_func ("/gmenu/attributes/iterate", test_attribute_iter);
  g_test_add_func ("/gmenu/links", test_links);
  g_test_add_func ("/gmenu/mutable", test_mutable);
  g_test_add_func ("/gmenu/convenience", test_convenience);
  g_test_add_func ("/gmenu/menuitem", test_menuitem);

  ret = g_test_run ();

  session_bus_down ();

  return ret;
}
/* vim:set foldmethod=marker: */
