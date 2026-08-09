//=============================================================================//
// ┌─────────────────────────────────────────────────────────────────────────┐ 
// │                                                                                                     
// │                       ██╗      ██████╗███████╗                                               
// │                       ██║     ██╔════╝██╔════╝                                               
// │                       ██║     ██║     █████╗                                                   
// │                       ██║     ██║     ██╔══╝                                                   
// │                       ███████╗╚██████╗███████╗                                            
// │                       ╚══════╝ ╚═════╝╚══════╝                            	            
// │                                                                                                    
// │            Building living worlds through simulation.                                              
// │                                                                                                    
// │         Simple things should be simple. Complex things should be composed from simple things       
// │                                                                                                    
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
//-----------------------------------------------------------------------------//
// File:
//      Version.h
//
// Purpose:
//      Provides compile-time version information for the Living Commonwealth
//      Engine.
//
// Project:
//      Living Commonwealth Engine (LCE)
//
// License:
//      MIT License
//
// SPDX-License-Identifier: MIT
//
// Copyright:
//      (c) 2026-present LCE Contributors
//=============================================================================//

#pragma once

#include <string_view>

namespace LCE::Version
{
    //-------------------------------------------------------------------------
    // Engine Version
    //
    // These values define the current version of the Living Commonwealth
    // Engine. They are compile-time constants and should only be updated when
    // releasing a new version.
    //-------------------------------------------------------------------------

    inline constexpr int MajorValue = 0;
    inline constexpr int MinorValue = 4;
    inline constexpr int PatchValue = 0;

    inline constexpr std::string_view VersionString = "0.4.0-alpha";

    inline constexpr std::string_view EngineName = "Living Commonwealth Engine";

    //-------------------------------------------------------------------------
    // Returns the major version number.
    //-------------------------------------------------------------------------
    [[nodiscard]]
    constexpr int Major() noexcept;

    //-------------------------------------------------------------------------
    // Returns the minor version number.
    //-------------------------------------------------------------------------
    [[nodiscard]]
    constexpr int Minor() noexcept;

    //-------------------------------------------------------------------------
    // Returns the patch version number.
    //-------------------------------------------------------------------------
    [[nodiscard]]
    constexpr int Patch() noexcept;

    //-------------------------------------------------------------------------
    // Returns the complete version string.
    //-------------------------------------------------------------------------
    [[nodiscard]]
    constexpr std::string_view String() noexcept;

    //-------------------------------------------------------------------------
    // Returns the engine name.
    //-------------------------------------------------------------------------
    [[nodiscard]]
    constexpr std::string_view Name() noexcept;
}