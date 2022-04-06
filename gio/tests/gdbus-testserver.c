#include <gio/gio.h>
#include <stdlib.h>

static xdbus_node_info_t *introspection_data = NULL;
static xmain_loop_t *loop = NULL;
static xhashtable_t *properties = NULL;

static const xchar_t introspection_xml[] =
  "<node>"
  "  <interface name='com.example.Frob'>"
  "    <method name='Quit'>"
  "    </method>"
  "    <method name='TestArrayOfStringTypes'>"
  "      <arg direction='in'  type='as' name='val_string' />"
  "      <arg direction='in'  type='ao' name='val_objpath' />"
  "      <arg direction='in'  type='ag' name='val_signature' />"
  "      <arg direction='out' type='as' />"
  "      <arg direction='out' type='ao' />"
  "      <arg direction='out' type='ag' />"
  "    </method>"
  "    <method name='TestPrimitiveTypes'>"
  "      <arg direction='in'  type='y' name='val_byte' />"
  "      <arg direction='in'  type='b' name='val_boolean' />"
  "      <arg direction='in'  type='n' name='val_int16' />"
  "      <arg direction='in'  type='q' name='val_uint16' />"
  "      <arg direction='in'  type='i' name='val_int32' />"
  "      <arg direction='in'  type='u' name='val_uint32' />"
  "      <arg direction='in'  type='x' name='val_int64' />"
  "      <arg direction='in'  type='t' name='val_uint64' />"
  "      <arg direction='in'  type='d' name='val_double' />"
  "      <arg direction='in'  type='s' name='val_string' />"
  "      <arg direction='in'  type='o' name='val_objpath' />"
  "      <arg direction='in'  type='g' name='val_signature' />"
  "      <arg direction='out' type='y' />"
  "      <arg direction='out' type='b' />"
  "      <arg direction='out' type='n' />"
  "      <arg direction='out' type='q' />"
  "      <arg direction='out' type='i' />"
  "      <arg direction='out' type='u' />"
  "      <arg direction='out' type='x' />"
  "      <arg direction='out' type='t' />"
  "      <arg direction='out' type='d' />"
  "      <arg direction='out' type='s' />"
  "      <arg direction='out' type='o' />"
  "      <arg direction='out' type='g' />"
  "    </method>"
  "    <method name='EmitSignal'>"
  "      <arg direction='in'  type='s' name='str1' />"
  "      <arg direction='in'  type='o' name='objpath1' />"
  "    </method>"
  "    <method name='TestArrayOfPrimitiveTypes'>"
  "      <arg direction='in'  type='ay' name='val_byte' />"
  "      <arg direction='in'  type='ab' name='val_boolean' />"
  "      <arg direction='in'  type='an' name='val_int16' />"
  "      <arg direction='in'  type='aq' name='val_uint16' />"
  "      <arg direction='in'  type='ai' name='val_int32' />"
  "      <arg direction='in'  type='au' name='val_uint32' />"
  "      <arg direction='in'  type='ax' name='val_int64' />"
  "      <arg direction='in'  type='at' name='val_uint64' />"
  "      <arg direction='in'  type='ad' name='val_double' />"
  "      <arg direction='out' type='ay' />"
  "      <arg direction='out' type='ab' />"
  "      <arg direction='out' type='an' />"
  "      <arg direction='out' type='aq' />"
  "      <arg direction='out' type='ai' />"
  "      <arg direction='out' type='au' />"
  "      <arg direction='out' type='ax' />"
  "      <arg direction='out' type='at' />"
  "      <arg direction='out' type='ad' />"
  "    </method>"
  "    <method name='FrobSetProperty'>"
  "      <arg direction='in'  type='s' name='prop_name' />"
  "      <arg direction='in'  type='v' name='prop_value' />"
  "    </method>"
  "    <signal name='TestSignal'>"
  "      <arg type='s' name='str1' />"
  "      <arg type='o' name='objpath1' />"
  "      <arg type='v' name='variant1' />"
  "    </signal>"
  "    <method name='TestComplexArrays'>"
  "      <arg direction='in'  type='a(ii)' name='aii' />"
  "      <arg direction='in'  type='aa(ii)' name='aaii' />"
  "      <arg direction='in'  type='aas' name='aas' />"
  "      <arg direction='in'  type='aa{ss}' name='ahashes' />"
  "      <arg direction='in'  type='aay' name='aay' />"
  "      <arg direction='in'  type='av' name='av' />"
  "      <arg direction='in'  type='aav' name='aav' />"
  "      <arg direction='out' type='a(ii)' />"
  "      <arg direction='out' type='aa(ii)' />"
  "      <arg direction='out' type='aas' />"
  "      <arg direction='out' type='aa{ss}' />"
  "      <arg direction='out' type='aay' />"
  "      <arg direction='out' type='av' />"
  "      <arg direction='out' type='aav' />"
  "    </method>"
  "    <method name='TestVariant'>"
  "      <arg direction='in'  type='v' name='v' />"
  "      <arg direction='in'  type='b' name='modify' />"
  "      <arg direction='out' type='v' />"
  "    </method>"
  "    <method name='FrobInvalidateProperty'>"
  "      <arg direction='in'  type='s' name='new_value' />"
  "    </method>"
  "    <method name='HelloWorld'>"
  "      <arg direction='in'  type='s' name='hello_message' />"
  "      <arg direction='out' type='s' />"
  "    </method>"
  "    <method name='PairReturn'>"
  "      <arg direction='out' type='s' />"
  "      <arg direction='out' type='u' />"
  "    </method>"
  "    <method name='TestStructureTypes'>"
  "      <arg direction='in'  type='(ii)' name='s1' />"
  "      <arg direction='in'  type='(s(ii)aya{ss})' name='s2' />"
  "      <arg direction='out' type='(ii)' />"
  "      <arg direction='out' type='(s(ii)aya{ss})' />"
  "    </method>"
  "    <method name='EmitSignal2'>"
  "    </method>"
  "    <method name='DoubleHelloWorld'>"
  "      <arg direction='in'  type='s' name='hello1' />"
  "      <arg direction='in'  type='s' name='hello2' />"
  "      <arg direction='out' type='s' />"
  "      <arg direction='out' type='s' />"
  "    </method>"
  "    <method name='Sleep'>"
  "      <arg direction='in'  type='i' name='msec' />"
  "    </method>"
  "    <method name='TestHashTables'>"
  "      <arg direction='in'  type='a{yy}' name='hyy' />"
  "      <arg direction='in'  type='a{bb}' name='hbb' />"
  "      <arg direction='in'  type='a{nn}' name='hnn' />"
  "      <arg direction='in'  type='a{qq}' name='hqq' />"
  "      <arg direction='in'  type='a{ii}' name='hii' />"
  "      <arg direction='in'  type='a{uu}' name='huu' />"
  "      <arg direction='in'  type='a{xx}' name='hxx' />"
  "      <arg direction='in'  type='a{tt}' name='htt' />"
  "      <arg direction='in'  type='a{dd}' name='hdd' />"
  "      <arg direction='in'  type='a{ss}' name='hss' />"
  "      <arg direction='in'  type='a{oo}' name='hoo' />"
  "      <arg direction='in'  type='a{gg}' name='hgg' />"
  "      <arg direction='out' type='a{yy}' />"
  "      <arg direction='out' type='a{bb}' />"
  "      <arg direction='out' type='a{nn}' />"
  "      <arg direction='out' type='a{qq}' />"
  "      <arg direction='out' type='a{ii}' />"
  "      <arg direction='out' type='a{uu}' />"
  "      <arg direction='out' type='a{xx}' />"
  "      <arg direction='out' type='a{tt}' />"
  "      <arg direction='out' type='a{dd}' />"
  "      <arg direction='out' type='a{ss}' />"
  "      <arg direction='out' type='a{oo}' />"
  "      <arg direction='out' type='a{gg}' />"
  "    </method>"
  "    <signal name='TestSignal2'>"
  "      <arg type='i' name='int1' />"
  "    </signal>"
  "    <method name='TestComplexHashTables'>"
  "      <arg direction='in'  type='a{s(ii)}' name='h_str_to_pair' />"
  "      <arg direction='in'  type='a{sv}' name='h_str_to_variant' />"
  "      <arg direction='in'  type='a{sav}' name='h_str_to_av' />"
  "      <arg direction='in'  type='a{saav}' name='h_str_to_aav' />"
  "      <arg direction='in'  type='a{sa(ii)}' name='h_str_to_array_of_pairs' />"
  "      <arg direction='in'  type='a{sa{ss}}' name='hash_of_hashes' />"
  "      <arg direction='out' type='a{s(ii)}' />"
  "      <arg direction='out' type='a{sv}' />"
  "      <arg direction='out' type='a{sav}' />"
  "      <arg direction='out' type='a{saav}' />"
  "      <arg direction='out' type='a{sa(ii)}' />"
  "      <arg direction='out' type='a{sa{ss}}' />"
  "    </method>"
  "    <property type='y' name='y' access='readwrite' />"
  "    <property type='b' name='b' access='readwrite' />"
  "    <property type='n' name='n' access='readwrite' />"
  "    <property type='q' name='q' access='readwrite' />"
  "    <property type='i' name='i' access='readwrite' />"
  "    <property type='u' name='u' access='readwrite' />"
  "    <property type='x' name='x' access='readwrite' />"
  "    <property type='t' name='t' access='readwrite' />"
  "    <property type='d' name='d' access='readwrite' />"
  "    <property type='s' name='s' access='readwrite' />"
  "    <property type='o' name='o' access='readwrite' />"
  "    <property type='ay' name='ay' access='readwrite' />"
  "    <property type='ab' name='ab' access='readwrite' />"
  "    <property type='an' name='an' access='readwrite' />"
  "    <property type='aq' name='aq' access='readwrite' />"
  "    <property type='ai' name='ai' access='readwrite' />"
  "    <property type='au' name='au' access='readwrite' />"
  "    <property type='ax' name='ax' access='readwrite' />"
  "    <property type='at' name='at' access='readwrite' />"
  "    <property type='ad' name='ad' access='readwrite' />"
  "    <property type='as' name='as' access='readwrite' />"
  "    <property type='ao' name='ao' access='readwrite' />"
  "    <property type='s' name='foo' access='readwrite' />"
  "    <property type='s' name='PropertyThatWillBeInvalidated' access='readwrite' />"
  "  </interface>"
  "</node>";

