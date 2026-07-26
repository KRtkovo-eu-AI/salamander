// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_storage.h
    Isolated persistent storage for scripted extensions.
*/

#pragma once

#include "../shared/spl_base.h"

namespace Salamatrix
{
    namespace Storage
    {

#define SALAMATRIX_SERVICE_STORAGE "Salamatrix.Storage"
#define SALAMATRIX_STORAGE_VERSION_1_0 0x00010000

        enum StorageValueType
        {
            StorageValueMissing = 0,
            StorageValueString = 1,
            StorageValueInteger = 2,
            StorageValueBoolean = 3
        };

        class IStorageService
        {
        public:
            virtual DWORD WINAPI GetVersion() const = 0;
            virtual StorageValueType WINAPI GetValueType(
                const char* extensionId,
                const char* key) const = 0;
            virtual BOOL WINAPI GetString(
                const char* extensionId,
                const char* key,
                char* buffer,
                int bufferSize,
                int* requiredSize) const = 0;
            virtual BOOL WINAPI GetInteger(
                const char* extensionId,
                const char* key,
                LONGLONG* value) const = 0;
            virtual BOOL WINAPI GetBoolean(
                const char* extensionId,
                const char* key,
                BOOL* value) const = 0;
            virtual BOOL WINAPI SetString(
                const char* extensionId,
                const char* key,
                const char* value) = 0;
            virtual BOOL WINAPI SetInteger(
                const char* extensionId,
                const char* key,
                LONGLONG value) = 0;
            virtual BOOL WINAPI SetBoolean(
                const char* extensionId,
                const char* key,
                BOOL value) = 0;
            virtual BOOL WINAPI DeleteValue(
                const char* extensionId,
                const char* key) = 0;
            virtual BOOL WINAPI ClearExtension(const char* extensionId) = 0;
            // Enumeration order is unspecified and may change between calls.
            // Keys are UTF-8; requiredKeySize includes the terminating NUL.
            // Callers must tolerate the collection changing between calls.
            virtual int WINAPI GetKeyCount(const char* extensionId) const
            {
                UNREFERENCED_PARAMETER(extensionId);
                return -1;
            }
            virtual BOOL WINAPI GetKeyAt(
                const char* extensionId,
                int index,
                char* keyBuffer,
                int keyBufferSize,
                int* requiredKeySize,
                StorageValueType* type) const
            {
                UNREFERENCED_PARAMETER(extensionId);
                UNREFERENCED_PARAMETER(index);
                if (keyBuffer != NULL && keyBufferSize > 0)
                    keyBuffer[0] = 0;
                if (requiredKeySize != NULL)
                    *requiredKeySize = 0;
                if (type != NULL)
                    *type = StorageValueMissing;
                return FALSE;
            }

        protected:
            virtual ~IStorageService() {}
        };

        class StorageService : public IStorageService
        {
        private:
            enum
            {
                MaxEntries = 1024,
                MaxExtensionIdLength = 127,
                MaxKeyLength = 255,
                MaxStringLength = 16384,
                BlobMagic = 0x53584d53, // "SMXS"
                BlobVersion = 1
            };

#pragma pack(push, 1)
            struct BlobHeader
            {
                DWORD Magic;
                DWORD Version;
                DWORD EntryCount;
            };

            struct BlobEntry
            {
                WORD ExtensionIdLength;
                WORD KeyLength;
                DWORD Type;
                DWORD DataLength;
            };
#pragma pack(pop)

            struct Entry
            {
                char ExtensionId[MaxExtensionIdLength + 1];
                char Key[MaxKeyLength + 1];
                StorageValueType Type;
                char* StringValue;
                LONGLONG IntegerValue;
                BOOL BooleanValue;

                Entry()
                    : Type(StorageValueMissing),
                      StringValue(NULL),
                      IntegerValue(0),
                      BooleanValue(FALSE)
                {
                    ExtensionId[0] = 0;
                    Key[0] = 0;
                }
            };

            Entry* Entries[MaxEntries];
            int EntryCount;
            BOOL Modified;
            mutable CRITICAL_SECTION Lock;

            StorageService(const StorageService&);
            StorageService& operator=(const StorageService&);

            class ScopedLock
            {
            private:
                CRITICAL_SECTION* Section;

