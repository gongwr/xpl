/*
 * Copyright © 2011 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gmenu.h"

#include "gaction.h"
#include <string.h>

#include "gicon.h"

/**
 * SECTION:gmenu
 * @title: xmenu_t
 * @short_description: A simple implementation of xmenu_model_t
 * @include: gio/gio.h
 *
 * #xmenu_t is a simple implementation of #xmenu_model_t.
 * You populate a #xmenu_t by adding #xmenu_item_t instances to it.
 *
 * There are some convenience functions to allow you to directly
 * add items (avoiding #xmenu_item_t) for the common cases. To add
 * a regular item, use xmenu_insert(). To add a section, use
 * xmenu_insert_section(). To add a submenu, use
 * xmenu_insert_submenu().
 */

/**
 * xmenu_t:
 *
 * #xmenu_t is an opaque structure type.  You must access it using the
 * functions below.
 *
 * Since: 2.32
 */

/**
 * xmenu_item_t:
 *
 * #xmenu_item_t is an opaque structure type.  You must access it using the
 * functions below.
 *
 * Since: 2.32
 */

struct _GMenuItem
{
  xobject_t parent_instance;

  xhashtable_t *attributes;
  xhashtable_t *links;
  xboolean_t    cow;
};

typedef xobject_class_t xmenu_item_class_t;

struct _GMenu
{
  xmenu_model_t parent_instance;

  xarray_t   *items;
  xboolean_t  mutable;
};

typedef xmenu_model_class_t xmenu_class_t;

XDEFINE_TYPE (xmenu, xmenu, XTYPE_MENU_MODEL)
XDEFINE_TYPE (xmenu_item, xmenu_item, XTYPE_OBJECT)

struct item
{
  xhashtable_t *attributes;
  xhashtable_t *links;
};

static xboolean_t
xmenu_is_mutable (xmenu_model_t *model)
{
  xmenu_t *menu = XMENU (model);

  return menu->mutable;
}

static xint_t
xmenu_get_n_items (xmenu_model_t *model)
{
  xmenu_t *menu = XMENU (model);

  return menu->items->len;
}

static void
xmenu_get_item_attributes (xmenu_model_t  *model,
                            xint_t         position,
                            xhashtable_t **table)
{
  xmenu_t *menu = XMENU (model);

  *table = xhash_table_ref (g_array_index (menu->items, struct item, position).attributes);
}

static void
xmenu_get_item_links (xmenu_model_t  *model,
                       xint_t         position,
                       xhashtable_t **table)
{
  xmenu_t *menu = XMENU (model);

  *table = xhash_table_ref (g_array_index (menu->items, struct item, position).links);
}

/**
 * xmenu_insert_item:
 * @menu: a #xmenu_t
 * @position: the position at which to insert the item
 * @item: the #xmenu_item_t to insert
 *
 * Inserts @item into @menu.
 *
 * The "insertion" is actually done by copying all of the attribute and
 * link values of @item and using them to form a new item within @menu.
 * As such, @item itself is not really inserted, but rather, a menu item
 * that is exactly the same as the one presently described by @item.
 *
 * This means that @item is essentially useless after the insertion
 * occurs.  Any changes you make to it are ignored unless it is inserted
 * again (at which point its updated values will be copied).
 *
 * You should probably just free @item once you're done.
 *
 * There are many convenience functions to take care of common cases.
 * See xmenu_insert(), xmenu_insert_section() and
 * xmenu_insert_submenu() as well as "prepend" and "append" variants of
 * each of these functions.
 *
 * Since: 2.32
 */
void
xmenu_insert_item (xmenu_t     *menu,
                    xint_t       position,
                    xmenu_item_t *item)
{
  struct item new_item;

  g_return_if_fail (X_IS_MENU (menu));
  g_return_if_fail (X_IS_MENU_ITEM (item));

  if (position < 0 || (xuint_t) position > menu->items->len)
    position = menu->items->len;

  new_item.attributes = xhash_table_ref (item->attributes);
  new_item.links = xhash_table_ref (item->links);
  item->cow = TRUE;

  g_array_insert_val (menu->items, position, new_item);
  xmenu_model_items_changed (XMENU_MODEL (menu), position, 0, 1);
}

/**
 * xmenu_prepend_item:
 * @menu: a #xmenu_t
 * @item: a #xmenu_item_t to prepend
 *
 * Prepends @item to the start of @menu.
 *
 * See xmenu_insert_item() for more information.
 *
 * Since: 2.32
 */
void
xmenu_prepend_item (xmenu_t     *menu,
                     xmenu_item_t *item)
{
  xmenu_insert_item (menu, 0, item);
}

/**
 * xmenu_append_item:
 * @menu: a #xmenu_t
 * @item: a #xmenu_item_t to append
 *
 * Appends @item to the end of @menu.
 *
 * See xmenu_insert_item() for more information.
 *
 * Since: 2.32
 */
void
xmenu_append_item (xmenu_t     *menu,
                    xmenu_item_t *item)
{
  xmenu_insert_item (menu, -1, item);
}

