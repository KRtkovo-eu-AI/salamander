#include "precomp.h"

// spolecny interface pro pluginova data archivatoru
CArcPluginDataInterface ArcPluginDataInterface;

// ****************************************************************************


void WINAPI CArcPluginDataInterface::SetupView(BOOL leftPanel, CSalamanderViewAbstract *view, const char *archivePath,
                                   const CFileData *upperDir)
{
}

void WINAPI CArcPluginDataInterface::ColumnFixedWidthShouldChange(BOOL leftPanel, const CColumn *column, int newFixedWidth)
{
}

void WINAPI CArcPluginDataInterface::ColumnWidthWasChanged(BOOL leftPanel, const CColumn *column, int newWidth)
{

}

BOOL WINAPI CPluginInterfaceForArchiver::ListArchive(CSalamanderForOperationsAbstract *salamander, const char *fileName,
                                         CSalamanderDirectoryAbstract *dir,
                                         CPluginDataInterfaceAbstract *&pluginData)
{
  return TRUE;
}

BOOL WINAPI CPluginInterfaceForArchiver::UnpackArchive(CSalamanderForOperationsAbstract *salamander,
                                           const char *fileName, CPluginDataInterfaceAbstract *pluginData,
                                           const char *targetDir, const char *archiveRoot,
                                           SalEnumSelection next, void *nextParam)
{
  return TRUE;
}

BOOL WINAPI CPluginInterfaceForArchiver::UnpackOneFile(CSalamanderForOperationsAbstract *salamander,
                                           const char *fileName, CPluginDataInterfaceAbstract *pluginData,
                                           const char *nameInArchive, const CFileData *fileData,
                                           const char *targetDir, const char *newFileName,
                                           BOOL *renamingNotSupported)
{
  return FALSE;
}

BOOL WINAPI CPluginInterfaceForArchiver::PackToArchive(CSalamanderForOperationsAbstract *salamander,
                                           const char *fileName, const char *archiveRoot,
                                           BOOL move, const char *sourcePath,
                                           SalEnumSelection2 next, void *nextParam)
{
  return TRUE;
}

BOOL WINAPI CPluginInterfaceForArchiver::DeleteFromArchive(CSalamanderForOperationsAbstract *salamander,
                                               const char *fileName, CPluginDataInterfaceAbstract *pluginData,
                                               const char *archiveRoot, SalEnumSelection next, void *nextParam)
{
  return TRUE;
}
BOOL WINAPI CPluginInterfaceForArchiver::UnpackWholeArchive(CSalamanderForOperationsAbstract *salamander,
                                                const char *fileName, const char *mask,
                                                const char *targetDir, BOOL delArchiveWhenDone,
                                                CDynamicString *archiveVolumes)
{
  (void)salamander;
  (void)fileName;
  (void)mask;
  (void)targetDir;
  (void)delArchiveWhenDone;
  (void)archiveVolumes;
  return TRUE;
}

BOOL WINAPI CPluginInterfaceForArchiver::CanCloseArchive(CSalamanderForOperationsAbstract *salamander,
                                             const char *fileName, BOOL force, int panel)
{
	return TRUE;
}

BOOL WINAPI CPluginInterfaceForArchiver::GetCacheInfo(char *tempPath, BOOL *ownDelete, BOOL *cacheCopies)
{
  if (tempPath != NULL)
    tempPath[0] = '\0';
  if (ownDelete != NULL)
    *ownDelete = FALSE;
  if (cacheCopies != NULL)
    *cacheCopies = TRUE;
  return FALSE;
}

void ClearTEMPIfNeeded(HWND parent)
{
}

void WINAPI CPluginInterfaceForArchiver::DeleteTmpCopy(const char *fileName, BOOL firstFile)
{
  (void)fileName;
  (void)firstFile;
}

BOOL WINAPI CPluginInterfaceForArchiver::PrematureDeleteTmpCopy(HWND parent, int copiesCount)
{
        (void)parent;
        (void)copiesCount;
        return TRUE;
}
