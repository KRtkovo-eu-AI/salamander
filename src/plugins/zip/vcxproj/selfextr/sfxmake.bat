@echo off

setlocal
set "sfxmake_exit_code=1"

if "%MSB%"=="" set "MSB=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"

REM
REM Set the resulting package version to match the version
REM supported by the ZIP plugin and Zip2Sfx
REM

set package_compatible_version=5
set package_version=5

REM
REM Set this variable to the current location of sfxmake.exe
REM
set sfxmake_exe="..\sfxmake\release\sfxmake.exe"

if not exist %sfxmake_exe% (
  echo.
  if not exist "..\sfxmake\Release\Intermediate" (
    mkdir "..\sfxmake\Release\Intermediate"
    if errorlevel 1 goto BUILD_FAILED
  )
  "%MSB%" ..\sfxmake\sfxmake.vcxproj /t:rebuild "/p:Configuration=Release" /p:Platform=Win32 /m:%NUMBER_OF_PROCESSORS%
  if errorlevel 1 goto BUILD_FAILED
  if not exist %sfxmake_exe% (
    echo.
    echo %sfxmake_exe% still does not exist!
    goto BUILD_FAILED
  )
)

if "%1"=="" goto INVALID_LANGUAGE1
if "%2"=="" goto INVALID_LANGUAGE1

set output_directory=%~2

REM Create the target directory
if not exist "%output_directory%" (
  mkdir "%output_directory%"
  if errorlevel 1 goto BUILD_FAILED
)

if %1==ENGLISH_VERSION goto ENGLISH
if %1==CZECH_VERSION goto CZECH
if %1==SLOVAK_VERSION goto SLOVAK
if %1==GERMAN_VERSION goto GERMAN
if %1==HUNGARIAN_VERSION goto HUNGARIAN
if %1==SPANISH_VERSION goto SPANISH
if %1==ROMANIAN_VERSION goto ROMANIAN
if %1==RUSSIAN_VERSION goto RUSSIAN
if %1==CHINESESIMPL_VERSION goto CHINESESIMPL
if %1==DUTCH_VERSION goto DUTCH
if %1==FRENCH_VERSION goto FRENCH
if %1==ITALIAN_VERSION goto ITALIAN

goto INVALID_LANGUAGE2

:MAKE_SFX

echo.
echo ======================================================
echo Making %1 (code page: %2)
echo ======================================================
echo.
setlocal
set SALAMANDER_SFX_LANGUAGE=%1
set SALAMANDER_SFX_CODEPAGE=%2
if not exist Release\Intermediate (
  mkdir Release\Intermediate
  if errorlevel 1 goto MAKE_SFX_FAILED
)
if exist Release del /f /q Release\*.*
"%MSB%" selfextr.vcxproj /t:rebuild "/p:Configuration=Release" /p:Platform=Win32 /m:%NUMBER_OF_PROCESSORS%
if errorlevel 1 goto MAKE_SFX_FAILED
echo.
if not exist ReleaseEx\Intermediate (
  mkdir ReleaseEx\Intermediate
  if errorlevel 1 goto MAKE_SFX_FAILED
)
if exist ReleaseEx del /f /q ReleaseEx\*.*
"%MSB%" selfextr.vcxproj /t:rebuild "/p:Configuration=ReleaseEx" /p:Platform=Win32 /m:%NUMBER_OF_PROCESSORS%
if errorlevel 1 goto MAKE_SFX_FAILED
endlocal
%sfxmake_exe% "%output_directory%%3.sfx" "..\..\selfextr\language\%3\texts.txt" %package_version% %package_compatible_version%
if errorlevel 1 exit /b %errorlevel%
exit /b 0

:MAKE_SFX_FAILED
endlocal & exit /b %errorlevel%

:ENGLISH

call :MAKE_SFX %1 windows-1252 english
if errorlevel 1 goto BUILD_FAILED
goto SUCCESS

:CZECH

call :MAKE_SFX %1 windows-1250 czech
if errorlevel 1 goto BUILD_FAILED
goto SUCCESS

:SLOVAK

call :MAKE_SFX %1 windows-1250 slovak
if errorlevel 1 goto BUILD_FAILED
goto SUCCESS

:GERMAN

call :MAKE_SFX %1 windows-1252 german
if errorlevel 1 goto BUILD_FAILED
goto SUCCESS

:HUNGARIAN

call :MAKE_SFX %1 windows-1250 hungarian
if errorlevel 1 goto BUILD_FAILED
goto SUCCESS

:SPANISH

call :MAKE_SFX %1 windows-1252 spanish
if errorlevel 1 goto BUILD_FAILED
goto SUCCESS

:ROMANIAN

call :MAKE_SFX %1 windows-1250 romanian
if errorlevel 1 goto BUILD_FAILED
goto SUCCESS

:RUSSIAN

call :MAKE_SFX %1 windows-1251 russian
if errorlevel 1 goto BUILD_FAILED
goto SUCCESS

:CHINESESIMPL

call :MAKE_SFX %1 gb2312 chinesesimplified
if errorlevel 1 goto BUILD_FAILED
goto SUCCESS

:DUTCH

call :MAKE_SFX %1 windows-1252 dutch
if errorlevel 1 goto BUILD_FAILED
goto SUCCESS

:FRENCH

call :MAKE_SFX %1 windows-1252 french
if errorlevel 1 goto BUILD_FAILED
goto SUCCESS

:ITALIAN

call :MAKE_SFX %1 windows-1252 italian
if errorlevel 1 goto BUILD_FAILED
goto SUCCESS

:INVALID_LANGUAGE1

echo No language or output directory specified.
echo.
goto USAGE

:INVALID_LANGUAGE2

echo Invalid language specified.
echo.

:USAGE

echo Usage: sfxmake.bat language output_directory
echo.
echo Valid values for the 'language' parameter: ENGLISH_VERSION, etc.
echo
echo Output directory should have trailing backslash.
goto FINISH

:SUCCESS
set "sfxmake_exit_code=0"
goto FINISH

:BUILD_FAILED
set "sfxmake_exit_code=%errorlevel%"
if "%sfxmake_exit_code%"=="0" set "sfxmake_exit_code=1"
echo.
echo SFX language build failed with exit code %sfxmake_exit_code%.

:FINISH
if "%making_all_versions%"=="" echo.
if "%making_all_versions%"=="" pause
endlocal & exit /b %sfxmake_exit_code%
