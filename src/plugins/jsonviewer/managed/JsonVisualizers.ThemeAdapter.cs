// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Runtime.CompilerServices;
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

        protected override void OnSizeChanged(EventArgs e)
        {
            base.OnSizeChanged(e);
            if (pgJsonObject is not null)
            {
                NativeThemeRefreshScheduler.ScheduleNativeDarkModeRefresh(pgJsonObject);
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

        protected override void OnSizeChanged(EventArgs e)
        {
            base.OnSizeChanged(e);
            if (lvGrid is not null)
            {
                NativeThemeRefreshScheduler.ScheduleNativeDarkModeRefresh(lvGrid);
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
        private static readonly ConditionalWeakTable<Control, RefreshState> s_refreshes = new ConditionalWeakTable<Control, RefreshState>();

        public static void ScheduleNativeDarkModeRefresh(Control control)
        {
            if (!control.IsHandleCreated || control.IsDisposed)
            {
                return;
            }

            if (!s_refreshes.TryGetValue(control, out var state))
            {
                state = new RefreshState();
                s_refreshes.Add(control, state);
                control.Disposed += (_, _) =>
                {
                    StopRefresh(state);
                    s_refreshes.Remove(control);
                };
            }

            state.Timer?.Stop();
            state.Timer?.Dispose();

            state.Timer = new Timer { Interval = 25 };
            state.Timer.Tick += (_, _) =>
            {
                StopRefresh(state);
                if (!control.IsDisposed && control.IsHandleCreated)
                {
                    ThemeHelper.ApplyNativeDarkMode(control);
                }
            };
            state.Timer.Start();
        }

        private static void StopRefresh(RefreshState state)
        {
            state.Timer?.Stop();
            state.Timer?.Dispose();
            state.Timer = null;
        }

        private sealed class RefreshState
        {
            public Timer Timer { get; set; }
        }
    }
}
