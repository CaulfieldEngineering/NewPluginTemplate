@echo off
setlocal enabledelayedexpansion

REM =========================================================================
REM  Build Web + Deploy to WordPress via SFTP
REM  Builds the WASM plugin and uploads to WordPress via SFTP.
REM  Uses SSH key auth — no password prompts.
REM
REM  Prerequisites:
REM    - Emscripten SDK: set EMSDK env var, or install to %USERPROFILE%\emsdk
REM    - Wonderlab repo cloned alongside this repo
REM    - SSH key with SFTP access to WordPress host
REM
REM  TODO: Set deployment variables below before using.
REM =========================================================================

:: --- Elevate to Administrator ---
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    goto UACPrompt
) else ( goto gotAdmin )
:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%temp%\getadmin.vbs"
    "%temp%\getadmin.vbs"
    exit /B
:gotAdmin
    if exist "%temp%\getadmin.vbs" ( del "%temp%\getadmin.vbs" )
    pushd "%CD%"
    CD /D "%~dp0"

setlocal enabledelayedexpansion
cd /d "%~dp0"
cd ..\..

:: ==========================================================================
:: DEPLOYMENT CONFIGURATION
:: TODO: Update these values for the target WordPress host before using.
:: ==========================================================================
set "SSH_KEY=%USERPROFILE%\.ssh\id_wp"
set "SFTP_HOST=yoursite@sftp.wp.com"
set "WP_DEST=/srv/htdocs/demo/"

:: ==========================================================================
:: Read config.cmake
:: ==========================================================================
for /f "usebackq tokens=*" %%L in (`findstr "set(w" config.cmake`) do (
    set "_line=%%L"
    set "_line=!_line:set(=!"
    set "_line=!_line:)=!"
    for /f "tokens=1,*" %%a in ("!_line!") do (
        set "_key=%%a"
        set "_val=%%b"
        set "_val=!_val:"=!"
        set "_val=!_val: =!"
        set "!_key!=!_val!"
    )
)

set "REPO=!wPLUGIN_NAME!"
set "VERSION=!wPROJECT_VERSION!"

set "PLUGIN_SRC=%cd%\plugin\source"
set "WONDERLAB=%cd%\..\Wonderlab"
set "WEB_BUILD_DIR=%cd%\build\web"

echo.
echo  =========================================================================
echo    Build Web + Deploy to WordPress
echo  =========================================================================
echo.
echo    Plugin:   !REPO! v!VERSION!
echo    Dest:     !SFTP_HOST!!WP_DEST!!REPO!/
echo.
echo  **************************************************************************
echo  **                                                                      **
echo  **   WARNING: This will deploy to the LIVE PRODUCTION website.          **
echo  **   Changes will be immediately visible to customers.                  **
echo  **                                                                      **
echo  **   For internal testing, use build_deploy_ce.bat instead.             **
echo  **                                                                      **
echo  **************************************************************************
echo.
set /p CONFIRM="Continue with PRODUCTION deploy? (y/n): "
if /i not "!CONFIRM!"=="y" (
    echo    Cancelled.
    pause
    exit /b 0
)
echo.

:: =========================================================================
::  [1/4] Build WASM
:: =========================================================================
echo  [1/4] Building WebAssembly...
echo.

:: -- Find Emscripten SDK --
if defined EMSDK (
    set "EMSDK_DIR=!EMSDK!"
) else if exist "%USERPROFILE%\emsdk" (
    set "EMSDK_DIR=%USERPROFILE%\emsdk"
) else (
    echo  ERROR: Emscripten SDK not found.
    echo         Set the EMSDK environment variable or install to %%USERPROFILE%%\emsdk
    pause
    exit /b 1
)

:: -- Check prerequisites --
if not exist "!EMSDK_DIR!\upstream\emscripten\emcc.bat" (
    echo    ERROR: Emscripten SDK not found at !EMSDK_DIR!
    pause
    exit /b 1
)
if not exist "!WONDERLAB!\cmake\WonderlabBuild.cmake" (
    echo    ERROR: Wonderlab not found at !WONDERLAB!
    pause
    exit /b 1
)

:: -- Find BinaryData --
set "BINARY_DATA_DIR=%cd%\build\plugin\images\juce_binarydata_!REPO!BinaryData\JuceLibraryCode"
if not exist "!BINARY_DATA_DIR!\BinaryData.h" (
    echo    BinaryData.h not found. Running native CMake configure...
    cmake -S . -B build -G "Visual Studio 17 2022" -A x64 >nul 2>&1
)
if not exist "!BINARY_DATA_DIR!\BinaryData.h" (
    echo    ERROR: BinaryData.h still not found.
    pause
    exit /b 1
)

:: -- Create build directory --
if not exist "!WEB_BUILD_DIR!" mkdir "!WEB_BUILD_DIR!"

:: -- Forward slash paths for CMake --
set "PLUGIN_SRC_FWD=!PLUGIN_SRC:\=/!"
set "WONDERLAB_FWD=!WONDERLAB:\=/!"
set "BINARY_DATA_FWD=!BINARY_DATA_DIR:\=/!"

