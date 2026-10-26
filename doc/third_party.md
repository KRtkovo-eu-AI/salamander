<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-06 | Preserve upstream and Branch View third-party credits. | Integrate the port without losing drive-query or JSON Viewer attribution. | https://github.com/KRtkovo-eu-AI/salamander/pull/769 | GPT-6 (Codex) |
| 2026-11-05 | Credit pd4d10/json-viewer for JSON Web Viewer. | Salamatrix WebView2 JSON extension vendors that renderer. |  | Composer |
| 2026-11-04 | Rename the product to Open Samandarin. | Distinguish this community fork from upstream Open Salamander. | https://github.com/KRtkovo-eu-AI/salamander/issues/776 | Grok 4.7 |
-->
# Third-Party Software Notices

[Open Samandarin](https://samandarin.net/) builds on the work of many authors, maintainers, and
projects. This page records the components, code, ideas, and tooling that are
used by the application and its plugins. Thank you to everyone listed here for
making their work available.

## Core Application and Shared Libraries

| Component | Used for | Attribution and license notes |
| --- | --- | --- |
| Branch Rename metadata and temporary-selection integration | Exact per-item rename, cache/focus refresh and safe selection cleanup in experimental Branch View | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Scoped BranchItemRenamed/declaration/success hooks reuse the accepted S20a wide I/O and selection header; local lock/callback and modal identity guards adapt the staged target. Existing native harness extended without removing previous checks. GPL-2.0-or-later; source notices preserved. |
| Branch flat-copy source planning | Actual per-item wide parent/name, ADS/network flags and read-storage scheduling | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Scoped BuildScriptMain hunks reuse accepted S19/S20b worker I/O; local copy-only/cache/CWD guards and existing native harness test adapters support staged activation. GPL-2.0-or-later; source notices retained. |
| Branch relative-directory copy | Optional localized copy dialog, per-file ancestors and Skip labels | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Scoped hunks preserve target paths and scheduling; local CWD failure cleanup and native test adapters. GPL-2.0-or-later; notices preserved. |
| Branch move source planning and completion refresh | Exact source scheduling, volume status and Branch notification dispatch | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, Open Salamander Authors and Samandarin Contributors. Scoped hunks and native test adapters preserve staged safety gates; GPL-2.0-or-later, notices retained. |
| Branch permanent deletion and confirmations | Exact parent/name planning and relative identity in confirmation | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, Open Salamander Authors and Samandarin Contributors. Local single-item wide confirmation, safe unknown worker configuration and native extraction test adapters; GPL-2.0-or-later, notices preserved. |
| Branch exact-selection recycle integration | Whole-selection queue, per-item paths and failure-safe setup | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, Open Salamander Authors and Samandarin Contributors. Retains the accepted target recycle/normalization policy; local HRESULT/current-identity guards and native queue/worker adapters. GPL-2.0-or-later, notices preserved. |
| Branch archive entry by content | Exact item identity when opening unknown archive extensions | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, Open Salamander Authors and Samandarin Contributors. Local callback guards, dynamic archive display/unload storage and native test adapters; GPL-2.0-or-later, notices preserved. |
| Branch shortcut and reparse target navigation | Exact wide target paths and focus from experimental Branch View | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Local callback/tab identity guards, filesystem PIDL normalization and native/GUI integration tests preserve Windows Resolve policy. GPL-2.0-or-later; source notices preserved. |
| Branch attribute source planning | Exact per-item parent and basename during attribute operation preparation | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Scoped action admission preserves local cache/CWD guards; existing actual-block harness extended without removing assertions. Basic attribute and NTFS command activation validated locally in S31b3a3/S31b3a4b. GPL-2.0-or-later; source notices preserved. |
| Branch attribute dialog integration | Exact operation selection across modal callbacks | Controller and native extraction pattern adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Local owned tab/root/generation/path guards preserve current selection across basic and NTFS attribute dialogs. GPL-2.0-or-later; notices preserved. |
| Attribute worker regression harness | Exact wide file/time I/O and failure handling before Branch attribute activation | Test extraction and owned-fixture patterns adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Local fix to existing DoChangeAttrs propagates timestamp/restore failures through established error handling. GPL-2.0-or-later; source notices preserved. |
| Direct unpack source identity | Owned exact archive path before modal callbacks | Local integration repair of CFilesWindow::Unpack, based on the exact-path helpers and native extraction pattern in [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, Open Salamander Authors and Samandarin Contributors. Real owned file identity tests preserve staged command gates and Windows errors; GPL-2.0-or-later, source notices retained. |
| Branch metadata dialog integration | Owned exact paths, guarded property writes and wide localized errors | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, Open Salamander Authors and Samandarin Contributors. Local modal/selection guards, safe typed-wide display, tracked SVG bitmap cleanup and actual-code native test adapters preserve Windows handlers and capability checks. GPL-2.0-or-later; notices retained. |
| Windows metadata I/O regression harness | Exact Unicode/long JPEG property writes, reads and clear with real Windows handlers | Actual-code extraction/property-test pattern adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Local WIC-owned fixtures test existing host metadata functions without modifying production or weakening handler capabilities. GPL-2.0-or-later; notices preserved. |
| Branch NTFS confirmations and command activation | Wide localized confirmation identities and guarded compression/EFS operations | Controller and native extraction patterns build on [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Local typed-wide confirmation and existing owned-selection guards preserve Windows error/confirmation policy. GPL-2.0-or-later; notices preserved. |
| NTFS worker path and failure integration | Exact wide compression/EFS targets and time/read-only cleanup | Builds on worker code and native extraction patterns from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Local owned-path forwarding and error propagation preserve Windows APIs, SYSTEM confirmation and existing call compatibility. GPL-2.0-or-later; notices preserved. |
| REGEXP | Regular expression matching | Copyright (C) 1986 Henry Spencer, University of Toronto. |
| AES code | Cryptographic routines | Written by Dr Brian Gladman. Copyright (C) 2001 Dr Brian Gladman. |
| PNGLite | PNG image support | Based on PNGLite by Daniel Karling. Copyright (C) 2007 Daniel Karling. |
| Nano SVG | SVG parsing/rendering support | Copyright (c) 2013-2014 Mikko Mononen. Used by the main application toolbar/menu icons and by the PictView plugin for SVG viewing (NanoSVG limits apply: no CSS, limited text and filters). |
| PictView deflate inflater | PictView plugin | Deflate/zlib inflater used for XCF tiles, PNG IDAT, SKP ZIP, and compressed 3DM previews. The bit-reader and Huffman construction follow Mark Adler's puff. Copyright (C) 2002-2013 Mark Adler. zlib license. |
| openNURBS 3DM preview layout | PictView plugin | Rhinoceros `.3dm` properties-table preview chunks and `ON_WindowsBitmap::ReadCompressed` layout follow [McNeel openNURBS](https://github.com/mcneel/opennurbs). Copyright (c) Robert McNeel & Associates. Used as a format reference only; openNURBS itself is not bundled. |
| SQLite | Embedded database support | SQLite is in the Public Domain. |
| LibTomCrypt | Checksum plugin cryptographic primitives | LibTomCrypt is public domain. As Tom St Denis wrote: "As should all quality software be." |
| LGPL libraries | Shared third-party libraries | Licensed under the GNU Library General Public License. A copy of the license is included with this software. |
| Unicode and Win32 Long Paths | Filename handling portions | Samandarin includes filename handling code and implementation work derived from, or substantially informed by, fork [Sally](https://github.com/0xeb/sally) by [Elias Bachaalany (0xeb)](https://github.com/0xeb). Licensed under the GNU Library General Public License. |
| Lua 5.5.0 | Bundled interpreter for the Salamatrix Lua Runtime | [Lua.org, PUC-Rio](https://www.lua.org/), installed from the pinned vcpkg `lua[tools]` package. Copyright (c) 1994-2025 Lua.org, PUC-Rio. MIT License; the complete notice is distributed as `plugins/extension-runtimes/luaruntime/runtime/LICENSE-LUA.txt`. |
| win32-darkmodelib | Native Windows dark-mode support for shared and Salamatrix dialogs | Vendored from [ozone10/win32-darkmodelib](https://github.com/ozone10/win32-darkmodelib), commit `b58eb027c4a114c1b1c8fd09870ead982f1b1e72`. Copyright (c) 2025-2026 ozone10. Licensed under the Mozilla Public License 2.0, with portions under the MIT License; see `src/third_party/darkmodelib/LICENSE.md` and `LICENSE-MIT.md`. |
| llama.cpp | On-demand CPU inference executable for SalamatrixAI | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp), pinned release `b10107`. MIT License; the configuration downloader retrieves the runtime and its license notice directly for the user. |
| Qwen2.5-Coder 1.5B Instruct GGUF | Recommended local SalamatrixAI model | [Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-1.5B-Instruct-GGUF), pinned `q4_k_m` file. Apache License 2.0; the configuration downloader retrieves the model and license notice directly for the user. |
| Qwen2.5-Coder 0.5B Instruct GGUF | Lightweight, English-prompt-only local SalamatrixAI model | [Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF](https://huggingface.co/Qwen/Qwen2.5-Coder-0.5B-Instruct-GGUF), pinned `q4_k_m` file. Apache License 2.0; the configuration downloader retrieves the model and license notice directly for the user. |
| Salamatrix owned sides/context serialization | Dynamic full UTF-8 JSON from wide snapshots and explicit native ABI limits | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Three production files preserve SPDX/copyright; adapted consumer tests validate actual payload, capacity and framing behavior. GPL-2.0-or-later. |
| Owned worker operation paths | Wide copy/retry/ADS, directory and attribute I/O with existing scheduling | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Scoped worker/BuildName hunks, exact path helpers and native fixtures; preserve target handle-accounting fix, add BuildName acceptance coverage. GPL-2.0-or-later, original notices retained. |
| Wide viewer enumeration and captions | Optional QueryService navigation with complete owned paths and paired PictView consumer | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Scoped host service/dispatcher and exact viewer/PictView sources and native tests; GPL-2.0-or-later, original notices preserved. |
| Wide file-action launch | Exact Unicode/long argv and current directory for external editor/viewer and user menu | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Scoped donor launch hunks and native child-process tests; source notices preserved. GPL-2.0-or-later. |
| Wide disk file-size boundary | Exact Unicode/long file and symlink target sizes with explicit errors | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Milan Kase, Open Salamander Authors and Samandarin Contributors. SalGetFileSize2 and both size test files copied exactly; ordinary getter harness adapted to S04 implementation, real Branch assertions deferred to S26. GPL-2.0-or-later, notices preserved. |
| Undelete captured restore identities | Owned wide restore paths and lossless checks at legacy image boundary | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Milan Kase, Open Salamander Authors and Samandarin Contributors. Eight production files and C++ fixture copied exactly; Python adapts configuration selection only. EFS fixture uses a stub. GPL-2.0-or-later, original source notices preserved. |
| PAK existing-only Optimize snapshot | Stable captured inputs, exact parent notifications and no creation of missing optimization sources | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Milan Kase, Open Salamander Authors and Samandarin Contributors. Five production files and C++ fixture copied exactly; Python adapts current shared test prefix and configuration selection. GPL-2.0-or-later, source notices preserved. |
| 7-Zip owned focused archive input | Stable full path and progress/result strings for Test Archive | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Milan Kase, Open Salamander Authors and Samandarin Contributors. Three production files and complete native C++ fixture copied exactly; Python driver adapts existing compiler helper import. GPL-2.0-or-later, original source notices preserved. |
| ZIP owned archive menu paths | Stable selected/focused command inputs and exact parent notifications | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Milan Kase, Open Salamander Authors and Samandarin Contributors. Three production files copied exactly; shared ZIP/7-Zip test adapted to defer 7-Zip until S14. GPL-2.0-or-later, source notices preserved. |
| Paired PictView thumbnail and EXIF migration | Owned inputs, wide safe EXIF replacement and retained legacy export | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Milan Kase, Open Salamander Authors and Samandarin Contributors. Four production files and C++ fixture copied exactly, driver adapted only for configuration selection, source notices preserved. GPL-2.0-or-later. |
| WIC thumbnail callback output preflight | Long path recognition and legacy memory output before paired EXIF migration | Existing WIC implementation by Open Salamander Authors and Samandarin Contributors, also present unchanged in [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`. Local baseline repair reuses the encoder; native harness adapts the donor disk selection test driver pattern. GPL-2.0-or-later. |
| Automation owned COM items and collection snapshots | Stable wide item identities, shared snapshots and enumeration after panel changes | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Milan Kase, Open Salamander Authors and Samandarin Contributors. Production files preserve SPDX/copyright; native test adapted to stage Automation separately from S11 Salamatrix and exercise actual collection/enumerator code. GPL-2.0-or-later. |
| Split & Combine owned inputs, wide paths and input identity safety | Stable source snapshots, actual-parent targets/discovery, UTF-8 BAT and guarded output creation | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Production files and native actual-consumer harness preserve SPDX notices; test runner registration adapted to existing stages. GPL-2.0-or-later. |
| Checksum owned source identities, wide Calculate/Verify and manifest export | Stable disk snapshots before modeless workers, exact per-item paths and manifest-parent resolution | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Production hunks and native command/worker/parser/export harness preserved with SPDX notices. Native digest facade is a stub; local validation additionally uses the real plugin and independently calculated hashes. GPL-2.0-or-later. |
| Batch Rename owned identities, preview and wide rename/Undo I/O | Immutable disk selection, per-item parents, bounded path generation and exact file operations | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Production hunks, renamer_paths.h/renamer_io.h and native test harnesses preserve original SPDX notices; local integration retains existing test registrations and verifies distinct same-basename contents through actual rename/Undo. GPL-2.0-or-later. |
| File Comparator owned disk selections and native consumer tests | Exact selected/focused identities, unambiguous target pairing and bounded UTF-8 input | From [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Production hunks and native test driver preserved; native template additionally checks the snapshot UTF-8 capacity boundary. Existing dialog cleanup fix retained. GPL-2.0-or-later; original SPDX notices preserved. |
| Icon-reader root scratch safety prerequisite | Heap-backed narrow/wide preparation without changing legacy shell path limits | Narrow CPathBuffer sizing adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Independent wide scratch and UTF-8-first/ACP character-offset repair avoid baseline corruption before S06 validation; full Branch/icon/overlay migration remains deferred to S26/S28. Native tests compile the actual preparation and path-buffer methods using the existing SafeFile test driver. GPL-2.0-or-later; SPDX notices preserved. |
| Owned disk selection SDK, host service and DemoPlug example | Immutable copied disk-row metadata and paths through optional QueryService | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. SDK header, host service and released ABI golden preserved; DemoPlug retains its existing archive example. Native SDK/ownership assertions retained, PictView consumer subset deferred to S12. GPL-2.0-or-later; original SPDX notices preserved. |
| Ordinary panel path getters and PanelItemPaths service | Complete Unicode/long disk-row paths through optional QueryService | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Ordinary getter subset moved from `branch_view.cpp`; existing wide join retained and byte fallback corrected to UTF-8-first/ACP. Tests adapt the donor ordinary getter fixtures and existing SafeFile compiler harness. GPL-2.0-or-later; original SPDX notices preserved. |
| SafeFile Retry/Skip, Unicode/long-path I/O and native harness | Core file I/O regression coverage; S03 accepted at 770f48e0 after local runtime and exact-head CI | Adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Samandarin Contributors. GPL-2.0-or-later; original SPDX notices are preserved. |
| ZIP listing ownership, archive dispatch and regression harness | Baseline leak and wrong-plugin unpacking repair required by S03 runtime validation | Builds on [KRtkovo-eu-AI/salamander](https://github.com/KRtkovo-eu-AI/salamander), target `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449`, by Open Salamander Authors and Samandarin Contributors. Test extraction/compiler patterns adapted from the SafeFile harness in PR #769, donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`. GPL-2.0-or-later; original SPDX notices preserved. Independent baseline fixes, no additional donor feature import. |
| Core wide handle wrappers and native fixtures | LoadImage and directory-notification A/W tracking | Wrapper hunks from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, building on Open Salamander Authors and Samandarin Contributors. Fixture lifecycle adapted from its SafeFile native harness; direct core tracker coverage added locally. GPL-2.0-or-later; original SPDX notices preserved. |
| Plugin wide file handle wrappers and native diagnostics tests | CreateFile and FindFirstFile A/W monitoring | Wrapper hunks from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`, by Open Salamander Authors and Samandarin Contributors. Fixture lifecycle adapted from its SafeFile harness via S02a; direct registry/fault coverage added locally. GPL-2.0-or-later; SPDX preserved. S02b accepted at code checkpoint `23d5406d` after full local runtime and exact-head fork CI. |
| Test Reporter | Pull-request test result reports | [dorny/test-reporter](https://github.com/dorny/test-reporter), used by GitHub Actions to publish JUnit and Python xUnit results as Checks and workflow summaries. MIT License. |
| Native Samandarin shutdown lifetime and regression harness | Bounded initialization readiness, callback draining and timely SDK diagnostics | Independent baseline repair building on [KRtkovo-eu-AI/salamander](https://github.com/KRtkovo-eu-AI/salamander), target `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449`, by Open Salamander Authors and Samandarin Contributors. Sent-message-only join adapted from its `src/plugins/shared/salamatrix_thread_join.h`; native project/harness follows accepted S02b tests. GPL-2.0-or-later; original SPDX notices preserved. No additional donor feature import. |
| pytest | Python test execution and xUnit report generation in pull-request CI | [pytest-dev/pytest](https://github.com/pytest-dev/pytest). MIT License. |
| cursor-sdk | Optional Python SDK for batch SLT translation through the Cursor API | Used only by `tools/localization/translate_slt_with_openai.py` when `-Provider cursor` is selected. Install with `pip install cursor-sdk`. |
| OpenRouter API | Optional remote provider for batch SLT translation | Used by `tools/localization/translate_slt_with_openai.py` by default with the `openai/gpt-5.4-nano` model. Requires a user-supplied `OPENROUTER_API_KEY`; no key is stored in the repository. |

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
| TagLib 2.3 | Multimedia Viewer plugin | Audio metadata library by Scott Wheeler and TagLib contributors. Used for Unicode metadata and audio properties in Ogg Vorbis, Opus, FLAC, MP4, APE, WavPack, TrueAudio, AIFF, DSD, Matroska/WebM, and related formats. Distributed under the GNU LGPL 2.1 or Mozilla Public License 1.1. Source: https://github.com/taglib/taglib |
| Internal viewer | Unicode viewer portions | Samandarin includes code and implementation work derived from, or substantially informed by, fork [Sally](https://github.com/0xeb/sally) by [Elias Bachaalany (0xeb)](https://github.com/0xeb). Licensed under the GNU Library General Public License. |
| Jan Patera | PictView plugin | Portions of the PictView plugin are licensed from Jan Patera. Copyright (C) 1994-2026 Jan Patera. |
| libexif | PictView plugin | Portions of the PictView plugin use libexif. Copyright (C) 2001-2019 Curtis Galloway and Lutz Muller. |
| Newtonsoft.Json 13.0.3 | JSON Viewer plugin | By James Newton-King, released under the MIT License. |
| Microsoft .NET Framework 4.8 | JSON Viewer plugin | Provided by Microsoft Corporation under the Microsoft .NET Framework license terms. |
| pd4d10/json-viewer 2.2.0 | JSON Viewer (nodeJS) Salamatrix extension | Port of Firefox's JSON Viewer for Chromium by Rongjian Zhang ([pd4d10/json-viewer](https://github.com/pd4d10/json-viewer)). MIT License for the wrapper; the bundled UI includes Mozilla DevTools JSON Viewer code under MPL-2.0 and React under MIT. Vendored as `src/extensions/json-viewer-nodejs/vendor/pd4d10-json-viewer/` (`content-script.js` from the Chrome Web Store package). |
| Prism.js 1.29.0 | Viewer Frame (Salamatrix and Prism Text Viewer) | By Lea Verou and PrismJS contributors, released under the MIT License. |
| Microsoft.Web.WebView2 1.0.2420.47 | Viewer Frame | By Microsoft Corporation, distributed under the BSD 3-Clause License. Prism virtual-host naming follows Microsoft's [virtual host mapping guidance](https://learn.microsoft.com/en-us/dotnet/api/microsoft.web.webview2.core.corewebview2.setvirtualhostnametofoldermapping) to avoid navigation delays with `.local`. Bounded Prism instance reuse follows Microsoft's [WebView2 performance guidance](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/performance#avoid-redundant-webview2-instances). |
| Markdig 0.36.2 | MarkdigRenderer for WebView2 Render Viewer | By Alexandre Mutel and contributors, released under the BSD 2-Clause License. |

## Network, Synchronization, and Device Plugins

| Component or author | Used in | Attribution and license notes |
| --- | --- | --- |
| OpenSSL | FTP Client plugin and SFTP dependencies | Copyright (c) 1998-2026 The OpenSSL Project Authors. All rights reserved. OpenSSL 1.0.x is distributed under the OpenSSL and original SSLeay licenses. OpenSSL 3.x is distributed under the Apache License 2.0. The vcpkg pipeline installs OpenSSL for the FTP plugin compatibility DLLs and for the SFTP plugin dependencies. |
| libssh2 | SFTP plugin | By Daniel Stenberg, Simon Josefsson, and libssh2 contributors. Distributed under the BSD 3-Clause License. The vcpkg pipeline installs the `libssh2` package for the SFTP plugin dependencies. |
| Dupl3xx | SFTP plugin | Plugin authored and maintained by Dupl3xx. Source repository: [salamander-sftp-plugin](https://github.com/Dupl3xx/salamander-sftp-plugin). |
| Martin Prikryl | WinSCP plugin | Portions of the WinSCP plugin are licensed from Martin Prikryl. Copyright (C) 2000-2026 Martin Prikryl. |
| Juraj Rojko | Windows Mobile plugin | Includes software written by Juraj Rojko. Copyright (C) 2003-2026 Juraj Rojko. |
| Microsoft .NET Framework 4.8 | Samandarin Update Notifier plugin | Provided by Microsoft Corporation under the Microsoft .NET Framework license terms. |

## Hardware Monitor Extension

| Component | Used in | Attribution and license notes |
| --- | --- | --- |
| HardView | Hardware Monitor extension | C++/CLI bridge library for hardware monitoring. Copyright (c) 2025 gafoo. Licensed under the MIT License. Source: [gafoo173/HardView](https://github.com/gafoo173/HardView). |
| LibreHardwareMonitorLib 0.9.6.0 | Hardware Monitor extension | .NET hardware monitoring library. LibreHardwareMonitorLib is licensed under the GNU General Public License v3.0. Source: [LibreHardwareMonitor/LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor). |
| HidSharp 2.6.4.0 | Hardware Monitor extension | HID device access library, transitive dependency of LibreHardwareMonitorLib. Copyright (C) 2010 James F. Bellinger. Licensed under the GNU Lesser General Public License v3.0. |

## User Interface, Documentation, and Markdown Rendering

| Component or contribution | Used for | Attribution and license notes |
| --- | --- | --- |
| cmark-gfm | CommonMark and GitHub-Flavored Markdown parsing/rendering | GitHub's fork of `commonmark/cmark`, a CommonMark parsing and rendering library and program in C. Copyright (C) 2009 Public Software Group e. V., Berlin, Germany; Copyright (C) 2012 Vicent Marti; Copyright (C) 2012 GitHub, Inc.; Copyright (C) 2014-2015 John MacFarlane; Copyright (c) 2013 Karl Dubost. |
| github-markdown-css | Markdown document styling | Copyright (c) 2014 Dave Liepmann; Copyright (c) Sindre Sorhus; Copyright (c) 2016 Osmo Salomaa. |
| Base of Tree View panel | Tree View panel | Based on changes proposed by fgodoy. |
| Localization scripts | Localization tooling portions | Samandarin includes localization tooling adapted from, or substantially informed by, fork [Sally](https://github.com/0xeb/sally) by [Elias Bachaalany (0xeb)](https://github.com/0xeb). Licensed under the GNU Library General Public License. |
| Command shell template SVG icons | Windows Terminal profile menu icons | SVG artwork adapted from Microsoft product icon shapes for PowerShell, Windows PowerShell, Windows Terminal, Command Prompt, and Visual Studio, and from the SVG Repo Azure Cloud Shell icon. |

### S20a ordinary rename and temporary selection

Header/native fixtures EXACT and scoped host rename/selection hunks ADAPTED from [PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Samandarin Contributors, GPL-2.0-or-later. Original SPDX notices preserved. Branch integration deferred.

### S20b ordinary copy/move planning

Scoped donor planning buffers, Unicode Copy-of formatter and item-parent split-path hunks adapted from [PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, GPL-2.0-or-later; original notices preserved. Exact NameW capture extends ordinary-panel identity handling.

### S20c exact delete/recycle and typed error formatting

Scoped host/worker hunks, operationrecycle.h, recycle-only native fixture and typed text tests adapted from [PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, GPL-2.0-or-later. branch_view_text.h and its test project copied exactly; original notices preserved. Local adaptation resolves ordinary recycle eligibility through the exact wide volume root. Future Branch resource tests retained but deferred.

### S21a exact Shell and clipboard identities

branchshell.h, branchclipboard.h and their full context-menu/operation native fixtures/projects are copied exactly from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin Contributors, GPL-2.0-or-later. Original notices preserved; the already adapted S20c recycle helper remains unchanged.

### S21b ordinary disk Shell call sites

Scoped shellib, shellsup, fileswn5 and fileswn9 hunks derive from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin Contributors, GPL-2.0-or-later. Disk guard adaptation reuses existing exact item getters; no Branch stubs or virtual-panel changes.

### S22 owned archive enumeration

CPanelTmpEnumData, PanelEnumDiskSelection and the actual-function fixture derive from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin Contributors, GPL-2.0-or-later. Exact fixture driver; explicit file-only disk route adapter until Branch integration. Notices retained.

### S23 array capacity reservation

common/array.h Reserve/empty cleanup and array_reserve_tests.cpp/vcxproj copied exactly from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin Contributors, GPL-2.0-or-later. Original notices retained.

### S25 isolated Branch scanner

branch_view.h/.cpp scanner/path/snapshot subset and branch_view_tests derive from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin Contributors, GPL-2.0-or-later. Exact shared worker lifetime implementation; fixture-only future scheduler/sort checks retained deferred. All original notices preserved.

### S26a association preparation and reentrant lifetime

AssociationPreparation/session/lifetime, internal Shell association epoch/cache guards and branch_preparation_tests derive from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin Contributors, GPL-2.0-or-later. Exact helper/test blocks, scoped host hunks preserving target icon accounting; all notices retained.

### S27a1 Branch viewer/editor and exact parent navigation

Scoped OpenBranchItemDirectory/declaration/menu/dispatch derive from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin Contributors, GPL-2.0-or-later. Local current-row/preflight/wide-focus and transition viewer-policy adapters preserve target navigation and the S18 launch implementation; original notices retained. Enter/archive dispatch remains deferred after actual GUI boundary failures.

### S27a2 Branch Enter and archive metadata preflight

Scoped Execute Branch Shell/archive dispatch and CM_OPEN allowance derive from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Current-cache/owned-path guards, explicit long
Shell rejection and wide archive metadata preflight are local adaptations.
Original notices retained. Archive correction validated; long executable Shell
launch remains unresolved and S27a2 is not fully accepted.

### S27a2 comparative launch research (no production source import)

Reference-only analysis credits Eugene Roshal/Far Group (FarManager, BSD-3-Clause),
Alexander Koblov and Double Commander contributors (GPL-2.0-or-later), the Explorer++
Project (GPL-3.0-only), Files Community (MIT), and Directory Opus/GPSoftware developer
guidance. Pinned source links and findings are recorded in
[the S27a2 comparison](branch-view-port/steps/S27a2-shell-comparison.md).
No source from these projects was imported into production. The local independent
Shell class-selection probe did not solve the long-path launch failure.

### S27a2 program/document separation

Independent local implementation, Samandarin Contributors, GPL-2.0-or-later.
FAR Manager (Eugene Roshal/Far Group) and Files Community informed the conceptual
program/document split; their source code and fallback chains were not imported.
Microsoft Windows Attachment Services owns final authorization and execution.
S27a2 is now locally completed with explicitly accepted unsupported-case limits;
see [the completion record](branch-view-port/steps/S27a2-completion.md).

### S27d1 Branch Properties

Scoped quiet context-menu / complete IDataObject / SHMultiFileProperties fallback
hunks copied from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and
Samandarin Contributors, GPL-2.0-or-later. Original notices retained. Local current
cache/whole-selection guards and staged command gate adapt these hunks to the
incremental port. Accepted S21 Shell helpers and native tests reused unchanged.

### S27d2 Branch clipboard copy

Existing complete Shell data-object/clipboard publication block and native Shell
identity tests reused from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and
Samandarin Contributors, GPL-2.0-or-later. Original notices retained. Local staged
copy-only activation and actual-block regression harness adapt the accepted S21
implementation to Branch selection. No duplicate Shell framework introduced.

### S27d3a Branch cut export

Cut publication, full-identity cross-panel marking and ClearCutToClipFlag reused
from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Original notices retained. Staged activation and
actual-block native regression harness are local incremental-port adaptations.

### S27d3b Branch clipboard paste

Wide clipboard ingestion and operation planner reused from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Original notices retained. Owned wide-first drop
metadata adapter and actual-function regression cases are local adaptations.

### S27d4 Branch context menu

TrackBranchHostContextMenu, containing-folder interception, mixed-parent fallback
and deferred command dispatch copied as scoped hunks from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Original notices retained. Local staged activation
and native recording-menu facade adapt the verified donor behavior to this port.

### S27d5a Branch drag source

Existing drag source and QueryContinueDrag are reused from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Original notices retained. Local staged activation,
nothrow allocation and explicit native facades preserve the port's failure contract.

### S27d5b Branch receiving

Native drop/clipboard ingestion and wide worker integration are reused from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Original notices retained. Private root identity,
staged safe receiving, queue ownership guards and native boundary tests are local
adaptations; no public plugin ABI or Windows security policy changes.

### S27e1 Branch ZIP/7-Zip packing

The host selection callback and archive integration reuse
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Original notices retained. The staged bundled-packer
gate, nothrow allocation adaptation and native boundary facades are local work;
ZIP/7-Zip engines and the public plugin ABI remain unchanged.

### S27e2 Branch archive move cleanup

Archive callbacks and ZIP/7-Zip packing/cleanup algorithms reuse
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Original notices retained. Wide source deletion,
full-path UTF8 preservation, staged activation and owned native fixtures are local
adaptations; original archive success/cancellation behavior and plugin ABI remain.

### S27f.01 Branch File Comparator integration

Owned File Comparator selection/paths and the existing engine reuse
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Original notices retained. The staged private host
command policy and recording dispatch tests are local adaptations; comparator
consumer blobs and plugin SDK ABI remain unchanged.

### S27f.02 Branch Batch Rename integration

Batch Rename owned selection and wide I/O reuse
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Original notices and plugin blobs retained.
The narrow private host command permission and recording tests are adaptations;
plugin engine and SDK ABI are unchanged.

### S27f.03 Branch Checksum Calculate/Verify integration

Checksum owned selection, manifest handling and engines reuse
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Original notices and all plugin blobs retained.
Private host command permission and recording dispatch tests are local adaptations.

### S27f.04 Branch Split & Combine integration

Split & Combine owned paths, plans and I/O engines reuse
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Original notices/plugin blobs retained.
Private host command permission and dispatch tests are local adaptations.

### S27f.05 Branch Automation integration

Automation owned COM selections and script consumers reuse
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Existing notices and accepted S10/S11 adaptations
are retained. Private host command permission and dispatch tests are local adaptations.

### S27f.06a Branch Salamatrix package integration

Owned full-path context and runtime consumers reuse
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Existing notices/accepted adaptations retained.
Private registered-command permission and dispatch tests are local adaptations.
Local validation uses PHP Group's official PHP CLI and existing Lua.org/PUC-Rio MIT
interpreter assets, with notices preserved; no new interpreter redistribution.

### S27f.06b Branch Automation menu integration

Automation repository/manifest and immutable selection consumers reuse
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, including work by Milan Kase,
Ondřej Kotas, Open Salamander Authors and Samandarin Contributors,
GPL-2.0-or-later. Existing notices retained. Private posted-command gates,
repository macro/wide-path and one-shot dispatch integration fixes, balanced
handle tracking and focused native regression are local adaptations.

### S27f.07 PictView Branch thumbnail integration

PictView thumbnail selection and paired EXIF10 consumers retain the accepted code
from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, including work by Milan Kase,
Open Salamander Authors and Samandarin Contributors, GPL-2.0-or-later. Existing
PictView/EXIF notices remain intact. The narrow host command gate and its policy
regressions are local integration adaptations; actual component code is unchanged.

### S27f.08a ZIP Branch command and message boundaries

Owned ZIP command selection retains [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, including work by Milan Kase,
Open Salamander Authors and Samandarin Contributors, GPL-2.0-or-later. Existing
notices retained. Exact menu association, private Test Archive gate, Unicode-safe
existing host hard wrapping and focused regressions are local integration repairs.

### S27f.08b1 ZIP Branch comments

Owned ZIP archive selection retains [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, including work by Milan Kase,
Open Salamander Authors and Samandarin Contributors, GPL-2.0-or-later. Existing
notices retained. Comment-only private gate, early stop on failed save reopen,
Unicode-created dialog and heap-backed title formatting plus native regressions
are local repairs of demonstrated integration/baseline defects.

### S27f.08b2 ZIP self-extracting archives

Retains owned ZIP/SFX commands from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, and the ZIP/SFX/PE resource
work of Milan Kase, Open Salamander Authors and Samandarin Contributors under
GPL-2.0-or-later; original file notices preserved. Heap paths, Unicode dialog/
resource I/O, invalid-handle and allocation-flow repairs and native tests are local
adaptations of demonstrated baseline prerequisites; original engines retained.

### S27f.09 7-Zip focused archive tests

Retains S14 owned TestArchive/ArchiveNameMessage from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander Authors and
Samandarin Contributors, GPL-2.0-or-later; all original notices and7-Zip engine
credits preserved. Narrow host command gate and exact wide focused-name association
are local integration adaptations of a reproduced truncated-display-name defect.

### S27f.11 Undelete command integration

Retains the S16 owned restore and Connect paths from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
pinned donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Milan Kase,
Open Salamander Authors and Samandarin Contributors, GPL-2.0-or-later. Original
notices preserved. Narrow command admission and private snapshot-based Unicode
Restore label are local integration adaptations of a reproduced GUI alias defect.

### S27f.12 Last Command routing

Adapts existing host Last Command lookup and dispatch from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
pinned donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander
Authors and Samandarin Contributors, GPL-2.0-or-later. Original notices retained;
local integration adds current-command policy and named-item validation to the
experimental Branch gate, preserving ordinary-panel routing.

### S27f.13 Checksum exact Focus

Adapts Checksum Focus and host focus routing from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
pinned donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander
Authors and Samandarin Contributors, GPL-2.0-or-later. Original notices retained.
Uses the already accepted S27a1 owned-wide navigation approach for disk panels;
local adaptation removes demonstrated legacy focus truncation without SDK changes.

### S27f.14 PictView refresh and Salamatrix API reference

Adapts auxiliary command integration from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
pinned donor47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander
Authors and Samandarin Contributors, GPL-2.0-or-later. Original notices retained.
PictView refresh production body remains exact donor; local Salamatrix helper
adaptation preserves wide installation identity and bounded UTF8 viewer input.

### S28 asynchronous properties and display integration

Restores selected-layout behavior from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
pinned donor 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander
Authors and Samandarin Contributors under GPL-2.0-or-later. Original notices
retained. Reuses the property worker, lifetime/generation guards, thumbnails and
association preparation already imported in S26a/b; adds only the narrow local
transition-command adapter and updated regression contracts.

### S28a optional relative Branch View Path display

Extends the Branch View integration derived from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
pinned donor 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander
Authors and Samandarin Contributors under GPL-2.0-or-later. Original notices
retained. The new Panels preference and display formatter are a local user-requested
extension absent from that donor; no additional third-party implementation imported.

### S29 Branch View tab and history restoration

Adapts tab snapshots, disk-root identity checks and history state ownership from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander Authors and
Samandarin Contributors under GPL-2.0-or-later. Original notices retained.
Existing capture/restore logic remains gated; local command admission and native
regression harness integrate it into the incremental port. Debug/Release GUI
validation passed; no complete restart-persistence guarantee is introduced.

### S30 Branch View recursive refresh and publication validation

Imports recursive watcher integration and connects the existing refresh scheduler
from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
pinned donor 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander Authors
and Samandarin Contributors under GPL-2.0-or-later. Original notices retained.
Scheduler, preview/publication and association-preparation algorithms were already
imported and are reused unchanged. Local integration preserves UTF8-first watcher
paths, owned long notification paths, and automatic-message semantics after Stop.

### S31a1 create-directory wrapper integration repair

Reuses the exact SalCreateDirectoryEx delegation to WorkerCreateDirectory from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander Authors and
Samandarin Contributors under GPL-2.0-or-later. Existing notices retained.
The previously ported wide helper is unchanged. The local regression harness
extracts actual host/SDK wrappers, reuses the existing MSVC test support and
adds real I/O plus controlled DOS-alias retry tests to cover the integration gap.

### S31a2 ANSI rename fallback integration repair

Reuses the exact early Branch dispatch from RenameFileInternal to its existing
wide implementation in [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
pinned 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander Authors and
Samandarin Contributors under GPL-2.0-or-later. Existing notices retained.
The wide implementation and its identity/cache guards remain unchanged.
Local actual-code native tests force ANSI edit fallback and exercise precise
Unicode/long-path I/O; no additional third-party implementation is imported.

### S31a3a Shell context-menu retry validation

Restores the exact post-retry ContextMenu/menu guard and quiet selection capture
from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
pinned 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander Authors and
Samandarin Contributors under GPL-2.0-or-later. Existing notices retained.
The local native regression extracts the real retry/guard/fallback flow and uses
controlled COM doubles with owned Win32 menus. Existing safety assertions remain.

### S31a3b full Branch contract activation

Retains the assertions adapted from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
pinned 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander Authors and
Samandarin Contributors under GPL-2.0-or-later. The local port now registers the
complete existing contract suites, adjusts the clipboard selector to the shared
disk implementation and retains rename notifications in All scope. No new
third-party implementation is imported; original provenance remains intact.

### S31b4b Unpack Unicode target integration

Adapts CUnpackDialog, CFilesWindow::Unpack and the existing widepath pattern
from [Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769),
pinned 47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander Authors and
Samandarin Contributors under GPL-2.0-or-later. Existing notices retained.
Local repairs add atomic Unicode target transfer, owned target/default buffers
and scoped wide empty-directory cleanup. Tests extract actual code into real
HWND and NTFS fixtures; no additional third-party code is imported. Branch
Unpack remains blocked while ZIP backend limits and modal guards are resolved.

### S31b4c1 ZIP extraction target integration

Adapts ExtractFiles/ExtractSingleFile and reuses ZipPathToWide from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander Authors and
Samandarin Contributors under GPL-2.0-or-later. Existing notices retained.
Local fixes validate exact extraction capacity and safe relative components,
and use wide output attributes/failed-output deletion while retaining the
host SafeFileCreate and existing engine CRC/cancel decisions. Native tests
extract actual functions into owned NTFS fixtures; no third-party code is added.

### S31b5a destination flow audit

Reviews FilesAction, ParsePath, SalParsePath and DropCopyMove against
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander Authors and
Samandarin Contributors under GPL-2.0-or-later. No additional third-party code
is imported; the audit records local integration prerequisites and retains guards.

### S31b5b1 path parser integration correction

Adapts CFilesWindow::ParsePath/SalParsePath from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, by Open Salamander Authors and
Samandarin Contributors, GPL-2.0-or-later. Existing notices preserved.
Local fixes preserve full wide/UTF8 context, reject failed/truncated bases,
and bound localized error/trace formatting. Native tests extract actual code
with explicit UI/plugin/absolute-name facades and real owned NTFS attributes.
No additional third-party code imported; Branch admission remains guarded.

### S31b5b2 destination identity integration

Locally adapts the FilesAction flow and existing target-tab registry lookup
pattern in mainwnd1.cpp. Reference context:
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and
Samandarin Contributors, GPL-2.0-or-later. Existing notices retained.
Adds owned wide destination identity and callback rechecks with explicit
native registry doubles; no additional third-party code or SDK ABI change.

### S31b5b3a directory creation notification correction

Locally adapts SalSplitWindowsPath notification storage from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and
Samandarin Contributors, GPL-2.0-or-later. Notices preserved. Native fixtures
extract the existing shared split/create code and use real owned NTFS directories
with explicit UI/notification doubles. No additional third-party code imported.

### S31b5b3b split path capacity and ABI bridge correction

Adapts shared path split/create functions and their core/SDK bridge callers from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Notices preserved. Local capacity guards and a
private Windows packing step retain exact fitting results and caller-owned mask
storage; public virtual SDK remains unchanged. Native tests extract actual code
with explicit UI/notification doubles and owned NTFS fixtures. No new third-party code.

### S31b5b3c1 atomic Copy/Move destination dialog

Adapts existing dialog/history flow from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Notices preserved. The local existing atomic
Unpack reader is reused unchanged. Native fixtures extract actual dialog/history
and conversions, with explicit base/criteria doubles and real Unicode controls.
No new third-party component imported.

### S31b5b3c2 implicit disk destination preflight

Local adaptation of FilesAction and its owned target snapshot from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Notices preserved. Exact default-text and parsed/
packed root-mask guards preserve explicit manual destination semantics. Tests
extract actual methods with explicit panel-registry doubles. No new third-party code.

### S31b5b3d1 disk Copy into a Branch destination

Locally staged activation of FilesAction from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Notices preserved. The existing destination and
worker flow is retained; the local admission policy enables only verified disk
Copy while other Branch destinations stay deferred. No new third-party import.

### S31b5b3d2 disk Move into a Branch destination

Locally staged admission of the existing shared FilesAction Move pipeline from
[Samandarin PR #769](https://github.com/KRtkovo-eu-AI/salamander/pull/769), pinned
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413, Open Salamander Authors and Samandarin
Contributors, GPL-2.0-or-later. Notices preserved. The private policy now admits
verified disk Copy/Move, keeping all owned-root and modal identity checks.
No new third-party code imported.
