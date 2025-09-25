using System;
using System.IO;

namespace Salamander.AutomationTests;

/// <summary>
/// Provides configuration values for the UI automation tests.
/// </summary>
public static class TestConfiguration
{
    private const string ApplicationPathEnvironmentVariable = "SALAMANDER_APP_PATH";
    private static readonly string[] DefaultExecutableCandidates =
    {
        Path.Combine("..", "..", "..", "..", "src", "vcxproj", "build", "Salamand.exe"),
        Path.Combine("..", "..", "..", "..", "src", "vcxproj", "build", "bin", "Salamand.exe"),
        Path.Combine("..", "..", "..", "..", "bin", "Salamand.exe")
    };

    /// <summary>
    /// Resolves the path to the Salamander executable to be tested.
    /// </summary>
    /// <returns>The full path to the executable.</returns>
    /// <exception cref="FileNotFoundException">Thrown when the executable cannot be located.</exception>
    public static string ResolveApplicationPath()
    {
        var environmentOverride = Environment.GetEnvironmentVariable(ApplicationPathEnvironmentVariable);
        if (!string.IsNullOrWhiteSpace(environmentOverride))
        {
            var normalized = Path.GetFullPath(environmentOverride);
            if (File.Exists(normalized))
            {
                return normalized;
            }

            throw new FileNotFoundException($"The path specified in the {ApplicationPathEnvironmentVariable} environment variable does not exist.", normalized);
        }

        var baseDirectory = AppContext.BaseDirectory;
        foreach (var relativePath in DefaultExecutableCandidates)
        {
            var candidate = Path.GetFullPath(Path.Combine(baseDirectory, relativePath));
            if (File.Exists(candidate))
            {
                return candidate;
            }
        }

        throw new FileNotFoundException(
            "Could not locate Salamand.exe. Set the SALAMANDER_APP_PATH environment variable to the built executable before running the tests.");
    }
}
