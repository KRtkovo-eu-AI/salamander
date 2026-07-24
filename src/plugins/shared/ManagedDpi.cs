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

        // Set every .NET Framework 4.7+ dynamic-DPI opt-in before even
        // touching typeof(Application). Salamander is a native PMv2 host, so
        // System.Windows.Forms can be loaded after the process awareness was
        // fixed by the EXE manifest and otherwise cache its legacy
        // SystemAware behavior before the WinForms config section is read.
        AppContext.SetSwitch("Switch.System.Windows.Forms.DoNotSupportDpiChanges", false);
        AppContext.SetSwitch("Switch.System.Windows.Forms.EnableDpiChangedMessageHandling", true);
        AppContext.SetSwitch("Switch.System.Windows.Forms.EnableDpiChangedHighDpiImprovements", true);
        AppContext.SetSwitch("Switch.System.Windows.Forms.EnableWindowsFormsHighDpiAutoResizing", false);

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

// The native host receives transient DPI messages while a window straddles
// two monitors. Never multiply the current layout: capture one immutable
// 96-DPI snapshot and restore exact values from it after coalescing messages.
internal class DpiAwareForm : Form
{
    private const int WmSize = 0x0005;
    private const int WmEnterSizeMove = 0x0231;
    private const int WmExitSizeMove = 0x0232;
    private const int WmDpiChanged = 0x02E0;
    private const int SizeMinimized = 1;

    [StructLayout(LayoutKind.Sequential)]
    private struct NativeRect
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    private sealed class ControlSnapshot
    {
        public Rectangle Bounds96;
        public Padding Margin96;
        public Padding Padding96;
        public Size MinimumSize96;
        public Size MaximumSize96;
        public Font FontTemplate = null!;
    }

    private readonly Dictionary<Control, ControlSnapshot> _snapshots =
        new Dictionary<Control, ControlSnapshot>();
    private readonly List<Font> _activeFonts = new List<Font>();
    private Size _logicalWindowSize;
    private int _currentDpi = 96;
    private int _pendingDpi;
    private Rectangle _pendingSuggested;
    private bool _baselineCaptured;
    private bool _dpiApplyPosted;
    private bool _applyingDpi;
    private bool _inSizeMove;
    private readonly Timer _dpiSettleTimer;

    internal DpiAwareForm()
    {
        ManagedApplication.EnsurePerMonitorThread();
        AutoScaleDimensions = new SizeF(96.0F, 96.0F);
        AutoScaleMode = AutoScaleMode.Dpi;
        Font initialFont = CloneFont(SystemFonts.MessageBoxFont);
        _activeFonts.Add(initialFont);
        Font = initialFont;
        _dpiSettleTimer = new Timer { Interval = 75 };
        _dpiSettleTimer.Tick += (_, _) =>
        {
            _dpiSettleTimer.Stop();
            if (!_inSizeMove)
            {
                ApplyPendingDpiChange();
            }
        };
    }

    protected override void CreateHandle()
    {
        ManagedApplication.EnsurePerMonitorThread();
        if (!_baselineCaptured)
        {
            DisableNestedAutoScaling(this);
            CaptureSubtree(this, 96);
            _logicalWindowSize = Size;
            _baselineCaptured = true;
        }
        base.CreateHandle();
    }

    protected int ScaleLogical(int logicalPixels)
    {
        return Math.Max(1, ScaleValue(logicalPixels, _currentDpi));
    }

    protected override void OnHandleCreated(EventArgs e)
    {
        base.OnHandleCreated(e);
        int handleDpi = DeviceDpi > 0 ? DeviceDpi : 96;
        ApplySnapshot(handleDpi, Rectangle.Empty);
    }

