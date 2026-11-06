// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-05 | Composite SVG thumbnails with premul-correct blending. | NanoSVG leaves premultiplied BGRA; multiplying by alpha again darkened edges. | | Composer |

#include "precomp.h"
#include "../shared/spl_diskselection.h"

#include "lib/pvw32dll.h"
#include "pictview.h"
#include "exif/exif.h"
#include "pictview.rh"
#include "pictview.rh2"
#include "lang/lang.rh"
#include "native/NativeDecoder.h"

// Salamander-proprietary flag for super-fast low-quality JPEG-decompression

#define PVSF_SUPERFAST 0x8000000

#define FL_SKIP_ALL 1
#define FL_SKIP 2
#define FL_OVERWRITE_RO_ALL 4
#define FL_OVERWRITE_RO 8
#define FL_CANCEL 0x10
#define FL_JFXX 0x20
#define FL_JFXX_ALL 0x40
#define FL_JFXX_SKIP_ALL 0x80
#define FL_NON_JPEG_ALL 0x100
#define FL_NON_IMG_SKIP_ALL 0x200
#define FL_KEEP (FL_SKIP_ALL | FL_OVERWRITE_RO_ALL | FL_JFXX_ALL | FL_JFXX_SKIP_ALL | FL_NON_JPEG_ALL | FL_NON_IMG_SKIP_ALL)

#define NANOSVG_IMPLEMENTATION
#include "../../common/dep/nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../../common/dep/nanosvg/nanosvgrast.h"

static BOOL IsSVGThumbnailFile(LPCTSTR filename)
{
    const TCHAR* ext = _tcsrchr(filename, _T('.'));
    return ext != NULL && _tcsicmp(ext, _T(".svg")) == 0;
}

static BOOL IsSTLThumbnailFile(LPCTSTR filename)
{
    const TCHAR* ext = _tcsrchr(filename, _T('.'));
    return ext != NULL && _tcsicmp(ext, _T(".stl")) == 0;
}

static char* ReadWholeFileForSVG(LPCTSTR filename)
{
    char* buffer = NULL;
    HANDLE hFile = HANDLES_Q(CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                        FILE_FLAG_SEQUENTIAL_SCAN, NULL));
    if (hFile != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER size;
        if (GetFileSizeEx(hFile, &size) && size.QuadPart > 0 && size.QuadPart <= 32 * 1024 * 1024)
        {
            buffer = (char*)malloc((size_t)size.QuadPart + 1);
            if (buffer != NULL)
            {
                DWORD read = 0;
                if (ReadFile(hFile, buffer, (DWORD)size.QuadPart, &read, NULL) && read == (DWORD)size.QuadPart)
                {
                    buffer[size.QuadPart] = 0;
                }
                else
                {
                    free(buffer);
                    buffer = NULL;
                }
            }
        }
        HANDLES(CloseHandle(hFile));
    }
    return buffer;
}

static BOOL LoadSVGThumbnail(LPCTSTR filename, int thumbWidth, int thumbHeight,
                             CSalamanderThumbnailMakerAbstract* thumbMaker)
{
    if (thumbMaker == NULL || thumbWidth <= 0 || thumbHeight <= 0)
        return FALSE;

    char* svg = ReadWholeFileForSVG(filename);
    if (svg == NULL)
        return FALSE;

    BOOL ret = FALSE;
    NSVGimage* image = nsvgParse(svg, "px", 96.0f);
    if (image != NULL && image->width > 0.0f && image->height > 0.0f)
    {
        int targetWidth = thumbWidth;
        int targetHeight = thumbHeight;
        const float scale = min((float)thumbWidth / image->width, (float)thumbHeight / image->height);
        targetWidth = max(1, (int)(image->width * scale + 0.5f));
        targetHeight = max(1, (int)(image->height * scale + 0.5f));

        if (thumbMaker->SetParameters(targetWidth, targetHeight, 0))
        {
            BYTE* pixels = (BYTE*)malloc((size_t)targetWidth * targetHeight * 4);
            NSVGrasterizer* rast = nsvgCreateRasterizer();
            if (pixels != NULL && rast != NULL)
            {
                memset(pixels, 0, (size_t)targetWidth * targetHeight * 4);
                nsvgRasterize(rast, image, 0.0f, 0.0f, scale, pixels, targetWidth, targetHeight, targetWidth * 4);

                // nsvgRasterize emits premultiplied BGRA (see nanosvgrast.h patch).
                // Flatten onto the panel background with premul-correct blending;
                // the thumbnail maker wants opaque RGB (alpha cleared).
                const BYTE bgR = GetRValue(G.rgbPanelBackground);
                const BYTE bgG = GetGValue(G.rgbPanelBackground);
                const BYTE bgB = GetBValue(G.rgbPanelBackground);
                for (int i = 0; i < targetWidth * targetHeight; i++)
                {
                    BYTE* p = pixels + (size_t)i * 4;
                    const unsigned alpha = p[3];
                    const unsigned inv = 255 - alpha;
                    p[0] = (BYTE)min(255u, (unsigned)p[0] + (bgB * inv + 127) / 255);
                    p[1] = (BYTE)min(255u, (unsigned)p[1] + (bgG * inv + 127) / 255);
                    p[2] = (BYTE)min(255u, (unsigned)p[2] + (bgR * inv + 127) / 255);
                    p[3] = 0;
                }

                int processedRows = 0;
                while (processedRows < targetHeight)
                {
                    if (thumbMaker->GetCancelProcessing())
                        break;
                    const int rows = min(32, targetHeight - processedRows);
                    if (!thumbMaker->ProcessBuffer(pixels + (size_t)processedRows * targetWidth * 4, rows))
                        break;
                    processedRows += rows;
                }
                ret = TRUE;
            }
            else
            {
                thumbMaker->SetError();
            }
            if (rast != NULL)
                nsvgDeleteRasterizer(rast);
            free(pixels);
        }
    }
    if (image != NULL)
        nsvgDelete(image);
    free(svg);
    return ret;
}

static bool StlThumbCancel(void* ctx)
{
    CSalamanderThumbnailMakerAbstract* maker = static_cast<CSalamanderThumbnailMakerAbstract*>(ctx);
    return maker != NULL && maker->GetCancelProcessing();
}

