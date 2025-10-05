﻿// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

struct CSourceFile
{
    char* FullName;        // allocated file name (with full path)
    char* Name;            // pointer within FullName after the last backslash or to the start of FullName
    char* Ext;             // for files: pointer in Name after the first dot from the right (except a dot at
                           // the start of the name) or to the end of Name if there is no extension;
                           // for directories: pointer to the end of Name (directories have no extensions)
    CQuadWord Size;        // file size in bytes
    DWORD Attr;            // file attributes - ORed FILE_ATTRIBUTE_XXX constants
    FILETIME LastWrite;    // time of the last write to the file (UTC-based time)
    unsigned NameLen : 15; // length of the FullName string (strlen(FullName))
    unsigned IsDir : 1;
    // unsigned Delete:	1; // the destructor should call free(FullName);
    unsigned State : 1; // 0 -- file not renamed (error, cancel, undo)
                        // 1 -- successfully renamed

    CSourceFile(const CFileData* fileData, const char* path, int pathLen, BOOL isDir);
    CSourceFile(CSourceFile* orig);
    CSourceFile(CSourceFile* orig, const char* newName);
    CSourceFile(WIN32_FIND_DATA& fd, const char* path, int pathLen);
    ~CSourceFile();
    CSourceFile* SetName(const char* name);
};

enum CChangeCase
{
    ccDontChange,
    ccLower,
    ccUpper,
    ccMixed,
    ccStripDia
};

enum CRenameSpec
{
    rsFileName,
    rsRelativePath,
    rsFullPath
};

struct CRenamerOptions
{
    char NewName[2 * MAX_PATH];
    char SearchFor[2 * MAX_PATH];
    char ReplaceWith[MAX_PATH];
    BOOL CaseSensitive;
    BOOL WholeWords;
    BOOL Global;
    BOOL RegExp;
    BOOL ExcludeExt;
    CChangeCase FileCase;
    CChangeCase ExtCase;
    BOOL IncludePath;
    CRenameSpec Spec;

    CRenamerOptions() { Reset(FALSE); }
    void Reset(BOOL soft);
    BOOL Load(HKEY regKey, CSalamanderRegistryAbstract* registry);
    BOOL Save(HKEY regKey, CSalamanderRegistryAbstract* registry);
};

// ****************************************************************************

extern const char* VarOriginalName;
extern const char* VarDrive;
extern const char* VarPath;
extern const char* VarRelativePath;
extern const char* VarName;
extern const char* VarNamePart;
extern const char* VarExtPart;
extern const char* VarSize;
extern const char* VarTime;
extern const char* VarDate;
extern const char* VarCounter;

class CRenamer;

struct CExecuteNewNameParam
{
    CRenameSpec Spec;
    const CSourceFile* File;
    int Counter;
    int RootLen;
};

extern CVarString::CVariableEntry NewNameVariables[];

enum CRenamerErrorType
{
    retGenericError,
    retNewName,
    retBMSearch,
    retRegExp,
    retReplacePattern
};

// ****************************************************************************

class CRenamer
{
protected:
    char (&Root)[MAX_PATH];
    int& RootLen;

    // information about the last error
    int Error, ErrorPos1, ErrorPos2;
    CRenamerErrorType ErrorType;

    CRenameSpec Spec;
    CVarString NewName;
    CChangeCase FileCase;
    CChangeCase ExtCase;
    BOOL IncludePath;

    BOOL Substitute;
    CSalamanderBMSearchData* BMSearch;
    CRegExpAbstract* RegExp;
    char ReplaceWith[MAX_PATH];
    int ReplaceWithLen;
    BOOL UseRegExp;
    BOOL WholeWords;
    BOOL Global;
    BOOL ExcludeExt;

public:
    CRenamer(char (&root)[MAX_PATH], int& rootLen);
    ~CRenamer();

    BOOL IsGood() { return Error == 0; }
    void GetError(int& error, int& errorPos1, int& errorPos2,
                  CRenamerErrorType& errorType)
    {
        error = Error;
        errorPos1 = ErrorPos1;
        errorPos2 = ErrorPos2;
        errorType = ErrorType;
    }

    BOOL SetOptions(CRenamerOptions* options);

    int Rename(CSourceFile* file, int counter, char* newName,
               char** newPart);

protected:
    int BMSearchForward(const char* string, int length, int offset);
    int BMSubst(const char* source, int len, char* dest, int max);
    BOOL ValidetaReplacePattern(); // must be called before SafeSubst, but only after RegCompile
    int SafeSubst(char* dest, int max, int& pos);
    int RESubst(const char* source, int len, char* dest, int max);
};

void ChangeCase(CChangeCase change, char* dst, const char* src,
                const char* start, const char* end);
