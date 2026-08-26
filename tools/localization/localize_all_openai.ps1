<# .SYNOPSIS Headlessly translates every selected language/module using OpenRouter (default), Cursor, or OpenAI. #>
[CmdletBinding()] param(
 [string]$BuildRoot=".\build\out\salamand\Release_x64", [string[]]$Languages, [string[]]$Modules,
 [ValidateSet("openrouter","cursor","openai")][string]$Provider=$(if($env:TRANSLATION_PROVIDER){$env:TRANSLATION_PROVIDER}else{"openrouter"}),
 [string]$Model="",
 [ValidateRange(1,32)][int]$MaxParallelRequests=4,
 [switch]$DryRun, [switch]$ForceRetranslate, [switch]$AutoTrimTranslations, [switch]$BuildLanguagePacks, [switch]$ImportOnly)
Set-StrictMode -Version Latest; $ErrorActionPreference="Stop"
Import-Module (Join-Path $PSScriptRoot "Localization.Common.psm1") -Force
$repoRoot=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent; $workspace=Join-Path $repoRoot "out\localization-openai"; $log=Join-Path $workspace "localize.log"; $apiTrace=Join-Path $workspace "openai-requests.jsonl"
function Expand-List([string[]]$Values){ @($Values | ForEach-Object { $_ -split '[,;]' } | ForEach-Object {$_.Trim().ToLowerInvariant()} | Where-Object {$_} | Sort-Object -Unique) }
if(-not $Model){$Model=switch($Provider){
 "openrouter" {if($env:OPENROUTER_MODEL){$env:OPENROUTER_MODEL}else{"openai/gpt-5.4-nano"}; break}
 "openai" {if($env:OPENAI_MODEL){$env:OPENAI_MODEL}else{"gpt-5-mini"}; break}
 default {if($env:CURSOR_MODEL){$env:CURSOR_MODEL}else{"grok-4.5"}; break}
}}
if(-not $ImportOnly){
 if($Provider -eq "openai" -and -not $env:OPENAI_API_KEY){throw "OPENAI_API_KEY is not set. Set it in the environment; never store it in the repository."}
 if($Provider -eq "openrouter" -and -not $env:OPENROUTER_API_KEY){throw "OPENROUTER_API_KEY is not set. Create one at https://openrouter.ai/keys ; never store it in the repository."}
 if($Provider -eq "cursor" -and -not $env:CURSOR_API_KEY){throw "CURSOR_API_KEY is not set. Create one at https://cursor.com/dashboard/integrations ; never store it in the repository."}
}
if($Languages){$selectedLanguages=Expand-List $Languages}else{$selectedLanguages=@(Get-ChildItem (Join-Path $repoRoot 'translations') -Directory | ForEach-Object Name | Sort-Object)}
$runtime=(Resolve-Path $BuildRoot).Path; $availableModules=@('salamand') + @(Get-ChildItem (Join-Path $runtime 'plugins') -Recurse -Filter english.slg | ForEach-Object {$_.Directory.Parent.Name.ToLowerInvariant()}) | Sort-Object -Unique
if($Modules){$selectedModules=Expand-List $Modules}else{$selectedModules=$availableModules}
$unknown=@($selectedModules | Where-Object {$availableModules -notcontains $_}); if($unknown){throw "Unknown modules: $($unknown -join ', ')"}

# Sync .slt files from plugin submodules into translations/
$syncScript=Join-Path $PSScriptRoot 'sync_plugin_translations.ps1'; if(Test-Path $syncScript){ & $syncScript -RepoRoot $repoRoot }

