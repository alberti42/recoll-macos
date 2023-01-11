#[[

Experimental CMake build.

Since this wasn't planned to be used immediately, the version requirements are very aggressive:

Debian 12+ (Bookworm) (or equivalents)
Qt6.4+
cmake3.25+
C++17 compiler

-----------------------------------------------
    Usage
-----------------------------------------------

cd .../recoll/src

cmake -S . -B build_dir \
      --install-prefix=/usr/local/ \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release

# omit `-G Ninja` to use classical unix makefile as generator

cmake --build build_dir --parallel 7

cmake --install ./build_dir/

Toggling Options

cmake -Dsome_option=ON .....

-----------------------------------------------
    macOS
-----------------------------------------------

To assemble a portable .app bundle, you must use the Qt from official installer.
The Homebrew Qt doesn't work, this is a known bug and unlikely to be fixed in short time.

To specify qt path, specify `CMAKE_PREFIX_PATH` when generating the build_dir

cmake -DCMAKE_PREFIX_PATH="~/Qt/6.4.2/macos" ....

 ~/Qt/6.4.2/macos/bin/macdeployqt recoll.app

To completely avoid codesign problems, just force one #TODO: use personal codesign

codesign --force --deep -s - recoll.app


]]

cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(recoll
        VERSION 1.34.0
        LANGUAGES CXX C)

### Configure Options

if (LINUX)
    option(enable_x11mon "x11mon" ON)
else ()
    option(enable_x11mon "x11mon" OFF)
endif ()

option(enable_webengine "enable webengine" ON)

### Platforms

if (APPLE)
    # assuming packages are installed via homebrew.
    # External dependencies will be copied automatically via macdeployqt
    include_directories(/usr/local/include /opt/homebrew/include)
    add_compile_definitions(RECOLL_AS_MAC_BUNDLE)
endif ()

## Definitions soup

add_compile_definitions(BUILDING_RECOLL)
add_compile_definitions(READFILE_ENABLE_ZLIB READFILE_ENABLE_MINIZ READFILE_ENABLE_MD5)

add_compile_definitions(HAVE_DLOPEN)

add_compile_definitions(RECOLL_DATADIR="${CMAKE_INSTALL_PREFIX}/share/recoll")

if (NOT enable_x11mon)
    add_compile_definitions(DISABLE_X11MON)
endif ()

if (enable_webengine)
    add_compile_definitions(USING_WEBENGINE)
endif ()


### External Libraries

set(QtComponents Core Gui Widgets PrintSupport LinguistTools)
set(RECOLL_QT_LIBRARIES Qt6::Core Qt6::Gui Qt6::Widgets Qt6::PrintSupport)

if (enable_webengine)
    list(APPEND QtComponents WebEngineWidgets)
    list(APPEND RECOLL_QT_LIBRARIES Qt::WebEngineWidgets)
endif ()

find_package(Qt6 REQUIRED COMPONENTS ${QtComponents})

# To know ${VARS} provided with find_package, check the urls

find_package(LibXml2 REQUIRED) # https://cmake.org/cmake/help/latest/module/FindLibXml2.html
find_package(LibXslt REQUIRED) # https://cmake.org/cmake/help/latest/module/FindLibXslt.html
find_package(Xapian REQUIRED) # https://github.com/xapian/xapian/blob/master/xapian-core/cmake/xapian-config.cmake.in
if (enable_x11mon)
    find_package(X11 REQUIRED) # https://cmake.org/cmake/help/latest/module/FindX11.html
endif ()
if (APPLE)
    find_package(ZLIB REQUIRED) # https://cmake.org/cmake/help/latest/module/FindZLIB.html
    find_package(Iconv REQUIRED) # https://cmake.org/cmake/help/latest/module/FindIconv.html
endif ()
#

qt_standard_project_setup()

# simulate autoheader
configure_file(common/autoconfig.h.cmake.in autoconfig.h @ONLY)


