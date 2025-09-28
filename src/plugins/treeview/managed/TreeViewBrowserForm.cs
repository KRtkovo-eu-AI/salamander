// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#nullable enable

using System;
using System.Drawing;
using System.Globalization;
using System.Windows.Forms;
using Raccoom.Windows.Forms;

namespace OpenSalamander.TreeViewBrowser;

internal sealed class TreeViewBrowserForm : Form
{
    private readonly TreeViewFolderBrowser _folderBrowser;
    private readonly ListBox _selectionList;
    private readonly ToolStripStatusLabel _statusLabel;
    private readonly ToolStripComboBox _rootCombo;
    private readonly ToolStripButton _showVirtualButton;
    private readonly SplitContainer _splitContainer;
    private readonly TreeStrategyShell32Provider _shellProvider;
    private bool _initializingRoot;
    private string _initialPath;

    public TreeViewBrowserForm(string? initialPath)
    {
        _initialPath = initialPath ?? string.Empty;

        Text = "Tree View Browser";
        StartPosition = FormStartPosition.CenterParent;
        MinimumSize = new Size(720, 480);

        _shellProvider = new TreeStrategyShell32Provider
        {
            EnableContextMenu = true,
            ShowAllShellObjects = true,
            ShowFiles = false,
            RootFolder = Raccoom.Win32.ShellAPI.CSIDL.DRIVES,
        };

        _folderBrowser = new TreeViewFolderBrowser
        {
            Dock = DockStyle.Fill,
            CheckBoxBehaviorMode = CheckBoxBehaviorMode.RecursiveChecked,
        };
        _folderBrowser.DataSource = _shellProvider;
        _folderBrowser.SelectedDirectoriesChanged += OnSelectedDirectoriesChanged;
        _folderBrowser.AfterSelect += OnAfterSelect;
        _folderBrowser.KeyUp += OnFolderBrowserKeyUp;

        _selectionList = new ListBox
        {
            Dock = DockStyle.Fill,
        };

        var selectionGroup = new GroupBox
        {
            Dock = DockStyle.Fill,
            Text = "Checked directories",
        };
        selectionGroup.Controls.Add(_selectionList);

        _splitContainer = new SplitContainer
        {
            Dock = DockStyle.Fill,
            Orientation = Orientation.Vertical,
            Panel2MinSize = 180,
            Panel1MinSize = 220,
        };
        _splitContainer.Panel1.Controls.Add(_folderBrowser);
        _splitContainer.Panel2.Controls.Add(selectionGroup);

        var statusStrip = new StatusStrip();
        _statusLabel = new ToolStripStatusLabel
        {
            Spring = true,
            TextAlign = ContentAlignment.MiddleLeft,
            Text = "Ready.",
        };
        statusStrip.Items.Add(_statusLabel);

        var toolStrip = new ToolStrip
        {
            GripStyle = ToolStripGripStyle.Hidden,
            RenderMode = ToolStripRenderMode.System,
        };

        toolStrip.Items.Add(new ToolStripLabel("Root:"));

        _rootCombo = new ToolStripComboBox
        {
            DropDownStyle = ComboBoxStyle.DropDownList,
            AutoSize = false,
            Width = 220,
        };
        _rootCombo.SelectedIndexChanged += OnRootComboSelectedIndexChanged;
        _rootCombo.Items.Add(new RootFolderOption("Desktop", Raccoom.Win32.ShellAPI.CSIDL.DESKTOP));
        _rootCombo.Items.Add(new RootFolderOption("Computer", Raccoom.Win32.ShellAPI.CSIDL.DRIVES));
        _rootCombo.Items.Add(new RootFolderOption("Documents", Raccoom.Win32.ShellAPI.CSIDL.PERSONAL));
        _rootCombo.Items.Add(new RootFolderOption("User Profile", Raccoom.Win32.ShellAPI.CSIDL.PROFILE));
        toolStrip.Items.Add(_rootCombo);

        toolStrip.Items.Add(new ToolStripSeparator());

        var refreshButton = new ToolStripButton("Refresh")
        {
            DisplayStyle = ToolStripItemDisplayStyle.Text,
        };
        refreshButton.Click += (_, _) => RefreshTree(GetSelectedNodePath());
        toolStrip.Items.Add(refreshButton);

        _showVirtualButton = new ToolStripButton("Virtual Folders")
        {
            Checked = _shellProvider.ShowAllShellObjects,
            CheckOnClick = true,
            DisplayStyle = ToolStripItemDisplayStyle.Text,
        };
        _showVirtualButton.CheckedChanged += (_, _) => ToggleVirtualFolders();
        toolStrip.Items.Add(_showVirtualButton);

        Controls.Add(_splitContainer);
        Controls.Add(statusStrip);
        Controls.Add(toolStrip);

        toolStrip.Dock = DockStyle.Top;
        statusStrip.Dock = DockStyle.Bottom;

        Shown += OnShown;
        Resize += OnResize;
    }

