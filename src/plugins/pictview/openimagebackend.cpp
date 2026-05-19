// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#ifdef INT32
#undef INT32
#endif
#ifdef UINT32
#undef UINT32
#endif
#include <wincodec.h>

#include "lib/pvw32dll.h"
#include "PixelAccess.h"
#include "pictview.h"

#pragma comment(lib, "windowscodecs.lib")

struct COpenImageBackendHandle
{
    PVImageInfo ImageInfo;
    PVImageHandles ImageHandles;
    PVFormatSpecificInfo FormatInfo;
    char FileName[MAX_PATH];
    BYTE* Pixels;
    BYTE** Lines;
    DWORD BufferSize;
    LONG StretchWidth;
    LONG StretchHeight;
    DWORD StretchMode;
    COLORREF BkColor;
    DWORD DecodedFrameIndex;
    DWORD LogicalScreenWidth;
    DWORD LogicalScreenHeight;
    BOOL Decoded;
    BOOL BitmapSource;
    LPPVImageSequence Sequence;
};

struct COpenImageSurface
{
    DWORD Width;
    DWORD Height;
    DWORD BytesPerLine;
    DWORD BytesPerPixel;
    BYTE* Pixels;
    BYTE** Lines;
};

static PVCODE WINAPI PVReadImage2Open(LPPVHandle Img, HDC PaintDC, RECT* pDRect, TProgressProc Progress, void* AppSpecific, int ImageIndex);
static PVCODE WINAPI PVCloseImageOpen(LPPVHandle Img);
static PVCODE WINAPI PVDrawImageOpen(LPPVHandle Img, HDC PaintDC, int X, int Y, LPRECT rect);
static const char* WINAPI PVGetErrorTextOpen(DWORD ErrorCode);
static PVCODE WINAPI PVOpenImageExOpen(LPPVHandle* Img, LPPVOpenImageExInfo pOpenExInfo, LPPVImageInfo pImgInfo, int Size);
static PVCODE WINAPI PVSetBkHandleOpen(LPPVHandle Img, COLORREF BkColor);
static DWORD WINAPI PVGetDLLVersionOpen(void);
static PVCODE WINAPI PVSetStretchParametersOpen(LPPVHandle Img, DWORD Width, DWORD Height, DWORD Mode);
static PVCODE WINAPI PVLoadFromClipboardOpen(LPPVHandle* Img, LPPVImageInfo pImgInfo, int Size);
static PVCODE WINAPI PVGetImageInfoOpen(LPPVHandle Img, LPPVImageInfo pImgInfo, int Size, int ImageIndex);
static PVCODE WINAPI PVSetParamOpen(LPPVHandle Img);
static PVCODE WINAPI PVGetHandles2Open(LPPVHandle Img, LPPVImageHandles* pHandles);
static PVCODE WINAPI PVSaveImageOpen(LPPVHandle Img, const char* OutFName, LPPVSaveImageInfo pSii, TProgressProc Progress, void* AppSpecific, int ImageIndex);
static PVCODE WINAPI PVChangeImageOpen(LPPVHandle Img, DWORD Flags);
static DWORD WINAPI PVIsOutCombSupportedOpen(int Fmt, int Compr, int Colors, int ColorModel);
static PVCODE WINAPI PVReadImageSequenceOpen(LPPVHandle Img, LPPVImageSequence* ppSeq);
static PVCODE WINAPI PVCropImageOpen(LPPVHandle Img, int Left, int Top, int Width, int Height);

static void ResetHandleInfo(COpenImageBackendHandle* pHandle);
static void ReleaseDecodedSurface(COpenImageBackendHandle* pHandle);
static void FreeOpenImageBackendHandle(COpenImageBackendHandle* pHandle);
static PVCODE CopyImageInfoOut(const PVImageInfo& srcInfo, LPPVImageInfo pImgInfo, int Size);
static DWORD GetAlignedStride(DWORD width);
static PVCODE HrToPVCode(HRESULT hr);
static DWORD MapContainerFormat(const GUID& formatGuid, const char** pFormatName);
static DWORD MapCompression(DWORD format);
static BOOL HasExifMetadata(IWICBitmapFrameDecode* pFrame, DWORD format);
static BOOL GetMetadataValueUI2(IWICMetadataQueryReader* pReader, LPCWSTR pszQuery, WORD* pValue);
static BOOL GetMetadataValueUI1(IWICMetadataQueryReader* pReader, LPCWSTR pszQuery, BYTE* pValue);
static BOOL GetFrameExifOrientation(IWICBitmapFrameDecode* pFrame, DWORD format, WORD* pOrientation);
static DWORD MapExifOrientationToFlags(WORD orientation);
static BOOL HasFrameInfoForImage(const COpenImageBackendHandle* pHandle, DWORD imageIndex);
static PVCODE LoadFrameInfoFromFile(COpenImageBackendHandle* pHandle, DWORD imageIndex);
static PVCODE DecodeFrameFromFile(COpenImageBackendHandle* pHandle, DWORD imageIndex);
static PVCODE DecodeBitmapHandle(COpenImageBackendHandle* pHandle, HBITMAP hBitmap);
static PVCODE AllocateDecodedSurface(COpenImageBackendHandle* pHandle, DWORD width, DWORD height);
static COpenImageBackendHandle* GetOpenImageBackendHandle(LPPVHandle Img);
static PVCODE EnsureDecodedFrame(COpenImageBackendHandle* pHandle, DWORD imageIndex);
static PVCODE RotateDecodedImage(COpenImageBackendHandle* pHandle, BOOL clockwise);
static void InitSurface(COpenImageSurface* pSurface);
static void ReleaseSurface(COpenImageSurface* pSurface);
static PVCODE AllocateSurface(COpenImageSurface* pSurface, DWORD width, DWORD height, DWORD bytesPerPixel);
static void CopyPixelToSurface(const COpenImageSurface* pSrc, DWORD srcX, DWORD srcY, COpenImageSurface* pDst, DWORD dstX, DWORD dstY);
static PVCODE CreateTransformedSurface(const COpenImageSurface* pSrc, const LPPVSaveImageInfo pSii, COpenImageSurface* pDst, DWORD bytesPerPixel);
static PVCODE CreateSurfaceFromHandle(COpenImageBackendHandle* pHandle, COpenImageSurface* pSurface, DWORD bytesPerPixel);
static PVCODE ApplySurfaceToHandle(COpenImageBackendHandle* pHandle, const COpenImageSurface* pSurface);
static BOOL IsOutputFormatSupported(int fmt, int colors, int colorModel, BOOL userDefinedOutput);
static REFGUID GetEncoderContainerFormat(int fmt);
static PVCODE SaveSurfaceToRawOutput(const COpenImageSurface* pSurface, const LPPVSaveImageInfo pSii, TProgressProc Progress, void* AppSpecific);
static WICPixelFormatGUID GetSurfacePixelFormat(const COpenImageSurface* pSurface);
static WICPixelFormatGUID GetEncoderPixelFormat(int fmt);
static PVCODE CreateBitmapSourceFromSurface(IWICImagingFactory* pFactory, const COpenImageSurface* pSurface, IWICBitmapSource** ppBitmapSource);
static PVCODE PrepareBitmapSourceForEncoder(IWICImagingFactory* pFactory, IWICBitmapSource* pSourceBitmap, WICPixelFormatGUID pixelFormat, IWICBitmapSource** ppWriteSource);
static PVCODE SaveSurfaceToWicFile(const COpenImageSurface* pSurface, const char* fileName, const LPPVSaveImageInfo pSii, TProgressProc Progress, void* AppSpecific);
static void ReleaseSequence(COpenImageBackendHandle* pHandle);
static HBITMAP CreateBitmapFromSurface32(const COpenImageSurface* pSurface);
static void FillSurfaceRect32(COpenImageSurface* pSurface, const RECT* pRect, COLORREF color);
static void BlendSurface32(COpenImageSurface* pCanvas, const COpenImageSurface* pFrame, int left, int top);
static PVCODE BuildGifSequence(COpenImageBackendHandle* pHandle);

template <class T>
static void SafeRelease(T*& pObject)
{
    if (pObject != NULL)
    {
        pObject->Release();
        pObject = NULL;
    }
}

class CCoInitScope
{
public:
    HRESULT Hr;
    BOOL NeedUninitialize;

public:
    CCoInitScope()
    {
        Hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        NeedUninitialize = SUCCEEDED(Hr);
        if (Hr == RPC_E_CHANGED_MODE)
            Hr = S_OK;
    }

    ~CCoInitScope()
    {
        if (NeedUninitialize)
            CoUninitialize();
    }
};

static CPVW32DLL OpenImageBackend = {
    PVReadImage2Open,
    PVCloseImageOpen,
    PVDrawImageOpen,
    PVGetErrorTextOpen,
    PVOpenImageExOpen,
    PVSetBkHandleOpen,
    PVGetDLLVersionOpen,
    PVSetStretchParametersOpen,
    PVLoadFromClipboardOpen,
    PVGetImageInfoOpen,
    PVSetParamOpen,
    PVGetHandles2Open,
    PVSaveImageOpen,
    PVChangeImageOpen,
    PVIsOutCombSupportedOpen,
    PVReadImageSequenceOpen,
    PVCropImageOpen,
    GetRGBAtCursor,
    CalculateHistogram,
    CreateThumbnail,
    SimplifyImageSequence,
};

BOOL InitOpenImageBackend()
{
    PVW32DLL = OpenImageBackend;
    PVW32DLL.Handle = NULL;
    lstrcpynA(PVW32DLL.Version, "Open backend / WIC", SizeOf(PVW32DLL.Version));
    return TRUE;
}