### Source files
set(librecoll_SOURCES
        aspell/rclaspell.cpp
        aspell/rclaspell.h
        bincimapmime/convert.cc
        bincimapmime/convert.h
        bincimapmime/mime-inputsource.h
        bincimapmime/mime-parsefull.cc
        bincimapmime/mime-parseonlyheader.cc
        bincimapmime/mime-printbody.cc
        bincimapmime/mime-utils.h
        bincimapmime/mime.cc
        bincimapmime/mime.h
        common/webstore.cpp
        common/webstore.h
        common/conf_post.h
        common/cstr.cpp
        common/cstr.h
        common/rclconfig.cpp
        common/rclconfig.h
        common/rclinit.cpp
        common/rclinit.h
        common/syngroups.cpp
        common/syngroups.h
        common/textsplit.cpp
        common/textsplitko.cpp
        common/textsplit.h
        common/unacpp.cpp
        common/unacpp.h
        common/uproplist.h
        common/utf8fn.cpp
        common/utf8fn.h
        index/webqueuefetcher.cpp
        index/webqueuefetcher.h
        index/checkretryfailed.cpp
        index/checkretryfailed.h
        index/exefetcher.cpp
        index/exefetcher.h
        index/fetcher.cpp
        index/fetcher.h
        index/fsfetcher.cpp
        index/fsfetcher.h
        index/idxdiags.h
        index/idxdiags.cpp
        index/idxstatus.h
        index/idxstatus.cpp
        index/mimetype.cpp
        index/mimetype.h
        index/rclmon.h
        index/recollindex.h
        index/subtreelist.cpp
        index/subtreelist.h
        internfile/Filter.h
        internfile/extrameta.cpp
        internfile/extrameta.h
        internfile/htmlparse.cpp
        internfile/htmlparse.h
        internfile/indextext.h
        internfile/internfile.cpp
        internfile/internfile.h
        internfile/mh_exec.cpp
        internfile/mh_exec.h
        internfile/mh_execm.cpp
        internfile/mh_execm.h
        internfile/mh_html.cpp
        internfile/mh_html.h
        internfile/mh_mail.cpp
        internfile/mh_mail.h
        internfile/mh_mbox.cpp
        internfile/mh_mbox.h
        internfile/mh_null.h
        internfile/mh_symlink.h
        internfile/mh_text.cpp
        internfile/mh_text.h
        internfile/mh_unknown.h
        internfile/mh_xslt.cpp
        internfile/mh_xslt.h
        internfile/mimehandler.cpp
        internfile/mimehandler.h
        internfile/myhtmlparse.cpp
        internfile/myhtmlparse.h
        internfile/txtdcode.cpp
        internfile/uncomp.cpp
        internfile/uncomp.h
        query/docseq.cpp
        query/docseq.h
        query/docseqdb.cpp
        query/docseqdb.h
        query/docseqdocs.h
        query/docseqhist.cpp
        query/docseqhist.h
        query/dynconf.cpp
        query/dynconf.h
        query/filtseq.cpp
        query/filtseq.h
        common/plaintorich.cpp
        common/plaintorich.h
        query/qresultstore.cpp
        query/qresultstore.h
        query/recollq.cpp
        query/recollq.h
        query/reslistpager.cpp
        query/reslistpager.h
        query/sortseq.cpp
        query/sortseq.h
        query/wasaparse.cpp # generated via wasaparse.ypp
        query/wasaparseaux.cpp
        query/wasaparserdriver.h
        query/wasatorcl.h
        rcldb/daterange.cpp
        rcldb/daterange.h
        rcldb/expansiondbs.cpp
        rcldb/expansiondbs.h
        rcldb/rclabstract.cpp
        rcldb/rclabsfromtext.cpp
        rcldb/rcldb.cpp
        rcldb/rcldb.h
        rcldb/rcldb_p.h
        rcldb/rcldoc.cpp
        rcldb/rcldoc.h
        rcldb/rcldups.cpp
        rcldb/rclquery.cpp
        rcldb/rclquery.h
        rcldb/rclquery_p.h
        rcldb/rclterms.cpp
        rcldb/rclvalues.cpp
        rcldb/rclvalues.h
        rcldb/searchdata.cpp
        rcldb/searchdata.h
        rcldb/searchdatatox.cpp
        rcldb/searchdataxml.cpp
        rcldb/stemdb.cpp
        rcldb/stemdb.h
        rcldb/stoplist.cpp
        rcldb/stoplist.h
        rcldb/synfamily.cpp
        rcldb/synfamily.h
        rcldb/termproc.h
        rcldb/xmacros.h
        unac/unac.cpp
        unac/unac.h
        unac/unac_version.h
        utils/appformime.cpp
        utils/appformime.h
        utils/base64.cpp
        utils/base64.h
        utils/cancelcheck.cpp
        utils/cancelcheck.h
        utils/chrono.h
        utils/chrono.cpp
        utils/circache.cpp
        utils/circache.h
        utils/closefrom.cpp
        utils/closefrom.h
        utils/cmdtalk.cpp
        utils/cmdtalk.h
        utils/conftree.cpp
        utils/conftree.h
        utils/copyfile.cpp
        utils/copyfile.h
        utils/cpuconf.cpp
        utils/cpuconf.h
        utils/damlev.h
        utils/dlib.cpp
        utils/dlib.h
        utils/ecrontab.cpp
        utils/ecrontab.h
        utils/execmd.cpp
        utils/execmd.h
        utils/fileudi.cpp
        utils/fileudi.h
        utils/fstreewalk.cpp
        utils/fstreewalk.h
        utils/hldata.h
        utils/hldata.cpp
        utils/idfile.cpp
        utils/idfile.h
        utils/listmem.cpp
        utils/listmem.h
        utils/log.cpp
        utils/log.h
        utils/md5.cpp
        utils/md5.h
        utils/md5ut.cpp
        utils/md5ut.h
        utils/mimeparse.cpp
        utils/mimeparse.h
        utils/miniz.cpp
        utils/miniz.h
        utils/netcon.cpp
        utils/netcon.h
        utils/pathut.cpp
        utils/pathut.h
        utils/picoxml.h
        utils/pxattr.cpp
        utils/pxattr.h
        utils/rclionice.cpp
        utils/rclionice.h
        utils/rclutil.h
        utils/rclutil.cpp
        utils/readfile.cpp
        utils/readfile.h
        utils/smallut.cpp
        utils/smallut.h
        utils/strmatcher.cpp
        utils/strmatcher.h
        utils/transcode.cpp
        utils/transcode.h
        utils/utf8iter.cpp
        utils/utf8iter.h
        utils/wipedir.cpp
        utils/wipedir.h
        utils/workqueue.h
        utils/x11mon.cpp
        utils/x11mon.h
        utils/zlibut.cpp
        utils/zlibut.h
        xaposix/safefcntl.h
        xaposix/safesysstat.h
        xaposix/safesyswait.h
        xaposix/safeunistd.h
        )