/**
 * xmenu_freeze:
 * @menu: a #xmenu_t
 *
 * Marks @menu as frozen.
 *
 * After the menu is frozen, it is an error to attempt to make any
 * changes to it.  In effect this means that the #xmenu_t API must no
 * longer be used.
 *
 * This function causes xmenu_model_is_mutable() to begin returning
 * %FALSE, which has some positive performance implications.
 *
 * Since: 2.32
 */
void
xmenu_freeze (xmenu_t *menu)
{
  g_return_if_fail (X_IS_MENU (menu));

  menu->mutable = FALSE;
}

/**
 * xmenu_new:
 *
 * Creates a new #xmenu_t.
 *
 * The new menu has no items.
 *
 * Returns: a new #xmenu_t
 *
 * Since: 2.32
 */
xmenu_t *
xmenu_new (void)
{
  return xobject_new (XTYPE_MENU, NULL);
}

/**
 * xmenu_insert:
 * @menu: a #xmenu_t
 * @position: the position at which to insert the item
 * @label: (nullable): the section label, or %NULL
 * @detailed_action: (nullable): the detailed action string, or %NULL
 *
 * Convenience function for inserting a normal menu item into @menu.
 * Combine xmenu_item_new() and xmenu_insert_item() for a more flexible
 * alternative.
 *
 * Since: 2.32
 */
void
xmenu_insert (xmenu_t       *menu,
               xint_t         position,
               const xchar_t *label,
               const xchar_t *detailed_action)
{
  xmenu_item_t *menu_item;

  menu_item = xmenu_item_new (label, detailed_action);
  xmenu_insert_item (menu, position, menu_item);
  xobject_unref (menu_item);
}

/**
 * xmenu_prepend:
 * @menu: a #xmenu_t
 * @label: (nullable): the section label, or %NULL
 * @detailed_action: (nullable): the detailed action string, or %NULL
 *
 * Convenience function for prepending a normal menu item to the start
 * of @menu.  Combine xmenu_item_new() and xmenu_insert_item() for a more
 * flexible alternative.
 *
 * Since: 2.32
 */
void
xmenu_prepend (xmenu_t       *menu,
                const xchar_t *label,
                const xchar_t *detailed_action)
{
  xmenu_insert (menu, 0, label, detailed_action);
}

/**
 * xmenu_append:
 * @menu: a #xmenu_t
 * @label: (nullable): the section label, or %NULL
 * @detailed_action: (nullable): the detailed action string, or %NULL
 *
 * Convenience function for appending a normal menu item to the end of
 * @menu.  Combine xmenu_item_new() and xmenu_insert_item() for a more
 * flexible alternative.
 *
 * Since: 2.32
 */
void
xmenu_append (xmenu_t       *menu,
               const xchar_t *label,
               const xchar_t *detailed_action)
{
  xmenu_insert (menu, -1, label, detailed_action);
}

/**
 * xmenu_insert_section:
 * @menu: a #xmenu_t
 * @position: the position at which to insert the item
 * @label: (nullable): the section label, or %NULL
 * @section: a #xmenu_model_t with the items of the section
 *
 * Convenience function for inserting a section menu item into @menu.
 * Combine xmenu_item_new_section() and xmenu_insert_item() for a more
 * flexible alternative.
 *
 * Since: 2.32
 */
void
xmenu_insert_section (xmenu_t       *menu,
                       xint_t         position,
                       const xchar_t *label,
                       xmenu_model_t  *section)
{
  xmenu_item_t *menu_item;

  menu_item = xmenu_item_new_section (label, section);
  xmenu_insert_item (menu, position, menu_item);
  xobject_unref (menu_item);
}


/**
 * xmenu_prepend_section:
 * @menu: a #xmenu_t
 * @label: (nullable): the section label, or %NULL
 * @section: a #xmenu_model_t with the items of the section
 *
 * Convenience function for prepending a section menu item to the start
 * of @menu.  Combine xmenu_item_new_section() and xmenu_insert_item() for
 * a more flexible alternative.
 *
 * Since: 2.32
 */
void
xmenu_prepend_section (xmenu_t       *menu,
                        const xchar_t *label,
                        xmenu_model_t  *section)
{
  xmenu_insert_section (menu, 0, label, section);
}

/**
 * xmenu_append_section:
 * @menu: a #xmenu_t
 * @label: (nullable): the section label, or %NULL
 * @section: a #xmenu_model_t with the items of the section
 *
 * Convenience function for appending a section menu item to the end of
 * @menu.  Combine xmenu_item_new_section() and xmenu_insert_item() for a
 * more flexible alternative.
 *
 * Since: 2.32
 */
void
xmenu_append_section (xmenu_t       *menu,
                       const xchar_t *label,
                       xmenu_model_t  *section)
{
  xmenu_insert_section (menu, -1, label, section);
}

/**
 * xmenu_insert_submenu:
 * @menu: a #xmenu_t
 * @position: the position at which to insert the item
 * @label: (nullable): the section label, or %NULL
 * @submenu: a #xmenu_model_t with the items of the submenu
 *
 * Convenience function for inserting a submenu menu item into @menu.
 * Combine xmenu_item_new_submenu() and xmenu_insert_item() for a more
 * flexible alternative.
 *
 * Since: 2.32
 */
