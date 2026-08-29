# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
FTP_FS = REPOSITORY_ROOT / "src" / "plugins" / "ftp" / "fs2.cpp"


def main() -> None:
    source = FTP_FS.read_text(encoding="utf-8")
    restore_start = source.index("int encryptedControlConnection = currentFSNameIndex")
    restore_end = source.index("ControlConnection->SetStartTime();", restore_start)
    restore = source[restore_start:restore_end]

    required_profile_fields = (
        "server->InitialPath",
        "server->ServerType",
        "server->UseListingsCache",
        "server->UsePassiveMode",
        "server->KeepConnectionAlive",
        "server->KeepAliveSendEvery",
        "server->KeepAliveStopAfter",
        "server->KeepAliveCommand",
        "server->TransferMode",
        "server->ProxyServerUID",
        "server->EncryptDataConnection",
        "server->CompressData",
        "server->InitFTPCommands",
        "server->ListCommand",
    )
    missing = [field for field in required_profile_fields if field not in restore]
    if missing:
        raise AssertionError(f"restored FTP connection ignores profile fields: {missing}")

    if "bookmark->AnonymousConnection ? strcmp(user, FTP_ANONYMOUS) == 0" not in restore:
        raise AssertionError("bookmark lookup must match anonymous profiles explicitly")
    if "(bookmark->EncryptControlConnection == 1) == encryptedControlConnection" not in restore:
        raise AssertionError("bookmark lookup must distinguish FTP from FTPS")
    if restore.index("if (path != NULL)") > restore.index("else if (server != NULL)"):
        raise AssertionError("the saved current path must take precedence over the profile initial path")
    if "password == NULL" not in restore or "server->SavePassword" not in restore:
        raise AssertionError("profile password must be used only when the URL has no password")
    if "proxyUIDToCheck" not in restore or "EnsurePasswordCanBeDecrypted" not in restore:
        raise AssertionError("the selected profile proxy password must be validated")


if __name__ == "__main__":
    main()
