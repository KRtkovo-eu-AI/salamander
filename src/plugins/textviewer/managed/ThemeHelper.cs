// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#nullable enable

using System;
using System.Drawing;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace OpenSalamander.TextViewer;

internal static class ThemeHelper
{
    private static readonly ConditionalWeakTable<WebBrowser, BrowserThemeState> s_browserThemeStates = new();

    private const int SALCOL_ITEM_FG_NORMAL = 6;
    private const int SALCOL_ITEM_FG_SELECTED = 7;
    private const int SALCOL_ITEM_BK_NORMAL = 11;
    private const int SALCOL_ITEM_BK_SELECTED = 12;
    private const int SALCOL_HOT_PANEL = 23;

    public static bool TryGetPalette(out ThemePalette palette)
    {
        var current = GetPalette();
        if (current.HasValue)
        {
            palette = current.Value;
            return true;
        }

        palette = default;
        return false;
    }

    public static void ApplyTheme(Form form)
    {
        if (!TryGetPalette(out var palette))
        {
            NativeMethods.SetDarkModeEnabled(false);
            return;
        }

        NativeMethods.SetDarkModeEnabled(palette.IsDark);
        ApplyPalette(form, palette);

        form.ControlAdded -= FormOnControlAddedApplyPalette;
        form.ControlAdded += FormOnControlAddedApplyPalette;

        if (form.IsHandleCreated)
        {
            NativeMethods.ApplyImmersiveDarkMode(form.Handle, palette.IsDark, palette.ControlBorder);
        }

        form.HandleCreated -= OnFormHandleCreatedApplyDarkMode;
        form.HandleCreated += OnFormHandleCreatedApplyDarkMode;
    }

    public static void ApplyTheme(Control control)
    {
        if (!TryGetPalette(out var palette))
        {
            NativeMethods.SetDarkModeEnabled(false);
            return;
        }

        NativeMethods.SetDarkModeEnabled(palette.IsDark);
        ApplyPalette(control, palette);
    }

    private static ThemePalette? GetPalette()
    {
        try
        {
            uint background = NativeMethods.GetCurrentColor(SALCOL_ITEM_BK_NORMAL);
            uint foreground = NativeMethods.GetCurrentColor(SALCOL_ITEM_FG_NORMAL);
            uint highlightBackground = NativeMethods.GetCurrentColor(SALCOL_ITEM_BK_SELECTED);
            uint highlightForeground = NativeMethods.GetCurrentColor(SALCOL_ITEM_FG_SELECTED);
            uint accent = NativeMethods.GetCurrentColor(SALCOL_HOT_PANEL);

            if (background == 0 && foreground == 0)
            {
                return new ThemePalette(SystemColors.Control,
                    SystemColors.ControlText,
                    SystemColors.Highlight,
                    SystemColors.HighlightText,
                    SystemColors.HotTrack);
            }

            var palette = new ThemePalette(
                ColorTranslator.FromWin32(unchecked((int)background)),
                ColorTranslator.FromWin32(unchecked((int)foreground)),
                highlightBackground != 0 ? ColorTranslator.FromWin32(unchecked((int)highlightBackground)) : SystemColors.Highlight,
                highlightForeground != 0 ? ColorTranslator.FromWin32(unchecked((int)highlightForeground)) : SystemColors.HighlightText,
                accent != 0 ? ColorTranslator.FromWin32(unchecked((int)accent)) : SystemColors.HotTrack);

            return palette;
        }
        catch (DllNotFoundException)
        {
        }
        catch (EntryPointNotFoundException)
        {
        }

        return null;
    }

    private static void FormOnControlAddedApplyPalette(object? sender, ControlEventArgs e)
    {
        if (TryGetPalette(out var palette))
        {
            NativeMethods.SetDarkModeEnabled(palette.IsDark);
            ApplyPalette(e.Control, palette);
        }
    }

