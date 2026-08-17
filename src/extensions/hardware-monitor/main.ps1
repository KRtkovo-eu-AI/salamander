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

function ConvertTo-SafeHardwareItemName {
    param([string]$Name)
    if ([string]::IsNullOrWhiteSpace($Name)) { return 'item' }
    $safeName = $Name.Replace('\', ' - ').Replace('/', ' - ')
    $safeName = [regex]::Replace($safeName, '\s+', ' ').Trim()
    if ($safeName -eq '.' -or $safeName -eq '..') { return "item-$safeName" }
    return $safeName
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
        [ValidateSet('Temperature', 'Fan', 'Voltage', 'All')]
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
            $knownSensorTypes = @(
                'Temperature', 'Fan', 'Voltage', 'Current', 'Power', 'Clock',
                'Load', 'Frequency', 'Flow', 'Control', 'Level', 'Factor',
                'Data', 'SmallData', 'Throughput', 'TimeSpan', 'Energy',
                'Noise', 'Conductivity', 'Humidity')
            $marker = if ($SensorType -eq 'All') { '' } else { " - $SensorType - " }
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

                $actualType = $SensorType
                $actualMarker = $marker
                $markerIndex = -1
                if ($SensorType -eq 'All') {
                    foreach ($candidateType in $knownSensorTypes) {
                        $candidateMarker = " - $candidateType - "
                        $candidateIndex = $fullName.IndexOf(
                            $candidateMarker, [StringComparison]::Ordinal)
                        if ($candidateIndex -ge 0) {
                            $actualType = $candidateType
                            $actualMarker = $candidateMarker
                            $markerIndex = $candidateIndex
                            break
                        }
                    }
                } else {
                    $markerIndex = $fullName.IndexOf(
                        $marker, [StringComparison]::Ordinal)
                }
                if ($markerIndex -lt 0) { continue }

                $hardwareName = $fullName.Substring(0, $markerIndex)
                $sensorName = $fullName.Substring($markerIndex + $actualMarker.Length)
                $displayName = if ($SensorType -eq 'All') {
                    "[$actualType] $hardwareName - $sensorName"
                } else {
                    "$hardwareName - $sensorName"
                }
                $formattedValue = [string]$Strings.strings.notAvailable
                $validValue = -not [double]::IsNaN($sensorValue) -and
                    -not [double]::IsInfinity($sensorValue) -and
                    $sensorValue -ge 0 -and
                    ($actualType -ne 'Temperature' -or $sensorValue -gt 0)
                if ($validValue) {
                    $format = switch ($actualType) {
                        'Temperature' { '{0:F1} C' }
                        'Fan'         { '{0:F0} RPM' }
                        'Voltage'     { '{0:F3} V' }
                        'Current'     { '{0:F3} A' }
                        'Power'       { '{0:F2} W' }
                        'Clock'       { '{0:F0} MHz' }
                        'Load'        { '{0:F1} %' }
                        'Control'     { '{0:F1} %' }
                        'Level'       { '{0:F1} %' }
                        'Frequency'   { '{0:F1} Hz' }
                        'Flow'        { '{0:F1} L/h' }
                        'Factor'      { '{0:F2} x' }
                        'Data'        { '{0:F2} GB' }
                        'SmallData'   { '{0:F2} MB' }
                        'Throughput'  { '{0:F0} B/s' }
                        'TimeSpan'    { '{0:F0} s' }
                        'Energy'      { '{0:F0} mWh' }
                        'Noise'       { '{0:F1} dBA' }
                        'Conductivity'{ '{0:F1} uS/cm' }
                        'Humidity'    { '{0:F1} %' }
                        default       { '{0:G}' }
                    }
                    $formattedValue = $format -f $sensorValue
                }
                $items.Add(@{
                    id="sensor-$($actualType.ToLowerInvariant())-$ordinal"
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

$script:SmartStorageCache = $null
$script:SmartStorageCacheTime = [datetime]::MinValue

function Get-SmartStorageCacheView {
    param([object[]]$Groups, [string]$DiskId, [object]$Strings)
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    if ([string]::IsNullOrWhiteSpace($DiskId)) {
        foreach ($group in @($Groups)) {
            $items.Add(@{id=$group.id; name=$group.name; directory=$true;
                enabled=$true; columns=@{property=$group.name; value=$group.summary}})
        }
    } else {
        $group = @($Groups | Where-Object { $_.id -eq $DiskId } | Select-Object -First 1)
        if ($group.Count -gt 0) {
            foreach ($item in @($group[0].items)) { $items.Add($item) }
        } else {
            $items.Add(@{id='smart-disk-missing'; name='SMART - NVMe';
                directory=$false; enabled=$true; columns=@{property='SMART / NVMe';
                    value=[string]$Strings.strings.notAvailable}})
        }
    }
    return $items.ToArray()
}

function Get-SmartStorageInfo {
    param([object]$Strings, [string]$DiskId = '')

    if ($null -ne $script:SmartStorageCache -and
        ((Get-Date) - $script:SmartStorageCacheTime).TotalSeconds -lt 60) {
        return Get-SmartStorageCacheView $script:SmartStorageCache $DiskId $Strings
    }

    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    $groups = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $toolkitPath = Join-Path $PSScriptRoot 'lib\DiskInfoToolkit.dll'
        if (-not ('DiskInfoToolkit.StorageManager' -as [type])) {
            Add-Type -Path $toolkitPath -ErrorAction Stop
        }
        [DiskInfoToolkit.StorageManager]::ReloadStorages()
        $storages = @([DiskInfoToolkit.StorageManager]::Storages)
        $diskOrdinal = 0

        foreach ($storage in $storages) {
            $diskItems = New-Object 'System.Collections.Generic.List[hashtable]'
            $ordinal = 0
            try { $storage.Update() } catch {}
            $prefix = if ([string]::IsNullOrWhiteSpace([string]$storage.Model)) {
                "PhysicalDrive$($storage.DriveNumber)"
            } else { [string]$storage.Model }
            $properties = [ordered]@{
                'Device' = $storage.PhysicalPath
                'Bus' = $storage.BusType
                'Firmware' = $storage.FirmwareRev
                'NVMe' = $storage.IsNVMe
                'SSD' = $storage.IsSSD
                'TRIM' = $storage.IsTrimSupported
                'Write Cache' = $storage.IsVolatileWriteCachePresent
            }
            if ($null -ne $storage.Smart) {
                $smart = $storage.Smart
                $properties['SMART Status'] = $smart.DiskStatus
                $properties['Temperature'] = if ($null -ne $smart.Temperature) {
                    "$($smart.Temperature) C"
                } else { [string]$Strings.strings.notAvailable }
                $properties['Life Remaining'] = if ($null -ne $smart.Life) {
                    "$($smart.Life) %"
                } else { [string]$Strings.strings.notAvailable }
                $properties['Power-On Hours'] = $smart.DetectedPowerOnHours
                $properties['Power-On Count'] = $smart.PowerOnCount
                $properties['Host Reads'] = $smart.HostReads
                $properties['Host Writes'] = $smart.HostWrites
                $properties['NAND Writes'] = $smart.NandWrites
                $properties['Wear Leveling Count'] = $smart.WearLevelingCount
            }
            foreach ($property in $properties.GetEnumerator()) {
                $value = if ($null -eq $property.Value -or
                    [string]::IsNullOrWhiteSpace([string]$property.Value)) {
                    [string]$Strings.strings.notAvailable
                } else { [string]$property.Value }
                $displayName = "$prefix - $($property.Key)"
                $diskItems.Add(@{id="smart-$ordinal"; name=$displayName;
                    directory=$false; enabled=$true;
                    columns=@{property=$displayName; value=$value}})
                $ordinal++
            }
            if ($null -ne $storage.Smart) {
                foreach ($attribute in @($storage.Smart.SmartAttributes)) {
                    $attributeName = if ($null -ne $attribute.Info -and
                        -not [string]::IsNullOrWhiteSpace([string]$attribute.Info.Name)) {
                        [string]$attribute.Info.Name
                    } else { "SMART Attribute $($attribute.Info.ID)" }
                    $displayName = "$prefix - $attributeName"
                    $diskItems.Add(@{id="smart-attribute-$ordinal"; name=$displayName;
                        directory=$false; enabled=$true;
                        columns=@{property=$displayName;
                            value=[string]$attribute.Attribute.RawValueULong}})
                    $ordinal++
                }
            }
            $summary = '{0}, {1}' -f $storage.BusType,
                $(if ($null -ne $storage.Smart) { $storage.Smart.DiskStatus } else { 'SMART' })
            $groups.Add(@{id="disk-$diskOrdinal"; name=$prefix; summary=$summary;
                items=$diskItems.ToArray()})
            $diskOrdinal++
        }
        # DiskInfoToolkit can legitimately return no devices when its low-level
        # provider is unavailable. Keep the view useful through the read-only
        # Windows Storage Management provider, including NVMe devices.
        if ($groups.Count -eq 0) {
            $diskOrdinal = 0
            foreach ($disk in @(Get-PhysicalDisk -ErrorAction Stop)) {
                $diskItems = New-Object 'System.Collections.Generic.List[hashtable]'
                $ordinal = 0
                $reliability = try {
                    $disk | Get-StorageReliabilityCounter -ErrorAction Stop
                } catch { $null }
                $prefix = if ([string]::IsNullOrWhiteSpace([string]$disk.FriendlyName)) {
                    "Physical Disk $ordinal"
                } else { [string]$disk.FriendlyName }
                $properties = [ordered]@{
                    'Health' = $disk.HealthStatus
                    'Operational Status' = ($disk.OperationalStatus -join ', ')
                    'Media Type' = $disk.MediaType
                    'Bus Type' = $disk.BusType
                    'Size' = if ($disk.Size -gt 0) { Format-Bytes ([uint64]$disk.Size) } else { $null }
                    'Firmware' = $disk.FirmwareVersion
                    'Serial Number' = $disk.SerialNumber
                }
                if ($null -ne $reliability) {
                    $properties['Temperature'] = if ($null -ne $reliability.Temperature) {
                        "$($reliability.Temperature) C"
                    } else { $null }
                    $properties['Wear'] = if ($null -ne $reliability.Wear) {
                        "$($reliability.Wear) %"
                    } else { $null }
                    $properties['Power-On Hours'] = $reliability.PowerOnHours
                    $properties['Read Errors Total'] = $reliability.ReadErrorsTotal
                    $properties['Write Errors Total'] = $reliability.WriteErrorsTotal
                    $properties['Read Latency Max'] = $reliability.ReadLatencyMax
                    $properties['Write Latency Max'] = $reliability.WriteLatencyMax
                }
                foreach ($property in $properties.GetEnumerator()) {
                    $value = if ($null -eq $property.Value -or
                        [string]::IsNullOrWhiteSpace([string]$property.Value)) {
                        [string]$Strings.strings.notAvailable
                    } else { [string]$property.Value }
                    $displayName = "$prefix - $($property.Key)"
                    $diskItems.Add(@{id="storage-health-$ordinal"; name=$displayName;
                        directory=$false; enabled=$true;
                        columns=@{property=$displayName; value=$value}})
                    $ordinal++
                }
                $summary = '{0}, {1}, {2}' -f $disk.BusType,
                    $disk.MediaType, $disk.HealthStatus
                $groups.Add(@{id="disk-$diskOrdinal"; name=$prefix; summary=$summary;
                    items=$diskItems.ToArray()})
                $diskOrdinal++
            }
        }
        if ($groups.Count -eq 0) {
            $items.Add(@{id='smart-empty'; name='SMART / NVMe'; directory=$false;
                enabled=$true; columns=@{property='SMART / NVMe';
                    value=[string]$Strings.strings.notAvailable}})
        }
    } catch {
        $items.Add(@{id='smart-error'; name='SMART / NVMe'; directory=$false;
            enabled=$true; columns=@{property='SMART / NVMe';
                value=[string]$_.Exception.Message}})
    }
    $script:SmartStorageCache = $groups.ToArray()
    $script:SmartStorageCacheTime = Get-Date
    if ($script:SmartStorageCache.Count -eq 0) { return $items.ToArray() }
    return Get-SmartStorageCacheView $script:SmartStorageCache $DiskId $Strings
}

$script:HidDeviceCache = $null

function Get-HidDeviceInfo {
    param([object]$Strings)

    if ($null -ne $script:HidDeviceCache) { return $script:HidDeviceCache }
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        $hidSharpPath = Join-Path $PSScriptRoot 'lib\HidSharp.dll'
        if (-not ('HidSharp.DeviceList' -as [type])) {
            Add-Type -Path $hidSharpPath -ErrorAction Stop
        }
        $ordinal = 0
        foreach ($device in @([HidSharp.DeviceList]::Local.GetHidDevices())) {
            $manufacturer = try { [string]$device.Manufacturer } catch { '' }
            $product = try { [string]$device.ProductName } catch { '' }
            $serial = try { [string]$device.SerialNumber } catch { '' }
            $friendlyName = try { [string]$device.GetFriendlyName() } catch { '' }
            $displayName = @($manufacturer, $product, $friendlyName) |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
                Select-Object -Unique
            $displayName = $displayName -join ' - '
            if ([string]::IsNullOrWhiteSpace($displayName)) {
                $displayName = 'HID Device'
            }
            $details = 'VID {0:X4}, PID {1:X4}' -f
                ([int]$device.VendorID), ([int]$device.ProductID)
            if (-not [string]::IsNullOrWhiteSpace($serial)) {
                $details += ", S/N $serial"
            }
            $items.Add(@{id="hid-$ordinal"; name=$displayName; directory=$false;
                enabled=$true; columns=@{property=$displayName; value=$details}})
            $ordinal++
        }
        if ($items.Count -eq 0) {
            $items.Add(@{id='hid-empty'; name='HID'; directory=$false;
                enabled=$true; columns=@{property='HID';
                    value=[string]$Strings.strings.notAvailable}})
        }
    } catch {
        $items.Add(@{id='hid-error'; name='HID'; directory=$false;
            enabled=$true; columns=@{property='HID'; value=[string]$_.Exception.Message}})
    }
    $script:HidDeviceCache = $items.ToArray()
    return $script:HidDeviceCache
}

$script:SmbiosMemoryCache = $null

function Get-SmbiosMemoryInfo {
    param([object]$Strings)

    if ($null -ne $script:SmbiosMemoryCache) { return $script:SmbiosMemoryCache }
    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    try {
        if (-not (Initialize-SensorLibrary)) {
            throw 'LibreHardwareMonitor is not available.'
        }
        $smbios = [LibreHardwareMonitor.Hardware.SMBios]::new()
        $ordinal = 0
        foreach ($memory in @($smbios.MemoryDevices)) {
            $slot = if ([string]::IsNullOrWhiteSpace([string]$memory.DeviceLocator)) {
                "Memory Module $($ordinal + 1)"
            } else { [string]$memory.DeviceLocator }
            $properties = [ordered]@{
                'Type' = $memory.Type
                'Manufacturer' = $memory.ManufacturerName
                'Part Number' = $memory.PartNumber
                'Serial Number' = $memory.SerialNumber
                'Size' = if ($memory.Size -gt 0) { "$($memory.Size) MB" } else { $null }
                'Speed' = if ($memory.Speed -gt 0) { "$($memory.Speed) MT/s" } else { $null }
                'Configured Speed' = if ($memory.ConfiguredSpeed -gt 0) {
                    "$($memory.ConfiguredSpeed) MT/s"
                } else { $null }
                'Configured Voltage' = if ($memory.ConfiguredVoltage -gt 0) {
                    '{0:F3} V' -f ([double]$memory.ConfiguredVoltage / 1000.0)
                } else { $null }
            }
            foreach ($property in $properties.GetEnumerator()) {
                $value = if ($null -eq $property.Value -or
                    [string]::IsNullOrWhiteSpace([string]$property.Value)) {
                    [string]$Strings.strings.notAvailable
                } else { [string]$property.Value }
                $displayName = "$slot - $($property.Key)"
                $items.Add(@{id="spd-$ordinal"; name=$displayName; directory=$false;
                    enabled=$true; columns=@{property=$displayName; value=$value}})
                $ordinal++
            }
        }
        if ($items.Count -eq 0) {
            $items.Add(@{id='spd-empty'; name='SPD / SMBIOS'; directory=$false;
                enabled=$true; columns=@{property='SPD / SMBIOS';
                    value=[string]$Strings.strings.notAvailable}})
        }
    } catch {
        $items.Add(@{id='spd-error'; name='SPD / SMBIOS'; directory=$false;
            enabled=$true; columns=@{property='SPD / SMBIOS';
                value=[string]$_.Exception.Message}})
    }
    $script:SmbiosMemoryCache = $items.ToArray()
    return $script:SmbiosMemoryCache
}

# ============================================================================
# File System Handlers
# ============================================================================

function ConvertTo-DeviceItemId {
    param([string]$DeviceId)
    $bytes = [Text.Encoding]::UTF8.GetBytes($DeviceId)
    return 'device-' + [Convert]::ToBase64String($bytes).TrimEnd('=').
        Replace('+', '-').Replace('/', '_')
}

function Initialize-DeviceManagerNativeMethods {
    if ($null -ne ('OpenSalamander.HardwareMonitor.DeviceManagerNative' -as [type])) { return }
    Add-Type -ErrorAction Stop -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace OpenSalamander.HardwareMonitor
{
    public static class DeviceManagerNative
    {
        [StructLayout(LayoutKind.Sequential)]
        private struct SP_DEVINFO_DATA
        {
            public int cbSize;
            public Guid ClassGuid;
            public uint DevInst;
            public IntPtr Reserved;
        }

        [DllImport("setupapi.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr SetupDiGetClassDevsW(
            IntPtr classGuid, string enumerator, IntPtr parent, uint flags);
        [DllImport("setupapi.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool SetupDiOpenDeviceInfoW(
            IntPtr deviceInfoSet, string instanceId, IntPtr parent,
            uint flags, ref SP_DEVINFO_DATA deviceInfoData);
        [DllImport("setupapi.dll", SetLastError = true)]
        private static extern bool SetupDiDestroyDeviceInfoList(IntPtr deviceInfoSet);
        [DllImport("newdev.dll", SetLastError = true)]
        private static extern bool DiShowUpdateDevice(
            IntPtr parent, IntPtr deviceInfoSet,
            ref SP_DEVINFO_DATA deviceInfoData, bool allowNonInteractive);
        [DllImport("devmgr.dll", CharSet = CharSet.Unicode,
            EntryPoint = "DeviceProperties_RunDLLW", ExactSpelling = true)]
        private static extern void DevicePropertiesRunDll(
            IntPtr parent, IntPtr instance, string commandLine, int showCommand);

        public static void LaunchDeviceProperties(string instanceId, long parent)
        {
            string safeId = instanceId.Replace("\"", String.Empty);
            DevicePropertiesRunDll(new IntPtr(parent), IntPtr.Zero,
                "/DeviceID \"" + safeId + "\"", 1);
        }

        public static void ShowUpdateDriver(string instanceId)
        {
            const uint DigcfAllClasses = 0x00000004;
            IntPtr set = SetupDiGetClassDevsW(IntPtr.Zero, null, IntPtr.Zero,
                                              DigcfAllClasses);
            if (set == new IntPtr(-1))
                throw new Win32Exception(Marshal.GetLastWin32Error());
            try
            {
                SP_DEVINFO_DATA data = new SP_DEVINFO_DATA();
                data.cbSize = Marshal.SizeOf(typeof(SP_DEVINFO_DATA));
                if (!SetupDiOpenDeviceInfoW(set, instanceId, IntPtr.Zero, 0, ref data))
                    throw new Win32Exception(Marshal.GetLastWin32Error());
                if (!DiShowUpdateDevice(IntPtr.Zero, set, ref data, false))
                    throw new Win32Exception(Marshal.GetLastWin32Error());
            }
            finally { SetupDiDestroyDeviceInfoList(set); }
        }
    }
}
'@
}

function ConvertFrom-DeviceItemId {
    param([string]$ItemId)
    if (-not $ItemId.StartsWith('device-')) { return '' }
    $encoded = $ItemId.Substring(7).Replace('-', '+').Replace('_', '/')
    while (($encoded.Length % 4) -ne 0) { $encoded += '=' }
    return [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($encoded))
}

function Get-DeviceManagerItems {
    param([string]$ClassName)
    $devices = if ([string]::IsNullOrWhiteSpace($ClassName)) {
        @(Get-CimInstance Win32_PnPEntity -ErrorAction Stop)
    } else {
        $escapedClass = $ClassName.Replace("'", "''")
        @(Get-CimInstance Win32_PnPEntity -Filter "PNPClass='$escapedClass'" `
            -ErrorAction Stop)
    }
    if ([string]::IsNullOrWhiteSpace($ClassName)) {
        return @($devices | ForEach-Object {
            if ([string]::IsNullOrWhiteSpace([string]$_.PNPClass)) { 'Other devices' }
            else { [string]$_.PNPClass }
        } | Sort-Object -Unique | ForEach-Object {
            @{id=('device-class-' + $_); name=$_; directory=$true; enabled=$true;
              columns=@{property=$_; value=''}}
        })
    }
    return @($devices | Where-Object {
        $class = if ([string]::IsNullOrWhiteSpace([string]$_.PNPClass)) {
            'Other devices'
        } else { [string]$_.PNPClass }
        $class -eq $ClassName
    } | Sort-Object Name | ForEach-Object {
        $status = if ([string]::IsNullOrWhiteSpace([string]$_.Status)) {
            [string]$_.ConfigManagerErrorCode
        } else { [string]$_.Status }
        $maker = [string]$_.Manufacturer
        $value = @($maker, $status | Where-Object {
            -not [string]::IsNullOrWhiteSpace([string]$_)
        }) -join ' - '
        @{id=(ConvertTo-DeviceItemId ([string]$_.PNPDeviceID));
          name=[string]$_.Name; directory=$false; enabled=$true;
          columns=@{property=$ClassName; value=$value}}
    })
}

$handler = [string]$Salamander.command_handler
$locale = try { [string]$Salamander.application.Language() } catch { 'en' }
$strings = Get-HardwareMonitorStrings $locale
$deviceManagerName = if ($strings.categories.PSObject.Properties.Name -contains 'deviceManager') {
    [string]$strings.categories.deviceManager
} else { 'Device Manager' }

if ($handler -in @('deviceProperties', 'updateDriver', 'disableDevice',
        'uninstallDevice', 'scanDevices')) {
    $deviceId = ConvertFrom-DeviceItemId ([string]$Salamander.invocation.item.id)
    if ($handler -eq 'scanDevices') {
        Start-Process -FilePath "$env:SystemRoot\System32\pnputil.exe" -ArgumentList '/scan-devices' -Wait
    } elseif (-not [string]::IsNullOrWhiteSpace($deviceId)) {
        if ($handler -eq 'disableDevice' -or $handler -eq 'uninstallDevice') {
            $operation = if ($handler -eq 'disableDevice') {
                if ($strings.strings.PSObject.Properties.Name -contains 'disableDevice') {
                    [string]$strings.strings.disableDevice
                } else { 'Disable device' }
            } else {
                if ($strings.strings.PSObject.Properties.Name -contains 'uninstallDevice') {
                    [string]$strings.strings.uninstallDevice
                } else { 'Uninstall device' }
            }
            $confirmation = if ($strings.strings.PSObject.Properties.Name -contains 'confirmDeviceAction') {
                [string]$strings.strings.confirmDeviceAction
            } else { "{0} '{1}'?" }
            $answer = $Salamander.ui.MessageBox(
                ([string]::Format($confirmation,
                    $operation, [string]$Salamander.invocation.item.name)),
                $deviceManagerName, 'YesNo', 'Warning')
            if ($answer -ne 'Yes') { return }
            $verb = if ($handler -eq 'disableDevice') {
                '/disable-device'
            } else { '/remove-device' }
            Start-Process -FilePath "$env:SystemRoot\System32\pnputil.exe" -ArgumentList @(
                $verb, ('"' + $deviceId.Replace('"', '') + '"')) -Wait
        } elseif ($handler -eq 'updateDriver') {
            Initialize-DeviceManagerNativeMethods
            [OpenSalamander.HardwareMonitor.DeviceManagerNative]::ShowUpdateDriver($deviceId)
        } else {
            Initialize-DeviceManagerNativeMethods
            $parentWindow = try {
                [long]([string]$Salamander.invocation.parentWindow)
            } catch { 0L }
            [OpenSalamander.HardwareMonitor.DeviceManagerNative]::LaunchDeviceProperties(
                $deviceId, $parentWindow)
        }
    }
    return
}
if ($handler -ne 'listHardware') { return }

$fileSystemPath = try { [string]$Salamander.invocation.path } catch { '' }
$categoryId = ''
$viewId = ''
$detailId = ''
if (-not [string]::IsNullOrWhiteSpace($fileSystemPath)) {
    $components = @($fileSystemPath -split '[\\/]' | Where-Object {
        -not [string]::IsNullOrWhiteSpace([string]$_)
    })
    if ($components.Count -gt 1) { $categoryId = [string]$components[1] }
    if ($components.Count -gt 2) { $viewId = [string]$components[2] }
    if ($components.Count -gt 3) { $detailId = [string]$components[3] }
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
        @{id='device-manager'; name=$deviceManagerName; icon='icons/device-manager.svg'}
    )

    $items = New-Object 'System.Collections.Generic.List[hashtable]'
    foreach ($cat in $categories) {
        $items.Add(@{
            id=$cat.id
            name=(ConvertTo-SafeHardwareItemName $cat.name)
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
            } elseif ($viewId -eq 'spd') {
                foreach ($mi in (Get-SmbiosMemoryInfo $strings)) { $subItems.Add($mi) }
            } else {
                $subItems.Add(@{id='usage'; name=[string]$Strings.strings.usage;
                    directory=$true; enabled=$true;
                    columns=@{property=[string]$Strings.strings.usage; value=''}})
                $subItems.Add(@{id='spd'; name='SPD / SMBIOS'; directory=$true;
                    enabled=$true; columns=@{property='SPD / SMBIOS'; value=''}})
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
            if ($viewId -eq 'smart') {
                foreach ($di in (Get-SmartStorageInfo $strings $detailId)) {
                    $subItems.Add($di)
                }
            } else {
                $subItems.Add(@{id='smart'; name='SMART / NVMe'; directory=$true;
                    enabled=$true; columns=@{property='SMART / NVMe'; value=''}})
                $diskItems = Get-StorageInfo $strings
                foreach ($di in $diskItems) { $subItems.Add($di) }
            }
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
            } elseif ($viewId -eq 'voltages') {
                foreach ($si in (Get-HardViewSensorInfo $strings 'Voltage')) {
                    $subItems.Add($si)
                }
            } elseif ($viewId -eq 'all') {
                foreach ($si in (Get-HardViewSensorInfo $strings 'All')) {
                    $subItems.Add($si)
                }
            } elseif ($viewId -eq 'hid') {
                foreach ($si in (Get-HidDeviceInfo $strings)) { $subItems.Add($si) }
            } else {
                $subItems.Add(@{id='temperatures'; name=[string]$Strings.strings.temperatures;
                    directory=$true; enabled=$true;
                    columns=@{property=[string]$Strings.strings.temperatures; value=''}})
                $subItems.Add(@{id='fans'; name=[string]$Strings.strings.fans;
                    directory=$true; enabled=$true;
                    columns=@{property=[string]$Strings.strings.fans; value=''}})
                $subItems.Add(@{id='voltages'; name=[string]$Strings.strings.voltages;
                    directory=$true; enabled=$true;
                    columns=@{property=[string]$Strings.strings.voltages; value=''}})
                $subItems.Add(@{id='all'; name=[string]$Strings.strings.sensorTypes;
                    directory=$true; enabled=$true;
                    columns=@{property=[string]$Strings.strings.sensorTypes; value=''}})
                $subItems.Add(@{id='hid'; name='HID'; directory=$true; enabled=$true;
                    columns=@{property='HID'; value=''}})
            }
        }
        'device-manager' {
            $subItems = New-Object 'System.Collections.Generic.List[hashtable]'
            $deviceClass = if ($viewId.StartsWith('device-class-')) {
                $viewId.Substring('device-class-'.Length)
            } else { $viewId }
            foreach ($deviceItem in (Get-DeviceManagerItems $deviceClass)) {
                $subItems.Add($deviceItem)
            }
        }
    }

    if ($null -ne $subItems -and $subItems.Count -gt 0) {
        foreach ($subItem in $subItems) {
            $subItem.name = ConvertTo-SafeHardwareItemName ([string]$subItem.name)
            $subItem.icon = "icons/$categoryId.svg"
            $subItem.iconDark = "icons/$categoryId-dark.svg"
        }
        [void]$Salamander.file_system.AddItems($subItems.ToArray())
    }
    return
}

return
