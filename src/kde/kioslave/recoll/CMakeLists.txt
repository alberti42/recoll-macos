cmake_minimum_required(VERSION 2.6)

project(kio_recoll)

find_package(KDE4 REQUIRED)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=7130
    -DRECOLL_DATADIR=\\"${CMAKE_INSTALL_PREFIX}/share/recoll\\"
    -DLIBDIR=\\"${CMAKE_INSTALL_PREFIX}/lib\\"
    -DHAVE_CONFIG_H
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}") 

set(rcltop ${CMAKE_CURRENT_SOURCE_DIR}/../../../)

# Execute minimum recoll configuration to create autoconfig.h and version.h
execute_process(COMMAND ${rcltop}/configure --without-gui --enable-pic --prefix=${CMAKE_INSTALL_PREFIX} --mandir=${CMAKE_INSTALL_PREFIX}/share/man
		WORKING_DIRECTORY ${rcltop}
)
execute_process(COMMAND make common/rclversion.h
		WORKING_DIRECTORY ${rcltop}
)
execute_process(COMMAND make 
		WORKING_DIRECTORY ${rcltop}/lib
)

link_directories(${rcltop}/lib)

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

set(kio_recoll_SRCS  kio_recoll.cpp htmlif.cpp dirif.cpp)

CHECK_LIBRARY_EXISTS(dl dlopen "" DLOPEN_IN_LIBDL)
IF(DLOPEN_IN_LIBDL)
	LIST(APPEND EXTRA_LIBS dl)
ENDIF(DLOPEN_IN_LIBDL)
CHECK_LIBRARY_EXISTS(pthread pthread_sigmask "" PTHREAD_IN_LIBPTHREAD)
IF(PTHREAD_IN_LIBPTHREAD)
	LIST(APPEND EXTRA_LIBS pthread)
ENDIF(PTHREAD_IN_LIBPTHREAD)

kde4_add_plugin(kio_recoll ${kio_recoll_SRCS})

target_link_libraries(kio_recoll rcl xapian z ${EXTRA_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS kio_recoll DESTINATION ${PLUGIN_INSTALL_DIR})

IF ("${KDE_VERSION_MAJOR}.${KDE_VERSION_MINOR}" GREATER 4.0)
     install(FILES recoll.protocol recollf.protocol DESTINATION ${SERVICES_INSTALL_DIR})
ELSE ("${KDE_VERSION_MAJOR}.${KDE_VERSION_MINOR}" GREATER 4.0)
   install(FILES recollnolist.protocol DESTINATION ${SERVICES_INSTALL_DIR}
   		 RENAME recoll.protocol)
ENDIF ("${KDE_VERSION_MAJOR}.${KDE_VERSION_MINOR}" GREATER 4.0)

install(FILES data/welcome.html	data/help.html
	      DESTINATION  ${DATA_INSTALL_DIR}/kio_recoll)