static xboolean_t
end_sleep (xpointer_t data)
{
  xdbus_method_invocation_t *invocation = data;

  xdbus_method_invocation_return_value (invocation, NULL);
  xobject_unref (invocation);

  return G_SOURCE_REMOVE;
}

static void
handle_method_call (xdbus_connection_t       *connection,
                    const xchar_t           *sender,
                    const xchar_t           *object_path,
                    const xchar_t           *interface_name,
                    const xchar_t           *method_name,
                    xvariant_t              *parameters,
                    xdbus_method_invocation_t *invocation,
                    xpointer_t               user_data)
{
  if (xstrcmp0 (method_name, "HelloWorld") == 0)
    {
      const xchar_t *greeting;

      xvariant_get (parameters, "(&s)", &greeting);
      if (xstrcmp0 (greeting, "Yo") == 0)
        {
          xdbus_method_invocation_return_dbus_error (invocation,
                                                      "com.example.TestException",
                                                      "Yo is not a proper greeting");
        }
      else
        {
          xchar_t *response;
          response = xstrdup_printf ("You greeted me with '%s'. Thanks!", greeting);
          xdbus_method_invocation_return_value (invocation,
                                                 xvariant_new ("(s)", response));
          g_free ( response);
        }
    }
  else if (xstrcmp0 (method_name, "DoubleHelloWorld") == 0)
    {
      const xchar_t *hello1, *hello2;
      xchar_t *reply1, *reply2;

      xvariant_get (parameters, "(&s&s)", &hello1, &hello2);
      reply1 = xstrdup_printf ("You greeted me with '%s'. Thanks!", hello1);
      reply2 = xstrdup_printf ("Yo dawg, you uttered '%s'. Thanks!", hello2);
      xdbus_method_invocation_return_value (invocation,
                                             xvariant_new ("(ss)", reply1, reply2));
      g_free (reply1);
      g_free (reply2);
    }
  else if (xstrcmp0 (method_name, "PairReturn") == 0)
    {
      xdbus_method_invocation_return_value (invocation,
                                             xvariant_new ("(su)", "foo", 42));
    }
  else if (xstrcmp0 (method_name, "TestPrimitiveTypes") == 0)
    {
      xuchar_t val_byte;
      xboolean_t val_boolean;
      gint16 val_int16;
      xuint16_t val_uint16;
      gint32 val_int32;
      xuint32_t val_uint32;
      sint64_t val_int64;
      xuint64_t val_uint64;
      xdouble_t val_double;
      const xchar_t *val_string;
      const xchar_t *val_objpath;
      const xchar_t *val_signature;
      xchar_t *ret_string;
      xchar_t *ret_objpath;
      xchar_t *ret_signature;

      xvariant_get (parameters, "(ybnqiuxtd&s&o&g)",
                     &val_byte,
                     &val_boolean,
                     &val_int16,
                     &val_uint16,
                     &val_int32,
                     &val_uint32,
                     &val_int64,
                     &val_uint64,
                     &val_double,
                     &val_string,
                     &val_objpath,
                     &val_signature);

      ret_string = xstrconcat (val_string, val_string, NULL);
      ret_objpath = xstrconcat (val_objpath, "/modified", NULL);
      ret_signature = xstrconcat (val_signature, val_signature, NULL);

      xdbus_method_invocation_return_value (invocation,
          xvariant_new ("(ybnqiuxtdsog)",
                         val_byte + 1,
                         !val_boolean,
                         val_int16 + 1,
                         val_uint16 + 1,
                         val_int32 + 1,
                         val_uint32 + 1,
                         val_int64 + 1,
                         val_uint64 + 1,
                         - val_double + 0.123,
                         ret_string,
                         ret_objpath,
                         ret_signature));

      g_free (ret_string);
      g_free (ret_objpath);
      g_free (ret_signature);
    }
  else if (xstrcmp0 (method_name, "TestArrayOfPrimitiveTypes") == 0)
    {
      xvariant_t *v;
      const xuchar_t *bytes;
      const gint16 *int16s;
      const xuint16_t *uint16s;
      const gint32 *int32s;
      const xuint32_t *uint32s;
      const sint64_t *int64s;
      const xuint64_t *uint64s;
      const xdouble_t *doubles;
      xsize_t n_elts;
      xsize_t i, j;
      xvariant_builder_t ret;

      xvariant_builder_init (&ret, G_VARIANT_TYPE ("(ayabanaqaiauaxatad)"));

      v = xvariant_get_child_value (parameters, 0);
      bytes = xvariant_get_fixed_array (v, &n_elts, 1);
      xvariant_builder_open (&ret, G_VARIANT_TYPE ("ay"));
      for (j = 0; j < 2; j++)
        for (i = 0; i < n_elts; i++)
          xvariant_builder_add (&ret, "y", bytes[i]);
      xvariant_builder_close (&ret);
      xvariant_unref (v);

      v = xvariant_get_child_value (parameters, 1);
      bytes = xvariant_get_fixed_array (v, &n_elts, 1);
      xvariant_builder_open (&ret, G_VARIANT_TYPE ("ab"));
      for (j = 0; j < 2; j++)
        for (i = 0; i < n_elts; i++)
          xvariant_builder_add (&ret, "b", (xboolean_t)bytes[i]);
      xvariant_builder_close (&ret);
      xvariant_unref (v);

      v = xvariant_get_child_value (parameters, 2);
      int16s = xvariant_get_fixed_array (v, &n_elts, 2);
      xvariant_builder_open (&ret, G_VARIANT_TYPE ("an"));
      for (j = 0; j < 2; j++)
        for (i = 0; i < n_elts; i++)
          xvariant_builder_add (&ret, "n", int16s[i]);
      xvariant_builder_close (&ret);
      xvariant_unref (v);

      v = xvariant_get_child_value (parameters, 3);
      uint16s = xvariant_get_fixed_array (v, &n_elts, 2);
      xvariant_builder_open (&ret, G_VARIANT_TYPE ("aq"));
      for (j = 0; j < 2; j++)
        for (i = 0; i < n_elts; i++)
          xvariant_builder_add (&ret, "q", uint16s[i]);
      xvariant_builder_close (&ret);
      xvariant_unref (v);

      v = xvariant_get_child_value (parameters, 4);
      int32s = xvariant_get_fixed_array (v, &n_elts, 4);
      xvariant_builder_open (&ret, G_VARIANT_TYPE ("ai"));
      for (j = 0; j < 2; j++)
        for (i = 0; i < n_elts; i++)
          xvariant_builder_add (&ret, "i", int32s[i]);
      xvariant_builder_close (&ret);
      xvariant_unref (v);

      v = xvariant_get_child_value (parameters, 5);
      uint32s = xvariant_get_fixed_array (v, &n_elts, 4);
      xvariant_builder_open (&ret, G_VARIANT_TYPE ("au"));
      for (j = 0; j < 2; j++)
        for (i = 0; i < n_elts; i++)
          xvariant_builder_add (&ret, "u", uint32s[i]);
      xvariant_builder_close (&ret);
      xvariant_unref (v);

      v = xvariant_get_child_value (parameters, 6);
      int64s = xvariant_get_fixed_array (v, &n_elts, 8);
      xvariant_builder_open (&ret, G_VARIANT_TYPE ("ax"));
      for (j = 0; j < 2; j++)
        for (i = 0; i < n_elts; i++)
          xvariant_builder_add (&ret, "x", int64s[i]);
      xvariant_builder_close (&ret);
      xvariant_unref (v);

      v = xvariant_get_child_value (parameters, 7);
      uint64s = xvariant_get_fixed_array (v, &n_elts, 8);
      xvariant_builder_open (&ret, G_VARIANT_TYPE ("at"));
      for (j = 0; j < 2; j++)
        for (i = 0; i < n_elts; i++)
          xvariant_builder_add (&ret, "t", uint64s[i]);
      xvariant_builder_close (&ret);
      xvariant_unref (v);

      v = xvariant_get_child_value (parameters, 8);
      doubles = xvariant_get_fixed_array (v, &n_elts, sizeof (xdouble_t));
      xvariant_builder_open (&ret, G_VARIANT_TYPE ("ad"));
      for (j = 0; j < 2; j++)
        for (i = 0; i < n_elts; i++)
          xvariant_builder_add (&ret, "d", doubles[i]);
      xvariant_builder_close (&ret);
      xvariant_unref (v);

      xdbus_method_invocation_return_value (invocation,
                                             xvariant_builder_end (&ret));
    }
  else if (xstrcmp0 (method_name, "TestArrayOfStringTypes") == 0)
    {
      xvariant_iter_t *iter1;
      xvariant_iter_t *iter2;
      xvariant_iter_t *iter3;
      xvariant_iter_t *iter;
      xvariant_builder_t ret;
      const xchar_t *s;
      xint_t i;

      xvariant_builder_init (&ret, G_VARIANT_TYPE ("(asaoag)"));
      xvariant_get (parameters, "(asaoag)", &iter1, &iter2, &iter3);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("as"));
      for (i = 0; i < 2; i++)
        {
          iter = xvariant_iter_copy (iter1);
          while (xvariant_iter_loop (iter, "s", &s))
            xvariant_builder_add (&ret, "s", s);
          xvariant_iter_free (iter);
        }
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("ao"));
      for (i = 0; i < 2; i++)
        {
          iter = xvariant_iter_copy (iter1);
          while (xvariant_iter_loop (iter, "o", &s))
            xvariant_builder_add (&ret, "o", s);
          xvariant_iter_free (iter);
        }
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("ag"));
      for (i = 0; i < 2; i++)
        {
          iter = xvariant_iter_copy (iter1);
          while (xvariant_iter_loop (iter, "g", &s))
            xvariant_builder_add (&ret, "g", s);
          xvariant_iter_free (iter);
        }
      xvariant_builder_close (&ret);

      xvariant_iter_free (iter1);
      xvariant_iter_free (iter2);
      xvariant_iter_free (iter3);

      xdbus_method_invocation_return_value (invocation,
                                             xvariant_builder_end (&ret));
    }
  else if (xstrcmp0 (method_name, "TestHashTables") == 0)
    {
      xvariant_t *v;
      xvariant_iter_t iter;
      xvariant_builder_t ret;
      xuint8_t y1, y2;
      xboolean_t b1, b2;
      gint16 n1, n2;
      xuint16_t q1, q2;
      xint_t i1, i2;
      xuint_t u1, u2;
      sint64_t x1, x2;
      xuint64_t t1, t2;
      xdouble_t d1, d2;
      xchar_t *s1, *s2;

      xvariant_builder_init (&ret, G_VARIANT_TYPE ("(a{yy}a{bb}a{nn}a{qq}a{ii}a{uu}a{xx}a{tt}a{dd}a{ss}a{oo}a{gg})"));

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("a{yy}"));
      v = xvariant_get_child_value (parameters, 0);
      xvariant_iter_init (&iter, v);
      while (xvariant_iter_loop (&iter, "yy", &y1, &y2))
        xvariant_builder_add (&ret, "{yy}", y1 * 2, (y2 * 3) & 255);
      xvariant_unref (v);
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("a{bb}"));
      v = xvariant_get_child_value (parameters, 1);
      xvariant_iter_init (&iter, v);
      while (xvariant_iter_loop (&iter, "bb", &b1, &b2))
        xvariant_builder_add (&ret, "{bb}", b1, TRUE);
      xvariant_unref (v);
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("a{nn}"));
      v = xvariant_get_child_value (parameters, 2);
      xvariant_iter_init (&iter, v);
      while (xvariant_iter_loop (&iter, "nn", &n1, &n2))
        xvariant_builder_add (&ret, "{nn}", n1 * 2, n2 * 3);
      xvariant_unref (v);
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("a{qq}"));
      v = xvariant_get_child_value (parameters, 3);
      xvariant_iter_init (&iter, v);
      while (xvariant_iter_loop (&iter, "qq", &q1, &q2))
        xvariant_builder_add (&ret, "{qq}", q1 * 2, q2 * 3);
      xvariant_unref (v);
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("a{ii}"));
      v = xvariant_get_child_value (parameters, 4);
      xvariant_iter_init (&iter, v);
      while (xvariant_iter_loop (&iter, "ii", &i1, &i2))
        xvariant_builder_add (&ret, "{ii}", i1 * 2, i2 * 3);
      xvariant_unref (v);
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("a{uu}"));
      v = xvariant_get_child_value (parameters, 5);
      xvariant_iter_init (&iter, v);
      while (xvariant_iter_loop (&iter, "uu", &u1, &u2))
        xvariant_builder_add (&ret, "{uu}", u1 * 2, u2 * 3);
      xvariant_unref (v);
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("a{xx}"));
      v = xvariant_get_child_value (parameters, 6);
      xvariant_iter_init (&iter, v);
      while (xvariant_iter_loop (&iter, "xx", &x1, &x2))
        xvariant_builder_add (&ret, "{xx}", x1 + 2, x2  + 1);
      xvariant_unref (v);
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("a{tt}"));
      v = xvariant_get_child_value (parameters, 7);
      xvariant_iter_init (&iter, v);
      while (xvariant_iter_loop (&iter, "tt", &t1, &t2))
        xvariant_builder_add (&ret, "{tt}", t1 + 2, t2  + 1);
      xvariant_unref (v);
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("a{dd}"));
      v = xvariant_get_child_value (parameters, 8);
      xvariant_iter_init (&iter, v);
      while (xvariant_iter_loop (&iter, "dd", &d1, &d2))
        xvariant_builder_add (&ret, "{dd}", d1 + 2.5, d2  + 5.0);
      xvariant_unref (v);
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("a{ss}"));
      v = xvariant_get_child_value (parameters, 9);
      xvariant_iter_init (&iter, v);
      while (xvariant_iter_loop (&iter, "ss", &s1, &s2))
        {
          xchar_t *tmp1, *tmp2;
          tmp1 = xstrconcat (s1, "mod", NULL);
          tmp2 = xstrconcat (s2, s2, NULL);
          xvariant_builder_add (&ret, "{ss}", tmp1, tmp2);
          g_free (tmp1);
          g_free (tmp2);
        }
      xvariant_unref (v);
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("a{oo}"));
      v = xvariant_get_child_value (parameters, 10);
      xvariant_iter_init (&iter, v);
      while (xvariant_iter_loop (&iter, "oo", &s1, &s2))
        {
          xchar_t *tmp1, *tmp2;
          tmp1 = xstrconcat (s1, "/mod", NULL);
          tmp2 = xstrconcat (s2, "/mod2", NULL);
          xvariant_builder_add (&ret, "{oo}", tmp1, tmp2);
          g_free (tmp1);
          g_free (tmp2);
        }
      xvariant_unref (v);
      xvariant_builder_close (&ret);

      xvariant_builder_open (&ret, G_VARIANT_TYPE ("a{gg}"));
      v = xvariant_get_child_value (parameters, 11);
      xvariant_iter_init (&iter, v);
      while (xvariant_iter_loop (&iter, "gg", &s1, &s2))
        {
          xchar_t *tmp1, *tmp2;
          tmp1 = xstrconcat (s1, "assgit", NULL);
          tmp2 = xstrconcat (s2, s2, NULL);
          xvariant_builder_add (&ret, "{gg}", tmp1, tmp2);
          g_free (tmp1);
          g_free (tmp2);
        }
      xvariant_unref (v);
      xvariant_builder_close (&ret);

      xdbus_method_invocation_return_value (invocation,
                                             xvariant_builder_end (&ret));
    }
  else if (xstrcmp0 (method_name, "TestStructureTypes") == 0)
    {
      xint_t x, y, x1, y1;
      const xchar_t *desc;
      xvariant_iter_t *iter1, *iter2;
      xchar_t *desc_ret;
      xvariant_builder_t ret1, ret2;
      xvariant_iter_t *iter;
      xvariant_t *v;
      xchar_t *s1, *s2;

      xvariant_get (parameters, "((ii)(&s(ii)aya{ss}))",
                     &x, &y, &desc, &x1, &y1, &iter1, &iter2);

      desc_ret = xstrconcat (desc, "... in bed!", NULL);

      xvariant_builder_init (&ret1, G_VARIANT_TYPE ("ay"));
      iter = xvariant_iter_copy (iter1);
      while (xvariant_iter_loop (iter1, "y", &v))
        xvariant_builder_add (&ret1, "y", v);
      while (xvariant_iter_loop (iter, "y", &v))
        xvariant_builder_add (&ret1, "y", v);
      xvariant_iter_free (iter);
      xvariant_iter_free (iter1);

      xvariant_builder_init (&ret2, G_VARIANT_TYPE ("a{ss}"));
      while (xvariant_iter_loop (iter1, "ss", &s1, &s2))
        {
          xchar_t *tmp;
          tmp = xstrconcat (s2, " ... in bed!", NULL);
          xvariant_builder_add (&ret1, "{ss}", s1, tmp);
          g_free (tmp);
        }
      xvariant_iter_free (iter2);

      xdbus_method_invocation_return_value (invocation,
           xvariant_new ("((ii)(&s(ii)aya{ss}))",
                          x + 1, y + 1, desc_ret, x1 + 2, y1 + 2,
                          &ret1, &ret2));

      g_free (desc_ret);
    }
  else if (xstrcmp0 (method_name, "TestVariant") == 0)
    {
      xvariant_t *v;
      xboolean_t modify;
      xvariant_t *ret;

      xvariant_get (parameters, "(vb)", &v, &modify);

      /* FIXME handle more cases */
      if (modify)
        {
          if (xvariant_is_of_type (v, G_VARIANT_TYPE_BOOLEAN))
            {
              ret = xvariant_new_boolean (FALSE);
            }
          else if (xvariant_is_of_type (v, G_VARIANT_TYPE_TUPLE))
            {
              ret = xvariant_new ("(si)", "other struct", 100);
            }
          else
            g_assert_not_reached ();
        }
      else
        ret = v;

      xdbus_method_invocation_return_value (invocation, ret);
      xvariant_unref (v);
    }
  else if (xstrcmp0 (method_name, "TestComplexArrays") == 0)
    {
      /* FIXME */
      xdbus_method_invocation_return_value (invocation, parameters);
    }
  else if (xstrcmp0 (method_name, "TestComplexHashTables") == 0)
    {
      /* FIXME */
      xdbus_method_invocation_return_value (invocation, parameters);
    }
  else if (xstrcmp0 (method_name, "FrobSetProperty") == 0)
    {
      xchar_t *name;
      xvariant_t *value;
      xvariant_get (parameters, "(sv)", &name, &value);
      xhash_table_replace (properties, name, value);
      xdbus_connection_emit_signal (connection,
                                     NULL,
                                     "/com/example/test_object_t",
                                     "org.freedesktop.DBus.Properties",
                                     "PropertiesChanged",
                                     xvariant_new_parsed ("('com.example.Frob', [{%s, %v}], @as [])", name, value),
                                     NULL);
      xdbus_method_invocation_return_value (invocation, NULL);
    }
  else if (xstrcmp0 (method_name, "FrobInvalidateProperty") == 0)
    {
      const xchar_t *value;
      xvariant_get (parameters, "(&s)", &value);
      xhash_table_replace (properties, xstrdup ("PropertyThatWillBeInvalidated"), xvariant_ref_sink (xvariant_new_string (value)));

      xdbus_connection_emit_signal (connection,
                                     NULL,
                                     "/com/example/test_object_t",
                                     "org.freedesktop.DBus.Properties",
                                     "PropertiesChanged",
                                     xvariant_new_parsed ("('com.example.Frob', @a{sv} [], ['PropertyThatWillBeInvalidated'])"),
                                     NULL);
      xdbus_method_invocation_return_value (invocation, NULL);
    }
  else if (xstrcmp0 (method_name, "EmitSignal") == 0)
    {
      const xchar_t *str;
      const xchar_t *path;
      xchar_t *str_ret;
      xchar_t *path_ret;
      xvariant_get (parameters, "(&s&o)", &str, &path);
      str_ret = xstrconcat (str, " .. in bed!", NULL);
      path_ret = xstrconcat (path, "/in/bed", NULL);
      xdbus_connection_emit_signal (connection,
                                     NULL,
                                     "/com/example/test_object_t",
                                     "com.example.Frob",
                                     "TestSignal",
                                     xvariant_new_parsed ("(%s, %o, <'a variant'>)", str_ret, path_ret),
                                     NULL);
      g_free (str_ret);
      g_free (path_ret);
      xdbus_method_invocation_return_value (invocation, NULL);
    }
  else if (xstrcmp0 (method_name, "EmitSignal2") == 0)
    {
      xdbus_connection_emit_signal (connection,
                                     NULL,
                                     "/com/example/test_object_t",
                                     "com.example.Frob",
                                     "TestSignal2",
                                     xvariant_new_parsed ("(42, )"),
                                     NULL);
      xdbus_method_invocation_return_value (invocation, NULL);
    }
  else if (xstrcmp0 (method_name, "Sleep") == 0)
    {
      xint_t msec;

      xvariant_get (parameters, "(i)", &msec);

      g_timeout_add ((xuint_t)msec, end_sleep, xobject_ref (invocation));
    }
  else if (xstrcmp0 (method_name, "Quit") == 0)
    {
      xdbus_method_invocation_return_value (invocation, NULL);
      xmain_loop_quit (loop);
    }
}

