# Oznámenia o softvéri tretích strán

[Open Salamander: Samandarin](https://samandarin.krtkovo.eu/) je postavený na práci mnohých autorov, správcov a
projektov. Táto stránka uvádza komponenty, kód, nápady a nástroje, ktoré
aplikácia a jej pluginy využívajú. Ďakujeme všetkým tu uvedeným za
sprístupnenie ich práce.

## Jadro aplikácie a zdieľané knižnice

| Komponent | Použitie | Informácie o autorstve a licencii |
| --- | --- | --- |
| REGEXP | Regulárne výrazy | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| AES kód | Kryptografické rutiny | Napísal Dr Brian Gladman. Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | Podpora obrázkov PNG | Založené na PNGLite od Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | Podpora parsovania/vykresľovania SVG | Copyright (c) 2013-2014 Mikko Mononen. |
| SQLite | Podpora vloženej databázy | SQLite je vo verejnej doméne. |
| LibTomCrypt | Kryptografické prvky pluginu kontrolných súčtov | LibTomCrypt je vo verejnej doméne. Ako napísal Tom St Denis: „Ako by mal byť každý kvalitný softvér." |
| LGPL knižnice | Zdieľané knižnice tretích strán | Licencované pod GNU Library General Public License. Kópia licencie je súčasťou tohto softvéru. |
| Unicode a Win32 dlhé cesty | Časti spracovania názvov súborov | Samandarin obsahuje kód a implementačné práce odvodené z fork [Sally](https://github.com/0xeb/sally) od [Elias Bachaalany (0xeb)](https://github.com/0xeb), alebo podstatne inšpirované ním. Licencované pod GNU Library General Public License. |
| Lua 5.5.0 | Pribalený interpreter pre Salamatrix Lua Runtime | [Lua.org, PUC-Rio](https://www.lua.org/), nainštalovaný z pripnutého vcpkg balíka `lua[tools]`. Copyright (c) 1994-2025 Lua.org, PUC-Rio. Licencia MIT; úplné oznámenie je distribuované ako `plugins/extension-runtimes/luaruntime/runtime/LICENSE-LUA.txt`. |
| win32-darkmodelib | Natívna podpora tmavého režimu Windows pre zdieľané dialógy a dialógy Salamatrix | Prevzaté z [ozone10/win32-darkmodelib](https://github.com/ozone10/win32-darkmodelib), commit `b58eb027c4a114c1b1c8fd09870ead982f1b1e72`. Licencia Mozilla Public License 2.0, časti pod licenciou MIT. |
| llama.cpp | Spustiteľný súbor pre lokálnu CPU inferenciu SalamatrixAI na požiadanie | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp), pripnuté vydanie `b10107`. Licencia MIT. |
| Qwen2.5-Coder 1.5B Instruct GGUF | Odporúčaný lokálny model SalamatrixAI | [Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF), pripnutý súbor `q4_k_m`. Licencia Apache 2.0. |
| Qwen2.5-Coder 0.5B Instruct GGUF | Ľahký lokálny model SalamatrixAI iba pre anglické prompty | [Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF), pripnutý súbor `q4_k_m`. Licencia Apache 2.0. |
| Test Reporter | Správy s výsledkami testov pull requestov | [dorny/test-reporter](https://github.com/dorny/test-reporter), používané v GitHub Actions. Licencia MIT. |
| pytest | Spúšťanie Python testov a generovanie xUnit reportov v CI | [pytest-dev/pytest](https://github.com/pytest-dev/pytest). Licencia MIT. |

## Pluginy pre archiváciu, kompresiu a diskové obrazy

| Komponent | Použitie v | Informácie o autorstve a licencii |
| --- | --- | --- |
| 7-Zip | Plugin 7-ZIP | Knižnica pre archiváciu súborov 7-Zip. Copyright (C) 1999-2026 Igor Pavlov. Distribuované pod GNU LGPL, s obmedzením unRAR pre kód RAR. |
| zlib | Plugin ZIP | Časti pluginu ZIP používajú zlib. Copyright (C) 1995-2002 Jean-loup Gailly a Mark Adler. |
| bzip2 | Plugin TAR | Knižnica bzip2. Copyright (C) 1996-2000 Julian R Seward. |
| ARJ dekomprimačná knižnica | Plugin UnARJ | Dekomprimačná knižnica od ARJ Software, Inc. Copyright (C) 1990-1997 ARJ Software, Inc. |
| Microsoft CAB dekomprimačná knižnica | Plugin UnCAB | Dekomprimačná knižnica od Microsoft Corporation. Copyright (C) Microsoft Corporation 1993-1997. |
| UnRAR | Plugin UnRAR | Dekomprimačná knižnica od Alexander Roshal. Copyright (C) 1993-2026 Alexander Roshal. Distribuované pod licenciou unRAR. Vcpkg pipeline inštaluje balík `unrar` pre runtime DLL pluginu UnRAR. |
| ISZ SDK | Plugin UnISO | Časti pluginu UnISO používajú ISZ SDK. Copyright (C) 2002-2006 EZB Systems, Inc. Všetky práva vyhradené. |
| CHMLIB | Plugin UnCHM | Knižnica CHMLIB. Copyright (C) 2001-2010 Jed Wing. |

## Pluginy pre prehliadač, multimédiá a formáty súborov

| Komponent alebo autor | Použitie v | Informácie o autorstve a licencii |
| --- | --- | --- |
| Tomas Jelinek | Plugin Multimedia Viewer | Obsahuje softvér napísaný Tomášom Jelínkom. Copyright (C) 2003-2026 Tomas Jelinek. |
| Interný prehliadač | Časti prehliadača Unicode | Samandarin obsahuje kód a implementačné práce odvodené z fork [Sally](https://github.com/0xeb/sally) od [Elias Bachaalany (0xeb)](https://github.com/0xeb), alebo podstatne inšpirované ním. Licencované pod GNU Library General Public License. |
| Jan Patera | Plugin PictView | Časti pluginu PictView sú licencované od Jana Patery. Copyright (C) 1994-2026 Jan Patera. |
| libexif | Plugin PictView | Časti pluginu PictView používajú libexif. Copyright (C) 2001-2019 Curtis Galloway a Lutz Muller. |
| Newtonsoft.Json 13.0.3 | Plugin JSON Viewer | Od Jamese Newton-Kinga, vydané pod MIT License. |
| Microsoft .NET Framework 4.8 | Plugin JSON Viewer | Poskytnuté Microsoft Corporation pod licenčnými podmienkami Microsoft .NET Framework. |
| PrismSharp 1.0.0-beta | Plugin PrismSharp Text Viewer | Od Tomáša Kubca, vydané pod MIT License. Obsahuje Prism.js od Lea Verou a prispievateľov PrismJS pod MIT License. |
| Microsoft.Web.WebView2 1.0.2420.47 | Plugin PrismSharp Text Viewer | Od Microsoft Corporation, distribuované pod BSD 3-Clause License. |
| Microsoft .NET Framework 4.8 | Plugin PrismSharp Text Viewer | Poskytnuté Microsoft Corporation pod licenčnými podmienkami Microsoft .NET Framework. |
| Newtonsoft.Json 13.0.3 | Plugin PrismSharp Text Viewer | Od Jamese Newton-Kinga, vydané pod MIT License. |
| Markdig 0.36.2 | Plugin WebView2 Render Viewer | Od Alexendra Mutela a prispievateľov, vydané pod BSD 2-Clause License. |
| Microsoft.Web.WebView2 1.0.2420.47 | Plugin WebView2 Render Viewer | Od Microsoft Corporation, distribuované pod BSD 3-Clause License. |
| Microsoft .NET Framework 4.8 | Plugin WebView2 Render Viewer | Poskytnuté Microsoft Corporation pod licenčnými podmienkami Microsoft .NET Framework. |

## Pluginy pre sieť, synchronizáciu a zariadenia

| Komponent alebo autor | Použitie v | Informácie o autorstve a licencii |
| --- | --- | --- |
| OpenSSL | Plugin FTP Client a závislosti SFTP | Copyright (c) 1998-2026 The OpenSSL Project Authors. Všetky práva vyhradené. OpenSSL 1.0.x je distribuované pod OpenSSL a pôvodnými SSLeay licenciami. OpenSSL 3.x je distribuované pod Apache License 2.0. Vcpkg pipeline inštaluje OpenSSL pre kompatibilitu DLL pluginu FTP a pre závislosti pluginu SFTP. |
| libssh2 | Plugin SFTP | Od Daniela Stenberga, Simona Josefa a prispievateľov libssh2. Distribuované pod BSD 3-Clause License. Vcpkg pipeline inštaluje balík `libssh2` pre závislosti pluginu SFTP. |
| Martin Prikryl | Plugin WinSCP | Časti pluginu WinSCP sú licencované od Martina Prikryla. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Plugin Windows Mobile | Obsahuje softvér napísaný Jurajom Rojkom. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Plugin Samandarin Update Notifier | Poskytnuté Microsoft Corporation pod licenčnými podmienkami Microsoft .NET Framework. |

## Používateľské rozhranie, dokumentácia a vykresľovanie Markdownu

| Komponent alebo príspevok | Použitie pre | Informácie o autorstve a licencii |
| --- | --- | --- |
| cmark-gfm | Parsovanie/vykresľovanie CommonMark a GitHub-Flavored Markdown | GitHubov fork `commonmark/cmark`, knižnica a program pre parsovanie a vykresľovanie CommonMark v C. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Štýlovanie dokumentov Markdown | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Základ panelu Tree View | Panel Tree View | Založené na zmenách navrhnutých fgodoy. |
| Lokalizačné skripty | Časti nástrojov pre lokalizáciu | Samandarin obsahuje nástroje pre lokalizáciu upravené z fork [Sally](https://github.com/0xeb/sally) od [Elias Bachaalany (0xeb)](https://github.com/0xeb), alebo podstatne inšpirované ním. Licencované pod GNU Library General Public License. |
| SVG ikony šablón príkazových prostredí | Ikony ponuky profilov Windows Terminal | SVG grafika upravená podľa ikon produktov Microsoft a ikony Azure Cloud Shell zo SVG Repo. |