void
xmenu_insert_submenu (xmenu_t       *menu,
                       xint_t         position,
                       const xchar_t *label,
                       xmenu_model_t  *submenu)
{
  xmenu_item_t *menu_item;

  menu_item = xmenu_item_new_submenu (label, submenu);
  xmenu_insert_item (menu, position, menu_item);
  xobject_unref (menu_item);
}

/**
 * xmenu_prepend_submenu:
 * @menu: a #xmenu_t
 * @label: (nullable): the section label, or %NULL
 * @submenu: a #xmenu_model_t with the items of the submenu
 *
 * Convenience function for prepending a submenu menu item to the start
 * of @menu.  Combine xmenu_item_new_submenu() and xmenu_insert_item() for
 * a more flexible alternative.
 *
 * Since: 2.32
 */
void
xmenu_prepend_submenu (xmenu_t       *menu,
                        const xchar_t *label,
                        xmenu_model_t  *submenu)
{
  xmenu_insert_submenu (menu, 0, label, submenu);
}

/**
 * xmenu_append_submenu:
 * @menu: a #xmenu_t
 * @label: (nullable): the section label, or %NULL
 * @submenu: a #xmenu_model_t with the items of the submenu
 *
 * Convenience function for appending a submenu menu item to the end of
 * @menu.  Combine xmenu_item_new_submenu() and xmenu_insert_item() for a
 * more flexible alternative.
 *
 * Since: 2.32
 */
void
xmenu_append_submenu (xmenu_t       *menu,
                       const xchar_t *label,
                       xmenu_model_t  *submenu)
{
  xmenu_insert_submenu (menu, -1, label, submenu);
}

static void
xmenu_clear_item (struct item *item)
{
  if (item->attributes != NULL)
    xhash_table_unref (item->attributes);
  if (item->links != NULL)
    xhash_table_unref (item->links);
}

/**
 * xmenu_remove:
 * @menu: a #xmenu_t
 * @position: the position of the item to remove
 *
 * Removes an item from the menu.
 *
 * @position gives the index of the item to remove.
 *
 * It is an error if position is not in range the range from 0 to one
 * less than the number of items in the menu.
 *
 * It is not possible to remove items by identity since items are added
 * to the menu simply by copying their links and attributes (ie:
 * identity of the item itself is not preserved).
 *
 * Since: 2.32
 */
void
xmenu_remove (xmenu_t *menu,
               xint_t   position)
{
  g_return_if_fail (X_IS_MENU (menu));
  g_return_if_fail (0 <= position && (xuint_t) position < menu->items->len);

  xmenu_clear_item (&g_array_index (menu->items, struct item, position));
  g_array_remove_index (menu->items, position);
  xmenu_model_items_changed (XMENU_MODEL (menu), position, 1, 0);
}

/**
 * xmenu_remove_all:
 * @menu: a #xmenu_t
 *
 * Removes all items in the menu.
 *
 * Since: 2.38
 **/
void
xmenu_remove_all (xmenu_t *menu)
{
  xint_t i, n;

  g_return_if_fail (X_IS_MENU (menu));
  n = menu->items->len;

  for (i = 0; i < n; i++)
    xmenu_clear_item (&g_array_index (menu->items, struct item, i));
  g_array_set_size (menu->items, 0);

  xmenu_model_items_changed (XMENU_MODEL (menu), 0, n, 0);
}

static void
xmenu_finalize (xobject_t *object)
{
  xmenu_t *menu = XMENU (object);
  struct item *items;
  xint_t n_items;
  xint_t i;

  n_items = menu->items->len;
  items = (struct item *) g_array_free (menu->items, FALSE);
  for (i = 0; i < n_items; i++)
    xmenu_clear_item (&items[i]);
  g_free (items);

  XOBJECT_CLASS (xmenu_parent_class)
    ->finalize (object);
}

static void
xmenu_init (xmenu_t *menu)
{
  menu->items = g_array_new (FALSE, FALSE, sizeof (struct item));
  menu->mutable = TRUE;
}

static void
xmenu_class_init (xmenu_class_t *class)
{
  xmenu_model_class_t *model_class = XMENU_MODEL_CLASS (class);
  xobject_class_t *object_class = XOBJECT_CLASS (class);

  object_class->finalize = xmenu_finalize;

  model_class->is_mutable = xmenu_is_mutable;
  model_class->get_n_items = xmenu_get_n_items;
  model_class->get_item_attributes = xmenu_get_item_attributes;
  model_class->get_item_links = xmenu_get_item_links;
}


static void
xmenu_item_clear_cow (xmenu_item_t *menu_item)
{
  if (menu_item->cow)
    {
      xhash_table_iter_t iter;
      xhashtable_t *new;
      xpointer_t key;
      xpointer_t val;

      new = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t) xvariant_unref);
      xhash_table_iter_init (&iter, menu_item->attributes);
      while (xhash_table_iter_next (&iter, &key, &val))
        xhash_table_insert (new, xstrdup (key), xvariant_ref (val));
      xhash_table_unref (menu_item->attributes);
      menu_item->attributes = new;

      new = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t) xobject_unref);
      xhash_table_iter_init (&iter, menu_item->links);
      while (xhash_table_iter_next (&iter, &key, &val))
        xhash_table_insert (new, xstrdup (key), xobject_ref (val));
      xhash_table_unref (menu_item->links);
      menu_item->links = new;

      menu_item->cow = FALSE;
    }
}

