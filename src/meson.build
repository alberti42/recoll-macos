project(
  'recoll',
  'cpp',
  license: 'GPL 2+',
  version: '1.38.0',
  default_options: ['cpp_std=c++17','buildtype=debugoptimized'],
  meson_version: '>=0.49',
)
cpp = meson.get_compiler('cpp')

librecolldeps = []
librecolldeps += dependency('threads')
librecolldeps += dependency('xapian-core')
librecolldeps += dependency('libxml-2.0')
librecolldeps += dependency('libxslt')
librecolldeps += dependency('zlib')
if get_option('libmagic')
  librecolldeps += dependency('libmagic')
endif
librecolldeps += dependency('x11')
# Searching for iconv is a meson 0.60 feature. Hopefully, for now all the systems where the meson
# build is used have iconv in libc. Otherwise look at glib search for iconv for how to do it.
#librecolldeps += dependency('iconv', method = 'auto')

recoll_datadir = get_option('prefix') / get_option('datadir') / 'recoll'

add_project_arguments(['-DBUILDING_RECOLL',
  '-DREADFILE_ENABLE_ZLIB', '-DREADFILE_ENABLE_MINIZ', '-DREADFILE_ENABLE_MD5',
  '-DRECOLL_DATADIR="' + recoll_datadir + '"',
], language: 'cpp')


recoll_conf = configuration_data()
recoll_conf.set_quoted('PACKAGE_VERSION', meson.project_version())
recoll_conf.set_quoted('FILE_PROG', get_option('file-command'))
recoll_conf.set('IDX_THREADS', get_option('idxthreads'))
recoll_conf.set('ENABLE_LIBMAGIC', get_option('libmagic'))
if cpp.compiles('''
        #include <stdlib.h>
        #include <iconv.h>
        void tryit()
        {
            iconv_t ic;
            size_t isiz, osiz;
            const char *icp;
            char *ocp;
            ic = iconv_open("utf-8", "utf-8");
            iconv(ic, &icp, &isiz, &ocp, &osiz);
        }''',
        args: '-Werror',
        name : 'ICONV CONST')
    recoll_conf.set('ICONV_CONST', 'const')
elif cpp.compiles('''
        #include <stdlib.h>
        #include <iconv.h>
        void tryit()
        {
            iconv_t ic;
            size_t isiz, osiz;
            char *icp;
            char *ocp;
            ic = iconv_open("utf-8", "utf-8");
            iconv(ic, &icp, &isiz, &ocp, &osiz);
        }''',
        args: '-Werror',
        name : 'ICONV NOT CONST')
    recoll_conf.set('ICONV_CONST', '')
endif

if cpp.compiles('''#include<stdlib.h>
    char* func() {char t[10];return mkdtemp(t);}
    ''', name : 'mkdtemp')
  recoll_conf.set('HAVE_MKDTEMP', true)
endif
if cpp.has_header('features.h')
    recoll_conf.set('HAVE_FEATURES_H', true)
endif
if cpp.has_header('spawn.h')
    recoll_conf.set('HAVE_SPAWN_H', true)
    if cpp.has_function('posix_spawn', prefix : '#include <spawn.h>')
        recoll_conf.set10('HAVE_POSIX_SPAWN', true)
        recoll_conf.set('USE_POSIX_SPAWN', get_option('spawn'))
    endif
endif
if cpp.has_header('malloc.h')
    malloc_include = '<malloc.h>'
    recoll_conf.set('HAVE_MALLOC_H', true)
endif
if cpp.has_header('malloc/malloc.h')
    malloc_include = '<malloc/malloc.h>'
    recoll_conf.set('HAVE_MALLOC_MALLOC_H', true)
endif
if cpp.has_function('malloc_trim', prefix : '#include ' + malloc_include)
    recoll_conf.set('HAVE_MALLOC_TRIM', true)
endif
if cpp.has_function('setrlimit', prefix : '''#include <sys/time.h>
       #include <sys/resource.h>''')
    recoll_conf.set('HAVE_SETRLIMIT', true)
endif
if cpp.has_function('kqueue', prefix : '''#include <sys/event.h>
       #include <sys/time.h>''')
    recoll_conf.set('HAVE_KQUEUE', true)
endif

recoll_conf.set('HAVE_VSNPRINTF', cpp.has_function('vsnprintf'))
recoll_conf.set('HAVE_POSIX_FADVISE', cpp.has_function('posix_fadvise', prefix : '#include <fcntl.h>'))

if get_option('inotify') and cpp.has_function('inotify_add_watch', prefix: '#include <sys/inotify.h>')
    recoll_conf.set('RCL_USE_INOTIFY', true)
    recoll_conf.set('RCL_MONITOR', true)
elif get_option('fam')
    libfam = cpp.find_library('gamin')
    if libfam
        recoll_conf.set('RCL_USE_FAM', true)
        recoll_conf.set('RCL_MONITOR', true)
        librecolldeps += libfam
    endif                          
endif

recoll_conf.set('IDX_THREADS', get_option('idxthreads'))
recoll_conf.set('RCL_SPLIT_CAMELCASE', get_option('camelcase'))
recoll_conf.set('USE_SIMDUTF', get_option('simdutf'))

cfile = configure_file(output: 'autoconfig.h', configuration: recoll_conf)


