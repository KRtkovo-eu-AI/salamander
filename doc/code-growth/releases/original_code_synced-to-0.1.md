# Source code growth report

Baseline: **original_code_synced** (`697a4ac6e0f57998a6024b1492612f50761ec02e`)  
Current: **5.0-samandarin-0.1**  
NLOC: **969,956 -> 1,002,824** (**+32,868**)  
Files: **2,555 -> 2,756** (**+201**)  
Functions/methods: **31,536 -> 33,006** (**+1,470**)

## Modules

| Module | Baseline NLOC | Current NLOC | Delta | Files delta |
| --- | ---: | ---: | ---: | ---: |
| `3rd-party/treeview` | 0 | 9,615 | +9,615 | +48 |
| `src/core` | 176,493 | 183,741 | +7,248 | +2 |
| `3rd-party/jsonViewer` | 0 | 3,299 | +3,299 | +38 |
| `src/plugins/textviewer` | 0 | 3,151 | +3,151 | +15 |
| `src/plugins/webview2renderviewer` | 0 | 2,655 | +2,655 | +15 |
| `src/plugins/jsonviewer` | 0 | 2,352 | +2,352 | +16 |
| `src/plugins/samandarin` | 0 | 2,019 | +2,019 | +12 |
| `src/plugins/csdemo` | 0 | 1,182 | +1,182 | +13 |
| `tools/utfnames` | 0 | 826 | +826 | +1 |
| `tools/comments` | 819 | 1,050 | +231 | +41 |
| `src/common` | 188,430 | 188,597 | +167 | +0 |
| `src/lang` | 3,750 | 3,813 | +63 | +0 |
| `src/setup` | 7,189 | 7,224 | +35 | +0 |
| `src/plugins/mmviewer` | 12,985 | 13,010 | +25 | +0 |

## Files

| File | Baseline NLOC | Current NLOC | Delta | Files delta |
| --- | ---: | ---: | ---: | ---: |
| `src/mainwnd3.cpp` | 5,632 | 7,719 | +2,087 | +0 |
| `3rd-party/treeview/Raccoom.TreeViewFolderBrowser.DataProviders/Win32/Logicaldisk.cs` | 0 | 1,973 | +1,973 | +1 |
| `src/tabwnd.cpp` | 38 | 1,508 | +1,470 | +0 |
| `src/plugins/textviewer/managed/ViewerHost.cs` | 0 | 1,324 | +1,324 | +1 |
| `3rd-party/treeview/Raccoom.TreeViewFolderBrowser.DataProviders/Win32/Shell/ShellAPI.cs` | 0 | 1,222 | +1,222 | +1 |
| `3rd-party/treeview/Raccoom.TreeViewFolderBrowser.DataProviders/Win32/Shell/ShellItem.cs` | 0 | 1,165 | +1,165 | +1 |
| `src/plugins/webview2renderviewer/managed/ViewerHost.cs` | 0 | 1,113 | +1,113 | +1 |
| `src/plugins/samandarin/managed/EntryPoint.cs` | 0 | 1,002 | +1,002 | +1 |
| `tools/utfnames/utfnames.cpp` | 0 | 826 | +826 | +1 |
| `3rd-party/jsonViewer/JsonViewer/JsonViewer.cs` | 0 | 791 | +791 | +1 |
| `src/plugins/jsonviewer/managed/ViewerHost.cs` | 0 | 735 | +735 | +1 |
| `src/plugins/jsonviewer/managed/ThemeHelper.cs` | 0 | 725 | +725 | +1 |
| `src/darkmode.cpp` | 0 | 689 | +689 | +1 |
| `3rd-party/treeview/Raccoom.TreeViewFolderBrowser.DataProviders/Win32/SystemImageList.cs` | 0 | 681 | +681 | +1 |
| `3rd-party/treeview/Raccoom.TreeViewFolderBrowser.DataProviders/Ftp/ftplib.cs` | 0 | 650 | +650 | +1 |
| `src/plugins/textviewer/textviewer.cpp` | 0 | 649 | +649 | +1 |
| `3rd-party/treeview/Raccoom.TreeViewFolderBrowser.Demo/CollapsibleSplitter.cs` | 0 | 642 | +642 | +1 |
| `src/plugins/webview2renderviewer/managed/ThemeHelper.cs` | 0 | 544 | +544 | +1 |
| `src/plugins/textviewer/managed/ThemeHelper.cs` | 0 | 544 | +544 | +1 |
| `src/plugins/samandarin/managed/ThemeHelper.cs` | 0 | 533 | +533 | +1 |

## Scopes/classes

