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
// │     (this line is reserved for a joke or quote from the author)
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      RegistrySnapshot.h
//
// Purpose:
//
//      Defines the snapshot format: a pure-data capture of every live
//      entity and its components, so the simulation can ride inside a
//      game's save file (co-save, 0.4.0).
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

#pragma once

#include "LCE/Simulation/EntityId.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <typeindex>
#include <vector>

namespace LCE::Simulation
{
    //-------------------------------------------------------------------------
    // The version of the core's snapshot schema. The adapter layers its
    // OWN versioning on top of this when writing the durable co-save
    // record — save-compatibility is the adapter's job (it names the
    // component types, it migrates old saves).
    //-------------------------------------------------------------------------
    inline constexpr std::uint32_t kSnapshotVersion = 1;

    //-------------------------------------------------------------------------
    // ComponentBlob
    //
    // One component instance as raw bytes. The core never interprets these
    // bytes — only the adapter's serializer does. A snapshot is a pure
    // data exchange; no game knowledge crosses it.
    //-------------------------------------------------------------------------
    using ComponentBlob = std::vector<std::byte>;

    //-------------------------------------------------------------------------
    // ComponentSerializer<T>
    //
    // The adapter registers one of these per component type it wants
    // persisted. Compile-time type in; runtime blob out — and back again.
    // This is type erasure applied a third time: the registry handles any
    // component without naming it, while the caller's serializer gives the
    // bytes meaning.
    //-------------------------------------------------------------------------
    template <typename T>
    struct ComponentSerializer
    {
        std::function<ComponentBlob(const T&)> Serialize;
        std::function<T(const ComponentBlob&)> Deserialize;
    };

    //-------------------------------------------------------------------------
    // SnapshotComponent
    //
    // One component of one entity. Type is the store's key — it identifies
    // the component kind so Restore can find the right store. Data is the
    // opaque bytes.
    //-------------------------------------------------------------------------
    struct SnapshotComponent
    {
        std::type_index Type;
        ComponentBlob Data;
    };

    //-------------------------------------------------------------------------
    // SnapshotEntity
    //
    // One live entity and everything it owned at capture time. The ID is
    // preserved exactly (index + generation) so the game's mapping to the
    // entity survives a save/load.
    //-------------------------------------------------------------------------
    struct SnapshotEntity
    {
        EntityId Id;
        std::vector<SnapshotComponent> Components;
    };

    //-------------------------------------------------------------------------
    // RegistrySnapshot
    //
    // The whole registry, as data. A process-local exchange format: the
    // adapter translates it into the game's save record (stable type
    // names, its own versioning) and back.
    //-------------------------------------------------------------------------
    struct RegistrySnapshot
    {
        std::uint32_t Version = kSnapshotVersion;
        std::vector<SnapshotEntity> Entities;
    };
}
