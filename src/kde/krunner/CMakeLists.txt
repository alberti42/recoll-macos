# KF5 version of the recoll krunner module.
# Create from https://develop.kde.org/docs/extend/plasma/krunner/ as example
# Dependancies:
#  ki18n-devel plasma-framework-devel kconfig-devel kpackage-devel
#  krunner-devel knotifications-devel

cmake_minimum_required(VERSION 3.16)
project(runnerexample)

set(KF5_MIN_VERSION "5.0")

# Include the Extra-CMake-Modules project
find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS I18n Config Runner KIO Notifications Config)

## Recoll stuff
set(rcltop ${CMAKE_CURRENT_SOURCE_DIR}/../../)

add_definitions(
    -DRECOLL_DATADIR="${CMAKE_INSTALL_PREFIX}/share/recoll"
    -DLIBDIR="${CMAKE_INSTALL_PREFIX}/lib"
    -DHAVE_CONFIG_H
)


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

  if (EXISTS ${CMAKE_BINARY_DIR}/../../librecoll.la)
    link_directories(${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_BINARY_DIR}/../../.libs)
  elseif(EXISTS ${CMAKE_BINARY_DIR}/../../../librecoll.la)
    link_directories(${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_BINARY_DIR}/../../../.libs)
  elseif(EXISTS ${CMAKE_BINARY_DIR}/../../../../librecoll.la)
    link_directories(${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_BINARY_DIR}/../../../../.libs)
  endif()
endif()

kcoreaddons_add_plugin(krunner_recoll SOURCES recollrunner.cpp INSTALL_NAMESPACE "kf6/krunner")
if (NOT RECOLL_PUBLIC_LIB)
add_dependencies(krunner_recoll rcllib)
endif()
set_property(TARGET krunner_recoll PROPERTY CXX_STANDARD 14)

target_link_libraries(krunner_recoll
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

