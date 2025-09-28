using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Drawing.Design;
using Newtonsoft.Json;
using System.IO;
using System.Threading;
using EPocalipse.Json.Viewer.Properties;
using System.Threading.Tasks;

namespace EPocalipse.Json.Viewer
{
    public partial class JsonViewer : UserControl, INotifyPropertyChanged
    {
        private string _json;
        private int _mode = 0;
        private JsonObjectTree _tree;
        private JsonObjectTree _oldTree;
        private ErrorDetails _errorDetails;
        private readonly PluginsManager _pluginsManager = new PluginsManager();
        bool _updating;
        Control _lastVisualizerControl;

        public event PropertyChangedEventHandler PropertyChanged;

        public JsonViewer()
        {
            InitializeComponent();
            try
            {
                _pluginsManager.Initialize();
            }
            catch( Exception e )
            {
                MessageBox.Show(string.Format( Resources.ConfigMessage, e.Message ), "Json Viewer", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            backgroundWorker1.WorkerSupportsCancellation = true;
            tabControl.SelectedIndexChanged += new EventHandler(tabControl_SelectedIndexChanged);
        }

        private void tabControl_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch ((sender as TabControl).SelectedIndex)
            {
                case 0:
                    if (_tree != null && _tree != _oldTree)
                    {
                        VisualizeJsonTree(_tree);
                        _oldTree = _tree;
                    }
                    break;
            }
        }

        [Editor( "System.ComponentModel.Design.MultilineStringEditor, System.Design, Version=2.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a", typeof( UITypeEditor ) )]
        public string Json
        {
            get
            {
                return _json;
            }
            set
            {
                if( _json != value )
                {
                    _json = value.Trim();
                    Redraw();
                }
            }
        }

        // 0: ready/idle
        // 1: waiting for user
        // 2: parsing
        // 3: error
        [DefaultValue(0)]
        public int Mode
        {
            get
            {
                return _mode;
            }
            set
            {
                _mode = value;
                PropertyChanged(this, new PropertyChangedEventArgs("Mode"));
            }
        }

        [DefaultValue(25)]
        public int MaxErrorCount { get; set; } = 25;

        private void Redraw()
        {
            try
            {
                tvJson.BeginUpdate();
                try
                {
                    if( !string.IsNullOrEmpty( _json ) )
                    {
                        backgroundWorker1.RunWorkerAsync();
                    } else
                    {
                        Reset();
                    }
                }
                finally
                {
                    tvJson.EndUpdate();
                }
            }
            catch( Exception e )
            {
                ShowException( e );
            }
        }

        private void Reset()
        {
            ClearInfo();
            tvJson.Nodes.Clear();
            pnlVisualizer.Controls.Clear();
            _lastVisualizerControl = null;
            cbVisualizers.Items.Clear();
            GC.Collect();
        }

        private void GetParseErrorDetails( Exception parserError )
        {
            UnbufferedStringReader strReader = new UnbufferedStringReader( _json );
            using( JsonReader reader = new JsonTextReader( strReader ) )
            {
                try
                {
                    while( reader.Read() )
                    { };
                }
                catch( Exception e )
                {
                    _errorDetails._err = e.Message;
                    _errorDetails._pos = strReader.Position;
                }
            }
            if( _errorDetails.Error == null )
                _errorDetails._err = parserError.Message;
            if( _errorDetails.Position == 0 )
                _errorDetails._pos = _json.Length;
            if( !txtJson.ContainsFocus )
                // MarkError( _errorDetails );
            ShowInfo( _errorDetails );
        }

        private void MarkError( ErrorDetails _errorDetails )
        {
            Action markErr = () => {
                txtJson.Select(Math.Max(0, _errorDetails.Position - 1), 10);
                txtJson.ScrollToCaret();
            };
            txtJson.Invoke(markErr);
        }

        private void VisualizeJsonTree( JsonObjectTree tree )
        {
            if (tree == null) return;
            tvJson.Nodes.Clear();
            AddNode( tvJson.Nodes, tree.Root );
            JsonViewerTreeNode node = GetRootNode();
            InitVisualizers( node );
            node.Expand();
            tvJson.SelectedNode = node;
            GC.Collect();
        }

        private void AddNode( TreeNodeCollection nodes, JsonObject jsonObject )
        {
            JsonViewerTreeNode newNode = new JsonViewerTreeNode( jsonObject );
            nodes.Add( newNode );
            newNode.Text = jsonObject.Text;
            newNode.Tag = jsonObject;
            newNode.ImageIndex = (int)jsonObject.JsonType;
            newNode.SelectedImageIndex = newNode.ImageIndex;

            foreach( JsonObject field in jsonObject.Fields )
            {
                AddNode( newNode.Nodes, field );
            }
        }

        [Browsable( false )]
        public ErrorDetails ErrorDetails
        {
            get
            {
                return _errorDetails;
            }
        }

        public void Clear()
        {
            Json = string.Empty;
        }

        public void ShowInfo( string info )
        {
            Action errInfo = () =>
            {
                lblError.Text = info;
                lblError.Tag = null;
                lblError.Enabled = false;
            };
            lblError.Invoke(errInfo);
            Action tabErrInfo = () => tabControl.SelectedTab = pageTextView;
            tabControl.Invoke(tabErrInfo);
        }

        public void ShowInfo( ErrorDetails error )
        {
            ShowInfo( error.Error );
            Action errInfo = () =>
            {
                lblError.Text = error.Error;
                lblError.Tag = error;
                lblError.Enabled = true;
            };
            lblError.Invoke(errInfo);
            Action tabErrInfo = () => tabControl.SelectedTab = pageTextView;
            tabControl.Invoke(tabErrInfo);
        }

        public void ClearInfo()
        {
            lblError.Text = string.Empty;
        }

        [Browsable( false )]
        public bool HasErrors
        {
            get
            {
                return _errorDetails._err != null;
            }
        }

        public int MaxErrorCount1 { get => MaxErrorCount; set => MaxErrorCount = value; }

        private async void txtJson_TextChangedAsync( object sender, EventArgs e )
        {
            Mode = 1;
            async Task<bool> UserKeepsTyping()
            {
                string txt = txtJson.Text;
                await Task.Delay(500);
                return txt != txtJson.Text;
            }
            if (await UserKeepsTyping()) return;
            ClearInfo();
            Mode = 2;
            Json = txtJson.Text;
            if (Mode == 2) Mode = 0;
        }

        private void txtFind_TextChanged( object sender, EventArgs e )
        {
            txtFind.BackColor = SystemColors.Window;
            FindNext( true, true );
        }

        public bool FindNext( bool includeSelected )
        {
            return FindNext( txtFind.Text, includeSelected );
        }

        public void FindNext( bool includeSelected, bool fromUI )
        {
            if( !FindNext( includeSelected ) && fromUI )
                txtFind.BackColor = Color.LightCoral;
        }

        public bool FindNext( string text, bool includeSelected )
        {
            TreeNode startNode = tvJson.SelectedNode;
            if( startNode == null && HasNodes() )
                startNode = GetRootNode();
            if( startNode != null )
            {
                startNode = FindNext( startNode, text, includeSelected );
                if( startNode != null )
                {
                    tvJson.SelectedNode = startNode;
                    return true;
                }
            }
            return false;
        }

        public TreeNode FindNext( TreeNode startNode, string text, bool includeSelected )
        {
            if( text == string.Empty )
                return startNode;

            if( includeSelected && IsMatchingNode( startNode, text ) )
                return startNode;

            TreeNode originalStartNode = startNode;
            startNode = GetNextNode( startNode );
            text = text.ToLower();
            while( startNode != originalStartNode )
            {
                if( IsMatchingNode( startNode, text ) )
                    return startNode;
                startNode = GetNextNode( startNode );
            }

            return null;
        }

        private TreeNode GetNextNode( TreeNode startNode )
        {
            TreeNode next = startNode.FirstNode ?? startNode.NextNode;
            if( next == null )
            {
                while( startNode != null && next == null )
                {
                    startNode = startNode.Parent;
                    if( startNode != null )
                        next = startNode.NextNode;
                }
                if( next == null )
                {
                    next = GetRootNode();
                    FlashControl( txtFind, Color.Cyan );
                }
            }
            return next;
        }

        private bool IsMatchingNode( TreeNode startNode, string text )
        {
            return ( startNode.Text.ToLower().Contains( text ) );
        }

        private JsonViewerTreeNode GetRootNode()
        {
            if( tvJson.Nodes.Count > 0 )
                return (JsonViewerTreeNode)tvJson.Nodes[0];
            return null;
        }

        private bool HasNodes()
        {
            return ( tvJson.Nodes.Count > 0 );
        }

        private void txtFind_KeyDown( object sender, KeyEventArgs e )
        {
            if( e.KeyCode == Keys.Enter )
            {
                FindNext( false, true );
            }
            if( e.KeyCode == Keys.Escape )
            {
                HideFind();
            }
        }

        private void FlashControl( Control control, Color color )
        {
            Color prevColor = control.BackColor;
            try
            {
                control.BackColor = color;
                control.Refresh();
                Thread.Sleep( 25 );
            }
            finally
            {
                control.BackColor = prevColor;
                control.Refresh();
            }
        }

        public void ShowTab( Tabs tab )
        {
            tabControl.SelectedIndex = (int)tab;
        }

        private void btnFormat_Click( object sender, EventArgs e )
        {
            try
            {
                string json = txtJson.Text;
                JsonSerializer s = new JsonSerializer();
                JsonReader reader = new JsonTextReader( new StringReader( json ) );
                object jsonObject = s.Deserialize( reader );
                if( jsonObject != null )
                {
                    StringWriter sWriter = new StringWriter();
                    JsonWriter writer = new JsonTextWriter( sWriter );
                    writer.Formatting = Formatting.Indented;
                    //writer..Indentation = 4;
                    //writer.IndentChar = ' ';
                    s.Serialize( writer, jsonObject );
                    txtJson.Text = sWriter.ToString();
                }
            }
            catch( Exception ex )
            {
                ShowException( ex );
            }
        }

        private void ShowException( Exception e )
        {
            MessageBox.Show( this, e.Message, "Json Viewer", MessageBoxButtons.OK, MessageBoxIcon.Error );
        }

        private void btnStripToSqr_Click( object sender, EventArgs e )
        {
            StripTextTo( '[', ']' );
        }

        private void btnStripToCurly_Click( object sender, EventArgs e )
        {
            StripTextTo( '{', '}' );
        }

        private void StripTextTo( char sChr, char eChr )
        {
            string text = txtJson.Text;
            int start = text.IndexOf( sChr );
            int end = text.LastIndexOf( eChr );
            int newLen = end - start + 1;
            if( newLen > 1 )
            {
                txtJson.Text = text.Substring( start, newLen );
            }
        }

        private void tvJson_AfterSelect( object sender, TreeViewEventArgs e )
        {
            if( _pluginsManager.DefaultVisualizer == null )
                return;

            cbVisualizers.BeginUpdate();
            _updating = true;
            try
            {
                JsonViewerTreeNode node = (JsonViewerTreeNode)e.Node;
                IJsonVisualizer lastActive = node.LastVisualizer;
                if( lastActive == null )
                    lastActive = (IJsonVisualizer)cbVisualizers.SelectedItem;
                if( lastActive == null )
                    lastActive = _pluginsManager.DefaultVisualizer;

                cbVisualizers.Items.Clear();
                cbVisualizers.Items.AddRange( node.Visualizers.ToArray() );
                int index = cbVisualizers.Items.IndexOf( lastActive );
                if( index != -1 )
                {
                    cbVisualizers.SelectedIndex = index;
                }
                else
                {
                    cbVisualizers.SelectedIndex = cbVisualizers.Items.IndexOf( _pluginsManager.DefaultVisualizer );
                }
            }
            finally
            {
                cbVisualizers.EndUpdate();
                _updating = false;
            }
            ActivateVisualizer();
        }

        private void ActivateVisualizer()
        {
            IJsonVisualizer visualizer = (IJsonVisualizer)cbVisualizers.SelectedItem;
            if( visualizer != null )
            {
                JsonObject jsonObject = GetSelectedTreeNode().JsonObject;
                Control visualizerCtrl = visualizer.GetControl( jsonObject );
                if( _lastVisualizerControl != visualizerCtrl )
                {
                    pnlVisualizer.Controls.Remove( _lastVisualizerControl );
                    pnlVisualizer.Controls.Add( visualizerCtrl );
                    visualizerCtrl.Dock = DockStyle.Fill;
                    _lastVisualizerControl = visualizerCtrl;
                }
                visualizer.Visualize( jsonObject );
            }
        }


        private void cbVisualizers_SelectedIndexChanged( object sender, EventArgs e )
        {
            if( !_updating && GetSelectedTreeNode() != null )
            {
                ActivateVisualizer();
                GetSelectedTreeNode().LastVisualizer = (IJsonVisualizer)cbVisualizers.SelectedItem;
            }
        }

        private JsonViewerTreeNode GetSelectedTreeNode()
        {
            return (JsonViewerTreeNode)tvJson.SelectedNode;
        }

        private void tvJson_BeforeExpand( object sender, TreeViewCancelEventArgs e )
        {
            foreach( JsonViewerTreeNode node in e.Node.Nodes )
            {
                InitVisualizers( node );
            }
        }

        private void InitVisualizers( JsonViewerTreeNode node )
        {
            if( !node.Initialized )
            {
                node.Initialized = true;
                JsonObject jsonObject = node.JsonObject;
                foreach( ICustomTextProvider textVis in _pluginsManager.TextVisualizers )
                {
                    if( textVis.CanVisualize( jsonObject ) )
                        node.TextVisualizers.Add( textVis );
                }

                node.RefreshText();

                foreach( IJsonVisualizer visualizer in _pluginsManager.Visualizers )
                {
                    if( visualizer.CanVisualize( jsonObject ) )
                        node.Visualizers.Add( visualizer );
                }
            }
        }

        private void btnCloseFind_Click( object sender, EventArgs e )
        {
            HideFind();
        }

        private void JsonViewer_KeyDown( object sender, KeyEventArgs e )
        {
            if( e.KeyCode == Keys.F && e.Control )
            {
                ShowFind();
            }
        }

        private void HideFind()
        {
            pnlFind.Visible = false;
            tvJson.Focus();
        }

        private void ShowFind()
        {
            pnlFind.Visible = true;
            txtFind.Focus();
        }

        private void findToolStripMenuItem_Click( object sender, EventArgs e )
        {
            ShowFind();
        }

        private void expandallToolStripMenuItem_Click( object sender, EventArgs e )
        {
            tvJson.BeginUpdate();
            try
            {
                if( tvJson.SelectedNode != null )
                {
                    TreeNode topNode = tvJson.TopNode;
                    tvJson.SelectedNode.ExpandAll();
                    tvJson.TopNode = topNode;
                }
            }
            finally
            {
                tvJson.EndUpdate();
            }
        }

        private void tvJson_MouseDown( object sender, MouseEventArgs e )
        {
            if( e.Button == MouseButtons.Right )
            {
                TreeNode node = tvJson.GetNodeAt( e.Location );
                if( node != null )
                {
                    tvJson.SelectedNode = node;
                }
            }
        }

        private void rightToolStripMenuItem_Click( object sender, EventArgs e )
        {
            if( sender == mnuShowOnBottom )
            {
                spcViewer.Orientation = Orientation.Horizontal;
                mnuShowOnRight.Checked = false;
            }
            else
            {
                spcViewer.Orientation = Orientation.Vertical;
                mnuShowOnBottom.Checked = false;
            }
        }

        private void cbVisualizers_Format( object sender, ListControlConvertEventArgs e )
        {
            e.Value = ( (IJsonViewerPlugin)e.ListItem ).DisplayName;
        }

        private void mnuTree_Opening( object sender, CancelEventArgs e )
        {
            mnuFind.Enabled = ( GetRootNode() != null );
            mnuExpandAll.Enabled = ( GetSelectedTreeNode() != null );

            mnuCopy.Enabled = mnuExpandAll.Enabled;
            mnuCopyValue.Enabled = mnuExpandAll.Enabled;
        }

        private void btnCopy_Click( object sender, EventArgs e )
        {
            string text;
            if( txtJson.SelectionLength > 0 )
                text = txtJson.SelectedText;
            else
                text = txtJson.Text;
            Clipboard.SetText( text );
        }

        private void btnPaste_Click( object sender, EventArgs e )
        {
            txtJson.Text = Clipboard.GetText();
        }

        private void mnuCopy_Click( object sender, EventArgs e )
        {
            JsonViewerTreeNode node = GetSelectedTreeNode();
            if( node != null )
            {
                Clipboard.SetText( node.Text );
            }
        }

        private void mnuCopyName_Click( object sender, EventArgs e )
        {
            JsonViewerTreeNode node = GetSelectedTreeNode();

            if( node != null && node.JsonObject.Id != null )
            {
                JsonObject obj = node.Tag as JsonObject;
                Clipboard.SetText( obj.Id );
            }
            else
            {
                Clipboard.SetText( "" );
            }

        }

        private void mnuCopyValue_Click( object sender, EventArgs e )
        {
            JsonViewerTreeNode node = GetSelectedTreeNode();
            if( node != null && node.Tag != null )
            {
                JsonObject obj = node.Tag as JsonObject;
                Clipboard.SetText( obj.Value.ToString() );
            }
            else
            {
                Clipboard.SetText( "null" );
            }
        }

        private void lblError_LinkClicked( object sender, LinkLabelLinkClickedEventArgs e )
        {
            if( lblError.Enabled && lblError.Tag != null )
            {
                ErrorDetails err = (ErrorDetails)lblError.Tag;
                MarkError( err );
            }
        }

        private void removeNewLineMenuItem_Click( object sender, EventArgs e )
        {
            StripFromText( '\n', '\r' );
        }

        private void removeSpecialCharsToolStripMenuItem_Click( object sender, EventArgs e )
        {
            string text = txtJson.Text;
            text = text.Replace( @"\""", @"""" );
            txtJson.Text = text;
        }

        private void StripFromText( params char[] chars )
        {
            string text = txtJson.Text;
            foreach( char ch in chars )
            {
                text = text.Replace( ch.ToString(), "" );
            }
            txtJson.Text = text;
        }

        private void BackgroundWorker1_DoWork(object sender, DoWorkEventArgs e)
        {
            try
            {
                _tree = JsonObjectTree.Parse(_json);
                Mode = 0;
            }
            catch (JsonParseError err)
            {
                GetParseErrorDetails(err);
                Mode = 3;
            }
        }

        public void refreshFromString(string text)
        {
            txtJson.Text = text;
            if (_json != text)
            {
                _json = text.Trim();
                Redraw();
            }
            try
            {
                tvJson.BeginUpdate();
                try
                {
                    if (!string.IsNullOrEmpty(_json))
                    {
                        try
                        {
                            _tree = JsonObjectTree.Parse(_json);
                            Mode = 0;
                        }
                        catch (JsonParseError err)
                        {
                            GetParseErrorDetails(err);
                            Mode = 3;
                        }
                        VisualizeJsonTree(_tree);
                        _oldTree = _tree;
                    }
                    else
                    {
                        Reset();
                    }
                }
                finally
                {
                    tvJson.EndUpdate();
                }
            }
            catch (Exception e)
            {
                ShowException(e);
            }
        }
    }

    public struct ErrorDetails
    {
        internal string _err;
        internal int _pos;

        public string Error
        {
            get
            {
                return _err;
            }
        }

        public int Position
        {
            get
            {
                return _pos;
            }
        }

        public void Clear()
        {
            _err = null;
            _pos = 0;
        }
    }

    public class JsonViewerTreeNode : TreeNode
    {
        public JsonViewerTreeNode( JsonObject jsonObject )
        {
            JsonObject = jsonObject;
        }

        public List<ICustomTextProvider> TextVisualizers { get; } = new List<ICustomTextProvider>();

        public List<IJsonVisualizer> Visualizers { get; } = new List<IJsonVisualizer>();

        public JsonObject JsonObject { get; }

        internal bool Initialized { get; set; }

        internal void RefreshText()
        {
            StringBuilder sb = new StringBuilder( JsonObject.Text );
            foreach( ICustomTextProvider textVisualizer in TextVisualizers )
            {
                try
                {
                    string customText = textVisualizer.GetText( JsonObject );
                    sb.Append( " (" + customText + ")" );
                }
                catch
                {
                    //silently ignore
                }
            }
            string text = sb.ToString();
            if( text != Text )
                Text = text;
        }

        public IJsonVisualizer LastVisualizer { get; set; }
    }

    public enum Tabs { Viewer, Text };
}