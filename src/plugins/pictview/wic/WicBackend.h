// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <objbase.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <windows.h>

#include "../lib/PVW32DLL.h"
#include "../pictview.h"

namespace PictView::Wic
{
class Backend;

struct ImageHandle;

using Microsoft::WRL::ComPtr;

struct FrameData
{
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    ComPtr<IWICBitmapSource> colorConvertedSource;
    UINT width = 0;
    UINT height = 0;
    UINT stride = 0;
    std::vector<BYTE> pixels;
    std::vector<BYTE*> linePointers;
    std::vector<RGBQUAD> palette;
    BITMAPINFOHEADER bmi{};
    HBITMAP hbitmap = nullptr;
    bool decoded = false;
};

struct ImageHandle
{
    Backend* backend = nullptr;
    std::wstring fileName;
    DWORD openFlags = 0;
    std::vector<FrameData> frames;
    DWORD stretchWidth = 0;
    DWORD stretchHeight = 0;
    DWORD stretchMode = PV_STRETCH_NO;
    COLORREF background = RGB(0, 0, 0);
    PVImageInfo baseInfo{};
    PVImageHandles handles{};
};

/**
 * Lightweight RAII helper around CoInitialize/CoUninitialize.  The viewer
 * already initialises COM on most threads, but the WIC backend may also be
 * invoked from helper worker threads that do not call into COM yet.  We keep
 * this helper header-only to avoid the dependency on ATL.
 */
class ScopedCoInit
{
public:
    ScopedCoInit();
    ScopedCoInit(const ScopedCoInit&) = delete;
    ScopedCoInit& operator=(const ScopedCoInit&) = delete;
    ~ScopedCoInit();

    bool Succeeded() const
    {
        return m_hr == S_OK || m_hr == S_FALSE || m_hr == RPC_E_CHANGED_MODE;
    }

private:
    HRESULT m_hr;
    bool m_needUninit;
};

class Backend
{
public:
    Backend();
    Backend(const Backend&) = delete;
    Backend& operator=(const Backend&) = delete;

    static Backend& Instance();

    bool Populate(CPVW32DLL& table);

    IWICImagingFactory* Factory() const { return m_factory.Get(); }

private:
    static PVCODE WINAPI sPVOpenImageEx(LPPVHandle* Img, LPPVOpenImageExInfo pOpenExInfo, LPPVImageInfo pImgInfo, int size);
    static PVCODE WINAPI sPVCloseImage(LPPVHandle Img);
    static PVCODE WINAPI sPVReadImage2(LPPVHandle Img, HDC paintDC, RECT* dRect, TProgressProc progress, void* appSpecific,
                                       int imageIndex);
    static PVCODE WINAPI sPVDrawImage(LPPVHandle Img, HDC paintDC, int x, int y, LPRECT rect);
    static const char* WINAPI sPVGetErrorText(DWORD errorCode);
    static PVCODE WINAPI sPVSetBkHandle(LPPVHandle Img, COLORREF bkColor);
    static DWORD WINAPI sPVGetDLLVersion();
    static PVCODE WINAPI sPVSetStretchParameters(LPPVHandle Img, DWORD width, DWORD height, DWORD mode);
    static PVCODE WINAPI sPVLoadFromClipboard(LPPVHandle* Img, LPPVImageInfo pImgInfo, int size);
    static PVCODE WINAPI sPVGetImageInfo(LPPVHandle Img, LPPVImageInfo pImgInfo, int size, int imageIndex);
    static PVCODE WINAPI sPVSetParam(LPPVHandle Img);
    static PVCODE WINAPI sPVGetHandles2(LPPVHandle Img, LPPVImageHandles* pHandles);
    static PVCODE WINAPI sPVSaveImage(LPPVHandle Img, const char* outFileName, LPPVSaveImageInfo pSii, TProgressProc progress,
                                      void* appSpecific, int imageIndex);
    static PVCODE WINAPI sPVChangeImage(LPPVHandle Img, DWORD flags);
    static DWORD WINAPI sPVIsOutCombSupported(int format, int compression, int colors, int colorModel);
    static PVCODE WINAPI sPVReadImageSequence(LPPVHandle Img, LPPVImageSequence* seq);
    static PVCODE WINAPI sPVCropImage(LPPVHandle Img, int left, int top, int width, int height);
    static bool sGetRGBAtCursor(LPPVHandle Img, DWORD colors, int x, int y, RGBQUAD* rgb, int* index);
    static PVCODE sCalculateHistogram(LPPVHandle Img, const LPPVImageInfo info, LPDWORD luminosity, LPDWORD red, LPDWORD green,
                                      LPDWORD blue, LPDWORD rgb);
    static PVCODE sCreateThumbnail(LPPVHandle Img, LPPVSaveImageInfo sii, int imageIndex, DWORD imgWidth, DWORD imgHeight,
                                   int thumbWidth, int thumbHeight, CSalamanderThumbnailMakerAbstract* thumbMaker,
                                   DWORD thumbFlags, TProgressProc progressProc, void* progressProcArg);
    static PVCODE sSimplifyImageSequence(LPPVHandle Img, HDC dc, int screenWidth, int screenHeight, LPPVImageSequence& seq,
                                         const COLORREF& bgColor);

    static ImageHandle* FromHandle(LPPVHandle handle);

    ScopedCoInit m_comScope;
    ComPtr<IWICImagingFactory> m_factory;
};

} // namespace PictView::Wic