:: -- Generate CMakeLists.txt --
echo         Generating CMakeLists.txt...
(
echo cmake_minimum_required(VERSION 3.15^)
echo project(!REPO!Web LANGUAGES C CXX^)
echo set(CMAKE_CXX_STANDARD 20^)
echo set(CMAKE_CXX_STANDARD_REQUIRED ON^)
echo.
echo set(PLUGIN_NAME "!REPO!"^)
echo set(PLUGIN_VERSION "!VERSION!"^)
echo set(PLUGIN_SRC_DIR "!PLUGIN_SRC_FWD!"^)
echo set(WONDERLAB_DIR "!WONDERLAB_FWD!"^)
echo set(PLUGIN_PROCESSOR_HEADER "PluginProcessor.h"^)
echo set(PLUGIN_PROCESSOR_CLASS audio_plugin::AudioPluginAudioProcessor^)
echo set(PLUGIN_EXTRA_INCLUDE_DIRS "!BINARY_DATA_FWD!"^)
echo set(PLUGIN_EXTRA_SOURCE_DIRS "!BINARY_DATA_FWD!"^)
echo.
echo include(!WONDERLAB_FWD!/cmake/WonderlabBuild.cmake^)
) > "!WEB_BUILD_DIR!\CMakeLists.txt"

:: -- Configure + Build --
echo         Configuring...
cd "!WEB_BUILD_DIR!"
call "!EMSDK_DIR!\upstream\emscripten\emcmake.bat" cmake -G Ninja -S . -B . -DCMAKE_BUILD_TYPE=Release >nul 2>&1
if %errorlevel% neq 0 (
    echo    ERROR: CMake configuration failed.
    pause
    exit /b 1
)

echo         Compiling...
call "!EMSDK_DIR!\upstream\emscripten\emmake.bat" ninja
if %errorlevel% neq 0 (
    echo    ERROR: Build failed.
    pause
    exit /b 1
)
cd "%~dp0\.."

:: -- Copy web assets --
echo         Copying web assets...
copy /y "!WONDERLAB!\web\index.html" "!WEB_BUILD_DIR!\" >nul
copy /y "!WONDERLAB!\web\index.php" "!WEB_BUILD_DIR!\" >nul 2>nul
copy /y "!WONDERLAB!\web\serve.php" "!WEB_BUILD_DIR!\" >nul 2>nul

:: -- Copy audio samples --
set "SAMPLE_DIR=%cd%\plugin\sample_audio"
set "TRACK_LIST="
if exist "!SAMPLE_DIR!" (
    if not exist "!WEB_BUILD_DIR!\audio" mkdir "!WEB_BUILD_DIR!\audio"
    for %%f in ("!SAMPLE_DIR!\*.*") do (
        copy "%%f" "!WEB_BUILD_DIR!\audio\" >nul
        if defined TRACK_LIST (
            set "TRACK_LIST=!TRACK_LIST!, { title: '%%~nf', file: 'audio/%%~nxf' }"
        ) else (
            set "TRACK_LIST={ title: '%%~nf', file: 'audio/%%~nxf' }"
        )
    )
)

:: -- Generate plugin-config.js --
(
echo window.WONDERLAB_CONFIG = {
echo     pluginName: '!REPO!',
echo     pluginWidth: 750,
echo     pluginHeight: 515,
echo     wasmJs: '!REPO!Web.js',
echo     tracks: [!TRACK_LIST!]
echo };
) > "!WEB_BUILD_DIR!\plugin-config.js"

echo.
echo    Build complete.

:: =========================================================================
::  [2/4] Upload web files
:: =========================================================================
echo.
echo  [2/4] Uploading web files...

set "DEST_PATH=!WP_DEST!!REPO!"
set "CMDS=%TEMP%\wp_deploy_cmds.txt"
(
echo cd !DEST_PATH!
echo put "!WEB_BUILD_DIR!\index.html"
echo put "!WEB_BUILD_DIR!\plugin-config.js"
echo put "!WEB_BUILD_DIR!\!REPO!Web.js"
echo put "!WEB_BUILD_DIR!\!REPO!Web.wasm"
) > "!CMDS!"

REM Include PHP files only if they were copied (optional, WordPress-specific)
if exist "!WEB_BUILD_DIR!\index.php" (
    echo put "!WEB_BUILD_DIR!\index.php" >> "!CMDS!"
)
if exist "!WEB_BUILD_DIR!\serve.php" (
    echo put "!WEB_BUILD_DIR!\serve.php" >> "!CMDS!"
)

sftp -i "!SSH_KEY!" -b "!CMDS!" !SFTP_HOST!
if %errorlevel% neq 0 (
    echo    ERROR: SFTP upload failed.
    del "!CMDS!" 2>nul
    pause
    exit /b 1
)
del "!CMDS!" 2>nul

:: =========================================================================
::  [3/4] Upload audio samples
:: =========================================================================
echo.
echo  [3/4] Uploading audio...

if exist "!WEB_BUILD_DIR!\audio" (
    set "ACMDS=%TEMP%\wp_deploy_audio.txt"
    (
    echo cd !DEST_PATH!/audio
    for %%f in ("!WEB_BUILD_DIR!\audio\*.*") do (
        echo put "%%f"
    )
    ) > "!ACMDS!"
    sftp -i "!SSH_KEY!" -b "!ACMDS!" !SFTP_HOST!
    del "!ACMDS!" 2>nul
) else (
    echo    No audio files, skipping.
)

:: =========================================================================
::  [4/4] Done
:: =========================================================================
echo.
echo  =========================================================================
echo    Deployed!
echo  =========================================================================
echo.
pause
