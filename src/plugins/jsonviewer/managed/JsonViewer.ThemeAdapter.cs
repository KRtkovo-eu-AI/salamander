// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Drawing;
using System.Windows.Forms;

namespace EPocalipse.Json.Viewer
{
    public partial class JsonViewer
    {
        protected override void OnHandleCreated(EventArgs e)
        {
            base.OnHandleCreated(e);
            ApplyCurrentTheme();
        }

        protected override void OnParentChanged(EventArgs e)
        {
            base.OnParentChanged(e);
            ApplyCurrentTheme();
        }

        private void ApplyCurrentTheme()
        {
            ThemeHelper.ApplyTheme(this);

            if (TopLevelControl is Form form)
            {
                ThemeHelper.ApplyTheme(form);
            }

            if (!ThemeHelper.TryGetPalette(out var palette))
            {
                return;
            }

            ApplyPalette(palette);
        }

        private void ApplyPalette(ThemeHelper.ThemePalette palette)
        {
            BackColor = palette.Background;
            ForeColor = palette.Foreground;

            StyleTabControl(palette);
            StyleSplitContainer(palette);

            pnlVisualizer.BackColor = palette.Background;
            pnlVisualizer.ForeColor = palette.Foreground;

            pnlFind.BackColor = palette.ControlBackground;
            pnlFind.ForeColor = palette.Foreground;

            txtFind.BackColor = palette.InputBackground;
            txtFind.ForeColor = palette.InputForeground;

            txtJson.BackColor = palette.InputBackground;
            txtJson.ForeColor = palette.InputForeground;

            lblError.LinkColor = palette.Accent;
            lblError.ActiveLinkColor = palette.HighlightForeground;
            lblError.VisitedLinkColor = palette.Accent;
        }

        private void StyleTabControl(ThemeHelper.ThemePalette palette)
        {
            tabControl.BackColor = palette.ControlBackground;
            tabControl.ForeColor = palette.Foreground;

            foreach (TabPage tab in tabControl.TabPages)
            {
                tab.UseVisualStyleBackColor = false;
                tab.BackColor = palette.Background;
                tab.ForeColor = palette.Foreground;
            }

            tabControl.Invalidate();
        }

        private void StyleSplitContainer(ThemeHelper.ThemePalette palette)
        {
            spcViewer.BackColor = palette.ControlBackground;
            spcViewer.ForeColor = palette.Foreground;

            spcViewer.Panel1.BackColor = palette.Background;
            spcViewer.Panel1.ForeColor = palette.Foreground;

            spcViewer.Panel2.BackColor = palette.Background;
            spcViewer.Panel2.ForeColor = palette.Foreground;
        }
    }
}
