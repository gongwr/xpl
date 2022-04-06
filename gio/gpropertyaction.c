/*
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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gpropertyaction.h"

#include "gsettings-mapping.h"
#include "gaction.h"
#include "glibintl.h"

/**
 * SECTION:gpropertyaction
 * @title: xproperty_action_t
 * @short_description: A xaction_t reflecting a xobject_t property
 * @include: gio/gio.h
 *
 * A #xproperty_action_t is a way to get a #xaction_t with a state value
 * reflecting and controlling the value of a #xobject_t property.
 *
 * The state of the action will correspond to the value of the property.
 * Changing it will change the property (assuming the requested value
 * matches the requirements as specified in the #xparam_spec_t).
 *
 * Only the most common types are presently supported.  Booleans are
 * mapped to booleans, strings to strings, signed/unsigned integers to
 * int32/uint32 and floats and doubles to doubles.
 *
 * If the property is an enum then the state will be string-typed and
 * conversion will automatically be performed between the enum value and
 * "nick" string as per the #xenum_value_t table.
 *
 * Flags types are not currently supported.
 *
 * Properties of object types, boxed types and pointer types are not
 * supported and probably never will be.
 *
 * Properties of #xvariant_t types are not currently supported.
 *
 * If the property is boolean-valued then the action will have a NULL
 * parameter type, and activating the action (with no parameter) will
 * toggle the value of the property.
 *
 * In all other cases, the parameter type will correspond to the type of
 * the property.
 *
 * The general idea here is to reduce the number of locations where a
 * particular piece of state is kept (and therefore has to be synchronised
 * between). #xproperty_action_t does not have a separate state that is kept
 * in sync with the property value -- its state is the property value.
 *
 * For example, it might be useful to create a #xaction_t corresponding to
 * the "visible-child-name" property of a #GtkStack so that the current
 * page can be switched from a menu.  The active radio indication in the
 * menu is then directly determined from the active page of the
 * #GtkStack.
 *
 * An anti-example would be binding the "active-id" property on a
 * #GtkComboBox.  This is because the state of the combobox itself is
 * probably uninteresting and is actually being used to control
 * something else.
 *
 * Another anti-example would be to bind to the "visible-child-name"
 * property of a #GtkStack if this value is actually stored in
 * #xsettings_t.  In that case, the real source of the value is
 * #xsettings_t.  If you want a #xaction_t to control a setting stored in
 * #xsettings_t, see g_settings_create_action() instead, and possibly
 * combine its use with g_settings_bind().
 *
 * Since: 2.38
 **/
struct _GPropertyAction
{
  xobject_t     parent_instance;

  xchar_t              *name;
  xpointer_t            object;
  xparam_spec_t         *pspec;
  const xvariant_type_t *state_type;
  xboolean_t            invert_boolean;
};

/**
 * xproperty_action_t:
 *
 * This type is opaque.
 *
 * Since: 2.38
 **/

typedef xobject_class_t GPropertyActionClass;

static void g_property_action_iface_init (xaction_interface_t *iface);
G_DEFINE_TYPE_WITH_CODE (xproperty_action, g_property_action, XTYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (XTYPE_ACTION, g_property_action_iface_init))

enum
{
  PROP_NONE,
  PROP_NAME,
  PROP_PARAMETER_TYPE,
  PROP_ENABLED,
  PROP_STATE_TYPE,
  PROP_STATE,
  PROP_OBJECT,
  PROP_PROPERTY_NAME,
  PROP_INVERT_BOOLEAN
};

static xboolean_t
g_property_action_get_invert_boolean (xaction_t *action)
{
  xproperty_action_t *paction = G_PROPERTY_ACTION (action);

  return paction->invert_boolean;
}

static const xchar_t *
g_property_action_get_name (xaction_t *action)
{
  xproperty_action_t *paction = G_PROPERTY_ACTION (action);

  return paction->name;
}

static const xvariant_type_t *
g_property_action_get_parameter_type (xaction_t *action)
{
  xproperty_action_t *paction = G_PROPERTY_ACTION (action);

  return paction->pspec->value_type == XTYPE_BOOLEAN ? NULL : paction->state_type;
}

static const xvariant_type_t *
g_property_action_get_state_type (xaction_t *action)
{
  xproperty_action_t *paction = G_PROPERTY_ACTION (action);

  return paction->state_type;
}

