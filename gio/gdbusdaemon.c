#include "config.h"

#include <string.h>
#include <stdlib.h>

#include <gstdio.h>
#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>
#include "gdbusdaemon.h"
#include "glibintl.h"

#include "gdbus-daemon-generated.h"

#define DBUS_SERVICE_NAME  "org.freedesktop.DBus"

/* Owner flags */
#define DBUS_NAME_FLAG_ALLOW_REPLACEMENT 0x1 /**< Allow another service to become the primary owner if requested */
#define DBUS_NAME_FLAG_REPLACE_EXISTING  0x2 /**< Request to replace the current primary owner */
#define DBUS_NAME_FLAG_DO_NOT_QUEUE      0x4 /**< If we can not become the primary owner do not place us in the queue */

/* Replies to request for a name */
#define DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER  1 /**< Service has become the primary owner of the requested name */
#define DBUS_REQUEST_NAME_REPLY_IN_QUEUE       2 /**< Service could not become the primary owner and has been placed in the queue */
#define DBUS_REQUEST_NAME_REPLY_EXISTS         3 /**< Service is already in the queue */
#define DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER  4 /**< Service is already the primary owner */

/* Replies to releasing a name */
#define DBUS_RELEASE_NAME_REPLY_RELEASED        1 /**< Service was released from the given name */
#define DBUS_RELEASE_NAME_REPLY_NON_EXISTENT    2 /**< The given name does not exist on the bus */
#define DBUS_RELEASE_NAME_REPLY_NOT_OWNER       3 /**< Service is not an owner of the given name */

/* Replies to service starts */
#define DBUS_START_REPLY_SUCCESS         1 /**< Service was auto started */
#define DBUS_START_REPLY_ALREADY_RUNNING 2 /**< Service was already running */

#define IDLE_TIMEOUT_MSEC 3000

struct _GDBusDaemon
{
  _GFreedesktopDBusSkeleton parent_instance;

  xchar_t *address;
  xuint_t timeout;
  xchar_t *tmpdir;
  xdbus_server_t *server;
  xchar_t *guid;
  xhashtable_t *clients;
  xhashtable_t *names;
  xuint32_t next_major_id;
  xuint32_t next_minor_id;
};

struct _GDBusDaemonClass
{
  _GFreedesktopDBusSkeletonClass parent_class;
};

enum {
  PROP_0,
  PROP_ADDRESS,
};

enum
{
  SIGNAL_IDLE_TIMEOUT,
  NR_SIGNALS
};

static xuint_t g_dbus_daemon_signals[NR_SIGNALS];


static void initable_iface_init      (xinitable_iface_t         *initable_iface);
static void g_dbus_daemon_iface_init (_GFreedesktopDBusIface *iface);

#define g_dbus_daemon_get_type _g_dbus_daemon_get_type
G_DEFINE_TYPE_WITH_CODE (GDBusDaemon, g_dbus_daemon, _XTYPE_FREEDESKTOP_DBUS_SKELETON,
			 G_IMPLEMENT_INTERFACE (XTYPE_INITABLE, initable_iface_init)
			 G_IMPLEMENT_INTERFACE (_XTYPE_FREEDESKTOP_DBUS, g_dbus_daemon_iface_init))

typedef struct {
  GDBusDaemon *daemon;
  char *id;
  xdbus_connection_t *connection;
  xlist_t *matches;
} Client;

typedef struct {
  Client *client;
  xuint32_t flags;
} NameOwner;

typedef struct {
  int refcount;

  char *name;
  GDBusDaemon *daemon;

  NameOwner *owner;
  xlist_t *queue;
} Name;

enum {
  MATCH_ELEMENT_TYPE,
  MATCH_ELEMENT_SENDER,
  MATCH_ELEMENT_INTERFACE,
  MATCH_ELEMENT_MEMBER,
  MATCH_ELEMENT_PATH,
  MATCH_ELEMENT_PATH_NAMESPACE,
  MATCH_ELEMENT_DESTINATION,
  MATCH_ELEMENT_ARG0NAMESPACE,
  MATCH_ELEMENT_EAVESDROP,
  MATCH_ELEMENT_ARGN,
  MATCH_ELEMENT_ARGNPATH,
};

typedef struct {
  xuint16_t type;
  xuint16_t arg;
  char *value;
} MatchElement;

typedef struct {
  xboolean_t eavesdrop;
  GDBusMessageType type;
  int n_elements;
  MatchElement *elements;
} Match;

static xdbus_message_t *filter_function   (xdbus_connection_t *connection,
					xdbus_message_t    *message,
					xboolean_t         incoming,
					xpointer_t         user_data);
static void          connection_closed (xdbus_connection_t *connection,
					xboolean_t         remote_peer_vanished,
					xerror_t          *error,
					Client          *client);

static NameOwner *
name_owner_new (Client *client, xuint32_t flags)
{
  NameOwner *owner;

  owner = g_new0 (NameOwner, 1);
  owner->client = client;
  owner->flags = flags;
  return owner;
}

static void
name_owner_free (NameOwner *owner)
{
  g_free (owner);
}

static Name *
name_new (GDBusDaemon *daemon, const char *str)
{
  Name *name;

  name = g_new0 (Name, 1);
  name->refcount = 1;
  name->daemon = daemon;
  name->name = xstrdup (str);

  xhash_table_insert (daemon->names, name->name, name);

  return name;
}

static Name *
name_ref (Name *name)
{
  g_assert (name->refcount > 0);
  name->refcount++;
  return name;
}

static void
name_unref (Name *name)
{
  g_assert (name->refcount > 0);
  if (--name->refcount == 0)
    {
      xhash_table_remove (name->daemon->names, name->name);
      g_free (name->name);
      g_free (name);
    }
}

static Name *
name_ensure (GDBusDaemon *daemon, const char *str)
{
  Name *name;

  name = xhash_table_lookup (daemon->names, str);

  if (name != NULL)
    return name_ref (name);
  return name_new (daemon, str);
}

static Name *
name_lookup (GDBusDaemon *daemon, const char *str)
{
  return xhash_table_lookup (daemon->names, str);
}

static xboolean_t
is_key (const char *key_start, const char *key_end, const char *value)
{
  xsize_t len = strlen (value);

  g_assert (key_end >= key_start);
  if (len != (xsize_t) (key_end - key_start))
    return FALSE;

  return strncmp (key_start, value, len) == 0;
}

