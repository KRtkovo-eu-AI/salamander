# S31 publication: localized string inserts and six CodeQL alerts

The six current-head CodeQL annotations in PR #63 identify non-constant printf
formats in five host Unpack/Pack diagnostics and ShowParsePathError. They are
fixed at the sinks, without query exclusions, suppressions or reduced checks.

`src/common/localizedtext.h` renders only narrow `%s` inserts and literal `%%`.
It measures capacity with overflow checks and copies values literally, preserving
percent-bearing Unicode paths. Unknown directives, argument-count mismatch and
insufficient storage fail without exposing a partial message. Existing guarded
memcpy remains active. The create-directory confirmation aborts on failure before
asking permission; error paths retain localized Windows diagnostics. Parse-path
messages retain owned arguments (LoadStr rotates storage) and allocation failure
handling. No new file operation or archive deletion is admitted.

Debug and Release x64 host builds passed with warnings as errors. Actual-source
native tests passed in both configurations: localized renderer/confirmation guard
346 checks, parse-path context 44, Pack directory scanner 62 and Unpack controller
4886. All original assertions remain. Tests cover invalid `%n`, null/missing/extra
inserts, capacity/red zones, long Unicode paths and exact percent-bearing text.
All five relevant English resource templates plus eleven translations (60 total)
passed restricted-grammar and exact-render checks; this does not qualify the
complete packaged language GUI. Destination and operation/transition contracts
also passed. Existing operation/lifetime evidence remains scoped to unchanged
behavior; these message paths are qualified by the new tests and current builds.

Evidence outside Git: `evidence/S31-codeql-fixes`, including original failures,
final bounded command records, source hashes and the committed validation record.
Initial fixture compilation/zero-capacity expectations and the initial host
memcpy macro integration failure remain recorded; corrected final runs passed.
Remote current-head PR tests, clean full supported-solution rebuild and CodeQL
(including the findings check, not just successful analysis execution) must rerun.
No SDK, workflow, compiler warning, test expectation, registry or Windows security
policy is weakened. No claim of release readiness or new S31 scope is made.
