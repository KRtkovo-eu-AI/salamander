// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;

namespace OpenSalamander.ManagedBootstrap;

public static class EntryPoint
{
    // ICLRRuntimeHost.ExecuteInDefaultAppDomain requires this exact signature.
    public static int Configure(string argument)
    {
        const string frameworkName = ".NETFramework,Version=v4.8";

        // A native CLR host has no managed entry assembly, so the default
        // AppDomain otherwise has no TargetFrameworkName. WinForms reads and
        // caches this value when System.Windows.Forms is first initialized.
        AppDomain.CurrentDomain.SetData("TargetFrameworkName", frameworkName);

        AppContext.SetSwitch(
            "Switch.System.Windows.Forms.DoNotSupportDpiChanges", false);
        AppContext.SetSwitch(
            "Switch.System.Windows.Forms.EnableDpiChangedMessageHandling", true);
        AppContext.SetSwitch(
            "Switch.System.Windows.Forms.EnableDpiChangedHighDpiImprovements", true);
        AppContext.SetSwitch(
            "Switch.System.Windows.Forms.EnableWindowsFormsHighDpiAutoResizing", true);

        // AppContext.TargetFrameworkName may already have cached null in a
        // native host. WinForms ConfigurationOptions reads SetupInformation
        // directly, so validate the same source that WinForms consumes.
        return string.Equals(
            AppDomain.CurrentDomain.SetupInformation.TargetFrameworkName,
            frameworkName,
            StringComparison.Ordinal) ? 0 : 1;
    }
}
