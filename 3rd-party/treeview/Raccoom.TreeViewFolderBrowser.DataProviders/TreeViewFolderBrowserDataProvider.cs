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

namespace Raccoom.Windows.Forms
{
	/// <summary>
	/// <c>TreeStrategyFolderBrowserProvider</c> is the standard data provider for <see cref="TreeViewFolderBrowser"/> which is based on <see cref="ROOT.CIMV2.Win32.Logicaldisk"/>, System.IO and <see cref="Raccoom.Win32.SystemImageList"/>
	/// <seealso cref="TreeStrategyDataProviderBase"/>
	/// </summary>
	[System.Drawing.ToolboxBitmap(typeof (System.Data.SqlClient.SqlDataAdapter))]
	public class TreeStrategyFolderBrowserProvider : TreeStrategySystemFolderBrowserProviderBase
	{
		#region fields
		/// <summary>designer</summary>
		private System.Environment.SpecialFolder _specialFolderRootFolder;
		#endregion

		/// <summary>
		/// Initializes a new instance of the <c>TreeStrategyFolderBrowserProvider</c> class.
		/// </summary>
		public TreeStrategyFolderBrowserProvider()
		{
			this.DriveTypes = DriveTypes.NoRootDirectory | DriveTypes.RemovableDisk | DriveTypes.LocalDisk | DriveTypes.NetworkDrive | DriveTypes.CompactDisc | DriveTypes.RAMDisk;
		}

		#region public interface
		/// <summary>
		/// Gets or sets the root folder where the browsing starts from.
		/// </summary>
		[System.ComponentModel.Browsable(true), System.ComponentModel.Category("FolderBrowser"), System.ComponentModel.Description("Only the specified folder and any subfolders that are beneath it will appear in the dialog box and be selectable.")]
		[System.ComponentModel.DefaultValue(System.Environment.SpecialFolder.MyComputer)]
		public System.Environment.SpecialFolder RootFolder
		{
			get { return _specialFolderRootFolder; }
			set { _specialFolderRootFolder = value; }
		}
		#endregion

		#region ITreeViewFolderBrowserDataProvider Members

		public override void QueryContextMenuItems(TreeNodePath node)
		{
		}

		private System.Windows.Forms.TreeNodeCollection RequestDriveCollection()
		{
			switch (RootFolder)
			{
				case System.Environment.SpecialFolder.Desktop:
					return Helper.TreeView.Nodes[0].Nodes[1].Nodes;
				default:
					return Helper.TreeView.Nodes;
			}
		}

		public override void RequestChildNodes(TreeNodePath parent, System.Windows.Forms.TreeViewCancelEventArgs e)
		{
			if (parent.Path == null) return;
			//
			System.IO.DirectoryInfo directory = new System.IO.DirectoryInfo(parent.Path);
			// check persmission
			new System.Security.Permissions.FileIOPermission(System.Security.Permissions.FileIOPermissionAccess.PathDiscovery, directory.FullName).Demand();
			//					
			foreach (System.IO.DirectoryInfo dir in directory.GetDirectories())
			{
				if ((dir.Attributes & System.IO.FileAttributes.System) == System.IO.FileAttributes.System)
				{
					continue;
				}
				if ((dir.Attributes & System.IO.FileAttributes.Hidden) == System.IO.FileAttributes.Hidden)
				{
					continue;
				}
				TreeNodePath newNode = this.CreateTreeNode(parent, dir.Name, dir.FullName, false,false, false);
				//
				try
				{
					if (dir.GetDirectories().GetLength(0) > 0)
					{
						newNode.AddDummyNode();
					}
				}
				catch(System.UnauthorizedAccessException)
				{	
					// eat the exception
				}
				catch(System.Exception ex)
				{
					throw ex;
				}
			}
			if (ShowFiles)
			{
				foreach (System.IO.FileInfo file in directory.GetFiles())
				{
					this.CreateTreeNode(parent.Nodes, parent, file.Name, file.FullName, true, false, false);
				}
			}
		}