    private static void OnFormHandleCreatedApplyDarkMode(object? sender, EventArgs e)
    {
        if (sender is not Form form)
        {
            return;
        }

        var palette = GetPalette();
        if (palette.HasValue)
        {
            NativeMethods.ApplyImmersiveDarkMode(form.Handle, palette.Value.IsDark, palette.Value.ControlBorder);
        }
    }

    private static void ApplyPalette(Form form, ThemePalette palette)
    {
        form.BackColor = palette.Background;
        form.ForeColor = palette.Foreground;

        foreach (Control child in form.Controls)
        {
            ApplyPalette(child, palette);
        }
    }

    private static void ApplyPalette(Control control, ThemePalette palette)
    {
        switch (control)
        {
            case TextBoxBase textBox:
                textBox.BackColor = palette.InputBackground;
                textBox.ForeColor = palette.InputForeground;
                break;
            case ComboBox comboBox:
                comboBox.BackColor = palette.InputBackground;
                comboBox.ForeColor = palette.InputForeground;
                comboBox.DrawMode = DrawMode.OwnerDrawFixed;
                comboBox.DrawItem -= ComboBoxOnDrawItem;
                comboBox.DrawItem += ComboBoxOnDrawItem;
                break;
            case ListView listView:
                listView.BackColor = palette.InputBackground;
                listView.ForeColor = palette.InputForeground;
                listView.BorderStyle = BorderStyle.FixedSingle;
                break;
            case TreeView treeView:
                treeView.BackColor = palette.InputBackground;
                treeView.ForeColor = palette.InputForeground;
                treeView.LineColor = palette.Accent;
                break;
            case Button button when palette.IsDark:
                button.FlatStyle = FlatStyle.Flat;
                button.ForeColor = palette.Foreground;
                button.BackColor = palette.ControlBackground;
                button.FlatAppearance.BorderColor = palette.ControlBorder;
                break;
            case LinkLabel link:
                link.LinkColor = palette.Accent;
                link.ActiveLinkColor = palette.HighlightForeground;
                link.VisitedLinkColor = palette.Accent;
                break;
            case ToolStrip toolStrip:
                ThemeRenderer.Attach(toolStrip, palette);
                break;
            case WebBrowser webBrowser:
                NativeMethods.SetDarkModeEnabled(palette.IsDark);
                ApplyWebBrowserTheme(webBrowser, palette);
                break;
        }

        if (control is not Button)
        {
            control.BackColor = palette.Background;
        }

        control.ForeColor = palette.Foreground;

        if (control.ContextMenuStrip is ContextMenuStrip menu)
        {
            ThemeRenderer.Attach(menu, palette);
        }

        foreach (Control child in control.Controls)
        {
            ApplyPalette(child, palette);
        }
    }

    private static void ComboBoxOnDrawItem(object? sender, DrawItemEventArgs e)
    {
        if (sender is not ComboBox comboBox)
        {
            return;
        }

        if (!TryGetPalette(out var palette))
        {
            comboBox.DrawMode = DrawMode.Normal;
            return;
        }

        e.DrawBackground();
        var background = (e.State & DrawItemState.Selected) != 0 ? palette.HighlightBackground : palette.InputBackground;
        var foreground = (e.State & DrawItemState.Selected) != 0 ? palette.HighlightForeground : palette.InputForeground;

        using (var brush = new SolidBrush(background))
        {
            e.Graphics.FillRectangle(brush, e.Bounds);
        }

        string text = e.Index >= 0 && e.Index < comboBox.Items.Count
            ? Convert.ToString(comboBox.Items[e.Index]) ?? string.Empty
            : comboBox.Text;

        using (var textBrush = new SolidBrush(foreground))
        {
            e.Graphics.DrawString(text, comboBox.Font, textBrush, e.Bounds);
        }
    }

    private static void ApplyWebBrowserTheme(WebBrowser browser, ThemePalette palette)
    {
        browser.BackColor = palette.Background;
        browser.ForeColor = palette.Foreground;

        var state = s_browserThemeStates.GetValue(browser, static _ => new BrowserThemeState());
        state.Attach(browser);
        state.Apply(browser);
        UpdateBrowserDocumentTheme(browser, palette);
    }

