<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-06 | Repair PR #64 contracts, PowerShell UTF-8 pipes and pinned Lua downloads. | Resolve reproduced CI failures without reducing assertions or changing dependency versions. | https://github.com/majklvi/salamander/pull/64 | GPT-6 (Codex) |
-->

# S32 publication CI repairs - 6 November 2026

Baseline: `6226279ab38a1e1152aa3f71328bc1f84b654a2d`, branch
`feature/pr769-s32-public-activation`, existing fork validation draft #64.
This follow-up changes one runtime bootstrap, tests and dependency preparation.
Host/SDK C++ and public runtime methods/payloads are unchanged.

## Reproduced failures and repairs

- `salamatrix_regression_tests` required the obsolete scalar `DeferredModule`.
  Validate the existing production vector instead: one reference per loader
  acquisition, reserve before callbacks, refusal before teardown on allocation
  failure, retaining prior references across reload, and exact reverse draining
  only after all consumers release. Do not revert the ownership fix.
- `association_icon_cache_tests` extracted `IsAssociated` without its new cache
  dependency. Extract the real cache members, preparation/query methods and
  release-time invalidation. Keep all original assertions; additionally exercise
  prepared positive/negative answers, reuse, invalidation during a Shell query,
  and repeated per-file dynamic EXE icon lookup. Only the OS query is substituted.
- PowerShell inherited legacy console encodings for SMX1 text pipes. Bind direct
  input/output streams to strict BOM-free UTF-8, retain LF framing and explicit
  flushing, and leave the caller console/code page untouched. This restores the
  existing common UTF-8 contract; it adds no public capability. All five language
  providers are tested, including raw Unicode host replies. The regression runs
  PowerShell under inherited encoding and isolated child CP437/CP1252 consoles.
- Clean builds timed out downloading pinned Lua `5.5.0#1` before compilation.
  A standard manifest overlay retains the exact Microsoft vcpkg port at baseline
  `a0b1c8d3a477c1cb4813d8e127a56961707ca42b`, port tree
  `6767a2f27deb2b906d4353bd91c9bd26b30a7cf1`. Its only portfile behavior
  change is an Alpine HTTPS mirror after the official URL. Preserve SHA512,
  features, patches, compiler/install behavior, caches and license extraction.
  The two archives are byte-identical; a mismatched hash is still rejected.
  See `tools/vcpkg/third-party-libs/overlay-ports/lua/README.md` for provenance.

No existing CI/test groups, warnings, assertions, security checks or platform
exclusions are removed. No workflow edit, Windows setting change, new upstream
PR, reference-base merge or additional feature activation is part of this fix.

## Local qualification

Evidence is outside Git at `../evidence/PR64-ci-fixes`; records include source
hashes or the exact binary diff against the baseline above.

| Boundary | Result and evidence |
| --- | --- |
| Salamatrix contract and actual shutdown | PASS; actual ownership methods Debug/Release, refusal/reload/OOM paths, native Unicode/long executable paths; `contracts/validation.json`. |
| Association native fixture | PASS with every original assertion retained and actual production cache bodies; all nine focused contract/native negative controls detect their intended defect. |
| Five real runtime workers | PASS for JavaScript/Python/PowerShell/PHP/Lua with exact options, types, results/errors and Unicode round-trip. Both pwsh and Windows PowerShell pass all three encoding contexts; original-worker negative controls reproduce corruption under both legacy code pages. |
| PowerShell malformed/closed channel | Existing termination behavior retained; invalid UTF-8 rejected before host calls; `powershell/wire-boundary-and-negative-control.json`. |
| Lua downloader | PASS: actual vcpkg fallback after forced origin failure, wrong-SHA512 rejection, overlay selection and exact upstream payload/license comparison; `lua/summary.json`. |
| Lua source builds | PASS x64, Win32 and ARM64, standard post-build checks, binary cache disabled. x64/Win32 interpreter UTF-8/math smoke PASS; ARM64 PE machine checked, not executed on this x64 host. |
| Host and changed PowerShell provider | Debug/Release x64 MSBuild `/warnaserror` PASS; staged worker bytes match source. Initial local Debug incremental language link hit LNK1000; rebuilding that unchanged language project then normal host closure passed. Failed log retained. |
| Existing integration evidence | C++/SDK inputs remain byte-identical to qualified `0991f680`; prior scoped native/GUI evidence is reused for those unchanged boundaries, not relabelled as new clean-platform validation. |

Git preserves the copied port bytes through a local `.gitattributes` rule.
Authored-change whitespace checks pass; the full diff check reports retained
upstream CMake whitespace and patch context lines, which are not reformatted.

The current publication commit still requires fresh hosted PR tests, actual
CodeQL analysis/findings and a clean full supported Win32/x64/ARM64 build.
Results for `6226279a` are historical: PR compiler matrix and actual CodeQL passed,
three test groups failed, and clean-build retry passed ARM64 but failed x64/Win32
at the Lua download. A successful retry must bind the new exact head.

Accepted S31 external validation limits, inherited strict translation-layout
failures, deferred ChangeDir, hidden PAK Optimize and the separately reported
large Find freeze remain explicit. This change does not certify release readiness.
