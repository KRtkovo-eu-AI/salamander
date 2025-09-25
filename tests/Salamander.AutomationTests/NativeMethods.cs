using System;
using System.Runtime.InteropServices;

namespace Salamander.AutomationTests;

internal static class NativeMethods
{
    public const int WM_COMMAND = 0x0111;

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);
}
