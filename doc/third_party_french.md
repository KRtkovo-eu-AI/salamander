# Avis de logiciels tiers

[Open Salamander: Samandarin](https://samandarin.net/) s'appuie sur le travail de nombreux auteurs,
mainteneurs et projets. Cette page répertorie les composants, le code, les idées
et les outils utilisés par l'application et ses plugins. Merci à tous ceux qui
sont listés ici d'avoir rendu leur travail disponible.

## Application principale et bibliothèques partagées

| Composant | Utilisation | Notes sur l'attribution et la licence |
| --- | --- | --- |
| REGEXP | Correspondance d'expressions régulières | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| Code AES | Routines cryptographiques | Écrit par Dr Brian Gladman. Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | Support d'images PNG | Basé sur PNGLite par Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | Support de parsing/rendering SVG | Copyright (c) 2013-2014 Mikko Mononen. Utilisé pour les icônes de barre d'outils et de menus de l'application principale et par le plugin PictView pour l'affichage SVG (limites NanoSVG : pas de CSS, texte et filtres limités). |
| Inflateur deflate PictView | Plugin PictView | Inflateur deflate/zlib pour les tuiles XCF, IDAT PNG, ZIP SKP et aperçus 3DM compressés. Le lecteur de bits et les tables de Huffman suivent puff de Mark Adler. Copyright (C) 2002-2013 Mark Adler. Licence zlib. |
| openNURBS 3DM preview layout | PictView plugin | Rhinoceros `.3dm` properties-table preview chunks and `ON_WindowsBitmap::ReadCompressed` layout follow [McNeel openNURBS](https://github.com/mcneel/opennurbs). Copyright (c) Robert McNeel & Associates. Used as a format reference only; openNURBS itself is not bundled. |
| SQLite | Support de base de données intégrée | SQLite est dans le domaine public. |
| LibTomCrypt | Primitives cryptographiques du plugin de contrôle d'intégrité | LibTomCrypt est dans le domaine public. Comme l'a écrit Tom St Denis : « Comme tout logiciel de qualité devrait l'être. » |
| Bibliothèques LGPL | Bibliothèques tierces partagées | Sous licence GNU Library General Public License. Une copie de la licence est incluse avec ce logiciel. |
| Unicode et longs chemins Win32 | Parties de gestion des noms de fichiers | Samandarin contient du code et du travail de dérivation du fork [Sally](https://github.com/0xeb/sally) par [Elias Bachaalany (0xeb)](https://github.com/0xeb), ou substantiellement inspiré par celui-ci. Sous licence GNU Library General Public License. |
| Lua 5.5.0 | Interpréteur inclus pour le runtime Lua de Salamatrix | [Lua.org, PUC-Rio](https://www.lua.org/), installé depuis le paquet vcpkg épinglé `lua[tools]`. Copyright (c) 1994-2025 Lua.org, PUC-Rio. Licence MIT ; l'avis complet est distribué sous `plugins/extension-runtimes/luaruntime/runtime/LICENSE-LUA.txt`. |
| win32-darkmodelib | Prise en charge native du mode sombre Windows pour les boîtes de dialogue partagées et Salamatrix | Provenant de [ozone10/win32-darkmodelib](https://github.com/ozone10/win32-darkmodelib), commit `b58eb027c4a114c1b1c8fd09870ead982f1b1e72`. Mozilla Public License 2.0, avec des parties sous licence MIT.; see `src/third_party/darkmodelib/LICENSE.md` and `LICENSE-MIT.md`. |
| llama.cpp | Exécutable d'inférence CPU locale à la demande pour SalamatrixAI | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp), version épinglée `b10107`. Licence MIT. The configuration downloader retrieves the runtime and its license notice directly for the user. |
| Qwen2.5-Coder 1.5B Instruct GGUF | Modèle local SalamatrixAI recommandé | [Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF), fichier `q4_k_m` épinglé. Licence Apache 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Qwen2.5-Coder 0.5B Instruct GGUF | Modèle local SalamatrixAI léger, prompts anglais uniquement | [Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF), fichier `q4_k_m` épinglé. Licence Apache 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Test Reporter | Rapports de résultats de tests des pull requests | [dorny/test-reporter](https://github.com/dorny/test-reporter), utilisé dans GitHub Actions. Licence MIT. It publishes JUnit and Python xUnit results as Checks and workflow summaries. |
| pytest | Exécution des tests Python et génération de rapports xUnit dans la CI | [pytest-dev/pytest](https://github.com/pytest-dev/pytest). Licence MIT. |
| cursor-sdk | Optional Python SDK for batch SLT translation through the Cursor API | Used only by `tools/localization/translate_slt_with_openai.py` when `-Provider cursor` is selected. Install with `pip install cursor-sdk`. |
| OpenRouter API | Optional remote provider for batch SLT translation | Used by `tools/localization/translate_slt_with_openai.py` by default with the `openai/gpt-5.4-nano` model. Requires a user-supplied `OPENROUTER_API_KEY`; no key is stored in the repository. |

## Plugins d'archivage, de compression et d'image disque

| Composant | Utilisé dans | Notes sur l'attribution et la licence |
| --- | --- | --- |
| 7-Zip | Plugin 7-ZIP | Bibliothèque d'archivage de fichiers 7-Zip. Copyright (C) 1999-2026 Igor Pavlov. Distribué sous GNU LGPL, avec la restriction unRAR pour le code RAR. |
| zlib | Plugin ZIP | Des parties du plugin ZIP utilisent zlib. Copyright (C) 1995-2002 Jean-loup Gailly et Mark Adler. |
| bzip2 | Plugin TAR | Bibliothèque bzip2. Copyright (C) 1996-2000 Julian R Seward. |
| Bibliothèque de décompression ARJ | Plugin UnARJ | Bibliothèque de décompression écrite par ARJ Software, Inc. Copyright (C) 1990-1997 ARJ Software, Inc. |
| Bibliothèque de décompression Microsoft CAB | Plugin UnCAB | Bibliothèque de décompression écrite par Microsoft Corporation. Copyright (C) Microsoft Corporation 1993-1997. |
| UnRAR | Plugin UnRAR | Bibliothèque de décompression écrite par Alexander Roshal. Copyright (C) 1993-2026 Alexander Roshal. Distribué sous la licence unRAR. Le pipeline vcpkg installe le paquet `unrar` pour le DLL runtime du plugin UnRAR. |
| ISZ SDK | Plugin UnISO | Des parties du plugin UnISO utilisent ISZ SDK. Copyright (C) 2002-2006 EZB Systems, Inc. Tous droits réservés. |
| CHMLIB | Plugin UnCHM | Bibliothèque CHMLIB. Copyright (C) 2001-2010 Jed Wing. |

## Plugins de visualiseur, multimédia et de formats de fichiers

| Composant ou auteur | Utilisé dans | Notes sur l'attribution et la licence |
| --- | --- | --- |
| Tomas Jelinek | Plugin Multimedia Viewer | Inclut du logiciel écrit par Tomas Jelinek. Copyright (C) 2003-2026 Tomas Jelinek. |
| TagLib 2.3 | Plugin Multimedia Viewer | Bibliothèque de métadonnées audio de Scott Wheeler et des contributeurs de TagLib. Utilisée pour les métadonnées Unicode et les propriétés audio des formats Ogg Vorbis, Opus, FLAC, MP4, APE, WavPack, TrueAudio, AIFF, DSD, Matroska/WebM et apparentés. Distribuée sous GNU LGPL 2.1 ou Mozilla Public License 1.1. Source : https://github.com/taglib/taglib |
| Visualiseur interne | Parties du visualiseur Unicode | Samandarin contient du code et du travail de dérivation du fork [Sally](https://github.com/0xeb/sally) par [Elias Bachaalany (0xeb)](https://github.com/0xeb), ou substantiellement inspiré par celui-ci. Sous licence GNU Library General Public License. |
| Jan Patera | Plugin PictView | Des parties du plugin PictView sont sous licence de Jan Patera. Copyright (C) 1994-2026 Jan Patera. |
| libexif | Plugin PictView | Des parties du plugin PictView utilisent libexif. Copyright (C) 2001-2019 Curtis Galloway et Lutz Muller. |
| Newtonsoft.Json 13.0.3 | Plugin JSON Viewer | Par James Newton-King, publié sous la licence MIT. |
| Microsoft .NET Framework 4.8 | Plugin JSON Viewer | Fourni par Microsoft Corporation sous les termes de la licence Microsoft .NET Framework. |
| Prism.js 1.29.0 | Viewer Frame (Salamatrix et Prism Text Viewer) | Par Lea Verou et les contributeurs PrismJS, publié sous la licence MIT. |
| Microsoft.Web.WebView2 1.0.2420.47 | Viewer Frame | Par Microsoft Corporation, distribué sous la licence BSD 3-Clause. |
| Markdig 0.36.2 | MarkdigRenderer pour WebView2 Render Viewer | Par Alexandre Mutel et les contributeurs, publié sous la licence BSD 2-Clause. |

## Plugins réseau, synchronisation et périphériques

| Composant ou auteur | Utilisé dans | Notes sur l'attribution et la licence |
| --- | --- | --- |
| OpenSSL | Plugin FTP Client et dépendances SFTP | Copyright (c) 1998-2026 The OpenSSL Project Authors. Tous droits réservés. OpenSSL 1.0.x est distribué sous les licences OpenSSL et SSLeay originales. OpenSSL 3.x est distribué sous la Apache License 2.0. Le pipeline vcpkg installe OpenSSL pour les DLL de compatibilité du plugin FTP et pour les dépendances du plugin SFTP. |
| libssh2 | Plugin SFTP | Par Daniel Stenberg, Simon Josefsson et les contributeurs libssh2. Distribué sous la licence BSD 3-Clause. Le pipeline vcpkg installe le paquet `libssh2` pour les dépendances du plugin SFTP. |
| Dupl3xx | Plugin SFTP | Plugin créé et maintenu par Dupl3xx. Dépôt source : [salamander-sftp-plugin](https://github.com/Dupl3xx/salamander-sftp-plugin). |
| Martin Prikryl | Plugin WinSCP | Des parties du plugin WinSCP sont sous licence de Martin Prikryl. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Plugin Windows Mobile | Inclut du logiciel écrit par Juraj Rojko. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Plugin Samandarin Update Notifier | Fourni par Microsoft Corporation sous les termes de la licence Microsoft .NET Framework. |

## Extension Moniteur matériel

| Composant | Utilisé dans | Notes sur l'attribution et la licence |
| --- | --- | --- |
| HardView | Extension Moniteur matériel | Bibliothèque pont C++/CLI pour la surveillance du matériel. Copyright (c) 2025 gafoo. Publié sous MIT License. Source : [gafoo173/HardView](https://github.com/gafoo173/HardView). |
| LibreHardwareMonitorLib 0.9.6.0 | Extension Moniteur matériel | Bibliothèque .NET de surveillance du matériel. LibreHardwareMonitorLib est publié sous GNU General Public License v3.0. Source : [LibreHardwareMonitor/LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor). |
| HidSharp 2.6.4.0 | Extension Moniteur matériel | Bibliothèque d'accès aux périphériques HID, dépendance transitive de LibreHardwareMonitorLib. Copyright (C) 2010 James F. Bellinger. Publié sous GNU Lesser General Public License v3.0. |

## Interface utilisateur, documentation et rendu Markdown

| Composant ou contribution | Utilisé pour | Notes sur l'attribution et la licence |
| --- | --- | --- |
| cmark-gfm | Parsing/rendering CommonMark et GitHub-Flavored Markdown | Fork de GitHub de `commonmark/cmark`, une bibliothèque et un programme de parsing et rendu CommonMark en C. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Style des documents Markdown | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| La base du panneau Tree View | Panneau Tree View | Basé sur les modifications proposées par fgodoy. |
| Scripts de localisation | Parties d'outils de localisation | Samandarin contient des outils de localisation adaptés du fork [Sally](https://github.com/0xeb/sally) par [Elias Bachaalany (0xeb)](https://github.com/0xeb), ou substantiellement inspirés par celui-ci. Sous licence GNU Library General Public License. |
| Icônes SVG des modèles d'interpréteur de commandes | Icônes du menu de profils de Windows Terminal | Illustrations SVG adaptées des icônes de produits Microsoft et de l'icône Azure Cloud Shell de SVG Repo. |
