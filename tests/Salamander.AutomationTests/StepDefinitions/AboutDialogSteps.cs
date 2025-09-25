using System;
using System.Linq;
using FlaUI.Core.AutomationElements;
using FlaUI.Core.Definitions;
using FlaUI.Core.Tools;
using NUnit.Framework;
using Reqnroll;

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
        var menuBar = mainWindow.FindMainMenu();

        var helpMenuItem = menuBar.FindMenuItem("Help")
                           ?? throw new InvalidOperationException("The Help menu could not be located.");
        helpMenuItem.ExpandMenuItem();

        var aboutMenuItem = Retry.WhileNull(
                () => helpMenuItem.FindMenuItem("About"),
                timeout: TimeSpan.FromSeconds(5))
            .Result ?? throw new InvalidOperationException("The About menu item could not be located.");

        aboutMenuItem.InvokeMenuItem();

        _aboutWindow = Retry.WhileNull(
                () => mainWindow.ModalWindows.FirstOrDefault(window => window.Title.Contains("About", StringComparison.OrdinalIgnoreCase)),
                timeout: TimeSpan.FromSeconds(5))
            .Result ?? throw new InvalidOperationException("The About dialog did not appear within the expected time.");
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
}

internal static class MenuElementExtensions
{
    private static readonly StringComparison MenuTextComparison = StringComparison.OrdinalIgnoreCase;

    public static Menu FindMainMenu(this Window window)
    {
        if (window == null)
        {
            throw new ArgumentNullException(nameof(window));
        }

        var mainMenu = window.FindFirstDescendant(cf => cf.ByControlType(ControlType.MenuBar))?.AsMenu()
                        ?? window.FindFirstDescendant(cf => cf.ByControlType(ControlType.Menu))?.AsMenu();

        return mainMenu ?? throw new InvalidOperationException("The main window does not contain a menu bar.");
    }

    public static MenuItem? FindMenuItem(this Menu menu, string nameFragment)
    {
        if (menu == null)
        {
            throw new ArgumentNullException(nameof(menu));
        }

        if (string.IsNullOrWhiteSpace(nameFragment))
        {
            throw new ArgumentException("A menu item name fragment must be provided.", nameof(nameFragment));
        }

        foreach (var item in menu.Items)
        {
            if (item.Name.Contains(nameFragment, MenuTextComparison))
            {
                return item;
            }
        }

        foreach (var item in menu.Items)
        {
            var match = item.FindMenuItemRecursive(nameFragment);
            if (match != null)
            {
                return match;
            }
        }

        return null;
    }

    public static MenuItem? FindMenuItem(this MenuItem menuItem, string nameFragment)
    {
        if (menuItem == null)
        {
            throw new ArgumentNullException(nameof(menuItem));
        }

        if (string.IsNullOrWhiteSpace(nameFragment))
        {
            throw new ArgumentException("A menu item name fragment must be provided.", nameof(nameFragment));
        }

        return menuItem.FindMenuItemRecursive(nameFragment);
    }

    public static void ExpandMenuItem(this MenuItem menuItem)
    {
        if (menuItem == null)
        {
            throw new ArgumentNullException(nameof(menuItem));
        }

        var expandCollapse = menuItem.Patterns.ExpandCollapse?.PatternOrDefault;
        if (expandCollapse != null && expandCollapse.ExpandCollapseState != ExpandCollapseState.Expanded)
        {
            expandCollapse.Expand();
        }
        else
        {
            menuItem.Click();
        }
    }

    public static void InvokeMenuItem(this MenuItem menuItem)
    {
        if (menuItem == null)
        {
            throw new ArgumentNullException(nameof(menuItem));
        }

        var invoke = menuItem.Patterns.Invoke?.PatternOrDefault;
        if (invoke != null)
        {
            invoke.Invoke();
        }
        else
        {
            menuItem.Click();
        }
    }

    private static MenuItem? FindMenuItemRecursive(this MenuItem menuItem, string nameFragment)
    {
        if (menuItem.Name.Contains(nameFragment, MenuTextComparison))
        {
            return menuItem;
        }

        menuItem.ExpandMenuItem();

        foreach (var child in menuItem.Items)
        {
            var match = child.FindMenuItemRecursive(nameFragment);
            if (match != null)
            {
                return match;
            }
        }

        return null;
    }
}
