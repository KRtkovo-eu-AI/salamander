// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace OpenSalamander;

internal static class ManagedDpiTrace
{
    private static readonly object SyncRoot = new object();
    internal static readonly string LogPath =
        Path.Combine(Path.GetTempPath(), "OpenSalamander.ManagedDpi.log");

    internal static void Write(string eventName, Form form = null!, int messageDpi = 0)
    {
        try
        {
            IntPtr hwnd = form != null && form.IsHandleCreated
                ? form.Handle
                : IntPtr.Zero;
            uint windowDpi = hwnd != IntPtr.Zero ? GetDpiForWindow(hwnd) : 0;
            IntPtr awareness = hwnd != IntPtr.Zero
                ? GetWindowDpiAwarenessContext(hwnd)
                : IntPtr.Zero;
            IntPtr threadAwareness = GetThreadDpiAwarenessContext();
            bool pmv2 = awareness != IntPtr.Zero &&
                        AreDpiAwarenessContextsEqual(awareness, new IntPtr(-4));
            bool threadPmv2 = threadAwareness != IntPtr.Zero &&
                              AreDpiAwarenessContextsEqual(threadAwareness, new IntPtr(-4));
            string typeName = form?.GetType().FullName ?? "-";
            int deviceDpi = form?.DeviceDpi ?? 0;
            string targetFramework = AppContext.TargetFrameworkName ?? "-";
            string setupFramework =
                AppDomain.CurrentDomain.SetupInformation.TargetFrameworkName ?? "-";
            string line =
                $"{DateTime.UtcNow:O} pid={Process.GetCurrentProcess().Id} " +
                $"tid={GetCurrentThreadId()} event={eventName} form={typeName} " +
                $"msgDpi={messageDpi} deviceDpi={deviceDpi} windowDpi={windowDpi} " +
                $"context=0x{awareness.ToInt64():X} pmv2={pmv2} " +
                $"threadContext=0x{threadAwareness.ToInt64():X} threadPmv2={threadPmv2} " +
                $"targetFramework={targetFramework} setupFramework={setupFramework}" +
                $"{Environment.NewLine}";
            lock (SyncRoot)
            {
                File.AppendAllText(LogPath, line);
            }
        }
        catch
        {
            // Diagnostics must never affect plugin UI.
        }
    }

    internal static void WriteWinFormsDpiState()
    {
        try
        {
            Type helper = typeof(Form).Assembly.GetType(
                "System.Windows.Forms.DpiHelper", throwOnError: true);
            const BindingFlags flags =
                BindingFlags.Static | BindingFlags.NonPublic |
                BindingFlags.Public;

            object messageHandling = helper.GetProperty(
                "EnableDpiChangedMessageHandling", flags).GetValue(null, null);
            object improvements = helper.GetProperty(
                "EnableDpiChangedHighDpiImprovements", flags).GetValue(null, null);
            object highDpi = helper.GetField(
                "enableHighDpi", flags).GetValue(null);
            object awareness = helper.GetField(
                "dpiAwarenessValue", flags).GetValue(null);

            Write(
                $"WinFormsDpiState.highDpi={highDpi}" +
                $".messageHandling={messageHandling}" +
                $".improvements={improvements}" +
                $".awareness={awareness}");
        }
        catch (Exception ex)
        {
            Write($"WinFormsDpiState.error={ex.GetType().Name}");
        }
    }

    [DllImport("kernel32.dll")]
    private static extern uint GetCurrentThreadId();

    [DllImport("user32.dll")]
    private static extern uint GetDpiForWindow(IntPtr hwnd);

    [DllImport("user32.dll")]
    private static extern IntPtr GetWindowDpiAwarenessContext(IntPtr hwnd);

    [DllImport("user32.dll")]
    private static extern IntPtr GetThreadDpiAwarenessContext();

    [DllImport("user32.dll")]
    private static extern bool AreDpiAwarenessContextsEqual(
        IntPtr first, IntPtr second);
}

// Managed plugins share the default AppDomain but are separate assemblies.
// AppDomain data prevents every plugin from trying to change process-wide
// WinForms defaults after another plugin has already created a control.
internal static class ManagedApplication
{
    private const string InitializedKey = "OpenSalamander.WinForms.Initialized";
    private const uint MonitorDefaultToNearest = 2;
    private const int MonitorDpiTypeEffective = 0;

