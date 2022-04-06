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

#ifndef __XMENU_MODEL_H__
#define __XMENU_MODEL_H__

#include <glib-object.h>

#include <gio/giotypes.h>

G_BEGIN_DECLS

/**
 * XMENU_ATTRIBUTE_ACTION:
 *
 * The menu item attribute which holds the action name of the item.  Action
 * names are namespaced with an identifier for the action group in which the
 * action resides. For example, "win." for window-specific actions and "app."
 * for application-wide actions.
 *
 * See also xmenu_model_get_item_attribute() and xmenu_item_set_attribute().
 *
 * Since: 2.32
 **/
#define XMENU_ATTRIBUTE_ACTION "action"

/**
 * XMENU_ATTRIBUTE_ACTION_NAMESPACE:
 *
 * The menu item attribute that holds the namespace for all action names in
 * menus that are linked from this item.
 *
 * Since: 2.36
 **/
#define XMENU_ATTRIBUTE_ACTION_NAMESPACE "action-namespace"

/**
 * XMENU_ATTRIBUTE_TARGET:
 *
 * The menu item attribute which holds the target with which the item's action
 * will be activated.
 *
 * See also xmenu_item_set_action_and_target()
 *
 * Since: 2.32
 **/
#define XMENU_ATTRIBUTE_TARGET "target"

/**
 * XMENU_ATTRIBUTE_LABEL:
 *
 * The menu item attribute which holds the label of the item.
 *
 * Since: 2.32
 **/
#define XMENU_ATTRIBUTE_LABEL "label"

/**
 * XMENU_ATTRIBUTE_ICON:
 *
 * The menu item attribute which holds the icon of the item.
 *
 * The icon is stored in the format returned by xicon_serialize().
 *
 * This attribute is intended only to represent 'noun' icons such as
 * favicons for a webpage, or application icons.  It should not be used
 * for 'verbs' (ie: stock icons).
 *
 * Since: 2.38
 **/
#define XMENU_ATTRIBUTE_ICON "icon"

/**
 * XMENU_LINK_SUBMENU:
 *
 * The name of the link that associates a menu item with a submenu.
 *
 * See also xmenu_item_set_link().
 *
 * Since: 2.32
 **/
#define XMENU_LINK_SUBMENU "submenu"

/**
 * XMENU_LINK_SECTION:
 *
 * The name of the link that associates a menu item with a section.  The linked
 * menu will usually be shown in place of the menu item, using the item's label
 * as a header.
 *
 * See also xmenu_item_set_link().
 *
 * Since: 2.32
 **/
#define XMENU_LINK_SECTION "section"

#define XTYPE_MENU_MODEL                                   (xmenu_model_get_type ())
#define XMENU_MODEL(inst)                                  (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_MENU_MODEL, xmenu_model))
#define XMENU_MODEL_CLASS(class)                           (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_MENU_MODEL, xmenu_model_class_t))
#define X_IS_MENU_MODEL(inst)                               (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_MENU_MODEL))
#define X_IS_MENU_MODEL_CLASS(class)                        (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_MENU_MODEL))
#define XMENU_MODEL_GET_CLASS(inst)                        (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_MENU_MODEL, xmenu_model_class_t))

typedef struct _GMenuModelPrivate                           GMenuModelPrivate;
typedef struct _GMenuModelClass                             xmenu_model_class_t;

typedef struct _GMenuAttributeIterPrivate                   GMenuAttributeIterPrivate;
typedef struct _GMenuAttributeIterClass                     xmenu_attribute_iter_class_t;
typedef struct _GMenuAttributeIter                          xmenu_attribute_iter_t;

typedef struct _GMenuLinkIterPrivate                        xmenu_link_iter_private_t;
typedef struct _xmenu_link_iter_class                          xmenu_link_iter_class_t;
typedef struct _xmenu_link_iter                               xmenu_link_iter_t;

struct _GMenuModel
{
  xobject_t            parent_instance;
  GMenuModelPrivate *priv;
};

