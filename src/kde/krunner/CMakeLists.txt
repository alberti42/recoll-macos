# Create from https://develop.kde.org/docs/extend/plasma/krunner/ as example
# Dependancies:
#  ki18n-devel plasma-framework-devel kconfig-devel kpackage-devel
#  krunner-devel knotifications-devel

cmake_minimum_required(VERSION 3.16)
project(runnerexample)

set(KF5_MIN_VERSION "5.90")

# Include the Extra-CMake-Modules project
find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n Config Runner KIO Notifications)


## Recoll stuff
add_definitions(
    -DRECOLL_DATADIR="${CMAKE_INSTALL_PREFIX}/share/recoll"
    -DLIBDIR="${CMAKE_INSTALL_PREFIX}/lib"
    -DHAVE_CONFIG_H
)


set(rcltop ${CMAKE_CURRENT_SOURCE_DIR}/../../)

# Execute recoll configuration to create autoconfig.h and version.h and
# generate a PIC lib
execute_process(COMMAND ${rcltop}/configure --disable-static --disable-qtgui --disable-x11mon --disable-python-chm --disable-python-module --prefix=${CMAKE_INSTALL_PREFIX} --mandir=${CMAKE_INSTALL_PREFIX}/share/man
    WORKING_DIRECTORY ${rcltop}
)

link_directories(${rcltop}/.libs ${CMAKE_INSTALL_PREFIX}/lib)

include_directories (${CMAKE_SOURCE_DIR}
  ${rcltop}/aspell 
  ${rcltop}/bincimapmime
  ${rcltop}/common 
  ${rcltop}/index 
  ${rcltop}/internfile 
  ${rcltop}/query 
  ${rcltop}/rcldb 
  ${rcltop}/unac
  ${rcltop}/utils 
  ${rcltop}/qtgui
)

add_custom_target(rcllib
    COMMAND make -j 3 PicStatic
    WORKING_DIRECTORY ${rcltop}
)


kcoreaddons_add_plugin(runner_recoll SOURCES recollrunner.cpp INSTALL_NAMESPACE "kf5/krunner")
add_dependencies(runner_recoll rcllib)


target_link_libraries(runner_recoll
    KF5::I18n
    KF5::ConfigCore
    KF5::Runner
    KF5::KIOWidgets
    KF5::Notifications
    recoll
    xapian
    xslt
    xml2
    X11
    dl
    z
    pthread
)