static void ResetHandleInfo(COpenImageBackendHandle* pHandle)
{
    memset(&pHandle->ImageInfo, 0, sizeof(pHandle->ImageInfo));
    memset(&pHandle->ImageHandles, 0, sizeof(pHandle->ImageHandles));
    memset(&pHandle->FormatInfo, 0, sizeof(pHandle->FormatInfo));
    pHandle->ImageInfo.cbSize = sizeof(pHandle->ImageInfo);
    pHandle->ImageInfo.FSI = &pHandle->FormatInfo;
    pHandle->ImageInfo.Comment = NULL;
    pHandle->ImageInfo.StretchMode = PV_STRETCH_NO;
    pHandle->FormatInfo.cbSize = sizeof(pHandle->FormatInfo);
    pHandle->StretchWidth = 0;
    pHandle->StretchHeight = 0;
    pHandle->StretchMode = PV_STRETCH_NO;
    pHandle->BkColor = RGB(255, 255, 255);
    pHandle->DecodedFrameIndex = 0;
    pHandle->LogicalScreenWidth = 0;
    pHandle->LogicalScreenHeight = 0;
    pHandle->Decoded = FALSE;
}

static void ReleaseDecodedSurface(COpenImageBackendHandle* pHandle)
{
    if (pHandle == NULL)
        return;

    if (pHandle->Lines != NULL)
    {
        free(pHandle->Lines);
        pHandle->Lines = NULL;
    }
    if (pHandle->Pixels != NULL)
    {
        free(pHandle->Pixels);
        pHandle->Pixels = NULL;
    }
    pHandle->BufferSize = 0;
    pHandle->Decoded = FALSE;
    pHandle->ImageHandles.pLines = NULL;
    pHandle->ImageHandles.Palette = NULL;
}

static void FreeOpenImageBackendHandle(COpenImageBackendHandle* pHandle)
{
    if (pHandle == NULL)
        return;

    ReleaseSequence(pHandle);
    ReleaseDecodedSurface(pHandle);
    delete pHandle;
}

static PVCODE CopyImageInfoOut(const PVImageInfo& srcInfo, LPPVImageInfo pImgInfo, int Size)
{
    if (pImgInfo == NULL || Size <= 0)
        return PVC_OK;

    memset(pImgInfo, 0, Size);
    memcpy(pImgInfo, &srcInfo, min(Size, (int)sizeof(srcInfo)));
    return PVC_OK;
}

static DWORD GetAlignedStride(DWORD width)
{
    return ((width * 24 + 31) / 32) * 4;
}

static PVCODE HrToPVCode(HRESULT hr)
{
    if (SUCCEEDED(hr))
        return PVC_OK;

    switch (hr)
    {
    case E_OUTOFMEMORY:
        return PVC_OUT_OF_MEMORY;

    case WINCODEC_ERR_COMPONENTNOTFOUND:
    case WINCODEC_ERR_UNKNOWNIMAGEFORMAT:
        return PVC_UNSUP_FILE_TYPE;

    case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
        return PVC_CANNOT_OPEN_FILE;

    default:
        return PVC_READING_ERROR;
    }
}

static DWORD MapContainerFormat(const GUID& formatGuid, const char** pFormatName)
{
    if (pFormatName != NULL)
        *pFormatName = "WIC";

    if (IsEqualGUID(formatGuid, GUID_ContainerFormatBmp))
    {
        if (pFormatName != NULL)
            *pFormatName = "BMP";
        return PVF_BMP;
    }
    if (IsEqualGUID(formatGuid, GUID_ContainerFormatGif))
    {
        if (pFormatName != NULL)
            *pFormatName = "GIF";
        return PVF_GIF;
    }
    if (IsEqualGUID(formatGuid, GUID_ContainerFormatIco))
    {
        if (pFormatName != NULL)
            *pFormatName = "ICO";
        return PVF_ICO;
    }
    if (IsEqualGUID(formatGuid, GUID_ContainerFormatJpeg))
    {
        if (pFormatName != NULL)
            *pFormatName = "JPEG";
        return PVF_JPG;
    }
    if (IsEqualGUID(formatGuid, GUID_ContainerFormatPng))
    {
        if (pFormatName != NULL)
            *pFormatName = "PNG";
        return PVF_PNG;
    }
    if (IsEqualGUID(formatGuid, GUID_ContainerFormatTiff))
    {
        if (pFormatName != NULL)
            *pFormatName = "TIFF";
        return PVF_TIFF;
    }
    if (IsEqualGUID(formatGuid, GUID_ContainerFormatDds))
    {
        if (pFormatName != NULL)
            *pFormatName = "DDS";
        return PVF_DDS;
    }
    return 0;
}

static DWORD MapCompression(DWORD format)
{
    switch (format)
    {
    case PVF_JPG:
        return PVCS_JPEG_HUFFMAN;

    case PVF_PNG:
    case PVF_DDS:
        return PVCS_DEFLATE;

    case PVF_TIFF:
    case PVF_GIF:
    case PVF_BMP:
    case PVF_ICO:
    default:
        return PVCS_NO_COMPRESSION;
    }
}

static BOOL HasExifMetadata(IWICBitmapFrameDecode* pFrame, DWORD format)
{
    IWICMetadataQueryReader* pReader = NULL;
    PROPVARIANT var;
    HRESULT hr;
    BOOL hasExif = FALSE;

    if (pFrame == NULL || (format != PVF_JPG && format != PVF_TIFF))
        return FALSE;

    hr = pFrame->GetMetadataQueryReader(&pReader);
    if (FAILED(hr))
        return FALSE;

    PropVariantInit(&var);
    if (format == PVF_JPG)
        hr = pReader->GetMetadataByName(L"/app1/ifd/{ushort=274}", &var);
    else
        hr = pReader->GetMetadataByName(L"/ifd/{ushort=274}", &var);

    if (SUCCEEDED(hr))
        hasExif = TRUE;

    PropVariantClear(&var);
    SafeRelease(pReader);
    return hasExif;
}

static BOOL GetMetadataValueUI2(IWICMetadataQueryReader* pReader, LPCWSTR pszQuery, WORD* pValue)
{
    PROPVARIANT var;
    HRESULT hr;
    BOOL ok = FALSE;

    if (pReader == NULL || pszQuery == NULL || pValue == NULL)
        return FALSE;

    PropVariantInit(&var);
    hr = pReader->GetMetadataByName(pszQuery, &var);
    if (SUCCEEDED(hr) && var.vt == VT_UI2)
    {
        *pValue = var.uiVal;
        ok = TRUE;
    }
    PropVariantClear(&var);
    return ok;
}

static BOOL GetMetadataValueUI1(IWICMetadataQueryReader* pReader, LPCWSTR pszQuery, BYTE* pValue)
{
    PROPVARIANT var;
    HRESULT hr;
    BOOL ok = FALSE;

    if (pReader == NULL || pszQuery == NULL || pValue == NULL)
        return FALSE;

    PropVariantInit(&var);
    hr = pReader->GetMetadataByName(pszQuery, &var);
    if (SUCCEEDED(hr) && var.vt == VT_UI1)
    {
        *pValue = var.bVal;
        ok = TRUE;
    }
    PropVariantClear(&var);
    return ok;
}

static BOOL GetFrameExifOrientation(IWICBitmapFrameDecode* pFrame, DWORD format, WORD* pOrientation)
{
    IWICMetadataQueryReader* pReader = NULL;
    HRESULT hr;
    BOOL ok = FALSE;

    if (pOrientation != NULL)
        *pOrientation = 1;
    if (pFrame == NULL || pOrientation == NULL)
        return FALSE;
    if (format != PVF_JPG && format != PVF_TIFF)
        return FALSE;

    hr = pFrame->GetMetadataQueryReader(&pReader);
    if (FAILED(hr))
        return FALSE;

    if (format == PVF_JPG)
        ok = GetMetadataValueUI2(pReader, L"/app1/ifd/{ushort=274}", pOrientation);
    else
        ok = GetMetadataValueUI2(pReader, L"/ifd/{ushort=274}", pOrientation);

    SafeRelease(pReader);
    return ok;
}

static DWORD MapExifOrientationToFlags(WORD orientation)
{
    switch (orientation)
    {
    case 2:
        return PVFF_FLIP_HOR;

    case 3:
        return PVFF_BOTTOMTOTOP | PVFF_FLIP_HOR;

    case 4:
        return PVFF_BOTTOMTOTOP;

    case 5:
        return PVFF_BOTTOMTOTOP | PVFF_ROTATE90;

    case 6:
        return PVFF_ROTATE90;

    case 7:
        return PVFF_FLIP_HOR | PVFF_ROTATE90;

    case 8:
        return PVFF_BOTTOMTOTOP | PVFF_FLIP_HOR | PVFF_ROTATE90;

    default:
        return 0;
    }
}

