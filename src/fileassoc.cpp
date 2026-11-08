// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
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
const char* const FileAssociationProvenance = "Provenance";
const char* const FileAssociationSourceProvenance = "Source Provenance";
const DWORD FileAssociationsCurrentSchemaVersion = 1;

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
    return value <= static_cast<DWORD>(TargetKind::Unassigned);
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

bool CFileAssociationModel::NormalizePattern(const char* pattern, std::string* normalized,
                                              std::string* error)
{
    if (normalized == NULL)
    {
        SetError(error, "normalized pattern output is required");
        return false;
    }

    normalized->clear();
    std::string value = TrimPattern(pattern);
    if (value.empty())
    {
        SetError(error, "pattern must not be empty");
        return false;
    }
    if (value.find(';') != std::string::npos)
    {
        SetError(error, "one association must contain exactly one pattern");
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

bool CFileAssociationModel::Validate(std::string* error) const
{
    for (size_t i = 0; i < Items.size(); ++i)
    {
        for (size_t j = i + 1; j < Items.size(); ++j)
        {
            const Association& first = Items[i];
            const Association& second = Items[j];
            if (first.AssociationCategory != second.AssociationCategory ||
                first.Operation != second.Operation ||
                !PatternsOverlap(first.Pattern, second.Pattern))
                continue;
            if (IsExactOverridePair(first, second))
                continue;
            SetError(error, "overlapping patterns in one association category are not allowed");
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
            std::string normalized;
            if (!NormalizePattern(pattern.data(), &normalized, error))
                itemError = true;
            else
            {
                Association item;
                item.Pattern = normalized;
                item.AssociationCategory = static_cast<Category>(category);
                item.Operation = static_cast<PanelOperation>(operation);
                item.AssociationTarget =
                    Target(static_cast<TargetKind>(targetKind), targetId.data());
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

    // Persisted conflicts are diagnostic rows and must survive a round trip.
    // Other overlapping rows are malformed unless they form an explicit
    // default/user override pair.
    for (size_t i = 0; i < loaded.Items.size() && !itemError; ++i)
    {
        for (size_t j = i + 1; j < loaded.Items.size(); ++j)
        {
            const Association& first = loaded.Items[i];
            const Association& second = loaded.Items[j];
            if (first.AssociationCategory != second.AssociationCategory ||
                first.Operation != second.Operation ||
                !PatternsOverlap(first.Pattern, second.Pattern) ||
                IsExactOverridePair(first, second) ||
                first.AssociationProvenance == Provenance::Conflict ||
                second.AssociationProvenance == Provenance::Conflict)
                continue;
            SetError(error, "overlapping persisted associations are not allowed");
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
    const bool firstDefault = first.AssociationProvenance == Provenance::Default ||
                              first.AssociationProvenance == Provenance::Legacy;
    const bool secondDefault = second.AssociationProvenance == Provenance::Default ||
                               second.AssociationProvenance == Provenance::Legacy;
    const bool firstUser = first.AssociationProvenance == Provenance::User ||
                           first.AssociationProvenance == Provenance::Disabled;
    const bool secondUser = second.AssociationProvenance == Provenance::User ||
                            second.AssociationProvenance == Provenance::Disabled;
    return (firstDefault && secondUser) || (secondDefault && firstUser);
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
            if (first.AssociationCategory != second.AssociationCategory ||
                first.Operation != second.Operation ||
                !PatternsOverlap(first.Pattern, second.Pattern) ||
                IsExactOverridePair(first, second))
                continue;
            first.AssociationProvenance = Provenance::Conflict;
            second.AssociationProvenance = Provenance::Conflict;
        }
    }
}

} // namespace FileAssociations
} // namespace Salamander
