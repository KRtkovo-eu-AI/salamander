// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System.Drawing;
using System.IO;
using System.Reflection;

namespace OpenSalamander.WebView2RenderViewer;

internal static class ViewerResources
{
    private const string ViewerIconResourceName = "OpenSalamander.WebView2RenderViewer.Resources.viewer.ico";
    private static Icon? s_viewerIcon;

    public static Icon ViewerIcon
    {
        get
        {
            return s_viewerIcon ??= LoadViewerIcon();
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
}
