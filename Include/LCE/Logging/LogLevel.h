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
// │          “99 little bugs in the code — and they’re all mine.”
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
    //-------------------------------------------------------------------------
    // The severity levels supported by the LCE logging system.
    //-------------------------------------------------------------------------
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
