// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;

namespace OpenSalamander.Samandarin;

internal static class AuthenticodeVerifier
{
    private static readonly string[] ExpectedPublisherNeedles =
    {
        "Open Source Developer",
        "Kotas",
        "KRtkovo",
    };

    private static readonly HashSet<string> UnsignedRedistributableNames = new(StringComparer.OrdinalIgnoreCase)
    {
        "7za.dll",
        "7zwrapper.dll",
        "unrar.dll",
        "chmlib.dll",
        "sqlite.dll",
        "libeay32.dll",
        "ssleay32.dll",
        "Newtonsoft.Json.dll",
        "WebView2Loader.dll",
        "dbghelp.dll",
        "ucrtbase.dll",
        "vcruntime140.dll",
        "vcruntime140_1.dll",
        "msvcp140.dll",
        "concrt140.dll",
        "lua.dll",
        "lua.exe",
        "python.exe",
        "python3.dll",
        "python311.dll",
        "python312.dll",
        "python313.dll",
    };

    private static readonly string[] SignedPackageExtensions = { ".spl", ".dll", ".exe", ".slg" };

    public static void VerifyExtractedPackage(string extractedRoot, OfficialPackageKind kind)
    {
        var files = Directory.EnumerateFiles(extractedRoot, "*.*", SearchOption.AllDirectories)
            .Where(path => SignedPackageExtensions.Contains(Path.GetExtension(path), StringComparer.OrdinalIgnoreCase))
            .ToList();
        if (kind == OfficialPackageKind.Plugin &&
            !files.Any(path => string.Equals(Path.GetExtension(path), ".spl", StringComparison.OrdinalIgnoreCase)))
        {
            throw new InvalidDataException(NativeStrings.Get(NativeStringId.PluginInstallUnsignedBinary));
        }

        var firstPartySigned = false;
        foreach (var file in files)
        {
            var extension = Path.GetExtension(file);
            var fileName = Path.GetFileName(file);
            var isFirstParty = string.Equals(extension, ".spl", StringComparison.OrdinalIgnoreCase) ||
                               string.Equals(extension, ".slg", StringComparison.OrdinalIgnoreCase);
            if (!isFirstParty && IsUnsignedRedistributable(fileName))
            {
                continue;
            }

            if (!TryVerifyTrust(file, out var publisher))
            {
                throw new InvalidDataException(
                    NativeStrings.Format(NativeStringId.PluginInstallUnsignedBinaryNamed, fileName));
            }

            if (isFirstParty)
            {
                if (!IsExpectedPublisher(publisher))
                {
                    throw new InvalidDataException(
                        NativeStrings.Format(NativeStringId.PluginInstallUnexpectedPublisher, fileName, publisher));
                }
                firstPartySigned = true;
            }
        }

        if (kind == OfficialPackageKind.Plugin && !firstPartySigned)
        {
            throw new InvalidDataException(NativeStrings.Get(NativeStringId.PluginInstallUnsignedBinary));
        }
    }

    public static bool TryGetPublisher(string path, out string publisher)
    {
        publisher = string.Empty;
        try
        {
            if (!TryVerifyTrust(path, out publisher))
            {
                return false;
            }
            return !string.IsNullOrWhiteSpace(publisher);
        }
        catch
        {
            publisher = string.Empty;
            return false;
        }
    }

    internal static bool IsExpectedPublisher(string publisher)
    {
        if (string.IsNullOrWhiteSpace(publisher))
        {
            return false;
        }

        return ExpectedPublisherNeedles.Any(needle =>
            publisher.IndexOf(needle, StringComparison.OrdinalIgnoreCase) >= 0);
    }

    private static bool IsUnsignedRedistributable(string fileName)
    {
        if (UnsignedRedistributableNames.Contains(fileName))
        {
            return true;
        }

        return fileName.StartsWith("api-ms-win-", StringComparison.OrdinalIgnoreCase) ||
               fileName.StartsWith("System.", StringComparison.OrdinalIgnoreCase) ||
               fileName.StartsWith("Microsoft.Web.WebView2.", StringComparison.OrdinalIgnoreCase) ||
               fileName.StartsWith("WebView2", StringComparison.OrdinalIgnoreCase);
    }

    private static bool TryVerifyTrust(string path, out string publisher)
    {
        publisher = string.Empty;
        var fileInfo = new WINTRUST_FILE_INFO
        {
            cbStruct = (uint)Marshal.SizeOf(typeof(WINTRUST_FILE_INFO)),
            pcwszFilePath = path,
        };
        var fileInfoPtr = Marshal.AllocHGlobal(Marshal.SizeOf(fileInfo));
        try
        {
            Marshal.StructureToPtr(fileInfo, fileInfoPtr, false);
            var data = new WINTRUST_DATA
            {
                cbStruct = (uint)Marshal.SizeOf(typeof(WINTRUST_DATA)),
                dwUIChoice = 2, // WTD_UI_NONE
                fdwRevocationChecks = 0, // WTD_REVOKE_NONE
                dwUnionChoice = 1, // WTD_CHOICE_FILE
                pFile = fileInfoPtr,
                dwStateAction = 1, // WTD_STATEACTION_VERIFY
                dwProvFlags = 0x10, // WTD_CACHE_ONLY_URL_RETRIEVAL
            };
            var action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
            var result = WinVerifyTrust(new IntPtr(-1), ref action, ref data);
            data.dwStateAction = 2; // WTD_STATEACTION_CLOSE
            WinVerifyTrust(new IntPtr(-1), ref action, ref data);
            if (result != 0)
            {
                return false;
            }
        }
        finally
        {
            Marshal.FreeHGlobal(fileInfoPtr);
        }

        try
        {
            using var certificate = new X509Certificate2(X509Certificate.CreateFromSignedFile(path));
            publisher = certificate.GetNameInfo(X509NameType.SimpleName, false);
            if (string.IsNullOrWhiteSpace(publisher))
            {
                publisher = certificate.Subject;
            }
            return true;
        }
        catch
        {
            return false;
        }
    }

    private static readonly Guid WINTRUST_ACTION_GENERIC_VERIFY_V2 =
        new("00AAC56B-CD44-11d0-8CC2-00C04FC295EE");

    [DllImport("wintrust.dll", ExactSpelling = true)]
    private static extern uint WinVerifyTrust(IntPtr hwnd, [In] ref Guid pgActionID, [In] ref WINTRUST_DATA pWVTData);

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct WINTRUST_FILE_INFO
    {
        public uint cbStruct;
        public string pcwszFilePath;
        public IntPtr hFile;
        public IntPtr pgKnownSubject;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct WINTRUST_DATA
    {
        public uint cbStruct;
        public IntPtr pPolicyCallbackData;
        public IntPtr pSIPClientData;
        public uint dwUIChoice;
        public uint fdwRevocationChecks;
        public uint dwUnionChoice;
        public IntPtr pFile;
        public uint dwStateAction;
        public IntPtr hWVTStateData;
        public IntPtr pwszURLReference;
        public uint dwProvFlags;
        public uint dwUIContext;
        public IntPtr pSignatureSettings;
    }
}