static void
xmenu_item_finalize (xobject_t *object)
{
  xmenu_item_t *menu_item = XMENU_ITEM (object);

  xhash_table_unref (menu_item->attributes);
  xhash_table_unref (menu_item->links);

  XOBJECT_CLASS (xmenu_item_parent_class)
    ->finalize (object);
}

static void
xmenu_item_init (xmenu_item_t *menu_item)
{
  menu_item->attributes = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t) xvariant_unref);
  menu_item->links = xhash_table_new_full (xstr_hash, xstr_equal, g_free, xobject_unref);
  menu_item->cow = FALSE;
}

static void
xmenu_item_class_init (xmenu_item_class_t *class)
{
  class->finalize = xmenu_item_finalize;
}

/* We treat attribute names the same as xsettings_t keys:
 * - only lowercase ascii, digits and '-'
 * - must start with lowercase
 * - must not end with '-'
 * - no consecutive '-'
 * - not longer than 1024 chars
 */
static xboolean_t
valid_attribute_name (const xchar_t *name)
{
  xint_t i;

  if (!g_ascii_islower (name[0]))
    return FALSE;

  for (i = 1; name[i]; i++)
    {
      if (name[i] != '-' &&
          !g_ascii_islower (name[i]) &&
          !g_ascii_isdigit (name[i]))
        return FALSE;

      if (name[i] == '-' && name[i + 1] == '-')
        return FALSE;
    }

  if (name[i - 1] == '-')
    return FALSE;

  if (i > 1024)
    return FALSE;

  return TRUE;
}

/**
 * xmenu_item_set_attribute_value:
 * @menu_item: a #xmenu_item_t
 * @attribute: the attribute to set
 * @value: (nullable): a #xvariant_t to use as the value, or %NULL
 *
 * Sets or unsets an attribute on @menu_item.
 *
 * The attribute to set or unset is specified by @attribute. This
 * can be one of the standard attribute names %XMENU_ATTRIBUTE_LABEL,
 * %XMENU_ATTRIBUTE_ACTION, %XMENU_ATTRIBUTE_TARGET, or a custom
 * attribute name.
 * Attribute names are restricted to lowercase characters, numbers
 * and '-'. Furthermore, the names must begin with a lowercase character,
 * must not end with a '-', and must not contain consecutive dashes.
 *
 * must consist only of lowercase
 * ASCII characters, digits and '-'.
 *
 * If @value is non-%NULL then it is used as the new value for the
 * attribute.  If @value is %NULL then the attribute is unset. If
 * the @value #xvariant_t is floating, it is consumed.
 *
 * See also xmenu_item_set_attribute() for a more convenient way to do
 * the same.
 *
 * Since: 2.32
 */
void
xmenu_item_set_attribute_value (xmenu_item_t   *menu_item,
                                 const xchar_t *attribute,
                                 xvariant_t    *value)
{
  g_return_if_fail (X_IS_MENU_ITEM (menu_item));
  g_return_if_fail (attribute != NULL);
  g_return_if_fail (valid_attribute_name (attribute));

  xmenu_item_clear_cow (menu_item);

  if (value != NULL)
    xhash_table_insert (menu_item->attributes, xstrdup (attribute), xvariant_ref_sink (value));
  else
    xhash_table_remove (menu_item->attributes, attribute);
}

/**
 * xmenu_item_set_attribute:
 * @menu_item: a #xmenu_item_t
 * @attribute: the attribute to set
 * @format_string: (nullable): a #xvariant_t format string, or %NULL
 * @...: positional parameters, as per @format_string
 *
 * Sets or unsets an attribute on @menu_item.
 *
 * The attribute to set or unset is specified by @attribute. This
 * can be one of the standard attribute names %XMENU_ATTRIBUTE_LABEL,
 * %XMENU_ATTRIBUTE_ACTION, %XMENU_ATTRIBUTE_TARGET, or a custom
 * attribute name.
 * Attribute names are restricted to lowercase characters, numbers
 * and '-'. Furthermore, the names must begin with a lowercase character,
 * must not end with a '-', and must not contain consecutive dashes.
 *
 * If @format_string is non-%NULL then the proper position parameters
 * are collected to create a #xvariant_t instance to use as the attribute
 * value.  If it is %NULL then the positional parameterrs are ignored
 * and the named attribute is unset.
 *
 * See also xmenu_item_set_attribute_value() for an equivalent call
 * that directly accepts a #xvariant_t.
 *
 * Since: 2.32
 */
void
xmenu_item_set_attribute (xmenu_item_t   *menu_item,
                           const xchar_t *attribute,
                           const xchar_t *format_string,
                           ...)
{
  xvariant_t *value;

  if (format_string != NULL)
    {
      va_list ap;

      va_start (ap, format_string);
      value = xvariant_new_va (format_string, NULL, &ap);
      va_end (ap);
    }
  else
    value = NULL;

  xmenu_item_set_attribute_value (menu_item, attribute, value);
}

