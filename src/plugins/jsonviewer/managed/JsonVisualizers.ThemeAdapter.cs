// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Windows.Forms;

namespace EPocalipse.Json.Viewer
{
    public partial class JsonObjectVisualizer
    {
        protected override void OnHandleCreated(EventArgs e)
        {
            base.OnHandleCreated(e);
            pgJsonObject.SelectedObjectsChanged -= PgJsonObjectOnSelectedObjectsChanged;
            pgJsonObject.SelectedObjectsChanged += PgJsonObjectOnSelectedObjectsChanged;
            ApplyPropertyGridTheme();
        }

        protected override void OnVisibleChanged(EventArgs e)
        {
            base.OnVisibleChanged(e);
            if (Visible)
            {
                ApplyPropertyGridTheme();
            }
        }

        private void PgJsonObjectOnSelectedObjectsChanged(object sender, EventArgs e)
        {
            ApplyPropertyGridTheme();
        }

        private void ApplyPropertyGridTheme()
        {
            ThemeHelper.ApplyTheme(this);
            ThemeHelper.ApplyNativeDarkMode(pgJsonObject);

            if (pgJsonObject.IsHandleCreated)
            {
                pgJsonObject.BeginInvoke(new Action(() =>
                {
                    if (pgJsonObject.IsHandleCreated)
                    {
                        ThemeHelper.ApplyNativeDarkMode(pgJsonObject);
                        pgJsonObject.Invalidate(true);
                    }
                }));
            }
        }
    }

    public partial class GridVisualizer
    {
        protected override void OnHandleCreated(EventArgs e)
        {
            base.OnHandleCreated(e);
            ApplyGridTheme();
        }

        protected override void OnVisibleChanged(EventArgs e)
        {
            base.OnVisibleChanged(e);
            if (Visible)
            {
                ApplyGridTheme();
            }
        }

        private void ApplyGridTheme()
        {
            ThemeHelper.ApplyTheme(this);
            ThemeHelper.ApplyNativeDarkMode(lvGrid);
        }
    }
}
