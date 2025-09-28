# Summary of Findings

- **Tab container integration** – the main window now maintains per-side tab arrays and associated UI handles, while each `CFilesWindow` records its side and optional tab customisations so legacy logic still resolves panel orientation without relying on the UI state.【F:src/mainwnd.h†L386-L408】【F:src/fileswnd.h†L764-L777】【F:src/fileswn1.cpp†L1312-L1367】
- **Lifecycle consolidation** – all panel creation and activation paths flow through `AddPanelTab`/`SwitchPanelTab`, which update active panel pointers, history, change notifications, and refresh queues even when only the default panel exists.【F:src/mainwnd3.cpp†L91-L207】
- **Configuration safeguards** – registry loading collapses extra entries when tabs are disabled, recreates missing panels with rollback on failure, and runtime toggles prune surplus tabs while refreshing command states to keep the classic UI pristine.【F:src/mainwnd2.cpp†L2667-L2768】【F:src/mainwnd3.cpp†L1528-L1559】
- **System interactions hardened** – directory watcher management now reuses normalised handles across tabs, and plug-in unload checks iterate hidden panels to stop providers from disappearing underneath inactive tabs.【F:src/snooper.cpp†L960-L1039】【F:src/mainwnd4.cpp†L361-L414】

These changes centralise panel management while guarding every tab-specific action behind `Configuration.UsePanelTabs`, so disabling the feature restores the original single-panel experience without behavioural regressions.【F:src/mainwnd3.cpp†L482-L1202】【F:src/mainwnd3.cpp†L7174-L7188】