            public:
                explicit ScopedLock(CRITICAL_SECTION* section)
                    : Section(section)
                {
                    EnterCriticalSection(Section);
                }

                ~ScopedLock()
                {
                    LeaveCriticalSection(Section);
                }
            };

            static BOOL IsIdentifier(
                const char* value,
                int maximumLength,
                BOOL allowColon)
            {
                if (value == NULL || value[0] == 0)
                    return FALSE;

                int length = 0;
                while (value[length] != 0)
                {
                    unsigned char character =
                        static_cast<unsigned char>(value[length]);
                    if (!((character >= 'A' && character <= 'Z') ||
                          (character >= 'a' && character <= 'z') ||
                          (character >= '0' && character <= '9') ||
                          character == '.' ||
                          character == '_' ||
                          character == '-' ||
                          (allowColon && character == ':')))
                    {
                        return FALSE;
                    }
                    if (++length > maximumLength)
                        return FALSE;
                }
                return TRUE;
            }

            static BOOL IsValidExtensionId(const char* extensionId)
            {
                return IsIdentifier(
                    extensionId, MaxExtensionIdLength, FALSE);
            }

            static BOOL IsValidKey(const char* key)
            {
                return IsIdentifier(key, MaxKeyLength, TRUE);
            }

            static BOOL IsValidString(const char* value)
            {
                if (value == NULL)
                    return FALSE;
                return strlen(value) <= MaxStringLength;
            }

            Entry* FindEntry(const char* extensionId, const char* key) const
            {
                for (int index = 0; index < EntryCount; ++index)
                {
                    if (_stricmp(Entries[index]->ExtensionId, extensionId) == 0 &&
                        _stricmp(Entries[index]->Key, key) == 0)
                    {
                        return Entries[index];
                    }
                }
                return NULL;
            }

            Entry* FindOrCreateEntry(
                const char* extensionId,
                const char* key)
            {
                Entry* entry = FindEntry(extensionId, key);
                if (entry != NULL)
                    return entry;
                if (EntryCount >= MaxEntries)
                    return NULL;

                entry = new Entry;
                if (entry == NULL)
                    return NULL;
                strcpy_s(
                    entry->ExtensionId,
                    _countof(entry->ExtensionId),
                    extensionId);
                strcpy_s(entry->Key, _countof(entry->Key), key);
                Entries[EntryCount++] = entry;
                return entry;
            }

            void ClearEntryValue(Entry* entry)
            {
                if (entry->StringValue != NULL)
                {
                    free(entry->StringValue);
                    entry->StringValue = NULL;
                }
                entry->IntegerValue = 0;
                entry->BooleanValue = FALSE;
                entry->Type = StorageValueMissing;
            }

            void ClearEntries()
            {
                for (int index = 0; index < EntryCount; ++index)
                {
                    ClearEntryValue(Entries[index]);
                    delete Entries[index];
                    Entries[index] = NULL;
                }
                EntryCount = 0;
            }

            static BOOL IsDataAvailable(
                const BYTE* current,
                const BYTE* end,
                size_t length)
            {
                return current <= end &&
                       length <= static_cast<size_t>(end - current);
            }

