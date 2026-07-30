Set-StrictMode -Version 2.0

function Initialize-ExtensionDarkMode {
    if ($null -ne ('OpenSalamander.Extensions.DarkModeNativeMethods' -as [type])) {
        return
    }
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

namespace OpenSalamander.Extensions
{
    public static class DarkModeNativeMethods
    {
        [DllImport("dwmapi.dll")]
        public static extern int DwmSetWindowAttribute(
            IntPtr hwnd, int attribute, ref int value, int valueSize);

        [DllImport("uxtheme.dll", CharSet = CharSet.Unicode)]
        public static extern int SetWindowTheme(
            IntPtr hwnd, string subAppName, string subIdList);
    }
}
'@
}

function Set-ExtensionDarkMode {
    param([System.Windows.Forms.Form]$Form)

    if (-not $script:UseWindowsDarkMode) { return }

    Initialize-ExtensionDarkMode
    $background = [System.Drawing.Color]::FromArgb(32, 32, 32)
    $surface = [System.Drawing.Color]::FromArgb(45, 45, 48)
    $input = [System.Drawing.Color]::FromArgb(37, 37, 38)
    $header = [System.Drawing.Color]::FromArgb(50, 50, 54)
    $border = [System.Drawing.Color]::FromArgb(80, 80, 80)
    $text = [System.Drawing.Color]::FromArgb(241, 241, 241)
    $selection = [System.Drawing.Color]::FromArgb(0, 122, 204)

    $controls = New-Object System.Collections.Stack
    $controls.Push($Form)
    while ($controls.Count -gt 0) {
        $control = $controls.Pop()
        $control.BackColor = $background
        $control.ForeColor = $text

        if ($control -is [System.Windows.Forms.Button]) {
            $control.UseVisualStyleBackColor = $false
            $control.FlatStyle = 'Flat'
            $control.BackColor = $surface
            $control.FlatAppearance.BorderColor = $border
        } elseif (
            $control -is [System.Windows.Forms.TextBox] -or
            $control -is [System.Windows.Forms.RichTextBox] -or
            $control -is [System.Windows.Forms.ComboBox]) {
            $control.BackColor = $input
        } elseif ($control -is [System.Windows.Forms.DataGridView]) {
            $control.BackgroundColor = $background
            $control.GridColor = $border
            $control.EnableHeadersVisualStyles = $false
            $control.ColumnHeadersDefaultCellStyle.BackColor = $header
            $control.ColumnHeadersDefaultCellStyle.ForeColor = $text
            $control.ColumnHeadersDefaultCellStyle.SelectionBackColor = $header
            $control.ColumnHeadersDefaultCellStyle.SelectionForeColor = $text
            $control.DefaultCellStyle.BackColor = $input
            $control.DefaultCellStyle.ForeColor = $text
            $control.DefaultCellStyle.SelectionBackColor = $selection
            $control.DefaultCellStyle.SelectionForeColor = $text
            $control.AlternatingRowsDefaultCellStyle.BackColor = $surface
            $control.AlternatingRowsDefaultCellStyle.ForeColor = $text
        }

        try {
            [void][OpenSalamander.Extensions.DarkModeNativeMethods]::SetWindowTheme(
                $control.Handle, 'DarkMode_Explorer', $null)
        } catch {}
        foreach ($child in $control.Controls) {
            $controls.Push($child)
        }
    }

    $enable = 1
    try {
        $result =
            [OpenSalamander.Extensions.DarkModeNativeMethods]::DwmSetWindowAttribute(
                $Form.Handle, 20, [ref]$enable, 4)
        if ($result -ne 0) {
            [void][OpenSalamander.Extensions.DarkModeNativeMethods]::DwmSetWindowAttribute(
                $Form.Handle, 19, [ref]$enable, 4)
        }
    } catch {}
}

function Get-NavigatorStrings {
    param([string]$Locale)

    $candidates = New-Object System.Collections.Generic.List[string]
    if (-not [string]::IsNullOrWhiteSpace($Locale)) {
        $candidates.Add($Locale)
        $primary = ($Locale -split '-')[0]
        if ($primary -ne $Locale) { $candidates.Add($primary) }
    }
    $candidates.Add('en')

    foreach ($candidate in $candidates) {
        $path = Join-Path $PSScriptRoot "locales\$candidate.json"
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            return (Get-Content -LiteralPath $path -Raw -Encoding UTF8 | ConvertFrom-Json).strings
        }
    }
    throw 'The English localization resource is missing.'
}

function Invoke-NavigatorGit {
    param(
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [switch]$AllowFailure
    )

    $lines = @(& $script:GitExecutable -c 'core.quotepath=false' -C $WorkingDirectory @Arguments 2>&1)
    $exitCode = $LASTEXITCODE
    $text = ($lines | ForEach-Object { [string]$_ }) -join "`n"
    if ($exitCode -ne 0 -and -not $AllowFailure) {
        if ([string]::IsNullOrWhiteSpace($text)) {
            $text = $script:Strings.gitFailed
        }
        throw $text
    }
    return [pscustomobject]@{
        ExitCode = $exitCode
        Text = $text.Trim()
        Lines = @($lines | ForEach-Object { [string]$_ })
    }
}

