// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#nullable enable

using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Net;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;
using ColorfulCode;

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
                "Text Viewer Plugin",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return 1;
        }

        if (asynchronous && parsed.CloseHandle == IntPtr.Zero)
        {
            MessageBox.Show(parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null,
                "The native host did not provide a synchronization handle for the viewer.",
                "Text Viewer Plugin",
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
                "Text Viewer Plugin",
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
                "Text Viewer Plugin",
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
                "Text Viewer Plugin",
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
        private ViewerSession? _session;
        private WebBrowser? _browser;
        private bool _allowClose;
        private bool _taskbarStyleApplied;
        private IntPtr _ownerRestore;
        private bool _ownerAttached;

        public TextViewerForm()
        {
            Text = "Text Viewer";
            StartPosition = FormStartPosition.Manual;
            ShowInTaskbar = false;
            MinimizeBox = true;
            MaximizeBox = true;
            KeyPreview = true;
            AutoScaleMode = AutoScaleMode.Dpi;
            ClientSize = new Size(800, 600);

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
                    "Text Viewer Plugin",
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

            var viewer = new WebBrowser
            {
                Dock = DockStyle.Fill,
                AllowWebBrowserDrop = false,
                AllowNavigation = false,
                IsWebBrowserContextMenuEnabled = true,
                WebBrowserShortcutsEnabled = true,
                ScriptErrorsSuppressed = true,
            };

            Controls.Add(viewer);
            ThemeHelper.ApplyTheme(viewer);
            _browser = viewer;
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

            string html = ColorfulCodeRenderer.BuildDocument(text, extension, caption);
            _browser.DocumentText = html;
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

        private static string BuildCaption(string caption)
        {
            if (string.IsNullOrWhiteSpace(caption))
            {
                return "Text Viewer";
            }

            return string.Format(CultureInfo.CurrentCulture, "{0} - Text Viewer", caption);
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

    private static class ColorfulCodeRenderer
    {
        private const string DefaultThemeName = "InspiredGitHub";
        private static readonly Lazy<SyntaxSet> s_syntaxSet = new(() => SyntaxSet.LoadDefaults());
        private static readonly Lazy<ThemeSet> s_themeSet = new(() => ThemeSet.LoadDefaults());

        public static string BuildDocument(string text, string extension, string? caption)
        {
            var content = HighlightOrFallback(text, extension);
            return WrapDocument(content, caption);
        }

        private static RenderedContent HighlightOrFallback(string text, string extension)
        {
            try
            {
                var syntax = GetSyntax(extension);
                var theme = GetTheme();

                if (syntax is not null && theme is not null)
                {
                    var highlighted = syntax.HighlightToHtml(text, theme);
                    if (!string.IsNullOrEmpty(highlighted))
                    {
                        var styled = ApplyThemeStyles(highlighted, theme, out var globalStyle);
                        return new RenderedContent(styled, globalStyle);
                    }
                }
            }
            catch
            {
                // fall back to plain text
            }

            return new RenderedContent(BuildPlainTextHtml(text), null);
        }

        private static Syntax? GetSyntax(string extension)
        {
            var syntaxSet = s_syntaxSet.Value;

            if (!string.IsNullOrEmpty(extension))
            {
                try
                {
                    var syntax = syntaxSet.FindByExtension(extension);
                    if (syntax is not null)
                    {
                        return syntax;
                    }
                }
                catch
                {
                }
            }

            try
            {
                return syntaxSet.FindByExtension("txt");
            }
            catch
            {
                return null;
            }
        }

        private static Theme? GetTheme()
        {
            var themeSet = s_themeSet.Value;

            try
            {
                return themeSet[DefaultThemeName];
            }
            catch
            {
            }

            try
            {
                var valuesProperty = themeSet.GetType().GetProperty("Values");
                if (valuesProperty?.GetValue(themeSet) is IEnumerable values)
                {
                    foreach (var value in values)
                    {
                        if (value is Theme theme)
                        {
                            return theme;
                        }
                    }
                }
            }
            catch
            {
            }

            return null;
        }

        private static string WrapDocument(RenderedContent content, string? caption)
        {
            ThemeHelper.ThemePalette? palette = ThemeHelper.TryGetPalette(out var value) ? value : null;

            var builder = new StringBuilder();
            builder.AppendLine("<!DOCTYPE html>");
            builder.Append("<html><head><meta charset=\"utf-8\"/><meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"/>");
            if (palette.HasValue && palette.Value.IsDark)
            {
                builder.Append("<meta name=\"color-scheme\" content=\"dark\"/>");
            }
            builder.Append("<title>");
            builder.Append(WebUtility.HtmlEncode(string.IsNullOrWhiteSpace(caption) ? "Text Viewer" : caption));
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
            return $"<pre>{encoded}</pre>";
        }
        private static string ApplyThemeStyles(string html, Theme theme, out string? globalPreStyle)
        {
            var styleMap = BuildClassStyleMap(theme, out globalPreStyle);
            if (styleMap.Count == 0)
            {
                return html;
            }

            return Regex.Replace(html,
                "(<[^>]+?\\sclass\\s*=\\s*)([\"'])([^\"'>]+)(\\2[^>]*>)",
                match =>
                {
                    var before = match.Groups[1].Value;
                    var quote = match.Groups[2].Value;
                    var classValue = match.Groups[3].Value;
                    var after = match.Groups[4].Value;

                    var fullTag = match.Value;
                    if (fullTag.IndexOf("style=", 0, StringComparison.OrdinalIgnoreCase) >= 0)
                    {
                        return fullTag;
                    }

                    var style = BuildStyleForClasses(classValue, styleMap);
                    if (string.IsNullOrEmpty(style))
                    {
                        return fullTag;
                    }

                    var suffix = after.Length > 0 ? after.Substring(1) : string.Empty;
                    return $"{before}{quote}{classValue}{quote} style=\"{style}\"{suffix}";
                }, RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
        }

        private static IReadOnlyDictionary<string, string> BuildClassStyleMap(Theme theme, out string? globalPreStyle)
        {
            var map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            string? global = null;

            foreach (var rule in EnumerateThemeRules(theme))
            {
                var declaration = BuildCssDeclaration(rule.Settings);
                if (string.IsNullOrEmpty(declaration))
                {
                    continue;
                }

                if (rule.ScopeNames.Count == 0)
                {
                    global ??= declaration;
                    continue;
                }

                foreach (var scope in rule.ScopeNames)
                {
                    if (string.IsNullOrWhiteSpace(scope))
                    {
                        continue;
                    }

                    var trimmed = scope.Trim();
                    AddStyleMapping(map, trimmed, declaration);

                    foreach (var part in trimmed.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries))
                    {
                        AddStyleMapping(map, part, declaration);

                        foreach (var token in part.Split(new[] { '.' }, StringSplitOptions.RemoveEmptyEntries))
                        {
                            AddStyleMapping(map, token, declaration);
                        }
                    }
                }
            }

            globalPreStyle = global;
            return map;
        }

        private static void AddStyleMapping(IDictionary<string, string> map, string key, string value)
        {
            if (string.IsNullOrEmpty(key) || map.ContainsKey(key))
            {
                return;
            }

            map[key] = value;
        }

        private static string BuildStyleForClasses(string classValue, IReadOnlyDictionary<string, string> styleMap)
        {
            if (string.IsNullOrWhiteSpace(classValue))
            {
                return string.Empty;
            }

            var builder = new StringBuilder();
            var tokens = classValue.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (var token in tokens)
            {
                AppendStyleForToken(builder, token, styleMap);
            }

            return builder.ToString();
        }

        private static void AppendStyleForToken(StringBuilder builder, string token, IReadOnlyDictionary<string, string> styleMap)
        {
            if (styleMap.TryGetValue(token, out var style))
            {
                builder.Append(style);
            }

            if (token.IndexOf('.') >= 0)
            {
                var parts = token.Split(new[] { '.' }, StringSplitOptions.RemoveEmptyEntries);
                foreach (var part in parts)
                {
                    if (styleMap.TryGetValue(part, out var partStyle))
                    {
                        builder.Append(partStyle);
                    }
                }
            }
        }

        private static string BuildCssDeclaration(object settings)
        {
            var builder = new StringBuilder();

            AppendColor(builder, settings, "Foreground", "color");
            AppendColor(builder, settings, "Background", "background-color");

            AppendFontStyle(builder, settings);
            AppendFontWeight(builder, settings);

            return builder.ToString();
        }

        private static void AppendColor(StringBuilder builder, object settings, string propertyName, string cssProperty)
        {
            var color = GetColor(settings, propertyName);
            if (!string.IsNullOrEmpty(color))
            {
                builder.Append(cssProperty);
                builder.Append(':');
                builder.Append(color);
                builder.Append(';');
            }
        }

        private static void AppendFontStyle(StringBuilder builder, object settings)
        {
            var value = GetString(settings, "FontStyle");
            if (string.IsNullOrEmpty(value))
            {
                return;
            }

            if (value.IndexOf("italic", 0, StringComparison.OrdinalIgnoreCase) >= 0)
            {
                builder.Append("font-style:italic;");
            }
            if (value.IndexOf("bold", 0, StringComparison.OrdinalIgnoreCase) >= 0)
            {
                builder.Append("font-weight:bold;");
            }
            if (value.IndexOf("underline", 0, StringComparison.OrdinalIgnoreCase) >= 0)
            {
                builder.Append("text-decoration:underline;");
            }
        }

        private static void AppendFontWeight(StringBuilder builder, object settings)
        {
            var weight = GetString(settings, "FontWeight");
            if (string.IsNullOrEmpty(weight))
            {
                return;
            }

            if (int.TryParse(weight, NumberStyles.Integer, CultureInfo.InvariantCulture, out var numeric) && numeric > 0)
            {
                builder.Append("font-weight:");
                builder.Append(numeric.ToString(CultureInfo.InvariantCulture));
                builder.Append(';');
                return;
            }

            builder.Append("font-weight:");
            builder.Append(weight);
            builder.Append(';');
        }

        private static string? GetColor(object settings, string propertyName)
        {
            object? value = GetMember(settings, propertyName);
            if (value is null)
            {
                return null;
            }

            if (value is string str)
            {
                return NormalizeColor(str);
            }

            if (value is Color color)
            {
                return ColorTranslator.ToHtml(color);
            }

            var toString = value.ToString();
            if (!string.IsNullOrEmpty(toString))
            {
                return NormalizeColor(toString);
            }

            return null;
        }

        private static string? NormalizeColor(string value)
        {
            if (string.IsNullOrWhiteSpace(value))
            {
                return null;
            }

            value = value.Trim();
            if (value.StartsWith("#", StringComparison.Ordinal))
            {
                return value;
            }

            if (uint.TryParse(value, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out var numeric))
            {
                if (value.Length <= 6)
                {
                    return "#" + value.PadLeft(6, '0');
                }

                return "#" + numeric.ToString("X8", CultureInfo.InvariantCulture);
            }

            return value;
        }

        private static string? GetString(object settings, string propertyName)
        {
            object? value = GetMember(settings, propertyName);
            return value switch
            {
                null => null,
                string str => string.IsNullOrWhiteSpace(str) ? null : str,
                _ => value.ToString(),
            };
        }

        private static object? GetMember(object instance, string memberName)
        {
            if (instance is null)
            {
                return null;
            }

            var type = instance.GetType();

            var property = type.GetProperty(memberName, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
            if (property is not null)
            {
                return property.GetValue(instance);
            }

            var field = type.GetField(memberName, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
            if (field is not null)
            {
                return field.GetValue(instance);
            }

            return null;
        }

        private static IEnumerable<ThemeRule> EnumerateThemeRules(Theme theme)
        {
            var themeType = theme.GetType();
            var candidates = new[] { "Settings", "Scopes", "Rules" };
            foreach (var candidate in candidates)
            {
                if (TryGetEnumerable(theme, candidate, out var values))
                {
                    foreach (var setting in values)
                    {
                        if (setting is null)
                        {
                            continue;
                        }

                        var style = GetMember(setting, "Settings") ?? GetMember(setting, "Style") ?? GetMember(setting, "SettingsValue");
                        if (style is null)
                        {
                            continue;
                        }

                        var scopes = ExtractScopes(setting);
                        yield return new ThemeRule(scopes, style);
                    }
                    yield break;
                }
            }
        }

        private static bool TryGetEnumerable(object instance, string memberName, out IEnumerable values)
        {
            values = Array.Empty<object>();
            var member = GetMember(instance, memberName);
            if (member is IEnumerable enumerable)
            {
                values = enumerable;
                return true;
            }

            return false;
        }

        private static IReadOnlyCollection<string> ExtractScopes(object setting)
        {
            var scopes = new List<string>();

            object? scopeValue = GetMember(setting, "Scope") ?? GetMember(setting, "Scopes");
            if (scopeValue is string scopeString)
            {
                AddScopes(scopes, scopeString);
            }
            else if (scopeValue is IEnumerable enumerable)
            {
                foreach (var item in enumerable)
                {
                    if (item is string s)
                    {
                        AddScopes(scopes, s);
                    }
                }
            }

            return scopes;
        }

        private static void AddScopes(ICollection<string> target, string? value)
        {
            if (string.IsNullOrWhiteSpace(value))
            {
                return;
            }

            var parts = value.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (var part in parts)
            {
                var trimmed = part.Trim();
                if (!string.IsNullOrEmpty(trimmed))
                {
                    target.Add(trimmed);
                }
            }
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

        private readonly struct ThemeRule
        {
            public ThemeRule(IReadOnlyCollection<string> scopeNames, object settings)
            {
                ScopeNames = scopeNames;
                Settings = settings;
            }

            public IReadOnlyCollection<string> ScopeNames { get; }
            public object Settings { get; }
        }
    }
}
