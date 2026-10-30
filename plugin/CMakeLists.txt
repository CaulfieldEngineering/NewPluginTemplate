cmake_minimum_required(VERSION 3.22)

# Version is needed by JUCE.
# =====================================================================
project(${wPLUGIN_NAME} VERSION ${wPROJECT_VERSION})

# Force the use of the static runtime (/MT or /MTd) for MSVC
# =====================================================================
# This MUST be set BEFORE juce_add_plugin() to ensure JUCE targets also use
# static runtime. Using CMAKE_MSVC_RUNTIME_LIBRARY is the modern CMake way
# (3.15+) and is more reliable than string replacement.
# Static runtime means the plugin won't need VC++ Redistributable installed
# on the user's machine - everything needed is bundled into the .exe/.dll
if (MSVC)
    # Set static runtime library for all targets created after this point
    # MultiThreaded = /MT (Release), MultiThreadedDebug = /MTd (Debug)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    message(STATUS "MSVC: Using static runtime library (/MT) for all targets")
endif()

# Adding a directory with the library/application name as a subfolder of the
# include folder is a good practice. It helps avoid name clashes later on.
# =====================================================================
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/source")

# [Claude 2026-10-29] Copy the built plugin into the system plugin folders (as before). The local test runner
# (plugin/tests/run_all.ps1) and the CI test jobs turn it off: on Windows that copy needs administrator rights.
# =====================================================================
option(WLA_COPY_PLUGIN_AFTER_BUILD "Copy the plugin to the system plugin folders after building" ON)

# Adds a plugin target (that's basically what the Projucer does).
# =====================================================================
juce_add_plugin( ${PROJECT_NAME}
    COMPANY_NAME "${wCOMPANY_NAME}"
    BUNDLE_ID "com.${wCOMPANY_ID}.${wPLUGIN_NAME}"  # Valid bundle identifier without spaces
    IS_SYNTH ${wIS_SYNTH}
    NEEDS_MIDI_INPUT ${wNEEDS_MIDI_INPUT}
    NEEDS_MIDI_OUTPUT ${wNEEDS_MIDI_OUTPUT}
    PLUGIN_MANUFACTURER_CODE ${wPLUGIN_MANUFACTURER_CODE}
    PLUGIN_CODE ${wPLUGIN_CODE}
    FORMATS ${wFORMATS}
    PRODUCT_NAME "${wPLUGIN_NAME}"
    VST3_CATEGORIES ${wPLUGIN_CATEGORY}
    COPY_PLUGIN_AFTER_BUILD ${WLA_COPY_PLUGIN_AFTER_BUILD}  # [Claude 2026-10-29] was TRUE; still on by default
)

# DAW Launch Targets
# =====================================================================
# These create lightweight targets in the VS solution that build the VST3
# and then launch a DAW. Set one as the Startup Project and hit F5.
# To add a new DAW, just add another entry to the list below.
# =====================================================================
if (MSVC)
    file(WRITE "${CMAKE_BINARY_DIR}/launcher_stub.cpp" "int main() { return 0; }\n")

    set(DAW_TARGETS
        "Launch_Reaper|C:/Program Files/REAPER (x64)/reaper.exe"
        "Launch_Cubase|C:/Program Files/Steinberg/Cubase 12/Cubase12.exe"
        "Launch_Cantabile|C:/Program Files/Topten Software/Cantabile 4.0/Cantabile.exe"
    )

    foreach(DAW_ENTRY ${DAW_TARGETS})
        string(REPLACE "|" ";" DAW_PARTS "${DAW_ENTRY}")
        list(GET DAW_PARTS 0 DAW_NAME)
        list(GET DAW_PARTS 1 DAW_EXE)

        if (EXISTS "${DAW_EXE}")
            add_executable(${DAW_NAME} "${CMAKE_BINARY_DIR}/launcher_stub.cpp")
            add_dependencies(${DAW_NAME} ${PROJECT_NAME}_VST3)
            set_target_properties(${DAW_NAME} PROPERTIES
                VS_DEBUGGER_COMMAND "${DAW_EXE}"
                FOLDER "Launch Targets"
            )
            message(STATUS "Launch target: ${DAW_NAME} -> ${DAW_EXE}")
        else()
            message(STATUS "Launch target: ${DAW_NAME} -> SKIPPED (not found: ${DAW_EXE})")
        endif()
    endforeach()
endif()