# Could not find a way to make this work. In case wasaparse.cpp/h need to be rebuilt just do it with
#   ylwrap query/wasaparse.ypp
#bison = find_program('bison')
#wasaparse_cpp = custom_target('wasaparse.cpp',
#  output: ['wasaparse.cpp', 'wasaparse.h'],
#  input: 'query/wasaparse.ypp',
#  command: ['./ylwrap', '@INPUT@'],
#)

librecoll_sources = [
'aspell/rclaspell.cpp',
'bincimapmime/convert.cc',
'bincimapmime/mime-parsefull.cc',
'bincimapmime/mime-parseonlyheader.cc',
'bincimapmime/mime-printbody.cc',
'bincimapmime/mime.cc',
'common/cjksplitter.cpp',
'common/cnsplitter.cpp',
'common/cstr.cpp',
'common/kosplitter.cpp',
'common/rclconfig.cpp',
'common/rclinit.cpp',
'common/syngroups.cpp',
'common/textsplit.cpp',
'common/unacpp.cpp',
'common/utf8fn.cpp',
'common/webstore.cpp',
'index/webqueuefetcher.cpp',
'index/checkretryfailed.cpp',
'index/exefetcher.cpp',
'index/fetcher.cpp',
'index/fsfetcher.cpp',
'index/idxdiags.cpp',
'index/idxstatus.cpp',
'index/mimetype.cpp',
'index/subtreelist.cpp',
'internfile/extrameta.cpp',
'internfile/htmlparse.cpp',
'internfile/internfile.cpp',
'internfile/mh_exec.cpp',
'internfile/mh_execm.cpp',
'internfile/mh_html.cpp',
'internfile/mh_mail.cpp',
'internfile/mh_mbox.cpp',
'internfile/mh_text.cpp',
'internfile/mh_xslt.cpp',
'internfile/mimehandler.cpp',
'internfile/myhtmlparse.cpp',
'internfile/txtdcode.cpp',
'internfile/uncomp.cpp',
'query/docseq.cpp',
'query/docseqdb.cpp',
'query/docseqhist.cpp',
'query/dynconf.cpp',
'query/filtseq.cpp',
'common/plaintorich.cpp',
'query/qresultstore.cpp',
'query/recollq.cpp',
'query/reslistpager.cpp',
'query/sortseq.cpp',
'query/wasaparse.cpp',
'query/wasaparseaux.cpp',
'rcldb/daterange.cpp',
'rcldb/expansiondbs.cpp',
'rcldb/rclabstract.cpp',
'rcldb/rclabsfromtext.cpp',
'rcldb/rcldb.cpp',
'rcldb/rcldoc.cpp',
'rcldb/rcldups.cpp',
'rcldb/rclquery.cpp',
'rcldb/rclterms.cpp',
'rcldb/rclvalues.cpp',
'rcldb/searchdata.cpp',
'rcldb/searchdatatox.cpp',
'rcldb/searchdataxml.cpp',
'rcldb/stemdb.cpp',
'rcldb/stoplist.cpp',
'rcldb/synfamily.cpp',
'unac/unac.cpp',
'utils/appformime.cpp',
'utils/base64.cpp',
'utils/cancelcheck.cpp',
'utils/chrono.cpp',
'utils/circache.cpp',
'utils/closefrom.cpp',
'utils/cmdtalk.cpp',
'utils/conftree.cpp',
'utils/copyfile.cpp',
'utils/cpuconf.cpp',
'utils/ecrontab.cpp',
'utils/execmd.cpp',
'utils/fileudi.cpp',
'utils/fstreewalk.cpp',
'utils/hldata.cpp',
'utils/idfile.cpp',
'utils/listmem.cpp',
'utils/log.cpp',
'utils/md5.cpp',
'utils/md5ut.cpp',
'utils/mimeparse.cpp',
'utils/miniz.cpp',
'utils/netcon.cpp',
'utils/pathut.cpp',
'utils/pxattr.cpp',
'utils/rclionice.cpp',
'utils/rclutil.cpp',
'utils/readfile.cpp',
'utils/smallut.cpp',
'utils/strmatcher.cpp',
'utils/transcode.cpp',
'utils/utf8iter.cpp',
'utils/wipedir.cpp',
'utils/x11mon.cpp',
'utils/zlibut.cpp',
]

librecoll_incdir = include_directories('utils', 'common', 'bincimapmime', 'aspell', 'xaposix',
'rcldb', 'unac', 'index', 'internfile')

librecoll = library(
  'librecoll',
  librecoll_sources,
  name_prefix: '',
  version: meson.project_version(),
  include_directories: librecoll_incdir,
  dependencies: librecolldeps,
  install: true,
)

if get_option('indexer')

recollindex_sources = [
    'index/checkindexed.cpp',
    'index/fsindexer.cpp',
    'index/indexer.cpp',
    'index/rclmonprc.cpp',
    'index/rclmonrcv.cpp',
    'index/recollindex.cpp',
    'index/webqueue.cpp',
]

recollindexdeps = []
recollindexdeps += declare_dependency(link_with : librecoll,
  )
recollindex = executable(
  'recollindex',
  recollindex_sources,
  name_prefix: '',
  include_directories: librecoll_incdir,
  dependencies: recollindexdeps,
  install: true,
)

endif
