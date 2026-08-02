# Samandarin release source-code growth

These reports compare source-code metrics between consecutive published
Samandarin release tags. They use the same analyzer and Markdown format as the
pull-request source-code growth report.

Tag `5.0-samandarin-0.13` does not exist: version 0.13 was explicitly skipped.
The final report therefore compares the consecutive published tags 0.12 and
0.14.

| Published releases | NLOC | Delta | Files delta | Functions/methods delta | Report |
| --- | ---: | ---: | ---: | ---: | --- |
| 0.1 -> 0.2 | 1,002,824 -> 1,012,814 | +9,990 | +42 | +493 | [Details](5.0-samandarin-0.1-to-0.2.md) |
| 0.2 -> 0.3 | 1,012,814 -> 1,034,386 | +21,572 | +41 | +911 | [Details](5.0-samandarin-0.2-to-0.3.md) |
| 0.3 -> 0.4 | 1,034,386 -> 1,038,107 | +3,721 | +2 | +96 | [Details](5.0-samandarin-0.3-to-0.4.md) |
| 0.4 -> 0.5 | 1,038,107 -> 1,041,542 | +3,435 | +12 | +166 | [Details](5.0-samandarin-0.4-to-0.5.md) |
| 0.5 -> 0.6 | 1,041,542 -> 1,054,121 | +12,579 | +6 | +445 | [Details](5.0-samandarin-0.5-to-0.6.md) |
| 0.6 -> 0.7 | 1,054,121 -> 1,054,404 | +283 | 0 | +12 | [Details](5.0-samandarin-0.6-to-0.7.md) |
| 0.7 -> 0.8 | 1,054,404 -> 1,058,204 | +3,800 | +5 | +135 | [Details](5.0-samandarin-0.7-to-0.8.md) |
| 0.8 -> 0.9 | 1,058,204 -> 1,299,549 | +241,345 | +1,066 | +10,199 | [Details](5.0-samandarin-0.8-to-0.9.md) |
| 0.9 -> 0.10 | 1,299,549 -> 1,299,533 | -16 | 0 | +3 | [Details](5.0-samandarin-0.9-to-0.10.md) |
| 0.10 -> 0.11 | 1,299,533 -> 1,302,330 | +2,797 | +1 | +54 | [Details](5.0-samandarin-0.10-to-0.11.md) |
| 0.11 -> 0.12 | 1,302,330 -> 1,312,499 | +10,169 | +23 | +527 | [Details](5.0-samandarin-0.11-to-0.12.md) |
| 0.12 -> 0.14 | 1,312,499 -> 1,364,680 | +52,181 | +116 | +2,452 | [Details](5.0-samandarin-0.12-to-0.14.md) |

NLOC means non-comment source lines as reported by `lizard`. Each detailed
report lists the 20 largest changes by module, file, scope/class, and function.

To regenerate one or more adjacent comparisons, run:

```powershell
python tools/code_metrics/generate_release_reports.py `
  --repository-root . `
  --output-directory doc/code-growth/releases `
  5.0-samandarin-0.1 5.0-samandarin-0.2 5.0-samandarin-0.3
```