static xboolean_t
parse_key (MatchElement *element, const char *key_start, const char *key_end)
{
  xboolean_t res = TRUE;

  if (is_key (key_start, key_end, "type"))
    {
      element->type = MATCH_ELEMENT_TYPE;
    }
  else if (is_key (key_start, key_end, "sender"))
    {
      element->type = MATCH_ELEMENT_SENDER;
    }
  else if (is_key (key_start, key_end, "interface"))
    {
      element->type = MATCH_ELEMENT_INTERFACE;
    }
  else if (is_key (key_start, key_end, "member"))
    {
      element->type = MATCH_ELEMENT_MEMBER;
    }
  else if (is_key (key_start, key_end, "path"))
    {
      element->type = MATCH_ELEMENT_PATH;
    }
  else if (is_key (key_start, key_end, "path_namespace"))
    {
      element->type = MATCH_ELEMENT_PATH_NAMESPACE;
    }
  else if (is_key (key_start, key_end, "destination"))
    {
      element->type = MATCH_ELEMENT_DESTINATION;
    }
  else if (is_key (key_start, key_end, "arg0namespace"))
    {
      element->type = MATCH_ELEMENT_ARG0NAMESPACE;
    }
  else if (is_key (key_start, key_end, "eavesdrop"))
    {
      element->type = MATCH_ELEMENT_EAVESDROP;
    }
  else if (key_end - key_start > 3 && is_key (key_start, key_start + 3, "arg"))
    {
      const char *digits = key_start + 3;
      const char *end_digits = digits;

      while (end_digits < key_end && g_ascii_isdigit (*end_digits))
	end_digits++;

      if (end_digits == key_end) /* argN */
	{
	  element->type = MATCH_ELEMENT_ARGN;
	  element->arg = atoi (digits);
	}
      else if (is_key (end_digits, key_end, "path")) /* argNpath */
	{
	  element->type = MATCH_ELEMENT_ARGNPATH;
	  element->arg = atoi (digits);
	}
      else
	res = FALSE;
    }
  else
    res = FALSE;

  return res;
}

static const char *
parse_value (MatchElement *element, const char *s)
{
  char quote_char;
  xstring_t *value;

  value = xstring_new ("");

  quote_char = 0;

  for (;*s; s++)
    {
      if (quote_char == 0)
	{
	  switch (*s)
	    {
	    case '\'':
	      quote_char = '\'';
	      break;

	    case ',':
	      s++;
	      goto out;

	    case '\\':
	      quote_char = '\\';
	      break;

	    default:
	      xstring_append_c (value, *s);
	      break;
	    }
	}
      else if (quote_char == '\\')
	{
	  /* \ only counts as an escape if escaping a quote mark */
	  if (*s != '\'')
	    xstring_append_c (value, '\\');

	  xstring_append_c (value, *s);
	  quote_char = 0;
	}
      else /* quote_char == ' */
	{
	  if (*s == '\'')
	    quote_char = 0;
	  else
	    xstring_append_c (value, *s);
	}
    }

 out:

  if (quote_char == '\\')
    xstring_append_c (value, '\\');
  else if (quote_char == '\'')
    {
      xstring_free (value, TRUE);
      return NULL;
    }

  element->value = xstring_free (value, FALSE);
  return s;
}

static Match *
match_new (const char *str)
{
  Match *match;
  xarray_t *elements;
  const char *p;
  const char *key_start;
  const char *key_end;
  MatchElement element;
  xboolean_t eavesdrop;
  GDBusMessageType type;
  xsize_t i;

  eavesdrop = FALSE;
  type = G_DBUS_MESSAGE_TYPE_INVALID;
  elements = g_array_new (TRUE, TRUE, sizeof (MatchElement));

  p = str;

  while (*p != 0)
    {
      memset (&element, 0, sizeof (element));

      /* Skip initial whitespace */
      while (*p && g_ascii_isspace (*p))
	p++;

      key_start = p;

      /* Read non-whitespace non-equals chars */
      while (*p && *p != '=' && !g_ascii_isspace (*p))
	p++;

      key_end = p;

      /* Skip any whitespace after key */
      while (*p && g_ascii_isspace (*p))
	p++;

      if (key_start == key_end)
	continue; /* Allow trailing whitespace */

      if (*p != '=')
	goto error;

      ++p;

      if (!parse_key (&element, key_start, key_end))
	goto error;

      p = parse_value (&element, p);
      if (p == NULL)
	goto error;

      if (element.type == MATCH_ELEMENT_EAVESDROP)
	{
	  if (strcmp (element.value, "true") == 0)
	    eavesdrop = TRUE;
	  else if (strcmp (element.value, "false") == 0)
	    eavesdrop = FALSE;
	  else
	    {
	      g_free (element.value);
	      goto error;
	    }
	  g_free (element.value);
	}
      else if (element.type == MATCH_ELEMENT_TYPE)
	{
	  if (strcmp (element.value, "signal") == 0)
	    type = G_DBUS_MESSAGE_TYPE_SIGNAL;
	  else if (strcmp (element.value, "method_call") == 0)
	    type = G_DBUS_MESSAGE_TYPE_METHOD_CALL;
	  else if (strcmp (element.value, "method_return") == 0)
	    type = G_DBUS_MESSAGE_TYPE_METHOD_RETURN;
	  else if (strcmp (element.value, "error") == 0)
	    type = G_DBUS_MESSAGE_TYPE_ERROR;
	  else
	    {
	      g_free (element.value);
	      goto error;
	    }
	  g_free (element.value);
	}
      else
	g_array_append_val (elements, element);
    }

  match = g_new0 (Match, 1);
  match->n_elements = elements->len;
  match->elements = (MatchElement *)g_array_free (elements, FALSE);
  match->eavesdrop = eavesdrop;
  match->type = type;

  return match;

 error:
  for (i = 0; i < elements->len; i++)
    g_free (g_array_index (elements, MatchElement, i).value);
  g_array_free (elements, TRUE);
  return NULL;
}

static void
match_free (Match *match)
{
  int i;
  for (i = 0; i < match->n_elements; i++)
    g_free (match->elements[i].value);
  g_free (match->elements);
  g_free (match);
}

