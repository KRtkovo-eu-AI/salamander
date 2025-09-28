// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#nullable enable

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32.SafeHandles;
using JsonViewerControl = EPocalipse.Json.Viewer.JsonViewer;
using ViewerTabs = EPocalipse.Json.Viewer.Tabs;
using JsonViewerResources = EPocalipse.Json.Viewer.Properties.Resources;

namespace OpenSalamander.JsonViewer;

internal static class ViewerHost
{
    private static readonly object s_threadLock = new();
    private static ViewerThread? s_viewerThread;
    private static readonly object s_sessionLock = new();
    private static readonly HashSet<ViewerSession> s_activeSessions = new();
    private static readonly ManualResetEventSlim s_sessionsDrained = new(true);
    private static readonly TimeSpan s_shutdownTimeout = TimeSpan.FromSeconds(5);

    public static int Launch(IntPtr parent, string payload, bool asynchronous)
    {
        if (!ViewCommandPayload.TryParse(payload, asynchronous, out var parsed))
        {
            MessageBox.Show(parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null,
                "Unable to parse parameters provided for the JSON viewer.",
                "JSON Viewer Plugin",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return 1;
        }

        if (asynchronous && parsed.CloseHandle == IntPtr.Zero)
        {
            MessageBox.Show(parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null,
                "The native host did not provide a synchronization handle for the viewer.",
                "JSON Viewer Plugin",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return 1;
        }

        ViewerSession session;
        try
        {
            session = new ViewerSession(parent, parsed, asynchronous);
        }
        catch (Exception ex)
        {
            MessageBox.Show(parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null,
                $"Unable to prepare the JSON viewer session.\n{ex.Message}",
                "JSON Viewer Plugin",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return 1;
        }

        RegisterSession(session);

        ViewerThread? thread;
        try
        {
            thread = EnsureViewerThread();
        }
        catch (Exception ex)
        {
            session.MarkStartupFailed();
            session.Complete();
            MessageBox.Show(session.OwnerWindow,
                $"Unable to initialize the JSON viewer.\n{ex.Message}",
                "JSON Viewer Plugin",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            if (!asynchronous)
            {
                session.WaitForCompletion();
            }
            return 1;
        }

        if (!thread.TryShow(session))
        {
            session.MarkStartupFailed();
            session.Complete();
            MessageBox.Show(session.OwnerWindow,
                "Unable to open the JSON viewer window.",
                "JSON Viewer Plugin",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            if (!asynchronous)
            {
                session.WaitForCompletion();
            }
            return 1;
        }

        if (!asynchronous)
        {
            session.WaitForCompletion();
            return session.StartupSucceeded ? 0 : 1;
        }

        return 0;
    }

    public static int ReleaseSessions(bool forceClose)
    {
        if (!forceClose)
        {
            return HasActiveSessions ? 1 : 0;
        }

        ViewerThread? thread;
        lock (s_threadLock)
        {
            thread = s_viewerThread;
        }

        if (thread is null)
        {
            return 0;
        }

        if (!thread.TryShutdown(s_shutdownTimeout))
        {
            return 1;
        }

        if (!s_sessionsDrained.Wait(s_shutdownTimeout))
        {
            return 1;
        }

        s_sessionsDrained.Set();
        return 0;
    }

    private static ViewerThread EnsureViewerThread()
    {
        lock (s_threadLock)
        {
            s_viewerThread ??= new ViewerThread();
            return s_viewerThread;
        }
    }

    private static void RegisterSession(ViewerSession session)
    {
        lock (s_sessionLock)
        {
            if (s_activeSessions.Count == 0)
            {
                s_sessionsDrained.Reset();
            }

            s_activeSessions.Add(session);
        }
    }

    private static void SessionCompleted(ViewerSession session)
    {
        lock (s_sessionLock)
        {
            if (s_activeSessions.Remove(session) && s_activeSessions.Count == 0)
            {
                s_sessionsDrained.Set();
            }
        }
    }

    private static bool HasActiveSessions
    {
        get
        {
            lock (s_sessionLock)
            {
                return s_activeSessions.Count > 0;
            }
        }
    }

    private static void OnViewerThreadExited(ViewerThread thread)
    {
        lock (s_threadLock)
        {
            if (ReferenceEquals(s_viewerThread, thread))
            {
                s_viewerThread = null;
            }
        }

        s_sessionsDrained.Set();
    }

    private sealed class ViewerThread
    {
        private readonly Thread _thread;
        private readonly AutoResetEvent _ready = new(false);
        private JsonViewerApplicationContext? _context;

        public ViewerThread()
        {
            _thread = new Thread(Run)
            {
                IsBackground = true,
                Name = "JSON Viewer"
            };
            _thread.SetApartmentState(ApartmentState.STA);
            _thread.Start();
            _ready.WaitOne();
        }

        public bool TryShow(ViewerSession session)
        {
            var context = _context;
            if (context is null)
            {
                return false;
            }

            return context.TryShow(session);
        }

        public bool TryShutdown(TimeSpan timeout)
        {
            var context = _context;
            if (context is not null)
            {
                if (!context.TryClose(timeout))
                {
                    return false;
                }
            }

            if (timeout <= TimeSpan.Zero)
            {
                _thread.Join();
                return true;
            }

            return _thread.Join(timeout);
        }

        private void Run()
        {
            try
            {
                using var context = new JsonViewerApplicationContext();
                _context = context;
                _ready.Set();
                Application.Run(context);
            }
            finally
            {
                _context = null;
                _ready.Set();
                OnViewerThreadExited(this);
            }
        }
    }

    private sealed class ViewerSession
    {
        private readonly EventWaitHandle? _closeEvent;
        private readonly ManualResetEventSlim? _completionEvent;
        private int _closeSignaled;
        private int _completionSignaled;

        public ViewerSession(IntPtr parent, ViewCommandPayload payload, bool asynchronous)
        {
            Parent = parent;
            Payload = payload;
            OwnerWindow = parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null;

            if (payload.CloseHandle != IntPtr.Zero)
            {
                _closeEvent = new EventWaitHandle(false, EventResetMode.AutoReset)
                {
                    SafeWaitHandle = new SafeWaitHandle(payload.CloseHandle, ownsHandle: false)
                };
            }

            if (!asynchronous)
            {
                _completionEvent = new ManualResetEventSlim(false);
            }
        }

        public IntPtr Parent { get; }
        public ViewCommandPayload Payload { get; }
        public IWin32Window? OwnerWindow { get; }
        public bool StartupSucceeded { get; private set; } = true;

        public void SignalClosed()
        {
            var handle = _closeEvent;
            if (handle is null)
            {
                return;
            }

            if (Interlocked.Exchange(ref _closeSignaled, 1) != 0)
            {
                return;
            }

            try
            {
                handle.Set();
            }
            catch (ObjectDisposedException)
            {
            }
            catch (InvalidOperationException)
            {
            }
            catch (IOException)
            {
            }
            catch (UnauthorizedAccessException)
            {
            }
            finally
            {
                handle.Dispose();
            }
        }

        public void Complete()
        {
            if (Interlocked.Exchange(ref _completionSignaled, 1) != 0)
            {
                return;
            }

            SignalClosed();
            ViewerHost.SessionCompleted(this);
            _completionEvent?.Set();
        }

        public void WaitForCompletion()
        {
            _completionEvent?.Wait();
        }

        public void MarkStartupFailed()
        {
            StartupSucceeded = false;
        }
    }

    private sealed class JsonViewerApplicationContext : ApplicationContext
    {
        private readonly Control _dispatcher;
        private readonly JsonViewerForm _form;

        public JsonViewerApplicationContext()
        {
            _dispatcher = new Control();
            _dispatcher.CreateControl();

            _form = new JsonViewerForm();
            _form.FormClosed += OnFormClosed;
            MainForm = _form;
        }

        public bool TryShow(ViewerSession session)
        {
            if (!_dispatcher.IsHandleCreated)
            {
                return false;
            }

            _dispatcher.BeginInvoke(new MethodInvoker(() => _form.ShowSession(session)));
            return true;
        }

        public bool TryClose(TimeSpan timeout)
        {
            if (!_dispatcher.IsHandleCreated || _dispatcher.IsDisposed)
            {
                return true;
            }

            using var completion = new ManualResetEventSlim(false);
            try
            {
                _dispatcher.BeginInvoke(new MethodInvoker(() =>
                {
                    try
                    {
                        if (!_form.IsDisposed)
                        {
                            _form.ForceClose();
                        }
                    }
                    finally
                    {
                        completion.Set();
                    }
                }));
            }
            catch (ObjectDisposedException)
            {
                return true;
            }
            catch (InvalidOperationException)
            {
                return true;
            }

            return completion.Wait(timeout);
        }

        private void OnFormClosed(object? sender, FormClosedEventArgs e)
        {
            ExitThread();
        }
    }

    private sealed class ViewCommandPayload
    {
        private ViewCommandPayload(string filePath, string caption, Rectangle bounds, uint showCommand,
                                   bool alwaysOnTop, IntPtr closeHandle)
        {
            FilePath = filePath;
            Caption = caption;
            Bounds = bounds;
            ShowCommand = showCommand;
            AlwaysOnTop = alwaysOnTop;
            CloseHandle = closeHandle;
        }

        public string FilePath { get; }
        public string Caption { get; }
        public Rectangle Bounds { get; }
        public uint ShowCommand { get; }
        public bool AlwaysOnTop { get; }
        public IntPtr CloseHandle { get; }

        public static bool TryParse(string payload, bool asynchronous, out ViewCommandPayload result)
        {
            result = null!;
            if (string.IsNullOrEmpty(payload))
            {
                return false;
            }

            var map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            var parts = payload.Split('|');
            foreach (var part in parts)
            {
                if (string.IsNullOrEmpty(part))
                {
                    continue;
                }

                var kv = part.Split(new[] { '=' }, 2);
                if (kv.Length == 2)
                {
                    map[kv[0]] = kv[1];
                }
            }

            if (!map.TryGetValue("path", out var encodedPath))
            {
                return false;
            }

            string filePath;
            if (TryDecodeBase64(encodedPath, out var decodedPath))
            {
                filePath = decodedPath;
            }
            else
            {
                filePath = encodedPath?.Trim() ?? string.Empty;
            }

            if (string.IsNullOrEmpty(filePath))
            {
                return false;
            }

            string caption = Path.GetFileName(filePath);
            if (map.TryGetValue("caption", out var encodedCaption) && !string.IsNullOrEmpty(encodedCaption))
            {
                if (TryDecodeBase64(encodedCaption, out var decodedCaption) && !string.IsNullOrWhiteSpace(decodedCaption))
                {
                    caption = decodedCaption.Trim();
                }
                else if (!string.IsNullOrEmpty(encodedCaption))
                {
                    caption = encodedCaption.Trim();
                }
            }

            int left = ReadInt(map, "left");
            int top = ReadInt(map, "top");
            int width = Math.Max(ReadInt(map, "width"), 0);
            int height = Math.Max(ReadInt(map, "height"), 0);
            uint showCommand = ReadUInt(map, "show");
            bool alwaysOnTop = ReadBool(map, "ontop");
            IntPtr closeHandle = asynchronous ? ReadHandle(map, "close") : IntPtr.Zero;

            var bounds = new Rectangle(left, top, width, height);
            result = new ViewCommandPayload(filePath, caption, bounds, showCommand, alwaysOnTop, closeHandle);
            return true;
        }

        private static bool TryDecodeBase64(string value, out string decoded)
        {
            decoded = string.Empty;
            var trimmedValue = value?.Trim();
            if (string.IsNullOrEmpty(trimmedValue))
            {
                return false;
            }

            try
            {
                var bytes = Convert.FromBase64String(trimmedValue);
                decoded = Encoding.UTF8.GetString(bytes);
                return true;
            }
            catch (FormatException)
            {
                return false;
            }
            catch (DecoderFallbackException)
            {
                return false;
            }
        }

        private static int ReadInt(IDictionary<string, string> map, string key)
        {
            if (map.TryGetValue(key, out var value) && int.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsed))
            {
                return parsed;
            }
            return 0;
        }

        private static uint ReadUInt(IDictionary<string, string> map, string key)
        {
            if (map.TryGetValue(key, out var value) && uint.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsed))
            {
                return parsed;
            }
            return 0;
        }

        private static bool ReadBool(IDictionary<string, string> map, string key)
        {
            return map.TryGetValue(key, out var value) && value == "1";
        }

        private static IntPtr ReadHandle(IDictionary<string, string> map, string key)
        {
            if (map.TryGetValue(key, out var value) && ulong.TryParse(value, out var parsed))
            {
                return new IntPtr(unchecked((long)parsed));
            }
            return IntPtr.Zero;
        }
    }

    private sealed class JsonViewerForm : Form
    {
        private readonly JsonViewerControl _viewer;
        private ViewerSession? _session;
        private bool _jsonLoaded;
        private IntPtr _ownerRestore;
        private bool _ownerAttached;
        private bool _allowClose;

        public JsonViewerForm()
        {
            Text = "JSON Viewer";
            StartPosition = FormStartPosition.Manual;
            ShowInTaskbar = false;
            MinimizeBox = true;
            MaximizeBox = true;
            KeyPreview = true;
            Icon = JsonViewerResources.JsonViewerIcon;

            _viewer = new JsonViewerControl
            {
                Dock = DockStyle.Fill
            };
            _viewer.PropertyChanged += OnViewerPropertyChanged;
            Controls.Add(_viewer);

            HandleCreated += OnHandleCreated;
            HandleDestroyed += OnHandleDestroyed;
            FormClosing += OnFormClosing;
        }

        public void ShowSession(ViewerSession session)
        {
            if (!IsHandleCreated)
            {
                CreateControl();
            }

            if (_session is not null && !ReferenceEquals(_session, session))
            {
                _session.Complete();
            }

            _session = session;
            _jsonLoaded = false;

            Text = BuildCaption(session.Payload.Caption);
            ApplyOwner(session.Parent);
            ApplyPlacement(session.Payload);

            try
            {
                string json = File.ReadAllText(session.Payload.FilePath);
                _viewer.ShowTab(ViewerTabs.Viewer);
                _viewer.refreshFromString(json);
                _jsonLoaded = true;
            }
            catch (Exception ex)
            {
                session.MarkStartupFailed();
                MessageBox.Show(session.OwnerWindow,
                    $"Unable to open the selected file.\n{ex.Message}",
                    "JSON Viewer Plugin",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                var failedSession = session;
                BeginInvoke(new MethodInvoker(() => HideSession(failedSession)));
                return;
            }

            if (!Visible)
            {
                ShowInTaskbar = true;
                Show();
            }
            else
            {
                if (!ShowInTaskbar)
                {
                    ShowInTaskbar = true;
                }
                Activate();
            }

            NativeMethods.SetForegroundWindow(Handle);
        }

        public void AllowClose()
        {
            _allowClose = true;
        }

        public void ForceClose()
        {
            _allowClose = true;
            var activeSession = _session;
            if (activeSession is not null)
            {
                HideSession(activeSession);
            }

            if (!IsDisposed)
            {
                try
                {
                    Close();
                }
                catch (InvalidOperationException)
                {
                }
            }
        }

        protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
        {
            if (keyData == Keys.Escape)
            {
                var currentSession = _session;
                BeginInvoke(new MethodInvoker(() => HideSession(currentSession)));
                return true;
            }

            return base.ProcessCmdKey(ref msg, keyData);
        }

        private void OnHandleCreated(object? sender, EventArgs e)
        {
            ApplyOwner(_session?.Parent ?? IntPtr.Zero);
        }

        private void OnHandleDestroyed(object? sender, EventArgs e)
        {
            DetachOwner();
        }

        private void OnFormClosing(object? sender, FormClosingEventArgs e)
        {
            if (!_allowClose)
            {
                e.Cancel = true;
                if (!_jsonLoaded)
                {
                    _session?.MarkStartupFailed();
                }

                var closingSession = _session;
                BeginInvoke(new MethodInvoker(() => HideSession(closingSession)));
                return;
            }

            HideSession(null);
        }

        private void ApplyPlacement(ViewCommandPayload payload)
        {
            var bounds = payload.Bounds;
            if (bounds.Width > 0 && bounds.Height > 0)
            {
                Bounds = bounds;
            }
            else if (!Visible)
            {
                StartPosition = FormStartPosition.CenterScreen;
            }

            WindowState = FormWindowState.Normal;
            if (payload.ShowCommand == NativeMethods.SW_SHOWMAXIMIZED)
            {
                WindowState = FormWindowState.Maximized;
            }
            else if (payload.ShowCommand == NativeMethods.SW_SHOWMINIMIZED)
            {
                WindowState = FormWindowState.Minimized;
            }

            TopMost = payload.AlwaysOnTop;
        }

        private void ApplyOwner(IntPtr parent)
        {
            if (!IsHandleCreated)
            {
                return;
            }

            if (_ownerAttached)
            {
                NativeMethods.SetWindowLongPtr(Handle, NativeMethods.GWL_HWNDPARENT, _ownerRestore);
                _ownerRestore = IntPtr.Zero;
                _ownerAttached = false;
            }

            if (parent != IntPtr.Zero)
            {
                _ownerRestore = NativeMethods.SetWindowLongPtr(Handle, NativeMethods.GWL_HWNDPARENT, parent);
                _ownerAttached = true;
            }
        }

        private void DetachOwner()
        {
            if (!IsHandleCreated)
            {
                return;
            }

            if (_ownerAttached)
            {
                NativeMethods.SetWindowLongPtr(Handle, NativeMethods.GWL_HWNDPARENT, _ownerRestore);
                _ownerRestore = IntPtr.Zero;
                _ownerAttached = false;
            }
        }

        private void HideSession(ViewerSession? target)
        {
            if (target is not null && !ReferenceEquals(_session, target))
            {
                return;
            }

            if (_session is null)
            {
                if (Visible)
                {
                    ShowInTaskbar = false;
                    Hide();
                }
                return;
            }

            if (Visible)
            {
                ShowInTaskbar = false;
                Hide();
            }

            _session.Complete();
            _session = null;
            _jsonLoaded = false;
            DetachOwner();
        }

        private static string BuildCaption(string caption)
        {
            if (string.IsNullOrWhiteSpace(caption))
            {
                return "JSON Viewer";
            }
            return string.Format(CultureInfo.CurrentCulture, "{0} - JSON Viewer", caption);
        }

        private static void OnViewerPropertyChanged(object? sender, PropertyChangedEventArgs e)
        {
            // The embedded control raises PropertyChanged without guarding against
            // null handlers, so simply subscribing prevents it from throwing.
        }
    }
}