    protected override void WndProc(ref Message m)
    {
        if (m.Msg == WmEnterSizeMove)
        {
            _inSizeMove = true;
            _dpiSettleTimer.Stop();
            base.WndProc(ref m);
            return;
        }

        if (m.Msg == WmDpiChanged)
        {
            int newDpi = unchecked((int)(m.WParam.ToInt64() & 0xffff));
            Rectangle suggested = Rectangle.Empty;
            if (m.LParam != IntPtr.Zero)
            {
                NativeRect native = Marshal.PtrToStructure<NativeRect>(m.LParam);
                suggested = Rectangle.FromLTRB(
                    native.Left, native.Top, native.Right, native.Bottom);
            }

            QueueDpiChange(newDpi, suggested);
            // We own dynamic scaling for these native-hosted WinForms. Do not
            // let DefWndProc/WinForms apply the suggested rectangle while the
            // mouse is still moving the window across the monitor boundary.
            m.Result = IntPtr.Zero;
            return;
        }

        if (m.Msg == WmExitSizeMove)
        {
            base.WndProc(ref m);
            _inSizeMove = false;
            if (_dpiApplyPosted)
            {
                _dpiSettleTimer.Stop();
                _dpiSettleTimer.Start();
            }
            return;
        }

        base.WndProc(ref m);

        if (m.Msg == WmSize && !_applyingDpi && !_dpiApplyPosted &&
            m.WParam.ToInt32() != SizeMinimized && _baselineCaptured)
        {
            _logicalWindowSize = ToLogical(Size, _currentDpi);
        }
    }

    protected virtual void OnPerMonitorDpiChanged(int oldDpi, int newDpi)
    {
    }

    private void QueueDpiChange(int dpi, Rectangle suggested)
    {
        if (dpi <= 0 || IsDisposed)
        {
            return;
        }

        _pendingDpi = dpi;
        _pendingSuggested = suggested;
        _dpiApplyPosted = true;
        if (!IsHandleCreated || _inSizeMove)
        {
            return;
        }

        _dpiSettleTimer.Stop();
        _dpiSettleTimer.Start();
    }

    private void ApplyPendingDpiChange()
    {
        if (IsDisposed || !IsHandleCreated)
        {
            _dpiApplyPosted = false;
            return;
        }

        int requestedDpi = _pendingDpi;
        Rectangle suggested = _pendingSuggested;
        _dpiApplyPosted = false;

        uint windowDpi = GetDpiForWindow(Handle);
        int finalDpi = windowDpi > 0 ? (int)windowDpi : requestedDpi;
        if (finalDpi == _currentDpi)
        {
            return;
        }

        // A newer transition won the race. Do not apply an obsolete rectangle
        // belonging to the other monitor.
        if (finalDpi != requestedDpi)
        {
            suggested = Rectangle.Empty;
        }
        ApplySnapshot(finalDpi, suggested);
    }

    private void ApplySnapshot(int dpi, Rectangle suggested)
    {
        if (_applyingDpi || dpi <= 0 || !_baselineCaptured)
        {
            return;
        }

        int oldDpi = _currentDpi;
        _applyingDpi = true;
        _currentDpi = dpi;
        SuspendTree(this);
        try
        {
            DisableNestedAutoScaling(this);
            RestoreSubtree(this, dpi);
            AutoScaleDimensions = new SizeF(dpi, dpi);

            Size exactSize = ScaleSize(_logicalWindowSize, dpi);
            Point location = suggested.IsEmpty ? Location : suggested.Location;
            Bounds = new Rectangle(location, exactSize);

            OnPerMonitorDpiChanged(oldDpi, dpi);
        }
        finally
        {
            ResumeTree(this);
            _applyingDpi = false;
        }

        PerformLayout();
        Invalidate(true);
    }

    private void CaptureSubtree(Control root, int sourceDpi)
    {
        if (!_snapshots.ContainsKey(root))
        {
            PropertyDescriptor fontProperty = TypeDescriptor.GetProperties(root)["Font"];
            bool explicitFont = ReferenceEquals(root, this) ||
                                (fontProperty != null && fontProperty.ShouldSerializeValue(root));
            _snapshots.Add(root, new ControlSnapshot
            {
                Bounds96 = ToLogical(root.Bounds, sourceDpi),
                Margin96 = ToLogical(root.Margin, sourceDpi),
                Padding96 = ToLogical(root.Padding, sourceDpi),
                MinimumSize96 = ToLogical(root.MinimumSize, sourceDpi),
                MaximumSize96 = ToLogical(root.MaximumSize, sourceDpi),
                FontTemplate = explicitFont && root.Font != null
                    ? CloneFont(root.Font)
                    : null!,
            });
            root.ControlAdded += OnDescendantControlAdded;
        }

        foreach (Control child in root.Controls)
        {
            CaptureSubtree(child, sourceDpi);
        }
    }

