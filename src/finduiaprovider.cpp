// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-07 | Expose Find results through virtualized UI Automation. | Bound UIA tree enumeration while keeping every result accessible. | https://github.com/KRtkovo-eu-AI/salamander/issues/819 | GPT-6 (Codex) |

#include "precomp.h"
#include "finduia.h"

#include <objbase.h>
#include <UIAutomation.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <new>

namespace
{
enum class ElementKind
{
    List,
    Row,
    Cell,
    Column
};

constexpr int ColumnCount = 6;

template <class T>
HRESULT Store(T* out, T value)
{
    if (out == nullptr)
        return E_POINTER;
    *out = value;
    return S_OK;
}

HRESULT StoreText(VARIANT* value, const std::wstring& text)
{
    value->bstrVal = SysAllocStringLen(text.data(), static_cast<UINT>(text.size()));
    if (value->bstrVal == nullptr)
        return E_OUTOFMEMORY;
    value->vt = VT_BSTR;
    return S_OK;
}

HRESULT StoreBool(VARIANT* value, bool flag)
{
    value->vt = VT_BOOL;
    value->boolVal = flag ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

bool Contains(const RECT& rect, double x, double y)
{
    return x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom;
}

long Pixels(long long value)
{
    return static_cast<long>((std::clamp)(value, static_cast<long long>((std::numeric_limits<long>::min)()),
                                         static_cast<long long>((std::numeric_limits<long>::max)())));
}

class FindUiaProvider final : public IRawElementProviderSimple,
                              public IRawElementProviderFragment,
                              public IRawElementProviderFragmentRoot,
                              public IItemContainerProvider,
                              public IVirtualizedItemProvider,
                              public IScrollItemProvider,
                              public ISelectionProvider,
                              public ISelectionItemProvider,
                              public IInvokeProvider,
                              public IGridProvider,
                              public ITableProvider,
                              public IGridItemProvider,
                              public ITableItemProvider,
                              public IScrollProvider
{
    std::atomic<ULONG> References{1};
    const std::shared_ptr<CFindUiaState> State;
    const ElementKind Kind;
    const UINT_PTR Revision;
    const int Row;
    const int Column;
    IUnknown* Marshaler = nullptr;

    static FindUiaProvider* Create(const std::shared_ptr<CFindUiaState>& state, ElementKind kind,
                                   UINT_PTR revision = 0, int row = -1, int column = -1);

    std::shared_ptr<const CFindUiaView> View() const
    {
        auto view = State->ReadView();
        if (!view || view->Window == nullptr ||
            (Kind != ElementKind::List && view->Revision != Revision) || Row >= view->Count)
            return {};
        return view;
    }

    template<class Interface>
    HRESULT Element(Interface** result, ElementKind kind, UINT_PTR revision = 0, int row = -1, int column = -1)
    {
        FindUiaProvider* provider = Create(State, kind, revision, row, column);
        if (!provider)
            return E_OUTOFMEMORY;
        *result = static_cast<Interface*>(provider);
        return S_OK;
    }

    HRESULT Action(CFindUiaAction action, int index)
    {
        const auto view = View();
        return view ? State->Act(view->Revision, action, index) : UIA_E_ELEMENTNOTAVAILABLE;
    }

    bool VisibleRow(const CFindUiaView& view) const
    {
        return Row >= view.First && Row < view.Last;
    }

    RECT Geometry(const CFindUiaView& view) const
    {
        if (Kind == ElementKind::List)
            return view.Bounds;
        RECT rect{};
        if (Kind == ElementKind::Column)
            rect = view.Columns[Column];
        else if (VisibleRow(view))
        {
            const long long top = static_cast<long long>(view.FirstRow.top) +
                                  static_cast<long long>(Row - view.First) * view.RowHeight;
            rect = {view.FirstRow.left, Pixels(top), view.FirstRow.right, Pixels(top + view.RowHeight)};
            if (Kind == ElementKind::Cell)
            {
                rect.left = view.Columns[Column].left;
                rect.right = view.Columns[Column].right;
            }
        }
        RECT clipped{};
        IntersectRect(&clipped, &rect, &view.Bounds);
        return clipped;
    }

    HRESULT ColumnHeaders(const CFindUiaView& view, SAFEARRAY** result, bool all);
    HRESULT ScrollAxis(const CFindUiaView& view, ScrollAmount amount, bool vertical);

public:
    FindUiaProvider(const std::shared_ptr<CFindUiaState>& state, ElementKind kind, UINT_PTR revision,
                    int row, int column)
        : State(state), Kind(kind), Revision(revision), Row(row), Column(column)
    {
    }

    ~FindUiaProvider()
    {
        if (Marshaler)
            Marshaler->Release();
    }

    static IRawElementProviderSimple* CreateRoot(const std::shared_ptr<CFindUiaState>& state)
    {
        return Create(state, ElementKind::List);
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** result) override;
    ULONG STDMETHODCALLTYPE AddRef() override { return ++References; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG remaining = --References;
        if (!remaining)
            delete this;
        return remaining;
    }

    HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* value) override;
    HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID id, IUnknown** result) override;
    HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id, VARIANT* value) override;
    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** result) override;

    HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, IRawElementProviderFragment** result) override;
    HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** result) override;
    HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* result) override;
    HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** result) override { return Store(result, static_cast<SAFEARRAY*>(nullptr)); }
    HRESULT STDMETHODCALLTYPE SetFocus() override
    {
        return Action(CFindUiaAction::Focus, Kind == ElementKind::List ? -1 : Row);
    }
    HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** result) override;
    HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** result) override;
    HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment** result) override;

    HRESULT STDMETHODCALLTYPE FindItemByProperty(IRawElementProviderSimple* after, PROPERTYID property,
                                                  VARIANT value, IRawElementProviderSimple** result) override;
    HRESULT STDMETHODCALLTYPE Realize() override { return ScrollIntoView(); }
    HRESULT STDMETHODCALLTYPE ScrollIntoView() override { return Action(CFindUiaAction::ScrollIntoView, Row); }

    HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY** result) override;
    HRESULT STDMETHODCALLTYPE get_CanSelectMultiple(BOOL* value) override { return Store(value, TRUE); }
    HRESULT STDMETHODCALLTYPE get_IsSelectionRequired(BOOL* value) override { return Store(value, FALSE); }
    HRESULT STDMETHODCALLTYPE Select() override { return Action(CFindUiaAction::Select, Row); }
    HRESULT STDMETHODCALLTYPE AddToSelection() override { return Action(CFindUiaAction::AddSelection, Row); }
    HRESULT STDMETHODCALLTYPE RemoveFromSelection() override { return Action(CFindUiaAction::RemoveSelection, Row); }
    HRESULT STDMETHODCALLTYPE get_IsSelected(BOOL* value) override;
    HRESULT STDMETHODCALLTYPE get_SelectionContainer(IRawElementProviderSimple** result) override;
    HRESULT STDMETHODCALLTYPE Invoke() override
    {
        return Kind == ElementKind::Column ? Action(CFindUiaAction::SortColumn, Column) :
                                             Action(CFindUiaAction::Invoke, Row);
    }

    HRESULT STDMETHODCALLTYPE GetItem(int row, int column, IRawElementProviderSimple** result) override;
    HRESULT STDMETHODCALLTYPE get_RowCount(int* value) override;
    HRESULT STDMETHODCALLTYPE get_ColumnCount(int* value) override { return Store(value, ColumnCount); }
    HRESULT STDMETHODCALLTYPE GetRowHeaders(SAFEARRAY** result) override { return Store(result, static_cast<SAFEARRAY*>(nullptr)); }
    HRESULT STDMETHODCALLTYPE GetColumnHeaders(SAFEARRAY** result) override;
    HRESULT STDMETHODCALLTYPE get_RowOrColumnMajor(RowOrColumnMajor* value) override { return Store(value, RowOrColumnMajor_RowMajor); }
    HRESULT STDMETHODCALLTYPE get_Row(int* value) override { return Store(value, Row); }
    HRESULT STDMETHODCALLTYPE get_Column(int* value) override { return Store(value, Column); }
    HRESULT STDMETHODCALLTYPE get_RowSpan(int* value) override { return Store(value, 1); }
    HRESULT STDMETHODCALLTYPE get_ColumnSpan(int* value) override { return Store(value, 1); }
    HRESULT STDMETHODCALLTYPE get_ContainingGrid(IRawElementProviderSimple** result) override { return get_SelectionContainer(result); }
    HRESULT STDMETHODCALLTYPE GetRowHeaderItems(SAFEARRAY** result) override { return Store(result, static_cast<SAFEARRAY*>(nullptr)); }
    HRESULT STDMETHODCALLTYPE GetColumnHeaderItems(SAFEARRAY** result) override;

    HRESULT STDMETHODCALLTYPE Scroll(ScrollAmount horizontal, ScrollAmount vertical) override;
    HRESULT STDMETHODCALLTYPE SetScrollPercent(double horizontal, double vertical) override;
    HRESULT STDMETHODCALLTYPE get_HorizontalScrollPercent(double* value) override;
    HRESULT STDMETHODCALLTYPE get_VerticalScrollPercent(double* value) override;
    HRESULT STDMETHODCALLTYPE get_HorizontalViewSize(double* value) override;
    HRESULT STDMETHODCALLTYPE get_VerticalViewSize(double* value) override;
    HRESULT STDMETHODCALLTYPE get_HorizontallyScrollable(BOOL* value) override;
    HRESULT STDMETHODCALLTYPE get_VerticallyScrollable(BOOL* value) override;
};