static xvariant_t *
g_property_action_get_state_hint (xaction_t *action)
{
  xproperty_action_t *paction = G_PROPERTY_ACTION (action);

  if (paction->pspec->value_type == XTYPE_INT)
    {
      GParamSpecInt *pspec = (GParamSpecInt *)paction->pspec;
      return xvariant_new ("(ii)", pspec->minimum, pspec->maximum);
    }
  else if (paction->pspec->value_type == XTYPE_UINT)
    {
      GParamSpecUInt *pspec = (GParamSpecUInt *)paction->pspec;
      return xvariant_new ("(uu)", pspec->minimum, pspec->maximum);
    }
  else if (paction->pspec->value_type == XTYPE_FLOAT)
    {
      GParamSpecFloat *pspec = (GParamSpecFloat *)paction->pspec;
      return xvariant_new ("(dd)", (double)pspec->minimum, (double)pspec->maximum);
    }
  else if (paction->pspec->value_type == XTYPE_DOUBLE)
    {
      GParamSpecDouble *pspec = (GParamSpecDouble *)paction->pspec;
      return xvariant_new ("(dd)", pspec->minimum, pspec->maximum);
    }

  return NULL;
}

static xboolean_t
g_property_action_get_enabled (xaction_t *action)
{
  return TRUE;
}

static void
g_property_action_set_state (xproperty_action_t *paction,
                             xvariant_t        *variant)
{
  xvalue_t value = G_VALUE_INIT;

  xvalue_init (&value, paction->pspec->value_type);
  g_settings_get_mapping (&value, variant, NULL);

  if (paction->pspec->value_type == XTYPE_BOOLEAN && paction->invert_boolean)
    xvalue_set_boolean (&value, !xvalue_get_boolean (&value));

  xobject_set_property (paction->object, paction->pspec->name, &value);
  xvalue_unset (&value);
}

static void
g_property_action_change_state (xaction_t  *action,
                                xvariant_t *value)
{
  xproperty_action_t *paction = G_PROPERTY_ACTION (action);

  g_return_if_fail (xvariant_is_of_type (value, paction->state_type));

  g_property_action_set_state (paction, value);
}

static xvariant_t *
g_property_action_get_state (xaction_t *action)
{
  xproperty_action_t *paction = G_PROPERTY_ACTION (action);
  xvalue_t value = G_VALUE_INIT;
  xvariant_t *result;

  xvalue_init (&value, paction->pspec->value_type);
  xobject_get_property (paction->object, paction->pspec->name, &value);

  if (paction->pspec->value_type == XTYPE_BOOLEAN && paction->invert_boolean)
    xvalue_set_boolean (&value, !xvalue_get_boolean (&value));

  result = g_settings_set_mapping (&value, paction->state_type, NULL);
  xvalue_unset (&value);

  return xvariant_ref_sink (result);
}

static void
g_property_action_activate (xaction_t  *action,
                            xvariant_t *parameter)
{
  xproperty_action_t *paction = G_PROPERTY_ACTION (action);

  if (paction->pspec->value_type == XTYPE_BOOLEAN)
    {
      xboolean_t value;

      g_return_if_fail (paction->pspec->value_type == XTYPE_BOOLEAN && parameter == NULL);

      xobject_get (paction->object, paction->pspec->name, &value, NULL);
      value = !value;
      xobject_set (paction->object, paction->pspec->name, value, NULL);
    }
  else
    {
      g_return_if_fail (parameter != NULL && xvariant_is_of_type (parameter, paction->state_type));

      g_property_action_set_state (paction, parameter);
    }
}

static const xvariant_type_t *
g_property_action_determine_type (xparam_spec_t *pspec)
{
  if (XTYPE_IS_ENUM (pspec->value_type))
    return G_VARIANT_TYPE_STRING;

  switch (pspec->value_type)
    {
    case XTYPE_BOOLEAN:
      return G_VARIANT_TYPE_BOOLEAN;

    case XTYPE_INT:
      return G_VARIANT_TYPE_INT32;

    case XTYPE_UINT:
      return G_VARIANT_TYPE_UINT32;

    case XTYPE_DOUBLE:
    case XTYPE_FLOAT:
      return G_VARIANT_TYPE_DOUBLE;

    case XTYPE_STRING:
      return G_VARIANT_TYPE_STRING;

    default:
      g_critical ("Unable to use xproperty_action_t with property '%s::%s' of type '%s'",
                  xtype_name (pspec->owner_type), pspec->name, xtype_name (pspec->value_type));
      return NULL;
    }
}

