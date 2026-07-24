// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace OpenSalamander;

// Managed plugins share the default AppDomain but are separate assemblies.
// AppDomain data prevents every plugin from trying to change process-wide
// WinForms defaults after another plugin has already created a control.
internal static class ManagedApplication
{
    private const string InitializedKey = "OpenSalamander.WinForms.Initialized";

    public static void Initialize()
    {
        // Initialize can be called from more than one native/plugin thread.
        // Process-wide WinForms defaults are shared, but DPI awareness is a
        // property of the actual thread which is about to create an HWND.
        EnsurePerMonitorThread();

        lock (typeof(Application))
        {
            if (AppDomain.CurrentDomain.GetData(InitializedKey) is bool initialized && initialized)
            {
                return;
            }

            // These switches must be set before the first WinForms HWND is
            // created. The native host is PMv2-aware, but .NET Framework 4.8
            // otherwise keeps its legacy SystemAware control-scaling path
            // when WinForms is loaded from a native executable.
            AppContext.SetSwitch("Switch.System.Windows.Forms.DoNotSupportDpiChanges", false);
            AppContext.SetSwitch("Switch.System.Windows.Forms.EnableWindowsFormsHighDpiAutoResizing", true);
            Application.EnableVisualStyles();
            try
            {
                Application.SetCompatibleTextRenderingDefault(false);
            }
            catch (InvalidOperationException)
            {
                // A third-party managed plugin may have created the first
                // WinForms control before one of our plugins was dispatched.
                // DPI configuration is process-wide and already loaded; only
                // this optional text-rendering default is too late to change.
            }
            AppDomain.CurrentDomain.SetData(InitializedKey, true);
        }
    }

    public static void EnsurePerMonitorThread()
    {
        try
        {
            // Managed plugin windows are often constructed on private UI
            // threads. A thread does not reliably inherit the native caller's
            // temporary DPI context, so set PMv2 on the actual WinForms thread
            // before its first top-level HWND is created.
            SetThreadDpiAwarenessContext(new IntPtr(-4));
        }
        catch (EntryPointNotFoundException)
        {
            // Windows versions without per-monitor-v2 support keep the
            // process-level fallback selected by the native host.
        }
        catch (DllNotFoundException)
        {
        }
    }

    [DllImport("user32.dll")]
    private static extern IntPtr SetThreadDpiAwarenessContext(IntPtr dpiContext);
}

// The host executable opts WinForms into its .NET Framework 4.7+ PMv2 path.
// AutoScaleMode.Dpi gives programmatically constructed forms a stable 96-DPI
// design baseline; WinForms performs the single dynamic scaling pass.
internal class DpiAwareForm : Form
{
    internal DpiAwareForm()
    {
        ManagedApplication.EnsurePerMonitorThread();
        AutoScaleDimensions = new SizeF(96.0F, 96.0F);
        AutoScaleMode = AutoScaleMode.Dpi;
        // SystemFonts owns this instance. WinForms PMv2 recreates the native
        // HFONT as part of its own DPI pass; cloning and disposing fonts here
        // can invalidate an ambient Font still referenced by child controls.
        Font = SystemFonts.MessageBoxFont;
    }

    protected override void CreateHandle()
    {
        // The constructor body runs after the base Form constructor. Repeat
        // the context selection at the decisive point immediately before the
        // top-level HWND captures its DPI awareness.
        ManagedApplication.EnsurePerMonitorThread();
        base.CreateHandle();
    }

    protected int ScaleLogical(int logicalPixels)
    {
        return Math.Max(1, logicalPixels * DeviceDpi / 96);
    }

    protected override void OnDpiChanged(DpiChangedEventArgs e)
    {
        int oldDpi = e.DeviceDpiOld;
        int newDpi = e.DeviceDpiNew;

        SuspendLayout();
        try
        {
            // WinForms owns the single bounds and font autoscale pass.
            base.OnDpiChanged(e);
            OnPerMonitorDpiChanged(oldDpi, newDpi);
            PerformLayout();
        }
        finally
        {
            ResumeLayout(true);
        }

        Invalidate(true);
    }

    protected virtual void OnPerMonitorDpiChanged(int oldDpi, int newDpi)
    {
    }
}
