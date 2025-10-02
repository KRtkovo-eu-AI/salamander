// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Drawing;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace EPocalipse.Json.Viewer
{
    internal static class ThemeHelper
    {
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
            var palette = GetPalette();
            if (palette is null)
            {
                return;
            }

            ApplyToControl(form, palette.Value);
            ApplyFormChrome(form, palette.Value);

            form.HandleCreated += (_, _) =>
            {
                var refreshed = GetPalette();
                if (refreshed.HasValue)
                {
                    NativeMethods.ApplyImmersiveDarkMode(form.Handle, refreshed.Value.IsDark, refreshed.Value.ControlBorder);
                    ApplyFormChrome(form, refreshed.Value);
                }
            };

            if (form.IsHandleCreated)
            {
                NativeMethods.ApplyImmersiveDarkMode(form.Handle, palette.Value.IsDark, palette.Value.ControlBorder);
            }
        }

        public static void ApplyTheme(Control root)
        {
            if (root is Form form)
            {
                ApplyTheme(form);
                return;
            }

            var palette = GetPalette();
            if (palette is null)
            {
                return;
            }

            ApplyToControl(root, palette.Value);
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

        private static void ApplyToControl(Control control, ThemePalette palette)
        {
            bool backgroundSet = false;
            bool foregroundSet = false;

            switch (control)
            {
                case Form formControl:
                    formControl.ForeColor = palette.Foreground;
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case JsonViewer viewer:
                    viewer.BackColor = palette.Background;
                    viewer.ForeColor = palette.Foreground;
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case Panel panel when control is not TabPage:
                    panel.BackColor = palette.Background;
                    panel.ForeColor = palette.Foreground;
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case SplitContainer split:
                    split.BackColor = palette.ControlBackground;
                    split.ForeColor = palette.Foreground;
                    split.Panel1.BackColor = palette.Background;
                    split.Panel1.ForeColor = palette.Foreground;
                    split.Panel2.BackColor = palette.Background;
                    split.Panel2.ForeColor = palette.Foreground;
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case TextBoxBase textBox:
                    textBox.BackColor = palette.InputBackground;
                    textBox.ForeColor = palette.InputForeground;
                    textBox.BorderStyle = BorderStyle.FixedSingle;
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case ComboBox comboBox:
                    comboBox.BackColor = palette.InputBackground;
                    comboBox.ForeColor = palette.InputForeground;
                    comboBox.DrawMode = DrawMode.OwnerDrawFixed;
                    comboBox.DrawItem -= ComboBoxOnDrawItem;
                    comboBox.DrawItem += ComboBoxOnDrawItem;
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case ListView listView:
                    listView.BackColor = palette.InputBackground;
                    listView.ForeColor = palette.InputForeground;
                    listView.OwnerDraw = false;
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case TreeView treeView:
                    treeView.BackColor = palette.InputBackground;
                    treeView.ForeColor = palette.InputForeground;
                    treeView.LineColor = palette.Accent;
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case PropertyGrid grid:
                    ApplyToPropertyGrid(grid, palette);
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case DataGridView dataGrid:
                    ApplyToDataGridView(dataGrid, palette);
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case ToolStrip toolStrip:
                    ThemeRenderer.Attach(toolStrip, palette);
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case TabControl tabControl:
                    PrepareTabControl(tabControl);
                    ApplyToTabControl(tabControl, palette);
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case LinkLabel linkLabel:
                    linkLabel.LinkColor = palette.Accent;
                    linkLabel.ActiveLinkColor = palette.HighlightForeground;
                    linkLabel.VisitedLinkColor = palette.Accent;
                    foregroundSet = true;
                    break;
                case Button button when palette.IsDark:
                    button.FlatStyle = FlatStyle.Flat;
                    button.BackColor = palette.ControlBackground;
                    button.ForeColor = palette.Foreground;
                    button.FlatAppearance.BorderColor = palette.ControlBorder;
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
            }

            if (!backgroundSet && control is not ToolStrip)
            {
                control.BackColor = palette.Background;
                backgroundSet = true;
            }

            if (!foregroundSet)
            {
                control.ForeColor = palette.Foreground;
            }

            if (control.ContextMenuStrip is ContextMenuStrip contextual)
            {
                ThemeRenderer.Attach(contextual, palette);
            }

            control.ControlAdded -= ControlOnControlAdded;
            control.ControlAdded += ControlOnControlAdded;

            foreach (Control child in control.Controls)
            {
                ApplyToControl(child, palette);
            }
        }

        private static void ApplyFormChrome(Form form, ThemePalette palette)
        {
            if (form.Padding != Padding.Empty)
            {
                form.Padding = Padding.Empty;
            }

            if (form.BackColor != palette.Background)
            {
                form.BackColor = palette.Background;
            }

            if (form.ForeColor != palette.Foreground)
            {
                form.ForeColor = palette.Foreground;
            }

            form.Paint -= FormOnPaint;
            form.Paint += FormOnPaint;
        }

        private static void FormOnPaint(object? sender, PaintEventArgs e)
        {
            if (sender is not Form form)
            {
                return;
            }

            var palette = GetPalette();
            if (!palette.HasValue)
            {
                form.Paint -= FormOnPaint;
                return;
            }

            var size = form.ClientSize;
            if (size.Width <= 1 || size.Height <= 1)
            {
                return;
            }

            var bounds = new Rectangle(0, 0, size.Width - 1, size.Height - 1);
            using var border = new Pen(palette.Value.ControlBorder);
            e.Graphics.DrawRectangle(border, bounds);
        }

        private static void PrepareTabControl(TabControl tabControl)
        {
            var setStyle = typeof(Control).GetMethod("SetStyle", BindingFlags.Instance | BindingFlags.NonPublic);
            var updateStyles = typeof(Control).GetMethod("UpdateStyles", BindingFlags.Instance | BindingFlags.NonPublic);
            var doubleBuffered = typeof(Control).GetProperty("DoubleBuffered", BindingFlags.Instance | BindingFlags.NonPublic);

            const ControlStyles styles = ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer;

            setStyle?.Invoke(tabControl, new object[] { styles, true });
            doubleBuffered?.SetValue(tabControl, true, null);
            updateStyles?.Invoke(tabControl, null);

            tabControl.HandleCreated -= TabControlOnHandleCreatedApplyTheme;
            tabControl.HandleCreated += TabControlOnHandleCreatedApplyTheme;

            if (tabControl.IsHandleCreated)
            {
                NativeMethods.DisableVisualStyles(tabControl.Handle);
            }
        }

        private static void ControlOnControlAdded(object? sender, ControlEventArgs e)
        {
            var palette = GetPalette();
            if (palette.HasValue)
            {
                ApplyToControl(e.Control, palette.Value);
            }
        }

        private static void ApplyToPropertyGrid(PropertyGrid grid, ThemePalette palette)
        {
            grid.BackColor = palette.Background;
            grid.ForeColor = palette.Foreground;
            grid.ViewBackColor = palette.InputBackground;
            grid.ViewForeColor = palette.InputForeground;
            grid.ViewBorderColor = palette.ControlBorder;
            grid.HelpBackColor = palette.Background;
            grid.HelpForeColor = palette.Foreground;
            grid.HelpBorderColor = palette.ControlBorder;
            grid.CommandsBackColor = palette.ControlBackground;
            grid.CommandsForeColor = palette.Foreground;
            grid.CategoryForeColor = palette.Accent;
            grid.CategorySplitterColor = palette.ControlBorder;
            grid.LineColor = palette.ControlBorder;
        }

        private static void ApplyToDataGridView(DataGridView grid, ThemePalette palette)
        {
            grid.BackgroundColor = palette.Background;
            grid.GridColor = palette.ControlBorder;
            grid.DefaultCellStyle.BackColor = palette.InputBackground;
            grid.DefaultCellStyle.ForeColor = palette.InputForeground;
            grid.DefaultCellStyle.SelectionBackColor = palette.HighlightBackground;
            grid.DefaultCellStyle.SelectionForeColor = palette.HighlightForeground;
            grid.ColumnHeadersDefaultCellStyle.BackColor = palette.ControlBackground;
            grid.ColumnHeadersDefaultCellStyle.ForeColor = palette.Foreground;
            grid.EnableHeadersVisualStyles = false;
        }

        private static void ApplyToTabControl(TabControl tabControl, ThemePalette palette)
        {
            tabControl.DrawMode = TabDrawMode.OwnerDrawFixed;
            tabControl.Appearance = TabAppearance.Normal;
            tabControl.SizeMode = TabSizeMode.Normal;
            var desiredPadding = new Point(16, 6);
            if (tabControl.Padding != desiredPadding)
            {
                tabControl.Padding = desiredPadding;
            }
            tabControl.Paint -= TabControlOnPaint;
            tabControl.Paint += TabControlOnPaint;
            tabControl.SelectedIndexChanged -= TabControlOnSelectedIndexChanged;
            tabControl.SelectedIndexChanged += TabControlOnSelectedIndexChanged;
            tabControl.BackColor = palette.ControlBackground;
            tabControl.ForeColor = palette.Foreground;

            foreach (TabPage tabPage in tabControl.TabPages)
            {
                ApplyToTabPage(tabPage, palette);
            }

            tabControl.ControlAdded -= TabControlOnControlAdded;
            tabControl.ControlAdded += TabControlOnControlAdded;
            tabControl.Invalidate();
        }

        private static void ApplyToTabPage(TabPage tabPage, ThemePalette palette)
        {
            tabPage.UseVisualStyleBackColor = false;
            tabPage.BackColor = palette.Background;
            tabPage.ForeColor = palette.Foreground;
        }

        private static void TabControlOnControlAdded(object? sender, ControlEventArgs e)
        {
            if (sender is not TabControl || e.Control is not TabPage tabPage)
            {
                return;
            }

            var palette = GetPalette();
            if (palette.HasValue)
            {
                ApplyToTabPage(tabPage, palette.Value);
                if (sender is TabControl tabControl)
                {
                    tabControl.Invalidate();
                }
            }
        }

        private static void TabControlOnSelectedIndexChanged(object? sender, EventArgs e)
        {
            if (sender is TabControl tabControl)
            {
                tabControl.Invalidate();
            }
        }

        private static void TabControlOnHandleCreatedApplyTheme(object? sender, EventArgs e)
        {
            if (sender is not TabControl tabControl)
            {
                return;
            }

            NativeMethods.DisableVisualStyles(tabControl.Handle);
        }

        private static void ComboBoxOnDrawItem(object? sender, DrawItemEventArgs e)
        {
            if (sender is not ComboBox comboBox)
            {
                return;
            }

            var palette = GetPalette();
            if (palette is null)
            {
                comboBox.DrawMode = DrawMode.Normal;
                return;
            }

            var bounds = e.Bounds;
            Color background = (e.State & DrawItemState.Selected) != 0 ? palette.Value.HighlightBackground : palette.Value.InputBackground;
            Color foreground = (e.State & DrawItemState.Selected) != 0 ? palette.Value.HighlightForeground : palette.Value.InputForeground;

            using (var brush = new SolidBrush(background))
            {
                e.Graphics.FillRectangle(brush, bounds);
            }

            string text = e.Index >= 0 && e.Index < comboBox.Items.Count
                ? Convert.ToString(comboBox.Items[e.Index]) ?? string.Empty
                : comboBox.Text;

            using (var textBrush = new SolidBrush(foreground))
            {
                e.Graphics.DrawString(text, comboBox.Font, textBrush, bounds);
            }

            e.DrawFocusRectangle();
        }

        private static void TabControlOnPaint(object? sender, PaintEventArgs e)
        {
            if (sender is not TabControl tabControl)
            {
                return;
            }

            var palette = GetPalette();
            if (palette is null)
            {
                tabControl.Paint -= TabControlOnPaint;
                tabControl.Invalidate();
                return;
            }

            var client = tabControl.ClientRectangle;
            if (client.Width <= 0 || client.Height <= 0)
            {
                return;
            }

            e.Graphics.Clear(palette.Value.ControlBackground);

            int stripHeight = tabControl.DisplayRectangle.Top;
            if (stripHeight < 0)
            {
                stripHeight = 0;
            }

            if (stripHeight > 0)
            {
                using var strip = new SolidBrush(palette.Value.TabStripBackground);
                e.Graphics.FillRectangle(strip, new Rectangle(0, 0, client.Width, stripHeight));
            }

            for (int i = 0; i < tabControl.TabPages.Count; i++)
            {
                var tabPage = tabControl.TabPages[i];
                var tabRect = tabControl.GetTabRect(i);
                if (tabRect.Width <= 0 || tabRect.Height <= 0)
                {
                    continue;
                }

                DrawTab(e.Graphics, tabControl, tabPage, tabRect, palette.Value, i == tabControl.SelectedIndex);
            }

            using (var body = new SolidBrush(palette.Value.Background))
            {
                var rect = tabControl.DisplayRectangle;
                rect.Inflate(1, 1);
                rect.Intersect(new Rectangle(0, 0, client.Width, client.Height));
                if (rect.Width > 0 && rect.Height > 0)
                {
                    e.Graphics.FillRectangle(body, rect);
                }
            }

            using (var border = new Pen(palette.Value.TabBorder))
            {
                e.Graphics.DrawRectangle(border, new Rectangle(0, 0, client.Width - 1, client.Height - 1));
            }
        }

        private static void DrawTab(Graphics graphics, TabControl tabControl, TabPage tabPage, Rectangle tabRect, ThemePalette palette, bool isSelected)
        {
            var bounds = tabRect;
            bounds.Inflate(-1, -1);
            if (bounds.Width <= 0 || bounds.Height <= 0)
            {
                bounds = tabRect;
            }

            var textBounds = Rectangle.Inflate(bounds, -8, -2);
            if (textBounds.Width <= 0 || textBounds.Height <= 0)
            {
                textBounds = bounds;
            }

            Color background = isSelected ? palette.TabActiveBackground : palette.TabInactiveBackground;
            Color foreground = isSelected ? palette.TabActiveForeground : palette.TabInactiveForeground;
            if (!tabPage.Enabled)
            {
                foreground = ControlPaint.Light(foreground);
            }

            using (var brush = new SolidBrush(background))
            {
                graphics.FillRectangle(brush, bounds);
            }

            var format = TextFormatFlags.HorizontalCenter |
                         TextFormatFlags.VerticalCenter |
                         TextFormatFlags.EndEllipsis |
                         TextFormatFlags.NoPrefix;
            var font = tabPage.Font ?? tabControl.Font;
            TextRenderer.DrawText(graphics, tabPage.Text, font, textBounds, foreground, format);

            using var border = new Pen(palette.TabBorder);
            if (bounds.Width > 0 && bounds.Height > 0)
            {
                graphics.DrawRectangle(border, bounds);
            }
        }

        private static int ComputeLuminance(Color color)
        {
            return (color.R * 30 + color.G * 59 + color.B * 11) / 100;
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

        private static Color Blend(Color from, Color to, double amount)
        {
            int r = from.R + (int)((to.R - from.R) * amount);
            int g = from.G + (int)((to.G - from.G) * amount);
            int b = from.B + (int)((to.B - from.B) * amount);
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
                AccentEmphasis = Blend(background, accent, IsDark ? 0.45 : 0.2);
                TabStripBackground = ControlBackground;
                TabInactiveBackground = ControlBackground;
                TabInactiveForeground = foreground;
                TabActiveBackground = HighlightBackground;
                TabActiveForeground = HighlightForeground;
                TabBorder = ControlBorder;
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

            public Color AccentEmphasis { get; }

            public Color TabStripBackground { get; }

            public Color TabInactiveBackground { get; }

            public Color TabInactiveForeground { get; }

            public Color TabActiveBackground { get; }

            public Color TabActiveForeground { get; }

            public Color TabBorder { get; }
        }

        private static class ThemeRenderer
        {
            public static void Attach(ToolStrip toolStrip, ThemePalette palette)
            {
                toolStrip.Renderer = new Renderer(palette);
                toolStrip.BackColor = palette.ControlBackground;
                toolStrip.ForeColor = palette.Foreground;
                toolStrip.GripStyle = ToolStripGripStyle.Hidden;
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
            }

            private sealed class Renderer : ToolStripProfessionalRenderer
            {
                private readonly ThemePalette _palette;

                public Renderer(ThemePalette palette)
                    : base(new ColorTable(palette))
                {
                    RoundedEdges = false;
                    _palette = palette;
                }

                protected override void OnRenderToolStripBackground(ToolStripRenderEventArgs e)
                {
                    e.Graphics.Clear(_palette.ControlBackground);
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

                protected override void OnRenderMenuItemBackground(ToolStripItemRenderEventArgs e)
                {
                    RenderItemBackground(e);
                }

                protected override void OnRenderButtonBackground(ToolStripItemRenderEventArgs e)
                {
                    RenderItemBackground(e);
                }

                private void RenderItemBackground(ToolStripItemRenderEventArgs e)
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

            [DllImport("JsonViewer.Spl", CallingConvention = CallingConvention.StdCall)]
            public static extern uint JsonViewer_GetCurrentColor(int color);

            [DllImport("dwmapi.dll", PreserveSig = true)]
            private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attribute, ref int value, int size);

            [DllImport("uxtheme.dll", CharSet = CharSet.Unicode, SetLastError = true)]
            private static extern int SetWindowTheme(IntPtr hwnd, string? appName, string? idList);

            public static uint GetCurrentColor(int color)
            {
                try
                {
                    return JsonViewer_GetCurrentColor(color);
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

            public static void DisableVisualStyles(IntPtr handle)
            {
                if (handle == IntPtr.Zero)
                {
                    return;
                }

                try
                {
                    SetWindowTheme(handle, string.Empty, string.Empty);
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
}