    internal static void InitializeDpiPolicyAtModuleLoad()
    {
        // Set the .NET Framework 4.7+ dynamic-DPI switches before touching the
        // Application type. The EXE config remains the primary WinForms opt-in.
        AppContext.SetSwitch("Switch.System.Windows.Forms.DoNotSupportDpiChanges", false);
        AppContext.SetSwitch("Switch.System.Windows.Forms.EnableDpiChangedMessageHandling", true);
        AppContext.SetSwitch("Switch.System.Windows.Forms.EnableDpiChangedHighDpiImprovements", true);
        AppContext.SetSwitch("Switch.System.Windows.Forms.EnableWindowsFormsHighDpiAutoResizing", true);
        ManagedDpiTrace.Write("InitializeDpiPolicyAtModuleLoad");
    }

    public static void Initialize()
    {
        InitializeDpiPolicyAtModuleLoad();
        ManagedDpiTrace.Write("Initialize");

        using (EnterPerMonitorV2())
        {
            lock (typeof(Application))
            {
                if (AppDomain.CurrentDomain.GetData(InitializedKey) is bool initialized && initialized)
                {
                    return;
                }

                Application.EnableVisualStyles();
                EnableNetFrameworkDpiMessageHandling();
                ManagedDpiTrace.WriteWinFormsDpiState();
                try
                {
                    Application.SetCompatibleTextRenderingDefault(false);
                }
                catch (InvalidOperationException)
                {
                    // A third-party managed plugin may have created the first
                    // WinForms control before one of our plugins was dispatched.
                }
                AppDomain.CurrentDomain.SetData(InitializedKey, true);
            }
        }
    }

    private static void EnableNetFrameworkDpiMessageHandling()
    {
        Type helper = typeof(Form).Assembly.GetType(
            "System.Windows.Forms.DpiHelper", throwOnError: true);
        const BindingFlags flags =
            BindingFlags.Static | BindingFlags.NonPublic |
            BindingFlags.Public;

        // Initialize the regular WinForms configuration first, then correct
        // the two compatibility gates that remain disabled in a native CLR
        // host without managed entry-assembly target-framework metadata.
        helper.GetMethod(
            "InitializeDpiHelperForWinforms", flags).Invoke(null, null);
        helper.GetField(
            "enableHighDpi", flags).SetValue(null, true);
        helper.GetField(
            "dpiAwarenessValue", flags).SetValue(null, "PerMonitorV2");
        helper.GetField(
            "enableDpiChangedMessageHandling", flags).SetValue(null, true);
        helper.GetField(
            "enableDpiChangedHighDpiImprovements", flags).SetValue(null, true);
    }

    public static IDisposable EnterPerMonitorV2()
    {
        try
        {
            return new DpiAwarenessScope(
                SetThreadDpiAwarenessContext(new IntPtr(-4)));
        }
        catch (EntryPointNotFoundException)
        {
            // Older Windows versions keep the process-level fallback.
        }
        catch (DllNotFoundException)
        {
        }
        return DpiAwarenessScope.Empty;
    }

    public static IDisposable EnterPerMonitorV1()
    {
        try
        {
            return new DpiAwarenessScope(
                SetThreadDpiAwarenessContext(new IntPtr(-3)));
        }
        catch (EntryPointNotFoundException)
        {
            // Older Windows versions keep the process-level fallback.
        }
        catch (DllNotFoundException)
        {
        }
        return DpiAwarenessScope.Empty;
    }

    internal static int GetEffectiveDpiForWindow(IntPtr window)
    {
        if (window == IntPtr.Zero)
        {
            return 96;
        }

        IntPtr root = GetAncestor(window, 2); // GA_ROOT
        if (root != IntPtr.Zero)
        {
            window = root;
        }

        // GetDpiForMonitor is used deliberately here: this dialog performs
        // all scaling itself and needs the monitor's effective DPI even when
        // its native host has just processed a live display-scale change.
        try
        {
            IntPtr monitor = MonitorFromWindow(
                window, MonitorDefaultToNearest);
            uint dpiX;
            uint dpiY;
            if (monitor != IntPtr.Zero &&
                GetDpiForMonitor(
                    monitor, MonitorDpiTypeEffective, out dpiX, out dpiY) == 0 &&
                dpiX > 0)
            {
                return unchecked((int)dpiX);
            }
        }
        catch (EntryPointNotFoundException)
        {
        }
        catch (DllNotFoundException)
        {
        }

        uint windowDpi = GetDpiForWindow(window);
        return windowDpi > 0 ? unchecked((int)windowDpi) : 96;
    }

