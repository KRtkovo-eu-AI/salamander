// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

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

namespace OpenSalamander.JsonViewer;

internal static class ViewerHost
{
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

        var request = new ViewerRequest(parent, parsed);

        if (asynchronous)
        {
            if (request.CloseEvent is null)
            {
                MessageBox.Show(request.OwnerWindow,
                    "The native host did not provide a synchronization handle for the viewer.",
                    "JSON Viewer Plugin",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return 1;
            }

            var thread = new Thread(() => RunViewer(request))
            {
                IsBackground = false,
                Name = "JSON Viewer"
            };
            thread.SetApartmentState(ApartmentState.STA);
            thread.Start();
            return 0;
        }

        RunViewer(request);
        return request.StartupSucceeded ? 0 : 1;
    }

    private static void RunViewer(ViewerRequest request)
    {
        try
        {
            using var context = new JsonViewerApplicationContext(request);
            Application.Run(context);
        }
        catch (Exception ex)
        {
            request.MarkStartupFailed();
            MessageBox.Show(request.OwnerWindow,
                $"Unable to open the JSON viewer window.\n{ex.Message}",
                "JSON Viewer Plugin",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            request.SignalClosed();
        }
    }

    private sealed class JsonViewerApplicationContext : ApplicationContext
    {
        private readonly ViewerRequest _request;
        private readonly JsonViewerForm _form;

        public JsonViewerApplicationContext(ViewerRequest request)
        {
            _request = request;
            _form = new JsonViewerForm(request);
            _form.FormClosed += OnFormClosed;
            MainForm = _form;
        }

        private void OnFormClosed(object? sender, FormClosedEventArgs e)
        {
            _request.SignalClosed();
            ExitThread();
        }
    }

    private sealed class ViewerRequest
    {
        public ViewerRequest(IntPtr parent, ViewCommandPayload payload)
        {
            Parent = parent;
            Payload = payload;
            OwnerWindow = parent != IntPtr.Zero ? new WindowHandleWrapper(parent) : null;

            if (payload.CloseHandle != IntPtr.Zero)
            {
                CloseEvent = new EventWaitHandle(false, EventResetMode.AutoReset);
                CloseEvent.SafeWaitHandle = new SafeWaitHandle(payload.CloseHandle, ownsHandle: false);
            }
        }

        public IntPtr Parent { get; }
        public ViewCommandPayload Payload { get; }
        public IWin32Window? OwnerWindow { get; }
        public EventWaitHandle? CloseEvent { get; }
        public bool StartupSucceeded { get; private set; } = true;

        public void SignalClosed()
        {
            CloseEvent?.Set();
        }

        public void MarkStartupFailed()
        {
            StartupSucceeded = false;
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
        private readonly ViewerRequest _request;
        private readonly JsonViewerControl _viewer;
        private bool _jsonLoaded;

        public JsonViewerForm(ViewerRequest request)
        {
            _request = request;

            Text = BuildCaption(request.Payload.Caption);
            StartPosition = FormStartPosition.Manual;
            ShowInTaskbar = false;
            MinimizeBox = true;
            MaximizeBox = true;

            _viewer = new JsonViewerControl
            {
                Dock = DockStyle.Fill
            };
            _viewer.PropertyChanged += OnViewerPropertyChanged;
            Controls.Add(_viewer);

            HandleCreated += OnHandleCreated;
            Shown += OnShown;
            FormClosing += OnFormClosing;
        }

        protected override void OnLoad(EventArgs e)
        {
            base.OnLoad(e);

            var bounds = _request.Payload.Bounds;
            if (bounds.Width > 0 && bounds.Height > 0)
            {
                Bounds = bounds;
            }
            else
            {
                StartPosition = FormStartPosition.CenterScreen;
            }

            if (_request.Payload.ShowCommand == NativeMethods.SW_SHOWMAXIMIZED)
            {
                WindowState = FormWindowState.Maximized;
            }
            else if (_request.Payload.ShowCommand == NativeMethods.SW_SHOWMINIMIZED)
            {
                WindowState = FormWindowState.Minimized;
            }

            TopMost = _request.Payload.AlwaysOnTop;
        }

        private void OnHandleCreated(object? sender, EventArgs e)
        {
            if (_request.Parent != IntPtr.Zero)
            {
                NativeMethods.SetWindowLongPtr(Handle, NativeMethods.GWL_HWNDPARENT, _request.Parent);
            }
        }

        private void OnShown(object? sender, EventArgs e)
        {
            LoadJson();
            NativeMethods.SetForegroundWindow(Handle);
        }

        private void OnFormClosing(object? sender, FormClosingEventArgs e)
        {
            if (!_jsonLoaded)
            {
                _request.MarkStartupFailed();
            }
        }

        private void LoadJson()
        {
            try
            {
                string json = File.ReadAllText(_request.Payload.FilePath);
                _viewer.ShowTab(ViewerTabs.Viewer);
                _viewer.refreshFromString(json);
                _jsonLoaded = true;
            }
            catch (Exception ex)
            {
                _request.MarkStartupFailed();
                MessageBox.Show(_request.OwnerWindow,
                    $"Unable to open the selected file.\n{ex.Message}",
                    "JSON Viewer Plugin",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                BeginInvoke(new MethodInvoker(Close));
            }
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
