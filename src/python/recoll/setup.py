from distutils.core import setup, Extension
top = '../../'

module1 = Extension('recoll',
                    define_macros = [('MAJOR_VERSION', '1'),
                                     ('MINOR_VERSION', '0'),
                                     ('UNAC_VERSION', '"1.0.7"'),
                                     ('STATFS_INCLUDE', '"sys/mount.h"'),
                                     ('RECOLL_DATADIR', 
                                      '"/usr/local/share/recoll"')
                                     ],
                    include_dirs = ['/usr/local/include',
                                    top + 'utils', 
                                    top + 'common', 
                                    top + 'rcldb',
                                    top + 'query',
                                    top + 'unac'
                                    ],
                    libraries = ['xapian', 'iconv'],
                    library_dirs = ['/usr/local/lib'],
                    sources = ['pyrecoll.cpp',
                               top + 'common/rclconfig.cpp',
                               top + 'common/rclinit.cpp',
                               top + 'common/textsplit.cpp',
                               top + 'common/unacpp.cpp',
                               top + 'query/wasastringtoquery.cpp',
                               top + 'query/wasatorcl.cpp',
                               top + 'rcldb/pathhash.cpp',
                               top + 'rcldb/rcldb.cpp',
                               top + 'rcldb/rcldoc.cpp',
                               top + 'rcldb/rclquery.cpp',
                               top + 'rcldb/searchdata.cpp',
                               top + 'rcldb/stemdb.cpp',
                               top + 'rcldb/stoplist.cpp',
                               top + 'unac/unac.c',
                               top + 'utils/base64.cpp',
                               top + 'utils/conftree.cpp',
                               top + 'utils/debuglog.cpp',
                               top + 'utils/md5.cpp',
                               top + 'utils/pathut.cpp',
                               top + 'utils/readfile.cpp',
                               top + 'utils/smallut.cpp',
                               top + 'utils/transcode.cpp',
                               top + 'utils/wipedir.cpp'
                               ])

setup (name = 'Recoll',
       version = '1.0',
       description = 'Query/Augment a Recoll full text index',
       author = 'J.F. Dockes',
       author_email = 'jean-francois.dockes@wanadoo.fr',
       long_description = '''
''',
       ext_modules = [module1])
