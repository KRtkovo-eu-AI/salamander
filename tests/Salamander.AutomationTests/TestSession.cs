using System;
using FlaUI.Core;
using FlaUI.Core.AutomationElements;
using FlaUI.Core.Tools;
using FlaUI.UIA3;

namespace Salamander.AutomationTests;

/// <summary>
/// Manages the lifecycle of the Salamander application under test.
/// </summary>
public static class TestSession
{
    private static readonly TimeSpan DefaultTimeout = TimeSpan.FromSeconds(10);

    private static Application? _application;
    private static UIA3Automation? _automation;
    private static Window? _mainWindow;

    public static bool IsRunning => _application is { HasExited: false };

    public static Application Application => _application ?? throw new InvalidOperationException("The application has not been started yet.");

    public static UIA3Automation Automation => _automation ?? throw new InvalidOperationException("The UI Automation instance is not available.");

    public static Window MainWindow => _mainWindow ?? throw new InvalidOperationException("The main window is not available.");

    /// <summary>
    /// Starts the Salamander application if it is not already running.
    /// </summary>
    public static void Start()
    {
        if (_application is { HasExited: false })
        {
            return;
        }

        var executablePath = TestConfiguration.ResolveApplicationPath();
        _application = Application.Launch(executablePath);
        _automation = new UIA3Automation();
        _mainWindow = Retry.WhileNull(
                () => _application.GetMainWindow(_automation, DefaultTimeout),
                timeout: DefaultTimeout)
            .Result ?? throw new InvalidOperationException("Failed to locate the Salamander main window.");
    }

    /// <summary>
    /// Attempts to close the Salamander application and releases UI Automation resources.
    /// </summary>
    public static void Shutdown()
    {
        try
        {
            if (_mainWindow is { IsAvailable: true })
            {
                _mainWindow.Close();
                WaitForExit();
            }
            else if (_application is { HasExited: false })
            {
                _application.Close();
                WaitForExit();
            }
        }
        finally
        {
            _mainWindow = null;

            _automation?.Dispose();
            _automation = null;

            _application?.Dispose();
            _application = null;
        }
    }

    /// <summary>
    /// Waits for the Salamander process to exit.
    /// </summary>
    public static void WaitForExit()
    {
        if (_application is null)
        {
            return;
        }

        Retry.WhileTrue(() => !_application.HasExited, timeout: DefaultTimeout);
    }
}
