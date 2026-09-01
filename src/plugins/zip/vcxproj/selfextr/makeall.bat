@echo off

setlocal
set "build_exit_code=1"
set "sfx_build_lock_acquired="
set "sfx_pushd_done="

REM
REM Usage: makeall.bat
REM
REM Build the SFX package into the sfx subdirectory

if "%OPENSAL_BUILD_DIR%"=="" (
  echo Please set OPENSAL_BUILD_DIR environment variable.
  echo.
  goto cleanup
)

if not exist "%OPENSAL_BUILD_DIR%salamander" (
  echo Build directory does not exist: %OPENSAL_BUILD_DIR%salamander
  echo.
  goto cleanup
)

set "sfx_build_lock=%TEMP%\OpenSalamander-SFX-build.lock"
:acquire_sfx_build_lock
2>nul mkdir "%sfx_build_lock%"
if errorlevel 1 (
  echo Another SFX build is using the shared self-extractor outputs. Waiting...
  CHOICE /T 1 /C yn /D y >nul
  goto acquire_sfx_build_lock
)
set "sfx_build_lock_acquired=yes"

pushd "%~dp0"
if errorlevel 1 goto build_failed
set "sfx_pushd_done=yes"
set making_all_versions=dummy

echo Making all language versions.

for %%v in (ENGLISH_VERSION CZECH_VERSION SLOVAK_VERSION GERMAN_VERSION ^
            HUNGARIAN_VERSION SPANISH_VERSION ROMANIAN_VERSION ^
            RUSSIAN_VERSION CHINESESIMPL_VERSION DUTCH_VERSION ^
            FRENCH_VERSION ITALIAN_VERSION) do (
  call "%~dp0sfxmake.bat" %%v sfx\
  if errorlevel 1 goto build_failed
)

echo.
echo Deleting intermediate directories...

if exist ..\sfxmake\Release rmdir /Q /S ..\sfxmake\Release
call :remove_dir_autoretry Release
call :remove_dir_autoretry ReleaseEx

rem @mkdir Other_SFX
rem @call :sfxmove ..\..\RELEASE\plugins\zip\sfx\hungarian.sfx Other_SFX\hungarian.sfx

echo.
if "%makeall_should_pause%"=="" (
  echo Press any key to copy SFX packages to %OPENSAL_BUILD_DIR%salamander...
  echo.
  pause
)

for %%t in (Debug_x86 Release_x86 Debug_x64 Release_x64 Release_arm64 "Release clean_x64" "Release clean_arm64") do (
  echo.
  echo Copying SFX packages to %%~t...
  if not exist "%OPENSAL_BUILD_DIR%salamander\%%~t\plugins\zip\sfx" (
    mkdir "%OPENSAL_BUILD_DIR%salamander\%%~t\plugins\zip\sfx"
    if errorlevel 1 goto build_failed
  )
  for %%s in (czech.sfx english.sfx german.sfx slovak.sfx spanish.sfx ^
              romanian.sfx hungarian.sfx russian.sfx chinesesimplified.sfx ^
              dutch.sfx french.sfx italian.sfx) do (
    call :sfxcopy "sfx\%%s" "%OPENSAL_BUILD_DIR%salamander\%%~t\plugins\zip\sfx\%%s"
    if errorlevel 1 goto build_failed
  )
)

if exist sfx rmdir /Q /S sfx
set "build_exit_code=0"
goto cleanup

:build_failed
set "build_exit_code=%errorlevel%"
if "%build_exit_code%"=="0" set "build_exit_code=1"
echo.
echo SFX build failed with exit code %build_exit_code%.

:cleanup
if "%sfx_pushd_done%"=="yes" popd
if "%sfx_build_lock_acquired%"=="yes" rmdir /Q /S "%sfx_build_lock%" >nul 2>nul
if "%makeall_should_pause%"=="" (
  echo.
  pause
)
endlocal & exit /b %build_exit_code%


rem ----------------------------- Copy routine

:sfxcopy

copy /Y %1 %2
if errorlevel 1 (
  echo !!!!!!!!!!!!!!!!!!!!   ERROR   !!!!!!!!!!!!!!!!!!!!
  echo !!!!!!!!!!!!!!!!!!!!   ERROR   !!!!!!!!!!!!!!!!!!!!
  echo !!!!!!!!!!!!!!!!!!!!   ERROR   !!!!!!!!!!!!!!!!!!!!
  if "%makeall_should_pause%"=="" pause
  exit /b 1
)
exit /b 0

rem ----------------------------- Move routine

:sfxmove

move /Y %1 %2
if errorlevel 1 (
  echo !!!!!!!!!!!!!!!!!!!!   ERROR   !!!!!!!!!!!!!!!!!!!!
  echo !!!!!!!!!!!!!!!!!!!!   ERROR   !!!!!!!!!!!!!!!!!!!!
  echo !!!!!!!!!!!!!!!!!!!!   ERROR   !!!!!!!!!!!!!!!!!!!!
  if "%makeall_should_pause%"=="" pause
  exit /b 1
)
exit /b 0

rem ----------------------------- Remove directory with auto-retry routine

:remove_dir_autoretry
if exist %1 rmdir /Q /S %1
if exist %1 (
  echo Trying to remove directory %1 again after 1 second waiting...
  CHOICE /T 1 /C yn /D y >nul
  goto :remove_dir_autoretry
)
exit /b