static xboolean_t
match_equal (Match *a, Match *b)
{
  int i;

  if (a->eavesdrop != b->eavesdrop)
    return FALSE;
  if (a->type != b->type)
    return FALSE;
 if (a->n_elements != b->n_elements)
    return FALSE;
  for (i = 0; i < a->n_elements; i++)
    {
      if (a->elements[i].type != b->elements[i].type ||
	  a->elements[i].arg != b->elements[i].arg ||
	  strcmp (a->elements[i].value, b->elements[i].value) != 0)
	return FALSE;
    }
  return TRUE;
}

static const xchar_t *
message_get_argN (xdbus_message_t *message, int n, xboolean_t allow_path)
{
  const xchar_t *ret;
  xvariant_t *body;

  ret = NULL;

  body = xdbus_message_get_body (message);

  if (body != NULL && xvariant_is_of_type (body, G_VARIANT_TYPE_TUPLE))
    {
      xvariant_t *item;
      item = xvariant_get_child_value (body, n);
      if (xvariant_is_of_type (item, G_VARIANT_TYPE_STRING) ||
	  (allow_path && xvariant_is_of_type (item, G_VARIANT_TYPE_OBJECT_PATH)))
	ret = xvariant_get_string (item, NULL);
      xvariant_unref (item);
    }

  return ret;
}

enum {
  CHECK_TYPE_STRING,
  CHECK_TYPE_NAME,
  CHECK_TYPE_PATH_PREFIX,
  CHECK_TYPE_PATH_RELATED,
  CHECK_TYPE_NAMESPACE_PREFIX
};

static xboolean_t
match_matches (GDBusDaemon *daemon,
	       Match *match, xdbus_message_t *message,
	       xboolean_t has_destination)
{
  MatchElement *element;
  Name *name;
  int i, len, len2;
  const char *value;
  int check_type;

  if (has_destination && !match->eavesdrop)
    return FALSE;

  if (match->type != G_DBUS_MESSAGE_TYPE_INVALID &&
      xdbus_message_get_message_type (message) != match->type)
    return FALSE;

  for (i = 0; i < match->n_elements; i++)
    {
      element = &match->elements[i];
      check_type = CHECK_TYPE_STRING;
      switch (element->type)
	{
	case MATCH_ELEMENT_SENDER:
	  check_type = CHECK_TYPE_NAME;
	  value = xdbus_message_get_sender (message);
	  if (value == NULL)
	    value = DBUS_SERVICE_NAME;
	  break;
	case MATCH_ELEMENT_DESTINATION:
	  check_type = CHECK_TYPE_NAME;
	  value = xdbus_message_get_destination (message);
	  break;
	case MATCH_ELEMENT_INTERFACE:
	  value = xdbus_message_get_interface (message);
	  break;
	case MATCH_ELEMENT_MEMBER:
	  value = xdbus_message_get_member (message);
	  break;
	case MATCH_ELEMENT_PATH:
	  value = xdbus_message_get_path (message);
	  break;
	case MATCH_ELEMENT_PATH_NAMESPACE:
	  check_type = CHECK_TYPE_PATH_PREFIX;
	  value = xdbus_message_get_path (message);
	  break;
	case MATCH_ELEMENT_ARG0NAMESPACE:
	  check_type = CHECK_TYPE_NAMESPACE_PREFIX;
	  value = message_get_argN (message, 0, FALSE);
	  break;
	case MATCH_ELEMENT_ARGN:
	  value = message_get_argN (message, element->arg, FALSE);
	  break;
	case MATCH_ELEMENT_ARGNPATH:
	  check_type = CHECK_TYPE_PATH_RELATED;
	  value = message_get_argN (message, element->arg, TRUE);
	  break;
	default:
	case MATCH_ELEMENT_TYPE:
	case MATCH_ELEMENT_EAVESDROP:
	  g_assert_not_reached ();
	}

      if (value == NULL)
	return FALSE;

      switch (check_type)
	{
	case CHECK_TYPE_STRING:
	  if (strcmp (element->value, value) != 0)
	    return FALSE;
	  break;
	case CHECK_TYPE_NAME:
	  name = name_lookup (daemon, element->value);
	  if (name != NULL && name->owner != NULL)
	    {
	      if (strcmp (name->owner->client->id, value) != 0)
		return FALSE;
	    }
	  else if (strcmp (element->value, value) != 0)
	    return FALSE;
	  break;
	case CHECK_TYPE_PATH_PREFIX:
	  len = strlen (element->value);

	  /* Make sure to handle the case of element->value == '/'. */
	  if (len == 1)
	    break;

	  /* Fail if there's no prefix match, or if the prefix match doesn't
	   * finish at the end of or at a separator in the @value. */
	  if (!xstr_has_prefix (value, element->value))
	    return FALSE;
	  if (value[len] != 0 && value[len] != '/')
	    return FALSE;

	  break;
	case CHECK_TYPE_PATH_RELATED:
	  len = strlen (element->value);
	  len2 = strlen (value);

	  if (!(strcmp (value, element->value) == 0 ||
		(len2 > 0 && value[len2-1] == '/' && xstr_has_prefix (element->value, value)) ||
		(len > 0 && element->value[len-1] == '/' && xstr_has_prefix (value, element->value))))
	    return FALSE;
	  break;
	case CHECK_TYPE_NAMESPACE_PREFIX:
	  len = strlen (element->value);
	  if (!(xstr_has_prefix (value, element->value) &&
		(value[len] == 0 || value[len] == '.')))
	    return FALSE;
	  break;
	default:
	  g_assert_not_reached ();
	}
    }

  return TRUE;
}

static void
broadcast_message (GDBusDaemon *daemon,
		   xdbus_message_t *message,
		   xboolean_t has_destination,
		   xboolean_t preserve_serial,
		   Client *not_to)
{
  xlist_t *clients, *l, *ll;
  xdbus_message_t *copy;

  clients = xhash_table_get_values (daemon->clients);
  for (l = clients; l != NULL; l = l->next)
    {
      Client *client = l->data;

      if (client == not_to)
	continue;

      for (ll = client->matches; ll != NULL; ll = ll->next)
	{
	  Match *match = ll->data;

	  if (match_matches (daemon, match, message, has_destination))
	    break;
	}

      if (ll != NULL)
	{
	  copy = xdbus_message_copy (message, NULL);
	  if (copy)
	    {
	      g_dbus_connection_send_message (client->connection, copy,
					      preserve_serial?G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL:0, NULL, NULL);
	      xobject_unref (copy);
	    }
	}
    }

  xlist_free (clients);
}