static BOOL LoadSTLThumbnail(LPCTSTR filename, int thumbWidth, int thumbHeight,
                             CSalamanderThumbnailMakerAbstract* thumbMaker)
{
    if (thumbMaker == NULL || thumbWidth <= 0 || thumbHeight <= 0)
        return FALSE;

#ifdef _UNICODE
    std::wstring path = PluginPathAddExtendedPrefixW(filename);
#else
    std::wstring wide = PluginMultiByteToWidePath(filename, CP_UTF8);
    if (wide.empty())
        wide = PluginMultiByteToWidePath(filename, CP_ACP);
    if (wide.empty())
        return FALSE;
    std::wstring path = PluginPathAddExtendedPrefixW(wide.c_str());
#endif

    PictView::Native::Frame frame;
    const PictView::Native::Status st = PictView::Native::RasterizeStlFile(
        path.c_str(), static_cast<UINT>(thumbWidth), static_cast<UINT>(thumbHeight), RGB(0xFF, 0xC9, 0x24), frame,
        StlThumbCancel, thumbMaker);
    if (st != PictView::Native::Status::Ok || frame.width == 0 || frame.height == 0 || frame.bgra.empty())
        return FALSE;
    if (thumbMaker->GetCancelProcessing())
        return FALSE;
    if (!thumbMaker->SetParameters((int)frame.width, (int)frame.height, 0))
        return FALSE;

    const BYTE bgR = GetRValue(G.rgbPanelBackground);
    const BYTE bgG = GetGValue(G.rgbPanelBackground);
    const BYTE bgB = GetBValue(G.rgbPanelBackground);
    BYTE* pixels = frame.bgra.data();
    const int count = (int)frame.width * (int)frame.height;
    for (int i = 0; i < count; i++)
    {
        BYTE* p = pixels + (size_t)i * 4;
        const BYTE alpha = p[3];
        p[0] = (BYTE)((p[0] * alpha + bgB * (255 - alpha) + 127) / 255);
        p[1] = (BYTE)((p[1] * alpha + bgG * (255 - alpha) + 127) / 255);
        p[2] = (BYTE)((p[2] * alpha + bgR * (255 - alpha) + 127) / 255);
        p[3] = 0;
    }

    int processedRows = 0;
    const int targetHeight = (int)frame.height;
    const int targetWidth = (int)frame.width;
    BOOL ret = TRUE;
    while (processedRows < targetHeight)
    {
        if (thumbMaker->GetCancelProcessing())
        {
            ret = FALSE;
            break;
        }
        const int rows = min(32, targetHeight - processedRows);
        // ProcessBuffer returns FALSE when the image is complete (or cancelled).
        if (!thumbMaker->ProcessBuffer(pixels + (size_t)processedRows * targetWidth * 4, rows))
            break;
        processedRows += rows;
    }
    return ret;
}

class CEnumFiles
{
    CSalamanderDiskSelection Selection;
    int Index;
    BOOL Valid;
public:
    CEnumFiles() : Index(0), Valid(Selection.Capture(SalamanderGeneral, PANEL_SOURCE)) {}
    BOOL IsValid() const { return Valid; }
    const CSalamanderDiskSelectionItem* GetFile()
    {
        while (Index < Selection.GetCount())
        {
            const CSalamanderDiskSelectionItem* item = Selection.GetItem(Index++);
            if (!item->IsDir) return item;
        }
        return NULL;
    }
    int GetFileCount() const
    {
        int count = 0;
        for (int i = 0; i < Selection.GetCount(); ++i) if (!Selection.GetItem(i)->IsDir) ++count;
        return count;
    }
};

// The temporary file is reserved in the source directory with CREATE_NEW and
// remains owned until an atomic replace succeeds. A failed rename never deletes
// the original image; the destructor removes only this operation's temporary.
class CThumbnailTempFile
{
public:
    std::wstring Path;
    ~CThumbnailTempFile() { if (!Path.empty()) DeleteFileW(Path.c_str()); }
    BOOL Create(const wchar_t* directory)
    {
        GUID id;
        wchar_t token[40];
        for (int attempt = 0; attempt < 8; ++attempt)
        {
            if (FAILED(CoCreateGuid(&id)) || !StringFromGUID2(id, token, _countof(token))) return FALSE;
            std::wstring candidate = PluginPathAddExtendedPrefixW(directory);
            if (!candidate.empty() && candidate.back() != L'\\') candidate += L'\\';
            candidate += L".pv-"; candidate += token; candidate += L".tmp";
            HANDLE file = HANDLES_Q(CreateFileW(candidate.c_str(), GENERIC_WRITE, 0, NULL,
                CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL));
            if (file != INVALID_HANDLE_VALUE)
            {
                HANDLES(CloseHandle(file));
                Path.swap(candidate);
                return TRUE;
            }
            if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS) return FALSE;
        }
        return FALSE;
    }
};

// Restore the original read-only state if a confirmed overwrite subsequently
// fails or is cancelled. The referenced operation path outlives this scope.
class CThumbnailFileAttributes
{
    const wchar_t* Path;
    DWORD Original = 0;
    bool Restore = false;
public:
    explicit CThumbnailFileAttributes(const wchar_t* path) : Path(path) {}
    ~CThumbnailFileAttributes() { if (Restore) SetFileAttributesW(Path, Original); }
    BOOL ClearReadOnly(DWORD attributes)
    {
        if (!SetFileAttributesW(Path, attributes & ~FILE_ATTRIBUTE_READONLY)) return FALSE;
        if (!Restore) Original = attributes;
        Restore = true;
        return TRUE;
    }
    void Commit() { Restore = false; }
};

// Expand only the existing typed localization placeholders. Values are data,
// never format strings, and long Unicode names are not truncated into buffers.
static std::string FormatThumbnailText(const char* pattern, const char* text, int first = 0, int second = 0)
{
    std::string output;
    int number = 0;
    for (const char* p = pattern; p != NULL && *p != 0;)
    {
        if (p[0] == '%' && p[1] == '%') { output += '%'; p += 2; }
        else if (p[0] == '%' && p[1] == 'd' && number < 2) { output += std::to_string(number++ == 0 ? first : second); p += 2; }
        else if (p[0] == '%' && p[1] == 's') { if (text != NULL) output += text; p += 2; }
        else if (p[0] == '%' && p[1] == 'h' && p[2] == 's') { if (text != NULL) output += text; p += 3; }
        else output += *p++;
    }
    return output;
}

/* MyMemWriteFunc - used for creating JPEG thumbnails */
DWORD WINAPI MyMemWriteFunc(void* AppSpecific, void* pData, DWORD Size)
{
    psReadMemFuncData tmp = (psReadMemFuncData)AppSpecific;

    if (tmp->Pos < 0 || Size > static_cast<DWORD>(INT_MAX - tmp->Pos))
        return 0;
    if (tmp->Pos + (int)Size > tmp->Size)
    {
        unsigned char* resized = (unsigned char*)realloc(tmp->Buffer, tmp->Pos + Size);
        if (resized == NULL) return 0;
        tmp->Buffer = resized;
    }
    memcpy(tmp->Buffer + tmp->Pos, pData, Size);
    tmp->Pos += Size;
    if (tmp->Size < tmp->Pos)
    {
        tmp->Size = tmp->Pos;
    }
    return Size;
}

/* MyMemReadFunc - used for reading JPEG thumbnail data */
DWORD WINAPI MyMemReadFunc(void* AppSpecific, void* pData, DWORD Size)
{
    psReadMemFuncData tmp = (psReadMemFuncData)AppSpecific;

    if (tmp->Pos + (int)Size > tmp->Size)
    {
        Size = tmp->Size - tmp->Pos;
    }
    memcpy(pData, tmp->Buffer + tmp->Pos, Size);
    tmp->Pos += Size;
    return Size;
}

