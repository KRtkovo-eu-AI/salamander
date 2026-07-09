# Avisos de software de terceros

[Open Salamander: Samandarin](https://github.com/KRtkovo-eu-AI/salamander) se basa en el trabajo de muchos autores,
mantenedores y proyectos. Esta página enumera los componentes, código, ideas
y herramientas utilizados por la aplicación y sus complementos. Gracias a todos
los aquí mencionados por hacer disponible su trabajo.

## Aplicación principal y bibliotecas compartidas

| Componente | Uso | Notas de atribución y licencia |
| --- | --- | --- |
| REGEXP | Coincidencia de expresiones regulares | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| Código AES | Rutinas criptográficas | Escrito por Dr Brian Gladman. Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | Soporte de imágenes PNG | Basado en PNGLite de Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | Soporte de parseo/renderizado SVG | Copyright (c) 2013-2014 Mikko Mononen. |
| SQLite | Soporte de base de datos embebida | SQLite está en el dominio público. |
| LibTomCrypt | Primitivas criptográficas del complemento de suma de verificación | LibTomCrypt está en el dominio público. Como escribió Tom St Denis: "Como debería ser todo software de calidad." |
| Bibliotecas LGPL | Bibliotecas de terceros compartidas | Licenciadas bajo la GNU Library General Public License. Se incluye una copia de la licencia con este software. |
| Unicode y rutas largas Win32 | Partes de manejo de nombres de archivo | Samandarin incluye código y trabajo de implementación derivado del fork [Sally](https://github.com/0xeb/sally) de [Elias Bachaalany (0xeb)](https://github.com/0xeb), o sustancialmente inspirado por él. Licenciado bajo la GNU Library General Public License. |

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
| PrismSharp 1.0.0-beta | Complemento PrismSharp Text Viewer | Por Tomáš Kubec, publicado bajo la licencia MIT. Incluye Prism.js de Lea Verou y los colaboradores de PrismJS bajo la licencia MIT. |
| Microsoft.Web.WebView2 1.0.2420.47 | Complemento PrismSharp Text Viewer | Por Microsoft Corporation, distribuido bajo la licencia BSD 3-Clause. |
| Microsoft .NET Framework 4.8 | Complemento PrismSharp Text Viewer | Proporcionado por Microsoft Corporation bajo los términos de licencia de Microsoft .NET Framework. |
| Newtonsoft.Json 13.0.3 | Complemento PrismSharp Text Viewer | Por James Newton-King, publicado bajo la licencia MIT. |
| Markdig 0.36.2 | Complemento WebView2 Render Viewer | Por Alexandre Mutel y colaboradores, publicado bajo la licencia BSD 2-Clause. |
| Microsoft.Web.WebView2 1.0.2420.47 | Complemento WebView2 Render Viewer | Por Microsoft Corporation, distribuido bajo la licencia BSD 3-Clause. |
| Microsoft .NET Framework 4.8 | Complemento WebView2 Render Viewer | Proporcionado por Microsoft Corporation bajo los términos de licencia de Microsoft .NET Framework. |

## Complementos de red, sincronización y dispositivos

| Componente o autor | Uso en | Notas de atribución y licencia |
| --- | --- | --- |
| OpenSSL | Complemento FTP Client y dependencias SFTP | Copyright (c) 1998-2026 The OpenSSL Project Authors. Todos los derechos reservados. OpenSSL 1.0.x se distribuye bajo las licencias OpenSSL y SSLeay originales. OpenSSL 3.x se distribuye bajo la Apache License 2.0. El pipeline vcpkg instala OpenSSL para los DLL de compatibilidad del complemento FTP y para las dependencias del complemento SFTP. |
| libssh2 | Complemento SFTP | Por Daniel Stenberg, Simon Josefsson y colaboradores de libssh2. Distribuido bajo la licencia BSD 3-Clause. El pipeline vcpkg instala el paquete `libssh2` para las dependencias del complemento SFTP. |
| Martin Prikryl | Complemento WinSCP | Partes del complemento WinSCP están licenciadas de Martin Prikryl. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Complemento Windows Mobile | Incluye software escrito por Juraj Rojko. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Complemento Samandarin Update Notifier | Proporcionado por Microsoft Corporation bajo los términos de licencia de Microsoft .NET Framework. |

## Interfaz de usuario, documentación y renderizado de Markdown

| Componente o contribución | Uso para | Notas de atribución y licencia |
| --- | --- | --- |
| cmark-gfm | Parseo/renderizado de CommonMark y GitHub-Flavored Markdown | Fork de GitHub de `commonmark/cmark`, una biblioteca y programa de parseo y renderizado de CommonMark en C. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Estilizado de documentos Markdown | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Cambios del panel Tree View | Panel Tree View | Basado en cambios propuestos por fgodoy. |
| Scripts de localización | Partes de herramientas de localización | Samandarin incluye herramientas de localización adaptadas del fork [Sally](https://github.com/0xeb/sally) de [Elias Bachaalany (0xeb)](https://github.com/0xeb), o sustancialmente inspiradas por él. Licenciado bajo la GNU Library General Public License. |