static void
send_name_owner_changed (GDBusDaemon *daemon,
			 const char *name,
			 const char *old_owner,
			 const char *new_owner)
{
  xdbus_message_t *signal_message;

  signal_message = xdbus_message_new_signal ("/org/freedesktop/DBus",
					      "org.freedesktop.DBus",
					      "NameOwnerChanged");
  xdbus_message_set_body (signal_message,
			   xvariant_new ("(sss)",
					  name,
					  old_owner ? old_owner : "",
					  new_owner ? new_owner : ""));

  broadcast_message (daemon, signal_message, FALSE, FALSE, NULL);
  xobject_unref (signal_message);

}

static xboolean_t
name_unqueue_owner (Name *name, Client *client)
{
  xlist_t *l;

  for (l = name->queue; l != NULL; l = l->next)
    {
      NameOwner *other = l->data;

      if (other->client == client)
	{
	  name->queue = xlist_delete_link (name->queue, l);
	  name_unref (name);
	  name_owner_free (other);
	  return TRUE;
	}
    }

  return FALSE;
}

static void
name_replace_owner (Name *name, NameOwner *owner)
{
  GDBusDaemon *daemon = name->daemon;
  NameOwner *old_owner;
  char *old_name = NULL, *new_name = NULL;
  Client *new_client = NULL;

  if (owner)
    new_client = owner->client;

  name_ref (name);

  old_owner = name->owner;
  if (old_owner)
    {
      Client *old_client = old_owner->client;

      g_assert (old_owner->client != new_client);

      g_dbus_connection_emit_signal (old_client->connection,
				     NULL, "/org/freedesktop/DBus",
				     "org.freedesktop.DBus", "NameLost",
				     xvariant_new ("(s)",
						    name->name), NULL);

      old_name = xstrdup (old_client->id);
      if (old_owner->flags & DBUS_NAME_FLAG_DO_NOT_QUEUE)
	{
	  name_unref (name);
	  name_owner_free (old_owner);
	}
      else
	name->queue = xlist_prepend (name->queue, old_owner);
    }

  name->owner = owner;
  if (owner)
    {
      name_unqueue_owner (name, owner->client);
      name_ref (name);
      new_name = new_client->id;

      g_dbus_connection_emit_signal (new_client->connection,
				     NULL, "/org/freedesktop/DBus",
				     "org.freedesktop.DBus", "NameAcquired",
				     xvariant_new ("(s)",
						    name->name), NULL);
    }

  send_name_owner_changed (daemon, name->name, old_name, new_name);

  g_free (old_name);

  name_unref (name);
}

static void
name_release_owner (Name *name)
{
  NameOwner *next_owner = NULL;

  name_ref (name);

  /* Will someone else take over? */
  if (name->queue)
    {
      next_owner = name->queue->data;
      name_unref (name);
      name->queue = xlist_delete_link (name->queue, name->queue);
    }

  name->owner->flags |= DBUS_NAME_FLAG_DO_NOT_QUEUE;
  name_replace_owner (name, next_owner);

  name_unref (name);
}

static void
name_queue_owner (Name *name, NameOwner *owner)
{
  xlist_t *l;

  for (l = name->queue; l != NULL; l = l->next)
    {
      NameOwner *other = l->data;

      if (other->client == owner->client)
	{
	  other->flags = owner->flags;
	  name_owner_free (owner);
	  return;
	}
    }

  name->queue = xlist_append (name->queue, owner);
  name_ref (name);
}

static Client *
client_new (GDBusDaemon *daemon, xdbus_connection_t *connection)
{
  Client *client;
  xerror_t *error = NULL;

  client = g_new0 (Client, 1);
  client->daemon = daemon;
  client->id = xstrdup_printf (":%d.%d", daemon->next_major_id, daemon->next_minor_id);
  client->connection = xobject_ref (connection);

  if (daemon->next_minor_id == G_MAXUINT32)
    {
      daemon->next_minor_id = 0;
      daemon->next_major_id++;
    }
  else
    daemon->next_minor_id++;

  xobject_set_data (G_OBJECT (connection), "client", client);
  xhash_table_insert (daemon->clients, client->id, client);

  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (daemon), connection,
				    "/org/freedesktop/DBus", &error);
  g_assert_no_error (error);

  g_signal_connect (connection, "closed", G_CALLBACK (connection_closed), client);
  g_dbus_connection_add_filter (connection,
				filter_function,
				client, NULL);

  send_name_owner_changed (daemon, client->id, NULL, client->id);

  return client;
}

static void
client_free (Client *client)
{
  GDBusDaemon *daemon = client->daemon;
  xlist_t *l, *names;

  g_dbus_interface_skeleton_unexport_from_connection (G_DBUS_INTERFACE_SKELETON (daemon),
						      client->connection);

  xhash_table_remove (daemon->clients, client->id);

  names = xhash_table_get_values (daemon->names);
  for (l = names; l != NULL; l = l->next)
    {
      Name *name = l->data;

      name_ref (name);

      if (name->owner && name->owner->client == client)
	name_release_owner (name);

      name_unqueue_owner (name, client);

      name_unref (name);
    }
  xlist_free (names);

  send_name_owner_changed (daemon, client->id, client->id, NULL);

  xobject_unref (client->connection);

  for (l = client->matches; l != NULL; l = l->next)
    match_free (l->data);
  xlist_free (client->matches);

  g_free (client->id);
  g_free (client);
}

static xboolean_t
idle_timeout_cb (xpointer_t user_data)
{
  GDBusDaemon *daemon = user_data;

  daemon->timeout = 0;

  g_signal_emit (daemon,
		 g_dbus_daemon_signals[SIGNAL_IDLE_TIMEOUT],
		 0);

  return G_SOURCE_REMOVE;
}

static void
connection_closed (xdbus_connection_t *connection,
		   xboolean_t remote_peer_vanished,
		   xerror_t *error,
		   Client *client)
{
  GDBusDaemon *daemon = client->daemon;

  client_free (client);

  if (xhash_table_size (daemon->clients) == 0)
    daemon->timeout = g_timeout_add (IDLE_TIMEOUT_MSEC,
				     idle_timeout_cb,
				     daemon);
}

