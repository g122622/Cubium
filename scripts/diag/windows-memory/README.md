# Windows 服务端内存诊断脚本

用于定位 `minecraft-server.exe` 在 Windows 上的内存占用构成。方法论与实测数据见 `docs/MEMORY.md`。

## 目录结构

```
scripts/diag/windows-memory/
├── README.md           # 本文件
├── resident2.ps1       # 逐页驻留归因（核心工具）：枚举已提交区域 + QueryWorkingSetEx 判定每页是否驻留
├── vmmap2.ps1          # 区域分类：按 State/Type/Protect/AllocationBase/File 聚合（无驻留信息）
├── sample-mem.ps1      # 周期性采样工作集/提交量/句柄/线程数到 CSV
├── parse_umdh.py       # 解析 UMDH 堆快照，按调用栈聚合 Top 分配点
└── report.py           # 把 cdb 的 `ln` 符号解析结果合并回 parse_umdh.py 的输出
```

## 使用前提

**必须使用 `cdb.exe`（Windows SDK Debuggers）**，路径通常为 `D:\Windows Kits\10\Debuggers\x64\`。`resident2.ps1` 与 `vmmap2.ps1` 仅需 PowerShell。

## 标准排查流程

### 第 0 步：剥离 Tracy 诊断开销（最关键）

Windows 上若不剥离，测量结果会被 ~178 MB 的 Tracy 诊断开销主导。**无需重新构建**的临时手段：

```powershell
$env:TRACY_SYMBOL_OFFLINE_RESOLVE = '1'
Start-Process -FilePath build\bin\RelWithDebInfo\minecraft-server.exe -ArgumentList '--profiler-enabled=false'
```

根本解是改用 `windows-clang-release-noprof` 预设构建。

### 第 1 步：量化与驻留归因

```powershell
# 逐页驻留归因（先拿到 PID）
.\resident2.ps1 -ProcId <PID> -Tag t0
```

输出 `res_<tag>_summary.txt`（分类汇总）与 `res_<tag>_detail.csv`（逐区域明细）。**看驻留量，别看提交量**——Tracy 的 rpmalloc 会把提交量推到 800 MB 以上而实际驻留仅 1 MB。

### 第 2 步：堆尺寸直方图（定位大头的第一手段）

用 cdb attach 后执行 `!heap -s` 拿到堆地址，再：

```
!heap -stat -h <heapAddr> -m 800
```

尺寸直方图能立刻暴露「某尺寸 × 大量块」这类集中分配，比逐个符号化调用栈快得多。

### 第 3 步：内容指纹（区分同尺寸不同用途）

对可疑尺寸类抽样转储，按字节模式判别用途：

```
!heap -flt s <hexSize>
db <userPtr> L20
```

典型指纹：全 `0xFF` = 天空光全亮 nibble；全 `0x00` = 无光段或全同值 palette；规律半字节（`0x1111…`/`0x2222…`）= palette 索引打包。

### 第 4 步：调用栈归因（确证归属）

**方式 A：cdb 符号注释**（最快）。`!heap -flt s <size>` 的块清单里会夹杂类型名注释（如 `minecraft_server!std::_Ref_count_obj2<...>::vftable`），可直接读出类型。

**方式 B：UMDH 全栈快照**（最完整，需先开 UST）：

```bash
# 开启用户态堆栈追踪（测完务必用 -ust 关闭）
"/d/Windows Kits/10/Debuggers/x64/gflags.exe" //i minecraft-server.exe +ust

# 重启进程后抓快照
export _NT_SYMBOL_PATH="srv*<symCacheDir>*https://msdl.microsoft.com/download/symbols"
"/d/Windows Kits/10/Debuggers/x64/umdh.exe" -p:<PID> -f:umdh.txt

# 解析并符号化
python parse_umdh.py umdh.txt
# 用 cdb 对 umdh.txt.stacks.txt 中的地址批量执行 `ln <addr>`，输出到 sym_resolved.txt
python report.py sym_resolved.txt umdh.txt.stacks.txt
```

**方式 C：单块精确归因**（UST 已开时）：

```
!heap -p -a <userPtr>
```

## 容易踩的坑

1. **`--profiler-enabled=false` 不能关闭 Tracy**。该 flag 只门控 Perfetto 侧；Tracy 是编译期接入，静态初始化期即分配，且 `MC_TRACY_ON_DEMAND` 只影响事件入队、不影响这些分配。不剥离 Tracy 就开始优化，任何收益都会被 178 MB 噪声淹没。

2. **PowerShell 脚本须带 UTF-8 BOM**。本目录脚本含中文注释，用 `-File` 调用时无 BOM 会 parser error（`Unexpected token ')'`）。新增脚本务必保持 BOM。

3. **`!heap -flt s <size>` 的 `UserSize` 是十六进制且不带 `0x`**。例如 2048 字节要写 `800`，8657 字节写 `21d1`。传错值会返回 0 块而不报错。

4. **UST（`+ust`）本身有显著开销**，且会把堆切到更慢的路径。测完必须 `gflags //i <exe> -ust` 关闭，否则后续测量全部被污染。

5. **UMDH 快照非常大**（实测 41 MB / 190 万行），`grep` 全文件会很慢；用 `parse_umdh.py` 流式解析。

6. **`cdb -p <pid>` 在进程已退出时报 `Win32 error 0n87`（参数错误）**，容易被误判为命令写错。先确认进程存活。

7. **`!address -summary` 需要 ntdll 符号**（`srv*<cache>*https://msdl.microsoft.com/download/symbols`），首次会联网下载；不加符号路径会报 `No symbols for ntdll. Cannot continue.`，但 `!heap -stat` 与 `!heap -flt` 不需要符号即可用。

8. **`resident2.ps1` 用一次 `QueryWorkingSetEx` 批量查全部页**（实测 24 万页约 10 秒），不要改成逐页调用——那会慢两个数量级。
