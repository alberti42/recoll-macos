from distutils.core import setup, Extension

module1 = Extension('recollq',
                    define_macros = [('MAJOR_VERSION', '1'),
                                     ('MINOR_VERSION', '0'),
                                     ('UNAC_VERSION', '"1.0.7"'),
                                     ('STATFS_INCLUDE', '"sys/mount.h"'),
                                     ('RECOLL_DATADIR', 
                                      '"/usr/local/share/recoll"')
                                     ],
                    include_dirs = ['/usr/local/include',
                                    '../utils', 
                                    '../common', 
                                    '../rcldb',
                                    '../query',
                                    '../unac'
                                    ],
                    libraries = ['xapian', 'iconv'],
                    library_dirs = ['/usr/local/lib'],
                    sources = ['recoll_query.cpp',
                               '../common/rclconfig.cpp',
                               '../common/rclinit.cpp',
                               '../common/textsplit.cpp',
                               '../common/unacpp.cpp',
                               '../query/wasastringtoquery.cpp',
                               '../query/wasatorcl.cpp',
                               '../rcldb/pathhash.cpp',
                               '../rcldb/rcldb.cpp',
                               '../rcldb/rclquery.cpp',
                               '../rcldb/searchdata.cpp',
                               '../rcldb/stemdb.cpp',
                               '../rcldb/stoplist.cpp',
                               '../unac/unac.c',
                               '../utils/base64.cpp',
                               '../utils/conftree.cpp',
                               '../utils/debuglog.cpp',
                               '../utils/md5.cpp',
                               '../utils/pathut.cpp',
                               '../utils/readfile.cpp',
                               '../utils/smallut.cpp',
                               '../utils/transcode.cpp',
                               '../utils/wipedir.cpp'
                               ])


setup (name = 'RecollQuery',
       version = '1.0',
       description = 'Enable querying a Recoll full text index',
       author = 'J.F. Dockes',
       author_email = 'jean-francois.dockes@wanadoo.fr',
       long_description = '''
This is really just a demo package for now.
''',
       ext_modules = [module1])
