// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
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
        }
    }
}
