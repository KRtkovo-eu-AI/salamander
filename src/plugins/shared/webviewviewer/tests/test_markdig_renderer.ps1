param(
    [Parameter(Mandatory = $true)]
    [string] $RendererPath
)

$ErrorActionPreference = 'Stop'
$resolvedRenderer = (Resolve-Path -LiteralPath $RendererPath).Path
$startInfo = [Diagnostics.ProcessStartInfo]::new($resolvedRenderer)
$startInfo.UseShellExecute = $false
$startInfo.RedirectStandardInput = $true
$startInfo.RedirectStandardOutput = $true
$startInfo.RedirectStandardError = $true
$startInfo.CreateNoWindow = $true
$process = [Diagnostics.Process]::Start($startInfo)

try {
    $markdown = @'
| A | B |
|---|---|
| 1 | 2 |

- [x] task

Footnote[^1]

[^1]: note
'@
    $request = [Text.Encoding]::UTF8.GetBytes($markdown)
    $header = [BitConverter]::GetBytes([int]$request.Length)
    $process.StandardInput.BaseStream.Write($header, 0, $header.Length)
    $process.StandardInput.BaseStream.Write($request, 0, $request.Length)
    $process.StandardInput.BaseStream.Flush()
    $process.StandardInput.Close()

    $responseHeader = [byte[]]::new(5)
    $process.StandardOutput.BaseStream.ReadExactly($responseHeader, 0, $responseHeader.Length)
    $responseLength = [BitConverter]::ToInt32($responseHeader, 1)
    if ($responseLength -lt 0 -or $responseLength -gt 16MB) {
        throw "Invalid response length: $responseLength"
    }
    $response = [byte[]]::new($responseLength)
    $process.StandardOutput.BaseStream.ReadExactly($response, 0, $response.Length)
    if (-not $process.WaitForExit(30000)) {
        $process.Kill()
        throw 'MarkdigRenderer timed out.'
    }

    $html = [Text.Encoding]::UTF8.GetString($response)
    if ($responseHeader[0] -ne 1 -or
        $html -notmatch '<table>' -or
        $html -notmatch 'task-list-item' -or
        $html -notmatch 'footnote') {
        throw "Unexpected Markdig response: $html"
    }
    Write-Host "Markdig NativeAOT IPC passed ($responseLength UTF-8 bytes)."
}
finally {
    if (-not $process.HasExited) {
        $process.Kill()
    }
    $process.Dispose()
}
