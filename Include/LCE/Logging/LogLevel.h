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
// │          “99 little bugs in the code, 99 little bugs in the code.
// │                 Take one down, patch it around, 127 little bugs in the code…”
// │                                                                                                    
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
//-----------------------------------------------------------------------------//
// File:
//
//      LogLevel.h
//
// Purpose:
//
//      Defines the logging severity levels used by the Living Commonwealth
//      Engine.
//
// Project:
//
//      Living Commonwealth Engine (LCE)
//
// License:
//
//      MIT License
//
// SPDX-License-Identifier: MIT
//
// Copyright:
//
//      (c) 2026-present LCE Contributors
//=============================================================================//
namespace LCE::Logging
{
    /// <summary>
    /// Defines the severity levels supported by the LCE logging system.
    /// </summary>
    enum class LogLevel
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };
}