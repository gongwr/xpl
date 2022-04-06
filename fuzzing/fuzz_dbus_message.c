#include "fuzz.h"

static const GDBusCapabilityFlags flags = G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING;

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  xssize_t bytes;
  xdbus_message_t *msg = NULL;
  guchar *blob = NULL;
  xsize_t msg_size;

  fuzz_set_logging_func ();

  bytes = xdbus_message_bytes_needed ((guchar*) data, size, NULL);
  if (bytes <= 0)
    return 0;

  msg = xdbus_message_new_from_blob ((guchar*) data, size, flags, NULL);
  if (msg == NULL)
    return 0;

  blob = xdbus_message_to_blob (msg, &msg_size, flags, NULL);

  g_free (blob);
  xobject_unref (msg);
  return 0;
}
