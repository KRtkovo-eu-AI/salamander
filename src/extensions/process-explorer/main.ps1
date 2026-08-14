Set-StrictMode -Version 2.0

function Get-ProcessExplorerStrings {
    param([string]$Locale)
    $candidates = New-Object System.Collections.Generic.List[string]
    if (-not [string]::IsNullOrWhiteSpace($Locale)) {
        $candidates.Add($Locale)
        $primary = ($Locale -split '-')[0]
        if ($primary -ne $Locale) { $candidates.Add($primary) }
    }
    $candidates.Add('en')
    foreach ($candidate in $candidates) {
        $path = Join-Path $PSScriptRoot "locales\$candidate.json"
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            return (Get-Content -LiteralPath $path -Raw -Encoding UTF8 |
                ConvertFrom-Json).strings
        }
    }
    throw 'The English localization resource is missing.'
}

function Initialize-ProcessNativeMethods {
    if ($null -ne ('OpenSalamander.ProcessExplorer.NativeMethods' -as [type])) { return }
    Add-Type -ErrorAction Stop -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace OpenSalamander.ProcessExplorer
{
    public static class NativeMethods
    {
        private const uint ProcessQueryLimitedInformation = 0x1000;
        private const uint ProcessVmRead = 0x0010;
        private const uint ProcessTerminate = 0x0001;
        private const uint TokenQuery = 0x0008;
        private const int TokenUser = 1;
        private const uint Th32csSnapProcess = 0x00000002;
        private static readonly IntPtr InvalidHandleValue = new IntPtr(-1);

        [StructLayout(LayoutKind.Sequential)]
        private struct SidAndAttributes { public IntPtr Sid; public uint Attributes; }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct ProcessEntry32
        {
            public uint Size;
            public uint Usage;
            public uint ProcessId;
            public IntPtr DefaultHeapId;
            public uint ModuleId;
            public uint Threads;
            public uint ParentProcessId;
            public int BasePriority;
            public uint Flags;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
            public string ExeFile;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct ProcessMemoryCountersEx2
        {
            public uint Size;
            public uint PageFaultCount;
            public UIntPtr PeakWorkingSetSize;
            public UIntPtr WorkingSetSize;
            public UIntPtr QuotaPeakPagedPoolUsage;
            public UIntPtr QuotaPagedPoolUsage;
            public UIntPtr QuotaPeakNonPagedPoolUsage;
            public UIntPtr QuotaNonPagedPoolUsage;
            public UIntPtr PagefileUsage;
            public UIntPtr PeakPagefileUsage;
            public UIntPtr PrivateUsage;
            public UIntPtr PrivateWorkingSetSize;
            public ulong SharedCommitUsage;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct ShellExecuteInfo
        {
            public int Size;
            public uint Mask;
            public IntPtr Window;
            [MarshalAs(UnmanagedType.LPWStr)] public string Verb;
            [MarshalAs(UnmanagedType.LPWStr)] public string File;
            [MarshalAs(UnmanagedType.LPWStr)] public string Parameters;
            [MarshalAs(UnmanagedType.LPWStr)] public string Directory;
            public int Show;
            public IntPtr Instance;
            public IntPtr IdList;
            [MarshalAs(UnmanagedType.LPWStr)] public string Class;
            public IntPtr ClassKey;
            public uint HotKey;
            public IntPtr IconOrMonitor;
            public IntPtr Process;
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenProcess(uint access, bool inheritHandle, int processId);
        [DllImport("kernel32.dll")]
        private static extern bool CloseHandle(IntPtr handle);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool TerminateProcess(IntPtr process, uint exitCode);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool QueryFullProcessImageName(
            IntPtr process, uint flags, StringBuilder path, ref uint pathLength);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr CreateToolhelp32Snapshot(uint flags, uint processId);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool Process32First(IntPtr snapshot, ref ProcessEntry32 entry);
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool Process32Next(IntPtr snapshot, ref ProcessEntry32 entry);
        [DllImport("shell32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool ShellExecuteEx(ref ShellExecuteInfo info);
        [DllImport("psapi.dll", SetLastError = true)]
        private static extern bool GetProcessMemoryInfo(
            IntPtr process, out ProcessMemoryCountersEx2 counters, uint size);
        [DllImport("advapi32.dll", SetLastError = true)]
        private static extern bool OpenProcessToken(IntPtr process, uint access, out IntPtr token);
        [DllImport("advapi32.dll", SetLastError = true)]
        private static extern bool GetTokenInformation(IntPtr token, int informationClass,
            IntPtr information, int informationLength, out int returnLength);
        [DllImport("advapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool LookupAccountSid(string systemName, IntPtr sid,
            StringBuilder name, ref uint nameLength, StringBuilder domain,
            ref uint domainLength, out int use);

        public static string GetUserName(int processId)
        {
            IntPtr process = OpenProcess(ProcessQueryLimitedInformation, false, processId);
            if (process == IntPtr.Zero) return String.Empty;
            try
            {
                IntPtr token;
                if (!OpenProcessToken(process, TokenQuery, out token)) return String.Empty;
                try
                {
                    int length = 0;
                    GetTokenInformation(token, TokenUser, IntPtr.Zero, 0, out length);
                    if (length <= 0) return String.Empty;
                    IntPtr buffer = Marshal.AllocHGlobal(length);
                    try
                    {
                        if (!GetTokenInformation(token, TokenUser, buffer, length, out length))
                            return String.Empty;
                        IntPtr sid = Marshal.PtrToStructure<SidAndAttributes>(buffer).Sid;
                        uint nameLength = 0, domainLength = 0; int use;
                        LookupAccountSid(null, sid, null, ref nameLength, null, ref domainLength, out use);
                        StringBuilder name = new StringBuilder((int)nameLength);
                        StringBuilder domain = new StringBuilder((int)domainLength);
                        if (!LookupAccountSid(null, sid, name, ref nameLength, domain, ref domainLength, out use))
                            return String.Empty;
                        return domain.Length == 0 ? name.ToString() : domain + "\\" + name;
                    }
                    finally { Marshal.FreeHGlobal(buffer); }
                }
                finally { CloseHandle(token); }
            }
            finally { CloseHandle(process); }
        }

        public static string GetExecutablePath(int processId)
        {
            IntPtr process = OpenProcess(ProcessQueryLimitedInformation, false, processId);
            if (process == IntPtr.Zero) return String.Empty;
            try
            {
                uint length = 32768;
                StringBuilder path = new StringBuilder((int)length);
                return QueryFullProcessImageName(process, 0, path, ref length)
                    ? path.ToString() : String.Empty;
            }
            finally { CloseHandle(process); }
        }

        public static ulong GetPrivateWorkingSet(int processId)
        {
            IntPtr process = OpenProcess(
                ProcessQueryLimitedInformation | ProcessVmRead,
                false, processId);
            if (process == IntPtr.Zero) return 0;
            try
            {
                ProcessMemoryCountersEx2 counters = new ProcessMemoryCountersEx2();
                counters.Size = (uint)Marshal.SizeOf(typeof(ProcessMemoryCountersEx2));
                return GetProcessMemoryInfo(process, out counters, counters.Size)
                    ? counters.PrivateWorkingSetSize.ToUInt64() : 0;
            }
            finally { CloseHandle(process); }
        }

        private static void AddTerminationOrder(
            int processId, Dictionary<int, List<int>> children,
            HashSet<int> visited, List<int> order)
        {
            if (!visited.Add(processId)) return;
            List<int> childIds;
            if (children.TryGetValue(processId, out childIds))
                foreach (int childId in childIds)
                    AddTerminationOrder(childId, children, visited, order);
            order.Add(processId);
        }

        private static List<int> GetTerminationOrder(int processId)
        {
            Dictionary<int, List<int>> children = new Dictionary<int, List<int>>();
            IntPtr snapshot = CreateToolhelp32Snapshot(Th32csSnapProcess, 0);
            if (snapshot != InvalidHandleValue)
            {
                try
                {
                    ProcessEntry32 entry = new ProcessEntry32();
                    entry.Size = (uint)Marshal.SizeOf(typeof(ProcessEntry32));
                    if (Process32First(snapshot, ref entry))
                    {
                        do
                        {
                            int parentId = unchecked((int)entry.ParentProcessId);
                            List<int> childIds;
                            if (!children.TryGetValue(parentId, out childIds))
                            {
                                childIds = new List<int>();
                                children[parentId] = childIds;
                            }
                            childIds.Add(unchecked((int)entry.ProcessId));
                        }
                        while (Process32Next(snapshot, ref entry));
                    }
                }
                finally { CloseHandle(snapshot); }
            }
            List<int> order = new List<int>();
            AddTerminationOrder(processId, children, new HashSet<int>(), order);
            return order;
        }

        public static bool EndProcess(int processId, bool tree, out int error)
        {
            error = 0;
            List<int> order = tree ? GetTerminationOrder(processId)
                                   : new List<int>(new int[] { processId });
            foreach (int id in order)
            {
                IntPtr process = OpenProcess(ProcessTerminate, false, id);
                if (process == IntPtr.Zero)
                {
                    int currentError = Marshal.GetLastWin32Error();
                    if ((id == processId || currentError != 87) && error == 0)
                        error = currentError;
                    continue;
                }
                try
                {
                    if (!TerminateProcess(process, 1) && error == 0)
                        error = Marshal.GetLastWin32Error();
                }
                finally { CloseHandle(process); }
            }
            return error == 0;
        }

        private static bool ExecuteShellVerb(
            string verb, string file, string parameters, out int error)
        {
            ShellExecuteInfo info = new ShellExecuteInfo();
            info.Size = Marshal.SizeOf(typeof(ShellExecuteInfo));
            info.Mask = 0u;
            info.Verb = verb;
            info.File = file;
            info.Parameters = parameters;
            info.Show = 1;
            bool result = ShellExecuteEx(ref info);
            error = result ? 0 : Marshal.GetLastWin32Error();
            return result;
        }

        public static bool OpenFileLocation(string path, out int error)
        {
            return ExecuteShellVerb("open", "explorer.exe",
                "/select,\"" + path + "\"", out error);
        }

    }
}
'@
}

function Test-ProcessSuspended {
    param([System.Diagnostics.Process]$Process)
    try {
        $threads = @($Process.Threads)
        if ($threads.Count -eq 0) { return $false }
        foreach ($thread in $threads) {
            if ($thread.ThreadState -ne [System.Diagnostics.ThreadState]::Wait -or
                $thread.WaitReason -ne [System.Diagnostics.ThreadWaitReason]::Suspended) {
                return $false
            }
        }
        return $true
    } catch { return $false }
}

$handler = [string]$Salamander.command_handler
$locale = try { [string]$Salamander.application.Language() } catch { 'en' }
$strings = Get-ProcessExplorerStrings $locale

function Show-ProcessExplorerError {
    param([string]$Message, [int]$ErrorCode = 0)
    if ($ErrorCode -ne 0) {
        $Message = [string]::Format($Message, $ErrorCode)
    }
    [void]$Salamander.ui.MessageBox(
        $Message, [string]$strings.errorTitle, 'OK', 'Error')
}

if ($handler -in @('endTask', 'endProcessTree', 'openFileLocation', 'properties')) {
    Initialize-ProcessNativeMethods
    $processId = 0
    if ($null -eq $Salamander.invocation.item -or
        -not [int]::TryParse([string]$Salamander.invocation.item.id, [ref]$processId) -or
        $processId -lt 0) {
        Show-ProcessExplorerError ([string]$strings.processUnavailable)
        return
    }
    if ($handler -eq 'endTask' -or $handler -eq 'endProcessTree') {
        $errorCode = 0
        $tree = $handler -eq 'endProcessTree'
        if (-not [OpenSalamander.ProcessExplorer.NativeMethods]::EndProcess(
                $processId, $tree, [ref]$errorCode)) {
            Show-ProcessExplorerError ([string]$strings.endFailed) $errorCode
        }
        return
    }
    $path = [OpenSalamander.ProcessExplorer.NativeMethods]::GetExecutablePath($processId)
    if ([string]::IsNullOrWhiteSpace($path)) {
        Show-ProcessExplorerError ([string]$strings.pathUnavailable)
        return
    }
    $errorCode = 0
    if ($handler -eq 'openFileLocation') {
        if (-not [OpenSalamander.ProcessExplorer.NativeMethods]::OpenFileLocation(
                $path, [ref]$errorCode)) {
            Show-ProcessExplorerError ([string]$strings.openLocationFailed) $errorCode
        }
    } else {
        $propertiesResult = $Salamander.ui.FileProperties($path)
        if ($null -eq $propertiesResult -or -not $propertiesResult.shown) {
            $propertiesError = if ($null -ne $propertiesResult) {
                [int]$propertiesResult.error
            } else { 31 }
            Show-ProcessExplorerError ([string]$strings.propertiesFailed) `
                $propertiesError
        }
    }
    return
}

if ($handler -ne 'listProcesses') { return }

Initialize-ProcessNativeMethods
$items = New-Object 'System.Collections.Generic.List[hashtable]'
$knownExecutablePaths = @{}
$itemsByProcessId = @{}
$initialCpuTicks = @{}
$cpuSampleStarted = [System.Diagnostics.Stopwatch]::GetTimestamp()
$processes = try {
    @(Get-Process -IncludeUserName -ErrorAction Stop |
        Sort-Object ProcessName, Id)
} catch {
    @(Get-Process | Sort-Object ProcessName, Id)
}
foreach ($process in $processes) {
    try {
        $name = $null
        $executablePath = ''
        try {
            $mainModule = $process.MainModule
            $name = [string]$mainModule.ModuleName
            $executablePath = [string]$mainModule.FileName
        } catch {}
        if ([string]::IsNullOrWhiteSpace($executablePath)) {
            try {
                $executablePath =
                    [OpenSalamander.ProcessExplorer.NativeMethods]::GetExecutablePath(
                        $process.Id)
            } catch {}
        }
        if ($process.Id -eq 0) { $name = 'System Idle Process' }
        if ([string]::IsNullOrWhiteSpace($name)) {
            $name = [string]$process.ProcessName
            if ($name -notmatch '\.' -and $name -notin @(
                'System', 'Registry', 'Memory Compression', 'Secure System',
                'System Idle Process')) {
                $name += '.exe'
            }
        }
        $status = if ($process.HasExited) {
            [string]$strings.ended
        } elseif (Test-ProcessSuspended $process) {
            [string]$strings.suspended
        } else {
            [string]$strings.running
        }
        $userName = try { [string]$process.UserName } catch { '' }
        $processIdText = [string]$process.Id
        $privateWorkingSet =
            [OpenSalamander.ProcessExplorer.NativeMethods]::GetPrivateWorkingSet(
                $process.Id)
        $memoryText = if ($privateWorkingSet -gt 0) {
            ([Math]::Round($privateWorkingSet / 1KB)).ToString(
                [Globalization.CultureInfo]::InvariantCulture) + ' K'
        } else { '' }
        $item = @{
            id=$processIdText; name=$name
            compactName=($name + ' [' + $processIdText + ']')
            directory=$false; enabled=$true; columns=@{
                pid=$processIdText; status=$status; userName=$userName
                cpu=''; memory=$memoryText }}
        try {
            $initialCpuTicks[$processIdText] =
                [long]$process.TotalProcessorTime.Ticks
        } catch {}
        $itemsByProcessId[$processIdText] = $item
        if (-not [string]::IsNullOrWhiteSpace($executablePath)) {
            $item.fileIcon = $executablePath
            $pathKey = $name.ToUpperInvariant()
            if (-not $knownExecutablePaths.ContainsKey($pathKey)) {
                $knownExecutablePaths[$pathKey] = $executablePath
            } elseif ($knownExecutablePaths[$pathKey] -ne $executablePath) {
                # Do not guess when identical names originate in different folders.
                $knownExecutablePaths[$pathKey] = ''
            }
        }
        $items.Add($item)
    } catch {
        # Processes may exit or become inaccessible while the snapshot is built.
    } finally {
        $process.Dispose()
    }
}
$minimumSampleSeconds = 0.25
$sampleElapsed = ([System.Diagnostics.Stopwatch]::GetTimestamp() -
    $cpuSampleStarted) / [double][System.Diagnostics.Stopwatch]::Frequency
if ($sampleElapsed -lt $minimumSampleSeconds) {
    Start-Sleep -Milliseconds ([int][Math]::Ceiling(
        ($minimumSampleSeconds - $sampleElapsed) * 1000.0))
}
$sampleElapsed = ([System.Diagnostics.Stopwatch]::GetTimestamp() -
    $cpuSampleStarted) / [double][System.Diagnostics.Stopwatch]::Frequency
$processorCount = [Math]::Max(1, [Environment]::ProcessorCount)
foreach ($process in @(Get-Process -ErrorAction SilentlyContinue)) {
    try {
        $processIdText = [string]$process.Id
        if ($initialCpuTicks.ContainsKey($processIdText) -and
            $itemsByProcessId.ContainsKey($processIdText)) {
            $cpuTicks = [long]$process.TotalProcessorTime.Ticks -
                [long]$initialCpuTicks[$processIdText]
            if ($cpuTicks -ge 0 -and $sampleElapsed -gt 0) {
                $cpuPercent = 100.0 * $cpuTicks /
                    [TimeSpan]::TicksPerSecond / $sampleElapsed /
                    $processorCount
                $itemsByProcessId[$processIdText].columns.cpu =
                    ([Math]::Min(100.0, $cpuPercent)).ToString(
                        '0.0', [Globalization.CultureInfo]::InvariantCulture) +
                    ' %'
            }
        }
    } catch {
        # The process can exit between the two CPU samples.
    } finally {
        $process.Dispose()
    }
}
foreach ($item in $items) {
    if (-not $item.ContainsKey('fileIcon')) {
        $pathKey = ([string]$item.name).ToUpperInvariant()
        if ($knownExecutablePaths.ContainsKey($pathKey) -and
            -not [string]::IsNullOrWhiteSpace(
                [string]$knownExecutablePaths[$pathKey])) {
            $item.fileIcon = [string]$knownExecutablePaths[$pathKey]
        }
    }
}
[void]$Salamander.file_system.AddItems($items.ToArray())
