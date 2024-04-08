# Building the recollaspell Python extension. Same as pyaspell, but this is not packaged.

py_mod = import('python')
python = py_mod.find_installation(
    'python3',
)
py3_dep = dependency('python3', required: true)

chmlib = python.extension_module(
    '_chmlib',
    [
        'recollchm/swig_chm.c',
    ],
    c_args: ['-DSWIG_COBJECT_TYPES'],
    dependencies: [py3_dep, cc.find_library('libchm'),],
    install: true,
    subdir: 'recollchm',
)

chmextra = python.extension_module(
    'extra',
    [
        'recollchm/extra.c',
    ],
    c_args: ['-D__PYTHON__'],
    dependencies: [py3_dep, cc.find_library('libchm'),],
    install: true,
    subdir: 'recollchm',
)

python.install_sources(
    [
        'recollchm/__init__.py',
        'recollchm/chm.py',
        'recollchm/chmlib.py',
    ],
    subdir: 'recollchm',
)
