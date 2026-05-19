// Copyright © 2009 by Christoph Richner. All rights are reserved.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
// 
// website http://www.raccoom.net, email support@raccoom.net, msn chrisdarebell@msn.com

using System;
namespace Raccoom.Windows.Forms
{
    /// <summary>
    /// <c>TreeStrategyFolderBrowserProvider</c> is the shell32 interop data provider for <see cref="TreeViewFolderBrowser"/> which is based on <see cref="ROOT.CIMV2.Win32.Logicaldisk"/>, <c>Shell32</c> Interop and <see cref="Raccoom.Win32.SystemImageList"/>
    /// <seealso cref="TreeStrategyFolderBrowserProvider"/>
    /// </summary>
    /// <remarks>
    /// Shell32 does not support the .NET System.Security.Permissions system. There is no code access permission, only FileSystem ACL.
    /// </remarks>
    [System.ComponentModel.DefaultProperty("ShowAllShellObjects"), System.Drawing.ToolboxBitmap(typeof(System.Data.SqlClient.SqlDataAdapter))]
    public class TreeStrategyShell32Provider : TreeStrategyFolderBrowserProvider
    {
        #region fields

        /// <summary>Shell32 Com Object</summary>
        private Raccoom.Win32.ShellBrowser _shell = new Raccoom.Win32.ShellBrowser();

        /// <summary>drive tree node (mycomputer) root collection</summary>
        private System.Windows.Forms.TreeNodeCollection _rootCollection = null;

        /// <summary>show only filesystem</summary>
        private bool _showAllShellObjects = false;

        /// <summary>enable shell context menu</summary>
        private bool _enableContextMenu = false;

        #endregion

        #region constructors

        #endregion

        #region public interface
        new public Raccoom.Win32.ShellAPI.CSIDL RootFolder { get; set; }

        /// <summary>
        /// Enables or disables the context menu which show's the folder item's shell verbs.
        /// </summary>
        [System.ComponentModel.Browsable(true), System.ComponentModel.Category("Shell32"), System.ComponentModel.Description("Specifies if the context menu is enabled."), System.ComponentModel.DefaultValue(false)]
        public bool EnableContextMenu
        {
            get { return _enableContextMenu; }
            set { _enableContextMenu = value; }
        }

        /// <summary>
        /// Gets or sets if virtual shell folders are displayed or not. virtual shell folders are system folders like control panel.
        /// </summary>
        [System.ComponentModel.Browsable(true), System.ComponentModel.Category("Shell32"), System.ComponentModel.Description("Display file system and virtual shell folders."), System.ComponentModel.DefaultValue(false)]
        public bool ShowAllShellObjects
        {
            get { return _showAllShellObjects; }
            set { _showAllShellObjects = value; }
        }

        #endregion

        #region ITreeViewFolderBrowserDataProvider Members
        public override void QueryContextMenuItems(TreeNodePath node)
        {
            if (!EnableContextMenu) return;
            //
            Raccoom.Win32.ShellItem fi = node.Tag as Raccoom.Win32.ShellItem;
            if (fi == null) return;
            ////
            //foreach (.FolderItemVerb verb in fi..Verbs())
            //{
            //    if (verb.Name.Length == 0) continue;
            //    //
            //    MenuItemShellVerb item = new MenuItemShellVerb(verb);
            //    Helper.TreeView.ContextMenu.MenuItems.Add(item);
            //}
        }

        protected override void SetIcon(TreeViewFolderBrowser treeView, TreeNodePath node)
        {
            // base.SetIcon(treeView, node);            
        }

        public override void RequestRootNode()
        {
            // do not call base class here
            // base.RequestRootNode();

            AttachSystemImageList(Helper);
            // setup up root node collection
            switch (RootFolder)
            {
                case Raccoom.Win32.ShellAPI.CSIDL.DESKTOP :
                    // create root node <Desktop>
                    TreeNodePath desktopNode = CreateTreeNode(Helper.TreeView.Nodes, null, _shell.DesktopItem);
                    _rootCollection = desktopNode.Nodes;
                    // enable shell objects always to fill desktop level
                    bool settingBackup = _showAllShellObjects;
                    _showAllShellObjects = true;
                    // set setting back to original value
                    _showAllShellObjects = settingBackup;
                    break;
                case  Raccoom.Win32.ShellAPI.CSIDL.DRIVES:
                    this.FillMyComputer(_shell.MyComputerItem, Helper.TreeView.Nodes, Helper);
                    break;
                default:
                    //
                    TreeNodePath rootNode = CreateTreeNode(Helper.TreeView.Nodes, null, _shell.GetSpecialFolderShellItem(RootFolder));
                    if (!rootNode.HasDummyNode) rootNode.AddDummyNode();
                    _rootCollection = rootNode.Nodes;
                    break;
            }
        }