/**
 * xmenu_model_class_t::get_item_attributes:
 * @model: the #xmenu_model_t to query
 * @item_index: The #xmenu_item_t to query
 * @attributes: (out) (element-type utf8 GLib.Variant): Attributes on the item
 *
 * Gets all the attributes associated with the item in the menu model.
 */
/**
 * xmenu_model_class_t::get_item_links:
 * @model: the #xmenu_model_t to query
 * @item_index: The #xmenu_item_t to query
 * @links: (out) (element-type utf8 Gio.MenuModel): Links from the item
 *
 * Gets all the links associated with the item in the menu model.
 */
struct _GMenuModelClass
{
  xobject_class_t parent_class;

  xboolean_t              (*is_mutable)                       (xmenu_model_t          *model);
  xint_t                  (*get_n_items)                      (xmenu_model_t          *model);
  void                  (*get_item_attributes)              (xmenu_model_t          *model,
                                                             xint_t                 item_index,
                                                             xhashtable_t         **attributes);
  xmenu_attribute_iter_t *  (*iterate_item_attributes)          (xmenu_model_t          *model,
                                                             xint_t                 item_index);
  xvariant_t *            (*get_item_attribute_value)         (xmenu_model_t          *model,
                                                             xint_t                 item_index,
                                                             const xchar_t         *attribute,
                                                             const xvariant_type_t  *expected_type);
  void                  (*get_item_links)                   (xmenu_model_t          *model,
                                                             xint_t                 item_index,
                                                             xhashtable_t         **links);
  xmenu_link_iter_t *       (*iterate_item_links)               (xmenu_model_t          *model,
                                                             xint_t                 item_index);
  xmenu_model_t *          (*get_item_link)                    (xmenu_model_t          *model,
                                                             xint_t                 item_index,
                                                             const xchar_t         *link);
};

