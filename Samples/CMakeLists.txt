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
