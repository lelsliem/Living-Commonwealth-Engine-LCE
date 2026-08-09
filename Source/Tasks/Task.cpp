//=============================================================================//
// ┌─────────────────────────────────────────────────────────────────────────┐ //
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
// │"Every block of stone has a statue inside it and it is the task of the sculptor to discover it." - Michelangelo
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      Task.cpp
//
// Purpose:
//
//      Implements the LCE task contract.
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

#include "LCE/Tasks/Task.h"

#include <utility>

namespace LCE::Tasks
{
    Task::Task(Callback callback)
        : m_Callback(std::move(callback))
    {
    }

    void Task::Execute()
    {
        if (m_Callback)
        {
            m_Callback();
        }
    }
}