/**
 * xmenu_item_set_link:
 * @menu_item: a #xmenu_item_t
 * @link: type of link to establish or unset
 * @model: (nullable): the #xmenu_model_t to link to (or %NULL to unset)
 *
 * Creates a link from @menu_item to @model if non-%NULL, or unsets it.
 *
 * Links are used to establish a relationship between a particular menu
 * item and another menu.  For example, %XMENU_LINK_SUBMENU is used to
 * associate a submenu with a particular menu item, and %XMENU_LINK_SECTION
 * is used to create a section. Other types of link can be used, but there
 * is no guarantee that clients will be able to make sense of them.
 * Link types are restricted to lowercase characters, numbers
 * and '-'. Furthermore, the names must begin with a lowercase character,
 * must not end with a '-', and must not contain consecutive dashes.
 *
 * Since: 2.32
 */
void
xmenu_item_set_link (xmenu_item_t   *menu_item,
                      const xchar_t *link,
                      xmenu_model_t  *model)
{
  g_return_if_fail (X_IS_MENU_ITEM (menu_item));
  g_return_if_fail (link != NULL);
  g_return_if_fail (valid_attribute_name (link));

  xmenu_item_clear_cow (menu_item);

  if (model != NULL)
    xhash_table_insert (menu_item->links, xstrdup (link), xobject_ref (model));
  else
    xhash_table_remove (menu_item->links, link);
}

/**
 * xmenu_item_get_attribute_value:
 * @menu_item: a #xmenu_item_t
 * @attribute: the attribute name to query
 * @expected_type: (nullable): the expected type of the attribute
 *
 * Queries the named @attribute on @menu_item.
 *
 * If @expected_type is specified and the attribute does not have this
 * type, %NULL is returned.  %NULL is also returned if the attribute
 * simply does not exist.
 *
 * Returns: (nullable) (transfer full): the attribute value, or %NULL
 *
 * Since: 2.34
 */
xvariant_t *
xmenu_item_get_attribute_value (xmenu_item_t          *menu_item,
                                 const xchar_t        *attribute,
                                 const xvariant_type_t *expected_type)
{
  xvariant_t *value;

  xreturn_val_if_fail (X_IS_MENU_ITEM (menu_item), NULL);
  xreturn_val_if_fail (attribute != NULL, NULL);

  value = xhash_table_lookup (menu_item->attributes, attribute);

  if (value != NULL)
    {
      if (expected_type == NULL || xvariant_is_of_type (value, expected_type))
        xvariant_ref (value);
      else
        value = NULL;
    }

  return value;
}

/**
 * xmenu_item_get_attribute:
 * @menu_item: a #xmenu_item_t
 * @attribute: the attribute name to query
 * @format_string: a #xvariant_t format string
 * @...: positional parameters, as per @format_string
 *
 * Queries the named @attribute on @menu_item.
 *
 * If the attribute exists and matches the #xvariant_type_t corresponding
 * to @format_string then @format_string is used to deconstruct the
 * value into the positional parameters and %TRUE is returned.
 *
 * If the attribute does not exist, or it does exist but has the wrong
 * type, then the positional parameters are ignored and %FALSE is
 * returned.
 *
 * Returns: %TRUE if the named attribute was found with the expected
 *     type
 *
 * Since: 2.34
 */
xboolean_t
xmenu_item_get_attribute (xmenu_item_t   *menu_item,
                           const xchar_t *attribute,
                           const xchar_t *format_string,
                           ...)
{
  xvariant_t *value;
  va_list ap;

  xreturn_val_if_fail (X_IS_MENU_ITEM (menu_item), FALSE);
  xreturn_val_if_fail (attribute != NULL, FALSE);
  xreturn_val_if_fail (format_string != NULL, FALSE);

  value = xhash_table_lookup (menu_item->attributes, attribute);

  if (value == NULL)
    return FALSE;

  if (!xvariant_check_format_string (value, format_string, FALSE))
    return FALSE;

  va_start (ap, format_string);
  xvariant_get_va (value, format_string, NULL, &ap);
  va_end (ap);

  return TRUE;
}

/**
 * xmenu_item_get_link:
 * @menu_item: a #xmenu_item_t
 * @link: the link name to query
 *
 * Queries the named @link on @menu_item.
 *
 * Returns: (nullable) (transfer full): the link, or %NULL
 *
 * Since: 2.34
 */
xmenu_model_t *
xmenu_item_get_link (xmenu_item_t   *menu_item,
                      const xchar_t *link)
{
  xmenu_model_t *model;

  xreturn_val_if_fail (X_IS_MENU_ITEM (menu_item), NULL);
  xreturn_val_if_fail (link != NULL, NULL);
  xreturn_val_if_fail (valid_attribute_name (link), NULL);

  model = xhash_table_lookup (menu_item->links, link);

  if (model)
    xobject_ref (model);

  return model;
}

/**
 * xmenu_item_set_label:
 * @menu_item: a #xmenu_item_t
 * @label: (nullable): the label to set, or %NULL to unset
 *
 * Sets or unsets the "label" attribute of @menu_item.
 *
 * If @label is non-%NULL it is used as the label for the menu item.  If
 * it is %NULL then the label attribute is unset.
 *
 * Since: 2.32
 */
