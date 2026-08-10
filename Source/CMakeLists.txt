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

# One name everywhere: consumers inside the tree (samples, tools,
# adapters via FetchContent) and consumers of the installed package both
# write LCE::Core. The alias keeps the in-tree and exported identities
# identical.
add_library(LCE::Core ALIAS LCE.Core)

#------------------------------------------------------------------------------
# Public Includes
#
# BUILD_INTERFACE points at the source tree while developing;
# INSTALL_INTERFACE points at the installed headers so the exported
# package never references a machine-specific source path.
#------------------------------------------------------------------------------

include(GNUInstallDirs)

target_include_directories(
    LCE.Core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../Include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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