static xvariant_t *
handle_get_property (xdbus_connection_t  *connection,
                     const xchar_t      *sender,
                     const xchar_t      *object_path,
                     const xchar_t      *interface_name,
                     const xchar_t      *property_name,
                     xerror_t          **error,
                     xpointer_t          user_data)
{
  xvariant_t *ret;

  ret = xhash_table_lookup (properties, property_name);
  if (ret)
    {
      g_assert (!xvariant_is_floating (ret));
      xvariant_ref (ret);
    }
  else
    {
      g_set_error (error,
                   G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                   "no such property: %s", property_name);
    }

  return ret;
}

static xboolean_t
handle_set_property (xdbus_connection_t  *connection,
                     const xchar_t      *sender,
                     const xchar_t      *object_path,
                     const xchar_t      *interface_name,
                     const xchar_t      *property_name,
                     xvariant_t         *value,
                     xerror_t          **error,
                     xpointer_t          user_data)
{
  g_set_error (error,
               G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
               "SetProperty not implemented");
  return FALSE;
}

static const xdbus_interface_vtable_t interface_vtable =
{
  handle_method_call,
  handle_get_property,
  handle_set_property,
  { 0 }
};

static void
on_bus_acquired (xdbus_connection_t *connection,
                 const xchar_t     *name,
                 xpointer_t         user_data)
{
  xuint_t id;

  id = xdbus_connection_register_object (connection,
                                          "/com/example/test_object_t",
                                          introspection_data->interfaces[0],
                                          &interface_vtable,
                                          NULL,
                                          NULL,
                                          NULL);
  g_assert (id > 0);
}

