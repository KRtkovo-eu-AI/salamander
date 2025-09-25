using FlaUI.Core;
using FlaUI.Core.AutomationElements;
using FlaUI.Core.Tools;
using TechTalk.SpecFlow;

namespace Salamander.UiTests.Support;

[Binding]
public sealed class Hooks
{
    private readonly ScenarioContext _scenarioContext;

    public Hooks(ScenarioContext scenarioContext)
    {
        _scenarioContext = scenarioContext;
    }

    [AfterScenario]
    public void Cleanup()
    {
        if (_scenarioContext.TryGetValue("AboutWindow", out Window? aboutWindow))
        {
            try
            {
                if (!aboutWindow.IsOffscreen)
                {
                    aboutWindow.Close();
                    Wait.UntilInputIsProcessed();
                }
            }
            catch
            {
                // ignored - best effort cleanup
            }
        }

        if (_scenarioContext.TryGetValue("MainWindow", out Window? mainWindow))
        {
            try
            {
                if (!mainWindow.Patterns.Window.Pattern.Current.IsModal)
                {
                    mainWindow.Close();
                    Wait.UntilInputIsProcessed();
                }
            }
            catch
            {
                // ignored - best effort cleanup
            }
        }

        if (_scenarioContext.TryGetValue(nameof(Application), out Application? application))
        {
            try
            {
                if (!application.HasExited)
                {
                    application.Close();
                    application.WaitWhileMainHandleIsAlive(TimeSpan.FromSeconds(5));
                }
            }
            catch
            {
                // ignored - best effort cleanup
            }
        }

        if (_scenarioContext.TryGetValue(nameof(AutomationBase), out AutomationBase? automation))
        {
            automation.Dispose();
        }
    }
}
