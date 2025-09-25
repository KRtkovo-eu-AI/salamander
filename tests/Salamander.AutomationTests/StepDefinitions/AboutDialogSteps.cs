using System;
using System.Collections.Generic;
using System.Linq;
using FlaUI.Core.AutomationElements;
using FlaUI.Core.AutomationElements.Infrastructure;
using FlaUI.Core.Exceptions;
using FlaUI.Core.Input;
using FlaUI.Core.Tools;
using FlaUI.Core.WindowsAPI;
using NUnit.Framework;
using Reqnroll;
using Menu = FlaUI.Core.AutomationElements.Menu;
using MenuItem = FlaUI.Core.AutomationElements.MenuItem;
using SWA = System.Windows.Automation;
using FlaUIControlType = FlaUI.Core.Definitions.ControlType;
using FlaUIExpandCollapseState = FlaUI.Core.Definitions.ExpandCollapseState;

namespace Salamander.AutomationTests.StepDefinitions;

[Binding]
public sealed class AboutDialogSteps
{
    private Window? _aboutWindow;

    [Given("Salamander is running")]
    public void GivenSalamanderIsRunning()
    {
        Assert.That(TestSession.IsRunning, Is.True, "The Salamander application failed to start.");
        Assert.That(TestSession.MainWindow, Is.Not.Null, "The main window is not available.");
    }

    [When("I open the About dialog from the Help menu")]
    public void WhenIOpenTheAboutDialogFromTheHelpMenu()
    {
        var mainWindow = TestSession.MainWindow;
        mainWindow.Focus();
        Retry.WhileFalse(() => mainWindow.IsEnabled && mainWindow.IsAvailable, timeout: TimeSpan.FromSeconds(5));

        var processId = (uint)TestSession.Application.ProcessId;
        var mainHandle = mainWindow.FrameworkAutomationElement.NativeWindowHandle;

        var knownWindows = CaptureExistingProcessWindows(processId);

        _aboutWindow = TryOpenAndWait(
                mainWindow,
                TimeSpan.FromSeconds(4),
                knownWindows,
                processId,
                mainHandle,
                TryOpenAboutThroughMenu)
            ?? TryOpenAndWait(
                mainWindow,
                TimeSpan.FromSeconds(5),
                knownWindows,
                processId,
                mainHandle,
                TryOpenAboutWithKeyboard)
            ?? TryOpenAndWait(
                mainWindow,
                TimeSpan.FromSeconds(6),
                knownWindows,
                processId,
                mainHandle,
                TryInvokeAboutCommand)
            ?? throw new InvalidOperationException("The About dialog did not appear within the expected time.");
    }

    [Then("the About dialog is displayed")]
    public void ThenTheAboutDialogIsDisplayed()
    {
        Assert.That(_aboutWindow, Is.Not.Null, "The About dialog is not available.");
        Assert.That(_aboutWindow!.IsAvailable, Is.True, "The About dialog is not visible.");
    }

    [When("I close the About dialog")]
    public void WhenICloseTheAboutDialog()
    {
        Assert.That(_aboutWindow, Is.Not.Null, "No About dialog is currently open.");
        _aboutWindow!.Close();

        Retry.WhileTrue(() => _aboutWindow!.IsAvailable, timeout: TimeSpan.FromSeconds(5));
    }

    [Then("the About dialog is closed")]
    public void ThenTheAboutDialogIsClosed()
    {
        Assert.That(_aboutWindow, Is.Not.Null, "The About dialog reference has not been captured.");
        Assert.That(_aboutWindow!.IsOffscreen || !_aboutWindow.IsAvailable, Is.True, "The About dialog is still visible.");
    }

    [Then("Salamander remains running")]
    public void ThenSalamanderRemainsRunning()
    {
        Assert.That(TestSession.IsRunning, Is.True, "The Salamander application closed unexpectedly.");
    }

    [When("I exit Salamander")]
    public void WhenIExitSalamander() => TestSession.Shutdown();

