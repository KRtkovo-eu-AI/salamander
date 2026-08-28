# Hinweise zu Software Dritter

[Open Salamander: Samandarin](https://samandarin.net/) baut auf der Arbeit vieler Autoren, Betreuer und
Projekte auf. Diese Seite listet die Komponenten, Code, Ideen und Werkzeuge
auf, die von der Anwendung und ihren Plugins verwendet werden. Vielen Dank an
alle hier Genannten für die Bereitstellung ihrer Arbeit.

## Kernanwendung und gemeinsame Bibliotheken

| Komponente | Verwendung | Hinweise zu Autorrecht und Lizenz |
| --- | --- | --- |
| REGEXP | Reguläre Ausdrücke | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| AES-Code | Kryptografische Routinen | Geschrieben von Dr. Brian Gladman. Copyright (C) 2001 Dr. Brian Gladman. |
| PNGLite | PNG-Bildunterstützung | Basierend auf PNGLite von Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | SVG-Parser-/Rendering-Unterstützung | Copyright (c) 2013-2014 Mikko Mononen. Wird für die Symbolleisten- und Menüsymbole der Hauptanwendung und im PictView-Plugin zur SVG-Anzeige verwendet (NanoSVG-Einschränkungen: kein CSS, eingeschränkter Text und Filter). |
| PictView-Deflate-Inflater | PictView-Plugin | Deflate/zlib-Inflater für XCF-Kacheln, PNG-IDAT, SKP-ZIP und komprimierte 3DM-Vorschauen. Bit-Reader und Huffman-Aufbau folgen Mark Adlers puff. Copyright (C) 2002-2013 Mark Adler. zlib-Lizenz. |
| openNURBS 3DM preview layout | PictView plugin | Rhinoceros `.3dm` properties-table preview chunks and `ON_WindowsBitmap::ReadCompressed` layout follow [McNeel openNURBS](https://github.com/mcneel/opennurbs). Copyright (c) Robert McNeel & Associates. Used as a format reference only; openNURBS itself is not bundled. |
| SQLite | Eingebettete Datenbankunterstützung | SQLite ist Gemeinfrei. |
| LibTomCrypt | Kryptografische Primitive des Checksum-Plugins | LibTomCrypt ist gemeinfrei. Wie Tom St Denis schrieb: „Wie es jede qualitativ hochwertige Software sein sollte." |
| LGPL-Bibliotheken | Gemeinsame Bibliotheken Dritter | Lizentiert unter der GNU Library General Public License. Eine Kopie der Lizenz ist dieser Software beigefügt. |
| Unicode und Win32 lange Pfade | Dateinamen-Verarbeitungsteile | Samandarin enthält Code und Implementierungsarbeiten, die von Fork [Sally](https://github.com/0xeb/sally) von [Elias Bachaalany (0xeb)](https://github.com/0xeb) abgeleitet oder wesentlich davon inspiriert sind. Lizentiert unter der GNU Library General Public License. |
| Lua 5.5.0 | Mitgelieferter Interpreter für die Salamatrix Lua Runtime | [Lua.org, PUC-Rio](https://www.lua.org/), installiert aus dem festgelegten vcpkg-Paket `lua[tools]`. Copyright (c) 1994-2025 Lua.org, PUC-Rio. MIT-Lizenz; der vollständige Hinweis wird als `plugins/extension-runtimes/luaruntime/runtime/LICENSE-LUA.txt` verteilt. |
| win32-darkmodelib | Native Windows-Dunkelmodus-Unterstützung für gemeinsame und Salamatrix-Dialoge | Aus [ozone10/win32-darkmodelib](https://github.com/ozone10/win32-darkmodelib), Commit `b58eb027c4a114c1b1c8fd09870ead982f1b1e72`. Mozilla Public License 2.0, Teile unter MIT-Lizenz.; see `src/third_party/darkmodelib/LICENSE.md` and `LICENSE-MIT.md`. |
| llama.cpp | Ausführbare Datei für lokale SalamatrixAI-CPU-Inferenz | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp), festgelegtes Release `b10107`. MIT-Lizenz. The configuration downloader retrieves the runtime and its license notice directly for the user. |
| Qwen2.5-Coder 1.5B Instruct GGUF | Empfohlenes lokales SalamatrixAI-Modell | [Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF), festgelegte Datei `q4_k_m`. Apache License 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Qwen2.5-Coder 0.5B Instruct GGUF | Leichtes lokales SalamatrixAI-Modell nur für englische Prompts | [Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF), festgelegte Datei `q4_k_m`. Apache License 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Test Reporter | Testergebnisberichte für Pull Requests | [dorny/test-reporter](https://github.com/dorny/test-reporter), verwendet in GitHub Actions. MIT-Lizenz. It publishes JUnit and Python xUnit results as Checks and workflow summaries. |
| pytest | Python-Testausführung und xUnit-Berichte in CI | [pytest-dev/pytest](https://github.com/pytest-dev/pytest). MIT-Lizenz. |
| cursor-sdk | Optional Python SDK for batch SLT translation through the Cursor API | Used only by `tools/localization/translate_slt_with_openai.py` when `-Provider cursor` is selected. Install with `pip install cursor-sdk`. |
| OpenRouter API | Optional remote provider for batch SLT translation | Used by `tools/localization/translate_slt_with_openai.py` by default with the `openai/gpt-5.4-nano` model. Requires a user-supplied `OPENROUTER_API_KEY`; no key is stored in the repository. |

## Archivierungs-, Kompressions- und Disk-Image-Plugins

| Komponente | Verwendung in | Hinweise zu Autorrecht und Lizenz |
| --- | --- | --- |
| 7-Zip | 7-ZIP-Plugin | 7-Zip Dateiarchivierungsbibliothek. Copyright (C) 1999-2026 Igor Pavlov. Vertrieben unter GNU LGPL, mit der unRAR-Beschränkung für den RAR-Code. |
| zlib | ZIP-Plugin | Teile des ZIP-Plugins verwenden zlib. Copyright (C) 1995-2002 Jean-loup Gailly und Mark Adler. |
| bzip2 | TAR-Plugin | bzip2-Bibliothek. Copyright (C) 1996-2000 Julian R Seward. |
| ARJ-Dekomprimierungsbibliothek | UnARJ-Plugin | Dekomprimierungsbibliothek geschrieben von ARJ Software, Inc. Copyright (C) 1990-1997 ARJ Software, Inc. |
| Microsoft CAB-Dekomprimierungsbibliothek | UnCAB-Plugin | Dekomprimierungsbibliothek geschrieben von Microsoft Corporation. Copyright (C) Microsoft Corporation 1993-1997. |
| UnRAR | UnRAR-Plugin | Dekomprimierungsbibliothek geschrieben von Alexander Roshal. Copyright (C) 1993-2026 Alexander Roshal. Vertrieben unter der unRAR-Lizenz. Die vcpkg-Pipeline installiert das Paket `unrar` für die Laufzeit-DLL des UnRAR-Plugins. |
| ISZ SDK | UnISO-Plugin | Teile des UnISO-Plugins verwenden ISZ SDK. Copyright (C) 2002-2006 EZB Systems, Inc. Alle Rechte vorbehalten. |
| CHMLIB | UnCHM-Plugin | CHMLIB-Bibliothek. Copyright (C) 2001-2010 Jed Wing. |

## Viewer-, Medien- und Dateiformat-Plugins

| Komponente oder Autor | Verwendung in | Hinweise zu Autorrecht und Lizenz |
| --- | --- | --- |
| Tomas Jelinek | Multimedia Viewer Plugin | Enthält Software geschrieben von Tomas Jelinek. Copyright (C) 2003-2026 Tomas Jelinek. |
| Interner Viewer | Unicode-Viewer-Teile | Samandarin enthält Code und Implementierungsarbeiten, die von Fork [Sally](https://github.com/0xeb/sally) von [Elias Bachaalany (0xeb)](https://github.com/0xeb) abgeleitet oder wesentlich davon inspiriert sind. Lizentiert unter der GNU Library General Public License. |
| Jan Patera | PictView-Plugin | Teile des PictView-Plugins sind von Jan Patera lizenziert. Copyright (C) 1994-2026 Jan Patera. |
| libexif | PictView-Plugin | Teile des PictView-Plugins verwenden libexif. Copyright (C) 2001-2019 Curtis Galloway und Lutz Muller. |
| Newtonsoft.Json 13.0.3 | JSON Viewer Plugin | Von James Newton-King, veröffentlicht unter der MIT-Lizenz. |
| Microsoft .NET Framework 4.8 | JSON Viewer Plugin | Bereitgestellt von Microsoft Corporation unter den Lizenzbedingungen des Microsoft .NET Framework. |
| Prism.js 1.29.0 | Viewer Frame (Salamatrix und Prism Text Viewer) | Von Lea Verou und den PrismJS-Mitwirkenden, veröffentlicht unter der MIT-Lizenz. |
| Microsoft.Web.WebView2 1.0.2420.47 | Viewer Frame | Von Microsoft Corporation, vertrieben unter der BSD 3-Clause-Lizenz. |
| Markdig 0.36.2 | MarkdigRenderer für WebView2 Render Viewer | Von Alexandre Mutel und Mitwirkenden, veröffentlicht unter der BSD 2-Clause-Lizenz. |

## Netzwerk-, Synchronisations- und Geräte-Plugins

| Komponente oder Autor | Verwendung in | Hinweise zu Autorrecht und Lizenz |
| --- | --- | --- |
| OpenSSL | FTP Client Plugin und SFTP-Abhängigkeiten | Copyright (c) 1998-2026 The OpenSSL Project Authors. Alle Rechte vorbehalten. OpenSSL 1.0.x wird unter den OpenSSL- und originalen SSLeay-Lizenzen vertrieben. OpenSSL 3.x wird unter der Apache License 2.0 vertrieben. Die vcpkg-Pipeline installiert OpenSSL für die FTP-Plugin-Kompatibilitäts-DLLs und für die SFTP-Plugin-Abhängigkeiten. |
| libssh2 | SFTP-Plugin | Von Daniel Stenberg, Simon Josefsson und libssh2-Mitwirkenden. Vertrieben unter der BSD 3-Clause-Lizenz. Die vcpkg-Pipeline installiert das Paket `libssh2` für die SFTP-Plugin-Abhängigkeiten. |
| Dupl3xx | SFTP-Plugin | Von Dupl3xx entwickelt und gepflegt. Quellcode-Repository: [salamander-sftp-plugin](https://github.com/Dupl3xx/salamander-sftp-plugin). |
| Martin Prikryl | WinSCP-Plugin | Teile des WinSCP-Plugins sind von Martin Prikryl lizenziert. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Windows Mobile Plugin | Enthält Software geschrieben von Juraj Rojko. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Samandarin Update Notifier Plugin | Bereitgestellt von Microsoft Corporation unter den Lizenzbedingungen des Microsoft .NET Framework. |

## Hardware-Monitor-Erweiterung

| Komponente | Verwendung in | Hinweise zu Autorrecht und Lizenz |
| --- | --- | --- |
| HardView | Hardware-Monitor-Erweiterung | C++/CLI-Brückenbibliothek für Hardware-Überwachung. Copyright (c) 2025 gafoo. Lizentiert unter der MIT License. Quelle: [gafoo173/HardView](https://github.com/gafoo173/HardView). |
| LibreHardwareMonitorLib 0.9.6.0 | Hardware-Monitor-Erweiterung | .NET-Hardware-Überwachungsbibliothek. LibreHardwareMonitorLib ist lizenziert unter der GNU General Public License v3.0. Quelle: [LibreHardwareMonitor/LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor). |
| HidSharp 2.6.4.0 | Hardware-Monitor-Erweiterung | HID-Gerätzugriffsbibliothek, Transitivabhängigkeit von LibreHardwareMonitorLib. Copyright (C) 2010 James F. Bellinger. Lizentiert unter der GNU Lesser General Public License v3.0. |

## Benutzeroberfläche, Dokumentation und Markdown-Rendering

| Komponente oder Beitrag | Verwendung für | Hinweise zu Autorrecht und Lizenz |
| --- | --- | --- |
| cmark-gfm | CommonMark und GitHub-Flavored Markdown Parsing/Rendering | GitHubs Fork von `commonmark/cmark`, eine CommonMark-Parser- und Rendering-Bibliothek und -Programm in C. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Markdown-Dokumentgestaltung | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Tree View Panel-Änderungen | Tree View Panel | Basierend auf Änderungen, die von fgodoy vorgeschlagen wurden. |
| Lokalisierungsskripte | Lokalisierungswerkzeugteile | Samandarin enthält Lokalisierungswerkzeuge, die von Fork [Sally](https://github.com/0xeb/sally) von [Elias Bachaalany (0xeb)](https://github.com/0xeb) angepasst oder wesentlich davon inspiriert sind. Lizentiert unter der GNU Library General Public License. |
| SVG-Symbole für Befehlsshell-Vorlagen | Symbole im Windows-Terminal-Profilmenü | SVG-Grafiken, angepasst aus Microsoft-Produktsymbolen und dem Azure-Cloud-Shell-Symbol von SVG Repo. |