static void
on_name_acquired (xdbus_connection_t *connection,
                  const xchar_t     *name,
                  xpointer_t         user_data)
{
}

static void
on_name_lost (xdbus_connection_t *connection,
              const xchar_t     *name,
              xpointer_t         user_data)
{
  exit (1);
}

int
main (int argc, char *argv[])
{
  xuint_t owner_id;

  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  properties = xhash_table_new_full (xstr_hash, xstr_equal, g_free, (xdestroy_notify_t)xvariant_unref);
  xhash_table_insert (properties, xstrdup ("y"), xvariant_ref_sink (xvariant_new_byte (1)));
  xhash_table_insert (properties, xstrdup ("b"), xvariant_ref_sink (xvariant_new_boolean (TRUE)));
  xhash_table_insert (properties, xstrdup ("n"), xvariant_ref_sink (xvariant_new_int16 (2)));
  xhash_table_insert (properties, xstrdup ("q"), xvariant_ref_sink (xvariant_new_uint16 (3)));
  xhash_table_insert (properties, xstrdup ("i"), xvariant_ref_sink (xvariant_new_int32 (4)));
  xhash_table_insert (properties, xstrdup ("u"), xvariant_ref_sink (xvariant_new_uint32 (5)));
  xhash_table_insert (properties, xstrdup ("x"), xvariant_ref_sink (xvariant_new_int64 (6)));
  xhash_table_insert (properties, xstrdup ("t"), xvariant_ref_sink (xvariant_new_uint64 (7)));
  xhash_table_insert (properties, xstrdup ("d"), xvariant_ref_sink (xvariant_new_double (7.5)));
  xhash_table_insert (properties, xstrdup ("s"), xvariant_ref_sink (xvariant_new_string ("a string")));
  xhash_table_insert (properties, xstrdup ("o"), xvariant_ref_sink (xvariant_new_object_path ("/some/path")));
  xhash_table_insert (properties, xstrdup ("ay"), xvariant_ref_sink (xvariant_new_parsed ("[@y 1, @y 11]")));
  xhash_table_insert (properties, xstrdup ("ab"), xvariant_ref_sink (xvariant_new_parsed ("[true, false]")));
  xhash_table_insert (properties, xstrdup ("an"), xvariant_ref_sink (xvariant_new_parsed ("[@n 2, @n 12]")));
  xhash_table_insert (properties, xstrdup ("aq"), xvariant_ref_sink (xvariant_new_parsed ("[@q 3, @q 13]")));
  xhash_table_insert (properties, xstrdup ("ai"), xvariant_ref_sink (xvariant_new_parsed ("[@i 4, @i 14]")));
  xhash_table_insert (properties, xstrdup ("au"), xvariant_ref_sink (xvariant_new_parsed ("[@u 5, @u 15]")));
  xhash_table_insert (properties, xstrdup ("ax"), xvariant_ref_sink (xvariant_new_parsed ("[@x 6, @x 16]")));
  xhash_table_insert (properties, xstrdup ("at"), xvariant_ref_sink (xvariant_new_parsed ("[@t 7, @t 17]")));
  xhash_table_insert (properties, xstrdup ("ad"), xvariant_ref_sink (xvariant_new_parsed ("[7.5, 17.5]")));
  xhash_table_insert (properties, xstrdup ("as"), xvariant_ref_sink (xvariant_new_parsed ("['a string', 'another string']")));
  xhash_table_insert (properties, xstrdup ("ao"), xvariant_ref_sink (xvariant_new_parsed ("[@o '/some/path', @o '/another/path']")));
  xhash_table_insert (properties, xstrdup ("foo"), xvariant_ref_sink (xvariant_new_string ("a frobbed string")));
  xhash_table_insert (properties, xstrdup ("PropertyThatWillBeInvalidated"), xvariant_ref_sink (xvariant_new_string ("InitialValue")));

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "com.example.TestService",
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             NULL,
                             NULL);

  loop = xmain_loop_new (NULL, FALSE);
  xmain_loop_run (loop);

  g_bus_unown_name (owner_id);

  g_dbus_node_info_unref (introspection_data);
  xhash_table_unref (properties);

  return 0;
}
