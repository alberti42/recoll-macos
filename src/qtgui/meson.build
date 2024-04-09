# Meson file to build the QT Recoll GUI
# We are using a custom target to run qmake and make as I don't want to have to duplicate the .pro
# into a meson build as it is used on Windows and MacOS, with special constraints.
# Meson is used to build the .pro from a .pro.in as per recoll configuration, then the custom target
# is defined. Makes use of a small script located in the qtgui directory.

qtgui_conf = configuration_data()
qtgui_conf.set('srcdir', meson.source_root() / 'qtgui')
if not get_option('webkit') or get_option('webengine')
    qtgui_conf.set('QMAKE_ENABLE_WEBKIT', '#')
endif
if not get_option('webengine')
    qtgui_conf.set('QMAKE_ENABLE_WEBENGINE', '#')
endif
if not get_option('qtzeitgeist')
    qtgui_conf.set('QMAKE_ENABLE_ZEITGEIST', '#')
endif
if not get_option('guidebug')
    qtgui_conf.set('QMAKE_ENABLE_GUIDEBUG', '#')
endif
qtgui_conf.set('LIBRECOLL_FOR_QMAKE', librecoll.full_path())

# Tried xapian.get_pkgconfig_variable('libs')/'--libs'/'Libs' with no success
pkgc = find_program('pkg-config')
libstr = run_command(pkgc, ['--libs', 'xapian-core'], check: true).stdout().strip()
qtgui_conf.set('XAPIAN_LIBS', libstr)
qtgui_conf.set('LIBICONV', '')
libstr = run_command(pkgc, ['--libs', 'libxslt'], check: true).stdout().strip()
qtgui_conf.set('XSLT_LIBS', libstr)
qtgui_conf.set('LIBQZEITGEIST', '')
pro_file = configure_file(
    output: 'recoll.pro',
    input: 'recoll.pro.in',
    configuration: qtgui_conf,
)

qmkmk = find_program(meson.source_root() / 'qtgui' / 'qmkmk.sh')

infile = pro_file
outfile = 'recoll'

recoll = custom_target(
    'recoll',
    output: outfile,
    input: infile,
    command: [qmkmk, '@INPUT@'],
    install: true,
    install_dir: 'bin',
)

