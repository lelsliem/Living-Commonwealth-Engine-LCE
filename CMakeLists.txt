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
    VERSION 0.5.0
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