#ifdef new
#undef new
#define RESTORE_FIND_UIA_DEBUG_NEW
#endif
FindUiaProvider* FindUiaProvider::Create(const std::shared_ptr<CFindUiaState>& state, ElementKind kind,
                                         UINT_PTR revision, int row, int column)
{
    auto* provider = new (std::nothrow) FindUiaProvider(state, kind, revision, row, column);
    if (provider)
        CoCreateFreeThreadedMarshaler(static_cast<IRawElementProviderSimple*>(provider), &provider->Marshaler);
    return provider;
}
#ifdef RESTORE_FIND_UIA_DEBUG_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_FIND_UIA_DEBUG_NEW
#endif

HRESULT FindUiaProvider::QueryInterface(REFIID id, void** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    if (id == __uuidof(IUnknown) || id == __uuidof(IRawElementProviderSimple))
        *result = static_cast<IRawElementProviderSimple*>(this);
    else if (id == __uuidof(IRawElementProviderFragment))
        *result = static_cast<IRawElementProviderFragment*>(this);
    else if (Kind == ElementKind::List)
    {
        if (id == __uuidof(IRawElementProviderFragmentRoot))
            *result = static_cast<IRawElementProviderFragmentRoot*>(this);
        else if (id == __uuidof(IItemContainerProvider))
            *result = static_cast<IItemContainerProvider*>(this);
        else if (id == __uuidof(ISelectionProvider))
            *result = static_cast<ISelectionProvider*>(this);
        else if (id == __uuidof(IGridProvider))
            *result = static_cast<IGridProvider*>(this);
        else if (id == __uuidof(ITableProvider))
            *result = static_cast<ITableProvider*>(this);
        else if (id == __uuidof(IScrollProvider))
            *result = static_cast<IScrollProvider*>(this);
    }
    else if (Kind == ElementKind::Row || Kind == ElementKind::Cell)
    {
        if (id == __uuidof(IVirtualizedItemProvider))
            *result = static_cast<IVirtualizedItemProvider*>(this);
        else if (id == __uuidof(IScrollItemProvider))
            *result = static_cast<IScrollItemProvider*>(this);
        else if (Kind == ElementKind::Row && id == __uuidof(ISelectionItemProvider))
            *result = static_cast<ISelectionItemProvider*>(this);
        else if (Kind == ElementKind::Row && id == __uuidof(IInvokeProvider))
            *result = static_cast<IInvokeProvider*>(this);
        else if (Kind == ElementKind::Cell && id == __uuidof(IGridItemProvider))
            *result = static_cast<IGridItemProvider*>(this);
        else if (Kind == ElementKind::Cell && id == __uuidof(ITableItemProvider))
            *result = static_cast<ITableItemProvider*>(this);
    }
    else if (Kind == ElementKind::Column && id == __uuidof(IInvokeProvider) && Column < 5)
        *result = static_cast<IInvokeProvider*>(this);

    if (*result)
    {
        AddRef();
        return S_OK;
    }
    return id == __uuidof(IMarshal) && Marshaler ? Marshaler->QueryInterface(id, result) : E_NOINTERFACE;
}