            static BOOL ValidateBlob(
                const BYTE* data,
                DWORD dataSize,
                DWORD* entryCount)
            {
                if (entryCount != NULL)
                    *entryCount = 0;
                if (data == NULL || dataSize < sizeof(BlobHeader))
                    return FALSE;

                const BYTE* current = data;
                const BYTE* end = data + dataSize;
            BlobHeader header;
            memcpy(&header, current, sizeof(header));
            if (header.Magic != BlobMagic ||
                header.Version != BlobVersion ||
                header.EntryCount > MaxEntries)
            {
                return FALSE;
            }
            current += sizeof(header);

            for (DWORD index = 0; index < header.EntryCount; ++index)
            {
                if (!IsDataAvailable(current, end, sizeof(BlobEntry)))
                    return FALSE;
                BlobEntry serialized;
                memcpy(&serialized, current, sizeof(serialized));
                current += sizeof(serialized);

                if (serialized.ExtensionIdLength == 0 ||
                    serialized.ExtensionIdLength > MaxExtensionIdLength ||
                    serialized.KeyLength == 0 ||
                    serialized.KeyLength > MaxKeyLength)
                {
                    return FALSE;
                }

                size_t namesLength =
                    serialized.ExtensionIdLength + serialized.KeyLength;
                if (!IsDataAvailable(current, end, namesLength))
                    return FALSE;

                    char extensionId[MaxExtensionIdLength + 1];
                    char key[MaxKeyLength + 1];
                    memcpy(
                    extensionId,
                    current,
                    serialized.ExtensionIdLength);
                extensionId[serialized.ExtensionIdLength] = 0;
                current += serialized.ExtensionIdLength;
                memcpy(key, current, serialized.KeyLength);
                key[serialized.KeyLength] = 0;
                current += serialized.KeyLength;
                if (!IsValidExtensionId(extensionId) || !IsValidKey(key))
                    return FALSE;

                DWORD expectedLength = 0;
                if (serialized.Type == StorageValueString)
                {
                    if (serialized.DataLength > MaxStringLength)
                        return FALSE;
                    expectedLength = serialized.DataLength;
                }
                else if (serialized.Type == StorageValueInteger)
                {
                    expectedLength = sizeof(LONGLONG);
                }
                else if (serialized.Type == StorageValueBoolean)
                {
                    expectedLength = sizeof(DWORD);
                    }
                    else
                    {
                        return FALSE;
                    }

                if (serialized.DataLength != expectedLength ||
                    !IsDataAvailable(current, end, expectedLength))
                {
                    return FALSE;
                }
                if (serialized.Type == StorageValueString &&
                    serialized.DataLength > 0 &&
                    memchr(current, 0, serialized.DataLength) != NULL)
                {
                    return FALSE;
                }
                    current += expectedLength;
                }

            if (current != end)
                return FALSE;
            if (entryCount != NULL)
                *entryCount = header.EntryCount;
            return TRUE;
            }

            BOOL LoadBlob(const BYTE* data, DWORD dataSize)
            {
                DWORD serializedCount = 0;
                if (!ValidateBlob(data, dataSize, &serializedCount))
                    return FALSE;

                ClearEntries();
            const BYTE* current = data + sizeof(BlobHeader);
            for (DWORD index = 0; index < serializedCount; ++index)
            {
                BlobEntry serialized;
                memcpy(&serialized, current, sizeof(serialized));
                current += sizeof(serialized);

                char extensionId[MaxExtensionIdLength + 1];
                char key[MaxKeyLength + 1];
                memcpy(
                    extensionId,
                    current,
                    serialized.ExtensionIdLength);
                extensionId[serialized.ExtensionIdLength] = 0;
                current += serialized.ExtensionIdLength;
                memcpy(key, current, serialized.KeyLength);
                key[serialized.KeyLength] = 0;
                current += serialized.KeyLength;

                if (FindEntry(extensionId, key) != NULL)
                {
                    ClearEntries();
                    return FALSE;
                }

                Entry* entry = FindOrCreateEntry(extensionId, key);
                    if (entry == NULL)
                    {
                        ClearEntries();
                        return FALSE;
                }

                entry->Type =
                    static_cast<StorageValueType>(serialized.Type);
                if (entry->Type == StorageValueString)
                {
                    entry->StringValue =
                        static_cast<char*>(malloc(serialized.DataLength + 1));
                    if (entry->StringValue == NULL)
                        {
                            ClearEntries();
                            return FALSE;
                        }
                        memcpy(
                        entry->StringValue,
                        current,
                        serialized.DataLength);
                    entry->StringValue[serialized.DataLength] = 0;
                    }
                    else if (entry->Type == StorageValueInteger)
                    {
                        memcpy(
                            &entry->IntegerValue,
                            current,
                            sizeof(entry->IntegerValue));
                    }
                    else
                    {
                        DWORD booleanValue = 0;
                        memcpy(
                            &booleanValue,
                            current,
                            sizeof(booleanValue));
                        entry->BooleanValue =
                            booleanValue != 0 ? TRUE : FALSE;
                    }
                current += serialized.DataLength;
                }
                Modified = FALSE;
                return TRUE;
            }

