#include "pch.h"


DWORD WINAPI CPluginInterfaceForMenuExt::GetMenuItemState(int id, DWORD eventMask)
{
  return 0;
}

BOOL WINAPI CPluginInterfaceForMenuExt::ExecuteMenuItem(CSalamanderForOperationsAbstract *salamander,
                                            HWND parent, int id, DWORD eventMask)
{
	switch (id)
	{
		case MENUCMD_DLG:
		{
			//MessageBox(NULL, "ExecuteMenuItem", "Error", MB_OK | MB_ICONERROR);
			break;
		}
	}

  return FALSE;
}

BOOL WINAPI CPluginInterfaceForMenuExt::HelpForMenuItem(HWND parent, int id)
{
  return TRUE;
}