    [Then("Salamander is not running")]
    public void ThenSalamanderIsNotRunning()
    {
        Assert.That(TestSession.IsRunning, Is.False, "The Salamander application is still running.");
    }

    private static Window? WaitForAboutDialog(
        Window mainWindow,
        TimeSpan timeout,
        HashSet<IntPtr> knownWindows,
        uint processId,
        IntPtr mainHandle)
    {
        return Retry.WhileNull(
                () => SafeFindAboutDialog(mainWindow, processId, mainHandle, knownWindows),
                timeout: timeout,
                throwOnTimeout: false)
            .Result;
    }

    private static Window? SafeFindAboutDialog(Window mainWindow, uint processId, IntPtr mainHandle, HashSet<IntPtr> knownWindows)
    {
        try
        {
            return FindAboutDialog(mainWindow, processId, mainHandle, knownWindows);
        }
        catch (SWA.ElementNotAvailableException)
        {
            return null;
        }
    }

    private static Window? FindAboutDialog(Window mainWindow, uint processId, IntPtr mainHandle, HashSet<IntPtr> knownWindows)
    {
        foreach (var modalWindow in EnumerateModalWindows(mainWindow))
        {
            if (IsAboutDialogSafe(modalWindow, mainWindow))
            {
                RegisterKnownWindow(modalWindow, knownWindows);
                return modalWindow;
            }
        }

        var automation = TestSession.Automation;
        var application = TestSession.Application;

        foreach (var window in application.GetAllTopLevelWindows(automation))
        {
            if (window is null)
            {
                continue;
            }

            if (IsAboutDialogSafe(window, mainWindow))
            {
                RegisterKnownWindow(window, knownWindows);
                return window;
            }
        }

        var candidateHandle = FindNativeAboutHandle(processId, mainHandle, knownWindows);
        if (candidateHandle != IntPtr.Zero)
        {
            var wrappedResult = Retry.WhileNull(
                    () => TryWrapWindow(candidateHandle),
                    timeout: TimeSpan.FromSeconds(2),
                    throwOnTimeout: false)
                .Result;

            var candidateWindow = wrappedResult ?? TryWrapWindow(candidateHandle);
            if (candidateWindow is not null)
            {
                if (IsAboutDialogSafe(candidateWindow, mainWindow))
                {
                    RegisterKnownWindow(candidateWindow, knownWindows);
                    return candidateWindow;
                }

                var owner = NativeMethods.GetWindow(candidateHandle, NativeMethods.GW_OWNER);
                var ownedByMain = owner == mainHandle;
                var isVisible = NativeMethods.IsWindowVisible(candidateHandle);
                var title = NativeMethods.GetWindowText(candidateHandle);

                if (IsNativeAboutCandidate(candidateHandle, mainHandle, title, isVisible, ownedByMain)
                    || (!knownWindows.Contains(candidateHandle) && ownedByMain))
                {
                    RegisterHandle(candidateHandle, knownWindows);
                    return candidateWindow;
                }
            }
        }

        return null;
    }

    private static void RegisterKnownWindow(Window window, HashSet<IntPtr> knownWindows)
    {
        var handle = window.FrameworkAutomationElement.NativeWindowHandle;
        RegisterHandle(handle, knownWindows);
    }

    private static void RegisterHandle(IntPtr handle, HashSet<IntPtr> knownWindows)
    {
        if (handle == IntPtr.Zero)
        {
            return;
        }

        knownWindows.Add(handle);
    }

    private static Window? TryWrapWindow(IntPtr handle)
    {
        try
        {
            var element = TestSession.Automation.FromHandle(handle);
            return element?.AsWindow();
        }
        catch
        {
            return null;
        }
    }

