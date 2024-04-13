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


aspellexts = []
foreach v : py3versions
    py3_name = 'python' + v
    python_installation = py_mod.find_installation(py3_name)
    aspellexts += python_installation.extension_module(
        'recollaspell',
        [
           'aspell.c',
        ],
        dependencies: [
            python_installation.dependency(),
            cc.find_library('aspell'),
        ],
        install: true,
    )
endforeach
