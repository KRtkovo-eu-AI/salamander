using System;
using System.Globalization;
using System.IO;
using System.Text.RegularExpressions;

namespace Salamander.AutomationTests;

internal static class ResourceHeaderIds
{
    private static readonly Regex DefineRegex = new(
        @"^#define\s+(?<name>\w+)\s+(?<value>0x[0-9A-Fa-f]+|\d+)",
        RegexOptions.Compiled);

    public static int GetValue(string symbol)
    {
        if (string.IsNullOrWhiteSpace(symbol))
        {
            throw new ArgumentException("A symbol name must be provided.", nameof(symbol));
        }

        var headerPath = Path.Combine(TestConfiguration.RepositoryRoot, "src", "resource.rh2");
        if (!File.Exists(headerPath))
        {
            throw new FileNotFoundException("The resource header could not be located.", headerPath);
        }

        foreach (var line in File.ReadLines(headerPath))
        {
            var match = DefineRegex.Match(line);
            if (!match.Success)
            {
                continue;
            }

            if (!string.Equals(match.Groups["name"].Value, symbol, StringComparison.Ordinal))
            {
                continue;
            }

            var valueText = match.Groups["value"].Value;
            return valueText.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
                ? int.Parse(valueText[2..], NumberStyles.HexNumber, CultureInfo.InvariantCulture)
                : int.Parse(valueText, NumberStyles.Integer, CultureInfo.InvariantCulture);
        }

        throw new InvalidOperationException($"Symbol '{symbol}' was not found in {headerPath}.");
    }
}
