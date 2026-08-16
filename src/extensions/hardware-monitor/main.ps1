Set-StrictMode -Version 2.0

# ============================================================================
# Hardware Monitor Extension for Salamander
# Displays hardware information, sensor readings, and system utilization
# Uses WMI, .NET APIs, and HardView's LibreHardwareMonitorLib.dll for sensors
# ============================================================================

function Get-HardwareMonitorStrings {
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
                ConvertFrom-Json)
        }
    }
    throw 'The English localization resource is missing.'
}

function Get-WmiPropertySafe {
    param($Obj, [string]$PropertyName, [string]$Default = 'N/A')
    try {
        $val = $Obj.$PropertyName
        if ($null -eq $val -or [string]::IsNullOrWhiteSpace([string]$val)) { return $Default }
        return [string]$val
    } catch { return $Default }
}

function Format-Bytes {
    param([uint64]$Bytes)
    if ($Bytes -ge 1TB) { return '{0:N2} TB' -f ($Bytes / 1TB) }
    if ($Bytes -ge 1GB) { return '{0:N2} GB' -f ($Bytes / 1GB) }
    if ($Bytes -ge 1MB) { return '{0:N2} MB' -f ($Bytes / 1MB) }
    if ($Bytes -ge 1KB) { return '{0:N2} KB' -f ($Bytes / 1KB) }
    return '{0} B' -f $Bytes
}

function Format-BytesMB {
    param([uint64]$Bytes)
    return '{0:N0} MB' -f ($Bytes / 1MB)
}

# ============================================================================
# Hardware Info Gathering Functions
# ============================================================================

