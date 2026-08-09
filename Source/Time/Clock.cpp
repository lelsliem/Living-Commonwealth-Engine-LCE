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
// │            “If time travel is possible, where are the tourists from the future?” ― Stephen Hawking
// │                                                                                                    
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
//-----------------------------------------------------------------------------//
//
// File:
//
//      Clock.cpp
//
// Purpose:
//
//      Implements the LCE simulation clock.
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

#include "LCE/Time/Clock.h"

namespace LCE::Time
{
    Clock::Duration Clock::Elapsed() const noexcept
    {
        return std::chrono::duration_cast<Duration>(
            std::chrono::steady_clock::now() - m_StartTime);
    }
}