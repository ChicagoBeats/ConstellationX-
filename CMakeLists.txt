cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)

project(ConstellationXVocal VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(CONSTELLATIONX_BUILD_TESTS "Build the JUCE-free DSP engine unit tests" OFF)
if (CONSTELLATIONX_BUILD_TESTS)
    enable_testing()
    add_executable(ConstellationXEngineTests tests/engine_test.cpp tests/preset_test.cpp
        Source/ConstellationEngine.h Source/ParamTable.h)
    add_test(NAME ConstellationXEngineTests COMMAND ConstellationXEngineTests)
endif()

# JUCE 6.1.6 is pinned to follow the supplied UniverseX project.
# Place JUCE 6.1.6 in ./JUCE, or let FetchContent download it.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 6.1.6
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(ConstellationXVocal
    COMPANY_NAME "Chicago Beats Audio Labs"
    PLUGIN_MANUFACTURER_CODE ChBt
    PLUGIN_CODE CxV1
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS VST3 Standalone
    PRODUCT_NAME "ConstellationX Vocal"
)

target_sources(ConstellationXVocal PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/ConstellationEngine.h
    Source/ParamTable.h
)

target_compile_definitions(ConstellationXVocal PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ConstellationXVocal
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if (MSVC)
    target_compile_options(ConstellationXVocal PRIVATE /MP)
endif()