static PVCODE LoadFrameInfoFromFile(COpenImageBackendHandle* pHandle, DWORD imageIndex)
{
    CCoInitScope coInit;
    IWICImagingFactory* pFactory = NULL;
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pFrame = NULL;
    IWICMetadataQueryReader* pDecoderReader = NULL;
    WCHAR fileNameW[MAX_PATH];
    WIN32_FILE_ATTRIBUTE_DATA attrData;
    GUID containerFormat = GUID_NULL;
    const char* formatName = "WIC";
    UINT width = 0;
    UINT height = 0;
    UINT frameCount = 0;
    double dpiX = 0.0;
    double dpiY = 0.0;
    DWORD format = 0;
    WORD logicalWidth = 0;
    WORD logicalHeight = 0;
    WORD orientation = 1;
    HRESULT hr;

    if (pHandle == NULL || pHandle->BitmapSource || pHandle->FileName[0] == 0)
        return PVC_INVALID_HANDLE;

    if (FAILED(coInit.Hr))
        return HrToPVCode(coInit.Hr);

    if (0 == MultiByteToWideChar(CP_ACP, 0, pHandle->FileName, -1, fileNameW, SizeOf(fileNameW)))
        return PVC_CANNOT_OPEN_FILE;

    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWICImagingFactory, (void**)&pFactory);
    if (SUCCEEDED(hr))
        hr = pFactory->CreateDecoderFromFilename(fileNameW, NULL, GENERIC_READ,
                                                 WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (SUCCEEDED(hr))
        hr = pDecoder->GetContainerFormat(&containerFormat);
    if (SUCCEEDED(hr))
        pDecoder->GetMetadataQueryReader(&pDecoderReader);
    if (SUCCEEDED(hr))
        hr = pDecoder->GetFrameCount(&frameCount);
    if (SUCCEEDED(hr) && imageIndex >= frameCount)
        hr = WINCODEC_ERR_FRAMEMISSING;
    if (SUCCEEDED(hr))
        hr = pDecoder->GetFrame(imageIndex, &pFrame);
    if (SUCCEEDED(hr))
        hr = pFrame->GetSize(&width, &height);
    if (SUCCEEDED(hr))
        hr = pFrame->GetResolution(&dpiX, &dpiY);
    if (FAILED(hr))
    {
        SafeRelease(pFrame);
        SafeRelease(pDecoder);
        SafeRelease(pFactory);
        return hr == WINCODEC_ERR_FRAMEMISSING ? PVC_NO_MORE_IMAGES : HrToPVCode(hr);
    }

    format = MapContainerFormat(containerFormat, &formatName);
    if (format == 0)
    {
        SafeRelease(pFrame);
        SafeRelease(pDecoder);
        SafeRelease(pFactory);
        return PVC_UNSUP_FILE_TYPE;
    }

    ReleaseSequence(pHandle);
    ReleaseDecodedSurface(pHandle);
    ResetHandleInfo(pHandle);
    pHandle->ImageInfo.Width = width;
    pHandle->ImageInfo.Height = height;
    pHandle->ImageInfo.BytesPerLine = GetAlignedStride(width);
    pHandle->ImageInfo.Colors = PV_COLOR_TC24;
    pHandle->ImageInfo.Format = format;
    pHandle->ImageInfo.ColorModel = PVCM_RGB;
    pHandle->ImageInfo.NumOfImages = frameCount;
    pHandle->ImageInfo.CurrentImage = imageIndex;
    pHandle->ImageInfo.StretchedWidth = width;
    pHandle->ImageInfo.StretchedHeight = height;
    pHandle->ImageInfo.HorDPI = (DWORD)(dpiX + 0.5);
    pHandle->ImageInfo.VerDPI = (DWORD)(dpiY + 0.5);
    pHandle->ImageInfo.Compression = MapCompression(format);
    pHandle->ImageInfo.TotalBitDepth = 24;
    lstrcpynA(pHandle->ImageInfo.Info1, formatName, SizeOf(pHandle->ImageInfo.Info1));
    lstrcpynA(pHandle->ImageInfo.Info2, "Open backend (WIC)", SizeOf(pHandle->ImageInfo.Info2));

    if (format == PVF_GIF && pDecoderReader != NULL)
    {
        if (GetMetadataValueUI2(pDecoderReader, L"/logscrdesc/Width", &logicalWidth))
            pHandle->LogicalScreenWidth = logicalWidth;
        if (GetMetadataValueUI2(pDecoderReader, L"/logscrdesc/Height", &logicalHeight))
            pHandle->LogicalScreenHeight = logicalHeight;
        if (pHandle->LogicalScreenWidth != 0 && pHandle->LogicalScreenHeight != 0)
        {
            pHandle->FormatInfo.GIF.ScreenWidth = pHandle->LogicalScreenWidth;
            pHandle->FormatInfo.GIF.ScreenHeight = pHandle->LogicalScreenHeight;
        }
        if (frameCount > 1 && pHandle->LogicalScreenWidth != 0 && pHandle->LogicalScreenHeight != 0)
        {
            pHandle->ImageInfo.Flags |= PVFF_IMAGESEQUENCE;
            pHandle->ImageInfo.NumOfImages = 1;
            pHandle->ImageInfo.Width = pHandle->LogicalScreenWidth;
            pHandle->ImageInfo.Height = pHandle->LogicalScreenHeight;
            pHandle->ImageInfo.BytesPerLine = GetAlignedStride(pHandle->ImageInfo.Width);
            pHandle->ImageInfo.StretchedWidth = pHandle->ImageInfo.Width;
            pHandle->ImageInfo.StretchedHeight = pHandle->ImageInfo.Height;
            pHandle->StretchWidth = (LONG)pHandle->ImageInfo.Width;
            pHandle->StretchHeight = (LONG)pHandle->ImageInfo.Height;
        }
    }

    if (GetFrameExifOrientation(pFrame, format, &orientation))
    {
        pHandle->ImageInfo.Flags |= PVFF_EXIF;
        pHandle->ImageInfo.Flags |= MapExifOrientationToFlags(orientation);
    }
    else if (HasExifMetadata(pFrame, format))
        pHandle->ImageInfo.Flags |= PVFF_EXIF;

    if (GetFileAttributesExA(pHandle->FileName, GetFileExInfoStandard, &attrData))
        pHandle->ImageInfo.FileSize = attrData.nFileSizeLow;

    pHandle->StretchWidth = (LONG)width;
    pHandle->StretchHeight = (LONG)height;
    pHandle->StretchMode = PV_STRETCH_NO;

    SafeRelease(pFrame);
    SafeRelease(pDecoderReader);
    SafeRelease(pDecoder);
    SafeRelease(pFactory);
    return PVC_OK;
}

static PVCODE AllocateDecodedSurface(COpenImageBackendHandle* pHandle, DWORD width, DWORD height)
{
    DWORD stride;
    BYTE* pPixels;
    BYTE** pLines;
    DWORD y;

    if (pHandle == NULL || width == 0 || height == 0)
        return PVC_INVALID_HANDLE;

    ReleaseDecodedSurface(pHandle);

    stride = GetAlignedStride(width);
    pPixels = (BYTE*)malloc(stride * height);
    if (pPixels == NULL)
        return PVC_OUT_OF_MEMORY;
    pLines = (BYTE**)malloc(sizeof(BYTE*) * height);
    if (pLines == NULL)
    {
        free(pPixels);
        return PVC_OUT_OF_MEMORY;
    }

    for (y = 0; y < height; y++)
        pLines[y] = pPixels + y * stride;

    pHandle->Pixels = pPixels;
    pHandle->Lines = pLines;
    pHandle->BufferSize = stride * height;
    pHandle->ImageHandles.pLines = pLines;
    pHandle->ImageHandles.Palette = NULL;
    pHandle->ImageInfo.Width = width;
    pHandle->ImageInfo.Height = height;
    pHandle->ImageInfo.BytesPerLine = stride;
    pHandle->ImageInfo.Colors = PV_COLOR_TC24;
    pHandle->ImageInfo.ColorModel = PVCM_RGB;
    pHandle->ImageInfo.TotalBitDepth = 24;
    if (pHandle->StretchWidth == 0 && pHandle->StretchHeight == 0)
    {
        pHandle->StretchWidth = (LONG)width;
        pHandle->StretchHeight = (LONG)height;
    }
    pHandle->Decoded = TRUE;
    return PVC_OK;
}

static void InitSurface(COpenImageSurface* pSurface)
{
    if (pSurface != NULL)
        memset(pSurface, 0, sizeof(*pSurface));
}

static void ReleaseSurface(COpenImageSurface* pSurface)
{
    if (pSurface == NULL)
        return;

    if (pSurface->Lines != NULL)
    {
        free(pSurface->Lines);
        pSurface->Lines = NULL;
    }
    if (pSurface->Pixels != NULL)
    {
        free(pSurface->Pixels);
        pSurface->Pixels = NULL;
    }
    pSurface->Width = 0;
    pSurface->Height = 0;
    pSurface->BytesPerLine = 0;
    pSurface->BytesPerPixel = 0;
}

static PVCODE AllocateSurface(COpenImageSurface* pSurface, DWORD width, DWORD height, DWORD bytesPerPixel)
{
    DWORD y;

    if (pSurface == NULL || width == 0 || height == 0 || (bytesPerPixel != 3 && bytesPerPixel != 4))
        return PVC_INVALID_DIMENSIONS;

    ReleaseSurface(pSurface);
    pSurface->BytesPerLine = ((width * bytesPerPixel) + 3) & ~3;
    pSurface->Pixels = (BYTE*)malloc(pSurface->BytesPerLine * height);
    if (pSurface->Pixels == NULL)
        return PVC_OUT_OF_MEMORY;
    pSurface->Lines = (BYTE**)malloc(sizeof(BYTE*) * height);
    if (pSurface->Lines == NULL)
    {
        free(pSurface->Pixels);
        pSurface->Pixels = NULL;
        return PVC_OUT_OF_MEMORY;
    }

    for (y = 0; y < height; y++)
        pSurface->Lines[y] = pSurface->Pixels + y * pSurface->BytesPerLine;
    memset(pSurface->Pixels, 0, pSurface->BytesPerLine * height);
    pSurface->Width = width;
    pSurface->Height = height;
    pSurface->BytesPerPixel = bytesPerPixel;
    return PVC_OK;
}

static void CopyPixelToSurface(const COpenImageSurface* pSrc, DWORD srcX, DWORD srcY, COpenImageSurface* pDst, DWORD dstX, DWORD dstY)
{
    BYTE* pSrcPixel;
    BYTE* pDstPixel;

    pSrcPixel = pSrc->Lines[srcY] + srcX * pSrc->BytesPerPixel;
    pDstPixel = pDst->Lines[dstY] + dstX * pDst->BytesPerPixel;
    pDstPixel[0] = pSrcPixel[0];
    pDstPixel[1] = pSrcPixel[1];
    pDstPixel[2] = pSrcPixel[2];
    if (pDst->BytesPerPixel == 4)
        pDstPixel[3] = pSrc->BytesPerPixel == 4 ? pSrcPixel[3] : 0;
}

static PVCODE CreateSurfaceFromHandle(COpenImageBackendHandle* pHandle, COpenImageSurface* pSurface, DWORD bytesPerPixel)
{
    DWORD x;
    DWORD y;
    PVCODE ret;

    if (pHandle == NULL || pSurface == NULL || !pHandle->Decoded)
        return PVC_INVALID_HANDLE;

    InitSurface(pSurface);
    ret = AllocateSurface(pSurface, pHandle->ImageInfo.Width, pHandle->ImageInfo.Height, bytesPerPixel);
    if (ret != PVC_OK)
        return ret;

    for (y = 0; y < pSurface->Height; y++)
    {
        BYTE* pSrcLine = pHandle->Lines[y];
        BYTE* pDstLine = pSurface->Lines[y];

        if (bytesPerPixel == 3)
        {
            memcpy(pDstLine, pSrcLine, pHandle->ImageInfo.Width * 3);
        }
        else
        {
            for (x = 0; x < pHandle->ImageInfo.Width; x++)
            {
                pDstLine[x * 4] = pSrcLine[x * 3];
                pDstLine[x * 4 + 1] = pSrcLine[x * 3 + 1];
                pDstLine[x * 4 + 2] = pSrcLine[x * 3 + 2];
                pDstLine[x * 4 + 3] = 0;
            }
        }
    }
    return PVC_OK;
}