function ConvertFrom-GitWorktreePorcelain {
    param([string[]]$Lines)

    $result = New-Object System.Collections.Generic.List[object]
    $record = $null
    foreach ($line in @($Lines) + '') {
        if ([string]::IsNullOrEmpty($line)) {
            if ($null -ne $record -and -not [string]::IsNullOrWhiteSpace($record.Path)) {
                $result.Add($record)
            }
            $record = $null
            continue
        }
        if ($line.StartsWith('worktree ')) {
            if ($null -ne $record -and -not [string]::IsNullOrWhiteSpace($record.Path)) {
                $result.Add($record)
            }
            $record = [pscustomobject]@{
                Path = $line.Substring(9)
                Head = ''
                Branch = ''
                Detached = $false
                Bare = $false
                Locked = $false
                Prunable = $false
                Status = ''
                Upstream = ''
                Ahead = 0
                Behind = 0
            }
            continue
        }
        if ($null -eq $record) { continue }
        if ($line.StartsWith('HEAD ')) { $record.Head = $line.Substring(5); continue }
        if ($line.StartsWith('branch ')) {
            $record.Branch = $line.Substring(7) -replace '^refs/heads/', ''
            continue
        }
        if ($line -eq 'detached') { $record.Detached = $true; continue }
        if ($line -eq 'bare') { $record.Bare = $true; continue }
        if ($line.StartsWith('locked')) { $record.Locked = $true; continue }
        if ($line.StartsWith('prunable')) { $record.Prunable = $true; continue }
    }
    return $result.ToArray()
}

function Get-WorktreeState {
    param([object]$Worktree)

    if (-not (Test-Path -LiteralPath $Worktree.Path -PathType Container)) {
        $Worktree.Status = $script:Strings.missing
        return
    }
    $status = Invoke-NavigatorGit -WorkingDirectory $Worktree.Path `
        -Arguments @('status', '--porcelain=v1', '--untracked-files=normal') -AllowFailure
    if ($status.ExitCode -ne 0) {
        $Worktree.Status = $script:Strings.unavailable
        return
    }
    $Worktree.Status = if ([string]::IsNullOrWhiteSpace($status.Text)) {
        $script:Strings.clean
    } else {
        $script:Strings.dirty
    }

    $upstream = Invoke-NavigatorGit -WorkingDirectory $Worktree.Path `
        -Arguments @('rev-parse', '--abbrev-ref', '--symbolic-full-name', '@{upstream}') -AllowFailure
    if ($upstream.ExitCode -eq 0 -and -not [string]::IsNullOrWhiteSpace($upstream.Text)) {
        $Worktree.Upstream = $upstream.Text
        $counts = Invoke-NavigatorGit -WorkingDirectory $Worktree.Path `
            -Arguments @('rev-list', '--left-right', '--count', 'HEAD...@{upstream}') -AllowFailure
        if ($counts.ExitCode -eq 0) {
            $parts = @($counts.Text -split '\s+')
            if ($parts.Count -ge 2) {
                $Worktree.Ahead = [int]$parts[0]
                $Worktree.Behind = [int]$parts[1]
            }
        }
    }
}

function Get-NavigatorWorktrees {
    param([string]$RepositoryRoot)

    $listed = Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('worktree', 'list', '--porcelain')
    $worktrees = @(ConvertFrom-GitWorktreePorcelain -Lines $listed.Lines)
    $progress = $null
    try {
        if ($worktrees.Count -gt 0) {
            $progress = $Salamander.ui.Progress(
                $script:Strings.loading, $worktrees.Count, $false, $false, $true)
        }
        for ($index = 0; $index -lt $worktrees.Count; $index++) {
            if ($null -ne $progress -and $progress.IsCancelled()) { break }
            Get-WorktreeState -Worktree $worktrees[$index]
            if ($null -ne $progress) {
                [void]$progress.Update(
                    $index + 1, $worktrees.Count, $worktrees[$index].Path)
            }
        }
    }
    finally {
        if ($null -ne $progress) { $progress.Close() }
    }
    return $worktrees
}

function Get-NavigatorBranches {
    param(
        [string]$RepositoryRoot,
        [object[]]$Worktrees
    )

    $checkedOut = @{}
    foreach ($worktree in $Worktrees) {
        if (-not [string]::IsNullOrWhiteSpace($worktree.Branch)) {
            $checkedOut[$worktree.Branch] = $worktree.Path
        }
    }

    $currentResult = Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('symbolic-ref', '--quiet', '--short', 'HEAD') -AllowFailure
    $currentBranch = if ($currentResult.ExitCode -eq 0) {
        $currentResult.Text
    } else {
        ''
    }

    $listed = Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @(
            'for-each-ref',
            '--format=%(refname)%09%(refname:short)%09%(upstream:short)',
            'refs/heads',
            'refs/remotes'
        )
    $branches = New-Object System.Collections.Generic.List[object]
    foreach ($line in $listed.Lines) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        $parts = @($line -split "`t", 3)
        if ($parts.Count -lt 2) { continue }

        $reference = $parts[0]
        $name = $parts[1]
        $upstream = if ($parts.Count -ge 3) { $parts[2] } else { '' }
        $isRemote = $reference.StartsWith('refs/remotes/')
        if ($isRemote -and $name.EndsWith('/HEAD')) { continue }

        $worktreePath = ''
        if (-not $isRemote -and $checkedOut.ContainsKey($name)) {
            $worktreePath = [string]$checkedOut[$name]
        }
        $branches.Add([pscustomobject]@{
            Name = $name
            Reference = $reference
            IsRemote = $isRemote
            IsCurrent = (-not $isRemote -and $name -eq $currentBranch)
            Upstream = $upstream
            WorktreePath = $worktreePath
        })
    }

    return @($branches | Sort-Object IsRemote, Name)
}

