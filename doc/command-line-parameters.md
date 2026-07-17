# Command-line parameters

This document describes every command-line form accepted by the two executable
programs shipped with Open Salamander. Parameter names are case-insensitive.
Quote an argument that contains spaces. `salamand.exe` accepts only the
hyphen-prefixed forms listed below, except where another form is explicitly
shown.

## `salamand.exe`

### Panel paths and activation

| Parameter | Argument | Description |
| --- | --- | --- |
| `-l <path>` | Required path | Opens `<path>` in the left panel. Environment variables in the path are expanded. |
| `-r <path>` | Required path | Opens `<path>` in the right panel. Environment variables in the path are expanded. |
| `-a <path>` | Required path | Opens `<path>` in the active panel. Environment variables in the path are expanded. |
| `-p <0\|1\|2>` | Required panel number | Selects the active panel: `0` leaves the selection unchanged, `1` selects the left panel, and `2` selects the right panel. |
| `-aj <path>` | Required path | Opens `<path>` in the active panel using the Jump List *hot path* syntax. This is an internal parameter used by Jump List shortcuts. |

Paths which are not plugin file-system paths are made absolute relative to the
current directory. For example:

```bat
salamand.exe -l "C:\\Source Code" -r "%USERPROFILE%\\Downloads" -p 2
```

### Configuration and appearance

| Parameter | Argument | Description |
| --- | --- | --- |
| `-c <file>` | Required configuration-file name or path | Uses the specified configuration file instead of the default `config.reg`. A relative name is looked up beside `salamand.exe`, then in the user's Roaming AppData directory. |
| `-i <index>` | Required icon index | Forces the main-window icon to the specified zero-based index. Valid indices range from `0` up to the number of built-in main-window icons minus one. |
| `-t <text>` | Required title prefix | Forces the main-window title prefix. |
| `-o` | None | Forces single-instance behaviour: if another compatible Salamander instance is running, the new invocation activates it instead of opening another instance. |
| `-language <name>` or `/language <name>` | Required language name | Selects `<name>.slg` from the `lang` directory. The name must not contain a path separator, colon, or dot. |

### Welcome dialog and installer integration

| Parameter | Argument | Description |
| --- | --- | --- |
| `-welcome` | None | Forces the configuration Welcome dialog to open during startup, even when Salamander already has a usable configuration. The installer uses this after installation. |
| `-run_notepad <file>` | Required file path | Requests opening `<file>` in Notepad after Salamander has started. This is an internal installer/SFX integration parameter. |

Unrecognised parameters, missing required arguments, and slash-prefixed forms
other than `/language` cause Salamander to show an invalid-command-line error.

## `utils\\salmon.exe`

`salmon.exe` is Open Salamander's crash-report helper. It is normally launched
by Salamander and is not intended to be run directly by end users.

| Parameter form | Description |
| --- | --- |
| `salmon.exe --test` | Creates a test bug-report package in the temporary directory and verifies its compression workflow. `/test` is an equivalent spelling. No other arguments may follow. |
| `salmon.exe "<file-mapping-name>" "<language-file>"` | Normal internal invocation. `<file-mapping-name>` is the shared-memory mapping created by Salamander; `<language-file>` is the `.slg` file name used for report UI strings. Both arguments must be quoted. |

For the normal internal form, the first two arguments must be quoted as shown
above. The helper reads those two arguments and ignores any remaining text.
Invalid direct invocations show the crash helper's command-line error.