static PVCODE CreateTransformedSurface(const COpenImageSurface* pSrc, const LPPVSaveImageInfo pSii, COpenImageSurface* pDst, DWORD bytesPerPixel)
{
    DWORD cropLeft = 0;
    DWORD cropTop = 0;
    DWORD cropWidth;
    DWORD cropHeight;
    DWORD rotatedWidth;
    DWORD rotatedHeight;
    DWORD outWidth;
    DWORD outHeight;
    DWORD x;
    DWORD y;
    BOOL flipHor;
    BOOL flipVert;
    BOOL rotate90;
    PVCODE ret;

    if (pSrc == NULL || pDst == NULL)
        return PVC_INVALID_HANDLE;

    cropWidth = pSrc->Width;
    cropHeight = pSrc->Height;
    flipHor = pSii != NULL && (pSii->Flags & PVSF_FLIP_HOR);
    flipVert = pSii != NULL && (pSii->Flags & PVSF_FLIP_VERT);
    rotate90 = pSii != NULL && (pSii->Flags & PVSF_ROTATE90);

    if (pSii != NULL && pSii->CropWidth != 0 && pSii->CropHeight != 0)
    {
        if (pSii->CropLeft >= pSrc->Width || pSii->CropTop >= pSrc->Height)
            return PVC_INVALID_DIMENSIONS;
        if (pSii->CropLeft + pSii->CropWidth > pSrc->Width || pSii->CropTop + pSii->CropHeight > pSrc->Height)
            return PVC_INVALID_DIMENSIONS;

        cropLeft = pSii->CropLeft;
        cropTop = pSii->CropTop;
        cropWidth = pSii->CropWidth;
        cropHeight = pSii->CropHeight;
    }

    rotatedWidth = rotate90 ? cropHeight : cropWidth;
    rotatedHeight = rotate90 ? cropWidth : cropHeight;
    outWidth = (pSii != NULL && pSii->Width != 0) ? pSii->Width : rotatedWidth;
    outHeight = (pSii != NULL && pSii->Height != 0) ? pSii->Height : rotatedHeight;
    if (outWidth == 0 || outHeight == 0)
        return PVC_INVALID_DIMENSIONS;

    InitSurface(pDst);
    ret = AllocateSurface(pDst, outWidth, outHeight, bytesPerPixel);
    if (ret != PVC_OK)
        return ret;

    for (y = 0; y < outHeight; y++)
    {
        DWORD rotatedY = (y * rotatedHeight) / outHeight;

        for (x = 0; x < outWidth; x++)
        {
            DWORD rotatedX = (x * rotatedWidth) / outWidth;
            DWORD croppedX;
            DWORD croppedY;
            DWORD srcX;
            DWORD srcY;

            if (rotate90)
            {
                croppedX = rotatedY;
                croppedY = cropHeight - 1 - rotatedX;
            }
            else
            {
                croppedX = rotatedX;
                croppedY = rotatedY;
            }

            srcX = cropLeft + croppedX;
            srcY = cropTop + croppedY;
            if (flipHor)
                srcX = pSrc->Width - 1 - srcX;
            if (flipVert)
                srcY = pSrc->Height - 1 - srcY;
            CopyPixelToSurface(pSrc, srcX, srcY, pDst, x, y);
        }
    }
    return PVC_OK;
}

static PVCODE ApplySurfaceToHandle(COpenImageBackendHandle* pHandle, const COpenImageSurface* pSurface)
{
    LONG oldStretchWidth;
    LONG oldStretchHeight;
    int signX;
    int signY;
    PVCODE ret;
    DWORD y;

    if (pHandle == NULL || pSurface == NULL || pSurface->BytesPerPixel != 3)
        return PVC_INVALID_HANDLE;

    oldStretchWidth = pHandle->StretchWidth;
    oldStretchHeight = pHandle->StretchHeight;
    signX = oldStretchWidth < 0 ? -1 : 1;
    signY = oldStretchHeight < 0 ? -1 : 1;

    ret = AllocateDecodedSurface(pHandle, pSurface->Width, pSurface->Height);
    if (ret != PVC_OK)
        return ret;

    for (y = 0; y < pSurface->Height; y++)
        memcpy(pHandle->Lines[y], pSurface->Lines[y], pSurface->Width * 3);

    pHandle->StretchWidth = signX * (LONG)pSurface->Width;
    pHandle->StretchHeight = signY * (LONG)pSurface->Height;
    pHandle->ImageInfo.StretchedWidth = labs(pHandle->StretchWidth);
    pHandle->ImageInfo.StretchedHeight = labs(pHandle->StretchHeight);
    pHandle->ImageInfo.CurrentImage = pHandle->DecodedFrameIndex;
    return PVC_OK;
}

static BOOL IsOutputFormatSupported(int fmt, int colors, int colorModel, BOOL userDefinedOutput)
{
    if (userDefinedOutput)
        return fmt == PVF_RAW && (colors == PV_COLOR_TC32 || colors == PV_COLOR_TC24) && colorModel == PVCM_RGB;

    if (colorModel != PVCM_RGB)
        return FALSE;

    switch (fmt)
    {
    case PVF_BMP:
    case PVF_GIF:
    case PVF_JPG:
    case PVF_PNG:
    case PVF_TIFF:
        return colors == PV_COLOR_TC24 || colors == PV_COLOR_TC32;

    default:
        return FALSE;
    }
}

static REFGUID GetEncoderContainerFormat(int fmt)
{
    switch (fmt)
    {
    case PVF_BMP:
        return GUID_ContainerFormatBmp;

    case PVF_GIF:
        return GUID_ContainerFormatGif;

    case PVF_JPG:
        return GUID_ContainerFormatJpeg;

    case PVF_PNG:
        return GUID_ContainerFormatPng;

    case PVF_TIFF:
        return GUID_ContainerFormatTiff;

    default:
        return GUID_NULL;
    }
}

static PVCODE SaveSurfaceToRawOutput(const COpenImageSurface* pSurface, const LPPVSaveImageInfo pSii, TProgressProc Progress, void* AppSpecific)
{
    DWORD written;
    DWORD totalSize;

    if (pSurface == NULL || pSii == NULL || pSii->WriteFunc == NULL)
        return PVC_INVALID_HANDLE;

    totalSize = pSurface->BytesPerLine * pSurface->Height;
    written = pSii->WriteFunc(AppSpecific, pSurface->Pixels, totalSize);
    if (Progress != NULL)
        Progress(100, AppSpecific);
    return written == totalSize ? PVC_OK : PVC_WRITING_ERROR;
}

static WICPixelFormatGUID GetSurfacePixelFormat(const COpenImageSurface* pSurface)
{
    if (pSurface != NULL && pSurface->BytesPerPixel == 4)
        return GUID_WICPixelFormat32bppBGRA;

    return GUID_WICPixelFormat24bppBGR;
}

static WICPixelFormatGUID GetEncoderPixelFormat(int fmt)
{
    switch (fmt)
    {
    case PVF_GIF:
        return GUID_WICPixelFormat8bppIndexed;

    default:
        return GUID_WICPixelFormat24bppBGR;
    }
}

static PVCODE CreateBitmapSourceFromSurface(IWICImagingFactory* pFactory, const COpenImageSurface* pSurface, IWICBitmapSource** ppBitmapSource)
{
    IWICBitmap* pBitmap = NULL;
    HRESULT hr;

    if (ppBitmapSource != NULL)
        *ppBitmapSource = NULL;
    if (pFactory == NULL || pSurface == NULL || pSurface->Pixels == NULL || ppBitmapSource == NULL)
        return PVC_INVALID_HANDLE;

    hr = pFactory->CreateBitmapFromMemory(pSurface->Width, pSurface->Height, GetSurfacePixelFormat(pSurface),
                                          pSurface->BytesPerLine, pSurface->BytesPerLine * pSurface->Height,
                                          pSurface->Pixels, &pBitmap);
    if (FAILED(hr))
        return HrToPVCode(hr);

    *ppBitmapSource = pBitmap;
    return PVC_OK;
}

static PVCODE PrepareBitmapSourceForEncoder(IWICImagingFactory* pFactory, IWICBitmapSource* pSourceBitmap, WICPixelFormatGUID pixelFormat, IWICBitmapSource** ppWriteSource)
{
    WICPixelFormatGUID sourcePixelFormat;
    IWICFormatConverter* pConverter = NULL;
    IWICPalette* pPalette = NULL;
    HRESULT hr;
    PVCODE ret = PVC_OK;

    if (ppWriteSource != NULL)
        *ppWriteSource = NULL;
    if (pFactory == NULL || pSourceBitmap == NULL || ppWriteSource == NULL)
        return PVC_INVALID_HANDLE;

    hr = pSourceBitmap->GetPixelFormat(&sourcePixelFormat);
    if (FAILED(hr))
        return HrToPVCode(hr);

    if (IsEqualGUID(sourcePixelFormat, pixelFormat))
    {
        pSourceBitmap->AddRef();
        *ppWriteSource = pSourceBitmap;
        return PVC_OK;
    }

    hr = pFactory->CreateFormatConverter(&pConverter);
    if (SUCCEEDED(hr) && IsEqualGUID(pixelFormat, GUID_WICPixelFormat8bppIndexed))
        hr = pFactory->CreatePalette(&pPalette);
    if (SUCCEEDED(hr) && pPalette != NULL)
        hr = pPalette->InitializeFromBitmap(pSourceBitmap, 256, FALSE);
    if (SUCCEEDED(hr))
        hr = pConverter->Initialize(pSourceBitmap, pixelFormat,
                                    IsEqualGUID(pixelFormat, GUID_WICPixelFormat8bppIndexed) ? WICBitmapDitherTypeErrorDiffusion : WICBitmapDitherTypeNone,
                                    pPalette, 0.0,
                                    IsEqualGUID(pixelFormat, GUID_WICPixelFormat8bppIndexed) ? WICBitmapPaletteTypeMedianCut : WICBitmapPaletteTypeCustom);
    if (SUCCEEDED(hr))
    {
        *ppWriteSource = pConverter;
        pConverter = NULL;
    }
    else
    {
        ret = HrToPVCode(hr);
    }

    SafeRelease(pPalette);
    SafeRelease(pConverter);
    return ret;
}