		public override void RequestRootNode()
		{
			bool populateDrives = true;
            AttachSystemImageList(Helper);
            //
			System.Windows.Forms.TreeNodeCollection rootNodeCollection;
			System.Windows.Forms.TreeNodeCollection driveRootNodeCollection;
			// setup up root node collection
			switch (RootFolder)
			{
				case System.Environment.SpecialFolder.Desktop:
					// create root node <Desktop>
					TreeNodePath desktopNode = this.CreateTreeNode(null, System.Environment.SpecialFolder.Desktop.ToString(), string.Empty, false, false, true);
					rootNodeCollection = Helper.TreeView.Nodes[0].Nodes;
					// create child node <Personal>
					string personalDirectory = System.Environment.GetFolderPath(System.Environment.SpecialFolder.Personal);
					this.CreateTreeNode(desktopNode, System.IO.Path.GetFileName(personalDirectory), personalDirectory, false, true, false);
					// create child node <MyComuter>
					TreeNodePath myComputerNode = this.CreateTreeNode(desktopNode, System.Environment.SpecialFolder.MyComputer.ToString(), string.Empty,false,  false, true);
					driveRootNodeCollection = myComputerNode.Nodes;
					break;
				case System.Environment.SpecialFolder.MyComputer:
					rootNodeCollection = Helper.TreeView.Nodes;
					driveRootNodeCollection = rootNodeCollection;
					break;
				default:
					rootNodeCollection = Helper.TreeView.Nodes;
					driveRootNodeCollection = rootNodeCollection;
					// create root node with specified SpecialFolder
					this.CreateTreeNode(null, System.IO.Path.GetFileName(System.Environment.GetFolderPath(RootFolder)), System.Environment.GetFolderPath(RootFolder),false,  true, false);
					populateDrives = false;
					break;
			}
			if (populateDrives)
			{
				// populate local machine drives
				foreach (ROOT.CIMV2.Win32.Logicaldisk logicalDisk in GetLogicaldiskCollection())
				{
					try
					{
						string name = string.Empty;
						string path = logicalDisk.Name + "\\";
						name = logicalDisk.Description;
						//
						name += (name != string.Empty) ? " (" + path + ")" : path;
						// add node to root collection
						this.CreateTreeNode(driveRootNodeCollection, null, name, path, false, true, false);
					}
					catch (System.Exception doh)
					{
						throw doh;
					}
				}
			}
		}

		/// <summary>
		/// Focus the specified folder and scroll it in to view.
		/// </summary>
		/// <param name="directoryPath">The path which should be focused</param>
		public override void ShowNode(string directoryPath)
		{
			if ((directoryPath == null) || (directoryPath == "") || (directoryPath == string.Empty)) return;
			// start search at root node
			System.Windows.Forms.TreeNodeCollection nodeCol = RequestDriveCollection();
			//
			if (!System.IO.Directory.Exists(directoryPath) || nodeCol == null) return;
			//
			System.IO.DirectoryInfo dirInfo = new System.IO.DirectoryInfo(directoryPath);
			// get path tokens
			System.Collections.ArrayList dirs = new System.Collections.ArrayList();
			dirs.Add(dirInfo.FullName);
			//
			while (dirInfo.Parent != null)
			{
				dirs.Add(dirInfo.Parent.FullName);
				//
				dirInfo = dirInfo.Parent;
			}
			// try to expand all path tokens
			Helper.TreeView.Cursor = System.Windows.Forms.Cursors.WaitCursor;
			Helper.TreeView.BeginUpdate();
			// load on demand was not fired till now
			if (nodeCol.Count == 1 && ((TreeNodePath) nodeCol[0]).Path == null)
			{
				nodeCol[0].Parent.Expand();
			}
			try
			{
				//			
				for (int i = dirs.Count - 1; i >= 0; i--)
				{
					foreach (TreeNodePath n in nodeCol)
					{
						if (n.Path.ToLower().CompareTo(dirs[i].ToString().ToLower()) == 0)
						{
							nodeCol = n.Nodes;
							if (i == 0)
							{
								n.EnsureVisible();
								Helper.TreeView.SelectedNode = n;
							}
							else
							{
								n.Expand();
							}
							break;
						}
					}
				}
			}
			catch (System.Exception e)
			{
				throw e;
			}
			finally
			{
				Helper.TreeView.EndUpdate();
				Helper.TreeView.Cursor = System.Windows.Forms.Cursors.Default;
			}
		}

		#endregion
		

		public override string ToString()
		{
			return "System.IO Provider";
		}

	}
}