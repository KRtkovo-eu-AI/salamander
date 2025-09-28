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
    /// TreeViewFolderBrowser works a bite like FolderBrowserDialog but was designed to let the user choose many directories by <c>Chechboxes</c>.	
    /// <seealso cref="CheckBoxBehaviorMode"/><seealso cref="TreeNodePath"/>
    /// </summary>	
    /// <remarks>
    /// Because this class delegates the drive, folder and ImageList specific task's to a <see cref="ITreeStrategyDataProvider"/> instance, this class needs a wired <see cref="ITreeStrategyDataProvider"/> instance before you can call any method wich fill's the tree view.
    /// </remarks>
    [System.ComponentModel.Designer(typeof(TreeViewFolderBrowserDesigner))]
    [System.Drawing.ToolboxBitmap(typeof(System.Windows.Forms.TreeView))]
    [System.ComponentModel.DefaultProperty("CheckboxBehaviorMode"), System.ComponentModel.DefaultEvent("SelectedDirectoriesChanged")]
    public class TreeViewFolderBrowser : System.Windows.Forms.TreeView
    {
        #region fields

        /// <summary>
        /// Fired if a directory was selected or deselected.
        /// </summary>
        public event EventHandler<SelectedDirectoriesChangedEventArgs> SelectedDirectoriesChanged;

        /// <summary>
        /// Fired if a data provider has changed
        /// </summary>
        public event System.EventHandler DataSourceChanged;

        /// <summary>
        /// Fired if a CheckboxBehaviorMode has changed
        /// </summary>
        public event System.EventHandler CheckBoxBehaviorModeChanged;

        /// <summary>holds the path list</summary>
        private System.Collections.ObjectModel.Collection<string> _folderList;

        /// <summary>flag used to suppress CheckItem Event</summary>
        private int _suppressCheck;

        /// <summary>font used to mark nodes which contains checked sub nodes</summary>
        private System.Drawing.Font boldFont_;

        /// <summary>current working mode</summary>
        private CheckBoxBehaviorMode _checkboxBehavior;

        /// <summary>data provider which is responsible to manage this instance</summary>
        private ITreeStrategyDataProvider _dataProvider;

        /// <summary>data provider helper instance</summary>
        private TreeViewFolderBrowserNodeFactory _nodeFactory;

        #endregion

        #region constructors

        /// <summary>
        /// Initializes a new instance of the <c>TreeViewFolderBrowser</c> class.
        /// </summary>
        public TreeViewFolderBrowser()
        {
            InitializeComponent();
            // initalize a new helper instance for this tree view.
            _nodeFactory = new TreeViewFolderBrowserNodeFactory(this);
            //
            this.ContextMenu = new System.Windows.Forms.ContextMenu();
            this.ContextMenu.Popup += new System.EventHandler(OnContextMenu_Popup);
            //
            this.CheckBoxBehaviorMode = CheckBoxBehaviorMode.SingleChecked;
            // init bold font
            boldFont_ = new System.Drawing.Font(this.Font, System.Drawing.FontStyle.Bold);
            // Gets the operating system themes feature. This field is read-only.
            if (System.Windows.Forms.OSFeature.Feature.GetVersionPresent(System.Windows.Forms.OSFeature.Themes) != null)
            {
                ShowRootLines = false;
                ShowLines = false;
            }
        }

        #endregion

        #region public interface

        /// <summary>
        /// Gets or sets <see cref="ITreeStrategyDataProvider"/> which is responsible to fill this <c>TreeViewFolderBrowser</c> instance.
        /// </summary>
        [System.ComponentModel.Browsable(true), System.ComponentModel.Category("DataProvider"), System.ComponentModel.Description("DataSource specifies the DataProvider which is responsible to fill this instance")]
        [System.ComponentModel.DefaultValue(System.Environment.SpecialFolder.MyComputer)]
        [System.ComponentModel.TypeConverter(typeof(System.ComponentModel.ExpandableObjectConverter))]
        public ITreeStrategyDataProvider DataSource
        {
            get { return _dataProvider; }
            set
            {
                bool changed = !object.Equals(_dataProvider, value);
                // Detach old provider
                if (_dataProvider != null && changed)
                {
                    _dataProvider.Detach();
                }
                _dataProvider = value;
                // attach new provider
                if (_dataProvider != null)
                {
                    base.ImageList = null;
                    //
                    _dataProvider.Attach(_nodeFactory);
                }
                else
                {
                    base.ImageList = null;
                }
                //
                if (changed) OnDataSourceChanged(System.EventArgs.Empty);
            }
        }

        /// <summary>
        /// Gets a value indicating whether check boxes are displayed next to the tree nodes in the tree view control.
        /// </summary>
        new public bool CheckBoxes
        {
            get { return base.CheckBoxes; }
            //			set
            //			{
            //				base.CheckBoxes = value;				
            //			}
        }

        /// <summary>
        /// List contains the path for all checked items.
        /// </summary>
        [System.ComponentModel.Browsable(false)]
        public virtual System.Collections.ObjectModel.Collection<string> SelectedDirectories
        {
            get
            {
                if (_folderList == null)
                {
                    _folderList = new System.Collections.ObjectModel.Collection<string>();
                }
                return _folderList;
            }
        }

        /// <summary>
        /// Specify how the tree view handles checkboxes and associated events.
        /// </summary>
        [System.ComponentModel.Browsable(true), System.ComponentModel.Category("DataProvider"), System.ComponentModel.Description("Specify how the tree view handles checkboxes and associated events"), System.ComponentModel.DefaultValue(CheckBoxBehaviorMode.SingleChecked)]
        public virtual CheckBoxBehaviorMode CheckBoxBehaviorMode
        {
            get { return _checkboxBehavior; }
            set
            {
                bool changed = object.Equals(_checkboxBehavior, value);
                //
                _checkboxBehavior = value;
                //
                if (changed) OnCheckBoxBehaviorModeChanged(System.EventArgs.Empty);
            }
        }
        /// <summary>
        /// Clears the TreeView and popluates the root level.
        /// </summary>
        public virtual void Populate()
        {
            Populate(null);
        }

        /// <summary>
        /// Clears the TreeView and popluates the root level.
        /// </summary>
        /// <param name="selectedNodePath">The path of the folder that should be selected after population.</param>
        public virtual void Populate(string selectedNodePath)
        {
            // clear out the old values
            this.BeginUpdate();
            this.Nodes.Clear();
            Cursor = System.Windows.Forms.Cursors.WaitCursor;
            try
            {
                base.CheckBoxes = (this._checkboxBehavior != CheckBoxBehaviorMode.None);
                _dataProvider.RequestRootNode();
                //
                if (selectedNodePath != null && selectedNodePath.Length > 0)
                {
                    _dataProvider.ShowNode(selectedNodePath);
                }
            }
            finally
            {
                this.EndUpdate();
                Cursor = System.Windows.Forms.Cursors.Default;
            }
        }


        #endregion

        #region internal interface

        private void OnContextMenu_Popup(object sender, System.EventArgs e)
        {
            this.OnContextMenuPopup(e);
        }

        /// <summary>
        /// Handles the OnMenuPopup event and invokes <c>QueryContextMenuItems</c> on the current Dataprovider.
        /// </summary>
        /// <param name="e"></param>
        protected virtual void OnContextMenuPopup(System.EventArgs e)
        {
            if (_dataProvider == null) return;
            //
            ContextMenu.MenuItems.Clear();
            //			
            TreeNodePath node = _nodeFactory.TreeView.GetNodeAt(_nodeFactory.TreeView.PointToClient(System.Windows.Forms.Cursor.Position)) as TreeNodePath;
            if (node == null) return;
            //
            _dataProvider.QueryContextMenuItems(node);
        }

        /// <summary>
        /// True to supress OnBeforeCheck Execution, otherwise false to allow it.
        /// </summary>
        /// <param name="suppressEvent"></param>
        protected internal virtual void SuppressCheckEvent(bool suppressEvent)
        {
            this._suppressCheck += (suppressEvent) ? +1 : -1;
        }

        /// <summary>
        /// Indicates if OnBeforeCheck is permitted to call code
        /// </summary>
        protected bool IsCheckEventSuppressed
        {
            get { return this._suppressCheck != 0; }
        }

        /// <summary>
        /// Populates the Directory structure for a given path.
        /// </summary>
        /// <param name="parent">Parent node for which the data is to retrieve</param>
        /// <param name="e"><c>TreeViewCancelEventArgs</c> to abort current expanding.</param>
        protected virtual void RequestChildNodes(TreeNodePath parent, System.Windows.Forms.TreeViewCancelEventArgs e)
        {
            if (parent == null) throw new ArgumentNullException("parent");
            if (e == null) throw new ArgumentNullException("e");
            //
            if (!parent.HasDummyNode || parent.Path == null) return;
            // everything ok, here we go
            this.BeginUpdate();
            try
            {
                parent.RemoveDummyNode();
                // if we have not scanned this folder before
                _dataProvider.RequestChildNodes(parent, e);
            }
            finally
            {
                this.EndUpdate();
            }
        }

        /// <summary>
        /// Toggle the check flag for tree nodes, works recursive
        /// </summary>
        /// <param name="parent"></param>
        /// <param name="check"></param>
        protected virtual void CheckNodesRecursive(System.Windows.Forms.TreeNode parent, bool check)
        {
            if (parent == null) throw new ArgumentNullException("parent");
            //
            foreach (System.Windows.Forms.TreeNode n in parent.Nodes)
            {
                n.Checked = check;
                //
                CheckNodesRecursive(n, check);
            }
        }

        /// <summary>
        /// Add or removes the nodes recursive to or from the folderList_.
        /// </summary>
        /// <param name="parent"></param>
        /// <param name="add"></param>
        protected virtual void ExchangeFoldersRecursive(TreeNodePath parent, bool add)
        {
            if (parent == null) throw new ArgumentNullException("parent");
            //
            foreach (TreeNodePath n in parent.Nodes)
            {
                if (n.Path != null)
                {
                    ExchangePath(n.Path, add);
                    MarkNode(parent);
                }
                //
                ExchangeFoldersRecursive(n, add);
            }
        }

        /// <summary>
        /// Add or removes path from the folderList_.
        /// </summary>
        /// <param name="path"></param>
        /// <param name="add"></param>
        protected virtual void ExchangePath(string path, bool add)
        {
            if (string.IsNullOrEmpty(path)) throw new ArgumentNullException("path");
            //
            if (add)
            {
                if (!_folderList.Contains(path))
                {
                    _folderList.Add(path);
                    // notfiy add
                    OnSelectedDirectoriesChanged(new SelectedDirectoriesChangedEventArgs(path, System.Windows.Forms.CheckState.Checked));
                }
            }
            else
            {
                if (_folderList.Contains(path))
                {
                    _folderList.Remove(path);
                    // notfiy remove
                    OnSelectedDirectoriesChanged(new SelectedDirectoriesChangedEventArgs(path, System.Windows.Forms.CheckState.Unchecked));
                }
            }
        }

        /// <summary>
        /// Set the text bold if there is a child node checked.
        /// </summary>
        /// <param name="node"></param>
        protected internal virtual void MarkNode(TreeNodePath node)
        {
            if (node == null) throw new ArgumentNullException("node");
            //
            if (this._checkboxBehavior == CheckBoxBehaviorMode.None) return;
            //
            if (node == null) return;
            // no path selected, no node could be marked
            if (_folderList.Count == 0)
            {
                if ((node.NodeFont != null) && (node.NodeFont.Bold))
                {
                    node.NodeFont = this.Font;
                }
                return;
            }
            // check current node path against selected item list (folderlist_)
            if (_dataProvider.HasSelectedChildNodes(_folderList, node))
            {
                node.NodeFont = boldFont_;
            }
            else
            {
                node.NodeFont = this.Font;
            }
        }

        /// <summary>
        /// Set the text bold for each parent node if there is a child node checked.
        /// </summary>
        /// <param name="parent"></param>
        protected virtual void MarkNodesRecursive(TreeNodePath parent)
        {
            if (this._checkboxBehavior == CheckBoxBehaviorMode.None) return;
            //
            if (parent == null) return;
            //
            MarkNode(parent);
            if (parent.Parent != null)
            {
                MarkNodesRecursive(parent.Parent as TreeNodePath);
            }
        }

        protected virtual void UIVisualizeErrorMessage(string mainTitle, Exception ex)
        {
            System.Windows.Forms.MessageBox.Show(mainTitle + Environment.NewLine + Environment.NewLine + ex.Message, System.Windows.Forms.Application.ProductName, System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Error, System.Windows.Forms.MessageBoxDefaultButton.Button1, System.Windows.Forms.MessageBoxOptions.DefaultDesktopOnly, false);
        }

        #endregion

        #region events
        /// <summary>
        /// Raises the CheckboxBehaviorModeChanged event.
        /// </summary>
        /// <param name="e">An EventArgs that contains the event data.</param>
        protected virtual void OnCheckBoxBehaviorModeChanged(System.EventArgs e)
        {
            if (CheckBoxBehaviorModeChanged != null) CheckBoxBehaviorModeChanged(this, e);
        }

        /// <summary>
        /// Raises the DataSourceChanged event.
        /// </summary>
        /// <param name="e">An EventArgs that contains the event data.</param>
        protected virtual void OnDataSourceChanged(System.EventArgs e)
        {
            if (DataSourceChanged != null) DataSourceChanged(this, e);
        }

        /// <summary>
        /// Used for drives like floppy, cd - rom ect. where it can be that no valid medium is inserted.
        /// in this case the click on the + will remove the +, after double click there's a new + to give the user
        /// the chance to browse this device after inserting a valid medium.
        /// </summary>		
        protected override void OnDoubleClick(System.EventArgs e)
        {
            if (this.SelectedNode == null) return;
            //
            TreeNodePath node = this.SelectedNode as TreeNodePath;
            if (node == null || node.Path == null) return;
            //
            if ((node.Nodes.Count > 0) || (node.Path.Length > 3)) return;
            //
            node.AddDummyNode();
            //
            base.OnDoubleClick(e);
        }

        /// <summary>
        /// Fired before check action occurs, manages the folderList_.
        /// </summary>		
        protected override void OnBeforeCheck(System.Windows.Forms.TreeViewCancelEventArgs e)
        {
            // check suppress flag
            if (IsCheckEventSuppressed)
            {
                base.OnBeforeCheck(e);
                return;
            }
            // get current action		
            bool check = !e.Node.Checked;
            // is it allowed to check item ?
            if ((this.CheckBoxBehaviorMode == CheckBoxBehaviorMode.RecursiveChecked) && (!check) && (e.Node.Parent != null) && (e.Node.Parent.Checked))
            {
                e.Cancel = true;
                base.OnBeforeCheck(e);
                return;
            }
            // set supress flag
            SuppressCheckEvent(true);
            // stop drawing tree content
            this.BeginUpdate();
            // set cursor
            this.Cursor = System.Windows.Forms.Cursors.WaitCursor;
            //
            try
            {
                // add or remove path
                ExchangePath(((TreeNodePath)e.Node).Path, check);
                // handle recursive behaviour
                if (this.CheckBoxBehaviorMode == CheckBoxBehaviorMode.RecursiveChecked)
                {
                    // remove all childs from folderList_
                    ExchangeFoldersRecursive(e.Node as TreeNodePath, false);
                    // check child nodes to reflect parent check state
                    CheckNodesRecursive(e.Node, check);
                }
                // update marked nodes fonts
                MarkNodesRecursive(e.Node.Parent as TreeNodePath);
            }
            catch (System.Exception exception)
            {
                UIVisualizeErrorMessage("OnBeforeCheck", exception);
            }
            finally
            {
                // reset supress flag
                SuppressCheckEvent(false);
                // let the tree redraw his content
                this.EndUpdate();
                // reset the cursor
                Cursor = System.Windows.Forms.Cursors.Default;
            }
            //
            base.OnBeforeCheck(e);
        }

        /// <summary>
        /// Fired before node expands, used to fill next level in directory structure.
        /// </summary>		
        protected override void OnBeforeExpand(System.Windows.Forms.TreeViewCancelEventArgs e)
        {
            TreeNodePath node = e.Node as TreeNodePath;
            //
            Cursor = System.Windows.Forms.Cursors.WaitCursor;
            try
            {
                RequestChildNodes(node, e);
            }
            catch (System.Exception ex)
            {
                UIVisualizeErrorMessage("OnBeforeExpand", ex);
                e.Cancel = true;
            }
            finally
            {
                Cursor = System.Windows.Forms.Cursors.Default;
            }
            //
            base.OnBeforeExpand(e);
        }

        /// <summary>
        /// Raises the SelectedDirectoriesChanged event.<seealso cref="SelectedDirectoriesChangedEventArgs"/>
        /// </summary>
        protected virtual void OnSelectedDirectoriesChanged(SelectedDirectoriesChangedEventArgs e)
        {
            if (this.SelectedDirectoriesChanged != null) SelectedDirectoriesChanged(this, e);
        }

        #endregion

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            // 
            // TreeViewFolderBrowser
            // 
            this.HideSelection = false;

        }

        #endregion
    }

    /// <summary>
    /// A simple designer class for the <see cref="TreeViewFolderBrowser"/> control to remove 
    /// unwanted properties at design time.
    /// </summary>
    public class TreeViewFolderBrowserDesigner : System.Windows.Forms.Design.ControlDesigner
    {
        /// <summary>
        /// Allows a designer to change or remove items from the set of properties that it exposes through a TypeDescriptor. 
        /// </summary>
        /// <param name="properties">The properties for the class of the component.</param>
        protected override void PreFilterProperties(System.Collections.IDictionary properties)
        {
            properties.Remove("CheckBoxes");
            properties.Remove("ImageList");
            properties.Remove("SelectedImageIndex");
            properties.Remove("ImageIndex");
            properties.Remove("ContextMenu");
        }
    }


    /// <summary>
    /// Provides data for the SelectedDirectoriesChangedDelegate event of a TreeViewFolderBrowser control.
    /// </summary>
    public class SelectedDirectoriesChangedEventArgs : System.EventArgs
    {
        #region fields

        /// <summary>File path</summary>
        private readonly string _path;

        /// <summary>Checkstate</summary>
        private readonly System.Windows.Forms.CheckState _checkState;

        #endregion

        #region constructors

        /// <summary>Initalize a new instance of SelectedDirectoriesChangedEventArgs</summary>
        public SelectedDirectoriesChangedEventArgs(string path, System.Windows.Forms.CheckState checkState)
        {
            _path = path;
            _checkState = checkState;
        }

        #endregion

        #region public interface

        /// <summary>Gets the path which was modified</summary>
        public string Path
        {
            get { return _path; }
        }

        /// <summary>Gets the check state for the path</summary>
        public System.Windows.Forms.CheckState CheckState
        {
            get { return _checkState; }
        }

        #endregion
    }

    /// <summary>
    /// Indicating whether check boxes are displayed next to the tree nodes in the tree view control and how the tree view handle related events.
    /// </summary>
    public enum CheckBoxBehaviorMode
    {
        /// <summary>
        /// No check boxes are displayed next to the tree nodes in the tree view control.
        /// </summary>
        None,
        /// <summary>
        /// Check boxes are displayed next to the tree nodes in the tree view control. The user can check directories.
        /// </summary>
        SingleChecked,
        /// <summary>
        /// Check boxes are displayed next to the tree nodes in the tree view control. The user can check directories, the subdirectories are checked recursive.
        /// </summary>
        RecursiveChecked
    }
}