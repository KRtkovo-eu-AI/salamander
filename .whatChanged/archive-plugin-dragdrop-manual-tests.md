# Archive / Plugin FS Drag-and-Drop Manual Repro Coverage

Use an Open Salamander build with the shell extension registered. Run each scenario once with the left mouse button and once with the right mouse button where applicable.

1. **Normal filesystem panel**
   - Open a normal disk folder, select one file, and drag it to another disk folder.
   - Expected: drag-and-drop starts normally and the file operation completes or can be cancelled without errors.

2. **ZIP/archive panel with explicit selection**
   - Open a ZIP archive, select one or more listed archive entries, and drag them to a disk folder.
   - Expected: OLE drag starts, the shell-extension handoff completes, and unpack/copy runs after drop.

3. **ZIP/archive panel with no explicit selection but focused item**
   - Clear selection in a ZIP archive, focus a real file or directory entry, and start dragging from that focused row.
   - Expected: the focused item is treated as the one-item drag list and the drag starts normally.

4. **ZIP/archive root with empty ArcPath**
   - Open a ZIP archive at its root so `ArcPath` is empty, focus or select a real entry, and drag it to a disk folder.
   - Expected: the empty archive subpath is accepted and the drag starts normally.

5. **Archive containing directories but no files**
   - Open an archive location that lists directories only, focus or select a real directory, and drag it to a disk folder.
   - Expected: directory-only archive content produces a non-empty drag list and does not crash.

6. **Attempted drag from empty space**
   - Click and drag below the last visible archive/plugin FS item.
   - Expected: item drag is not started; box selection or no-op behavior occurs, and OLE is not entered.

7. **Computed drag item count is zero**
   - Exercise an invalid source state such as no selected items and no valid focused item, then attempt an archive/plugin FS drag.
   - Expected: diagnostic logging reports the invalid source state and the drag aborts before `DoDragDrop`.
