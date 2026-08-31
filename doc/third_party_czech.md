# Oznámení o softwaru třetích stran

[Open Salamander: Samandarin](https://samandarin.net/) je postaven na práci mnoha autorů, správců a
projektů. Tato stránka uvádí komponenty, kód, nápady a nástroje, které
aplikace a její pluginy využívají. Děkujeme všem zde uvedeným za zpřístupnění
jejich práce.

## Jádro aplikace a sdílené knihovny

| Komponenta | Použití | Informace o autorství a licenci |
| --- | --- | --- |
| REGEXP | Regulární výrazy | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| AES kód | Kryptografické rutiny | Napsal Dr Brian Gladman. Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | Podpora obrázků PNG | Založeno na PNGLite od Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | Podpora parsování/vykreslování SVG | Copyright (c) 2013-2014 Mikko Mononen. Používá se pro ikony nástrojových lišt a nabídek hlavní aplikace a v pluginu PictView pro prohlížení SVG (platí limity NanoSVG: bez CSS, omezený text a filtry). |
| PictView deflate inflater | Plugin PictView | Inflater deflate/zlib pro dlaždice XCF, PNG IDAT, ZIP SKP a komprimované náhledy 3DM. Čtení bitů a sestavení Huffmanových tabulek vychází z puff od Marka Adlera. Copyright (C) 2002-2013 Mark Adler. Licence zlib. |
| openNURBS 3DM preview layout | PictView plugin | Rhinoceros `.3dm` properties-table preview chunks and `ON_WindowsBitmap::ReadCompressed` layout follow [McNeel openNURBS](https://github.com/mcneel/opennurbs). Copyright (c) Robert McNeel & Associates. Used as a format reference only; openNURBS itself is not bundled. |
| SQLite | Podpora vložené databáze | SQLite je ve veřejné doméně. |
| LibTomCrypt | Kryptografické prvky pluginu_checksum | LibTomCrypt je ve veřejné doméně. Jak napsal Tom St Denis: „Jak by měl být každý kvalitní software." |
| LGPL knihovny | Sdílené knihovny třetích stran | Licencovány pod GNU Library General Public License. Kopie licence je součástí tohoto softwaru. |
| Unicode a Win32 dlouhé cesty | Části zpracování názvů souborů | Samandarin obsahuje kód a implementační práce odvozené z, nebo podstatně inspirované, forkem [Sally](https://github.com/0xeb/sally) od [Elias Bachaalany (0xeb)](https://github.com/0xeb). Licencováno pod GNU Library General Public License. |
| Lua 5.5.0 | Přibalený interpret pro Salamatrix Lua Runtime | [Lua.org, PUC-Rio](https://www.lua.org/), instalováno z připnutého vcpkg balíčku `lua[tools]`. Copyright (c) 1994-2025 Lua.org, PUC-Rio. Licence MIT; úplné oznámení je distribuováno jako `plugins/extension-runtimes/luaruntime/runtime/LICENSE-LUA.txt`. |
| win32-darkmodelib | Nativní podpora tmavého režimu Windows pro sdílené dialogy a dialogy Salamatrix | Převzato z [ozone10/win32-darkmodelib](https://github.com/ozone10/win32-darkmodelib), commit `b58eb027c4a114c1b1c8fd09870ead982f1b1e72`. Licence Mozilla Public License 2.0, části pod licencí MIT.; see `src/third_party/darkmodelib/LICENSE.md` and `LICENSE-MIT.md`. |
| llama.cpp | Spustitelný soubor pro lokální CPU inferenci SalamatrixAI na vyžádání | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp), připnuté vydání `b10107`. Licence MIT. The configuration downloader retrieves the runtime and its license notice directly for the user. |
| Qwen2.5-Coder 1.5B Instruct GGUF | Doporučený lokální model SalamatrixAI | [Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF), připnutý soubor `q4_k_m`. Licence Apache 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Qwen2.5-Coder 0.5B Instruct GGUF | Lehký lokální model SalamatrixAI pouze pro anglické prompty | [Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF), připnutý soubor `q4_k_m`. Licence Apache 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Test Reporter | Zprávy s výsledky testů pull requestů | [dorny/test-reporter](https://github.com/dorny/test-reporter), používané v GitHub Actions. Licence MIT. It publishes JUnit and Python xUnit results as Checks and workflow summaries. |
| pytest | Spouštění Python testů a generování xUnit reportů v CI | [pytest-dev/pytest](https://github.com/pytest-dev/pytest). Licence MIT. |
| cursor-sdk | Optional Python SDK for batch SLT translation through the Cursor API | Used only by `tools/localization/translate_slt_with_openai.py` when `-Provider cursor` is selected. Install with `pip install cursor-sdk`. |
| OpenRouter API | Optional remote provider for batch SLT translation | Used by `tools/localization/translate_slt_with_openai.py` by default with the `openai/gpt-5.4-nano` model. Requires a user-supplied `OPENROUTER_API_KEY`; no key is stored in the repository. |

## Pluginy pro archivaci, kompresi a diskové obrazy

| Komponenta | Použití v | Informace o autorství a licenci |
| --- | --- | --- |
| 7-Zip | Plugin 7-ZIP | Knihovna pro archivaci souborů 7-Zip. Copyright (C) 1999-2026 Igor Pavlov. Distribuováno pod GNU LGPL, s omezením unRAR pro kód RAR. |
| zlib | Plugin ZIP | Části pluginu ZIP používají zlib. Copyright (C) 1995-2002 Jean-loup Gailly a Mark Adler. |
| bzip2 | Plugin TAR | Knihovna bzip2. Copyright (C) 1996-2000 Julian R Seward. |
| ARJ dekomprimační knihovna | Plugin UnARJ | Dekomprimační knihovna od ARJ Software, Inc. Copyright (C) 1990-1997 ARJ Software, Inc. |
| Microsoft CAB dekomprimační knihovna | Plugin UnCAB | Dekomprimační knihovna od Microsoft Corporation. Copyright (C) Microsoft Corporation 1993-1997. |
| UnRAR | Plugin UnRAR | Dekomprimační knihovna od Alexander Roshal. Copyright (C) 1993-2026 Alexander Roshal. Distribuováno pod licencí unRAR. Vcpkg pipeline instaluje balíček `unrar` pro runtime DLL pluginu UnRAR. |
| ISZ SDK | Plugin UnISO | Části pluginu UnISO používají ISZ SDK. Copyright (C) 2002-2006 EZB Systems, Inc. Všechna práva vyhrazena. |
| CHMLIB | Plugin UnCHM | Knihovna CHMLIB. Copyright (C) 2001-2010 Jed Wing. |

## Pluginy pro prohlížeč, média a formáty souborů

| Komponenta nebo autor | Použití v | Informace o autorství a licenci |
| --- | --- | --- |
| Tomas Jelinek | Plugin Multimedia Viewer | Obsahuje software napsaný Tomášem Jelínkem. Copyright (C) 2003-2026 Tomas Jelinek. |
| TagLib 2.3 | Plugin Multimedia Viewer | Knihovna zvukových metadat od Scotta Wheelera a přispěvatelů projektu TagLib. Používá se pro Unicode metadata a zvukové vlastnosti formátů Ogg Vorbis, Opus, FLAC, MP4, APE, WavPack, TrueAudio, AIFF, DSD, Matroska/WebM a souvisejících formátů. Distribuováno pod licencí GNU LGPL 2.1 nebo Mozilla Public License 1.1. Zdroj: https://github.com/taglib/taglib |
| Interní prohlížeč | Části prohlížeče Unicode | Samandarin obsahuje kód a implementační práce odvozené z, nebo podstatně inspirované, forkem [Sally](https://github.com/0xeb/sally) od [Elias Bachaalany (0xeb)](https://github.com/0xeb). Licencováno pod GNU Library General Public License. |
| Jan Patera | Plugin PictView | Části pluginu PictView jsou licencovány od Jana Patery. Copyright (C) 1994-2026 Jan Patera. |
| libexif | Plugin PictView | Části pluginu PictView používají libexif. Copyright (C) 2001-2019 Curtis Galloway a Lutz Muller. |
| Newtonsoft.Json 13.0.3 | Plugin JSON Viewer | Od Jamese Newton-Kinga, vydáno pod MIT License. |
| Microsoft .NET Framework 4.8 | Plugin JSON Viewer | Poskytnuto Microsoft Corporation pod licenčními podmínkami Microsoft .NET Framework. |
| Prism.js 1.29.0 | Viewer Frame (Salamatrix a Prism Text Viewer) | Od Ley Verou a přispěvatelů PrismJS, vydáno pod MIT License. |
| Microsoft.Web.WebView2 1.0.2420.47 | Viewer Frame | Od Microsoft Corporation, distribuováno pod BSD 3-Clause License. |
| Markdig 0.36.2 | MarkdigRenderer pro WebView2 Render Viewer | Od Alexendra Mutela a přispěvatelů, vydáno pod BSD 2-Clause License. |

## Pluginy pro síť, synchronizaci a zařízení

| Komponenta nebo autor | Použití v | Informace o autorství a licenci |
| --- | --- | --- |
| OpenSSL | Plugin FTP Client a závislosti SFTP | Copyright (c) 1998-2026 The OpenSSL Project Authors. Všechna práva vyhrazena. OpenSSL 1.0.x je distribuováno pod OpenSSL a původní SSLeay licencemi. OpenSSL 3.x je distribuováno pod Apache License 2.0. Vcpkg pipeline instaluje OpenSSL pro kompatibilitu DLL pluginu FTP a pro závislosti pluginu SFTP. |
| libssh2 | Plugin SFTP | Od Daniela Stenberga, Simona Josefa a přispěvatelů libssh2. Distribuováno pod BSD 3-Clause License. Vcpkg pipeline instaluje balíček `libssh2` pro závislosti pluginu SFTP. |
| Dupl3xx | Plugin SFTP | Plugin vytvořil a udržuje Dupl3xx. Zdrojový repozitář: [salamander-sftp-plugin](https://github.com/Dupl3xx/salamander-sftp-plugin). |
| Martin Prikryl | Plugin WinSCP | Části pluginu WinSCP jsou licencovány od Martina Prikryla. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Plugin Windows Mobile | Obsahuje software napsaný Jurajem Rojkem. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Plugin Samandarin Update Notifier | Poskytnuto Microsoft Corporation pod licenčními podmínkami Microsoft .NET Framework. |

## Rozšíření Monitor hardwaru

| Komponenta | Použití v | Informace o autorství a licenci |
| --- | --- | --- |
| HardView | Rozšíření Monitor hardwaru | C++/CLI mostová knihovna pro monitorování hardwaru. Copyright (c) 2025 gafoo. Licencováno pod MIT License. Zdroj: [gafoo173/HardView](https://github.com/gafoo173/HardView). |
| LibreHardwareMonitorLib 0.9.6.0 | Rozšíření Monitor hardwaru | .NET knihovna pro monitorování hardwaru. LibreHardwareMonitorLib je licencováno pod GNU General Public License v3.0. Zdroj: [LibreHardwareMonitor/LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor). |
| HidSharp 2.6.4.0 | Rozšíření Monitor hardwaru | Knihovna pro přístup k HID zařízením, přechodná závislost LibreHardwareMonitorLib. Copyright (C) 2010 James F. Bellinger. Licencováno pod GNU Lesser General Public License v3.0. |

## Uživatelské rozhraní, dokumentace a vykreslování Markdownu

| Komponenta nebo příspěvek | Použití pro | Informace o autorství a licenci |
| --- | --- | --- |
| cmark-gfm | Parsování/vykreslování CommonMark a GitHub-Flavored Markdown | GitHubův fork `commonmark/cmark`, knihovna a program pro parsování a vykreslování CommonMark v C. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Stylování dokumentů Markdown | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Základ panel stromového zobrazení | Panel Tree View | Založeno na změnách navržených fgodoy. |
| lokalizační scripty | Části nástrojů pro lokalizaci | Samandarin obsahuje nástroje pro lokalizaci upravené z, nebo podstatně inspirované, forkem [Sally](https://github.com/0xeb/sally) od [Elias Bachaalany (0xeb)](https://github.com/0xeb). Licencováno pod GNU Library General Public License. |
| SVG ikony šablon příkazových prostředí | Ikony nabídky profilů Windows Terminal | SVG grafika upravená podle ikon produktů Microsoft a ikony Azure Cloud Shell ze SVG Repo. |