    private sealed class BrowserThemeState
    {
        private readonly EventHandler _handleCreatedHandler;
        private readonly WebBrowserDocumentCompletedEventHandler _documentCompletedHandler;
        private readonly EventHandler _disposedHandler;
        private bool _attached;

        public BrowserThemeState()
        {
            _handleCreatedHandler = OnHandleCreated;
            _documentCompletedHandler = OnDocumentCompleted;
            _disposedHandler = OnDisposed;
        }

        public void Attach(WebBrowser browser)
        {
            if (_attached)
            {
                return;
            }

            browser.HandleCreated += _handleCreatedHandler;
            browser.DocumentCompleted += _documentCompletedHandler;
            browser.Disposed += _disposedHandler;
            _attached = true;
        }

        public void Apply(WebBrowser browser)
        {
            if (browser.IsHandleCreated)
            {
                NativeMethods.ApplyDarkModeTree(browser.Handle);
                if (TryGetPalette(out var palette))
                {
                    UpdateBrowserDocumentTheme(browser, palette);
                }
            }
        }

        private void OnHandleCreated(object? sender, EventArgs e)
        {
            if (sender is WebBrowser browser)
            {
                NativeMethods.ApplyDarkModeTree(browser.Handle);
            }
        }

        private void OnDocumentCompleted(object? sender, WebBrowserDocumentCompletedEventArgs e)
        {
            if (sender is WebBrowser browser)
            {
                NativeMethods.ApplyDarkModeTree(browser.Handle);
                if (TryGetPalette(out var palette))
                {
                    UpdateBrowserDocumentTheme(browser, palette);
                }
            }
        }

        private void OnDisposed(object? sender, EventArgs e)
        {
            if (sender is not WebBrowser browser)
            {
                return;
            }

            browser.HandleCreated -= _handleCreatedHandler;
            browser.DocumentCompleted -= _documentCompletedHandler;
            browser.Disposed -= _disposedHandler;
            s_browserThemeStates.Remove(browser);
        }
    }

    private static int ComputeLuminance(Color color)
    {
        return (color.R * 30 + color.G * 59 + color.B * 11) / 100;
    }

    private static void UpdateBrowserDocumentTheme(WebBrowser browser, ThemePalette palette)
    {
        if (browser.Document is not HtmlDocument document)
        {
            return;
        }

        string scheme = palette.IsDark ? "dark" : "light";

        HtmlElement? root = TryGetDocumentElement(document);
        if (root is not null)
        {
            root.SetAttribute("data-theme", scheme);
            ApplyCssDeclaration(root, "color-scheme", scheme);
            ApplyScrollbarTheme(root, palette);
        }

        if (document.Body is HtmlElement body)
        {
            ApplyScrollbarTheme(body, palette);
        }

        EnsureColorSchemeMeta(document, palette.IsDark);
    }

    private static void ApplyCssDeclaration(HtmlElement element, string propertyName, string? propertyValue)
    {
        string current = element.GetAttribute("style");
        string updated = UpdateCssDeclaration(current, propertyName, propertyValue);
        element.SetAttribute("style", updated);
    }

    private static string UpdateCssDeclaration(string? style, string propertyName, string? propertyValue)
    {
        bool hasValue = !string.IsNullOrWhiteSpace(propertyValue);

        if (string.IsNullOrWhiteSpace(style))
        {
            return hasValue ? propertyName + ":" + propertyValue + ";" : string.Empty;
        }

        var builder = new StringBuilder();
        bool replaced = false;
        string[] declarations = style.Split(';');
        foreach (string declaration in declarations)
        {
            if (string.IsNullOrWhiteSpace(declaration))
            {
                continue;
            }

            int separatorIndex = declaration.IndexOf(':');
            if (separatorIndex <= 0)
            {
                continue;
            }

            string name = declaration.Substring(0, separatorIndex).Trim();
            string value = declaration.Substring(separatorIndex + 1).Trim();

            if (string.Equals(name, propertyName, StringComparison.OrdinalIgnoreCase))
            {
                if (!replaced && hasValue)
                {
                    builder.Append(propertyName).Append(':').Append(propertyValue).Append(';');
                }

                replaced = true;
                continue;
            }

            builder.Append(name).Append(':').Append(value).Append(';');
        }

        if (!replaced && hasValue)
        {
            builder.Append(propertyName).Append(':').Append(propertyValue).Append(';');
        }

        return builder.ToString();
    }