static PVCODE SaveSurfaceToWicFile(const COpenImageSurface* pSurface, const char* fileName, const LPPVSaveImageInfo pSii, TProgressProc Progress, void* AppSpecific)
{
    CCoInitScope coInit;
    IWICImagingFactory* pFactory = NULL;
    IWICStream* pStream = NULL;
    IWICBitmapEncoder* pEncoder = NULL;
    IWICBitmapFrameEncode* pFrameEncode = NULL;
    IPropertyBag2* pPropertyBag = NULL;
    IWICBitmapSource* pSourceBitmap = NULL;
    IWICBitmapSource* pWriteSource = NULL;
    WCHAR fileNameW[MAX_PATH];
    WICPixelFormatGUID pixelFormat;
    HRESULT hr;
    PVCODE ret = PVC_OK;
    int fmt;

    if (pSurface == NULL || fileName == NULL || *fileName == 0 || pSii == NULL)
        return PVC_CANNOT_OPEN_FILE;
    if (FAILED(coInit.Hr))
        return HrToPVCode(coInit.Hr);
    if (0 == MultiByteToWideChar(CP_ACP, 0, fileName, -1, fileNameW, SizeOf(fileNameW)))
        return PVC_CANNOT_OPEN_FILE;

    fmt = pSii->Format;
    pixelFormat = GetEncoderPixelFormat(fmt);

    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWICImagingFactory, (void**)&pFactory);
    if (SUCCEEDED(hr))
        hr = pFactory->CreateStream(&pStream);
    if (SUCCEEDED(hr))
        hr = pStream->InitializeFromFilename(fileNameW, GENERIC_WRITE);
    if (SUCCEEDED(hr))
        hr = pFactory->CreateEncoder(GetEncoderContainerFormat(fmt), NULL, &pEncoder);
    if (SUCCEEDED(hr))
        ret = CreateBitmapSourceFromSurface(pFactory, pSurface, &pSourceBitmap);
    if (SUCCEEDED(hr) && ret == PVC_OK)
        hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
    if (SUCCEEDED(hr) && ret == PVC_OK)
        hr = pEncoder->CreateNewFrame(&pFrameEncode, &pPropertyBag);
    if (SUCCEEDED(hr) && ret == PVC_OK)
        hr = pFrameEncode->Initialize(pPropertyBag);
    if (SUCCEEDED(hr) && ret == PVC_OK)
        hr = pFrameEncode->SetSize(pSurface->Width, pSurface->Height);
    if (SUCCEEDED(hr) && (pSii->HorDPI != 0 || pSii->VerDPI != 0))
        hr = pFrameEncode->SetResolution(pSii->HorDPI != 0 ? pSii->HorDPI : 96.0,
                                         pSii->VerDPI != 0 ? pSii->VerDPI : 96.0);
    if (SUCCEEDED(hr))
        hr = pFrameEncode->SetPixelFormat(&pixelFormat);
    if (SUCCEEDED(hr) && ret == PVC_OK)
        ret = PrepareBitmapSourceForEncoder(pFactory, pSourceBitmap, pixelFormat, &pWriteSource);
    if (SUCCEEDED(hr) && ret == PVC_OK)
        hr = pFrameEncode->WriteSource(pWriteSource, NULL);
    if (SUCCEEDED(hr))
        hr = pFrameEncode->Commit();
    if (SUCCEEDED(hr))
        hr = pEncoder->Commit();

    if (Progress != NULL)
        Progress(100, AppSpecific);

    if (ret == PVC_OK)
        ret = HrToPVCode(hr);
    SafeRelease(pWriteSource);
    SafeRelease(pSourceBitmap);
    SafeRelease(pPropertyBag);
    SafeRelease(pFrameEncode);
    SafeRelease(pEncoder);
    SafeRelease(pStream);
    SafeRelease(pFactory);
    return ret == PVC_READING_ERROR ? PVC_WRITING_ERROR : ret;
}

static void ReleaseSequence(COpenImageBackendHandle* pHandle)
{
    LPPVImageSequence pSeq;

    if (pHandle == NULL)
        return;

    pSeq = pHandle->Sequence;
    while (pSeq != NULL)
    {
        LPPVImageSequence pNext = pSeq->pNext;

        if (pSeq->TransparentHandle != NULL)
            DeleteObject(pSeq->TransparentHandle);
        if (pSeq->ImgHandle != NULL)
            DeleteObject(pSeq->ImgHandle);
        free(pSeq);
        pSeq = pNext;
    }
    pHandle->Sequence = NULL;
}

static HBITMAP CreateBitmapFromSurface32(const COpenImageSurface* pSurface)
{
    BITMAPINFO bi;
    void* pBits = NULL;
    HBITMAP hBitmap;
    DWORD y;

    if (pSurface == NULL || pSurface->BytesPerPixel != 4)
        return NULL;

    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = pSurface->Width;
    bi.bmiHeader.biHeight = -(LONG)pSurface->Height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    hBitmap = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (hBitmap == NULL || pBits == NULL)
        return NULL;

    for (y = 0; y < pSurface->Height; y++)
        memcpy((BYTE*)pBits + y * pSurface->BytesPerLine, pSurface->Lines[y], pSurface->Width * 4);
    return hBitmap;
}

static void FillSurfaceRect32(COpenImageSurface* pSurface, const RECT* pRect, COLORREF color)
{
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
    LONG x;
    LONG y;

    if (pSurface == NULL || pRect == NULL || pSurface->BytesPerPixel != 4)
        return;

    left = max(0, pRect->left);
    top = max(0, pRect->top);
    right = min((LONG)pSurface->Width, pRect->right);
    bottom = min((LONG)pSurface->Height, pRect->bottom);
    for (y = top; y < bottom; y++)
    {
        BYTE* pLine = pSurface->Lines[y];

        for (x = left; x < right; x++)
        {
            BYTE* pPixel = pLine + x * 4;

            pPixel[0] = GetBValue(color);
            pPixel[1] = GetGValue(color);
            pPixel[2] = GetRValue(color);
            pPixel[3] = 0;
        }
    }
}

static void BlendSurface32(COpenImageSurface* pCanvas, const COpenImageSurface* pFrame, int left, int top)
{
    DWORD x;
    DWORD y;

    if (pCanvas == NULL || pFrame == NULL || pCanvas->BytesPerPixel != 4 || pFrame->BytesPerPixel != 4)
        return;

    for (y = 0; y < pFrame->Height; y++)
    {
        LONG dstY = top + (LONG)y;

        if (dstY < 0 || dstY >= (LONG)pCanvas->Height)
            continue;

        for (x = 0; x < pFrame->Width; x++)
        {
            LONG dstX = left + (LONG)x;
            BYTE* pSrcPixel;
            BYTE* pDstPixel;
            BYTE alpha;

            if (dstX < 0 || dstX >= (LONG)pCanvas->Width)
                continue;

            pSrcPixel = pFrame->Lines[y] + x * 4;
            pDstPixel = pCanvas->Lines[dstY] + dstX * 4;
            alpha = pSrcPixel[3];
            if (alpha == 0)
                continue;
            if (alpha == 255)
            {
                memcpy(pDstPixel, pSrcPixel, 4);
            }
            else
            {
                pDstPixel[0] = (BYTE)((pSrcPixel[0] * alpha + pDstPixel[0] * (255 - alpha)) / 255);
                pDstPixel[1] = (BYTE)((pSrcPixel[1] * alpha + pDstPixel[1] * (255 - alpha)) / 255);
                pDstPixel[2] = (BYTE)((pSrcPixel[2] * alpha + pDstPixel[2] * (255 - alpha)) / 255);
                pDstPixel[3] = 255;
            }
        }
    }
}

