cmake_minimum_required(VERSION 2.6)

project(kio_recoll)

find_package(KDE4 REQUIRED)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=7130
    -DRECOLL_DATADIR=\\"${CMAKE_INSTALL_PREFIX}/share/recoll\\"
    -DLIBDIR=\\"${CMAKE_INSTALL_PREFIX}/lib\\"
    -DHAVE_CONFIG_H
)
set(CMAKE_CXX_FLAGS "-fPIC ${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}") 

set(rcltop ${CMAKE_CURRENT_SOURCE_DIR}/../../../)

if (NOT RECOLL_PUBLIC_LIB)
# Execute recoll configuration to create autoconfig.h and version.h and
# generate a PIC lib
execute_process(COMMAND ${rcltop}/configure --disable-static --disable-qtgui --disable-x11mon --disable-python-chm --disable-python-module --prefix=${CMAKE_INSTALL_PREFIX} --mandir=${CMAKE_INSTALL_PREFIX}/share/man
		WORKING_DIRECTORY ${rcltop}
)
add_custom_target(rcllib
                 COMMAND make PicStatic
                 WORKING_DIRECTORY ${rcltop}
)

link_directories(${rcltop}/.libs ${CMAKE_INSTALL_PREFIX}/lib)
endif()

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES}
  ${rcltop}/common 
  ${rcltop}/internfile 
  ${rcltop}/query 
  ${rcltop}/rcldb 
  ${rcltop}/utils 
)

if (RECOLL_PUBLIC_LIB)
  include_directories (
    ${CMAKE_INSTALL_PREFIX}/include/recoll
  )
  # Check if running inside a build tree, then use the not yet installed lib
  # Check if running inside a build tree, then use the not yet installed lib
  if (EXISTS ${CMAKE_BINARY_DIR}/../../librecoll.la)
    link_directories(${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_BINARY_DIR}/../../.libs)
  elseif(EXISTS ${CMAKE_BINARY_DIR}/../../../librecoll.la)
    link_directories(${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_BINARY_DIR}/../../../.libs)
  elseif(EXISTS ${CMAKE_BINARY_DIR}/../../../../librecoll.la)
    link_directories(${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_BINARY_DIR}/../../../../.libs)
  endif()
endif()

set(kio_recoll_SRCS  kio_recoll.cpp htmlif.cpp dirif.cpp ${rcltop}/qtgui/guiutils.cpp)

CHECK_LIBRARY_EXISTS(dl dlopen "" DLOPEN_IN_LIBDL)
IF(DLOPEN_IN_LIBDL)
	LIST(APPEND EXTRA_LIBS dl)
ENDIF(DLOPEN_IN_LIBDL)
CHECK_LIBRARY_EXISTS(pthread pthread_sigmask "" PTHREAD_IN_LIBPTHREAD)
IF(PTHREAD_IN_LIBPTHREAD)
	LIST(APPEND EXTRA_LIBS pthread)
ENDIF(PTHREAD_IN_LIBPTHREAD)

kde4_add_plugin(kio_recoll ${kio_recoll_SRCS})

if (NOT RECOLL_PUBLIC_LIB)
  add_dependencies(kio_recoll rcllib)
endif()

target_link_libraries(kio_recoll recoll xapian xslt xml2 z ${EXTRA_LIBS} ${KDE4_KIO_LIBS} X11)

install(TARGETS kio_recoll DESTINATION ${PLUGIN_INSTALL_DIR})

IF ("${KDE_VERSION_MAJOR}.${KDE_VERSION_MINOR}" GREATER 4.0)
     install(FILES recoll.protocol recollf.protocol DESTINATION ${SERVICES_INSTALL_DIR})
ELSE ("${KDE_VERSION_MAJOR}.${KDE_VERSION_MINOR}" GREATER 4.0)
   install(FILES recollnolist.protocol DESTINATION ${SERVICES_INSTALL_DIR}
   		 RENAME recoll.protocol)
ENDIF ("${KDE_VERSION_MAJOR}.${KDE_VERSION_MINOR}" GREATER 4.0)

install(FILES data/welcome.html	data/help.html
	      DESTINATION  ${DATA_INSTALL_DIR}/kio_recoll)