# Explicitly set runtime library for the plugin target (belt and suspenders approach)
# =====================================================================
# This ensures the plugin target definitely uses static runtime, even if
# something tries to override it later
if (MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Pre-Processor Directive for Demo vs. Full Version
# =====================================================================
if (BUILD_DEMO)
    add_definitions(-DDEMO_VERSION)
    # Include demo-specific files, exclude features, etc.
    message(STATUS "Configuring Demo Version")
else()
    # Full version configurations
    message(STATUS "Configuring Full Version")
endif()


# Run the Python script to generate the file list
# =====================================================================
# [Claude 2026-10-29] Find Python instead of assuming "python3": on Windows "python3" can be the Microsoft Store
# stub, which fails (it did on the mini PC, in Tertiary and here). Falls back to python3 if CMake finds none.
find_package(Python3 COMPONENTS Interpreter QUIET)
if (NOT Python3_EXECUTABLE)
    set(Python3_EXECUTABLE python3)
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/createSourceList.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE PYTHON_RUN_RESULT
)

if(NOT PYTHON_RUN_RESULT EQUAL "0")
    message(FATAL_ERROR "Failed to run Python script to generate file list")
endif()

# Include the generated file list
# =====================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/sourceListResult.cmake)


# Console out the value of PLUGIN_SOURCES
# =====================================================================
message(STATUS "Source files for ${PROJECT_NAME}: ${PLUGIN_SOURCES}")

# Sets the source files of the plugin project.
# =====================================================================
target_sources(${PROJECT_NAME}
    PRIVATE
        ${PLUGIN_SOURCES}
)

# Sets the include directories of the plugin project.
# =====================================================================
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# [Claude 2026-10-29] build_id.h (commit, uncommitted changes, time), written before every build, for a hidden
# build number in the plugin window (see PluginEditor.cpp: Ctrl+Shift+click). cmake/build_id.cmake.
# =====================================================================
add_custom_target(wla_build_id ALL
    COMMAND ${CMAKE_COMMAND} -DOUT=${CMAKE_CURRENT_BINARY_DIR}/generated/build_id.h -DSRC=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_id.cmake
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/generated/build_id.h)
add_dependencies(${PROJECT_NAME} wla_build_id)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Links to all necessary dependencies. The present ones are recommended by JUCE.
# If you use one of the additional modules, like the DSP module, you need to specify it here.
# =====================================================================
message(STATUS "Plugin: CMakeLists.txt - target_link_libraries")
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_NAME}BinaryData
        juce::juce_audio_utils
        juce::juce_dsp
        WizardCore::AudioModules   # Shared DSP library — Chorus, LFO, DelayLine, etc.
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# These definitions are recommended by JUCE.
# =====================================================================
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
	JUCE_WIN_PER_MONITOR_DPI_AWARE=1
)

# Enables all warnings and treats warnings as errors.
# =====================================================================
# This needs to be set up only for your projects, not 3rd party
#if (MSVC)
#    target_compile_options(${PROJECT_NAME} PRIVATE /Wall /WX)
#    # Silences some deprecation warnings that (hopefully) will be fixed in a future release of JUCE.
#    target_compile_definitions(${PROJECT_NAME}
#        PRIVATE
#            _SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING)
#else()
#    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
#endif()


