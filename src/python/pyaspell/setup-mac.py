# Mac build for the pyaspell extension. Not used at the moment because did not find a good way
# to distribute the corresponding Python version.

from setuptools import setup, Extension

# For shadow builds: references to the source tree
import os
root = "/Users/dockes/Recoll/"
top = os.path.join(root, "recoll", "src")
pytop = os.path.join(top, "python", "pyaspell")
aspell = os.path.join(root, "aspell-0.60.7")

setup (name = 'aspell-python-py3',
    version = '1.15',
    ext_modules = [Extension('aspell',
                             [os.path.join(pytop, "aspell.c")],
                             include_dirs = [
                                 os.path.join(aspell, "interfaces", "cc"), 
                             ],
                             libraries = ["aspell"],
                             library_dirs = [
                                 os.path.join(root, 
                                              "build-libaspell-Qt_6_4_2_for_macOS-Release/")],
                             )],
    description      = "Wrapper around GNU Aspell for Python 3",
    author           = "Wojciech Muła",
    author_email     = "wojciech_mula@poczta.onet.pl",
    maintainer       = "Wojciech Muła",
    maintainer_email = "wojciech_mula@poczta.onet.pl",
    url              = "http://github.com/WojciechMula/aspell-python",
    platforms        = ["Linux", "Windows", "MacOS"],
    license          = "BSD (3 clauses)",
    long_description = "aspell-python - C extension for Python 3",
    classifiers      = [
        "Development Status :: 5 - Production/Stable",
        "License :: OSI Approved :: BSD License",
        "Programming Language :: C",
        "Topic :: Software Development :: Libraries",
        "Topic :: Text Editors :: Text Processing",
    ],
)
