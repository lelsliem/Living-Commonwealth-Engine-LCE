#==============================================================================
#
#  Living Commonwealth Engine (LCE)
#  Building living worlds through simulation.
#
#  SPDX-License-Identifier: MIT
#
#==============================================================================

# Sample applications arrive with the 0.5.0 SDK milestone, built only
# when LCE_BUILD_SAMPLES=ON.
#
# Sample Host — the runnable non-game proof that any engine can embed
# LCE: one farmer, two merchants, the full decide -> act -> observe ->
# remember loop, plus a mid-story save/load. The sample every modder
# reads first; its main.cpp is the course.
add_executable(LCE.SampleHost
    SampleHost/main.cpp
)

target_link_libraries(LCE.SampleHost
    PRIVATE
        LCE.Core
)

target_compile_features(LCE.SampleHost
    PRIVATE
        cxx_std_23
)

# The tuning file ships next to the executable so "run from the build
# Bin" demonstrates the modder's knob from the first launch.
add_custom_command(TARGET LCE.SampleHost POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/SampleHost/host.ini
        $<TARGET_FILE_DIR:LCE.SampleHost>/host.ini
)

# ---------------------------------------------------------------------------
# Sample Modules — the teaching patterns, one idea each. Read them in
# order: Farmer (the atoms), Village (relationships), Market (world
# facts + calendar). Each is a small main.cpp that prints its lesson.
# ---------------------------------------------------------------------------

# SAMPLE 1 — The Farmer: the smallest living loop (one mind, one need,
# one decision, one outcome).
add_executable(LCE.SampleFarmer
    Sample-Farmer/main.cpp
)

# SAMPLE 2 — The Village: many minds and the feelings between them;
# outcomes shape relationships and relationships steer choices.
add_executable(LCE.SampleVillage
    Sample-Village/main.cpp
)

# SAMPLE 3 — The Market: world facts (the door is a fading memory),
# the world calendar, and day-stamped weather memories.
add_executable(LCE.SampleMarket
    Sample-Market/main.cpp
)

# SAMPLE 4 — The Economy (0.8.2): dynamic pricing, supply chains,
# trade routes, and market events — prices are memories with numbers
# on them. Zero new engine surface.
add_executable(LCE.SampleEconomy
    Sample-Economy/main.cpp
)

# SAMPLE 5 — Legacy (0.8.2): death, inheritance, and the name that
# outlives the voice — Bequeath, InheritMemory, LeaveLegacy.
add_executable(LCE.SampleLegacy
    Sample-Legacy/main.cpp
)

# SAMPLE 6 — Weather (0.8.2): seasons on the calendar and weather as
# day-stamped facts that shape needs and decisions.
add_executable(LCE.SampleWeather
    Sample-Weather/main.cpp
)

# SAMPLE 7 — Children (0.8.2): family as a group — birth, inherited
# attitudes, and the traits that make each child their own.
add_executable(LCE.SampleChildren
    Sample-Children/main.cpp
)

foreach(sample LCE.SampleFarmer LCE.SampleVillage LCE.SampleMarket LCE.SampleEconomy LCE.SampleLegacy LCE.SampleWeather LCE.SampleChildren)
    target_link_libraries(${sample}
        PRIVATE
            LCE.Core
    )

    target_compile_features(${sample}
        PRIVATE
            cxx_std_23
    )
endforeach()