# Define and create source groups for JUCE files
# =====================================================================
file(GLOB_RECURSE ALL_JUCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../libs/juce/modules/juce_*")
source_group("JUCE Modules" FILES ${ALL_JUCE_FILES})

# Define and create source groups for WizardCore files
# =====================================================================
# WizardCore_SOURCE_DIR is set by CPM — it resolves to either:
#   - The local repo path (when CPM_WizardCore_SOURCE env var is set)
#   - The cached clone in libs/wizardcore/ (when cloned from GitHub)
# We use this variable so the source groups work regardless of which
# mode CPM is operating in.
# =====================================================================
set(WIZARDCORE_SRC_ROOT "${WizardCore_SOURCE_DIR}/plugin/source")
if(EXISTS "${WIZARDCORE_SRC_ROOT}")
    file(GLOB_RECURSE ALL_WIZARDCORE_FILES
        "${WIZARDCORE_SRC_ROOT}/*.cpp"
        "${WIZARDCORE_SRC_ROOT}/*.h"
    )
    foreach(WIZARDCORE_FILE ${ALL_WIZARDCORE_FILES})
        file(RELATIVE_PATH REL_PATH "${WIZARDCORE_SRC_ROOT}" "${WIZARDCORE_FILE}")
        get_filename_component(FILE_DIR "${REL_PATH}" DIRECTORY)
        if(FILE_DIR STREQUAL "")
            source_group("WizardCore" FILES "${WIZARDCORE_FILE}")
        else()
            string(REPLACE "/" "\\" GROUP_PATH "${FILE_DIR}")
            source_group("WizardCore\\${GROUP_PATH}" FILES "${WIZARDCORE_FILE}")
        endif()
    endforeach()
    # Add WizardCore files to the target so Visual Studio shows .h files
    # as siblings alongside .cpp files (not hidden under dependency arrows).
    # Without this, VS only discovers headers via #include and nests them.
    target_sources(${PROJECT_NAME} PRIVATE ${ALL_WIZARDCORE_FILES})

    list(LENGTH ALL_WIZARDCORE_FILES WIZARDCORE_FILE_COUNT)
    message(STATUS "Organized ${WIZARDCORE_FILE_COUNT} WizardCore files into Visual Studio source groups")
endif()


# =====================================================================
# [Claude 2026-10-29] Test and review programs, ported from Tertiary. None is part of a release build: each is
# switched on by an option (off by default). Names and plugin settings come from config.cmake.
# See plugin/tests/README.md.
# =====================================================================

# The plugin settings JUCE would give its own targets, for programs that compile the plugin's sources directly
if (wIS_SYNTH)
    set(WLA_IS_SYNTH 1)
else()
    set(WLA_IS_SYNTH 0)
endif()
if (wNEEDS_MIDI_INPUT)
    set(WLA_MIDI_IN 1)
else()
    set(WLA_MIDI_IN 0)
endif()
if (wNEEDS_MIDI_OUTPUT)
    set(WLA_MIDI_OUT 1)
else()
    set(WLA_MIDI_OUT 0)
endif()
set(WLA_PLUGIN_DEFINES
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1
    JucePlugin_Name="${wPLUGIN_NAME}" JucePlugin_VersionString="${wPROJECT_VERSION}" JucePlugin_IsSynth=${WLA_IS_SYNTH}
    JucePlugin_WantsMidiInput=${WLA_MIDI_IN} JucePlugin_ProducesMidiOutput=${WLA_MIDI_OUT} JucePlugin_IsMidiEffect=0)

function(wla_test_program name source)
    juce_add_console_app(${name} PRODUCT_NAME "${name}")
    target_sources(${name} PRIVATE ${PLUGIN_SOURCES} ${source})
    target_compile_definitions(${name} PRIVATE ${WLA_PLUGIN_DEFINES} ${ARGN})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/source
                                               ${CMAKE_CURRENT_BINARY_DIR}/generated)
    add_dependencies(${name} wla_build_id)
    target_link_libraries(${name} PRIVATE ${PROJECT_NAME}BinaryData juce::juce_audio_utils juce::juce_dsp
                                          WizardCore::AudioModules juce::juce_recommended_config_flags)
    if (MSVC)
        set_property(TARGET ${name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endfunction()

# ReviewRender (-DWLA_REVIEW_RENDER=ON): runs the processor (and editor, for video frames) offline. Used by the
# render regression (tests/regression/render_matrix.py) and for review material. See review/ReviewRender.cpp.
option(WLA_REVIEW_RENDER "Build the offline ReviewRender tool" OFF)
if (WLA_REVIEW_RENDER)
    wla_test_program(ReviewRender ${CMAKE_CURRENT_SOURCE_DIR}/review/ReviewRender.cpp WLA_REVIEW_RENDER_BUILD=1)
endif()

# Unit tests and the host test (-DWLA_TESTS=ON). The release pipeline builds both and runs the unit tests and
# pluginval on the very build it signs.
#   <wPLUGIN_NAME>Tests   unit tests on the processor (tests/PluginTests.cpp)   exit code 0 = all passed
#   HostTest              loads the BUILT plugin (VST3, or the installed AU) as a DAW does (tests/HostTest.cpp)
option(WLA_TESTS "Build the unit tests and the host test" OFF)
if (WLA_TESTS)
    wla_test_program(${PROJECT_NAME}Tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/PluginTests.cpp JUCE_UNIT_TESTS=0)
    wla_test_program(HostTest ${CMAKE_CURRENT_SOURCE_DIR}/tests/HostTest.cpp
        JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_AU=$<BOOL:${APPLE}>
        WLA_AU_ID=",${wPLUGIN_CODE},${wPLUGIN_MANUFACTURER_CODE}")
endif()
