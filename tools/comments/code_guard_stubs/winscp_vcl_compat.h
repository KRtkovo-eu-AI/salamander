#pragma once

#ifndef __BORLANDC__
#ifndef __fastcall
#define __fastcall
#endif
#ifndef __closure
#define __closure
#endif
#ifndef __published
#define __published public
#endif
#ifndef DYNAMIC
#define DYNAMIC
#endif
#ifndef PACKAGE
#define PACKAGE
#endif
#ifndef False
#define False false
#endif
#ifndef True
#define True true
#endif

#include <string>
#include <vector>
#include <stdexcept>
#include <windows.h>
#include <commdlg.h>

using AnsiString = std::string;
using UnicodeString = std::wstring;
using String = AnsiString;
using Char = char;
using Integer = int;
using Boolean = bool;

struct TObject { virtual ~TObject() = default; };
struct TPersistent : TObject {};
struct TComponent : TPersistent { explicit TComponent(TComponent* = nullptr) {} };
struct TControl : TComponent { using TComponent::TComponent; };
struct TWinControl : TControl { using TControl::TControl; };
struct TCustomForm : TWinControl { using TWinControl::TWinControl; };
struct TForm : TCustomForm { using TCustomForm::TCustomForm; int ShowModal() { return 0; } virtual void DoShow() {} };
struct TFrame : TCustomForm { using TCustomForm::TCustomForm; };
struct TApplication : TComponent {};
inline TApplication* Application = nullptr;

struct Exception : std::runtime_error {
    explicit Exception(const AnsiString& msg = AnsiString()) : std::runtime_error(msg) {}
    Exception(int, int = 0) : std::runtime_error("") {}
};
namespace Sysutils { using Exception = ::Exception; }

struct TStrings : TObject {};
struct TStringList : TStrings {};
namespace Classes { using TStrings = ::TStrings; }

struct TListItem; struct TTreeNode; struct TStatusPanel; struct TBasicAction;
struct TMessage {}; struct TPoint { int x = 0; int y = 0; }; struct TRect {};
struct TShiftState {}; enum TMouseButton { mbLeft, mbRight, mbMiddle };
struct TCreateParams {}; struct TWMKeyDown {}; struct TWMHelp {}; struct TWMContextMenu {};
struct TCMCancelMode {}; struct TCMDialogKey {};
using TNotifyEvent = void (__fastcall *)(TObject*);

struct TVarRec {}; struct PResStringRec__ {}; using PResStringRec = PResStringRec__*;
#endif
