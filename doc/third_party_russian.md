# Уведомления о программном обеспечении сторонних разработчиков

[Open Salamander: Samandarin](https://samandarin.krtkovo.eu/) основан на работе множества авторов, кураторов и
проектов. Эта страница содержит список компонентов, кода, идей и инструментов,
используемых приложением и его плагинами. Благодарим всех перечисленных здесь
лиц за предоставление их работы.

## Ядро приложения и общие библиотеки

| Компонент | Используется для | Примечания об авторских правах и лицензии |
| --- | --- | --- |
| REGEXP | Сопоставление регулярных выражений | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| Код AES | Криптографическиеoutines | Написано доктором Brian Gladman. Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | Поддержка изображений PNG | Основано на PNGLite от Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | Поддержка парсинга/рендеринга SVG | Copyright (c) 2013-2014 Mikko Mononen. |
| SQLite | Поддержка встроенной базы данных | SQLite находится в общественном достоянии. |
| LibTomCrypt | Криптографические примитивы плагина контрольных сумм | LibTomCrypt находится в общественном достоянии. Как написал Tom St Denis: «Как и всё качественное программное обеспечение должно быть.» |
| LGPL библиотеки | Общие сторонние библиотеки | Лицензированы под GNU Library General Public License. Копия лицензии прилагается к данному программному обеспечению. |
| Unicode и длинные пути Win32 | Части обработки имён файлов | Samandarin содержит код и реализации, производные от форка [Sally](https://github.com/0xeb/sally) авторства [Elias Bachaalany (0xeb)](https://github.com/0xeb), или существенно вдохновлённые им. Лицензировано под GNU Library General Public License. |
| Lua 5.5.0 | Встроенный интерпретатор для Salamatrix Lua Runtime | [Lua.org, PUC-Rio](https://www.lua.org/), установлен из закреплённого пакета vcpkg `lua[tools]`. Copyright (c) 1994-2025 Lua.org, PUC-Rio. Лицензия MIT; полный текст распространяется как `plugins/extension-runtimes/luaruntime/runtime/LICENSE-LUA.txt`. |
| win32-darkmodelib | Нативная поддержка тёмного режима Windows для общих диалогов и диалогов Salamatrix | Получено из [ozone10/win32-darkmodelib](https://github.com/ozone10/win32-darkmodelib), commit `b58eb027c4a114c1b1c8fd09870ead982f1b1e72`. Mozilla Public License 2.0, отдельные части под лицензией MIT. |
| llama.cpp | Исполняемый файл локального CPU-вывода SalamatrixAI | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp), закреплённый выпуск `b10107`. Лицензия MIT. |
| Qwen2.5-Coder 1.5B Instruct GGUF | Рекомендуемая локальная модель SalamatrixAI | [Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF), закреплённый файл `q4_k_m`. Лицензия Apache 2.0. |
| Qwen2.5-Coder 0.5B Instruct GGUF | Облегчённая локальная модель SalamatrixAI только для английских запросов | [Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF), закреплённый файл `q4_k_m`. Лицензия Apache 2.0. |
| Test Reporter | Отчёты о результатах тестов pull request | [dorny/test-reporter](https://github.com/dorny/test-reporter), используется в GitHub Actions. Лицензия MIT. |
| pytest | Запуск тестов Python и создание отчётов xUnit в CI | [pytest-dev/pytest](https://github.com/pytest-dev/pytest). Лицензия MIT. |

## Плагины архивации, сжатия и дисковых образов

| Компонент | Используется в | Примечания об авторских правах и лицензии |
| --- | --- | --- |
| 7-Zip | Плагин 7-ZIP | Библиотека архивации файлов 7-Zip. Copyright (C) 1999-2026 Igor Pavlov. Распространяется под GNU LGPL, с ограничением unRAR для кода RAR. |
| zlib | Плагин ZIP | Части плагина ZIP используют zlib. Copyright (C) 1995-2002 Jean-loup Gailly и Mark Adler. |
| bzip2 | Плагин TAR | Библиотека bzip2. Copyright (C) 1996-2000 Julian R Seward. |
| Библиотека распаковки ARJ | Плагин UnARJ | Библиотека распаковки, написанная ARJ Software, Inc. Copyright (C) 1990-1997 ARJ Software, Inc. |
| Библиотека распаковки Microsoft CAB | Плагин UnCAB | Библиотека распаковки, написанная Microsoft Corporation. Copyright (C) Microsoft Corporation 1993-1997. |
| UnRAR | Плагин UnRAR | Библиотека распаковки, написанная Alexander Roshal. Copyright (C) 1993-2026 Alexander Roshal. Распространяется под лицензией unRAR. Конвейер vcpkg устанавливает пакет `unrar` для DLL времени выполнения плагина UnRAR. |
| ISZ SDK | Плагин UnISO | Части плагина UnISO используют ISZ SDK. Copyright (C) 2002-2006 EZB Systems, Inc. Все права защищены. |
| CHMLIB | Плагин UnCHM | Библиотека CHMLIB. Copyright (C) 2001-2010 Jed Wing. |

## Плагины просмотрщика, мультимедиа и файловых форматов

| Компонент или автор | Используется в | Примечания об авторских правах и лицензии |
| --- | --- | --- |
| Tomas Jelinek | Плагин Multimedia Viewer | Включает программное обеспечение, написанное Tomas Jelinek. Copyright (C) 2003-2026 Tomas Jelinek. |
| Внутренний просмотрщик | Части просмотрщика Unicode | Samandarin содержит код и реализации, производные от форка [Sally](https://github.com/0xeb/sally) авторства [Elias Bachaalany (0xeb)](https://github.com/0xeb), или существенно вдохновлённые им. Лицензировано под GNU Library General Public License. |
| Jan Patera | Плагин PictView | Части плагина PictView лицензированы от Jan Patera. Copyright (C) 1994-2016 Jan Patera. |
| libexif | Плагин PictView | Части плагина PictView используют libexif. Copyright (C) 2001-2019 Curtis Galloway и Lutz Muller. |
| Newtonsoft.Json 13.0.3 | Плагин JSON Viewer | Автор: James Newton-King, выпущено под лицензией MIT. |
| Microsoft .NET Framework 4.8 | Плагин JSON Viewer | Предоставлено Microsoft Corporation по условиям лицензии Microsoft .NET Framework. |
| PrismSharp 1.0.0-beta | Плагин PrismSharp Text Viewer | Автор: Tomáš Kubec, выпущено под лицензией MIT. Включает Prism.js от Lea Verou и контрибьюторов PrismJS под лицензией MIT. |
| Microsoft.Web.WebView2 1.0.2420.47 | Плагин PrismSharp Text Viewer | Автор: Microsoft Corporation, распространяется под лицензией BSD 3-Clause. |
| Microsoft .NET Framework 4.8 | Плагин PrismSharp Text Viewer | Предоставлено Microsoft Corporation по условиям лицензии Microsoft .NET Framework. |
| Newtonsoft.Json 13.0.3 | Плагин PrismSharp Text Viewer | Автор: James Newton-King, выпущено под лицензией MIT. |
| Markdig 0.36.2 | Плагин WebView2 Render Viewer | Автор: Alexandre Mutel и контрибьюторы, выпущено под лицензией BSD 2-Clause. |
| Microsoft.Web.WebView2 1.0.2420.47 | Плагин WebView2 Render Viewer | Автор: Microsoft Corporation, распространяется под лицензией BSD 3-Clause. |
| Microsoft .NET Framework 4.8 | Плагин WebView2 Render Viewer | Предоставлено Microsoft Corporation по условиям лицензии Microsoft .NET Framework. |

## Плагины сети, синхронизации и устройств

| Компонент или автор | Используется в | Примечания об авторских правах и лицензии |
| --- | --- | --- |
| OpenSSL | Плагин FTP Client и зависимости SFTP | Copyright (c) 1998-2026 The OpenSSL Project Authors. Все права защищены. OpenSSL 1.0.x распространяется под лицензиями OpenSSL и оригинальной SSLeay. OpenSSL 3.x распространяется под Apache License 2.0. Конвейер vcpkg устанавливает OpenSSL для DLL совместимости плагина FTP и для зависимостей плагина SFTP. |
| libssh2 | Плагин SFTP | Автор: Daniel Stenberg, Simon Josefsson и контрибьюторы libssh2. Распространяется под лицензией BSD 3-Clause. Конвейер vcpkg устанавливает пакет `libssh2` для зависимостей плагина SFTP. |
| Martin Prikryl | Плагин WinSCP | Части плагина WinSCP лицензированы от Martin Prikryl. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Плагин Windows Mobile | Включает программное обеспечение, написанное Juraj Rojko. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Плагин Samandarin Update Notifier | Предоставлено Microsoft Corporation по условиям лицензии Microsoft .NET Framework. |

## Пользовательский интерфейс, документация и рендеринг Markdown

| Компонент или вклад | Используется для | Примечания об авторских правах и лицензии |
| --- | --- | --- |
| cmark-gfm | Парсинг/рендеринг CommonMark и GitHub-Flavored Markdown | Форк GitHub репозитория `commonmark/cmark`, библиотека и программа для парсинга и рендеринга CommonMark на C. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Стилизация документов Markdown | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Изменения панели Tree View | Панель Tree View | Основано на изменениях, предложенных fgodoy. |
| Скрипты локализации | Части инструментов локализации | Samandarin содержит инструменты локализации, адаптированные из форка [Sally](https://github.com/0xeb/sally) авторства [Elias Bachaalany (0xeb)](https://github.com/0xeb), или существенно вдохновлённые им. Лицензировано под GNU Library General Public License. |
| SVG-значки шаблонов командной оболочки | Значки меню профилей Windows Terminal | SVG-графика, адаптированная из значков продуктов Microsoft и значка Azure Cloud Shell из SVG Repo. |
