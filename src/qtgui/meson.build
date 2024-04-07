qtgui_conf = configuration_data()
qtgui_conf.set('srcdir', meson.source_root() / 'qtgui')
if get_option('webkit')
    qtgui_conf.set('QMAKE_ENABLE_WEBKIT', '')
else
    qtgui_conf.set('QMAKE_ENABLE_WEBKIT', '#')
endif
if get_option('webengine')
    qtgui_conf.set('QMAKE_ENABLE_WEBENGINE', '')
else
    qtgui_conf.set('QMAKE_ENABLE_WEBENGINE', '#')
endif
if get_option('qtzeitgeist')
    qtgui_conf.set('QMAKE_ENABLE_ZEITGEIST', '')
else
    qtgui_conf.set('QMAKE_ENABLE_ZEITGEIST', '#')
endif
if get_option('guidebug')
    qtgui_conf.set('QMAKE_ENABLE_GUIDEBUG', '')
else
    qtgui_conf.set('QMAKE_ENABLE_GUIDEBUG', '#')
endif
if get_option('publiclib')
    qtgui_conf.set('PUBLIC_LIB_FALSE', '')
else
    qtgui_conf.set('PUBLIC_LIB_FALSE', '#')
endif

# Tried xapian.get_pkgconfig_variable('libs')/'--libs'/'Libs' with no success
pkgc = find_program('pkg-config')
libstr = run_command(pkgc, ['--libs', 'xapian-core'], check: true).stdout().strip()
qtgui_conf.set('XAPIAN_LIBS', libstr)
qtgui_conf.set('LIBICONV', '')
libstr = run_command(pkgc, ['--libs', 'libxslt'], check: true).stdout().strip()
qtgui_conf.set('XSLT_LIBS', libstr)
qtgui_conf.set('LIBQZEITGEIST', '')
# libdir is only used with publiclib=false which we don't support at the moment
qtgui_conf.set('libdir', '')
pro_file = configure_file(
    output: 'recoll.pro',
    input: 'recoll.pro.in',
    configuration: qtgui_conf,
)

qmkmk = find_program(meson.source_root() / 'qtgui' / 'qmkmk.sh')

infile = pro_file
outfile = 'recoll'

recoll = custom_target('recoll',
  output : outfile,
  input : infile,
  command : [qmkmk, '@INPUT@'],
  install : true,
  install_dir: 'bin',
)