XPL_AVAILABLE_IN_2_32
xtype_t                   xmenu_model_get_type                           (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_32
xboolean_t                xmenu_model_is_mutable                         (xmenu_model_t         *model);
XPL_AVAILABLE_IN_2_32
xint_t                    xmenu_model_get_n_items                        (xmenu_model_t         *model);

XPL_AVAILABLE_IN_2_32
xmenu_attribute_iter_t *    xmenu_model_iterate_item_attributes            (xmenu_model_t         *model,
                                                                         xint_t                item_index);
XPL_AVAILABLE_IN_2_32
xvariant_t *              xmenu_model_get_item_attribute_value           (xmenu_model_t         *model,
                                                                         xint_t                item_index,
                                                                         const xchar_t        *attribute,
                                                                         const xvariant_type_t *expected_type);
XPL_AVAILABLE_IN_2_32
xboolean_t                xmenu_model_get_item_attribute                 (xmenu_model_t         *model,
                                                                         xint_t                item_index,
                                                                         const xchar_t        *attribute,
                                                                         const xchar_t        *format_string,
                                                                         ...);
XPL_AVAILABLE_IN_2_32
xmenu_link_iter_t *         xmenu_model_iterate_item_links                 (xmenu_model_t         *model,
                                                                         xint_t                item_index);
XPL_AVAILABLE_IN_2_32
xmenu_model_t *            xmenu_model_get_item_link                      (xmenu_model_t         *model,
                                                                         xint_t                item_index,
                                                                         const xchar_t        *link);

XPL_AVAILABLE_IN_2_32
void                    xmenu_model_items_changed                      (xmenu_model_t         *model,
                                                                         xint_t                position,
                                                                         xint_t                removed,
                                                                         xint_t                added);


#define XTYPE_MENU_ATTRIBUTE_ITER                          (xmenu_attribute_iter_get_type ())
#define XMENU_ATTRIBUTE_ITER(inst)                         (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_MENU_ATTRIBUTE_ITER, xmenu_attribute_iter))
#define XMENU_ATTRIBUTE_ITER_CLASS(class)                  (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_MENU_ATTRIBUTE_ITER, xmenu_attribute_iter_class))
#define X_IS_MENU_ATTRIBUTE_ITER(inst)                      (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_MENU_ATTRIBUTE_ITER))
#define X_IS_MENU_ATTRIBUTE_ITER_CLASS(class)               (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_MENU_ATTRIBUTE_ITER))
#define XMENU_ATTRIBUTE_ITER_GET_CLASS(inst)               (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_MENU_ATTRIBUTE_ITER, xmenu_attribute_iter_class))

struct _GMenuAttributeIter
{
  xobject_t parent_instance;
  GMenuAttributeIterPrivate *priv;
};

struct _GMenuAttributeIterClass
{
  xobject_class_t parent_class;

  xboolean_t      (*get_next) (xmenu_attribute_iter_t  *iter,
                             const xchar_t        **out_name,
                             xvariant_t           **value);
};

XPL_AVAILABLE_IN_2_32
xtype_t                   xmenu_attribute_iter_get_type                  (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_32
xboolean_t                xmenu_attribute_iter_get_next                  (xmenu_attribute_iter_t  *iter,
                                                                         const xchar_t        **out_name,
                                                                         xvariant_t           **value);
XPL_AVAILABLE_IN_2_32
xboolean_t                xmenu_attribute_iter_next                      (xmenu_attribute_iter_t  *iter);
XPL_AVAILABLE_IN_2_32
const xchar_t *           xmenu_attribute_iter_get_name                  (xmenu_attribute_iter_t  *iter);
XPL_AVAILABLE_IN_2_32
xvariant_t *              xmenu_attribute_iter_get_value                 (xmenu_attribute_iter_t  *iter);


#define XTYPE_MENU_LINK_ITER                               (xmenu_link_iter_get_type ())
#define XMENU_LINK_ITER(inst)                              (XTYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             XTYPE_MENU_LINK_ITER, xmenu_link_iter_t))
#define XMENU_LINK_ITER_CLASS(class)                       (XTYPE_CHECK_CLASS_CAST ((class),                       \
                                                             XTYPE_MENU_LINK_ITER, xmenu_link_iter_class_t))
#define X_IS_MENU_LINK_ITER(inst)                           (XTYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             XTYPE_MENU_LINK_ITER))
#define X_IS_MENU_LINK_ITER_CLASS(class)                    (XTYPE_CHECK_CLASS_TYPE ((class),                       \
                                                             XTYPE_MENU_LINK_ITER))
#define XMENU_LINK_ITER_GET_CLASS(inst)                    (XTYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                             XTYPE_MENU_LINK_ITER, xmenu_link_iter_class_t))

struct _xmenu_link_iter
{
  xobject_t parent_instance;
  xmenu_link_iter_private_t *priv;
};

struct _xmenu_link_iter_class
{
  xobject_class_t parent_class;

  xboolean_t      (*get_next) (xmenu_link_iter_t  *iter,
                             const xchar_t   **out_link,
                             xmenu_model_t    **value);
};

XPL_AVAILABLE_IN_2_32
xtype_t                   xmenu_link_iter_get_type                       (void) G_GNUC_CONST;

XPL_AVAILABLE_IN_2_32
xboolean_t                xmenu_link_iter_get_next                       (xmenu_link_iter_t  *iter,
                                                                         const xchar_t   **out_link,
                                                                         xmenu_model_t    **value);
XPL_AVAILABLE_IN_2_32
xboolean_t                xmenu_link_iter_next                           (xmenu_link_iter_t  *iter);
XPL_AVAILABLE_IN_2_32
const xchar_t *           xmenu_link_iter_get_name                       (xmenu_link_iter_t  *iter);
XPL_AVAILABLE_IN_2_32
xmenu_model_t *            xmenu_link_iter_get_value                      (xmenu_link_iter_t  *iter);

G_END_DECLS

#endif /* __XMENU_MODEL_H__ */
