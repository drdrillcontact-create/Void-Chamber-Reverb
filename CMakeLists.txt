cmake_minimum_required(VERSION 3.16)
project(VOID_CHAMBER_REVERB LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------
# JUCE plugin metadata
# ------------------------------------------------------------
set(JUCE_PLUGIN_NAME "Void Chamber Reverb")
set(JUCE_PLUGIN_DESCRIPTION "Spacey Stereo Algorithmic Reverb")
set(JUCE_PLUGIN_MANUFACTURER "D.A Production")
set(JUCE_PLUGIN_VERSION "1.0.0")
set(JUCE_PLUGIN_CODE "VCRB")
set(JUCE_PLUGIN_CODE2 "VCRB")
set(JUCE_PLUGIN_CHANNEL_CONFIGS "1:1 2:2")
set(JUCE_PLUGIN_METADATA_URL "https://github.com/your-org/void-chamber-reverb")

# Enable both VST3 and AudioUnit (per-platform targets handle the rest)
set(JUCE_PLUGIN_FORMATS "VST3;AudioUnit" CACHE STRING "Plugin formats to build")

# ------------------------------------------------------------
# JUCE setup
# ------------------------------------------------------------
# The user must point JUCE_ROOT to a local JUCE checkout or install.
# Example: -DJUCE_ROOT=/path/to/JUCE
if(NOT DEFINED JUCE_ROOT)
    set(JUCE_ROOT "$ENV{JUCE_ROOT}" CACHE PATH "Path to JUCE framework")
endif()

if(NOT JUCE_ROOT)
    message(FATAL_ERROR "JUCE_ROOT is not defined. Set -DJUCE_ROOT=/path/to/JUCE or export JUCE_ROOT env var.")
endif()

set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_TOOLS OFF CACHE BOOL "" FORCE)

list(APPEND CMAKE_MODULE_PATH "${JUCE_ROOT}/extras/CMake/modules")
find_package(JUCE REQUIRED CONFIG)

# ------------------------------------------------------------
# Target
# ------------------------------------------------------------
juce_add_plugin(MyPlugin
    SOURCES
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/ReverbDSP.cpp
        Source/ReverbDSP.h
)

target_include_directories(MyPlugin PRIVATE
    Source
)

# Per-platform output tweaks
if(WINDOWS)
    # VST3 is the primary target on Windows
    set_target_properties(MyPlugin PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
elseif(APPLE)
    # AudioUnit is the primary target on macOS
    set_target_properties(MyPlugin PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.daproduction.voidchamberreverb"
    )
endif()
