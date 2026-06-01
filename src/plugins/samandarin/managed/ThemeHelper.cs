// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#nullable enable

using System;
using System.Drawing;
using System.Resources;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace OpenSalamander.Samandarin;

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
        if (!TryGetPalette(out var palette))
        {
            return;
        }

        ApplyPalette(form, palette);

        if (form.IsHandleCreated)
        {
            NativeMethods.ApplyImmersiveDarkMode(form.Handle, palette.IsDark, palette.ControlBorder);
        }

        form.HandleCreated += (_, _) =>
        {
            if (TryGetPalette(out var refreshed))
            {
                NativeMethods.ApplyImmersiveDarkMode(form.Handle, refreshed.IsDark, refreshed.ControlBorder);
            }
        };
    }

    public static void InvalidatePalette()
    {
        s_cachedPalette = null;
    }

    public static DialogResult ShowMessageBox(IWin32Window? owner, string text, string caption, MessageBoxButtons buttons, MessageBoxIcon icon)
    {
        if (!TryGetPalette(out _))
        {
            return MessageBox.Show(owner, text, caption, buttons, icon);
        }

        using var dialog = new Form
        {
            Text = caption,
            StartPosition = owner is null ? FormStartPosition.CenterScreen : FormStartPosition.CenterParent,
            FormBorderStyle = FormBorderStyle.FixedDialog,
            MaximizeBox = false,
            MinimizeBox = false,
            ShowInTaskbar = false,
            AutoSize = true,
            AutoSizeMode = AutoSizeMode.GrowAndShrink,
            Padding = new Padding(12),
        };

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            AutoSize = true,
            AutoSizeMode = AutoSizeMode.GrowAndShrink,
            ColumnCount = 2,
            RowCount = 2,
            Margin = Padding.Empty,
        };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));

        bool hasIcon = false;

        if (icon != MessageBoxIcon.None)
        {
            var image = GetMessageBoxIcon(icon);
            if (image is not null)
            {
                var picture = new PictureBox
                {
                    Image = image,
                    SizeMode = PictureBoxSizeMode.AutoSize,
                    Margin = new Padding(0, 0, 12, 0),
                };
                layout.Controls.Add(picture, 0, 0);
                hasIcon = true;
            }
        }

        var textLabel = new Label
        {
            AutoSize = true,
            MaximumSize = new Size(480, 0),
            Margin = Padding.Empty,
            Text = text,
        };
        var textColumn = hasIcon ? 1 : 0;
        layout.Controls.Add(textLabel, textColumn, 0);
        if (!hasIcon)
        {
            layout.SetColumnSpan(textLabel, 2);
        }

        var buttonsPanel = new FlowLayoutPanel
        {
            FlowDirection = FlowDirection.RightToLeft,
            AutoSize = true,
            AutoSizeMode = AutoSizeMode.GrowAndShrink,
            Dock = DockStyle.Fill,
            Padding = Padding.Empty,
            Margin = new Padding(0, 12, 0, 0),
            WrapContents = false,
        };
        layout.Controls.Add(buttonsPanel, 0, 1);
        layout.SetColumnSpan(buttonsPanel, 2);

        Button? defaultButton = null;
        Button? cancelButton = null;
        foreach (var definition in GetButtonDefinitions(buttons))
        {
            var button = new Button
            {
                Text = definition.text,
                DialogResult = definition.result,
                AutoSize = true,
            };
            button.Margin = buttonsPanel.Controls.Count == 0 ? Padding.Empty : new Padding(6, 0, 0, 0);
            buttonsPanel.Controls.Add(button);

            if (definition.isDefault && defaultButton is null)
            {
                defaultButton = button;
            }

            if (definition.isCancel)
            {
                cancelButton = button;
            }
        }

        dialog.AcceptButton = defaultButton;
        if (cancelButton is not null)
        {
            dialog.CancelButton = cancelButton;
        }

        dialog.Controls.Add(layout);
        ApplyTheme(dialog);

        var ownerWindow = owner;
        ownerWindow ??= Form.ActiveForm;
        if (ownerWindow is null && Application.OpenForms.Count > 0)
        {
            ownerWindow = Application.OpenForms[0];
        }

        return ownerWindow is null ? dialog.ShowDialog() : dialog.ShowDialog(ownerWindow);
    }

    private static bool TryGetPalette(out ThemePalette palette)
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
        }

        if (!(control is Button))
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

        var palette = GetPalette();
        if (palette is null)
        {
            comboBox.DrawMode = DrawMode.Normal;
            return;
        }

        e.DrawBackground();
        var background = (e.State & DrawItemState.Selected) != 0 ? palette.Value.HighlightBackground : palette.Value.InputBackground;
        var foreground = (e.State & DrawItemState.Selected) != 0 ? palette.Value.HighlightForeground : palette.Value.InputForeground;

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

    private static void ApplyPalette(Form form, ThemePalette palette)
    {
        form.BackColor = palette.Background;
        form.ForeColor = palette.Foreground;

        foreach (Control child in form.Controls)
        {
            ApplyPalette(child, palette);
        }
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
            ControlBackground = IsDark ? Lighten(background, 0.08) : SystemColors.Control;
            ControlBorder = IsDark ? Lighten(background, 0.16) : SystemColors.ControlDark;
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

    private static readonly ResourceManager s_resourceManager = new("System.Windows.Forms.SR", typeof(MessageBox).Assembly);

    private static Image? GetMessageBoxIcon(MessageBoxIcon icon)
    {
        return icon switch
        {
            MessageBoxIcon.Hand or MessageBoxIcon.Stop or MessageBoxIcon.Error => SystemIcons.Error.ToBitmap(),
            MessageBoxIcon.Question => SystemIcons.Question.ToBitmap(),
            MessageBoxIcon.Exclamation or MessageBoxIcon.Warning => SystemIcons.Warning.ToBitmap(),
            MessageBoxIcon.Asterisk or MessageBoxIcon.Information => SystemIcons.Information.ToBitmap(),
            _ => null,
        };
    }

    private static (DialogResult result, string text, bool isDefault, bool isCancel)[] GetButtonDefinitions(MessageBoxButtons buttons)
    {
        return buttons switch
        {
            MessageBoxButtons.OK => new[]
            {
                (DialogResult.OK, GetString("DialogResultOK", "OK"), true, true),
            },
            MessageBoxButtons.OKCancel => new[]
            {
                (DialogResult.Cancel, GetString("DialogResultCancel", "Cancel"), false, true),
                (DialogResult.OK, GetString("DialogResultOK", "OK"), true, false),
            },
            MessageBoxButtons.YesNo => new[]
            {
                (DialogResult.No, GetString("DialogResultNo", "No"), false, true),
                (DialogResult.Yes, GetString("DialogResultYes", "Yes"), true, false),
            },
            MessageBoxButtons.YesNoCancel => new[]
            {
                (DialogResult.Cancel, GetString("DialogResultCancel", "Cancel"), false, true),
                (DialogResult.No, GetString("DialogResultNo", "No"), false, false),
                (DialogResult.Yes, GetString("DialogResultYes", "Yes"), true, false),
            },
            MessageBoxButtons.RetryCancel => new[]
            {
                (DialogResult.Cancel, GetString("DialogResultCancel", "Cancel"), false, true),
                (DialogResult.Retry, GetString("DialogResultRetry", "Retry"), true, false),
            },
            MessageBoxButtons.AbortRetryIgnore => new[]
            {
                (DialogResult.Ignore, GetString("DialogResultIgnore", "Ignore"), false, true),
                (DialogResult.Retry, GetString("DialogResultRetry", "Retry"), false, false),
                (DialogResult.Abort, GetString("DialogResultAbort", "Abort"), true, false),
            },
            _ => new[]
            {
                (DialogResult.OK, GetString("DialogResultOK", "OK"), true, true),
            },
        };
    }

    private static string GetString(string name, string fallback)
    {
        try
        {
            return s_resourceManager.GetString(name) ?? fallback;
        }
        catch (MissingManifestResourceException)
        {
            return fallback;
        }
    }

    private static class ThemeRenderer
    {
        public static void Attach(ToolStrip dropDown, ThemePalette palette)
        {
            dropDown.Renderer = new Renderer(palette);
            dropDown.BackColor = palette.ControlBackground;
            dropDown.ForeColor = palette.Foreground;
            foreach (ToolStripItem item in dropDown.Items)
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
                RoundedEdges = false;
                _palette = palette;
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

        [DllImport("Samandarin.Spl", CallingConvention = CallingConvention.StdCall)]
        public static extern uint Samandarin_GetCurrentColor(int color);

        [DllImport("dwmapi.dll", PreserveSig = true)]
        private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attribute, ref int value, int size);

        public static uint GetCurrentColor(int color)
        {
            try
            {
                return Samandarin_GetCurrentColor(color);
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
