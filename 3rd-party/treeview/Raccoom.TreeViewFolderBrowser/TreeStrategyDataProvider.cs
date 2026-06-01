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
	/// <c>ITreeViewFolderBrowserDataProvider</c> is used by a <see cref="TreeViewFolderBrowser"/> instance and is responsible for
	/// <list type="bullet"> 
	/// <item>retrieve the computer drives and directories</item>
	/// <item>Imagelist which is used to assign images to the nodes created by this instance.</item>  
	/// <item>ContextMenu</item>
	/// </list>
	/// <see cref="TreeViewFolderBrowser"/> calls the interface method's and provide a <see cref="TreeViewFolderBrowserHelper"/> instance which let you create node's and give you access to the underlying <see cref="TreeViewFolderBrowser"/> instance.
	/// </summary>
	public interface ITreeStrategyDataProvider
	{
		/// <summary>
		/// The <c>TreeViewFolderBrowser</c> calls this method to let the provider initialize itself.
		/// </summary>
		/// <param name="helper">Helper class to create TreeNodes</param>
		void Attach(TreeViewFolderBrowserNodeFactory helper);

		/// <summary>
		/// The <c>TreeViewFolderBrowser</c> calls this method to let the provider detach itself.
		/// </summary>
		void Detach();

		/// <summary>
		/// Fired before the context menu popup.
		/// </summary>
		/// <param name="node">The node on which the context menu was requested.</param>
		void QueryContextMenuItems(TreeNodePath node);

		/// <summary>
		/// Fill the root level.
		/// </summary>
		void RequestRootNode();

		/// <summary>
		/// Fill the Directory structure for a given path.
		/// </summary>
		/// <param name="parent">The expanding node.</param>
		/// <param name="e">EventArgs that let you cancel the node expanding.</param>
		void RequestChildNodes(TreeNodePath parent, System.Windows.Forms.TreeViewCancelEventArgs e);

		/// <summary>
		/// Gets the tree node collection which holds the drive node's. The requested collection is than used to search a specific node.
		/// </summary>
		void ShowNode(string path);
		/// <summary>
		/// Determines if the node has to be a marked bold, because the <c>items</c> collection contains <c>node</c> child items.
		/// </summary>
		/// <param name="items">The selected items we have to check against for the <c>node</c></param>
		/// <param name="node">The node for which we have to check the IsBold State</param>
		/// <returns>True to mark the node bold, False to use the regular Font</returns>
        bool HasSelectedChildNodes(System.Collections.ObjectModel.Collection<string> items, TreeNodePath node);
	}

	/// <summary>
	/// <c>TreeViewFolderBrowserDataProviderBase</c> serves as a abstract standard implementation for <see cref="ITreeStrategyDataProvider"/> and is responsible for
	/// <list type="bullet"> 
	/// <item>retrieve the computer drives and directories</item>
	/// <item>Imagelist which is used to assign images to the nodes created by this instance.</item>  
	/// <item>ContextMenu</item>
	/// </list>
	/// </summary>
	public abstract class TreeStrategyDataProviderBase : ITreeStrategyDataProvider
	{
		#region fields

		/// <summary>Holds an instance to the Helper Class provided trough attach process</summary>
		private TreeViewFolderBrowserNodeFactory _helper;

		#endregion

		#region ITreeStrategyDataProvider Members

		/// <summary>
		/// Gets the helper class needed to create nodes
		/// </summary>
		protected virtual TreeViewFolderBrowserNodeFactory Helper
		{
			get { return _helper; }
		}

		/// <summary>
		/// The <c>TreeViewFolderBrowser</c> calls this method to let the provider initialize itself.
		/// </summary>
		/// <param name="helper">Helper class to create TreeNodes</param>
		public virtual void Attach(TreeViewFolderBrowserNodeFactory helper)
		{
			_helper = helper;
		}

		/// <summary>
		/// The <c>TreeViewFolderBrowser</c> calls this method to let the provider detach itself.
		/// </summary>
		public virtual void Detach()
		{
			_helper = null;
		}	

		/// <summary>
		/// Fired before the context menu popup.
		/// </summary>
		/// <param name="node">The node on which the context menu was requested.</param>
		public virtual void QueryContextMenuItems(TreeNodePath node)
		{
		}

		/// <summary>
		/// Fill the root level.
		/// </summary>
		public virtual void RequestRootNode()
		{
		}

		/// <summary>
		/// Fill the Directory structure for a given path.
		/// </summary>
		/// <param name="parent">The expanding node.</param>
		/// <param name="e">EventArgs that let you cancel the node expanding.</param>
		public virtual void RequestChildNodes(TreeNodePath parent, System.Windows.Forms.TreeViewCancelEventArgs e)
		{
		}

		/// <summary>
		/// Gets the tree node collection which holds the drive node's. The requested collection is than used to search a specific node.
		/// </summary>
		public virtual void ShowNode(string path)
		{
		}
        /// <summary>
        /// Checks if a given child nodes is contained in the given selection
        /// </summary>
        /// <param name="items"></param>
        /// <param name="node"></param>
        /// <returns></returns>
        public virtual bool HasSelectedChildNodes(System.Collections.ObjectModel.Collection<string> items, TreeNodePath node)
		{
			return false;
		}

		#endregion
	}

}