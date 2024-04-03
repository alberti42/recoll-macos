# KF5 version of the kio_recoll module cmake file.

cmake_minimum_required(VERSION 3.16)
project(kio_recoll)

option(RECOLL_PUBLIC_LIB "Use the Recoll public shared library" 1)

set(KF5_MIN_VERSION "5.0")

# Include the Extra-CMake-Modules project
find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n Config KIO Notifications)

## Recoll stuff

set(rcltop ${CMAKE_CURRENT_SOURCE_DIR}/../../../)

if (NOT RECOLL_PUBLIC_LIB)
# Execute recoll configuration to create autoconfig.h and version.h and
# generate a PIC lib
execute_process(COMMAND ${rcltop}/configure --disable-static --disable-qtgui --disable-x11mon --disable-python-chm --disable-python-module --prefix=${CMAKE_INSTALL_PREFIX} --mandir=${CMAKE_INSTALL_PREFIX}/share/man
		WORKING_DIRECTORY ${rcltop}
)
add_custom_target(rcllib
                 COMMAND make -j 3 PicStatic
                 WORKING_DIRECTORY ${rcltop}
)

link_directories(${rcltop}/.libs ${CMAKE_INSTALL_PREFIX}/lib)
endif()

include_directories (
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
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
  if (EXISTS ${CMAKE_BINARY_DIR}/../../librecoll.la)
    link_directories(${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_BINARY_DIR}/../../.libs)
  elseif(EXISTS ${CMAKE_BINARY_DIR}/../../../librecoll.la)
    link_directories(${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_BINARY_DIR}/../../../.libs)
  elseif(EXISTS ${CMAKE_BINARY_DIR}/../../../../librecoll.la)
    link_directories(${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_BINARY_DIR}/../../../../.libs)
  endif()
endif()

add_definitions(
    -DRECOLL_DATADIR="${CMAKE_INSTALL_PREFIX}/share/recoll"
    -DLIBDIR="${CMAKE_INSTALL_PREFIX}/lib"
    -DHAVE_CONFIG_H
)

set(kio_recoll_SRCS kio_recoll.cpp htmlif.cpp dirif.cpp ${rcltop}/qtgui/guiutils.cpp)

add_library(kio_recoll MODULE ${kio_recoll_SRCS})

if (NOT RECOLL_PUBLIC_LIB)
  add_dependencies(kio_recoll rcllib)
endif()

target_link_libraries(kio_recoll
    recoll
    xapian
    xslt
    xml2
    KF5::KIOCore
    X11
    dl
    z
    pthread
)

install(FILES data/welcome.html	data/help.html
      DESTINATION  ${DATA_INSTALL_DIR}/kio_recoll)

set_target_properties(kio_recoll PROPERTIES OUTPUT_NAME "kio_recoll")
# This is not set on ubuntu focal
set(QT_MAJOR_VERSION 6)
install(TARGETS kio_recoll
      DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/kio)
