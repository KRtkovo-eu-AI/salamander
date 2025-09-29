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
	/// TreeViewStrategyCultureInfo uses the System.Globalization.CultureInfo class to let the user choose
	/// languages (cultures) in a two level filled tree structure
	/// This class serves as a reference implementation and shows how less code it needs to write
	/// a data provider.	
	/// </summary>
	public class TreeStrategyCultureInfoProvider : TreeStrategyDataProviderBase
	{
		#region fields
		/// <summary>ImageList used to assign icons</summary>
		private System.Windows.Forms.ImageList _imageList;
		/// <summary>needed for designer support</summary>
		private System.ComponentModel.IContainer components;
		/// <summary>flag to control which display name that is used</summary>
		private bool _useNativeNames;
		#endregion

		#region constructor	
		/// <summary>
		/// 
		/// </summary>
		public TreeStrategyCultureInfoProvider()
		{
			InitializeComponent();
		}

		#endregion

		#region public interface
		/// <summary>
		/// Gets or sets the flag that controls which display name that is used.
		/// Native names are in the given language, english names not.
		/// </summary>
		[System.ComponentModel.Category("Behaviour")]
		public bool UseNativeNames
		{
			get
			{
				return _useNativeNames;
			}
			set
			{
				_useNativeNames= value;
			}
		}
		/// <summary>
		/// Attaches the imagelist to the tree view
		/// </summary>
		/// <param name="helper"></param>
		public override void Attach(TreeViewFolderBrowserNodeFactory helper)
		{
			base.Attach (helper);
			//
			helper.TreeView.ImageList = this._imageList;
		}

		/// <summary>
		/// Populates the culture neutrale CultureInfo instances
		/// </summary>
		public override void RequestRootNode()
		{
			base.RequestRootNode ();
			//
			TreeNodeBase node;
			foreach(System.Globalization.CultureInfo ci in System.Globalization.CultureInfo.GetCultures(System.Globalization.CultureTypes.NeutralCultures))
			{
				node = Helper.CreateTreeNode(Helper.TreeView.Nodes, null, _useNativeNames ? ci.NativeName : ci.EnglishName, ci.Name, true, false, true);
				node.ImageIndex = 0;
				node.SelectedImageIndex = 1;
			}
		}
		/// <summary>
		/// Popluates the culture specific CultureInfo instances (Children)
		/// </summary>
		/// <param name="parent"></param>
		/// <param name="e"></param>
		public override void RequestChildNodes(TreeNodePath parent, System.Windows.Forms.TreeViewCancelEventArgs e)
		{
			base.RequestChildNodes (parent, e);
			//			
			TreeNodeBase node;
			foreach(System.Globalization.CultureInfo ci in System.Globalization.CultureInfo.GetCultures(System.Globalization.CultureTypes.SpecificCultures))
			{
				if(ci.Parent.Name == parent.Path)
				{
					node = Helper.CreateTreeNode(parent, _useNativeNames ? ci.NativeName : ci.EnglishName, ci.Name, false, false, true);
					node.ImageIndex = 2;
					node.SelectedImageIndex = 3;
				}
			}
		}

		/// <summary>
		/// Seeks for marked culture specific CultureInfos to mark the parent invariant CultureInfo bold. 
		/// </summary>
		/// <param name="items"></param>
		/// <param name="node"></param>
		/// <returns></returns>
        public override bool HasSelectedChildNodes(System.Collections.ObjectModel.Collection<string> items, TreeNodePath node)
		{
			foreach(string path in items)
			{
				if(path.IndexOf(node.Path)!=-1) return true;
			}
			return base.HasSelectedChildNodes (items, node);
		}

		/// <summary>
		/// Returns the name of this data provider
		/// </summary>
		/// <returns></returns>
		public override string ToString()
		{
			return "CultureInfo Provider";
		}

		#endregion

		#region private interface
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container();
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(TreeStrategyCultureInfoProvider));
			this._imageList = new System.Windows.Forms.ImageList(this.components);
			// 
			// _imageList
			// 
			this._imageList.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit;
			this._imageList.ImageSize = new System.Drawing.Size(16, 16);
			this._imageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("_imageList.ImageStream")));
			this._imageList.TransparentColor = System.Drawing.Color.Transparent;

		}
		#endregion
	}
}
