# S22 acceptance

## S22 — IMPLEMENTED / archive callback and ZIP smoke PASS (2026-10-23)

Pinned donor CPanelTmpEnumData owned relative UTF8 name/link-size cache and actual
PanelEnumDiskSelection file callback ported; existing root+relative SDK ABI and
ZIP/7-Zip engines unchanged. DiskDirBuildEnumeration uses full item identity.
ADAPTED donor IsBranchView guard to explicit ExactFileSelection state, false by
default and enabled by Pack only for file-only disk selections. Directories/mixed
selections retain original recursive tree. SourcePanel is initialized; no fake
Branch methods. S27e will connect real Branch selection. No Branch activation.
Fixture driver EXACT; cpp.in ADAPTED solely for route state binding instead of
future Branch method; all assertions retained. Registered as native-generated.

Actual production callback/state/converter fixtures362/362 per Debug/Release
PASS: duplicate basenames in separate parents, all enum modes, ordered reset,
64-bit metadata, pointer lifetime, invalid index/identity/conversion/allocation
failure, cached link sizes/ignored errors across move-packer source removal.
Dependencies are deterministic fixture panel/link replies, not actual packing.
Host x64 Debug/Release /warnaserror PASS36.42/93.93s; final restored Debug8.65s.
All24 source/menu groups PASS. GUI actual ZIP plugin Debug/Release: ordinary.txt
and Czech/CJK/emoji name inside Unicode parent, correct exact archive member
names and decompressed SHA256. All4 input hashes unchanged. Release normal exit.

Debug ZIP creation uncovered shutdown heap warning, not hidden or called clean:
641 bytes/21 blocks, ten system MENU48+16 pairs and one1-byte01 block. Temporary
heap report logging and explicitly disabled new callback route reproduced the
same exact allocation profile using legacy callback/ASCII inputs. This isolates
the warning from the S22 callback; underlying allocator origin remains unresolved.
Diagnostic patches/logs retained outside Git; heap.cpp byte restored, fileswn7
production manifest rechecked, final production Debug rebuilt. Warning closed
normally; no process/report remains. No suppression or unrelated engine fix.

Private configstorage.ini restored; installed user host untouched. Evidence in
../evidence/S22 includes hashes, legacy comparison and actual ZIP validations.
No real Branch packing/move-packer GUI claim (S27e). No push/PR/CI.
