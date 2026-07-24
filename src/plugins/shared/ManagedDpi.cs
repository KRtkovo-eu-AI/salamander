// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
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
        lock (typeof(Application))
        {
            if (AppDomain.CurrentDomain.GetData(InitializedKey) is bool initialized && initialized)
            {
                return;
            }

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
}

// The host executable opts WinForms into its .NET Framework 4.7+ PMv2 path.
// AutoScaleMode.Dpi gives programmatically constructed forms a stable 96-DPI
// design baseline; WinForms performs the single dynamic scaling pass.
internal class DpiAwareForm : Form
{
    private readonly Dictionary<Control, Font> _dpiFonts = new Dictionary<Control, Font>();

    internal DpiAwareForm()
    {
        AutoScaleDimensions = new SizeF(96.0F, 96.0F);
        AutoScaleMode = AutoScaleMode.Dpi;
        Font = CloneFont(SystemFonts.MessageBoxFont);
        _dpiFonts[this] = Font;
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
            // WinForms owns the single bounds/autoscale pass. Afterwards force
            // every explicitly assigned point font to recreate its native
            // HFONT for the new monitor. This covers third-party controls whose
            // designer font prevents them from inheriting the Form font.
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
        base.Dispose(disposing);
        if (disposing)
        {
            foreach (Font font in _dpiFonts.Values)
            {
                font.Dispose();
            }
            _dpiFonts.Clear();
        }
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
            _dpiFonts.TryGetValue(root, out previous);
            root.Font = replacement;
            _dpiFonts[root] = replacement;
            if (previous != null && !ReferenceEquals(previous, replacement))
            {
                previous.Dispose();
            }
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
