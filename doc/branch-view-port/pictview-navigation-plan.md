# PictView navigation in Branch View

## User outcome

Open an image from a recursive disk panel in PictView and use its existing Previous and Next commands to move through the eligible images in the source panel's current order. Images with the same basename in different directories remain distinct. The Path column display preference has no effect on which image opens.

The target is Open Samandarin 0.17. The starting commit is `aef7a2dba7f38614a9166cff66662480efc1d5fc`. Use the official `5.0-samandarin-0.16` release as a comparison point.

## What is already in the code

- PictView already queries the wide `Salamander.ViewerEnumeration` service and uses it for Previous, Next and panel selection through `PictViewEnumerateFile` in `src/plugins/pictview/pictview.cpp`.
- `src/plugins/pictview/render1.cpp` handles navigation commands and passes returned names into the normal image open path. `src/salamdr6.cpp` implements the host service.
- Branch View exposes row identities through full paths. The owned DiskSelection service used for batch plug-in operations serves a different purpose from viewer navigation.
- The reported runtime failure is the starting point. Code inspection alone does not establish whether it is an opening-mode issue, a lost source context, an enumeration problem or a decoder path problem.

## Work to complete

1. **Reproduce and isolate.** Use ordinary disk view and Branch View in official 0.16 and the working 0.17 baseline. Explicitly select PictView with View With and repeat through the standard F3 route with a PictView association. Record which viewer opens, whether Previous and Next are enabled, and which images actually load. Verify normal panel navigation as the control.
2. **Trace the first failing handoff.** Follow the full path, source UID and current index from the panel through the plug-in viewer launch, the PictView viewer thread and renderer, the host enumeration call, and the decoder. Identify where the actual path, source context, eligibility or busy/end status diverges. Check both command enablement and execution. Preserve existing order, supported formats and association policy.
3. **Repair only the failing route.** Reuse the existing wide enumeration service. If a PictView-specific change suffices, keep it there. If the host loses the context, repair that handoff and check the Internal Viewer. Preserve the existing SDK ABI and compatibility on older supported hosts. Treat a missing or stale source as unavailable through the existing contract. Keep exact paths through captions, reload and decoder I/O. Preserve ordinary panel, archive and standalone viewer behavior.
4. **Verify and prepare for review.** Register a focused regression test in the existing runner at the failing boundary when practical. Build the affected host, PictView and dependencies in Debug and Release x64 with warnings as errors. Use real GUI proof on owned image fixtures and bind evidence to the tested revisions. After an authorized implementation push, check fork CI for that head and report platform checks and CodeQL accurately, including any skips.

## Acceptance

| Scenario | Expected observable behavior |
|---|---|
| Three or more recognizable images across sibling and nested directories, including identical basenames | Explicit PictView launch opens the selected image. Next followed by Previous returns to the same image. The order matches the displayed panel order among eligible images. Verify by distinct pixels or image dimensions, plus the full path. |
| Non-image between valid images, first and last eligible image | Existing association/format rules and endpoint behavior are preserved. No wrong file opens and there is no loop. |
| Unicode leaf and parents, byte-long UTF-8 path, and Win32-long path | Initial open, navigation in both directions, reload and title retain the exact image identity through the decoder, within existing supported format and OS boundaries. |
| Reordering or refreshing the source, closing the tab, and switching Branch View off while the viewer stays open | Navigation follows the existing host contract or cleanly reports unavailability. It never uses the new row at a stale index or the wrong same-named file. |
| Ordinary panel, Internal Viewer, and applicable standalone or archive entry points | Existing navigation and availability rules are preserved. PictView's selected-image navigation still uses the intended selection wherever the current UI offers it. |

The first implementation covers opening and Previous/Next in PictView. Broader viewer migrations, new SDK interfaces, and changes to Find, scanning or file operations belong in separate work. If the baseline already meets acceptance, retain the verification and document the exact reproduction conditions before changing code.
