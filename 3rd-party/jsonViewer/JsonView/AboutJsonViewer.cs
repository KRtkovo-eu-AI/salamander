using System;
using System.Windows.Forms;
using System.Reflection;
using System.Diagnostics;

namespace EPocalipse.Json.JsonView
{
    partial class AboutJsonViewer : Form
    {
        public AboutJsonViewer()
        {
            InitializeComponent();
        }

        private void LinkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Process.Start("https://www.flaticon.com/authors/vaadin");
        }

        private void linkLabel2_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Process.Start("https://github.com/realworld666/JsonViewer");
        }

        private void linkLabel3_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Process.Start("https://github.com/alebcay/JSONViewer");
        }

        private void linkLabel4_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
        Process.Start("https://github.com/KRtkovo-eu/JsonViewerForAltapSalamander");
        }
    }
}
