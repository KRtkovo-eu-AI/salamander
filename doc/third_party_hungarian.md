# Harmadik fél szoftvereinek értesítései

[Open Salamander: Samandarin](https://samandarin.net/) számos szerző, karbantartó és projekt munkájára
épül. Ez az oldal felsorolja az alkalmazás és bővítményei által használt
összetevőket, kódot, ötleteket és eszközöket. Köszönjük mindenkinek, aki itt
szerepel, hogy hozzáférhetővé tették munkájukat.

## Alkalmazás magja és megosztott könyvtárak

| Összetevő | Felhasználás | Szerzői jog és licenc megjegyzések |
| --- | --- | --- |
| REGEXP | Reguláris kifejezések illesztése | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| AES kód | Kriptográfiai rutinok | Írta Dr Brian Gladman. Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | PNG kép támogatás | A PNGLite alapján, szerző: Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | SVG megjelenítési támogatás | Copyright (c) 2013-2014 Mikko Mononen. A főalkalmazás eszköztár- és menüikonjaihoz, valamint a PictView bővítmény SVG-megjelenítéséhez használatos (NanoSVG korlátok: nincs CSS, korlátozott szöveg és szűrők). |
| PictView deflate inflater | PictView bővítmény | Deflate/zlib inflater XCF csempékhez, PNG IDAT-hoz, SKP ZIP-hez és tömörített 3DM előnézetekhez. A bitolvasó és a Huffman puff alapján készült (Mark Adler). Copyright (C) 2002-2013 Mark Adler. zlib licenc. |
| openNURBS 3DM preview layout | PictView plugin | Rhinoceros `.3dm` properties-table preview chunks and `ON_WindowsBitmap::ReadCompressed` layout follow [McNeel openNURBS](https://github.com/mcneel/opennurbs). Copyright (c) Robert McNeel & Associates. Used as a format reference only; openNURBS itself is not bundled. |
| SQLite | Beágyazott adatbázis támogatás | SQLite közszférában van. |
| LibTomCrypt | Ellenőrző összeg bővítmény kriptográfiai alapelemei | LibTomCrypt közszférában van. Ahogy Tom St Denis írta: „Ahogy minden minőségi szoftvernek kellene lennie." |
| LGPL könyvtárak | Megosztott harmadik fél könyvtárak | A GNU Library General Public License alatt licencelve. A licence másolata mellékelve van ezzel a szoftverrel. |
| Unicode és Win32 hosszú elérési utak | Fájlnév kezelés részek | Samandarin tartalmaz kódot és megvalósítási munkákat, amelyek a fork [Sally](https://github.com/0xeb/sally) [Elias Bachaalany (0xeb)](https://github.com/0xeb) származékai, vagy jelentősen abból inspiráltak. A GNU Library General Public License alatt licencelve. |
| Lua 5.5.0 | Beépített értelmező a Salamatrix Lua Runtime számára | [Lua.org, PUC-Rio](https://www.lua.org/), a rögzített `lua[tools]` vcpkg csomagból telepítve. Copyright (c) 1994-2025 Lua.org, PUC-Rio. MIT licenc; a teljes nyilatkozat helye: `plugins/extension-runtimes/luaruntime/runtime/LICENSE-LUA.txt`. |
| win32-darkmodelib | Natív Windows sötét mód támogatás a közös és Salamatrix párbeszédablakokhoz | Forrás: [ozone10/win32-darkmodelib](https://github.com/ozone10/win32-darkmodelib), commit `b58eb027c4a114c1b1c8fd09870ead982f1b1e72`. Mozilla Public License 2.0, egyes részek MIT licenc alatt.; see `src/third_party/darkmodelib/LICENSE.md` and `LICENSE-MIT.md`. |
| llama.cpp | Helyi SalamatrixAI CPU-következtetési futtatható állomány | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp), rögzített `b10107` kiadás. MIT licenc. The configuration downloader retrieves the runtime and its license notice directly for the user. |
| Qwen2.5-Coder 1.5B Instruct GGUF | Ajánlott helyi SalamatrixAI modell | [Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF), rögzített `q4_k_m` fájl. Apache License 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Qwen2.5-Coder 0.5B Instruct GGUF | Könnyű helyi SalamatrixAI modell, csak angol promptokhoz | [Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF), rögzített `q4_k_m` fájl. Apache License 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Test Reporter | Pull request teszteredmény-jelentések | [dorny/test-reporter](https://github.com/dorny/test-reporter), GitHub Actions használatban. MIT licenc. It publishes JUnit and Python xUnit results as Checks and workflow summaries. |
| pytest | Python tesztfuttatás és xUnit jelentések a CI-ben | [pytest-dev/pytest](https://github.com/pytest-dev/pytest). MIT licenc. |
| cursor-sdk | Optional Python SDK for batch SLT translation through the Cursor API | Used only by `tools/localization/translate_slt_with_openai.py` when `-Provider cursor` is selected. Install with `pip install cursor-sdk`. |
| OpenRouter API | Optional remote provider for batch SLT translation | Used by `tools/localization/translate_slt_with_openai.py` by default with the `openai/gpt-5.4-nano` model. Requires a user-supplied `OPENROUTER_API_KEY`; no key is stored in the repository. |

## Archiválási, tömörítési és lemezkép bővítmények

| Összetevő | Felhasználás itt | Szerzői jog és licenc megjegyzések |
| --- | --- | --- |
| 7-Zip | 7-ZIP bővítmény | 7-Zip fájl archiváló könyvtár. Copyright (C) 1999-2026 Igor Pavlov. A GNU LGPL alatt terjesztve, a RAR kódra vonatkozó unRAR korlátozással. |
| zlib | ZIP bővítmény | A ZIP bővítmény egyes részei zlib-t használnak. Copyright (C) 1995-2002 Jean-loup Gailly és Mark Adler. |
| bzip2 | TAR bővítmény | bzip2 könyvtár. Copyright (C) 1996-2000 Julian R Seward. |
| ARJ dekomprimáló könyvtár | UnARJ bővítmény | Dekomprimáló könyvtár, írta: ARJ Software, Inc. Copyright (C) 1990-1997 ARJ Software, Inc. |
| Microsoft CAB dekomprimáló könyvtár | UnCAB bővítmény | Dekomprimáló könyvtár, írta: Microsoft Corporation. Copyright (C) Microsoft Corporation 1993-1997. |
| UnRAR | UnRAR bővítmény | Dekomprimáló könyvtár, írta: Alexander Roshal. Copyright (C) 1993-2026 Alexander Roshal. Az unlicence alatt terjesztve. A vcpkg csővezeték telepíti az `unrar` csomagot az UnRAR bővítmény futtatható DLL-jéhez. |
| ISZ SDK | UnISO bővítmény | Az UnISO bővítmény egyes részei ISZ SDK-t használnak. Copyright (C) 2002-2006 EZB Systems, Inc. Minden jog fenntartva. |
| CHMLIB | UnCHM bővítmény | CHMLIB könyvtár. Copyright (C) 2001-2010 Jed Wing. |

## Nézegető, média és fájlformátum bővítmények

| Összetevő vagy szerző | Felhasználás itt | Szerzői jog és licenc megjegyzések |
| --- | --- | --- |
| Tomas Jelinek | Multimedia Viewer bővítmény | Tartalmaz Tomas Jelinek által írt szoftvert. Copyright (C) 2003-2026 Tomas Jelinek. |
| TagLib 2.3 | Multimedia Viewer bővítmény | Scott Wheeler és a TagLib közreműködőinek hangmetaadat-könyvtára. Unicode metaadatokhoz és hangtulajdonságokhoz használjuk Ogg Vorbis, Opus, FLAC, MP4, APE, WavPack, TrueAudio, AIFF, DSD, Matroska/WebM és kapcsolódó formátumokban. A GNU LGPL 2.1 vagy a Mozilla Public License 1.1 feltételei szerint terjesztve. Forrás: https://github.com/taglib/taglib |
| Belső nézegető | Unicode nézegető részek | Samandarin tartalmaz kódot és megvalósítási munkákat, amelyek a fork [Sally](https://github.com/0xeb/sally) [Elias Bachaalany (0xeb)](https://github.com/0xeb) származékai, vagy jelentősen abból inspiráltak. A GNU Library General Public License alatt licencelve. |
| Jan Patera | PictView bővítmény | A PictView bővítmény egyes részei Jan Patera licencéből származnak. Copyright (C) 1994-2026 Jan Patera. |
| libexif | PictView bővítmény | A PictView bővítmény egyes részei libexif-et használnak. Copyright (C) 2001-2019 Curtis Galloway és Lutz Muller. |
| Newtonsoft.Json 13.0.3 | JSON Viewer bővítmény | James Newton-King, MIT Licenc alatt kiadva. |
| Microsoft .NET Framework 4.8 | JSON Viewer bővítmény | A Microsoft Corporation biztosítja a Microsoft .NET Framework licenc feltételei szerint. |
| Prism.js 1.29.0 | Viewer Frame (Salamatrix és Prism Text Viewer) | Lea Verou és a PrismJS közreműködők, MIT Licenc alatt kiadva. |
| Microsoft.Web.WebView2 1.0.2420.47 | Viewer Frame | Microsoft Corporation, BSD 3-Clause Licenc alatt terjesztve. |
| Markdig 0.36.2 | MarkdigRenderer a WebView2 Render Viewerhez | Alexandre Mutel és közreműködők, BSD 2-Clause Licenc alatt kiadva. |

## Hálózati, szinkronizációs és eszköz bővítmények

| Összetevő vagy szerző | Felhasználás itt | Szerzői jog és licenc megjegyzések |
| --- | --- | --- |
| OpenSSL | FTP Client bővítmény és SFTP függőségek | Copyright (c) 1998-2026 The OpenSSL Project Authors. Minden jog fenntartva. Az OpenSSL 1.0.x az OpenSSL és az eredeti SSLeay licencek alatt terjesztve. Az OpenSSL 3.x az Apache License 2.0 alatt terjesztve. A vcpkg csővezeték telepíti az OpenSSL-t a FTP bővítmény kompatibilitási DLL-jeihez és az SFTP bővítmény függőségeihez. |
| libssh2 | SFTP bővítmény | Daniel Stenberg, Simon Josefsson és a libssh2 közreműködők. BSD 3-Clause Licenc alatt terjesztve. A vcpkg csővezeték telepíti a `libssh2` csomagot az SFTP bővítmény függőségeihez. |
| Dupl3xx | SFTP bővítmény | A bővítményt Dupl3xx készítette és tartja karban. Forráskód-tárhely: [salamander-sftp-plugin](https://github.com/Dupl3xx/salamander-sftp-plugin). |
| Martin Prikryl | WinSCP bővítmény | A WinSCP bővítmény egyes részei Martin Prikryl licencéből származnak. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Windows Mobile bővítmény | Tartalmaz Juraj Rojko által írt szoftvert. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Samandarin Update Notifier bővítmény | A Microsoft Corporation biztosítja a Microsoft .NET Framework licenc feltételei szerint. |

## Hardverfigyelő kiterjesztés

| Összetevő | Felhasználás | Szerzői jog és licenc megjegyzések |
| --- | --- | --- |
| HardView | Hardverfigyelő kiterjesztés | C++/CLI híd könyvtár hardverfigyeléshez. Copyright (c) 2025 gafoo. MIT License alatt licencelve. Forrás: [gafoo173/HardView](https://github.com/gafoo173/HardView). |
| LibreHardwareMonitorLib 0.9.6.0 | Hardverfigyelő kiterjesztés | .NET hardverfigyelési könyvtár. A LibreHardwareMonitorLib a GNU General Public License v3.0 alatt van licencelve. Forrás: [LibreHardwareMonitor/LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor). |
| HidSharp 2.6.4.0 | Hardverfigyelő kiterjesztés | HID eszközhozzáférési könyvtár, a LibreHardwareMonitorLib transzitív függősége. Copyright (C) 2010 James F. Bellinger. GNU Lesser General Public License v3.0 alatt licencelve. |

## Felhasználói felület, dokumentáció és Markdown megjelenítés

| Összetevő vagy hozzájárulás | Felhasználás | Szerzői jog és licenc megjegyzések |
| --- | --- | --- |
| cmark-gfm | CommonMark és GitHub-Flavored Markdown elemzés/megjelenítés | GitHub forkja a `commonmark/cmark`-nak, egy CommonMark elemző és megjelenítő könyvtár és program C-ben. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Markdown dokumentumok stílusozása | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Tree View panel módosítások | Tree View panel | A fgodoy által javasolt módosítások alapján. |
| Lokalizációs szkriptek | Lokalizációs eszköz részek | Samandarin tartalmaz lokalizációs eszközöket, amelyek a fork [Sally](https://github.com/0xeb/sally) [Elias Bachaalany (0xeb)](https://github.com/0xeb) adaptált, vagy jelentősen abból inspirált munkájából származnak. A GNU Library General Public License alatt licencelve. |
| Parancsértelmező-sablonok SVG ikonjai | Windows Terminal profilmenü ikonjai | Microsoft termékikonokból és az SVG Repo Azure Cloud Shell ikonjából átdolgozott SVG grafika. |
