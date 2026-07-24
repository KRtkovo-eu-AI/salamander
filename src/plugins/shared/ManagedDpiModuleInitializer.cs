// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;

namespace System.Runtime.CompilerServices
{
    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    internal sealed class ModuleInitializerAttribute : Attribute
    {
    }
}

namespace OpenSalamander
{
    internal static class ManagedDpiModuleInitializer
    {
        [System.Runtime.CompilerServices.ModuleInitializer]
        internal static void Initialize()
        {
            ManagedApplication.InitializeDpiPolicyAtModuleLoad();
            ManagedDpiTrace.Write("ModuleInitializer");
        }
    }
}
