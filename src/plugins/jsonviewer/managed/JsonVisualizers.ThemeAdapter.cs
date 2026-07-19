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
            ThemeHelper.RecreateHandleForInitialDarkTheme(pgJsonObject);
            ThemeHelper.ApplyNativeDarkMode(pgJsonObject);

            NativeThemeRefreshScheduler.ScheduleNativeDarkModeRefresh(pgJsonObject);
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
            ThemeHelper.RecreateHandleForInitialDarkTheme(lvGrid);
            ThemeHelper.ApplyNativeDarkMode(lvGrid);
            NativeThemeRefreshScheduler.ScheduleNativeDarkModeRefresh(lvGrid);
        }
    }

    internal static class NativeThemeRefreshScheduler
    {
        public static void ScheduleNativeDarkModeRefresh(Control control)
        {
            if (!control.IsHandleCreated || control.IsDisposed)
            {
                return;
            }

            int remainingPasses = 4;
            var timer = new Timer { Interval = 50 };
            timer.Tick += (_, _) =>
            {
                if (control.IsDisposed || !control.IsHandleCreated || --remainingPasses <= 0)
                {
                    timer.Stop();
                    timer.Dispose();
                    return;
                }

                ThemeHelper.ApplyNativeDarkMode(control);
                control.Invalidate(true);
            };
            control.Disposed += (_, _) =>
            {
                timer.Stop();
                timer.Dispose();
            };
            timer.Start();
        }
    }
}
