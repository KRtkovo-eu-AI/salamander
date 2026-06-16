Set-StrictMode -Version Latest
function Invoke-SalamanderTranslatorQuiet {
    [CmdletBinding()] param([Parameter(Mandatory)][string]$TranslatorExe,[Parameter(Mandatory)][string[]]$Arguments,[Parameter(Mandatory)][string]$FailureMessage,[Parameter(Mandatory)][string]$DiagnosticLog,[int[]]$ExpectedExitCodes=@(1),[int]$TimeoutSeconds=120,[int]$WindowGraceSeconds=5)
    $info=[Diagnostics.ProcessStartInfo]::new(); $info.FileName=$TranslatorExe; $info.WorkingDirectory=Split-Path $TranslatorExe -Parent; $info.UseShellExecute=$false; $info.CreateNoWindow=$true
    $info.Arguments=($Arguments|ForEach-Object{if($_ -match '\s'){'"'+$_+'"'}else{$_}})-join ' '
    $process=[Diagnostics.Process]::Start($info)
    $deadline=[DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $windowDeadline=[DateTime]::UtcNow.AddSeconds($WindowGraceSeconds)
    while(-not $process.HasExited){
        if([DateTime]::UtcNow -ge $deadline){
            $process.Kill()
            throw "$FailureMessage Translator timed out after $TimeoutSeconds seconds and was terminated. Diagnostika: $DiagnosticLog"
        }
        if([DateTime]::UtcNow -ge $windowDeadline){
            $process.Refresh()
            if($process.MainWindowHandle -ne [IntPtr]::Zero){
                $title=$process.MainWindowTitle
                $process.Kill()
                throw "$FailureMessage Translator opened an interactive window instead of finishing quiet mode$(if($title){" ('$title')"}). The process was terminated. Diagnostika: $DiagnosticLog"
            }
        }
        Start-Sleep -Milliseconds 250
    }
    @("Timestamp: $([DateTime]::Now.ToString('s'))","Command: $TranslatorExe $($Arguments -join ' ')","Exit code: $($process.ExitCode)","") | Add-Content -LiteralPath $DiagnosticLog -Encoding UTF8
    if($ExpectedExitCodes -notcontains $process.ExitCode){throw "$FailureMessage Diagnostika: $DiagnosticLog"}
}
Export-ModuleMember -Function Invoke-SalamanderTranslatorQuiet
