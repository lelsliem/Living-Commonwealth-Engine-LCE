#==============================================================================
#
#  Living Commonwealth Engine (LCE)
#  Building living worlds through simulation.
#
#  SPDX-License-Identifier: MIT
#
#==============================================================================

cmake_minimum_required(VERSION 3.28)

#------------------------------------------------------------------------------
# Project Information
#------------------------------------------------------------------------------

project(
    LivingCommonwealthEngine
    VERSION 0.8.6
    DESCRIPTION "Living Commonwealth Engine"
    LANGUAGES CXX
)

#------------------------------------------------------------------------------
# Global Build Settings
#------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#------------------------------------------------------------------------------
# Build Options
#------------------------------------------------------------------------------

option(LCE_BUILD_TESTS
    "Build LCE test applications."
    ON)

option(LCE_BUILD_SAMPLES
    "Build LCE sample applications."
    OFF)

option(LCE_BUILD_TOOLS
    "Build LCE developer tools (LCE Doctor)."
    OFF)

option(LCE_BUILD_DOCS
    "Build project documentation."
    OFF)

#------------------------------------------------------------------------------
# Output Directories
#------------------------------------------------------------------------------

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Bin")

#------------------------------------------------------------------------------
# Compiler Configuration
#------------------------------------------------------------------------------

if(MSVC)

    add_compile_options(

        /W4             # Warning Level 4
        /WX             # Treat warnings as errors

        /permissive-    # Strict standard compliance

        /utf-8          # UTF-8 source

        /Zc:__cplusplus # Correct __cplusplus value

    )

    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>")

endif()

#------------------------------------------------------------------------------
# Third-Party Dependencies
#------------------------------------------------------------------------------

# spdlog provides the backend for LCE's logging subsystem.
# LCE links to spdlog privately so it does not become part of the public API.

# Fetched via FetchContent rather than vendored: a fresh clone configures
# and builds with no manual steps — no gitlinks, no submodule bookkeeping.
# Pinned to the version used since 0.0.1.

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.17.0
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(spdlog)

#------------------------------------------------------------------------------
# Build Targets
#------------------------------------------------------------------------------

add_subdirectory(Source)

if(LCE_BUILD_TESTS)
    add_subdirectory(Tests)
endif()

if(LCE_BUILD_SAMPLES)
    add_subdirectory(Samples)
endif()

if(LCE_BUILD_TOOLS)
    add_subdirectory(Tools)
endif()

#------------------------------------------------------------------------------
# Packaging (0.5.0 SDK)
#
# Install rules + an exported package, so consumers can find_package(LCE)
# after cmake --install. The official embedding recipe stays FetchContent
# (the primary path, documented in Docs/SDK/Embedding.md); find_package
# is the installed-path option. The exported target is LCE::Core (the
# in-tree target keeps its honest name, LCE.Core).
#------------------------------------------------------------------------------

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# The friendly public name: LCE::Core on the consumer side.
set_target_properties(LCE.Core PROPERTIES EXPORT_NAME Core)

# LCE.Core is a static library, so its private link to spdlog is part of
# the consumer's link interface ($<LINK_ONLY:spdlog>). spdlog therefore
# rides in the same export set and installs alongside the core — the
# installed package is self-contained: find_package(LCE) needs nothing
# else on the path. (The FetchContent embedding path is the alternative,
# documented in Docs/SDK/Embedding.md.)
install(TARGETS LCE.Core spdlog
    EXPORT  LCETargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# spdlog's public headers, next to the core's.
install(DIRECTORY ${spdlog_SOURCE_DIR}/include/spdlog
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# The stable public header surface — Include/LCE and nothing else.
install(DIRECTORY Include/LCE
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT LCETargets
    FILE        LCETargets.cmake
    NAMESPACE   LCE::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LCE
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LCEConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LCEConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LCE
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/LCEConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LCEConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LCEConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LCE
)