        public override void RequestChildNodes(TreeNodePath parent, System.Windows.Forms.TreeViewCancelEventArgs e)
        {
            Raccoom.Win32.ShellItem folderItem = ((Raccoom.Win32.ShellItem)parent.Tag);
            folderItem.Expand(this.ShowFiles, true, System.IntPtr.Zero);
            //
            TreeNodePath node = null;
            System.IO.DriveInfo driveInfo;
            //
            foreach (Raccoom.Win32.ShellItem childFolder in folderItem.SubFolders)
            {
                if (!_showAllShellObjects && !childFolder.IsFileSystem) continue;
                //
                if (DriveTypes != DriveTypes.All && childFolder.IsDisk)
                {
                    driveInfo = new System.IO.DriveInfo(childFolder.Path);
                    //                                       
                    switch (driveInfo.DriveType)
                    {
                        case System.IO.DriveType.CDRom:
                            if ((DriveTypes & DriveTypes.CompactDisc) == 0) continue;
                            break;
                        case System.IO.DriveType.Fixed:
                            if ((DriveTypes & DriveTypes.LocalDisk) == 0) continue;
                            break;
                        case System.IO.DriveType.Network:
                            if ((DriveTypes & DriveTypes.NetworkDrive) == 0) continue;
                            break;
                        case System.IO.DriveType.NoRootDirectory:
                            if ((DriveTypes & DriveTypes.NoRootDirectory) == 0) continue;
                            break;
                        case System.IO.DriveType.Ram:
                            if ((DriveTypes & DriveTypes.RAMDisk) == 0) continue;
                            break;
                        case System.IO.DriveType.Removable:
                            if ((DriveTypes & DriveTypes.RemovableDisk) == 0) continue;
                            break;
                        case System.IO.DriveType.Unknown:
                            if ((DriveTypes & DriveTypes.NoRootDirectory) == 0) continue;
                            break;
                    }
                }
                //						
                node = CreateTreeNode(null, parent, childFolder);
            }
            if (!ShowFiles) return;
            //
            foreach (Raccoom.Win32.ShellItem fileItem in folderItem.SubFiles)
            {
                node = CreateTreeNode(null, parent, fileItem);
            }
        }

        private System.Windows.Forms.TreeNodeCollection RequestDriveCollection()
        {
            return _rootCollection;

        }

        #endregion

        #region internal interface
        protected virtual TreeNodePath CreateTreeNode(System.Windows.Forms.TreeNodeCollection parentCollection, TreeNodePath parentNode, Raccoom.Win32.ShellItem shellItem)
        {
            if (shellItem == null) throw new ArgumentNullException("shellItem");
            //
            TreeNodePath node = CreateTreeNode(parentCollection, parentNode, shellItem.Text, shellItem.Path, !shellItem.IsFolder, shellItem.HasSubfolder, !shellItem.IsFileSystem);
            node.ImageIndex = shellItem.ImageIndex;
            node.SelectedImageIndex = shellItem.SelectedImageIndex;
            node.Tag = shellItem;
            //
            shellItem.ShellItemUpdated += delegate(object sender, EventArgs e)
            {
                node.Text = shellItem.Text;
                node.ImageIndex = shellItem.ImageIndex;
                node.SelectedImageIndex = shellItem.SelectedImageIndex;                
            };
            return node;
        }

        /// <summary>
        /// Popluates the MyComputer node
        /// </summary>
        /// <param name="folderItem"></param>
        /// <param name="parentCollection"></param>
        /// <param name="helper"></param>
        protected virtual void FillMyComputer(Raccoom.Win32.ShellItem folderItem, System.Windows.Forms.TreeNodeCollection parentCollection, TreeViewFolderBrowserNodeFactory helper)
        {
            _rootCollection = parentCollection;
            // get wmi logical disk's if we have to 			
            System.IO.DriveInfo driveInfo;
            //
            folderItem.Expand(true, true, System.IntPtr.Zero);
            //
            foreach (Raccoom.Win32.ShellItem fi in folderItem.SubFolders)
            {
                // only File System shell objects ?
                if (!_showAllShellObjects && !fi.IsFileSystem) continue;
                //
                if (DriveTypes != DriveTypes.All && fi.IsDisk)
                {
                    driveInfo = new System.IO.DriveInfo(fi.Path);
                    //                                       
                    switch (driveInfo.DriveType)
                    {
                        case System.IO.DriveType.CDRom:
                            if ((DriveTypes & DriveTypes.CompactDisc) == 0) continue;
                            break;
                        case System.IO.DriveType.Fixed:
                            if ((DriveTypes & DriveTypes.LocalDisk) == 0) continue;
                            break;
                        case System.IO.DriveType.Network:
                            if ((DriveTypes & DriveTypes.NetworkDrive) == 0) continue;
                            break;
                        case System.IO.DriveType.NoRootDirectory:
                            if ((DriveTypes & DriveTypes.NoRootDirectory) == 0) continue;
                            break;
                        case System.IO.DriveType.Ram:
                            if ((DriveTypes & DriveTypes.RAMDisk) == 0) continue;
                            break;
                        case System.IO.DriveType.Removable:
                            if ((DriveTypes & DriveTypes.RemovableDisk) == 0) continue;
                            break;
                        case System.IO.DriveType.Unknown:
                            if ((DriveTypes & DriveTypes.NoRootDirectory) == 0) continue;
                            break;
                    }
                }
                // create new node
                TreeNodePath node = CreateTreeNode(parentCollection, null, fi);
            }
        }

        /// <summary>
        /// Do we have to add a dummy node (+ sign)
        /// </summary>
        protected virtual bool IsFolderWithChilds(Raccoom.Win32.ShellItem fi)
        {
            return _showAllShellObjects || (fi.IsFileSystem && fi.IsFolder && !fi.IsBrowsable);
        }

        #endregion

        public override string ToString()
        {
            return "Shell32 Provider";
        }
    }

    /// <summary>
    /// Extends the <c>MenuItem</c> class with a Shell32.FolderItemVerb.
    /// </summary>
    public class MenuItemShellVerb : System.Windows.Forms.MenuItem
    {

    }
}