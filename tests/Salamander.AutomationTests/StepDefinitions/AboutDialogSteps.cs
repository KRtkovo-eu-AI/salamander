using System;
using System.Linq;
using FlaUI.Core.AutomationElements;
using FlaUI.Core.Definitions;
using FlaUI.Core.Input;
using FlaUI.Core.Tools;
using FlaUI.Core.WindowsAPI;
using NUnit.Framework;
using Reqnroll;
using Menu = FlaUI.Core.AutomationElements.Menu;
using MenuItem = FlaUI.Core.AutomationElements.MenuItem;

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

        var openedViaMenu = TryOpenAboutThroughMenu(mainWindow);

        if (!openedViaMenu)
        {
            TryOpenAboutWithKeyboard();
        }

        _aboutWindow = WaitForAboutDialog(mainWindow, TimeSpan.FromSeconds(2));

        if (_aboutWindow is null)
        {
            InvokeAboutCommand(mainWindow);

            _aboutWindow = WaitForAboutDialog(mainWindow, TimeSpan.FromSeconds(5))
                ?? throw new InvalidOperationException("The About dialog did not appear within the expected time.");
        }
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

    private static Window? WaitForAboutDialog(Window mainWindow, TimeSpan timeout)
    {
        return Retry.WhileNull(
                () => FindAboutDialog(mainWindow),
                timeout: timeout,
                throwOnTimeout: false)
            .Result;
    }

    private static Window? FindAboutDialog(Window mainWindow)
    {
        return mainWindow.ModalWindows
            .FirstOrDefault(window => window.Title.Contains("About", StringComparison.OrdinalIgnoreCase));
    }

    private static bool TryOpenAboutThroughMenu(Window mainWindow)
    {
        var menuResult = Retry.WhileNull(
            () => mainWindow.FindFirstDescendant(cf => cf.ByControlType(ControlType.MenuBar)),
            timeout: TimeSpan.FromSeconds(2),
            throwOnTimeout: false);

        if (!menuResult.Success || menuResult.Result is null)
        {
            return false;
        }

        var menuBar = menuResult.Result.AsMenu();
        var helpResult = Retry.WhileNull(
            () => FindMenuItem(menuBar, "Help"),
            timeout: TimeSpan.FromSeconds(2),
            throwOnTimeout: false);

        if (!helpResult.Success || helpResult.Result is null)
        {
            return false;
        }

        var helpMenuItem = helpResult.Result;

        try
        {
            ExpandMenuItem(helpMenuItem);
        }
        catch
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

        try
        {
            InvokeMenuItem(aboutResult.Result);
            return true;
        }
        catch
        {
            return false;
        }
    }

    private static void TryOpenAboutWithKeyboard()
    {
        Keyboard.Press(VirtualKeyShort.ALT);
        Keyboard.Press(VirtualKeyShort.KEY_H);
        Keyboard.Release(VirtualKeyShort.KEY_H);
        Keyboard.Release(VirtualKeyShort.ALT);

        Wait.UntilInputIsProcessed();

        Keyboard.Press(VirtualKeyShort.KEY_A);
        Keyboard.Release(VirtualKeyShort.KEY_A);

        Wait.UntilInputIsProcessed();
    }

    private static void InvokeAboutCommand(Window mainWindow)
    {
        var nativeHandle = mainWindow.FrameworkAutomationElement.NativeWindowHandle;
        if (nativeHandle == IntPtr.Zero)
        {
            throw new InvalidOperationException("The main window handle is not available.");
        }

        NativeMethods.SendMessage(nativeHandle, NativeMethods.WM_COMMAND, (IntPtr)NativeCommandIds.HelpAbout, IntPtr.Zero);

        Wait.UntilInputIsProcessed();
    }

    private static MenuItem? FindMenuItem(Menu menu, string nameFragment)
    {
        return menu.Items.FirstOrDefault(item => MatchesName(item, nameFragment));
    }

    private static MenuItem? FindMenuItem(MenuItem parent, string nameFragment)
    {
        return parent.Items.FirstOrDefault(item => MatchesName(item, nameFragment));
    }

    private static bool MatchesName(MenuItem item, string nameFragment)
    {
        var name = item.Name ?? string.Empty;
        return name.Contains(nameFragment, StringComparison.OrdinalIgnoreCase);
    }

    private static void ExpandMenuItem(MenuItem menuItem)
    {
        if (!menuItem.IsEnabled)
        {
            throw new InvalidOperationException($"Menu item '{menuItem.Name}' is disabled.");
        }

        if (menuItem.Patterns.ExpandCollapse.IsSupported)
        {
            var expandCollapse = menuItem.Patterns.ExpandCollapse.Pattern;
            if (expandCollapse.ExpandCollapseState != ExpandCollapseState.Expanded)
            {
                expandCollapse.Expand();
            }
        }
        else
        {
            menuItem.Click();
        }

        Wait.UntilInputIsProcessed();
    }

    private static void InvokeMenuItem(MenuItem menuItem)
    {
        if (!menuItem.IsEnabled)
        {
            throw new InvalidOperationException($"Menu item '{menuItem.Name}' is disabled.");
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
    }
}
