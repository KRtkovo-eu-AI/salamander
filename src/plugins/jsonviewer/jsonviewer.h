// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

//****************************************************************************
//
// Copyright (c) 2023 Open Salamander Authors
//
// This is a part of the Open Salamander SDK library.
//
//****************************************************************************

#pragma once

// globalni data
extern HINSTANCE DLLInstance; // handle k SPL-ku - jazykove nezavisle resourcy
extern HINSTANCE HLanguage;   // handle k SLG-cku - jazykove zavisle resourcy

// obecne rozhrani Salamandera - platne od startu az do ukonceni pluginu
extern CSalamanderGeneralAbstract* SalamanderGeneral;

char* LoadStr(int resID);

class CPluginInterfaceForViewer : public CPluginInterfaceForViewerAbstract
{
public:
    virtual BOOL WINAPI ViewFile(const char* name, int left, int top, int width, int height,
                                 UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE* lock,
                                 BOOL* lockOwner, CSalamanderPluginViewerData* viewerData,
                                 int enumFilesSourceUID, int enumFilesCurrentIndex) override;

    virtual BOOL WINAPI CanViewFile(const char* name) override;
};

class CPluginInterface : public CPluginInterfaceAbstract
{
public:
    virtual void WINAPI About(HWND parent) override;

    virtual BOOL WINAPI Release(HWND parent, BOOL force) override;

    virtual void WINAPI LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry) override {}
    virtual void WINAPI SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry) override {}
    virtual void WINAPI Configuration(HWND parent) override {}

    virtual void WINAPI Connect(HWND parent, CSalamanderConnectAbstract* salamander) override;

    virtual void WINAPI ReleasePluginDataInterface(CPluginDataInterfaceAbstract* pluginData) override {}

    virtual CPluginInterfaceForArchiverAbstract* WINAPI GetInterfaceForArchiver() override { return NULL; }
    virtual CPluginInterfaceForViewerAbstract* WINAPI GetInterfaceForViewer() override;
    virtual CPluginInterfaceForMenuExtAbstract* WINAPI GetInterfaceForMenuExt() override { return NULL; }
    virtual CPluginInterfaceForFSAbstract* WINAPI GetInterfaceForFS() override { return NULL; }
    virtual CPluginInterfaceForThumbLoaderAbstract* WINAPI GetInterfaceForThumbLoader() override { return NULL; }

    virtual void WINAPI Event(int event, DWORD param) override {}
    virtual void WINAPI ClearHistory(HWND parent) override {}
    virtual void WINAPI AcceptChangeOnPathNotification(const char* path, BOOL includingSubdirs) override {}

    virtual void WINAPI PasswordManagerEvent(HWND parent, int event) override {}
};

// rozhrani pluginu poskytnute Salamanderovi
extern CPluginInterface PluginInterface;
extern CPluginInterfaceForViewer InterfaceForViewer;