            BOOL SaveBlob(BYTE** data, DWORD* dataSize) const
            {
                if (data == NULL || dataSize == NULL)
                    return FALSE;
                *data = NULL;
                *dataSize = 0;

                size_t totalSize = sizeof(BlobHeader);
                for (int index = 0; index < EntryCount; ++index)
                {
                    const Entry* entry = Entries[index];
                    totalSize += sizeof(BlobEntry);
                    totalSize += strlen(entry->ExtensionId);
                    totalSize += strlen(entry->Key);
                    if (entry->Type == StorageValueString)
                        totalSize += strlen(entry->StringValue);
                    else if (entry->Type == StorageValueInteger)
                        totalSize += sizeof(LONGLONG);
                    else
                        totalSize += sizeof(DWORD);
                }
                if (totalSize > MAXDWORD)
                    return FALSE;

                BYTE* blob = static_cast<BYTE*>(malloc(totalSize));
                if (blob == NULL)
                    return FALSE;

                BYTE* current = blob;
                BlobHeader header;
                header.Magic = BlobMagic;
                header.Version = BlobVersion;
                header.EntryCount = static_cast<DWORD>(EntryCount);
                memcpy(current, &header, sizeof(header));
                current += sizeof(header);

                for (int index = 0; index < EntryCount; ++index)
                {
                    const Entry* entry = Entries[index];
                    BlobEntry serialized;
                    serialized.ExtensionIdLength =
                        static_cast<WORD>(strlen(entry->ExtensionId));
                    serialized.KeyLength =
                        static_cast<WORD>(strlen(entry->Key));
                    serialized.Type = static_cast<DWORD>(entry->Type);
                    if (entry->Type == StorageValueString)
                    {
                        serialized.DataLength =
                            static_cast<DWORD>(strlen(entry->StringValue));
                    }
                    else if (entry->Type == StorageValueInteger)
                    {
                        serialized.DataLength = sizeof(LONGLONG);
                    }
                    else
                    {
                        serialized.DataLength = sizeof(DWORD);
                    }

                    memcpy(current, &serialized, sizeof(serialized));
                    current += sizeof(serialized);
                    memcpy(
                        current,
                        entry->ExtensionId,
                        serialized.ExtensionIdLength);
                    current += serialized.ExtensionIdLength;
                    memcpy(current, entry->Key, serialized.KeyLength);
                    current += serialized.KeyLength;

                    if (entry->Type == StorageValueString)
                    {
                        memcpy(
                            current,
                            entry->StringValue,
                            serialized.DataLength);
                    }
                    else if (entry->Type == StorageValueInteger)
                    {
                        memcpy(
                            current,
                            &entry->IntegerValue,
                            sizeof(entry->IntegerValue));
                    }
                    else
                    {
                        DWORD booleanValue =
                            entry->BooleanValue ? 1 : 0;
                        memcpy(
                            current,
                            &booleanValue,
                            sizeof(booleanValue));
                    }
                    current += serialized.DataLength;
                }

                *data = blob;
                *dataSize = static_cast<DWORD>(totalSize);
                return TRUE;
            }

        public:
            StorageService()
                : EntryCount(0),
                  Modified(FALSE)
            {
                memset(Entries, 0, sizeof(Entries));
                InitializeCriticalSection(&Lock);
            }

            virtual ~StorageService()
            {
                ClearEntries();
                DeleteCriticalSection(&Lock);
            }

            virtual DWORD WINAPI GetVersion() const
            {
                return SALAMATRIX_STORAGE_VERSION_1_0;
            }

            const char* GetApiSchema() const
            {
                return "{\"methods\":[\"get\",\"set\",\"remove\",\"clear\",\"keys\"],\"valueTypes\":[\"string\",\"integer\",\"boolean\"]}";
            }

            virtual StorageValueType WINAPI GetValueType(
                const char* extensionId,
                const char* key) const
            {
                if (!IsValidExtensionId(extensionId) || !IsValidKey(key))
                    return StorageValueMissing;
                ScopedLock lock(&Lock);
                Entry* entry = FindEntry(extensionId, key);
                return entry != NULL ? entry->Type : StorageValueMissing;
            }

            virtual BOOL WINAPI GetString(
                const char* extensionId,
                const char* key,
                char* buffer,
                int bufferSize,
                int* requiredSize) const
            {
                if (requiredSize != NULL)
                    *requiredSize = 0;
                if (buffer != NULL && bufferSize > 0)
                    buffer[0] = 0;
                if (!IsValidExtensionId(extensionId) || !IsValidKey(key))
                    return FALSE;

                ScopedLock lock(&Lock);
                Entry* entry = FindEntry(extensionId, key);
                if (entry == NULL || entry->Type != StorageValueString)
                    return FALSE;

                int size = static_cast<int>(strlen(entry->StringValue)) + 1;
                if (requiredSize != NULL)
                    *requiredSize = size;
                if (buffer == NULL || bufferSize < size)
                    return FALSE;
                strcpy_s(buffer, bufferSize, entry->StringValue);
                return TRUE;
            }

