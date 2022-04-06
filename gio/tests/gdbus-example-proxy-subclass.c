
#include <gio/gio.h>

/* ---------------------------------------------------------------------------------------------------- */
/* The D-Bus interface definition we want to create a xdbus_proxy_t-derived type for: */
/* ---------------------------------------------------------------------------------------------------- */

static const xchar_t introspection_xml[] =
  "<node>"
  "  <interface name='org.freedesktop.Accounts.User'>"
  "    <method name='Frobnicate'>"
  "      <arg name='flux' type='s' direction='in'/>"
  "      <arg name='baz' type='s' direction='in'/>"
  "      <arg name='result' type='s' direction='out'/>"
  "    </method>"
  "    <signal name='Changed'/>"
  "    <property name='AutomaticLogin' type='b' access='readwrite'/>"
  "    <property name='RealName' type='s' access='read'/>"
  "    <property name='UserName' type='s' access='read'/>"
  "  </interface>"
  "</node>";

/* ---------------------------------------------------------------------------------------------------- */
/* Definition of the AccountsUser type */
/* ---------------------------------------------------------------------------------------------------- */

#define ACCOUNTS_TYPE_USER         (accounts_user_get_type ())
#define ACCOUNTS_USER(o)           (XTYPE_CHECK_INSTANCE_CAST ((o), ACCOUNTS_TYPE_USER, AccountsUser))
#define ACCOUNTS_USER_CLASS(k)     (XTYPE_CHECK_CLASS_CAST((k), ACCOUNTS_TYPE_USER, AccountsUserClass))
#define ACCOUNTS_USER_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), ACCOUNTS_TYPE_USER, AccountsUserClass))
#define ACCOUNTS_IS_USER(o)        (XTYPE_CHECK_INSTANCE_TYPE ((o), ACCOUNTS_TYPE_USER))
#define ACCOUNTS_IS_USER_CLASS(k)  (XTYPE_CHECK_CLASS_TYPE ((k), ACCOUNTS_TYPE_USER))

typedef struct _AccountsUser        AccountsUser;
typedef struct _AccountsUserClass   AccountsUserClass;
typedef struct _AccountsUserPrivate AccountsUserPrivate;

struct _AccountsUser
{
  /*< private >*/
  xdbus_proxy_t parent_instance;
  AccountsUserPrivate *priv;
};

struct _AccountsUserClass
{
  /*< private >*/
  GDBusProxyClass parent_class;
  void (*changed) (AccountsUser *user);
};

xtype_t        accounts_user_get_type            (void) G_GNUC_CONST;

const xchar_t *accounts_user_get_user_name       (AccountsUser        *user);
const xchar_t *accounts_user_get_real_name       (AccountsUser        *user);
xboolean_t     accounts_user_get_automatic_login (AccountsUser        *user);

void         accounts_user_frobnicate          (AccountsUser        *user,
                                                const xchar_t         *flux,
                                                xint_t                 baz,
                                                xcancellable_t        *cancellable,
                                                xasync_ready_callback_t  callback,
                                                xpointer_t             user_data);
xchar_t       *accounts_user_frobnicate_finish   (AccountsUser        *user,
                                                xasync_result_t        *res,
                                                xerror_t             **error);
xchar_t       *accounts_user_frobnicate_sync     (AccountsUser        *user,
                                                const xchar_t         *flux,
                                                xint_t                 baz,
                                                xcancellable_t        *cancellable,
                                                xerror_t             **error);

/* ---------------------------------------------------------------------------------------------------- */
/* Implementation of the AccountsUser type */
/* ---------------------------------------------------------------------------------------------------- */

/* A more efficient approach than parsing XML is to use const static
 * xdbus_interface_info_t, xdbus_method_info_t, ... structures
 */
static xdbus_interface_info_t *
accounts_user_get_interface_info (void)
{
  static xsize_t has_info = 0;
  static xdbus_interface_info_t *info = NULL;
  if (g_once_init_enter (&has_info))
    {
      xdbus_node_info_t *introspection_data;
      introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
      info = introspection_data->interfaces[0];
      g_once_init_leave (&has_info, 1);
    }
  return info;
}

enum
{
  PROP_0,
  PROP_USER_NAME,
  PROP_REAL_NAME,
  PROP_AUTOMATIC_LOGIN,
};

enum
{
  CHANGED_SIGNAL,
  LAST_SIGNAL
};

static xuint_t signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE (AccountsUser, accounts_user, XTYPE_DBUS_PROXY)

static void
accounts_user_finalize (xobject_t *object)
{
  G_GNUC_UNUSED AccountsUser *user = ACCOUNTS_USER (object);

  if (G_OBJECT_CLASS (accounts_user_parent_class)->finalize != NULL)
    G_OBJECT_CLASS (accounts_user_parent_class)->finalize (object);
}

static void
accounts_user_init (AccountsUser *user)
{
  /* Sets the expected interface */
  g_dbus_proxy_set_interface_info (G_DBUS_PROXY (user), accounts_user_get_interface_info ());
}

static void
accounts_user_get_property (xobject_t    *object,
                            xuint_t       prop_id,
                            xvalue_t     *value,
                            xparam_spec_t *pspec)
{
  AccountsUser *user = ACCOUNTS_USER (object);

  switch (prop_id)
    {
    case PROP_USER_NAME:
      xvalue_set_string (value, accounts_user_get_user_name (user));
      break;

    case PROP_REAL_NAME:
      xvalue_set_string (value, accounts_user_get_real_name (user));
      break;

    case PROP_AUTOMATIC_LOGIN:
      xvalue_set_boolean (value, accounts_user_get_automatic_login (user));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

const xchar_t *
accounts_user_get_user_name (AccountsUser *user)
{
  xvariant_t *value;
  const xchar_t *ret;
  g_return_val_if_fail (ACCOUNTS_IS_USER (user), NULL);
  value = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (user), "UserName");
  ret = xvariant_get_string (value, NULL);
  xvariant_unref (value);
  return ret;
}

const xchar_t *
accounts_user_get_real_name (AccountsUser *user)
{
  xvariant_t *value;
  const xchar_t *ret;
  g_return_val_if_fail (ACCOUNTS_IS_USER (user), NULL);
  value = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (user), "RealName");
  ret = xvariant_get_string (value, NULL);
  xvariant_unref (value);
  return ret;
}