set(recollindex_SOURCES
        index/checkindexed.cpp
        index/checkindexed.h
        index/fsindexer.cpp
        index/fsindexer.h
        index/indexer.cpp
        index/indexer.h
        index/rclmonprc.cpp
        index/rclmonrcv.cpp
        index/recollindex.cpp
        index/webqueue.cpp
        index/webqueue.h
        )

set(recollq_SOURCE
        query/recollqmain.cpp
        query/recollq.cpp
        query/recollq.h)

# qtgui related

set(qtgui_ui_SOURCES
        qtgui/actsearch.ui
        qtgui/advsearch.ui
        qtgui/crontool.ui
        qtgui/firstidx.ui
        qtgui/idxsched.ui
        qtgui/preview.ui
        qtgui/ptrans.ui
        qtgui/rclmain.ui
        qtgui/restable.ui
        qtgui/rtitool.ui
        qtgui/snippets.ui
        qtgui/specialindex.ui
        qtgui/spell.ui
        qtgui/ssearchb.ui
        qtgui/uiprefs.ui
        qtgui/viewaction.ui
        qtgui/webcache.ui
        qtgui/widgets/editdialog.ui
        qtgui/widgets/listdialog.ui
        qtgui/winschedtool.ui
        )

set(qtgui_SOURCES
        qtgui/actsearch_w.cpp
        qtgui/actsearch_w.h
        qtgui/advsearch_w.cpp
        qtgui/advsearch_w.h
        qtgui/advshist.cpp
        qtgui/advshist.h
        qtgui/confgui/confgui.cpp
        qtgui/confgui/confgui.h
        qtgui/confgui/confguiindex.cpp
        qtgui/confgui/confguiindex.h
        qtgui/crontool.cpp
        qtgui/crontool.h
        qtgui/firstidx.h
        qtgui/fragbuts.cpp
        qtgui/fragbuts.h
        qtgui/guiutils.cpp
        qtgui/guiutils.h
        qtgui/idxmodel.cpp
        qtgui/idxmodel.h
        qtgui/idxsched.h
        qtgui/main.cpp
        qtgui/multisave.cpp
        qtgui/multisave.h
        qtgui/preview_load.cpp
        qtgui/preview_load.h
        qtgui/preview_plaintorich.cpp
        qtgui/preview_plaintorich.h
        qtgui/preview_w.cpp
        qtgui/preview_w.h
        qtgui/ptrans_w.cpp
        qtgui/ptrans_w.h
        qtgui/rclhelp.cpp
        qtgui/rclhelp.h
        qtgui/rclm_idx.cpp
        qtgui/rclm_menus.cpp
        qtgui/rclm_preview.cpp
        qtgui/rclm_saveload.cpp
        qtgui/rclm_sidefilters.cpp
        qtgui/rclm_view.cpp
        qtgui/rclm_wins.cpp
        qtgui/rclmain_w.cpp
        qtgui/rclmain_w.h
        qtgui/rclzg.cpp
        qtgui/rclzg.h
        qtgui/recoll.h
        qtgui/reslist.cpp
        qtgui/reslist.h
        qtgui/respopup.cpp
        qtgui/respopup.h
        qtgui/restable.cpp
        qtgui/restable.h
        qtgui/rtitool.cpp
        qtgui/rtitool.h
        qtgui/scbase.cpp
        qtgui/scbase.h
        qtgui/searchclause_w.cpp
        qtgui/searchclause_w.h
        qtgui/snippets_w.cpp
        qtgui/snippets_w.h
        qtgui/specialindex.h
        qtgui/spell_w.cpp
        qtgui/spell_w.h
        qtgui/ssearch_w.cpp
        qtgui/ssearch_w.h
        qtgui/systray.cpp
        qtgui/systray.h
        qtgui/uiprefs_w.cpp
        qtgui/uiprefs_w.h
        qtgui/viewaction_w.cpp
        qtgui/viewaction_w.h
        qtgui/webcache.cpp
        qtgui/webcache.h
        qtgui/widgets/editdialog.h
        qtgui/widgets/listdialog.h
        qtgui/widgets/qxtconfirmationmessage.cpp
        qtgui/widgets/qxtconfirmationmessage.h
        qtgui/widgets/qxtglobal.h
        qtgui/xmltosd.cpp
        qtgui/xmltosd.h
        )

