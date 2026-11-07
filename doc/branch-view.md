# Branch View

Branch View presents the files below a disk folder as one panel listing. It is
implemented in the host's disk panel, so a file retains its actual location for
viewing, editing, selection and file operations. It does not create a virtual
folder or move any files.

The [plug-in path contract and consumer audit](branch-view-plugin-paths.md)
explains which operations use the shared host resolver, how archive packing
preserves relative paths, and how supplied plug-ins use the standard owned selection.

## Using the view

- Open a disk folder and press **Ctrl+B**,
  or choose **Branch View** in the panel menu. The feature is unavailable in
  archive and plug-in filesystem panels.
- The panel keeps its current layout, including Icons, Thumbnails and custom
  Detailed templates. In Detailed view, **Path** shows each file's actual parent
  folder and supports sorting and resizing; its width belongs to this tab.
  Paths are absolute by default. In **Options > Configuration > Panels**,
  enable relative Branch View paths to display parents relative to the scan
  root instead. The preference is saved and affects display only; operations
  continue to use complete paths.
  Path is not added to Icons or Thumbnails. Existing custom columns and the
  Separate Extension setting remain available.
- Only files appear, apart from the ordinary `..` navigation row where
  applicable. Two files with the same name remain separate rows, distinguished
  by Path in Detailed view. The first scan adds results progressively while the
  panel remains usable. Later scans keep existing results visible until their
  replacement is ready.
- The directory line distinguishes initial scanning from background refresh,
  and reports completion or folders that could not be read. A background
  refresh or its cancellation reports the retained snapshot count. The tab caption identifies Branch View.
- **Stop Branch View Scan**, or Escape when quick search is not active, stops
  enumeration and keeps the currently visible results. It discards unpublished
  results and pending refresh requests, then pauses automatic refresh until an
  explicit Refresh. Filesystem changes below the root normally request a
  background refresh; they do not interrupt a scan already in progress.
- **Open Containing Folder**, in the file context menu and the main Files menu,
  opens the focused file's real parent and focuses
  that file. Navigating elsewhere leaves Branch View. Ctrl+B returns to the
  ordinary folder listing and restores its sort settings. It preserves the
  current layout, including a layout selected while Branch View was active.
- F3, editing, Enter, rename, copy/move, deletion, clipboard and shell actions
  resolve the chosen row's actual path. Viewer next/previous navigation uses
  the panel's ordering, subject to the viewer's normal masks/association rules.
- File Comparator (**Ctrl+Shift+C**) receives the actual paths of both files,
  including duplicate names in different subfolders, selected-plus-focused
  pairs, and comparisons between panels.
- Batch Rename receives an owned snapshot of every selected file's actual
  path. In **Filename** mode, each renamed file stays in its own parent folder;
  equal names in different folders remain distinct. **Relative path** mode
  continues to use the Branch View root. Undo restores the original full paths.
- F5/F6 initially copy or move selected files into the destination as a flat
  selection. The optional **Keep subfolder paths relative to the Branch View
  root** checkbox preserves their relative directory structure. Ordinary
  conflict handling still applies when destination names collide.
- Focus and selected files survive refresh by exact full path. Internal
  Save/Restore Selection also uses full paths when saved from Branch View.
  Clipboard name lists keep their existing name-only semantics.
- Duplicating a tab, reopening a closed tab, or navigating back through path
  history restores Branch View and its in-memory focus/selection snapshot.
  Restoration is asynchronous: a selected file is restored when its scan
  result arrives. Files that disappeared are not recreated.

## Scope and boundaries

The first version enumerates normal disk folders and network folders. Archives
are listed as files; the scanner never enumerates their contents. Entering an
archive invokes the existing archive view. Plug-in filesystems are outside this
version's recursive listing scope.

Directory symbolic links and junctions (name-surrogate reparse points) are
skipped. This prevents cycles and traversal outside the selected tree. Other
reparse directories, such as ordinary cloud-provider directories, are not
blanket-excluded. File links remain file entries. Hidden/system folder exclusion
follows the panel setting; file filters are applied to the collected files and
do not prune ordinary subfolders based on filename masks.

The file context menu uses the native Shell menu for one file or a selection
from the same physical parent folder. It therefore includes the file-type verbs
and installed Shell extensions available in ordinary folder browsing. For a
selection spanning different folders, or when the Shell cannot provide a menu,
Branch View offers the applicable Salamander commands instead. Third-party
Shell extensions are not offered by this fallback. Open Containing Folder is
available in both menus and acts on the focused file.

