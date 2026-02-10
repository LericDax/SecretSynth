cmake_minimum_required(VERSION 3.24)

include(cmake/PluginIdentity.cmake)

project(${SECRET_SYNTH_PLUGIN_NAME} VERSION ${SECRET_SYNTH_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

if(APPLE)
    set(SECRET_SYNTH_PLUGIN_FORMATS VST3 AU)
else()
    set(SECRET_SYNTH_PLUGIN_FORMATS VST3)
endif()

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME ${SECRET_SYNTH_MANUFACTURER_NAME}
    BUNDLE_ID ${SECRET_SYNTH_BUNDLE_ID}
    PLUGIN_MANUFACTURER_CODE ${SECRET_SYNTH_MANUFACTURER_CODE}
    PLUGIN_CODE ${SECRET_SYNTH_PLUGIN_CODE}
    FORMATS ${SECRET_SYNTH_PLUGIN_FORMATS}
    PRODUCT_NAME ${SECRET_SYNTH_PLUGIN_NAME}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    JUCE_VST3_CAN_REPLACE_VST2 0
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
)

juce_generate_juce_header(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
        src/dsp/osc/PhaseWarpOscillator.cpp
        src/dsp/osc/PhaseWarpOscillator.h
        src/dsp/SimpleVoice.cpp
        src/dsp/SimpleVoice.h
        src/dsp/voice/Voice.cpp
        src/dsp/voice/Voice.h
        src/dsp/voice/VoiceManager.cpp
        src/dsp/voice/VoiceManager.h
        src/plugin/PluginEditor.cpp
        src/plugin/PluginEditor.h
        src/plugin/PluginProcessor.cpp
        src/plugin/PluginProcessor.h
        src/ui/MainEditorComponent.cpp
        src/ui/MainEditorComponent.h
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

enable_testing()

add_executable(secretsynth_dsp_tests
    tests/test_simple_voice.cpp
    tests/dsp/test_phase_warp_oscillator.cpp
    src/dsp/osc/PhaseWarpOscillator.cpp
    src/dsp/osc/PhaseWarpOscillator.h
    src/dsp/SimpleVoice.cpp
    src/dsp/SimpleVoice.h
)

target_compile_features(secretsynth_dsp_tests PRIVATE cxx_std_20)
add_test(NAME secretsynth_dsp_tests COMMAND secretsynth_dsp_tests)

add_executable(secretsynth_osc_benchmark
    tools/osc_quality_benchmark.cpp
    src/dsp/osc/PhaseWarpOscillator.cpp
    src/dsp/osc/PhaseWarpOscillator.h
)

target_compile_features(secretsynth_osc_benchmark PRIVATE cxx_std_20)

add_executable(secretsynth_voice_tests
    tests/dsp/test_voice_manager.cpp
    src/dsp/voice/Voice.cpp
    src/dsp/voice/Voice.h
    src/dsp/voice/VoiceManager.cpp
    src/dsp/voice/VoiceManager.h
)

target_compile_features(secretsynth_voice_tests PRIVATE cxx_std_20)
add_test(NAME secretsynth_voice_tests COMMAND secretsynth_voice_tests)
