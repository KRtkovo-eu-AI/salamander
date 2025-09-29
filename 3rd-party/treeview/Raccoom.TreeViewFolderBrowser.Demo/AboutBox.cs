
// Copyright © 2006 by Christoph Richner. All rights are reserved.
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
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;

namespace Raccoom.Windows.Forms
{

	/// <summary>
	/// A Form that fades into view at creation, and fades out of view at destruction.
	/// </summary>
	public class AboutBox : Form
	{
		#region members

		// *******************************************************************
		// Attributes.
		// *******************************************************************

		/// <summary>
		/// Flag to control whether the form fades in or out of view.
		/// </summary>
		private bool m_fadeInFlag;

		/// <summary>
		/// Timer to drive the fading process.
		/// </summary>
		private System.Windows.Forms.Timer m_fadeInOutTimer;
		protected System.Windows.Forms.RichTextBox richTextBox1;
		
		private System.Windows.Forms.ContextMenu contextMenu1;
		private System.Windows.Forms.ImageList imageList1;
		protected System.Windows.Forms.PictureBox rcmPictureBox1;
		private System.Windows.Forms.GroupBox rcmGroupBoxHeader1;
		private System.Windows.Forms.Button rcmButton1;
		private System.Windows.Forms.Button rcmButton3;
		private System.Windows.Forms.MenuItem mnuEmail;
		private System.Windows.Forms.MenuItem mnuWebsite;

		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components;
		#endregion

		// *************************************************************************
		// Constructors.
		// *************************************************************************

		/// <summary>
		/// Default constructor.
		/// </summary>
		public AboutBox()
		{
			InitializeComponent();		
			//
			if(!DesignMode)
			{				
				this.rcmGroupBoxHeader1.Text = Application.ProductName + " " + Application.ProductVersion.Substring(0,3);
				this.richTextBox1.Text = GetDisplayText;
				
			}
		} // End FadingForm()


		#region internal
		protected virtual string GetDisplayText
		{
			get
			{	
				string descText = "Copyright © "+System.IO.File.GetCreationTime(Application.ExecutablePath).Year+" by Christoph Richner.\nAll rights are reserved.\n\r";
				int length = descText.Length;
				//
				Attribute[] desc = (Attribute[]) System.Reflection.Assembly.GetEntryAssembly().GetCustomAttributes(typeof(System.Reflection.AssemblyDescriptionAttribute),true);
				if (desc.GetLength(0) == 1)
				{
					descText += ((System.Reflection.AssemblyDescriptionAttribute)desc[0]).Description;
				} 
				if(descText.Length == length)
				{
					descText += "If you like this  code then feel free to go ahead and use it. The only thing I ask is that you don't remove or alter my copyright notice.\nYour use of this software is entirely at your own risk. I make no claims or warrantees about the reliability or fitness of this code for any particular purpose.\nIf you make changes or additions to this code please mark your code as being yours.\nIf you have questions or comments then please contact me at: microweb@bluewin.ch";
				}					
				return descText;
			}
		}
		// *******************************************************************
		// Timer event handlers.
		// *******************************************************************

		/// <summary>
		/// Timer tick event handler. Used to drive the fading activity.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void m_fadeInOutTimer_Tick(object sender, System.EventArgs e)
		{

			// How should we fade?
			if (m_fadeInFlag == false)
			{
			
				Opacity -= (m_fadeInOutTimer.Interval / 1000.0);

				// Should we continue to fade?
				if (this.Opacity > 0)
					m_fadeInOutTimer.Enabled = true;
				else
				{
					
					m_fadeInOutTimer.Enabled = false;
					Close();

				} // End else we should close the form.

			} // End if we should fade in.
			else
			{

				Opacity += (m_fadeInOutTimer.Interval / 1000.0);
				m_fadeInOutTimer.Enabled = (Opacity < 1.0);
				m_fadeInFlag = (Opacity < 1.0);
				
			} // End else we should fade out.
		
		} // End m_fadeInOutTimer_Tick()

		// *******************************************************************
		// Private methods.
		// *******************************************************************

		/// <summary>
		/// The mysterious DoNothing method! What does it mean? Why is it 
		/// here? Will these questions ever be answered!!!
		/// </summary>
		private void _DoNothing() { }

		private void ShellExecute(string param)
		{
			System.Diagnostics.Process.Start(param);
		}

		#endregion

		#region events		
		
		// *******************************************************************

		/// <summary>
		/// Used to initiate the fade in process.
		/// </summary>
		/// <param name="e"></param>
		protected override void OnLoad(EventArgs e)
		{

			base.OnLoad(e);

			// Should we start fading?
			if (!DesignMode)
			{
				
				m_fadeInFlag = true;
				Opacity = 0;
				
				m_fadeInOutTimer.Enabled = true;

			} // End if we should start the fading process.
						
		} // End OnLoad()

		// *******************************************************************

		/// <summary>
		/// Used to control the fade out process.
		/// </summary>
		/// <param name="e"></param>
		protected override void OnClosing(CancelEventArgs e)
		{
			
			base.OnClosing(e);

			// If the user canceled then don't fade anything.
			if (e.Cancel == true)
				return;

			// Should we fade instead of closing?
			if (Opacity > 0)
			{
				m_fadeInFlag = false;
				m_fadeInOutTimer.Enabled = true;
				e.Cancel = true;
			} // End if we should fade instead of closing.

		} // End OnClosing()
		private void mnuEmail_Click(object sender, System.EventArgs e)
		{
			ShellExecute("mailto:support@raccoom.net?subject="+Application.ProductName);				
		}

