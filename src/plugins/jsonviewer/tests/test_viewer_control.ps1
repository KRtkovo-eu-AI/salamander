param(
    [Parameter(Mandatory = $true)]
    [string]$PluginDirectory
)

$ErrorActionPreference = 'Stop'

Add-Type -Path (Join-Path $PluginDirectory 'Newtonsoft.Json.dll')
Add-Type -Path (Join-Path $PluginDirectory 'JsonViewer.Managed.dll')

$viewer = New-Object EPocalipse.Json.Viewer.JsonViewer
$viewer.add_PropertyChanged({ param($sender, $eventArgs) })
try {
    $flags = [Reflection.BindingFlags]'Instance,NonPublic'
    $tree = $viewer.GetType().GetField('tvJson', $flags).GetValue($viewer)
    $tabs = $viewer.GetType().GetField('tabControl', $flags).GetValue($viewer)

    $viewer.refreshFromString('{"name":"value","items":[1,2]}')
    if ($tree.Nodes.Count -ne 1) {
        throw "Valid JSON produced $($tree.Nodes.Count) root nodes instead of one."
    }

    $viewer.refreshFromString('{"a":')
    if ($tabs.SelectedTab.Name -ne 'pageTextView') {
        throw "Invalid JSON did not select the error-bearing Text tab."
    }

    Write-Output 'JSON Viewer control regression passed.'
}
finally {
    $viewer.Dispose()
}