            virtual BOOL WINAPI GetInteger(
                const char* extensionId,
                const char* key,
                LONGLONG* value) const
            {
                if (value == NULL ||
                    !IsValidExtensionId(extensionId) ||
                    !IsValidKey(key))
                {
                    return FALSE;
                }
                ScopedLock lock(&Lock);
                Entry* entry = FindEntry(extensionId, key);
                if (entry == NULL || entry->Type != StorageValueInteger)
                    return FALSE;
                *value = entry->IntegerValue;
                return TRUE;
            }

            virtual BOOL WINAPI GetBoolean(
                const char* extensionId,
                const char* key,
                BOOL* value) const
            {
                if (value == NULL ||
                    !IsValidExtensionId(extensionId) ||
                    !IsValidKey(key))
                {
                    return FALSE;
                }
                ScopedLock lock(&Lock);
                Entry* entry = FindEntry(extensionId, key);
                if (entry == NULL || entry->Type != StorageValueBoolean)
                    return FALSE;
                *value = entry->BooleanValue;
                return TRUE;
            }

            virtual BOOL WINAPI SetString(
                const char* extensionId,
                const char* key,
                const char* value)
            {
                if (!IsValidExtensionId(extensionId) ||
                    !IsValidKey(key) ||
                    !IsValidString(value))
                {
                    return FALSE;
                }

                size_t valueLength = strlen(value);
                char* copy =
                    static_cast<char*>(malloc(valueLength + 1));
                if (copy == NULL)
                    return FALSE;
                strcpy_s(copy, valueLength + 1, value);

                ScopedLock lock(&Lock);
                Entry* entry = FindOrCreateEntry(extensionId, key);
                if (entry == NULL)
                {
                    free(copy);
                    return FALSE;
                }
                ClearEntryValue(entry);
                entry->StringValue = copy;
                entry->Type = StorageValueString;
                Modified = TRUE;
                return TRUE;
            }

            virtual BOOL WINAPI SetInteger(
                const char* extensionId,
                const char* key,
                LONGLONG value)
            {
                if (!IsValidExtensionId(extensionId) || !IsValidKey(key))
                    return FALSE;
                ScopedLock lock(&Lock);
                Entry* entry = FindOrCreateEntry(extensionId, key);
                if (entry == NULL)
                    return FALSE;
                ClearEntryValue(entry);
                entry->IntegerValue = value;
                entry->Type = StorageValueInteger;
                Modified = TRUE;
                return TRUE;
            }

            virtual BOOL WINAPI SetBoolean(
                const char* extensionId,
                const char* key,
                BOOL value)
            {
                if (!IsValidExtensionId(extensionId) || !IsValidKey(key))
                    return FALSE;
                ScopedLock lock(&Lock);
                Entry* entry = FindOrCreateEntry(extensionId, key);
                if (entry == NULL)
                    return FALSE;
                ClearEntryValue(entry);
                entry->BooleanValue = value ? TRUE : FALSE;
                entry->Type = StorageValueBoolean;
                Modified = TRUE;
                return TRUE;
            }

            virtual BOOL WINAPI DeleteValue(
                const char* extensionId,
                const char* key)
            {
                if (!IsValidExtensionId(extensionId) || !IsValidKey(key))
                    return FALSE;
                ScopedLock lock(&Lock);
                for (int index = 0; index < EntryCount; ++index)
                {
                    if (_stricmp(
                            Entries[index]->ExtensionId,
                            extensionId) == 0 &&
                        _stricmp(Entries[index]->Key, key) == 0)
                    {
                        ClearEntryValue(Entries[index]);
                        delete Entries[index];
                        for (int move = index;
                             move + 1 < EntryCount;
                             ++move)
                        {
                            Entries[move] = Entries[move + 1];
                        }
                        Entries[--EntryCount] = NULL;
                        Modified = TRUE;
                        return TRUE;
                    }
                }
                return FALSE;
            }

