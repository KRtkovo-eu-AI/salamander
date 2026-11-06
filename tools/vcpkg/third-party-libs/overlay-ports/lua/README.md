<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-06 | Retain the pinned Lua port with a verified source mirror. | Keep clean CI builds available during Lua.org timeouts. | https://github.com/majklvi/salamander/pull/64 | GPT-6 (Codex) |
-->
# Pinned Lua port overlay

Copied from [Microsoft vcpkg](https://github.com/microsoft/vcpkg/tree/a0b1c8d3a477c1cb4813d8e127a56961707ca42b/ports/lua) at baseline `a0b1c8d3a477c1cb4813d8e127a56961707ca42b`, port tree `6767a2f27deb2b906d4353bd91c9bd26b30a7cf1`, version `5.5.0#1`. Copyright (c) Microsoft Corporation; the exact MIT notice is retained in `LICENSE-vcpkg.txt`.

The only upstream port change is an additional HTTPS URL in `portfile.cmake`. The remaining upstream files are copied byte-for-byte; local `.gitattributes` preserves their mixed line endings on checkout. `../../vcpkg-configuration.json` enables this overlay through standard manifest configuration, including for existing non-git vcpkg installations. This changelog also covers that JSON configuration and the unchanged upstream files, whose formats and notices are preserved.

The [Alpine Linux source archive](https://distfiles.alpinelinux.org/distfiles/edge/lua-5.5.0.tar.gz) is identical to the [official Lua.org release](https://www.lua.org/ftp/lua-5.5.0.tar.gz): 396950 bytes, official SHA256 `57ccc32bbbd005cab75bcc52444052535af691789dba2b9016d5c50640d68b3d`. The upstream SHA512 remains `3253d2cdc929da6438095a30d66ef16a1abdbb0ada8fee238705b3b38492f14be9553640fdca6b25661e01155ba5582032e0a2ef064e4c283e85efc0a128cabe`. Both checksums were checked against downloads from both HTTPS endpoints. vcpkg continues to reject archive contents that do not match its pinned SHA512.

The port keeps its original features, dependencies, patches, CMake build, interpreter/DLL installation, and Lua MIT license extraction. When changing the pinned Lua version, update this overlay from the corresponding upstream port and verify the mirror against the new upstream checksum.
