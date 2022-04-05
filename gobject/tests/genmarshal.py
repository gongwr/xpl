#!/usr/bin/python3
# -*- coding: utf-8 -*-
#
# Copyright © 2019 Endless Mobile, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301  USA

"""Integration tests for glib-genmarshal utility."""

import collections
import os
import shutil
import subprocess
import sys
import tempfile
from textwrap import dedent
import unittest

import taptestrunner


# Disable line length warnings as wrapping the C code templates would be hard
# flake8: noqa: E501


Result = collections.namedtuple("Result", ("info", "out", "err", "subs"))


class TestGenmarshal(unittest.TestCase):
    """Integration test for running glib-genmarshal.

    This can be run when installed or uninstalled. When uninstalled, it
    requires G_TEST_BUILDDIR and G_TEST_SRCDIR to be set.

    The idea with this test harness is to test the glib-genmarshal utility, its
    handling of command line arguments, its exit statuses, and its handling of
    various marshaller lists. In future we could split the core glib-genmarshal
    parsing and generation code out into a library and unit test that, and
    convert this test to just check command line behaviour.
    """

    # Track the cwd, we want to back out to that to clean up our tempdir
    cwd = ""

    def setUp(self):
        self.timeout_seconds = 10  # seconds per test
        self.tmpdir = tempfile.TemporaryDirectory()
        self.cwd = os.getcwd()
        os.chdir(self.tmpdir.name)
        print("tmpdir:", self.tmpdir.name)
        if "G_TEST_BUILDDIR" in os.environ:
            self.__genmarshal = os.path.join(
                os.environ["G_TEST_BUILDDIR"], "..", "glib-genmarshal"
            )
        else:
            self.__genmarshal = shutil.which("glib-genmarshal")
        print("genmarshal:", self.__genmarshal)

    def tearDown(self):
        os.chdir(self.cwd)
        self.tmpdir.cleanup()

    def runGenmarshal(self, *args):
        argv = [self.__genmarshal]

        # shebang lines are not supported on native
        # Windows consoles
        if os.name == "nt":
            argv.insert(0, sys.executable)

        argv.extend(args)
        print("Running:", argv)

        env = os.environ.copy()
        env["LC_ALL"] = "C.UTF-8"
        print("Environment:", env)

        # We want to ensure consistent line endings...
        info = subprocess.run(
            argv,
            timeout=self.timeout_seconds,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=env,
            universal_newlines=True,
        )
        info.check_returncode()
        out = info.stdout.strip()
        err = info.stderr.strip()

        # Known substitutions for standard boilerplate
        subs = {
            "standard_top_comment": "This file is generated by glib-genmarshal, do not modify "
            "it. This code is licensed under the same license as the "
            "containing project. Note that it links to GLib, so must "
            "comply with the LGPL linking clauses.",
            "standard_top_pragma": dedent(
                """
                #ifndef __G_CCLOSURE_USER_MARSHAL_MARSHAL_H__
                #define __G_CCLOSURE_USER_MARSHAL_MARSHAL_H__
                """
            ).strip(),
            "standard_bottom_pragma": dedent(
                """
                #endif /* __G_CCLOSURE_USER_MARSHAL_MARSHAL_H__ */
                """
            ).strip(),
            "standard_includes": dedent(
                """
                #include <glib-object.h>
                """
            ).strip(),
            "standard_marshal_peek_defines": dedent(
                """
                #ifdef G_ENABLE_DEBUG
                #define g_marshal_value_peek_boolean(v)  g_value_get_boolean (v)
                #define g_marshal_value_peek_char(v)     g_value_get_schar (v)
                #define g_marshal_value_peek_uchar(v)    g_value_get_uchar (v)
                #define g_marshal_value_peek_int(v)      g_value_get_int (v)
                #define g_marshal_value_peek_uint(v)     g_value_get_uint (v)
                #define g_marshal_value_peek_long(v)     g_value_get_long (v)
                #define g_marshal_value_peek_ulong(v)    g_value_get_ulong (v)
                #define g_marshal_value_peek_int64(v)    g_value_get_int64 (v)
                #define g_marshal_value_peek_uint64(v)   g_value_get_uint64 (v)
                #define g_marshal_value_peek_enum(v)     g_value_get_enum (v)
                #define g_marshal_value_peek_flags(v)    g_value_get_flags (v)
                #define g_marshal_value_peek_float(v)    g_value_get_float (v)
                #define g_marshal_value_peek_double(v)   g_value_get_double (v)
                #define g_marshal_value_peek_string(v)   (char*) g_value_get_string (v)
                #define g_marshal_value_peek_param(v)    g_value_get_param (v)
                #define g_marshal_value_peek_boxed(v)    g_value_get_boxed (v)
                #define g_marshal_value_peek_pointer(v)  g_value_get_pointer (v)
                #define g_marshal_value_peek_object(v)   g_value_get_object (v)
                #define g_marshal_value_peek_variant(v)  g_value_get_variant (v)
                #else /* !G_ENABLE_DEBUG */
                /* WARNING: This code accesses GValues directly, which is UNSUPPORTED API.
                 *          Do not access GValues directly in your code. Instead, use the
                 *          g_value_get_*() functions
                 */
                #define g_marshal_value_peek_boolean(v)  (v)->data[0].v_int
                #define g_marshal_value_peek_char(v)     (v)->data[0].v_int
                #define g_marshal_value_peek_uchar(v)    (v)->data[0].v_uint
                #define g_marshal_value_peek_int(v)      (v)->data[0].v_int
                #define g_marshal_value_peek_uint(v)     (v)->data[0].v_uint
                #define g_marshal_value_peek_long(v)     (v)->data[0].v_long
                #define g_marshal_value_peek_ulong(v)    (v)->data[0].v_ulong
                #define g_marshal_value_peek_int64(v)    (v)->data[0].v_int64
                #define g_marshal_value_peek_uint64(v)   (v)->data[0].v_uint64
                #define g_marshal_value_peek_enum(v)     (v)->data[0].v_long
                #define g_marshal_value_peek_flags(v)    (v)->data[0].v_ulong
                #define g_marshal_value_peek_float(v)    (v)->data[0].v_float
                #define g_marshal_value_peek_double(v)   (v)->data[0].v_double
                #define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer
                #define g_marshal_value_peek_param(v)    (v)->data[0].v_pointer
                #define g_marshal_value_peek_boxed(v)    (v)->data[0].v_pointer
                #define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer
                #define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer
                #define g_marshal_value_peek_variant(v)  (v)->data[0].v_pointer
                #endif /* !G_ENABLE_DEBUG */
                """
            ).strip(),
        }

        result = Result(info, out, err, subs)

        print("Output:", result.out)
        return result

    def runGenmarshalWithList(self, list_contents, *args):
        with tempfile.NamedTemporaryFile(
            dir=self.tmpdir.name, suffix=".list", delete=False
        ) as list_file:
            # Write out the list.
            list_file.write(list_contents.encode("utf-8"))
            print(list_file.name + ":", list_contents)
            list_file.flush()

            header_result = self.runGenmarshal(list_file.name, "--header", *args)
            body_result = self.runGenmarshal(list_file.name, "--body", *args)

            header_result.subs["list_path"] = list_file.name
            body_result.subs["list_path"] = list_file.name

            return (header_result, body_result)

    def test_help(self):
        """Test the --help argument."""
        result = self.runGenmarshal("--help")
        self.assertIn("usage: glib-genmarshal", result.out)

    def test_no_args(self):
        """Test running with no arguments at all."""
        result = self.runGenmarshal()
        self.assertEqual("", result.err)
        self.assertEqual("", result.out)

    def test_empty_list(self):
        """Test running with an empty list."""
        (header_result, body_result) = self.runGenmarshalWithList("", "--quiet")

        self.assertEqual("", header_result.err)
        self.assertEqual("", body_result.err)

        self.assertEqual(
            dedent(
                """
            /* {standard_top_comment} */
            {standard_top_pragma}

            {standard_includes}

            G_BEGIN_DECLS


            G_END_DECLS

            {standard_bottom_pragma}
            """
            )
            .strip()
            .format(**header_result.subs),
            header_result.out.strip(),
        )

        self.assertEqual(
            dedent(
                """
            /* {standard_top_comment} */
            {standard_includes}

            {standard_marshal_peek_defines}
            """
            )
            .strip()
            .format(**body_result.subs),
            body_result.out.strip(),
        )

    def test_void_boolean(self):
        """Test running with a basic VOID:BOOLEAN list."""
        (header_result, body_result) = self.runGenmarshalWithList(
            "VOID:BOOLEAN", "--quiet"
        )

        self.assertEqual("", header_result.err)
        self.assertEqual("", body_result.err)

        self.assertEqual(
            dedent(
                """
            /* {standard_top_comment} */
            {standard_top_pragma}

            {standard_includes}

            G_BEGIN_DECLS

            /* VOID:BOOLEAN ({list_path}:1) */
            #define g_cclosure_user_marshal_VOID__BOOLEAN	g_cclosure_marshal_VOID__BOOLEAN


            G_END_DECLS

            {standard_bottom_pragma}
            """
            )
            .strip()
            .format(**header_result.subs),
            header_result.out.strip(),
        )

        self.assertEqual(
            dedent(
                """
            /* {standard_top_comment} */
            {standard_includes}

            {standard_marshal_peek_defines}
            """
            )
            .strip()
            .format(**body_result.subs),
            body_result.out.strip(),
        )

    def test_void_boolean_int64(self):
        """Test running with a non-trivial VOID:BOOLEAN,INT64 list."""
        (header_result, body_result) = self.runGenmarshalWithList(
            "VOID:BOOLEAN,INT64", "--quiet"
        )

        self.assertEqual("", header_result.err)
        self.assertEqual("", body_result.err)

        self.assertEqual(
            dedent(
                """
            /* {standard_top_comment} */
            {standard_top_pragma}

            {standard_includes}

            G_BEGIN_DECLS

            /* VOID:BOOLEAN,INT64 ({list_path}:1) */
            extern
            void g_cclosure_user_marshal_VOID__BOOLEAN_INT64 (GClosure     *closure,
                                                              GValue       *return_value,
                                                              xuint_t         n_param_values,
                                                              const GValue *param_values,
                                                              xpointer_t      invocation_hint,
                                                              xpointer_t      marshal_data);


            G_END_DECLS

            {standard_bottom_pragma}
            """
            )
            .strip()
            .format(**header_result.subs),
            header_result.out.strip(),
        )

        self.assertEqual(
            dedent(
                """
            /* {standard_top_comment} */
            {standard_includes}

            {standard_marshal_peek_defines}

            /* VOID:BOOLEAN,INT64 ({list_path}:1) */
            void
            g_cclosure_user_marshal_VOID__BOOLEAN_INT64 (GClosure     *closure,
                                                         GValue       *return_value G_GNUC_UNUSED,
                                                         xuint_t         n_param_values,
                                                         const GValue *param_values,
                                                         xpointer_t      invocation_hint G_GNUC_UNUSED,
                                                         xpointer_t      marshal_data)
            {{
              typedef void (*GMarshalFunc_VOID__BOOLEAN_INT64) (xpointer_t data1,
                                                                xboolean_t arg1,
                                                                gint64 arg2,
                                                                xpointer_t data2);
              GCClosure *cc = (GCClosure *) closure;
              xpointer_t data1, data2;
              GMarshalFunc_VOID__BOOLEAN_INT64 callback;

              g_return_if_fail (n_param_values == 3);

              if (G_CCLOSURE_SWAP_DATA (closure))
                {{
                  data1 = closure->data;
                  data2 = g_value_peek_pointer (param_values + 0);
                }}
              else
                {{
                  data1 = g_value_peek_pointer (param_values + 0);
                  data2 = closure->data;
                }}
              callback = (GMarshalFunc_VOID__BOOLEAN_INT64) (marshal_data ? marshal_data : cc->callback);

              callback (data1,
                        g_marshal_value_peek_boolean (param_values + 1),
                        g_marshal_value_peek_int64 (param_values + 2),
                        data2);
            }}
            """
            )
            .strip()
            .format(**body_result.subs),
            body_result.out.strip(),
        )

    def test_void_variant_nostdinc_valist_marshaller(self):
        """Test running with a basic VOID:VARIANT list, but without the
        standard marshallers, and with valist support enabled. This checks that
        the valist marshaller for VARIANT correctly sinks floating variants.

        See issue #1793.
        """
        (header_result, body_result) = self.runGenmarshalWithList(
            "VOID:VARIANT", "--quiet", "--nostdinc", "--valist-marshaller"
        )

        self.assertEqual("", header_result.err)
        self.assertEqual("", body_result.err)

        self.assertEqual(
            dedent(
                """
            /* {standard_top_comment} */
            {standard_top_pragma}

            G_BEGIN_DECLS

            /* VOID:VARIANT ({list_path}:1) */
            extern
            void g_cclosure_user_marshal_VOID__VARIANT (GClosure     *closure,
                                                        GValue       *return_value,
                                                        xuint_t         n_param_values,
                                                        const GValue *param_values,
                                                        xpointer_t      invocation_hint,
                                                        xpointer_t      marshal_data);
            extern
            void g_cclosure_user_marshal_VOID__VARIANTv (GClosure *closure,
                                                         GValue   *return_value,
                                                         xpointer_t  instance,
                                                         va_list   args,
                                                         xpointer_t  marshal_data,
                                                         int       n_params,
                                                         xtype_t    *param_types);


            G_END_DECLS

            {standard_bottom_pragma}
            """
            )
            .strip()
            .format(**header_result.subs),
            header_result.out.strip(),
        )

        self.assertEqual(
            dedent(
                """
            /* {standard_top_comment} */
            {standard_marshal_peek_defines}

            /* VOID:VARIANT ({list_path}:1) */
            void
            g_cclosure_user_marshal_VOID__VARIANT (GClosure     *closure,
                                                   GValue       *return_value G_GNUC_UNUSED,
                                                   xuint_t         n_param_values,
                                                   const GValue *param_values,
                                                   xpointer_t      invocation_hint G_GNUC_UNUSED,
                                                   xpointer_t      marshal_data)
            {{
              typedef void (*GMarshalFunc_VOID__VARIANT) (xpointer_t data1,
                                                          xpointer_t arg1,
                                                          xpointer_t data2);
              GCClosure *cc = (GCClosure *) closure;
              xpointer_t data1, data2;
              GMarshalFunc_VOID__VARIANT callback;

              g_return_if_fail (n_param_values == 2);

              if (G_CCLOSURE_SWAP_DATA (closure))
                {{
                  data1 = closure->data;
                  data2 = g_value_peek_pointer (param_values + 0);
                }}
              else
                {{
                  data1 = g_value_peek_pointer (param_values + 0);
                  data2 = closure->data;
                }}
              callback = (GMarshalFunc_VOID__VARIANT) (marshal_data ? marshal_data : cc->callback);

              callback (data1,
                        g_marshal_value_peek_variant (param_values + 1),
                        data2);
            }}

            void
            g_cclosure_user_marshal_VOID__VARIANTv (GClosure *closure,
                                                    GValue   *return_value G_GNUC_UNUSED,
                                                    xpointer_t  instance,
                                                    va_list   args,
                                                    xpointer_t  marshal_data,
                                                    int       n_params,
                                                    xtype_t    *param_types)
            {{
              typedef void (*GMarshalFunc_VOID__VARIANT) (xpointer_t data1,
                                                          xpointer_t arg1,
                                                          xpointer_t data2);
              GCClosure *cc = (GCClosure *) closure;
              xpointer_t data1, data2;
              GMarshalFunc_VOID__VARIANT callback;
              xpointer_t arg0;
              va_list args_copy;

              G_VA_COPY (args_copy, args);
              arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
              if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
                arg0 = g_variant_ref_sink (arg0);
              va_end (args_copy);


              if (G_CCLOSURE_SWAP_DATA (closure))
                {{
                  data1 = closure->data;
                  data2 = instance;
                }}
              else
                {{
                  data1 = instance;
                  data2 = closure->data;
                }}
              callback = (GMarshalFunc_VOID__VARIANT) (marshal_data ? marshal_data : cc->callback);

              callback (data1,
                        arg0,
                        data2);
              if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
                g_variant_unref (arg0);
            }}
            """
            )
            .strip()
            .format(**body_result.subs),
            body_result.out.strip(),
        )

    def test_void_string_nostdinc(self):
        """Test running with a basic VOID:STRING list, but without the
        standard marshallers, and with valist support enabled. This checks that
        the valist marshaller for STRING correctly skips a string copy if the
        argument is static.

        See issue #1792.
        """
        (header_result, body_result) = self.runGenmarshalWithList(
            "VOID:STRING", "--quiet", "--nostdinc", "--valist-marshaller"
        )

        self.assertEqual("", header_result.err)
        self.assertEqual("", body_result.err)

        self.assertEqual(
            dedent(
                """
            /* {standard_top_comment} */
            {standard_top_pragma}

            G_BEGIN_DECLS

            /* VOID:STRING ({list_path}:1) */
            extern
            void g_cclosure_user_marshal_VOID__STRING (GClosure     *closure,
                                                       GValue       *return_value,
                                                       xuint_t         n_param_values,
                                                       const GValue *param_values,
                                                       xpointer_t      invocation_hint,
                                                       xpointer_t      marshal_data);
            extern
            void g_cclosure_user_marshal_VOID__STRINGv (GClosure *closure,
                                                        GValue   *return_value,
                                                        xpointer_t  instance,
                                                        va_list   args,
                                                        xpointer_t  marshal_data,
                                                        int       n_params,
                                                        xtype_t    *param_types);


            G_END_DECLS

            {standard_bottom_pragma}
            """
            )
            .strip()
            .format(**header_result.subs),
            header_result.out.strip(),
        )

        self.assertEqual(
            dedent(
                """
            /* {standard_top_comment} */
            {standard_marshal_peek_defines}

            /* VOID:STRING ({list_path}:1) */
            void
            g_cclosure_user_marshal_VOID__STRING (GClosure     *closure,
                                                  GValue       *return_value G_GNUC_UNUSED,
                                                  xuint_t         n_param_values,
                                                  const GValue *param_values,
                                                  xpointer_t      invocation_hint G_GNUC_UNUSED,
                                                  xpointer_t      marshal_data)
            {{
              typedef void (*GMarshalFunc_VOID__STRING) (xpointer_t data1,
                                                         xpointer_t arg1,
                                                         xpointer_t data2);
              GCClosure *cc = (GCClosure *) closure;
              xpointer_t data1, data2;
              GMarshalFunc_VOID__STRING callback;

              g_return_if_fail (n_param_values == 2);

              if (G_CCLOSURE_SWAP_DATA (closure))
                {{
                  data1 = closure->data;
                  data2 = g_value_peek_pointer (param_values + 0);
                }}
              else
                {{
                  data1 = g_value_peek_pointer (param_values + 0);
                  data2 = closure->data;
                }}
              callback = (GMarshalFunc_VOID__STRING) (marshal_data ? marshal_data : cc->callback);

              callback (data1,
                        g_marshal_value_peek_string (param_values + 1),
                        data2);
            }}

            void
            g_cclosure_user_marshal_VOID__STRINGv (GClosure *closure,
                                                   GValue   *return_value G_GNUC_UNUSED,
                                                   xpointer_t  instance,
                                                   va_list   args,
                                                   xpointer_t  marshal_data,
                                                   int       n_params,
                                                   xtype_t    *param_types)
            {{
              typedef void (*GMarshalFunc_VOID__STRING) (xpointer_t data1,
                                                         xpointer_t arg1,
                                                         xpointer_t data2);
              GCClosure *cc = (GCClosure *) closure;
              xpointer_t data1, data2;
              GMarshalFunc_VOID__STRING callback;
              xpointer_t arg0;
              va_list args_copy;

              G_VA_COPY (args_copy, args);
              arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
              if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
                arg0 = g_strdup (arg0);
              va_end (args_copy);


              if (G_CCLOSURE_SWAP_DATA (closure))
                {{
                  data1 = closure->data;
                  data2 = instance;
                }}
              else
                {{
                  data1 = instance;
                  data2 = closure->data;
                }}
              callback = (GMarshalFunc_VOID__STRING) (marshal_data ? marshal_data : cc->callback);

              callback (data1,
                        arg0,
                        data2);
              if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
                g_free (arg0);
            }}
            """
            )
            .strip()
            .format(**body_result.subs),
            body_result.out.strip(),
        )

    def test_void_param_nostdinc(self):
        """Test running with a basic VOID:PARAM list, but without the
        standard marshallers, and with valist support enabled. This checks that
        the valist marshaller for PARAM correctly skips a param copy if the
        argument is static.

        See issue #1792.
        """
        self.maxDiff = None  # TODO
        (header_result, body_result) = self.runGenmarshalWithList(
            "VOID:PARAM", "--quiet", "--nostdinc", "--valist-marshaller"
        )

        self.assertEqual("", header_result.err)
        self.assertEqual("", body_result.err)

        self.assertEqual(
            dedent(
                """
            /* {standard_top_comment} */
            {standard_top_pragma}

            G_BEGIN_DECLS

            /* VOID:PARAM ({list_path}:1) */
            extern
            void g_cclosure_user_marshal_VOID__PARAM (GClosure     *closure,
                                                      GValue       *return_value,
                                                      xuint_t         n_param_values,
                                                      const GValue *param_values,
                                                      xpointer_t      invocation_hint,
                                                      xpointer_t      marshal_data);
            extern
            void g_cclosure_user_marshal_VOID__PARAMv (GClosure *closure,
                                                       GValue   *return_value,
                                                       xpointer_t  instance,
                                                       va_list   args,
                                                       xpointer_t  marshal_data,
                                                       int       n_params,
                                                       xtype_t    *param_types);


            G_END_DECLS

            {standard_bottom_pragma}
            """
            )
            .strip()
            .format(**header_result.subs),
            header_result.out.strip(),
        )

        self.assertEqual(
            dedent(
                """
            /* {standard_top_comment} */
            {standard_marshal_peek_defines}

            /* VOID:PARAM ({list_path}:1) */
            void
            g_cclosure_user_marshal_VOID__PARAM (GClosure     *closure,
                                                 GValue       *return_value G_GNUC_UNUSED,
                                                 xuint_t         n_param_values,
                                                 const GValue *param_values,
                                                 xpointer_t      invocation_hint G_GNUC_UNUSED,
                                                 xpointer_t      marshal_data)
            {{
              typedef void (*GMarshalFunc_VOID__PARAM) (xpointer_t data1,
                                                        xpointer_t arg1,
                                                        xpointer_t data2);
              GCClosure *cc = (GCClosure *) closure;
              xpointer_t data1, data2;
              GMarshalFunc_VOID__PARAM callback;

              g_return_if_fail (n_param_values == 2);

              if (G_CCLOSURE_SWAP_DATA (closure))
                {{
                  data1 = closure->data;
                  data2 = g_value_peek_pointer (param_values + 0);
                }}
              else
                {{
                  data1 = g_value_peek_pointer (param_values + 0);
                  data2 = closure->data;
                }}
              callback = (GMarshalFunc_VOID__PARAM) (marshal_data ? marshal_data : cc->callback);

              callback (data1,
                        g_marshal_value_peek_param (param_values + 1),
                        data2);
            }}

            void
            g_cclosure_user_marshal_VOID__PARAMv (GClosure *closure,
                                                  GValue   *return_value G_GNUC_UNUSED,
                                                  xpointer_t  instance,
                                                  va_list   args,
                                                  xpointer_t  marshal_data,
                                                  int       n_params,
                                                  xtype_t    *param_types)
            {{
              typedef void (*GMarshalFunc_VOID__PARAM) (xpointer_t data1,
                                                        xpointer_t arg1,
                                                        xpointer_t data2);
              GCClosure *cc = (GCClosure *) closure;
              xpointer_t data1, data2;
              GMarshalFunc_VOID__PARAM callback;
              xpointer_t arg0;
              va_list args_copy;

              G_VA_COPY (args_copy, args);
              arg0 = (xpointer_t) va_arg (args_copy, xpointer_t);
              if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
                arg0 = g_param_spec_ref (arg0);
              va_end (args_copy);


              if (G_CCLOSURE_SWAP_DATA (closure))
                {{
                  data1 = closure->data;
                  data2 = instance;
                }}
              else
                {{
                  data1 = instance;
                  data2 = closure->data;
                }}
              callback = (GMarshalFunc_VOID__PARAM) (marshal_data ? marshal_data : cc->callback);

              callback (data1,
                        arg0,
                        data2);
              if ((param_types[0] & G_SIGNAL_TYPE_STATIC_SCOPE) == 0 && arg0 != NULL)
                g_param_spec_unref (arg0);
            }}
            """
            )
            .strip()
            .format(**body_result.subs),
            body_result.out.strip(),
        )


if __name__ == "__main__":
    unittest.main(testRunner=taptestrunner.TAPTestRunner())