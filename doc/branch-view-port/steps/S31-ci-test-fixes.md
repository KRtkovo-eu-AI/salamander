# S31 PR #63 — portable history and shortcut fixtures

The current-head CI failed two generated native test groups. These are fixture
environment assumptions, not evidence that weakening production path handling or
Windows security policy is necessary. No production function or security control
changes in this fix.

## Copy/Move history

The original test encoded Czech text through CP_ACP and expected the original
Unicode text on every machine. CP1252 cannot encode all those characters without
best-fit/replacement, so that expectation cannot hold on an English CI runner.
The history decoder is supposed to decode the actual stored legacy bytes.

The fixture now runs the same actual dialog Transfer and real Unicode HWNDs with
native ACP, CP1250, CP1252 and UTF8. Only CP_ACP selection is process-local; real
Windows conversion APIs perform the conversions, and no machine locale changes.
CP1250 keeps the original Czech text. CP1252 uses cafe with an accent; UTF8 uses
Japanese/emoji. Independent strict encoding/roundtrip checks reject default or
best-fit replacement and ensure distinct ACP bytes exercise the legacy fallback.
The original history check still demands the exact expected Unicode text. All
other original assertions remain unchanged, including caller red zones, complete
long paths, atomic refusal and no history/options mutation after invalid input.
Debug/Release: all four ACP modes passed, 56 checks each.

## Branch shortcut target

Windows CI TEMP can include RUNNER~1. Production legitimately expands filesystem
aliases; comparing its complete result to an unexpanded fixture parent fails.
The test now resolves the temporary parent before creating any test files and
independently checks matching volume/file identity for that directory. It does
not normalize an observed production result or loosen expected path comparisons.
All original shortcut assertion expressions remain byte-for-byte equivalent after
whitespace normalization. Exact directory/name focus, long paths, actual Shell
links, actual junctions, stale callbacks and failure/security flags are retained.

Each Debug/Release run covers the ordinary parent, an owned parent junction and
GetShortPathName of an owned long-spelled parent. No 8.3 or security setting is
changed. When Windows does not generate an alternate short name, its returned
spelling still resolves to the same owned directory; the explicit junction run
always exercises a distinct alias. These cases passed with 95/96/97 checks.

## Proof against weakened checks

Isolated generated negative controls force the real dialog to decode ACP bytes
as UTF8, or retain the short fixture parent. They fail the original exact history
or exact destination/focus assertions in Debug and Release. They never replace
the positive test or modify production sources. No test is skipped, no expected
failure is accepted in CI, and compiler /W4 /WX remains enabled.

Evidence outside Git: `evidence/S31-ci-test-fixes` contains final native records,
original templates, assertion-expression comparison, four expected negative
failures and source hashes. A generated literal compile error and an over-specific
negative-probe expectation remain recorded separately; corrected final probes
passed. Existing required CI/CodeQL and the clean full supported-solution build
must run for the new commit; earlier head results do not qualify this fix.
