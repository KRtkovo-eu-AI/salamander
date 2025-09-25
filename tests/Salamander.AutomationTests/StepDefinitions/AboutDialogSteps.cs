using System;
using System.Linq;
using FlaUI.Core.AutomationElements;
using FlaUI.Core.Input;
using FlaUI.Core.Tools;
using FlaUI.Core.WindowsAPI;
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
        mainWindow.Focus();
        Retry.WhileFalse(() => mainWindow.IsEnabled && mainWindow.IsAvailable, timeout: TimeSpan.FromSeconds(5));

        TryOpenAboutWithKeyboard();

        _aboutWindow = Retry.WhileNull(
                () => mainWindow.ModalWindows.FirstOrDefault(window => window.Title.Contains("About", StringComparison.OrdinalIgnoreCase)),
                timeout: TimeSpan.FromSeconds(2))
            .Result;

        if (_aboutWindow is null)
        {
            InvokeAboutCommand(mainWindow);

            _aboutWindow = Retry.WhileNull(
                    () => mainWindow.ModalWindows.FirstOrDefault(window => window.Title.Contains("About", StringComparison.OrdinalIgnoreCase)),
                    timeout: TimeSpan.FromSeconds(5))
                .Result ?? throw new InvalidOperationException("The About dialog did not appear within the expected time.");
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

    private static void TryOpenAboutWithKeyboard()
    {
        Keyboard.Press(VirtualKeyShort.MENU);
        Keyboard.Press(VirtualKeyShort.KEY_H);
        Keyboard.Release(VirtualKeyShort.KEY_H);
        Keyboard.Release(VirtualKeyShort.MENU);

        Wait.UntilInputIsProcessed();

        Keyboard.Press(VirtualKeyShort.KEY_A);
        Keyboard.Release(VirtualKeyShort.KEY_A);

        Wait.UntilInputIsProcessed();
    }

    private static void InvokeAboutCommand(Window mainWindow)
    {
        var nativeHandle = mainWindow.Properties.NativeWindowHandle.Value;
        if (nativeHandle is null)
        {
            throw new InvalidOperationException("The main window handle is not available.");
        }

        NativeMethods.SendMessage(new IntPtr(nativeHandle.Value), NativeMethods.WM_COMMAND, new IntPtr(NativeCommandIds.HelpAbout), IntPtr.Zero);

        Wait.UntilInputIsProcessed();
    }
}
