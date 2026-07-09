# Notificări despre software-ul terților

[Open Salamander: Samandarin](https://github.com/KRtkovo-eu-AI/salamander) se bazează pe munca multor autori, maintaineri și
proiecte. Această pagină enumeră componentele, codul, ideile și instrumentele
folosite de aplicație și pluginurile sale. Mulțumim tuturor celor enumerați aici
pentru punerea la dispoziție a muncii lor.

## Aplicația principală și bibliotecile partajate

| Component | Utilizare | Note despre atribuire și licență |
| --- | --- | --- |
| REGEXP | Potrivirea expresiilor regulate | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| Cod AES | Rutine criptografice | Scris de Dr Brian Gladman. Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | Suport pentru imagini PNG | Bazat pe PNGLite de Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | Suport pentru parsarea/redarea SVG | Copyright (c) 2013-2014 Mikko Mononen. |
| SQLite | Suport pentru baza de date încorporată | SQLite este în domeniul public. |
| LibTomCrypt | Primitive criptografice ale pluginului de verificare | LibTomCrypt este în domeniul public. După cum a scris Tom St Denis: „Așa cum ar trebui să fie orice software de calitate." |
| Biblioteci LGPL | Biblioteci terțe partajate | Licențiate sub GNU Library General Public License. O copie a licenței este inclusă cu acest software. |
| Unicode și căi lungi Win32 | Părți de manipulare a numelor de fișiere | Samandarin conține cod și munca de implementare derivată din forkul [Sally](https://github.com/0xeb/sally) al [Elias Bachaalany (0xeb)](https://github.com/0xeb), sau substanțial inspirată de acesta. Licențiat sub GNU Library General Public License. |

## Pluginuri de arhivare, compresie și imagine de disc

| Component | Utilizat în | Note despre atribuire și licență |
| --- | --- | --- |
| 7-Zip | Plugin 7-ZIP | Bibliotecă de arhivare fișiere 7-Zip. Copyright (C) 1999-2026 Igor Pavlov. Distribuit sub GNU LGPL, cu restricția unRAR pentru codul RAR. |
| zlib | Plugin ZIP | Părți din pluginul ZIP folosesc zlib. Copyright (C) 1995-2002 Jean-loup Gailly și Mark Adler. |
| bzip2 | Plugin TAR | Bibliotecă bzip2. Copyright (C) 1996-2000 Julian R Seward. |
| Bibliotecă de decompresie ARJ | Plugin UnARJ | Bibliotecă de decompresie scrisă de ARJ Software, Inc. Copyright (C) 1990-1997 ARJ Software, Inc. |
| Bibliotecă de decompresie Microsoft CAB | Plugin UnCAB | Bibliotecă de decompresie scrisă de Microsoft Corporation. Copyright (C) Microsoft Corporation 1993-1997. |
| UnRAR | Plugin UnRAR | Bibliotecă de decompresie scrisă de Alexander Roshal. Copyright (C) 1993-2026 Alexander Roshal. Distribuit sub licența unRAR. Pipeline-ul vcpkg instalează pachetul `unrar` pentru DLL-ul de runtime al pluginului UnRAR. |
| ISZ SDK | Plugin UnISO | Părți din pluginul UnISO folosesc ISZ SDK. Copyright (C) 2002-2006 EZB Systems, Inc. Toate drepturile rezervate. |
| CHMLIB | Plugin UnCHM | Bibliotecă CHMLIB. Copyright (C) 2001-2010 Jed Wing. |

## Pluginuri de vizualizare, media și format de fișier

| Component sau autor | Utilizat în | Note despre atribuire și licență |
| --- | --- | --- |
| Tomas Jelinek | Plugin Multimedia Viewer | Include software scris de Tomas Jelinek. Copyright (C) 2003-2026 Tomas Jelinek. |
| Vizualizator intern | Părți ale vizualizatorului Unicode | Samandarin conține cod și munca de implementare derivată din forkul [Sally](https://github.com/0xeb/sally) al [Elias Bachaalany (0xeb)](https://github.com/0xeb), sau substanțial inspirată de acesta. Licențiat sub GNU Library General Public License. |
| Jan Patera | Plugin PictView | Părți din pluginul PictView sunt licențiate de la Jan Patera. Copyright (C) 1994-2026 Jan Patera. |
| libexif | Plugin PictView | Părți din pluginul PictView folosesc libexif. Copyright (C) 2001-2019 Curtis Galloway și Lutz Muller. |
| Newtonsoft.Json 13.0.3 | Plugin JSON Viewer | De James Newton-King, publicat sub licența MIT. |
| Microsoft .NET Framework 4.8 | Plugin JSON Viewer | Furnizat de Microsoft Corporation sub termenii licenței Microsoft .NET Framework. |
| PrismSharp 1.0.0-beta | Plugin PrismSharp Text Viewer | De Tomáš Kubec, publicat sub licența MIT. Include Prism.js de Lea Verou și contribuitorii PrismJS sub licența MIT. |
| Microsoft.Web.WebView2 1.0.2420.47 | Plugin PrismSharp Text Viewer | De Microsoft Corporation, distribuit sub licența BSD 3-Clause. |
| Microsoft .NET Framework 4.8 | Plugin PrismSharp Text Viewer | Furnizat de Microsoft Corporation sub termenii licenței Microsoft .NET Framework. |
| Newtonsoft.Json 13.0.3 | Plugin PrismSharp Text Viewer | De James Newton-King, publicat sub licența MIT. |
| Markdig 0.36.2 | Plugin WebView2 Render Viewer | De Alexandre Mutel și contribuitori, publicat sub licența BSD 2-Clause. |
| Microsoft.Web.WebView2 1.0.2420.47 | Plugin WebView2 Render Viewer | De Microsoft Corporation, distribuit sub licența BSD 3-Clause. |
| Microsoft .NET Framework 4.8 | Plugin WebView2 Render Viewer | Furnizat de Microsoft Corporation sub termenii licenței Microsoft .NET Framework. |

## Pluginuri de rețea, sincronizare și dispozitive

| Component sau autor | Utilizat în | Note despre atribuire și licență |
| --- | --- | --- |
| OpenSSL | Plugin FTP Client și dependențe SFTP | Copyright (c) 1998-2026 The OpenSSL Project Authors. Toate drepturile rezervate. OpenSSL 1.0.x este distribuit sub licențele OpenSSL și SSLeay originale. OpenSSL 3.x este distribuit sub Apache License 2.0. Pipeline-ul vcpkg instalează OpenSSL pentru DLL-urile de compatibilitate ale pluginului FTP și pentru dependențele pluginului SFTP. |
| libssh2 | Plugin SFTP | De Daniel Stenberg, Simon Josefsson și contribuitorii libssh2. Distribuit sub licența BSD 3-Clause. Pipeline-ul vcpkg instalează pachetul `libssh2` pentru dependențele pluginului SFTP. |
| Martin Prikryl | Plugin WinSCP | Părți din pluginul WinSCP sunt licențiate de la Martin Prikryl. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Plugin Windows Mobile | Include software scris de Juraj Rojko. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Plugin Samandarin Update Notifier | Furnizat de Microsoft Corporation sub termenii licenței Microsoft .NET Framework. |

## Interfața cu utilizatorul, documentația și redarea Markdown

| Component sau contribuție | Utilizat pentru | Note despre atribuire și licență |
| --- | --- | --- |
| cmark-gfm | Parsarea/redarea CommonMark și GitHub-Flavored Markdown | Fork-ul GitHub al `commonmark/cmark`, o bibliotecă și program de parsare și redare CommonMark în C. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Stilizarea documentelor Markdown | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Modificări ale panoului Tree View | Panoul Tree View | Bazat pe modificările propuse de fgodoy. |
| Scripturi de localizare | Părți de instrumente de localizare | Samandarin conține instrumente de localizare adaptate din forkul [Sally](https://github.com/0xeb/sally) al [Elias Bachaalany (0xeb)](https://github.com/0xeb), sau substanțial inspirate de acesta. Licențiat sub GNU Library General Public License. |
