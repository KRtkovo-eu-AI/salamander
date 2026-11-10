// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-10 | Make pattern normalization safe for aliased input and output. | Fix ReplaceAssociations clearing the same string it passes as its input, which made Apply reject every row as empty. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 |
// | 2026-11-09 | Validate explicit overrides as the effective exact-key choice. | Allow a user action to shadow retained defaults even if the legacy candidates were not normalized into a conflict pair. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 |
// | 2026-11-09 | Allow user overrides to replace conflicting plugin targets. | Validation must accept a chosen user target alongside retained conflict candidates so Apply can commit it. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 |
// | 2026-11-09 | Fix error reporting in custom-handler storage helpers. | Free functions cannot call the association model's private SetError method. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 (Codex) |
// | 2026-11-09 | Migrate stored editor presets to Built-in targets. | Keep existing configurations compatible while retiring Preset from the editor. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 (Codex) |
// | 2026-11-09 | Persist named custom handler catalogs. | Keep custom viewers and editors tied to the selected configuration and resolve them by stable ID. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 (Codex) |
// | 2026-11-09 | Restrict association conflicts to competing exact patterns. | Keep universal and unrelated extension masks from being reported as collisions. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | GPT-6 (Codex) |
// | 2026-11-09 | Persist canonical extension owner labels. | Restore the localized owner display after a clean startup or a later configuration load. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-09 | Reject mask-group exclusion syntax in canonical patterns. | A single association row cannot preserve the legacy `|` exclusion semantics without losing matching behavior. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-08 | Add transactional replacement for the configuration editor. | Normalize and validate edited rows before replacing the live model. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Add configuration-storage persistence entry points. | Route canonical associations through registry and portable configuration storage without duplicating serialization. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Persist and validate the association record count. | Detect gaps in numeric registry records rather than silently loading a truncated model. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Preserve serialized association provenance during load. | Legacy conflicts must remain visible and must not be rewritten as ordinary legacy rows. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Add versioned persistence for canonical associations. | Keep the new backend portable across Registry and configuration-file storage before runtime migration. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Implement the canonical file-association model contract. | Start the backend-first implementation of provenance-aware per-pattern associations. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |

#include "precomp.h"
#include "fileassoc.h"
#include "configstorage.h"

#include <cctype>
#include <cstring>

