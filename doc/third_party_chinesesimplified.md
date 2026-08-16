# 第三方软件声明

[Open Salamander: Samandarin](https://samandarin.net/) 基于众多作者、维护者和项目的工作成果构建。本页记录了应用程序及其插件所使用的组件、代码、 ideas 和工具。感谢所有在此列出的人员，使他们的工作得以共享。

## 核心应用程序和共享库

| 组件 | 用途 | 归属和许可证说明 |
| --- | --- | --- |
| REGEXP | 正则表达式匹配 | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| AES 代码 | 加密例程 | 由 Dr Brian Gladman 编写。Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | PNG 图像支持 | 基于 Daniel Karling 的 PNGLite。Copyright (C) 2007 Daniel Karling. |
| Nano SVG | SVG 解析/渲染支持 | Copyright (c) 2013-2014 Mikko Mononen. |
| SQLite | 嵌入式数据库支持 | SQLite 处于公共领域。 |
| LibTomCrypt | 校验和插件加密原语 | LibTomCrypt 处于公共领域。正如 Tom St Denis 所写："就像所有优质软件应该的那样。" |
| LGPL 库 | 共享第三方库 | 根据 GNU Library General Public License 授权。本软件附带许可证副本。 |
| Unicode 和 Win32 长路径 | 文件名处理部分 | Samandarin 包含源自 [Sally](https://github.com/0xeb/sally) 的 fork [Elias Bachaalany (0xeb)](https://github.com/0xeb) 的代码和实现工作，或大量受其启发。根据 GNU Library General Public License 授权。 |
| Lua 5.5.0 | Salamatrix Lua Runtime 随附的解释器 | [Lua.org, PUC-Rio](https://www.lua.org/)，由固定的 vcpkg `lua[tools]` 包安装。Copyright (c) 1994-2025 Lua.org, PUC-Rio。MIT 许可证；完整声明以 `plugins/extension-runtimes/luaruntime/runtime/LICENSE-LUA.txt` 分发。 |
| win32-darkmodelib | 共享对话框和 Salamatrix 对话框的原生 Windows 深色模式支持 | 来自 [ozone10/win32-darkmodelib](https://github.com/ozone10/win32-darkmodelib)，commit `b58eb027c4a114c1b1c8fd09870ead982f1b1e72`。Mozilla Public License 2.0，部分代码采用 MIT 许可证。 |
| llama.cpp | SalamatrixAI 按需本地 CPU 推理可执行文件 | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp)，固定版本 `b10107`。MIT 许可证。 |
| Qwen2.5-Coder 1.5B Instruct GGUF | 推荐的 SalamatrixAI 本地模型 | [Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF)，固定的 `q4_k_m` 文件。Apache License 2.0。 |
| Qwen2.5-Coder 0.5B Instruct GGUF | 仅支持英文提示的轻量 SalamatrixAI 本地模型 | [Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF)，固定的 `q4_k_m` 文件。Apache License 2.0。 |
| Test Reporter | 拉取请求测试结果报告 | [dorny/test-reporter](https://github.com/dorny/test-reporter)，用于 GitHub Actions。MIT 许可证。 |
| pytest | CI 中的 Python 测试执行和 xUnit 报告生成 | [pytest-dev/pytest](https://github.com/pytest-dev/pytest)。MIT 许可证。 |

## 归档、压缩和磁盘映像插件

| 组件 | 使用于 | 归属和许可证说明 |
| --- | --- | --- |
| 7-Zip | 7-ZIP 插件 | 7-Zip 文件归档库。Copyright (C) 1999-2026 Igor Pavlov. 在 GNU LGPL 下分发，带有针对 RAR 代码的 unRAR 限制。 |
| zlib | ZIP 插件 | ZIP 插件的部分内容使用 zlib。Copyright (C) 1995-2002 Jean-loup Gailly 和 Mark Adler。 |
| bzip2 | TAR 插件 | bzip2 库。Copyright (C) 1996-2000 Julian R Seward。 |
| ARJ 解压库 | UnARJ 插件 | 由 ARJ Software, Inc. 编写的解压库。Copyright (C) 1990-1997 ARJ Software, Inc. |
| Microsoft CAB 解压库 | UnCAB 插件 | 由 Microsoft Corporation 编写的解压库。Copyright (C) Microsoft Corporation 1993-1997. |
| UnRAR | UnRAR 插件 | 由 Alexander Roshal 编写的解压库。Copyright (C) 1993-2026 Alexander Roshal. 在 unRAR 许可证下分发。vcpkg 流水线为 UnRAR 插件运行时 DLL 安装 `unrar` 包。 |
| ISZ SDK | UnISO 插件 | UnISO 插件的部分内容使用 ISZ SDK。Copyright (C) 2002-2006 EZB Systems, Inc. 保留所有权利。 |
| CHMLIB | UnCHM 插件 | CHMLIB 库。Copyright (C) 2001-2010 Jed Wing. |

## 查看器、媒体和文件格式插件

| 组件或作者 | 使用于 | 归属和许可证说明 |
| --- | --- | --- |
| Tomas Jelinek | Multimedia Viewer 插件 | 包含由 Tomas Jelinek 编写的软件。Copyright (C) 2003-2026 Tomas Jelinek. |
| 内置查看器 | Unicode 查看器部分 | Samandarin 包含源自 [Sally](https://github.com/0xeb/sally) 的 fork [Elias Bachaalany (0xeb)](https://github.com/0xeb) 的代码和实现工作，或大量受其启发。根据 GNU Library General Public License 授权。 |
| Jan Patera | PictView 插件 | PictView 插件的部分内容由 Jan Patera 授权。Copyright (C) 1994-2026 Jan Patera. |
| libexif | PictView 插件 | PictView 插件的部分内容使用 libexif。Copyright (C) 2001-2019 Curtis Galloway 和 Lutz Muller. |
| Newtonsoft.Json 13.0.3 | JSON Viewer 插件 | 由 James Newton-King 提供，在 MIT 许可证下发布。 |
| Microsoft .NET Framework 4.8 | JSON Viewer 插件 | 由 Microsoft Corporation 根据 Microsoft .NET Framework 许可条款提供。 |
| PrismSharp 1.0.0-beta | PrismSharp Text Viewer 插件 | 由 Tomáš Kubec 提供，在 MIT 许可证下发布。包含 Lea Verou 和 PrismJS 贡献者的 Prism.js，在 MIT 许可证下。 |
| Microsoft.Web.WebView2 1.0.2420.47 | PrismSharp Text Viewer 插件 | 由 Microsoft Corporation 提供，在 BSD 3-Clause 许可证下分发。 |
| Microsoft .NET Framework 4.8 | PrismSharp Text Viewer 插件 | 由 Microsoft Corporation 根据 Microsoft .NET Framework 许可条款提供。 |
| Newtonsoft.Json 13.0.3 | PrismSharp Text Viewer 插件 | 由 James Newton-King 提供，在 MIT 许可证下发布。 |
| Markdig 0.36.2 | WebView2 Render Viewer 插件 | 由 Alexandre Mutel 和贡献者提供，在 BSD 2-Clause 许可证下发布。 |
| Microsoft.Web.WebView2 1.0.2420.47 | WebView2 Render Viewer 插件 | 由 Microsoft Corporation 提供，在 BSD 3-Clause 许可证下分发。 |
| Microsoft .NET Framework 4.8 | WebView2 Render Viewer 插件 | 由 Microsoft Corporation 根据 Microsoft .NET Framework 许可条款提供。 |

## 网络、同步和设备插件

| 组件或作者 | 使用于 | 归属和许可证说明 |
| --- | --- | --- |
| OpenSSL | FTP Client 插件和 SFTP 依赖项 | Copyright (c) 1998-2026 The OpenSSL Project Authors. 保留所有权利。OpenSSL 1.0.x 在 OpenSSL 和原始 SSLeay 许可证下分发。OpenSSL 3.x 在 Apache License 2.0 下分发。vcpkg 流水线为 FTP 插件兼容性 DLL 和 SFTP 插件依赖项安装 OpenSSL。 |
| libssh2 | SFTP 插件 | 由 Daniel Stenberg、Simon Josefsson 和 libssh2 贡献者提供。在 BSD 3-Clause 许可证下分发。vcpkg 流水线为 SFTP 插件依赖项安装 `libssh2` 包。 |
| Dupl3xx | SFTP 插件 | 插件由 Dupl3xx 编写和维护。源代码仓库：[salamander-sftp-plugin](https://github.com/Dupl3xx/salamander-sftp-plugin)。 |
| Martin Prikryl | WinSCP 插件 | WinSCP 插件的部分内容由 Martin Prikryl 授权。Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Windows Mobile 插件 | 包含由 Juraj Rojko 编写的软件。Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Samandarin Update Notifier 插件 | 由 Microsoft Corporation 根据 Microsoft .NET Framework 许可条款提供。 |

## 硬件监控器扩展

| 组件 | 用途 | 归属和许可证说明 |
| --- | --- | --- |
| HardView | 硬件监控器扩展 | C++/CLI 桥接库，用于硬件监控。Copyright (c) 2025 gafoo。根据 MIT License 授权。源代码：[gafoo173/HardView](https://github.com/gafoo173/HardView)。 |
| LibreHardwareMonitorLib 0.9.6.0 | 硬件监控器扩展 | .NET 硬件监控库。LibreHardwareMonitorLib 根据 GNU General Public License v3.0 授权。源代码：[LibreHardwareMonitor/LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor)。 |
| HidSharp 2.6.4.0 | 硬件监控器扩展 | HID 设备访问库，LibreHardwareMonitorLib 的传递依赖项。Copyright (C) 2010 James F. Bellinger。根据 GNU Lesser General Public License v3.0 授权。 |

## 用户界面、文档和 Markdown 渲染

| 组件或贡献 | 用途 | 归属和许可证说明 |
| --- | --- | --- |
| cmark-gfm | CommonMark 和 GitHub-Flavored Markdown 解析/渲染 | GitHub 对 `commonmark/cmark` 的分支，一个用 C 编写的 CommonMark 解析和渲染库及程序。Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Markdown 文档样式 | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Tree View 面板更改 | Tree View 面板 | 基于 fgodoy 提出的更改。 |
| 本地化脚本 | 本地化工具部分 | Samandarin 包含从 [Sally](https://github.com/0xeb/sally) 的 fork [Elias Bachaalany (0xeb)](https://github.com/0xeb) 改编或大量受其启发的本地化工具。根据 GNU Library General Public License 授权。 |
| 命令外壳模板 SVG 图标 | Windows Terminal 配置文件菜单图标 | SVG 图形改编自 Microsoft 产品图标和 SVG Repo 的 Azure Cloud Shell 图标。 |
