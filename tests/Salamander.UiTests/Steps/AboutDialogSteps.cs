using System.Diagnostics;
using FlaUI.Core;
using FlaUI.Core.AutomationElements;
using FlaUI.Core.Definitions;
using FlaUI.Core.Tools;
using FlaUI.UIA3;
using NUnit.Framework;
using Salamander.UiTests.Support;
using TechTalk.SpecFlow;

namespace Salamander.UiTests.Steps;

[Binding]
public class AboutDialogSteps
{
    private readonly ScenarioContext _scenarioContext;
    private Application? _application;
    private AutomationBase? _automation;
    private Window? _mainWindow;
    private Window? _aboutWindow;

    public AboutDialogSteps(ScenarioContext scenarioContext)
    {
        _scenarioContext = scenarioContext;
    }

    [Given("the Salamander application is started")]
    public void GivenTheSalamanderApplicationIsStarted()
    {
        var applicationPath = TestConfiguration.ResolveApplicationPath();
        _application = Application.Launch(new ProcessStartInfo(applicationPath)
        {
            WorkingDirectory = Path.GetDirectoryName(applicationPath)
        });
        _automation = new UIA3Automation();
        _mainWindow = _application.GetMainWindow(_automation, TimeSpan.FromSeconds(15));

        Assert.That(_mainWindow, Is.Not.Null, "The Salamander main window could not be located.");

        _scenarioContext.Set(_application, nameof(Application));
        _scenarioContext.Set(_automation, nameof(AutomationBase));
        _scenarioContext.Set(_mainWindow, "MainWindow");
    }

    [When("I open the About dialog")]
    public void WhenIOpenTheAboutDialog()
    {
        _mainWindow ??= _scenarioContext.TryGetValue("MainWindow", out Window? window)
            ? window
            : throw new InvalidOperationException("The main window is not available in the scenario context.");

        var aboutMenuItem = _mainWindow.FindAllDescendants(cf => cf.ByControlType(ControlType.MenuItem))
            .FirstOrDefault(item => item.Name.Contains("About", StringComparison.OrdinalIgnoreCase));

        Assert.That(aboutMenuItem, Is.Not.Null, "Unable to locate a menu item with 'About' in its name.");

        aboutMenuItem.Invoke();
    }

    [Then("the About dialog is displayed")]
    public void ThenTheAboutDialogIsDisplayed()
    {
        _application ??= _scenarioContext.TryGetValue(nameof(Application), out Application? application)
            ? application
            : throw new InvalidOperationException("The application reference was not found in the scenario context.");
        _automation ??= _scenarioContext.TryGetValue(nameof(AutomationBase), out AutomationBase? automation)
            ? automation
            : throw new InvalidOperationException("The automation reference was not found in the scenario context.");

        var aboutDialogResult = Retry.WhileNull(
            () => _application.GetAllTopLevelWindows(_automation)
                .FirstOrDefault(window => window.Title.Contains("About", StringComparison.OrdinalIgnoreCase)),
            timeout: TimeSpan.FromSeconds(5),
            throwOnTimeout: true);

        _aboutWindow = aboutDialogResult.Result;

        Assert.That(_aboutWindow, Is.Not.Null, "The About dialog did not appear within the expected time.");

        _scenarioContext.Set(_aboutWindow, "AboutWindow");
    }

    [When("I close the About dialog")]
    public void WhenICloseTheAboutDialog()
    {
        _aboutWindow ??= _scenarioContext.TryGetValue("AboutWindow", out Window? window)
            ? window
            : throw new InvalidOperationException("The About dialog reference is not available in the scenario context.");

        _aboutWindow.Close();
        Wait.UntilInputIsProcessed();
    }

    [When("I exit the Salamander application")]
    public void WhenIExitTheSalamanderApplication()
    {
        _mainWindow ??= _scenarioContext.TryGetValue("MainWindow", out Window? window)
            ? window
            : throw new InvalidOperationException("The main window is not available in the scenario context.");

        _mainWindow.Close();
        Wait.UntilInputIsProcessed();
    }

    [Then("the Salamander application is closed")]
    public void ThenTheSalamanderApplicationIsClosed()
    {
        _application ??= _scenarioContext.TryGetValue(nameof(Application), out Application? application)
            ? application
            : throw new InvalidOperationException("The application reference was not found in the scenario context.");

        var exitResult = Retry.WhileFalse(() => _application.HasExited, timeout: TimeSpan.FromSeconds(10));
        Assert.That(exitResult.Success, Is.True, "The Salamander process is still running after requesting it to close.");
    }
}