static void
g_property_action_notify (xobject_t    *object,
                          xparam_spec_t *pspec,
                          xpointer_t    user_data)
{
  xproperty_action_t *paction = user_data;

  g_assert (object == paction->object);
  g_assert (pspec == paction->pspec);

  xobject_notify (G_OBJECT (paction), "state");
}

static void
g_property_action_set_property_name (xproperty_action_t *paction,
                                     const xchar_t     *property_name)
{
  xparam_spec_t *pspec;
  xchar_t *detailed;

  pspec = xobject_class_find_property (G_OBJECT_GET_CLASS (paction->object), property_name);

  if (pspec == NULL)
    {
      g_critical ("Attempted to use non-existent property '%s::%s' for xproperty_action_t",
                  G_OBJECT_TYPE_NAME (paction->object), property_name);
      return;
    }

  if (~pspec->flags & G_PARAM_READABLE || ~pspec->flags & G_PARAM_WRITABLE || pspec->flags & G_PARAM_CONSTRUCT_ONLY)
    {
      g_critical ("Property '%s::%s' used with xproperty_action_t must be readable, writable, and not construct-only",
                  G_OBJECT_TYPE_NAME (paction->object), property_name);
      return;
    }

  paction->pspec = pspec;

  detailed = xstrconcat ("notify::", paction->pspec->name, NULL);
  paction->state_type = g_property_action_determine_type (paction->pspec);
  xsignal_connect (paction->object, detailed, G_CALLBACK (g_property_action_notify), paction);
  g_free (detailed);
}

