using System.Reflection;

namespace Salamander.UiTests.Support;

public static class TestConfiguration
{
    private const string AppPathEnvironmentVariable = "SALAMANDER_APP_PATH";

    public static string ResolveApplicationPath()
    {
        var pathFromEnvironment = Environment.GetEnvironmentVariable(AppPathEnvironmentVariable);
        if (!string.IsNullOrWhiteSpace(pathFromEnvironment) && File.Exists(pathFromEnvironment))
        {
            return Path.GetFullPath(pathFromEnvironment);
        }

        var potentialPath = LocateExecutableInRepository();
        if (potentialPath is not null)
        {
            return potentialPath;
        }

        throw new FileNotFoundException(
            $"Unable to find the Salamander executable. Set the '{AppPathEnvironmentVariable}' environment variable to point to the built application before running the UI tests.");
    }

    private static string? LocateExecutableInRepository()
    {
        var assemblyLocation = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location)!;
        var repositoryRoot = Path.GetFullPath(Path.Combine(assemblyLocation, "..", "..", "..", ".."));

        if (!Directory.Exists(repositoryRoot))
        {
            return null;
        }

        return Directory.EnumerateFiles(repositoryRoot, "salamand*.exe", SearchOption.AllDirectories)
            .Concat(Directory.EnumerateFiles(repositoryRoot, "Salamander*.exe", SearchOption.AllDirectories))
            .Select(Path.GetFullPath)
            .FirstOrDefault(File.Exists);
    }
}