    private static HtmlElement? TryGetDocumentElement(HtmlDocument document)
    {
        HtmlElementCollection elements = document.GetElementsByTagName("html");
        if (elements.Count > 0)
        {
            return elements[0];
        }

        if (document.Body is HtmlElement body)
        {
            return body.Parent ?? body;
        }

        return null;
    }

    private static void ApplyScrollbarTheme(HtmlElement? element, ThemePalette palette)
    {
        if (element is null)
        {
            return;
        }

        if (!palette.IsDark)
        {
            ApplyCssDeclaration(element, "scrollbar-base-color", null);
            ApplyCssDeclaration(element, "scrollbar-face-color", null);
            ApplyCssDeclaration(element, "scrollbar-track-color", null);
            ApplyCssDeclaration(element, "scrollbar-arrow-color", null);
            ApplyCssDeclaration(element, "scrollbar-highlight-color", null);
            ApplyCssDeclaration(element, "scrollbar-3dlight-color", null);
            ApplyCssDeclaration(element, "scrollbar-shadow-color", null);
            ApplyCssDeclaration(element, "scrollbar-darkshadow-color", null);
            return;
        }

        Color face = palette.ControlBackground;
        Color track = palette.Background;
        Color arrow = palette.InputForeground;
        Color highlight = Lighten(face, 0.2);
        Color light = Lighten(face, 0.35);
        Color shadow = Darken(face, 0.25);
        Color darkShadow = Darken(face, 0.45);

        ApplyCssDeclaration(element, "scrollbar-base-color", ToCssColor(face));
        ApplyCssDeclaration(element, "scrollbar-face-color", ToCssColor(face));
        ApplyCssDeclaration(element, "scrollbar-track-color", ToCssColor(track));
        ApplyCssDeclaration(element, "scrollbar-arrow-color", ToCssColor(arrow));
        ApplyCssDeclaration(element, "scrollbar-highlight-color", ToCssColor(highlight));
        ApplyCssDeclaration(element, "scrollbar-3dlight-color", ToCssColor(light));
        ApplyCssDeclaration(element, "scrollbar-shadow-color", ToCssColor(shadow));
        ApplyCssDeclaration(element, "scrollbar-darkshadow-color", ToCssColor(darkShadow));
    }

    private static string ToCssColor(Color color)
    {
        return $"#{color.R:X2}{color.G:X2}{color.B:X2}";
    }

    private static void EnsureColorSchemeMeta(HtmlDocument document, bool dark)
    {
        string content = dark ? "dark light" : "light dark";
        HtmlElementCollection metas = document.GetElementsByTagName("meta");
        for (int i = 0; i < metas.Count; i++)
        {
            HtmlElement meta = metas[i];
            if (string.Equals(meta.GetAttribute("name"), "color-scheme", StringComparison.OrdinalIgnoreCase))
            {
                meta.SetAttribute("content", content);
                return;
            }
        }

        HtmlElementCollection heads = document.GetElementsByTagName("head");
        if (heads.Count == 0)
        {
            return;
        }

        HtmlElement head = heads[0];
        HtmlElement newMeta = document.CreateElement("meta");
        if (newMeta is null)
        {
            return;
        }

        newMeta.SetAttribute("name", "color-scheme");
        newMeta.SetAttribute("content", content);
        head.AppendChild(newMeta);
    }

    private static Color Lighten(Color color, double amount)
    {
        int r = color.R + (int)((255 - color.R) * amount);
        int g = color.G + (int)((255 - color.G) * amount);
        int b = color.B + (int)((255 - color.B) * amount);
        return Color.FromArgb(Clamp(r), Clamp(g), Clamp(b));
    }