/* MyMemSeekFunc - used for reading JPEG thumbnail data */
DWORD WINAPI MyMemSeekFunc(void* AppSpecific, LONG NewPos, int Origin)
{
    psReadMemFuncData tmp = (psReadMemFuncData)AppSpecific;

    /* Origin can be any of FILE_BEGIN/FILE_CURRENT/FILE_END */
    switch (Origin)
    {
    case FILE_BEGIN:
        tmp->Pos = NewPos;
        break;
    case FILE_CURRENT:
        tmp->Pos += NewPos;
        break;
    case FILE_END:
        tmp->Pos = tmp->Size - NewPos;
        break;
    }
    return tmp->Pos;
} /* MyMemSeekFunc */

/* MySeekFunc - used for export */
DWORD WINAPI MySeekFunc(void* AppSpecific, LONG NewPos, int Origin)
{
    return NewPos;
}

/* MyWriteFunc - used for export */
DWORD WINAPI MyWriteFunc(void* AppSpecific, void* pData, DWORD Size)
{
    psWriteFuncData tmp = (psWriteFuncData)AppSpecific;

    return tmp->thumbMaker->ProcessBuffer(pData, Size / tmp->bytesperline) * Size;
}

typedef struct tagThumbProgressData
{
    sReadMemFuncData wfd;
    int ind;
    int last;
    CSalamanderForOperationsAbstract* Salamander;
} sThumbProgressData, *psThumbProgressData;

// Return FALSE if stop processing
BOOL WINAPI ThumbProgresProc(int done, void* data)
{
    psThumbProgressData ppd = (psThumbProgressData)data;

    if (ppd->last < done)
    {
        ppd->last = done;
        return !ppd->Salamander->ProgressSetSize(CQuadWord(0, done), CQuadWord(0, ppd->ind), TRUE);
    }
    return FALSE;
}

