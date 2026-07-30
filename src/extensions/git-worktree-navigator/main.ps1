Set-StrictMode -Version 2.0

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

function Get-WorktreeDisplayBranch {
    param([object]$Worktree)
    if ($Worktree.Detached) {
        if ($Worktree.Head.Length -gt 10) { return $Worktree.Head.Substring(0, 10) }
        return $Worktree.Head
    }
    if ($Worktree.Bare) { return $script:Strings.bare }
    return $Worktree.Branch
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

    $branch.Add_TextChanged({
        $safe = $branch.Text -replace '[\\/:*?"<>| ]', '-'
        if (-not [string]::IsNullOrWhiteSpace($safe)) {
            $parent = Split-Path -Parent $RepositoryRoot
            $path.Text = Join-Path $parent $safe
        }
    })
    $browse.Add_Click({
        $picker = New-Object System.Windows.Forms.FolderBrowserDialog
        $picker.Description = $script:Strings.chooseFolder
        $picker.SelectedPath = if (Test-Path -LiteralPath $path.Text) {
            $path.Text
        } else {
            Split-Path -Parent $RepositoryRoot
        }
        if ($picker.ShowDialog($form) -eq [System.Windows.Forms.DialogResult]::OK) {
            $path.Text = $picker.SelectedPath
        }
        $picker.Dispose()
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
        [void][System.Windows.Forms.MessageBox]::Show(
            $script:Strings.required, $script:Strings.title, 'OK', 'Warning')
        return $false
    }
    $checked = Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('check-ref-format', '--branch', $request.Branch) -AllowFailure
    if ($checked.ExitCode -ne 0) {
        [void][System.Windows.Forms.MessageBox]::Show(
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
        [void][System.Windows.Forms.MessageBox]::Show(
            $script:Strings.cannotRemoveCurrent, $script:Strings.title, 'OK', 'Warning')
        return $false
    }
    Get-WorktreeState -Worktree $Worktree
    if ($Worktree.Status -ne $script:Strings.clean) {
        [void][System.Windows.Forms.MessageBox]::Show(
            $script:Strings.cannotRemoveDirty, $script:Strings.title, 'OK', 'Warning')
        return $false
    }
    $question = [string]::Format(
        $script:Strings.confirmRemove,
        [Environment]::NewLine,
        $Worktree.Path)
    $answer = [System.Windows.Forms.MessageBox]::Show(
        $question, $script:Strings.title, 'YesNo', 'Warning')
    if ($answer -ne [System.Windows.Forms.DialogResult]::Yes) { return $false }
    Invoke-NavigatorGit -WorkingDirectory $RepositoryRoot `
        -Arguments @('worktree', 'remove', '--', $Worktree.Path) | Out-Null
    [void]$Salamander.ui.Notify(
        $script:Strings.removed, $script:Strings.title, 3000)
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
    $form.ClientSize = New-Object System.Drawing.Size(980, 520)
    $form.MinimumSize = New-Object System.Drawing.Size(800, 420)

    $repository = New-Object System.Windows.Forms.Label
    $repository.Text = "$($script:Strings.repository): $RepositoryRoot"
    $repository.AutoEllipsis = $true
    $repository.Anchor = 'Top,Left,Right'
    $repository.SetBounds(12, 12, 956, 22)
    $form.Controls.Add($repository)

    $grid = New-Object System.Windows.Forms.DataGridView
    $grid.Anchor = 'Top,Bottom,Left,Right'
    $grid.SetBounds(12, 40, 956, 420)
    $grid.ReadOnly = $true
    $grid.AllowUserToAddRows = $false
    $grid.AllowUserToDeleteRows = $false
    $grid.AllowUserToResizeRows = $false
    $grid.AutoSizeColumnsMode = 'Fill'
    $grid.MultiSelect = $false
    $grid.RowHeadersVisible = $false
    $grid.SelectionMode = 'FullRowSelect'
    [void]$grid.Columns.Add('branch', $script:Strings.branch)
    [void]$grid.Columns.Add('status', $script:Strings.status)
    [void]$grid.Columns.Add('sync', $script:Strings.sync)
    [void]$grid.Columns.Add('path', $script:Strings.path)
    $grid.Columns['branch'].FillWeight = 18
    $grid.Columns['status'].FillWeight = 12
    $grid.Columns['sync'].FillWeight = 20
    $grid.Columns['path'].FillWeight = 50
    $form.Controls.Add($grid)

    $buttonDefinitions = @(
        @('openSource', $script:Strings.openSource),
        @('openTarget', $script:Strings.openTarget),
        @('openBoth', $script:Strings.openBoth),
        @('create', $script:Strings.create),
        @('remove', $script:Strings.remove),
        @('copy', $script:Strings.copyReport),
        @('refresh', $script:Strings.refresh),
        @('close', $script:Strings.close)
    )
    $buttons = @{}
    $x = 12
    foreach ($definition in $buttonDefinitions) {
        $button = New-Object System.Windows.Forms.Button
        $button.Name = $definition[0]
        $button.Text = $definition[1]
        $button.Anchor = 'Bottom,Left'
        $button.SetBounds($x, 474, 112, 32)
        $form.Controls.Add($button)
        $buttons[$definition[0]] = $button
        $x += 118
    }
    $buttons.close.Anchor = 'Bottom,Right'
    $buttons.close.Left = $form.ClientSize.Width - 124

    $script:NavigatorWorktrees = @()
    $refreshGrid = {
        $script:NavigatorWorktrees = @(
            Get-NavigatorWorktrees -RepositoryRoot $RepositoryRoot)
        $grid.Rows.Clear()
        foreach ($worktree in $script:NavigatorWorktrees) {
            $sync = if ([string]::IsNullOrWhiteSpace($worktree.Upstream)) {
                $script:Strings.noUpstream
            } else {
                "$($worktree.Upstream)  +$($worktree.Ahead)/-$($worktree.Behind)"
            }
            $row = $grid.Rows.Add(
                (Get-WorktreeDisplayBranch $worktree),
                $worktree.Status,
                $sync,
                $worktree.Path)
            $grid.Rows[$row].Tag = $worktree
        }
        if ($grid.Rows.Count -gt 0) { $grid.Rows[0].Selected = $true }
    }
    $selected = {
        if ($grid.SelectedRows.Count -eq 0) { return $null }
        return $grid.SelectedRows[0].Tag
    }

    $buttons.openSource.Add_Click({
        $item = & $selected
        if ($null -ne $item) { [void]$Salamander.source_side.CreateTab($item.Path) }
    })
    $buttons.openTarget.Add_Click({
        $item = & $selected
        if ($null -ne $item) { [void]$Salamander.target_side.CreateTab($item.Path) }
    })
    $buttons.openBoth.Add_Click({
        $item = & $selected
        if ($null -ne $item) {
            [void]$Salamander.source_side.CreateTab($RepositoryRoot)
            [void]$Salamander.target_side.CreateTab($item.Path)
        }
    })
    $buttons.create.Add_Click({
        try {
            if (New-NavigatorWorktree -RepositoryRoot $RepositoryRoot) {
                & $refreshGrid
            }
        } catch {
            [void][System.Windows.Forms.MessageBox]::Show(
                $_.Exception.Message, $script:Strings.title, 'OK', 'Error')
        }
    })
    $buttons.remove.Add_Click({
        $item = & $selected
        if ($null -eq $item) { return }
        try {
            if (Remove-NavigatorWorktree -RepositoryRoot $RepositoryRoot -Worktree $item) {
                & $refreshGrid
            }
        } catch {
            [void][System.Windows.Forms.MessageBox]::Show(
                $_.Exception.Message, $script:Strings.title, 'OK', 'Error')
        }
    })
    $buttons.copy.Add_Click({
        Copy-NavigatorReport -RepositoryRoot $RepositoryRoot `
            -Worktrees $script:NavigatorWorktrees
    })
    $buttons.refresh.Add_Click({ & $refreshGrid })
    $buttons.close.Add_Click({ $form.Close() })
    $grid.Add_CellDoubleClick({
        $item = & $selected
        if ($null -ne $item) {
            [void]$Salamander.source_side.CreateTab($item.Path)
        }
    })

    try {
        & $refreshGrid
        [void]$form.ShowDialog()
    }
    finally {
        $form.Dispose()
    }
}

if ($null -eq (Get-Variable -Name Salamander -ErrorAction SilentlyContinue)) {
    return
}

if ($Salamander.command_handler -eq 'open') {
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
    [System.Windows.Forms.Application]::EnableVisualStyles()

    try {
        $language = $Salamander.application.Language()
        $script:Strings = Get-NavigatorStrings -Locale $language.locale
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
        Show-NavigatorWindow -RepositoryRoot $rootResult.Text
    }
    catch {
        if ($null -ne (Get-Variable -Name Strings -Scope Script -ErrorAction SilentlyContinue)) {
            $title = $script:Strings.title
        } else {
            $title = 'Git Worktree Navigator'
        }
        [void][System.Windows.Forms.MessageBox]::Show(
            $_.Exception.Message, $title, 'OK', 'Error')
    }
}
