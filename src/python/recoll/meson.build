
# Building the Recoll Python extension.

# As a custom target. Problem: don't know how to invoke install. Pro: this could have shared the
# setup.py. Con: hacky anyway.
#pyrecoll_conf = configuration_data()
#pyrecoll_conf.set('srcdir', meson.source_root() / 'python' / 'recoll')
#pyrecoll_conf.set10('RECOLL_PUBLIC_LIB', get_option('publiclib'))
#pyrecoll_conf.set_quoted('RECOLL_DATADIR', recoll_datadir)
#pyrecoll_conf.set10('DEF_EXT4_BIRTH_TIME', get_option('ext4-birthtime'))
#setup_file = configure_file(
#    output: 'setup.py',
#    input: 'setup.py.in',
#    configuration: pyrecoll_conf,
#)
#python = find_program('python3')
#infile = setup_file
#outfile = 'pyrecoll'
#pyrecoll = custom_target('pyrecoll',
#  output : outfile,
#  input : infile,
#  command : [python, '@INPUT@', 'build'],
#  install : true,
#  install_dir: 'bin',
#)

pyrecoll_incdir = include_directories(
    '../../common',
    '../../index',
    '../../internfile',
    '../../query',
    '../../rcldb',
    '../../utils',
)

py_mod = import('python')
python = py_mod.find_installation(
    'python3',
)
py3_dep = dependency('python3', required: true)
extension = python.extension_module(
    '_recoll',
    [
        'pyrclextract.cpp',
        'pyrecoll.cpp',
        'pyresultstore.cpp',
    ],
    include_directories: pyrecoll_incdir,
    dependencies: py3_dep,
    link_with: librecoll,
    install: true,
    subdir: 'recoll',
)
python.install_sources(
    [
        'recoll/__init__.py',
        'recoll/conftree.py',
        'recoll/qresultstore.py',
        'recoll/rclconfig.py',
        'recoll/rclextract.py',
        'recoll/recoll.py',
    ],
    subdir: 'recoll',
)