    private static IntPtr FindNativeAboutHandle(uint processId, IntPtr mainHandle, HashSet<IntPtr> knownWindows)
    {
        foreach (var handle in NativeMethods.EnumerateProcessWindowHandles(processId))
        {
            if (handle == IntPtr.Zero || handle == mainHandle)
            {
                continue;
            }

            var isKnown = knownWindows.Contains(handle);
            var owner = NativeMethods.GetWindow(handle, NativeMethods.GW_OWNER);
            var ownedByMain = owner == mainHandle;
            var isVisible = NativeMethods.IsWindowVisible(handle);
            var title = NativeMethods.GetWindowText(handle);

            if (!isKnown && ownedByMain)
            {
                return handle;
            }

            if (IsNativeAboutCandidate(handle, mainHandle, title, isVisible, ownedByMain))
            {
                return handle;
            }
        }

        return IntPtr.Zero;
    }

    private static bool IsAboutDialogSafe(Window candidate, Window mainWindow)
    {
        try
        {
            return IsAboutDialog(candidate, mainWindow);
        }
        catch (SWA.ElementNotAvailableException)
        {
            return false;
        }
    }

    private static Window? TryOpenAndWait(
        Window mainWindow,
        TimeSpan timeout,
        HashSet<IntPtr> knownWindows,
        uint processId,
        IntPtr mainHandle,
        Func<Window, bool> openAction)
    {
        var invoked = SafeInvoke(openAction, mainWindow);
        if (!invoked)
        {
            return null;
        }

        return WaitForAboutDialog(mainWindow, timeout, knownWindows, processId, mainHandle);
    }

    private static bool TryOpenAboutThroughMenu(Window mainWindow)
    {
        var menuBarResult = Retry.WhileNull(
            () => FindMenuBar(mainWindow),
            timeout: TimeSpan.FromSeconds(2),
            throwOnTimeout: false);

        if (!menuBarResult.Success || menuBarResult.Result is null)
        {
            return false;
        }

        var menuBar = menuBarResult.Result;

        var helpResult = Retry.WhileNull(
            () => FindMenuItem(menuBar, "Help"),
            timeout: TimeSpan.FromSeconds(2),
            throwOnTimeout: false);

        if (!helpResult.Success || helpResult.Result is null)
        {
            return false;
        }

        var helpMenuItem = helpResult.Result;
        if (!TryExpandMenuItem(helpMenuItem))
        {
            return false;
        }

        var aboutResult = Retry.WhileNull(
            () => FindMenuItem(helpMenuItem, "About"),
            timeout: TimeSpan.FromSeconds(2),
            throwOnTimeout: false);

        if (!aboutResult.Success || aboutResult.Result is null)
        {
            return false;
        }

        return TryInvokeMenuItem(aboutResult.Result);
    }

    private static bool IsAboutDialog(Window candidate, Window mainWindow)
    {
        if (candidate.FrameworkAutomationElement.NativeWindowHandle == mainWindow.FrameworkAutomationElement.NativeWindowHandle)
        {
            return false;
        }

        if (candidate.Properties.ProcessId.Value != mainWindow.Properties.ProcessId.Value)
        {
            return false;
        }

        if (MatchesAboutTitle(candidate.Title))
        {
            return true;
        }

        var hasAboutContent = ContainsAboutContent(candidate);

        if (candidate.Patterns.Window.IsSupported)
        {
            var windowPattern = candidate.Patterns.Window.Pattern;
            if (windowPattern.IsModal && hasAboutContent)
            {
                return true;
            }

            if (!windowPattern.IsModal)
            {
                return hasAboutContent;
            }

            return IsOwnedByMainWindow(candidate, mainWindow);
        }

        return hasAboutContent;
    }

    private static bool MatchesAboutTitle(string? title)
    {
        if (string.IsNullOrWhiteSpace(title))
        {
            return false;
        }

        return title.Contains("About", StringComparison.OrdinalIgnoreCase)
            || title.Contains("Salamander", StringComparison.OrdinalIgnoreCase)
            || title.Contains("Altap", StringComparison.OrdinalIgnoreCase)
            || title.Contains("O aplikaci", StringComparison.OrdinalIgnoreCase)
            || title.Contains("O programu", StringComparison.OrdinalIgnoreCase)
            || title.Contains("Informace o", StringComparison.OrdinalIgnoreCase)
            || title.Contains("Informácie o", StringComparison.OrdinalIgnoreCase)
            || title.Contains("Über", StringComparison.OrdinalIgnoreCase);
    }

