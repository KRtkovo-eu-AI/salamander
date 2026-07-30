### Plugin Updates in Samandarin plugin
> [!NOTE]
> Those files are just base templates, they may be not actual!
> 
> Up to date plugin catalog sources are published here:
> - Stable: https://samandarin.krtkovo.eu/catalogs/plugins-stable.json
> - Unofficial 3rd party: https://samandarin.krtkovo.eu/catalogs/plugins-unofficial.json
> - Extension Runtimes: https://samandarin.krtkovo.eu/catalogs/extension-runtimes.json

<img width="966" height="643" alt="image" src="https://github.com/user-attachments/assets/c1088472-335b-4421-82bb-fea8531beb92" />

Catalog plugin entries may set `icon` to `plugin` to use the icon extracted from the installed `.spl` file, or to an absolute URL, a URL relative to the catalog JSON file, or a local file path for a PNG/JPEG/BMP/ICO catalog icon. If a catalog icon cannot be loaded, Samandarin falls back to the installed plugin icon when the plugin is installed.

Schema version 4 adds an optional `dependencies` array containing catalog plugin
IDs. Dependencies are transitive. Before installing, updating, or opening the
web page for an entry, Samandarin warns about missing dependencies and offers
to install official packages automatically or open their web pages together.
Older catalogs without `dependencies` remain supported.

```json
{
  "id": "salamatrixailocalllama",
  "dependencies": [
    "salamatrixai"
  ]
}
```
