cmake_minimum_required(VERSION 3.22)
project(VoidChamberReverb VERSION 1.0.0 LANGUAGES C CXX)

# Use the supplied source tree, not an unrelated system package or a network fetch.
set(JUCE_ROOT "$ENV{JUCE_ROOT}" CACHE PATH "Path to the JUCE source tree")
if(NOT JUCE_ROOT OR NOT EXISTS "${JUCE_ROOT}/CMakeLists.txt"
                 OR NOT EXISTS "${JUCE_ROOT}/modules/juce_core/juce_core.h")
    message(FATAL_ERROR "Set JUCE_ROOT (environment or -DJUCE_ROOT=/path/to/JUCE) "
                        "to a JUCE source tree containing CMakeLists.txt and modules/.")
endif()
add_subdirectory("${JUCE_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)

# Reject truncated artwork/font downloads at configure time, not at native paint.
file(SIZE "${CMAKE_CURRENT_SOURCE_DIR}/Assets/fonts/CormorantGaramond.ttf" title_font_bytes)
if(title_font_bytes LESS 1000)
    message(FATAL_ERROR "Embedded title font is missing or truncated")
endif()

# Binary assets (background scene and the five Space Type orbs).
juce_add_binary_data(VoidChamberAssets
    SOURCES
        Assets/titanium-knob-body.png
        Assets/void-background.jpg
        Assets/space-void.jpg
        Assets/space-hall.jpg
        Assets/space-plate.jpg
        Assets/space-cavern.jpg
        Assets/space-ambient.jpg
        Assets/fonts/CormorantGaramond.ttf
        Assets/fonts/Raleway-200.ttf
        Assets/fonts/Raleway-300.ttf
        Assets/fonts/Raleway-400.ttf
        Assets/fonts/Raleway-500.ttf
        Assets/fonts/Raleway-600.ttf
        Assets/fonts/Raleway-700.ttf)

juce_add_plugin(VoidChamberReverb
    PRODUCT_NAME "Void Chamber Reverb"
    DESCRIPTION "Spacey Stereo Algorithmic Reverb"
    COMPANY_NAME "D.A Production"
    BUNDLE_ID com.daproduction.voidchamberreverb
    PLUGIN_MANUFACTURER_CODE Dapr
    PLUGIN_CODE Vcrb
    FORMATS VST3 AU
    VST3_CATEGORIES Fx Reverb
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(VoidChamberReverb PRIVATE
    Source/PluginProcessor.cpp Source/PluginProcessor.h
    Source/PluginEditor.cpp    Source/PluginEditor.h
    Source/ReverbDSP.cpp       Source/ReverbDSP.h
    Source/VoidGUI.h)

target_compile_features(VoidChamberReverb PRIVATE cxx_std_17)
target_compile_definitions(VoidChamberReverb PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(VoidChamberReverb
    PRIVATE juce::juce_audio_processors VoidChamberAssets
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

include(CTest)
if(BUILD_TESTING)
    add_executable(ReverbDSPTests Tests/ReverbDSPTests.cpp Source/ReverbDSP.cpp)
    target_include_directories(ReverbDSPTests PRIVATE Source)
    target_compile_features(ReverbDSPTests PRIVATE cxx_std_17)
    target_compile_definitions(ReverbDSPTests PRIVATE JUCE_USE_CURL=0)
    target_link_libraries(ReverbDSPTests PRIVATE juce::juce_audio_basics)
    add_test(NAME ReverbImpulseResponse COMMAND ReverbDSPTests)
    add_test(NAME EditorSnapshot COMMAND EditorSnapshotTest "${CMAKE_CURRENT_BINARY_DIR}/editor-snapshot.png")

    # Headless render of the full editor so the layout can be checked visually.
    add_executable(EditorSnapshotTest
        Tests/EditorSnapshotTest.cpp
        Source/PluginProcessor.cpp Source/PluginProcessor.h
        Source/PluginEditor.cpp    Source/PluginEditor.h
        Source/ReverbDSP.cpp       Source/ReverbDSP.h
        Source/VoidGUI.h)
    target_include_directories(EditorSnapshotTest PRIVATE Source)
    target_compile_features(EditorSnapshotTest PRIVATE cxx_std_17)
    target_compile_definitions(EditorSnapshotTest PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
        JucePlugin_Name="Void Chamber Reverb"
        JucePlugin_IsMidiEffect=0
        JucePlugin_IsSynth=0)
    target_link_libraries(EditorSnapshotTest PRIVATE
        juce::juce_audio_processors juce::juce_gui_basics VoidChamberAssets
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
