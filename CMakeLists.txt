# Mandatory line, sets the minimum version of CMake that should be used with this repository.
# I specified 3.22 because I trust it. However, currently I have 3.26 installed on my machine.
# To verify your version run
# $ cmake --version
# =====================================================================
cmake_minimum_required(VERSION 3.22)

# Force 64-bit architecture on Windows to prevent 32-bit builds
# This is critical for VST3 plugins as modern DAWs (like Cubase) no longer support 32-bit plugins
# =====================================================================
if(WIN32 AND NOT CMAKE_GENERATOR_PLATFORM)
    set(CMAKE_GENERATOR_PLATFORM "x64" CACHE STRING "Platform architecture" FORCE)
    message(STATUS "CMakeLists.txt: Forcing x64 architecture on Windows")
endif()


# Include the configuration file
# =====================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/config.cmake)


# Allow environment variable to override the default version from config.cmake (e.g., GitHub tag)
# =====================================================================
if(NOT DEFINED wPROJECT_VERSION AND DEFINED ENV{wPROJECT_VERSION})
  set(wPROJECT_VERSION "$ENV{wPROJECT_VERSION}")
endif()


# [Claude 2026-10-29] Oldest macOS the Mac build runs on, from wMACOS_MIN_VERSION in config.cmake (Tertiary uses
# 11.0, Big Sur). Must be set before project(). The universal build (Apple Silicon + Intel) is chosen in the
# workflows with -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"; plugin/tests/ci/mac_checks.sh checks both.
# =====================================================================
if (NOT DEFINED wMACOS_MIN_VERSION)
    set(wMACOS_MIN_VERSION "11.0")
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "${wMACOS_MIN_VERSION}" CACHE STRING "Oldest macOS supported")


# Use the variables defined in config.cmake
# =====================================================================
message(STATUS "CMakeLists.txt: Setting configuration variables.")
project(${wPLUGIN_NAME} VERSION ${wPROJECT_VERSION})


# Always use the newest C++ standard on green-field projects if possible.
# =====================================================================
message(STATUS "CMakeLists.txt: Assigning C++ standard.")
set(CMAKE_CXX_STANDARD 23)
if (APPLE)
    # On Mac, we need to wait for a new JUCE version that fixes the compilation issue
    set(CMAKE_CXX_STANDARD 20)
endif()


# Determine the value of DEMO_BUILD
# =====================================================================
if (BUILD_DEMO)
    set(DEMO_BUILD True)
else()
    set(DEMO_BUILD False)
endif()


# Configure the Windows Installation package files
# This substitues all $w variables with those defined in config.cmake
# =====================================================================
message(STATUS "CMakeLists.txt: Configuring Windows Installation Files.")
configure_file(
  ${CMAKE_SOURCE_DIR}/package_config/installer_win.iss.in
  ${CMAKE_BINARY_DIR}/installer_win.iss
  @ONLY
)


# Configure the Windows Installation package files
# This substitues all $w variables with those defined in config.cmake
# =====================================================================
message(STATUS "CMakeLists.txt: Configuring Windows Installation Files.")
configure_file(
  ${CMAKE_SOURCE_DIR}/package_config/installer_win.iss.in
  ${CMAKE_BINARY_DIR}/installer_win.iss
  @ONLY
)

# [Claude 2026-10-29] The Mac installer's choices (VST3, AU, Standalone) follow wFORMATS
include(${CMAKE_SOURCE_DIR}/cmake/installer_formats.cmake)

# Configure the macOS Distribution package file - Full
# This substitues all $w variables with those defined in config.cmake
# =====================================================================
message(STATUS "CMakeLists.txt: Configuring macOS Distribution XML (Full).")
configure_file(
  ${CMAKE_SOURCE_DIR}/package_config/mac/distribution-full.xml
  ${CMAKE_BINARY_DIR}/distribution-full.xml
  @ONLY
)

# Configure the macOS Distribution package file - Demo
# This substitues all $w variables with those defined in config.cmake
# =====================================================================
message(STATUS "CMakeLists.txt: Configuring macOS Distribution XML (Demo).")
configure_file(
  ${CMAKE_SOURCE_DIR}/package_config/mac/distribution-demo.xml
  ${CMAKE_BINARY_DIR}/distribution-demo.xml
  @ONLY
)

# I like to download the dependencies to the same folder as the project.
# If you want to install them system wide, set CPM_SOURCE_CACHE with the path to the dependencies
# either as an environment variable or pass it to the cmake script with -DCPM_SOURCE_CACHE=<path>.
# =====================================================================
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)


# Downloads CPM if not already downloaded. CPM is an easy-to-use package manager nicely integrated with CMake.
# =====================================================================
include(cmake/cpm.cmake)

# This commands downloads AND configures JUCE. It sets up some variables, like JUCE_SOURCE_DIR.
# =====================================================================
CPMAddPackage(
    NAME JUCE
    GIT_TAG 8.0.8  # ← Updated to latest stable release
    GITHUB_REPOSITORY juce-framework/JUCE
    SOURCE_DIR ${LIB_DIR}/juce
)

# WizardCore — Shared DSP/utility library
# Provides reusable audio modules (Chorus, LFO, DelayLine, etc.)
# under the WizardCore:: CMake namespace. See plugin/CMakeLists.txt for usage.
#
# LOCAL DEVELOPMENT vs CI/PRODUCTION:
#   If the environment variable CPM_WizardCore_SOURCE is set, CPM will use
#   that local directory as-is — no clone, no checkout, no commit required.
#   This is the recommended workflow for development: edit WizardCore locally,
#   hit Ctrl+B in Visual Studio, and changes compile immediately.
#
#   When CPM_WizardCore_SOURCE is NOT set (e.g., on a CI runner), CPM falls
#   back to cloning from GitHub at the specified GIT_TAG.
#
#   Development:  set CPM_WizardCore_SOURCE=C:\path\to\WizardCore
#   Production:   leave unset — CI clones from GitHub
#   Ship time:    pin GIT_TAG to a specific version tag (e.g., v1.0.0)
# =====================================================================
CPMAddPackage(
    NAME WizardCore
    GIT_TAG staging  # ← Tracks latest WizardCore. Pin to a version tag at ship time.
    GITHUB_REPOSITORY CaulfieldEngineering/WizardCore
)

# Adds googletest.
# =====================================================================
# CPMAddPackage(
#     NAME GOOGLETEST
#     GITHUB_REPOSITORY google/googletest
#     GIT_TAG v1.13.0
#     VERSION 1.13.0
#     SOURCE_DIR ${LIB_DIR}/googletest
#     OPTIONS
#         "INSTALL_GTEST OFF"
#         "gtest_force_shared_crt ON"
# )

# This command allows running tests from the "build" folder (the one where CMake generates the project to).
# =====================================================================
enable_testing()

# Adds all the targets configured in the "plugin" folder.
# =====================================================================
add_subdirectory(plugin)

# Adds graphical assets folder
# =====================================================================
add_subdirectory(plugin/images)

# Adds all the targets configured in the "test" folder.
# =====================================================================
#add_subdirectory(test)

