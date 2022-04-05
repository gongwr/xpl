#include <gio/gio.h>

#define XTYPE_DBUS_DAEMON (_g_dbus_daemon_get_type ())
#define G_DBUS_DAEMON(o) (XTYPE_CHECK_INSTANCE_CAST ((o), XTYPE_DBUS_DAEMON, GDBusDaemon))
#define G_DBUS_DAEMON_CLASS(k) (XTYPE_CHECK_CLASS_CAST ((k), XTYPE_DBUS_DAEMON, GDBusDaemonClass))
#define G_DBUS_DAEMON_GET_CLASS(o) (XTYPE_INSTANCE_GET_CLASS ((o), XTYPE_DBUS_DAEMON, GDBusDaemonClass))
#define X_IS_DBUS_DAEMON(o) (XTYPE_CHECK_INSTANCE_TYPE ((o), XTYPE_DBUS_DAEMON))
#define X_IS_DBUS_DAEMON_CLASS(k) (XTYPE_CHECK_CLASS_TYPE ((k), XTYPE_DBUS_DAEMON))

typedef struct _GDBusDaemon GDBusDaemon;
typedef struct _GDBusDaemonClass GDBusDaemonClass;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GDBusDaemon, g_object_unref)

xtype_t _g_dbus_daemon_get_type (void) G_GNUC_CONST;

GDBusDaemon *_g_dbus_daemon_new (const char *address,
				 xcancellable_t *cancellable,
				 xerror_t **error);

const char *_g_dbus_daemon_get_address (GDBusDaemon *daemon);
