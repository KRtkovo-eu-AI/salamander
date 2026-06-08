; Inno Setup script for Open Salamander Samandarin x64.
;
; This file is intentionally kept in the runbook folder next to setup_x64.inf.
; It mirrors the file list, shortcut locations, registry keys and private setup
; metadata from doc/runbook-setup/setup_x64.inf as closely as Inno Setup allows.
;
; Build example:
;   iscc.exe "doc\runbook-setup\inno_setup_salamander_x64.iss" /DPayloadDir="H:\_projects\salamander\output\salamander\Release_x64"
;
; If /DPayloadDir is not supplied, OPENSAL_BUILD_DIR is used when present; otherwise
; the fallback path below is relative to this .iss file.

#define MyAppName "Open Salamander Samandarin"
#define MyAppDisplayName "Open Salamander 5.0 Samandarin 0.5 (x64)"
#define MyAppVersion "5.0-samandarin-0.5"
#define MyAppPublisher "KRtekTM"
#define MyAppURL "https://github.com/KRtkovo-eu-AI/salamander"
#define MyAppExeName "salamand.exe"
#define SLG "english.slg"
#ifndef PayloadDir
  #if GetEnv("OPENSAL_BUILD_DIR") != ""
    #define PayloadDir AddBackslash(GetEnv("OPENSAL_BUILD_DIR")) + "salamander\Release_x64"
  #else
    #define PayloadDir "..\..\output\salamander\Release_x64"
  #endif
#endif