The displayed list is a changing filesystem snapshot, not a transaction.
Enumeration continues past inaccessible folders and reports their count. A
cancel request stops further work cooperatively; Windows/network drivers may
finish an outstanding SMB call later. Closing a tab does not wait for that call
or leave the worker with a pointer to the closed panel.

The scan and panel metadata preserve wide Unicode names and extended Win32
paths. A destination application or older plug-in can still impose its own
path/encoding limitations. The existing narrow `CFileData.Name` field remains a
bounded, UTF-8-safe display mirror; code needing identity must use the panel's
wide per-item resolver, not reconstruct `root + Name`.

Saved navigation history persists the Branch flag, Path width and wide focused
path. The complete selection and view snapshot are session state. This feature
does not promise restoration of the active Branch listing and every selection
after application restart.

## Host architecture

`src/branch_view.h` and `.cpp` contain the scanner, per-panel metadata and mode
transitions. No field is added to the plug-in-shared `CFileData` structure, and
`PluginData` is not repurposed for disk paths.

Each scan result owns `WIN32_FIND_DATAW`, its actual parent, and its path relative
to the scan root. The panel maps the stable allocation pointer `CFileData.Name`
to this metadata. Shallow row sorting moves the record but preserves this key.
Full wide paths are selection/focus identities; full UTF-8 paths identify icon,
thumbnail and Explorer-property cache entries. A second cache-key-to-row index
is rebuilt after sorting. Equal basenames therefore do not share cache entries.
Rename remaps the old key to the renamed record and requests a fresh scan.

Consumers use `GetItemFullPathW`, `GetItemDirectoryW`, `GetItemIdentityW`, or
`GetItemRelativePathW`. This boundary is essential: the panel's own `GetPathW()`
continues to identify the Branch root and is not the parent of every row.
Viewer enumeration passes complete paths and validates the source tab and
current item, including after reorder. PictView consumes the new wide viewer
enumeration service; existing plug-in interface slots remain in place.

Disk plug-ins use the standard `CSalamanderDiskSelection` facade in
`spl_diskselection.h`. The host captures full paths, names and metadata atomically
into an immutable owned snapshot; plug-in code is the same for ordinary and
recursive disk panels. Refresh cannot invalidate the captured data. File
Comparator, Batch Rename and the other supplied disk-selection consumers use
this boundary before dialogs or workers start. Existing SDK vtable prefixes and
`CFileData` remain unchanged. See the [SDK contract and consumer audit](branch-view-plugin-paths.md)
for lifetime, error handling, migration and host-version requirements.

The worker uses wide `FindFirstFileExW`/`FindNextFileW` and an explicit stack of
directories. It publishes groups of up to 256 files and also publishes at
folder boundaries. It owns a reference-counted scan state and no window or
panel pointer. Replacing a scan replaces that state, so a late old result
cannot enter the new listing. Cancellation sets the cooperative flag and calls
`CancelSynchronousIo`; it never forcibly terminates the scan thread.

The UI polls at 200 ms. The first scan publishes bounded previews (up to
2,048, then 8,192 entries), followed by the complete result. The progress count
continues to report all discovered files. This avoids repeatedly rebuilding a
large growing listing on the UI thread. When previous results exist, a refresh instead collects its new
snapshot in the background and replaces the visible rows atomically after
completion. Automatic refresh does not republish identical snapshots;
enumeration order and last-access timestamps alone do not count as a change.
An explicit Refresh does rebuild the view even when the filesystem snapshot is
unchanged, so filters, hidden-name settings and properties are reapplied.
Starting an automatic refresh does not invalidate the current snapshot
generation or its pending property jobs.
Publication respects the existing operation/refresh suspension guards and
reuses disk `ReadDirectory` row construction for filtering, icons and metadata.
Applying results does not synchronously reopen the root folder.

Automatic notifications never restart an active scan. After a completed scan,
the next automatic scan waits for a cooldown equal to the last scan's duration,
clamped to 5-30 seconds. Changes are also debounced for one second after the
latest notification, with a five-second maximum from the first pending
notification; the cooldown still takes precedence. An explicit Refresh bypasses
these automatic delays once no scan is active. Stop clears pending work and
pauses automatic refresh until an explicit Refresh. The directory watcher uses
subtree notifications in Branch mode and ordinary single-directory notifications
otherwise.

