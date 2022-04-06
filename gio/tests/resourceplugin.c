/*
 * Ensure the xio_module_*() symbols are exported
 * on all supported compilers without using config.h.
 * This must be done before including any GLib headers,
 * since XPL_AVAILABLE_IN_ALL, which is used to mark the
 * xio_module*() symbols, is defined to be _XPL_EXTERN,
 * which must be overridden to export the symbols.
 */
#include "modules/symbol-visibility.h"
#define _XPL_EXTERN XPL_TEST_EXPORT_SYMBOL

#include <gio/gio.h>

void
xio_module_load (xio_module_t *module)
{
}

void
xio_module_unload (xio_module_t   *module)
{
}

char **
xio_module_query (void)
{
  return NULL;
}