xboolean_t
accounts_user_get_automatic_login (AccountsUser *user)
{
  xvariant_t *value;
  xboolean_t ret;
  g_return_val_if_fail (ACCOUNTS_IS_USER (user), FALSE);
  value = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (user), "AutomaticLogin");
  ret = xvariant_get_boolean (value);
  xvariant_unref (value);
  return ret;
}

static void
accounts_user_g_signal (xdbus_proxy_t   *proxy,
                        const xchar_t  *sender_name,
                        const xchar_t  *signal_name,
                        xvariant_t     *parameters)
{
  AccountsUser *user = ACCOUNTS_USER (proxy);
  if (xstrcmp0 (signal_name, "Changed") == 0)
    g_signal_emit (user, signals[CHANGED_SIGNAL], 0);
}

static void
accounts_user_g_properties_changed (xdbus_proxy_t          *proxy,
                                    xvariant_t            *changed_properties,
                                    const xchar_t* const  *invalidated_properties)
{
  AccountsUser *user = ACCOUNTS_USER (proxy);
  xvariant_iter_t *iter;
  const xchar_t *key;

  if (changed_properties != NULL)
    {
      xvariant_get (changed_properties, "a{sv}", &iter);
      while (xvariant_iter_next (iter, "{&sv}", &key, NULL))
        {
          if (xstrcmp0 (key, "AutomaticLogin") == 0)
            xobject_notify (G_OBJECT (user), "automatic-login");
          else if (xstrcmp0 (key, "RealName") == 0)
            xobject_notify (G_OBJECT (user), "real-name");
          else if (xstrcmp0 (key, "UserName") == 0)
            xobject_notify (G_OBJECT (user), "user-name");
        }
      xvariant_iter_free (iter);
    }
}

static void
accounts_user_class_init (AccountsUserClass *klass)
{
  xobject_class_t *gobject_class;
  GDBusProxyClass *proxy_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = accounts_user_get_property;
  gobject_class->finalize = accounts_user_finalize;

  proxy_class = G_DBUS_PROXY_CLASS (klass);
  proxy_class->g_signal             = accounts_user_g_signal;
  proxy_class->g_properties_changed = accounts_user_g_properties_changed;

  xobject_class_install_property (gobject_class,
                                   PROP_USER_NAME,
                                   g_param_spec_string ("user-name",
                                                        "User Name",
                                                        "The user name of the user",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (gobject_class,
                                   PROP_REAL_NAME,
                                   g_param_spec_string ("real-name",
                                                        "Real Name",
                                                        "The real name of the user",
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));

  xobject_class_install_property (gobject_class,
                                   PROP_AUTOMATIC_LOGIN,
                                   g_param_spec_boolean ("automatic-login",
                                                         "Automatic Login",
                                                         "Whether the user is automatically logged in",
                                                         FALSE,
                                                         G_PARAM_READABLE |
                                                         G_PARAM_STATIC_STRINGS));

  signals[CHANGED_SIGNAL] = g_signal_new ("changed",
                                          ACCOUNTS_TYPE_USER,
                                          G_SIGNAL_RUN_LAST,
                                          G_STRUCT_OFFSET (AccountsUserClass, changed),
                                          NULL,
                                          NULL,
                                          g_cclosure_marshal_VOID__VOID,
                                          XTYPE_NONE,
                                          0);
}

xchar_t *
accounts_user_frobnicate_sync (AccountsUser        *user,
                               const xchar_t         *flux,
                               xint_t                 baz,
                               xcancellable_t        *cancellable,
                               xerror_t             **error)
{
  xchar_t *ret;
  xvariant_t *value;

  g_return_val_if_fail (ACCOUNTS_IS_USER (user), NULL);

  ret = NULL;

  value = g_dbus_proxy_call_sync (G_DBUS_PROXY (user),
                                  "Frobnicate",
                                  xvariant_new ("(si)",
                                                 flux,
                                                 baz),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  cancellable,
                                  error);
  if (value != NULL)
    {
      xvariant_get (value, "(s)", &ret);
      xvariant_unref (value);
    }
  return ret;
}

void
accounts_user_frobnicate (AccountsUser        *user,
                          const xchar_t         *flux,
                          xint_t                 baz,
                          xcancellable_t        *cancellable,
                          xasync_ready_callback_t  callback,
                          xpointer_t             user_data)
{
  g_return_if_fail (ACCOUNTS_IS_USER (user));
  g_dbus_proxy_call (G_DBUS_PROXY (user),
                     "Frobnicate",
                     xvariant_new ("(si)",
                                    flux,
                                    baz),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     cancellable,
                     callback,
                     user_data);
}


xchar_t *
accounts_user_frobnicate_finish (AccountsUser        *user,
                                 xasync_result_t        *res,
                                 xerror_t             **error)
{
  xchar_t *ret;
  xvariant_t *value;

  ret = NULL;
  value = g_dbus_proxy_call_finish (G_DBUS_PROXY (user), res, error);
  if (value != NULL)
    {
      xvariant_get (value, "(s)", &ret);
      xvariant_unref (value);
    }
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

xint_t
main (xint_t argc, xchar_t *argv[])
{
  return 0;
}
