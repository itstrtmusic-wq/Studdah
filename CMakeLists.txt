cmake_minimum_required(VERSION 3.22)
project(Studdah VERSION 0.8.0 LANGUAGES C CXX)
set(JUCE_PATH "" CACHE PATH "Path to the JUCE source tree")
if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt" OR NOT EXISTS "${JUCE_PATH}/modules/juce_audio_processors")
    message(FATAL_ERROR "Set JUCE_PATH to a complete JUCE source tree.")
endif()
add_subdirectory("${JUCE_PATH}" JUCE)
juce_add_plugin(Studdah
    COMPANY_NAME "TherealTadoe Productions"
    PLUGIN_MANUFACTURER_CODE Tado
    PLUGIN_CODE Stdh
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    FORMATS VST3 Standalone
    PRODUCT_NAME "Studdah")
target_sources(Studdah PRIVATE Source/PluginProcessor.cpp)
target_compile_features(Studdah PRIVATE cxx_std_20)
target_compile_definitions(Studdah PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Studdah PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

include(CTest)
if(BUILD_TESTING)
    add_executable(StuddahBufferTests Tests/CircularAudioBufferTests.cpp)
    target_compile_features(StuddahBufferTests PRIVATE cxx_std_20)
    add_test(NAME CircularAudioBuffer COMMAND StuddahBufferTests)
    add_executable(StuddahTempoTests Tests/TempoClockTests.cpp)
    target_compile_features(StuddahTempoTests PRIVATE cxx_std_20)
    add_test(NAME TempoClock COMMAND StuddahTempoTests)
    add_executable(StuddahStutterTests Tests/StutterEngineTests.cpp)
    target_compile_features(StuddahStutterTests PRIVATE cxx_std_20)
    add_test(NAME StutterEngine COMMAND StuddahStutterTests)
    add_executable(StuddahReverseReverbTests Tests/ReverseReverbTests.cpp)
    target_compile_features(StuddahReverseReverbTests PRIVATE cxx_std_20)
    add_test(NAME ReverseReverb COMMAND StuddahReverseReverbTests)
    add_executable(StuddahPatternTests Tests/PatternSequencerTests.cpp)
    target_compile_features(StuddahPatternTests PRIVATE cxx_std_20)
    add_test(NAME PatternSequencer COMMAND StuddahPatternTests)
    add_executable(StuddahWaveformTests Tests/WaveformHistoryTests.cpp)
    target_compile_features(StuddahWaveformTests PRIVATE cxx_std_20)
    add_test(NAME WaveformHistory COMMAND StuddahWaveformTests)
endif()