static xboolean_t
handle_add_match (_GFreedesktopDBus *object,
		  xdbus_method_invocation_t *invocation,
		  const xchar_t *arg_rule)
{
  Client *client = xobject_get_data (G_OBJECT (xdbus_method_invocation_get_connection (invocation)), "client");
  Match *match;

  match = match_new (arg_rule);

  if (match == NULL)
    xdbus_method_invocation_return_error (invocation,
					   G_DBUS_ERROR, G_DBUS_ERROR_MATCH_RULE_INVALID,
					   "Invalid rule: %s", arg_rule);
  else
    {
      client->matches = xlist_prepend (client->matches, match);
      _g_freedesktop_dbus_complete_add_match (object, invocation);
    }
  return TRUE;
}

static xboolean_t
handle_get_connection_selinux_security_context (_GFreedesktopDBus *object,
						xdbus_method_invocation_t *invocation,
						const xchar_t *arg_name)
{
  xdbus_method_invocation_return_error (invocation,
					 G_DBUS_ERROR, G_DBUS_ERROR_SELINUX_SECURITY_CONTEXT_UNKNOWN,
					 "selinux context not supported");
  _g_freedesktop_dbus_complete_get_connection_selinux_security_context (object, invocation, "");
  return TRUE;
}

static xboolean_t
handle_get_connection_unix_process_id (_GFreedesktopDBus *object,
				       xdbus_method_invocation_t *invocation,
				       const xchar_t *arg_name)
{
  xdbus_method_invocation_return_error (invocation,
					 G_DBUS_ERROR, G_DBUS_ERROR_UNIX_PROCESS_ID_UNKNOWN,
					 "connection pid not supported");
  return TRUE;
}

static xboolean_t
handle_get_connection_unix_user (_GFreedesktopDBus *object,
				 xdbus_method_invocation_t *invocation,
				 const xchar_t *arg_name)
{
  xdbus_method_invocation_return_error (invocation,
					 G_DBUS_ERROR, G_DBUS_ERROR_UNIX_PROCESS_ID_UNKNOWN,
					 "connection user not supported");
  return TRUE;
}

static xboolean_t
handle_get_id (_GFreedesktopDBus *object,
	       xdbus_method_invocation_t *invocation)
{
  GDBusDaemon *daemon = G_DBUS_DAEMON (object);
  _g_freedesktop_dbus_complete_get_id (object, invocation,
				       daemon->guid);
  return TRUE;
}

static xboolean_t
handle_get_name_owner (_GFreedesktopDBus *object,
		       xdbus_method_invocation_t *invocation,
		       const xchar_t *arg_name)
{
  GDBusDaemon *daemon = G_DBUS_DAEMON (object);
  Name *name;

  if (strcmp (arg_name, DBUS_SERVICE_NAME) == 0)
    {
      _g_freedesktop_dbus_complete_get_name_owner (object, invocation, DBUS_SERVICE_NAME);
      return TRUE;
    }

  if (arg_name[0] == ':')
    {
      if (xhash_table_lookup (daemon->clients, arg_name) == NULL)
	xdbus_method_invocation_return_error (invocation,
					       G_DBUS_ERROR, G_DBUS_ERROR_NAME_HAS_NO_OWNER,
					       "Could not get owner of name '%s': no such name", arg_name);
      else
	_g_freedesktop_dbus_complete_get_name_owner (object, invocation, arg_name);
      return TRUE;
    }

  name = name_lookup (daemon, arg_name);
  if (name == NULL || name->owner == NULL)
    {
      xdbus_method_invocation_return_error (invocation,
					     G_DBUS_ERROR, G_DBUS_ERROR_NAME_HAS_NO_OWNER,
					     "Could not get owner of name '%s': no such name", arg_name);
      return TRUE;
    }

  _g_freedesktop_dbus_complete_get_name_owner (object, invocation, name->owner->client->id);
  return TRUE;
}

static xboolean_t
handle_hello (_GFreedesktopDBus *object,
	      xdbus_method_invocation_t *invocation)
{
  Client *client = xobject_get_data (G_OBJECT (xdbus_method_invocation_get_connection (invocation)), "client");
  _g_freedesktop_dbus_complete_hello (object, invocation, client->id);

  g_dbus_connection_emit_signal (client->connection,
				 NULL, "/org/freedesktop/DBus",
				 "org.freedesktop.DBus", "NameAcquired",
				 xvariant_new ("(s)",
						client->id), NULL);

  return TRUE;
}

static xboolean_t
handle_list_activatable_names (_GFreedesktopDBus *object,
			       xdbus_method_invocation_t *invocation)
{
  const char *names[] = { NULL };

  _g_freedesktop_dbus_complete_list_activatable_names (object,
						       invocation,
						       names);
  return TRUE;
}

static xboolean_t
handle_list_names (_GFreedesktopDBus *object,
		   xdbus_method_invocation_t *invocation)
{
  GDBusDaemon *daemon = G_DBUS_DAEMON (object);
  xptr_array_t *array;
  xlist_t *clients, *names, *l;

  array = xptr_array_new ();

  clients = xhash_table_get_values (daemon->clients);
  for (l = clients; l != NULL; l = l->next)
    {
      Client *client = l->data;

      xptr_array_add (array, client->id);
    }

  xlist_free (clients);

  names = xhash_table_get_values (daemon->names);
  for (l = names; l != NULL; l = l->next)
    {
      Name *name = l->data;

      xptr_array_add (array, name->name);
    }

  xlist_free (names);

  xptr_array_add (array, NULL);

  _g_freedesktop_dbus_complete_list_names (object,
					   invocation,
					   (const xchar_t * const*)array->pdata);
  xptr_array_free (array, TRUE);
  return TRUE;
}

static xboolean_t
handle_list_queued_owners (_GFreedesktopDBus *object,
			   xdbus_method_invocation_t *invocation,
			   const xchar_t *arg_name)
{
  GDBusDaemon *daemon = G_DBUS_DAEMON (object);
  xptr_array_t *array;
  Name *name;
  xlist_t *l;

  array = xptr_array_new ();

  name = name_lookup (daemon, arg_name);
  if (name && name->owner)
    {
      for (l = name->queue; l != NULL; l = l->next)
	{
	  Client *client = l->data;

	  xptr_array_add (array, client->id);
	}
    }

  xptr_array_add (array, NULL);

  _g_freedesktop_dbus_complete_list_queued_owners (object,
						   invocation,
						   (const xchar_t * const*)array->pdata);
  xptr_array_free (array, TRUE);
  return TRUE;
}

