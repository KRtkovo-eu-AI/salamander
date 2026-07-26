# Salamander Tools (uv Workflow)

## Prerequisites
- Install [uv](https://docs.astral.sh/uv/) so the Python utilities can manage their own environment.
  - Windows (PowerShell): `irm https://astral.sh/uv/install.ps1 | iex`
  - Linux / macOS (shell): `curl -Ls https://astral.sh/uv/install.sh | sh`
  - Alternatively: `pipx install uv`
- Verify the installation with `uv --version`.

## Initial Setup
1. `cd tools` inside the Salamander repository.
2. Run `uv sync` to create/update the local virtual environment and install the locked dependencies.
   - Repeat `uv sync` whenever `pyproject.toml` or `uv.lock` changes.

## Running Utilities
- Execute any script via `uv run`. Examples:
  - `uv run comment-translation-status --help`
  - `uv run comment-translation-status --project-root ..\src --no-recursion --name-filter dialogs*.cpp`
  - `uv run comment-translation-status --project-root ..\src`
  - `uv run comment-find-czech-words --project-root ..\src --output czech_words.txt`
  - `uv run comment-find-czech-words --project-root ..\src\plugins\ --exclude shared\ --output plugins_czech_words.txt`
  - `uv run comment-word-counter --project-root ..\src --output word_counts.txt`
  - `uv run comment-code-guard --base-repo-path \salamander_code_guard`  
  - `uv run comment-code-guard --base-repo-path \salamander_code_guard --sub-path src\plugins\zip`  
- `uv run` ensures the command uses the environment defined by `uv sync`.

## Managing Dependencies
- Add or update dependencies with `uv add <package>` and then commit the updated `pyproject.toml` and `uv.lock`.
- To remove a dependency use `uv remove <package>` followed by `uv sync`.

## Troubleshooting
- If activation fails after switching Python versions, rerun `uv sync`.
- Delete the `.venv` directory inside `tools/` and `uv sync` again to rebuild a broken environment.

## Prefilled plugin release URLs
- Build plugin archives with `tools/package_salamander_plugins.ps1` first.
- Generate a GitHub release form URL for one archive with:
  ```powershell
  .\tools\new_plugin_release_url.ps1 -ArchivePath .\plugin-packages-x64\plugin_5.0_ftp_5.01_x64.7z
  ```
- The generated URL targets `KRtkovo-eu-AI/salamander-plugins` by default and pre-fills the release tag, title, body, plugin description, CRC32, MD5, SHA1, and SHA256. Add `-PluginDescription` only if you need to override the built-in source-code description table. The script opens the URL in a browser by default; add `-NoOpen` if you only want to print it, or `-CopyToClipboard` to copy the URL.

## Runtime package verification

Before transferring a build to the target machine, validate the standalone
runtime/helper packages without starting Salamander:

```powershell
.\tools\verify_runtime_packages.ps1 -SalamanderPath .\build\salamander\Release_x64
```

The check verifies the expected `.spl` files, PE architecture, mandatory
`SalamanderPluginEntry`/`SalamanderPluginGetReqVer` exports, and each shipped
runtime bootstrap file. It does not load plugins, modify the registry, or
replace target-machine GUI validation.
