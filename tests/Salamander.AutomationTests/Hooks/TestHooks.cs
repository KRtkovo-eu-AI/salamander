using Reqnroll;

namespace Salamander.AutomationTests.Hooks;

[Binding]
public sealed class TestHooks
{
    [BeforeScenario(Order = 0)]
    public void StartApplication() => TestSession.Start();

    [AfterScenario(Order = 100)]
    public void StopApplication() => TestSession.Shutdown();
}