HRESULT FindUiaProvider::get_ProviderOptions(ProviderOptions* value)
{
    return Store(value, static_cast<ProviderOptions>(ProviderOptions_ServerSideProvider |
                                                       (Marshaler ? ProviderOptions_UseComThreading : 0)));
}

HRESULT FindUiaProvider::GetPatternProvider(PATTERNID id, IUnknown** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    const auto query = [this, result](REFIID iid) {
        const HRESULT hr = QueryInterface(iid, reinterpret_cast<void**>(result));
        return hr == E_NOINTERFACE ? S_OK : hr;
    };
    switch (id)
    {
    case UIA_ItemContainerPatternId: return query(__uuidof(IItemContainerProvider));
    case UIA_VirtualizedItemPatternId: return query(__uuidof(IVirtualizedItemProvider));
    case UIA_ScrollItemPatternId: return query(__uuidof(IScrollItemProvider));
    case UIA_SelectionPatternId: return query(__uuidof(ISelectionProvider));
    case UIA_SelectionItemPatternId: return query(__uuidof(ISelectionItemProvider));
    case UIA_InvokePatternId: return query(__uuidof(IInvokeProvider));
    case UIA_GridPatternId: return query(__uuidof(IGridProvider));
    case UIA_TablePatternId: return query(__uuidof(ITableProvider));
    case UIA_GridItemPatternId: return query(__uuidof(IGridItemProvider));
    case UIA_TableItemPatternId: return query(__uuidof(ITableItemProvider));
    case UIA_ScrollPatternId: return query(__uuidof(IScrollProvider));
    default: return S_OK;
    }
}