namespace Salamander
{
namespace FileAssociations
{

namespace
{

const char* const FileAssociationsSchemaVersion = "Schema Version";
const char* const FileAssociationsCount = "Association Count";
const char* const FileAssociationsItems = "Associations";
const char* const FileAssociationPattern = "Pattern";
const char* const FileAssociationCategory = "Category";
const char* const FileAssociationOperation = "Operation";
const char* const FileAssociationTargetKind = "Target Kind";
const char* const FileAssociationTargetId = "Target ID";
const char* const FileAssociationTargetLabel = "Target Label";
const char* const FileAssociationProvenance = "Provenance";
const char* const FileAssociationSourceProvenance = "Source Provenance";
const char* const CustomHandlersKey = "Custom Association Handlers";
const DWORD FileAssociationsCurrentSchemaVersion = 1;
const DWORD CustomHandlersSchemaVersion = 1;

void SetErrorMessage(std::string* error, const char* message)
{
    if (error != NULL)
        *error = message != NULL ? message : "file association error";
}

std::vector<CustomHandler>& CustomViewers()
{
    static std::vector<CustomHandler> handlers;
    return handlers;
}

std::vector<CustomHandler>& CustomEditors()
{
    static std::vector<CustomHandler> handlers;
    return handlers;
}

std::vector<CustomHandler>& HandlersFor(CustomHandlerKind kind)
{
    return kind == CustomHandlerKind::Viewer ? CustomViewers() : CustomEditors();
}

bool IsValidCustomHandler(const CustomHandler& handler)
{
    return !handler.Id.empty() && handler.Id.size() <= 127 &&
           !handler.Name.empty() && handler.Name.size() <= 255 &&
           !handler.Command.empty() && handler.Command.size() <= SAL_MAX_PATH &&
           handler.Arguments.size() <= SAL_MAX_PATH &&
           handler.InitDir.size() <= SAL_MAX_PATH;
}

bool IsWildcardPattern(const std::string& pattern)
{
    return pattern.find('*') != std::string::npos || pattern.find('?') != std::string::npos;
}

bool IsUniversalPattern(const std::string& pattern)
{
    return pattern == "*.*" || pattern == "*";
}

bool IsSimpleExtensionPattern(const std::string& pattern)
{
    if (pattern.size() < 3 || pattern[0] != '*' || pattern[1] != '.')
        return false;
    return pattern.find('*', 1) == std::string::npos &&
           pattern.find('?', 1) == std::string::npos;
}

bool IsValidCategory(DWORD value)
{
    return value <= static_cast<DWORD>(Category::PanelArchive);
}

bool IsValidOperation(DWORD value)
{
    return value <= static_cast<DWORD>(PanelOperation::DeletePack);
}

bool IsValidProvenance(DWORD value)
{
    return value <= static_cast<DWORD>(Provenance::Conflict);
}

bool IsValidTargetKind(DWORD value)
{
    return value <= static_cast<DWORD>(TargetKind::CustomEditor);
}

std::string TrimPattern(const char* pattern)
{
    if (pattern == NULL)
        return std::string();

    const char* begin = pattern;
    while (*begin != 0 && std::isspace(static_cast<unsigned char>(*begin)))
        ++begin;

    const char* end = begin + strlen(begin);
    while (end > begin && std::isspace(static_cast<unsigned char>(end[-1])))
        --end;

    return std::string(begin, static_cast<size_t>(end - begin));
}

} // namespace

const std::vector<CustomHandler>& GetCustomHandlers(CustomHandlerKind kind)
{
    return HandlersFor(kind);
}

const CustomHandler* FindCustomHandler(CustomHandlerKind kind, const char* id)
{
    if (id == NULL || id[0] == 0)
        return NULL;
    const std::vector<CustomHandler>& handlers = GetCustomHandlers(kind);
    for (size_t i = 0; i < handlers.size(); ++i)
        if (_stricmp(handlers[i].Id.c_str(), id) == 0)
            return &handlers[i];
    return NULL;
}

BOOL ReplaceCustomHandlers(CustomHandlerKind kind,
                           const std::vector<CustomHandler>& handlers,
                           std::string* error)
{
    if (error != NULL)
        error->clear();
    std::vector<CustomHandler> replacement;
    replacement.reserve(handlers.size());
    for (size_t i = 0; i < handlers.size(); ++i)
    {
        if (!IsValidCustomHandler(handlers[i]))
        {
            SetErrorMessage(error, "custom handler record is incomplete or too large");
            return FALSE;
        }
        for (size_t j = 0; j < i; ++j)
            if (_stricmp(handlers[i].Id.c_str(), handlers[j].Id.c_str()) == 0 ||
                _stricmp(handlers[i].Name.c_str(), handlers[j].Name.c_str()) == 0)
            {
                SetErrorMessage(error, "custom handler IDs and names must be unique");
                return FALSE;
            }
        replacement.push_back(handlers[i]);
    }
    HandlersFor(kind).swap(replacement);
    return TRUE;
}

BOOL LoadCustomHandlersFromConfigurationStorage(CConfigurationStorage& storage,
                                               std::string* error)
{
    if (error != NULL)
        error->clear();
    HKEY root = NULL;
    if (!storage.OpenConfigurationRootKey(root, FALSE))
    {
        CustomViewers().clear();
        CustomEditors().clear();
        return TRUE;
    }

    HKEY key = NULL;
    if (!OpenKey(root, CustomHandlersKey, key))
    {
        CustomViewers().clear();
        CustomEditors().clear();
        CloseKey(root);
        return TRUE;
    }

    DWORD version = 0;
    DWORD count = 0;
    BOOL valid = GetValue(key, "Schema Version", REG_DWORD, &version, sizeof(version)) &&
                 version == CustomHandlersSchemaVersion &&
                 GetValue(key, "Record Count", REG_DWORD, &count, sizeof(count)) &&
                 count <= 512;
    std::vector<CustomHandler> viewers;
    std::vector<CustomHandler> editors;
    char name[16];
    char text[SAL_MAX_PATH + 1];
    for (DWORD i = 0; valid && i < count; ++i)
    {
        _snprintf_s(name, _countof(name), _TRUNCATE, "%lu", i + 1);
        HKEY item = NULL;
        if (!OpenKey(key, name, item))
        {
            valid = FALSE;
            break;
        }
        DWORD kindValue = 0;
        CustomHandler handler;
        valid = GetValue(item, "Kind", REG_DWORD, &kindValue, sizeof(kindValue)) &&
                kindValue <= static_cast<DWORD>(CustomHandlerKind::Editor) &&
                GetValue(item, "ID", REG_SZ, text, sizeof(text));
        if (valid) handler.Id = text;
        if (valid && GetValue(item, "Name", REG_SZ, text, sizeof(text))) handler.Name = text;
        else valid = FALSE;
        if (valid && GetValue(item, "Command", REG_SZ, text, sizeof(text))) handler.Command = text;
        else valid = FALSE;
        if (valid && GetValue(item, "Arguments", REG_SZ, text, sizeof(text))) handler.Arguments = text;
        else valid = FALSE;
        if (valid && GetValue(item, "Initial Directory", REG_SZ, text, sizeof(text))) handler.InitDir = text;
        else valid = FALSE;
        if (valid && IsValidCustomHandler(handler))
            (kindValue == static_cast<DWORD>(CustomHandlerKind::Viewer) ? viewers : editors).push_back(handler);
        else
            valid = FALSE;
        CloseKey(item);
    }
    CloseKey(key);
    CloseKey(root);
    if (!valid || !ReplaceCustomHandlers(CustomHandlerKind::Viewer, viewers, error) ||
        !ReplaceCustomHandlers(CustomHandlerKind::Editor, editors, error))
    {
        CustomViewers().clear();
        CustomEditors().clear();
        if (error != NULL && error->empty())
            *error = "custom handler configuration is invalid";
        return FALSE;
    }
    return TRUE;
}

BOOL SaveCustomHandlersToConfigurationStorage(CConfigurationStorage& storage,
                                               std::string* error)
{
    if (error != NULL)
        error->clear();
    if (CustomViewers().size() + CustomEditors().size() > 512)
    {
        SetErrorMessage(error, "custom handler count exceeds the supported limit");
        return FALSE;
    }
    HKEY root = NULL;
    if (!storage.OpenConfigurationRootKey(root, TRUE))
    {
        SetErrorMessage(error, "unable to open custom handler configuration root");
        return FALSE;
    }
    HKEY key = NULL;
    BOOL saved = CreateKey(root, CustomHandlersKey, key);
    if (saved)
        saved = ClearKey(key);
    DWORD version = CustomHandlersSchemaVersion;
    DWORD count = static_cast<DWORD>(CustomViewers().size() + CustomEditors().size());
    if (saved)
        saved = SetValue(key, "Schema Version", REG_DWORD, &version, sizeof(version));
    if (saved)
        saved = SetValue(key, "Record Count", REG_DWORD, &count, sizeof(count));
    char name[16];
    DWORD index = 0;
    for (int kind = 0; saved && kind < 2; ++kind)
    {
        const CustomHandlerKind handlerKind = static_cast<CustomHandlerKind>(kind);
        const std::vector<CustomHandler>& handlers = GetCustomHandlers(handlerKind);
        for (size_t i = 0; saved && i < handlers.size(); ++i)
        {
            _snprintf_s(name, _countof(name), _TRUNCATE, "%lu", ++index);
            HKEY item = NULL;
            saved = CreateKey(key, name, item);
            DWORD kindValue = static_cast<DWORD>(handlerKind);
            if (saved) saved = SetValue(item, "Kind", REG_DWORD, &kindValue, sizeof(kindValue));
            if (saved) saved = SetValue(item, "ID", REG_SZ, handlers[i].Id.c_str(), -1);
            if (saved) saved = SetValue(item, "Name", REG_SZ, handlers[i].Name.c_str(), -1);
            if (saved) saved = SetValue(item, "Command", REG_SZ, handlers[i].Command.c_str(), -1);
            if (saved) saved = SetValue(item, "Arguments", REG_SZ, handlers[i].Arguments.c_str(), -1);
            if (saved) saved = SetValue(item, "Initial Directory", REG_SZ, handlers[i].InitDir.c_str(), -1);
            if (item != NULL)
                CloseKey(item);
        }
    }
    if (key != NULL)
        CloseKey(key);
    CloseKey(root);
    if (!saved)
        SetErrorMessage(error, "unable to save custom handler configuration");
    return saved;
}

bool CFileAssociationModel::NormalizePattern(const char* pattern, std::string* normalized,
                                              std::string* error)
{
    if (normalized == NULL)
    {
        SetError(error, "normalized pattern output is required");
        return false;
    }

    // Callers may normalize a string in place (ReplaceAssociations does this
    // for each copied row). Copy the source before clearing the output so the
    // input pointer cannot be invalidated by that clear.
    const std::string sourcePattern = pattern != NULL ? pattern : "";
    normalized->clear();
    std::string value = TrimPattern(sourcePattern.c_str());
    if (value.empty())
    {
        SetError(error, "pattern must not be empty");
        return false;
    }
    if (value.find(';') != std::string::npos || value.find('|') != std::string::npos)
    {
        SetError(error, "one association must contain exactly one pattern without mask-group separators");
        return false;
    }
    if (value.find('\\') != std::string::npos || value.find('/') != std::string::npos)
    {
        SetError(error, "pattern must not contain a path separator");
        return false;
    }

    for (size_t i = 0; i < value.size(); ++i)
        value[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));

