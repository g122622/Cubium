# 一次性对全部已提交区域做 QueryWorkingSetEx，统计每区域与每分类的物理驻留量。
# 比逐页调用快得多：把所有页地址塞进一个大数组，一次 API 调用。
param(
    [Parameter(Mandatory = $true)][int]$ProcId,
    [string]$OutDir = "C:\Users\Administrator\AppData\Local\Temp\memdiag",
    [string]$Tag = "t0"
)

$ErrorActionPreference = "Stop"

$src = @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class RN {
    [StructLayout(LayoutKind.Sequential)]
    public struct MBI { public IntPtr BaseAddress; public IntPtr AllocationBase; public uint AllocationProtect; public IntPtr RegionSize; public uint State; public uint Protect; public uint Type; }

    [StructLayout(LayoutKind.Sequential)]
    public struct WSEX { public IntPtr VirtualAddress; public UIntPtr Flags; }

    [DllImport("kernel32.dll", SetLastError = true)] public static extern IntPtr OpenProcess(uint a, bool b, int pid);
    [DllImport("kernel32.dll", SetLastError = true)] public static extern int VirtualQueryEx(IntPtr h, IntPtr addr, out MBI mbi, IntPtr len);
    [DllImport("kernel32.dll", SetLastError = true)] public static extern bool CloseHandle(IntPtr h);
    [DllImport("psapi.dll", CharSet = CharSet.Unicode, SetLastError = true)] public static extern uint GetMappedFileNameW(IntPtr h, IntPtr p, StringBuilder sb, uint n);
    [DllImport("psapi.dll", SetLastError = true)] public static extern bool QueryWorkingSetEx(IntPtr h, [In, Out] WSEX[] pv, uint cb);

    public static long[] Probe(IntPtr h, long[] pageAddrs) {
        int n = pageAddrs.Length;
        var arr = new WSEX[n];
        for (int i = 0; i < n; i++) { arr[i].VirtualAddress = (IntPtr)pageAddrs[i]; arr[i].Flags = UIntPtr.Zero; }
        if (!QueryWorkingSetEx(h, arr, (uint)(n * 16))) return null;
        var res = new long[n];
        for (int i = 0; i < n; i++) {
            ulong f = (ulong)arr[i].Flags;
            res[i] = ((f & 1UL) != 0) ? 1L : 0L;   // bit0 = Valid
        }
        return res;
    }
}
"@

Add-Type -TypeDefinition $src -Language CSharp

$h = [RN]::OpenProcess(0x0410, $false, $ProcId)
if ($h -eq [IntPtr]::Zero) { throw "OpenProcess failed for pid $ProcId" }

$PAGE = 0x1000
$mbiSize = [Runtime.InteropServices.Marshal]::SizeOf([type][RN+MBI])

# 第一遍：枚举已提交区域
$regions = New-Object System.Collections.Generic.List[object]
$addr = [IntPtr]::Zero
$sb = New-Object System.Text.StringBuilder 1024
$guard = 0
while ($true) {
    $mbi = New-Object RN+MBI
    if ([RN]::VirtualQueryEx($h, $addr, [ref]$mbi, [IntPtr]$mbiSize) -eq 0) { break }
    $guard++; if ($guard -gt 400000) { break }
    if ($mbi.State -eq 0x1000) {
        $f = ""
        if ($mbi.Type -eq 0x1000000 -or $mbi.Type -eq 0x40000) {
            [void]$sb.Clear()
            if ([RN]::GetMappedFileNameW($h, $mbi.BaseAddress, $sb, 1024) -gt 0) { $f = Split-Path -Leaf $sb.ToString() }
        }
        $regions.Add([pscustomobject]@{
            Base = [int64]$mbi.BaseAddress; Size = [int64]$mbi.RegionSize
            Protect = $mbi.Protect; Type = $mbi.Type; File = $f
        })
    }
    $next = [int64]$mbi.BaseAddress + [int64]$mbi.RegionSize
    if ($next -le [int64]$mbi.BaseAddress) { break }
    $addr = [IntPtr]$next
}

# 第二遍：把全部页地址收进一个数组，一次调用
$pages = New-Object System.Collections.Generic.List[int64]
$regPageStart = New-Object System.Collections.Generic.List[int]
foreach ($r in $regions) {
    $regPageStart.Add($pages.Count)
    $n = [int]($r.Size / $PAGE)
    $b = $r.Base
    for ($i = 0; $i -lt $n; $i++) { $pages.Add($b + $i * $PAGE) }
}
Write-Host "committed regions: $($regions.Count)  pages: $($pages.Count)"

$flags = [RN]::Probe($h, $pages.ToArray())
if ($null -eq $flags) { throw "QueryWorkingSetEx failed" }
[void][RN]::CloseHandle($h)

