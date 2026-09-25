# Enumerate all memory regions of a target process via VirtualQueryEx and aggregate
# by State/Type/Protect/AllocationBase/File.  Windows analogue of `vmmap -summary`.
# Usage: powershell -NoProfile -File vmmap2.ps1 -ProcId <pid> -Tag <t0|t1|...>
param(
    [Parameter(Mandatory = $true)][int]$ProcId,
    [string]$OutDir = "C:\Users\Administrator\AppData\Local\Temp\memdiag",
    [string]$Tag = "t0",
    [int]$TopN = 25
)

$ErrorActionPreference = "Stop"

$src = @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class VMMapNative {
    [StructLayout(LayoutKind.Sequential)]
    public struct MEMORY_BASIC_INFORMATION {
        public IntPtr BaseAddress;
        public IntPtr AllocationBase;
        public uint AllocationProtect;
        public IntPtr RegionSize;
        public uint State;
        public uint Protect;
        public uint Type;
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern int VirtualQueryEx(IntPtr hProcess, IntPtr lpAddress, out MEMORY_BASIC_INFORMATION lpBuffer, IntPtr dwLength);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool CloseHandle(IntPtr hObject);

    [DllImport("psapi.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern uint GetMappedFileNameW(IntPtr hProcess, IntPtr lpv, StringBuilder lpFilename, uint nSize);

    [DllImport("psapi.dll", SetLastError = true)]
    public static extern bool GetModuleInformation(IntPtr hProcess, IntPtr hModule, out MODULEINFO lpmodinfo, uint cb);

    [StructLayout(LayoutKind.Sequential)]
    public struct MODULEINFO {
        public IntPtr lpBaseOfDll;
        public uint SizeOfImage;
        public IntPtr EntryPoint;
    }

    [DllImport("psapi.dll", SetLastError = true)]
    public static extern bool EnumProcessModules(IntPtr hProcess, IntPtr[] lphModule, uint cb, out uint lpcbNeeded);
}
"@

Add-Type -TypeDefinition $src -Language CSharp

# PROCESS_QUERY_INFORMATION(0x0400) | PROCESS_VM_READ(0x0010)
$h = [VMMapNative]::OpenProcess(0x0410, $false, $ProcId)
if ($h -eq [IntPtr]::Zero) { throw "OpenProcess failed for pid $ProcId" }

# ---- build module base -> module file map so we can attribute allocations to DLLs
$modMap = @{}
$need = 0
$buf = New-Object IntPtr[] 2048
if ([VMMapNative]::EnumProcessModules($h, $buf, [uint32](2048 * [IntPtr]::Size), [ref]$need)) {
    $n = [int]($need / [IntPtr]::Size)
    $sb2 = New-Object System.Text.StringBuilder 1024
    for ($i = 0; $i -lt $n; $i++) {
        $mi = New-Object VMMapNative+MODULEINFO
        if ([VMMapNative]::GetModuleInformation($h, $buf[$i], [ref]$mi, [uint32][Runtime.InteropServices.Marshal]::SizeOf([type][VMMapNative+MODULEINFO]))) {
            $base = [int64]$mi.lpBaseOfDll
            $end = $base + [int64]$mi.SizeOfImage
            $modMap[$base] = @{ End = $end; Name = ("mod_0x{0:X}" -f $base) }
        }
    }
}

$stateName = @{ 0x1000 = "COMMIT"; 0x2000 = "RESERVE"; 0x10000 = "FREE" }
$typeName = @{ 0x1000000 = "IMAGE"; 0x40000 = "MAPPED"; 0x20000 = "PRIVATE" }

function Get-ProtName([uint32]$p) {
    if ($p -eq 0) { return "-" }
    $base = $p -band 0xFF
    $flags = ""
    if ($p -band 0x100) { $flags += "G" }
    if ($p -band 0x200) { $flags += "N" }
    if ($p -band 0x400) { $flags += "W" }
    if ($p -band 0x40000000) { $flags += "!" }
    switch ($base) {
        0x01 { return "NOACCESS$flags" }
        0x02 { return "R$flags" }
        0x04 { return "RW$flags" }
        0x08 { return "WCOPY$flags" }
        0x10 { return "X$flags" }
        0x20 { return "RX$flags" }
        0x40 { return "RWX$flags" }
        0x80 { return "RWXCOPY$flags" }
        default { return ("P{0:X}$flags" -f $base) }
    }
}

$mbiSize = [Runtime.InteropServices.Marshal]::SizeOf([type][VMMapNative+MEMORY_BASIC_INFORMATION])
$addr = [IntPtr]::Zero
$rows = New-Object System.Collections.Generic.List[object]
$sb = New-Object System.Text.StringBuilder 1024
$count = 0

while ($true) {
    $mbi = New-Object VMMapNative+MEMORY_BASIC_INFORMATION
    $ret = [VMMapNative]::VirtualQueryEx($h, $addr, [ref]$mbi, [IntPtr]$mbiSize)
    if ($ret -eq 0) { break }
    $count++
    if ($count -gt 400000) { break }

    $fname = ""
    if ($mbi.Type -eq 0x1000000 -or $mbi.Type -eq 0x40000) {
        [void]$sb.Clear()
        if ([VMMapNative]::GetMappedFileNameW($h, $mbi.BaseAddress, $sb, 1024) -gt 0) {
            $fname = $sb.ToString()
            $fname = Split-Path -Leaf $fname
        }
    }

    $allocBase = [int64]$mbi.AllocationBase
    $owner = ""
    if ($modMap.ContainsKey($allocBase)) { $owner = $modMap[$allocBase].Name }

    $rows.Add([pscustomobject]@{
        Base      = [int64]$mbi.BaseAddress
        AllocBase = $allocBase
        SizeMB    = [math]::Round([int64]$mbi.RegionSize / 1MB, 5)
        State     = $stateName[[int]$mbi.State]
        Type      = $typeName[[int]$mbi.Type]
        Protect   = Get-ProtName $mbi.Protect
        File      = $fname
        Owner     = $owner
    })

    $next = [int64]$mbi.BaseAddress + [int64]$mbi.RegionSize
    if ($next -le [int64]$mbi.BaseAddress) { break }
    $addr = [IntPtr]$next
}
[void][VMMapNative]::CloseHandle($h)

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$detailPath = Join-Path $OutDir "vm_$Tag`_detail.csv"
$rows | Export-Csv -Path $detailPath -NoTypeInformation -Encoding utf8

$out = New-Object System.Collections.Generic.List[string]
$out.Add("=== $Tag  regions=$($rows.Count) ===")
$out.Add("")
$out.Add("--- by State+Type (committed MB) ---")
$rows | Group-Object { "$($_.State)/$($_.Type)" } |
    Sort-Object { -($_.Group | Measure-Object SizeMB -Sum).Sum } | ForEach-Object {
    $sum = ($_.Group | Measure-Object SizeMB -Sum).Sum
    $out.Add(("{0,-20} {1,11:F2} MB  ({2} regions)" -f $_.Name, $sum, $_.Count))
}
$out.Add("")
$out.Add("--- COMMIT by Type+Protect ---")
$rows | Where-Object { $_.State -eq "COMMIT" } | Group-Object { "$($_.Type)/$($_.Protect)" } |
    Sort-Object { -($_.Group | Measure-Object SizeMB -Sum).Sum } | Select-Object -First 20 | ForEach-Object {
    $sum = ($_.Group | Measure-Object SizeMB -Sum).Sum
    $out.Add(("{0,-20} {1,11:F2} MB  ({2} regions)" -f $_.Name, $sum, $_.Count))
}
$out.Add("")
$out.Add("--- COMMIT PRIVATE size buckets ---")
$rows | Where-Object { $_.State -eq "COMMIT" -and $_.Type -eq "PRIVATE" } |
    Group-Object { if ($_.SizeMB -ge 64) { "a>=64MB" } elseif ($_.SizeMB -ge 16) { "b16-64MB" } elseif ($_.SizeMB -ge 4) { "c4-16MB" } elseif ($_.SizeMB -ge 1) { "d1-4MB" } elseif ($_.SizeMB -ge 0.0625) { "e64KB-1MB" } else { "f<64KB" } } |
    Sort-Object Name | ForEach-Object {
    $sum = ($_.Group | Measure-Object SizeMB -Sum).Sum
    $out.Add(("{0,-14} {1,11:F2} MB  ({2} regions)" -f $_.Name, $sum, $_.Count))
}
$out.Add("")
$out.Add("--- COMMIT PRIVATE grouped by AllocationBase, Top $TopN (MB) ---")
$rows | Where-Object { $_.State -eq "COMMIT" -and $_.Type -eq "PRIVATE" } |
    Group-Object AllocBase | Sort-Object { -($_.Group | Measure-Object SizeMB -Sum).Sum } | Select-Object -First $TopN | ForEach-Object {
    $sum = ($_.Group | Measure-Object SizeMB -Sum).Sum
    $own = ($_.Group | Select-Object -First 1).Owner
    $out.Add(("{0,11:F2} MB  allocBase=0x{1:X12}  regions={2,-5} {3}" -f $sum, [int64]$_.Name, $_.Count, $own))
}
$out.Add("")
$out.Add("--- COMMIT MAPPED/IMAGE grouped by File, Top $TopN (MB) ---")
$rows | Where-Object { $_.State -eq "COMMIT" -and $_.File -ne "" } |
    Group-Object File | Sort-Object { -($_.Group | Measure-Object SizeMB -Sum).Sum } | Select-Object -First $TopN | ForEach-Object {
    $sum = ($_.Group | Measure-Object SizeMB -Sum).Sum
    $out.Add(("{0,11:F2} MB  {1}" -f $sum, $_.Name))
}
$out.Add("")
$out.Add("--- Top $TopN largest COMMIT PRIVATE regions ---")
$rows | Where-Object { $_.State -eq "COMMIT" -and $_.Type -eq "PRIVATE" } |
    Sort-Object SizeMB -Descending | Select-Object -First $TopN | ForEach-Object {
    $out.Add(("{0,10:F3} MB  0x{1:X16}  alloc=0x{2:X16} {3,-8} {4}" -f $_.SizeMB, $_.Base, $_.AllocBase, $_.Protect, $_.File))
}

$summaryPath = Join-Path $OutDir "vm_$Tag`_summary.txt"
$out | Out-File -FilePath $summaryPath -Encoding ascii
$out | ForEach-Object { Write-Output $_ }
Write-Output ""
Write-Output "detail: $detailPath"
