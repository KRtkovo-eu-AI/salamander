# Уведомления о программном обеспечении сторонних разработчиков

[Open Salamander: Samandarin](https://samandarin.net/) основан на работе множества авторов, кураторов и
проектов. Эта страница содержит список компонентов, кода, идей и инструментов,
используемых приложением и его плагинами. Благодарим всех перечисленных здесь
лиц за предоставление их работы.

## Ядро приложения и общие библиотеки

| Компонент | Используется для | Примечания об авторских правах и лицензии |
| --- | --- | --- |
| REGEXP | Сопоставление регулярных выражений | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| Код AES | Криптографическиеoutines | Написано доктором Brian Gladman. Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | Поддержка изображений PNG | Основано на PNGLite от Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | Поддержка парсинга/рендеринга SVG | Copyright (c) 2013-2014 Mikko Mononen. Используется для значков панели инструментов и меню основного приложения и плагином PictView для просмотра SVG (ограничения NanoSVG: без CSS, ограниченный текст и фильтры). |
| PictView deflate inflater | Плагин PictView | Inflater deflate/zlib для тайлов XCF, PNG IDAT, ZIP SKP и сжатых превью 3DM. Чтение битов и таблицы Хаффмана по puff Марка Адлера. Copyright (C) 2002-2013 Mark Adler. Лицензия zlib. |
| openNURBS 3DM preview layout | PictView plugin | Rhinoceros `.3dm` properties-table preview chunks and `ON_WindowsBitmap::ReadCompressed` layout follow [McNeel openNURBS](https://github.com/mcneel/opennurbs). Copyright (c) Robert McNeel & Associates. Used as a format reference only; openNURBS itself is not bundled. |
| SQLite | Поддержка встроенной базы данных | SQLite находится в общественном достоянии. |
| LibTomCrypt | Криптографические примитивы плагина контрольных сумм | LibTomCrypt находится в общественном достоянии. Как написал Tom St Denis: «Как и всё качественное программное обеспечение должно быть.» |
| LGPL библиотеки | Общие сторонние библиотеки | Лицензированы под GNU Library General Public License. Копия лицензии прилагается к данному программному обеспечению. |
| Unicode и длинные пути Win32 | Части обработки имён файлов | Samandarin содержит код и реализации, производные от форка [Sally](https://github.com/0xeb/sally) авторства [Elias Bachaalany (0xeb)](https://github.com/0xeb), или существенно вдохновлённые им. Лицензировано под GNU Library General Public License. |
| Lua 5.5.0 | Встроенный интерпретатор для Salamatrix Lua Runtime | [Lua.org, PUC-Rio](https://www.lua.org/), установлен из закреплённого пакета vcpkg `lua[tools]`. Copyright (c) 1994-2025 Lua.org, PUC-Rio. Лицензия MIT; полный текст распространяется как `plugins/extension-runtimes/luaruntime/runtime/LICENSE-LUA.txt`. |
| win32-darkmodelib | Нативная поддержка тёмного режима Windows для общих диалогов и диалогов Salamatrix | Получено из [ozone10/win32-darkmodelib](https://github.com/ozone10/win32-darkmodelib), commit `b58eb027c4a114c1b1c8fd09870ead982f1b1e72`. Mozilla Public License 2.0, отдельные части под лицензией MIT.; see `src/third_party/darkmodelib/LICENSE.md` and `LICENSE-MIT.md`. |
| llama.cpp | Исполняемый файл локального CPU-вывода SalamatrixAI | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp), закреплённый выпуск `b10107`. Лицензия MIT. The configuration downloader retrieves the runtime and its license notice directly for the user. |
| Qwen2.5-Coder 1.5B Instruct GGUF | Рекомендуемая локальная модель SalamatrixAI | [Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF), закреплённый файл `q4_k_m`. Лицензия Apache 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Qwen2.5-Coder 0.5B Instruct GGUF | Облегчённая локальная модель SalamatrixAI только для английских запросов | [Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF), закреплённый файл `q4_k_m`. Лицензия Apache 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Test Reporter | Отчёты о результатах тестов pull request | [dorny/test-reporter](https://github.com/dorny/test-reporter), используется в GitHub Actions. Лицензия MIT. It publishes JUnit and Python xUnit results as Checks and workflow summaries. |
| pytest | Запуск тестов Python и создание отчётов xUnit в CI | [pytest-dev/pytest](https://github.com/pytest-dev/pytest). Лицензия MIT. |
| cursor-sdk | Optional Python SDK for batch SLT translation through the Cursor API | Used only by `tools/localization/translate_slt_with_openai.py` when `-Provider cursor` is selected. Install with `pip install cursor-sdk`. |
| OpenRouter API | Optional remote provider for batch SLT translation | Used by `tools/localization/translate_slt_with_openai.py` by default with the `openai/gpt-5.4-nano` model. Requires a user-supplied `OPENROUTER_API_KEY`; no key is stored in the repository. |

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
| Jan Patera | Плагин PictView | Части плагина PictView лицензированы от Jan Patera. Copyright (C) 1994-2026 Jan Patera. |
| libexif | Плагин PictView | Части плагина PictView используют libexif. Copyright (C) 2001-2019 Curtis Galloway и Lutz Muller. |
| Newtonsoft.Json 13.0.3 | Плагин JSON Viewer | Автор: James Newton-King, выпущено под лицензией MIT. |
| Microsoft .NET Framework 4.8 | Плагин JSON Viewer | Предоставлено Microsoft Corporation по условиям лицензии Microsoft .NET Framework. |
| Prism.js 1.29.0 | Viewer Frame (Salamatrix и Prism Text Viewer) | Авторы: Lea Verou и контрибьюторы PrismJS, выпущено под лицензией MIT. |
| Microsoft.Web.WebView2 1.0.2420.47 | Viewer Frame | Автор: Microsoft Corporation, распространяется под лицензией BSD 3-Clause. |
| Markdig 0.36.2 | MarkdigRenderer для WebView2 Render Viewer | Авторы: Alexandre Mutel и контрибьюторы, выпущено под лицензией BSD 2-Clause. |

## Плагины сети, синхронизации и устройств

| Компонент или автор | Используется в | Примечания об авторских правах и лицензии |
| --- | --- | --- |
| OpenSSL | Плагин FTP Client и зависимости SFTP | Copyright (c) 1998-2026 The OpenSSL Project Authors. Все права защищены. OpenSSL 1.0.x распространяется под лицензиями OpenSSL и оригинальной SSLeay. OpenSSL 3.x распространяется под Apache License 2.0. Конвейер vcpkg устанавливает OpenSSL для DLL совместимости плагина FTP и для зависимостей плагина SFTP. |
| libssh2 | Плагин SFTP | Автор: Daniel Stenberg, Simon Josefsson и контрибьюторы libssh2. Распространяется под лицензией BSD 3-Clause. Конвейер vcpkg устанавливает пакет `libssh2` для зависимостей плагина SFTP. |
| Dupl3xx | Плагин SFTP | Плагин создан и поддерживается Dupl3xx. Исходный репозиторий: [salamander-sftp-plugin](https://github.com/Dupl3xx/salamander-sftp-plugin). |
| Martin Prikryl | Плагин WinSCP | Части плагина WinSCP лицензированы от Martin Prikryl. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Плагин Windows Mobile | Включает программное обеспечение, написанное Juraj Rojko. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Плагин Samandarin Update Notifier | Предоставлено Microsoft Corporation по условиям лицензии Microsoft .NET Framework. |

## Расширение Аппаратный монитор

| Компонент | Используется в | Примечания об авторских правах и лицензии |
| --- | --- | --- |
| HardView | Расширение Аппаратный монитор | Библиотека-мост C++/CLI для мониторинга оборудования. Copyright (c) 2025 gafoo. Лицензировано под MIT License. Источник: [gafoo173/HardView](https://github.com/gafoo173/HardView). |
| LibreHardwareMonitorLib 0.9.6.0 | Расширение Аппаратный монитор | .NET библиотека мониторинга оборудования. LibreHardwareMonitorLib лицензирована под GNU General Public License v3.0. Источник: [LibreHardwareMonitor/LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor). |
| HidSharp 2.6.4.0 | Расширение Аппаратный монитор | Библиотека доступа к HID-устройствам, транзитивная зависимость LibreHardwareMonitorLib. Copyright (C) 2010 James F. Bellinger. Лицензировано под GNU Lesser General Public License v3.0. |

## Пользовательский интерфейс, документация и рендеринг Markdown

| Компонент или вклад | Используется для | Примечания об авторских правах и лицензии |
| --- | --- | --- |
| cmark-gfm | Парсинг/рендеринг CommonMark и GitHub-Flavored Markdown | Форк GitHub репозитория `commonmark/cmark`, библиотека и программа для парсинга и рендеринга CommonMark на C. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Стилизация документов Markdown | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Изменения панели Tree View | Панель Tree View | Основано на изменениях, предложенных fgodoy. |
| Скрипты локализации | Части инструментов локализации | Samandarin содержит инструменты локализации, адаптированные из форка [Sally](https://github.com/0xeb/sally) авторства [Elias Bachaalany (0xeb)](https://github.com/0xeb), или существенно вдохновлённые им. Лицензировано под GNU Library General Public License. |
| SVG-значки шаблонов командной оболочки | Значки меню профилей Windows Terminal | SVG-графика, адаптированная из значков продуктов Microsoft и значка Azure Cloud Shell из SVG Repo. |