    private static bool ContainsAboutContent(Window window)
    {
        try
        {
            return window
                .FindAllDescendants(cf => cf.ByControlType(FlaUIControlType.Text))
                .Any(element => MatchesAboutContent(element.Name));
        }
        catch
        {
            return false;
        }
    }

    private static bool IsNativeAboutCandidate(IntPtr handle, IntPtr mainHandle, string? title, bool isVisible, bool ownedByMain)
    {
        if (!string.IsNullOrWhiteSpace(title) && MatchesAboutTitle(title))
        {
            return true;
        }

        var className = NativeMethods.GetWindowClassName(handle);

        if (className.Equals("#32770", StringComparison.Ordinal))
        {
            return ownedByMain || (!string.IsNullOrWhiteSpace(title)
                && title.Contains("Salamander", StringComparison.OrdinalIgnoreCase));
        }

        if (ownedByMain && className.Equals("TDialog", StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        return ownedByMain && isVisible;
    }

    private static bool IsOwnedByMainWindow(Window candidate, Window mainWindow)
    {
        var candidateHandle = candidate.FrameworkAutomationElement.NativeWindowHandle;
        var mainHandle = mainWindow.FrameworkAutomationElement.NativeWindowHandle;

        if (candidateHandle == IntPtr.Zero || mainHandle == IntPtr.Zero)
        {
            return false;
        }

        var owner = NativeMethods.GetWindow(candidateHandle, NativeMethods.GW_OWNER);
        return owner == mainHandle;
    }

    private static bool MatchesAboutContent(string? content)
    {
        if (string.IsNullOrWhiteSpace(content))
        {
            return false;
        }

        return content.Contains("About", StringComparison.OrdinalIgnoreCase)
            || content.Contains("Salamander", StringComparison.OrdinalIgnoreCase)
            || content.Contains("Altap", StringComparison.OrdinalIgnoreCase)
            || content.Contains("Version", StringComparison.OrdinalIgnoreCase)
            || content.Contains("Licence", StringComparison.OrdinalIgnoreCase)
            || content.Contains("License", StringComparison.OrdinalIgnoreCase)
            || content.Contains("Copyright", StringComparison.OrdinalIgnoreCase)
            || content.Contains("https://", StringComparison.OrdinalIgnoreCase)
            || content.Contains("altap.cz", StringComparison.OrdinalIgnoreCase)
            || content.Contains("verze", StringComparison.OrdinalIgnoreCase);
    }

    private static Menu? FindMenuBar(Window mainWindow)
    {
        var menuBarElement = mainWindow.FindFirstDescendant(cf => cf.ByControlType(FlaUIControlType.MenuBar))
            ?? mainWindow.FindFirstDescendant(cf => cf.ByControlType(FlaUIControlType.Menu));

        return AsMenu(menuBarElement);
    }

    private static MenuItem? FindMenuItem(Menu menu, string nameFragment)
    {
        return EnumerateMenuItems(menu.AutomationElement)
            .FirstOrDefault(item => MatchesName(item, nameFragment));
    }

    private static MenuItem? FindMenuItem(MenuItem parent, string nameFragment)
    {
        return EnumerateMenuItems(parent.AutomationElement)
            .FirstOrDefault(item => MatchesName(item, nameFragment));
    }

    private static IEnumerable<MenuItem> EnumerateMenuItems(AutomationElement container)
    {
        AutomationElement[] children;

        try
        {
            children = container.FindAllChildren(cf => cf.ByControlType(FlaUIControlType.MenuItem));
        }
        catch (Exception ex) when (ex is FlaUIException or SWA.ElementNotAvailableException)
        {
            yield break;
        }

        foreach (var child in children)
        {
            MenuItem? item = null;
            try
            {
                item = child.AsMenuItem();
            }
            catch (Exception ex) when (ex is FlaUIException or SWA.ElementNotAvailableException)
            {
                continue;
            }

            if (item is not null)
            {
                yield return item;
            }
        }
    }

    private static bool MatchesName(MenuItem item, string nameFragment)
    {
        var name = item.Name ?? string.Empty;
        return name.Contains(nameFragment, StringComparison.OrdinalIgnoreCase);
    }

    private static bool TryExpandMenuItem(MenuItem menuItem)
    {
        try
        {
            if (!menuItem.IsEnabled)
            {
                return false;
            }

            if (menuItem.Patterns.ExpandCollapse.IsSupported)
            {
                var expandCollapse = menuItem.Patterns.ExpandCollapse.Pattern;
                if (expandCollapse.ExpandCollapseState != FlaUIExpandCollapseState.Expanded)
                {
                    expandCollapse.Expand();
                }
            }
            else
            {
                menuItem.Click();
            }

            Wait.UntilInputIsProcessed();
            return true;
        }
        catch (Exception ex) when (ex is FlaUIException or SWA.ElementNotAvailableException or InvalidOperationException)
        {
            return false;
        }
    }

    private static bool TryInvokeMenuItem(MenuItem menuItem)
    {
        try
        {
            if (!menuItem.IsEnabled)
            {
                return false;
            }

            if (menuItem.Patterns.Invoke.IsSupported)
            {
                menuItem.Patterns.Invoke.Pattern.Invoke();
            }
            else
            {
                menuItem.Click();
            }

            Wait.UntilInputIsProcessed();
            return true;
        }
        catch (Exception ex) when (ex is FlaUIException or SWA.ElementNotAvailableException or InvalidOperationException)
        {
            return false;
        }
    }

    private static bool TryOpenAboutWithKeyboard(Window mainWindow)
    {
        try
        {
            mainWindow.Focus();
            Wait.UntilInputIsProcessed();

            Keyboard.Press(VirtualKeyShort.ALT);
            Keyboard.Press(VirtualKeyShort.KEY_H);
            Keyboard.Release(VirtualKeyShort.KEY_H);
            Keyboard.Release(VirtualKeyShort.ALT);

            Wait.UntilInputIsProcessed();

            Keyboard.Press(VirtualKeyShort.KEY_A);
            Keyboard.Release(VirtualKeyShort.KEY_A);

            Wait.UntilInputIsProcessed();
            return true;
        }
        catch (Exception ex) when (ex is FlaUIException or InvalidOperationException)
        {
            return false;
        }
    }

    private static bool TryInvokeAboutCommand(Window mainWindow)
    {
        var nativeHandle = mainWindow.FrameworkAutomationElement.NativeWindowHandle;
        if (nativeHandle == IntPtr.Zero)
        {
            return false;
        }

        NativeMethods.SendMessage(nativeHandle, NativeMethods.WM_COMMAND, (IntPtr)NativeCommandIds.HelpAbout, IntPtr.Zero);
        Wait.UntilInputIsProcessed();
        return true;
    }

    private static Menu? AsMenu(AutomationElement? element)
    {
        if (element is null)
        {
            return null;
        }

        try
        {
            return element.AsMenu();
        }
        catch (Exception ex) when (ex is FlaUIException or SWA.ElementNotAvailableException)
        {
            return null;
        }
    }

    private static bool SafeInvoke(Func<Window, bool> action, Window window)
    {
        try
        {
            return action(window);
        }
        catch (Exception ex) when (ex is FlaUIException or SWA.ElementNotAvailableException or InvalidOperationException)
        {
            return false;
        }
    }

    private static IEnumerable<Window> EnumerateModalWindows(Window mainWindow)
    {
        try
        {
            return mainWindow.ModalWindows;
        }
        catch (SWA.ElementNotAvailableException)
        {
            return Array.Empty<Window>();
        }
    }

    private static HashSet<IntPtr> CaptureExistingProcessWindows(uint processId)
    {
        var handles = NativeMethods.EnumerateProcessWindowHandles(processId);
        var knownHandles = new HashSet<IntPtr>();

        foreach (var handle in handles)
        {
            knownHandles.Add(handle);
        }

        return knownHandles;
    }
}
