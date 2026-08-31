# Mededelingen over software van derden

[Open Salamander: Samandarin](https://samandarin.net/) bouwt voort op het werk van vele auteurs,
beheerders en projecten. Deze pagina vermeldt de componenten, code, ideeën en
hulpmiddelen die worden gebruikt door de applicatie en haar plug-ins. Bedankt
aan iedereen die hier wordt genoemd voor het beschikbaar stellen van hun werk.

## Kernapplicatie en gedeelde bibliotheken

| Component | Gebruikt voor | Attributie- en licentie-opmerkingen |
| --- | --- | --- |
| REGEXP | Reguliere expressie matching | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| AES code | Cryptografische routines | Geschreven door Dr Brian Gladman. Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | PNG afbeeldingsondersteuning | Gebaseerd op PNGLite door Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | SVG parsing/rendering ondersteuning | Copyright (c) 2013-2014 Mikko Mononen. Gebruikt voor de werkbalk- en menu-iconen van de hoofdtoepassing en door de PictView-plug-in voor SVG-weergave (NanoSVG-beperkingen: geen CSS, beperkte tekst en filters). |
| PictView deflate-inflater | PictView-invoegtoepassing | Deflate/zlib-inflater voor XCF-tegels, PNG-IDAT, SKP-ZIP en gecomprimeerde 3DM-voorbeelden. Bitreader en Huffman volgen puff van Mark Adler. Copyright (C) 2002-2013 Mark Adler. zlib-licentie. |
| openNURBS 3DM preview layout | PictView plugin | Rhinoceros `.3dm` properties-table preview chunks and `ON_WindowsBitmap::ReadCompressed` layout follow [McNeel openNURBS](https://github.com/mcneel/opennurbs). Copyright (c) Robert McNeel & Associates. Used as a format reference only; openNURBS itself is not bundled. |
| SQLite | Ingebedde database ondersteuning | SQLite is in het publieke domein. |
| LibTomCrypt | Cryptografische primitieven van het checksum plug-in | LibTomCrypt is publiek domein. Zoals Tom St Denis schreef: "Zoals elke kwaliteitssoftware zou moeten zijn." |
| LGPL bibliotheken | Gedeelde bibliotheken van derden | Gelicenseerd onder de GNU Library General Public License. Een kopie van de licentie is bijgesloten bij deze software. |
| Unicode en Win32 lange paden | Bestandsnaam verwerkingsgedeeltes | Samandarin bevat code en implementatiewerk afgeleid van fork [Sally](https://github.com/0xeb/sally) door [Elias Bachaalany (0xeb)](https://github.com/0xeb), of substantial geïnspireerd daardoor. Gelicenseerd onder de GNU Library General Public License. |
| Lua 5.5.0 | Meegeleverde interpreter voor de Salamatrix Lua Runtime | [Lua.org, PUC-Rio](https://www.lua.org/), geïnstalleerd vanuit het vastgezette vcpkg-pakket `lua[tools]`. Copyright (c) 1994-2025 Lua.org, PUC-Rio. MIT-licentie; de volledige kennisgeving wordt gedistribueerd als `plugins/extension-runtimes/luaruntime/runtime/LICENSE-LUA.txt`. |
| win32-darkmodelib | Native Windows-donkere-modusondersteuning voor gedeelde en Salamatrix-dialoogvensters | Afkomstig van [ozone10/win32-darkmodelib](https://github.com/ozone10/win32-darkmodelib), commit `b58eb027c4a114c1b1c8fd09870ead982f1b1e72`. Mozilla Public License 2.0, met delen onder de MIT-licentie.; see `src/third_party/darkmodelib/LICENSE.md` and `LICENSE-MIT.md`. |
| llama.cpp | Uitvoerbaar bestand voor lokale SalamatrixAI CPU-inferentie | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp), vastgezette release `b10107`. MIT-licentie. The configuration downloader retrieves the runtime and its license notice directly for the user. |
| Qwen2.5-Coder 1.5B Instruct GGUF | Aanbevolen lokaal SalamatrixAI-model | [Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF), vastgezet `q4_k_m`-bestand. Apache License 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Qwen2.5-Coder 0.5B Instruct GGUF | Licht lokaal SalamatrixAI-model, alleen Engelse prompts | [Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF), vastgezet `q4_k_m`-bestand. Apache License 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Test Reporter | Testresultaten voor pull requests | [dorny/test-reporter](https://github.com/dorny/test-reporter), gebruikt in GitHub Actions. MIT-licentie. It publishes JUnit and Python xUnit results as Checks and workflow summaries. |
| pytest | Python-tests en xUnit-rapporten in CI | [pytest-dev/pytest](https://github.com/pytest-dev/pytest). MIT-licentie. |
| cursor-sdk | Optional Python SDK for batch SLT translation through the Cursor API | Used only by `tools/localization/translate_slt_with_openai.py` when `-Provider cursor` is selected. Install with `pip install cursor-sdk`. |
| OpenRouter API | Optional remote provider for batch SLT translation | Used by `tools/localization/translate_slt_with_openai.py` by default with the `openai/gpt-5.4-nano` model. Requires a user-supplied `OPENROUTER_API_KEY`; no key is stored in the repository. |

## Archiverings-, compressie- en schijfimage plug-ins

| Component | Gebruikt in | Attributie- en licentie-opmerkingen |
| --- | --- | --- |
| 7-Zip | 7-ZIP plug-in | 7-Zip bestandsarchiveringsbibliotheek. Copyright (C) 1999-2026 Igor Pavlov. Gedistribueerd onder GNU LGPL, met de unRAR-beperking voor de RAR-code. |
| zlib | ZIP plug-in | Delen van de ZIP plug-in gebruiken zlib. Copyright (C) 1995-2002 Jean-loup Gailly en Mark Adler. |
| bzip2 | TAR plug-in | bzip2 bibliotheek. Copyright (C) 1996-2000 Julian R Seward. |
| ARJ decompressiebibliotheek | UnARJ plug-in | Decompressiebibliotheek geschreven door ARJ Software, Inc. Copyright (C) 1990-1997 ARJ Software, Inc. |
| Microsoft CAB decompressiebibliotheek | UnCAB plug-in | Decompressiebibliotheek geschreven door Microsoft Corporation. Copyright (C) Microsoft Corporation 1993-1997. |
| UnRAR | UnRAR plug-in | Decompressiebibliotheek geschreven door Alexander Roshal. Copyright (C) 1993-2026 Alexander Roshal. Gedistribueerd onder de unRAR-licentie. De vcpkg-pipeline installeert het `unrar`-pakket voor de runtime DLL van het UnRAR plug-in. |
| ISZ SDK | UnISO plug-in | Delen van de UnISO plug-in gebruiken ISZ SDK. Copyright (C) 2002-2006 EZB Systems, Inc. Alle rechten voorbehouden. |
| CHMLIB | UnCHM plug-in | CHMLIB bibliotheek. Copyright (C) 2001-2010 Jed Wing. |

## Viewer, media en bestandsformaat plug-ins

| Component of auteur | Gebruikt in | Attributie- en licentie-opmerkingen |
| --- | --- | --- |
| Tomas Jelinek | Multimedia Viewer plug-in | Bevat software geschreven door Tomas Jelinek. Copyright (C) 2003-2026 Tomas Jelinek. |
| TagLib 2.3 | Multimedia Viewer-plug-in | Audiometadatabibliotheek van Scott Wheeler en bijdragers aan TagLib. Wordt gebruikt voor Unicode-metadata en audio-eigenschappen in Ogg Vorbis, Opus, FLAC, MP4, APE, WavPack, TrueAudio, AIFF, DSD, Matroska/WebM en verwante indelingen. Gedistribueerd onder de GNU LGPL 2.1 of Mozilla Public License 1.1. Bron: https://github.com/taglib/taglib |
| Interne viewer | Unicode viewer gedeeltes | Samandarin bevat code en implementatiewerk afgeleid van fork [Sally](https://github.com/0xeb/sally) door [Elias Bachaalany (0xeb)](https://github.com/0xeb), of substantial geïnspireerd daardoor. Gelicenseerd onder de GNU Library General Public License. |
| Jan Patera | PictView plug-in | Delen van de PictView plug-in zijn gelicenseerd van Jan Patera. Copyright (C) 1994-2026 Jan Patera. |
| libexif | PictView plug-in | Delen van de PictView plug-in gebruiken libexif. Copyright (C) 2001-2019 Curtis Galloway en Lutz Muller. |
| Newtonsoft.Json 13.0.3 | JSON Viewer plug-in | Door James Newton-King, uitgegeven onder de MIT-licentie. |
| Microsoft .NET Framework 4.8 | JSON Viewer plug-in | Verstrekt door Microsoft Corporation onder de licentievoorwaarden van het Microsoft .NET Framework. |
| Prism.js 1.29.0 | Viewer Frame (Salamatrix en Prism Text Viewer) | Door Lea Verou en PrismJS-bijdragers, uitgegeven onder de MIT-licentie. |
| Microsoft.Web.WebView2 1.0.2420.47 | Viewer Frame | Door Microsoft Corporation, gedistribueerd onder de BSD 3-Clause-licentie. |
| Markdig 0.36.2 | MarkdigRenderer voor WebView2 Render Viewer | Door Alexandre Mutel en bijdragers, uitgegeven onder de BSD 2-Clause-licentie. |

## Netwerk-, synchronisatie- en apparaat plug-ins

| Component of auteur | Gebruikt in | Attributie- en licentie-opmerkingen |
| --- | --- | --- |
| OpenSSL | FTP Client plug-in en SFTP-afhankelijkheden | Copyright (c) 1998-2026 The OpenSSL Project Authors. Alle rechten voorbehouden. OpenSSL 1.0.x wordt gedistribueerd onder de OpenSSL- en originele SSLeay-licenties. OpenSSL 3.x wordt gedistribueerd onder de Apache License 2.0. De vcpkg-pipeline installeert OpenSSL voor de FTP plug-in compatibiliteits-DLL's en voor de SFTP plug-in afhankelijkheden. |
| libssh2 | SFTP plug-in | Door Daniel Stenberg, Simon Josefsson en libssh2-bijdragers. Gedistribueerd onder de BSD 3-Clause-licentie. De vcpkg-pipeline installeert het `libssh2`-pakket voor de SFTP plug-in afhankelijkheden. |
| Dupl3xx | SFTP plug-in | Plug-in geschreven en onderhouden door Dupl3xx. Bronrepository: [salamander-sftp-plugin](https://github.com/Dupl3xx/salamander-sftp-plugin). |
| Martin Prikryl | WinSCP plug-in | Delen van de WinSCP plug-in zijn gelicenseerd van Martin Prikryl. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Windows Mobile plug-in | Bevat software geschreven door Juraj Rojko. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Samandarin Update Notifier plug-in | Verstrekt door Microsoft Corporation onder de licentievoorwaarden van het Microsoft .NET Framework. |

## Hardware-monitor extensie

| Component | Gebruikt in | Attributie- en licentie-opmerkingen |
| --- | --- | --- |
| HardView | Hardware-monitor extensie | C++/CLI bridgebibliotheek voor hardwarebewaking. Copyright (c) 2025 gafoo. Gelicenseerd onder MIT License. Bron: [gafoo173/HardView](https://github.com/gafoo173/HardView). |
| LibreHardwareMonitorLib 0.9.6.0 | Hardware-monitor extensie | .NET hardwarebewakingsbibliotheek. LibreHardwareMonitorLib is gelicenseerd onder GNU General Public License v3.0. Bron: [LibreHardwareMonitor/LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor). |
| HidSharp 2.6.40 | Hardware-monitor extensie | HID-apparaattoegangsbibliotheek, transitieve afhankelijkheid van LibreHardwareMonitorLib. Copyright (C) 2010 James F. Bellinger. Gelicenseerd onder GNU Lesser General Public License v3.0. |

## Gebruikersinterface, documentatie en Markdown rendering

| Component of bijdrage | Gebruikt voor | Attributie- en licentie-opmerkingen |
| --- | --- | --- |
| cmark-gfm | CommonMark en GitHub-Flavored Markdown parsing/rendering | GitHub's fork van `commonmark/cmark`, een CommonMark parsing- en renderingbibliotheek en -programma in C. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Markdown documentopmaak | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Tree View paneelwijzigingen | Tree View paneel | Gebaseerd op wijzigingen voorgesteld door fgodoy. |
| Lokalisatiescripts | Lokalisatie hulpmiddelgedeeltes | Samandarin bevat lokalisatiehulpmiddelen aangepast van fork [Sally](https://github.com/0xeb/sally) door [Elias Bachaalany (0xeb)](https://github.com/0xeb), of substantial geïnspireerd daardoor. Gelicenseerd onder de GNU Library General Public License. |
| SVG-pictogrammen voor opdrachtsjablonen | Profielmenu-pictogrammen van Windows Terminal | SVG-illustraties aangepast van Microsoft-productpictogrammen en het Azure Cloud Shell-pictogram van SVG Repo. |
