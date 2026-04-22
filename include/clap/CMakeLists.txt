add_library(clap INTERFACE)
add_library(clap::clap ALIAS clap)

target_include_directories(clap INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_options(clap INTERFACE
  -freflection
  -fcontracts
)

target_compile_features(clap INTERFACE cxx_std_26)
target_link_libraries(clap INTERFACE stdc++exp)

include(GNUInstallDirs)

install(TARGETS clap EXPORT clapTargets)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

install(EXPORT clapTargets
        NAMESPACE clap::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clap)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/clapConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/clapConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clap
)
write_basic_package_version_file("clapConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/clapConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/clapConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clap)