static PVCODE BuildGifSequence(COpenImageBackendHandle* pHandle)
{
    CCoInitScope coInit;
    IWICImagingFactory* pFactory = NULL;
    IWICBitmapDecoder* pDecoder = NULL;
    WCHAR fileNameW[MAX_PATH];
    UINT frameCount = 0;
    HRESULT hr;
    DWORD frameIndex;
    COpenImageSurface canvasSurface;
    COpenImageSurface previousSurface;
    RECT fullRect;
    PVCODE ret = PVC_OK;
    LPPVImageSequence pSeqHead = NULL;
    LPPVImageSequence* ppSeqTail = &pSeqHead;

    if (pHandle == NULL || pHandle->FileName[0] == 0 || pHandle->ImageInfo.Format != PVF_GIF)
        return PVC_INVALID_HANDLE;
    if (pHandle->Sequence != NULL)
        return PVC_OK;
    if (FAILED(coInit.Hr))
        return HrToPVCode(coInit.Hr);
    if (0 == MultiByteToWideChar(CP_ACP, 0, pHandle->FileName, -1, fileNameW, SizeOf(fileNameW)))
        return PVC_CANNOT_OPEN_FILE;

    InitSurface(&canvasSurface);
    InitSurface(&previousSurface);

    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWICImagingFactory, (void**)&pFactory);
    if (SUCCEEDED(hr))
        hr = pFactory->CreateDecoderFromFilename(fileNameW, NULL, GENERIC_READ,
                                                 WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (SUCCEEDED(hr))
        hr = pDecoder->GetFrameCount(&frameCount);
    if (FAILED(hr))
    {
        ret = HrToPVCode(hr);
        goto GIF_DONE;
    }

    if (pHandle->LogicalScreenWidth == 0 || pHandle->LogicalScreenHeight == 0)
    {
        pHandle->LogicalScreenWidth = pHandle->ImageInfo.Width;
        pHandle->LogicalScreenHeight = pHandle->ImageInfo.Height;
    }

    ret = AllocateSurface(&canvasSurface, pHandle->LogicalScreenWidth, pHandle->LogicalScreenHeight, 4);
    if (ret != PVC_OK)
        goto GIF_DONE;
    ret = AllocateSurface(&previousSurface, pHandle->LogicalScreenWidth, pHandle->LogicalScreenHeight, 4);
    if (ret != PVC_OK)
        goto GIF_DONE;

    fullRect.left = 0;
    fullRect.top = 0;
    fullRect.right = pHandle->LogicalScreenWidth;
    fullRect.bottom = pHandle->LogicalScreenHeight;
    FillSurfaceRect32(&canvasSurface, &fullRect, pHandle->BkColor);

    for (frameIndex = 0; frameIndex < frameCount && ret == PVC_OK; frameIndex++)
    {
        IWICBitmapFrameDecode* pFrame = NULL;
        IWICMetadataQueryReader* pReader = NULL;
        IWICFormatConverter* pConverter = NULL;
        COpenImageSurface frameSurface;
        WORD left = 0;
        WORD top = 0;
        WORD frameWidth = 0;
        WORD frameHeight = 0;
        WORD delay = 0;
        BYTE disposal = PVDM_UNDEFINED;
        RECT frameRect;
        HBITMAP hFrameBitmap;
        LPPVImageSequence pSeq;

        InitSurface(&frameSurface);
        hr = pDecoder->GetFrame(frameIndex, &pFrame);
        if (SUCCEEDED(hr))
            pFrame->GetMetadataQueryReader(&pReader);
        if (SUCCEEDED(hr))
            hr = pFactory->CreateFormatConverter(&pConverter);
        if (SUCCEEDED(hr))
            hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA,
                                        WICBitmapDitherTypeNone, NULL, 0.0,
                                        WICBitmapPaletteTypeCustom);
        if (FAILED(hr))
        {
            ret = HrToPVCode(hr);
            ReleaseSurface(&frameSurface);
            SafeRelease(pConverter);
            SafeRelease(pReader);
            SafeRelease(pFrame);
            break;
        }

        if (!GetMetadataValueUI2(pReader, L"/imgdesc/Left", &left))
            left = 0;
        if (!GetMetadataValueUI2(pReader, L"/imgdesc/Top", &top))
            top = 0;
        if (!GetMetadataValueUI2(pReader, L"/imgdesc/Width", &frameWidth))
            frameWidth = 0;
        if (!GetMetadataValueUI2(pReader, L"/imgdesc/Height", &frameHeight))
            frameHeight = 0;
        GetMetadataValueUI2(pReader, L"/grctlext/Delay", &delay);
        GetMetadataValueUI1(pReader, L"/grctlext/Disposal", &disposal);

        if (frameWidth == 0 || frameHeight == 0)
        {
            UINT w = 0;
            UINT h = 0;

            pFrame->GetSize(&w, &h);
            frameWidth = (WORD)w;
            frameHeight = (WORD)h;
        }

        ret = AllocateSurface(&frameSurface, frameWidth, frameHeight, 4);
        if (ret == PVC_OK)
        {
            hr = pConverter->CopyPixels(NULL, frameSurface.BytesPerLine,
                                        frameSurface.BytesPerLine * frameSurface.Height, frameSurface.Pixels);
            if (FAILED(hr))
                ret = HrToPVCode(hr);
        }

        if (ret == PVC_OK && disposal == PVDM_PREVIOUS)
            memcpy(previousSurface.Pixels, canvasSurface.Pixels, canvasSurface.BytesPerLine * canvasSurface.Height);

        if (ret == PVC_OK)
            BlendSurface32(&canvasSurface, &frameSurface, left, top);

        hFrameBitmap = NULL;
        if (ret == PVC_OK)
        {
            hFrameBitmap = CreateBitmapFromSurface32(&canvasSurface);
            if (hFrameBitmap == NULL)
                ret = PVC_GDI_ERROR;
        }

        if (ret == PVC_OK)
        {
            pSeq = (LPPVImageSequence)malloc(sizeof(PVImageSequence));
            if (pSeq == NULL)
            {
                DeleteObject(hFrameBitmap);
                ret = PVC_OUT_OF_MEMORY;
            }
            else
            {
                memset(pSeq, 0, sizeof(*pSeq));
                pSeq->Rect = fullRect;
                pSeq->ImgHandle = hFrameBitmap;
                pSeq->Delay = delay != 0 ? delay * 10 : 100;
                pSeq->DisposalMethod = disposal;
                *ppSeqTail = pSeq;
                ppSeqTail = &pSeq->pNext;
            }
        }

        frameRect.left = left;
        frameRect.top = top;
        frameRect.right = left + frameWidth;
        frameRect.bottom = top + frameHeight;
        if (ret == PVC_OK)
        {
            switch (disposal)
            {
            case PVDM_BACKGROUND:
                FillSurfaceRect32(&canvasSurface, &frameRect, pHandle->BkColor);
                break;

            case PVDM_PREVIOUS:
                memcpy(canvasSurface.Pixels, previousSurface.Pixels, canvasSurface.BytesPerLine * canvasSurface.Height);
                break;
            }
        }

        ReleaseSurface(&frameSurface);
        SafeRelease(pConverter);
        SafeRelease(pReader);
        SafeRelease(pFrame);
    }

    if (ret == PVC_OK)
        pHandle->Sequence = pSeqHead;
    else
    {
        pHandle->Sequence = pSeqHead;
        ReleaseSequence(pHandle);
    }

GIF_DONE:
    ReleaseSurface(&previousSurface);
    ReleaseSurface(&canvasSurface);
    SafeRelease(pDecoder);
    SafeRelease(pFactory);
    return ret;
}

static PVCODE DecodeFrameFromFile(COpenImageBackendHandle* pHandle, DWORD imageIndex)
{
    CCoInitScope coInit;
    IWICImagingFactory* pFactory = NULL;
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pFrame = NULL;
    IWICFormatConverter* pConverter = NULL;
    WCHAR fileNameW[MAX_PATH];
    HRESULT hr;
    PVCODE ret;

    if (!HasFrameInfoForImage(pHandle, imageIndex))
    {
        ret = LoadFrameInfoFromFile(pHandle, imageIndex);
        if (ret != PVC_OK)
            return ret;
    }

    if (FAILED(coInit.Hr))
        return HrToPVCode(coInit.Hr);

    if (0 == MultiByteToWideChar(CP_ACP, 0, pHandle->FileName, -1, fileNameW, SizeOf(fileNameW)))
        return PVC_CANNOT_OPEN_FILE;

    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWICImagingFactory, (void**)&pFactory);
    if (SUCCEEDED(hr))
        hr = pFactory->CreateDecoderFromFilename(fileNameW, NULL, GENERIC_READ,
                                                 WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (SUCCEEDED(hr))
        hr = pDecoder->GetFrame(imageIndex, &pFrame);
    if (SUCCEEDED(hr))
        hr = pFactory->CreateFormatConverter(&pConverter);
    if (SUCCEEDED(hr))
        hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat24bppBGR,
                                    WICBitmapDitherTypeNone, NULL, 0.0,
                                    WICBitmapPaletteTypeCustom);
    if (FAILED(hr))
    {
        SafeRelease(pConverter);
        SafeRelease(pFrame);
        SafeRelease(pDecoder);
        SafeRelease(pFactory);
        return HrToPVCode(hr);
    }

    ret = AllocateDecodedSurface(pHandle, pHandle->ImageInfo.Width, pHandle->ImageInfo.Height);
    if (ret == PVC_OK)
    {
        hr = pConverter->CopyPixels(NULL, pHandle->ImageInfo.BytesPerLine,
                                    pHandle->BufferSize, pHandle->Pixels);
        if (FAILED(hr))
        {
            ReleaseDecodedSurface(pHandle);
            ret = HrToPVCode(hr);
        }
        else
        {
            pHandle->DecodedFrameIndex = imageIndex;
            pHandle->ImageInfo.CurrentImage = imageIndex;
        }
    }

    SafeRelease(pConverter);
    SafeRelease(pFrame);
    SafeRelease(pDecoder);
    SafeRelease(pFactory);
    return ret;
}

static PVCODE DecodeBitmapHandle(COpenImageBackendHandle* pHandle, HBITMAP hBitmap)
{
    BITMAP bmp;
    BITMAPINFO bmi;
    HDC hdc;
    PVCODE ret;
    int scanLines;

    if (pHandle == NULL || hBitmap == NULL)
        return PVC_INVALID_HANDLE;

    if (0 == GetObject(hBitmap, sizeof(bmp), &bmp))
        return PVC_INVALID_HANDLE;

    ResetHandleInfo(pHandle);
    pHandle->BitmapSource = TRUE;
    pHandle->ImageInfo.Format = PVF_BMP;
    pHandle->ImageInfo.Width = bmp.bmWidth;
    pHandle->ImageInfo.Height = bmp.bmHeight;
    pHandle->ImageInfo.NumOfImages = 1;
    pHandle->ImageInfo.Colors = PV_COLOR_TC24;
    pHandle->ImageInfo.ColorModel = PVCM_RGB;
    pHandle->ImageInfo.StretchedWidth = bmp.bmWidth;
    pHandle->ImageInfo.StretchedHeight = bmp.bmHeight;
    pHandle->ImageInfo.Compression = PVCS_NO_COMPRESSION;
    pHandle->ImageInfo.TotalBitDepth = 24;
    lstrcpynA(pHandle->ImageInfo.Info1, "BMP", SizeOf(pHandle->ImageInfo.Info1));
    lstrcpynA(pHandle->ImageInfo.Info2, "Open backend (bitmap handle)", SizeOf(pHandle->ImageInfo.Info2));
    pHandle->StretchWidth = bmp.bmWidth;
    pHandle->StretchHeight = bmp.bmHeight;

    ret = AllocateDecodedSurface(pHandle, bmp.bmWidth, bmp.bmHeight);
    if (ret != PVC_OK)
        return ret;

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = bmp.bmWidth;
    bmi.bmiHeader.biHeight = -bmp.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    hdc = GetDC(NULL);
    if (hdc == NULL)
    {
        ReleaseDecodedSurface(pHandle);
        return PVC_GDI_ERROR;
    }

    scanLines = GetDIBits(hdc, hBitmap, 0, bmp.bmHeight, pHandle->Pixels, &bmi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);
    if (scanLines == 0)
    {
        ReleaseDecodedSurface(pHandle);
        return PVC_GDI_ERROR;
    }

    pHandle->DecodedFrameIndex = 0;
    pHandle->ImageInfo.CurrentImage = 0;
    return PVC_OK;
}