HRESULT FindUiaProvider::GetPropertyValue(PROPERTYID id, VARIANT* value)
{
    if (!value)
        return E_POINTER;
    VariantInit(value);
    const auto view = View();
    if (!view)
        return UIA_E_ELEMENTNOTAVAILABLE;
    switch (id)
    {
    case UIA_NamePropertyId:
    case UIA_HelpTextPropertyId:
    {
        if (Kind == ElementKind::List)
            return id == UIA_NamePropertyId && !view->Name.empty() ? StoreText(value, view->Name) : S_OK;
        if (id == UIA_HelpTextPropertyId && Kind != ElementKind::Row)
            return S_OK;
        std::wstring text;
        const int column = Kind == ElementKind::Row ? (id == UIA_NamePropertyId ? 0 : 1) : Column;
        const HRESULT hr = State->ReadText(view->Revision, Row, column, text);
        return SUCCEEDED(hr) ? StoreText(value, text) : hr;
    }
    case UIA_ControlTypePropertyId:
        value->vt = VT_I4;
        switch (Kind)
        {
        case ElementKind::List: value->lVal = UIA_DataGridControlTypeId; break;
        case ElementKind::Row: value->lVal = UIA_DataItemControlTypeId; break;
        case ElementKind::Cell: value->lVal = UIA_TextControlTypeId; break;
        case ElementKind::Column: value->lVal = UIA_HeaderItemControlTypeId; break;
        }
        return S_OK;
    case UIA_AutomationIdPropertyId:
        if (Kind == ElementKind::Row)
        {
            wchar_t idText[80];
            swprintf_s(idText, L"FindRow-%llu-%d", static_cast<unsigned long long>(Revision), Row);
            value->bstrVal = SysAllocString(idText);
            if (!value->bstrVal)
                return E_OUTOFMEMORY;
            value->vt = VT_BSTR;
        }
        return S_OK;
    case UIA_IsControlElementPropertyId:
    case UIA_IsContentElementPropertyId:
        return StoreBool(value, true);
    case UIA_IsEnabledPropertyId:
        for (HWND window = view->Window; window != NULL; window = GetAncestor(window, GA_PARENT))
        {
            if (!IsWindowEnabled(window))
                return StoreBool(value, false);
        }
        return StoreBool(value, true);
    case UIA_IsKeyboardFocusablePropertyId:
        return StoreBool(value, Kind == ElementKind::List || Kind == ElementKind::Row);
    case UIA_HasKeyboardFocusPropertyId:
        return StoreBool(value, view->HasFocus &&
                                (Kind == ElementKind::List ? view->Focus < 0 :
                                 Kind == ElementKind::Row && view->Focus == Row));
    case UIA_IsOffscreenPropertyId:
    {
        const RECT rect = Geometry(*view);
        return StoreBool(value, IsRectEmpty(&rect) != FALSE);
    }
    case UIA_SelectionItemIsSelectedPropertyId:
        if (Kind == ElementKind::Row)
        {
            BOOL selected = FALSE;
            const HRESULT hr = get_IsSelected(&selected);
            return SUCCEEDED(hr) ? StoreBool(value, selected != FALSE) : hr;
        }
        return S_OK;
    default:
        return S_OK;
    }
}

HRESULT FindUiaProvider::get_HostRawElementProvider(IRawElementProviderSimple** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    if (Kind != ElementKind::List)
        return S_OK;
    const auto view = View();
    return view ? UiaHostProviderFromHwnd(view->Window, result) : UIA_E_ELEMENTNOTAVAILABLE;
}