| Scope/class | Baseline NLOC | Current NLOC | Delta | Functions delta |
| --- | ---: | ---: | ---: | ---: |
| `<global functions>` | 204,850 | 209,200 | +4,350 | +225 |
| `CMainWindow` | 14,641 | 17,059 | +2,418 | +58 |
| `ThemeHelper` | 0 | 1,467 | +1,467 | +65 |
| `CTabWindow` | 35 | 1,425 | +1,390 | +51 |
| `JsonViewer` | 0 | 1,124 | +1,124 | +63 |
| `Logicaldisk` | 0 | 864 | +864 | +109 |
| `ShellItem` | 0 | 824 | +824 | +33 |
| `CPluginInterface` | 4,516 | 5,324 | +808 | +80 |
| `EntryPoint` | 0 | 681 | +681 | +33 |
| `FtpClient` | 0 | 610 | +610 | +32 |
| `CollapsibleSplitter` | 0 | 522 | +522 | +14 |
| `UpdateCoordinator` | 0 | 512 | +512 | +26 |
| `TreeViewFolderBrowserDemoForm` | 0 | 496 | +496 | +12 |
| `ViewerHost` | 0 | 437 | +437 | +21 |
| `RenderViewerForm` | 0 | 387 | +387 | +23 |
| `ViewCommandPayload` | 0 | 366 | +366 | +21 |
| `TextViewerForm` | 0 | 339 | +339 | +23 |
| `NativeMethods` | 0 | 318 | +318 | +21 |
| `SHFILEINFO` | 0 | 275 | +275 | +15 |
| `Renderer` | 0 | 260 | +260 | +35 |

## Functions

| Function | Baseline NLOC | Current NLOC | Delta | Occurrences delta |
| --- | ---: | ---: | ---: | ---: |
| `src/plugins/textviewer/textviewer.cpp::CPluginInterface::Connect` | 0 | 481 | +481 | +1 |
| `src/mainwnd3.cpp::CMainWindow::WindowProc` | 4,889 | 5,289 | +400 | +0 |
| `3rd-party/treeview/Raccoom.TreeViewFolderBrowser.Demo/TreeViewFolderBrowserDemoForm.Designer.cs::TreeViewFolderBrowserDemoForm::InitializeComponent` | 0 | 392 | +392 | +1 |
| `3rd-party/jsonViewer/JsonViewer/JsonViewer.Designer.cs::JsonViewer::InitializeComponent` | 0 | 343 | +343 | +1 |
| `src/mainwnd3.cpp::CMainWindow::OnPanelTabContextMenu` | 0 | 316 | +316 | +1 |
| `3rd-party/treeview/Raccoom.TreeViewFolderBrowser.DataProviders/Win32/Shell/ShellItem.cs::ShellItem::Update` | 0 | 285 | +285 | +2 |
| `3rd-party/treeview/Raccoom.TreeViewFolderBrowser.DataProviders/Win32/Logicaldisk.cs::Logicaldisk::if` | 0 | 226 | +226 | +53 |
| `3rd-party/jsonViewer/JsonView/MainForm.Designer.cs::MainForm::InitializeComponent` | 0 | 208 | +208 | +1 |
| `tools/utfnames/utfnames.cpp::ListDirectory` | 0 | 182 | +182 | +1 |
| `src/mainwnd1.cpp::CMainWindow_RefreshCommandStates` | 365 | 532 | +167 | +0 |
| `3rd-party/treeview/Raccoom.TreeViewFolderBrowser.Demo/CollapsibleSplitter.cs::CollapsibleSplitter::OnPaint` | 0 | 165 | +165 | +1 |
| `src/mainwnd3.cpp::CMainWindow::TryCompletePanelTabDrag` | 0 | 160 | +160 | +1 |
| `src/mainwnd1.cpp::CMainWindow::FormatPanelPathForDisplay` | 0 | 132 | +132 | +1 |
| `3rd-party/jsonViewer/JsonViewer/JsonViewer.cs::JsonViewer::removeSpecialCharsToolStripMenuItem_Click` | 0 | 129 | +129 | +1 |
| `3rd-party/treeview/Raccoom.TreeViewFolderBrowser.DataProviders/Win32/SystemImageList.cs::SHFILEINFO::DrawImage` | 0 | 125 | +125 | +4 |
| `src/mainwnd1.cpp::CMainWindow::GetFormatedPathForTitle` | 132 | 11 | -121 | +0 |
| `src/plugins/jsonviewer/managed/ThemeHelper.cs::ThemeHelper::ApplyToControl` | 0 | 119 | +119 | +1 |
| `src/tabwnd.cpp::CTabWindow::DrawColoredTab` | 0 | 113 | +113 | +1 |
| `src/mainwnd2.cpp::LoadPanelSettingsFromKey` | 0 | 111 | +111 | +1 |
| `3rd-party/treeview/Raccoom.TreeViewFolderBrowser.Demo/CollapsibleSplitter.cs::CollapsibleSplitter::animationTimerTick` | 0 | 110 | +110 | +1 |