static COpenImageBackendHandle* GetOpenImageBackendHandle(LPPVHandle Img)
{
    return (COpenImageBackendHandle*)Img;
}

static BOOL HasFrameInfoForImage(const COpenImageBackendHandle* pHandle, DWORD imageIndex)
{
    if (pHandle == NULL || pHandle->BitmapSource || pHandle->FileName[0] == 0)
        return FALSE;

    return pHandle->ImageInfo.Width != 0 &&
           pHandle->ImageInfo.Height != 0 &&
           pHandle->ImageInfo.BytesPerLine != 0 &&
           pHandle->ImageInfo.CurrentImage == imageIndex;
}

static PVCODE EnsureDecodedFrame(COpenImageBackendHandle* pHandle, DWORD imageIndex)
{
    if (pHandle == NULL)
        return PVC_INVALID_HANDLE;

    if (pHandle->Decoded && pHandle->DecodedFrameIndex == imageIndex)
        return PVC_OK;

    if (pHandle->BitmapSource)
        return imageIndex == 0 ? PVC_OK : PVC_NO_MORE_IMAGES;

    return DecodeFrameFromFile(pHandle, imageIndex);
}

static PVCODE RotateDecodedImage(COpenImageBackendHandle* pHandle, BOOL clockwise)
{
    BYTE* pPixels;
    BYTE** pLines;
    DWORD srcWidth;
    DWORD srcHeight;
    DWORD srcStride;
    DWORD dstWidth;
    DWORD dstHeight;
    DWORD dstStride;
    DWORD x;
    DWORD y;
    LONG oldStretchWidth;
    LONG oldStretchHeight;

    if (pHandle == NULL)
        return PVC_INVALID_HANDLE;

    if (!pHandle->Decoded || pHandle->Pixels == NULL || pHandle->Lines == NULL)
        return PVC_INVALID_HANDLE;

    srcWidth = pHandle->ImageInfo.Width;
    srcHeight = pHandle->ImageInfo.Height;
    srcStride = pHandle->ImageInfo.BytesPerLine;
    dstWidth = srcHeight;
    dstHeight = srcWidth;
    dstStride = GetAlignedStride(dstWidth);

    pPixels = (BYTE*)malloc(dstStride * dstHeight);
    if (pPixels == NULL)
        return PVC_OUT_OF_MEMORY;
    pLines = (BYTE**)malloc(sizeof(BYTE*) * dstHeight);
    if (pLines == NULL)
    {
        free(pPixels);
        return PVC_OUT_OF_MEMORY;
    }
    for (y = 0; y < dstHeight; y++)
        pLines[y] = pPixels + y * dstStride;

    memset(pPixels, 0, dstStride * dstHeight);
    for (y = 0; y < srcHeight; y++)
    {
        BYTE* pSrcLine = pHandle->Lines[y];

        for (x = 0; x < srcWidth; x++)
        {
            DWORD dstX;
            DWORD dstY;
            BYTE* pDstPixel;

            if (clockwise)
            {
                dstX = srcHeight - 1 - y;
                dstY = x;
            }
            else
            {
                dstX = y;
                dstY = srcWidth - 1 - x;
            }

            pDstPixel = pLines[dstY] + dstX * 3;
            memcpy(pDstPixel, pSrcLine + x * 3, 3);
        }
    }

    free(pHandle->Pixels);
    free(pHandle->Lines);
    oldStretchWidth = pHandle->StretchWidth;
    oldStretchHeight = pHandle->StretchHeight;
    pHandle->Pixels = pPixels;
    pHandle->Lines = pLines;
    pHandle->BufferSize = dstStride * dstHeight;
    pHandle->ImageHandles.pLines = pLines;
    pHandle->ImageInfo.Width = dstWidth;
    pHandle->ImageInfo.Height = dstHeight;
    pHandle->ImageInfo.BytesPerLine = dstStride;
    pHandle->ImageInfo.StretchedWidth = labs(oldStretchHeight);
    pHandle->ImageInfo.StretchedHeight = labs(oldStretchWidth);
    pHandle->StretchWidth = oldStretchHeight;
    pHandle->StretchHeight = oldStretchWidth;
    return PVC_OK;
}

static PVCODE WINAPI PVReadImage2Open(LPPVHandle Img, HDC PaintDC, RECT* pDRect, TProgressProc Progress, void* AppSpecific, int ImageIndex)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);
    PVCODE ret;

    ret = EnsureDecodedFrame(pHandle, ImageIndex < 0 ? 0 : (DWORD)ImageIndex);
    if (ret != PVC_OK)
        return ret;

    if (Progress != NULL)
        Progress(100, AppSpecific);

    if (PaintDC != NULL)
        return PVDrawImageOpen(Img, PaintDC, pDRect ? pDRect->left : 0, pDRect ? pDRect->top : 0, pDRect);

    return PVC_OK;
}

static PVCODE WINAPI PVCloseImageOpen(LPPVHandle Img)
{
    FreeOpenImageBackendHandle(GetOpenImageBackendHandle(Img));
    return PVC_OK;
}

static PVCODE WINAPI PVDrawImageOpen(LPPVHandle Img, HDC PaintDC, int X, int Y, LPRECT rect)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);
    BITMAPINFO bmi;
    LONG dstWidth;
    LONG dstHeight;
    int oldStretchMode;
    int savedDc = 0;
    int stretchRet;

    if (pHandle == NULL || PaintDC == NULL)
        return PVC_INVALID_HANDLE;

    if (!pHandle->Decoded || pHandle->Pixels == NULL)
        return PVC_INVALID_HANDLE;

    dstWidth = pHandle->StretchWidth != 0 ? pHandle->StretchWidth : (LONG)pHandle->ImageInfo.Width;
    dstHeight = pHandle->StretchHeight != 0 ? pHandle->StretchHeight : (LONG)pHandle->ImageInfo.Height;
    if (dstWidth == 0 || dstHeight == 0)
        return PVC_INVALID_DIMENSIONS;

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = pHandle->ImageInfo.Width;
    bmi.bmiHeader.biHeight = -(LONG)pHandle->ImageInfo.Height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = pHandle->ImageInfo.BytesPerLine * pHandle->ImageInfo.Height;

    if (rect != NULL)
    {
        savedDc = SaveDC(PaintDC);
        IntersectClipRect(PaintDC, rect->left, rect->top, rect->right, rect->bottom);
    }

    oldStretchMode = SetStretchBltMode(PaintDC, pHandle->StretchMode == PV_STRETCH_NO ? COLORONCOLOR : (int)pHandle->StretchMode);
    if (pHandle->StretchMode == HALFTONE)
        SetBrushOrgEx(PaintDC, 0, 0, NULL);

    stretchRet = StretchDIBits(PaintDC, X, Y, dstWidth, dstHeight, 0, 0,
                               pHandle->ImageInfo.Width, pHandle->ImageInfo.Height,
                               pHandle->Pixels, &bmi, DIB_RGB_COLORS, SRCCOPY);

    SetStretchBltMode(PaintDC, oldStretchMode);
    if (savedDc != 0)
        RestoreDC(PaintDC, savedDc);

    return stretchRet == GDI_ERROR ? PVC_GDI_ERROR : PVC_OK;
}

static const char* WINAPI PVGetErrorTextOpen(DWORD ErrorCode)
{
    switch (ErrorCode)
    {
    case PVC_OK:
        return "Open-source PictView backend completed successfully.";

    case PVC_UNSUP_FILE_TYPE:
    case PVC_UNKNOWN_FILE_STRUCT:
        return "Open-source PictView backend does not support this image format yet.";

    case PVC_CANNOT_OPEN_FILE:
        return "Open-source PictView backend could not open the image file.";

    case PVC_NO_MORE_IMAGES:
        return "Open-source PictView backend could not find the requested image frame.";

    case PVC_WRITING_ERROR:
        return "Open-source PictView backend cannot save this image yet.";

    case PVC_INVALID_HANDLE:
        return "Open-source PictView backend has no valid image handle.";

    case PVC_GDI_ERROR:
        return "Open-source PictView backend failed in a GDI drawing operation.";

    case PVC_OUT_OF_MEMORY:
        return "Open-source PictView backend ran out of memory.";

    default:
        return "Open-source PictView backend operation failed.";
    }
}

static PVCODE WINAPI PVOpenImageExOpen(LPPVHandle* Img, LPPVOpenImageExInfo pOpenExInfo, LPPVImageInfo pImgInfo, int Size)
{
    COpenImageBackendHandle* pHandle;
    PVCODE ret;

    if (Img != NULL)
        *Img = NULL;
    if (pImgInfo != NULL && Size > 0)
        memset(pImgInfo, 0, Size);
    if (Img == NULL || pOpenExInfo == NULL)
        return PVC_INVALID_HANDLE;

    pHandle = new COpenImageBackendHandle;
    if (pHandle == NULL)
        return PVC_OUT_OF_MEMORY;
    memset(pHandle, 0, sizeof(*pHandle));
    ResetHandleInfo(pHandle);

    if ((pOpenExInfo->Flags & PVOF_USERDEFINED_INPUT) != 0)
    {
        FreeOpenImageBackendHandle(pHandle);
        return PVC_UNSUP_FILE_TYPE;
    }

    if ((pOpenExInfo->Flags & PVOF_ATTACH_TO_HANDLE) != 0)
    {
        ret = DecodeBitmapHandle(pHandle, (HBITMAP)pOpenExInfo->Handle);
    }
    else
    {
        if (pOpenExInfo->FileName == NULL || *pOpenExInfo->FileName == 0)
        {
            FreeOpenImageBackendHandle(pHandle);
            return PVC_CANNOT_OPEN_FILE;
        }

        lstrcpynA(pHandle->FileName, pOpenExInfo->FileName, SizeOf(pHandle->FileName));
        ret = LoadFrameInfoFromFile(pHandle, 0);
    }

    if (ret != PVC_OK)
    {
        FreeOpenImageBackendHandle(pHandle);
        return ret;
    }

    *Img = (LPPVHandle)pHandle;
    return CopyImageInfoOut(pHandle->ImageInfo, pImgInfo, Size);
}

