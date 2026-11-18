cmake_minimum_required(VERSION 3.22)
project(RelinkSmartSidechain VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(RelinkSmartSidechain
    COMPANY_NAME Relink
    BUNDLE_ID com.relink.smartsidechain
    PLUGIN_MANUFACTURER_CODE Rlnk
    PLUGIN_CODE Rssc
    FORMATS VST3
    PRODUCT_NAME "RELINK Smart Sidechain"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(RelinkSmartSidechain PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
juce_generate_juce_header(RelinkSmartSidechain)
target_compile_features(RelinkSmartSidechain PRIVATE cxx_std_17)
target_compile_definitions(RelinkSmartSidechain PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(RelinkSmartSidechain PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
