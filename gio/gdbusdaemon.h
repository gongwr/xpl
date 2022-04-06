#include <gio/gio.h>

#define XTYPE_DBUS_DAEMON (_xdbus_daemon_get_type ())
#define G_DBUS_DAEMON(o) (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_DAEMON, xdbus_daemon_t))
#define G_DBUS_DAEMON_CLASS(k) (XTYPE_CHECK_CLASS_CAST ((k), XTYPE_DBUS_DAEMON, xdbus_daemon_class_t))
#define G_DBUS_DAEMON_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DBUS_DAEMON, xdbus_daemon_class_t))
#define X_IS_DBUS_DAEMON(o) (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_DAEMON))
#define X_IS_DBUS_DAEMON_CLASS(k) (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DBUS_DAEMON))

typedef struct _GDBusDaemon xdbus_daemon_t;
typedef struct _xdbus_daemon_class xdbus_daemon_class_t;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(xdbus_daemon_t, xobject_unref)

xtype_t _xdbus_daemon_get_type (void) G_GNUC_CONST;

xdbus_daemon_t *_xdbus_daemon_new (const char *address,
				 xcancellable_t *cancellable,
				 xerror_t **error);

const char *_xdbus_daemon_get_address (xdbus_daemon_t *daemon);
