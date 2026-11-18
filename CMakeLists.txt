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
    Source/ReverbDSP.cpp       Source/ReverbDSP.h)

target_compile_features(VoidChamberReverb PRIVATE cxx_std_17)
target_compile_definitions(VoidChamberReverb PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(VoidChamberReverb
    PRIVATE juce::juce_audio_processors
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

include(CTest)
if(BUILD_TESTING)
    add_executable(ReverbDSPTests Tests/ReverbDSPTests.cpp Source/ReverbDSP.cpp)
    target_include_directories(ReverbDSPTests PRIVATE Source)
    target_compile_features(ReverbDSPTests PRIVATE cxx_std_17)
    target_compile_definitions(ReverbDSPTests PRIVATE JUCE_USE_CURL=0)
    target_link_libraries(ReverbDSPTests PRIVATE juce::juce_audio_basics)
    add_test(NAME ReverbImpulseResponse COMMAND ReverbDSPTests)
endif()
