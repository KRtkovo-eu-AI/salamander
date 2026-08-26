# Network use and privacy

Samandarin does not include product telemetry. It does not phone home with
usage statistics, panel contents, or crash dumps unless you choose to send a
report.

The table below lists outbound network contact that the application or a
bundled plugin may make. Destinations depend on configuration and on which
plugins you use.

| Contact | When | Destination | Notes |
| --- | --- | --- | --- |
| Application update check | Automatic when enabled (default weekly, plus optional check at startup) | `https://github.com/KRtkovo-eu-AI/salamander/releases/latest` | Disable with **Frequency = Disabled** and uncheck the startup check in Samandarin Configuration. |
| Plugin catalogs | Feature-triggered when you open Plugin Updates (and when that dialog refreshes) | `https://samandarin.net/catalogs/*.json` | Default sources include stable plugins, stable extensions, unofficial plugins, and extension runtimes. You can disable any source. |
| Official plugin/extension install | User-initiated Install/Update | GitHub `KRtkovo-eu-AI/salamander-plugins` release `.7z` URLs, often redirected to `objects.githubusercontent.com` | Fail-closed: catalog SHA-256 plus Authenticode. Staging stays in `%TEMP%`. |
| Crash reporter | Local ZIP is automatic; publishing is user-initiated | GitHub issue form only if you choose to open it | No silent upload of dumps. |
| Salamatrix AI | Feature-triggered after you configure a provider | Provider URL you enter (OpenAI-compatible HTTP, Ollama, Local LLaMA, and similar) | Nothing is sent until a provider is configured and used. |
| FTP, SFTP, WinSCP, Windows Mobile, Network | User-initiated connections | Hosts you connect to | Credentials and file contents follow those protocols. |
| WebView-based viewers | Feature-triggered when you view a document | Local WebView2 virtual host for file content; the page itself may then request other URLs | Treat untrusted HTML as active web content. |
| Internal Viewer / file operations | Local | None by default | Opening a file does not by itself contact the network. |

“Automatic” means the running application may do it without opening Plugin
Updates. “Feature-triggered” means a feature you use causes it. “User-initiated”
means an explicit action such as Install, connecting to a server, or opening a
GitHub form.

## Local / offline use

To keep a copy of Samandarin from making optional network requests:

1. Open **Samandarin Configuration**.
2. Set update **Frequency** to **Disabled**.
3. Uncheck the option that checks for application updates at startup.
4. Open Plugin Updates → configure catalog sources and disable every source you
   do not want, especially the unofficial catalog.
5. Do not configure Salamatrix AI providers (OpenAI-compatible HTTP, Ollama,
   Local LLaMA, or others).
6. Do not connect FTP/SFTP/WinSCP or other network plugins.

Plugin Manager still reads `plugin-receipts.json` and
`plugin-capabilities.json` next to `salamand.exe`. Those files are local.

HTTPS catalog and GitHub traffic still follows the system proxy when Windows
has one configured.

## What is not collected

There is no built-in telemetry channel. Crash dumps stay on disk until you
choose to file an issue. Plugin install receipts record package id, source URL,
SHA-256, signer, version, and verification time for **this installation
directory**. They travel with a portable copy of `salamand.exe`; they are not
uploaded.
