// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#nullable enable

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

namespace OpenSalamander.WebView2RenderViewer;

internal static class ThumbnailHost
{
    public static int Render(string payload)
    {
        if (!TryParse(payload, out string path, out string output, out int width, out int height) ||
            string.IsNullOrEmpty(path) || string.IsNullOrEmpty(output) || width <= 0 || height <= 0)
        {
            return 1;
        }

        Exception? error = null;
        using var done = new ManualResetEventSlim(false);
        var thread = new Thread(() =>
        {
            try
            {
                using var context = new ThumbnailContext(path, output, width, height);
                context.Completed += (_, __) => Application.ExitThread();
                Application.Run(context);
            }
            catch (Exception ex)
            {
                error = ex;
            }
            finally
            {
                done.Set();
            }
        })
        {
            IsBackground = true,
            Name = "WebView2RenderViewer Thumbnail Thread"
        };
        thread.SetApartmentState(ApartmentState.STA);
        thread.Start();

        if (!done.Wait(TimeSpan.FromSeconds(15)))
        {
            return 1;
        }

        return error == null && TryHasRenderedOutput(output) ? 0 : 1;
    }

    private static bool TryHasRenderedOutput(string output)
    {
        try
        {
            return new FileInfo(output).Length > 8;
        }
        catch (IOException)
        {
            return false;
        }
        catch (UnauthorizedAccessException)
        {
            return false;
        }
    }

    private static bool TryParse(string payload, out string path, out string output, out int width, out int height)
    {
        path = string.Empty;
        output = string.Empty;
        width = 0;
        height = 0;

        var values = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (var part in payload.Split('|'))
        {
            int equals = part.IndexOf('=');
            if (equals > 0)
            {
                values[part.Substring(0, equals)] = part.Substring(equals + 1);
            }
        }

        path = Decode(values.TryGetValue("path", out var p) ? p : string.Empty);
        output = Decode(values.TryGetValue("output", out var o) ? o : string.Empty);
        int.TryParse(values.TryGetValue("width", out var w) ? w : string.Empty, out width);
        int.TryParse(values.TryGetValue("height", out var h) ? h : string.Empty, out height);
        return true;
    }

    private static string Decode(string value)
    {
        if (string.IsNullOrEmpty(value))
        {
            return string.Empty;
        }

        try
        {
            return Encoding.UTF8.GetString(Convert.FromBase64String(value));
        }
        catch (FormatException)
        {
            return value;
        }
    }

    private sealed class ThumbnailContext : ApplicationContext
    {
        private readonly string _path;
        private readonly string _output;
        private readonly int _width;
        private readonly int _height;
        private readonly Form _form;
        private readonly WebView2 _webView;

        public event EventHandler? Completed;

        public ThumbnailContext(string path, string output, int width, int height)
        {
            _path = path;
            _output = output;
            _width = Math.Max(1, width);
            _height = Math.Max(1, height);
            _form = new Form
            {
                ShowInTaskbar = false,
                FormBorderStyle = FormBorderStyle.None,
                StartPosition = FormStartPosition.Manual,
                Bounds = new Rectangle(-32000, -32000, _width, _height)
            };
            _webView = new WebView2 { Dock = DockStyle.Fill };
            _form.Controls.Add(_webView);
            _form.Shown += async (_, __) => await RenderAsync().ConfigureAwait(true);
            _form.Show();
        }

        private async Task RenderAsync()
        {
            try
            {
                await _webView.EnsureCoreWebView2Async().ConfigureAwait(true);
                _webView.CoreWebView2.Settings.AreDefaultContextMenusEnabled = false;
                _webView.CoreWebView2.Settings.AreDevToolsEnabled = false;

                var navigated = new TaskCompletionSource<bool>();
                void OnNavigationCompleted(object? sender, CoreWebView2NavigationCompletedEventArgs args)
                {
                    _webView.NavigationCompleted -= OnNavigationCompleted;
                    navigated.TrySetResult(args.IsSuccess);
                }

                _webView.NavigationCompleted += OnNavigationCompleted;
                _webView.Source = new Uri(_path);
                if (!await navigated.Task.ConfigureAwait(true))
                {
                    return;
                }

                await Task.Delay(150).ConfigureAwait(true);
                using var stream = new MemoryStream();
                await _webView.CoreWebView2.CapturePreviewAsync(CoreWebView2CapturePreviewImageFormat.Png, stream).ConfigureAwait(true);
                stream.Position = 0;
                using var captured = Image.FromStream(stream);
                using var bitmap = new Bitmap(_width, _height, PixelFormat.Format32bppRgb);
                using (var graphics = Graphics.FromImage(bitmap))
                {
                    graphics.Clear(Color.White);
                    graphics.DrawImage(captured, new Rectangle(0, 0, _width, _height));
                }
                SaveRawThumbnail(bitmap, _output);
            }
            catch (Exception)
            {
            }
            finally
            {
                Completed?.Invoke(this, EventArgs.Empty);
                _form.Close();
            }
        }

        private static void SaveRawThumbnail(Bitmap bitmap, string output)
        {
            var bounds = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
            BitmapData? data = null;
            try
            {
                data = bitmap.LockBits(bounds, ImageLockMode.ReadOnly, PixelFormat.Format32bppRgb);
                using var stream = new FileStream(output, FileMode.Create, FileAccess.Write, FileShare.Read);
                using var writer = new BinaryWriter(stream);
                writer.Write(bitmap.Width);
                writer.Write(bitmap.Height);

                int rowBytes = checked(bitmap.Width * 4);
                byte[] row = new byte[rowBytes];
                for (int y = 0; y < bitmap.Height; ++y)
                {
                    IntPtr source = IntPtr.Add(data.Scan0, y * data.Stride);
                    Marshal.Copy(source, row, 0, rowBytes);
                    writer.Write(row, 0, rowBytes);
                }
            }
            finally
            {
                if (data != null)
                {
                    bitmap.UnlockBits(data);
                }
            }
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                _webView.Dispose();
                _form.Dispose();
            }
            base.Dispose(disposing);
        }
    }
}
