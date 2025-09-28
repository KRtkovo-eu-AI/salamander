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
	/// <c>TreeStrategyFTPProvider</c> implements <c>TreeStrategyDataProviderBase</c> and provides a explorer like ftp directory browsing
	/// using the FTP Component for .NET (April 2002 by Jérôme Lacaille www.codeproject.com)
	/// </summary> 
	public class TreeStrategyFTPProvider : TreeStrategyFolderBrowserProviderBase
	{
		#region fields
		/// <summary>server to connect to</summary>
		private string _server;

		/// <summary>port number to connect</summary>
		private int _port;

		/// <summary>username and password to login</summary>
		private System.Net.NetworkCredential _credential = null;

		/// <summary>FTP Component instance</summary>
		private FTPLib.FtpClient _ftp;
		/// <summary>the path separator the ftp server needs to split up paths</summary>
		private string _directorySeparatorChar;
		#endregion

		#region constructors

		/// <summary>
		/// Initializes a new instance of the <c>TreeStrategyFTPProvider</c> class.
		/// </summary>
		public TreeStrategyFTPProvider()
		{			
			this._credential = new System.Net.NetworkCredential("anonymous", "anonymous@");
			this._port = 21;
		}
		/// <summary>
		/// Initializes a new instance of the <c>TreeStrategyFTPProvider</c> class.
		/// </summary>
		/// <param name="credential">Username and Password used to login</param>
		/// <param name="server">Server adress</param>
		public TreeStrategyFTPProvider(string server, System.Net.NetworkCredential credential) : this(server, 21, credential)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <c>TreeStrategyFTPProvider</c> class used for Anonymous access.
		/// </summary>
		/// <param name="server">Server adress</param>
		public TreeStrategyFTPProvider(string server) : this(server, 21, new System.Net.NetworkCredential("anonymous", "anonymous@"))
		{
		}
		/// <summary>
		/// Initializes a new instance of the <c>TreeStrategyFTPProvider</c> class.
		/// </summary>
		/// <param name="credential">Username and Password used to login</param>
		/// <param name="port">Server port number</param>
		/// <param name="server">Server adress</param>
		public TreeStrategyFTPProvider(string server, int port, System.Net.NetworkCredential credential)
		{
			_server = server;
			_port = port;
			_credential = credential;
		}

		#endregion

		#region public interface
		[System.ComponentModel.Browsable(false)]
		public string DirectorySeparatorChar
		{
			get
			{
				return _directorySeparatorChar;
			}
		}
		[System.ComponentModel.Category("Data"), System.ComponentModel.Description("")]
		public string Server
		{
			get { return _server; }
			set
			{
				bool changed = !object.Equals(_server, value);
				_server = value; 
				if(changed) 
				{
					ResetFtp();					
				}
			}
		}

		[System.ComponentModel.Category("Data"), System.ComponentModel.Description("")]
		public int Port
		{
			get { return _port; }
			set 
			{
				bool changed = !object.Equals(_port, value);
				_port = value; 
				if(changed) 
				{
					ResetFtp();					
				}
			}
		}

		[System.ComponentModel.Category("Data"), System.ComponentModel.Description("")]
		[System.ComponentModel.TypeConverter(typeof (System.ComponentModel.ExpandableObjectConverter))]
		public System.Net.NetworkCredential Credential
		{
			get { return _credential; }
			set 
			{
				bool changed = !object.Equals(_credential, value);
				_credential = value; 
				if(changed) 
				{
					ResetFtp();					
				}
			}
		}

		public override string ToString()
		{
			return "FTP Provider";
		}
		internal struct AsyncDataStruct
		{
			public System.Collections.Specialized.StringCollection FileList;
			public System.IO.DirectoryInfo Directory;
		}
		public void DownloadFiles(System.Collections.Specialized.StringCollection fileList, System.IO.DirectoryInfo directoryInfo, System.ComponentModel.ProgressChangedEventHandler progressChangedEventHandler, System.ComponentModel.RunWorkerCompletedEventHandler runWorkerCompletedEventHandler)
		{			
			System.ComponentModel.BackgroundWorker worker = new System.ComponentModel.BackgroundWorker();
			if(progressChangedEventHandler!=null)
			{
				worker.ProgressChanged += progressChangedEventHandler;
				worker.WorkerReportsProgress = true;
				worker.WorkerSupportsCancellation = true;
			}
			if(runWorkerCompletedEventHandler!=null)
			{
				worker.RunWorkerCompleted += runWorkerCompletedEventHandler;
			}
			worker.DoWork += new System.ComponentModel.DoWorkEventHandler(DownloadFilesAsync);			
			AsyncDataStruct ads = new AsyncDataStruct();
			ads.Directory = directoryInfo;
			ads.FileList = fileList;
			worker.RunWorkerAsync(ads);
		}

		#endregion

		#region internal interface
		private void DownloadFilesAsync(object sender, System.ComponentModel.DoWorkEventArgs e)
		{
			AsyncDataStruct ads = (AsyncDataStruct) e.Argument;
			System.ComponentModel.BackgroundWorker worker = sender as System.ComponentModel.BackgroundWorker;
			//
			FTPLib.FtpClient ftp = FTP;
			int perc, old_perc = 0;
			//
			foreach(string remotePath in ads.FileList)
			{
				if(worker.CancellationPending)
				{
					break;
				}
				// open the file on the server and localy
				string remoteFilePath = System.IO.Path.GetDirectoryName(remotePath);
				string remoteFileName = System.IO.Path.GetFileName(remotePath);
				//
				ftp.ChangeDir(remoteFilePath);
				ftp.OpenDownload(remotePath, System.IO.Path.Combine(ads.Directory.FullName, remoteFileName), false);
    
				// download the file (read it from server, write it to disk)
				while(true)
				{
					perc = ftp.DoDownload();
        
					// No need to report progress everytime we get some bytes
					// because it causes a flickery effect on the screen in most cases.
					if(perc >  old_perc)
					{
						worker.ReportProgress(perc, remoteFileName);						
					}
        
					// is the download done?
					if(perc == 100)
						break;

					old_perc = perc;
				}
			}
		}
		/// <summary>
		/// Releases a existing ftp component (disconnect) instance
		/// </summary>
		private void ResetFtp()
		{
			if(_ftp!=null)
			{				
				_ftp.Disconnect();
			}
			_ftp = null;
		}
		/// <summary>
		/// Create a new instance or retrieve the existing one configured with the given connection string
		/// </summary>
		private FTPLib.FtpClient FTP
		{
			get
			{
				if (_ftp != null) return _ftp;
				//
				_ftp = new FTPLib.FtpClient();
				_ftp.port = Port;
				_ftp.server = _server;				
				_ftp.port = _port;	
				_ftp.user = _credential.UserName;
				_ftp.pass = _credential.Password;
				_ftp.Connect();	
				DetermineUseSlash();			
				//
				return _ftp;
			}
		}

		#endregion

		#region private interface		
		/// <summary>
		/// Tries to find out what path separator char the ftp servers needs to work correctly by trail and error.		
		/// </summary>
		private void DetermineUseSlash()
		{
			try
			{
				_directorySeparatorChar = "\\";
				FTP.ChangeDir(_directorySeparatorChar);
			} 
			catch
			{
				_directorySeparatorChar = "/";
			}
		}
		/// <summary>
		/// Internal helper methods to create nodes and assign them as root or child nodes.
		/// </summary>
		/// <param name="helper"></param>
		/// <param name="parentNode"></param>
		/// <param name="parentCollection"></param>
		private void RequestHelper(TreeViewFolderBrowserNodeFactory helper, TreeNodePath parentNode, System.Windows.Forms.TreeNodeCollection parentCollection)
		{
			FTPLib.FtpClient ftp = FTP;

			if (parentNode != null)
			{
				ftp.ChangeDir(parentNode.Path);				
			}
			else
			{				
				ftp.ChangeDir(_directorySeparatorChar);
			}
			//
			foreach(FTPLib.ServerFileData serverData in ftp.List())
			{
				// skip files if there should not be filled in
				if(!serverData.isDirectory && !ShowFiles) continue;
				//
				CreateTreeNode(parentNode,
					serverData.fileName, parentNode!=null ? parentNode.Path + "/"+ serverData.fileName : _directorySeparatorChar + serverData.fileName,
										!serverData.isDirectory,
										serverData.isDirectory,
					                    false);
			}
			//
			return;
		}
		#endregion

		#region events
		#endregion

		#region ITreeViewFolderBrowserDataProvider Members

		public override void Attach(TreeViewFolderBrowserNodeFactory helper)
		{
			base.Attach(helper);
			//
			helper.TreeView.ShowRootLines = true;
			helper.TreeView.ShowPlusMinus = true;
		}

		public override void Detach()
		{
			if (_ftp != null)
			{				
				_ftp.Disconnect();
			}
			base.Detach();
		}
		public override void RequestRootNode()
		{
			RequestHelper(Helper, null, Helper.TreeView.Nodes);
		}

		public override void RequestChildNodes(TreeNodePath parent, System.Windows.Forms.TreeViewCancelEventArgs e)
		{
			RequestHelper(Helper, parent, parent.Nodes);
		}

		/// <summary>
		/// Extract the icon for the file type (Extension)
		/// </summary>
		/// <remarks>SetIcon is overriden because icons are retrieved by it's path property. Because this class is dealing
		/// with ftp data and not the local file system, we have to treat SetIcon process slightley different than for local system data.</remarks>
		protected override void SetIcon(TreeViewFolderBrowser treeView, TreeNodePath node)
		{			
			bool isFilePath = false;
			string extension = "test" + System.IO.Path.GetExtension(node.Path);
			if (extension == "test")
			{
				extension = System.Environment.SystemDirectory;
				isFilePath = true;
			}
			node.ImageIndex = this.SystemImageList.IconIndex(extension, isFilePath, Raccoom.Win32.ShellIconStateConstants.ShellIconStateNormal);
			node.SelectedImageIndex = this.SystemImageList.IconIndex(extension, isFilePath, Raccoom.Win32.ShellIconStateConstants.ShellIconStateOpen);
		}
		#endregion	
	}
}