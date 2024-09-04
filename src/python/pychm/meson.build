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

# Define potential include and library directories to be considered in addition
search_include_dirs = [
]
search_lib_dirs = [
]

# Add additional paths if system is Darwin (macOS)
if host_machine.system() == 'darwin'
    # For Homebrew
    search_include_dirs += [
        '/opt/homebrew/include',
        '/usr/local/include/aspell',
    ]
    search_lib_dirs += [
        '/opt/homebrew/lib',
        '/usr/local/lib',
    ]

    # For MacPorts
    search_include_dirs += [
        '/opt/local/include',
    ]
    search_lib_dirs += [
        '/opt/local/lib',
    ]
endif

# Filter out non-existent directories
valid_include_dirs = []
foreach dir : search_include_dirs
    if fs.is_dir(dir)
        valid_include_dirs += dir
    endif
endforeach

valid_lib_dirs = []
foreach dir : search_lib_dirs
    if fs.is_dir(dir)        
        valid_lib_dirs += dir
    endif
endforeach

# Following https://mesonbuild.com/Reference-manual_returned_compiler.html#compilerfind_library,
# we specify additional lib directories to search in
extra_lib_dirs = valid_lib_dirs
# Following https://mesonbuild.com/Reference-manual_returned_compiler.html#check_header_include_directories
# we specify additional include directories
extra_inc_dirs = valid_include_dirs

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
        dependencies: [py3_dep, cc.find_library('chm', dirs: extra_lib_dirs, required: true),],
        include_directories: extra_inc_dirs,
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
