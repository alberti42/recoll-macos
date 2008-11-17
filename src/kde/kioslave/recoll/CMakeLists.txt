project(kio_recoll)
set(VERSION_MAJOR "1")
set(VERSION_MINOR "11")
set(VERSION_PATCH "0")
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules )

include(InstallRequiredSystemLibraries)

#set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Recoll KIO Slave")
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
#set(CPACK_PACKAGE_VERSION "${VERSION}")
#set(CPACK_GENERATOR TGZ)
#set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION}" CACHE INTERNAL "
#tarball basename")
#SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME} ${CPACK_PACKAGE_VERSION}")
#include(CPack)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include (UsePkgConfig)
include (KDE4Defaults)
include (MacroLibrary)
include(MacroOptionalAddSubdirectory)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

set(depth ../../../)

include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES}
${depth}/utils ${depth}/rcldb ${depth}/query ${depth}/unac
${depth}/common ${depth}/internfile ${depth}/index ${depth}/bincimapmime
)

set(kio_recoll_SRCS
   kio_recoll.cpp
)

kde4_add_plugin(kio_recoll ${kio_recoll_SRCS})
link_directories(/home/dockes/projets/fulltext/recoll/src/lib)
target_link_libraries(kio_recoll rcl xapian ${KDE4_KIO_LIBS})


install(TARGETS kio_recoll DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES recoll.protocol DESTINATION ${SERVICES_INSTALL_DIR} )
