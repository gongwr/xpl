#include "giotypes.h"

typedef struct _GApplicationImpl GApplicationImpl;

typedef struct
{
  xchar_t *name;

  xvariant_type_t *parameter_type;
  xboolean_t      enabled;
  xvariant_t     *state;
} RemoteActionInfo;

void                    xapplication_impl_destroy                      (GApplicationImpl   *impl);

GApplicationImpl *      xapplication_impl_register                     (xapplication_t        *application,
                                                                         const xchar_t         *appid,
                                                                         GApplicationFlags    flags,
                                                                         xaction_group_t        *exported_actions,
                                                                         xremote_action_group_t **remote_actions,
                                                                         xcancellable_t        *cancellable,
                                                                         xerror_t             **error);

void                    xapplication_impl_activate                     (GApplicationImpl   *impl,
                                                                         xvariant_t           *platform_data);

void                    xapplication_impl_open                         (GApplicationImpl   *impl,
                                                                         xfile_t             **files,
                                                                         xint_t                n_files,
                                                                         const xchar_t        *hint,
                                                                         xvariant_t           *platform_data);

int                     xapplication_impl_command_line                 (GApplicationImpl   *impl,
                                                                         const xchar_t *const *arguments,
                                                                         xvariant_t           *platform_data);

void                    xapplication_impl_flush                        (GApplicationImpl   *impl);

xdbus_connection_t *       xapplication_impl_get_dbus_connection          (GApplicationImpl   *impl);

const xchar_t *           xapplication_impl_get_dbus_object_path         (GApplicationImpl   *impl);

void                    xapplication_impl_set_busy_state               (GApplicationImpl   *impl,
                                                                         xboolean_t            busy);