HRESULT FindUiaProvider::Navigate(NavigateDirection direction, IRawElementProviderFragment** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    const auto view = View();
    if (!view)
        return UIA_E_ELEMENTNOTAVAILABLE;
    switch (Kind)
    {
    case ElementKind::List:
        if (view->First < view->Last)
        {
            if (direction == NavigateDirection_FirstChild)
                return Element(result, ElementKind::Row, view->Revision, view->First);
            if (direction == NavigateDirection_LastChild)
                return Element(result, ElementKind::Row, view->Revision, view->Last - 1);
        }
        break;
    case ElementKind::Row:
        if (direction == NavigateDirection_Parent)
            return Element(result, ElementKind::List);
        if (direction == NavigateDirection_NextSibling && VisibleRow(*view) && Row + 1 < view->Last)
            return Element(result, ElementKind::Row, Revision, Row + 1);
        if (direction == NavigateDirection_PreviousSibling && VisibleRow(*view) && Row > view->First)
            return Element(result, ElementKind::Row, Revision, Row - 1);
        if (VisibleRow(*view) && direction == NavigateDirection_FirstChild)
            return Element(result, ElementKind::Cell, Revision, Row, 0);
        if (VisibleRow(*view) && direction == NavigateDirection_LastChild)
            return Element(result, ElementKind::Cell, Revision, Row, ColumnCount - 1);
        break;
    case ElementKind::Cell:
        if (direction == NavigateDirection_Parent)
            return Element(result, ElementKind::Row, Revision, Row);
        if (VisibleRow(*view) && direction == NavigateDirection_NextSibling && Column + 1 < ColumnCount)
            return Element(result, ElementKind::Cell, Revision, Row, Column + 1);
        if (VisibleRow(*view) && direction == NavigateDirection_PreviousSibling && Column > 0)
            return Element(result, ElementKind::Cell, Revision, Row, Column - 1);
        break;
    case ElementKind::Column:
        if (direction == NavigateDirection_Parent)
            return Element(result, ElementKind::List);
        break;
    }
    return S_OK;
}

HRESULT FindUiaProvider::GetRuntimeId(SAFEARRAY** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    if (Kind == ElementKind::List)
        return S_OK;
    const auto revision = static_cast<unsigned long long>(Revision);
    LONG values[] = {UiaAppendRuntimeId, static_cast<LONG>(Kind), static_cast<LONG>(revision),
                     static_cast<LONG>(revision >> 32), Row, Column};
    SAFEARRAY* array = SafeArrayCreateVector(VT_I4, 0, _countof(values));
    if (!array)
        return E_OUTOFMEMORY;
    for (LONG i = 0; i < static_cast<LONG>(_countof(values)); ++i)
    {
        const HRESULT hr = SafeArrayPutElement(array, &i, &values[i]);
        if (FAILED(hr))
        {
            SafeArrayDestroy(array);
            return hr;
        }
    }
    *result = array;
    return S_OK;
}

HRESULT FindUiaProvider::get_BoundingRectangle(UiaRect* result)
{
    if (!result)
        return E_POINTER;
    const auto view = View();
    if (!view)
        return UIA_E_ELEMENTNOTAVAILABLE;
    const RECT rect = Geometry(*view);
    *result = {static_cast<double>(rect.left), static_cast<double>(rect.top),
               static_cast<double>(rect.right - rect.left), static_cast<double>(rect.bottom - rect.top)};
    return S_OK;
}

HRESULT FindUiaProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    if (!View())
        return UIA_E_ELEMENTNOTAVAILABLE;
    if (Kind == ElementKind::List)
    {
        *result = static_cast<IRawElementProviderFragmentRoot*>(this);
        AddRef();
        return S_OK;
    }
    return Element(result, ElementKind::List);
}

HRESULT FindUiaProvider::ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    const auto view = View();
    if (!view)
        return UIA_E_ELEMENTNOTAVAILABLE;
    if (!Contains(view->Bounds, x, y))
        return S_OK;
    for (int column = 0; column < ColumnCount; ++column)
    {
        if (Contains(view->Columns[column], x, y))
            return Element(result, ElementKind::Column, view->Revision, -1, column);
    }
    if (view->First < view->Last && view->RowHeight > 0 && y >= view->FirstRow.top)
    {
        const int row = view->First + static_cast<int>((y - view->FirstRow.top) / view->RowHeight);
        if (row < view->Last)
        {
            for (int col = 0; col < ColumnCount; ++col)
            {
                if (x >= view->Columns[col].left && x < view->Columns[col].right)
                    return Element(result, ElementKind::Cell, view->Revision, row, col);
            }
            return Element(result, ElementKind::Row, view->Revision, row);
        }
    }
    return Element(result, ElementKind::List);
}

HRESULT FindUiaProvider::GetFocus(IRawElementProviderFragment** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    const auto view = View();
    if (!view)
        return UIA_E_ELEMENTNOTAVAILABLE;
    if (!view->HasFocus)
        return S_OK;
    if (view->Focus >= 0 && view->Focus < view->Count)
        return Element(result, ElementKind::Row, view->Revision, view->Focus);
    *result = static_cast<IRawElementProviderFragment*>(this);
    AddRef();
    return S_OK;
}

bool DecodeRowId(const VARIANT& value, const CFindUiaView& view, int& index)
{
    constexpr wchar_t prefix[] = L"FindRow-";
    if (value.vt != VT_BSTR || !value.bstrVal ||
        wcsncmp(value.bstrVal, prefix, _countof(prefix) - 1) != 0)
        return false;
    const wchar_t* revisionText = value.bstrVal + _countof(prefix) - 1;
    wchar_t* separator = nullptr;
    const auto revision = wcstoull(revisionText, &separator, 10);
    if (separator == revisionText || *separator != L'-' || revision != view.Revision)
        return false;
    const wchar_t* rowText = separator + 1;
    wchar_t* end = nullptr;
    const long row = wcstol(rowText, &end, 10);
    if (end == rowText || *end != L'\0' || row < 0 || row >= view.Count)
        return false;
    index = static_cast<int>(row);
    return true;
}

