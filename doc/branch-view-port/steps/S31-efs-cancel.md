# S31 — EFS restore cancellation

Actual restore/owned-selection code now stops a known cancellation before opening
or truncating the destination, including cancellation requested by filename UI
callbacks. It treats ERROR_CANCELLED and a late progress cancellation as failure.
After closing handles it attempts exact-output cleanup and reports a Windows
cleanup refusal through the existing localized encrypted-restore error. It never
changes access, sharing, encryption or Windows security settings. Other I/O-error
partial recovery policy is unchanged.

Native actual-source tests retain existing assertions and add early/midstream,
late-success cancellation and cleanup-refusal scenarios: Debug/Release 89 checks
PASS each. UI keyboard state is explicitly simulated, with an Escape sizing case,
so a key pressed in another application cannot change this isolated test.

Actual Windows-generated EFS RAW exports, actual restore functions and real RAW
import API passed six path classes in each configuration, including byte-long,
UTF16-long and ADS. Early cancellation creates no output. Midstream cancellation
returns failure; Windows can temporarily deny deletion with sharing error 32.
That error is now reported, not ignored or overridden; an incomplete output can
remain. Exact original/default/alternate streams, backups and completed restores
were independently read back. Native service/UI policy remains an explicit facade.
Undelete plugin builds Debug and Release passed /warnaserror. Code signing was
explicitly skipped by the existing Release hook; no remote CI/CodeQL was run.

Evidence: `evidence/S31-efs-cancel/final-readback.json` outside Git binds source,
build artifacts, successful records and retained fixtures. The first old Debug
failure in `S31-efs-raw` remains unchanged. Its original DeleteFile error was not
captured, so its root cause cannot be retrospectively asserted; the unnecessary
pre-cancel creation and silent cleanup error are both corrected and verified.
A failed probe compile caused by a generated C++ literal remains recorded; its
corrected probe passed both configurations. Whole S31 remains ACTIVE.

Provenance: adapts Open Salamander Authors / Samandarin Contributors restore code
from KRtkovo-eu-AI/salamander PR #769, pinned
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413. No SDK virtual layout change.
