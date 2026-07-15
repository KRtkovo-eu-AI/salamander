// SPDX-FileCopyrightText: 2026 Open Salamander Authors
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
            Rectangle screen = SystemInformation.VirtualScreen;
            _form = new ThumbnailForm
            {
                ShowInTaskbar = false,
                FormBorderStyle = FormBorderStyle.None,
                StartPosition = FormStartPosition.Manual,
                Bounds = new Rectangle(screen.Left, screen.Top, _width, _height)
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

                var pixelsReady = new TaskCompletionSource<string>();
                void OnWebMessageReceived(object? sender, CoreWebView2WebMessageReceivedEventArgs args)
                {
                    _webView.CoreWebView2.WebMessageReceived -= OnWebMessageReceived;
                    pixelsReady.TrySetResult(args.TryGetWebMessageAsString() ?? string.Empty);
                }

                using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(10));
                using var timeoutRegistration = timeout.Token.Register(() => pixelsReady.TrySetResult(string.Empty));

                _webView.CoreWebView2.WebMessageReceived += OnWebMessageReceived;
                _webView.CoreWebView2.NavigateToString(BuildSvgCanvasHtml(_path, _width, _height));

                string pixelBase64 = await pixelsReady.Task.ConfigureAwait(true);
                if (string.IsNullOrEmpty(pixelBase64))
                {
                    return;
                }

                byte[] rgbaPixels = Convert.FromBase64String(pixelBase64);
                SaveRawThumbnail(rgbaPixels, _width, _height, _output);
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

        private sealed class ThumbnailForm : Form
        {
            protected override bool ShowWithoutActivation => true;
        }

        private static string BuildSvgCanvasHtml(string path, int width, int height)
        {
            byte[] svgBytes = File.ReadAllBytes(path);
            string svgBase64 = Convert.ToBase64String(svgBytes);
            string title = WebUtility.HtmlEncode(Path.GetFileName(path));
            string widthText = width.ToString(CultureInfo.InvariantCulture);
            string heightText = height.ToString(CultureInfo.InvariantCulture);
            return "<!doctype html><html><head><meta charset=\"utf-8\"><title>" + title + "</title>" +
                   "<style>html,body{width:100%;height:100%;margin:0;overflow:hidden;background:white;}" +
                   "canvas{display:block;width:100vw;height:100vh;}</style></head><body>" +
                   "<canvas id=\"c\" width=\"" + widthText + "\" height=\"" + heightText + "\"></canvas>" +
                   "<script>(()=>{const w=" + widthText + ",h=" + heightText + ";" +
                   "const done=v=>{try{chrome.webview.postMessage(v||'');}catch(e){}};" +
                   "const img=new Image();img.onload=()=>{try{const c=document.getElementById('c');const x=c.getContext('2d');" +
                   "x.fillStyle='white';x.fillRect(0,0,w,h);" +
                   "const s=Math.min(w/img.naturalWidth,h/img.naturalHeight);" +
                   "const dw=Math.max(1,Math.round(img.naturalWidth*s));const dh=Math.max(1,Math.round(img.naturalHeight*s));" +
                   "const dx=Math.floor((w-dw)/2);const dy=Math.floor((h-dh)/2);x.drawImage(img,dx,dy,dw,dh);" +
                   "const d=x.getImageData(0,0,w,h).data;const chunk=32768;let out=[];" +
                   "for(let i=0;i<d.length;i+=chunk){let e=Math.min(i+chunk,d.length),p='';for(let j=i;j<e;j++)p+=String.fromCharCode(d[j]);out.push(p);}" +
                   "done(btoa(out.join('')));}catch(e){done('');}};img.onerror=()=>done('');" +
                   "img.src='data:image/svg+xml;base64," + svgBase64 + "';})();</script></body></html>";
        }

        private static void SaveRawThumbnail(byte[] rgbaPixels, int width, int height, string output)
        {
            int pixelCount = checked(width * height);
            if (rgbaPixels.Length < checked(pixelCount * 4))
            {
                return;
            }

            using var stream = new FileStream(output, FileMode.Create, FileAccess.Write, FileShare.Read);
            using var writer = new BinaryWriter(stream);
            writer.Write(width);
            writer.Write(height);

            for (int i = 0; i < pixelCount; ++i)
            {
                int offset = i * 4;
                writer.Write(rgbaPixels[offset + 2]);
                writer.Write(rgbaPixels[offset + 1]);
                writer.Write(rgbaPixels[offset]);
                writer.Write((byte)0);
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