void
xmenu_item_set_label (xmenu_item_t   *menu_item,
                       const xchar_t *label)
{
  xvariant_t *value;

  if (label != NULL)
    value = xvariant_new_string (label);
  else
    value = NULL;

  xmenu_item_set_attribute_value (menu_item, XMENU_ATTRIBUTE_LABEL, value);
}

/**
 * xmenu_item_set_submenu:
 * @menu_item: a #xmenu_item_t
 * @submenu: (nullable): a #xmenu_model_t, or %NULL
 *
 * Sets or unsets the "submenu" link of @menu_item to @submenu.
 *
 * If @submenu is non-%NULL, it is linked to.  If it is %NULL then the
 * link is unset.
 *
 * The effect of having one menu appear as a submenu of another is
 * exactly as it sounds.
 *
 * Since: 2.32
 */
void
xmenu_item_set_submenu (xmenu_item_t  *menu_item,
                         xmenu_model_t *submenu)
{
  xmenu_item_set_link (menu_item, XMENU_LINK_SUBMENU, submenu);
}

/**
 * xmenu_item_set_section:
 * @menu_item: a #xmenu_item_t
 * @section: (nullable): a #xmenu_model_t, or %NULL
 *
 * Sets or unsets the "section" link of @menu_item to @section.
 *
 * The effect of having one menu appear as a section of another is
 * exactly as it sounds: the items from @section become a direct part of
 * the menu that @menu_item is added to.  See xmenu_item_new_section()
 * for more information about what it means for a menu item to be a
 * section.
 *
 * Since: 2.32
 */
void
xmenu_item_set_section (xmenu_item_t  *menu_item,
                         xmenu_model_t *section)
{
  xmenu_item_set_link (menu_item, XMENU_LINK_SECTION, section);
}

/**
 * xmenu_item_set_action_and_target_value:
 * @menu_item: a #xmenu_item_t
 * @action: (nullable): the name of the action for this item
 * @target_value: (nullable): a #xvariant_t to use as the action target
 *
 * Sets or unsets the "action" and "target" attributes of @menu_item.
 *
 * If @action is %NULL then both the "action" and "target" attributes
 * are unset (and @target_value is ignored).
 *
 * If @action is non-%NULL then the "action" attribute is set.  The
 * "target" attribute is then set to the value of @target_value if it is
 * non-%NULL or unset otherwise.
 *
 * Normal menu items (ie: not submenu, section or other custom item
 * types) are expected to have the "action" attribute set to identify
 * the action that they are associated with.  The state type of the
 * action help to determine the disposition of the menu item.  See
 * #xaction_t and #xaction_group_t for an overview of actions.
 *
 * In general, clicking on the menu item will result in activation of
 * the named action with the "target" attribute given as the parameter
 * to the action invocation.  If the "target" attribute is not set then
 * the action is invoked with no parameter.
 *
 * If the action has no state then the menu item is usually drawn as a
 * plain menu item (ie: with no additional decoration).
 *
 * If the action has a boolean state then the menu item is usually drawn
 * as a toggle menu item (ie: with a checkmark or equivalent
 * indication).  The item should be marked as 'toggled' or 'checked'
 * when the boolean state is %TRUE.
 *
 * If the action has a string state then the menu item is usually drawn
 * as a radio menu item (ie: with a radio bullet or equivalent
 * indication).  The item should be marked as 'selected' when the string
 * state is equal to the value of the @target property.
 *
 * See xmenu_item_set_action_and_target() or
 * xmenu_item_set_detailed_action() for two equivalent calls that are
 * probably more convenient for most uses.
 *
 * Since: 2.32
 */
void
xmenu_item_set_action_and_target_value (xmenu_item_t   *menu_item,
                                         const xchar_t *action,
                                         xvariant_t    *target_value)
{
  xvariant_t *action_value;

  if (action != NULL)
    {
      action_value = xvariant_new_string (action);
    }
  else
    {
      action_value = NULL;
      target_value = NULL;
    }

  xmenu_item_set_attribute_value (menu_item, XMENU_ATTRIBUTE_ACTION, action_value);
  xmenu_item_set_attribute_value (menu_item, XMENU_ATTRIBUTE_TARGET, target_value);
}

/**
 * xmenu_item_set_action_and_target:
 * @menu_item: a #xmenu_item_t
 * @action: (nullable): the name of the action for this item
 * @format_string: (nullable): a xvariant_t format string
 * @...: positional parameters, as per @format_string
 *
 * Sets or unsets the "action" and "target" attributes of @menu_item.
 *
 * If @action is %NULL then both the "action" and "target" attributes
 * are unset (and @format_string is ignored along with the positional
 * parameters).
 *
 * If @action is non-%NULL then the "action" attribute is set.
 * @format_string is then inspected.  If it is non-%NULL then the proper
 * position parameters are collected to create a #xvariant_t instance to
 * use as the target value.  If it is %NULL then the positional
 * parameters are ignored and the "target" attribute is unset.
 *
 * See also xmenu_item_set_action_and_target_value() for an equivalent
 * call that directly accepts a #xvariant_t.  See
 * xmenu_item_set_detailed_action() for a more convenient version that
 * works with string-typed targets.
 *
 * See also xmenu_item_set_action_and_target_value() for a
 * description of the semantics of the action and target attributes.
 *
 * Since: 2.32
 */
