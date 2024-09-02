# Building the recollaspell Python extension. Same as pyaspell, but this is not packaged.

py_mod = import('python')

py3versions_cmd = find_program('py3versions',
    native: true,
    required: false,
)
if py3versions_cmd.found()
    py3vs = run_command(py3versions_cmd, ['-iv',],
        check: true,
        capture: true,
        )
    py3versions = py3vs.stdout().split()
else
    py3versions = ['3',]
endif


chmexts = []
foreach v : py3versions
    py3_name = 'python' + v
    python_installation = py_mod.find_installation(py3_name)
    py3_dep = python_installation.dependency()

    chmexts += python_installation.extension_module(
        '_chmlib',
        [
            'recollchm/swig_chm.c',
        ],
        c_args: ['-DSWIG_COBJECT_TYPES'],
        dependencies: [py3_dep, cc.find_library('chm', dirs: '/opt/homebrew/lib', required: true),],
        include_directories: include_directories('/opt/homebrew/include'),
        install: true,
        subdir: 'recollchm',
    )

endforeach

python.install_sources(
    [
        'recollchm/__init__.py',
        'recollchm/chm.py',
        'recollchm/chmlib.py',
    ],
    subdir: 'recollchm',
    pure: false,
)