set(example_config_files
        sampleconf/fields
        sampleconf/fragment-buttons.xml
        sampleconf/mimeconf
        sampleconf/mimemap
        sampleconf/mimeview
        sampleconf/recoll-common.css
        sampleconf/recoll-common.qss
        sampleconf/recoll-dark.css
        sampleconf/recoll-dark.qss
        sampleconf/recoll.conf
        sampleconf/recoll.qss
        )

set(filter_files
        python/recoll/recoll/rclconfig.py
        python/recoll/recoll/conftree.py
        filters/abiword.xsl
        filters/cmdtalk.py
        filters/fb2.xsl
        filters/gnumeric.xsl
        filters/kosplitter.py
        filters/msodump.zip
        filters/okular-note.xsl
        filters/opendoc-body.xsl
        filters/opendoc-flat.xsl
        filters/opendoc-meta.xsl
        filters/openxml-meta.xsl
        filters/openxml-word-body.xsl
        filters/openxml-xls-body.xsl
        filters/ppt-dump.py
        filters/rcl7z.py
        filters/rclaptosidman
        filters/rclaspell-sugg.py
        filters/rclaudio.py
        filters/rclbasehandler.py
        filters/rclbibtex.sh
        filters/rclcheckneedretry.sh
        filters/rclchm.py
        filters/rcldia.py
        filters/rcldjvu.py
        filters/rcldoc.py
        filters/rcldvi
        filters/rclepub.py
        filters/rclepub1.py
        filters/rclexec1.py
        filters/rclexecm.py
        filters/rclfb2.py
        filters/rclgaim
        filters/rclgenxslt.py
        filters/rclhwp.py
        filters/rclics.py
        filters/rclimg
        filters/rclimg.py
        filters/rclinfo.py
        filters/rclipynb.py
        filters/rclkar.py
        filters/rclkwd
        filters/rcllatinclass.py
        filters/rcllatinstops.zip
        filters/rcllyx
        filters/rclman
        filters/rclmidi.py
        filters/rclocr.py
        filters/rclocrabbyy.py
        filters/rclocrcache.py
        filters/rclocrtesseract.py
        filters/rclopxml.py
        filters/rclorgmode.py
        filters/rclpdf.py
        filters/rclppt.py
        filters/rclps
        filters/rclpst.py
        filters/rclpurple
        filters/rclpython.py
        filters/rclrar.py
        filters/rclrtf.py
        filters/rclscribus
        filters/rclshowinfo
        filters/rcltar.py
        filters/rcltex
        filters/rcltext.py
        filters/rcluncomp
        filters/rcluncomp.py
        filters/rclwar.py
        filters/rclxls.py
        filters/rclxml.py
        filters/rclxmp.py
        filters/rclxslt.py
        filters/rclzip.py
        filters/recoll-we-move-files.py
        filters/recollepub.zip
        filters/svg.xsl
        filters/thunderbird-open-message.sh
        filters/xls-dump.py
        filters/xlsxmltocsv.py
        filters/xml.xsl
        )

