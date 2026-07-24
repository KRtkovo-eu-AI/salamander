// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Collections.Generic;
using System.ComponentModel;
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
        // Initialize can run on a native plugin callback thread. The process is
        // PMv2-aware, but an HWND captures the awareness of its creating
        // thread, so select PMv2 before WinForms creates any hidden windows.
        EnsurePerMonitorThread();

        lock (typeof(Application))
        {
            if (AppDomain.CurrentDomain.GetData(InitializedKey) is bool initialized && initialized)
            {
                return;
            }

            // These switches must be selected before the first WinForms HWND.
            // Without them .NET Framework loaded by a native executable can
            // remain on its legacy SystemAware (96-DPI) scaling path.
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
            // JSON Viewer owns a private UI thread. Select PMv2 there before
            // its hidden dispatcher or first top-level HWND is created.
            SetThreadDpiAwarenessContext(new IntPtr(-4));
        }
        catch (EntryPointNotFoundException)
        {
            // Older Windows versions keep the process-level fallback.
        }
        catch (DllNotFoundException)
        {
        }
    }

    [DllImport("user32.dll")]
    private static extern IntPtr SetThreadDpiAwarenessContext(IntPtr dpiContext);
}

// AutoScaleMode.Dpi and the standard WinForms PMv2 path are deliberately kept
// identical to commit f1c8c579, where HyperV was verified at 150%. WinForms
// owns the one bounds/font scaling pass; this class only recreates explicitly
// assigned fonts after the HWND has acquired its monitor DPI.
internal class DpiAwareForm : Form
{
    private readonly Dictionary<Control, Font> _dpiFonts =
        new Dictionary<Control, Font>();
    private readonly List<Font> _retiredDpiFonts = new List<Font>();

    internal DpiAwareForm()
    {
        ManagedApplication.EnsurePerMonitorThread();
        AutoScaleDimensions = new SizeF(96.0F, 96.0F);
        AutoScaleMode = AutoScaleMode.Dpi;
        Font = CloneFont(SystemFonts.MessageBoxFont);
        _dpiFonts[this] = Font;
    }

    protected override void CreateHandle()
    {
        // This is the decisive point at which the top-level HWND captures the
        // thread DPI context. Repeat it here because constructors and native
        // callbacks can run under a temporarily changed context.
        ManagedApplication.EnsurePerMonitorThread();
        base.CreateHandle();
    }

    protected int ScaleLogical(int logicalPixels)
    {
        return Math.Max(1, logicalPixels * DeviceDpi / 96);
    }

    protected override void OnHandleCreated(EventArgs e)
    {
        base.OnHandleCreated(e);
        RefreshExplicitFonts(this);
    }

    protected override void OnDpiChanged(DpiChangedEventArgs e)
    {
        int oldDpi = e.DeviceDpiOld;
        int newDpi = e.DeviceDpiNew;

        SuspendLayout();
        try
        {
            base.OnDpiChanged(e);
            RefreshExplicitFonts(this);
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

    protected override void Dispose(bool disposing)
    {
        // Destroy every child HWND before releasing any managed Font from
        // which WinForms may still need to create an HFONT. This fixes the
        // Font.ToLogFont "Parameter is not valid" crash without changing the
        // scaling behavior that was known to work for HyperV.
        base.Dispose(disposing);
        if (!disposing)
        {
            return;
        }

        foreach (Font font in _retiredDpiFonts)
        {
            font.Dispose();
        }
        _retiredDpiFonts.Clear();

        foreach (Font font in _dpiFonts.Values)
        {
            font.Dispose();
        }
        _dpiFonts.Clear();
    }

    private void RefreshExplicitFonts(Control root)
    {
        PropertyDescriptor fontProperty = TypeDescriptor.GetProperties(root)["Font"];
        bool hasExplicitFont = ReferenceEquals(root, this) ||
                               (fontProperty != null && fontProperty.ShouldSerializeValue(root));
        if (hasExplicitFont && root.Font != null)
        {
            Font replacement = CloneFont(root.Font);
            Font previous;
            if (_dpiFonts.TryGetValue(root, out previous) &&
                !ReferenceEquals(previous, replacement))
            {
                // Do not dispose it here. Descendants can still inherit the
                // old Font until the complete native DPI cascade has returned.
                _retiredDpiFonts.Add(previous);
            }
            root.Font = replacement;
            _dpiFonts[root] = replacement;
        }

        foreach (Control child in root.Controls)
        {
            RefreshExplicitFonts(child);
        }
    }

    private static Font CloneFont(Font source)
    {
        return new Font(source.FontFamily, source.Size, source.Style, source.Unit,
                        source.GdiCharSet, source.GdiVerticalFont);
    }
}
