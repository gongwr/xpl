charset_lib = static_library('charset', 'localcharset.c',
  include_directories : configinc,
  pic : true,
  c_args : [ '-DXPL_CHARSETALIAS_DIR="@0@"'.format(glib_charsetaliasdir) ] + glib_hidden_visibility_args)
