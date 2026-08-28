# Avisos de software de terceros

[Open Salamander: Samandarin](https://samandarin.net/) se basa en el trabajo de muchos autores,
mantenedores y proyectos. Esta página enumera los componentes, código, ideas
y herramientas utilizados por la aplicación y sus complementos. Gracias a todos
los aquí mencionados por hacer disponible su trabajo.

## Aplicación principal y bibliotecas compartidas

| Componente | Uso | Notas de atribución y licencia |
| --- | --- | --- |
| REGEXP | Coincidencia de expresiones regulares | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| Código AES | Rutinas criptográficas | Escrito por Dr Brian Gladman. Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | Soporte de imágenes PNG | Basado en PNGLite de Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | Soporte de parseo/renderizado SVG | Copyright (c) 2013-2014 Mikko Mononen. Se usa para los iconos de la barra de herramientas y menús de la aplicación principal y por el complemento PictView para ver SVG (límites de NanoSVG: sin CSS, texto y filtros limitados). |
| Inflador deflate de PictView | Complemento PictView | Inflador deflate/zlib para teselas XCF, IDAT PNG, ZIP SKP y vistas previas 3DM comprimidas. El lector de bits y Huffman siguen puff de Mark Adler. Copyright (C) 2002-2013 Mark Adler. Licencia zlib. |
| openNURBS 3DM preview layout | PictView plugin | Rhinoceros `.3dm` properties-table preview chunks and `ON_WindowsBitmap::ReadCompressed` layout follow [McNeel openNURBS](https://github.com/mcneel/opennurbs). Copyright (c) Robert McNeel & Associates. Used as a format reference only; openNURBS itself is not bundled. |
| SQLite | Soporte de base de datos embebida | SQLite está en el dominio público. |
| LibTomCrypt | Primitivas criptográficas del complemento de suma de verificación | LibTomCrypt está en el dominio público. Como escribió Tom St Denis: "Como debería ser todo software de calidad." |
| Bibliotecas LGPL | Bibliotecas de terceros compartidas | Licenciadas bajo la GNU Library General Public License. Se incluye una copia de la licencia con este software. |
| Unicode y rutas largas Win32 | Partes de manejo de nombres de archivo | Samandarin incluye código y trabajo de implementación derivado del fork [Sally](https://github.com/0xeb/sally) de [Elias Bachaalany (0xeb)](https://github.com/0xeb), o sustancialmente inspirado por él. Licenciado bajo la GNU Library General Public License. |
| Lua 5.5.0 | Intérprete incluido para Salamatrix Lua Runtime | [Lua.org, PUC-Rio](https://www.lua.org/), instalado desde el paquete vcpkg fijado `lua[tools]`. Copyright (c) 1994-2025 Lua.org, PUC-Rio. Licencia MIT; el aviso completo se distribuye como `plugins/extension-runtimes/luaruntime/runtime/LICENSE-LUA.txt`. |
| win32-darkmodelib | Compatibilidad nativa con el modo oscuro de Windows para diálogos compartidos y Salamatrix | Procedente de [ozone10/win32-darkmodelib](https://github.com/ozone10/win32-darkmodelib), commit `b58eb027c4a114c1b1c8fd09870ead982f1b1e72`. Mozilla Public License 2.0, con partes bajo licencia MIT.; see `src/third_party/darkmodelib/LICENSE.md` and `LICENSE-MIT.md`. |
| llama.cpp | Ejecutable de inferencia local por CPU para SalamatrixAI | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp), versión fijada `b10107`. Licencia MIT. The configuration downloader retrieves the runtime and its license notice directly for the user. |
| Qwen2.5-Coder 1.5B Instruct GGUF | Modelo local recomendado para SalamatrixAI | [Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF), archivo `q4_k_m` fijado. Licencia Apache 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Qwen2.5-Coder 0.5B Instruct GGUF | Modelo local ligero para SalamatrixAI, solo para prompts en inglés | [Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF), archivo `q4_k_m` fijado. Licencia Apache 2.0. The configuration downloader retrieves the model and license notice directly for the user. |
| Test Reporter | Informes de resultados de pruebas de pull requests | [dorny/test-reporter](https://github.com/dorny/test-reporter), utilizado en GitHub Actions. Licencia MIT. It publishes JUnit and Python xUnit results as Checks and workflow summaries. |
| pytest | Ejecución de pruebas Python y generación de informes xUnit en CI | [pytest-dev/pytest](https://github.com/pytest-dev/pytest). Licencia MIT. |
| cursor-sdk | Optional Python SDK for batch SLT translation through the Cursor API | Used only by `tools/localization/translate_slt_with_openai.py` when `-Provider cursor` is selected. Install with `pip install cursor-sdk`. |
| OpenRouter API | Optional remote provider for batch SLT translation | Used by `tools/localization/translate_slt_with_openai.py` by default with the `openai/gpt-5.4-nano` model. Requires a user-supplied `OPENROUTER_API_KEY`; no key is stored in the repository. |

## Complementos de archivado, compresión y imagen de disco

| Componente | Uso en | Notas de atribución y licencia |
| --- | --- | --- |
| 7-Zip | Complemento 7-ZIP | Biblioteca de archivado de archivos 7-Zip. Copyright (C) 1999-2026 Igor Pavlov. Distribuido bajo GNU LGPL, con la restricción unRAR para el código RAR. |
| zlib | Complemento ZIP | Partes del complemento ZIP usan zlib. Copyright (C) 1995-2002 Jean-loup Gailly y Mark Adler. |
| bzip2 | Complemento TAR | Biblioteca bzip2. Copyright (C) 1996-2000 Julian R Seward. |
| Biblioteca de descompresión ARJ | Complemento UnARJ | Biblioteca de descompresión escrita por ARJ Software, Inc. Copyright (C) 1990-1997 ARJ Software, Inc. |
| Biblioteca de descompresión Microsoft CAB | Complemento UnCAB | Biblioteca de descompresión escrita por Microsoft Corporation. Copyright (C) Microsoft Corporation 1993-1997. |
| UnRAR | Complemento UnRAR | Biblioteca de descompresión escrita por Alexander Roshal. Copyright (C) 1993-2026 Alexander Roshal. Distribuido bajo la licencia unRAR. El pipeline vcpkg instala el paquete `unrar` para el DLL de tiempo de ejecución del complemento UnRAR. |
| ISZ SDK | Complemento UnISO | Partes del complemento UnISO usan ISZ SDK. Copyright (C) 2002-2006 EZB Systems, Inc. Todos los derechos reservados. |
| CHMLIB | Complemento UnCHM | Biblioteca CHMLIB. Copyright (C) 2001-2010 Jed Wing. |

## Complementos de visor, medios y formatos de archivo

| Componente o autor | Uso en | Notas de atribución y licencia |
| --- | --- | --- |
| Tomas Jelinek | Complemento Multimedia Viewer | Incluye software escrito por Tomas Jelinek. Copyright (C) 2003-2026 Tomas Jelinek. |
| Visor interno | Partes del visor Unicode | Samandarin incluye código y trabajo de implementación derivado del fork [Sally](https://github.com/0xeb/sally) de [Elias Bachaalany (0xeb)](https://github.com/0xeb), o sustancialmente inspirado por él. Licenciado bajo la GNU Library General Public License. |
| Jan Patera | Complemento PictView | Partes del complemento PictView están licenciadas de Jan Patera. Copyright (C) 1994-2026 Jan Patera. |
| libexif | Complemento PictView | Partes del complemento PictView usan libexif. Copyright (C) 2001-2019 Curtis Galloway y Lutz Muller. |
| Newtonsoft.Json 13.0.3 | Complemento JSON Viewer | Por James Newton-King, publicado bajo la licencia MIT. |
| Microsoft .NET Framework 4.8 | Complemento JSON Viewer | Proporcionado por Microsoft Corporation bajo los términos de licencia de Microsoft .NET Framework. |
| Prism.js 1.29.0 | Viewer Frame (Salamatrix y Prism Text Viewer) | Por Lea Verou y los colaboradores de PrismJS, publicado bajo la licencia MIT. |
| Microsoft.Web.WebView2 1.0.2420.47 | Viewer Frame | Por Microsoft Corporation, distribuido bajo la licencia BSD 3-Clause. |
| Markdig 0.36.2 | MarkdigRenderer para WebView2 Render Viewer | Por Alexandre Mutel y colaboradores, publicado bajo la licencia BSD 2-Clause. |

## Complementos de red, sincronización y dispositivos

| Componente o autor | Uso en | Notas de atribución y licencia |
| --- | --- | --- |
| OpenSSL | Complemento FTP Client y dependencias SFTP | Copyright (c) 1998-2026 The OpenSSL Project Authors. Todos los derechos reservados. OpenSSL 1.0.x se distribuye bajo las licencias OpenSSL y SSLeay originales. OpenSSL 3.x se distribuye bajo la Apache License 2.0. El pipeline vcpkg instala OpenSSL para los DLL de compatibilidad del complemento FTP y para las dependencias del complemento SFTP. |
| libssh2 | Complemento SFTP | Por Daniel Stenberg, Simon Josefsson y colaboradores de libssh2. Distribuido bajo la licencia BSD 3-Clause. El pipeline vcpkg instala el paquete `libssh2` para las dependencias del complemento SFTP. |
| Dupl3xx | Complemento SFTP | Complemento creado y mantenido por Dupl3xx. Repositorio del código fuente: [salamander-sftp-plugin](https://github.com/Dupl3xx/salamander-sftp-plugin). |
| Martin Prikryl | Complemento WinSCP | Partes del complemento WinSCP están licenciadas de Martin Prikryl. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Complemento Windows Mobile | Incluye software escrito por Juraj Rojko. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Complemento Samandarin Update Notifier | Proporcionado por Microsoft Corporation bajo los términos de licencia de Microsoft .NET Framework. |

## Extensión Monitor de Hardware

| Componente | Uso en | Notas de atribución y licencia |
| --- | --- | --- |
| HardView | Extensión Monitor de Hardware | Biblioteca puente C++/CLI para monitoreo de hardware. Copyright (c) 2025 gafoo. Licenciado bajo MIT License. Fuente: [gafoo173/HardView](https://github.com/gafoo173/HardView). |
| LibreHardwareMonitorLib 0.9.6.0 | Extensión Monitor de Hardware | Biblioteca .NET de monitoreo de hardware. LibreHardwareMonitorLib está licenciada bajo GNU General Public License v3.0. Fuente: [LibreHardwareMonitor/LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor). |
| HidSharp 2.6.4.0 | Extensión Monitor de Hardware | Biblioteca de acceso a dispositivos HID, dependencia transitiva de LibreHardwareMonitorLib. Copyright (C) 2010 James F. Bellinger. Licenciado bajo GNU Lesser General Public License v3.0. |

## Interfaz de usuario, documentación y renderizado de Markdown

| Componente o contribución | Uso para | Notas de atribución y licencia |
| --- | --- | --- |
| cmark-gfm | Parseo/renderizado de CommonMark y GitHub-Flavored Markdown | Fork de GitHub de `commonmark/cmark`, una biblioteca y programa de parseo y renderizado de CommonMark en C. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Estilizado de documentos Markdown | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Panel Tree View | Panel Tree View | Basado en cambios propuestos por fgodoy. |
| Scripts de localización | Partes de herramientas de localización | Samandarin incluye herramientas de localización adaptadas del fork [Sally](https://github.com/0xeb/sally) de [Elias Bachaalany (0xeb)](https://github.com/0xeb), o sustancialmente inspiradas por él. Licenciado bajo la GNU Library General Public License. |
| Iconos SVG de plantillas de shell de comandos | Iconos del menú de perfiles de Windows Terminal | Gráficos SVG adaptados de iconos de productos Microsoft y del icono Azure Cloud Shell de SVG Repo. |
