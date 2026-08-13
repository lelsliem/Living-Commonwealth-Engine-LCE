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
// │          “Good software speaks. Great software explains.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
//-----------------------------------------------------------------------------//
// File:
//      Logger.h
//
// Purpose:
//      Defines the LCE logging interface — the severity levels, the
//      write entry point, and the convenience functions.
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
// Includes
//=============================================================================//

#include "LCE/Logging/LogLevel.h"

#include <string_view>

namespace LCE::Logging
{
    //-------------------------------------------------------------------------
    // Initializes the LCE logging system.
    //
    // This function prepares the underlying logging backend and should be
    // called once during engine startup before any log messages are written.
    //-------------------------------------------------------------------------
    void Initialize() noexcept;

    //-------------------------------------------------------------------------
    // Shuts down the logging system.
    //
    // Ensures any pending log messages are written before releasing resources.
    //-------------------------------------------------------------------------
    void Shutdown() noexcept;

    //-------------------------------------------------------------------------
    // Immediately writes any buffered log messages to their destination.
    //
    // Normally this is only required during shutdown or debugging.
    //-------------------------------------------------------------------------
    void Flush() noexcept;

    //-------------------------------------------------------------------------
    // Writes a message using the specified log level.
    //
    // This is the core logging function used internally by the convenience
    // functions below.
    //-------------------------------------------------------------------------
    void Write(
        LogLevel level,
        std::string_view message) noexcept;

    //-------------------------------------------------------------------------
    // Writes a trace message.
    //
    // Trace messages are intended for extremely detailed diagnostic output.
    //-------------------------------------------------------------------------
    void Trace(std::string_view message) noexcept;

    //-------------------------------------------------------------------------
    // Writes a debug message.
    //
    // Debug messages assist developers during development and testing.
    //-------------------------------------------------------------------------
    void Debug(std::string_view message) noexcept;

    //-------------------------------------------------------------------------
    // Writes an informational message.
    //
    // Information messages describe normal engine operation.
    //-------------------------------------------------------------------------
    void Info(std::string_view message) noexcept;

    //-------------------------------------------------------------------------
    // Writes a warning message.
    //
    // Warning messages indicate an unexpected situation that does not prevent
    // the engine from continuing.
    //-------------------------------------------------------------------------
    void Warning(std::string_view message) noexcept;

    //-------------------------------------------------------------------------
    // Writes an error message.
    //
    // Error messages indicate a failure that may affect functionality but does
    // not necessarily require the engine to terminate.
    //-------------------------------------------------------------------------
    void Error(std::string_view message) noexcept;

    //-------------------------------------------------------------------------
    // Writes a critical message.
    //
    // Critical messages indicate unrecoverable errors that require immediate
    // developer attention.
    //-------------------------------------------------------------------------
    void Critical(std::string_view message) noexcept;
}
