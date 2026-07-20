// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Automation Plugin for Open Salamander

    salamatrixbridge.cpp
    Thin Automation-side consumer bridge for the Salamatrix runtime plugin.
*/

#include "precomp.h"
#include "salamatrixbridge.h"

CAutomationSalamatrixBridge::CAutomationSalamatrixBridge()
{
    Reset();
}

void CAutomationSalamatrixBridge::Reset()
{
    m_bQueried = false;
    m_pScriptRoot = NULL;
    m_pUIService = NULL;
    m_pCommandService = NULL;
    m_pFileOperationsService = NULL;
    m_dwAutomationVersion = 0;
    m_dwUIVersion = 0;
    m_dwCommandsVersion = 0;
    m_dwFileOperationsVersion = 0;
}

void* CAutomationSalamatrixBridge::QueryService(
    CSalamanderGeneralAbstract* salamander,
    const char* serviceName,
    DWORD minVersion,
    DWORD* actualVersion)
{
    typedef BOOL(WINAPI* FSalamanderQueryService)(const char* serviceId, DWORD minimumVersion, void** serviceInterface, DWORD* providedVersion, const char** providerName);

    if (actualVersion != NULL)
        *actualVersion = 0;

    if (salamander == NULL || serviceName == NULL)
        return NULL;

    HMODULE host = GetModuleHandle(NULL);
    if (host == NULL)
        return NULL;

    FSalamanderQueryService queryService =
        (FSalamanderQueryService)GetProcAddress(host, "SalamanderQueryService");
    if (queryService == NULL)
        return NULL;

    void* serviceInterface = NULL;
    DWORD providedVersion = 0;
    if (!queryService(serviceName, minVersion, &serviceInterface, &providedVersion, NULL))
        return NULL;

    if (actualVersion != NULL)
        *actualVersion = providedVersion;

    return serviceInterface;
}

void CAutomationSalamatrixBridge::Refresh(CSalamanderGeneralAbstract* salamander)
{
    Reset();
    m_bQueried = true;

    m_pScriptRoot = static_cast<Salamatrix::Automation::ScriptRootAdapter*>(
        QueryService(salamander, SALAMATRIX_SERVICE_AUTOMATION_ADAPTER,
                     SALAMATRIX_AUTOMATION_VERSION_1_0, &m_dwAutomationVersion));

    m_pUIService = static_cast<Salamatrix::UI::IUIService*>(
        QueryService(salamander, SALAMATRIX_SERVICE_UI,
                     SALAMATRIX_UI_VERSION_1_0, &m_dwUIVersion));

    m_pCommandService = static_cast<Salamatrix::Commands::ICommandService*>(
        QueryService(salamander, SALAMATRIX_SERVICE_COMMANDS,
                     SALAMATRIX_COMMANDS_VERSION_1_0, &m_dwCommandsVersion));

    m_pFileOperationsService = static_cast<Salamatrix::FileOperations::IFileOperationsService*>(
        QueryService(salamander, SALAMATRIX_SERVICE_FILEOPERATIONS,
                     SALAMATRIX_FILEOPERATIONS_VERSION_1_0, &m_dwFileOperationsVersion));
}

void CAutomationSalamatrixBridge::GetStatusText(PTSTR buffer, int cchBuffer) const
{
    if (buffer == NULL || cchBuffer <= 0)
    {
        return;
    }

    if (!m_bQueried)
    {
        StringCchCopy(buffer, cchBuffer, TEXT("not queried yet"));
        return;
    }

    if (!IsAvailable())
    {
        StringCchCopy(buffer, cchBuffer, TEXT("not available (install/load Salamatrix Framework)"));
        return;
    }

    StringCchPrintf(buffer, cchBuffer,
                    TEXT("available (UI: %s, Commands: %s, FileOperations: %s)"),
                    HasUI() ? TEXT("yes") : TEXT("no"),
                    HasCommands() ? TEXT("yes") : TEXT("no"),
                    HasFileOperations() ? TEXT("yes") : TEXT("no"));
}