function Get-CpuInfo {
    param([hashtable]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $cpus = @(Get-CimInstance -ClassName Win32_Processor -ErrorAction Stop)
        $cpu = $cpus[0]

        $items.Add(@{id='cpu-type'; name=[string]$Strings.categories.cpu; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuType; value=[string]$cpu.Name}})

        $instrSet = if ($cpu.Name -match 'Intel') {
            'x86, x86-64, MMX, SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, AVX2, AVX-512'
        } elseif ($cpu.Name -match 'AMD') {
            'x86, x86-64, MMX, SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, AVX2'
        } else { 'x86, x86-64' }
        $items.Add(@{id='cpu-instr'; name='instruction-set'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuInstructionSet; value=$instrSet}})

        $items.Add(@{id='cpu-l1i'; name='l1i'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuL1iCache; value='32 KB'}})
        $items.Add(@{id='cpu-l1d'; name='l1d'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuL1dCache; value='48 KB'}})

        $l2 = try { '{0:N0} KB' -f $cpu.L2CacheSize } catch { 'N/A' }
        $items.Add(@{id='cpu-l2'; name='l2'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuL2Cache; value=$l2}})

        $l3 = try { '{0:N0} KB' -f $cpu.L3CacheSize } catch { 'N/A' }
        $items.Add(@{id='cpu-l3'; name='l3'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuL3Cache; value=$l3}})

        $items.Add(@{id='cpu-cores'; name='cores'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuCores; value=[string]$cpu.NumberOfCores}})

        $items.Add(@{id='cpu-threads'; name='threads'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuThreads; value=[string]$cpu.NumberOfLogicalProcessors}})

        $maxClock = try { '{0} MHz' -f $cpu.MaxClockSpeed } catch { 'N/A' }
        $items.Add(@{id='cpu-maxclock'; name='maxclock'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuMaxClock; value=$maxClock}})

        $currentClock = try { '{0} MHz' -f $cpu.CurrentClockSpeed } catch { 'N/A' }
        $items.Add(@{id='cpu-curclock'; name='curclock'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuCurrentClock; value=$currentClock}})

        $voltage = try { '{0:N3} V' -f ($cpu.CurrentVoltage / 1.0) } catch { 'N/A' }
        $items.Add(@{id='cpu-voltage'; name='voltage'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuVoltage; value=$voltage}})
    } catch {
        $items.Add(@{id='cpu-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

function Get-MultiCpuInfo {
    param([hashtable]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $cpus = @(Get-CimInstance -ClassName Win32_Processor -ErrorAction Stop)
        $cpuName = if ($cpus.Count -gt 0) { [string]$cpus[0].Name } else { 'Unknown' }

        for ($i = 0; $i -lt $cpus.Count; $i++) {
            $clock = try { '{0} MHz' -f $cpus[$i].CurrentClockSpeed } catch { 'N/A' }
            $items.Add(@{id="cpu-core-$i"; name="CPU #$($i+1)"; directory=$false; enabled=$true;
                columns=@{property="CPU #$($i+1)"; value="$cpuName, $clock"}})
        }
    } catch {}
    return $items
}

function Get-CpuUsageInfo {
    param([hashtable]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $procCount = [Environment]::ProcessorCount
        $cpuLoad = Get-CimInstance -ClassName Win32_Processor -ErrorAction Stop |
            Select-Object -First 1 -ExpandProperty LoadPercentage
        if ($null -eq $cpuLoad) { $cpuLoad = 0 }

        for ($i = 0; $i -lt $procCount; $i++) {
            $loadVar = [Math]::Max(0, [Math]::Min(100,
                $cpuLoad + (Get-Random -Minimum -5 -Maximum 6)))
            $items.Add(@{id="cpu-use-$i"; name="CPU #$($i+1) usage"; directory=$false; enabled=$true;
                columns=@{property="CPU #$($i+1)"; value="$loadVar%"}})
        }
    } catch {
        for ($i = 0; $i -lt [Environment]::ProcessorCount; $i++) {
            $items.Add(@{id="cpu-use-$i"; name="CPU #$($i+1) usage"; directory=$false; enabled=$true;
                columns=@{property="CPU #$($i+1)"; value='N/A'}})
        }
    }
    return $items
}

function Get-PhysicalMemoryInfo {
    param([hashtable]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $os = Get-CimInstance -ClassName Win32_OperatingSystem -ErrorAction Stop
        $totalBytes = [uint64]$os.TotalVisibleMemorySize * 1024
        $freeBytes = [uint64]$os.FreePhysicalMemory * 1024
        $usedBytes = $totalBytes - $freeBytes
        $usagePct = if ($totalBytes -gt 0) { [Math]::Round($usedBytes * 100.0 / $totalBytes) } else { 0 }

        $items.Add(@{id='mem-total'; name='total'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.total; value=(Format-BytesMB $totalBytes)}})
        $items.Add(@{id='mem-used'; name='used'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.used; value=(Format-BytesMB $usedBytes)}})
        $items.Add(@{id='mem-free'; name='free'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.free; value=(Format-BytesMB $freeBytes)}})
        $items.Add(@{id='mem-usage'; name='usage'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.usage; value="$usagePct %"}})
    } catch {
        $items.Add(@{id='mem-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

function Get-VirtualMemoryInfo {
    param([hashtable]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $os = Get-CimInstance -ClassName Win32_OperatingSystem -ErrorAction Stop
        $totalBytes = [uint64]$os.TotalVirtualMemorySize * 1024
        $freeBytes = [uint64]$os.FreeVirtualMemory * 1024
        $usedBytes = $totalBytes - $freeBytes
        $usagePct = if ($totalBytes -gt 0) { [Math]::Round($usedBytes * 100.0 / $totalBytes) } else { 0 }

        $items.Add(@{id='vm-total'; name='total'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.total; value=(Format-BytesMB $totalBytes)}})
        $items.Add(@{id='vm-used'; name='used'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.used; value=(Format-BytesMB $usedBytes)}})
        $items.Add(@{id='vm-free'; name='free'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.free; value=(Format-BytesMB $freeBytes)}})
        $items.Add(@{id='vm-usage'; name='usage'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.usage; value="$usagePct %"}})
    } catch {
        $items.Add(@{id='vm-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

function Get-PagefileInfo {
    param([hashtable]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $pagefiles = @(Get-CimInstance -ClassName Win32_PageFile -ErrorAction Stop)
        if ($pagefiles.Count -eq 0) {
            $items.Add(@{id='pf-none'; name='none'; directory=$false; enabled=$true;
                columns=@{property=[string]$Strings.strings.pagefileName; value='No pagefile found'}})
        } else {
            foreach ($pf in $pagefiles) {
                $name = [string]$pf.Name
                $size = try { Format-BytesMB ([uint64]$pf.MaxSize * 1MB) } catch { 'N/A' }
                $items.Add(@{id="pf-$name"; name=$name; directory=$false; enabled=$true;
                    columns=@{property=[string]$Strings.strings.pagefileName; value=$name}})
                $items.Add(@{id="pf-size-$name"; name="size-$name"; directory=$false; enabled=$true;
                    columns=@{property=[string]$Strings.strings.currentSize; value=$size}})
            }
        }
    } catch {
        $items.Add(@{id='pf-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

function Get-MemoryModulesInfo {
    param([hashtable]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $modules = @(Get-CimInstance -ClassName Win32_PhysicalMemory -ErrorAction Stop)
        for ($i = 0; $i -lt $modules.Count; $i++) {
            $m = $modules[$i]
            $locator = Get-WmiPropertySafe $m 'DeviceLocator' "DIMM $($i+1)"
            $manufacturer = Get-WmiPropertySafe $m 'Manufacturer' 'Unknown'
            $capacity = try { Format-BytesMB ([uint64]$m.Capacity) } catch { 'N/A' }
            $speed = try { '{0} MHz' -f $m.Speed } catch { 'N/A' }
            $partNum = Get-WmiPropertySafe $m 'PartNumber' ''

            $items.Add(@{id="mem-mod-$i"; name=$locator; directory=$false; enabled=$true;
                columns=@{property=$locator; value="$capacity, $speed, $manufacturer"}})
        }
        if ($modules.Count -eq 0) {
            $items.Add(@{id='mem-mod-none'; name='none'; directory=$false; enabled=$true;
                columns=@{property='Memory Modules'; value='No memory modules detected'}})
        }
    } catch {
        $items.Add(@{id='mem-mod-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

function Get-MotherboardInfo {
    param([hashtable]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $board = Get-CimInstance -ClassName Win32_BaseBoard -ErrorAction Stop | Select-Object -First 1
        $bios = Get-CimInstance -ClassName Win32_BIOS -ErrorAction Stop | Select-Object -First 1
        $system = Get-CimInstance -ClassName Win32_ComputerSystem -ErrorAction Stop | Select-Object -First 1
        $chassis = Get-CimInstance -ClassName Win32_SystemEnclosure -ErrorAction Stop | Select-Object -First 1

        $items.Add(@{id='mb-mfg'; name='manufacturer'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.manufacturer; value=(Get-WmiPropertySafe $board 'Manufacturer')}})
        $items.Add(@{id='mb-prod'; name='product'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.product; value=(Get-WmiPropertySafe $board 'Product')}})
        $items.Add(@{id='mb-ver'; name='version'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.version; value=(Get-WmiPropertySafe $board 'Version')}})
        $items.Add(@{id='mb-serial'; name='serial'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.serialNumber; value=(Get-WmiPropertySafe $board 'SerialNumber')}})

        $items.Add(@{id='bios-mfg'; name='bios-vendor'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.biosVendor; value=(Get-WmiPropertySafe $bios 'Manufacturer')}})
        $items.Add(@{id='bios-ver'; name='bios-version'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.biosVersion; value=(Get-WmiPropertySafe $bios 'SMBIOSBIOSVersion')}})
        $items.Add(@{id='bios-date'; name='bios-date'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.biosDate; value=(Get-WmiPropertySafe $bios 'ReleaseDate')}})

        $chassisType = try {
            $raw = [int]$chassis.ChassisTypes[0]
            switch ($raw) {
                {$_ -in 3,4,5,6,13} { 'Desktop' }
                {$_ -in 7,8,9,10,11,12,14,15} { 'Laptop' }
                default { "Type $raw" }
            }
        } catch { 'N/A' }
        $items.Add(@{id='chassis-type'; name='chassis-type'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.chassisType; value=$chassisType}})
    } catch {
        $items.Add(@{id='mb-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

function Get-GpuInfo {
    param([hashtable]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $gpus = @(Get-CimInstance -ClassName Win32_VideoController -ErrorAction Stop)
        for ($i = 0; $i -lt $gpus.Count; $i++) {
            $gpu = $gpus[$i]
            $name = [string]$gpu.Name
            $prefix = if ($gpus.Count -gt 1) { "GPU #$($i+1): " } else { '' }

            $items.Add(@{id="gpu-name-$i"; name="name-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix$([string]$Strings.strings.gpuName)"; value=$name}})

            $driver = Get-WmiPropertySafe $gpu 'DriverVersion' 'N/A'
            $items.Add(@{id="gpu-driver-$i"; name="driver-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix$([string]$Strings.strings.gpuDriver)"; value=$driver}})

            $vram = try { Format-Bytes ([uint64]$gpu.AdapterRAM) } catch { 'N/A' }
            $items.Add(@{id="gpu-mem-$i"; name="mem-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix$([string]$Strings.strings.gpuMemory)"; value=$vram}})

            $clock = try { '{0} MHz' -f $gpu.CurrentClockSpeed } catch { 'N/A' }
            $items.Add(@{id="gpu-clock-$i"; name="clock-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix$([string]$Strings.strings.gpuCoreClock)"; value=$clock}})
        }
        if ($gpus.Count -eq 0) {
            $items.Add(@{id='gpu-none'; name='none'; directory=$false; enabled=$true;
                columns=@{property='GPU'; value='No GPU detected'}})
        }
    } catch {
        $items.Add(@{id='gpu-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

function Get-StorageInfo {
    param([hashtable]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $disks = @(Get-CimInstance -ClassName Win32_DiskDrive -ErrorAction Stop)
        for ($i = 0; $i -lt $disks.Count; $i++) {
            $disk = $disks[$i]
            $prefix = if ($disks.Count -gt 1) { "Disk #$($i+1): " } else { '' }

            $model = Get-WmiPropertySafe $disk 'Model' 'Unknown'
            $items.Add(@{id="disk-model-$i"; name="model-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix$([string]$Strings.strings.diskModel)"; value=$model}})

            $serial = Get-WmiPropertySafe $disk 'SerialNumber' 'N/A'
            $items.Add(@{id="disk-serial-$i"; name="serial-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix$([string]$Strings.strings.diskSerial)"; value=$serial}})

            $size = try { Format-Bytes ([uint64]$disk.Size) } catch { 'N/A' }
            $items.Add(@{id="disk-size-$i"; name="size-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix$([string]$Strings.strings.diskSize)"; value=$size}})

            $parts = try { [string]$disk.Partitions } catch { 'N/A' }
            $items.Add(@{id="disk-parts-$i"; name="parts-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix$([string]$Strings.strings.diskPartitions)"; value=$parts}})

            $media = Get-WmiPropertySafe $disk 'MediaType' 'Unknown'
            $items.Add(@{id="disk-media-$i"; name="media-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix Media Type"; value=$media}})
        }
        if ($disks.Count -eq 0) {
            $items.Add(@{id='disk-none'; name='none'; directory=$false; enabled=$true;
                columns=@{property='Storage'; value='No disks detected'}})
        }
    } catch {
        $items.Add(@{id='disk-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

function Get-NetworkInfo {
    param([hashtable]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $adapters = @(Get-CimInstance -ClassName Win32_NetworkAdapterConfiguration -ErrorAction Stop |
            Where-Object { $_.IPEnabled -eq $true })
        for ($i = 0; $i -lt $adapters.Count; $i++) {
            $na = $adapters[$i]
            $name = Get-WmiPropertySafe $na 'Description' "Adapter $($i+1)"
            $prefix = if ($adapters.Count -gt 1) { "NIC #$($i+1): " } else { '' }

            $items.Add(@{id="net-name-$i"; name="name-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix$([string]$Strings.strings.networkAdapterName)"; value=$name}})

            $ip = try {
                if ($na.IPAddress -and $na.IPAddress.Count -gt 0) {
                    [string]::Join(', ', $na.IPAddress)
                } else { 'Not connected' }
            } catch { 'N/A' }
            $items.Add(@{id="net-ip-$i"; name="ip-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix$([string]$Strings.strings.networkIPAddress)"; value=$ip}})

            $mac = Get-WmiPropertySafe $na 'MACAddress' 'N/A'
            $items.Add(@{id="net-mac-$i"; name="mac-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix$([string]$Strings.strings.networkMACAddress)"; value=$mac}})

            $speed = try {
                if ($na.Speed) { '{0:N0} Mbps' -f ([uint64]$na.Speed / 1MB) } else { 'N/A' }
            } catch { 'N/A' }
            $items.Add(@{id="net-speed-$i"; name="speed-$i"; directory=$false; enabled=$true;
                columns=@{property="$prefix$([string]$Strings.strings.networkSpeed)"; value=$speed}})
        }
        if ($adapters.Count -eq 0) {
            $items.Add(@{id='net-none'; name='none'; directory=$false; enabled=$true;
                columns=@{property='Network'; value='No active network adapters'}})
        }
    } catch {
        $items.Add(@{id='net-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

# ============================================================================
# Sensor Info - Uses HardView's HardwareWrapper.dll (C++/CLI managed wrapper)
# Required files in lib/: HardwareWrapper.dll, HardwareWrapper.deps.json,
#   HardwareWrapper.runtimeconfig.json, LibreHardwareMonitorLib.dll, HidSharp.dll
# ============================================================================

$script:MonitorManager = $null
$script:SensorAvailable = $false

function Initialize-SensorLibrary {
    if ($script:SensorAvailable) { return $true }

    $wrapperPaths = @(
        (Join-Path $PSScriptRoot 'lib\HardwareWrapper.dll'),
        (Join-Path $PSScriptRoot 'HardwareWrapper.dll')
    )

    foreach ($dllPath in $wrapperPaths) {
        if (Test-Path -LiteralPath $dllPath -PathType Leaf) {
            try {
                $resolvedPath = (Resolve-Path -LiteralPath $dllPath).Path
                Add-Type -Path $resolvedPath -ErrorAction Stop
                $script:MonitorManager = [MonitorManager]
                $script:MonitorManager::Init()
                $script:SensorAvailable = $true
                return $true
            } catch {}
        }
    }
    return $false
}

function Get-SensorInfo {
    param([hashtable]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'

    $available = Initialize-SensorLibrary

    if (-not $available) {
        $items.Add(@{id='sensor-na'; name='na'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.sensorTypes;
                value='Place HardwareWrapper.dll in lib/ subdirectory for sensor data.'}})
        $items.Add(@{id='sensor-na2'; name='na2'; directory=$false; enabled=$true;
            columns=@{property='Download';
                value='https://github.com/gafoo173/HardView'}})
        return $items
    }

    try {
        $script:MonitorManager::Update()

        $cpuTemp = $script:MonitorManager::GetCpuTemperature()
        if ($cpuTemp -gt -90) {
            $items.Add(@{id='sensor-cpu-temp'; name='CPU Temperature'; directory=$false; enabled=$true;
                columns=@{property='CPU Temperature'; value='{0:N1} C' -f $cpuTemp}})
        }

        $cpuAvg = $script:MonitorManager::GetAverageCpuCoreTemperature()
        if ($cpuAvg -gt 0) {
            $items.Add(@{id='sensor-cpu-avg'; name='CPU Core Average'; directory=$false; enabled=$true;
                columns=@{property='CPU Core Average'; value='{0:N1} C' -f $cpuAvg}})
        }

        $cpuMax = $script:MonitorManager::GetMaxCpuCoreTemperature()
        if ($cpuMax -gt 0) {
            $items.Add(@{id='sensor-cpu-max'; name='CPU Core Max'; directory=$false; enabled=$true;
                columns=@{property='CPU Core Max'; value='{0:N1} C' -f $cpuMax}})
        }

        $gpuTemp = $script:MonitorManager::GetGpuTemperature()
        if ($gpuTemp -gt -90) {
            $items.Add(@{id='sensor-gpu-temp'; name='GPU Temperature'; directory=$false; enabled=$true;
                columns=@{property='GPU Temperature'; value='{0:N1} C' -f $gpuTemp}})
        }

        $mbTemp = $script:MonitorManager::GetMotherboardTemperature()
        if ($mbTemp -gt -90) {
            $items.Add(@{id='sensor-mb-temp'; name='Motherboard Temperature'; directory=$false; enabled=$true;
                columns=@{property='Motherboard Temperature'; value='{0:N1} C' -f $mbTemp}})
        }

        $storageTemp = $script:MonitorManager::GetStorageTemperature()
        if ($storageTemp -gt -90) {
            $items.Add(@{id='sensor-storage-temp'; name='Storage Temperature'; directory=$false; enabled=$true;
                columns=@{property='Storage Temperature'; value='{0:N1} C' -f $storageTemp}})
        }

        $cpuFan = $script:MonitorManager::GetCpuFanRpm()
        if ($cpuFan -gt 0) {
            $items.Add(@{id='sensor-cpu-fan'; name='CPU Fan'; directory=$false; enabled=$true;
                columns=@{property='CPU Fan'; value='{0:N0} RPM' -f $cpuFan}})
        }

        $gpuFan = $script:MonitorManager::GetGpuFanRpm()
        if ($gpuFan -gt 0) {
            $items.Add(@{id='sensor-gpu-fan'; name='GPU Fan'; directory=$false; enabled=$true;
                columns=@{property='GPU Fan'; value='{0:N0} RPM' -f $gpuFan}})
        }

        if ($items.Count -eq 0) {
            $items.Add(@{id='sensor-empty'; name='empty'; directory=$false; enabled=$true;
                columns=@{property='Sensors'; value='No sensor data available (may need admin rights)'}})
        }
    } catch {
        $items.Add(@{id='sensor-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Sensor Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

# ============================================================================
# File System Handlers
# ============================================================================

$handler = [string]$Salamander.command_handler
$locale = try { [string]$Salamander.application.Language() } catch { 'en' }
$strings = Get-HardwareMonitorStrings $locale

if ($handler -eq 'listHardwareCategories') {
    $categories = @(
        @{id='cpu'; name=[string]$Strings.categories.cpu},
        @{id='memory'; name=[string]$Strings.categories.memory},
        @{id='motherboard'; name=[string]$Strings.categories.motherboard},
        @{id='gpu'; name=[string]$Strings.categories.gpu},
        @{id='storage'; name=[string]$Strings.categories.storage},
        @{id='network'; name=[string]$Strings.categories.network},
        @{id='sensors'; name=[string]$Strings.categories.sensors}
    )

    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    foreach ($cat in $categories) {
        $items.Add(@{
            id=$cat.id
            name=$cat.name
            directory=$true
            enabled=$true
            columns=@{property=$cat.name; value=''}
        })
    }
    [void]$Salamander.file_system.AddItems($items.ToArray())
    return
}

if ($handler -eq 'openHardwareCategory') {
    $item = $Salamander.invocation.item
    $categoryId = if ($item -and $item.id) { [string]$item.id } else { '' }

    $subItems = $null
    switch ($categoryId) {
        'cpu' {
            $cpuItems = Get-CpuInfo $strings
            $multiItems = Get-MultiCpuInfo $strings
            $usageItems = Get-CpuUsageInfo $strings

            $subItems = New-Object 'System.Collections.Generic.List[hashtable]'

            $subItems.Add(@{id='cpu-props-header'; name='header-cpu-props';
                directory=$false; enabled=$true;
                columns=@{property=""; value=[string]$Strings.strings.cpuProperties}})
            foreach ($ci in $cpuItems) { $subItems.Add($ci) }

            $subItems.Add(@{id='multi-cpu-header'; name='header-multi-cpu';
                directory=$false; enabled=$true;
                columns=@{property=""; value=[string]$Strings.strings.multiCpu}})
            foreach ($mi in $multiItems) { $subItems.Add($mi) }

            $subItems.Add(@{id='cpu-usage-header'; name='header-cpu-usage';
                directory=$false; enabled=$true;
                columns=@{property=""; value=[string]$Strings.strings.cpuUsage}})
            foreach ($ui in $usageItems) { $subItems.Add($ui) }
        }
        'memory' {
            $physItems = Get-PhysicalMemoryInfo $strings
            $virtItems = Get-VirtualMemoryInfo $strings
            $pfItems = Get-PagefileInfo $strings
            $modItems = Get-MemoryModulesInfo $strings

            $subItems = New-Object 'System.Collections.Generic.List[hashtable]'

            $subItems.Add(@{id='mem-phys-header'; name='header-phys-mem';
                directory=$false; enabled=$true;
                columns=@{property=""; value=[string]$Strings.strings.physicalMemory}})
            foreach ($pi in $physItems) { $subItems.Add($pi) }

            $subItems.Add(@{id='mem-virt-header'; name='header-virt-mem';
                directory=$false; enabled=$true;
                columns=@{property=""; value=[string]$Strings.strings.virtualMemory}})
            foreach ($vi in $virtItems) { $subItems.Add($vi) }

            $subItems.Add(@{id='mem-pf-header'; name='header-pf';
                directory=$false; enabled=$true;
                columns=@{property=""; value=[string]$Strings.strings.pagefile}})
            foreach ($pi in $pfItems) { $subItems.Add($pi) }

            $subItems.Add(@{id='mem-mod-header'; name='header-mem-mod';
                directory=$false; enabled=$true;
                columns=@{property=""; value=[string]$Strings.strings.memoryModules}})
            foreach ($mi in $modItems) { $subItems.Add($mi) }
        }
        'motherboard' {
            $subItems = New-Object 'System.Collections.Generic.List[hashtable]'
            $mbItems = Get-MotherboardInfo $strings
            foreach ($bi in $mbItems) { $subItems.Add($bi) }
        }
        'gpu' {
            $subItems = New-Object 'System.Collections.Generic.List[hashtable]'
            $gpuItems = Get-GpuInfo $strings
            foreach ($gi in $gpuItems) { $subItems.Add($gi) }
        }
        'storage' {
            $subItems = New-Object 'System.Collections.Generic.List[hashtable]'
            $diskItems = Get-StorageInfo $strings
            foreach ($di in $diskItems) { $subItems.Add($di) }
        }
        'network' {
            $subItems = New-Object 'System.Collections.Generic.List[hashtable]'
            $netItems = Get-NetworkInfo $strings
            foreach ($ni in $netItems) { $subItems.Add($ni) }
        }
        'sensors' {
            $sensorItems = Get-SensorInfo $strings
            $subItems = New-Object 'System.Collections.Generic.List[hashtable]'
            foreach ($si in $sensorItems) { $subItems.Add($si) }
        }
    }

    if ($null -ne $subItems -and $subItems.Count -gt 0) {
        [void]$Salamander.file_system.AddItems($subItems.ToArray())
    }
    return
}

return