HRESULT FindUiaProvider::FindItemByProperty(IRawElementProviderSimple* after, PROPERTYID property,
                                             VARIANT value, IRawElementProviderSimple** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    const auto view = View();
    if (!view)
        return UIA_E_ELEMENTNOTAVAILABLE;
    int start = 0;
    if (after)
    {
        VARIANT previous{};
        const HRESULT hr = after->GetPropertyValue(UIA_AutomationIdPropertyId, &previous);
        int row = -1;
        const bool valid = SUCCEEDED(hr) && DecodeRowId(previous, *view, row);
        VariantClear(&previous);
        if (!valid)
            return E_INVALIDARG;
        start = row + 1;
    }
    int found = -1;
    HRESULT hr = S_OK;
    switch (property)
    {
    case 0:
        found = start;
        break;
    case UIA_NamePropertyId:
        if (value.vt != VT_BSTR || !value.bstrVal)
            return E_INVALIDARG;
        hr = State->FindName(view->Revision, start, value.bstrVal, &found);
        break;
    case UIA_AutomationIdPropertyId:
        if (!DecodeRowId(value, *view, found))
            return S_OK;
        break;
    case UIA_SelectionItemIsSelectedPropertyId:
        if (value.vt != VT_BOOL || value.boolVal != VARIANT_TRUE)
            return E_INVALIDARG;
        hr = State->FindSelected(view->Revision, start, &found);
        break;
    default:
        return E_INVALIDARG;
    }
    if (FAILED(hr) || found < start || found >= view->Count)
        return hr;
    return Element(result, ElementKind::Row, view->Revision, found);
}

HRESULT FindUiaProvider::GetSelection(SAFEARRAY** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    const auto view = View();
    if (!view)
        return UIA_E_ELEMENTNOTAVAILABLE;
    std::vector<int> rows;
    HRESULT hr = State->ReadSelection(view->Revision, rows);
    if (FAILED(hr))
        return hr;
    SAFEARRAY* array = SafeArrayCreateVector(VT_UNKNOWN, 0, static_cast<ULONG>(rows.size()));
    if (!array)
        return E_OUTOFMEMORY;
    for (LONG i = 0; i < static_cast<LONG>(rows.size()); ++i)
    {
        IRawElementProviderSimple* item = nullptr;
        hr = Element(&item, ElementKind::Row, view->Revision, rows[i]);
        if (SUCCEEDED(hr))
        {
            hr = SafeArrayPutElement(array, &i, item);
            item->Release();
        }
        if (FAILED(hr))
        {
            SafeArrayDestroy(array);
            return hr;
        }
    }
    *result = array;
    return S_OK;
}

HRESULT FindUiaProvider::get_IsSelected(BOOL* value)
{
    if (!value)
        return E_POINTER;
    const auto view = View();
    if (!view)
        return UIA_E_ELEMENTNOTAVAILABLE;
    if (VisibleRow(*view))
    {
        const size_t offset = static_cast<size_t>(Row - view->First);
        if (offset < view->Selected.size())
            return Store(value, view->Selected[offset] ? TRUE : FALSE);
    }
    return State->IsSelected(view->Revision, Row, value);
}

HRESULT FindUiaProvider::get_SelectionContainer(IRawElementProviderSimple** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    return View() ? Element(result, ElementKind::List) : UIA_E_ELEMENTNOTAVAILABLE;
}

HRESULT FindUiaProvider::GetItem(int row, int column, IRawElementProviderSimple** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    const auto view = View();
    if (!view)
        return UIA_E_ELEMENTNOTAVAILABLE;
    if (row < 0 || row >= view->Count || column < 0 || column >= ColumnCount)
        return E_INVALIDARG;
    return Element(result, ElementKind::Cell, view->Revision, row, column);
}

HRESULT FindUiaProvider::get_RowCount(int* value)
{
    const auto view = View();
    return view ? Store(value, view->Count) : UIA_E_ELEMENTNOTAVAILABLE;
}

