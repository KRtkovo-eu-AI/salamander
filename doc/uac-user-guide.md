<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-09 | Document the native UAC pilot and its limits. | Explain consent, partial results and the scope of acceptance testing. | | GPT-6.1 Sol (Codex) |
-->

# Administrator access in Open Samandarin

The UAC pilot runs the file manager with the account and permissions you normally use. If Windows denies an eligible operation, choose **Administrator** (the shield button) to request permission for the operation shown. Windows may ask for an administrator's credentials. Enter them in the Windows prompt. The signed pilot installer places the application and its matching broker in the protected **Open Samandarin UAC** folder under Program Files. See the [pilot packaging instructions](../src/uacbroker/README.md#signed-x64-pilot-installer).

## What one approval covers

- For an eligible file job, approval covers later eligible items within the same operation and the approved path scope. A new job or wider scope can require another approval. Overwriting an existing file still follows the usual conflict dialog. If the target changes while you are approving the operation, review the new conflict.
- In a protected folder panel, the shield marks an approved listing session. You can navigate within the approved subtree and refresh the listing. Reading file contents or changing files can require separate permission.
- In Find, accessible results can appear while the protected scope is awaiting approval. Approving a listing does not by itself authorize content, keyword or duplicate-content checks. If access is declined, results from accessible locations remain available. Treat results as incomplete when Find reports inaccessible locations or files.
- In the built-in ZIP panel, the approved read session covers navigation, refresh and reading eligible entries, including **F3**. **F5** extraction to a protected destination and edits to the original ZIP require separate approval. Leaving the archive ends its read session.

## Supported paths and operations

| Operation | Pilot scope |
| --- | --- |
| Create, rename and change metadata | **F7** for a single folder, **F2** and Quick Rename, and **Ctrl+F2** for supported attributes and timestamps. Selected link types can be renamed without touching their targets. |
| Copy and move | **F5** and **F6** for eligible local file operations, including guarded replacement where supported. Cross-volume F6 covers individual files between fixed NTFS volumes with a free destination name, up to 4 GiB including named streams. Only supported combinations of streams, sparse allocation, compression and security metadata are offered. |
| Copy a folder tree | The elevated **F5** tree pilot requires an eligible tree and a registered protected destination. It publishes into a free name. |
| EFS | Eligible single-file **F5** uses encrypted backup and restore on NTFS. Opening the copied file still requires the appropriate EFS key. |
| Delete | **Shift+Delete** offers elevation for eligible permanent deletion. This includes supported single symbolic links and junctions and leaves their targets intact. **Delete** continues to use the Windows Recycle Bin under the ordinary account. Protected recycling to the original user's bin is still under test. |
| Read and inspect | Protected panel listings, **Space** and **Ctrl+Q**, supported **Alt+F7** Find queries, and **F3** in the internal viewer. **F4** can explicitly launch an eligible editor as administrator for the original file. |
| ZIP and ZIP64 | Listing in the ordinary archive panel, internal viewing of eligible entries, extraction, and adding, replacing or deleting eligible entries in protected archives. Viewing and extracting are limited to 16 MiB per expanded file in this pilot. Extraction of a selection is also limited to 4,096 entries and a 64 MiB total request. |

The pilot applies to supported local paths and the built-in ZIP integration. Network shares, mapped drives, general plugin elevation, and portable elevation are outside the documented pilot. The [implementation plan](uac-implementation-plan.md) lists the detailed file system and metadata combinations and their verification status.

## Cancellation and interrupted operations

Cancelling the Windows prompt leaves the elevated step unstarted. Changes already completed earlier in the job remain in place. If Samandarin cannot confirm a result, check the source and destination before retrying. A cross-volume move can leave a complete destination and the original source. If a move into the Recycle Bin cannot be confirmed, check both the original location and the Recycle Bin. A failed recycle request never switches automatically to permanent deletion.