    if (value == "*")
        value = "*.*";
    else if (value[0] == '.')
        value = "*" + value;
    else if (value.find('*') == std::string::npos && value.find('?') == std::string::npos &&
             value.find('.') == std::string::npos)
        value = "*." + value;

    if (value == "*." || value.find("..") != std::string::npos)
    {
        SetError(error, "pattern has an invalid extension form");
        return false;
    }

    *normalized = value;
    return true;
}

bool CFileAssociationModel::PatternsOverlap(const std::string& first, const std::string& second)
{
    if (first == second || IsUniversalPattern(first) || IsUniversalPattern(second))
        return true;

    // Different simple suffix patterns cannot match the same filename. For
    // other wildcard forms, stay conservative until a full glob intersection
    // matcher is introduced with the migration implementation.
    if (IsSimpleExtensionPattern(first) && IsSimpleExtensionPattern(second))
        return false;
    if (!IsWildcardPattern(first) && !IsWildcardPattern(second))
        return false;
    return true;
}

bool CFileAssociationModel::Add(const char* pattern, Category category, PanelOperation operation,
                                const Target& target, Provenance provenance, std::string* error)
{
    std::string normalized;
    if (!NormalizePattern(pattern, &normalized, error))
        return false;

    Association item;
    item.Pattern = normalized;
    item.AssociationCategory = category;
    item.Operation = operation;
    item.AssociationTarget = target;
    item.AssociationProvenance = provenance;
    item.SourceProvenance = provenance;
    Items.push_back(item);
    if (!Validate(error))
    {
        Items.pop_back();
        return false;
    }
    return true;
}

bool CFileAssociationModel::AddLegacy(const char* pattern, Category category,
                                      PanelOperation operation, const Target& target,
                                      std::string* error)
{
    std::string normalized;
    if (!NormalizePattern(pattern, &normalized, error))
        return false;

    Association item;
    item.Pattern = normalized;
    item.AssociationCategory = category;
    item.Operation = operation;
    item.AssociationTarget = target;
    item.AssociationProvenance = Provenance::Legacy;
    item.SourceProvenance = Provenance::Legacy;
    Items.push_back(item);
    MarkLegacyConflicts();
    return true;
}

bool CFileAssociationModel::SetUserTarget(const char* pattern, Category category,
                                          PanelOperation operation, const Target& target,
                                          std::string* error)
{
    std::string normalized;
    if (!NormalizePattern(pattern, &normalized, error))
        return false;

    const std::vector<Association> previous = Items;
    for (size_t i = Items.size(); i > 0; --i)
    {
        const Association& item = Items[i - 1];
        if (SameKey(item, normalized, category, operation) &&
            (item.AssociationProvenance == Provenance::User ||
             item.AssociationProvenance == Provenance::Disabled))
            Items.erase(Items.begin() + i - 1);
    }
    if (Add(normalized.c_str(), category, operation, target, Provenance::User, error))
        return true;
    Items = previous;
    return false;
}

bool CFileAssociationModel::SetUnassigned(const char* pattern, Category category,
                                          PanelOperation operation, std::string* error)
{
    if (!SetUserTarget(pattern, category, operation, Target(TargetKind::Unassigned), error))
        return false;

    std::string normalized;
    if (!NormalizePattern(pattern, &normalized, error))
        return false;
    for (size_t i = Items.size(); i > 0; --i)
    {
        Association& item = Items[i - 1];
        if (SameKey(item, normalized, category, operation) &&
            item.AssociationProvenance == Provenance::User &&
            item.AssociationTarget.Kind == TargetKind::Unassigned)
        {
            item.AssociationProvenance = Provenance::Disabled;
            return true;
        }
    }
    SetError(error, "unassigned association was not stored");
    return false;
}

bool CFileAssociationModel::ResetUserOverride(const char* pattern, Category category,
                                               PanelOperation operation)
{
    std::string normalized;
    if (!NormalizePattern(pattern, &normalized))
        return false;

    bool removed = false;
    for (size_t i = Items.size(); i > 0; --i)
    {
        const Association& item = Items[i - 1];
        if (SameKey(item, normalized, category, operation) &&
            (item.AssociationProvenance == Provenance::User ||
             item.AssociationProvenance == Provenance::Disabled))
        {
            Items.erase(Items.begin() + i - 1);
            removed = true;
        }
    }
    return removed;
}

bool CFileAssociationModel::ReplaceAssociations(const std::vector<Association>& associations,
                                                std::string* error)
{
    std::vector<Association> replacement;
    replacement.reserve(associations.size());
    for (size_t i = 0; i < associations.size(); ++i)
    {
        Association item = associations[i];
        if (!NormalizePattern(item.Pattern.c_str(), &item.Pattern, error))
            return false;
        replacement.push_back(item);
    }

    CFileAssociationModel candidate;
    candidate.Items.swap(replacement);
    if (!candidate.Validate(error))
        return false;

    Items.swap(candidate.Items);
    return true;
}

bool CFileAssociationModel::Validate(std::string* error) const
{
    for (size_t i = 0; i < Items.size(); ++i)
    {
        const Association& item = Items[i];
        if ((item.AssociationTarget.Kind == TargetKind::CustomViewer &&
             (item.AssociationCategory != Category::View &&
              item.AssociationCategory != Category::AlternateView)) ||
            (item.AssociationTarget.Kind == TargetKind::CustomEditor &&
             item.AssociationCategory != Category::Edit) ||
            ((item.AssociationTarget.Kind == TargetKind::CustomViewer ||
              item.AssociationTarget.Kind == TargetKind::CustomEditor) &&
             item.AssociationTarget.StableId.empty()))
        {
            SetError(error, "custom association target does not match its category or has no ID");
            return false;
        }
        for (size_t j = i + 1; j < Items.size(); ++j)
        {
            const Association& first = Items[i];
            const Association& second = Items[j];
            if (!IsConflictingPair(first, second))
                continue;

            bool hasUserOverride = false;
            for (size_t k = 0; k < Items.size(); ++k)
            {
                const Association& candidate = Items[k];
                if (SameKey(candidate, first.Pattern, first.AssociationCategory, first.Operation) &&
                    (candidate.AssociationProvenance == Provenance::User ||
                     candidate.AssociationProvenance == Provenance::Disabled))
                {
                    hasUserOverride = true;
                    break;
                }
            }
            const bool firstIsUser = first.AssociationProvenance == Provenance::User ||
                                     first.AssociationProvenance == Provenance::Disabled;
            const bool secondIsUser = second.AssociationProvenance == Provenance::User ||
                                      second.AssociationProvenance == Provenance::Disabled;
            if (hasUserOverride && !(firstIsUser && secondIsUser))
                continue;
            if (first.AssociationProvenance == Provenance::Conflict &&
                second.AssociationProvenance == Provenance::Conflict)
                continue;
            // The old "overlapping patterns in one association category are not allowed"
            // rule was too broad: wildcard fallback and distinct extensions are valid.
            SetError(error, "one exact association pattern has multiple active targets");
            return false;
        }
    }
    return true;
}

const Association* CFileAssociationModel::FindExactEffective(const char* pattern,
                                                              Category category,
                                                              PanelOperation operation) const
{
    std::string normalized;
    if (!NormalizePattern(pattern, &normalized))
        return NULL;

    const Association* result = NULL;
    int priority = 0;
    for (size_t i = 0; i < Items.size(); ++i)
    {
        const Association& item = Items[i];
        if (!SameKey(item, normalized, category, operation))
            continue;
        const int itemPriority = EffectivePriority(item.AssociationProvenance);
        if (itemPriority > priority)
        {
            result = &item;
            priority = itemPriority;
        }
        else if (itemPriority == priority)
        {
            return NULL;
        }
    }
    return result;
}

BOOL CFileAssociationModel::Load(HKEY parentKey, const char* keyName, std::string* error)
{
    if (error != NULL)
        error->clear();
    if (parentKey == NULL || keyName == NULL || keyName[0] == 0)
    {
        SetError(error, "association storage key is invalid");
        return FALSE;
    }

    HKEY rootKey;
    if (!OpenKey(parentKey, keyName, rootKey))
        return TRUE;

    DWORD schemaVersion = 0;
    BOOL valid = GetValue(rootKey, FileAssociationsSchemaVersion, REG_DWORD,
                          &schemaVersion, sizeof(schemaVersion));
    if (!valid || schemaVersion != FileAssociationsCurrentSchemaVersion)
    {
        SetError(error, "unsupported file-association schema version");
        CloseKey(rootKey);
        return FALSE;
    }

    DWORD associationCount = 0;
    if (!GetValue(rootKey, FileAssociationsCount, REG_DWORD, &associationCount,
                  sizeof(associationCount)))
    {
        SetError(error, "file-association record count is missing");
        CloseKey(rootKey);
        return FALSE;
    }

    HKEY itemsKey;
    if (!OpenKey(rootKey, FileAssociationsItems, itemsKey))
    {
        CloseKey(rootKey);
        SetError(error, "file-association records are missing");
        return FALSE;
    }

    CFileAssociationModel loaded;
    HKEY itemKey;
    char itemName[30];
    std::vector<char> pattern(MAX_GROUPMASK);
    std::vector<char> targetId(4096);
    std::vector<char> targetLabel(4096);
    DWORD category = 0, operation = 0, targetKind = 0;
    DWORD provenance = 0, sourceProvenance = 0;
    bool itemError = false;
    for (DWORD itemIndex = 0; itemIndex < associationCount; ++itemIndex)
    {
        _ultoa(static_cast<unsigned long>(itemIndex + 1), itemName, 10);
        if (!OpenKey(itemsKey, itemName, itemKey))
        {
            SetError(error, "file-association record is missing");
            itemError = true;
            break;
        }
        pattern[0] = 0;
        targetId[0] = 0;
        targetLabel[0] = 0;
        if (!GetValue(itemKey, FileAssociationPattern, REG_SZ, pattern.data(),
                      static_cast<DWORD>(pattern.size())) ||
            !GetValue(itemKey, FileAssociationCategory, REG_DWORD, &category, sizeof(category)) ||
            !GetValue(itemKey, FileAssociationOperation, REG_DWORD, &operation, sizeof(operation)) ||
            !GetValue(itemKey, FileAssociationTargetKind, REG_DWORD, &targetKind, sizeof(targetKind)) ||
            !GetValue(itemKey, FileAssociationTargetId, REG_SZ, targetId.data(),
                      static_cast<DWORD>(targetId.size())) ||
            !GetValue(itemKey, FileAssociationProvenance, REG_DWORD, &provenance, sizeof(provenance)) ||
            !GetValue(itemKey, FileAssociationSourceProvenance, REG_DWORD,
                      &sourceProvenance, sizeof(sourceProvenance)) ||
            !IsValidCategory(category) || !IsValidOperation(operation) ||
            !IsValidTargetKind(targetKind) || !IsValidProvenance(provenance) ||
            !IsValidProvenance(sourceProvenance))
        {
            itemError = true;
        }
        else
        {
            if (!GetValue(itemKey, FileAssociationTargetLabel, REG_SZ,
                          targetLabel.data(), static_cast<DWORD>(targetLabel.size())))
                targetLabel[0] = 0;
            std::string normalized;
            if (!NormalizePattern(pattern.data(), &normalized, error))
                itemError = true;
            else
            {
                Association item;
                item.Pattern = normalized;
                item.AssociationCategory = static_cast<Category>(category);
                item.Operation = static_cast<PanelOperation>(operation);
                // Value 2 was the retired Preset kind in previously saved configurations.
                const bool wasPreset = targetKind == 2;
                TargetKind loadedTargetKind = wasPreset
                    ? TargetKind::BuiltIn : static_cast<TargetKind>(targetKind);
                const bool legacyEditorTarget =
                    static_cast<Category>(category) == Category::Edit &&
                    (wasPreset || strcmp(targetId.data(), "legacy") == 0 ||
                     strcmp(targetId.data(), "legacy-editor") == 0);
                if (loadedTargetKind == TargetKind::BuiltIn && legacyEditorTarget)
                {
                    const char legacyEditorId[] = "legacy-editor";
                    memcpy(targetId.data(), legacyEditorId, sizeof(legacyEditorId));
                }
                item.AssociationTarget =
                    Target(loadedTargetKind, targetId.data(), targetLabel.data());
                if (legacyEditorTarget)
                    item.AssociationTarget.DisplayLabel.clear();
                item.AssociationProvenance = static_cast<Provenance>(provenance);
                item.SourceProvenance = static_cast<Provenance>(sourceProvenance);
                loaded.Items.push_back(item);
            }
        }

        CloseKey(itemKey);
        if (itemError)
            break;
    }
    CloseKey(itemsKey);
    CloseKey(rootKey);

    if (itemError)
    {
        if (error == NULL || error->empty())
            SetError(error, "malformed file-association record");
        return FALSE;
    }

    // Conflict used to be persisted after broad-mask overlap checks. Restore
    // each row's saved origin and recompute conflicts with the exact-target
    // rule so configurations written by older builds drop false conflicts.
    for (size_t i = 0; i < loaded.Items.size(); ++i)
    {
        Association& item = loaded.Items[i];
        if (item.AssociationProvenance == Provenance::Conflict)
            item.AssociationProvenance = item.SourceProvenance == Provenance::Conflict
                                             ? Provenance::Legacy
                                             : item.SourceProvenance;
    }
    loaded.MarkLegacyConflicts();

    // Persisted conflicts are diagnostic rows for competing targets of the
    // same exact pattern and survive a round trip; all other competing active
    // targets are rejected.
    for (size_t i = 0; i < loaded.Items.size() && !itemError; ++i)
    {
        for (size_t j = i + 1; j < loaded.Items.size(); ++j)
        {
            const Association& first = loaded.Items[i];
            const Association& second = loaded.Items[j];
            if (!IsConflictingPair(first, second))
                continue;
            if (first.AssociationProvenance == Provenance::Conflict &&
                second.AssociationProvenance == Provenance::Conflict)
                continue;
            SetError(error, "persisted association pattern has multiple active targets");
            itemError = true;
            break;
        }
    }

    if (itemError)
        return FALSE;

    Items.swap(loaded.Items);
    return TRUE;
}

BOOL CFileAssociationModel::Save(HKEY parentKey, const char* keyName, std::string* error) const
{
    if (error != NULL)
        error->clear();
    if (parentKey == NULL || keyName == NULL || keyName[0] == 0)
    {
        SetError(error, "association storage key is invalid");
        return FALSE;
    }
    if (!Validate(error))
        return FALSE;

    HKEY rootKey;
    if (!CreateKey(parentKey, keyName, rootKey))
    {
        SetError(error, "unable to create file-association storage key");
        return FALSE;
    }

    BOOL saved = ClearKey(rootKey);
    DWORD schemaVersion = FileAssociationsCurrentSchemaVersion;
    DWORD associationCount = static_cast<DWORD>(Items.size());
    if (saved)
        saved = SetValue(rootKey, FileAssociationsSchemaVersion, REG_DWORD,
                         &schemaVersion, sizeof(schemaVersion));
    if (saved)
        saved = SetValue(rootKey, FileAssociationsCount, REG_DWORD,
                         &associationCount, sizeof(associationCount));

    HKEY itemsKey;
    if (saved)
        saved = CreateKey(rootKey, FileAssociationsItems, itemsKey);
    if (saved)
    {
        char itemName[30];
        for (size_t i = 0; i < Items.size() && saved; ++i)
        {
            itoa(static_cast<int>(i + 1), itemName, 10);
            HKEY itemKey;
            if (!CreateKey(itemsKey, itemName, itemKey))
            {
                saved = FALSE;
                break;
            }

            const Association& item = Items[i];
            const DWORD category = static_cast<DWORD>(item.AssociationCategory);
            const DWORD operation = static_cast<DWORD>(item.Operation);
            const DWORD targetKind = static_cast<DWORD>(item.AssociationTarget.Kind);
            const DWORD provenance = static_cast<DWORD>(item.AssociationProvenance);
            const DWORD sourceProvenance = static_cast<DWORD>(item.SourceProvenance);
            if (saved)
                saved = SetValue(itemKey, FileAssociationPattern, REG_SZ,
                                 item.Pattern.c_str(), -1);
            if (saved)
                saved = SetValue(itemKey, FileAssociationCategory, REG_DWORD,
                                 &category, sizeof(category));
            if (saved)
                saved = SetValue(itemKey, FileAssociationOperation, REG_DWORD,
                                 &operation, sizeof(operation));
            if (saved)
                saved = SetValue(itemKey, FileAssociationTargetKind, REG_DWORD,
                                 &targetKind, sizeof(targetKind));
            if (saved)
                saved = SetValue(itemKey, FileAssociationTargetId, REG_SZ,
                                 item.AssociationTarget.StableId.c_str(), -1);
            if (saved)
                saved = SetValue(itemKey, FileAssociationTargetLabel, REG_SZ,
                                 item.AssociationTarget.DisplayLabel.c_str(), -1);
            if (saved)
                saved = SetValue(itemKey, FileAssociationProvenance, REG_DWORD,
                                 &provenance, sizeof(provenance));
            if (saved)
                saved = SetValue(itemKey, FileAssociationSourceProvenance, REG_DWORD,
                                 &sourceProvenance, sizeof(sourceProvenance));
            CloseKey(itemKey);
        }
        CloseKey(itemsKey);
    }
    CloseKey(rootKey);
    if (!saved)
        SetError(error, "unable to save file-association records");
    return saved;
}

BOOL CFileAssociationModel::LoadFromConfigurationStorage(CConfigurationStorage& storage,
                                                          const char* keyName,
                                                          std::string* error)
{
    if (error != NULL)
        error->clear();
    if (keyName == NULL || keyName[0] == 0)
    {
        SetError(error, "association storage key is invalid");
        return FALSE;
    }

    HKEY rootKey = NULL;
    if (!storage.OpenConfigurationRootKey(rootKey, FALSE))
        return TRUE;

    BOOL loaded = Load(rootKey, keyName, error);
    CloseKey(rootKey);
    return loaded;
}

BOOL CFileAssociationModel::SaveToConfigurationStorage(CConfigurationStorage& storage,
                                                        const char* keyName,
                                                        std::string* error) const
{
    if (error != NULL)
        error->clear();
    if (keyName == NULL || keyName[0] == 0)
    {
        SetError(error, "association storage key is invalid");
        return FALSE;
    }

    HKEY rootKey = NULL;
    if (!storage.OpenConfigurationRootKey(rootKey, TRUE))
    {
        SetError(error, "unable to open configuration storage root");
        return FALSE;
    }

    BOOL saved = Save(rootKey, keyName, error);
    CloseKey(rootKey);
    return saved;
}

bool CFileAssociationModel::IsExactOverridePair(const Association& first,
                                                const Association& second)
{
    if (first.Pattern != second.Pattern)
        return false;
    const Provenance firstLayer = first.AssociationProvenance == Provenance::Conflict
                                      ? first.SourceProvenance
                                      : first.AssociationProvenance;
    const Provenance secondLayer = second.AssociationProvenance == Provenance::Conflict
                                       ? second.SourceProvenance
                                       : second.AssociationProvenance;
    // A migrated conflict represents multiple plugin/extension candidates at
    // the same exact key. A user choice must be allowed to shadow every one
    // of those candidates just like it shadows an ordinary default row.
    const bool firstDefault = firstLayer == Provenance::Default ||
                              firstLayer == Provenance::Legacy ||
                              first.AssociationProvenance == Provenance::Conflict;
    const bool secondDefault = secondLayer == Provenance::Default ||
                               secondLayer == Provenance::Legacy ||
                               second.AssociationProvenance == Provenance::Conflict;
    const bool firstUser = first.AssociationProvenance == Provenance::User ||
                           first.AssociationProvenance == Provenance::Disabled;
    const bool secondUser = second.AssociationProvenance == Provenance::User ||
                            second.AssociationProvenance == Provenance::Disabled;
    return (firstDefault && secondUser) || (secondDefault && firstUser);
}

bool CFileAssociationModel::IsConflictingPair(const Association& first,
                                               const Association& second)
{
    if (first.Pattern != second.Pattern ||
        first.AssociationCategory != second.AssociationCategory ||
        first.Operation != second.Operation ||
        first.AssociationTarget.Kind == TargetKind::Unassigned ||
        second.AssociationTarget.Kind == TargetKind::Unassigned ||
        first.AssociationProvenance == Provenance::Disabled ||
        second.AssociationProvenance == Provenance::Disabled ||
        IsExactOverridePair(first, second))
        return false;

    return first.AssociationTarget.Kind != second.AssociationTarget.Kind ||
           first.AssociationTarget.StableId != second.AssociationTarget.StableId ||
           first.AssociationTarget.DisplayLabel != second.AssociationTarget.DisplayLabel;
}

int CFileAssociationModel::EffectivePriority(Provenance provenance)
{
    switch (provenance)
    {
    case Provenance::User:
    case Provenance::Disabled:
        return 3;
    case Provenance::Legacy:
        return 2;
    case Provenance::Default:
        return 1;
    default:
        return 0;
    }
}

bool CFileAssociationModel::SameKey(const Association& item, const std::string& pattern,
                                    Category category, PanelOperation operation)
{
    return item.Pattern == pattern && item.AssociationCategory == category &&
           item.Operation == operation;
}

void CFileAssociationModel::SetError(std::string* error, const char* message)
{
    if (error != NULL)
        *error = message != NULL ? message : "file association error";
}

void CFileAssociationModel::MarkLegacyConflicts()
{
    for (size_t i = 0; i < Items.size(); ++i)
    {
        for (size_t j = i + 1; j < Items.size(); ++j)
        {
            Association& first = Items[i];
            Association& second = Items[j];
            if (!IsConflictingPair(first, second))
                continue;
            first.AssociationProvenance = Provenance::Conflict;
            second.AssociationProvenance = Provenance::Conflict;
        }
    }
}

} // namespace FileAssociations
} // namespace Salamander
