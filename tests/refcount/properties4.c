#include <glib.h>
#include <glib-object.h>

#define MY_TYPE_BADGER              (my_badger_get_type ())
#define MY_BADGER(obj)              (XTYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_BADGER, my_badger_t))
#define MY_IS_BADGER(obj)           (XTYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_BADGER))
#define MY_BADGER_CLASS(klass)      (XTYPE_CHECK_CLASS_CAST ((klass), MY_TYPE_BADGER, my_badger_class_t))
#define MY_IS_BADGER_CLASS(klass)   (XTYPE_CHECK_CLASS_TYPE ((klass), MY_TYPE_BADGER))
#define MY_BADGER_GET_CLASS(obj)    (XTYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_BADGER, my_badger_class_t))

enum {
  PROP_0,
  PROP_MAMA
};

typedef struct _my_badger my_badger_t;
typedef struct _my_badger_class my_badger_class_t;

struct _my_badger
{
  xobject_t parent_instance;

  my_badger_t * mama;
  xuint_t mama_notify_count;
};

struct _my_badger_class
{
  xobject_class_t parent_class;
};

static xtype_t my_badger_get_type (void);
XDEFINE_TYPE (my_badger, my_badger, XTYPE_OBJECT)

static void my_badger_dispose (xobject_t * object);

static void my_badger_get_property (xobject_t    *object,
				    xuint_t       prop_id,
				    xvalue_t     *value,
				    xparam_spec_t *pspec);
static void my_badger_set_property (xobject_t      *object,
				    xuint_t         prop_id,
				    const xvalue_t *value,
				    xparam_spec_t   *pspec);

static void my_badger_mama_notify (xobject_t    *object,
				   xparam_spec_t *pspec);

static void
my_badger_class_init (my_badger_class_t * klass)
{
  xobject_class_t *xobject_class;

  xobject_class = (xobject_class_t *) klass;

  xobject_class->dispose = my_badger_dispose;

  xobject_class->get_property = my_badger_get_property;
  xobject_class->set_property = my_badger_set_property;

  xobject_class_install_property (xobject_class,
				   PROP_MAMA,
				   xparam_spec_object ("mama",
							NULL,
							NULL,
							MY_TYPE_BADGER,
							XPARAM_READWRITE));
}

static void
my_badger_init (my_badger_t * self)
{
  xsignal_connect (self, "notify::mama", G_CALLBACK (my_badger_mama_notify),
      NULL);
}

static void
my_badger_dispose (xobject_t * object)
{
  my_badger_t * self;

  self = MY_BADGER (object);

  if (self->mama != NULL)
    {
      xobject_unref (self->mama);
      self->mama = NULL;
    }

  XOBJECT_CLASS (my_badger_parent_class)->dispose (object);
}

static void
my_badger_get_property (xobject_t    *object,
			xuint_t        prop_id,
			xvalue_t     *value,
			xparam_spec_t *pspec)
{
  my_badger_t *self;

  self = MY_BADGER (object);

  switch (prop_id)
    {
    case PROP_MAMA:
      xvalue_set_object (value, self->mama);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
my_badger_set_property (xobject_t      *object,
			xuint_t         prop_id,
			const xvalue_t *value,
			xparam_spec_t   *pspec)
{
  my_badger_t *self;

  self = MY_BADGER (object);

  switch (prop_id)
    {
    case PROP_MAMA:
      if (self->mama)
	xobject_unref (self->mama);
      self->mama = xvalue_dup_object (value);
      if (self->mama)
	xobject_set (self->mama, "mama", NULL, NULL); /* another notify */
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
my_badger_mama_notify (xobject_t    *object,
                       xparam_spec_t *pspec)
{
  my_badger_t *self;

  self = MY_BADGER (object);

  self->mama_notify_count++;
}

int
main (int argc, char **argv)
{
  my_badger_t * badger1, * badger2;
  xpointer_t test;

  g_print ("START: %s\n", argv[0]);
  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | g_log_set_always_fatal (G_LOG_FATAL_MASK));

  badger1 = xobject_new (MY_TYPE_BADGER, NULL);
  badger2 = xobject_new (MY_TYPE_BADGER, NULL);

  xobject_set (badger1, "mama", badger2, NULL);
  g_assert_cmpuint (badger1->mama_notify_count, ==, 1);
  g_assert_cmpuint (badger2->mama_notify_count, ==, 1);
  xobject_get (badger1, "mama", &test, NULL);
  xassert (test == badger2);
  xobject_unref (test);

  xobject_unref (badger1);
  xobject_unref (badger2);

  return 0;
}
