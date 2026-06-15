Set-StrictMode -Version Latest
function Invoke-SalamanderTranslatorQuiet {
    [CmdletBinding()] param([Parameter(Mandatory)][string]$TranslatorExe,[Parameter(Mandatory)][string[]]$Arguments,[Parameter(Mandatory)][string]$FailureMessage,[Parameter(Mandatory)][string]$DiagnosticLog,[int[]]$ExpectedExitCodes=@(1),[int]$TimeoutSeconds=120)
    $info=[Diagnostics.ProcessStartInfo]::new(); $info.FileName=$TranslatorExe; $info.WorkingDirectory=Split-Path $TranslatorExe -Parent; $info.UseShellExecute=$false
    foreach($argument in $Arguments){[void]$info.ArgumentList.Add($argument)}
    $process=[Diagnostics.Process]::Start($info)
    if(-not $process.WaitForExit($TimeoutSeconds * 1000)){
        $process.Kill($true)
        throw "$FailureMessage Translator timed out after $TimeoutSeconds seconds and was terminated. Diagnostika: $DiagnosticLog"
    }
    @("Timestamp: $([DateTime]::Now.ToString('s'))","Command: $TranslatorExe $($Arguments -join ' ')","Exit code: $($process.ExitCode)","") | Add-Content -LiteralPath $DiagnosticLog -Encoding UTF8
    if($ExpectedExitCodes -notcontains $process.ExitCode){throw "$FailureMessage Diagnostika: $DiagnosticLog"}
}
Export-ModuleMember -Function Invoke-SalamanderTranslatorQuiet
