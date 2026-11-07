# S23 acceptance

## S23 — IMPLEMENTED / exact array reservation PASS (2026-10-23)

src/common/array.h and array_reserve_tests.cpp/vcxproj copied EXACT from pinned
donor47661e92. Optional Reserve retains Count/items/state/Base/Delta on failure,
rounds to the existing growth lattice, checks overflow; empty reserved storage
is released by DestroyMembers/DetachMembers without reviving failed storage.
No new container, scanner, ordering or panel publication changes. Existing test
runner discovers the new project automatically; no runner edits required.

Actual shipped template native20 checks/config Debug/Release PASS, with only
realloc failure injection: failure does not poison later insertion, rounding,
negative/overflow inputs, insert/delete, empty cleanup, object lifetime counts.
Common-header consumer audit: host+Salmon x64 and Translator+Trace Server Win32
(their supported platform), all8 Debug/Release /warnaserror builds PASS. Host
52.09/82.82s final; initial Release95s linker timeout retained separately. Native
builds2.69/2.99s. The plugin SDK does not include this common header; no unrelated
30-plugin SDK rebuild claimed. reglib source consumer is compiled in host;
historical standalone REGLIBA is outside current supported main solution.

All24 source/menu groups PASS. Actual normal panel smoke D/R: two files in >260
UTF16 Unicode directory, explicit selection50/54 bytes; normal exit/no reports.
All4 fixture hashes unchanged; private configs restored. Hashes/evidence under
../evidence/S23. No public gate, SDK layout, user configuration, push/PR/CI change.