static void UpdateThumbnailsImpl(CSalamanderForOperationsAbstract* Salamander)
{
    int fileCnt, processed = 0;
    CEnumFiles fileEnum;
    std::string message;
    sThumbProgressData pd;
    const CSalamanderDiskSelectionItem* pFile = NULL;
    int flags = 0;
    HWND hProgress = 0;

    CALL_STACK_MESSAGE1("UpdateThumbnails");
    struct ProgressScope
    {
        CSalamanderForOperationsAbstract* Operations; bool Open = false;
        ~ProgressScope() { if (Open) Operations->CloseProgressDialog(); }
    } progress = {Salamander};
    if (!fileEnum.IsValid())
    {
        char error[1024];
        SalamanderGeneral->GetErrorText(GetLastError(), error, _countof(error));
        SalamanderGeneral->ShowMessageBox(error, LoadStr(IDS_ERRORTITLE), MSGBOX_ERROR);
        return;
    }
    memset(&pd.wfd, 0, sizeof(pd.wfd));
    struct BufferScope { sReadMemFuncData* Data; ~BufferScope() { free(Data->Buffer); } } buffer = {&pd.wfd};
    if (!(G.DontShowAnymore & DSA_UPDATE_THUMBNAILS))
    {
        BOOL checked = FALSE;

        fileCnt = ShowOneTimeMessage(SalamanderGeneral->GetMainWindowHWND(), IDS_REGENERATE_THUMB_WARNING,
                                     &checked, MSGBOXEX_YESNO | MSGBOXEX_SILENT, IDS_DONT_SHOW_AGAIN_UTH);
        if (checked)
        {
            G.DontShowAnymore |= DSA_UPDATE_THUMBNAILS;
        }
        if (fileCnt != IDYES)
        {
            return;
        }
    }

    fileCnt = fileEnum.GetFileCount();
    if (fileCnt)
    {
        Salamander->OpenProgressDialog(LoadStr(IDS_REGENERATE_THUMBNAIL_TITLE), TRUE, NULL, TRUE);
        progress.Open = true;
        Salamander->ProgressSetTotalSize(CQuadWord(0, 100), CQuadWord(0, fileCnt));
        hProgress = Salamander->ProgressGetHWND();
    }
    pd.ind = -1;
    pd.Salamander = Salamander;
    while ((pFile = fileEnum.GetFile()) != NULL && !(flags & FL_CANCEL))
    {
        const std::wstring operationPath = PluginPathAddExtendedPrefixW(pFile->FullPathW);
        const std::string path = SalamanderDiskSelection::Utf8FromWide(pFile->FullPathW);
        const std::string display = SalamanderDiskSelection::Utf8FromWide(pFile->NameW);
        const std::string directory = SalamanderDiskSelection::Utf8FromWide(pFile->DirectoryW);
        LPPVHandle PVHandle = NULL;
        struct ImageScope { LPPVHandle* Image; ~ImageScope() { if (*Image != NULL) PVW32DLL.PVCloseImage(*Image); } } image = {&PVHandle};
        PVImageInfo pvii;
        PVCODE code;

        PVOpenImageExInfo oiei;
        PVSaveImageInfo sii;
        EXIFREPLACETHUMBNAILW ReplaceThumbnail;
        CThumbnailTempFile newFile;
        CThumbnailFileAttributes sourceAttributes(operationPath.c_str());

        CALL_STACK_MESSAGE1("PVW32DLL.PVOpenImageEx");
        flags &= FL_KEEP;
        memset(&oiei, 0, sizeof(oiei));
        oiei.cbSize = sizeof(oiei);
        oiei.FileName = path.c_str();

        Salamander->ProgressDialogAddText(display.c_str(), TRUE /*update later*/);
        Salamander->ProgressSetSize(CQuadWord(0, 0), CQuadWord(0, ++pd.ind), FALSE /*update now*/);
        code = PVW32DLL.PVOpenImageEx(&PVHandle, &oiei, &pvii, sizeof(pvii));
        if (code != PVC_OK)
        {
            if (!(flags & FL_NON_IMG_SKIP_ALL))
            {
                message = FormatThumbnailText(LoadStr(IDS_ERROR_OPENING_CONTINUE), PVW32DLL.PVGetErrorText(code), code);
                switch (SalamanderGeneral->DialogError(hProgress, BUTTONS_SKIPCANCEL,
                                                       display.c_str(), message.c_str(), LoadStr(IDS_ERRORTITLE)))
                {
                case DIALOG_SKIPALL:
                    flags |= FL_NON_IMG_SKIP_ALL;
                    break;
                case DIALOG_CANCEL:
                    flags |= FL_CANCEL;
                    break;
                }
            }
            continue;
        }
        if (pvii.Format != PVF_JPG)
        {
            flags |= FL_SKIP;
            if (!(flags & FL_NON_JPEG_ALL))
            {
                switch (SalamanderGeneral->DialogError(hProgress, BUTTONS_SKIPCANCEL,
                                                       display.c_str(), LoadStr(IDS_NOT_JPEG_FILE), LoadStr(IDS_ERRORTITLE)))
                {
                case DIALOG_SKIPALL:
                    flags |= FL_NON_JPEG_ALL;
                    break;
                case DIALOG_CANCEL:
                    flags |= FL_CANCEL;
                    break;
                }
            }
        }
        else if (!(pvii.Flags & PVFF_EXIF))
        {
            if (flags & FL_JFXX_SKIP_ALL)
            {
                flags |= FL_SKIP;
            }
            else if (!(flags & FL_JFXX_ALL))
            {
                switch (SalamanderGeneral->DialogQuestion(hProgress, fileCnt > 1 ? BUTTONS_YESALLSKIPCANCEL : BUTTONS_YESNOCANCEL,
                                                          display.c_str(), LoadStr(IDS_NOT_EXIF_CREATE_JFXX), LoadStr(IDS_ERRORTITLE)))
                {
                case DIALOG_ALL:
                    flags |= FL_JFXX_ALL;
                    break;
                case DIALOG_SKIPALL:
                    flags |= FL_JFXX_SKIP_ALL;
                    // fall through
                case DIALOG_SKIP:
                case DIALOG_NO:
                    flags |= FL_SKIP;
                    break;
                case DIALOG_CANCEL:
                    flags |= FL_CANCEL;
                    break;
                }
            }
        }
        if (!(flags & (FL_SKIP | FL_CANCEL)))
        {
            //         if ((pvii.Format == PVF_JPG) && (pvii.Flags & PVFF_EXIF)) {

            memset(&sii, 0, sizeof(sii));
            sii.cbSize = sizeof(sii);
            sii.Format = PVF_JPG;
            sii.Colors = pvii.Colors;
            sii.ColorModel = PVCM_RGB;

            /* Image is saved using user-defined output handling functions */
            sii.Flags |= PVSF_USERDEFINED_OUTPUT;
            sii.WriteFunc = MyMemWriteFunc;
            sii.SeekFunc = MyMemSeekFunc;
            sii.Width = PV_THUMB_CREATE_WIDTH;
            sii.Height = pvii.Height * PV_THUMB_CREATE_WIDTH / pvii.Width;
            sii.ColorModel = pvii.Colors <= 256 ? PVCM_GRAYS : PVCM_RGB;
            if (sii.Height > PV_THUMB_CREATE_HEIGHT)
            {
                sii.Height = PV_THUMB_CREATE_HEIGHT;
                sii.Width = pvii.Width * PV_THUMB_CREATE_HEIGHT / pvii.Height;
            }
            /* Handle patological cases */
            if (!sii.Height)
                sii.Height = 1;
            if (!sii.Width)
                sii.Width = 1;

            memset(&pd.wfd, 0, sizeof(pd.wfd));
            /* AppSpecific is passed as handle to WriteFunc and SeekFunc */
            pd.last = 0;
            CALL_STACK_MESSAGE1("PVW32DLL.PVSaveImage");
            code = PVW32DLL.PVSaveImage(PVHandle, NULL, &sii, ThumbProgresProc, (void*)&pd, 0);
            // Free the image from memory as soon as possible, libexif may keep 2 copies in memory
            Salamander->ProgressSetSize(CQuadWord(0, 75), CQuadWord(0, pd.ind), FALSE /*update now*/);
            PVW32DLL.PVCloseImage(PVHandle);
            PVHandle = NULL;

            if (code == PVC_OK)
            {
                InitEXIF(hProgress, FALSE);
                ReplaceThumbnail = (EXIFREPLACETHUMBNAILW)GetProcAddress(EXIFLibrary, "EXIFReplaceThumbnailW");

                if (ReplaceThumbnail)
                {
                    if (newFile.Create(pFile->DirectoryW))
                    {
                        if (!ReplaceThumbnail(operationPath.c_str(), newFile.Path.c_str(), pd.wfd.Buffer, pd.wfd.Size))
                        {
                            message = FormatThumbnailText(LoadStr(IDS_REGENERATE_THUMB_WRITEFILE), display.c_str());
                            SalamanderGeneral->ShowMessageBox(message.c_str(), LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
                        }
                        else
                        {
                            do
                            {
                                if (MoveFileExW(newFile.Path.c_str(), operationPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                                {
                                    newFile.Path.clear();
                                    sourceAttributes.Commit();
                                    break;
                                }
                                const DWORD replaceError = GetLastError();
                                const DWORD currentAttributes = GetFileAttributesW(operationPath.c_str());
                                const bool readOnly = replaceError == ERROR_ACCESS_DENIED &&
                                    currentAttributes != INVALID_FILE_ATTRIBUTES &&
                                    (currentAttributes & FILE_ATTRIBUTE_READONLY) != 0;
                                flags &= ~FL_OVERWRITE_RO;
                                if (flags & FL_SKIP_ALL) break;
                                if (readOnly && (flags & FL_OVERWRITE_RO_ALL))
                                {
                                    flags |= FL_OVERWRITE_RO;
                                }
                                else
                                {
                                    int ret = replaceError;
                                    char errBuff[1024];
                                    int btns = BUTTONS_SKIPCANCEL;

                                    SalamanderGeneral->GetErrorText(ret, errBuff, SizeOf(errBuff));
                                    if (readOnly)
                                    {
                                        _tcscpy(errBuff, LoadStr(IDS_READ_ONLY_MODIFY));
                                        btns = BUTTONS_YESALLSKIPCANCEL;
                                    }
                                    // oh my God, each of the two functions supports just one of the two modes we need
                                    if (btns == BUTTONS_YESALLSKIPCANCEL)
                                    {
                                        ret = SalamanderGeneral->DialogQuestion(hProgress, btns,
                                                                                display.c_str(), errBuff, LoadStr(IDS_ERRORTITLE));
                                    }
                                    else
                                    {
                                        ret = SalamanderGeneral->DialogError(hProgress, btns,
                                                                             display.c_str(), errBuff, LoadStr(IDS_ERRORTITLE));
                                    }
                                    switch (ret)
                                    {
                                    case DIALOG_ALL:
                                        flags |= FL_OVERWRITE_RO_ALL;
                                        // fall through
                                    case DIALOG_YES:
                                        flags |= FL_OVERWRITE_RO;
                                        break;
                                    case DIALOG_SKIPALL:
                                        flags |= FL_SKIP_ALL;
                                        // fall through
                                    case DIALOG_SKIP:
                                        flags |= FL_SKIP;
                                        break;
                                    case DIALOG_CANCEL:
                                    default:
                                        flags |= FL_SKIP | FL_CANCEL;
                                        break;
                                    }
                                }
                                if (flags & FL_OVERWRITE_RO)
                                {
                                    if (!readOnly || !sourceAttributes.ClearReadOnly(currentAttributes))
                                        flags &= ~FL_OVERWRITE_RO;
                                }
                            } while ((flags & FL_OVERWRITE_RO) && !(flags & (FL_SKIP | FL_CANCEL)));
                            if (newFile.Path.empty())
                            {
                                SalamanderGeneral->PostChangeOnPathNotification(directory.c_str(), FALSE);
                                processed++;
                            }
                        }
                    }
                }
            }
            else if (code != PVC_CANCELED)
            {
                message = FormatThumbnailText(LoadStr(IDS_SAVEERROR), PVW32DLL.PVGetErrorText(code));
                SalamanderGeneral->ShowMessageBox(message.c_str(), LoadStr(IDS_ERRORTITLE), MSGBOX_ERROR);
            }
            free(pd.wfd.Buffer);
            pd.wfd.Buffer = NULL;
            Salamander->ProgressSetSize(CQuadWord(0, 100), CQuadWord(0, pd.ind + 1), FALSE /*update now*/);
            if (code == PVC_CANCELED)
            {
                // canceled from the progress function
                flags |= FL_CANCEL;
            }
        }
        if (PVHandle != NULL) PVW32DLL.PVCloseImage(PVHandle);
        PVHandle = NULL;
    }
    if (fileCnt)
    {
        Salamander->CloseProgressDialog();
        progress.Open = false;
        if (processed < fileCnt)
        {
            if (processed)
            {
                char fmt[128];
                CQuadWord cnts[2] = {CQuadWord(processed, 0), CQuadWord(fileCnt - processed, 0)};
                SalamanderGeneral->ExpandPluralString(fmt, sizeof(fmt), LoadStr(IDS_N_OF_N_FILES_PROCESSED), 2, cnts);
                message = FormatThumbnailText(fmt, NULL, processed, fileCnt);
            }
            else
            {
                message = LoadStr(IDS_NO_FILES_FOUND);
            }
            SalamanderGeneral->ShowMessageBox(message.c_str(), LoadStr(IDS_PLUGINNAME), MSGBOX_INFO);
        }
    }
    else
    {
        SalamanderGeneral->ShowMessageBox(LoadStr(IDS_NO_FILES_FOUND), LoadStr(IDS_PLUGINNAME), MSGBOX_INFO);
    }
}

void UpdateThumbnails(CSalamanderForOperationsAbstract* Salamander)
{
    try { UpdateThumbnailsImpl(Salamander); }
    catch (const std::bad_alloc&)
    {
        char error[1024];
        SalamanderGeneral->GetErrorText(ERROR_NOT_ENOUGH_MEMORY, error, _countof(error));
        SalamanderGeneral->ShowMessageBox(error, LoadStr(IDS_ERRORTITLE), MSGBOX_ERROR);
    }
}

BOOL WINAPI ThumbProgressProc(int Done, void* AppSpecific)
{
    psWriteFuncData tmp = (psWriteFuncData)AppSpecific;
    // ProcessBuffer returns FALSE when cancelled
    // ThumbProgressProc must return TRUE when cancelled
    return tmp->thumbMaker->ProcessBuffer(NULL, 0) ? FALSE : TRUE;
}

int ExtractWinThumbnail(LPCTSTR filename, unsigned char** pptr)
{
    HRESULT hr;
    LPSTORAGE pIStorage;
    LPSTREAM pIStream;
    int i, j, imglen = 0;
    LPTSTR tmp;
    OLECHAR *pwcFile, *pwcFName, *pwcImg = NULL, *pwcImgName;
    int ret = 0;
    ULONG nBytesRead;
    HANDLE hFind;
    WIN32_FIND_DATA ffd;
    WORD fileDate, fileTime;
#pragma pack(push, 1)
    struct
    {
        short cbSize;
        short version;
        ULONG cnt;
    } hdr;
    struct
    {
        ULONG cbSize;
        ULONG fileNum;
        FILETIME timeStamp;
    } item;
#pragma pack(pop)
    LARGE_INTEGER lint;
    static unsigned char HuffmanTbl[216] = {255, 196, 0, 31, 0, 0, 1, 5, 1, 1, 1, 1, 1,
                                            1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                            255, 196, 0, 181, 16, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1,
                                            125, 1, 2, 3, 0, 4, 17, 5, 18, 33, 49, 65, 6, 19, 81, 97, 7, 34, 113, 20,
                                            50, 129, 145, 161, 8, 35, 66, 177, 193, 21, 82, 209, 240, 36, 51, 98, 114, 130, 9, 10,
                                            22, 23, 24, 25, 26, 37, 38, 39, 40, 41, 42, 52, 53, 54, 55, 56, 57, 58, 67, 68,
                                            69, 70, 71, 72, 73, 74, 83, 84, 85, 86, 87, 88, 89, 90, 99, 100, 101, 102, 103, 104,
                                            105, 106, 115, 116, 117, 118, 119, 120, 121, 122, 131, 132, 133, 134, 135, 136, 137, 138, 146, 147,
                                            148, 149, 150, 151, 152, 153, 154, 162, 163, 164, 165, 166, 167, 168, 169, 170, 178, 179, 180, 181,
                                            182, 183, 184, 185, 186, 194, 195, 196, 197, 198, 199, 200, 201, 202, 210, 211, 212, 213, 214, 215,
                                            216, 217, 218, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 241, 242, 243, 244, 245, 246, 247,
                                            248, 249, 250};
    static unsigned char Quant75Tbl[71] = {255, 216, 255, 219, 0, 67, 0, 8, 6, 6, 7, 6, 5,
                                           8, 7, 7, 7, 9, 9, 8, 10, 12, 20, 13, 12, 11, 11, 12, 25, 18, 19, 15, 20,
                                           29, 26, 31, 30, 29, 26, 28, 28, 32, 36, 46, 39, 32, 34, 44, 35, 28, 28, 40, 55,
                                           41, 44, 48, 49, 52, 52, 52, 31, 39, 57, 61, 56, 50, 60, 46, 51, 52, 50};
    static unsigned char Quant50Tbl[71] = {255, 216, 255, 219, 0, 67, 0, 16, 11, 12, 14, 12, 10,
                                           16, 14, 13, 14, 18, 17, 16, 19, 24, 40, 26, 24, 22, 22, 24, 49, 35, 37, 29, 40,
                                           58, 51, 61, 60, 57, 51, 56, 55, 64, 72, 92, 78, 64, 68, 87, 69, 55, 56, 80, 109,
                                           81, 87, 95, 98, 103, 104, 103, 62, 77, 113, 121, 112, 100, 120, 92, 101, 103, 99};

    CALL_STACK_MESSAGE2(_T("ExtractWinThumbnail(%s)"), filename);
    *pptr = NULL;
    tmp = (LPTSTR)_tcsrchr(filename, '\\');
    if (!tmp)
    {
        tmp = (LPTSTR)filename; // can this ever happen?
    }
    else
    {
        tmp++;
    }
#ifdef _UNICODE
    i = tmp - filename;
    j = wcslen(filename);
    pwcFile = new OLECHAR[max(i + 9, j) + 1];
    lstrcpyn(pwcFile, filename, i);
#else
    i = MultiByteToWideChar(CP_ACP, 0, filename, (int)(tmp - filename), NULL, 0);
    j = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    pwcFile = new OLECHAR[max(i + 9, j) + 1];
    MultiByteToWideChar(CP_ACP, 0, filename, (int)(tmp - filename), pwcFile, i);
#endif
    wcscpy(pwcFile + i, L"Thumbs.db");

    // get the last modification time
    hFind = FindFirstFile(filename, &ffd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
    }
    else
    { // what's wrong?
        memset(&ffd.ftLastWriteTime, 0, sizeof(ffd.ftLastWriteTime));
    }
    // Get FAT-format time - see comment below
    FileTimeToDosDateTime(&ffd.ftLastWriteTime, &fileDate, &fileTime);

    hr = StgOpenStorage(pwcFile, NULL, STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE, NULL, 0, &pIStorage);
    if (!FAILED(hr))
    {
        hr = pIStorage->OpenStream(L"Catalog", NULL, STGM_READ | STGM_SHARE_EXCLUSIVE,
                                   NULL, &pIStream);
        if (!FAILED(hr))
        {
            // file structure:
            // hdr; some data; item array
            // each item:  cbSize, dwFileId, FILETIME, wcFileName
            CALL_STACK_MESSAGE1("ExtractWinThumbnail: Inside Thumbs.db");
#ifdef _UNICODE
            wcscpy(pwcFile, filename);
#else
            MultiByteToWideChar(CP_ACP, 0, filename, -1, pwcFile, j);
#endif
            pwcFName = (LPWSTR)wcsrchr(pwcFile, '\\');
            if (!pwcFName)
            {
                pwcFName = pwcFile; // can this ever happen?
            }
            else
            {
                pwcFName++;
            }
            pIStream->Read(&hdr, sizeof(hdr), &nBytesRead);
            if (nBytesRead == 2 * 2 + 4)
            {
                // hdr.cbSize: 8 -> W2K; 16: XP: skip the rest of the header
                lint.QuadPart = hdr.cbSize;
                pIStream->Seek(lint, STREAM_SEEK_SET, NULL);

                while ((int)hdr.cnt-- > 0)
                {
                    pIStream->Read(&item, sizeof(item), &nBytesRead);
                    if (nBytesRead != 2 * 4 + 8)
                    {
                        break;
                    }
                    item.cbSize -= sizeof(item);
                    if ((int)item.cbSize > imglen)
                    {
                        delete pwcImg;
                        pwcImg = new OLECHAR[item.cbSize / 2 /*from bytes to chars*/];
                        imglen = item.cbSize;
                    }
                    pIStream->Read(pwcImg, item.cbSize, &nBytesRead);
                    if (nBytesRead != item.cbSize)
                    {
                        break;
                    }
                    // get the file name without path
                    pwcImgName = (LPWSTR)wcsrchr(pwcImg, '\\');
                    if (!pwcImgName)
                    {
                        pwcImgName = pwcImg;
                    }
                    else
                    {
                        pwcImgName++;
                    }
                    // got the thumbnailed filename -> check if it's the one we're looking for
                    if (!wcscmp(pwcImgName, pwcFName))
                    {
                        OLECHAR wcNumFName[10], *wcptr = wcNumFName;
                        ULARGE_INTEGER newPos;
#if 1
                        WORD fatDate, fatTime;

                        FileTimeToDosDateTime(&item.timeStamp, &fatDate, &fatTime);
                        if ((fatDate != fileDate) || (fatTime != fileTime))
                        {
                            // FAT time is rounded to multiple of 2 seconds.
                            // Better to compare these low-precision times otherwise we would reject
                            // thumb caches copied along with images to FAT disks
                            break;
                        }
#else
                        if (memcmp(&item.timeStamp, &ffd.ftLastWriteTime, sizeof(FILETIME)))
                        {
                            // timestamp does not match the time/date of the file
                            break;
                        }
#endif
                        pIStream->Release();
                        pIStream = NULL;

                        while (item.fileNum > 0)
                        {
                            *wcptr++ = (WCHAR)((item.fileNum % 10) + '0');
                            item.fileNum /= 10;
                        }
                        *wcptr = 0;

                        // open the stream with the thumbnail
                        hr = pIStorage->OpenStream(wcNumFName, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE,
                                                   NULL, &pIStream);
                        if (!FAILED(hr))
                        {
                            lint.QuadPart = 0;
                            hr = pIStream->Seek(lint, STREAM_SEEK_END, &newPos);
                            if (!FAILED(hr))
                            {
                                // There are some 28 bytes of irrelevant or unknown meaning
                                // We also skip the SOF marker present here
                                lint.QuadPart = 28 + 2;
                                newPos.QuadPart -= 28 + 2;
                                pIStream->Seek(lint, STREAM_SEEK_SET, NULL);
                                *pptr = (unsigned char*)malloc(sizeof(HuffmanTbl) + sizeof(Quant75Tbl) + (int)newPos.QuadPart);
                                if (*pptr)
                                {
                                    // concatenate file header w/ appropriate quantization tables, Huffman tables
                                    // and the image data itself
                                    memcpy(*pptr, (hdr.version >= 5) ? Quant75Tbl : Quant50Tbl, sizeof(Quant75Tbl));
                                    memcpy(*pptr + sizeof(Quant75Tbl), HuffmanTbl, sizeof(HuffmanTbl));
                                    pIStream->Read(*pptr + sizeof(HuffmanTbl) + sizeof(Quant75Tbl), (DWORD)newPos.QuadPart, &nBytesRead);
                                    if (newPos.QuadPart == nBytesRead)
                                    {
                                        ret = sizeof(HuffmanTbl) + sizeof(Quant75Tbl) + (int)newPos.QuadPart;
                                    }
                                    else
                                    {
                                        free(*pptr);
                                        *pptr = NULL;
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }
            if (pIStream)
            {
                pIStream->Release();
            }
        }
        pIStorage->Release();
    }
    else
    {
        i = (int)_tcslen(filename);
        i += 1 + 28; // NULL+strlen(":\5Q30lsldxJoudresxAaaqpcawXc")
        tmp = (LPTSTR)malloc(i * sizeof(TCHAR));
        if (tmp)
        {
            HANDLE hFile;

            _tcscpy(tmp, filename);
            _tcscat(tmp, _T(":\5Q30lsldxJoudresxAaaqpcawXc"));
            hFile = CreateFile(tmp, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
            free(tmp);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                CALL_STACK_MESSAGE1("ExtractWinThumbnail: Parsing ADS");
                j = SetFilePointer(hFile, 0, NULL, FILE_END) - 140 - 2;
                if (j > 0)
                {
                    SetFilePointer(hFile, 0x74, NULL, FILE_BEGIN);
                    *pptr = (unsigned char*)malloc(sizeof(HuffmanTbl) + sizeof(Quant50Tbl) + j);
                    if (*pptr)
                    {
                        WORD fatDate, fatTime;

                        ReadFile(hFile, *pptr, 140 + 2 - 0x74, &nBytesRead, NULL);
                        FileTimeToDosDateTime((FILETIME*)*pptr, &fatDate, &fatTime);
                        if ((fatDate == fileDate) && (fatTime == fileTime))
                        {
                            // FAT time is rounded to multiple of 2 seconds.
                            // Better to compare these low-precision times otherwise we would reject

                            // concatenate file header w/ appropriate quantization tables, Huffman tables
                            // and the image data itself
                            memcpy(*pptr, Quant50Tbl, sizeof(Quant75Tbl));
                            memcpy(*pptr + sizeof(Quant50Tbl), HuffmanTbl, sizeof(HuffmanTbl));
                            ReadFile(hFile, *pptr + sizeof(HuffmanTbl) + sizeof(Quant75Tbl), j, &nBytesRead, NULL);
                            if (j == (int)nBytesRead)
                            {
                                ret = sizeof(HuffmanTbl) + sizeof(Quant75Tbl) + j;
                            }
                            else
                            {
                                free(*pptr);
                                *pptr = NULL;
                            }
                        }
                        else
                        {
                            // wrong time stamp -> ignore the thumbnail
                            free(*pptr);
                            *pptr = NULL;
                        }
                    }
                }
                CloseHandle(hFile);
            }
        }
    }
    delete pwcImg;
    delete pwcFile;
    return ret;
} /* ExtractWinThumbnail */

// open the specified file and convert it into a sequence of DWORDs
// i.e. 24 bits for color (R, G, B) and 8 bits of padding
// the size of one row in bytes is: image_width * sizeof(DWORD)

BOOL CPluginInterfaceForThumbLoader::LoadThumbnail(LPCTSTR filename, int thumbWidth, int thumbHeight,
                                                   CSalamanderThumbnailMakerAbstract* thumbMaker,
                                                   BOOL fastThumbnail)
{
    CALL_STACK_MESSAGE5(_T("CPluginInterfaceForThumbLoader::LoadThumbnail(%s, %d, %d, , %d)"),
                        filename, thumbWidth, thumbHeight, fastThumbnail);

    if (IsSTLThumbnailFile(filename))
        return LoadSTLThumbnail(filename, thumbWidth, thumbHeight, thumbMaker);

    if (IsSVGThumbnailFile(filename))
        return LoadSVGThumbnail(filename, thumbWidth, thumbHeight, thumbMaker);

    PVSaveImageInfo sii;
    LPPVHandle hPVImage;
    PVImageInfo pvii;
    PVOpenImageExInfo pvoi;
    sReadMemFuncData rmfd;
    PVCODE code;
    unsigned char* thumbData;
    DWORD pictureFlags = 0;

    char filenameA[_MAX_PATH];
    bool hasExactExifPath = ConvertPathToExifEncoding(filename, filenameA, sizeof(filenameA));
    const char* filenameForExif = filenameA;
    // The active WIC backend expects UTF-8 paths. The old EXIF helper still
    // uses the best available ANSI/short-path representation above, so keep a
    // separate filename for image decoding.
#ifdef _UNICODE
    std::string filenameForBackend = PluginWideToMultiBytePath(filename, CP_UTF8);
#else
    std::wstring filenameWide = PluginMultiByteToWidePath(filename, CP_UTF8);
    if (filenameWide.empty())
    {
        filenameWide = PluginMultiByteToWidePath(filename, CP_ACP);
    }
    std::string filenameForBackend = PluginWideToMultiBytePath(filenameWide.c_str(), CP_UTF8);
#endif

    pvoi.DataSize = ExtractWinThumbnail(filename, &thumbData);
    for (;;)
    {
        pvoi.cbSize = sizeof(pvoi);
        /* PVFF_FAST: Discard/do not alloc all unnecessary info */
        //     pvoi.Flags  = PVFF_FAST | (G.IgnoreThumbnails ? 0 : PVOF_THUMBNAIL);
        pvoi.Flags = PVFF_FAST | (G.IgnoreThumbnails ? 0 : (fastThumbnail ? PVOF_THUMBNAIL : 0));
        pvoi.FileName = filenameForBackend.c_str();
        if (pvoi.DataSize)
        {
            pvoi.Flags |= PVOF_USERDEFINED_INPUT;
            pvoi.ReadFunc = MyMemReadFunc;
            pvoi.SeekFunc = MyMemSeekFunc;
            pvoi.Handle = &rmfd;
            rmfd.Size = pvoi.DataSize;
            rmfd.Pos = 0;
            rmfd.Buffer = thumbData;
        }
        // open the image
        code = PVW32DLL.PVOpenImageEx(&hPVImage, &pvoi, &pvii, sizeof(pvii));
        if (code != PVC_OK)
        {
            free(thumbData);
            return FALSE; // probably not a bitmap
        }
        if (pvii.Height * pvii.Width > G.MaxThumbImgSize * 1024 * 1024)
        {
            // image too large and presumably thumbnailing it would take too much time
            free(thumbData);
            PVW32DLL.PVCloseImage(hPVImage);
            return FALSE;
        }

        if (!pvoi.DataSize || fastThumbnail || (pvii.Width >= (DWORD)thumbWidth) || (pvii.Height >= (DWORD)thumbHeight))
        {
            break;
        }
        // ignore thumbnail from thumbs.db or ADS - resize the real image data
        PVW32DLL.PVCloseImage(hPVImage);
        pvoi.DataSize = NULL;
    }

    if (pvii.Flags & PVFF_ROTATE90)
    {
        // 90/270deg rotated thumbnail, most likely EXIF JPEG
        int tmp = thumbWidth;
        thumbWidth = thumbHeight;
        thumbHeight = tmp;
    }
    if ((pvii.Flags & PVFF_THUMBNAIL) || pvoi.DataSize)
    {
        if ((pvii.Width < (DWORD)thumbWidth) && (pvii.Height < (DWORD)thumbHeight))
        {
            pictureFlags |= SSTHUMB_ONLY_PREVIEW;
        }
    }
    // 2007.03.14: The DLL temporarily doesn't honour PVFF_BOTTOMTOTOP if PVFF_ROTATE90 is also set (see Canon Raw *.CR2)
    if ((pvii.Flags & (PVFF_BOTTOMTOTOP /*| PVFF_ROTATE90*/)) == PVFF_BOTTOMTOTOP)
    {
        pictureFlags |= SSTHUMB_MIRROR_VERT;
    }
    if (pvii.Flags & PVFF_ROTATE90)
    {
        pictureFlags |= SSTHUMB_ROTATE_90CW;
    }
    if (pvii.Flags & PVFF_FLIP_HOR)
    {
        pictureFlags |= SSTHUMB_MIRROR_HOR;
    }
    if ((pvii.Format == PVF_JPG) && (pvii.Flags & PVFF_EXIF) && !(pvii.Flags & PVFF_THUMBNAIL))
    {
        InitEXIF(NULL, TRUE);
        if (EXIFLibrary)
        {
            EXIFGETORIENTATIONINFO getInfo = (EXIFGETORIENTATIONINFO)GetProcAddress(EXIFLibrary, "EXIFGetOrientationInfo");
#ifdef _UNICODE
            EXIFGETORIENTATIONINFOW getInfoW = (EXIFGETORIENTATIONINFOW)GetProcAddress(EXIFLibrary, "EXIFGetOrientationInfoW");
#endif
            EXIFGETORIENTATIONINFOFROMDATA getInfoFromData =
                (EXIFGETORIENTATIONINFOFROMDATA)GetProcAddress(EXIFLibrary, "EXIFGetOrientationInfoFromData");
            CExifFileBuffer exifBuffer;
            bool bufferLoaded = false;
            SThumbExifInfo info;
            ZeroMemory(&info, sizeof(info));
            BOOL gotInfo = FALSE;

#ifdef _UNICODE
            if (getInfoFromData)
            {
                bufferLoaded = exifBuffer.LoadFromFile(filename);
                if (bufferLoaded)
                {
                    gotInfo = getInfoFromData(exifBuffer.GetExifData(), exifBuffer.GetExifSize(), &info);
                    if (!gotInfo || !(info.flags & TEI_ORIENT))
                    {
                        gotInfo = FALSE;
                        ZeroMemory(&info, sizeof(info));
                    }
                }
            }

            if (!gotInfo && getInfoW)
            {
                gotInfo = getInfoW(filename, &info);
                if (gotInfo && !(info.flags & TEI_ORIENT))
                {
                    gotInfo = FALSE;
                    ZeroMemory(&info, sizeof(info));
                }
            }
            if (!gotInfo && getInfo)
            {
                if (hasExactExifPath)
                {
                    gotInfo = getInfo(filenameForExif, &info);
                }
                else
                {
                    CExifAnsiPath ansiPath;
                    if (ansiPath.PrepareFromFile(filename))
                    {
                        gotInfo = getInfo(ansiPath.GetPath(), &info);
                    }
                }
                if (gotInfo && !(info.flags & TEI_ORIENT))
                {
                    gotInfo = FALSE;
                    ZeroMemory(&info, sizeof(info));
                }
            }
#else
            if (getInfoFromData)
            {
                bufferLoaded = exifBuffer.LoadFromFile(filename);
                if (bufferLoaded)
                {
                    gotInfo = getInfoFromData(exifBuffer.GetExifData(), exifBuffer.GetExifSize(), &info);
                    if (!gotInfo || !(info.flags & TEI_ORIENT))
                    {
                        gotInfo = FALSE;
                        ZeroMemory(&info, sizeof(info));
                    }
                }
            }
            if (!gotInfo && getInfo)
            {
                gotInfo = getInfo(filename, &info);
                if (gotInfo && !(info.flags & TEI_ORIENT))
                {
                    gotInfo = FALSE;
                    ZeroMemory(&info, sizeof(info));
                }
            }
#endif
            if (!gotInfo && getInfoFromData && !bufferLoaded)
            {
                bufferLoaded = exifBuffer.LoadFromFile(filename);
                if (bufferLoaded)
                {
                    gotInfo = getInfoFromData(exifBuffer.GetExifData(), exifBuffer.GetExifSize(), &info);
                    if (!gotInfo || !(info.flags & TEI_ORIENT))
                    {
                        gotInfo = FALSE;
                        ZeroMemory(&info, sizeof(info));
                    }
                }
            }
            if (gotInfo)
            {
                if ((info.flags & (TEI_WIDTH | TEI_HEIGHT)) == (TEI_WIDTH | TEI_HEIGHT))
                {
                    if (((DWORD)info.Width != pvii.Width) || ((DWORD)info.Height != pvii.Height) || (info.Width < info.Height))
                    {
                        // the file has been modified (most likely rotated in something buggy
                        // like UfonView or cropped)
                        // Normal situation is info.Width > info.Height. Some cameras (namely Konica Minolta DiMAGE Z3
                        // seem to store image data with normal orientatation but (Orient != 1) && (info.Width < info.Height)
                        info.Orient = 1;
                        pictureFlags &= ~(SSTHUMB_MIRROR_VERT | SSTHUMB_MIRROR_HOR | SSTHUMB_ROTATE_90CW);
                    }
                }
                switch (info.Orient)
                {
                    //         case 1: /* normal case */
                case 2:
                    pictureFlags |= SSTHUMB_MIRROR_HOR;
                    break;
                case 3:
                    pictureFlags |= SSTHUMB_MIRROR_HOR | SSTHUMB_MIRROR_VERT;
                    break;
                case 4:
                    pictureFlags |= SSTHUMB_MIRROR_VERT;
                    break;
                case 5:
                    pictureFlags |= SSTHUMB_MIRROR_VERT | SSTHUMB_ROTATE_90CW;
                    break;
                case 6:
                    pictureFlags |= SSTHUMB_ROTATE_90CW;
                    break;
                case 7:
                    pictureFlags |= SSTHUMB_MIRROR_HOR | SSTHUMB_ROTATE_90CW;
                    break;
                case 8:
                    pictureFlags |= SSTHUMB_MIRROR_VERT | SSTHUMB_MIRROR_HOR | SSTHUMB_ROTATE_90CW;
                    break;
                }
            }
        }
    }
    memset(&sii, 0, sizeof(sii));
    sii.cbSize = sizeof(sii);
    sii.Format = PVF_RAW;
    sii.Compression = PVCS_NO_COMPRESSION;
    sii.Colors = PV_COLOR_TC32;
    sii.ColorModel = PVCM_RGB;

    if (((int)pvii.Width >= 8 * thumbWidth) || ((int)pvii.Height >= 8 * thumbHeight))
    {
        sii.Flags |= PVSF_SUPERFAST;
    }

    /*     if (pvii.Flags & PVFF_ROTATE90) {
        sii.Flags |= PVSF_ROTATE90;
     }*/
    // Ask the lib to do upside-down mirror possibly to avoid caching.
    // Works only if no simultaneous rotation
    // 2007.05.03: Do not rotate Win2K NTFS ADS thumbnails
    if (((pvii.Flags & (PVFF_BOTTOMTOTOP | PVFF_ROTATE90)) == PVFF_BOTTOMTOTOP) && !pvoi.DataSize)
    {
        sii.Flags |= PVSF_FLIP_VERT;
    }
    /*     if (pvii.Flags & PVFF_FLIP_HOR) {
        sii.Flags |= PVSF_FLIP_HOR;
     }*/
    /* Set transparent handle */
    CALL_STACK_MESSAGE1("PVW32DLL.PVSetBkHandle");
    PVW32DLL.PVSetBkHandle(hPVImage, G.rgbPanelBackground);

    { // Group needed by CALL_STACK_MESSAGE macros
        sWriteFuncData wfd;

        CALL_STACK_MESSAGE3("PVW32DLL.PVSaveImage, thumbFlags: %x, sii.Flags: %x", pictureFlags, sii.Flags);

        wfd.thumbMaker = thumbMaker;
        wfd.bytesperline = pvii.Width * 4;
        wfd.Size = wfd.bytesperline * pvii.Height;
        code = PVW32DLL.CreateThumbnail(hPVImage, &sii, pvii.CurrentImage, pvii.Width, pvii.Height,
                                        thumbWidth, thumbHeight, thumbMaker, pictureFlags, ThumbProgressProc, &wfd);

        // close the image
        CALL_STACK_MESSAGE1("PVW32DLL.PVCloseImage");
        free(thumbData);
        PVW32DLL.PVCloseImage(hPVImage);
    }
    if (code != PVC_OK)
    {
        thumbMaker->SetError();
    }
    return TRUE;
}

PVCODE CreateThumbnail(LPPVHandle hPVImage, LPPVSaveImageInfo pSii, int imageIndex, DWORD imgWidth, DWORD imgHeight,
                       int thumbWidth, int thumbHeight, CSalamanderThumbnailMakerAbstract* thumbMaker, DWORD thumbFlags,
                       TProgressProc progressProc, void* progressProcArg)
{
    if (thumbMaker->SetParameters(imgWidth, imgHeight, thumbFlags))
    {
        /* Image is saved using user-defined output handling functions */
        pSii->Flags |= PVSF_USERDEFINED_OUTPUT;
        pSii->WriteFunc = MyWriteFunc;
        pSii->SeekFunc = MySeekFunc;

        /* progressProcArg is passed as handle also to WriteFunc and SeekFunc */
        return PVW32DLL.PVSaveImage(hPVImage, NULL, pSii, progressProc, progressProcArg, imageIndex);
    }

    // thumbMaker->SetParameters can basically fail only because of out of memory
    return PVC_OOM;
}