    private void OnShown(object? sender, EventArgs e)
    {
        _initializingRoot = true;
        try
        {
            _rootCombo.SelectedIndex = 1; // Computer by default
        }
        finally
        {
            _initializingRoot = false;
        }

        AdjustSplitterForCurrentWidth();

        RefreshTree(string.IsNullOrWhiteSpace(_initialPath) ? null : _initialPath);
        _initialPath = string.Empty;
    }

    private void OnResize(object? sender, EventArgs e)
    {
        AdjustSplitterForCurrentWidth();
    }

    private void AdjustSplitterForCurrentWidth()
    {
        int width = _splitContainer.ClientSize.Width;
        if (width <= 0)
        {
            return;
        }

        int minDistance = _splitContainer.Panel1MinSize;
        int maxDistance = Math.Max(minDistance, width - _splitContainer.Panel2MinSize);
        int desired = (width * 2) / 3;

        if (desired < minDistance)
        {
            desired = minDistance;
        }
        else if (desired > maxDistance)
        {
            desired = maxDistance;
        }

        if (_splitContainer.SplitterDistance != desired)
        {
            _splitContainer.SplitterDistance = desired;
        }
    }

    private void OnRootComboSelectedIndexChanged(object? sender, EventArgs e)
    {
        if (_initializingRoot)
        {
            return;
        }

        RefreshTree(null);
    }

    private void ToggleVirtualFolders()
    {
        _shellProvider.ShowAllShellObjects = _showVirtualButton.Checked;
        RefreshTree(GetSelectedNodePath());
    }

    private void RefreshTree(string? pathToSelect)
    {
        if (_rootCombo.SelectedItem is RootFolderOption option)
        {
            _shellProvider.RootFolder = option.Csidl;
        }

        string? targetPath = string.IsNullOrWhiteSpace(pathToSelect) ? null : pathToSelect;

        try
        {
            UseWaitCursor = true;
            _folderBrowser.Populate(targetPath);
        }
        finally
        {
            UseWaitCursor = false;
        }

        UpdateSelectionList();
        UpdateSelectionStatus();

        if (!string.IsNullOrEmpty(targetPath))
        {
            _statusLabel.Text = targetPath;
        }
        else if (_folderBrowser.SelectedNode is TreeNodePath node && !string.IsNullOrEmpty(node.Path))
        {
            _statusLabel.Text = node.Path;
        }
    }

    private void OnSelectedDirectoriesChanged(object? sender, SelectedDirectoriesChangedEventArgs e)
    {
        UpdateSelectionList();
        UpdateSelectionStatus();

        if (!string.IsNullOrEmpty(e.Path))
        {
            _statusLabel.Text = string.Format(CultureInfo.CurrentCulture,
                "Checked: {0} → {1}", e.Path, e.CheckState);
        }
    }

    private void UpdateSelectionList()
    {
        _selectionList.BeginUpdate();
        _selectionList.Items.Clear();
        foreach (var path in _folderBrowser.SelectedDirectories)
        {
            _selectionList.Items.Add(path);
        }
        _selectionList.EndUpdate();
    }

    private void UpdateSelectionStatus()
    {
        int count = _folderBrowser.SelectedDirectories.Count;
        if (count == 0)
        {
            _statusLabel.Text = "No directories checked.";
        }
        else
        {
            _statusLabel.Text = string.Format(CultureInfo.CurrentCulture,
                "Checked directories: {0}", count);
        }
    }

    private void OnAfterSelect(object? sender, TreeViewEventArgs e)
    {
        if (e.Node is TreeNodePath node && !string.IsNullOrEmpty(node.Path))
        {
            _statusLabel.Text = node.Path;
        }
    }

    private void OnFolderBrowserKeyUp(object? sender, KeyEventArgs e)
    {
        if (e.KeyCode == Keys.F5)
        {
            RefreshTree(GetSelectedNodePath());
            e.Handled = true;
        }
    }

    private string? GetSelectedNodePath()
    {
        return (_folderBrowser.SelectedNode as TreeNodePath)?.Path;
    }

    private sealed record RootFolderOption(string Text, Raccoom.Win32.ShellAPI.CSIDL Csidl)
    {
        public override string ToString() => Text;
    }
}
