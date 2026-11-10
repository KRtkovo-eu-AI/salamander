---
name: salamatrix-extension-authoring
description: Create or extend Open Samandarin Salamatrix script packages, including Extension Bar/menu commands, file Viewers, virtual file-system panels, and combinations of these roles.
---

# Salamatrix Extension Authoring

Use this skill when creating or changing a manifest-backed Salamatrix extension package in the Open Samandarin repository. First identify the requested runtime and host role(s), then inspect the current repository contract and the closest working package before editing.

## Workflow

1. Read the repository `AGENTS.md` and relevant Salamatrix docs. Treat source and tests as authoritative when documentation and implementation differ. Begin with the source map in [references/project-map.md](references/project-map.md).
2. Choose the nearest actual sample for the requested role and runtime. For new packages prefer Salamatrix Studio's canonical schema-2 scaffold where it fits. Preserve existing project structure and fields; do not replace an existing entry point wholesale to add generated behavior.
   For a package combining several advanced roles (dynamic commands, persisted state, long-running refresh, or custom windows), also consult [references/combined-example.md](references/combined-example.md) if the installed TabuleSprZel sample is available.
3. Declare only the roles the package needs. A menu command, a Viewer association, and a `salamatrix:` file-system provider are separate contributions and may coexist in one schema-2 manifest. Read [references/extension-roles.md](references/extension-roles.md) for implementation patterns and lifecycle details.
4. Keep user-facing strings localizable. Declare locale files and route labels through the established localization contract. Keep manifest handler names aligned with the runtime entry point and the framework's invocation object.
5. Request the narrowest capabilities and security disclosures that match the implementation. Never add network, process execution, or elevation just for convenience. Make destructive or external effects visible and bounded.
6. Keep runtime behavior aligned with the shared Salamatrix contract. The five modern workers (JavaScript, Python, PowerShell, PHP, Lua) share public operations and semantics; inspect their facades/docs before relying on a method. Follow `AGENTS.md`'s parity rule if changing the runtime surface.
7. Validate the manifest and package assets using the repository's existing tests or checks relevant to the edited surface. For user-visible behavior, state what was verified and what needs a running Windows Salamander host if one is unavailable.

Do not treat a documentation example as proof that a role or field is implemented. Check the parser, dispatcher, and a working sample when adding unfamiliar fields. Avoid introducing a second UI, command, filesystem, or viewer backend when the shared Salamatrix host service already provides that function.


