// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Drawing;
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

        private static ThemePalette? s_cachedPalette;

        public static void ApplyTheme(Form form)
        {
            var palette = GetPalette();
            if (palette is null)
            {
                return;
            }

            ApplyToControl(form, palette.Value);

            form.HandleCreated += (_, _) =>
            {
                var refreshed = GetPalette();
                if (refreshed.HasValue)
                {
                    NativeMethods.ApplyImmersiveDarkMode(form.Handle, refreshed.Value.IsDark, refreshed.Value.ControlBorder);
                }
            };

            if (form.IsHandleCreated)
            {
                NativeMethods.ApplyImmersiveDarkMode(form.Handle, palette.Value.IsDark, palette.Value.ControlBorder);
            }
        }

        public static void ApplyTheme(Control root)
        {
            var palette = GetPalette();
            if (palette is null)
            {
                return;
            }

            ApplyToControl(root, palette.Value);
        }

        private static ThemePalette? GetPalette()
        {
            if (s_cachedPalette.HasValue)
            {
                return s_cachedPalette.Value;
            }

            try
            {
                uint background = NativeMethods.GetCurrentColor(SALCOL_ITEM_BK_NORMAL);
                uint foreground = NativeMethods.GetCurrentColor(SALCOL_ITEM_FG_NORMAL);
                uint highlightBackground = NativeMethods.GetCurrentColor(SALCOL_ITEM_BK_SELECTED);
                uint highlightForeground = NativeMethods.GetCurrentColor(SALCOL_ITEM_FG_SELECTED);
                uint accent = NativeMethods.GetCurrentColor(SALCOL_HOT_PANEL);

                if (background == 0 && foreground == 0)
                {
                    return s_cachedPalette = new ThemePalette(SystemColors.Control,
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

                s_cachedPalette = palette;
                return palette;
            }
            catch (DllNotFoundException)
            {
            }
            catch (EntryPointNotFoundException)
            {
            }

            return s_cachedPalette = null;
        }

        private static void ApplyToControl(Control control, ThemePalette palette)
        {
            bool backgroundSet = false;
            bool foregroundSet = false;

            switch (control)
            {
                case Form:
                    backgroundSet = true;
                    foregroundSet = true;
                    control.BackColor = palette.Background;
                    control.ForeColor = palette.Foreground;
                    break;
                case Panel panel:
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
                    ApplyToTabControl(tabControl, palette);
                    backgroundSet = true;
                    foregroundSet = true;
                    break;
                case TabPage tabPage:
                    tabPage.BackColor = palette.Background;
                    tabPage.ForeColor = palette.Foreground;
                    tabPage.UseVisualStyleBackColor = false;
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
            tabControl.DrawItem -= TabControlOnDrawItem;
            tabControl.DrawItem += TabControlOnDrawItem;
            tabControl.Paint -= TabControlOnPaint;
            tabControl.Paint += TabControlOnPaint;
            tabControl.BackColor = palette.ControlBackground;
            tabControl.ForeColor = palette.Foreground;
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

        private static void TabControlOnDrawItem(object? sender, DrawItemEventArgs e)
        {
            if (sender is not TabControl tabControl)
            {
                return;
            }

            var palette = GetPalette();
            if (palette is null)
            {
                tabControl.DrawMode = TabDrawMode.Normal;
                return;
            }

            bool isSelected = (e.State & DrawItemState.Selected) != 0;
            Color background = isSelected ? palette.Value.HighlightBackground : palette.Value.ControlBackground;
            Color foreground = isSelected ? palette.Value.HighlightForeground : palette.Value.Foreground;

            using (var brush = new SolidBrush(background))
            {
                e.Graphics.FillRectangle(brush, e.Bounds);
            }

            using (var textBrush = new SolidBrush(foreground))
            {
                string text = tabControl.TabPages[e.Index].Text;
                var layout = new StringFormat { Alignment = StringAlignment.Center, LineAlignment = StringAlignment.Center };
                e.Graphics.DrawString(text, tabControl.Font, textBrush, e.Bounds, layout);
            }

            if (isSelected)
            {
                using var border = new Pen(palette.Value.ControlBorder);
                var rect = e.Bounds;
                rect.Width -= 1;
                rect.Height -= 1;
                e.Graphics.DrawRectangle(border, rect);
            }
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

            var stripHeight = tabControl.DisplayRectangle.Top;
            if (stripHeight < 0)
            {
                stripHeight = 0;
            }

            using (var strip = new SolidBrush(palette.Value.ControlBackground))
            {
                var rect = new Rectangle(0, 0, client.Width, stripHeight);
                if (rect.Height > 0)
                {
                    e.Graphics.FillRectangle(strip, rect);
                }
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

            using (var border = new Pen(palette.Value.ControlBorder))
            {
                e.Graphics.DrawRectangle(border, new Rectangle(0, 0, client.Width - 1, client.Height - 1));
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

        private readonly struct ThemePalette
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
                    int border = enable ? ColorTranslator.ToWin32(borderColor) : -1;
                    DwmSetWindowAttribute(handle, DWMWA_BORDER_COLOR, ref border, sizeof(int));
                }
            }
        }
    }
}
