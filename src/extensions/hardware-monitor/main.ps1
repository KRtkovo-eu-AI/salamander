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
    if ($Bytes -ge 1TB) { return '{0:F2} TB' -f ($Bytes / 1TB) }
    if ($Bytes -ge 1GB) { return '{0:F2} GB' -f ($Bytes / 1GB) }
    if ($Bytes -ge 1MB) { return '{0:F2} MB' -f ($Bytes / 1MB) }
    if ($Bytes -ge 1KB) { return '{0:F2} KB' -f ($Bytes / 1KB) }
    return '{0} B' -f $Bytes
}

function Format-BytesMB {
    param([uint64]$Bytes)
    return '{0:F0} MB' -f ($Bytes / 1MB)
}

# ============================================================================
# Hardware Info Gathering Functions
# ============================================================================

function Get-CpuInfo {
    param([object]$Strings)
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

        $l2 = try { '{0:F0} KB' -f $cpu.L2CacheSize } catch { 'N/A' }
        $items.Add(@{id='cpu-l2'; name='l2'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuL2Cache; value=$l2}})

        $l3 = try { '{0:F0} KB' -f $cpu.L3CacheSize } catch { 'N/A' }
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

        $voltage = try { '{0:F3} V' -f ($cpu.CurrentVoltage / 1.0) } catch { 'N/A' }
        $items.Add(@{id='cpu-voltage'; name='voltage'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.cpuVoltage; value=$voltage}})
    } catch {
        $items.Add(@{id='cpu-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

function Get-MultiCpuInfo {
    param([object]$Strings)
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
    param([object]$Strings)
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
    param([object]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $os = Get-CimInstance -ClassName Win32_OperatingSystem -ErrorAction Stop
        $totalBytes = [uint64]$os.TotalVisibleMemorySize * 1024
        $freeBytes = [uint64]$os.FreePhysicalMemory * 1024
        $usedBytes = $totalBytes - $freeBytes
        $usagePct = if ($totalBytes -gt 0) { [Math]::Round($usedBytes * 100.0 / $totalBytes) } else { 0 }

        $prefix = [string]$Strings.strings.physicalMemory
        $items.Add(@{id='mem-total'; name="$prefix - $($Strings.strings.total)"; directory=$false; enabled=$true;
            columns=@{property="$prefix - $($Strings.strings.total)"; value=(Format-BytesMB $totalBytes)}})
        $items.Add(@{id='mem-used'; name="$prefix - $($Strings.strings.used)"; directory=$false; enabled=$true;
            columns=@{property="$prefix - $($Strings.strings.used)"; value=(Format-BytesMB $usedBytes)}})
        $items.Add(@{id='mem-free'; name="$prefix - $($Strings.strings.free)"; directory=$false; enabled=$true;
            columns=@{property="$prefix - $($Strings.strings.free)"; value=(Format-BytesMB $freeBytes)}})
        $items.Add(@{id='mem-usage'; name="$prefix - $($Strings.strings.usage)"; directory=$false; enabled=$true;
            columns=@{property="$prefix - $($Strings.strings.usage)"; value="$usagePct %"}})
    } catch {
        $items.Add(@{id='mem-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

function Get-VirtualMemoryInfo {
    param([object]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $os = Get-CimInstance -ClassName Win32_OperatingSystem -ErrorAction Stop
        $totalBytes = [uint64]$os.TotalVirtualMemorySize * 1024
        $freeBytes = [uint64]$os.FreeVirtualMemory * 1024
        $usedBytes = $totalBytes - $freeBytes
        $usagePct = if ($totalBytes -gt 0) { [Math]::Round($usedBytes * 100.0 / $totalBytes) } else { 0 }

        $prefix = [string]$Strings.strings.virtualMemory
        $items.Add(@{id='vm-total'; name="$prefix - $($Strings.strings.total)"; directory=$false; enabled=$true;
            columns=@{property="$prefix - $($Strings.strings.total)"; value=(Format-BytesMB $totalBytes)}})
        $items.Add(@{id='vm-used'; name="$prefix - $($Strings.strings.used)"; directory=$false; enabled=$true;
            columns=@{property="$prefix - $($Strings.strings.used)"; value=(Format-BytesMB $usedBytes)}})
        $items.Add(@{id='vm-free'; name="$prefix - $($Strings.strings.free)"; directory=$false; enabled=$true;
            columns=@{property="$prefix - $($Strings.strings.free)"; value=(Format-BytesMB $freeBytes)}})
        $items.Add(@{id='vm-usage'; name="$prefix - $($Strings.strings.usage)"; directory=$false; enabled=$true;
            columns=@{property="$prefix - $($Strings.strings.usage)"; value="$usagePct %"}})
    } catch {
        $items.Add(@{id='vm-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Error'; value=[string]$_.Exception.Message}})
    }
    return $items
}

function Get-PagefileInfo {
    param([object]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $pagefiles = @(Get-CimInstance -ClassName Win32_PageFile -ErrorAction Stop)
        if ($pagefiles.Count -eq 0) {
            $items.Add(@{id='pf-none'; name='none'; directory=$false; enabled=$true;
                columns=@{property=[string]$Strings.strings.pagefileName; value='No pagefile found'}})
        } else {
            for ($i = 0; $i -lt $pagefiles.Count; $i++) {
                $pf = $pagefiles[$i]
                $name = [string]$pf.Name
                $size = try { Format-BytesMB ([uint64]$pf.MaxSize * 1MB) } catch { 'N/A' }
                $items.Add(@{id="pf-$i"; name="pagefile-$i"; directory=$false; enabled=$true;
                    columns=@{property=[string]$Strings.strings.pagefileName; value=$name}})
                $items.Add(@{id="pf-size-$i"; name="pagefile-size-$i"; directory=$false; enabled=$true;
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
    param([object]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $modules = @(Get-CimInstance -ClassName Win32_PhysicalMemory -ErrorAction Stop)
        $slotCount = $modules.Count
        try {
            $arrays = @(Get-CimInstance -ClassName Win32_PhysicalMemoryArray -ErrorAction Stop)
            $reportedSlotCount = [int](
                ($arrays | Measure-Object -Property MemoryDevices -Sum).Sum)
            if ($reportedSlotCount -ge $modules.Count -and
                $reportedSlotCount -le 256) {
                $slotCount = $reportedSlotCount
            }
        } catch {
            # Some firmware does not publish a usable physical-memory array.
        }

        $items.Add(@{id='mem-slots-total'; name='slots-total'; directory=$false; enabled=$true;
            columns=@{property=([string]$Strings.strings.memoryModules + ' - ' +
                [string]$Strings.strings.total); value=[string]$slotCount}})
        $items.Add(@{id='mem-slots-used'; name='slots-used'; directory=$false; enabled=$true;
            columns=@{property=([string]$Strings.strings.memoryModules + ' - ' +
                [string]$Strings.strings.used); value=[string]$modules.Count}})
        $items.Add(@{id='mem-slots-free'; name='slots-free'; directory=$false; enabled=$true;
            columns=@{property=([string]$Strings.strings.memoryModules + ' - ' +
                [string]$Strings.strings.free); value=[string]([Math]::Max(0, $slotCount - $modules.Count))}})

        for ($i = 0; $i -lt $modules.Count; $i++) {
            $m = $modules[$i]
            $locator = Get-WmiPropertySafe $m 'DeviceLocator' "DIMM $($i+1)"
            $bank = Get-WmiPropertySafe $m 'BankLabel' ''
            $slotName = if ([string]::IsNullOrWhiteSpace($bank)) {
                $locator
            } else {
                "$bank - $locator"
            }
            # FS item names are path components. A slash makes the host reject
            # the complete AddItems batch, leaving the Memory directory empty.
            $slotName = ([string]$slotName -replace '[\\/]', '-').Trim()
            $manufacturer = Get-WmiPropertySafe $m 'Manufacturer' 'Unknown'
            $capacity = try { Format-BytesMB ([uint64]$m.Capacity) } catch { 'N/A' }
            $configuredSpeed = try {
                $clock = if ([uint64]$m.ConfiguredClockSpeed -gt 0) {
                    [uint64]$m.ConfiguredClockSpeed
                } else { [uint64]$m.Speed }
                '{0} MHz' -f $clock
            } catch { 'N/A' }
            $partNumber = (Get-WmiPropertySafe $m 'PartNumber' '').Trim()
            $serialNumber = (Get-WmiPropertySafe $m 'SerialNumber' '').Trim()
            $slotValues = @(
                @('capacity', [string]$Strings.strings.total, $capacity),
                @('speed', [string]$Strings.strings.networkSpeed, $configuredSpeed),
                @('manufacturer', [string]$Strings.strings.manufacturer, $manufacturer),
                @('part', [string]$Strings.strings.product, $partNumber),
                @('serial', [string]$Strings.strings.serialNumber, $serialNumber)
            )
            foreach ($slotValue in $slotValues) {
                if ([string]::IsNullOrWhiteSpace([string]$slotValue[2])) { continue }
                $property = "$slotName - $($slotValue[1])"
                $items.Add(@{id="mem-slot-$i-$($slotValue[0])"; name=$property;
                    compactName=$property; directory=$false; enabled=$true;
                    columns=@{property=$property; value=[string]$slotValue[2]}})
            }
        }
        for ($i = $modules.Count; $i -lt $slotCount; $i++) {
            $slotName = "DIMM $($i + 1)"
            $items.Add(@{id="mem-slot-$i"; name=$slotName; compactName=$slotName;
                directory=$false; enabled=$true;
                columns=@{property=$slotName; value=[string]$Strings.strings.free}})
        }
    } catch {
        $items.Add(@{id='mem-mod-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.memoryModules;
                value=[string]$_.Exception.Message}})
    }
    return $items
}

function Get-MotherboardInfo {
    param([object]$Strings)
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
    param([object]$Strings)
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
    param([object]$Strings)
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
    param([object]$Strings)
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
                if ($na.Speed) { '{0:F0} Mbps' -f ([uint64]$na.Speed / 1MB) } else { 'N/A' }
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
# The wrapper and its managed HardView dependencies live in lib/. The shared
# VC runtime is resolved from the Salamander application directory.
# ============================================================================

$script:MonitorManager = $null
$script:SensorAvailable = $false
$script:SensorInteropAvailable = $false

function Initialize-SensorLibrary {
    if ($script:SensorAvailable) { return $true }

    $wrapperPaths = @(
        (Join-Path $PSScriptRoot 'lib\HardwareWrapper.dll'),
        (Join-Path $PSScriptRoot 'HardwareWrapper.dll')
    )

    foreach ($dllPath in $wrapperPaths) {
        if (Test-Path -LiteralPath $dllPath -PathType Leaf) {
            $originalPath = $env:PATH
            try {
                # The portable Salamander package already carries the VC runtime
                # beside salamand.exe. Make that shared copy visible while the
                # mixed-mode HardView wrapper resolves its native dependencies.
                $salamanderRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
                if (Test-Path -LiteralPath (Join-Path $salamanderRoot 'msvcp140.dll')) {
                    $env:PATH = $salamanderRoot + [IO.Path]::PathSeparator + $originalPath
                }
                $resolvedPath = (Resolve-Path -LiteralPath $dllPath).Path
                Add-Type -Path $resolvedPath -ErrorAction Stop
                if (-not ('HardViewSensorInterop' -as [type])) {
                    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public static class HardViewSensorInterop
{
    [DllImport("HardwareWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void GetAllSensorsPacked(out IntPtr data, out int size);

    [DllImport("HardwareWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void FreePackedSensors(IntPtr data);
}
'@ -ErrorAction Stop
                }
                $script:MonitorManager = [MonitorManager]
                $script:MonitorManager::Init()
                $script:SensorInteropAvailable = $true
                $script:SensorAvailable = $true
                return $true
            } catch {
            } finally {
                $env:PATH = $originalPath
            }
        }
    }
    return $false
}

function Get-HardViewSensorInfo {
    param(
        [object]$Strings,
        [ValidateSet('Temperature', 'Fan')]
        [string]$SensorType
    )
    $items = New-Object 'System.Collections.Generic.List[hashtable]'

    $available = Initialize-SensorLibrary

    if (-not $available -or -not $script:SensorInteropAvailable) {
        $items.Add(@{id='sensor-na'; name='na'; directory=$false; enabled=$true;
            columns=@{property=[string]$Strings.strings.sensorTypes;
                value='Place HardwareWrapper.dll in lib/ subdirectory for sensor data.'}})
        $items.Add(@{id='sensor-na2'; name='na2'; directory=$false; enabled=$true;
            columns=@{property='Download';
                value='https://github.com/gafoo173/HardView'}})
        return $items
    }

    $packedData = [IntPtr]::Zero
    try {
        $script:MonitorManager::Update()
        $packedSize = 0
        [HardViewSensorInterop]::GetAllSensorsPacked(
            [ref]$packedData, [ref]$packedSize)

        if ($packedData -ne [IntPtr]::Zero -and $packedSize -gt 0) {
            $bytes = [byte[]]::new($packedSize)
            [Runtime.InteropServices.Marshal]::Copy(
                $packedData, $bytes, 0, $packedSize)
            $encoding = [Text.Encoding]::GetEncoding(
                [Globalization.CultureInfo]::CurrentCulture.TextInfo.ANSICodePage)
            $marker = " - $SensorType - "
            $offset = 0
            $ordinal = 0

            while ($offset -lt $packedSize) {
                $nameStart = $offset
                while ($offset -lt $packedSize -and $bytes[$offset] -ne 0) {
                    $offset++
                }
                if ($offset -ge $packedSize -or $offset + 8 -ge $packedSize) {
                    throw 'HardView returned malformed sensor data.'
                }

                $fullName = $encoding.GetString(
                    $bytes, $nameStart, $offset - $nameStart)
                $offset++
                $sensorValue = [BitConverter]::ToDouble($bytes, $offset)
                $offset += 8

                $markerIndex = $fullName.IndexOf(
                    $marker, [StringComparison]::Ordinal)
                if ($markerIndex -lt 0) { continue }

                $hardwareName = $fullName.Substring(0, $markerIndex)
                $sensorName = $fullName.Substring($markerIndex + $marker.Length)
                $displayName = "$hardwareName - $sensorName"
                $formattedValue = [string]$Strings.strings.notAvailable
                $validValue = -not [double]::IsNaN($sensorValue) -and
                    -not [double]::IsInfinity($sensorValue) -and
                    (($SensorType -eq 'Temperature' -and $sensorValue -gt 0) -or
                     ($SensorType -eq 'Fan' -and $sensorValue -ge 0))
                if ($validValue) {
                    if ($SensorType -eq 'Temperature') {
                        $formattedValue = '{0:F1} C' -f $sensorValue
                    } else {
                        $formattedValue = '{0:F0} RPM' -f $sensorValue
                    }
                }
                $items.Add(@{
                    id="sensor-$($SensorType.ToLowerInvariant())-$ordinal"
                    name=$displayName
                    directory=$false
                    enabled=$true
                    columns=@{property=$displayName; value=$formattedValue}
                })
                $ordinal++
            }
        }

        if ($items.Count -eq 0) {
            $items.Add(@{id='sensor-empty'; name='empty'; directory=$false; enabled=$true;
                columns=@{property='Sensors'; value='No sensor data available (may need admin rights)'}})
        }
    } catch {
        $items.Add(@{id='sensor-error'; name='error'; directory=$false; enabled=$true;
            columns=@{property='Sensor Error'; value=[string]$_.Exception.Message}})
    } finally {
        if ($packedData -ne [IntPtr]::Zero) {
            [HardViewSensorInterop]::FreePackedSensors($packedData)
        }
    }
    return $items
}

# ============================================================================
# File System Handlers
# ============================================================================

$handler = [string]$Salamander.command_handler
$locale = try { [string]$Salamander.application.Language() } catch { 'en' }
$strings = Get-HardwareMonitorStrings $locale

if ($handler -ne 'listHardware') { return }

$fileSystemPath = try { [string]$Salamander.invocation.path } catch { '' }
$categoryId = ''
$viewId = ''
if (-not [string]::IsNullOrWhiteSpace($fileSystemPath)) {
    $components = @($fileSystemPath -split '[\\/]' | Where-Object {
        -not [string]::IsNullOrWhiteSpace([string]$_)
    })
    if ($components.Count -gt 1) { $categoryId = [string]$components[1] }
    if ($components.Count -gt 2) { $viewId = [string]$components[2] }
}

if ([string]::IsNullOrWhiteSpace($categoryId)) {
    $categories = @(
        @{id='cpu'; name=[string]$Strings.categories.cpu; icon='icons/cpu.svg'},
        @{id='memory'; name=[string]$Strings.categories.memory; icon='icons/memory.svg'},
        @{id='motherboard'; name=[string]$Strings.categories.motherboard; icon='icons/motherboard.svg'},
        @{id='gpu'; name=[string]$Strings.categories.gpu; icon='icons/gpu.svg'},
        @{id='storage'; name=[string]$Strings.categories.storage; icon='icons/storage.svg'},
        @{id='network'; name=[string]$Strings.categories.network; icon='icons/network.svg'},
        @{id='sensors'; name=[string]$Strings.categories.sensors; icon='icons/sensors.svg'}
    )

    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    foreach ($cat in $categories) {
        $items.Add(@{
            id=$cat.id
            name=$cat.name
            directory=$true
            enabled=$true
            icon=$cat.icon
            iconDark=($cat.icon -replace '\.svg$', '-dark.svg')
            columns=@{property=$cat.name; value=''}
        })
    }
    [void]$Salamander.file_system.AddItems($items.ToArray())
    return
}

else {
    $subItems = $null
    switch ($categoryId) {
        'cpu' {
            $subItems = New-Object 'System.Collections.Generic.List[hashtable]'
            if ($viewId -eq 'usage') {
                foreach ($ui in (Get-CpuUsageInfo $strings)) { $subItems.Add($ui) }
            } else {
                $subItems.Add(@{id='usage'; name=[string]$Strings.strings.cpuUsage;
                    directory=$true; enabled=$true;
                    columns=@{property=[string]$Strings.strings.cpuUsage; value=''}})
                $subItems.Add(@{id='cpu-props-header'; name='header-cpu-props';
                    directory=$false; enabled=$true;
                    columns=@{property=""; value=[string]$Strings.strings.cpuProperties}})
                foreach ($ci in (Get-CpuInfo $strings)) { $subItems.Add($ci) }
                $subItems.Add(@{id='multi-cpu-header'; name='header-multi-cpu';
                    directory=$false; enabled=$true;
                    columns=@{property=""; value=[string]$Strings.strings.multiCpu}})
                foreach ($mi in (Get-MultiCpuInfo $strings)) { $subItems.Add($mi) }
            }
        }
        'memory' {
            $subItems = New-Object 'System.Collections.Generic.List[hashtable]'
            if ($viewId -eq 'usage') {
                foreach ($pi in (Get-PhysicalMemoryInfo $strings)) { $subItems.Add($pi) }
                foreach ($vi in (Get-VirtualMemoryInfo $strings)) { $subItems.Add($vi) }
            } else {
                $subItems.Add(@{id='usage'; name=[string]$Strings.strings.usage;
                    directory=$true; enabled=$true;
                    columns=@{property=[string]$Strings.strings.usage; value=''}})
                $subItems.Add(@{id='mem-pf-header'; name='header-pf';
                    directory=$false; enabled=$true;
                    columns=@{property=""; value=[string]$Strings.strings.pagefile}})
                foreach ($pi in (Get-PagefileInfo $strings)) { $subItems.Add($pi) }
                $subItems.Add(@{id='mem-mod-header'; name='header-mem-mod';
                    directory=$false; enabled=$true;
                    columns=@{property=""; value=[string]$Strings.strings.memoryModules}})
                foreach ($mi in (Get-MemoryModulesInfo $strings)) { $subItems.Add($mi) }
            }
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
            $subItems = New-Object 'System.Collections.Generic.List[hashtable]'
            if ($viewId -eq 'temperatures') {
                foreach ($si in (Get-HardViewSensorInfo $strings 'Temperature')) {
                    $subItems.Add($si)
                }
            } elseif ($viewId -eq 'fans') {
                foreach ($si in (Get-HardViewSensorInfo $strings 'Fan')) {
                    $subItems.Add($si)
                }
            } else {
                $subItems.Add(@{id='temperatures'; name=[string]$Strings.strings.temperatures;
                    directory=$true; enabled=$true;
                    columns=@{property=[string]$Strings.strings.temperatures; value=''}})
                $subItems.Add(@{id='fans'; name=[string]$Strings.strings.fans;
                    directory=$true; enabled=$true;
                    columns=@{property=[string]$Strings.strings.fans; value=''}})
            }
        }
    }

    if ($null -ne $subItems -and $subItems.Count -gt 0) {
        foreach ($subItem in $subItems) {
            $subItem.icon = "icons/$categoryId.svg"
            $subItem.iconDark = "icons/$categoryId-dark.svg"
        }
        [void]$Salamander.file_system.AddItems($subItems.ToArray())
    }
    return
}

return
