# UI automation tests

This folder contains the Visual Studio solution `Salamander.AutomationTests.sln` with a C# project for
running automated UI tests against Salamander using [FlaUI](https://github.com/FlaUI/FlaUI)
and [Reqnroll](https://github.com/reqnroll/Reqnroll).

## Getting started

1. Build the Salamander application (`Salamand.exe`).
2. Optionally set the `SALAMANDER_APP_PATH` environment variable to the full path of the built executable.
   If the variable is not set, the tests try a few common relative locations under the repository root.
3. Open the solution in Visual Studio and restore NuGet packages.
4. Run the tests using the Test Explorer.

The sample scenario opens Salamander, launches the **About** dialog from the **Help** menu,
closes the dialog, and then exits the application.
