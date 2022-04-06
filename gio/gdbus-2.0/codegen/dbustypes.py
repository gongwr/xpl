# -*- Mode: Python -*-

# GDBus - GLib D-Bus Library
#
# Copyright (C) 2008-2011 Red Hat, Inc.
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
# You should have received a copy of the GNU Lesser General
# Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
#
# Author: David Zeuthen <davidz@redhat.com>

from . import utils
from .utils import print_error


class Annotation:
    def __init__(self, key, value):
        self.key = key
        self.value = value
        self.annotations = []
        self.since = ""

    def post_process(self, interface_prefix, cns, cns_upper, cns_lower, container):
        key = self.key
        overridden_key = utils.lookup_annotation(
            self.annotations, "org.gtk.GDBus.C.Name"
        )
        if utils.is_ugly_case(overridden_key):
            self.key_lower = overridden_key.lower()
        else:
            if overridden_key:
                key = overridden_key
            self.key_lower = (
                utils.camel_case_to_uscore(key)
                .lower()
                .replace("-", "_")
                .replace(".", "_")
            )

        if len(self.since) == 0:
            self.since = utils.lookup_since(self.annotations)
            if len(self.since) == 0:
                self.since = container.since

        for a in self.annotations:
            a.post_process(interface_prefix, cns, cns_upper, cns_lower, self)