function Get-WorktreeDisplayBranch {
    param([object]$Worktree)
    if ($Worktree.Detached) {
        if ($Worktree.Head.Length -gt 10) { return $Worktree.Head.Substring(0, 10) }
        return $Worktree.Head
    }
    if ($Worktree.Bare) { return $script:Strings.bare }
    return $Worktree.Branch
}

function Invoke-NavigatorUiAction {
    param([scriptblock]$Action)

    try {
        & $Action
    }
    catch {
        [void]$Salamander.ui.MessageBox(
            $_.Exception.Message, $script:Strings.title, 'OK', 'Error')
    }
}

function Refresh-NavigatorSourcePanel {
    try {
        [void]$Salamander.source_side.Refresh($true, $false)
    }
    catch {
        # A Git operation has already succeeded; panel refresh is best effort.
    }
}

function Open-NavigatorTab {
    param(
        [object]$Side,
        [string]$Path
    )

    $result = $Side.CreateTab($Path)
    if ($null -ne $result) {
        $createdProperty = $result.PSObject.Properties['created']
        if ($null -ne $createdProperty -and -not [bool]$createdProperty.Value) {
            throw $script:Strings.openFailed
        }
    }
}

function Show-CreateWorktreeDialog {
    param([string]$RepositoryRoot)

    $form = New-Object System.Windows.Forms.Form
    $form.Text = $script:Strings.createTitle
    $form.StartPosition = 'CenterParent'
    $form.ClientSize = New-Object System.Drawing.Size(620, 170)
    $form.MinimizeBox = $false
    $form.MaximizeBox = $false
    $form.FormBorderStyle = 'FixedDialog'

    $branchLabel = New-Object System.Windows.Forms.Label
    $branchLabel.Text = $script:Strings.branch
    $branchLabel.SetBounds(12, 16, 100, 22)
    $form.Controls.Add($branchLabel)
    $branch = New-Object System.Windows.Forms.TextBox
    $branch.SetBounds(120, 13, 480, 24)
    $form.Controls.Add($branch)

    $pathLabel = New-Object System.Windows.Forms.Label
    $pathLabel.Text = $script:Strings.path
    $pathLabel.SetBounds(12, 52, 100, 22)
    $form.Controls.Add($pathLabel)
    $path = New-Object System.Windows.Forms.TextBox
    $path.SetBounds(120, 49, 390, 24)
    $form.Controls.Add($path)
    $browse = New-Object System.Windows.Forms.Button
    $browse.Text = $script:Strings.browse
    $browse.SetBounds(518, 47, 82, 28)
    $form.Controls.Add($browse)

    $newBranch = New-Object System.Windows.Forms.CheckBox
    $newBranch.Text = $script:Strings.newBranch
    $newBranch.Checked = $true
    $newBranch.SetBounds(120, 84, 300, 24)
    $form.Controls.Add($newBranch)

    $ok = New-Object System.Windows.Forms.Button
    $ok.Text = $script:Strings.create
    $ok.DialogResult = [System.Windows.Forms.DialogResult]::OK
    $ok.SetBounds(410, 124, 90, 30)
    $form.Controls.Add($ok)
    $cancel = New-Object System.Windows.Forms.Button
    $cancel.Text = $script:Strings.cancel
    $cancel.DialogResult = [System.Windows.Forms.DialogResult]::Cancel
    $cancel.SetBounds(510, 124, 90, 30)
    $form.Controls.Add($cancel)
    $form.AcceptButton = $ok
    $form.CancelButton = $cancel
    Set-ExtensionDarkMode -Form $form

    $branch.Add_TextChanged({
        $safe = $branch.Text -replace '[\\/:*?"<>| ]', '-'
        if (-not [string]::IsNullOrWhiteSpace($safe)) {
            $parent = Split-Path -Parent $RepositoryRoot
            $path.Text = Join-Path $parent $safe
        }
    })
    $browse.Add_Click({
        $initialPath = if (Test-Path -LiteralPath $path.Text) {
            $path.Text
        } else {
            Split-Path -Parent $RepositoryRoot
        }
        $picked = $Salamander.ui.PickFolder(
            $script:Strings.chooseFolder, $initialPath)
        if ($null -ne $picked -and [bool]$picked.selected) {
            $path.Text = [string]$picked.path
        }
    })

    try {
        if ($form.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) {
            return $null
        }
        return [pscustomobject]@{
            Branch = $branch.Text.Trim()
            Path = $path.Text.Trim()
            NewBranch = $newBranch.Checked
        }
    }
    finally {
        $form.Dispose()
    }
}

