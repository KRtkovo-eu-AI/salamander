@echo off
setlocal

rem Generic signing entry point used by Visual Studio post-build events.
rem Keep product-specific implementation in a neighboring script.

set "SCRIPT_DIR=%~dp0"
call "%SCRIPT_DIR%codesign_certum.cmd" %*
exit /b %ERRORLEVEL%
