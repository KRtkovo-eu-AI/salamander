// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Preserve extension owner labels in canonical targets. | Keep Salamatrix associations distinguishable from the framework after migration and restart. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-08 | Add transactional replacement for the configuration editor. | Let the canonical association page edit a complete model without exposing private storage or partially applying invalid rows. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Add configuration-storage persistence entry points. | Route canonical associations through registry and portable configuration storage without duplicating serialization. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Document complete association record persistence. | Ensure schema consumers can detect missing numeric records instead of accepting truncated input. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Make the persistence API self-contained. | Ensure consumers see the Windows registry handle and Boolean types declared by the public header. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Add versioned persistence for canonical associations. | Keep the new backend portable across Registry and configuration-file storage before runtime migration. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-07 | Add the canonical file-association model contract. | Start the backend-first implementation of provenance-aware per-pattern associations. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <windows.h>

class CConfigurationStorage;

namespace Salamander
{
namespace FileAssociations
{

enum class Category
{
    View,
    AlternateView,
    Edit,
    PanelArchive
};

enum class PanelOperation
{
    NotApplicable,
    ListUnpack,
    DeletePack
};

enum class Provenance
{
    Default,
    User,
    Legacy,
    Disabled,
    Conflict
};

enum class TargetKind
{
    BuiltIn,
    Plugin,
    Preset,
    Unassigned
};

struct Target
{
    TargetKind Kind;
    std::string StableId;
    std::string DisplayLabel;

    Target(TargetKind kind = TargetKind::Unassigned, const char* stableId = "",
           const char* displayLabel = "")
        : Kind(kind), StableId(stableId != NULL ? stableId : ""),
          DisplayLabel(displayLabel != NULL ? displayLabel : "") {}
};

struct Association
{
    std::string Pattern;
    Category AssociationCategory;
    PanelOperation Operation;
    Target AssociationTarget;
    Provenance AssociationProvenance;
    Provenance SourceProvenance;
};

class CFileAssociationModel
{
public:
    static bool NormalizePattern(const char* pattern, std::string* normalized,
                                 std::string* error = NULL);
    static bool PatternsOverlap(const std::string& first, const std::string& second);

    bool Add(const char* pattern, Category category, PanelOperation operation,
             const Target& target, Provenance provenance, std::string* error = NULL);
    bool AddLegacy(const char* pattern, Category category, PanelOperation operation,
                   const Target& target, std::string* error = NULL);
    bool SetUserTarget(const char* pattern, Category category, PanelOperation operation,
                       const Target& target, std::string* error = NULL);
    bool SetUnassigned(const char* pattern, Category category, PanelOperation operation,
                       std::string* error = NULL);
    bool ResetUserOverride(const char* pattern, Category category, PanelOperation operation);

    void Clear() { Items.clear(); }
    bool ReplaceAssociations(const std::vector<Association>& associations,
                             std::string* error = NULL);

    bool Validate(std::string* error = NULL) const;
    const Association* FindExactEffective(const char* pattern, Category category,
                                          PanelOperation operation) const;

    // Missing storage is not an error and leaves the model unchanged. A
    // present but malformed schema is rejected without partial replacement.
    BOOL Load(HKEY parentKey, const char* keyName, std::string* error = NULL);
    BOOL Save(HKEY parentKey, const char* keyName, std::string* error = NULL) const;
    BOOL LoadFromConfigurationStorage(CConfigurationStorage& storage, const char* keyName,
                                      std::string* error = NULL);
    BOOL SaveToConfigurationStorage(CConfigurationStorage& storage, const char* keyName,
                                    std::string* error = NULL) const;

    const std::vector<Association>& Associations() const { return Items; }

private:
    static bool IsExactOverridePair(const Association& first, const Association& second);
    void MarkLegacyConflicts();
    static int EffectivePriority(Provenance provenance);
    static bool SameKey(const Association& item, const std::string& pattern,
                        Category category, PanelOperation operation);
    static void SetError(std::string* error, const char* message);

    std::vector<Association> Items;
};

} // namespace FileAssociations
} // namespace Salamander
