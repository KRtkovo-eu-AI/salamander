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
    /// <c>TreeViewFolderBrowserNodeFactory</c> is like a bridge between <see cref="ITreeStrategyDataProvider"/> and <see cref="TreeViewFolderBrowser"/>
	/// and provides access to the needed informations to fill the tree view.
	/// </summary>
	public class TreeViewFolderBrowserNodeFactory
	{
		#region fields

		/// <summary>the managed tree view instance</summary>
		private TreeViewFolderBrowser _treeView;

		/// <summary>Delegate to invoke thread safe method calls</summary>
		private delegate TreeNodePath CreateTreeNodeDelegate(TreeNodePath parentNode, string text, string path, bool addDummyNode, bool forceChecked, bool isSpecialFolder);

		
		/// <summary>Delegate used to handle wait cursor</summary>
		private delegate void HandleWaitCursorDelegate(bool show);

		/// <summary>Delegate used to expand a parent node</summary>
		private delegate void ExpandNodeDelegate(System.Windows.Forms.TreeNode parentNode);

		#endregion

		#region constructors

		/// <summary>
		/// Initialize a new instance of TreeViewFolderBrowserHelper for the specified TreeViewFolderBrowser instance.
		/// </summary>
		/// <param name="treeView"></param>
		internal TreeViewFolderBrowserNodeFactory(TreeViewFolderBrowser treeView)
		{
			_treeView = treeView;
		}

		#endregion

		#region public interface

		/// <summary>
		/// Gets the underlying <see cref="TreeViewFolderBrowser"/> instance.
		/// </summary>
		public TreeViewFolderBrowser TreeView
		{
			get { return _treeView; }
		}

		/// <summary>
		/// Creates a tree node and add it to the <c>parentNode</c>.
		/// </summary>		
		/// <param name="parentNode">Parent node for the new created child node, can be null to indicate root level.</param>
		/// <param name="text">The text displayed in the label of the tree node.</param>
		/// <param name="path">The path the node represents.</param>
		/// <param name="addDummyNode">True to add + sign, otherwise no + sign appears.</param>
		/// <param name="forceChecked">True to check node in each case, otherwise false to allow normal check against selected paths.</param>
		/// <param name="isSpecialFolder">Specifies if this node is a special folder. Special folders do not request data from the attached data provider.</param>
		/// <returns>The newly created and added node</returns>
		public virtual TreeNodePath CreateTreeNode(TreeNodePath parentNode, string text, string path, bool addDummyNode, bool forceChecked, bool isSpecialFolder)
		{
			return CreateTreeNode(parentNode != null ? parentNode.Nodes : TreeView.Nodes, parentNode, text, path, addDummyNode, forceChecked, isSpecialFolder);
		}

		/// <summary>
		/// Creates a tree node and add it to the <c>TreeNodeCollection</c>.
		/// </summary>		
		/// <param name="parentCollection"><c>TreeNodeCollection</c> to which the new node will added.</param>
		/// <param name="parentNode">Parent node for the new created child node.</param>
		/// <param name="text">The text displayed in the label of the tree node.</param>
		/// <param name="path">The path the node represents.</param>
		/// <param name="addDummyNode">True to add + sign, otherwise no + sign appears.</param>
		/// <param name="forceChecked">True to check node in each case, otherwise false to allow normal check against selected paths.</param>
		/// <param name="isSpecialFolder">Specifies if this node is a special folder. Special folders do not request data from the attached data provider.</param>
		/// <returns>The newly created and added node</returns>
		public virtual TreeNodePath CreateTreeNode(System.Windows.Forms.TreeNodeCollection parentCollection, TreeNodePath parentNode, string text, string path, bool addDummyNode, bool forceChecked, bool isSpecialFolder)
		{
            if (parentCollection == null) throw new ArgumentNullException("parentCollection");
            //
			if (_treeView.InvokeRequired)
			{
				return _treeView.Invoke(new CreateTreeNodeDelegate(CreateTreeNode), new object[] {parentNode, text, path, addDummyNode, forceChecked, isSpecialFolder}) as TreeNodePath;
			}
			//
			forceChecked = forceChecked || (TreeView.CheckBoxBehaviorMode == CheckBoxBehaviorMode.RecursiveChecked && (parentNode != null && parentNode.Checked));
			//
			TreeNodePath childNode = CreateTreeNode(text, path, addDummyNode, forceChecked, isSpecialFolder);
			parentCollection.Add(childNode);
			return childNode;
		}
		/// <summary>
		/// Expands the tree node.
		/// </summary>
		/// <param name="parentNode">The Expand method expands the current TreeNode down to the next level of nodes.</param>
		public virtual void ExpandNode(System.Windows.Forms.TreeNode parentNode)
		{
			if (_treeView.InvokeRequired)
			{
				_treeView.Invoke(new ExpandNodeDelegate(ExpandNode), new object[] {parentNode});
				return;
			}
			System.Diagnostics.Debug.Assert(parentNode != null);
			parentNode.Expand();
		}

		#endregion

		#region private interface
		/// <summary>
		/// Thread safe Wait Cursor handling
		/// </summary>
		/// <param name="show"></param>
		private void HandleWaitCursor(bool show)
		{
			if (_treeView.InvokeRequired)
			{
				TreeView.Invoke(new HandleWaitCursorDelegate(HandleWaitCursor), new object[] {show});
				return;
			}
			if (!show)
			{
				TreeView.Cursor = System.Windows.Forms.Cursors.Default;
			}
			else
			{
				TreeView.Cursor = System.Windows.Forms.Cursors.WaitCursor;
			}
		}

		/// <summary>
		/// Creates a tree node and add it to the <c>TreeNodeCollection</c>.
		/// </summary>
		/// <param name="text">The text displayed in the label of the tree node.</param>
		/// <param name="path">The path the node represents.</param>
		/// <param name="addDummyNode">True to add + sign, otherwise no + sign appears.</param>
		/// <param name="forceChecked">True to check node in each case, otherwise false to allow normal check against selected paths.</param>
		/// <param name="isSpecialFolder">Specifies if this node is a special folder. Special folders do not request data from the attached data provider.</param>
		/// <returns></returns>
		private TreeNodePath CreateTreeNode(string text, string path, bool addDummyNode, bool forceChecked, bool isSpecialFolder)
		{
			//
			TreeNodePath newNode = new TreeNodePath(text, isSpecialFolder);
			// path
			newNode.Path = path;
			//						
			try
			{
				_treeView.SuppressCheckEvent(true);
				//
				if (forceChecked)
				{
					newNode.Checked = true;
				}
				else
				{
					newNode.Checked = _treeView.SelectedDirectories.Contains(path);
				}
				_treeView.MarkNode(newNode);
			}
			catch (System.ApplicationException e)
			{
				System.Diagnostics.Debug.WriteLine(e.Message, _treeView.Name);
			}
			finally
			{
				_treeView.SuppressCheckEvent(false);
			}
			//
			if (addDummyNode)
			{
				// add dummy node, otherwise there is no + sign
				newNode.AddDummyNode();
			}
			//
			return newNode;
		}

		#endregion
	}
}