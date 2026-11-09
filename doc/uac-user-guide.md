<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-09 | Describe F5 recovery and the panel size command. | Explain the fresh overwrite decision and the existing shortcut for folder sizes. | | GPT-6.1 Sol (Codex) |
| 2026-11-09 | Explain the source restriction for elevated security copies. | Document the protected source requirement verified in the VM. | | GPT-6.1 Sol (Codex) |
| 2026-11-09 | Document the same-account Shell recycle boundary. | The Windows credential path can use a different account’s bin. | | GPT-6.1 Sol (Codex) |
| 2026-11-09 | Update protected Recycle Bin acceptance after the Shell retry fix. | Record the verified original-account result and remaining restore checks. | | GPT-6.1 Sol (Codex) |
| 2026-11-09 | Record protected Recycle Bin acceptance status. | Distinguish the verified cancellation from the failed approved retry. | | GPT-6.1 Sol (Codex) |
| 2026-11-09 | Explain progress during elevated transfers. | Distinguish in-flight bytes from a confirmed result. | | GPT-6.1 Sol (Codex) |
| 2026-11-09 | Remove fixed F6 size and time limits from the pilot. | Describe the streamed move and its remaining eligibility rules. | | GPT-6.1 Sol (Codex) |
| 2026-11-09 | Describe the protected Recycle Bin pilot. | Distinguish a verified single-file recycle from permanent deletion. | | GPT-6.1 Sol (Codex) |
| 2026-11-09 | Describe the protected VM staging path. | Remove the custom installer from the UAC scope. | | GPT-6.1 Sol (Codex) |
| 2026-11-09 | Document the native UAC pilot and its limits. | Explain consent, partial results and the scope of acceptance testing. | | GPT-6.1 Sol (Codex) |
-->

# Administrator access in Open Samandarin

The UAC pilot runs the file manager with the account and permissions you normally use. If Windows denies an eligible operation, choose **Administrator** (the shield button) to request permission for the operation shown. Windows may ask for an administrator's credentials. Enter them in the Windows prompt. The VM pilot uses matching application and broker binaries staged in the protected **Open Samandarin UAC** folder under Program Files. See the [pilot staging instructions](../src/uacbroker/README.md#stage-the-matching-x64-binaries).

## What one approval covers

- For an eligible file job, approval covers later eligible items within the same operation and the approved path scope. A new job or wider scope can require another approval. Overwriting an existing file still follows the usual conflict dialog. If the target changes while you are approving the operation, review the new conflict.
- In a protected folder panel, the shield marks an approved listing session. You can navigate within the approved subtree and refresh the listing. Reading file contents or changing files can require separate permission.
- In Find, accessible results can appear while the protected scope is awaiting approval. Approving a listing does not by itself authorize content, keyword or duplicate-content checks. If access is declined, results from accessible locations remain available. Treat results as incomplete when Find reports inaccessible locations or files.
- In the built-in ZIP panel, the approved read session covers navigation, refresh and reading eligible entries, including **F3**. **F5** extraction to a protected destination and edits to the original ZIP require separate approval. Leaving the archive ends its read session.

## Supported paths and operations

| Operation | Pilot scope |
| --- | --- |
| Create, rename and change metadata | **F7** for a single folder, **F2** and Quick Rename, and **Ctrl+F2** for supported attributes and timestamps. Selected link types can be renamed without touching their targets. |
| Copy and move | **F5** and **F6** for eligible local file operations, including guarded replacement where supported. Cross-volume F6 covers individual files between fixed NTFS volumes with a free destination name. The transfer streams the file and its named streams, with no pilot byte cap or total transfer deadline. Only supported combinations of streams, sparse allocation, compression and security metadata are offered. Preserving explicit NTFS permissions through elevation requires a protected source file with an explicit DACL and no ordinary-user write access. |
| Copy a folder tree | The elevated **F5** tree pilot requires an eligible tree and a registered protected destination. It publishes into a free name. |
| EFS | Eligible single-file **F5** uses encrypted backup and restore on NTFS. Opening the copied file still requires the appropriate EFS key. |
| Delete | **Shift+Delete** offers elevation for eligible permanent deletion. This includes supported single symbolic links and junctions and leaves their targets intact. **Delete** continues to use the Windows Recycle Bin under the ordinary account. The protected pilot can offer a separate administrator retry for one eligible file after access is denied. A confirmed result requires the same file in the original user's bin. The protected retry is available for split-token administrators when Windows uses same-account elevation. Windows may show more than one shield prompt. Confirm only a move to the Recycle Bin. The VM verified cancellation, a same-account move and restoration to the protected location. A retry that would require a different administrator stays at the access-denied result. |
| Read and inspect | Protected panel listings, size calculation with **Space**, **Ctrl+Q** or **Ctrl+Shift+F10** in a disk panel, supported **Alt+F7** Find queries, and **F3** in the internal viewer. **F4** can explicitly launch an eligible editor as administrator for the original file. |
| ZIP and ZIP64 | Listing in the ordinary archive panel, internal viewing of eligible entries, extraction, and adding, replacing or deleting eligible entries in protected archives. Viewing and extracting are limited to 16 MiB per expanded file in this pilot. Extraction of a selection is also limited to 4,096 entries and a 64 MiB total request. |

The pilot applies to supported local paths and the built-in ZIP integration. Network shares, mapped drives, general plugin elevation, and portable elevation are outside the documented pilot. The [implementation plan](uac-implementation-plan.md) lists the detailed file system and metadata combinations and their verification status.

## Progress during elevated transfers

The normal Copy or Move window shows progress while the broker copies eligible files or a folder tree. A cross-volume move can still be checking the copy or removing the source after the data has been transferred. The operation reaches completion when the broker confirms the result.

For EFS, percentages appear during import of the encrypted data. The initial export has no known total size. For a move where ordinary copying succeeded and only source removal needs administrator access, the progress shown during copying comes from the normal worker.

## Cancellation and interrupted operations

Cancelling the Windows prompt leaves the elevated step unstarted. Changes already completed earlier in the job remain in place. If Samandarin cannot confirm a result, check the source and destination before retrying. A cross-volume move can leave a complete destination and the original source. If a move into the Recycle Bin cannot be confirmed, check both the original location and the Recycle Bin. A failed recycle request never switches automatically to permanent deletion.

If a protected **F5** overwrite is interrupted while the original is held in a recovery backup, retrying the same copy can restore the original. Review the normal overwrite dialog before proceeding. Choose **Cancel** to keep the restored file. If another operation is still using the destination and Samandarin reports a sharing violation, wait for that operation to finish before retrying.
