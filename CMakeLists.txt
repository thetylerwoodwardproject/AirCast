cmake_minimum_required(VERSION 3.22)

project(AirCast VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AirCast
    COMPANY_NAME                "Fully Modulated"
    BUNDLE_ID                   "com.fullymodulated.aircast"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    PLUGIN_MANUFACTURER_CODE    Fmdl
    PLUGIN_CODE                 Acst
    FORMATS                     VST3 Standalone AU
    PRODUCT_NAME                "AirCast"
)

juce_generate_juce_header(AirCast)

target_sources(AirCast
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(AirCast
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(AirCast
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Diagnostic-only console app: exercises the DSP classes directly (no plugin
# wrapper) to check metering getters. Not shipped, not linked into the plugin.
juce_add_console_app(DspProbe PRODUCT_NAME "DspProbe")
target_sources(DspProbe PRIVATE Tools/DspProbe.cpp)
target_link_libraries(DspProbe PRIVATE juce::juce_dsp juce::juce_core)