            virtual BOOL WINAPI ClearExtension(const char* extensionId)
            {
                if (!IsValidExtensionId(extensionId))
                    return FALSE;
                ScopedLock lock(&Lock);
                BOOL removed = FALSE;
                for (int index = EntryCount - 1; index >= 0; --index)
                {
                    if (_stricmp(
                            Entries[index]->ExtensionId,
                            extensionId) == 0)
                    {
                        ClearEntryValue(Entries[index]);
                        delete Entries[index];
                        for (int move = index;
                             move + 1 < EntryCount;
                             ++move)
                        {
                            Entries[move] = Entries[move + 1];
                        }
                        Entries[--EntryCount] = NULL;
                        removed = TRUE;
                    }
                }
                if (removed)
                    Modified = TRUE;
                return TRUE;
            }

            virtual int WINAPI GetKeyCount(const char* extensionId) const
            {
                if (!IsValidExtensionId(extensionId))
                    return -1;
                ScopedLock lock(&Lock);
                int count = 0;
                for (int index = 0; index < EntryCount; ++index)
                {
                    if (_stricmp(Entries[index]->ExtensionId, extensionId) == 0)
                        ++count;
                }
                return count;
            }

            virtual BOOL WINAPI GetKeyAt(
                const char* extensionId,
                int index,
                char* keyBuffer,
                int keyBufferSize,
                int* requiredKeySize,
                StorageValueType* type) const
            {
                if (keyBuffer != NULL && keyBufferSize > 0)
                    keyBuffer[0] = 0;
                if (requiredKeySize != NULL)
                    *requiredKeySize = 0;
                if (type != NULL)
                    *type = StorageValueMissing;
                if (!IsValidExtensionId(extensionId) || index < 0)
                    return FALSE;

                ScopedLock lock(&Lock);
                int match = 0;
                Entry* found = NULL;
                for (int entryIndex = 0;
                     entryIndex < EntryCount;
                     ++entryIndex)
                {
                    if (_stricmp(
                            Entries[entryIndex]->ExtensionId,
                            extensionId) != 0)
                        continue;
                    if (match++ == index)
                    {
                        found = Entries[entryIndex];
                        break;
                    }
                }
                if (found == NULL || !IsValidKey(found->Key))
                    return FALSE;
                int required = static_cast<int>(strlen(found->Key)) + 1;
                if (requiredKeySize != NULL)
                    *requiredKeySize = required;
                if (type != NULL)
                    *type = found->Type;
                if (keyBuffer == NULL || keyBufferSize < required)
                    return FALSE;
                strcpy_s(keyBuffer, keyBufferSize, found->Key);
                return TRUE;
            }

            BOOL LoadConfiguration(
                HKEY regKey,
                CSalamanderRegistryAbstract* registry)
            {
                if (regKey == NULL || registry == NULL)
                    return FALSE;
                ScopedLock lock(&Lock);
                ClearEntries();
                Modified = FALSE;

                DWORD dataSize = 0;
                if (!registry->GetSize(
                        regKey, "Storage", REG_BINARY, dataSize))
                {
                    return TRUE;
                }
                if (dataSize == 0)
                    return TRUE;

                BYTE* data = static_cast<BYTE*>(malloc(dataSize));
                if (data == NULL)
                    return FALSE;
                BOOL loaded =
                    registry->GetValue(
                        regKey,
                        "Storage",
                        REG_BINARY,
                        data,
                        dataSize) &&
                    LoadBlob(data, dataSize);
                free(data);
                return loaded;
            }

            BOOL SaveConfiguration(
                HKEY regKey,
                CSalamanderRegistryAbstract* registry)
            {
                if (regKey == NULL || registry == NULL)
                    return FALSE;
                ScopedLock lock(&Lock);
                if (!Modified)
                    return TRUE;

                if (EntryCount == 0)
                {
                    registry->DeleteValue(regKey, "Storage");
                    Modified = FALSE;
                    return TRUE;
                }

                BYTE* data = NULL;
                DWORD dataSize = 0;
                if (!SaveBlob(&data, &dataSize))
                    return FALSE;
                BOOL saved = registry->SetValue(
                    regKey,
                    "Storage",
                    REG_BINARY,
                    data,
                    dataSize);
                free(data);
                if (saved)
                    Modified = FALSE;
                return saved;
            }
        };

    } // namespace Storage
} // namespace Salamatrix
