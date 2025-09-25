using System;

namespace Salamander.AutomationTests;

internal static class NativeCommandIds
{
    private static readonly Lazy<int> HelpAboutLazy = new(() => ResourceHeaderIds.GetValue("CM_HELP_ABOUT"));

    public static int HelpAbout => HelpAboutLazy.Value;
}