static xboolean_t
handle_name_has_owner (_GFreedesktopDBus *object,
		       xdbus_method_invocation_t *invocation,
		       const xchar_t *arg_name)
{
  GDBusDaemon *daemon = G_DBUS_DAEMON (object);
  Name *name;
  Client *client;

  name = name_lookup (daemon, arg_name);
  client = xhash_table_lookup (daemon->clients, arg_name);

  _g_freedesktop_dbus_complete_name_has_owner (object, invocation,
					       name != NULL || client != NULL);
  return TRUE;
}

static xboolean_t
handle_release_name (_GFreedesktopDBus *object,
		     xdbus_method_invocation_t *invocation,
		     const xchar_t *arg_name)
{
  Client *client = xobject_get_data (G_OBJECT (xdbus_method_invocation_get_connection (invocation)), "client");
  GDBusDaemon *daemon = G_DBUS_DAEMON (object);
  Name *name;
  xuint32_t result;

  if (!g_dbus_is_name (arg_name))
    {
      xdbus_method_invocation_return_error (invocation,
					     G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
					     "Given bus name \"%s\" is not valid", arg_name);
      return TRUE;
    }

  if (*arg_name == ':')
    {
      xdbus_method_invocation_return_error (invocation,
					     G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
					     "Cannot release a service starting with ':' such as \"%s\"", arg_name);
      return TRUE;
    }

  if (strcmp (arg_name, DBUS_SERVICE_NAME) == 0)
    {
      xdbus_method_invocation_return_error (invocation,
					     G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
					     "Cannot release a service named " DBUS_SERVICE_NAME ", because that is owned by the bus");
      return TRUE;
    }

  name = name_lookup (daemon, arg_name);

  if (name == NULL)
    result = DBUS_RELEASE_NAME_REPLY_NON_EXISTENT;
  else if (name->owner && name->owner->client == client)
    {
      name_release_owner (name);
      result = DBUS_RELEASE_NAME_REPLY_RELEASED;
    }
  else if (name_unqueue_owner (name, client))
    result = DBUS_RELEASE_NAME_REPLY_RELEASED;
  else
    result = DBUS_RELEASE_NAME_REPLY_NOT_OWNER;

  _g_freedesktop_dbus_complete_release_name (object, invocation, result);
  return TRUE;
}

static xboolean_t
handle_reload_config (_GFreedesktopDBus *object,
		      xdbus_method_invocation_t *invocation)
{
  _g_freedesktop_dbus_complete_reload_config (object, invocation);
  return TRUE;
}

static xboolean_t
handle_update_activation_environment (_GFreedesktopDBus *object,
				      xdbus_method_invocation_t *invocation,
				      xvariant_t *arg_environment)
{
  xdbus_method_invocation_return_error (invocation,
					 G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
					 "UpdateActivationEnvironment not implemented");
  return TRUE;
}

static xboolean_t
handle_remove_match (_GFreedesktopDBus *object,
		     xdbus_method_invocation_t *invocation,
		     const xchar_t *arg_rule)
{
  Client *client = xobject_get_data (G_OBJECT (xdbus_method_invocation_get_connection (invocation)), "client");
  Match *match, *other_match;
  xlist_t *l;

  match = match_new (arg_rule);

  if (match == NULL)
    xdbus_method_invocation_return_error (invocation,
					   G_DBUS_ERROR, G_DBUS_ERROR_MATCH_RULE_INVALID,
					   "Invalid rule: %s", arg_rule);
  else
    {
      for (l = client->matches; l != NULL; l = l->next)
	{
	  other_match = l->data;
	  if (match_equal (match, other_match))
	    {
	      match_free (other_match);
	      client->matches = xlist_delete_link (client->matches, l);
	      break;
	    }
	}

      if (l == NULL)
	xdbus_method_invocation_return_error (invocation,
					       G_DBUS_ERROR, G_DBUS_ERROR_MATCH_RULE_NOT_FOUND,
					       "The given match rule wasn't found and can't be removed");
      else
	_g_freedesktop_dbus_complete_remove_match (object, invocation);
    }
  if (match)
    match_free (match);

  return TRUE;
}

static xboolean_t
handle_request_name (_GFreedesktopDBus *object,
		     xdbus_method_invocation_t *invocation,
		     const xchar_t *arg_name,
		     xuint_t flags)
{
  Client *client = xobject_get_data (G_OBJECT (xdbus_method_invocation_get_connection (invocation)), "client");
  GDBusDaemon *daemon = G_DBUS_DAEMON (object);
  Name *name;
  NameOwner *owner;
  xuint32_t result;

  if (!g_dbus_is_name (arg_name))
    {
      xdbus_method_invocation_return_error (invocation,
					     G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
					     "Requested bus name \"%s\" is not valid", arg_name);
      return TRUE;
    }

  if (*arg_name == ':')
    {
      xdbus_method_invocation_return_error (invocation,
					     G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
					     "Cannot acquire a service starting with ':' such as \"%s\"", arg_name);
      return TRUE;
    }

  if (strcmp (arg_name, DBUS_SERVICE_NAME) == 0)
    {
      xdbus_method_invocation_return_error (invocation,
					     G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
					     "Cannot acquire a service named " DBUS_SERVICE_NAME ", because that is reserved");
      return TRUE;
    }

  name = name_ensure (daemon, arg_name);
  if (name->owner == NULL)
    {
      owner = name_owner_new (client, flags);
      name_replace_owner (name, owner);

      result = DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;
    }
  else if (name->owner && name->owner->client == client)
    {
      name->owner->flags = flags;
      result = DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER;
    }
  else if ((flags & DBUS_NAME_FLAG_DO_NOT_QUEUE) &&
	   (!(flags & DBUS_NAME_FLAG_REPLACE_EXISTING) ||
	    !(name->owner->flags & DBUS_NAME_FLAG_ALLOW_REPLACEMENT)))
    {
      /* Unqueue if queued */
      name_unqueue_owner (name, client);
      result = DBUS_REQUEST_NAME_REPLY_EXISTS;
    }
  else if (!(flags & DBUS_NAME_FLAG_DO_NOT_QUEUE) &&
	   (!(flags & DBUS_NAME_FLAG_REPLACE_EXISTING) ||
	    !(name->owner->flags & DBUS_NAME_FLAG_ALLOW_REPLACEMENT)))
    {
      /* Queue the connection */
      owner = name_owner_new (client, flags);
      name_queue_owner (name, owner);
      result = DBUS_REQUEST_NAME_REPLY_IN_QUEUE;
    }
  else
    {
      /* Replace the current owner */

      owner = name_owner_new (client, flags);
      name_replace_owner (name, owner);

      result = DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER;
    }

  name_unref (name);

  _g_freedesktop_dbus_complete_request_name (object, invocation, result);
  return TRUE;
}