    private static Color Darken(Color color, double amount)
    {
        int r = color.R - (int)(color.R * amount);
        int g = color.G - (int)(color.G * amount);
        int b = color.B - (int)(color.B * amount);
        return Color.FromArgb(Clamp(r), Clamp(g), Clamp(b));
    }

    private static int Clamp(int value)
    {
        if (value < 0)
        {
            return 0;
        }

        if (value > 255)
        {
            return 255;
        }

        return value;
    }

    internal readonly struct ThemePalette
    {
        public ThemePalette(Color background, Color foreground, Color highlightBackground, Color highlightForeground, Color accent)
        {
            Background = background;
            Foreground = foreground;
            HighlightBackground = highlightBackground;
            HighlightForeground = highlightForeground;
            Accent = accent;

            IsDark = ComputeLuminance(background) < 128;
            ControlBackground = IsDark ? Lighten(background, 0.08) : Darken(background, 0.06);
            ControlBorder = IsDark ? Lighten(background, 0.16) : Darken(background, 0.16);
            InputBackground = IsDark ? Lighten(background, 0.12) : SystemColors.Window;
            InputForeground = IsDark ? foreground : SystemColors.WindowText;
        }

        public Color Background { get; }

        public Color Foreground { get; }

        public Color HighlightBackground { get; }

        public Color HighlightForeground { get; }

        public Color Accent { get; }

        public Color ControlBackground { get; }

        public Color ControlBorder { get; }

        public Color InputBackground { get; }

        public Color InputForeground { get; }

        public bool IsDark { get; }
    }

    private static class ThemeRenderer
    {
        public static void Attach(ToolStrip toolStrip, ThemePalette palette)
        {
            toolStrip.Renderer = new Renderer(palette);
            toolStrip.BackColor = palette.ControlBackground;
            toolStrip.ForeColor = palette.Foreground;

            foreach (ToolStripItem item in toolStrip.Items)
            {
                item.ForeColor = palette.Foreground;
            }
        }

        public static void Attach(ContextMenuStrip menu, ThemePalette palette)
        {
            menu.Renderer = new Renderer(palette);
            menu.BackColor = palette.ControlBackground;
            menu.ForeColor = palette.Foreground;

            foreach (ToolStripItem item in menu.Items)
            {
                item.ForeColor = palette.Foreground;
            }
        }

        private sealed class Renderer : ToolStripProfessionalRenderer
        {
            private readonly ThemePalette _palette;

            public Renderer(ThemePalette palette)
                : base(new ColorTable(palette))
            {
                _palette = palette;
                RoundedEdges = false;
            }

            protected override void OnRenderToolStripBackground(ToolStripRenderEventArgs e)
            {
                e.Graphics.Clear(_palette.ControlBackground);
            }

            protected override void OnRenderMenuItemBackground(ToolStripItemRenderEventArgs e)
            {
                PaintItemBackground(e);
            }

            protected override void OnRenderButtonBackground(ToolStripItemRenderEventArgs e)
            {
                PaintItemBackground(e);
            }

            protected override void OnRenderItemText(ToolStripItemTextRenderEventArgs e)
            {
                e.TextColor = e.Item.Enabled ? _palette.Foreground : ControlPaint.Light(_palette.Foreground);
                base.OnRenderItemText(e);
            }

            protected override void OnRenderSeparator(ToolStripSeparatorRenderEventArgs e)
            {
                using var pen = new Pen(_palette.ControlBorder);
                var rect = e.Item.Bounds;
                if (e.Item is ToolStripSeparator separator && separator.IsOnDropDown)
                {
                    int y = rect.Height / 2;
                    e.Graphics.DrawLine(pen, rect.Left + 4, y, rect.Right - 4, y);
                }
                else
                {
                    int x = rect.Width / 2;
                    e.Graphics.DrawLine(pen, x, rect.Top + 4, x, rect.Bottom - 4);
                }
            }