[Setup]
AppId=OpenSalamanderSamandarin-x64-5.0-samandarin-0.5
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppDisplayName}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\Open Salamander Samandarin
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputBaseFilename=setup_{#MyAppVersion}_win_x64
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
UninstallDisplayName={#MyAppDisplayName}
UninstallDisplayIcon=..\..\src\res\samandarin.ico
; Keep this disabled because [Registry] below writes the legacy Add/Remove Programs key pointing to Inno's uninstaller; enabling it would add a duplicate Inno uninstall entry.
CreateUninstallRegKey=no
SetupIconFile=..\..\src\res\samandarin.ico
LicenseFile={#SourcePath}\license.txt
DisableFinishedPage=yes
ChangesAssociations=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "startmenuicon"; Description: "Create a &Start Menu shortcut"; GroupDescription: "Shortcuts:"
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Dirs]
Name: "{app}\convert"
Name: "{app}\convert\centeuro"
Name: "{app}\convert\cyrillic"
Name: "{app}\convert\westeuro"
Name: "{app}\doc"
Name: "{app}\help"
Name: "{app}\help\english"
Name: "{app}\lang"
Name: "{app}\utils"
Name: "{app}\plugins"
Name: "{app}\plugins\7zip"
Name: "{app}\plugins\7zip\lang"
Name: "{app}\plugins\automation"
Name: "{app}\plugins\automation\lang"
Name: "{app}\plugins\automation\scripts"
Name: "{app}\plugins\dbviewer"
Name: "{app}\plugins\dbviewer\lang"
Name: "{app}\plugins\diskmap"
Name: "{app}\plugins\diskmap\lang"
Name: "{app}\plugins\filecomp"
Name: "{app}\plugins\filecomp\lang"
Name: "{app}\plugins\ftp"
Name: "{app}\plugins\ftp\lang"
Name: "{app}\plugins\hypervm"
Name: "{app}\plugins\hypervm\lang"
Name: "{app}\plugins\checksum"
Name: "{app}\plugins\checksum\lang"
Name: "{app}\plugins\ieviewer"
Name: "{app}\plugins\ieviewer\css"
Name: "{app}\plugins\ieviewer\lang"
Name: "{app}\plugins\jsonviewer"
Name: "{app}\plugins\jsonviewer\lang"
Name: "{app}\plugins\mmviewer"
Name: "{app}\plugins\mmviewer\lang"
Name: "{app}\plugins\nethood"
Name: "{app}\plugins\nethood\lang"
Name: "{app}\plugins\pak"
Name: "{app}\plugins\pak\lang"
Name: "{app}\plugins\peviewer"
Name: "{app}\plugins\peviewer\lang"
Name: "{app}\plugins\pictview"
Name: "{app}\plugins\pictview\lang"
Name: "{app}\plugins\portables"
Name: "{app}\plugins\portables\lang"
Name: "{app}\plugins\regedt"
Name: "{app}\plugins\regedt\lang"
Name: "{app}\plugins\renamer"
Name: "{app}\plugins\renamer\lang"
Name: "{app}\plugins\samandarin"
Name: "{app}\plugins\samandarin\lang"
Name: "{app}\plugins\serviceexplorer"
Name: "{app}\plugins\serviceexplorer\lang"
Name: "{app}\plugins\splitcbn"
Name: "{app}\plugins\splitcbn\lang"
Name: "{app}\plugins\tar"
Name: "{app}\plugins\tar\lang"
Name: "{app}\plugins\textviewer"
Name: "{app}\plugins\textviewer\lang"
Name: "{app}\plugins\textviewer\data"
Name: "{app}\plugins\textviewer\data\languages"
Name: "{app}\plugins\textviewer\data\themes"
Name: "{app}\plugins\unarj"
Name: "{app}\plugins\unarj\lang"
Name: "{app}\plugins\uncab"
Name: "{app}\plugins\uncab\lang"
Name: "{app}\plugins\undelete"
Name: "{app}\plugins\undelete\lang"
Name: "{app}\plugins\unfat"
Name: "{app}\plugins\unfat\lang"
Name: "{app}\plugins\unchm"
Name: "{app}\plugins\unchm\lang"
Name: "{app}\plugins\uniso"
Name: "{app}\plugins\uniso\lang"
Name: "{app}\plugins\unlha"
Name: "{app}\plugins\unlha\lang"
Name: "{app}\plugins\unmime"
Name: "{app}\plugins\unmime\lang"
Name: "{app}\plugins\unole"
Name: "{app}\plugins\unole\lang"
Name: "{app}\plugins\unrar"
Name: "{app}\plugins\unrar\lang"
Name: "{app}\plugins\webview2renderviewer"
Name: "{app}\plugins\webview2renderviewer\lang"
Name: "{app}\plugins\wmobile"
Name: "{app}\plugins\wmobile\lang"
Name: "{app}\plugins\zip"
Name: "{app}\plugins\zip\lang"
Name: "{app}\plugins\zip\sfx"
Name: "{app}\plugins\zip\zip2sfx"
Name: "{app}\remove"
Name: "{app}\toolbars"

[Files]
Source: "{#PayloadDir}\salamand.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-console-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-console-l1-2-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-datetime-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-debug-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-errorhandling-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-file-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-file-l1-2-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-file-l2-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-handle-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-heap-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-interlocked-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-libraryloader-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-localization-l1-2-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-memory-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-namedpipe-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-processenvironment-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-processthreads-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-processthreads-l1-1-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-profile-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-rtlsupport-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-string-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-synch-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-synch-l1-2-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-sysinfo-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-timezone-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-core-util-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-conio-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-convert-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-environment-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-filesystem-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-heap-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-locale-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-math-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-multibyte-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-private-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-process-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-runtime-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-stdio-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-string-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-time-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\api-ms-win-crt-utility-l1-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\concrt140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\msvcp140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\ucrtbase.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\vcruntime140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1-1.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250asci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250c852.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250cork.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250ebcd.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250iso1.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250iso2.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250kame.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250koi8.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\1250mac.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\c8521250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\c852asci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\c852kame.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\convert.cfg"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\cork1250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\corkasci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\ebcd1250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\ebcdasci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\iso11250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\iso1asci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\iso21250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\iso2asci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\kame1250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\kameasci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\kamec852.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\koi81250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\koi8asci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\mac1250.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\centeuro\macasci.tab"; DestDir: "{app}\convert\centeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1-1.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251bmik.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251c855.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251c866.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251ebcd.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251iso5.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251koir.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251koiu.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\1251macc.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\bmik1251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\c8551251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\c8661251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\convert.cfg"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\ebcd1251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\iso51251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\koir1251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\koiu1251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\cyrillic\macc1251.tab"; DestDir: "{app}\convert\cyrillic"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\10511252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1051asci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1-1.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\12521051.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252c437.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252c850.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252c860.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252ebcd.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252is15.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252iso1.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\1252macr.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\c4371252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\c437asci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\c8501252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\c850asci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\c8601252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\c860asci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\convert.cfg"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\ebcd1252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\ebcdasci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\is151252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\is15asci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\iso11252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\iso1asci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\macr1252.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\convert\westeuro\macrasci.tab"; DestDir: "{app}\convert\westeuro"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\license.txt"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\license_cz.txt"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\thirdpty.txt"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\lgpl.txt"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\readme.txt"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\doc\translations.txt"; DestDir: "{app}\doc"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\7zip.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\automation.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\dbviewer.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\diskmap.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\filecomp.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\ftp.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\checksum.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\ieviewer.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\mmviewer.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\nethood.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\pak.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\peviewer.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\regedt.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\renamer.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\salamand.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\salamand.chw"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\splitcbn.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\tar.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\unarj.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\uncab.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\unchm.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\undelete.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\unfat.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\uniso.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\unlha.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\unmime.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\wmobile.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\help\english\zip.chm"; DestDir: "{app}\help\english"; Flags: ignoreversion
Source: "{#PayloadDir}\lang\{#SLG}"; DestDir: "{app}\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\dbghelp.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\sqlite.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\ssleay32.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\libeay32.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\muires.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\salmon.exe"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\salopen.exe"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\salextx64.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\salextx86.dll"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\utils\salspawn.exe"; DestDir: "{app}\utils"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\plugins.ver"; DestDir: "{app}\plugins"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\7zip\7za.dll"; DestDir: "{app}\plugins\7zip"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\7zip\7zip.spl"; DestDir: "{app}\plugins\7zip"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\7zip\7zwrapper.dll"; DestDir: "{app}\plugins\7zip"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\7zip\lang\{#SLG}"; DestDir: "{app}\plugins\7zip\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\automation\automation.spl"; DestDir: "{app}\plugins\automation"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\automation\lang\{#SLG}"; DestDir: "{app}\plugins\automation\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\automation\scripts\Convert Images.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\automation\scripts\Count Lines.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\automation\scripts\Launch Elevated Command Prompt.vbs"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\automation\scripts\Make Link.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\automation\scripts\Make List (JScript).js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\automation\scripts\Make List (VBScript).vbs"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\automation\scripts\Unpack Multiple Archives.js"; DestDir: "{app}\plugins\automation\scripts"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\dbviewer\dbviewer.spl"; DestDir: "{app}\plugins\dbviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\dbviewer\lang\{#SLG}"; DestDir: "{app}\plugins\dbviewer\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\diskmap\diskmap.spl"; DestDir: "{app}\plugins\diskmap"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\diskmap\lang\{#SLG}"; DestDir: "{app}\plugins\diskmap\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\filecomp\fcremote.exe"; DestDir: "{app}\plugins\filecomp"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\filecomp\filecomp.spl"; DestDir: "{app}\plugins\filecomp"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\filecomp\lang\{#SLG}"; DestDir: "{app}\plugins\filecomp\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\folders\folders.spl"; DestDir: "{app}\plugins\folders"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\folders\lang\{#SLG}"; DestDir: "{app}\plugins\folders\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\ftp\ftp.spl"; DestDir: "{app}\plugins\ftp"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\ftp\lang\{#SLG}"; DestDir: "{app}\plugins\ftp\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\hypervm\hypervm.spl"; DestDir: "{app}\plugins\hypervm"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\hypervm\lang\{#SLG}"; DestDir: "{app}\plugins\hypervm\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\checksum\checksum.spl"; DestDir: "{app}\plugins\checksum"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\checksum\lang\{#SLG}"; DestDir: "{app}\plugins\checksum\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\ieviewer\ieviewer.spl"; DestDir: "{app}\plugins\ieviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\ieviewer\css\githubmd.css"; DestDir: "{app}\plugins\ieviewer\css"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\ieviewer\lang\{#SLG}"; DestDir: "{app}\plugins\ieviewer\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\jsonviewer\jsonviewer.spl"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\jsonviewer\JsonViewer.Managed.dll"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\jsonviewer\Newtonsoft.Json.dll"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\jsonviewer\DEPENDENCIES.md"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\jsonviewer\SUPPORTED_FILE_TYPES.md"; DestDir: "{app}\plugins\jsonviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\jsonviewer\lang\{#SLG}"; DestDir: "{app}\plugins\jsonviewer\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\mmviewer\mmviewer.spl"; DestDir: "{app}\plugins\mmviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\mmviewer\lang\{#SLG}"; DestDir: "{app}\plugins\mmviewer\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\nethood\nethood.spl"; DestDir: "{app}\plugins\nethood"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\nethood\lang\{#SLG}"; DestDir: "{app}\plugins\nethood\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\pak\pak.spl"; DestDir: "{app}\plugins\pak"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\pak\lang\{#SLG}"; DestDir: "{app}\plugins\pak\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\peviewer\peviewer.spl"; DestDir: "{app}\plugins\peviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\peviewer\lang\{#SLG}"; DestDir: "{app}\plugins\peviewer\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\pictview\pictview.spl"; DestDir: "{app}\plugins\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\pictview\exif.dll"; DestDir: "{app}\plugins\pictview"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\pictview\lang\{#SLG}"; DestDir: "{app}\plugins\pictview\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\portables\portables.spl"; DestDir: "{app}\plugins\portables"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\portables\lang\{#SLG}"; DestDir: "{app}\plugins\portables\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\regedt\regedt.spl"; DestDir: "{app}\plugins\regedt"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\regedt\lang\{#SLG}"; DestDir: "{app}\plugins\regedt\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\renamer\renamer.spl"; DestDir: "{app}\plugins\renamer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\renamer\lang\{#SLG}"; DestDir: "{app}\plugins\renamer\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\samandarin\samandarin.spl"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\samandarin\Samandarin.Managed.dll"; DestDir: "{app}\plugins\samandarin"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\samandarin\lang\{#SLG}"; DestDir: "{app}\plugins\samandarin\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\serviceexplorer\serviceexplorer.spl"; DestDir: "{app}\plugins\serviceexplorer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\serviceexplorer\lang\{#SLG}"; DestDir: "{app}\plugins\serviceexplorer\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\splitcbn\splitcbn.spl"; DestDir: "{app}\plugins\splitcbn"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\splitcbn\lang\{#SLG}"; DestDir: "{app}\plugins\splitcbn\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\tar\tar.spl"; DestDir: "{app}\plugins\tar"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\tar\lang\{#SLG}"; DestDir: "{app}\plugins\tar\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\textviewer.spl"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\TextViewer.Managed.dll"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\PrismSharp.dll"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\Newtonsoft.Json.dll"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\Microsoft.Web.WebView2.WinForms.dll"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\Microsoft.Web.WebView2.Core.dll"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\WebView2Loader.dll"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\DEPENDENCIES.md"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\SUPPORTED_FILE_TYPES.md"; DestDir: "{app}\plugins\textviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\abap.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\abnf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\actionscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ada.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\agda.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\al.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\antlr4.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\apacheconf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\apex.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\apl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\applescript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\aql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\arduino.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\arff.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\asciidoc.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\asm6502.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\asmatmel.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\aspnet.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\autohotkey.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\autoit.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\avisynth.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\avro-idl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bash.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\basic.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\batch.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bbcode.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bicep.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\birb.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bison.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bnf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\brainfuck.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\brightscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bro.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\bsl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\c.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cfscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cil.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\clike.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\clojure.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cmake.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cobol.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\coffeescript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\concurnas.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\coq.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cpp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\crystal.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\csharp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cshtml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\csp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\css.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\csv.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\cypher.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\d.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\dart.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\dataweave.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\dax.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\dhall.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\diff.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\django.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\dns-zone-file.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\docker.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\dot.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ebnf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\editorconfig.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\eiffel.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ejs.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\elixir.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\elm.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\erb.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\erlang.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\etlua.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\excel-formula.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\factor.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\false.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\firestore-security-rules.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\flow.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\fortran.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\fsharp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ftl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gap.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gcode.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gdscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gedcom.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gherkin.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\git.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\glsl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\gn.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\go.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\go-module.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\graphql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\groovy.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\haml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\handlebars.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\haskell.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\haxe.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\hcl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\hlsl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\hoon.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\hpkp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\hsts.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\http.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\chaiscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\icon.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\icu-message-format.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\idris.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\iecst.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ignore.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ichigojam.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\inform7.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ini.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\io.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\j.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\java.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\javadoc.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\javadoclike.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\javascript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\javastacktrace.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jexl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jolie.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jq.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jsdoc.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\json.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\json5.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jsonp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jsstacktrace.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\jsx.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\julia.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\keepalived.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\keyman.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\kotlin.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\kumir.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\kusto.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\languages.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\latex.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\latte.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\less.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\lilypond.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\liquid.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\lisp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\livescript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\llvm.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\log.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\lolcode.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\lua.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\magma.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\makefile.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\markdown.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\markup.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\markup-templating.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\matlab.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\maxscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\mel.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\mermaid.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\mizar.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\mongodb.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\monkey.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\moonscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\n1ql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\n4js.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nand2tetris-hdl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\naniscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nasm.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\neon.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nevod.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nginx.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nim.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nix.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\nsis.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\objectivec.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ocaml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\opencl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\openqasm.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\oz.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\parigp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\parser.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\pascal.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\pascaligo.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\pcaxis.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\peoplecode.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\perl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\php.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\phpdoc.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\plsql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\powerquery.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\powershell.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\processing.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\prolog.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\promql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\properties.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\protobuf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\psl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\pug.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\puppet.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\pure.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\purebasic.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\purescript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\python.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\q.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\qml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\qore.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\qsharp.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\r.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\racket.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\reason.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\regex.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\rego.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\renpy.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\rest.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\rip.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\roboconf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\robotframework.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\ruby.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\rust.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\sas.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\sass.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\scala.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\scss.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\shell-session.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\scheme.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\smali.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\smalltalk.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\smarty.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\sml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\solidity.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\solution-file.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\soy.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\sparql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\splunk-spl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\sqf.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\sql.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\squirrel.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\stan.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\stylus.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\swift.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\systemd.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\t4-cs.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\t4-templating.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\t4-vb.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\tap.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\tcl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\textile.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\toml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\tremor.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\tsx.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\tt2.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\turtle.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\twig.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\typescript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\typoscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\unrealscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\uri.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\v.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\vala.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\vbnet.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\velocity.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\verilog.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\vhdl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\vim.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\visual-basic.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\warpscript.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\wasm.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\web-idl.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\wiki.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\wolfram.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\wren.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\xeora.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\xojo.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\xquery.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\yaml.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\yang.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\languages\zig.json"; DestDir: "{app}\plugins\textviewer\data\languages"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\a11y-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\atom-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\base16-ateliersulphurpool.light.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\cb.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\coldark-cold.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\coldark-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\coy.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\darcula.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\dracula.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\duotone-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\duotone-earth.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\duotone-forest.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\duotone-light.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\duotone-sea.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\duotone-space.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\funky.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\ghcolors.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\gruvbox-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\gruvbox-light.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\holi-theme.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\hopscotch.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\l.txt"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\lucario.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\material-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\material-light.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\material-oceanic.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\night-owl.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\nord.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\okaidia.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\one-dark.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\one-light.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\pojoaque.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\prism.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\shades-of-purple.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\solarized-dark-atom.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\solarizedlight.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\synthwave84.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\tomorrow.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\twilight.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\vs.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\vsc-dark-plus.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\xonokai.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\data\themes\z-touch.json"; DestDir: "{app}\plugins\textviewer\data\themes"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\textviewer\lang\{#SLG}"; DestDir: "{app}\plugins\textviewer\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unarj\unarj.spl"; DestDir: "{app}\plugins\unarj"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unarj\lang\{#SLG}"; DestDir: "{app}\plugins\unarj\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\uncab\uncab.spl"; DestDir: "{app}\plugins\uncab"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\uncab\lang\{#SLG}"; DestDir: "{app}\plugins\uncab\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\undelete\undelete.spl"; DestDir: "{app}\plugins\undelete"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\undelete\lang\{#SLG}"; DestDir: "{app}\plugins\undelete\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unfat\unfat.spl"; DestDir: "{app}\plugins\unfat"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unfat\lang\{#SLG}"; DestDir: "{app}\plugins\unfat\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unchm\unchm.spl"; DestDir: "{app}\plugins\unchm"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unchm\chmlib.dll"; DestDir: "{app}\plugins\unchm"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unchm\lang\{#SLG}"; DestDir: "{app}\plugins\unchm\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\uniso\uniso.spl"; DestDir: "{app}\plugins\uniso"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\uniso\lang\{#SLG}"; DestDir: "{app}\plugins\uniso\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unlha\unlha.spl"; DestDir: "{app}\plugins\unlha"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unlha\lang\{#SLG}"; DestDir: "{app}\plugins\unlha\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unmime\unmime.spl"; DestDir: "{app}\plugins\unmime"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unmime\lang\{#SLG}"; DestDir: "{app}\plugins\unmime\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unole\unole.spl"; DestDir: "{app}\plugins\unole"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unole\lang\{#SLG}"; DestDir: "{app}\plugins\unole\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unrar\unrar.spl"; DestDir: "{app}\plugins\unrar"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unrar\unrar.dll"; DestDir: "{app}\plugins\unrar"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\unrar\lang\{#SLG}"; DestDir: "{app}\plugins\unrar\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\webview2renderviewer.spl"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\WebView2RenderViewer.Managed.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\Markdig.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\Microsoft.Web.WebView2.WinForms.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\Microsoft.Web.WebView2.Wpf.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\Microsoft.Web.WebView2.Core.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\WebView2Loader.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\System.Buffers.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\System.Memory.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\System.Numerics.Vectors.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\System.Runtime.CompilerServices.Unsafe.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\System.Text.Encoding.CodePages.dll"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\DEPENDENCIES.md"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\SUPPORTED_FILE_TYPES.md"; DestDir: "{app}\plugins\webview2renderviewer"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\webview2renderviewer\lang\{#SLG}"; DestDir: "{app}\plugins\webview2renderviewer\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\wmobile\wmobile.spl"; DestDir: "{app}\plugins\wmobile"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\wmobile\lang\{#SLG}"; DestDir: "{app}\plugins\wmobile\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\zip\zip.spl"; DestDir: "{app}\plugins\zip"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\zip\lang\{#SLG}"; DestDir: "{app}\plugins\zip\lang"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\zip\zip2sfx\readme.txt"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\zip\zip2sfx\sample.set"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion
Source: "{#PayloadDir}\plugins\zip\zip2sfx\zip2sfx.exe"; DestDir: "{app}\plugins\zip\zip2sfx"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\CalculateDirectorySizes.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\CalculateOccupiedSpace.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\CilpboardCut.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ClipboardCopy.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ClipboardPaste.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\CommandShell.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\CompareDirectories.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Configuration.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ConnectNetworkDrive.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Convert.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Copy.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\CreateDirectory.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Delete.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Disconnect.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\DriveInformation.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Edit.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\EditNewFile.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Email.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Filter.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\FindFilesAndDirectories.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\FocusNameInOtherPanel.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Forward.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\GoToHotPath.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\GoToPathfromOtherPanel.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\GoToShortcutTarget.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\HelpContents.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\HideSelectedNames.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\HideUnselectedNames.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ChangeAttributes.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ChangeCase.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ChangeDirectory.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Modify.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Move.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\MoveItemDown.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\MoveItemUp.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\New.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\NTFSCompress.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\NTFSUncompress.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\OpenFolder.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\OpenNameinOtherPanel.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Pack.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ParentDirectory.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\PasteShortcut.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Properties.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\QuickRename.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Refresh.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\RootDirectory.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Security.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SharedDirectories.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\ShowHiddenNames.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SmartColumnMode.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SortByAttributes.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SortByDate.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SortByExtension.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SortByName.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SortBySize.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\SwapPanels.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\TabsClose.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\TabsDuplicate.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\TabsNew.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\TabsNext.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\TabsPrevious.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Unpack.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\UserMenu.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\View.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\Views.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\toolbars\WhatIsThis.svg"; DestDir: "{app}\toolbars"; Flags: ignoreversion

[Icons]
Name: "{autodesktop}\Open Salamander Samandarin (x64)"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon
Name: "{group}\Open Salamander Samandarin (x64)"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: startmenuicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Registry]
; Legacy keys and values mirrored from [AddRegistryKeys] and [AddRegistryValues].
; UninstallString intentionally targets Inno Setup's uninstaller instead of the
; legacy remove\remove.exe payload so this installer can uninstall its own files.
Root: HKCU; Subkey: "Software\Open Salamander Samandarin\Applications\Open Salamander Samandarin (x64)"; ValueType: string; ValueName: "Last Directory"; ValueData: "{app}"; Flags: uninsdeletevalue
Root: HKLM64; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Open Salamander 5.0-samandarin-0.5 (x64)"; ValueType: string; ValueName: "DisplayName"; ValueData: "{#MyAppDisplayName}"; Flags: uninsdeletekey
Root: HKLM64; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Open Salamander 5.0-samandarin-0.5 (x64)"; ValueType: string; ValueName: "DisplayIcon"; ValueData: "{app}\{#MyAppExeName}"
Root: HKLM64; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Open Salamander 5.0-samandarin-0.5 (x64)"; ValueType: string; ValueName: "DisplayVersion"; ValueData: "{#MyAppVersion}"
Root: HKLM64; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Open Salamander 5.0-samandarin-0.5 (x64)"; ValueType: dword; ValueName: "VersionMajor"; ValueData: "5"
Root: HKLM64; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Open Salamander 5.0-samandarin-0.5 (x64)"; ValueType: dword; ValueName: "VersionMinor"; ValueData: "0"
Root: HKLM64; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Open Salamander 5.0-samandarin-0.5 (x64)"; ValueType: string; ValueName: "InstallLocation"; ValueData: "{app}"
Root: HKLM64; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Open Salamander 5.0-samandarin-0.5 (x64)"; ValueType: string; ValueName: "UninstallString"; ValueData: """{uninstallexe}"""
Root: HKLM64; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Open Salamander 5.0-samandarin-0.5 (x64)"; ValueType: string; ValueName: "QuietUninstallString"; ValueData: """{uninstallexe}"" /SILENT"
Root: HKLM64; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Open Salamander 5.0-samandarin-0.5 (x64)"; ValueType: string; ValueName: "Publisher"; ValueData: "{#MyAppPublisher}"
Root: HKLM64; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Open Salamander 5.0-samandarin-0.5 (x64)"; ValueType: string; ValueName: "HelpLink"; ValueData: "{#MyAppURL}"
Root: HKLM64; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Open Salamander 5.0-samandarin-0.5 (x64)"; ValueType: string; ValueName: "UrlInfoAbout"; ValueData: "{#MyAppURL}"

[UninstallRun]
; INF [DelShellExts] listed these shell-extension DLLs for cleanup.
Filename: "{sys}\regsvr32.exe"; Parameters: "/u /s ""{app}\utils\salextx64.dll"""; Flags: runhidden; RunOnceId: "UnregisterSalExtX64"
Filename: "{sys}\regsvr32.exe"; Parameters: "/u /s ""{app}\utils\salextx86.dll"""; Flags: runhidden; RunOnceId: "UnregisterSalExtX86"

[Code]


var
  DeleteUserConfigurationRegistry: Boolean;

function InitializeUninstall(): Boolean;
begin
  Result := True;
  DeleteUserConfigurationRegistry := False;

  if RegKeyExists(HKCU, 'Software\Open Salamander Samandarin\5.0-samandarin-0.5') then
  begin
    DeleteUserConfigurationRegistry :=
      MsgBox(
        'Do you want to remove the Open Salamander Samandarin user configuration from the registry?'#13#10#13#10 +
        'Registry key:'#13#10 +
        'HKCU\Software\Open Salamander Samandarin\5.0-samandarin-0.5'#13#10#13#10 +
        'Choose Yes to delete the key including all contents, or No to keep your settings.',
        mbConfirmation,
        MB_YESNO) = IDYES;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if (CurUninstallStep = usPostUninstall) and DeleteUserConfigurationRegistry then
    RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Open Salamander Samandarin\5.0-samandarin-0.5');
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  { The finished page otherwise shows Inno Setup's default large bitmap on the left. }
  WizardForm.WizardBitmapImage.Visible := CurPageID <> wpFinished;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  PluginsVer: String;
begin
  if CurStep = ssPostInstall then
  begin
    { Mirrors the setup_x64.inf IncrementFileContent metadata by ensuring plugins.ver exists.
      The legacy installer used the registry value
      HKCU\Software\Open Salamander Samandarin\5.0-samandarin-0.5\Configuration\Plugins.ver Version (x64)
      to decide whether selected plugins should be appended. Inno installs the staged plugins directly. }
    PluginsVer := ExpandConstant('{app}\plugins\plugins.ver');
    if not FileExists(PluginsVer) then
      SaveStringToFile(PluginsVer, '', False);
  end;
end;