		private void mnuWebsite_Click(object sender, System.EventArgs e)
		{
			ShellExecute("http://www.raccoom.net");
		}

		private void rcmButton1_Click(object sender, System.EventArgs e)
		{
			this.contextMenu1.Show(rcmButton1, new Point(0, rcmButton1.Height));
		}


		#endregion
		
		#region Windows Form Designer generated code
		// *************************************************************************
		// Overrides.
		// *************************************************************************

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose(bool disposing)
		{
			
			if (disposing)
			{

				if (components != null) 
					components.Dispose();

			} // End if

			base.Dispose(disposing);

		} // End Dispose()

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container();
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(AboutBox));
			this.m_fadeInOutTimer = new System.Windows.Forms.Timer(this.components);
			this.richTextBox1 = new System.Windows.Forms.RichTextBox();
			this.contextMenu1 = new System.Windows.Forms.ContextMenu();
			this.mnuEmail = new System.Windows.Forms.MenuItem();
			this.mnuWebsite = new System.Windows.Forms.MenuItem();
			this.imageList1 = new System.Windows.Forms.ImageList(this.components);
			this.rcmPictureBox1 = new System.Windows.Forms.PictureBox();
			this.rcmGroupBoxHeader1 = new System.Windows.Forms.GroupBox();
			this.rcmButton3 = new System.Windows.Forms.Button();
			this.rcmButton1 = new System.Windows.Forms.Button();
			this.rcmGroupBoxHeader1.SuspendLayout();
			this.SuspendLayout();
			// 
			// m_fadeInOutTimer
			// 
			this.m_fadeInOutTimer.Tick += new System.EventHandler(this.m_fadeInOutTimer_Tick);
			// 
			// richTextBox1
			// 
			this.richTextBox1.BackColor = System.Drawing.SystemColors.Control;
			this.richTextBox1.BorderStyle = System.Windows.Forms.BorderStyle.None;
			this.richTextBox1.Location = new System.Drawing.Point(96, 16);
			this.richTextBox1.Name = "richTextBox1";
			this.richTextBox1.ReadOnly = true;
			this.richTextBox1.Size = new System.Drawing.Size(208, 288);
			this.richTextBox1.TabIndex = 8;
			this.richTextBox1.Text = "";
			// 
			// contextMenu1
			// 
			this.contextMenu1.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																						 this.mnuEmail,
																						 this.mnuWebsite});
			// 
			// mnuEmail
			// 
			this.mnuEmail.Index = 0;
			this.mnuEmail.Text = "&Email";
			this.mnuEmail.Click += new System.EventHandler(this.mnuEmail_Click);
			// 
			// mnuWebsite
			// 
			this.mnuWebsite.Index = 1;
			this.mnuWebsite.Text = "&Website";
			this.mnuWebsite.Click += new System.EventHandler(this.mnuWebsite_Click);
			// 
			// imageList1
			// 
			this.imageList1.ColorDepth = System.Windows.Forms.ColorDepth.Depth24Bit;
			this.imageList1.ImageSize = new System.Drawing.Size(24, 24);
			this.imageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList1.ImageStream")));
			this.imageList1.TransparentColor = System.Drawing.Color.Transparent;
			// 
			// rcmPictureBox1
			// 
			this.rcmPictureBox1.BackColor = System.Drawing.Color.White;
			this.rcmPictureBox1.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
			this.rcmPictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("rcmPictureBox1.Image")));
			this.rcmPictureBox1.Location = new System.Drawing.Point(8, 8);
			this.rcmPictureBox1.Name = "rcmPictureBox1";
			this.rcmPictureBox1.Size = new System.Drawing.Size(66, 292);
			this.rcmPictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
			this.rcmPictureBox1.TabIndex = 12;
			this.rcmPictureBox1.TabStop = false;
			// 
			// rcmGroupBoxHeader1
			// 
			this.rcmGroupBoxHeader1.Controls.Add(this.rcmButton3);
			this.rcmGroupBoxHeader1.Controls.Add(this.rcmButton1);
			this.rcmGroupBoxHeader1.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.rcmGroupBoxHeader1.Location = new System.Drawing.Point(8, 312);
			this.rcmGroupBoxHeader1.Name = "rcmGroupBoxHeader1";
			this.rcmGroupBoxHeader1.Size = new System.Drawing.Size(296, 48);
			this.rcmGroupBoxHeader1.TabIndex = 14;
			this.rcmGroupBoxHeader1.TabStop = false;
			this.rcmGroupBoxHeader1.Text = "rcmGroupBoxHeader1";
			// 
			// rcmButton3
			// 
			this.rcmButton3.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.rcmButton3.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.rcmButton3.Location = new System.Drawing.Point(208, 16);
			this.rcmButton3.Name = "rcmButton3";
			this.rcmButton3.TabIndex = 2;
			this.rcmButton3.Text = "&Close";
			// 
			// rcmButton1
			// 
			this.rcmButton1.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.rcmButton1.Location = new System.Drawing.Point(120, 16);
			this.rcmButton1.Name = "rcmButton1";
			this.rcmButton1.TabIndex = 0;
			this.rcmButton1.Text = "&Internet";
			this.rcmButton1.Click += new System.EventHandler(this.rcmButton1_Click);
			// 
			// FormAbout
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.rcmButton3;
			this.ClientSize = new System.Drawing.Size(314, 368);
			this.Controls.Add(this.rcmGroupBoxHeader1);
			this.Controls.Add(this.rcmPictureBox1);
			this.Controls.Add(this.richTextBox1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "FormAbout";
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "About";
			this.rcmGroupBoxHeader1.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

	} // End class FadingForm

} // End namespace demo
