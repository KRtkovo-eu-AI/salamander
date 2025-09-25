# Salamander UI test suite

This directory contains an automated UI test project based on [FlaUI](https://github.com/FlaUI/FlaUI) and SpecFlow Gherkin scenarios.

## Project structure

- `Salamander.UiTests/Salamander.UiTests.csproj` – C# test project targeting Windows, configured with FlaUI and SpecFlow dependencies.
- `Features/` – Gherkin feature files describing behaviour-driven test scenarios.
- `Steps/` – Step definitions implementing the behaviour described in the feature files.
- `Support/` – Shared infrastructure used by the tests (configuration, hooks, etc.).

## Running the tests

1. Build the Salamander application (e.g. using Visual Studio) so that a `salamand*.exe` executable is available.
2. Optionally set the `SALAMANDER_APP_PATH` environment variable to point directly to the executable.
3. From the `tests/Salamander.UiTests` directory, execute `dotnet test`.

The sample scenario launches the application, opens the **About** dialog, closes it, and then exits the application.