function New-NavigatorWorktree {
    param([string]$RepositoryRoot)

    $request = Show-CreateWorktreeDialog -RepositoryRoot $RepositoryRoot
    if ($null -eq $request) { return $false }
    if ([string]::IsNullOrWhiteSpace($request.Branch) -or
        [string]::IsNullOrWhiteSpace($request.Path)) {
        [void]$Salamander.ui.MessageBox(
            $script:Strings.required, $script:Strings.title, 'OK', 'Warning')
        return $false
    }
    $checked = Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('check-ref-format', '--branch', $request.Branch) -AllowFailure
    if ($checked.ExitCode -ne 0) {
        [void]$Salamander.ui.MessageBox(
            $script:Strings.invalidBranch, $script:Strings.title, 'OK', 'Warning')
        return $false
    }

    $arguments = @('worktree', 'add')
    if ($request.NewBranch) { $arguments += @('-b', $request.Branch) }
    $arguments += @('--', $request.Path)
    if (-not $request.NewBranch) { $arguments += $request.Branch }
    Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot -Arguments $arguments | Out-Null
    [void]$Salamander.ui.Notify(
        $script:Strings.created, $script:Strings.title, 3000)
    return $true
}

function Remove-NavigatorWorktree {
    param([string]$RepositoryRoot, [object]$Worktree)

    if ($Worktree.Path -eq $RepositoryRoot) {
        [void]$Salamander.ui.MessageBox(
            $script:Strings.cannotRemoveCurrent, $script:Strings.title, 'OK', 'Warning')
        return $false
    }
    Get-WorktreeState -Worktree $Worktree
    if ($Worktree.Status -ne $script:Strings.clean) {
        [void]$Salamander.ui.MessageBox(
            $script:Strings.cannotRemoveDirty, $script:Strings.title, 'OK', 'Warning')
        return $false
    }
    $question = [string]::Format(
        $script:Strings.confirmRemove,
        [Environment]::NewLine,
        $Worktree.Path)
    $answer = $Salamander.ui.MessageBox(
        $question, $script:Strings.title, 'YesNo', 'Warning')
    if ($answer -ne 6) { return $false }
    Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('worktree', 'remove', '--', $Worktree.Path) | Out-Null
    [void]$Salamander.ui.Notify(
        $script:Strings.removed, $script:Strings.title, 3000)
    return $true
}

function Test-NavigatorWorktreeDirty {
    param([string]$RepositoryRoot)

    $status = Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('status', '--porcelain=v1', '--untracked-files=normal')
    return -not [string]::IsNullOrWhiteSpace($status.Text)
}