class Arg:
    def __init__(self, name, signature):
        self.name = name
        self.signature = signature
        self.annotations = []
        self.doc_string = ""
        self.since = ""

    def post_process(self, interface_prefix, cns, cns_upper, cns_lower, arg_number):
        if len(self.doc_string) == 0:
            self.doc_string = utils.lookup_docs(self.annotations)
        if len(self.since) == 0:
            self.since = utils.lookup_since(self.annotations)

        if self.name is None:
            self.name = "unnamed_arg%d" % arg_number
        # default to xvariant_t
        self.ctype_in_g = "xvariant_t *"
        self.ctype_in = "xvariant_t *"
        self.ctype_in_dup = "xvariant_t *"
        self.ctype_out = "xvariant_t **"
        self.gtype = "XTYPE_VARIANT"
        self.free_func = "xvariant_unref"
        self.format_in = "@" + self.signature
        self.format_out = "@" + self.signature
        self.gvariant_get = "XXX"
        self.gvalue_get = "xvalue_get_variant"
        self.array_annotation = ""

        if not utils.lookup_annotation(
            self.annotations, "org.gtk.GDBus.C.ForceGVariant"
        ):
            if self.signature == "b":
                self.ctype_in_g = "xboolean_t "
                self.ctype_in = "xboolean_t "
                self.ctype_out = "xboolean_t *"
                self.gtype = "XTYPE_BOOLEAN"
                self.free_func = None
                self.format_in = "b"
                self.format_out = "b"
                self.gvariant_get = "xvariant_get_boolean"
                self.gvalue_get = "xvalue_get_boolean"
            elif self.signature == "y":
                self.ctype_in_g = "xuchar_t "
                self.ctype_in = "xuchar_t "
                self.ctype_out = "xuchar_t *"
                self.gtype = "XTYPE_UCHAR"
                self.free_func = None
                self.format_in = "y"
                self.format_out = "y"
                self.gvariant_get = "xvariant_get_byte"
                self.gvalue_get = "xvalue_get_uchar"
            elif self.signature == "n":
                self.ctype_in_g = "xint_t "
                self.ctype_in = "gint16 "
                self.ctype_out = "gint16 *"
                self.gtype = "XTYPE_INT"
                self.free_func = None
                self.format_in = "n"
                self.format_out = "n"
                self.gvariant_get = "xvariant_get_int16"
                self.gvalue_get = "xvalue_get_int"
            elif self.signature == "q":
                self.ctype_in_g = "xuint_t "
                self.ctype_in = "xuint16_t "
                self.ctype_out = "xuint16_t *"
                self.gtype = "XTYPE_UINT"
                self.free_func = None
                self.format_in = "q"
                self.format_out = "q"
                self.gvariant_get = "xvariant_get_uint16"
                self.gvalue_get = "xvalue_get_uint"
            elif self.signature == "i":
                self.ctype_in_g = "xint_t "
                self.ctype_in = "xint_t "
                self.ctype_out = "xint_t *"
                self.gtype = "XTYPE_INT"
                self.free_func = None
                self.format_in = "i"
                self.format_out = "i"
                self.gvariant_get = "xvariant_get_int32"
                self.gvalue_get = "xvalue_get_int"
            elif self.signature == "u":
                self.ctype_in_g = "xuint_t "
                self.ctype_in = "xuint_t "
                self.ctype_out = "xuint_t *"
                self.gtype = "XTYPE_UINT"
                self.free_func = None
                self.format_in = "u"
                self.format_out = "u"
                self.gvariant_get = "xvariant_get_uint32"
                self.gvalue_get = "xvalue_get_uint"
            elif self.signature == "x":
                self.ctype_in_g = "sint64_t "
                self.ctype_in = "sint64_t "
                self.ctype_out = "sint64_t *"
                self.gtype = "XTYPE_INT64"
                self.free_func = None
                self.format_in = "x"
                self.format_out = "x"
                self.gvariant_get = "xvariant_get_int64"
                self.gvalue_get = "xvalue_get_int64"
            elif self.signature == "t":
                self.ctype_in_g = "xuint64_t "
                self.ctype_in = "xuint64_t "
                self.ctype_out = "xuint64_t *"
                self.gtype = "XTYPE_UINT64"
                self.free_func = None
                self.format_in = "t"
                self.format_out = "t"
                self.gvariant_get = "xvariant_get_uint64"
                self.gvalue_get = "xvalue_get_uint64"
            elif self.signature == "d":
                self.ctype_in_g = "xdouble_t "
                self.ctype_in = "xdouble_t "
                self.ctype_out = "xdouble_t *"
                self.gtype = "XTYPE_DOUBLE"
                self.free_func = None
                self.format_in = "d"
                self.format_out = "d"
                self.gvariant_get = "xvariant_get_double"
                self.gvalue_get = "xvalue_get_double"
            elif self.signature == "s":
                self.ctype_in_g = "const xchar_t *"
                self.ctype_in = "const xchar_t *"
                self.ctype_in_dup = "xchar_t *"
                self.ctype_out = "xchar_t **"
                self.gtype = "XTYPE_STRING"
                self.free_func = "g_free"
                self.format_in = "s"
                self.format_out = "s"
                self.gvariant_get = "xvariant_get_string"
                self.gvalue_get = "xvalue_get_string"
            elif self.signature == "o":
                self.ctype_in_g = "const xchar_t *"
                self.ctype_in = "const xchar_t *"
                self.ctype_in_dup = "xchar_t *"
                self.ctype_out = "xchar_t **"
                self.gtype = "XTYPE_STRING"
                self.free_func = "g_free"
                self.format_in = "o"
                self.format_out = "o"
                self.gvariant_get = "xvariant_get_string"
                self.gvalue_get = "xvalue_get_string"
            elif self.signature == "g":
                self.ctype_in_g = "const xchar_t *"
                self.ctype_in = "const xchar_t *"
                self.ctype_in_dup = "xchar_t *"
                self.ctype_out = "xchar_t **"
                self.gtype = "XTYPE_STRING"
                self.free_func = "g_free"
                self.format_in = "g"
                self.format_out = "g"
                self.gvariant_get = "xvariant_get_string"
                self.gvalue_get = "xvalue_get_string"
            elif self.signature == "ay":
                self.ctype_in_g = "const xchar_t *"
                self.ctype_in = "const xchar_t *"
                self.ctype_in_dup = "xchar_t *"
                self.ctype_out = "xchar_t **"
                self.gtype = "XTYPE_STRING"
                self.free_func = "g_free"
                self.format_in = "^ay"
                self.format_out = "^ay"
                self.gvariant_get = "xvariant_get_bytestring"
                self.gvalue_get = "xvalue_get_string"
            elif self.signature == "as":
                self.ctype_in_g = "const xchar_t *const *"
                self.ctype_in = "const xchar_t *const *"
                self.ctype_in_dup = "xchar_t **"
                self.ctype_out = "xchar_t ***"
                self.gtype = "XTYPE_STRV"
                self.free_func = "xstrfreev"
                self.format_in = "^as"
                self.format_out = "^as"
                self.gvariant_get = "xvariant_get_strv"
                self.gvalue_get = "xvalue_get_boxed"
                self.array_annotation = "(array zero-terminated=1)"
            elif self.signature == "ao":
                self.ctype_in_g = "const xchar_t *const *"
                self.ctype_in = "const xchar_t *const *"
                self.ctype_in_dup = "xchar_t **"
                self.ctype_out = "xchar_t ***"
                self.gtype = "XTYPE_STRV"
                self.free_func = "xstrfreev"
                self.format_in = "^ao"
                self.format_out = "^ao"
                self.gvariant_get = "xvariant_get_objv"
                self.gvalue_get = "xvalue_get_boxed"
                self.array_annotation = "(array zero-terminated=1)"
            elif self.signature == "aay":
                self.ctype_in_g = "const xchar_t *const *"
                self.ctype_in = "const xchar_t *const *"
                self.ctype_in_dup = "xchar_t **"
                self.ctype_out = "xchar_t ***"
                self.gtype = "XTYPE_STRV"
                self.free_func = "xstrfreev"
                self.format_in = "^aay"
                self.format_out = "^aay"
                self.gvariant_get = "xvariant_get_bytestring_array"
                self.gvalue_get = "xvalue_get_boxed"
                self.array_annotation = "(array zero-terminated=1)"

        for a in self.annotations:
            a.post_process(interface_prefix, cns, cns_upper, cns_lower, self)