void
xmenu_item_set_action_and_target (xmenu_item_t   *menu_item,
                                   const xchar_t *action,
                                   const xchar_t *format_string,
                                   ...)
{
  xvariant_t *value;

  if (format_string != NULL)
    {
      va_list ap;

      va_start (ap, format_string);
      value = xvariant_new_va (format_string, NULL, &ap);
      va_end (ap);
    }
  else
    value = NULL;

  xmenu_item_set_action_and_target_value (menu_item, action, value);
}

/**
 * xmenu_item_set_detailed_action:
 * @menu_item: a #xmenu_item_t
 * @detailed_action: the "detailed" action string
 *
 * Sets the "action" and possibly the "target" attribute of @menu_item.
 *
 * The format of @detailed_action is the same format parsed by
 * xaction_parse_detailed_name().
 *
 * See xmenu_item_set_action_and_target() or
 * xmenu_item_set_action_and_target_value() for more flexible (but
 * slightly less convenient) alternatives.
 *
 * See also xmenu_item_set_action_and_target_value() for a description of
 * the semantics of the action and target attributes.
 *
 * Since: 2.32
 */
void
xmenu_item_set_detailed_action (xmenu_item_t   *menu_item,
                                 const xchar_t *detailed_action)
{
  xerror_t *error = NULL;
  xvariant_t *target;
  xchar_t *name;

  if (!xaction_parse_detailed_name (detailed_action, &name, &target, &error))
    xerror ("xmenu_item_set_detailed_action: %s", error->message);

  xmenu_item_set_action_and_target_value (menu_item, name, target);
  if (target)
    xvariant_unref (target);
  g_free (name);
}

/**
 * xmenu_item_new:
 * @label: (nullable): the section label, or %NULL
 * @detailed_action: (nullable): the detailed action string, or %NULL
 *
 * Creates a new #xmenu_item_t.
 *
 * If @label is non-%NULL it is used to set the "label" attribute of the
 * new item.
 *
 * If @detailed_action is non-%NULL it is used to set the "action" and
 * possibly the "target" attribute of the new item.  See
 * xmenu_item_set_detailed_action() for more information.
 *
 * Returns: a new #xmenu_item_t
 *
 * Since: 2.32
 */
xmenu_item_t *
xmenu_item_new (const xchar_t *label,
                 const xchar_t *detailed_action)
{
  xmenu_item_t *menu_item;

  menu_item = xobject_new (XTYPE_MENU_ITEM, NULL);

  if (label != NULL)
    xmenu_item_set_label (menu_item, label);

  if (detailed_action != NULL)
    xmenu_item_set_detailed_action (menu_item, detailed_action);

  return menu_item;
}

/**
 * xmenu_item_new_submenu:
 * @label: (nullable): the section label, or %NULL
 * @submenu: a #xmenu_model_t with the items of the submenu
 *
 * Creates a new #xmenu_item_t representing a submenu.
 *
 * This is a convenience API around xmenu_item_new() and
 * xmenu_item_set_submenu().
 *
 * Returns: a new #xmenu_item_t
 *
 * Since: 2.32
 */
xmenu_item_t *
xmenu_item_new_submenu (const xchar_t *label,
                         xmenu_model_t  *submenu)
{
  xmenu_item_t *menu_item;

  menu_item = xobject_new (XTYPE_MENU_ITEM, NULL);

  if (label != NULL)
    xmenu_item_set_label (menu_item, label);

  xmenu_item_set_submenu (menu_item, submenu);

  return menu_item;
}

/**
 * xmenu_item_new_section:
 * @label: (nullable): the section label, or %NULL
 * @section: a #xmenu_model_t with the items of the section
 *
 * Creates a new #xmenu_item_t representing a section.
 *
 * This is a convenience API around xmenu_item_new() and
 * xmenu_item_set_section().
 *
 * The effect of having one menu appear as a section of another is
 * exactly as it sounds: the items from @section become a direct part of
 * the menu that @menu_item is added to.
 *
 * Visual separation is typically displayed between two non-empty
 * sections.  If @label is non-%NULL then it will be encorporated into
 * this visual indication.  This allows for labeled subsections of a
 * menu.
 *
 * As a simple example, consider a typical "Edit" menu from a simple
 * program.  It probably contains an "Undo" and "Redo" item, followed by
 * a separator, followed by "Cut", "Copy" and "Paste".
 *
 * This would be accomplished by creating three #xmenu_t instances.  The
 * first would be populated with the "Undo" and "Redo" items, and the
 * second with the "Cut", "Copy" and "Paste" items.  The first and
 * second menus would then be added as submenus of the third.  In XML
 * format, this would look something like the following:
 * |[
 * <menu id='edit-menu'>
 *   <section>
 *     <item label='Undo'/>
 *     <item label='Redo'/>
 *   </section>
 *   <section>
 *     <item label='Cut'/>
 *     <item label='Copy'/>
 *     <item label='Paste'/>
 *   </section>
 * </menu>
 * ]|
 *
 * The following example is exactly equivalent.  It is more illustrative
 * of the exact relationship between the menus and items (keeping in
 * mind that the 'link' element defines a new menu that is linked to the
 * containing one).  The style of the second example is more verbose and
 * difficult to read (and therefore not recommended except for the
 * purpose of understanding what is really going on).
 * |[
 * <menu id='edit-menu'>
 *   <item>
 *     <link name='section'>
 *       <item label='Undo'/>
 *       <item label='Redo'/>
 *     </link>
 *   </item>
 *   <item>
 *     <link name='section'>
 *       <item label='Cut'/>
 *       <item label='Copy'/>
 *       <item label='Paste'/>
 *     </link>
 *   </item>
 * </menu>
 * ]|
 *
 * Returns: a new #xmenu_item_t
 *
 * Since: 2.32
 */
