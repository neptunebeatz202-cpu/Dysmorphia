cmake_minimum_required(VERSION 3.22)

project(Dysmorphia VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── JUCE ─────────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(JUCE)

# ─── Plugin target ────────────────────────────────────────────────────────────
# VST (legacy 2) requires a valid VST2 SDK path. If you don't have it, remove
# "VST" from FORMATS and the plugin will build VST3 + AU only.
juce_add_plugin(Dysmorphia
    COMPANY_NAME                "kyoa"
    PLUGIN_MANUFACTURER_CODE    "KYOA"
    PLUGIN_CODE                 "Dysm"
    FORMATS                     VST3 AU
    PRODUCT_NAME                "Dysmorphia"
    VERSION                     "1.0.0"
    BUNDLE_ID                   "com.kyoa.dysmorphia"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             "Fx"
    AU_MAIN_TYPE                "kAudioUnitType_Effect"
    DESCRIPTION                 "Granular cloud chorus with spectral split and entropy engine"
)

# ─── Sources ──────────────────────────────────────────────────────────────────
target_sources(Dysmorphia
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

target_include_directories(Dysmorphia PRIVATE Source)

# ─── Binary resources ─────────────────────────────────────────────────────────
# Place saya.jpg in the Resources/ folder before building.
# The CMake conditional below embeds it at compile time via BinaryData.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/saya.jpg")
    juce_add_binary_data(DysmorphiaBinaryData
        NAMESPACE   BinaryData
        SOURCES     Resources/saya.jpg
    )
    target_link_libraries(Dysmorphia PRIVATE DysmorphiaBinaryData)
    target_compile_definitions(Dysmorphia PRIVATE DYSMORPHIA_HAS_BINARY_DATA=1)
    message(STATUS "Dysmorphia: saya.jpg background will be embedded.")
else()
    message(WARNING
        "Dysmorphia: Resources/saya.jpg not found — building without "
        "embedded background. Copy saya.jpg to Resources/ and rebuild.")
endif()

# ─── Compile definitions ──────────────────────────────────────────────────────
target_compile_definitions(Dysmorphia
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
    PRIVATE
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

# ─── Link libraries ───────────────────────────────────────────────────────────
target_link_libraries(Dysmorphia
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
