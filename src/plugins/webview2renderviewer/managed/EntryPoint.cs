// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#nullable enable

using System;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Text;
using System.Windows.Forms;

namespace OpenSalamander.WebView2RenderViewer;

public static class EntryPoint
{
    private static bool _initialized;
    private static readonly object s_resolutionLock = new();
    private static bool s_resolutionInitialized;
    private static string? s_baseDirectory;

    [STAThread]
    public static int Dispatch(string? argument)
    {
        IntPtr parent = IntPtr.Zero;

        try
        {
            EnsureApplicationInitialized();

            var parts = (argument ?? string.Empty).Split(new[] { ';' }, 3);
            var command = parts.Length > 0 ? parts[0] : string.Empty;
            parent = ParseHandle(parts.Length > 1 ? parts[1] : string.Empty);
            var payload = parts.Length > 2 ? parts[2] : string.Empty;

            return command switch
            {
                "View" => ViewerHost.Launch(parent, payload, asynchronous: true),
                "ViewSync" => ViewerHost.Launch(parent, payload, asynchronous: false),
                "Release" => ViewerHost.ReleaseSessions(ShouldForceRelease(payload)),
                _ => 1,
            };
        }
        catch (Exception ex)
        {
            if (parent != IntPtr.Zero)
            {
                MessageBox.Show(new WindowHandleWrapper(parent),
                    $"Unexpected managed exception:\n{ex.Message}",
                    "WebView2 Render Viewer .NET Plugin",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
            else
            {
                MessageBox.Show($"Unexpected managed exception:\n{ex.Message}",
                    "WebView2 Render Viewer .NET Plugin",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
            return -1;
        }
    }

    private static void EnsureApplicationInitialized()
    {
        if (_initialized)
        {
            return;
        }

        EnsureAssemblyResolution();

        Application.EnableVisualStyles();

        try
        {
            if (!Application.MessageLoop && Application.OpenForms.Count == 0)
            {
                Application.SetCompatibleTextRenderingDefault(false);
            }
        }
        catch (InvalidOperationException)
        {
            // A WinForms control has already been created somewhere else in the
            // process (for example, another plugin). In that case it's too late
            // to change the compatible text rendering default, so we simply
            // continue with the existing setting.
        }

        _initialized = true;
    }

    private static void EnsureAssemblyResolution()
    {
        if (s_resolutionInitialized)
        {
            return;
        }

        lock (s_resolutionLock)
        {
            if (s_resolutionInitialized)
            {
                return;
            }

            try
            {
                string? location = typeof(EntryPoint).Assembly.Location;
                if (!string.IsNullOrEmpty(location))
                {
                    string? directory = Path.GetDirectoryName(location);
                    if (!string.IsNullOrEmpty(directory))
                    {
                        s_baseDirectory = directory;
                        AppDomain.CurrentDomain.AssemblyResolve += OnAssemblyResolve;
                    }
                }

                TryRegisterCodePageProvider();
            }
            catch (IOException)
            {
            }
            catch (UnauthorizedAccessException)
            {
            }
            catch (PlatformNotSupportedException)
            {
            }

            s_resolutionInitialized = true;
        }
    }

    private static void TryRegisterCodePageProvider()
    {
        try
        {
            const string providerTypeName = "System.Text.CodePagesEncodingProvider, System.Text.Encoding.CodePages";
            var providerType = Type.GetType(providerTypeName, throwOnError: false);
            if (providerType == null)
            {
                return;
            }

            var instanceProperty = providerType.GetProperty("Instance", BindingFlags.Public | BindingFlags.Static);
            if (instanceProperty == null)
            {
                return;
            }

            if (instanceProperty.GetValue(null) is EncodingProvider provider)
            {
                Encoding.RegisterProvider(provider);
            }
        }
        catch (MissingMemberException)
        {
        }
        catch (TypeLoadException)
        {
        }
        catch (TargetInvocationException)
        {
        }
        catch (NotSupportedException)
        {
        }
    }

    private static Assembly? OnAssemblyResolve(object? sender, ResolveEventArgs args)
    {
        var baseDirectory = s_baseDirectory;
        if (string.IsNullOrEmpty(baseDirectory))
        {
            return null;
        }

        try
        {
            var assemblyName = new AssemblyName(args.Name);
            var simpleName = assemblyName.Name;
            if (string.IsNullOrEmpty(simpleName))
            {
                return null;
            }

            var candidatePath = Path.Combine(baseDirectory, simpleName + ".dll");
            if (!File.Exists(candidatePath))
            {
                return null;
            }

            return Assembly.LoadFrom(candidatePath);
        }
        catch (IOException)
        {
            return null;
        }
        catch (BadImageFormatException)
        {
            return null;
        }
    }

    private static IntPtr ParseHandle(string text)
    {
        if (ulong.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out var value))
        {
            return new IntPtr(unchecked((long)value));
        }

        return IntPtr.Zero;
    }

    private static bool ShouldForceRelease(string? payload)
    {
        var payloadText = payload?.Trim() ?? string.Empty;

        if (payloadText.Length == 0)
        {
            return false;
        }

        var segments = payloadText.Split('|');
        foreach (var segment in segments)
        {
            if (string.IsNullOrWhiteSpace(segment))
            {
                continue;
            }

            var kv = segment.Split(new[] { '=' }, 2);
            if (kv.Length == 0)
            {
                continue;
            }

            var key = kv[0].Trim();
            if (!key.Equals("force", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            if (kv.Length == 1)
            {
                return true;
            }

            var value = kv[1].Trim();
            return value.Equals("1", StringComparison.OrdinalIgnoreCase) ||
                   value.Equals("true", StringComparison.OrdinalIgnoreCase);
        }

        return false;
    }
}