$typeName = @{ 0x1000000 = "IMAGE"; 0x40000 = "MAPPED"; 0x20000 = "PRIVATE" }
function PName([uint32]$p) {
    $b = $p -band 0xFF; $fl = ""
    if ($p -band 0x100) { $fl += "G" }; if ($p -band 0x200) { $fl += "N" }; if ($p -band 0x400) { $fl += "C" }
    switch ($b) { 0x01 {"NOACCESS$fl"} 0x02 {"R$fl"} 0x04 {"RW$fl"} 0x08 {"WCOPY$fl"} 0x10 {"X$fl"} 0x20 {"RX$fl"} 0x40 {"RWX$fl"} 0x80 {"RWXCOPY$fl"} default {("P{0:X}$fl" -f $b)} }
}

$rows = New-Object System.Collections.Generic.List[object]
for ($i = 0; $i -lt $regions.Count; $i++) {
    $r = $regions[$i]
    $start = $regPageStart[$i]
    $n = [int]($r.Size / $PAGE)
    $res = 0
    for ($j = 0; $j -lt $n; $j++) { if ($flags[$start + $j] -eq 1) { $res++ } }
    $rows.Add([pscustomobject]@{
        Base = $r.Base; SizeMB = [math]::Round($r.Size / 1MB, 4)
        ResidentMB = [math]::Round($res * $PAGE / 1MB, 4)
        Ratio = if ($n -gt 0) { [math]::Round($res / $n, 3) } else { 0 }
        Type = $typeName[[int]$r.Type]; Protect = (PName $r.Protect); File = $r.File
    })
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$dp = Join-Path $OutDir "res_$Tag`_detail.csv"
$rows | Export-Csv -Path $dp -NoTypeInformation -Encoding utf8

$o = New-Object System.Collections.Generic.List[string]
$o.Add("=== resident $Tag ===")
foreach ($t in @("PRIVATE", "IMAGE", "MAPPED")) {
    $s = $rows | Where-Object { $_.Type -eq $t }
    $o.Add(("{0,-8} committed {1,9:F2} MB   resident {2,9:F2} MB  ({3} regions)" -f $t, ($s | Measure-Object SizeMB -Sum).Sum, ($s | Measure-Object ResidentMB -Sum).Sum, $s.Count))
}
$o.Add(("{0,-8} committed {1,9:F2} MB   resident {2,9:F2} MB" -f "TOTAL", ($rows | Measure-Object SizeMB -Sum).Sum, ($rows | Measure-Object ResidentMB -Sum).Sum))
$o.Add("")
$o.Add("--- PRIVATE by protect ---")
$rows | Where-Object { $_.Type -eq "PRIVATE" } | Group-Object Protect | Sort-Object { -($_.Group | Measure-Object ResidentMB -Sum).Sum } | ForEach-Object {
    $o.Add(("{0,-14} committed {1,9:F2} MB  resident {2,9:F2} MB  ({3} regions)" -f $_.Name, ($_.Group | Measure-Object SizeMB -Sum).Sum, ($_.Group | Measure-Object ResidentMB -Sum).Sum, $_.Count))
}
$o.Add("")
$o.Add("--- PRIVATE by committed-size bucket ---")
$rows | Where-Object { $_.Type -eq "PRIVATE" } | Group-Object {
    if ($_.SizeMB -ge 16) { "a >=16MB" } elseif ($_.SizeMB -ge 4) { "b 4-16MB" } elseif ($_.SizeMB -ge 1) { "c 1-4MB" } elseif ($_.SizeMB -ge 0.25) { "d 256KB-1MB" } elseif ($_.SizeMB -ge 0.0625) { "e 64-256KB" } else { "f <64KB" } } |
    Sort-Object Name | ForEach-Object {
    $o.Add(("{0,-14} committed {1,9:F2} MB  resident {2,9:F2} MB  ({3} regions)" -f $_.Name, ($_.Group | Measure-Object SizeMB -Sum).Sum, ($_.Group | Measure-Object ResidentMB -Sum).Sum, $_.Count))
}
$o.Add("")
$o.Add("--- top 25 resident PRIVATE regions ---")
$rows | Where-Object { $_.Type -eq "PRIVATE" } | Sort-Object ResidentMB -Descending | Select-Object -First 25 | ForEach-Object {
    $o.Add(("{0,9:F3} MB  0x{1:X12}  resident {2,8:F3} MB ({3,5:P0})  {4}" -f $_.SizeMB, [int64]$_.Base, $_.ResidentMB, $_.Ratio, $_.Protect))
}
$sp = Join-Path $OutDir "res_$Tag`_summary.txt"
$o | Out-File -FilePath $sp -Encoding ascii
$o | ForEach-Object { Write-Output $_ }
