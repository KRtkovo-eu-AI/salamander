// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "parser.h"

class CParserTagLib : public CParserInterface
{
public:
    CParserTagLib();

    virtual CParserResultEnum OpenFile(const char* fileName);
    virtual CParserResultEnum CloseFile();
    virtual CParserResultEnum GetFileInfo(COutputInterface* output);

private:
    std::wstring FileName;
};
