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

#ifndef __G_MENU_H__
#define __G_MENU_H__

#include <gio/gmenumodel.h>

G_BEGIN_DECLS

#define XTYPE_MENU          (g_menu_get_type ())
#define G_MENU(inst)         (XTYPE_CHECK_INSTANCE_CAST ((inst), \
                              XTYPE_MENU, GMenu))
#define X_IS_MENU(inst)      (XTYPE_CHECK_INSTANCE_TYPE ((inst), \
                              XTYPE_MENU))

#define XTYPE_MENU_ITEM     (g_menu_item_get_type ())
#define G_MENU_ITEM(inst)    (XTYPE_CHECK_INSTANCE_CAST ((inst), \
                              XTYPE_MENU_ITEM, GMenuItem))
#define X_IS_MENU_ITEM(inst) (XTYPE_CHECK_INSTANCE_TYPE ((inst), \
                              XTYPE_MENU_ITEM))

typedef struct _GMenuItem GMenuItem;
typedef struct _GMenu     GMenu;

XPL_AVAILABLE_IN_2_32
xtype_t       g_menu_get_type                         (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_32
GMenu *     g_menu_new                              (void);

XPL_AVAILABLE_IN_2_32
void        g_menu_freeze                           (GMenu       *menu);

XPL_AVAILABLE_IN_2_32
void        g_menu_insert_item                      (GMenu       *menu,
                                                     xint_t         position,
                                                     GMenuItem   *item);
XPL_AVAILABLE_IN_2_32
void        g_menu_prepend_item                     (GMenu       *menu,
                                                     GMenuItem   *item);
XPL_AVAILABLE_IN_2_32
void        g_menu_append_item                      (GMenu       *menu,
                                                     GMenuItem   *item);
XPL_AVAILABLE_IN_2_32
void        g_menu_remove                           (GMenu       *menu,
                                                     xint_t         position);

XPL_AVAILABLE_IN_2_38
void        g_menu_remove_all                       (GMenu       *menu);

XPL_AVAILABLE_IN_2_32
void        g_menu_insert                           (GMenu       *menu,
                                                     xint_t         position,
                                                     const xchar_t *label,
                                                     const xchar_t *detailed_action);
XPL_AVAILABLE_IN_2_32
void        g_menu_prepend                          (GMenu       *menu,
                                                     const xchar_t *label,
                                                     const xchar_t *detailed_action);
XPL_AVAILABLE_IN_2_32
void        g_menu_append                           (GMenu       *menu,
                                                     const xchar_t *label,
                                                     const xchar_t *detailed_action);

XPL_AVAILABLE_IN_2_32
void        g_menu_insert_section                   (GMenu       *menu,
                                                     xint_t         position,
                                                     const xchar_t *label,
                                                     GMenuModel  *section);
XPL_AVAILABLE_IN_2_32
void        g_menu_prepend_section                  (GMenu       *menu,
                                                     const xchar_t *label,
                                                     GMenuModel  *section);
XPL_AVAILABLE_IN_2_32
void        g_menu_append_section                   (GMenu       *menu,
                                                     const xchar_t *label,
                                                     GMenuModel  *section);

XPL_AVAILABLE_IN_2_32
void        g_menu_insert_submenu                   (GMenu       *menu,
                                                     xint_t        position,
                                                     const xchar_t *label,
                                                     GMenuModel  *submenu);
XPL_AVAILABLE_IN_2_32
void        g_menu_prepend_submenu                  (GMenu       *menu,
                                                     const xchar_t *label,
                                                     GMenuModel  *submenu);
XPL_AVAILABLE_IN_2_32
void        g_menu_append_submenu                   (GMenu       *menu,
                                                     const xchar_t *label,
                                                     GMenuModel  *submenu);


XPL_AVAILABLE_IN_2_32
xtype_t       g_menu_item_get_type                    (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_32
GMenuItem * g_menu_item_new                         (const xchar_t *label,
                                                     const xchar_t *detailed_action);

XPL_AVAILABLE_IN_2_34
GMenuItem * g_menu_item_new_from_model              (GMenuModel  *model,
                                                     xint_t         item_index);

XPL_AVAILABLE_IN_2_32
GMenuItem * g_menu_item_new_submenu                 (const xchar_t *label,
                                                     GMenuModel  *submenu);

XPL_AVAILABLE_IN_2_32
GMenuItem * g_menu_item_new_section                 (const xchar_t *label,
                                                     GMenuModel  *section);

XPL_AVAILABLE_IN_2_34
xvariant_t *  g_menu_item_get_attribute_value         (GMenuItem   *menu_item,
                                                     const xchar_t *attribute,
                                                     const xvariant_type_t *expected_type);
XPL_AVAILABLE_IN_2_34
xboolean_t    g_menu_item_get_attribute               (GMenuItem   *menu_item,
                                                     const xchar_t *attribute,
                                                     const xchar_t *format_string,
                                                     ...);
XPL_AVAILABLE_IN_2_34
GMenuModel *g_menu_item_get_link                    (GMenuItem   *menu_item,
                                                     const xchar_t *link);

XPL_AVAILABLE_IN_2_32
void        g_menu_item_set_attribute_value         (GMenuItem   *menu_item,
                                                     const xchar_t *attribute,
                                                     xvariant_t    *value);
XPL_AVAILABLE_IN_2_32
void        g_menu_item_set_attribute               (GMenuItem   *menu_item,
                                                     const xchar_t *attribute,
                                                     const xchar_t *format_string,
                                                     ...);
XPL_AVAILABLE_IN_2_32
void        g_menu_item_set_link                    (GMenuItem   *menu_item,
                                                     const xchar_t *link,
                                                     GMenuModel  *model);
XPL_AVAILABLE_IN_2_32
void        g_menu_item_set_label                   (GMenuItem   *menu_item,
                                                     const xchar_t *label);
XPL_AVAILABLE_IN_2_32
void        g_menu_item_set_submenu                 (GMenuItem   *menu_item,
                                                     GMenuModel  *submenu);
XPL_AVAILABLE_IN_2_32
void        g_menu_item_set_section                 (GMenuItem   *menu_item,
                                                     GMenuModel  *section);
XPL_AVAILABLE_IN_2_32
void        g_menu_item_set_action_and_target_value (GMenuItem   *menu_item,
                                                     const xchar_t *action,
                                                     xvariant_t    *target_value);
XPL_AVAILABLE_IN_2_32
void        g_menu_item_set_action_and_target       (GMenuItem   *menu_item,
                                                     const xchar_t *action,
                                                     const xchar_t *format_string,
                                                     ...);
XPL_AVAILABLE_IN_2_32
void        g_menu_item_set_detailed_action         (GMenuItem   *menu_item,
                                                     const xchar_t *detailed_action);

XPL_AVAILABLE_IN_2_38
void        g_menu_item_set_icon                    (GMenuItem   *menu_item,
                                                     xicon_t       *icon);

G_END_DECLS

#endif /* __G_MENU_H__ */
