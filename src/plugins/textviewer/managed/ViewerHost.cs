// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#nullable enable

using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;
using Orionsoft.PrismSharp.Highlighters.Abstract;
using Orionsoft.PrismSharp.Themes;
using Orionsoft.PrismSharp.Tokenizing;

namespace OpenSalamander.TextViewer;

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
                "Unable to parse parameters provided for the text viewer.",
                "Text Viewer .NET Plugin",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return 1;
        }

        if (asynchronous && parsed.CloseHandle == IntPtr.Zero)
        {
            MessageBox.Show(parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null,
                "The native host did not provide a synchronization handle for the viewer.",
                "Text Viewer .NET Plugin",
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
                $"Unable to prepare the text viewer session.\n{ex.Message}",
                "Text Viewer .NET Plugin",
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
                $"Unable to initialize the text viewer.\n{ex.Message}",
                "Text Viewer .NET Plugin",
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
                "Unable to open the text viewer window.",
                "Text Viewer .NET Plugin",
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
        private readonly AutoResetEvent _ready = new(false);
        private TextViewerApplicationContext? _context;

        public ViewerThread()
        {
            _thread = new Thread(Run)
            {
                IsBackground = true,
                Name = "Text Viewer",
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

        public bool TryClose(TimeSpan timeout)
        {
            var context = _context;
            if (context is not null)
            {
                if (!context.TryCloseAll(timeout))
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
                using var context = new TextViewerApplicationContext();
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

        public void MarkStartupFailed()
        {
            StartupSucceeded = false;
        }
    }

    private sealed class TextViewerApplicationContext : ApplicationContext
    {
        private readonly Control _dispatcher;
        private readonly List<TextViewerForm> _openForms = new();
        private readonly object _lock = new();

        public TextViewerApplicationContext()
        {
            _dispatcher = new Control();
            _dispatcher.HandleCreated += OnDispatcherHandleCreated;
            _dispatcher.HandleDestroyed += OnDispatcherHandleDestroyed;
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

            var form = new TextViewerForm();
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

        private void OnDispatcherHandleCreated(object? sender, EventArgs e)
        {
        }

        private void OnDispatcherHandleDestroyed(object? sender, EventArgs e)
        {
        }

        private void OnFormClosed(TextViewerForm form, ViewerSession session)
        {
            lock (_lock)
            {
                _openForms.Remove(form);
            }

            session.Complete();
        }
    }

    private sealed class TextViewerForm : Form
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
        private string? _currentDocumentText;
        private string? _currentDocumentLanguage;
        private string? _currentDocumentCaption;
        private bool _handlingThemeUpdate;
        private string? _pendingDocumentHtml;

        public TextViewerForm()
        {
            Text = "Text Viewer .NET";
            StartPosition = FormStartPosition.Manual;
            ShowInTaskbar = false;
            MinimizeBox = true;
            MaximizeBox = true;
            KeyPreview = true;
            AutoScaleMode = AutoScaleMode.Dpi;
            ClientSize = new Size(800, 600);
            Icon = ViewerResources.ViewerIcon;

            HandleCreated += OnHandleCreated;
            HandleDestroyed += OnHandleDestroyed;
            FormClosing += OnFormClosing;

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

            try
            {
                LoadFile(session.Payload.FilePath);
            }
            catch (Exception ex)
            {
                session.MarkStartupFailed();
                MessageBox.Show(session.OwnerWindow,
                    $"Unable to open the selected file.\n{ex.Message}",
                    "Text Viewer .NET Plugin",
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
            if (_browser is null)
            {
                return;
            }

            string text = File.ReadAllText(path);
            string extension = LanguageGuesser.FromFileName(path);
            string caption = Path.GetFileName(path);

            _currentDocumentText = text;
            _currentDocumentLanguage = extension;
            _currentDocumentCaption = caption;

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

            var core = browser.CoreWebView2;
            if (core is not null)
            {
                core.Settings.IsStatusBarEnabled = false;
                core.Settings.AreDefaultContextMenusEnabled = true;
                core.Settings.AreDefaultScriptDialogsEnabled = true;
                core.Settings.AreDevToolsEnabled = true;
            }

            if (_browserCore is not null)
            {
                _browserCore.WebMessageReceived -= OnBrowserWebMessageReceived;
            }

            _browserCore = browser.CoreWebView2;
            if (_browserCore is not null)
            {
                _browserCore.WebMessageReceived += OnBrowserWebMessageReceived;
            }

            ThemeHelper.ApplyTheme(browser);

            if (_pendingDocumentHtml is not null && core is not null)
            {
                core.NavigateToString(_pendingDocumentHtml);
                _pendingDocumentHtml = null;
            }
        }

        private void OnBrowserNavigationCompleted(object? sender, CoreWebView2NavigationCompletedEventArgs e)
        {
            if (sender is WebView2 browser)
            {
                ThemeHelper.ApplyTheme(browser);
            }
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

        private void HandleBrowserInitializationFailure(Exception exception)
        {
            _session?.MarkStartupFailed();
            MessageBox.Show(this,
                $"Unable to initialize the embedded browser.\n{exception.Message}",
                "Text Viewer .NET Plugin",
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

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
            }

            base.Dispose(disposing);
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

                RenderCurrentDocument();
            }
            finally
            {
                _handlingThemeUpdate = false;
            }
        }

        private void RenderCurrentDocument()
        {
            if (_currentDocumentText is null)
            {
                return;
            }

            string language = _currentDocumentLanguage ?? string.Empty;
            string caption = _currentDocumentCaption ?? string.Empty;

            string html = PrismSharpRenderer.BuildDocument(_currentDocumentText, language, caption);
            _pendingDocumentHtml = html;

            if (_browser?.CoreWebView2 is CoreWebView2 core)
            {
                core.NavigateToString(html);
                _pendingDocumentHtml = null;
                ThemeHelper.ApplyTheme(_browser);
            }
        }

        private static string BuildCaption(string caption)
        {
            if (string.IsNullOrWhiteSpace(caption))
            {
                return "Text Viewer .NET";
            }

            return string.Format(CultureInfo.CurrentCulture, "{0} - Text Viewer .NET", caption);
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

    private static class LanguageGuesser
    {
        public static string FromFileName(string filePath)
        {
            var extension = Path.GetExtension(filePath);
            if (string.IsNullOrEmpty(extension))
            {
                return string.Empty;
            }

            return extension.TrimStart('.').ToLowerInvariant();
        }
    }

    private static class PrismSharpRenderer
    {
        private static readonly Lazy<Tokenizer> s_tokenizer = new(() => new Tokenizer());
        private static readonly ConcurrentDictionary<ThemeNames, Theme> s_themeCache = new();
        private static readonly object s_highlightLock = new();
        private static readonly IReadOnlyDictionary<string, string> s_extensionOverrides = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["axaml"] = "xml",
            ["cmd"] = "batch",
            ["config"] = "xml",
            ["csproj"] = "xml",
            ["cxx"] = "cpp",
            ["fsproj"] = "xml",
            ["h"] = "c",
            ["hh"] = "cpp",
            ["hpp"] = "cpp",
            ["hxx"] = "cpp",
            ["htm"] = "html",
            ["jsonc"] = "json",
            ["json5"] = "json",
            ["markdown"] = "md",
            ["nuspec"] = "xml",
            ["plist"] = "xml",
            ["props"] = "xml",
            ["ps1"] = "powershell",
            ["psd1"] = "powershell",
            ["psm1"] = "powershell",
            ["storyboard"] = "xml",
            ["targets"] = "xml",
            ["vcxproj"] = "xml",
            ["vcproj"] = "xml",
            ["vbproj"] = "xml",
            ["xaml"] = "xml",
            ["xlf"] = "xml",
            ["yml"] = "yaml"
        };

        public static string BuildDocument(string text, string extension, string? caption)
        {
            var content = HighlightOrFallback(text, extension);
            return WrapDocument(content, caption);
        }

        private static RenderedContent HighlightOrFallback(string text, string extension)
        {
            try
            {
                var theme = GetTheme();
                if (theme is null)
                {
                    return new RenderedContent(BuildPlainTextHtml(text), null);
                }

                foreach (var language in GetLanguageCandidates(extension))
                {
                    try
                    {
                        var result = Highlight(text, language, theme);
                        if (!string.IsNullOrEmpty(result.Html))
                        {
                            return new RenderedContent(result.Html, result.PreStyle);
                        }
                    }
                    catch (FileNotFoundException)
                    {
                    }
                    catch (DirectoryNotFoundException)
                    {
                    }
                    catch (KeyNotFoundException)
                    {
                    }
                    catch
                    {
                        break;
                    }
                }
            }
            catch
            {
            }

            return new RenderedContent(BuildPlainTextHtml(text), null);
        }

        private static HighlightResult Highlight(string text, string language, Theme theme)
        {
            if (string.IsNullOrEmpty(language))
            {
                return default;
            }

            var tokenizer = s_tokenizer.Value;
            lock (s_highlightLock)
            {
                var highlighter = new InlineStyleHtmlHighlighter(tokenizer, theme);
                return highlighter.Highlight(text, language);
            }
        }

        private static IEnumerable<string> GetLanguageCandidates(string extension)
        {
            if (string.IsNullOrEmpty(extension))
            {
                return Array.Empty<string>();
            }

            var candidates = new List<string>();

            if (s_extensionOverrides.TryGetValue(extension, out var mapped))
            {
                candidates.Add(mapped);
            }

            candidates.Add(extension);

            if (extension.EndsWith("config", StringComparison.OrdinalIgnoreCase))
            {
                candidates.Add("xml");
            }

            if (extension.EndsWith("proj", StringComparison.OrdinalIgnoreCase))
            {
                candidates.Add("xml");
            }

            if (extension.EndsWith("json", StringComparison.OrdinalIgnoreCase))
            {
                candidates.Add("json");
            }

            if (extension.EndsWith("yaml", StringComparison.OrdinalIgnoreCase))
            {
                candidates.Add("yaml");
            }

            if (extension.EndsWith("md", StringComparison.OrdinalIgnoreCase))
            {
                candidates.Add("markdown");
            }

            return candidates
                .Where(candidate => !string.IsNullOrEmpty(candidate))
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToArray();
        }

        private static Theme? GetTheme()
        {
            try
            {
                var name = SelectThemeName();
                return s_themeCache.GetOrAdd(name, static themeName => Theme.Load(themeName));
            }
            catch
            {
                return null;
            }
        }

        private static ThemeNames SelectThemeName()
        {
            if (ThemeHelper.TryGetPalette(out var palette) && palette.IsDark)
            {
                return ThemeNames.OneDark;
            }

            return ThemeNames.Ghcolors;
        }

        private static string WrapDocument(RenderedContent content, string? caption)
        {
            ThemeHelper.ThemePalette? palette = ThemeHelper.TryGetPalette(out var value) ? value : null;

            var builder = new StringBuilder();
            builder.AppendLine("<!DOCTYPE html>");
            builder.Append("<html><head><meta charset=\"utf-8\"/><meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"/>");
            if (palette.HasValue)
            {
                string scheme = palette.Value.IsDark ? "dark" : "light";
                string fallback = palette.Value.IsDark ? "light" : "dark";
                builder.Append("<meta name=\"color-scheme\" content=\"")
                    .Append(scheme)
                    .Append(' ')
                    .Append(fallback)
                    .Append("\"/>");
                builder.Append("<script>(function(){var doc=document.documentElement;if(doc){doc.setAttribute('data-theme','")
                    .Append(scheme)
                    .Append("');doc.style.colorScheme='")
                    .Append(scheme)
                    .Append("';}})();</script>");
            }
            builder.Append("<script>(function(){if(window.chrome&&window.chrome.webview&&document){document.addEventListener('keydown',function(ev){if(ev&&ev.key==='Escape'){ev.preventDefault();try{window.chrome.webview.postMessage('escape');}catch(e){}}});}})();</script>");
            builder.Append("<title>");
            builder.Append(WebUtility.HtmlEncode(string.IsNullOrWhiteSpace(caption) ? "Text Viewer .NET" : caption));
            builder.Append("</title>");
            builder.Append("<style>");
            AppendBaseStyles(builder, palette, content.PreStyle);
            builder.Append("</style>");
            builder.Append("</head><body><div class=\"code-container\">");
            builder.Append(content.Html);
            builder.Append("</div></body></html>");
            return builder.ToString();
        }

        private static void AppendBaseStyles(StringBuilder builder, ThemeHelper.ThemePalette? palette, string? preStyle)
        {
            if (palette.HasValue)
            {
                var colors = palette.Value;
                builder.Append("body{margin:0;background-color:")
                    .Append(ToCssColor(colors.Background))
                    .Append(";font-family:'Segoe UI',sans-serif;color:")
                    .Append(ToCssColor(colors.Foreground))
                    .Append(";}");
                builder.Append(".code-container{padding:16px;background-color:")
                    .Append(ToCssColor(colors.Background))
                    .Append(";}");
                builder.Append("pre{margin:0;font-family:'Consolas','Courier New',monospace;font-size:13px;white-space:pre;")
                    .Append("background-color:")
                    .Append(ToCssColor(colors.ControlBackground))
                    .Append(";color:")
                    .Append(ToCssColor(colors.InputForeground))
                    .Append(';');
                if (!string.IsNullOrEmpty(preStyle))
                {
                    builder.Append(preStyle);
                }
                builder.Append("}");
                builder.Append("code{color:inherit;}");
                builder.Append("a{color:")
                    .Append(ToCssColor(colors.Accent))
                    .Append(";}");
                builder.Append("::selection{background-color:")
                    .Append(ToCssColor(colors.HighlightBackground))
                    .Append(";color:")
                    .Append(ToCssColor(colors.HighlightForeground))
                    .Append(";}");
            }
            else
            {
                builder.Append("body{margin:0;background-color:#f6f8fa;font-family:'Segoe UI',sans-serif;color:#24292e;}");
                builder.Append(".code-container{padding:16px;}");
                builder.Append("pre{margin:0;font-family:'Consolas','Courier New',monospace;font-size:13px;white-space:pre;");
                if (!string.IsNullOrEmpty(preStyle))
                {
                    builder.Append(preStyle);
                }
                builder.Append("}");
            }
        }

        private static string ToCssColor(Color color)
        {
            return $"#{color.R:X2}{color.G:X2}{color.B:X2}";
        }

        private static string BuildPlainTextHtml(string text)
        {
            var encoded = WebUtility.HtmlEncode(text ?? string.Empty);
            return $"<pre><code>{encoded}</code></pre>";
        }

        private readonly struct RenderedContent
        {
            public RenderedContent(string html, string? preStyle)
            {
                Html = html;
                PreStyle = preStyle;
            }

            public string Html { get; }
            public string? PreStyle { get; }
        }

        private readonly struct HighlightResult
        {
            public HighlightResult(string html, string? preStyle)
            {
                Html = html;
                PreStyle = preStyle;
            }

            public string Html { get; }
            public string? PreStyle { get; }
        }

        private sealed class InlineStyleHtmlHighlighter : AbstractHighlighter<HighlightResult>
        {
            private readonly StringBuilder _builder = new();
            private readonly Stack<bool> _openTags = new();
            private string? _documentStyle;

            public InlineStyleHtmlHighlighter(Tokenizer tokenizer, Theme theme)
            {
                Construct(tokenizer, theme);
            }

            protected override ThemeStyle BeginDocument(string language, ThemeStyle docStyle)
            {
                _builder.Clear();
                _openTags.Clear();
                _documentStyle = BuildCss(docStyle, null);
                _builder.Append("<pre><code");
                if (!string.IsNullOrEmpty(language))
                {
                    _builder.Append(" class=\"language-")
                        .Append(WebUtility.HtmlEncode(language))
                        .Append("\"");
                }
                _builder.Append(">");
                return docStyle;
            }

            protected override void EndDocument()
            {
                _builder.Append("</code></pre>");
                Result = new HighlightResult(_builder.ToString(), _documentStyle);
            }

            protected override ThemeStyle BeginContainer(Token token, ThemeStyle style, ThemeStyle parentStyle)
            {
                var effective = CombineStyles(style, parentStyle) ?? parentStyle ?? style ?? new ThemeStyle();
                var css = BuildCss(effective, parentStyle);
                if (!string.IsNullOrEmpty(css))
                {
                    _builder.Append("<span style=\"")
                        .Append(css)
                        .Append("\">");
                    _openTags.Push(true);
                }
                else if (style is not null)
                {
                    _builder.Append("<span>");
                    _openTags.Push(true);
                }
                else
                {
                    _openTags.Push(false);
                }

                return effective;
            }

            protected override void EndContainer()
            {
                if (_openTags.Count > 0 && _openTags.Pop())
                {
                    _builder.Append("</span>");
                }
            }

            protected override void AddSpan(string text, Token token, ThemeStyle style, ThemeStyle parentStyle)
            {
                if (string.IsNullOrEmpty(text))
                {
                    return;
                }

                var effective = CombineStyles(style, parentStyle);
                var css = BuildCss(effective, parentStyle);
                if (!string.IsNullOrEmpty(css))
                {
                    _builder.Append("<span style=\"")
                        .Append(css)
                        .Append("\">")
                        .Append(WebUtility.HtmlEncode(text))
                        .Append("</span>");
                }
                else if (style is not null)
                {
                    _builder.Append("<span>")
                        .Append(WebUtility.HtmlEncode(text))
                        .Append("</span>");
                }
                else
                {
                    _builder.Append(WebUtility.HtmlEncode(text));
                }
            }

            private static ThemeStyle? CombineStyles(ThemeStyle? style, ThemeStyle? parentStyle)
            {
                if (style is null)
                {
                    return parentStyle;
                }

                if (parentStyle is null)
                {
                    return style;
                }

                return style.MergeWith(parentStyle);
            }

            private static string? BuildCss(ThemeStyle? style, ThemeStyle? parentStyle)
            {
                if (style is null)
                {
                    return null;
                }

                var builder = new StringBuilder();

                AppendColor(builder, style.Color, parentStyle?.Color, "color");
                AppendColor(builder, style.Background, parentStyle?.Background, "background-color");
                AppendFont(builder, style.Bold, parentStyle?.Bold, "font-weight", "bold", "normal");
                AppendFont(builder, style.Italic, parentStyle?.Italic, "font-style", "italic", "normal");
                AppendFont(builder, style.Underline, parentStyle?.Underline, "text-decoration", "underline", "none");

                return builder.Length == 0 ? null : builder.ToString();
            }

            private static void AppendColor(StringBuilder builder, RgbaColor? color, RgbaColor? parent, string property)
            {
                if (color is null)
                {
                    return;
                }

                if (parent is not null && ColorsEqual(color!, parent))
                {
                    return;
                }

                builder.Append(property)
                    .Append(':')
                    .Append(color.ToColorString())
                    .Append(';');
            }

            private static void AppendFont(StringBuilder builder, bool? value, bool? parentValue, string property, string enabledValue, string disabledValue)
            {
                if (!value.HasValue)
                {
                    return;
                }

                if (value.Value)
                {
                    if (parentValue != true)
                    {
                        builder.Append(property)
                            .Append(':')
                            .Append(enabledValue)
                            .Append(';');
                    }
                }
                else if (parentValue == true)
                {
                    builder.Append(property)
                        .Append(':')
                        .Append(disabledValue)
                        .Append(';');
                }
            }

            private static bool ColorsEqual(RgbaColor left, RgbaColor right)
            {
                return Math.Abs(left.A - right.A) < 0.0001 && left.R == right.R && left.G == right.G && left.B == right.B;
            }
        }
    }
}
