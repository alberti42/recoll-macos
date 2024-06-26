# Semi-generic version of setup.py. This is not used anymore on ux systems (which use meson instead)
# Use as a base if you need another kind of build. Supposes that librecoll and the include files are
# available somewhere
# After customizing, install the wheel module with pip and run python3 setup.py bdist_wheel to
# obtain an installable .whl

from setuptools import setup, Extension
import os
import sys

# For shadow builds: references to the source tree
pytop = '/home/dockes/projets/fulltext/recoll/src/python/recoll'
# Need to set this. Where will the default config be stored. e.g. /usr/share/recoll on linux
RECOLL_DATADIR = '/usr/share/recoll'

top = os.path.join(pytop, '..', '..')

VERSION = open(os.path.join(top, "RECOLL-VERSION.txt")).read().strip()

# For shadow builds: reference to the top of the local tree (for finding
# generated .h files, e.g. autoconfig.h)
localtop = os.path.join(os.path.dirname(__file__), '..', '..')

    
extra_compile_args = ['-std=c++17']
define_macros = [('RECOLL_DATADIR', '"' + RECOLL_DATADIR + '"'),]
# Set to 1 depending on what is set in the main build (this is a meson option set to off by default)
DEF_EXT4_BIRTH_TIME = 0
if DEF_EXT4_BIRTH_TIME == 1:
   define_macros.append(('EXT4_BIRTH_TIME', 1))


# See setup.cfg. Not needed with older python3 versions. no idea, but needed else bdist_wheel fails.
if not os.path.exists('Recoll.egg-info'):
   os.mkdir('Recoll.egg-info')
   
# These are only needed if building with a full source tree and the includes are not installed
include_dirs = [
        os.path.join(localtop, 'common'),
        os.path.join(top, 'common'), 
        os.path.join(top, 'index'), 
        os.path.join(top, 'internfile'), 
        os.path.join(top, 'query'), 
        os.path.join(top, 'rcldb'), 
        os.path.join(top, 'utils'), 
]
# Use the standard location for installed includes (Linux, mostly).
include_dirs.append('/usr/include/recoll')

# This used to add .libs to allow building with an uninstalled lib in .libs. May need to tell where
# librecoll is if not in a standard place (as it would be on Linux)
library_dirs = []

# May need to use instead a variation of the following depending on your linker:
# libraries =  ['recoll', 'xapian', 'iconv', 'z']
libraries = ['recoll']

runtime_library_dirs = list()
    
module1 = Extension('_recoll',
                    define_macros = define_macros,
                    include_dirs = include_dirs,
                    extra_compile_args = extra_compile_args,
                    libraries = libraries,
                    library_dirs = library_dirs,
                    runtime_library_dirs = runtime_library_dirs,
                    sources = [os.path.join(pytop, 'pyrecoll.cpp'),
                               os.path.join(pytop, 'pyresultstore.cpp'),
                               os.path.join(pytop, 'pyrclextract.cpp')
                               ])

setup (name = 'Recoll',
       version = VERSION,
       description = 'Query/Augment a Recoll full text index',
       author = 'J.F. Dockes',
       author_email = 'jfd@recoll.org',
       url = 'https://www.recoll.org',
       license = 'GPL',
       package_dir = {'' : os.path.join(top, 'python', 'recoll')},
       long_description = '''
''',
       packages = ['recoll'],
       ext_package = 'recoll',
       ext_modules = [module1])