set(mtpics_files
        qtgui/mtpics/aptosid-book.png
        qtgui/mtpics/aptosid-manual.png
        qtgui/mtpics/archive.png
        qtgui/mtpics/book.png
        qtgui/mtpics/bookchap.png
        qtgui/mtpics/document.png
        qtgui/mtpics/drawing.png
        qtgui/mtpics/emblem-symbolic-link.png
        qtgui/mtpics/folder.png
        qtgui/mtpics/html.png
        qtgui/mtpics/image.png
        qtgui/mtpics/message.png
        qtgui/mtpics/mozilla_doc.png
        qtgui/mtpics/pdf.png
        qtgui/mtpics/pidgin.png
        qtgui/mtpics/postscript.png
        qtgui/mtpics/presentation.png
        qtgui/mtpics/sidux-book.png
        qtgui/mtpics/soffice.png
        qtgui/mtpics/source.png
        qtgui/mtpics/sownd.png
        qtgui/mtpics/spreadsheet.png
        qtgui/mtpics/text-x-python.png
        qtgui/mtpics/txt.png
        qtgui/mtpics/video.png
        qtgui/mtpics/wordprocessing.png
        )

set(COMMON_INCLUDES_DIRS
        bincimapmime
        common
        index
        internfile
        rcldb
        unac
        utils
        xaposix
        )

### Targets

# Libraries shared by all targets. The cmake manual says include_directories() should be avoided, btw.
include_directories(
        ${PROJECT_BINARY_DIR} # for autoconfig.h
        ${COMMON_INCLUDES_DIRS}
)

#--- librecoll

if (LINUX)
    add_library(librecoll SHARED ${librecoll_SOURCES})
else ()
    add_library(librecoll STATIC ${librecoll_SOURCES})
endif ()

target_link_libraries(librecoll PRIVATE ${XAPIAN_LIBRARIES} LibXml2::LibXml2 LibXslt::LibXslt)
set_target_properties(librecoll PROPERTIES VERSION ${PROJECT_VERSION})

if (APPLE)
    target_link_libraries(librecoll PRIVATE ZLIB::ZLIB Iconv::Iconv)
endif ()

#--- recollindex ---

add_executable(recollindex ${recollindex_SOURCES})
target_link_libraries(recollindex PRIVATE
        librecoll
        ${XAPIAN_LIBRARIES}
        )

if (enable_x11mon)
    target_link_libraries(recollindex PRIVATE ${X11_LIBRARIES})
endif ()

#--- recollq ---

add_executable(recollq ${recollq_SOURCE})
target_link_libraries(recollq PRIVATE
        librecoll
        ${XAPIAN_LIBRARIES}
        )

if (enable_x11mon)
    target_link_libraries(recollq PRIVATE ${X11_LIBRARIES})
endif ()

#--- recoll ---

add_executable(recoll ${qtgui_SOURCES} ${qtgui_ui_SOURCES})

