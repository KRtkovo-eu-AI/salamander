using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Raccoom.Windows.Forms
{
    public partial class TreeViewFolderBrowserDemoForm : Form
    {
        #region ctor
        public TreeViewFolderBrowserDemoForm()
        {
            InitializeComponent();
            // Hack PropertyGrid to add a custom command
            foreach (Control ctl in this.propertyGrid1.Controls)
            {
                if (!(ctl is ToolStrip)) continue;
                ToolStrip strip = (ToolStrip)ctl;
                strip.RenderMode = ToolStripRenderMode.System;
                strip.Items.Add("3. Apply changes now").Click += delegate(object sender, EventArgs e) { Populate(false); };
            }
        }
        #endregion

        #region internal interface
        protected override void OnShown(EventArgs e)
        {
            base.OnShown(e);
            //
            if (DesignMode) return;
            //
            this.Text = string.Format("{0} - {1} {2}", System.Windows.Forms.Application.CompanyName, System.Windows.Forms.Application.ProductName, System.Windows.Forms.Application.ProductVersion);
            //
            this._checkBoxBehaviourModeCombo.Items.Add(CheckBoxBehaviorMode.None);
            this._checkBoxBehaviourModeCombo.Items.Add(CheckBoxBehaviorMode.SingleChecked);
            this._checkBoxBehaviourModeCombo.Items.Add(CheckBoxBehaviorMode.RecursiveChecked);
            this._checkBoxBehaviourModeCombo.SelectedIndex = 1;
            //
            FillDataProviderCombo(_dataProviderCombo.Items);
            _dataProviderCombo.SelectedIndex = 0;
        }

        /// <summary>
        /// Plumb everything up and fill the tree view
        /// </summary>        
        void Populate(bool providerTypeChanged)
        {
            try
            {
                UseWaitCursor = true;
                //
                if (providerTypeChanged)
                {
                    _treeViewFolderBrowser.DataSource = (Raccoom.Windows.Forms.ITreeStrategyDataProvider)_dataProviderCombo.SelectedItem;
                    this.propertyGrid1.SelectedObject = _treeViewFolderBrowser.DataSource;
                }
                this._treeViewFolderBrowser.CheckBoxBehaviorMode = (CheckBoxBehaviorMode)_checkBoxBehaviourModeCombo.SelectedItem;                    
                _treeViewFolderBrowser.Populate();
            }
            catch (Exception ee)
            {
                MessageBox.Show(ee.Message, Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                UseWaitCursor = false;
            }
        }

        internal static void FillDataProviderCombo(System.Windows.Forms.ComboBox.ObjectCollection collection)
        {
            collection.Clear();
            //			
            Raccoom.Windows.Forms.TreeStrategyShell32Provider shell32Provider = new Raccoom.Windows.Forms.TreeStrategyShell32Provider();
            shell32Provider.EnableContextMenu = true;
            shell32Provider.ShowAllShellObjects = true;
            collection.Add(shell32Provider);
            //
            collection.Add(new Raccoom.Windows.Forms.TreeStrategyFolderBrowserProvider());
            //
            TreeStrategyFTPProvider ftpProvider = new TreeStrategyFTPProvider("ftp.dell.com");
            collection.Add(ftpProvider);
            //
            TreeStrategyDirectoryServices adsProvider = new TreeStrategyDirectoryServices();
            adsProvider.RootPath = "LDAP://localhost";
            collection.Add(adsProvider);
            //
            TreeStrategyCultureInfoProvider ciProvider = new TreeStrategyCultureInfoProvider();
            collection.Add(ciProvider);
        }
        #endregion

        #region events
        /// <summary>
        /// Attach selected data provider and reload the tree view
        /// </summary>
        private void _dataProviderCombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            Populate(true);
        }

        /// <summary>
        /// Visualize an item selection by setting status text and updating the list box
        /// </summary>
        private void _treeViewFolderBrowser_SelectedDirectoriesChanged(object sender, SelectedDirectoriesChangedEventArgs e)
        {
            this.toolStripStatusLabel1.Text = string.Format("Action: Checked, Result: {0} is now {1}", e.Path, e.CheckState.ToString());
            listBox1.DataSource = null;
            this.listBox1.DataSource = _treeViewFolderBrowser.SelectedDirectories;

        }

        /// <summary>
        ///  Visualize the selection of a node by showing the selected node text's in teh status bar
        /// </summary>
        private void treeViewRecursiveChecked_AfterSelect(object sender, System.Windows.Forms.TreeViewEventArgs e)
        {
            TreeNodePath node = e.Node as TreeNodePath;
            if (node != null) this.toolStripStatusLabel1.Text = string.Format("Action: Select, Result: {0}", node.Path);
        }


        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            this.Close();
        }
        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            new AboutBox().ShowDialog(this);
        }
        private void _treeViewFolderBrowser_KeyUp(object sender, KeyEventArgs e)
        {
            switch(e.KeyCode)
            {
                case Keys.F5:
                    if (_treeViewFolderBrowser.SelectedNode == null) return;
                    TreeNodePath node = (TreeNodePath)_treeViewFolderBrowser.SelectedNode;
                    node.Refresh();
                    e.Handled = true;
                    break;
            }
        }
        #endregion
    }
}
