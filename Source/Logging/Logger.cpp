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
// │          "I have not failed. I've just found 10,000 ways that won't work." — Thomas Edison
// │                                                                                                    
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
//-----------------------------------------------------------------------------//
// File:
//
//      Logger.cpp
//
// Purpose:
//
//      Implements the Living Commonwealth Engine logging system.
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
//-----------------------------------------------------------------------------
// Implementation Notes
//
// Logger.cpp intentionally contains all interaction with the third-party
// logging library.
//
// The rest of LCE communicates exclusively through the public functions
// declared in Logger.h.
//
// This separation keeps third-party dependencies isolated and allows the
// logging backend to be replaced without affecting the public API.
//-----------------------------------------------------------------------------
//=============================================================================//
// Third-Party Dependency
//
// Library:
//
//      spdlog
//
// Purpose:
//
//      Provides the underlying logging implementation.
//
// Why LCE Wraps It:
//
//      LCE exposes its own logging API. The implementation is intentionally
//      hidden so the backend can be replaced without changing the public SDK.
//
// Official Repository:
//
//      https://github.com/gabime/spdlog
//=============================================================================//

//=============================================================================//
// Includes
//=============================================================================//
#include "LCE/Logging/Logger.h"

#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

//=============================================================================//
// Private Data
//=============================================================================//

namespace
{
    /// <summary>
    /// The single logger instance used by the Living Commonwealth Engine.
    /// This object is private to this source file.
    /// </summary>
    std::shared_ptr<spdlog::logger> Logger;
}

//=============================================================================//
// Public Functions
//=============================================================================//

namespace LCE::Logging
{
    void Initialize() noexcept
    {
        // Create a coloured console logger.
        Logger = spdlog::stdout_color_mt("LCE");

        // Enable all log levels during early development.
        Logger->set_level(spdlog::level::trace);

        // Keep the output clean and easy to read.
        Logger->set_pattern("[%^%l%$] %v");

        Info("Living Commonwealth Engine initialized.");
    }

    void Shutdown() noexcept
    {
        Flush();

        Logger.reset();
        spdlog::shutdown();       // Release all logging resources owned by the backend.
    }

    void Flush() noexcept
    {
        if (Logger)
        {
            Logger->flush();
        }
    }

    // The level filter is set once in Initialize (all levels during
    // development) and is deliberately NOT configuration-driven: the
    // core's console logger is a development tool, and the adapter owns
    // its own verbosity through its own logging. ADR-0014 keeps this
    // stateless — no reach into a global Configuration.
    void Write(
        LogLevel level,
        std::string_view message) noexcept
    {
        if (!Logger)
        {
            return;
        }

        switch (level)
        {
        case LogLevel::Trace:
            Logger->trace(message);
            break;

        case LogLevel::Debug:
            Logger->debug(message);
            break;

        case LogLevel::Info:
            Logger->info(message);
            break;

        case LogLevel::Warning:
            Logger->warn(message);
            break;

        case LogLevel::Error:
            Logger->error(message);
            break;

        case LogLevel::Critical:
            Logger->critical(message);
            break;
        }
    }

    void Trace(std::string_view message) noexcept
    {
        Write(LogLevel::Trace, message);
    }

    void Debug(std::string_view message) noexcept
    {
        Write(LogLevel::Debug, message);
    }

    void Info(std::string_view message) noexcept
    {
        Write(LogLevel::Info, message);
    }

    void Warning(std::string_view message) noexcept
    {
        Write(LogLevel::Warning, message);
    }

    void Error(std::string_view message) noexcept
    {
        Write(LogLevel::Error, message);
    }

    void Critical(std::string_view message) noexcept
    {
        Write(LogLevel::Critical, message);
    }
}