    private sealed class DpiAwarenessScope : IDisposable
    {
        internal static readonly DpiAwarenessScope Empty =
            new DpiAwarenessScope(IntPtr.Zero);

        private IntPtr _oldContext;

        internal DpiAwarenessScope(IntPtr oldContext)
        {
            _oldContext = oldContext;
        }

        public void Dispose()
        {
            IntPtr oldContext = _oldContext;
            _oldContext = IntPtr.Zero;
            if (oldContext != IntPtr.Zero)
            {
                SetThreadDpiAwarenessContext(oldContext);
            }
        }
    }

    [DllImport("user32.dll", SetLastError = true)]
    private static extern IntPtr SetThreadDpiAwarenessContext(IntPtr dpiContext);

    [DllImport("user32.dll")]
    private static extern IntPtr GetAncestor(IntPtr window, uint flags);

    [DllImport("user32.dll")]
    private static extern IntPtr MonitorFromWindow(
        IntPtr window, uint flags);

    [DllImport("user32.dll")]
    private static extern uint GetDpiForWindow(IntPtr window);

    [DllImport("shcore.dll")]
    private static extern int GetDpiForMonitor(
        IntPtr monitor,
        int dpiType,
        out uint dpiX,
        out uint dpiY);
}

// WinForms owns the only bounds/control scaling pass. This base class only
// recreates explicitly assigned fonts after WinForms has updated DeviceDpi.
internal class DpiAwareForm : Form
{
    private const int WmDpiChanged = 0x02E0;
    private bool _initialDpiSynchronized;

    private readonly Dictionary<Control, Font> _dpiFonts =
        new Dictionary<Control, Font>();
    private readonly List<Font> _retiredDpiFonts = new List<Font>();

    internal DpiAwareForm()
    {
        AutoScaleDimensions = new SizeF(96.0F, 96.0F);
        AutoScaleMode = AutoScaleMode.Dpi;
        Font = CloneFont(SystemFonts.MessageBoxFont);
        _dpiFonts[this] = Font;
    }

    protected override void CreateHandle()
    {
        using (ManagedApplication.EnterPerMonitorV2())
        {
            base.CreateHandle();
        }
    }

    protected int ScaleLogical(int logicalPixels)
    {
        return Math.Max(1, logicalPixels * DeviceDpi / 96);
    }

    protected override void OnHandleCreated(EventArgs e)
    {
        base.OnHandleCreated(e);
        ManagedDpiTrace.Write("HandleCreated", this);
    }

    protected virtual bool SynchronizeInitialDpi => false;

    protected virtual bool ScaleInitialWindowBounds => true;

    protected override void OnLoad(EventArgs e)
    {
        if (SynchronizeInitialDpi && !_initialDpiSynchronized)
        {
            SynchronizeDpiFromWindow();
        }
        base.OnLoad(e);
    }

    protected override void WndProc(ref Message m)
    {
        if (m.Msg == WmDpiChanged)
        {
            int newDpi = unchecked((int)(m.WParam.ToInt64() & 0xffff));
            ManagedDpiTrace.Write("WM_DPICHANGED.before", this, newDpi);
            base.WndProc(ref m);
            ManagedDpiTrace.Write("WM_DPICHANGED.after", this, newDpi);
            return;
        }
        base.WndProc(ref m);
    }

    protected override void OnDpiChanged(DpiChangedEventArgs e)
    {
        int oldDpi = e.DeviceDpiOld;
        int newDpi = e.DeviceDpiNew;
        ManagedDpiTrace.Write("OnDpiChanged.before", this, newDpi);

        base.OnDpiChanged(e);
        OnPerMonitorDpiChanged(oldDpi, newDpi);
        Invalidate(true);
        ManagedDpiTrace.Write("OnDpiChanged.after", this, newDpi);
    }

