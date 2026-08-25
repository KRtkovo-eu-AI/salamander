@echo off

if /I "%~3"=="Debug" (
  call :mycopy_with_mkdir_bat "%~2" "%~1" "%~1\..\..\..\..\Debug_x64\plugins\zip\zip2sfx" "Debug_x64\plugins\zip\zip2sfx" "%~4"
  if errorlevel 1 exit /b 1
  exit /b 0
)

if /I "%~3"=="Release" (
  call :mycopy_with_mkdir_bat "%~2" "%~1" "%~1\..\..\..\..\Release_x64\plugins\zip\zip2sfx" "Release_x64\plugins\zip\zip2sfx" "%~4"
  if errorlevel 1 exit /b 1
  call :mycopy_with_mkdir_bat "%~2" "%~1" "%~1\..\..\..\..\Release_arm64\plugins\zip\zip2sfx" "Release_arm64\plugins\zip\zip2sfx" "%~4"
  if errorlevel 1 exit /b 1
  exit /b 0
)

if /I "%~3"=="Release clean" (
  call :mycopy_with_mkdir_bat "%~2" "%~1" "%~1\..\..\..\..\Release clean_x64\plugins\zip\zip2sfx" "Release clean_x64\plugins\zip\zip2sfx" "%~4"
  if errorlevel 1 exit /b 1
  call :mycopy_with_mkdir_bat "%~2" "%~1" "%~1\..\..\..\..\Release clean_arm64\plugins\zip\zip2sfx" "Release clean_arm64\plugins\zip\zip2sfx" "%~4"
  if errorlevel 1 exit /b 1
  exit /b 0
)

echo Unsupported ZIP2SFX configuration: "%~3"
exit /b 1


:mycopy_with_mkdir_bat

echo Copying %~1 to %~4...

if not exist "%~3" mkdir "%~3"

copy "%~2\%~1" "%~3\"
if errorlevel 1 (
  echo Error: unable to copy file "%~2\%~1" to "%~3".
  exit /b 1
)
if not "%~5"=="" (
  echo Copying %~n1.pdb to %~4...
  copy "%~2\%~n1.pdb" "%~3\"
  if errorlevel 1 (
    echo Error: unable to copy file "%~2\%~n1.pdb" to "%~3\".
    exit /b 1
  )
)
exit /b
