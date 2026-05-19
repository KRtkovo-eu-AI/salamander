#include "pch.h"

BOOL WINAPI CPluginInterfaceForViewer::ViewFile(const char *name, int left, int top, int width, int height,
                                    UINT showCmd, BOOL alwaysOnTop, BOOL returnLock, HANDLE *lock,
                                    BOOL *lockOwner, CSalamanderPluginViewerData *viewerData,
                                    int enumFilesSourceUID, int enumFilesCurrentIndex)
{
	return TRUE;
}