#==============================================================================
#
# Living Commonwealth Engine (LCE)
# Building living worlds through simulation.
#
# SPDX-License-Identifier: MIT
#
#==============================================================================

#------------------------------------------------------------------------------
# Source Discovery
#
# Automatically discovers all C++ source files beneath Source/.
# New .cpp files are picked up without manually editing this file.
#------------------------------------------------------------------------------

file(
    GLOB_RECURSE
    LCE_CORE_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

#------------------------------------------------------------------------------
# LCE Core
#------------------------------------------------------------------------------

add_library(
    LCE.Core
    STATIC
    ${LCE_CORE_SOURCES}
)

#------------------------------------------------------------------------------
# Public Includes
#------------------------------------------------------------------------------

target_include_directories(
    LCE.Core
    PUBLIC
        "${CMAKE_SOURCE_DIR}/Include"
)

#------------------------------------------------------------------------------
# Dependencies
#------------------------------------------------------------------------------

# spdlog is an implementation detail of the LCE logging subsystem.
# It is deliberately kept private to LCE.Core.

target_link_libraries(
    LCE.Core
    PRIVATE
        spdlog::spdlog
)

#------------------------------------------------------------------------------
# Language Standard
#------------------------------------------------------------------------------

target_compile_features(
    LCE.Core
    PUBLIC
        cxx_std_23
)