static void
g_property_action_set_property (xobject_t      *object,
                                xuint_t         prop_id,
                                const xvalue_t *value,
                                xparam_spec_t   *pspec)
{
  xproperty_action_t *paction = G_PROPERTY_ACTION (object);

  switch (prop_id)
    {
    case PROP_NAME:
      paction->name = xvalue_dup_string (value);
      break;

    case PROP_OBJECT:
      paction->object = xvalue_dup_object (value);
      break;

    case PROP_PROPERTY_NAME:
      g_property_action_set_property_name (paction, xvalue_get_string (value));
      break;

    case PROP_INVERT_BOOLEAN:
      paction->invert_boolean = xvalue_get_boolean (value);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
g_property_action_get_property (xobject_t    *object,
                                xuint_t       prop_id,
                                xvalue_t     *value,
                                xparam_spec_t *pspec)
{
  xaction_t *action = G_ACTION (object);

  switch (prop_id)
    {
    case PROP_NAME:
      xvalue_set_string (value, g_property_action_get_name (action));
      break;

    case PROP_PARAMETER_TYPE:
      xvalue_set_boxed (value, g_property_action_get_parameter_type (action));
      break;

    case PROP_ENABLED:
      xvalue_set_boolean (value, g_property_action_get_enabled (action));
      break;

    case PROP_STATE_TYPE:
      xvalue_set_boxed (value, g_property_action_get_state_type (action));
      break;

    case PROP_STATE:
      xvalue_take_variant (value, g_property_action_get_state (action));
      break;

    case PROP_INVERT_BOOLEAN:
      xvalue_set_boolean (value, g_property_action_get_invert_boolean (action));
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
g_property_action_finalize (xobject_t *object)
{
  xproperty_action_t *paction = G_PROPERTY_ACTION (object);

  xsignal_handlers_disconnect_by_func (paction->object, g_property_action_notify, paction);
  xobject_unref (paction->object);
  g_free (paction->name);

  G_OBJECT_CLASS (g_property_action_parent_class)
    ->finalize (object);
}

void
g_property_action_init (xproperty_action_t *property)
{
}

void
g_property_action_iface_init (xaction_interface_t *iface)
{
  iface->get_name = g_property_action_get_name;
  iface->get_parameter_type = g_property_action_get_parameter_type;
  iface->get_state_type = g_property_action_get_state_type;
  iface->get_state_hint = g_property_action_get_state_hint;
  iface->get_enabled = g_property_action_get_enabled;
  iface->get_state = g_property_action_get_state;
  iface->change_state = g_property_action_change_state;
  iface->activate = g_property_action_activate;
}

void
g_property_action_class_init (GPropertyActionClass *class)
{
  xobject_class_t *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = g_property_action_set_property;
  object_class->get_property = g_property_action_get_property;
  object_class->finalize = g_property_action_finalize;

  /**
   * xproperty_action_t:name:
   *
   * The name of the action.  This is mostly meaningful for identifying
   * the action once it has been added to a #xaction_map_t.
   *
   * Since: 2.38
   **/
  xobject_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        P_("Action Name"),
                                                        P_("The name used to invoke the action"),
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * xproperty_action_t:parameter-type:
   *
   * The type of the parameter that must be given when activating the
   * action.
   *
   * Since: 2.38
   **/
  xobject_class_install_property (object_class, PROP_PARAMETER_TYPE,
                                   g_param_spec_boxed ("parameter-type",
                                                       P_("Parameter Type"),
                                                       P_("The type of xvariant_t passed to activate()"),
                                                       XTYPE_VARIANT_TYPE,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * xproperty_action_t:enabled:
   *
   * If @action is currently enabled.
   *
   * If the action is disabled then calls to g_action_activate() and
   * g_action_change_state() have no effect.
   *
   * Since: 2.38
   **/
  xobject_class_install_property (object_class, PROP_ENABLED,
                                   g_param_spec_boolean ("enabled",
                                                         P_("Enabled"),
                                                         P_("If the action can be activated"),
                                                         TRUE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xproperty_action_t:state-type:
   *
   * The #xvariant_type_t of the state that the action has, or %NULL if the
   * action is stateless.
   *
   * Since: 2.38
   **/
  xobject_class_install_property (object_class, PROP_STATE_TYPE,
                                   g_param_spec_boxed ("state-type",
                                                       P_("State Type"),
                                                       P_("The type of the state kept by the action"),
                                                       XTYPE_VARIANT_TYPE,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * xproperty_action_t:state:
   *
   * The state of the action, or %NULL if the action is stateless.
   *
   * Since: 2.38
   **/
  xobject_class_install_property (object_class, PROP_STATE,
                                   g_param_spec_variant ("state",
                                                         P_("State"),
                                                         P_("The state the action is in"),
                                                         G_VARIANT_TYPE_ANY,
                                                         NULL,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * xproperty_action_t:object:
   *
   * The object to wrap a property on.
   *
   * The object must be a non-%NULL #xobject_t with properties.
   *
   * Since: 2.38
   **/
  xobject_class_install_property (object_class, PROP_OBJECT,
                                   g_param_spec_object ("object",
                                                        P_("Object"),
                                                        P_("The object with the property to wrap"),
                                                        XTYPE_OBJECT,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * xproperty_action_t:property-name:
   *
   * The name of the property to wrap on the object.
   *
   * The property must exist on the passed-in object and it must be
   * readable and writable (and not construct-only).
   *
   * Since: 2.38
   **/
  xobject_class_install_property (object_class, PROP_PROPERTY_NAME,
                                   g_param_spec_string ("property-name",
                                                        P_("Property name"),
                                                        P_("The name of the property to wrap"),
                                                        NULL,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * xproperty_action_t:invert-boolean:
   *
   * If %TRUE, the state of the action will be the negation of the
   * property value, provided the property is boolean.
   *
   * Since: 2.46
   */
  xobject_class_install_property (object_class, PROP_INVERT_BOOLEAN,
                                   g_param_spec_boolean ("invert-boolean",
                                                         P_("Invert boolean"),
                                                         P_("Whether to invert the value of a boolean property"),
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_STRINGS));
}

/**
 * g_property_action_new:
 * @name: the name of the action to create
 * @object: (type xobject_t.Object): the object that has the property
 *   to wrap
 * @property_name: the name of the property
 *
 * Creates a #xaction_t corresponding to the value of property
 * @property_name on @object.
 *
 * The property must be existent and readable and writable (and not
 * construct-only).
 *
 * This function takes a reference on @object and doesn't release it
 * until the action is destroyed.
 *
 * Returns: a new #xproperty_action_t
 *
 * Since: 2.38
 **/
xproperty_action_t *
g_property_action_new (const xchar_t *name,
                       xpointer_t     object,
                       const xchar_t *property_name)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (X_IS_OBJECT (object), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  return xobject_new (XTYPE_PROPERTY_ACTION,
                       "name", name,
                       "object", object,
                       "property-name", property_name,
                       NULL);
}