HRESULT FindUiaProvider::ColumnHeaders(const CFindUiaView& view, SAFEARRAY** result, bool all)
{
    const LONG count = all ? ColumnCount : 1;
    SAFEARRAY* array = SafeArrayCreateVector(VT_UNKNOWN, 0, count);
    if (!array)
        return E_OUTOFMEMORY;
    for (LONG i = 0; i < count; ++i)
    {
        IRawElementProviderSimple* header = nullptr;
        HRESULT hr = Element(&header, ElementKind::Column, view.Revision, -1, all ? i : Column);
        if (SUCCEEDED(hr))
        {
            hr = SafeArrayPutElement(array, &i, header);
            header->Release();
        }
        if (FAILED(hr))
        {
            SafeArrayDestroy(array);
            return hr;
        }
    }
    *result = array;
    return S_OK;
}

HRESULT FindUiaProvider::GetColumnHeaders(SAFEARRAY** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    const auto view = View();
    return view ? ColumnHeaders(*view, result, true) : UIA_E_ELEMENTNOTAVAILABLE;
}

HRESULT FindUiaProvider::GetColumnHeaderItems(SAFEARRAY** result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    const auto view = View();
    return view ? ColumnHeaders(*view, result, false) : UIA_E_ELEMENTNOTAVAILABLE;
}

struct ScrollInfo
{
    long long Position;
    long long Page;
    long long Total;
    long long Range() const { return (std::max)(0LL, Total - Page); }
    double Percent() const
    {
        return Range() ? 100.0 * (std::clamp)(Position, 0LL, Range()) / Range() : UIA_ScrollPatternNoScroll;
    }
    double ViewPercent() const
    {
        return Range() ? 100.0 * Page / Total : 100.0;
    }
};

ScrollInfo GetScrollInfo(const CFindUiaView& view, bool vertical)
{
    if (vertical)
        return {view.First, (std::max)(1, view.Last - view.First), view.Count};
    const auto left = static_cast<long long>(view.Columns[0].left);
    return {view.Bounds.left - left, (std::max)(1L, view.Bounds.right - view.Bounds.left),
            (std::max)(0LL, view.Columns[ColumnCount - 1].right - left)};
}

HRESULT FindUiaProvider::ScrollAxis(const CFindUiaView& view, ScrollAmount amount, bool vertical)
{
    if (amount == ScrollAmount_NoAmount)
        return S_OK;
    const auto scroll = GetScrollInfo(view, vertical);
    if (!scroll.Range())
        return UIA_E_INVALIDOPERATION;
    const long long unit = vertical ? (std::max)(1, view.RowHeight) : 1;
    const long long smallStep = vertical ? unit : (std::max)(1, view.RowHeight);
    const long long page = scroll.Page * unit;
    long long delta;
    switch (amount)
    {
    case ScrollAmount_LargeDecrement: delta = -page; break;
    case ScrollAmount_SmallDecrement: delta = -smallStep; break;
    case ScrollAmount_SmallIncrement: delta = smallStep; break;
    case ScrollAmount_LargeIncrement: delta = page; break;
    default: return E_INVALIDARG;
    }
    return State->Act(view.Revision, vertical ? CFindUiaAction::ScrollVertical : CFindUiaAction::ScrollHorizontal,
                      Pixels(delta));
}

HRESULT FindUiaProvider::Scroll(ScrollAmount horizontal, ScrollAmount vertical)
{
    const auto view = View();
    if (!view)
        return UIA_E_ELEMENTNOTAVAILABLE;
    const HRESULT horizontalResult = ScrollAxis(*view, horizontal, false);
    return FAILED(horizontalResult) ? horizontalResult : ScrollAxis(*view, vertical, true);
}

HRESULT FindUiaProvider::SetScrollPercent(double horizontal, double vertical)
{
    const auto view = View();
    if (!view)
        return UIA_E_ELEMENTNOTAVAILABLE;
    const auto setAxis = [this, &view](double percent, bool isVertical) -> HRESULT
    {
        if (percent == UIA_ScrollPatternNoScroll)
            return S_OK;
        if (!(percent >= 0.0 && percent <= 100.0))
            return E_INVALIDARG;
        const auto scroll = GetScrollInfo(*view, isVertical);
        if (!scroll.Range())
            return UIA_E_INVALIDOPERATION;
        const long long target = static_cast<long long>(std::llround(scroll.Range() * percent / 100.0));
        const long long unit = isVertical ? (std::max)(1, view->RowHeight) : 1;
        return State->Act(view->Revision, isVertical ? CFindUiaAction::ScrollVertical : CFindUiaAction::ScrollHorizontal,
                          Pixels((target - scroll.Position) * unit));
    };
    const HRESULT horizontalResult = setAxis(horizontal, false);
    return FAILED(horizontalResult) ? horizontalResult : setAxis(vertical, true);
}

HRESULT FindUiaProvider::get_HorizontalScrollPercent(double* value)
{
    const auto view = View();
    return view ? Store(value, GetScrollInfo(*view, false).Percent()) : UIA_E_ELEMENTNOTAVAILABLE;
}