class Method:
    def __init__(self, name, h_type_implies_unix_fd=True):
        self.name = name
        self.h_type_implies_unix_fd = h_type_implies_unix_fd
        self.in_args = []
        self.out_args = []
        self.annotations = []
        self.doc_string = ""
        self.since = ""
        self.deprecated = False
        self.unix_fd = False

    def post_process(
        self, interface_prefix, cns, cns_upper, cns_lower, containing_iface
    ):
        if len(self.doc_string) == 0:
            self.doc_string = utils.lookup_docs(self.annotations)
        if len(self.since) == 0:
            self.since = utils.lookup_since(self.annotations)
            if len(self.since) == 0:
                self.since = containing_iface.since

        name = self.name
        overridden_name = utils.lookup_annotation(
            self.annotations, "org.gtk.GDBus.C.Name"
        )
        if utils.is_ugly_case(overridden_name):
            self.name_lower = overridden_name.lower()
        else:
            if overridden_name:
                name = overridden_name
            self.name_lower = utils.camel_case_to_uscore(name).lower().replace("-", "_")
        self.name_hyphen = self.name_lower.replace("_", "-")

        arg_count = 0
        for a in self.in_args:
            a.post_process(interface_prefix, cns, cns_upper, cns_lower, arg_count)
            arg_count += 1
            if self.h_type_implies_unix_fd and "h" in a.signature:
                self.unix_fd = True

        for a in self.out_args:
            a.post_process(interface_prefix, cns, cns_upper, cns_lower, arg_count)
            arg_count += 1
            if self.h_type_implies_unix_fd and "h" in a.signature:
                self.unix_fd = True

        if (
            utils.lookup_annotation(self.annotations, "org.freedesktop.DBus.Deprecated")
            == "true"
        ):
            self.deprecated = True

        if utils.lookup_annotation(self.annotations, "org.gtk.GDBus.C.UnixFD"):
            self.unix_fd = True

        for a in self.annotations:
            a.post_process(interface_prefix, cns, cns_upper, cns_lower, self)


class Signal:
    def __init__(self, name):
        self.name = name
        self.args = []
        self.annotations = []
        self.doc_string = ""
        self.since = ""
        self.deprecated = False

    def post_process(
        self, interface_prefix, cns, cns_upper, cns_lower, containing_iface
    ):
        if len(self.doc_string) == 0:
            self.doc_string = utils.lookup_docs(self.annotations)
        if len(self.since) == 0:
            self.since = utils.lookup_since(self.annotations)
            if len(self.since) == 0:
                self.since = containing_iface.since

        name = self.name
        overridden_name = utils.lookup_annotation(
            self.annotations, "org.gtk.GDBus.C.Name"
        )
        if utils.is_ugly_case(overridden_name):
            self.name_lower = overridden_name.lower()
        else:
            if overridden_name:
                name = overridden_name
            self.name_lower = utils.camel_case_to_uscore(name).lower().replace("-", "_")
        self.name_hyphen = self.name_lower.replace("_", "-")

        arg_count = 0
        for a in self.args:
            a.post_process(interface_prefix, cns, cns_upper, cns_lower, arg_count)
            arg_count += 1

        if (
            utils.lookup_annotation(self.annotations, "org.freedesktop.DBus.Deprecated")
            == "true"
        ):
            self.deprecated = True

        for a in self.annotations:
            a.post_process(interface_prefix, cns, cns_upper, cns_lower, self)


