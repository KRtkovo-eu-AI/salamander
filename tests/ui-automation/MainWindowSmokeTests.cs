// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-10 | Add Taskscape source references to the test header. | Identify the UI test structure sources; https://github.com/taskscape/FileManager/blob/main/tests/FileManager.UiTests/BasicUiTests.cs and https://github.com/taskscape/FileManager/blob/main/tests/FileManager.UiTests/Infrastructure/FileManagerUiTestBase.cs. |  | GPT-6 (Codex) |
// | 2026-11-10 | Add a FlaUI/UIA3 main-window smoke test. | Confirm the native Salamander window is discoverable through UI Automation. |  | GPT-6 (Codex) |

using System.Diagnostics;
using FlaUI.Core;
using FlaUI.Core.AutomationElements;
using FlaUI.UIA3;
using NUnit.Framework;

namespace Samandarin.UiTests;

[TestFixture]
[NonParallelizable]
[Category("UI")]
public sealed class MainWindowSmokeTests
{
    private const string MainWindowClassName = "SalamanderMainWindowVer25";
    private Application? application;
    private UIA3Automation? automation;

    [SetUp]
    public void StartApplication()
    {
        Assert.That(Environment.GetEnvironmentVariable("SAMANDARIN_UI_DISPOSABLE_PROFILE"),
            Is.EqualTo("YES"),
            "UI tests require the runner's explicit disposable-profile confirmation.");

        var executable = Environment.GetEnvironmentVariable("SAMANDARIN_UI_EXE");
        Assert.That(executable, Is.Not.Null.And.Not.Empty,
            "Set SAMANDARIN_UI_EXE to the salamand.exe built from this checkout.");
        Assert.That(File.Exists(executable), Is.True,
            $"The selected Salamander executable does not exist: {executable}");

        var existingInstances = Process.GetProcessesByName("salamand");
        try
        {
            Assert.That(existingInstances, Is.Empty,
                "Close all Salamander instances first. Salamander is single-instance and could forward this launch to another profile.");
        }
        finally
        {
            foreach (var process in existingInstances)
                process.Dispose();
        }

        automation = new UIA3Automation();
        var startInfo = new ProcessStartInfo(Path.GetFullPath(executable!))
        {
            UseShellExecute = false,
            WorkingDirectory = Path.GetDirectoryName(Path.GetFullPath(executable!))!,
        };
        application = Application.Launch(startInfo);
    }

    [Test]
    public void Main_window_is_visible_and_accessible_through_uia3()
    {
        Assert.That(application, Is.Not.Null);
        Assert.That(automation, Is.Not.Null);

        var deadline = DateTime.UtcNow.AddSeconds(45);
        Window? mainWindow = null;
        while (DateTime.UtcNow < deadline && application is { HasExited: false })
        {
            mainWindow = application.GetAllTopLevelWindows(automation!)
                .FirstOrDefault(window =>
                    string.Equals(window.Properties.ClassName.ValueOrDefault,
                        MainWindowClassName, StringComparison.Ordinal));
            if (mainWindow is not null)
                break;

            Thread.Sleep(100);
        }

        Assert.That(mainWindow, Is.Not.Null,
            "UIA3 did not find the Salamander main window. If this is a fresh profile, start Salamander once manually and complete its first-run language dialog.");
        Assert.Multiple(() =>
        {
            Assert.That(mainWindow!.Properties.NativeWindowHandle.Value, Is.Not.EqualTo(nint.Zero));
            Assert.That(mainWindow.IsEnabled, Is.True);
            Assert.That(mainWindow.BoundingRectangle.Width, Is.GreaterThan(0));
            Assert.That(mainWindow.BoundingRectangle.Height, Is.GreaterThan(0));
        });
    }

    [TearDown]
    public void StopApplication()
    {
        try
        {
            if (application is { HasExited: false })
                application.Kill();
        }
        finally
        {
            application?.Dispose();
            automation?.Dispose();
        }
    }
}
