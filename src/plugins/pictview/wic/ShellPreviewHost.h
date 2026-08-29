// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "WicBackend.h"

namespace PictView::Wic
{
bool IsStlExtension(const std::wstring& path);

bool HandleHasInteractivePreview(const ImageHandle& handle);
HRESULT TryOpenInteractivePreview(ImageHandle& handle);
HRESULT ShowInteractivePreview(ImageHandle& handle, HWND hwnd, const RECT& rect, COLORREF background);
HRESULT ResizeInteractivePreview(ImageHandle& handle, const RECT& rect);
void ReleaseInteractivePreview(ImageHandle& handle);

} // namespace PictView::Wic