    protected virtual void OnPerMonitorDpiChanged(int oldDpi, int newDpi)
    {
    }

    private void SynchronizeDpiFromWindow()
    {
        if (IsDisposed || !IsHandleCreated || _initialDpiSynchronized)
        {
            return;
        }
        _initialDpiSynchronized = true;

        int windowDpi = unchecked((int)GetDpiForWindow(Handle));
        int controlDpi = DeviceDpi;
        ManagedDpiTrace.Write("InitialDpiSync", this, windowDpi);
        if (windowDpi <= 0 || controlDpi <= 0 || windowDpi == controlDpi)
        {
            return;
        }

        NativeRect suggested;
        if (!GetWindowRect(Handle, out suggested))
        {
            return;
        }
        if (ScaleInitialWindowBounds)
        {
            int width = suggested.Right - suggested.Left;
            int height = suggested.Bottom - suggested.Top;
            suggested.Right = suggested.Left +
                              width * windowDpi / controlDpi;
            suggested.Bottom = suggested.Top +
                               height * windowDpi / controlDpi;
        }

        IntPtr rectangle = Marshal.AllocHGlobal(
            Marshal.SizeOf(typeof(NativeRect)));
        try
        {
            Marshal.StructureToPtr(suggested, rectangle, false);
            long dpiPair = (uint)windowDpi |
                           ((long)(uint)windowDpi << 16);
            SendMessage(
                Handle, WmDpiChanged, new IntPtr(dpiPair), rectangle);
        }
        finally
        {
            Marshal.FreeHGlobal(rectangle);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct NativeRect
    {
        internal int Left;
        internal int Top;
        internal int Right;
        internal int Bottom;
    }

    [DllImport("user32.dll")]
    private static extern uint GetDpiForWindow(IntPtr hwnd);

    [DllImport("user32.dll")]
    private static extern bool GetWindowRect(
        IntPtr hwnd, out NativeRect rectangle);

    [DllImport("user32.dll")]
    private static extern IntPtr SendMessage(
        IntPtr hwnd, int message, IntPtr wParam, IntPtr lParam);

    protected override void Dispose(bool disposing)
    {
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

// These two legacy WinForms control trees are deliberately created as
// Per-Monitor V1 windows. Windows sends one WM_DPICHANGED to the top-level
// window, but does not run the PMv2 child-window scaling cascade. Every value
// is consequently rebuilt once from an immutable 96-DPI baseline.
#nullable disable
internal class DeterministicDpiForm : Form
{
    private const int WmDpiChanged = 0x02E0;
    private const int WmGetDpiScaledSize = 0x02E4;
    private const int WmSetRedraw = 0x000B;
    private const uint RdwInvalidate = 0x0001;
    private const uint RdwErase = 0x0004;
    private const uint RdwAllChildren = 0x0080;
    private const uint RdwFrame = 0x0400;

    private readonly Dictionary<Control, LogicalControl> _logicalControls =
        new Dictionary<Control, LogicalControl>();
    private readonly List<Font> _baselineFonts = new List<Font>();
    private readonly List<Font> _appliedFonts = new List<Font>();
    private readonly List<Font> _retiredDpiFonts = new List<Font>();
    private bool _baselineCaptured;
    private bool _applyingDpi;
    private bool _dpiVerificationPending;
    private Size _logicalWindowSize;
    private int _currentDpi = 96;

    internal DeterministicDpiForm()
    {
        AutoScaleMode = AutoScaleMode.None;
        Font = CloneFont(SystemFonts.MessageBoxFont);
    }

    protected virtual bool ScaleInitialWindowBounds => true;

    protected virtual Size LogicalWindowSize => Size.Empty;

    protected virtual bool VerifyDpiAfterPositionChange => false;

    internal int InitialDpi { get; set; }

    // Call after the complete control tree and its theme have been created,
    // but before the first HWND. This makes the baseline independent of any
    // initial WinForms/native DPI work performed during handle creation.
    internal void CaptureDpiBaseline()
    {
        if (_baselineCaptured)
        {
            return;
        }

        CaptureLogicalControls(this);
        _baselineCaptured = true;
    }

    protected int ScaleLogical(int logicalPixels)
    {
        return Math.Max(1, MulDiv(logicalPixels, _currentDpi, 96));
    }

    protected override void CreateHandle()
    {
        using (ManagedApplication.EnterPerMonitorV1())
        {
            base.CreateHandle();
        }
    }

    protected override void OnLoad(EventArgs e)
    {
        CaptureDpiBaseline();

        int targetDpi = InitialDpi;
        if (targetDpi <= 0)
        {
            targetDpi = ManagedApplication.GetEffectiveDpiForWindow(Handle);
        }
        if (targetDpi <= 0)
        {
            targetDpi = 96;
        }

        Rectangle initialBounds = Bounds;
        Size fixedLogicalSize = LogicalWindowSize;
        if (!fixedLogicalSize.IsEmpty)
        {
            _logicalWindowSize = fixedLogicalSize;
        }
        else if (ScaleInitialWindowBounds)
        {
            _logicalWindowSize = initialBounds.Size;
        }
        else
        {
            _logicalWindowSize = new Size(
                Math.Max(1, MulDiv(initialBounds.Width, 96, targetDpi)),
                Math.Max(1, MulDiv(initialBounds.Height, 96, targetDpi)));
        }

        Rectangle targetBounds = new Rectangle(
            initialBounds.Location,
            new Size(
                Math.Max(1, MulDiv(_logicalWindowSize.Width, targetDpi, 96)),
                Math.Max(1, MulDiv(_logicalWindowSize.Height, targetDpi, 96))));
        ManagedDpiTrace.Write("DeterministicDpi.OnLoad", this, targetDpi);
        ApplyDpi(96, targetDpi, targetBounds);
        base.OnLoad(e);
    }

    protected override void OnShown(EventArgs e)
    {
        base.OnShown(e);
        QueueDpiVerification();
    }

    protected override void WndProc(ref Message m)
    {
        if (m.Msg == WmGetDpiScaledSize)
        {
            if (!_baselineCaptured)
            {
                CaptureDpiBaseline();
            }

            if (!_baselineCaptured ||
                !TryInitializeFixedLogicalWindowSize())
            {
                base.WndProc(ref m);
                return;
            }

            int newDpi = m.WParam.ToInt32();
            if (newDpi > 0 && m.LParam != IntPtr.Zero)
            {
                NativeSize size = new NativeSize
                {
                    Width = Math.Max(
                        1, MulDiv(_logicalWindowSize.Width, newDpi, 96)),
                    Height = Math.Max(
                        1, MulDiv(_logicalWindowSize.Height, newDpi, 96))
                };
                Marshal.StructureToPtr(size, m.LParam, false);
                m.Result = new IntPtr(1);
                return;
            }
        }

        if (m.Msg == WmDpiChanged)
        {
            if (!_baselineCaptured)
            {
                CaptureDpiBaseline();
            }

            if (!_baselineCaptured ||
                !TryInitializeFixedLogicalWindowSize())
            {
                base.WndProc(ref m);
                return;
            }

            int newDpi = unchecked((int)(m.WParam.ToInt64() & 0xffff));
            if (!_applyingDpi && newDpi > 0 && newDpi != _currentDpi)
            {
                NativeRect suggested =
                    Marshal.PtrToStructure<NativeRect>(m.LParam);
                Rectangle bounds = new Rectangle(
                    suggested.Left,
                    suggested.Top,
                    Math.Max(
                        1, MulDiv(_logicalWindowSize.Width, newDpi, 96)),
                    Math.Max(
                        1, MulDiv(_logicalWindowSize.Height, newDpi, 96)));
                ApplyDpi(_currentDpi, newDpi, bounds);
            }
            m.Result = IntPtr.Zero;
            return;
        }

        base.WndProc(ref m);

        const int WmWindowPosChanged = 0x0047;
        const int WmSettingChange = 0x001A;
        if (VerifyDpiAfterPositionChange &&
            _baselineCaptured &&
            !_applyingDpi &&
            (m.Msg == WmWindowPosChanged || m.Msg == WmSettingChange))
        {
            QueueDpiVerification();
        }
    }

    protected virtual void OnDeterministicDpiChanged(
        int oldDpi, int newDpi)
    {
    }

    private void ApplyDpi(int oldDpi, int newDpi, Rectangle windowBounds)
    {
        ManagedDpiTrace.Write(
            "DeterministicDpi.Apply.before", this, newDpi);
        _applyingDpi = true;
        SendMessage(Handle, WmSetRedraw, IntPtr.Zero, IntPtr.Zero);
        SuspendTree(this);
        try
        {
            _currentDpi = newDpi;
            List<Font> previousFonts =
                new List<Font>(_appliedFonts);
            _appliedFonts.Clear();

            ApplyLogicalControls(this, newDpi);
            SetBounds(
                windowBounds.X,
                windowBounds.Y,
                windowBounds.Width,
                windowBounds.Height,
                BoundsSpecified.All);
            OnDeterministicDpiChanged(oldDpi, newDpi);

            // FontChanged/layout notifications are delivered while the
            // suspended tree is resumed below. Descendants may still expose
            // the formerly inherited Font until then, so disposing it here
            // can make a concurrent/reentrant paint call Font.Height on an
            // invalid GDI+ object. Retire it with the form instead.
            _retiredDpiFonts.AddRange(previousFonts);
        }
        finally
        {
            ResumeTree(this);
            SendMessage(Handle, WmSetRedraw, new IntPtr(1), IntPtr.Zero);
            RedrawWindow(
                Handle,
                IntPtr.Zero,
                IntPtr.Zero,
                RdwInvalidate | RdwErase | RdwAllChildren | RdwFrame);
            _applyingDpi = false;
            ManagedDpiTrace.Write(
                "DeterministicDpi.Apply.after", this, newDpi);
        }
    }

    private void QueueDpiVerification()
    {
        if (_dpiVerificationPending || IsDisposed || !IsHandleCreated)
        {
            return;
        }
        _dpiVerificationPending = true;
        BeginInvoke(new Action(VerifyWindowDpi));
    }

    private bool TryInitializeFixedLogicalWindowSize()
    {
        if (!_logicalWindowSize.IsEmpty)
        {
            return true;
        }

        Size fixedLogicalSize = LogicalWindowSize;
        if (fixedLogicalSize.IsEmpty)
        {
            // Windows can send the message after CreateHandle but before
            // OnLoad has derived a logical size from externally supplied
            // physical bounds (JsonViewer detached windows). Let WinForms
            // handle that early message rather than returning 1x1.
            return false;
        }

        _logicalWindowSize = fixedLogicalSize;
        return true;
    }

    private void VerifyWindowDpi()
    {
        _dpiVerificationPending = false;
        if (IsDisposed || !IsHandleCreated || _applyingDpi)
        {
            return;
        }

        int targetDpi = ManagedApplication.GetEffectiveDpiForWindow(Handle);
        if (targetDpi <= 0 || targetDpi == _currentDpi)
        {
            return;
        }

        NativeRect current;
        if (!GetWindowRect(Handle, out current))
        {
            return;
        }
        ApplyDpi(
            _currentDpi,
            targetDpi,
            new Rectangle(
                current.Left,
                current.Top,
                Math.Max(
                    1, MulDiv(_logicalWindowSize.Width, targetDpi, 96)),
                Math.Max(
                    1, MulDiv(_logicalWindowSize.Height, targetDpi, 96))));
    }

    private void CaptureLogicalControls(Control control)
    {
        PropertyDescriptor fontProperty =
            TypeDescriptor.GetProperties(control)["Font"];
        bool explicitFont =
            ReferenceEquals(control, this) ||
            (fontProperty != null &&
             fontProperty.ShouldSerializeValue(control));
        Font baselineFont =
            explicitFont && control.Font != null
                ? CloneFont(control.Font)
                : null;
        if (baselineFont != null)
        {
            _baselineFonts.Add(baselineFont);
        }

        _logicalControls[control] = new LogicalControl(
            control.Bounds,
            control.Margin,
            control.Padding,
            control.MinimumSize,
            control.MaximumSize,
            baselineFont);
        foreach (Control child in control.Controls)
        {
            CaptureLogicalControls(child);
        }
    }

    private void ApplyLogicalControls(Control control, int dpi)
    {
        LogicalControl logical;
        if (_logicalControls.TryGetValue(control, out logical))
        {
            if (!ReferenceEquals(control, this))
            {
                control.Bounds = Scale(logical.Bounds, dpi);
            }
            control.Margin = Scale(logical.Margin, dpi);
            control.Padding = Scale(logical.Padding, dpi);
            control.MinimumSize = Scale(logical.MinimumSize, dpi);
            control.MaximumSize = Scale(logical.MaximumSize, dpi);

            if (logical.Font != null)
            {
                Font font = CreateFontForDpi(logical.Font, dpi);
                control.Font = font;
                _appliedFonts.Add(font);
            }
        }

        foreach (Control child in control.Controls)
        {
            ApplyLogicalControls(child, dpi);
        }
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            foreach (Font font in _appliedFonts)
            {
                font.Dispose();
            }
            _appliedFonts.Clear();
            foreach (Font font in _retiredDpiFonts)
            {
                font.Dispose();
            }
            _retiredDpiFonts.Clear();
            foreach (Font font in _baselineFonts)
            {
                font.Dispose();
            }
            _baselineFonts.Clear();
            _logicalControls.Clear();
        }
        base.Dispose(disposing);
    }

    private static Rectangle Scale(Rectangle value, int dpi)
    {
        return new Rectangle(
            MulDiv(value.X, dpi, 96),
            MulDiv(value.Y, dpi, 96),
            Math.Max(0, MulDiv(value.Width, dpi, 96)),
            Math.Max(0, MulDiv(value.Height, dpi, 96)));
    }

    private static Padding Scale(Padding value, int dpi)
    {
        return new Padding(
            MulDiv(value.Left, dpi, 96),
            MulDiv(value.Top, dpi, 96),
            MulDiv(value.Right, dpi, 96),
            MulDiv(value.Bottom, dpi, 96));
    }

    private static Size Scale(Size value, int dpi)
    {
        return new Size(
            value.Width == 0 ? 0 : Math.Max(1, MulDiv(value.Width, dpi, 96)),
            value.Height == 0 ? 0 : Math.Max(1, MulDiv(value.Height, dpi, 96)));
    }

    private static Font CreateFontForDpi(Font baseline, int dpi)
    {
        float pixelSize = baseline.SizeInPoints * dpi / 72.0F;
        return new Font(
            baseline.FontFamily,
            pixelSize,
            baseline.Style,
            GraphicsUnit.Pixel,
            baseline.GdiCharSet,
            baseline.GdiVerticalFont);
    }

    private static void SuspendTree(Control control)
    {
        control.SuspendLayout();
        foreach (Control child in control.Controls)
        {
            SuspendTree(child);
        }
    }

    private static void ResumeTree(Control control)
    {
        foreach (Control child in control.Controls)
        {
            ResumeTree(child);
        }
        control.ResumeLayout(true);
    }

    private static Font CloneFont(Font source)
    {
        return new Font(
            source.FontFamily,
            source.SizeInPoints,
            source.Style,
            GraphicsUnit.Point,
            source.GdiCharSet,
            source.GdiVerticalFont);
    }

    private sealed class LogicalControl
    {
        internal LogicalControl(
            Rectangle bounds,
            Padding margin,
            Padding padding,
            Size minimumSize,
            Size maximumSize,
            Font font)
        {
            Bounds = bounds;
            Margin = margin;
            Padding = padding;
            MinimumSize = minimumSize;
            MaximumSize = maximumSize;
            Font = font;
        }

        internal Rectangle Bounds { get; }
        internal Padding Margin { get; }
        internal Padding Padding { get; }
        internal Size MinimumSize { get; }
        internal Size MaximumSize { get; }
        internal Font Font { get; }
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct NativeRect
    {
        internal int Left;
        internal int Top;
        internal int Right;
        internal int Bottom;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct NativeSize
    {
        internal int Width;
        internal int Height;
    }

    [DllImport("kernel32.dll")]
    private static extern int MulDiv(
        int number, int numerator, int denominator);

    [DllImport("user32.dll")]
    private static extern uint GetDpiForWindow(IntPtr hwnd);

    [DllImport("user32.dll")]
    private static extern bool GetWindowRect(
        IntPtr hwnd, out NativeRect rectangle);

    [DllImport("user32.dll")]
    private static extern IntPtr SendMessage(
        IntPtr hwnd, int message, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    private static extern bool RedrawWindow(
        IntPtr hwnd,
        IntPtr updateRectangle,
        IntPtr updateRegion,
        uint flags);
}
#nullable restore
