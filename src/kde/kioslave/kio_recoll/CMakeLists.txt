cmake_minimum_required(VERSION 3.16.0)
project(kio_recoll)

include(FeatureSummary)

set(QT_MIN_VERSION 5.2.0)
set(KF5_MIN_VERSION 5.0.0)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Network
    Widgets)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

# CoreAddons?
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS KIO)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

## Recoll stuff

set(rcltop ${CMAKE_CURRENT_SOURCE_DIR}/../../../)

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

include_directories (
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${rcltop}/common 
  ${rcltop}/internfile 
  ${rcltop}/query 
  ${rcltop}/rcldb 
  ${rcltop}/utils 
)

add_definitions(
    -DRECOLL_DATADIR="${CMAKE_INSTALL_PREFIX}/share/recoll"
    -DLIBDIR="${CMAKE_INSTALL_PREFIX}/lib"
    -DHAVE_CONFIG_H
)

set(kio_recoll_SRCS kio_recoll.cpp htmlif.cpp dirif.cpp ${rcltop}/qtgui/guiutils.cpp)


add_library(kio_recoll MODULE ${kio_recoll_SRCS})
add_dependencies(kio_recoll rcllib)

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
set(QT_MAJOR_VERSION 5)
install(TARGETS kio_recoll
      DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/kio)