$langIdByLanguage = @{
 chinesesimplified = 2052; czech = 1029; dutch = 1043; french = 1036; german = 1031
 hungarian = 1038; italian = 1040; romanian = 1048; russian = 1049; slovak = 1051; spanish = 3082
}
function Set-SltLangId([string]$Path,[string]$Language){
 $langId=$langIdByLanguage[$Language]
 if($null -eq $langId){return}
 $text=[IO.File]::ReadAllText($Path,[Text.UTF8Encoding]::new($true))
 $updated=[regex]::Replace($text,'(?m)^(LANGID,)\d+',('${1}' + $langId),1)
 if($updated -ne $text){[IO.File]::WriteAllText($Path,$updated,[Text.UTF8Encoding]::new($true))}
}
if($ImportOnly){
 if(-not (Test-Path (Join-Path $workspace 'runtime\utils\translator.exe'))){throw "Workspace '$workspace' does not exist. Run a full DryRun first to generate the workspace and candidates."}
} else {
 & (Join-Path $PSScriptRoot 'prepare_translation_workspace.ps1') -BuildRoot $runtime -OutputDir $workspace -Languages $selectedLanguages -Modules $selectedModules -Force
}
$translator=Join-Path $workspace 'runtime\utils\translator.exe'; $failures=0; $reports=@()
foreach($language in $selectedLanguages){foreach($module in $selectedModules){
 try {
  $project=Join-Path $workspace "projects\$language\$module\$module.atp"; $skeletonDir=Join-Path $workspace "skeleton\$language\$module"; $candidateDir=Join-Path $workspace "candidate\$language\$module"; New-Item -ItemType Directory -Force $skeletonDir,$candidateDir | Out-Null
  if(-not $ImportOnly){
   Invoke-SalamanderTranslatorQuiet -TranslatorExe $translator -Arguments @('-quiet-export-slt',$skeletonDir,$project) -FailureMessage "Skeleton export failed for $language/$module." -DiagnosticLog $log
   $source=Join-Path $skeletonDir "$module.slt"; $legacy=Join-Path $repoRoot "translations\$language\$module.slt"; $rebased=Join-Path $candidateDir "$module.slt"
   $translateAllCurrentStrings=$false; if(Test-Path $legacy){ & (Join-Path $PSScriptRoot 'rebase_text_archive.ps1') -CurrentArchive $source -LegacyArchive $legacy -OutputArchive $rebased } else { Copy-Item $source $rebased; $translateAllCurrentStrings=$true }
   $args=@($rebased,$rebased,'--language',$language,'--provider',$Provider,'--model',$Model,'--batch-size',[Math]::Max(1,40*$MaxParallelRequests),'--trace-file',$apiTrace,'--source-archive',$source); if($ForceRetranslate -or $translateAllCurrentStrings){$args+='--force-retranslate'}; if($AutoTrimTranslations){$args+='--trim-translations'}
    $json=& python (Join-Path $PSScriptRoot 'translate_slt_with_openai.py') @args; if(-not $?){throw "Translation failed."}; $report=$json|ConvertFrom-Json; if($report.failed -gt 0){Write-Warning ("{0}/{1}: {2} strings were left untranslated after validation retries." -f $language,$module,$report.failed)}; $reports += [pscustomobject]@{Language=$language;Module=$module;Found=$report.found;Translated=$report.translated;Skipped=$report.skipped;Failed=$report.failed;EstimatedInputCharacters=$report.estimated_input_characters}
  } else {
   $rebased=Join-Path $candidateDir "$module.slt"
   if(-not (Test-Path $rebased)){Write-Warning "No candidate file found for $language/$module - skipping import."; continue}
   Write-Host "ImportOnly: importing existing candidate for $language/$module"
  }
   Set-SltLangId -Path $rebased -Language $language
   Invoke-SalamanderTranslatorQuiet -TranslatorExe $translator -Arguments @('-quiet-import-slt',$candidateDir,$project) -FailureMessage "Candidate import/validation failed for $language/$module." -DiagnosticLog $log
   if(-not $DryRun){
    $destination=Join-Path $repoRoot "translations\$language"; New-Item -ItemType Directory -Force $destination|Out-Null
    Copy-Item -LiteralPath $rebased -Destination (Join-Path $destination "$module.slt") -Force
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
