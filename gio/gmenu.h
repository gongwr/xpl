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

#ifndef __XMENU_H__
#define __XMENU_H__

#include <gio/gmenumodel.h>

G_BEGIN_DECLS

#define XTYPE_MENU          (xmenu_get_type ())
#define XMENU(inst)         (XTYPE_CHECK_INSTANCE_CAST ((inst), \
                              XTYPE_MENU, xmenu))
#define X_IS_MENU(inst)      (XTYPE_CHECK_INSTANCE_TYPE ((inst), \
                              XTYPE_MENU))

#define XTYPE_MENU_ITEM     (xmenu_item_get_type ())
#define XMENU_ITEM(inst)    (XTYPE_CHECK_INSTANCE_CAST ((inst), \
                              XTYPE_MENU_ITEM, xmenu_item))
#define X_IS_MENU_ITEM(inst) (XTYPE_CHECK_INSTANCE_TYPE ((inst), \
                              XTYPE_MENU_ITEM))

typedef struct _GMenuItem xmenu_item_t;
typedef struct _GMenu     xmenu_t;

XPL_AVAILABLE_IN_2_32
xtype_t       xmenu_get_type                         (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_32
xmenu_t *     xmenu_new                              (void);

XPL_AVAILABLE_IN_2_32
void        xmenu_freeze                           (xmenu_t       *menu);

XPL_AVAILABLE_IN_2_32
void        xmenu_insert_item                      (xmenu_t       *menu,
                                                     xint_t         position,
                                                     xmenu_item_t   *item);
XPL_AVAILABLE_IN_2_32
void        xmenu_prepend_item                     (xmenu_t       *menu,
                                                     xmenu_item_t   *item);
XPL_AVAILABLE_IN_2_32
void        xmenu_append_item                      (xmenu_t       *menu,
                                                     xmenu_item_t   *item);
XPL_AVAILABLE_IN_2_32
void        xmenu_remove                           (xmenu_t       *menu,
                                                     xint_t         position);

XPL_AVAILABLE_IN_2_38
void        xmenu_remove_all                       (xmenu_t       *menu);

XPL_AVAILABLE_IN_2_32
void        xmenu_insert                           (xmenu_t       *menu,
                                                     xint_t         position,
                                                     const xchar_t *label,
                                                     const xchar_t *detailed_action);
XPL_AVAILABLE_IN_2_32
void        xmenu_prepend                          (xmenu_t       *menu,
                                                     const xchar_t *label,
                                                     const xchar_t *detailed_action);
XPL_AVAILABLE_IN_2_32
void        xmenu_append                           (xmenu_t       *menu,
                                                     const xchar_t *label,
                                                     const xchar_t *detailed_action);

XPL_AVAILABLE_IN_2_32
void        xmenu_insert_section                   (xmenu_t       *menu,
                                                     xint_t         position,
                                                     const xchar_t *label,
                                                     xmenu_model_t  *section);
XPL_AVAILABLE_IN_2_32
void        xmenu_prepend_section                  (xmenu_t       *menu,
                                                     const xchar_t *label,
                                                     xmenu_model_t  *section);
XPL_AVAILABLE_IN_2_32
void        xmenu_append_section                   (xmenu_t       *menu,
                                                     const xchar_t *label,
                                                     xmenu_model_t  *section);

XPL_AVAILABLE_IN_2_32
void        xmenu_insert_submenu                   (xmenu_t       *menu,
                                                     xint_t        position,
                                                     const xchar_t *label,
                                                     xmenu_model_t  *submenu);
XPL_AVAILABLE_IN_2_32
void        xmenu_prepend_submenu                  (xmenu_t       *menu,
                                                     const xchar_t *label,
                                                     xmenu_model_t  *submenu);
XPL_AVAILABLE_IN_2_32
void        xmenu_append_submenu                   (xmenu_t       *menu,
                                                     const xchar_t *label,
                                                     xmenu_model_t  *submenu);


XPL_AVAILABLE_IN_2_32
xtype_t       xmenu_item_get_type                    (void) G_GNUC_CONST;
XPL_AVAILABLE_IN_2_32
xmenu_item_t * xmenu_item_new                         (const xchar_t *label,
                                                     const xchar_t *detailed_action);

XPL_AVAILABLE_IN_2_34
xmenu_item_t * xmenu_item_new_from_model              (xmenu_model_t  *model,
                                                     xint_t         item_index);

XPL_AVAILABLE_IN_2_32
xmenu_item_t * xmenu_item_new_submenu                 (const xchar_t *label,
                                                     xmenu_model_t  *submenu);

XPL_AVAILABLE_IN_2_32
xmenu_item_t * xmenu_item_new_section                 (const xchar_t *label,
                                                     xmenu_model_t  *section);

XPL_AVAILABLE_IN_2_34
xvariant_t *  xmenu_item_get_attribute_value         (xmenu_item_t   *menu_item,
                                                     const xchar_t *attribute,
                                                     const xvariant_type_t *expected_type);
XPL_AVAILABLE_IN_2_34
xboolean_t    xmenu_item_get_attribute               (xmenu_item_t   *menu_item,
                                                     const xchar_t *attribute,
                                                     const xchar_t *format_string,
                                                     ...);
XPL_AVAILABLE_IN_2_34
xmenu_model_t *xmenu_item_get_link                    (xmenu_item_t   *menu_item,
                                                     const xchar_t *link);

XPL_AVAILABLE_IN_2_32
void        xmenu_item_set_attribute_value         (xmenu_item_t   *menu_item,
                                                     const xchar_t *attribute,
                                                     xvariant_t    *value);
XPL_AVAILABLE_IN_2_32
void        xmenu_item_set_attribute               (xmenu_item_t   *menu_item,
                                                     const xchar_t *attribute,
                                                     const xchar_t *format_string,
                                                     ...);
XPL_AVAILABLE_IN_2_32
void        xmenu_item_set_link                    (xmenu_item_t   *menu_item,
                                                     const xchar_t *link,
                                                     xmenu_model_t  *model);
XPL_AVAILABLE_IN_2_32
void        xmenu_item_set_label                   (xmenu_item_t   *menu_item,
                                                     const xchar_t *label);
XPL_AVAILABLE_IN_2_32
void        xmenu_item_set_submenu                 (xmenu_item_t   *menu_item,
                                                     xmenu_model_t  *submenu);
XPL_AVAILABLE_IN_2_32
void        xmenu_item_set_section                 (xmenu_item_t   *menu_item,
                                                     xmenu_model_t  *section);
XPL_AVAILABLE_IN_2_32
void        xmenu_item_set_action_and_target_value (xmenu_item_t   *menu_item,
                                                     const xchar_t *action,
                                                     xvariant_t    *target_value);
XPL_AVAILABLE_IN_2_32
void        xmenu_item_set_action_and_target       (xmenu_item_t   *menu_item,
                                                     const xchar_t *action,
                                                     const xchar_t *format_string,
                                                     ...);
XPL_AVAILABLE_IN_2_32
void        xmenu_item_set_detailed_action         (xmenu_item_t   *menu_item,
                                                     const xchar_t *detailed_action);

XPL_AVAILABLE_IN_2_38
void        xmenu_item_set_icon                    (xmenu_item_t   *menu_item,
                                                     xicon_t       *icon);

G_END_DECLS

#endif /* __XMENU_H__ */