static xboolean_t
handle_start_service_by_name (_GFreedesktopDBus *object,
			      xdbus_method_invocation_t *invocation,
			      const xchar_t *arg_name,
			      xuint_t arg_flags)
{
  GDBusDaemon *daemon = G_DBUS_DAEMON (object);
  Name *name;

  name = name_lookup (daemon, arg_name);
  if (name)
    _g_freedesktop_dbus_complete_start_service_by_name (object, invocation,
							DBUS_START_REPLY_ALREADY_RUNNING);
  else
    xdbus_method_invocation_return_error (invocation,
					   G_DBUS_ERROR, G_DBUS_ERROR_SERVICE_UNKNOWN,
					   "No support for activation for name: %s", arg_name);

  return TRUE;
}

G_GNUC_PRINTF(5, 6)
static void
return_error (Client *client, xdbus_message_t *message,
	      xquark                 domain,
	      xint_t                   code,
	      const xchar_t           *format,
	      ...)
{
  xdbus_message_t *reply;
  va_list var_args;
  char *error_message;
  xerror_t *error;
  xchar_t *dbus_error_name;

  va_start (var_args, format);
  error_message = xstrdup_vprintf (format, var_args);
  va_end (var_args);

  error = xerror_new_literal (domain, code, "");
  dbus_error_name = g_dbus_error_encode_gerror (error);

  reply = xdbus_message_new_method_error_literal (message,
						   dbus_error_name,
						   error_message);

  xerror_free (error);
  g_free (dbus_error_name);
  g_free (error_message);

  if (!g_dbus_connection_send_message (client->connection, reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL))
      g_warning ("Error sending reply");
  xobject_unref (reply);
}

static xdbus_message_t *
route_message (Client *source_client, xdbus_message_t *message)
{
  const char *dest;
  Client *dest_client;
  GDBusDaemon *daemon;

  daemon = source_client->daemon;

  dest_client = NULL;
  dest = xdbus_message_get_destination (message);
  if (dest != NULL && strcmp (dest, DBUS_SERVICE_NAME) != 0)
    {
      dest_client = xhash_table_lookup (daemon->clients, dest);

      if (dest_client == NULL)
	{
	  Name *name;
	  name = name_lookup (daemon, dest);
	  if (name && name->owner)
	    dest_client = name->owner->client;
	}

      if (dest_client == NULL)
	{
	  if (xdbus_message_get_message_type (message) == G_DBUS_MESSAGE_TYPE_METHOD_CALL)
	    return_error (source_client, message,
			  G_DBUS_ERROR, G_DBUS_ERROR_SERVICE_UNKNOWN,
			  "The name %s is unknown", dest);
	}
      else
	{
	  xerror_t *error = NULL;

	  if (!g_dbus_connection_send_message (dest_client->connection, message, G_DBUS_SEND_MESSAGE_FLAGS_PRESERVE_SERIAL, NULL, &error))
	    {
	      g_warning ("Error forwarding message: %s", error->message);
	      xerror_free (error);
	    }
	}
    }

  broadcast_message (daemon, message, dest_client != NULL, TRUE, dest_client);

  /* Swallow messages not for the bus */
  if (dest == NULL || strcmp (dest, DBUS_SERVICE_NAME) != 0)
    {
      xobject_unref (message);
      message = NULL;
    }

  return message;
}

static xdbus_message_t *
copy_if_locked (xdbus_message_t *message)
{
  if (xdbus_message_get_locked (message))
    {
      xdbus_message_t *copy = xdbus_message_copy (message, NULL);
      xobject_unref (message);
      message = copy;
    }
  return message;
}

static xdbus_message_t *
filter_function (xdbus_connection_t *connection,
		 xdbus_message_t    *message,
		 xboolean_t         incoming,
		 xpointer_t         user_data)
{
  Client *client = user_data;

  if (0)
    {
      const char *types[] = {"invalid", "method_call", "method_return", "error", "signal" };
      g_printerr ("%s%s %s %d(%d) sender: %s destination: %s %s %s.%s\n",
		client->id,
		incoming? "->" : "<-",
		types[xdbus_message_get_message_type (message)],
		xdbus_message_get_serial (message),
		xdbus_message_get_reply_serial (message),
		xdbus_message_get_sender (message),
		xdbus_message_get_destination (message),
		xdbus_message_get_path (message),
		xdbus_message_get_interface (message),
		xdbus_message_get_member (message));
    }

  if (incoming)
    {
      /* Ensure its not locked so we can set the sender */
      message = copy_if_locked (message);
      if (message == NULL)
	{
	  g_warning ("Failed to copy incoming message");
	  return NULL;
	}
      xdbus_message_set_sender (message, client->id);

      return route_message (client, message);
    }
  else
    {
      if (xdbus_message_get_sender (message) == NULL ||
          xdbus_message_get_destination (message) == NULL)
        {
          message = copy_if_locked (message);
          if (message == NULL)
            {
              g_warning ("Failed to copy outgoing message");
              return NULL;
            }
        }

      if (xdbus_message_get_sender (message) == NULL)
        xdbus_message_set_sender (message, DBUS_SERVICE_NAME);
      if (xdbus_message_get_destination (message) == NULL)
        xdbus_message_set_destination (message, client->id);
    }

  return message;
}

static xboolean_t
on_new_connection (xdbus_server_t *server,
		   xdbus_connection_t *connection,
		   xpointer_t user_data)
{
  GDBusDaemon *daemon = user_data;

  g_dbus_connection_set_exit_on_close (connection, FALSE);

  if (daemon->timeout)
    {
      xsource_remove (daemon->timeout);
      daemon->timeout = 0;
    }

  client_new (daemon, connection);

  return TRUE;
}

