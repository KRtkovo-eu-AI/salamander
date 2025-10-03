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
	/// TreeStrategyShellImageListProviderBase defines the common logic needed to assing icons form the <c>SystemImageList</c> class.
	/// <seealso cref="SystemImageList"/>
	/// </summary>
	[System.Drawing.ToolboxBitmap(typeof (System.Data.SqlClient.SqlDataAdapter))]
	public abstract class TreeStrategyShellImageListProviderBase  : TreeStrategyDataProviderBase, IDisposable
	{
		#region fields
		/// <summary>last CheckboxMode used to fill the tree view, saved to know about changes</summary>
		private CheckBoxBehaviorMode _checkboxMode;
		/// <summary>Shell32 ImageList</summary>
		private Raccoom.Win32.SystemImageList _systemImageList;
		#endregion
		
		#region internal interface		
		/// <summary>
		/// Gets the Shell32ImageList handle
		/// </summary>
		protected Raccoom.Win32.SystemImageList SystemImageList
		{
			get
			{
				return _systemImageList;
			}
		}
		/// <summary>
		/// Attaches the Shell32 ImageList to the TreeView
		/// </summary>
		/// <param name="helper">Helper class to have access to managed TreeView instance</param>
		protected virtual void AttachSystemImageList(TreeViewFolderBrowserNodeFactory helper)
		{
			if (_checkboxMode != helper.TreeView.CheckBoxBehaviorMode | _systemImageList == null)
			{
				// checkboxes recreate the control internal
				if (this._systemImageList != null)
				{
					Raccoom.Win32.SystemImageListHelper.SetTreeViewImageList(helper.TreeView, _systemImageList, false);
				}
				// create on demand
				if (_systemImageList == null)
				{
					// Shell32 ImageList
					_systemImageList = new Raccoom.Win32.SystemImageList(Raccoom.Win32.SystemImageListSize.SmallIcons);
					Raccoom.Win32.SystemImageListHelper.SetTreeViewImageList(helper.TreeView, _systemImageList, false);
				}
			}
			_checkboxMode = helper.TreeView.CheckBoxBehaviorMode;
		}

		/// <summary>
		/// Extract the icon for the file type (Extension)
		/// </summary>
		protected virtual void SetIcon(TreeViewFolderBrowser treeView, TreeNodePath node)
		{
            if (_systemImageList == null) throw new ArgumentNullException("_systemImageList");
            if (treeView == null) throw new ArgumentNullException("treeView");
            if (node == null) throw new ArgumentNullException("node");
            //
			node.ImageIndex = this._systemImageList.IconIndex(node.Path, true, Raccoom.Win32.ShellIconStateConstants.ShellIconStateNormal);
			node.SelectedImageIndex = this._systemImageList.IconIndex(node.Path, true, Raccoom.Win32.ShellIconStateConstants.ShellIconStateOpen);
		}
		/// <summary>
		/// Creates a new node and assigns an icon
		/// </summary>
		protected virtual TreeNodePath CreateTreeNode(TreeNodePath parentNode, string text, string path, bool isFile, bool addDummyNode, bool isSpecialFolder)
		{			
			return CreateTreeNode(parentNode != null ? parentNode.Nodes : Helper.TreeView.Nodes, parentNode, text, path, isFile, addDummyNode, isSpecialFolder);
		}

		/// <summary>
		/// Creates a new node and assigns an icon
		/// </summary>
		protected virtual TreeNodePath CreateTreeNode(System.Windows.Forms.TreeNodeCollection parentCollection, TreeNodePath parentNode, string text, string path, bool isFile, bool addDummyNode,  bool isSpecialFolder)
		{
            TreeNodePath node = Helper.CreateTreeNode(parentCollection != null ? parentCollection : parentNode.Nodes, parentNode, text, path, addDummyNode, false, isSpecialFolder);
			try
			{
				SetIcon(Helper.TreeView, node);
			}
			catch
			{
				node.ImageIndex = -1;
				node.SelectedImageIndex = -1;
			}
			return node;
		}
		#endregion		
	
		#region pulbic interface
		public override void Attach(TreeViewFolderBrowserNodeFactory helper)
		{
			base.Attach (helper);
			//
			this.AttachSystemImageList(helper);
		}		
		public override void Detach()
		{
			base.Detach ();
			//
			_checkboxMode = CheckBoxBehaviorMode.None;
		}

		#endregion

		#region IDisposable Members		
		/// <summary>
		/// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
		/// </summary>
		public void Dispose()
		{
			if (_systemImageList != null)
			{
				_systemImageList.Dispose();
			}
		}

		#endregion
	}
	/// <summary>
	/// TreeStrategyFolderBrowserProviderBase defines the common FolderBrowser environment and can be
	/// used for all Shell32ImageList based folder browser components to display folder and files
	/// associated with Windows Explorer like Icons.
	/// </summary>
	public abstract class TreeStrategyFolderBrowserProviderBase : TreeStrategyShellImageListProviderBase
	{
		#region fields
		/// <summary></summary>
		private bool _showFiles = false;
		private bool _hideFileExention = false;
		#endregion

		#region public interface
		[System.ComponentModel.Browsable(true), System.ComponentModel.Category("File System"), System.ComponentModel.Description("Display files."), System.ComponentModel.DefaultValue(false)]
		public bool ShowFiles
		{
			get { return _showFiles; }
			set { _showFiles = value; }
		}
		[System.ComponentModel.Browsable(true), System.ComponentModel.Category("File System"), System.ComponentModel.Description("Hide file extensions."), System.ComponentModel.DefaultValue(false)]
		public bool HideFileExtension
		{
			get { return _hideFileExention; }
			set { _hideFileExention= value; }
		}
		#endregion

		#region internal interface
        public override bool HasSelectedChildNodes(System.Collections.ObjectModel.Collection<string> items, TreeNodePath node)
		{
			bool isBold = false;
			//
			foreach (string s in items)
			{
				// if path is equal, return
				if (s.Equals(node.Path)) continue;
				// if path is substring, mark node bold, otherwise normal font is used
				if (s.IndexOf(node.Path) != -1)
				{
					isBold = true;
					break;
				}
				else
				{
					isBold = false;
				}
			}
			return isBold;
		}
       
		protected override TreeNodePath CreateTreeNode(System.Windows.Forms.TreeNodeCollection parentCollection, TreeNodePath parentNode, string text, string path, bool isFile, bool addDummyNode,  bool isSpecialFolder)
		{	
			if(isFile && _hideFileExention)
			{
				text = System.IO.Path.GetFileNameWithoutExtension(path);
			}
			return base.CreateTreeNode(parentCollection, parentNode, text, path, isFile, addDummyNode, isSpecialFolder);
		}
		#endregion
	}
	/// <summary>
	/// TreeStrategySystemFolderBrowserProviderBase extends <c>TreeStrategyFolderBrowserProviderBase</c> and can be used for folder browser components
	/// which shows the local machine drive types. The class uses WMI LogicalDisk classes to enumerate the machine local drives.
	/// The clue is that you can specifiy which drives types to include into the query, this way you can set a filter and show only
	/// Removable Drives or Local hard disk drives.
	/// </summary>
	public abstract class TreeStrategySystemFolderBrowserProviderBase : TreeStrategyFolderBrowserProviderBase
	{
		#region fields
		/// <summary>Specify which drive types are displayed.</summary>
		private DriveTypes _driveTypes;

		/// <summary>Fired if a drive types has changed</summary>
		public System.EventHandler DriveTypesChanged;
		#endregion

		#region public interface
		/// <summary>
		/// Specify which drive types are displayed.
		/// </summary>
		[System.ComponentModel.Browsable(true), System.ComponentModel.Category("File System"), System.ComponentModel.Description("Specify which drive types are displayed"), System.ComponentModel.DefaultValue(DriveTypes.All)]
		public virtual DriveTypes DriveTypes
		{
			get { return _driveTypes; }
			set
			{
				_driveTypes = value;
				//
				OnDriveTypesChanged(System.EventArgs.Empty);
			}
		}
		#endregion

		#region internal interface
		/// <summary>
		/// Returns a filtered <c>LogicaldiskCollection</c> filled with the requested drives.
		/// </summary>
		/// <returns></returns>
		protected virtual ROOT.CIMV2.Win32.Logicaldisk.LogicaldiskCollection GetLogicaldiskCollection()
		{
			return ROOT.CIMV2.Win32.Logicaldisk.GetInstances(null, GetWMIQueryStatement(Helper.TreeView));
		}
		/// <summary>
		/// Gets the WMI query string based on the current drive types.
		/// </summary>
		/// <returns></returns>
		protected virtual string GetWMIQueryStatement(TreeViewFolderBrowser treeView)
		{
			if ((DriveTypes & DriveTypes.All) == DriveTypes.All) return string.Empty;
			//
			string where = string.Empty;
			//
			System.Array array = System.Enum.GetValues(typeof (DriveTypes));
			//
			foreach (DriveTypes type in array)
			{
				if ((DriveTypes & type) == type)
				{
					if (where == string.Empty)
					{
						where += "drivetype = " + System.Enum.Format(typeof (Win32_LogicalDiskDriveTypes), System.Enum.Parse(typeof (Win32_LogicalDiskDriveTypes), type.ToString(), true), "d");
					}
					else
					{
						where += " OR drivetype = " + System.Enum.Format(typeof (Win32_LogicalDiskDriveTypes), System.Enum.Parse(typeof (Win32_LogicalDiskDriveTypes), type.ToString(), true), "d");
					}
				}
			}
			//
			return where;
		}
		/// <summary>
		/// Raises the DriveTypesChanged event.
		/// </summary>
		/// <param name="e">An EventArgs that contains the event data.</param>
		protected virtual void OnDriveTypesChanged(System.EventArgs e)
		{
			if (DriveTypesChanged != null) DriveTypesChanged(this, e);
		}

		#endregion
	}
}
