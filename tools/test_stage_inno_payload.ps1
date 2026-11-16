# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Verify component ownership manifest output. | Regression-test file inventory needed by managed upgrades. | | openrouter/openai/gpt-6-luna-pro |
# | 2026-11-14 | Test staging of installer-declared extension trees. | Detect missing Release payload files, including nested extension assets. | | openrouter/openai/gpt-6-luna-pro |

[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$stager = Join-Path $PSScriptRoot 'stage_inno_payload.ps1'
$fixtureRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('samandarin-stage-test-' + [guid]::NewGuid().ToString('N'))
$destinationRoot = Join-Path $fixtureRoot 'Release_x64'
$extensionRoot = Join-Path $fixtureRoot 'src\extensions\json-viewer-nodejs'
$demoExtensionRoot = Join-Path $fixtureRoot 'src\extensions\demos\python'

try {
    New-Item -ItemType Directory -Path (Join-Path $fixtureRoot 'doc\runbook-setup') -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $fixtureRoot 'src\res\toolbars') -Force | Out-Null
    New-Item -ItemType Directory -Path $extensionRoot -Force | Out-Null
    New-Item -ItemType Directory -Path $demoExtensionRoot -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $extensionRoot 'viewer\assets') -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'doc\runbook-setup\inno_setup_salamander_x64.iss') -Encoding UTF8 -Value @'
[Files]
Source: "{#PayloadDir}\toolbars\sample.bmp"; DestDir: "{app}\toolbars"; Flags: ignoreversion
Source: "{#PayloadDir}\extensions\json-viewer-nodejs\*"; DestDir: "{app}\extensions\json-viewer-nodejs"; Flags: recursesubdirs createallsubdirs; Check: IsPluginSelected('jsonwebviewer')
Source: "{#PayloadDir}\extensions\demos\*"; DestDir: "{app}\extensions\demos"; Flags: recursesubdirs createallsubdirs; Check: IsPluginSelected('salamatrixdemos')
Source: "{#PayloadDir}\plugins\sample\sample.spl"; DestDir: "{app}\plugins\sample"; Flags: ignoreversion; Check: IsPluginSelected('sample')
'@
    Set-Content -LiteralPath (Join-Path $fixtureRoot 'src\res\toolbars\sample.bmp') -Encoding UTF8 -Value 'toolbar bytes'
    Set-Content -LiteralPath (Join-Path $extensionRoot 'extension.json') -Encoding UTF8 -Value '{"id":"JsonViewerNodeJS"}'
    Set-Content -LiteralPath (Join-Path $extensionRoot 'main.mjs') -Encoding UTF8 -Value 'export const ready = true;'
    Set-Content -LiteralPath (Join-Path $extensionRoot 'viewer\assets\viewer.js') -Encoding UTF8 -Value 'window.viewerReady = true;'
    Set-Content -LiteralPath (Join-Path $demoExtensionRoot 'extension.json') -Encoding UTF8 -Value '{"id":"PythonDemo"}'
    Set-Content -LiteralPath (Join-Path $demoExtensionRoot 'main.py') -Encoding UTF8 -Value 'print("demo")'

    & $stager -RepositoryRoot $fixtureRoot -DestinationRoot $destinationRoot -RuntimeOnly

    foreach ($relativePath in @('toolbars\sample.bmp',
                                 'extensions\json-viewer-nodejs\extension.json',
                                 'extensions\json-viewer-nodejs\viewer\assets\viewer.js',
                                 'extensions\demos\python\main.py')) {
        if (-not (Test-Path -LiteralPath (Join-Path $destinationRoot $relativePath) -PathType Leaf)) {
            throw "Runtime-only staging did not copy $relativePath."
        }
    }
    $managedManifestPath = Join-Path $destinationRoot 'managed-plugin-files.txt'
    if (Test-Path -LiteralPath $managedManifestPath) {
        throw 'Runtime-only staging unexpectedly generated an installer manifest.'
    }
    Set-Content -LiteralPath $managedManifestPath -Encoding UTF8 -NoNewline -Value 'existing manifest'
    $originalHash = (Get-FileHash -LiteralPath $managedManifestPath -Algorithm SHA256).Hash
    $rejectedStale = $false
    try { & $stager -RepositoryRoot $fixtureRoot -DestinationRoot $destinationRoot -RuntimeOnly }
    catch { $rejectedStale = $_.Exception.Message -like 'Runtime-only staging cannot use a directory containing an existing managed-plugin manifest:*' }
    if (-not $rejectedStale -or (Get-FileHash -LiteralPath $managedManifestPath -Algorithm SHA256).Hash -ne $originalHash) {
        throw 'Runtime-only staging did not reject and preserve an existing installer manifest.'
    }
    Remove-Item -LiteralPath $managedManifestPath -Force
    $rejectedMissingPlugin = $false
    try { & $stager -RepositoryRoot $fixtureRoot -DestinationRoot $destinationRoot }
    catch { $rejectedMissingPlugin = $_.Exception.Message -like 'Managed plugin source file was not found:*' }
    if (-not $rejectedMissingPlugin -or (Test-Path -LiteralPath $managedManifestPath)) {
        throw 'Strict staging accepted a missing plugin or generated a partial manifest.'
    }
    New-Item -ItemType Directory -Path (Join-Path $destinationRoot 'plugins\sample') -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $destinationRoot 'plugins\sample\sample.spl') -Encoding UTF8 -Value 'plugin bytes'
    & $stager -RepositoryRoot $fixtureRoot -DestinationRoot $destinationRoot

    $expected = @(
        'extensions\json-viewer-nodejs\extension.json',
        'extensions\json-viewer-nodejs\main.mjs',
        'extensions\json-viewer-nodejs\viewer\assets\viewer.js',
        'extensions\demos\python\extension.json',
        'extensions\demos\python\main.py'
    )
    foreach ($relativePath in $expected) {
        $path = Join-Path $destinationRoot $relativePath
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Expected staged extension file is missing: $path"
        }
    }

    $managedManifest = @(Get-Content -LiteralPath (Join-Path $destinationRoot 'managed-plugin-files.txt'))
    $expectedManifest = @(
        'jsonwebviewer|extensions\json-viewer-nodejs\extension.json',
        'jsonwebviewer|extensions\json-viewer-nodejs\main.mjs',
        'jsonwebviewer|extensions\json-viewer-nodejs\viewer\assets\viewer.js',
        'salamatrixdemos|extensions\demos\python\extension.json',
        'salamatrixdemos|extensions\demos\python\main.py',
        'sample|plugins\sample\sample.spl'
    )
    if (Compare-Object -ReferenceObject $expectedManifest -DifferenceObject $managedManifest) {
        throw 'Component ownership manifest does not list the exact extension files.'
    }

    Write-Host 'PASS: extension files and their component ownership manifest are staged.'
}
finally {
    if (Test-Path -LiteralPath $fixtureRoot) {
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}
