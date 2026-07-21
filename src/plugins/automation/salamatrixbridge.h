// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Automation Plugin for Open Salamander

    salamatrixbridge.h
    Thin Automation-side consumer bridge for the Salamatrix runtime plugin.
*/

#pragma once

#include "../salamatrix/salamatrix_automation.h"

/// Caches host-registered Salamatrix services for the Automation plugin.
///
/// The bridge is intentionally consumer-only: it never creates a fallback
/// Salamatrix runtime and does not duplicate UI/command implementations. If the
/// Salamatrix runtime plugin is installed and loaded, Automation can use the
/// host service registry; otherwise callers can keep using legacy Automation
/// objects and future script wrappers can report a clear missing-runtime error.
class CAutomationSalamatrixBridge
{
private:
    bool m_bQueried;
    Salamatrix::Automation::ScriptRootAdapter* m_pScriptRoot;
    Salamatrix::UI::IUIService* m_pUIService;
    Salamatrix::Commands::ICommandService* m_pCommandService;
    Salamatrix::FileOperations::IFileOperationsService* m_pFileOperationsService;
    DWORD m_dwAutomationVersion;
    DWORD m_dwUIVersion;
    DWORD m_dwCommandsVersion;
    DWORD m_dwFileOperationsVersion;

    static void* QueryService(
        CSalamanderGeneralAbstract* salamander,
        const char* serviceName,
        DWORD minVersion,
        DWORD* actualVersion);

public:
    CAutomationSalamatrixBridge();

    void Reset();
    void Refresh(CSalamanderGeneralAbstract* salamander);

    bool WasQueried() const { return m_bQueried; }
    bool IsAvailable() const { return m_pScriptRoot != NULL; }
    bool HasUI() const { return m_pUIService != NULL; }
    bool HasCommands() const { return m_pCommandService != NULL; }
    bool HasFileOperations() const { return m_pFileOperationsService != NULL; }

    Salamatrix::Automation::ScriptRootAdapter* GetScriptRoot() const { return m_pScriptRoot; }
    Salamatrix::UI::IUIService* GetUIService() const { return m_pUIService; }
    Salamatrix::Commands::ICommandService* GetCommandService() const { return m_pCommandService; }
    Salamatrix::FileOperations::IFileOperationsService* GetFileOperationsService() const { return m_pFileOperationsService; }

    void GetStatusText(PTSTR buffer, int cchBuffer) const;
};
