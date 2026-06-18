@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Signs one file with Certum Open Source Code Signing in the Cloud / SimplySign.
rem Intended to be called from sign_with_retry.cmd by Visual Studio post-build events.

set "TARGET=%~1"
if "%TARGET%"=="" (
  echo Usage: %~nx0 path-to-file-to-sign
  exit /b 2
)

if not exist "%TARGET%" (
  echo Code signing target does not exist: "%TARGET%"
  exit /b 2
)

if /i not "%CODESIGN_ENABLED%"=="1" (
  echo Code signing skipped for "%TARGET%". Set CODESIGN_ENABLED=1 to enable.
  exit /b 0
)

set "SIGNTOOL=%CODESIGN_SIGNTOOL%"
if "%SIGNTOOL%"=="" set "SIGNTOOL=signtool.exe"

set "TIMESTAMP_URL=%CODESIGN_TIMESTAMP_URL%"
if "%TIMESTAMP_URL%"=="" set "TIMESTAMP_URL=http://timestamp.digicert.com"

set "DIGEST_ALGORITHM=%CODESIGN_DIGEST_ALGORITHM%"
if "%DIGEST_ALGORITHM%"=="" set "DIGEST_ALGORITHM=SHA256"

set "TIMESTAMP_DIGEST_ALGORITHM=%CODESIGN_TIMESTAMP_DIGEST_ALGORITHM%"
if "%TIMESTAMP_DIGEST_ALGORITHM%"=="" set "TIMESTAMP_DIGEST_ALGORITHM=SHA256"

set "RETRIES=%CODESIGN_RETRIES%"
if "%RETRIES%"=="" set "RETRIES=3"

set "RETRY_DELAY_SECONDS=%CODESIGN_RETRY_DELAY_SECONDS%"
if "%RETRY_DELAY_SECONDS%"=="" set "RETRY_DELAY_SECONDS=10"

set "CERT_SELECTOR=/a"
if not "%CODESIGN_CERT_SUBJECT%"=="" set "CERT_SELECTOR=/n ^"%CODESIGN_CERT_SUBJECT%^""
if not "%CODESIGN_CERT_SHA1%"=="" set "CERT_SELECTOR=/sha1 %CODESIGN_CERT_SHA1%"

set "DESCRIPTION_ARGS="
if not "%CODESIGN_DESCRIPTION%"=="" set "DESCRIPTION_ARGS=/d ^"%CODESIGN_DESCRIPTION%^""
if not "%CODESIGN_DESCRIPTION_URL%"=="" set "DESCRIPTION_ARGS=%DESCRIPTION_ARGS% /du ^"%CODESIGN_DESCRIPTION_URL%^""

set /a ATTEMPT=1

:sign_retry
echo Code signing "%TARGET%" using "%SIGNTOOL%" ^(attempt %ATTEMPT% of %RETRIES%^) ...
"%SIGNTOOL%" sign /fd %DIGEST_ALGORITHM% /tr "%TIMESTAMP_URL%" /td %TIMESTAMP_DIGEST_ALGORITHM% %CERT_SELECTOR% %DESCRIPTION_ARGS% "%TARGET%"
set "SIGN_RESULT=%ERRORLEVEL%"
if "%SIGN_RESULT%"=="0" goto verify_signature

if %ATTEMPT% GEQ %RETRIES% (
  echo Code signing failed for "%TARGET%" after %RETRIES% attempt^(s^).
  exit /b %SIGN_RESULT%
)

set /a ATTEMPT+=1
echo Code signing failed with exit code %SIGN_RESULT%. Retrying in %RETRY_DELAY_SECONDS% second^(s^) ...
timeout /t %RETRY_DELAY_SECONDS% /nobreak >nul
goto sign_retry

:verify_signature
echo Verifying Authenticode signature for "%TARGET%" ...
"%SIGNTOOL%" verify /pa /v "%TARGET%"
set "VERIFY_RESULT=%ERRORLEVEL%"
if not "%VERIFY_RESULT%"=="0" (
  echo Signature verification failed for "%TARGET%".
  exit /b %VERIFY_RESULT%
)

echo Code signing completed for "%TARGET%".
exit /b 0
