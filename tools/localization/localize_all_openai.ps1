<# .SYNOPSIS Headlessly translates every selected language/module using OpenAI. #>
[CmdletBinding()] param(
 [string]$BuildRoot=".\build\out\salamand\Release_x64", [string[]]$Languages, [string[]]$Modules,
 [string]$Model=$env:OPENAI_MODEL, [ValidateRange(1,32)][int]$MaxParallelRequests=4,
 [switch]$DryRun, [switch]$ForceRetranslate, [switch]$BuildLanguagePacks)
Set-StrictMode -Version Latest; $ErrorActionPreference="Stop"
Import-Module (Join-Path $PSScriptRoot "Localization.Common.psm1") -Force
$repoRoot=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent; $workspace=Join-Path $repoRoot "out\localization-openai"; $log=Join-Path $workspace "localize.log"; $apiTrace=Join-Path $workspace "openai-requests.jsonl"
function Expand-List([string[]]$Values){ @($Values | ForEach-Object { $_ -split '[,;]' } | ForEach-Object {$_.Trim().ToLowerInvariant()} | Where-Object {$_} | Sort-Object -Unique) }
if(-not $Model){$Model="gpt-5-mini"}; if(-not $env:OPENAI_API_KEY){throw "OPENAI_API_KEY is not set. Set it in the environment; never store it in the repository."}
if($Languages){$selectedLanguages=Expand-List $Languages}else{$selectedLanguages=@(Get-ChildItem (Join-Path $repoRoot 'translations') -Directory | ForEach-Object Name | Sort-Object)}
$runtime=(Resolve-Path $BuildRoot).Path; $availableModules=@('salamand') + @(Get-ChildItem (Join-Path $runtime 'plugins') -Recurse -Filter english.slg | ForEach-Object {$_.Directory.Parent.Name.ToLowerInvariant()}) | Sort-Object -Unique
if($Modules){$selectedModules=Expand-List $Modules}else{$selectedModules=$availableModules}
$unknown=@($selectedModules | Where-Object {$availableModules -notcontains $_}); if($unknown){throw "Unknown modules: $($unknown -join ', ')"}
& (Join-Path $PSScriptRoot 'prepare_translation_workspace.ps1') -BuildRoot $runtime -OutputDir $workspace -Languages $selectedLanguages -Modules $selectedModules -Force
$translator=Join-Path $workspace 'runtime\utils\translator.exe'; $failures=0; $reports=@()
foreach($language in $selectedLanguages){foreach($module in $selectedModules){
 try {
  $project=Join-Path $workspace "projects\$language\$module\$module.atp"; $skeletonDir=Join-Path $workspace "skeleton\$language\$module"; $candidateDir=Join-Path $workspace "candidate\$language\$module"; New-Item -ItemType Directory -Force $skeletonDir,$candidateDir | Out-Null
  Invoke-SalamanderTranslatorQuiet -TranslatorExe $translator -Arguments @('-quiet-export-slt',$skeletonDir,$project) -FailureMessage "Skeleton export failed for $language/$module." -DiagnosticLog $log
  $source=Join-Path $skeletonDir "$module.slt"; $legacy=Join-Path $repoRoot "translations\$language\$module.slt"; $rebased=Join-Path $candidateDir "$module.slt"
  if(Test-Path $legacy){ & (Join-Path $PSScriptRoot 'rebase_text_archive.ps1') -CurrentArchive $source -LegacyArchive $legacy -OutputArchive $rebased } else { Copy-Item $source $rebased }
  $args=@($rebased,$rebased,'--language',$language,'--model',$Model,'--batch-size',[Math]::Max(1,40*$MaxParallelRequests),'--trace-file',$apiTrace); if($DryRun){$args+='--dry-run'}; if($ForceRetranslate){$args+='--force-retranslate'}
  $json=& python (Join-Path $PSScriptRoot 'translate_slt_with_openai.py') @args; if(-not $?){throw "OpenAI translation failed."}; $report=$json|ConvertFrom-Json; if($report.failed -gt 0){Write-Warning "$language/$module: $($report.failed) strings were left untranslated after validation retries."}; $reports += [pscustomobject]@{Language=$language;Module=$module;Found=$report.found;Translated=$report.translated;Skipped=$report.skipped;Failed=$report.failed;EstimatedInputCharacters=$report.estimated_input_characters}
  if(-not $DryRun){
   $destination=Join-Path $repoRoot "translations\$language"; New-Item -ItemType Directory -Force $destination|Out-Null
   Copy-Item -LiteralPath $rebased -Destination (Join-Path $destination "$module.slt") -Force
   Invoke-SalamanderTranslatorQuiet -TranslatorExe $translator -Arguments @('-quiet-import-slt',$candidateDir,$project) -FailureMessage "Candidate import/validation failed for $language/$module." -DiagnosticLog $log
   Invoke-SalamanderTranslatorQuiet -TranslatorExe $translator -Arguments @('-quiet-export-slt',$destination,$project) -FailureMessage "Final export failed for $language/$module." -DiagnosticLog $log
  }
 } catch { $failures++; Write-Error -ErrorAction Continue "${language}/${module}: $_" }
}}
$reports | Format-Table -AutoSize
if($failures){
 Write-Warning "$failures language/module translation jobs failed. Successfully translated candidate SLT files were already copied to translations; see $log and $apiTrace for diagnostics."
 if($BuildLanguagePacks){throw "$failures language/module translation jobs failed; language packs were not built."}
}
if($BuildLanguagePacks -and -not $DryRun){ & (Join-Path $PSScriptRoot 'build_language_packs.ps1') -BuildRoot $runtime }
