# 周期性采样 minecraft-server.exe 的进程内存指标，输出 CSV。
# 用法: powershell -NoProfile -File sample-mem.ps1 -ProcName minecraft-server -OutCsv <path> [-IntervalSec 2]
param(
    [string]$ProcName = "minecraft-server",
    [string]$OutCsv = "mem.csv",
    [double]$IntervalSec = 2.0,
    [int]$MaxIdleIterations = 0
)

$ErrorActionPreference = "Stop"

$header = "iso_time,elapsed_s,pid,working_set_mb,peak_working_set_mb,private_bytes_mb," +
          "paged_private_mb,virtual_mb,handle_count,thread_count,cpu_s,gen0,gen1,gen2"
$header | Out-File -FilePath $OutCsv -Encoding utf8

$start = Get-Date
$idle = 0
$prev = $null

while ($true) {
    $procs = @(Get-Process -Name $ProcName -ErrorAction SilentlyContinue)
    if ($procs.Count -eq 0) {
        $idle++
        if ($MaxIdleIterations -gt 0 -and $idle -ge $MaxIdleIterations) { break }
        Start-Sleep -Milliseconds 500
        continue
    }
    $idle = 0
    $p = $procs[0]
    $now = Get-Date
    $elapsed = [math]::Round(($now - $start).TotalSeconds, 3)

    $line = "{0},{1},{2},{3:F3},{4:F3},{5:F3},{6:F3},{7:F3},{8},{9},{10:F3},{11},{12},{13}" -f `
        $now.ToString("yyyy-MM-dd HH:mm:ss.fff"), $elapsed, $p.Id,
        ($p.WorkingSet64 / 1MB), ($p.PeakWorkingSet64 / 1MB), ($p.PrivateMemorySize64 / 1MB),
        ($p.PagedMemorySize64 / 1MB), ($p.VirtualMemorySize64 / 1MB),
        $p.HandleCount, $p.Threads.Count, $p.TotalProcessorTime.TotalSeconds,
        $p.Threads.Count, 0, 0
    $line | Out-File -FilePath $OutCsv -Append -Encoding utf8
    $prev = $p

    Start-Sleep -Milliseconds ([int]($IntervalSec * 1000))
}