static void
g_dbus_daemon_finalize (xobject_t *object)
{
  GDBusDaemon *daemon = G_DBUS_DAEMON (object);
  xlist_t *clients, *l;

  if (daemon->timeout)
    xsource_remove (daemon->timeout);

  clients = xhash_table_get_values (daemon->clients);
  for (l = clients; l != NULL; l = l->next)
    client_free (l->data);
  xlist_free (clients);

  g_assert (xhash_table_size (daemon->clients) == 0);
  g_assert (xhash_table_size (daemon->names) == 0);

  xhash_table_destroy (daemon->clients);
  xhash_table_destroy (daemon->names);

  xobject_unref (daemon->server);

  if (daemon->tmpdir)
    {
      g_rmdir (daemon->tmpdir);
      g_free (daemon->tmpdir);
    }

  g_free (daemon->guid);
  g_free (daemon->address);

  G_OBJECT_CLASS (g_dbus_daemon_parent_class)->finalize (object);
}

static void
g_dbus_daemon_init (GDBusDaemon *daemon)
{
  daemon->next_major_id = 1;
  daemon->clients = xhash_table_new_full (xstr_hash, xstr_equal, NULL, NULL);
  daemon->names = xhash_table_new_full (xstr_hash, xstr_equal, NULL, NULL);
  daemon->guid = g_dbus_generate_guid ();
}

static xboolean_t
initable_init (xinitable_t     *initable,
	       xcancellable_t  *cancellable,
	       xerror_t       **error)
{
  GDBusDaemon *daemon = G_DBUS_DAEMON (initable);
  GDBusServerFlags flags;

  flags = G_DBUS_SERVER_FLAGS_NONE;
  if (daemon->address == NULL)
    {
#ifdef G_OS_UNIX
      if (g_unix_socket_address_abstract_names_supported ())
	daemon->address = xstrdup ("unix:tmpdir=/tmp/gdbus-daemon");
      else
	{
	  daemon->tmpdir = g_dir_make_tmp ("gdbus-daemon-XXXXXX", NULL);
	  daemon->address = xstrdup_printf ("unix:tmpdir=%s", daemon->tmpdir);
	}
      flags |= G_DBUS_SERVER_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER;
#else
      /* Don’t require authentication on Windows as that hasn’t been
       * implemented yet. */
      daemon->address = xstrdup ("nonce-tcp:");
      flags |= G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;
#endif
    }

  daemon->server = g_dbus_server_new_sync (daemon->address,
					   flags,
					   daemon->guid,
					   NULL,
					   cancellable,
					   error);
  if (daemon->server == NULL)
    return FALSE;


  g_dbus_server_start (daemon->server);

  g_signal_connect (daemon->server, "new-connection",
		    G_CALLBACK (on_new_connection),
		    daemon);

  return TRUE;
}

static void
g_dbus_daemon_set_property (xobject_t      *object,
			    xuint_t         prop_id,
			    const xvalue_t *value,
			    xparam_spec_t   *pspec)
{
  GDBusDaemon *daemon = G_DBUS_DAEMON (object);

  switch (prop_id)
    {
    case PROP_ADDRESS:
      g_free (daemon->address);
      daemon->address = xvalue_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_dbus_daemon_get_property (xobject_t    *object,
			    xuint_t       prop_id,
			    xvalue_t     *value,
			    xparam_spec_t *pspec)
{
  GDBusDaemon *daemon = G_DBUS_DAEMON (object);

  switch (prop_id)
    {
      case PROP_ADDRESS:
	xvalue_set_string (value, daemon->address);
	break;

    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_dbus_daemon_class_init (GDBusDaemonClass *klass)
{
  xobject_class_t *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = g_dbus_daemon_finalize;
  gobject_class->set_property = g_dbus_daemon_set_property;
  gobject_class->get_property = g_dbus_daemon_get_property;

  g_dbus_daemon_signals[SIGNAL_IDLE_TIMEOUT] =
    g_signal_new (I_("idle-timeout"),
		  XTYPE_DBUS_DAEMON,
		  G_SIGNAL_RUN_LAST,
		  0,
		  NULL, NULL,
		  NULL,
		  XTYPE_NONE, 0);

  xobject_class_install_property (gobject_class,
				   PROP_ADDRESS,
				   g_param_spec_string ("address",
							"Bus Address",
							"The address the bus should use",
							NULL,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT_ONLY |
							G_PARAM_STATIC_STRINGS));
}

static void
g_dbus_daemon_iface_init (_GFreedesktopDBusIface *iface)
{
  iface->handle_add_match = handle_add_match;
  iface->handle_get_connection_selinux_security_context = handle_get_connection_selinux_security_context;
  iface->handle_get_connection_unix_process_id = handle_get_connection_unix_process_id;
  iface->handle_get_connection_unix_user = handle_get_connection_unix_user;
  iface->handle_get_id = handle_get_id;
  iface->handle_get_name_owner = handle_get_name_owner;
  iface->handle_hello = handle_hello;
  iface->handle_list_activatable_names = handle_list_activatable_names;
  iface->handle_list_names = handle_list_names;
  iface->handle_list_queued_owners = handle_list_queued_owners;
  iface->handle_name_has_owner = handle_name_has_owner;
  iface->handle_release_name = handle_release_name;
  iface->handle_reload_config = handle_reload_config;
  iface->handle_update_activation_environment = handle_update_activation_environment;
  iface->handle_remove_match = handle_remove_match;
  iface->handle_request_name = handle_request_name;
  iface->handle_start_service_by_name = handle_start_service_by_name;
}

static void
initable_iface_init (xinitable_iface_t *initable_iface)
{
  initable_iface->init = initable_init;
}

GDBusDaemon *
_g_dbus_daemon_new (const char *address,
		    xcancellable_t *cancellable,
		    xerror_t **error)
{
  return xinitable_new (XTYPE_DBUS_DAEMON,
			 cancellable,
			 error,
			 "address", address,
			 NULL);
}

const char *
_g_dbus_daemon_get_address (GDBusDaemon *daemon)
{
  return g_dbus_server_get_client_address (daemon->server);
}