# we canont use the recoll.qrc file because the search path of `.qrc` is relative to the project root (?)
# To have a single cmake file, we either use this, or move & modify the recoll.qrc
qt_add_resources(recoll "images"
        PREFIX "/"
        BASE "qtgui"
        FILES
        qtgui/images/asearch.png
        qtgui/images/cancel.png
        qtgui/images/clock.png
        qtgui/images/close.png
        qtgui/images/code-block.png
        qtgui/images/down.png
        qtgui/images/firstpage.png
        qtgui/images/history.png
        qtgui/images/interro.png
        qtgui/images/menu.png
        qtgui/images/nextpage.png
        qtgui/images/prevpage.png
        qtgui/images/recoll.png
        qtgui/images/sortparms.png
        qtgui/images/spell.png
        qtgui/images/table.png
        qtgui/images/up.png
        )

target_include_directories(recoll PRIVATE
        qtgui
        qtgui/widgets
        query
        )

target_link_libraries(recoll PRIVATE
        librecoll
        ${XAPIAN_LIBRARIES}
        ${RECOLL_QT_LIBRARIES})

add_dependencies(recoll librecoll recollindex) # ensure `recollindex` and `librecoll` always built before `recoll`

qt_finalize_target(recoll)

# default target that build everything

add_custom_target(recoll_ALL ALL)
ADD_DEPENDENCIES(recoll_ALL recoll recollq recollindex librecoll)

### Special handling of translation files

# include all *.ts files under locale
file(GLOB TRANS_FILES "qtgui/i18n/*.ts")

# Put generated *.qm to output dir's locale
set_source_files_properties(${TRANS_FILES}
        PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/i18n")

# Do *.ts -> *.qm during the target `recoll` and store result files to ${QM_FILES}
qt_add_translations(recoll TS_FILES ${TRANS_FILES}
        QM_FILES_OUTPUT_VARIABLE QM_FILES)

### Install targets and other runtime files

if (LINUX)
    install(TARGETS librecoll LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(TARGETS recollindex RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(TARGETS recollq RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(TARGETS recoll RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    install(FILES ${CMAKE_SOURCE_DIR}/desktop/recoll-searchgui.desktop DESTINATION share/applications)
    install(FILES ${CMAKE_SOURCE_DIR}/desktop/recoll.appdata.xml DESTINATION share/metainfo)

    install(FILES ${CMAKE_SOURCE_DIR}/desktop/recoll.png DESTINATION share/icons/hicolor/48x48/apps)
    install(FILES ${CMAKE_SOURCE_DIR}/desktop/recoll.png DESTINATION share/icons/pixmaps)

    install(FILES ${CMAKE_SOURCE_DIR}/doc/user/usermanual.html
            ${CMAKE_SOURCE_DIR}/doc/user/docbook-xsl.css DESTINATION share/recoll/doc)

    install(FILES ${example_config_files} DESTINATION share/recoll/examples)
    install(FILES ${filter_files} DESTINATION share/recoll/filters)
    install(FILES ${mtpics_files} DESTINATION share/recoll/images)

    install(FILES ${QM_FILES} DESTINATION share/recoll/translations)
endif ()

# .app bundle assembling

if (APPLE)
    set(bundle_icon qtgui/images/recoll.icns)
    set(docs_files ${CMAKE_SOURCE_DIR}/doc/user/usermanual.html ${CMAKE_SOURCE_DIR}/doc/user/docbook-xsl.css)
    target_sources(recoll PRIVATE ${example_config_files} ${filter_files} ${mtpics_files} ${bundle_icon} ${docs_files})

    set_source_files_properties(${example_config_files} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/examples)
    set_source_files_properties(${filter_files} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/filters)
    set_source_files_properties(${mtpics_files} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/images)
    set_source_files_properties(${bundle_icon} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(${docs_files} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/doc)

    set_target_properties(recoll PROPERTIES
            MACOSX_BUNDLE_GUI_IDENTIFIER org.lesbonscomptes.recoll
            MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
            MACOSX_BUNDLE_ICON_FILE recoll.icns
            MACOSX_BUNDLE TRUE)

    set_target_properties(recollindex PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/recoll.app/Contents/MacOS/)

    set_target_properties(recollq PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/recoll.app/Contents/MacOS/)
endif ()