            private void PaintItemBackground(ToolStripItemRenderEventArgs e)
            {
                var rect = new Rectangle(Point.Empty, e.Item.Size);
                Color background = e.Item.Selected || e.Item.Pressed
                    ? _palette.HighlightBackground
                    : _palette.ControlBackground;
                using var brush = new SolidBrush(background);
                e.Graphics.FillRectangle(brush, rect);
                if (e.Item.Selected || e.Item.Pressed)
                {
                    using var border = new Pen(_palette.ControlBorder);
                    e.Graphics.DrawRectangle(border, new Rectangle(0, 0, rect.Width - 1, rect.Height - 1));
                }
            }
        }

        private sealed class ColorTable : ProfessionalColorTable
        {
            private readonly ThemePalette _palette;

            public ColorTable(ThemePalette palette)
            {
                _palette = palette;
                UseSystemColors = false;
            }

            public override Color ToolStripDropDownBackground => _palette.ControlBackground;

            public override Color MenuBorder => _palette.ControlBorder;

            public override Color MenuItemBorder => _palette.ControlBorder;

            public override Color MenuItemSelected => _palette.HighlightBackground;

            public override Color MenuItemSelectedGradientBegin => _palette.HighlightBackground;

            public override Color MenuItemSelectedGradientEnd => _palette.HighlightBackground;

            public override Color ImageMarginGradientBegin => _palette.ControlBackground;

            public override Color ImageMarginGradientMiddle => _palette.ControlBackground;

            public override Color ImageMarginGradientEnd => _palette.ControlBackground;
        }
    }

    private static class NativeMethods
    {
        private const int DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20 = 19;
        private const int DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
        private const int DWMWA_BORDER_COLOR = 34;

        [DllImport("TextViewer.Spl", CallingConvention = CallingConvention.StdCall)]
        public static extern uint TextViewer_GetCurrentColor(int color);

        [DllImport("dwmapi.dll", PreserveSig = true)]
        private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attribute, ref int value, int size);

        [DllImport("TextViewer.Spl", CallingConvention = CallingConvention.StdCall)]
        private static extern void TextViewer_ApplyDarkModeTree(IntPtr hwnd);

        [DllImport("TextViewer.Spl", CallingConvention = CallingConvention.StdCall)]
        private static extern void TextViewer_SetDarkModeState([MarshalAs(UnmanagedType.Bool)] bool enabled);

        public static void SetDarkModeEnabled(bool enabled)
        {
            try
            {
                TextViewer_SetDarkModeState(enabled);
            }
            catch (DllNotFoundException)
            {
            }
            catch (EntryPointNotFoundException)
            {
            }
        }

        public static uint GetCurrentColor(int color)
        {
            try
            {
                return TextViewer_GetCurrentColor(color);
            }
            catch (DllNotFoundException)
            {
                return 0;
            }
            catch (EntryPointNotFoundException)
            {
                return 0;
            }
        }

        public static void ApplyImmersiveDarkMode(IntPtr handle, bool enable, Color borderColor)
        {
            if (handle == IntPtr.Zero)
            {
                return;
            }

            var version = Environment.OSVersion.Version;
            if (version.Major < 10)
            {
                return;
            }

            int useDark = enable ? 1 : 0;
            if (version.Build >= 18985)
            {
                DwmSetWindowAttribute(handle, DWMWA_USE_IMMERSIVE_DARK_MODE, ref useDark, sizeof(int));
            }
            else
            {
                DwmSetWindowAttribute(handle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20, ref useDark, sizeof(int));
            }

            if (version.Build >= 22000)
            {
                int border = enable ? borderColor.ToArgb() : -1;
                DwmSetWindowAttribute(handle, DWMWA_BORDER_COLOR, ref border, sizeof(int));
            }
        }

        public static void ApplyDarkModeTree(IntPtr handle)
        {
            if (handle == IntPtr.Zero)
            {
                return;
            }

            try
            {
                TextViewer_ApplyDarkModeTree(handle);
            }
            catch (DllNotFoundException)
            {
            }
            catch (EntryPointNotFoundException)
            {
            }
        }
    }
}
