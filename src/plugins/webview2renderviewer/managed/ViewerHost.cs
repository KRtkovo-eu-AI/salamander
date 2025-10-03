// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#nullable enable

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Net;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace OpenSalamander.WebView2RenderViewer;

internal static class ViewerHost
{
    private static readonly object s_threadLock = new();
    private static ViewerThread? s_viewerThread;
    private static readonly object s_sessionLock = new();
    private static readonly HashSet<ViewerSession> s_activeSessions = new();
    private static readonly ManualResetEventSlim s_sessionsDrained = new(true);
    private static readonly TimeSpan s_shutdownTimeout = TimeSpan.FromSeconds(5);
    private const string EscapeScript = "(function(){if(window.chrome&&window.chrome.webview&&document){document.addEventListener('keydown',function(ev){if(ev&&ev.key==='Escape'){ev.preventDefault();try{window.chrome.webview.postMessage('escape');}catch(e){}}});}})();";

    public static int Launch(IntPtr parent, string payload, bool asynchronous)
    {
        if (!ViewCommandPayload.TryParse(payload, asynchronous, out var parsed))
        {
            MessageBox.Show(parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null,
                "Unable to parse parameters provided for the render viewer.",
                "WebView2 Render Viewer .NET Plugin",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return 1;
        }

        if (asynchronous && parsed.CloseHandle == IntPtr.Zero)
        {
            MessageBox.Show(parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null,
                "The native host did not provide a synchronization handle for the viewer.",
                "WebView2 Render Viewer .NET Plugin",
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
                $"Unable to prepare the render viewer session.\n{ex.Message}",
                "WebView2 Render Viewer .NET Plugin",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return 1;
        }

        RegisterSession(session);

        ViewerThread thread;
        try
        {
            thread = EnsureViewerThread();
        }
        catch (Exception ex)
        {
            session.MarkStartupFailed();
            session.Complete();
            MessageBox.Show(session.OwnerWindow,
                $"Unable to initialize the render viewer.\n{ex.Message}",
                "WebView2 Render Viewer .NET Plugin",
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
                "Unable to open the render viewer window.",
                "WebView2 Render Viewer .NET Plugin",
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

        if (!thread.TryClose(s_shutdownTimeout))
        {
            return 1;
        }

        if (!s_sessionsDrained.Wait(s_shutdownTimeout))
        {
            return 1;
        }

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
    }

    private sealed class ViewerThread
    {
        private readonly Thread _thread;
        private readonly ManualResetEventSlim _ready = new(false);
        private RenderViewerApplicationContext? _context;

        public ViewerThread()
        {
            _thread = new Thread(Run)
            {
                IsBackground = true,
                Name = "WebView2RenderViewer Thread"
            };
            _thread.SetApartmentState(ApartmentState.STA);
            _thread.Start();
            _ready.Wait();
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

        public bool TryClose(TimeSpan timeout)
        {
            var context = _context;
            if (context is null)
            {
                return true;
            }

            if (!context.TryCloseAll(timeout))
            {
                return false;
            }

            return _thread.Join(timeout);
        }

        private void Run()
        {
            try
            {
                using var context = new RenderViewerApplicationContext();
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
                    SafeWaitHandle = new Microsoft.Win32.SafeHandles.SafeWaitHandle(payload.CloseHandle, ownsHandle: false),
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

        public void MarkStartupFailed()
        {
            StartupSucceeded = false;
        }

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
            SessionCompleted(this);
            _completionEvent?.Set();
        }

        public void WaitForCompletion()
        {
            _completionEvent?.Wait();
        }
    }

    private sealed class RenderViewerApplicationContext : ApplicationContext
    {
        private readonly Control _dispatcher;
        private readonly List<RenderViewerForm> _openForms = new();
        private readonly object _lock = new();

        public RenderViewerApplicationContext()
        {
            _dispatcher = new Control();
            _dispatcher.CreateControl();
        }

        public bool TryShow(ViewerSession session)
        {
            if (_dispatcher.IsDisposed)
            {
                return false;
            }

            try
            {
                _dispatcher.BeginInvoke(new MethodInvoker(() => ShowInternal(session)));
                return true;
            }
            catch (ObjectDisposedException)
            {
                return false;
            }
            catch (InvalidOperationException)
            {
                return false;
            }
        }

        public bool TryCloseAll(TimeSpan timeout)
        {
            using var completion = new ManualResetEventSlim(false);

            try
            {
                _dispatcher.BeginInvoke(new MethodInvoker(() =>
                {
                    foreach (var form in _openForms.ToArray())
                    {
                        form.AllowClose();
                        form.Close();
                    }

                    completion.Set();
                    ExitThread();
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

        private void ShowInternal(ViewerSession session)
        {
            if (_dispatcher.IsDisposed)
            {
                session.MarkStartupFailed();
                session.Complete();
                return;
            }

            var form = new RenderViewerForm();
            form.FormClosed += (_, _) => OnFormClosed(form, session);

            if (!form.TryShow(session))
            {
                form.Dispose();
                session.MarkStartupFailed();
                session.Complete();
                return;
            }

            lock (_lock)
            {
                _openForms.Add(form);
            }
        }

        private void OnFormClosed(RenderViewerForm form, ViewerSession session)
        {
            lock (_lock)
            {
                _openForms.Remove(form);
            }

            session.Complete();
        }
    }

    private sealed class RenderViewerForm : Form
    {
        private const int WM_THEMECHANGED = 0x031A;
        private const int WM_SETTINGCHANGE = 0x001A;

        private ViewerSession? _session;
        private WebView2? _browser;
        private CoreWebView2? _browserCore;
        private bool _allowClose;
        private bool _taskbarStyleApplied;
        private IntPtr _ownerRestore;
        private bool _ownerAttached;
        private bool _handlingThemeUpdate;
        private string? _currentFilePath;
        private DocumentView? _currentView;
        private Uri? _pendingNavigationUri;
        private string? _pendingHtmlContent;

        public RenderViewerForm()
        {
            Text = "WebView2 Render Viewer .NET";
            StartPosition = FormStartPosition.Manual;
            ShowInTaskbar = false;
            MinimizeBox = true;
            MaximizeBox = true;
            KeyPreview = true;
            AutoScaleMode = AutoScaleMode.Dpi;
            ClientSize = new Size(800, 600);
            Icon = ViewerResources.ViewerIcon;

            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer |
                     ControlStyles.ResizeRedraw, true);
            UpdateStyles();

            HandleCreated += OnHandleCreated;
            HandleDestroyed += OnHandleDestroyed;
            FormClosing += OnFormClosing;
            FormClosed += OnFormClosed;

            ThemeHelper.ApplyTheme(this);
        }

        public bool TryShow(ViewerSession session)
        {
            _session = session;
            _allowClose = false;

            Text = BuildCaption(session.Payload.Caption);

            ApplyOwner(session.Parent);
            ApplyPlacement(session.Payload);
            EnsureBrowser();

            ThemeHelper.ApplyTheme(this);
            if (_browser is not null)
            {
                ThemeHelper.ApplyTheme(_browser);
            }

            try
            {
                LoadFile(session.Payload.FilePath);
            }
            catch (Exception ex)
            {
                _session = null;
                session.MarkStartupFailed();
                MessageBox.Show(session.OwnerWindow,
                    $"Unable to open the selected file.\n{ex.Message}",
                    "WebView2 Render Viewer .NET Plugin",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return false;
            }
            ShowInTaskbar = true;
            Show();
            Activate();
            NativeMethods.SetForegroundWindow(Handle);
            return true;
        }

        public void AllowClose()
        {
            _allowClose = true;
        }

        protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
        {
            if (keyData == Keys.Escape)
            {
                Close();
                return true;
            }

            return base.ProcessCmdKey(ref msg, keyData);
        }

        private void EnsureBrowser()
        {
            if (_browser is not null)
            {
                return;
            }

            var viewer = new WebView2
            {
                Dock = DockStyle.Fill,
                AllowExternalDrop = false,
            };

            viewer.CoreWebView2InitializationCompleted += OnBrowserInitializationCompleted;
            viewer.NavigationCompleted += OnBrowserNavigationCompleted;

            Controls.Add(viewer);
            ThemeHelper.ApplyTheme(viewer);

            _browser = viewer;

            try
            {
                _ = viewer.EnsureCoreWebView2Async();
            }
            catch (Exception ex)
            {
                HandleBrowserInitializationFailure(ex);
            }
        }

        private void LoadFile(string path)
        {
            _currentFilePath = path;

            string caption = _session?.Payload.Caption ?? string.Empty;
            if (string.IsNullOrWhiteSpace(caption))
            {
                caption = Path.GetFileName(path) ?? string.Empty;
            }

            _currentView = DocumentView.Create(path, caption);
            RenderCurrentDocument();
        }

        private void OnBrowserInitializationCompleted(object? sender, CoreWebView2InitializationCompletedEventArgs e)
        {
            if (sender is not WebView2 browser)
            {
                return;
            }

            if (!e.IsSuccess)
            {
                HandleBrowserInitializationFailure(e.InitializationException ?? new InvalidOperationException("WebView2 initialization failed."));
                return;
            }

            if (browser.CoreWebView2 is CoreWebView2 core)
            {
                core.Settings.IsStatusBarEnabled = false;
                core.Settings.AreDefaultContextMenusEnabled = true;
                core.Settings.AreDefaultScriptDialogsEnabled = true;
                core.Settings.AreDevToolsEnabled = true;

                if (_browserCore is not null)
                {
                    _browserCore.WebMessageReceived -= OnBrowserWebMessageReceived;
                }

                _browserCore = core;
                _browserCore.WebMessageReceived += OnBrowserWebMessageReceived;
                _ = _browserCore.AddScriptToExecuteOnDocumentCreatedAsync(EscapeScript);
            }

            ThemeHelper.ApplyTheme(browser);

            if (browser.CoreWebView2 is null)
            {
                return;
            }

            if (_pendingHtmlContent is not null)
            {
                browser.CoreWebView2.NavigateToString(_pendingHtmlContent);
                _pendingHtmlContent = null;
            }
            else if (_pendingNavigationUri is not null)
            {
                browser.CoreWebView2.Navigate(_pendingNavigationUri.AbsoluteUri);
                _pendingNavigationUri = null;
            }
            else if (_currentView is not null)
            {
                RenderCurrentDocument();
            }
        }

        private void OnBrowserNavigationCompleted(object? sender, CoreWebView2NavigationCompletedEventArgs e)
        {
            if (sender is WebView2 browser)
            {
                ThemeHelper.ApplyTheme(browser);
            }
        }

        private void HandleBrowserInitializationFailure(Exception exception)
        {
            _session?.MarkStartupFailed();
            MessageBox.Show(this,
                $"Unable to initialize the embedded browser.\n{exception.Message}",
                "WebView2 Render Viewer .NET Plugin",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            BeginInvoke(new MethodInvoker(() =>
            {
                _allowClose = true;
                Close();
            }));
        }

        private void OnHandleCreated(object? sender, EventArgs e)
        {
            ApplyOwner(_session?.Parent ?? IntPtr.Zero);
            EnsureTaskbarVisibility();
        }

        private void OnHandleDestroyed(object? sender, EventArgs e)
        {
            DetachOwner();
            _taskbarStyleApplied = false;
        }

        private void OnFormClosing(object? sender, FormClosingEventArgs e)
        {
            if (!_allowClose)
            {
                _allowClose = true;
            }

            if (_browserCore is not null)
            {
                _browserCore.WebMessageReceived -= OnBrowserWebMessageReceived;
                _browserCore = null;
            }

            _session?.SignalClosed();
        }

        private void OnFormClosed(object? sender, FormClosedEventArgs e)
        {
            var owner = _session?.Parent ?? IntPtr.Zero;
            _session = null;

            if (owner != IntPtr.Zero)
            {
                NativeMethods.SetForegroundWindow(owner);
            }
        }

        protected override void OnSystemColorsChanged(EventArgs e)
        {
            base.OnSystemColorsChanged(e);
            HandleThemeChanged();
        }

        protected override void WndProc(ref Message m)
        {
            base.WndProc(ref m);

            if (m.Msg == WM_THEMECHANGED || m.Msg == WM_SETTINGCHANGE)
            {
                HandleThemeChanged();
            }
        }

        private void HandleThemeChanged()
        {
            if (_handlingThemeUpdate)
            {
                return;
            }

            _handlingThemeUpdate = true;

            try
            {
                ThemeHelper.ApplyTheme(this);

                if (_browser is not null)
                {
                    ThemeHelper.ApplyTheme(_browser);
                }

                if (_currentFilePath is not null && _currentView is not null && _currentView.Kind == DocumentViewKind.Html)
                {
                    string caption = _currentView.Caption;
                    _currentView = DocumentView.Create(_currentFilePath, caption);
                }

                RenderCurrentDocument();
            }
            finally
            {
                _handlingThemeUpdate = false;
            }
        }

        private void RenderCurrentDocument()
        {
            if (_browser is null)
            {
                return;
            }

            if (_currentView is null)
            {
                return;
            }

            Text = BuildCaption(_currentView.Caption);

            var core = _browser.CoreWebView2;
            if (core is null)
            {
                if (_currentView.Kind == DocumentViewKind.Navigate)
                {
                    _pendingNavigationUri = _currentView.NavigateUri;
                    _pendingHtmlContent = null;
                }
                else
                {
                    _pendingHtmlContent = _currentView.HtmlContent;
                    _pendingNavigationUri = null;
                }
                return;
            }

            if (_currentView.Kind == DocumentViewKind.Navigate && _currentView.NavigateUri is Uri uri)
            {
                _pendingNavigationUri = null;
                _pendingHtmlContent = null;
                core.Navigate(uri.AbsoluteUri);
            }
            else if (_currentView.Kind == DocumentViewKind.Html)
            {
                string html = _currentView.HtmlContent ?? string.Empty;
                _pendingNavigationUri = null;
                _pendingHtmlContent = null;
                core.NavigateToString(html);
            }

            ThemeHelper.ApplyTheme(_browser);
        }

        private void ApplyPlacement(ViewCommandPayload payload)
        {
            Rectangle bounds = payload.Bounds;
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

        private void EnsureTaskbarVisibility()
        {
            if (!IsHandleCreated || _taskbarStyleApplied)
            {
                return;
            }

            int style = unchecked((int)NativeMethods.GetWindowLongPtr(Handle, NativeMethods.GWL_EXSTYLE).ToInt64());
            int updated = (style & ~NativeMethods.WS_EX_TOOLWINDOW) | NativeMethods.WS_EX_APPWINDOW;
            if (updated != style)
            {
                NativeMethods.SetWindowLongPtr(Handle, NativeMethods.GWL_EXSTYLE, new IntPtr(updated));
                NativeMethods.SetWindowPos(
                    Handle,
                    IntPtr.Zero,
                    0,
                    0,
                    0,
                    0,
                    NativeMethods.SWP_NOMOVE |
                    NativeMethods.SWP_NOSIZE |
                    NativeMethods.SWP_NOZORDER |
                    NativeMethods.SWP_NOACTIVATE |
                    NativeMethods.SWP_FRAMECHANGED |
                    NativeMethods.SWP_NOOWNERZORDER);
            }

            _taskbarStyleApplied = true;
        }

        private static string BuildCaption(string caption)
        {
            if (string.IsNullOrWhiteSpace(caption))
            {
                return "WebView2 Render Viewer .NET";
            }

            return string.Format(CultureInfo.CurrentCulture, "{0} - WebView2 Render Viewer .NET", caption);
        }

        private void OnBrowserWebMessageReceived(object? sender, CoreWebView2WebMessageReceivedEventArgs e)
        {
            if (e is null)
            {
                return;
            }

            if (!string.Equals(e.TryGetWebMessageAsString(), "escape", StringComparison.Ordinal))
            {
                return;
            }

            BeginInvoke(new MethodInvoker(() =>
            {
                if (!IsDisposed)
                {
                    Close();
                }
            }));
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
            if (map.TryGetValue(key, out var value) && ulong.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsed))
            {
                return new IntPtr(unchecked((long)parsed));
            }
            return IntPtr.Zero;
        }
    }

    private enum DocumentViewKind
    {
        Navigate,
        Html,
    }

    private sealed class DocumentView
    {
        public DocumentView(DocumentViewKind kind, string caption, Uri? navigateUri, string? htmlContent)
        {
            Kind = kind;
            Caption = caption;
            NavigateUri = navigateUri;
            HtmlContent = htmlContent;
        }

        public DocumentViewKind Kind { get; }
        public string Caption { get; }
        public Uri? NavigateUri { get; }
        public string? HtmlContent { get; }

        public static DocumentView Create(string path, string caption)
        {
            string extension = Path.GetExtension(path)?.TrimStart('.')?.ToLowerInvariant() ?? string.Empty;

            if (FileViewHelper.IsMarkdown(extension))
            {
                string markdown = File.ReadAllText(path);
                string html = MarkdownRenderer.BuildHtml(markdown, path, caption);
                return new DocumentView(DocumentViewKind.Html, caption, null, html);
            }

            var uri = new Uri(Path.GetFullPath(path));
            return new DocumentView(DocumentViewKind.Navigate, caption, uri, null);
        }
    }

    private static class FileViewHelper
    {
        private static readonly HashSet<string> s_markdownExtensions = new(StringComparer.OrdinalIgnoreCase)
        {
            "md", "markdown", "mdown", "mkd", "mdx",
        };

        public static bool IsMarkdown(string extension)
        {
            return s_markdownExtensions.Contains(extension);
        }
    }

    private static class MarkdownRenderer
    {
        private const string MarkdownContainerId = "markdown-root";

        public static string BuildHtml(string markdown, string sourcePath, string caption)
        {
            string markdownText = markdown ?? string.Empty;
            ThemeHelper.ThemePalette? palette = ThemeHelper.TryGetPalette(out var value) ? value : null;

            string? baseUrl = null;
            try
            {
                if (!string.IsNullOrEmpty(sourcePath))
                {
                    var directory = Path.GetDirectoryName(sourcePath);
                    if (!string.IsNullOrEmpty(directory))
                    {
                        var absolute = Path.GetFullPath(directory);
                        if (!absolute.EndsWith(Path.DirectorySeparatorChar.ToString(), StringComparison.Ordinal))
                        {
                            absolute += Path.DirectorySeparatorChar;
                        }
                        baseUrl = new Uri(absolute).AbsoluteUri;
                    }
                }
            }
            catch (UriFormatException)
            {
                baseUrl = null;
            }

            string markedScript = ViewerResources.MarkedScript.Replace("</script>", "<\\/script>");
            string encodedMarkdown = Convert.ToBase64String(Encoding.UTF8.GetBytes(markdownText));

            var builder = new StringBuilder();
            builder.AppendLine("<!DOCTYPE html>");
            builder.Append("<html><head><meta charset=\"utf-8\"/>");
            if (!string.IsNullOrEmpty(baseUrl))
            {
                builder.Append("<base href=\"")
                    .Append(WebUtility.HtmlEncode(baseUrl))
                    .Append("\"/>");
            }
            if (palette.HasValue)
            {
                string scheme = palette.Value.IsDark ? "dark" : "light";
                builder.Append("<meta name=\"color-scheme\" content=\"")
                    .Append(scheme)
                    .Append("\"/>");
            }
            builder.Append("<title>")
                .Append(WebUtility.HtmlEncode(string.IsNullOrWhiteSpace(caption) ? "WebView2 Render Viewer .NET" : caption))
                .Append("</title>");
            builder.Append("<script>").Append(EscapeScript).Append("</script>");
            builder.Append("<script>").Append(markedScript).Append("</script>");
            builder.Append("<style>");
            AppendStyles(builder, palette);
            builder.Append("</style>");
            builder.Append("</head><body>");
            builder.Append("<div id=\"")
                .Append(MarkdownContainerId)
                .Append("\" class=\"markdown-body\"></div>");
            builder.Append("<script>(()=>{try{const data=\"")
                .Append(encodedMarkdown)
                .Append("\";const bytes=Uint8Array.from(atob(data),c=>c.charCodeAt(0));");
            builder.Append("const decoder=typeof TextDecoder==='function'?new TextDecoder('utf-8'):{decode:function(arr){var result='';for(var i=0;i<arr.length;i++){result+=String.fromCharCode(arr[i]);}return decodeURIComponent(escape(result));}};");
            builder.Append("const markdown=decoder.decode(bytes);if(typeof marked==='function'){marked.setOptions({mangle:false,headerIds:false});const target=document.getElementById('\"")
                .Append(MarkdownContainerId)
                .Append("\"');if(target){target.innerHTML=marked.parse(markdown);}}}");
            builder.Append("catch(err){document.body.innerHTML='<pre style=\\"white-space:pre-wrap;font-family:Consolas,\\\\'Courier New\\\\',monospace;\\">'+(err&&err.message?err.message:err)+'</pre>';}})();</script>");
            builder.Append("</body></html>");
            return builder.ToString();
        }

        private static void AppendStyles(StringBuilder builder, ThemeHelper.ThemePalette? palette)
        {
            Color background = palette?.Background ?? SystemColors.Window;
            Color foreground = palette?.Foreground ?? SystemColors.WindowText;
            Color accent = palette?.Accent ?? SystemColors.HotTrack;
            Color border = palette?.ControlBorder ?? ControlPaint.Dark(background);
            Color codeBackground = palette?.ControlBackground ?? ControlPaint.Light(background);

            builder.Append("html,body{margin:0;padding:16px;font-family:'Segoe UI',SegoeUI,Helvetica,Arial,sans-serif;font-size:14px;line-height:1.6;background-color:")
                .Append(ColorToCss(background))
                .Append(";color:")
                .Append(ColorToCss(foreground))
                .Append(";}");
            builder.Append("h1,h2,h3,h4,h5,h6{color:")
                .Append(ColorToCss(foreground))
                .Append(";margin-top:1.2em;}");
            builder.Append("a{color:")
                .Append(ColorToCss(accent))
                .Append(";text-decoration:none;}a:hover{text-decoration:underline;}");
            builder.Append("code{font-family:'Consolas','Courier New',monospace;background-color:")
                .Append(ColorToCss(codeBackground))
                .Append(";padding:2px 4px;border-radius:4px;}");
            builder.Append("pre{overflow:auto;padding:12px;border-radius:6px;background-color:")
                .Append(ColorToCss(codeBackground))
                .Append(";border:1px solid ")
                .Append(ColorToCss(border))
                .Append(";}");
            builder.Append("table{border-collapse:collapse;margin:1em 0;width:100%;}th,td{border:1px solid ")
                .Append(ColorToCss(border))
                .Append(";padding:8px;text-align:left;}");
            builder.Append("blockquote{border-left:4px solid ")
                .Append(ColorToCss(accent))
                .Append(";margin:1em 0;padding:0.5em 1em;color:")
                .Append(ColorToCss(ControlPaint.Dark(foreground)))
                .Append(";background-color:")
                .Append(ColorToCss(ControlPaint.LightLight(background)))
                .Append(";}");
            builder.Append("img,video,iframe{max-width:100%;height:auto;display:block;margin:0.5em 0;}");
            builder.Append("hr{border:0;border-top:1px solid ")
                .Append(ColorToCss(border))
                .Append(";margin:2em 0;}");
            builder.Append("ul,ol{margin:0 0 1em 1.5em;}");
        }

        private static string ColorToCss(Color color)
        {
            return string.Format(CultureInfo.InvariantCulture, "#{0:X2}{1:X2}{2:X2}", color.R, color.G, color.B);
        }
    }
}
