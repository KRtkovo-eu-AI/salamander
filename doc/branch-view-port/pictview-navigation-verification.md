<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-20 | Record PictView opening and navigation evidence. | Document the verified host dispatch fix and build limits. |  | gpt-6.2-sol |
-->

# PictView in Branch View

## Diagnosis and scope

The first failure was in host dispatch. The Branch View command gate blocked the explicit View With commands. A separate precheck in `CFilesWindow::ViewFile` rejected explicit handlers and plugin associations before the resolved viewer was launched. Consequently, PictView received neither the selected image nor its source panel.

The host now applies the Branch View policy to the resolved handler and checks the bundled PictView registration before invoking the plugin. Other plugin viewers still cannot launch from Branch View. The normal dispatch rules continue to apply to ordinary panels.

`CFilesWindow::ViewFile` takes the full path from the current Branch View result and passes it with the enumeration source UID and row index. PictView already preserves that context. `PictViewEnumerateFile` uses the wide service in `salamdr6.cpp` to enumerate eligible files from the source. `render1.cpp` uses that same route for navigation availability and opens the returned wide path through the existing decoder. The patch leaves the enumeration service, decoder and public plugin API unchanged.

## Reproduction and results

The comparison used the [official Samandarin 0.16 x64 ZIP](https://github.com/KRtkovo-eu-AI/salamander/releases/download/5.0-samandarin-0.16/5.0-samandarin-0.16_win_x64.zip), SHA-256 `366A6675F2EE363F774968AA78FA37BEA618B356549D8D2B19C7030F5E0DCC52`. An isolated extraction was used for Files > View With > PictView. A separate isolated copy with only PictView registered established the F3 association, which opened PictView in the ordinary panel. In Branch View the explicit choice and associated F3 opened no image. The ordinary panel opened red, advanced to green over the text file and returned to red.

The 0.17 base is `aef7a2dba7f38614a9166cff66662480efc1d5fc`, with planning commit `eea495536ee1cc4c132edf505fc9e770a3b44891` immediately before this implementation. The affected viewer code on that base is identical to the official 0.16 tag. Unmodified 0.17 failed to compile in both x64 configurations (`shellsup.cpp`, undeclared `mi` and wide text passed to ANSI operations), so there is no unmodified 0.17 runtime observation.

The fixture was on local disk with the Branch View sorted by name. Paths below are relative to the fixture root. The long directory components are abbreviated here. File identity was checked in the viewer title using the full path and decoded dimensions.

| Source order | File | Expected and observed in Debug and Release after the fix |
| --- | --- | --- |
| 1 | `a/00-red.bmp` | Red, 81 × 51. Associated F3 opens this exact file. |
| 2 | `a/01-note.txt` | Skipped by PictView in both directions. Associated F3 opens the exact file in Internal Viewer. |
| 3 | `a/same.bmp` | Green, 92 × 52. Previous from the blue image lands here. |
| 4 | `b/same.bmp` | Blue, 103 × 53. Explicit View With opens this exact file. Next from green returns here. |
| 5 | `b/…/zx-long.bmp` | Cyan, 136 × 56. Absolute path of 277 UTF-16 code units and 457 UTF-8 bytes. |
| 6 | `b/…/zy-byte.bmp` | Magenta, 125 × 55. Absolute path of 146 UTF-16 code units and 296 UTF-8 bytes. |
| 7 | `東京/zz-青.bmp` | Yellow, 114 × 54. |

Space and Backspace followed this image order in both directions. Previous at the first image and Next at the last left the current image open. Reload on cyan kept its exact path. Turning off the originating Branch View while the blue image was open left it on blue in both directions, with the green namesake still present in the other panel. In the ordinary `a` panel, F3 and Next/Previous produced red → green → red. The GUI tests did not force a busy request or exercise selected-only navigation. The existing selection and busy code was reused.

## Build and test evidence

- `viewer_enumeration_tests` passed in Debug and Release x64 (11 cases each).
- `branch_panel_getters_tests.py` passed in Debug and Release x64. The focused policy test checks bundled PictView, missing registration, unsupported plugins and the existing built-in/external viewer rules.
- `branch_transition_contract_tests.py` (30 tests), `branch_operations_contract_tests.py` (20 tests), `file_association_model_contract_tests.py` and `pictview_native_decoder_contract_tests.py` passed.
- The PictView project built successfully in both x64 configurations with v145. The candidate hosts compiled and linked for GUI testing after a temporary, local correction to the unrelated Shell menu compilation error. The complete host build command then failed during payload staging because `plugins/7zip/7za.dll` was absent. The tests ran on isolated, manually staged binaries. The Shell correction was removed from the working tree and is outside this commit.

| Tested x64 binary | SHA-256 |
| --- | --- |
| Debug host | `0959BD618609453ED0B94B9C1C9D2C11069D61DACEE96AE996C773D19EC96B14` |
| Debug PictView | `713BF1C443136EF61643CE48C62B94CFFB474C719E6ECAC8FAFE77A7807792D3` |
| Release host | `197B0DA2407DB3D33961FF25F79DCC4D9B999500DC9E70FD1BA98AAB9D1FADE0` |
| Release PictView | `5E04B87102912F85F00D7676CA33D8DAD162EE390B99701453D76F1E9BA31028` |
