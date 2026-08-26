### Plugin Updates in Samandarin plugin
> [!NOTE]
> Those files are just base templates, they may be not actual!
> 
> Up to date plugin catalog sources are published here:
> - Stable: https://samandarin.net/catalogs/plugins-stable.json
> - Stable extensions: https://samandarin.net/catalogs/extensions-stable.json
> - Unofficial 3rd party: https://samandarin.net/catalogs/plugins-unofficial.json
> - Extension Runtimes: https://samandarin.net/catalogs/extension-runtimes.json

<img width="966" height="643" alt="image" src="https://github.com/user-attachments/assets/c1088472-335b-4421-82bb-fea8531beb92" />

Catalog plugin entries may set `icon` to `plugin` to use the icon extracted from the installed `.spl` file, or to an absolute URL, a URL relative to the catalog JSON file, or a local file path for a PNG/JPEG/BMP/ICO catalog icon. If a catalog icon cannot be loaded, Samandarin falls back to the installed plugin icon when the plugin is installed.

Schema version 4 adds an optional `dependencies` array containing catalog plugin
IDs. Dependencies are transitive. Before installing, updating, or opening the
web page for an entry, Samandarin warns about missing dependencies and offers
to install official packages automatically or open their web pages together.
Older catalogs without `dependencies` remain supported.

Schema version 5 requires `packageType` on every entry. Its value is `plugin`
for native `.spl` packages or `extension` for manifest-based packages installed
under the `extensions` directory. Samandarin uses this field for matching and
for selecting the installation root; archive names do not determine the type.

Schema version 6 adds independent package verification metadata:

- `packageSha256` — lowercase hex SHA-256 of the official `.7z` bytes. The
  digest must be computed from the archive file, never copied from GitHub
  release notes (GitHub also hosts the package). Official Plugin Updates
  auto-install is **fail-closed**: a missing, empty, or mismatched hash
  cancels installation. Unofficial entries may omit the field; they only open
  a browser and are shown as not cryptographically verified.
- `security` — optional curated disclosure (`networkAccess`,
  `externalProcesses`, `scriptExecution`, `activeWebContent`, `elevation`).
  Native `.spl` modules still run in-process; these flags are not a sandbox.

Older clients ignore unknown JSON fields. Publish live catalogs on
`samandarin.net` with `packageSha256` **before** shipping a client that
requires the hash. Use `tools/catalogs/fill_package_hashes.py` against local
`.7z` archives after `tools/package_salamander_plugins.ps1`.

```json
{
  "id": "salamatrixailocalllama",
  "packageSha256": "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
  "security": {
    "networkAccess": "possible",
    "externalProcesses": "yes",
    "scriptExecution": "yes",
    "activeWebContent": "no",
    "elevation": "never"
  },
  "dependencies": [
    "salamatrixai"
  ]
}
```