class Property:
    def __init__(self, name, signature, access):
        self.name = name
        self.signature = signature
        self.access = access
        self.annotations = []
        self.arg = Arg("value", self.signature)
        self.arg.annotations = self.annotations
        self.readable = False
        self.writable = False
        if self.access == "readwrite":
            self.readable = True
            self.writable = True
        elif self.access == "read":
            self.readable = True
        elif self.access == "write":
            self.writable = True
        else:
            print_error('Invalid access type "{}"'.format(self.access))
        self.doc_string = ""
        self.since = ""
        self.deprecated = False
        self.emits_changed_signal = True

    def post_process(
        self, interface_prefix, cns, cns_upper, cns_lower, containing_iface
    ):
        if len(self.doc_string) == 0:
            self.doc_string = utils.lookup_docs(self.annotations)
        if len(self.since) == 0:
            self.since = utils.lookup_since(self.annotations)
            if len(self.since) == 0:
                self.since = containing_iface.since

        name = self.name
        overridden_name = utils.lookup_annotation(
            self.annotations, "org.gtk.GDBus.C.Name"
        )
        if utils.is_ugly_case(overridden_name):
            self.name_lower = overridden_name.lower()
        else:
            if overridden_name:
                name = overridden_name
            self.name_lower = utils.camel_case_to_uscore(name).lower().replace("-", "_")
        self.name_hyphen = self.name_lower.replace("_", "-")
        # don't clash with the xtype_t getter, e.g.:
        # xtype_t foo_bar_get_type (void); G_GNUC_CONST
        if self.name_lower == "type":
            self.name_lower = "type_"

        # recalculate arg
        self.arg.annotations = self.annotations
        self.arg.post_process(interface_prefix, cns, cns_upper, cns_lower, 0)

        if (
            utils.lookup_annotation(self.annotations, "org.freedesktop.DBus.Deprecated")
            == "true"
        ):
            self.deprecated = True

        for a in self.annotations:
            a.post_process(interface_prefix, cns, cns_upper, cns_lower, self)

        # FIXME: for now we only support 'false' and 'const' on the signal itself,
        # see #674913 and
        # http://dbus.freedesktop.org/doc/dbus-specification.html#introspection-format
        # for details
        if utils.lookup_annotation(
            self.annotations, "org.freedesktop.DBus.Property.EmitsChangedSignal"
        ) in ("false", "const"):
            self.emits_changed_signal = False


class Interface:
    def __init__(self, name):
        self.name = name
        self.methods = []
        self.signals = []
        self.properties = []
        self.annotations = []
        self.doc_string = ""
        self.doc_string_brief = ""
        self.since = ""
        self.deprecated = False

    def post_process(self, interface_prefix, c_namespace):
        if len(self.doc_string) == 0:
            self.doc_string = utils.lookup_docs(self.annotations)
        if len(self.doc_string_brief) == 0:
            self.doc_string_brief = utils.lookup_brief_docs(self.annotations)
        if len(self.since) == 0:
            self.since = utils.lookup_since(self.annotations)

        if len(c_namespace) > 0:
            if utils.is_ugly_case(c_namespace):
                cns = c_namespace.replace("_", "")
                cns_upper = c_namespace.upper() + "_"
                cns_lower = c_namespace.lower() + "_"
            else:
                cns = c_namespace
                cns_upper = utils.camel_case_to_uscore(c_namespace).upper() + "_"
                cns_lower = utils.camel_case_to_uscore(c_namespace).lower() + "_"
        else:
            cns = ""
            cns_upper = ""
            cns_lower = ""

        overridden_name = utils.lookup_annotation(
            self.annotations, "org.gtk.GDBus.C.Name"
        )
        if utils.is_ugly_case(overridden_name):
            name = overridden_name.replace("_", "")
            name_with_ns = cns + name
            self.name_without_prefix = name
            self.camel_name = name_with_ns
            self.ns_upper = cns_upper
            self.name_lower = cns_lower + overridden_name.lower()
            self.name_upper = overridden_name.upper()

            # print_error('handle Ugly_Case "{}"'.format(overridden_name))
        else:
            if overridden_name:
                name = overridden_name
            else:
                name = self.name
                if name.startswith(interface_prefix):
                    name = name[len(interface_prefix) :]
            self.name_without_prefix = name
            name = utils.strip_dots(name)
            name_with_ns = utils.strip_dots(cns + "." + name)
            self.camel_name = name_with_ns
            self.ns_upper = cns_upper
            self.name_lower = cns_lower + utils.camel_case_to_uscore(name)
            self.name_upper = utils.camel_case_to_uscore(name).upper()

        self.name_hyphen = self.name_upper.lower().replace("_", "-")

        if (
            utils.lookup_annotation(self.annotations, "org.freedesktop.DBus.Deprecated")
            == "true"
        ):
            self.deprecated = True

        for m in self.methods:
            m.post_process(interface_prefix, cns, cns_upper, cns_lower, self)

        for s in self.signals:
            s.post_process(interface_prefix, cns, cns_upper, cns_lower, self)

        for p in self.properties:
            p.post_process(interface_prefix, cns, cns_upper, cns_lower, self)

        for a in self.annotations:
            a.post_process(interface_prefix, cns, cns_upper, cns_lower, self)
