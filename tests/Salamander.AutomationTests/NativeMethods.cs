using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Salamander.AutomationTests;

internal static class NativeMethods
{
    public const int WM_COMMAND = 0x0111;
    public const uint GW_OWNER = 4;

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr GetWindow(IntPtr hWnd, uint uCmd);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    private static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    private static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    public static IReadOnlyList<IntPtr> EnumerateProcessWindowHandles(uint processId)
    {
        var handles = new List<IntPtr>();
        EnumWindows((hWnd, _) =>
        {
            GetWindowThreadProcessId(hWnd, out var windowProcessId);
            if (windowProcessId == processId)
            {
                handles.Add(hWnd);
            }

            return true;
        }, IntPtr.Zero);

        return handles;
    }

    public static string GetWindowClassName(IntPtr hWnd)
    {
        var builder = new StringBuilder(256);
        _ = GetClassName(hWnd, builder, builder.Capacity);
        return builder.ToString();
    }

    public static string GetWindowText(IntPtr hWnd)
    {
        var builder = new StringBuilder(512);
        _ = GetWindowText(hWnd, builder, builder.Capacity);
        return builder.ToString();
    }
}
