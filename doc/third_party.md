# Third-Party Software Notices

[Open Salamander: Samandarin](https://samandarin.net/) builds on the work of many authors, maintainers, and
projects. This page records the components, code, ideas, and tooling that are
used by the application and its plugins. Thank you to everyone listed here for
making their work available.

## Core Application and Shared Libraries

| Component | Used for | Attribution and license notes |
| --- | --- | --- |
| REGEXP | Regular expression matching | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| AES code | Cryptographic routines | Written by Dr Brian Gladman. Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | PNG image support | Based on PNGLite by Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | SVG parsing/rendering support | Copyright (c) 2013-2014 Mikko Mononen. |
| SQLite | Embedded database support | SQLite is in the Public Domain. |
| LibTomCrypt | Checksum plugin cryptographic primitives | LibTomCrypt is public domain. As Tom St Denis wrote: "As should all quality software be." |
| LGPL libraries | Shared third-party libraries | Licensed under the GNU Library General Public License. A copy of the license is included with this software. |
| Unicode and Win32 Long Paths | Filename handling portions | Samandarin includes filename handling code and implementation work derived from, or substantially informed by, fork [Sally](https://github.com/0xeb/sally) by [Elias Bachaalany (0xeb)](https://github.com/0xeb). Licensed under the GNU Library General Public License. |
| Lua 5.5.0 | Bundled interpreter for the Salamatrix Lua Runtime | [Lua.org, PUC-Rio](https://www.lua.org/), installed from the pinned vcpkg `lua[tools]` package. Copyright (c) 1994-2025 Lua.org, PUC-Rio. MIT License; the complete notice is distributed as `plugins/extension-runtimes/luaruntime/runtime/LICENSE-LUA.txt`. |
| win32-darkmodelib | Native Windows dark-mode support for shared and Salamatrix dialogs | Vendored from [ozone10/win32-darkmodelib](https://github.com/ozone10/win32-darkmodelib), commit `b58eb027c4a114c1b1c8fd09870ead982f1b1e72`. Copyright (c) 2025-2026 ozone10. Licensed under the Mozilla Public License 2.0, with portions under the MIT License; see `src/third_party/darkmodelib/LICENSE.md` and `LICENSE-MIT.md`. |
| llama.cpp | On-demand CPU inference executable for SalamatrixAI | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp), pinned release `b10107`. MIT License; the configuration downloader retrieves the runtime and its license notice directly for the user. |
| Qwen2.5-Coder 1.5B Instruct GGUF | Recommended local SalamatrixAI model | [Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF), pinned `q4_k_m` file. Apache License 2.0; the configuration downloader retrieves the model and license notice directly for the user. |
| Qwen2.5-Coder 0.5B Instruct GGUF | Lightweight, English-prompt-only local SalamatrixAI model | [Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF), pinned `q4_k_m` file. Apache License 2.0; the configuration downloader retrieves the model and license notice directly for the user. |
| Test Reporter | Pull-request test result reports | [dorny/test-reporter](https://github.com/dorny/test-reporter), used by GitHub Actions to publish JUnit and Python xUnit results as Checks and workflow summaries. MIT License. |
| pytest | Python test execution and xUnit report generation in pull-request CI | [pytest-dev/pytest](https://github.com/pytest-dev/pytest). MIT License. |

## Archiving, Compression, and Disk Image Plugins

| Component | Used in | Attribution and license notes |
| --- | --- | --- |
| 7-Zip | 7-ZIP plugin | 7-Zip file archiver library. Copyright (C) 1999-2026 Igor Pavlov. Distributed under the GNU LGPL, with the unRAR restriction for the RAR code. |
| zlib | ZIP plugin | Portions of the ZIP plugin use zlib. Copyright (C) 1995-2002 Jean-loup Gailly and Mark Adler. |
| bzip2 | TAR plugin | bzip2 library. Copyright (C) 1996-2000 Julian R Seward. |
| ARJ decompression library | UnARJ plugin | Decompression library written by ARJ Software, Inc. Copyright (C) 1990-1997 ARJ Software, Inc. |
| Microsoft CAB decompression library | UnCAB plugin | Decompression library written by Microsoft Corporation. Copyright (C) Microsoft Corporation 1993-1997. |
| UnRAR | UnRAR plugin | Decompression library written by Alexander Roshal. Copyright (C) 1993-2026 Alexander Roshal. Distributed under the unRAR license. The vcpkg pipeline installs the `unrar` package for the UnRAR plugin runtime DLL. |
| ISZ SDK | UnISO plugin | Portions of the UnISO plugin use ISZ SDK. Copyright (C) 2002-2006 EZB Systems, Inc. All rights reserved. |
| CHMLIB | UnCHM plugin | CHMLIB library. Copyright (C) 2001-2010 Jed Wing. |

## Viewer, Media, and File Format Plugins

| Component or author | Used in | Attribution and license notes |
| --- | --- | --- |
| Tomas Jelinek | Multimedia Viewer plugin | Includes software written by Tomas Jelinek. Copyright (C) 2003-2026 Tomas Jelinek. |
| Internal viewer | Unicode viewer portions | Samandarin includes code and implementation work derived from, or substantially informed by, fork [Sally](https://github.com/0xeb/sally) by [Elias Bachaalany (0xeb)](https://github.com/0xeb). Licensed under the GNU Library General Public License. |
| Jan Patera | PictView plugin | Portions of the PictView plugin are licensed from Jan Patera. Copyright (C) 1994-2026 Jan Patera. |
| libexif | PictView plugin | Portions of the PictView plugin use libexif. Copyright (C) 2001-2019 Curtis Galloway and Lutz Muller. |
| Newtonsoft.Json 13.0.3 | JSON Viewer plugin | By James Newton-King, released under the MIT License. |
| Microsoft .NET Framework 4.8 | JSON Viewer plugin | Provided by Microsoft Corporation under the Microsoft .NET Framework license terms. |
| PrismSharp 1.0.0-beta | PrismSharp Text Viewer plugin | By Tomáš Kubec, released under the MIT License. Includes Prism.js by Lea Verou and the PrismJS contributors under the MIT License. |
| Microsoft.Web.WebView2 1.0.2420.47 | PrismSharp Text Viewer plugin | By Microsoft Corporation, distributed under the BSD 3-Clause License. |
| Microsoft .NET Framework 4.8 | PrismSharp Text Viewer plugin | Provided by Microsoft Corporation under the Microsoft .NET Framework license terms. |
| Newtonsoft.Json 13.0.3 | PrismSharp Text Viewer plugin | By James Newton-King, released under the MIT License. |
| Markdig 0.36.2 | WebView2 Render Viewer plugin | By Alexandre Mutel and contributors, released under the BSD 2-Clause License. |
| Microsoft.Web.WebView2 1.0.2420.47 | WebView2 Render Viewer plugin | By Microsoft Corporation, distributed under the BSD 3-Clause License. |
| Microsoft .NET Framework 4.8 | WebView2 Render Viewer plugin | Provided by Microsoft Corporation under the Microsoft .NET Framework license terms. |

## Network, Synchronization, and Device Plugins

| Component or author | Used in | Attribution and license notes |
| --- | --- | --- |
| OpenSSL | FTP Client plugin and SFTP dependencies | Copyright (c) 1998-2026 The OpenSSL Project Authors. All rights reserved. OpenSSL 1.0.x is distributed under the OpenSSL and original SSLeay licenses. OpenSSL 3.x is distributed under the Apache License 2.0. The vcpkg pipeline installs OpenSSL for the FTP plugin compatibility DLLs and for the SFTP plugin dependencies. |
| libssh2 | SFTP plugin | By Daniel Stenberg, Simon Josefsson, and libssh2 contributors. Distributed under the BSD 3-Clause License. The vcpkg pipeline installs the `libssh2` package for the SFTP plugin dependencies. |
| Dupl3xx | SFTP plugin | Plugin authored and maintained by Dupl3xx. Source repository: [salamander-sftp-plugin](https://github.com/Dupl3xx/salamander-sftp-plugin). |
| Martin Prikryl | WinSCP plugin | Portions of the WinSCP plugin are licensed from Martin Prikryl. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Windows Mobile plugin | Includes software written by Juraj Rojko. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Samandarin Update Notifier plugin | Provided by Microsoft Corporation under the Microsoft .NET Framework license terms. |

## User Interface, Documentation, and Markdown Rendering

| Component or contribution | Used for | Attribution and license notes |
| --- | --- | --- |
| cmark-gfm | CommonMark and GitHub-Flavored Markdown parsing/rendering | GitHub's fork of `commonmark/cmark`, a CommonMark parsing and rendering library and program in C. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Markdown document styling | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Base of Tree View panel | Tree View panel | Based on changes proposed by fgodoy. |
| Localization scripts | Localization tooling portions | Samandarin includes localization tooling adapted from, or substantially informed by, fork [Sally](https://github.com/0xeb/sally) by [Elias Bachaalany (0xeb)](https://github.com/0xeb). Licensed under the GNU Library General Public License. |
| Command shell template SVG icons | Windows Terminal profile menu icons | SVG artwork adapted from Microsoft product icon shapes for PowerShell, Windows PowerShell, Windows Terminal, Command Prompt, and Visual Studio, and from the SVG Repo Azure Cloud Shell icon. |
