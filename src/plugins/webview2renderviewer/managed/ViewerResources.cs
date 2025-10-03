// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System.Drawing;
using System.IO;
using System.Reflection;
using System.Text;

namespace OpenSalamander.WebView2RenderViewer;

internal static class ViewerResources
{
    private const string ViewerIconResourceName = "OpenSalamander.WebView2RenderViewer.Resources.viewer.ico";
    private const string MarkedScriptResourceName = "OpenSalamander.WebView2RenderViewer.Resources.marked.min.js";
    private static Icon? s_viewerIcon;
    private static string? s_markedScript;

    public static Icon ViewerIcon
    {
        get
        {
            return s_viewerIcon ??= LoadViewerIcon();
        }
    }

    public static string MarkedScript
    {
        get
        {
            return s_markedScript ??= LoadMarkedScript();
        }
    }

    private static Icon LoadViewerIcon()
    {
        var assembly = Assembly.GetExecutingAssembly();
        using Stream? stream = assembly.GetManifestResourceStream(ViewerIconResourceName);
        if (stream is not null)
        {
            return new Icon(stream);
        }

        return SystemIcons.Application;
    }

    private static string LoadMarkedScript()
    {
        string? script = LoadTextResource(MarkedScriptResourceName);
        if (!string.IsNullOrEmpty(script))
        {
            return script;
        }

        return "window.marked=window.marked||function(input){return input;};";
    }

    private static string? LoadTextResource(string resourceName)
    {
        var assembly = Assembly.GetExecutingAssembly();
        using Stream? stream = assembly.GetManifestResourceStream(resourceName);
        if (stream is null)
        {
            return null;
        }

        using var reader = new StreamReader(stream, Encoding.UTF8, detectEncodingFromByteOrderMarks: true);
        return reader.ReadToEnd();
    }
}
