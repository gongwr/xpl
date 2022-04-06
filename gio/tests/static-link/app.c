#include <gio/gio.h>

int main(int argc, char *argv[])
{
  xapplication_t *app = xapplication_new (NULL, 0);
  xobject_unref (app);
  return 0;
}
