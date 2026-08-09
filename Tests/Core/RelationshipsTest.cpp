//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      RelationshipsTest.cpp
//
// Purpose:
//
//      Verifies memories shape feelings — fair trade earns trust, wrongs
//      sour disposition — and that feelings drift toward neutral.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Simulation/Simulation.h"

namespace LCE::Tests
{
    bool RelationshipsTest()
    {
        Simulation::EntityRegistry registry;

        const auto farmer = registry.CreateEntity();
        const auto merchant = registry.CreateEntity();
        const auto stranger = registry.CreateEntity();

        // Two fair trades earn trust.
        Simulation::Remember(
            registry, farmer, { merchant, Simulation::InteractionKind::Trade, 1.0f });
        Simulation::Remember(
            registry, farmer, { merchant, Simulation::InteractionKind::Trade, 1.0f });

        // Being wronged sinks disposition.
        Simulation::Remember(
            registry, farmer, { stranger, Simulation::InteractionKind::Wronged, 1.0f });

        const auto relationships =
            registry.GetComponent<Simulation::Relationships>(farmer);

        if (!relationships)
        {
            return false;
        }

        const auto& toMerchant = relationships->ByEntity.at(merchant);
        const auto& toStranger = relationships->ByEntity.at(stranger);

        if (toMerchant.Trust < 0.29f || toMerchant.Trust > 0.31f)
        {
            return false;   // two trades x 0.15
        }

        if (toStranger.Disposition > -0.24f)
        {
            return false;   // one wrong x -0.25
        }

        // Feelings drift toward neutral over time.
        Simulation::Update(registry, 1.0);

        if (relationships->ByEntity.at(merchant).Trust >= 0.30f)
        {
            return false;
        }

        if (relationships->ByEntity.at(stranger).Disposition <= -0.25f)
        {
            return false;
        }

        return true;
    }
}