HRESULT FindUiaProvider::get_VerticalScrollPercent(double* value)
{
    const auto view = View();
    return view ? Store(value, GetScrollInfo(*view, true).Percent()) : UIA_E_ELEMENTNOTAVAILABLE;
}

HRESULT FindUiaProvider::get_HorizontalViewSize(double* value)
{
    const auto view = View();
    return view ? Store(value, GetScrollInfo(*view, false).ViewPercent()) : UIA_E_ELEMENTNOTAVAILABLE;
}

HRESULT FindUiaProvider::get_VerticalViewSize(double* value)
{
    const auto view = View();
    return view ? Store(value, GetScrollInfo(*view, true).ViewPercent()) : UIA_E_ELEMENTNOTAVAILABLE;
}

HRESULT FindUiaProvider::get_HorizontallyScrollable(BOOL* value)
{
    const auto view = View();
    return view ? Store(value, GetScrollInfo(*view, false).Range() ? TRUE : FALSE) : UIA_E_ELEMENTNOTAVAILABLE;
}

HRESULT FindUiaProvider::get_VerticallyScrollable(BOOL* value)
{
    const auto view = View();
    return view ? Store(value, GetScrollInfo(*view, true).Range() ? TRUE : FALSE) : UIA_E_ELEMENTNOTAVAILABLE;
}

constexpr unsigned EventCallbackRunning = 0x80000000U;

void RaiseFindUiaEvents(const std::shared_ptr<CFindUiaState>& state, unsigned flags)
{
    const auto view = state->ReadView();
    if (!view || !view->Window || !UiaClientsAreListening())
        return;
    IRawElementProviderSimple* root = FindUiaProvider::CreateRoot(state);
    if (!root)
        return;
    if (flags & FindUiaStructureChanged)
        UiaRaiseStructureChangedEvent(root, StructureChangeType_ChildrenInvalidated, nullptr, 0);
    if (flags & FindUiaSelectionChanged)
        UiaRaiseAutomationEvent(root, UIA_Selection_InvalidatedEventId);
    if ((flags & FindUiaFocusChanged) && view->HasFocus)
    {
        auto* focused = root;
        IRawElementProviderFragmentRoot* fragment = nullptr;
        if (SUCCEEDED(root->QueryInterface(__uuidof(IRawElementProviderFragmentRoot),
                                          reinterpret_cast<void**>(&fragment))))
        {
            IRawElementProviderFragment* row = nullptr;
            if (SUCCEEDED(fragment->GetFocus(&row)) && row)
            {
                IRawElementProviderSimple* simple = nullptr;
                if (SUCCEEDED(row->QueryInterface(__uuidof(IRawElementProviderSimple),
                                                 reinterpret_cast<void**>(&simple))))
                    focused = simple;
                row->Release();
            }
            fragment->Release();
        }
        UiaRaiseAutomationEvent(focused, UIA_AutomationFocusChangedEventId);
        if (focused != root)
            focused->Release();
    }
    root->Release();
}

void CALLBACK DispatchFindUiaEvents(PTP_CALLBACK_INSTANCE, void* context)
{
    const std::unique_ptr<std::shared_ptr<CFindUiaState>> owner(
        static_cast<std::shared_ptr<CFindUiaState>*>(context));
    auto& state = **owner;
    const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    do
    {
        const unsigned flags = state.PendingEvents.exchange(EventCallbackRunning) & ~EventCallbackRunning;
        if (flags && SUCCEEDED(apartment))
            RaiseFindUiaEvents(*owner, flags);
        unsigned running = EventCallbackRunning;
        if (state.PendingEvents.compare_exchange_strong(running, 0))
            break;
    } while (true);
    if (SUCCEEDED(apartment))
        CoUninitialize();
}
} // namespace

IRawElementProviderSimple* CreateFindUiaProvider(const std::shared_ptr<CFindUiaState>& state)
{
    return state ? FindUiaProvider::CreateRoot(state) : nullptr;
}

void QueueFindUiaEvents(const std::shared_ptr<CFindUiaState>& state, unsigned flags)
{
    if (!state || !flags || (state->PendingEvents.fetch_or(flags | EventCallbackRunning) & EventCallbackRunning))
        return;
#ifdef new
#undef new
#define RESTORE_FIND_UIA_EVENT_DEBUG_NEW
#endif
    auto* owner = new (std::nothrow) std::shared_ptr<CFindUiaState>(state);
#ifdef RESTORE_FIND_UIA_EVENT_DEBUG_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_FIND_UIA_EVENT_DEBUG_NEW
#endif
    if (!owner || !TrySubmitThreadpoolCallback(DispatchFindUiaEvents, owner, nullptr))
    {
        delete owner;
        state->PendingEvents.store(0);
    }
}