function Switch-NavigatorBranch {
    param(
        [string]$RepositoryRoot,
        [object]$Branch
    )

    if ($Branch.IsCurrent) { return $false }
    if (-not [string]::IsNullOrWhiteSpace($Branch.WorktreePath) -and
        $Branch.WorktreePath -ne $RepositoryRoot) {
        $message = [string]::Format(
            $script:Strings.branchInWorktree,
            $Branch.Name,
            $Branch.WorktreePath)
        [void]$Salamander.ui.MessageBox(
            $message, $script:Strings.title, 'OK', 'Warning')
        return $false
    }

    if (Test-NavigatorWorktreeDirty -RepositoryRoot $RepositoryRoot) {
        $answer = $Salamander.ui.MessageBox(
            $script:Strings.confirmDirtySwitch,
            $script:Strings.title,
            'YesNo',
            'Warning')
        if ($answer -ne 6) {
            return $false
        }
    }

    $arguments = @('switch')
    if ($Branch.IsRemote) {
        $separator = $Branch.Name.IndexOf('/')
        if ($separator -lt 1 -or $separator -ge ($Branch.Name.Length - 1)) {
            throw $script:Strings.invalidBranch
        }
        $localName = $Branch.Name.Substring($separator + 1)
        $local = Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
            -Arguments @('show-ref', '--verify', '--quiet', "refs/heads/$localName") `
            -AllowFailure
        if ($local.ExitCode -eq 0) {
            $arguments += $localName
        } else {
            $arguments += @('--track', $Branch.Name)
        }
    } else {
        $arguments += $Branch.Name
    }

    Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments $arguments | Out-Null
    Refresh-NavigatorSourcePanel
    [void]$Salamander.ui.Notify(
        ([string]::Format($script:Strings.switched, $Branch.Name)),
        $script:Strings.title,
        3500)
    return $true
}

function Invoke-NavigatorFetch {
    param([string]$RepositoryRoot)

    Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('fetch', '--all', '--prune') | Out-Null
    [void]$Salamander.ui.Notify(
        $script:Strings.fetchComplete, $script:Strings.title, 3500)
}

function Invoke-NavigatorPull {
    param([string]$RepositoryRoot)

    Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('pull', '--ff-only') | Out-Null
    Refresh-NavigatorSourcePanel
    [void]$Salamander.ui.Notify(
        $script:Strings.pullComplete, $script:Strings.title, 3500)
}

function Invoke-NavigatorPush {
    param([string]$RepositoryRoot)

    $upstream = Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('rev-parse', '--abbrev-ref', '--symbolic-full-name', '@{upstream}') `
        -AllowFailure
    if ($upstream.ExitCode -eq 0) {
        Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
            -Arguments @('push') | Out-Null
    } else {
        $branch = Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
            -Arguments @('symbolic-ref', '--quiet', '--short', 'HEAD') -AllowFailure
        if ($branch.ExitCode -ne 0 -or [string]::IsNullOrWhiteSpace($branch.Text)) {
            throw $script:Strings.detachedPush
        }
        $origin = Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
            -Arguments @('remote', 'get-url', 'origin') -AllowFailure
        if ($origin.ExitCode -ne 0) {
            throw $script:Strings.noPushUpstream
        }
        $answer = $Salamander.ui.MessageBox(
            ([string]::Format($script:Strings.confirmSetUpstream, $branch.Text)),
            $script:Strings.title,
            'YesNo',
            'Question')
        if ($answer -ne 6) { return }
        Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
            -Arguments @('push', '--set-upstream', 'origin', $branch.Text) | Out-Null
    }
    [void]$Salamander.ui.Notify(
        $script:Strings.pushComplete, $script:Strings.title, 3500)
}

function Show-NavigatorCommitDialog {
    param(
        [string]$RepositoryRoot,
        [string[]]$StatusLines
    )

    $form = New-Object System.Windows.Forms.Form
    $form.Text = $script:Strings.commitTitle
    $form.StartPosition = 'CenterParent'
    $form.ClientSize = New-Object System.Drawing.Size(660, 430)
    $form.MinimumSize = New-Object System.Drawing.Size(560, 390)
    $form.MinimizeBox = $false
    $form.MaximizeBox = $false

    $changesLabel = New-Object System.Windows.Forms.Label
    $changesLabel.Text = $script:Strings.changes
    $changesLabel.SetBounds(12, 12, 636, 22)
    $form.Controls.Add($changesLabel)

    $changes = New-Object System.Windows.Forms.TextBox
    $changes.Multiline = $true
    $changes.ReadOnly = $true
    $changes.ScrollBars = 'Both'
    $changes.WordWrap = $false
    $changes.Anchor = 'Top,Left,Right'
    $changes.SetBounds(12, 36, 636, 150)
    $changes.Text = $StatusLines -join [Environment]::NewLine
    $form.Controls.Add($changes)

    $messageLabel = New-Object System.Windows.Forms.Label
    $messageLabel.Text = $script:Strings.commitMessage
    $messageLabel.SetBounds(12, 198, 636, 22)
    $form.Controls.Add($messageLabel)

    $message = New-Object System.Windows.Forms.TextBox
    $message.Multiline = $true
    $message.ScrollBars = 'Vertical'
    $message.Anchor = 'Top,Bottom,Left,Right'
    $message.SetBounds(12, 222, 636, 120)
    $form.Controls.Add($message)

    $stageAll = New-Object System.Windows.Forms.CheckBox
    $stageAll.Text = $script:Strings.stageAll
    $stageAll.Checked = $true
    $stageAll.Anchor = 'Bottom,Left'
    $stageAll.SetBounds(12, 354, 390, 24)
    $form.Controls.Add($stageAll)

    $ok = New-Object System.Windows.Forms.Button
    $ok.Text = $script:Strings.commit
    $ok.DialogResult = [System.Windows.Forms.DialogResult]::OK
    $ok.Anchor = 'Bottom,Right'
    $ok.SetBounds(458, 388, 90, 30)
    $form.Controls.Add($ok)
    $cancel = New-Object System.Windows.Forms.Button
    $cancel.Text = $script:Strings.cancel
    $cancel.DialogResult = [System.Windows.Forms.DialogResult]::Cancel
    $cancel.Anchor = 'Bottom,Right'
    $cancel.SetBounds(558, 388, 90, 30)
    $form.Controls.Add($cancel)
    $form.AcceptButton = $ok
    $form.CancelButton = $cancel
    Set-ExtensionDarkMode -Form $form

    try {
        if ($form.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) {
            return $null
        }
        if ([string]::IsNullOrWhiteSpace($message.Text)) {
            [void]$Salamander.ui.MessageBox(
                $script:Strings.commitMessageRequired,
                $script:Strings.title,
                'OK',
                'Warning')
            return $null
        }
        return [pscustomobject]@{
            Message = $message.Text.Trim()
            StageAll = $stageAll.Checked
        }
    }
    finally {
        $form.Dispose()
    }
}

function Invoke-NavigatorCommit {
    param([string]$RepositoryRoot)

    $status = Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('status', '--porcelain=v1', '--untracked-files=normal')
    if ([string]::IsNullOrWhiteSpace($status.Text)) {
        [void]$Salamander.ui.Notify(
            $script:Strings.nothingToCommit, $script:Strings.title, 3500)
        return $false
    }

    $request = Show-NavigatorCommitDialog `
        -RepositoryRoot $RepositoryRoot `
        -StatusLines $status.Lines
    if ($null -eq $request) { return $false }

    if ($request.StageAll) {
        Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
            -Arguments @('add', '--all') | Out-Null
    }
    $staged = Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('diff', '--cached', '--quiet') -AllowFailure
    if ($staged.ExitCode -eq 0) {
        [void]$Salamander.ui.Notify(
            $script:Strings.noStagedChanges, $script:Strings.title, 3500)
        return $false
    }
    if ($staged.ExitCode -ne 1) {
        throw $script:Strings.gitFailed
    }

    Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('commit', '-m', $request.Message) | Out-Null
    Refresh-NavigatorSourcePanel
    [void]$Salamander.ui.Notify(
        $script:Strings.commitComplete, $script:Strings.title, 3500)
    return $true
}

function Copy-NavigatorReport {
    param([string]$RepositoryRoot, [object[]]$Worktrees)

    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("# $($script:Strings.title)")
    $lines.Add('')
    $lines.Add("$($script:Strings.repository): $RepositoryRoot")
    $lines.Add('')
    foreach ($worktree in $Worktrees) {
        $branch = Get-WorktreeDisplayBranch $worktree
        $sync = if ([string]::IsNullOrWhiteSpace($worktree.Upstream)) {
            $script:Strings.noUpstream
        } else {
            "$($worktree.Upstream), +$($worktree.Ahead)/-$($worktree.Behind)"
        }
        $lines.Add("- **$branch** -- $($worktree.Status) -- $sync")
        $lines.Add("  - $($worktree.Path)")
    }
    [void]$Salamander.clipboard.CopyText(($lines -join "`r`n"), $true)
}

function Show-NavigatorWindow {
    param([string]$RepositoryRoot)

    $form = New-Object System.Windows.Forms.Form
    $form.Text = $script:Strings.title
    $form.StartPosition = 'CenterScreen'
    $form.ClientSize = New-Object System.Drawing.Size(1100, 700)
    $form.MinimumSize = New-Object System.Drawing.Size(900, 620)

    $repository = New-Object System.Windows.Forms.Label
    $repository.Text = "$($script:Strings.repository): $RepositoryRoot"
    $repository.AutoEllipsis = $true
    $repository.Anchor = 'Top,Left,Right'
    $repository.SetBounds(12, 12, 1076, 22)
    $form.Controls.Add($repository)

    $worktreeLabel = New-Object System.Windows.Forms.Label
    $worktreeLabel.Text = $script:Strings.worktrees
    $worktreeLabel.SetBounds(12, 40, 1076, 22)
    $form.Controls.Add($worktreeLabel)

    $worktreeGrid = New-Object System.Windows.Forms.DataGridView
    $worktreeGrid.Anchor = 'Top,Left,Right'
    $worktreeGrid.SetBounds(12, 64, 1076, 220)
    $worktreeGrid.ReadOnly = $true
    $worktreeGrid.AllowUserToAddRows = $false
    $worktreeGrid.AllowUserToDeleteRows = $false
    $worktreeGrid.AllowUserToResizeRows = $false
    $worktreeGrid.AutoSizeColumnsMode = 'Fill'
    $worktreeGrid.MultiSelect = $false
    $worktreeGrid.RowHeadersVisible = $false
    $worktreeGrid.SelectionMode = 'FullRowSelect'
    [void]$worktreeGrid.Columns.Add('branch', $script:Strings.branch)
    [void]$worktreeGrid.Columns.Add('status', $script:Strings.status)
    [void]$worktreeGrid.Columns.Add('sync', $script:Strings.sync)
    [void]$worktreeGrid.Columns.Add('path', $script:Strings.path)
    $worktreeGrid.Columns['branch'].FillWeight = 18
    $worktreeGrid.Columns['status'].FillWeight = 12
    $worktreeGrid.Columns['sync'].FillWeight = 20
    $worktreeGrid.Columns['path'].FillWeight = 50
    $form.Controls.Add($worktreeGrid)

    $worktreeButtonDefinitions = @(
        @('openSource', $script:Strings.openSource),
        @('openTarget', $script:Strings.openTarget),
        @('openBoth', $script:Strings.openBoth),
        @('create', $script:Strings.create),
        @('remove', $script:Strings.remove),
        @('copy', $script:Strings.copyReport),
        @('refresh', $script:Strings.refresh)
    )
    $buttons = @{}
    $x = 12
    foreach ($definition in $worktreeButtonDefinitions) {
        $button = New-Object System.Windows.Forms.Button
        $button.Name = $definition[0]
        $button.Text = $definition[1]
        $button.SetBounds($x, 294, 124, 32)
        $form.Controls.Add($button)
        $buttons[$definition[0]] = $button
        $x += 130
    }

    $branchLabel = New-Object System.Windows.Forms.Label
    $branchLabel.Text = $script:Strings.branches
    $branchLabel.SetBounds(12, 340, 1076, 22)
    $form.Controls.Add($branchLabel)

    $branchGrid = New-Object System.Windows.Forms.DataGridView
    $branchGrid.Anchor = 'Top,Bottom,Left,Right'
    $branchGrid.SetBounds(12, 364, 1076, 270)
    $branchGrid.ReadOnly = $true
    $branchGrid.AllowUserToAddRows = $false
    $branchGrid.AllowUserToDeleteRows = $false
    $branchGrid.AllowUserToResizeRows = $false
    $branchGrid.AutoSizeColumnsMode = 'Fill'
    $branchGrid.MultiSelect = $false
    $branchGrid.RowHeadersVisible = $false
    $branchGrid.SelectionMode = 'FullRowSelect'
    [void]$branchGrid.Columns.Add('branch', $script:Strings.branch)
    [void]$branchGrid.Columns.Add('kind', $script:Strings.kind)
    [void]$branchGrid.Columns.Add('state', $script:Strings.state)
    [void]$branchGrid.Columns.Add('upstream', $script:Strings.sync)
    [void]$branchGrid.Columns.Add('worktree', $script:Strings.worktree)
    $branchGrid.Columns['branch'].FillWeight = 28
    $branchGrid.Columns['kind'].FillWeight = 12
    $branchGrid.Columns['state'].FillWeight = 14
    $branchGrid.Columns['upstream'].FillWeight = 20
    $branchGrid.Columns['worktree'].FillWeight = 36
    $form.Controls.Add($branchGrid)

    $branchButtonDefinitions = @(
        @('switch', $script:Strings.switch),
        @('fetch', $script:Strings.fetch),
        @('pull', $script:Strings.pull),
        @('push', $script:Strings.push),
        @('commit', $script:Strings.commit)
    )
    $x = 12
    foreach ($definition in $branchButtonDefinitions) {
        $button = New-Object System.Windows.Forms.Button
        $button.Name = $definition[0]
        $button.Text = $definition[1]
        $button.Anchor = 'Bottom,Left'
        $button.SetBounds($x, 646, 112, 32)
        $form.Controls.Add($button)
        $buttons[$definition[0]] = $button
        $x += 118
    }

    $close = New-Object System.Windows.Forms.Button
    $close.Text = $script:Strings.close
    $close.Anchor = 'Bottom,Right'
    $close.SetBounds(976, 646, 112, 32)
    $form.Controls.Add($close)

    $script:NavigatorWorktrees = @()
    $script:NavigatorBranches = @()
    $refreshAll = {
        $script:NavigatorWorktrees = @(
            Get-NavigatorWorktrees -RepositoryRoot $RepositoryRoot)
        $worktreeGrid.Rows.Clear()
        foreach ($worktree in $script:NavigatorWorktrees) {
            $sync = if ([string]::IsNullOrWhiteSpace($worktree.Upstream)) {
                $script:Strings.noUpstream
            } else {
                "$($worktree.Upstream)  +$($worktree.Ahead)/-$($worktree.Behind)"
            }
            $row = $worktreeGrid.Rows.Add(
                (Get-WorktreeDisplayBranch $worktree),
                $worktree.Status,
                $sync,
                $worktree.Path)
            $worktreeGrid.Rows[$row].Tag = $worktree
        }
        if ($worktreeGrid.Rows.Count -gt 0) {
            $worktreeGrid.Rows[0].Selected = $true
        }

        $script:NavigatorBranches = @(
            Get-NavigatorBranches `
                -RepositoryRoot $RepositoryRoot `
                -Worktrees $script:NavigatorWorktrees)
        $branchGrid.Rows.Clear()
        foreach ($branch in $script:NavigatorBranches) {
            $kind = if ($branch.IsRemote) {
                $script:Strings.remote
            } else {
                $script:Strings.local
            }
            $state = if ($branch.IsCurrent) {
                $script:Strings.current
            } elseif (-not [string]::IsNullOrWhiteSpace($branch.WorktreePath)) {
                $script:Strings.checkedOut
            } else {
                ''
            }
            $row = $branchGrid.Rows.Add(
                $branch.Name,
                $kind,
                $state,
                $branch.Upstream,
                $branch.WorktreePath)
            $branchGrid.Rows[$row].Tag = $branch
            if ($branch.IsCurrent) {
                $branchGrid.Rows[$row].DefaultCellStyle.Font =
                    New-Object System.Drawing.Font(
                        $branchGrid.Font,
                        [System.Drawing.FontStyle]::Bold)
            }
        }
        if ($branchGrid.Rows.Count -gt 0) {
            $currentRow = $null
            foreach ($row in $branchGrid.Rows) {
                if ($row.Tag.IsCurrent) {
                    $currentRow = $row
                    break
                }
            }
            if ($null -eq $currentRow) { $currentRow = $branchGrid.Rows[0] }
            $currentRow.Selected = $true
            $branchGrid.CurrentCell = $currentRow.Cells[0]
        }
    }
    $selectedWorktree = {
        if ($worktreeGrid.SelectedRows.Count -eq 0) { return $null }
        return $worktreeGrid.SelectedRows[0].Tag
    }
    $selectedBranch = {
        if ($branchGrid.SelectedRows.Count -eq 0) { return $null }
        return $branchGrid.SelectedRows[0].Tag
    }

    $buttons.openSource.Add_Click({
        Invoke-NavigatorUiAction {
            $item = & $selectedWorktree
            if ($null -ne $item) {
                Open-NavigatorTab -Side $Salamander.source_side -Path $item.Path
            }
        }
    })
    $buttons.openTarget.Add_Click({
        Invoke-NavigatorUiAction {
            $item = & $selectedWorktree
            if ($null -ne $item) {
                Open-NavigatorTab -Side $Salamander.target_side -Path $item.Path
            }
        }
    })
    $buttons.openBoth.Add_Click({
        Invoke-NavigatorUiAction {
            $item = & $selectedWorktree
            if ($null -ne $item) {
                Open-NavigatorTab `
                    -Side $Salamander.source_side `
                    -Path $RepositoryRoot
                Open-NavigatorTab `
                    -Side $Salamander.target_side `
                    -Path $item.Path
            }
        }
    })
    $buttons.create.Add_Click({
        Invoke-NavigatorUiAction {
            if (New-NavigatorWorktree -RepositoryRoot $RepositoryRoot) {
                & $refreshAll
            }
        }
    })
    $buttons.remove.Add_Click({
        Invoke-NavigatorUiAction {
            $item = & $selectedWorktree
            if ($null -eq $item) { return }
            if (Remove-NavigatorWorktree -RepositoryRoot $RepositoryRoot -Worktree $item) {
                & $refreshAll
            }
        }
    })
    $buttons.copy.Add_Click({
        Invoke-NavigatorUiAction {
            Copy-NavigatorReport -RepositoryRoot $RepositoryRoot `
                -Worktrees $script:NavigatorWorktrees
        }
    })
    $buttons.refresh.Add_Click({
        Invoke-NavigatorUiAction { & $refreshAll }
    })
    $buttons.switch.Add_Click({
        Invoke-NavigatorUiAction {
            $branch = & $selectedBranch
            if ($null -ne $branch -and
                (Switch-NavigatorBranch `
                    -RepositoryRoot $RepositoryRoot `
                    -Branch $branch)) {
                & $refreshAll
            }
        }
    })
    $buttons.fetch.Add_Click({
        Invoke-NavigatorUiAction {
            Invoke-NavigatorFetch -RepositoryRoot $RepositoryRoot
            & $refreshAll
        }
    })
    $buttons.pull.Add_Click({
        Invoke-NavigatorUiAction {
            Invoke-NavigatorPull -RepositoryRoot $RepositoryRoot
            & $refreshAll
        }
    })
    $buttons.push.Add_Click({
        Invoke-NavigatorUiAction {
            Invoke-NavigatorPush -RepositoryRoot $RepositoryRoot
            & $refreshAll
        }
    })
    $buttons.commit.Add_Click({
        Invoke-NavigatorUiAction {
            if (Invoke-NavigatorCommit -RepositoryRoot $RepositoryRoot) {
                & $refreshAll
            }
        }
    })
    $close.Add_Click({ $form.Close() })
    $worktreeGrid.Add_CellDoubleClick({
        Invoke-NavigatorUiAction {
            $item = & $selectedWorktree
            if ($null -ne $item) {
                Open-NavigatorTab -Side $Salamander.source_side -Path $item.Path
            }
        }
    })
    $branchGrid.Add_CellDoubleClick({
        Invoke-NavigatorUiAction {
            $branch = & $selectedBranch
            if ($null -ne $branch -and
                (Switch-NavigatorBranch `
                    -RepositoryRoot $RepositoryRoot `
                    -Branch $branch)) {
                & $refreshAll
            }
        }
    })
    Set-ExtensionDarkMode -Form $form

    try {
        & $refreshAll
        [void]$form.ShowDialog()
    }
    finally {
        $form.Dispose()
    }
}

if ($null -eq (Get-Variable -Name Salamander -ErrorAction SilentlyContinue)) {
    return
}

if ($Salamander.command_handler -eq 'open' -or
    $Salamander.command_handler -eq 'commit') {
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
    [System.Windows.Forms.Application]::EnableVisualStyles()

    try {
        $language = $Salamander.application.Language()
        $script:Strings = Get-NavigatorStrings -Locale $language.locale
        $appearance = $Salamander.application.Appearance()
        $darkProperty = $appearance.PSObject.Properties['windowsDarkMode']
        $script:UseWindowsDarkMode =
            $null -ne $darkProperty -and [bool]$darkProperty.Value
        $git = Get-Command git.exe -CommandType Application -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($null -eq $git) {
            [void]$Salamander.ui.Notify(
                $script:Strings.gitMissing, $script:Strings.title, 5000)
            return
        }
        $script:GitExecutable = $git.Source
        $context = $Salamander.source_side.Context()
        $rootResult = Invoke-NavigatorGit -WorkingDirectory $context.path `
            -Arguments @('rev-parse', '--show-toplevel') -AllowFailure
        if ($rootResult.ExitCode -ne 0 -or [string]::IsNullOrWhiteSpace($rootResult.Text)) {
            [void]$Salamander.ui.Notify(
                $script:Strings.notRepository, $script:Strings.title, 5000)
            return
        }
        if ($Salamander.command_handler -eq 'commit') {
            [void](Invoke-NavigatorCommit -RepositoryRoot $rootResult.Text)
        } else {
            Show-NavigatorWindow -RepositoryRoot $rootResult.Text
        }
    }
    catch {
        if ($null -ne (Get-Variable -Name Strings -Scope Script -ErrorAction SilentlyContinue)) {
            $title = $script:Strings.title
        } else {
            $title = 'Git Worktree Navigator'
        }
        [void]$Salamander.ui.MessageBox(
            $_.Exception.Message, $title, 'OK', 'Error')
    }
}