static PVCODE WINAPI PVSetBkHandleOpen(LPPVHandle Img, COLORREF BkColor)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);

    if (pHandle == NULL)
        return PVC_INVALID_HANDLE;

    pHandle->BkColor = BkColor;
    return PVC_OK;
}

static DWORD WINAPI PVGetDLLVersionOpen(void)
{
    return MAKELONG(0, 1);
}

static PVCODE WINAPI PVSetStretchParametersOpen(LPPVHandle Img, DWORD Width, DWORD Height, DWORD Mode)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);

    if (pHandle == NULL)
        return PVC_INVALID_HANDLE;

    pHandle->StretchWidth = (LONG)Width;
    pHandle->StretchHeight = (LONG)Height;
    pHandle->StretchMode = Mode;
    pHandle->ImageInfo.StretchedWidth = labs(pHandle->StretchWidth);
    pHandle->ImageInfo.StretchedHeight = labs(pHandle->StretchHeight);
    pHandle->ImageInfo.StretchMode = Mode;
    return PVC_OK;
}

static PVCODE WINAPI PVLoadFromClipboardOpen(LPPVHandle* Img, LPPVImageInfo pImgInfo, int Size)
{
    HANDLE hClipboardBitmap = NULL;
    HBITMAP hLocalBitmap = NULL;
    PVOpenImageExInfo openInfo;
    PVCODE ret;

    if (Img != NULL)
        *Img = NULL;
    if (pImgInfo != NULL && Size > 0)
        memset(pImgInfo, 0, Size);

    if (!OpenClipboard(NULL))
        return PVC_CANNOT_OPEN_FILE;

    hClipboardBitmap = GetClipboardData(CF_BITMAP);
    if (hClipboardBitmap != NULL)
        hLocalBitmap = (HBITMAP)CopyImage(hClipboardBitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    CloseClipboard();

    if (hLocalBitmap == NULL)
        return PVC_UNSUP_FILE_TYPE;

    memset(&openInfo, 0, sizeof(openInfo));
    openInfo.cbSize = sizeof(openInfo);
    openInfo.Flags = PVOF_ATTACH_TO_HANDLE;
    openInfo.Handle = hLocalBitmap;
    ret = PVOpenImageExOpen(Img, &openInfo, pImgInfo, Size);
    DeleteObject(hLocalBitmap);
    return ret;
}

static PVCODE WINAPI PVGetImageInfoOpen(LPPVHandle Img, LPPVImageInfo pImgInfo, int Size, int ImageIndex)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);
    PVCODE ret;
    LONG stretchWidth;
    LONG stretchHeight;
    DWORD stretchMode;

    if (pHandle == NULL)
    {
        if (pImgInfo != NULL && Size > 0)
            memset(pImgInfo, 0, Size);
        return PVC_INVALID_HANDLE;
    }

    if (ImageIndex < 0)
        ImageIndex = 0;

    stretchWidth = pHandle->StretchWidth;
    stretchHeight = pHandle->StretchHeight;
    stretchMode = pHandle->StretchMode;

    if (!pHandle->BitmapSource && (DWORD)ImageIndex != pHandle->ImageInfo.CurrentImage)
    {
        ret = LoadFrameInfoFromFile(pHandle, (DWORD)ImageIndex);
        if (ret != PVC_OK)
            return ret;

        pHandle->StretchWidth = stretchWidth;
        pHandle->StretchHeight = stretchHeight;
        pHandle->StretchMode = stretchMode;
        pHandle->ImageInfo.StretchedWidth = labs(stretchWidth);
        pHandle->ImageInfo.StretchedHeight = labs(stretchHeight);
        pHandle->ImageInfo.StretchMode = stretchMode;
    }
    else
    {
        pHandle->ImageInfo.CurrentImage = ImageIndex;
    }

    return CopyImageInfoOut(pHandle->ImageInfo, pImgInfo, Size);
}

static PVCODE WINAPI PVSetParamOpen(LPPVHandle Img)
{
    return Img != NULL ? PVC_OK : PVC_INVALID_HANDLE;
}

static PVCODE WINAPI PVGetHandles2Open(LPPVHandle Img, LPPVImageHandles* pHandles)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);

    if (pHandles != NULL)
        *pHandles = NULL;
    if (pHandle == NULL || pHandles == NULL)
        return PVC_INVALID_HANDLE;
    if (!pHandle->Decoded)
        return PVC_INVALID_HANDLE;

    *pHandles = &pHandle->ImageHandles;
    return PVC_OK;
}

static PVCODE WINAPI PVSaveImageOpen(LPPVHandle Img, const char* OutFName, LPPVSaveImageInfo pSii, TProgressProc Progress, void* AppSpecific, int ImageIndex)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);
    COpenImageSurface sourceSurface;
    COpenImageSurface outputSurface;
    PVCODE ret;
    DWORD bytesPerPixel;
    BOOL userDefinedOutput;

    if (pHandle == NULL || pSii == NULL)
        return PVC_INVALID_HANDLE;

    userDefinedOutput = (pSii->Flags & PVSF_USERDEFINED_OUTPUT) != 0;
    if (!IsOutputFormatSupported(pSii->Format, pSii->Colors, pSii->ColorModel, userDefinedOutput))
        return PVC_UNSUP_OUT_PARAMS;

    if (!userDefinedOutput && (OutFName == NULL || *OutFName == 0))
        return PVC_ERROR_CREATING_FILE;

    ret = EnsureDecodedFrame(pHandle, ImageIndex < 0 ? pHandle->ImageInfo.CurrentImage : (DWORD)ImageIndex);
    if (ret != PVC_OK)
        return ret;

    InitSurface(&sourceSurface);
    InitSurface(&outputSurface);

    bytesPerPixel = userDefinedOutput && pSii->Colors == PV_COLOR_TC32 ? 4 : 3;
    ret = CreateSurfaceFromHandle(pHandle, &sourceSurface, 3);
    if (ret == PVC_OK)
        ret = CreateTransformedSurface(&sourceSurface, pSii, &outputSurface, bytesPerPixel);
    if (ret == PVC_OK)
    {
        if (userDefinedOutput)
            ret = SaveSurfaceToRawOutput(&outputSurface, pSii, Progress, AppSpecific);
        else
            ret = SaveSurfaceToWicFile(&outputSurface, OutFName, pSii, Progress, AppSpecific);
    }

    ReleaseSurface(&outputSurface);
    ReleaseSurface(&sourceSurface);
    return ret;
}

static PVCODE WINAPI PVChangeImageOpen(LPPVHandle Img, DWORD Flags)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);
    PVCODE ret;

    if (pHandle == NULL)
        return PVC_INVALID_HANDLE;

    ret = EnsureDecodedFrame(pHandle, pHandle->ImageInfo.CurrentImage);
    if (ret != PVC_OK)
        return ret;

    switch (Flags)
    {
    case PVCF_ROTATE90CW:
        return RotateDecodedImage(pHandle, TRUE);

    case PVCF_ROTATE90CCW:
        return RotateDecodedImage(pHandle, FALSE);

    default:
        return PVC_INVALID_HANDLE;
    }
}

static DWORD WINAPI PVIsOutCombSupportedOpen(int Fmt, int Compr, int Colors, int ColorModel)
{
    BOOL userDefinedOutput = Fmt == PVF_RAW;

    return IsOutputFormatSupported(Fmt, Colors, ColorModel, userDefinedOutput) ? PVCS_DEFAULT : (DWORD)-1;
}

static PVCODE WINAPI PVReadImageSequenceOpen(LPPVHandle Img, LPPVImageSequence* ppSeq)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);
    PVCODE ret;

    if (ppSeq != NULL)
        *ppSeq = NULL;
    if (pHandle == NULL || ppSeq == NULL)
        return PVC_INVALID_HANDLE;
    if ((pHandle->ImageInfo.Flags & PVFF_IMAGESEQUENCE) == 0 || pHandle->ImageInfo.Format != PVF_GIF)
        return PVC_INVALID_HANDLE;

    ret = BuildGifSequence(pHandle);
    if (ret != PVC_OK)
        return ret;

    *ppSeq = pHandle->Sequence;
    return *ppSeq != NULL ? PVC_OK : PVC_INVALID_HANDLE;
}

static PVCODE WINAPI PVCropImageOpen(LPPVHandle Img, int Left, int Top, int Width, int Height)
{
    COpenImageBackendHandle* pHandle = GetOpenImageBackendHandle(Img);
    COpenImageSurface sourceSurface;
    COpenImageSurface croppedSurface;
    PVSaveImageInfo sii;
    PVCODE ret;

    if (pHandle == NULL || Left < 0 || Top < 0 || Width <= 0 || Height <= 0)
        return PVC_INVALID_HANDLE;

    ret = EnsureDecodedFrame(pHandle, pHandle->ImageInfo.CurrentImage);
    if (ret != PVC_OK)
        return ret;

    memset(&sii, 0, sizeof(sii));
    sii.CropLeft = Left;
    sii.CropTop = Top;
    sii.CropWidth = Width;
    sii.CropHeight = Height;

    InitSurface(&sourceSurface);
    InitSurface(&croppedSurface);
    ret = CreateSurfaceFromHandle(pHandle, &sourceSurface, 3);
    if (ret == PVC_OK)
        ret = CreateTransformedSurface(&sourceSurface, &sii, &croppedSurface, 3);
    if (ret == PVC_OK)
        ret = ApplySurfaceToHandle(pHandle, &croppedSurface);

    ReleaseSurface(&croppedSurface);
    ReleaseSurface(&sourceSurface);
    return ret;
}