Before replacing rows, the icon reader is suspended. Old metadata remains
available until the old listing is released, and is destroyed on panel teardown
only after the reader stops. Destruction cancels the worker directly and never
uses the live Stop Scan action: `WM_DESTROY` has already deleted the directory
line and other child controls by the time the panel destructor runs. Custom
Path sorting also suspends the reader and
restores focus by stable item identity. Visible custom Explorer columns load
in the background even when Path determines the row order. Display-only jobs
copy at most 128 missing visible rows per batch and merge their results into the
cache. They only invalidate the affected panel, without recalculating the
layout of every row. Explicit property sorting still processes the full list. A job that only loads
column values does not sort the panel when it completes; an active Explorer
property sort retains its separate sorting behavior. Property jobs capture full
item paths and the Branch generation, so equal basenames cannot share values
and completion from an earlier snapshot is discarded. A property job cannot
start while replacement rows are being applied. Publication explicitly starts
the next job after the new rows are installed, preventing an old row collection
from being captured under the new generation. Changing the requested property
sort supersedes the previous job without falling back to synchronous shell
property reads. Panel teardown disconnects completion delivery and releases
its job reference; it does not wait for a blocked property provider.

Activating a Branch panel resumes icon work by signal and handles deferred real
filesystem notifications through the automatic scheduler. It does not probe
the root or free space synchronously and does not start a scan merely because
the application regained focus. Auxiliary icon, thumbnail and property loading
does not display the directory-line scan indicator; the text reports the actual
scan state, including completion and Stop.

## Supported operations and limits

Branch View is available without an experiment environment variable. Its disk
panel restriction, operation allowlists, exact identity and lifetime checks stay
active. Availability of the view does not enable every plug-in or Shell command.
Unknown plug-in commands and unqualified external operation routes are refused;
archive and plug-in filesystem panels retain their existing separate behavior.

ZIP and 7-Zip packing and direct unpacking use qualified host routes. Removing
an archive after unpacking requires complete extraction and a verified owned list
of source volumes. Masked, skipped, cancelled, damaged or incomplete extraction
preserves the source. A true return from an archive engine alone is insufficient.
ZIP multi-volume/SFX packing to long target paths is explicitly refused; normal
ZIP/7-Zip packing remains available and short advanced routes keep their limits.

Program execution and document opening remain separate. Windows policy and
security checks apply. Shell associations may refuse long paths under default
Windows settings; the localized message explains system long-path support and
its limitations. The application does not change the registry or bypass security.
Enabling system long paths is not a guarantee that every association can open
one. Long-source ChangeDir remains deferred and is not claimed verified here.

Directory links are excluded as described above. Local UNC conversion tests do
not qualify real remote SMB latency, disconnect or cancellation behavior. Complete
per-operation ADS/ACL/hardlink preservation, full Undelete image/physical-disk
backend recovery and all-language runtime packaging still need their explicitly
recorded qualification. PAK Optimize remains hidden. Do not infer certification
of an older third-party binary from SDK prefix compatibility.

## Validation of this port

Source: Open Salamander Authors and Samandarin Contributors, PR #769 at
`47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`; target adaptations and exact checkpoints
are recorded in [the port evidence map](branch-view-port/REUSE.tsv).
Donor measurements and GUI tests are historical evidence of the donor, not test
results for this port. Their performance figures are intentionally not copied.

S31 publication head `773679b57ecd083e8e35bf20b7969144af634f0b` passed 130 native/
source-contract groups and 49 Python tests, current-head CodeQL with no new
alerts, VSIX, and the clean supported-solution Win32/x64/ARM64 Debug matrix
(existing SFTP exclusion retained). Those results qualify that head, not a later
activation patch. Relevant operations also have scoped local Debug/Release
build/native/GUI evidence, including exact duplicate identities, Unicode/long
paths, Pack Copy/Move/Cancel and complete/partial Unpack source preservation.

S32 activation qualification and its exact code/artifact bindings are recorded
in [S32](branch-view-port/steps/S32.md). Tests of the scanner, source contracts,
actual-source native harnesses and GUI smoke cover different layers; one does
not substitute for another. Missing evidence stays NOT_VERIFIED. Neither a green
CI nor this document is a claim of unrestricted release readiness; the retained
[validation limits](branch-view-port/steps/S31-final-validation.md) remain explicit.
