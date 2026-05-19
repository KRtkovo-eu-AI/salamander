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
	/// <c>TreeViewFolderBrowserDataProviderADS</c>  implements <c>TreeStrategyDataProviderBase</c> and provides a explorer like Active Directory browsing
	/// </summary>
	public class TreeStrategyDirectoryServices : System.ComponentModel.Component, ITreeStrategyDataProvider
	{
		#region fields

		/// <summary>Imagelist used for ADS Nodes</summary>
		private System.Windows.Forms.ImageList _imageList;

		/// <summary>Designer support</summary>
		private System.ComponentModel.IContainer components;

		/// <summary>Helper class to access the managed TreeViewFolderBrowser</summary>
		private TreeViewFolderBrowserNodeFactory _helper;

		/// <summary>Root path</summary>
		private string _rootPath = string.Empty;

		/// <summary>credential used to connect to the machine</summary>
		private System.Net.NetworkCredential _credential = null;

		/// <summary>AuthenticationType used to connect to the machine</summary>
		private System.DirectoryServices.AuthenticationTypes _authenticationType;

		#endregion

		#region constructors

		/// <summary>
		/// Initializes a new instance of the <c>TreeStrategyDirectoryServices</c> class.
		/// </summary>
		public TreeStrategyDirectoryServices()
		{
			InitializeComponent();
		}

		/// <summary>
		/// Initializes a new instance of the <c>TreeStrategyDirectoryServices</c> class.
		/// </summary>
		/// <param name="authenticationType">Used AuthenticationType</param>
		/// <param name="credential">Credential used to retrieve Data from the Directory Services.</param>
		public TreeStrategyDirectoryServices(System.Net.NetworkCredential credential, System.DirectoryServices.AuthenticationTypes authenticationType) : this()
		{
			this._credential = credential;
			this._authenticationType = authenticationType;
		}

		#endregion

		#region public interface
		/// <summary>
		/// Gets or sets the root folder where the browsing starts from.
		/// </summary>
		public string RootPath
		{
			get { return _rootPath; }
			set { _rootPath = value; }
		}
        /// <summary>
        /// Gets or sets the NetworkCredential used to connect to the machine
        /// </summary>
		[System.ComponentModel.Category("Data"), System.ComponentModel.Description("")]
		[System.ComponentModel.TypeConverter(typeof (System.ComponentModel.ExpandableObjectConverter))]
		public System.Net.NetworkCredential Credential
		{
			get { return _credential; }
			set { _credential = value; }
		}
		/// <summary>
		/// Gets or sets the AuthenticationType used to connect to the machine
		/// </summary>
		public System.DirectoryServices.AuthenticationTypes AuthenticationType
		{
			get { return _authenticationType; }
			set { _authenticationType = value; }
		}

		#endregion

		#region ITreeStrategyDataProvider Members
		public void Attach(TreeViewFolderBrowserNodeFactory helper)
		{
			_helper = helper;
			//
			_helper.TreeView.ImageList = this._imageList;
		}

		public void Detach()
		{
		}

		public void QueryContextMenuItems(TreeNodePath node)
		{
		}
		public void RequestChildNodes(TreeNodePath parent, System.Windows.Forms.TreeViewCancelEventArgs e)
		{
			System.DirectoryServices.DirectoryEntry parentEntry = parent.Tag as System.DirectoryServices.DirectoryEntry;
			foreach (System.DirectoryServices.DirectoryEntry entry in parentEntry.Children)
			{
				TreeNodePath node = _helper.CreateTreeNode(parent, entry.Name.Substring(3), entry.Path, true, false, false);
				node.Tag = entry;
				SetIcon(entry, node);
			}
		}

		public void RequestRootNode()
		{
			System.DirectoryServices.DirectoryEntry ds = null;
			//
			if (_credential != null)
			{
				ds = new System.DirectoryServices.DirectoryEntry(_rootPath, Credential.UserName, Credential.Password, AuthenticationType);
			}
			else
			{
				ds = new System.DirectoryServices.DirectoryEntry(_rootPath);
			}
			try
			{
				foreach (System.DirectoryServices.DirectoryEntry entry in ds.Children)
				{
					TreeNodePath node = _helper.CreateTreeNode(null, entry.Name.Substring(3), entry.Path, true, false, false);
					node.Tag = entry;
					SetIcon(entry, node);
				}

			}
			finally
			{
				if (ds != null) ds.Close();
			}
		}

		public void ShowNode(string path)
		{
			
		}

        public bool HasSelectedChildNodes(System.Collections.ObjectModel.Collection<string> items, TreeNodePath node)
		{
			bool isBold = false;
			//
			string[] nodePathTokens = ExtractTokens(node.Path);
			//
			foreach (string s in items)
			{
				string[] itemPathTokens = ExtractTokens(s);
				//
				int indexSource = nodePathTokens.GetUpperBound(0);
				int indexTarget = itemPathTokens.GetUpperBound(0);
				do
				{
					if(!nodePathTokens[indexSource].Equals(itemPathTokens[indexTarget]))
					{
						isBold = false;
						break;
					} 
					else
					{
						isBold = true;
					}
					//
					indexSource--;
					indexTarget--;
				}
				while(indexSource>=0 && indexTarget>=0);
				//
				isBold = isBold & indexSource < indexTarget;
				//
				if(isBold) break;
			}
			return isBold;
		}
		#endregion

		#region internal interface
		/// <summary>
		/// Determines and sets the ImageIndex for <c>node</c>
		/// </summary>
		/// <param name="entry">ADS Entry</param>
		/// <param name="node">TreeNode which represents <c>entry</c></param>
		private void SetIcon(System.DirectoryServices.DirectoryEntry entry, TreeNodePath node)
		{
			switch (entry.SchemaClassName)
			{
				case "organizationalUnit":
					node.ImageIndex = 0;
					node.SelectedImageIndex = 1;
					break;
				case "container":
					node.ImageIndex = 2;
					node.SelectedImageIndex = 3;
					break;
				case "user":
					node.ImageIndex = 5;
					node.SelectedImageIndex = 5;
					break;
				case "group":
					node.ImageIndex = 4;
					node.SelectedImageIndex = 4;
					break;
				case "printQueue":
					node.ImageIndex = 9;
					node.SelectedImageIndex = 9;
					break;
				case "computer":
					node.ImageIndex = 7;
					node.SelectedImageIndex = 7;
					break;
				default:
					node.ImageIndex = -1;
					node.SelectedImageIndex = -1;
					break;
			}
		}
		/// <summary>
		/// Extract the tokens from the ldap string
		/// </summary>
		/// <param name="value"></param>
		/// <returns></returns>
		private string[] ExtractTokens(string value)
		{
			return value.Replace("LDAP://","").Split(',');
			
		}
		/// <summary>
		/// Designer support
		/// </summary>
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container();
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof (TreeStrategyDirectoryServices));
			this._imageList = new System.Windows.Forms.ImageList(this.components);
			// 
			// _imageList
			// 
			this._imageList.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit;
			this._imageList.ImageSize = new System.Drawing.Size(16, 16);
			this._imageList.TransparentColor = System.Drawing.Color.Transparent;

		}

		#endregion
	
		/// <summary>
		/// Returns the display name for the <c>TreeStrategyDirectoryServices</c> dataprovider
		/// </summary>
		/// <returns></returns>
		public override string ToString()
		{
			return "DirectoryServices Provider";
		}

	}
}