using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using Microsoft.Win32;
using System.Diagnostics;

namespace InstallToSalamander
{
    public partial class Form1 : Form
    {
        private const string x64dir = @"C:\Program Files\Altap Salamander\plugins";
        private const string x86dir = @"C:\Program Files (x86)\Altap Salamander\plugins";

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            if(Directory.Exists(x64dir))
            {
                textBox1.Text = x64dir;
                folderBrowserDialog1.SelectedPath = x64dir;
            }
            else if(Directory.Exists(x86dir))
            {
                textBox1.Text = x86dir;
                folderBrowserDialog1.SelectedPath = x86dir;
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if(folderBrowserDialog1.ShowDialog() == DialogResult.OK)
            {
                textBox1.Text = folderBrowserDialog1.SelectedPath;
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            panel1.Visible = false;
            panel2.Visible = true;
            string installationPath = $@"{textBox1.Text}\jsonviewer";
            string releasedFilesPath = $@"{Environment.CurrentDirectory}\JsonViewPortable\";
            Process[] altapProc = Process.GetProcessesByName("salamand");
            string[] packageFiles = Directory.GetFiles(releasedFilesPath);

            progressBar1.Maximum = 6 + altapProc.Length + packageFiles.Length;
            progressBar1.Minimum = 0;
            progressBar1.Value = 0;

            progressBar1.Value = progressBar1.Value + 1;

            // Kill running instance of Altap Salamander
            foreach(Process altap in altapProc)
            {
                altap.Kill();
                progressBar1.Value = progressBar1.Value + 1;
            }
            progressBar1.Value = progressBar1.Value + 1;

            // Create new installation folder
            Directory.CreateDirectory(installationPath);
            progressBar1.Value = progressBar1.Value + 1;

            // Copy all files
            foreach (string file in packageFiles)
            {
                File.Copy(file, file.Substring(file.LastIndexOf("\\"), (file.Length - file.LastIndexOf("\\"))), true);
                progressBar1.Value = progressBar1.Value + 1;
            }

            // Write registry entries
            RegistryKey altapViewers = Registry.CurrentUser.OpenSubKey(@"Software\Altap\Altap Salamander 4.0\Viewers", true);
            int lastViewerId = altapViewers.SubKeyCount;
            altapViewers.DeleteSubKeyTree(lastViewerId.ToString());
            progressBar1.Value = progressBar1.Value + 1;

            RegistryKey jsonViewerAdded = altapViewers.CreateSubKey(lastViewerId.ToString());
            jsonViewerAdded.SetValue("Arguments", "$(FullName)");
            jsonViewerAdded.SetValue("Command", $@"{installationPath}\JsonView.exe");
            jsonViewerAdded.SetValue("Initial Directory", "$(FullPath)");
            jsonViewerAdded.SetValue("Masks", "*.json");
            jsonViewerAdded.SetValue("Type", 0);
            progressBar1.Value = progressBar1.Value + 1;

            int allFilesId = lastViewerId;
            allFilesId++;
            RegistryKey allFiles = altapViewers.CreateSubKey(allFilesId.ToString());
            allFiles.SetValue("Masks", "*.*");
            allFiles.SetValue("Type", 1);
            progressBar1.Value = progressBar1.Value + 1;

            button1.Visible = true;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}
