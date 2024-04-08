# Building the recollaspell Python extension. Same as pyaspell, but this is not packaged.

py_mod = import('python')
python = py_mod.find_installation(
    'python3',
)
py3_dep = dependency('python3', required: true)
aspellext = python.extension_module(
    'recollaspell',
    [
        'aspell.c',
    ],
    dependencies: [py3_dep, cc.find_library('libaspell'),],
    install: true,
)
