cmake_minimum_required(VERSION 3.24)

project(LUMIDANCER
    VERSION 0.2.0
    DESCRIPTION "LUMI//DANCER - Audio-reactive dancing mascot for Ableton Live"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ------------------------------------------------------------------ options
option(LUMI_BUILD_PLUGIN     "Build the JUCE plugin targets (fetches JUCE)" ON)
option(LUMI_BUILD_VST3       "Build the VST3 plugin"                 ON)
option(LUMI_BUILD_STANDALONE "Build the standalone application"      ON)
option(LUMI_BUILD_TESTS      "Build the JUCE-linked test target"     ON)
option(LUMI_BUILD_CORE_TESTS "Build the JUCE-free core test target (fast)" ON)
option(LUMI_ENABLE_ASAN      "Enable AddressSanitizer"               OFF)
option(LUMI_ENABLE_UBSAN     "Enable UndefinedBehaviorSanitizer"     OFF)
option(LUMI_ENABLE_LTO       "Enable link-time optimisation"         OFF)
option(LUMI_COPY_PLUGIN      "Copy plugin binaries into user plugin folders after build" OFF)

# ------------------------------------------------------------- sanitizers fn
function(ld_apply_sanitizers target)
    if(MSVC)
        if(LUMI_ENABLE_ASAN)
            target_compile_options(${target} PRIVATE /fsanitize=address)
        endif()
        # MSVC has no UBSan; documented in docs/testing.md.
    else()
        set(_san "")
        if(LUMI_ENABLE_ASAN)
            list(APPEND _san address)
        endif()
        if(LUMI_ENABLE_UBSAN)
            list(APPEND _san undefined)
        endif()
        if(_san)
            list(JOIN _san "," _sanArg)
            target_compile_options(${target} PRIVATE -fsanitize=${_sanArg} -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=${_sanArg})
        endif()
    endif()
endfunction()

# =====================================================================
#  JUCE-FREE CORE LIBRARY (the product "brain": audio analysis, host
#  beat clock, character rig maths, dance styles, expressions, idle
#  behaviour, constellation choreography, routines, particles, state
#  serialisation, deterministic randomness). Depends only on the C++20
#  standard library so it builds and unit-tests in seconds.
# =====================================================================
file(GLOB_RECURSE LD_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"          "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/analysis/*.cpp"      "${CMAKE_CURRENT_SOURCE_DIR}/src/analysis/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/timing/*.cpp"        "${CMAKE_CURRENT_SOURCE_DIR}/src/timing/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rig/*.cpp"           "${CMAKE_CURRENT_SOURCE_DIR}/src/rig/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dance/*.cpp"         "${CMAKE_CURRENT_SOURCE_DIR}/src/dance/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/constellation/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/constellation/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/fx/*.cpp"            "${CMAKE_CURRENT_SOURCE_DIR}/src/fx/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/state/*.cpp"         "${CMAKE_CURRENT_SOURCE_DIR}/src/state/*.h")

add_library(lumi_core STATIC ${LD_CORE_SOURCES})
target_include_directories(lumi_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(lumi_core PUBLIC cxx_std_20)
target_compile_definitions(lumi_core PUBLIC LUMI_VERSION_STRING="${PROJECT_VERSION}")
if(MSVC)
    target_compile_options(lumi_core PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(lumi_core PRIVATE -Wall -Wextra -Wshadow)
endif()
ld_apply_sanitizers(lumi_core)

# ------------------------------------------------ JUCE-free core test target
if(LUMI_BUILD_CORE_TESTS)
    enable_testing()
    file(GLOB_RECURSE LD_CORETEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/core/*.h")
    add_executable(lumi_core_tests ${LD_CORETEST_SOURCES})
    target_link_libraries(lumi_core_tests PRIVATE lumi_core)
    target_include_directories(lumi_core_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/core")
    ld_apply_sanitizers(lumi_core_tests)
    add_test(NAME LumiCoreTests COMMAND lumi_core_tests)
    set_tests_properties(LumiCoreTests PROPERTIES TIMEOUT 900)
endif()

# =====================================================================
#  JUCE PLUGIN TARGETS (only fetch JUCE when actually needed)
# =====================================================================
set(LD_NEED_JUCE OFF)
if(LUMI_BUILD_PLUGIN OR LUMI_BUILD_TESTS)
    set(LD_NEED_JUCE ON)
endif()

if(LD_NEED_JUCE)
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)

    # JUCE 8 (pinned). Licence: AGPLv3 (open-source tier) / commercial.
    FetchContent_Declare(JUCE
        URL https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.8.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(JUCE)

    file(GLOB_RECURSE LD_JUCE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp"     "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.h")

    set(LD_COMMON_DEFS
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=1        # required by the JUCE AGPL/open-source tier
        LUMI_VERSION_STRING="${PROJECT_VERSION}")

    # Embedded painted sprite frames (original project artwork).
    file(GLOB LD_SPRITE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/sprites/*.png")
    juce_add_binary_data(LumiSpriteData
        NAMESPACE LumiSprites
        HEADER_NAME LumiSpritesData.h
        SOURCES ${LD_SPRITE_FILES})
    set_target_properties(LumiSpriteData PROPERTIES POSITION_INDEPENDENT_CODE ON)

    set(LD_JUCE_MODULES
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra)
endif()

if(LD_NEED_JUCE AND LUMI_BUILD_PLUGIN)
    set(LD_FORMATS "")
    if(LUMI_BUILD_VST3)
        list(APPEND LD_FORMATS VST3)
    endif()
    if(LUMI_BUILD_STANDALONE)
        list(APPEND LD_FORMATS Standalone)
    endif()
    if(LD_FORMATS STREQUAL "")
        message(FATAL_ERROR "No plugin formats enabled.")
    endif()

    juce_add_plugin(LumiDancer
        VERSION                     ${PROJECT_VERSION}
        COMPANY_NAME                "ENAHKEM Audio"
        BUNDLE_ID                   "audio.enahkem.lumidancer"
        PLUGIN_MANUFACTURER_CODE    ENAK
        PLUGIN_CODE                 LUMI
        FORMATS                     ${LD_FORMATS}
        PRODUCT_NAME                "LUMI DANCER"
        PLUGIN_NAME                 "LUMI//DANCER"
        DESCRIPTION                 "Audio-reactive dancing mascot visualizer"
        IS_SYNTH                    FALSE
        NEEDS_MIDI_INPUT            FALSE
        NEEDS_MIDI_OUTPUT           FALSE
        IS_MIDI_EFFECT              FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD     ${LUMI_COPY_PLUGIN}
        VST3_CATEGORIES             Fx Analyzer
        MICROPHONE_PERMISSION_ENABLED TRUE
        MICROPHONE_PERMISSION_TEXT  "The standalone app listens to audio input so Lumi can dance to it.")

    target_sources(LumiDancer PRIVATE ${LD_JUCE_SOURCES})
    target_include_directories(LumiDancer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(LumiDancer PUBLIC ${LD_COMMON_DEFS} JUCE_MODAL_LOOPS_PERMITTED=0)
    target_link_libraries(LumiDancer
        PRIVATE lumi_core LumiSpriteData ${LD_JUCE_MODULES}
        PUBLIC  juce::juce_recommended_config_flags
                juce::juce_recommended_warning_flags)
    if(LUMI_ENABLE_LTO)
        target_link_libraries(LumiDancer PUBLIC juce::juce_recommended_lto_flags)
    endif()
    ld_apply_sanitizers(LumiDancer)
endif()

# ------------------------------------------------ JUCE-linked test target
if(LD_NEED_JUCE AND LUMI_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(LumiTests PRODUCT_NAME "LumiTests")

    file(GLOB_RECURSE LD_JUCETEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/juce/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/juce/*.h")

    target_sources(LumiTests PRIVATE ${LD_JUCE_SOURCES} ${LD_JUCETEST_SOURCES})
    target_include_directories(LumiTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/juce")
    target_compile_definitions(LumiTests PRIVATE
        ${LD_COMMON_DEFS}
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_STANDALONE_APPLICATION=1
        JucePlugin_Name="LUMI//DANCER"
        LUMI_TEST_BUILD=1)
    target_link_libraries(LumiTests
        PRIVATE lumi_core LumiSpriteData ${LD_JUCE_MODULES}
        PUBLIC  juce::juce_recommended_config_flags
                juce::juce_recommended_warning_flags)
    ld_apply_sanitizers(LumiTests)

    add_test(NAME LumiJuceTests COMMAND LumiTests)
    set_tests_properties(LumiJuceTests PROPERTIES TIMEOUT 1800)
endif()

# ------------------------------------------------------------- installation
# Plugin install locations are documented in docs/ and handled by the
# packaging script under /scripts. Nothing is hardcoded into the plugin.
