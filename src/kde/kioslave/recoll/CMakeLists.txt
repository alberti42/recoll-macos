project(kio_recoll)
set(VERSION_MAJOR "1")
set(VERSION_MINOR "11")
set(VERSION_PATCH "0")
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules )

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Recoll KIO Slave")
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION "${VERSION}")
set(CPACK_GENERATOR TGZ)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION}" CACHE INTERNAL "tarball basename")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME} ${CPACK_PACKAGE_VERSION}")
include(CPack)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include(UsePkgConfig)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=7130
    -DRECOLL_DATADIR=\\"${CMAKE_INSTALL_PREFIX}/share/recoll\\"
    -DLIBDIR=\\"${CMAKE_INSTALL_PREFIX}/lib\\"
    -DHAVE_CONFIG_H
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}") 

set(depth ../../../)
set(rcltop ${CMAKE_CURRENT_SOURCE_DIR}/${depth})

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES}
  ${rcltop}/aspell 
  ${rcltop}/bincimapmime
  ${rcltop}/common 
  ${rcltop}/index 
  ${rcltop}/internfile 
  ${rcltop}/query 
  ${rcltop}/rcldb 
  ${rcltop}/unac
  ${rcltop}/utils 
)

set(kio_recoll_SRCS  kio_recoll.cpp
${depth}/aspell/rclaspell.cpp
${depth}/bincimapmime/mime-getpart.cc 
${depth}/bincimapmime/mime-parsefull.cc 
${depth}/bincimapmime/mime-parseonlyheader.cc 
${depth}/bincimapmime/mime-printbody.cc 
${depth}/bincimapmime/mime-printdoc.cc 
${depth}/bincimapmime/mime-printheader.cc 
${depth}/bincimapmime/mime.cc 
${depth}/bincimapmime/convert.cc 
${depth}/bincimapmime/iodevice.cc 
${depth}/bincimapmime/iofactory.cc 
${depth}/common/rclconfig.cpp
${depth}/common/rclinit.cpp
${depth}/common/textsplit.cpp
${depth}/common/unacpp.cpp
${depth}/index/mimetype.cpp
${depth}/internfile/htmlparse.cpp
${depth}/internfile/myhtmlparse.cpp
${depth}/internfile/mimehandler.cpp
${depth}/internfile/mh_exec.cpp
${depth}/internfile/mh_html.cpp
${depth}/internfile/mh_mail.cpp
${depth}/internfile/mh_mbox.cpp
${depth}/internfile/mh_text.cpp
${depth}/query/docseq.cpp
${depth}/query/docseqdb.cpp
${depth}/query/plaintorich.cpp
${depth}/query/reslistpager.cpp
${depth}/query/wasastringtoquery.cpp
${depth}/query/wasatorcl.cpp
${depth}/rcldb/pathhash.cpp
${depth}/rcldb/rcldb.cpp
${depth}/rcldb/rcldoc.cpp
${depth}/rcldb/rclquery.cpp
${depth}/rcldb/searchdata.cpp
${depth}/rcldb/stemdb.cpp
${depth}/rcldb/stoplist.cpp
${depth}/unac/unac.c
${depth}/utils/base64.cpp
${depth}/utils/conftree.cpp
${depth}/utils/copyfile.cpp
${depth}/utils/debuglog.cpp
${depth}/utils/execmd.cpp
${depth}/utils/idfile.cpp
${depth}/utils/fileudi.cpp
${depth}/utils/md5.cpp
${depth}/utils/mimeparse.cpp
${depth}/utils/pathut.cpp
${depth}/utils/readfile.cpp
${depth}/utils/smallut.cpp
${depth}/utils/transcode.cpp
${depth}/utils/wipedir.cpp
)

kde4_add_plugin(kio_recoll ${kio_recoll_SRCS})

target_link_libraries(kio_recoll xapian dl ${KDE4_KIO_LIBS})

install(TARGETS kio_recoll DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES recoll.protocol DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES data/welcome.html	
	      DESTINATION  ${DATA_INSTALL_DIR}/kio_recoll)