xmenu_item_t *
xmenu_item_new_section (const xchar_t *label,
                         xmenu_model_t  *section)
{
  xmenu_item_t *menu_item;

  menu_item = xobject_new (XTYPE_MENU_ITEM, NULL);

  if (label != NULL)
    xmenu_item_set_label (menu_item, label);

  xmenu_item_set_section (menu_item, section);

  return menu_item;
}

/**
 * xmenu_item_new_from_model:
 * @model: a #xmenu_model_t
 * @item_index: the index of an item in @model
 *
 * Creates a #xmenu_item_t as an exact copy of an existing menu item in a
 * #xmenu_model_t.
 *
 * @item_index must be valid (ie: be sure to call
 * xmenu_model_get_n_items() first).
 *
 * Returns: a new #xmenu_item_t.
 *
 * Since: 2.34
 */
xmenu_item_t *
xmenu_item_new_from_model (xmenu_model_t *model,
                            xint_t        item_index)
{
  xmenu_model_class_t *class = XMENU_MODEL_GET_CLASS (model);
  xmenu_item_t *menu_item;

  menu_item = xobject_new (XTYPE_MENU_ITEM, NULL);

  /* With some trickery we can be pretty efficient.
   *
   * A xmenu_model_t must either implement iterate_item_attributes() or
   * get_item_attributes().  If it implements get_item_attributes() then
   * we are in luck -- we can just take a reference on the returned
   * hashtable and mark ourselves as copy-on-write.
   *
   * In the case that the model is based on get_item_attributes (which
   * is the case for both xmenu_t and xdbus_menu_model_t) then this is
   * basically just xhash_table_ref().
   */
  if (class->get_item_attributes)
    {
      xhashtable_t *attributes = NULL;

      class->get_item_attributes (model, item_index, &attributes);
      if (attributes)
        {
          xhash_table_unref (menu_item->attributes);
          menu_item->attributes = attributes;
          menu_item->cow = TRUE;
        }
    }
  else
    {
      xmenu_attribute_iter_t *iter;
      const xchar_t *attribute;
      xvariant_t *value;

      iter = xmenu_model_iterate_item_attributes (model, item_index);
      while (xmenu_attribute_iter_get_next (iter, &attribute, &value))
        xhash_table_insert (menu_item->attributes, xstrdup (attribute), value);
      xobject_unref (iter);
    }

  /* Same story for the links... */
  if (class->get_item_links)
    {
      xhashtable_t *links = NULL;

      class->get_item_links (model, item_index, &links);
      if (links)
        {
          xhash_table_unref (menu_item->links);
          menu_item->links = links;
          menu_item->cow = TRUE;
        }
    }
  else
    {
      xmenu_link_iter_t *iter;
      const xchar_t *link;
      xmenu_model_t *value;

      iter = xmenu_model_iterate_item_links (model, item_index);
      while (xmenu_link_iter_get_next (iter, &link, &value))
        xhash_table_insert (menu_item->links, xstrdup (link), value);
      xobject_unref (iter);
    }

  return menu_item;
}

/**
 * xmenu_item_set_icon:
 * @menu_item: a #xmenu_item_t
 * @icon: a #xicon_t, or %NULL
 *
 * Sets (or unsets) the icon on @menu_item.
 *
 * This call is the same as calling xicon_serialize() and using the
 * result as the value to xmenu_item_set_attribute_value() for
 * %XMENU_ATTRIBUTE_ICON.
 *
 * This API is only intended for use with "noun" menu items; things like
 * bookmarks or applications in an "Open With" menu.  Don't use it on
 * menu items corresponding to verbs (eg: stock icons for 'Save' or
 * 'Quit').
 *
 * If @icon is %NULL then the icon is unset.
 *
 * Since: 2.38
 **/
void
xmenu_item_set_icon (xmenu_item_t *menu_item,
                      xicon_t     *icon)
{
  xvariant_t *value;

  g_return_if_fail (X_IS_MENU_ITEM (menu_item));
  g_return_if_fail (icon == NULL || X_IS_ICON (icon));

  if (icon != NULL)
    value = xicon_serialize (icon);
  else
    value = NULL;

  xmenu_item_set_attribute_value (menu_item, XMENU_ATTRIBUTE_ICON, value);
  if (value)
    xvariant_unref (value);
}