    private void OnDescendantControlAdded(object sender, ControlEventArgs e)
    {
        DisableNestedAutoScaling(e.Control);
        CaptureSubtree(e.Control, Math.Max(1, _currentDpi));
    }

    private void RestoreSubtree(Control root, int dpi)
    {
        ControlSnapshot snapshot;
        if (_snapshots.TryGetValue(root, out snapshot))
        {
            root.Margin = ScalePadding(snapshot.Margin96, dpi);
            root.Padding = ScalePadding(snapshot.Padding96, dpi);
            root.MinimumSize = ScaleSize(snapshot.MinimumSize96, dpi);
            root.MaximumSize = ScaleSize(snapshot.MaximumSize96, dpi);
            if (!ReferenceEquals(root, this))
            {
                root.Bounds = ScaleRectangle(snapshot.Bounds96, dpi);
            }
            if (snapshot.FontTemplate != null)
            {
                Font font = CloneFont(snapshot.FontTemplate);
                _activeFonts.Add(font);
                root.Font = font;
            }
        }

        foreach (Control child in root.Controls)
        {
            RestoreSubtree(child, dpi);
        }
    }

    private static void DisableNestedAutoScaling(Control root)
    {
        foreach (Control child in root.Controls)
        {
            if (child is ContainerControl container)
            {
                container.AutoScaleMode = AutoScaleMode.None;
            }
            DisableNestedAutoScaling(child);
        }
    }

    private static void SuspendTree(Control root)
    {
        root.SuspendLayout();
        foreach (Control child in root.Controls)
        {
            SuspendTree(child);
        }
    }

    private static void ResumeTree(Control root)
    {
        foreach (Control child in root.Controls)
        {
            ResumeTree(child);
        }
        root.ResumeLayout(false);
    }

    protected override void Dispose(bool disposing)
    {
        base.Dispose(disposing);
        if (!disposing)
        {
            return;
        }

        foreach (ControlSnapshot snapshot in _snapshots.Values)
        {
            snapshot.FontTemplate?.Dispose();
        }
        _snapshots.Clear();

        foreach (Font font in _activeFonts)
        {
            font.Dispose();
        }
        _activeFonts.Clear();
        _dpiSettleTimer.Dispose();
    }

    private static int ScaleValue(int value, int dpi)
    {
        return (int)Math.Round(value * dpi / 96.0);
    }

    private static Rectangle ScaleRectangle(Rectangle value, int dpi)
    {
        return new Rectangle(
            ScaleValue(value.X, dpi), ScaleValue(value.Y, dpi),
            ScaleValue(value.Width, dpi), ScaleValue(value.Height, dpi));
    }

    private static Size ScaleSize(Size value, int dpi)
    {
        return new Size(
            value.Width == 0 ? 0 : Math.Max(1, ScaleValue(value.Width, dpi)),
            value.Height == 0 ? 0 : Math.Max(1, ScaleValue(value.Height, dpi)));
    }

    private static Padding ScalePadding(Padding value, int dpi)
    {
        return new Padding(
            ScaleValue(value.Left, dpi), ScaleValue(value.Top, dpi),
            ScaleValue(value.Right, dpi), ScaleValue(value.Bottom, dpi));
    }

    private static Rectangle ToLogical(Rectangle value, int dpi)
    {
        return new Rectangle(
            ToLogical(value.X, dpi), ToLogical(value.Y, dpi),
            ToLogical(value.Width, dpi), ToLogical(value.Height, dpi));
    }

    private static Size ToLogical(Size value, int dpi)
    {
        return new Size(ToLogical(value.Width, dpi), ToLogical(value.Height, dpi));
    }

    private static Padding ToLogical(Padding value, int dpi)
    {
        return new Padding(
            ToLogical(value.Left, dpi), ToLogical(value.Top, dpi),
            ToLogical(value.Right, dpi), ToLogical(value.Bottom, dpi));
    }

    private static int ToLogical(int value, int dpi)
    {
        return (int)Math.Round(value * 96.0 / Math.Max(1, dpi));
    }

    private static Font CloneFont(Font source)
    {
        return new Font(source.FontFamily, source.Size, source.Style, source.Unit,
                        source.GdiCharSet, source.GdiVerticalFont);
    }

    [DllImport("user32.dll")]
    private static extern uint GetDpiForWindow(IntPtr hwnd);
}
