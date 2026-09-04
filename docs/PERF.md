# WSL2 下使用 perf 进行 CPU 热点排查指南

本文档沉淀在 WSL2 环境下使用 `perf` 工具对 Cubium 项目进行 CPU 性能剖析的完整经验与标准步骤，供后续复用。

---

## 一、环境前提与限制

### 1.1 WSL2 内核与 perf 的版本错配问题

WSL2 使用微软深度定制的内核（如 `6.6.87.2-microsoft-standard-WSL2`），而 Ubuntu 官方仓库的 `linux-tools` 包与标准 Ubuntu 内核版本严格一一对应。直接通过 `apt install linux-tools-generic` 安装后，`/usr/bin/perf`（一个 wrapper 脚本）会在 `/usr/lib/linux-tools/$(uname -r)/perf` 路径下找不到匹配 WSL2 内核的 perf 二进制，从而报错：

```
WARNING: perf not found for kernel 6.6.87.2-microsoft-standard-WSL2
```

**注意**：网上多篇教程（如知乎、博客园、B站）讲的都是同一个方法——装 `linux-tools-generic`，从 `/usr/lib/linux-tools/<版本>/perf` 找到二进制复制到 `/usr/local/bin`。但这个方法**解决不了硬件计数器不可用的问题**（见 1.2）。`<not supported>` 是 WSL2 内核不暴露硬件 PMU 的架构性限制，跟 perf 二进制装在哪、什么版本无关。

### 1.2 硬件性能计数器（PMU）不可用

WSL2 虚拟化环境下，硬件 PMU（Performance Monitoring Unit）无法访问，导致所有硬件事件均报 `<not supported>`：

```
$ perf stat -e cycles,instructions true
<not supported>      cycles:u
<not supported>      instructions:u
<not supported>      branch-misses:u
<not supported>      cache-misses:u
```

即使满足"WSL ≥ 0.50.2 才支持硬件性能计数器"的版本条件，部分 CPU 架构（如 Alder Lake）仍然不支持。这是 WSL2 的架构性限制，无法通过配置绕过。

### 1.3 可用的替代方案：软件事件 `task-clock`

虽然硬件事件不可用，但软件事件 `task-clock` 可以正常工作：

```bash
# task-clock 软件事件可用
perf stat -e task-clock true
# 输出: 0.38 msec task-clock:u
```

**限制**：`task-clock` 在 WSL2 下采样率偏低。设置 99Hz 频率，实际有效采样率约 8Hz（即 120 秒约采到 12000 个样本）。这对于"大致 CPU 热点排查"足够，但无法做到指令级精确分析。

### 1.4 perf_event_paranoid 权限

WSL2 默认 `perf_event_paranoid = 2`，允许用户态采样。如果遇到权限问题：

```bash
# 查看当前值
cat /proc/sys/kernel/perf_event_paranoid

# 临时放宽（需要 root）
sudo sysctl -w kernel.perf_event_paranoid=1
```

---

## 二、标准操作步骤

### 2.1 定位可用的 perf 二进制

由于 `/usr/bin/perf` wrapper 找不到匹配内核的 perf，需要直接使用系统里已有的 perf 二进制：

```bash
# 查找系统已安装的 perf 二进制
ls /usr/lib/linux-tools-*/perf

# 例如找到: /usr/lib/linux-tools-6.8.0-138/perf
# 直接调用，绕过 wrapper
/usr/lib/linux-tools-6.8.0-138/perf --version
# 输出: perf version 6.8.12
```

建议在 `~/.bashrc` 中添加别名：

```bash
echo 'alias perf="/usr/lib/linux-tools-6.8.0-138/perf"' >> ~/.bashrc
source ~/.bashrc
```

### 2.2 方式一：perf 包裹程序启动（采样启动阶段）

适用于需要分析程序启动初始化阶段的场景。perf 作为父进程启动目标程序：

```bash
perf record -e task-clock -F 99 --call-graph dwarf,16384 \
    -o /tmp/perf_data.data \
    -- ./build/bin/RelWithDebInfo/minecraft-server
```

参数说明：
- `-e task-clock`：使用软件事件（硬件 cycles 不可用时的标准替代）
- `-F 99`：99Hz 采样频率，避开 100Hz 共振
- `--call-graph dwarf,16384`：用 DWARF 解栈（不依赖 frame pointer，最可靠），栈快照大小 16384 字节
- `-o`：输出文件路径
- `--`：分隔 perf 参数和目标程序命令

采样完成后用 `Ctrl+C` 停止（perf 会优雅 flush 数据到 perf.data）。

### 2.3 方式二：perf attach 到已运行进程（采样运行阶段）

适用于程序已在运行、需要采样其持续工作负载的场景：

```bash
# 先找到目标进程 PID
pgrep -f minecraft-server
# 例如输出: 301086

# attach 到该进程采样
perf record -e task-clock -F 99 --call-graph dwarf,16384 \
    -o /tmp/perf_data.data \
    -p <PID>
```

`-p <PID>` 指定要采样的进程。采样过程中程序正常运行，采样结束后 perf 自动 detach。

**注意**：attach 模式下 perf 不会随目标进程退出而退出，需要手动 `Ctrl+C` 或 `kill -INT <perf_pid>` 停止。

### 2.4 采样时长建议

| 目标 | 建议采样时长 | 预期样本数 |
|------|------------|-----------|
| 快速验证 perf 可用性 | 5-10 秒 | 500-1000 |
| 启动初始化阶段热点 | 15-30 秒 | 1500-3000 |
| 持续运行阶段热点 | 60-120 秒 | 6000-12000 |
| 低频事件捕获 | 300+ 秒 | 30000+ |

由于 WSL2 下 `task-clock` 实际采样率约 8Hz，120 秒约采到 12000 个样本，这对"大致 CPU 热点排查"足够。

---

## 三、分析 perf.data

### 3.1 清理损坏的 build-id 缓存（重要！）

perf 在分析阶段会通过 build-id 查找带调试符号的二进制。如果 `/home/<user>/.debug/.build-id/` 下存在损坏的符号链接（指向不存在的路径），会导致 `addr2line` 报错 `could not read first record`，整个 report 输出全是错误信息。

**诊断方法**：

```bash
# 查看 perf.data 里记录的 build-id
perf buildid-list -i /tmp/perf_data.data

# 检查 .debug 缓存是否有损坏的符号链接
ls -la /home/<user>/.debug/.build-id/
```

**清理方法**：

```bash
# 清理所有损坏的 build-id 符号链接
find /home/<user>/.debug/.build-id/ -type l | while read link; do
    target=$(readlink "$link")
    if [ ! -e "$link" ]; then
        rm -f "$link"
        echo "已清理损坏链接: $link -> $target"
    fi
done
```

清理后，perf 会直接从原始二进制（`build/bin/RelWithDebInfo/minecraft-server`）读取 debug info，该二进制带 `with debug_info, not stripped`，符号完整。

### 3.2 快速符号级热点分析（秒级完成）

`perf report --no-children -g none` 不展开调用栈，只做符号级聚合，速度极快（秒级），适合快速定位热点函数：

```bash
perf report -i /tmp/perf_data.data --stdio --no-children -g none > /tmp/perf_hotspots.txt 2>&1
```

输出示例（按线程拆分，每个线程内按占比排序）：

```
     5.94%  ServerMainThrea  minecraft-server  [.] mc::world::chunk::PalettedContainer::get(int) const
     2.18%  ServerCompute-4  minecraft-server  [.] mc::world::gen::noise::PerlinNoise::getValue(double, double, double) const
     ...
```

### 3.3 聚合所有线程的热点排名

`perf report` 默认按线程拆分显示。要得到聚合后的热点排名（把所有线程的相同样本合并统计），需要对输出做后处理：

```bash
grep "%" /tmp/perf_hotspots.txt \
  | awk '{
      pct = $1; sub(/%/, "", pct);
      sym = "";
      in_sym = 0;
      for (i=4; i<=NF; i++) {
          if ($i == "[.]") { in_sym = 1; continue; }
          if (in_sym) sym = sym (sym==""?"":" ") $i;
      }
      if (match(sym, /\[.*$/)) sym = substr(sym, 1, RSTART-1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", sym);
      agg[sym] += pct;
    }
    END {
        for (k in agg) printf "%6.2f%%  %s\n", agg[k], k;
    }' | sort -rn | head -40
```

### 3.4 按子系统归类汇总

为了得到宏观视角，可以按子系统归类汇总 CPU 开销。以下 awk 脚本将符号按功能模块归类：

```bash
grep "%" /tmp/perf_hotspots.txt \
  | awk '{
      pct = $1; sub(/%/, "", pct);
      sym = "";
      in_sym = 0;
      for (i=4; i<=NF; i++) {
          if ($i == "[.]") { in_sym = 1; continue; }
          if (in_sym) sym = sym (sym==""?"":" ") $i;
      }
      if (match(sym, /\[.*$/)) sym = substr(sym, 1, RSTART-1);
      gsub(/^[[:space:]]+|[[:space:]]+$/, "", sym);

      if (sym ~ /PerlinNoise::getValue/) cat = "PerlinNoise 采样";
      else if (sym ~ /CompiledDensityFunction/) cat = "密度函数树求值";
      else if (sym ~ /NoiseInterpolator::compute/) cat = "噪声插值";
      else if (sym ~ /BlendedNoise::compute/) cat = "混合噪声";
      else if (sym ~ /NoiseChunk::/) cat = "NoiseChunk 管理";
      else if (sym ~ /RTreeSubTree::search/) cat = "生物群系 R树搜索";
      else if (sym ~ /PalettedContainer/) cat = "PalettedContainer 操作";
      else if (sym ~ /gen::surface::|SurfaceRule/) cat = "地表规则";
      else if (sym ~ /aquifer::/) cat = "含水层";
      else if (sym ~ /__memset|__memmove|__memcmp/) cat = "libc 内存操作";
      else if (sym ~ /malloc|_int_malloc|_int_free/) cat = "内存分配";
      else if (sym ~ /pthread_mutex/) cat = "锁竞争";
      else if (sym ~ /perfetto/) cat = "Perfetto trace 写入";
      else if (sym ~ /deflate|longest_match|compress_block/) cat = "zlib 压缩";
      else cat = "其他";

      agg[cat] += pct;
    }
    END {
        for (k in agg) printf "%6.2f%%  %s\n", agg[k], k;
    }' | sort -rn
```

### 3.5 展开完整调用链（DWARF 解栈，慢）

如果需要确认热点函数的调用者（即这些热点是否都在某个目标函数的调用栈下），需要展开完整调用链：

```bash
# 含调用链的简报（DWARF 解栈，样本多时会非常慢）
perf report -i /tmp/perf_data.data --stdio --children > /tmp/perf_callchain.txt 2>&1
```

**性能警告**：DWARF 调用栈展开非常慢。12632 个样本全量解栈可能需要 10 分钟以上，且容易因超时被杀。建议：
- 先用 `-g none` 快速看符号级热点
- 如需调用链，考虑用 `perf script` 导出后用 `stackcollapse-perf` + `flamegraph` 处理
- 或直接用代码里已有的 Perfetto/Tracy 双轨 trace（见第五章）

### 3.6 生成火焰图（可选）

```bash
# 导出原始调用栈数据
perf script -i /tmp/perf_data.data > /tmp/perf_script.out

# 需要先 clone FlameGraph 仓库
git clone https://github.com/brendangregg/FlameGraph /tmp/FlameGraph

# 生成火焰图 SVG
/tmp/FlameGraph/stackcollapse-perf.pl /tmp/perf_script.out \
    | /tmp/FlameGraph/flamegraph.pl > /tmp/perf_flame.svg
```

用浏览器打开 `/tmp/perf_flame.svg`，可以直接搜索函数名查看其调用栈占比。

---

## 四、经验教训与避坑指南

### 4.1 `timeout` 命令配合 perf 的陷阱

**问题**：使用 `timeout 25 perf record ...` 时，`timeout` 发送 SIGTERM 给 perf，perf 被强制杀死而非优雅退出，导致 perf.data 没有生成。

**解决**：不要用 `timeout` 包裹 `perf record`。改用以下方式之一：
1. 后台启动 perf，等待 N 秒后用 `kill -INT <perf_pid>` 优雅停止
2. 直接前台运行 perf，手动 `Ctrl+C` 停止

```bash
# 推荐方式：后台启动 + SIGINT 优雅停止
perf record -e task-clock -F 99 --call-graph dwarf,16384 \
    -o /tmp/perf_data.data \
    -p <PID> &

PERF_PID=$!
sleep 120
kill -INT $PERF_PID
wait $PERF_PID
```

### 4.2 `pkill` 误伤当前 shell

**问题**：在脚本中使用 `pkill -9 -f "perf record"` 时，由于当前 shell 的命令行里也包含 "perf record" 字样，`pkill -f` 会匹配到当前 shell 并把它一起杀掉，导致整个命令块 exit 1。

**解决**：
1. 只用 `pkill -x minecraft-server`（精确匹配进程名，不用 `-f`）
2. 或者用 `pkill -9 -x perf`（精确匹配 perf 进程名）
3. 避免在 `pkill -f` 的模式里包含当前脚本自身会出现的字符串

### 4.3 `perf report --stdio` 全量解栈超时

**问题**：对 12632 个 DWARF 调用栈样本执行 `perf report --stdio`，全量解栈非常慢，容易超过 120 秒超时被杀。

**解决**：
1. 先用 `--no-children -g none` 快速看符号级热点（秒级完成）
2. 如需调用链，分批处理或限制输出行数
3. 考虑用 `perf script` + FlameGraph 工具链替代 `perf report`

### 4.4 build-id 缓存损坏导致 addr2line 报错

**问题**：`perf report` 输出全是 `addr2line ... could not read first record` 错误，看不到实际热点。

**根因**：`/home/<user>/.debug/.build-id/<xx>/<yy...>` 下存在损坏的符号链接，指向不存在的路径（如 `../../home/.../minecraft-server/<build-id>`），addr2line 读到的是损坏的缓存文件。

**解决**：见 3.1 节，清理损坏的 build-id 符号链接后，perf 会直接从原始二进制读取 debug info。

### 4.5 `pgrep` 进程名长度限制

**问题**：`pgrep minecraft-server` 报错 `pattern that searches for process name longer than 15 characters will result in zero matches`。

**原因**：Linux 内核限制进程名（comm）最大 15 字符。`minecraft-server` 是 16 字符，会被截断为 `minecraft-serv` 或类似。

**解决**：
1. 用 `pgrep -f minecraft-server`（匹配完整命令行，不受 15 字符限制）
2. 或用 `ps aux | grep minecraft-server | grep -v grep`

### 4.6 stdout 重定向与日志缓冲

**问题**：用 `timeout` 或管道运行服务端时，stdout 日志缓冲区在进程被杀时不会 flush，导致看不到日志输出。

**解决**：
1. 将 stdout/stderr 重定向到文件：`> /tmp/server_stdout.log 2>&1`
2. 使用后台启动方式而非 `timeout` 包裹
3. 服务端日志默认输出到控制台，重定向到文件后可以随时 `tail -f` 查看

---

## 五、与项目内置 trace 工具的配合

Cubium 项目代码中已大量使用 `MC_TRACE_SCOPED_EVENT` 宏进行 Perfetto + Tracy 双轨插桩。在性能分析时，perf 与项目内置 trace 工具各有优势，可以配合使用：

| 工具 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| **perf** | 全局视角，不限插桩点，能看到完整调用栈分布 | WSL2 下硬件事件不可用，采样率偏低 | 大致 CPU 热点排查，发现未预期的热点函数 |
| **Perfetto** | 函数级精确耗时，线程时序可视化 | 需要手动插桩，有约 1.8% 的写入开销 | 精确测量特定函数或子阶段的耗时 |
| **Tracy** | 实时火焰图，可交互分析 | 需要客户端连接 | 开发期实时性能分析 |

### 5.1 推荐的分析流程

1. **先用 perf 快速定位热点区域**：用 `task-clock` 采样，`--no-children -g none` 快速看符号级热点
2. **再用 Perfetto trace 精确测量**：针对 perf 发现的热点函数，查看其 Perfetto trace 中的精确耗时
3. **最后用 Tracy 实时验证**：对优化后的代码用 Tracy 实时观察性能变化

### 5.2 Perfetto trace 文件位置

服务端运行结束后，Perfetto trace 会写入：
- `server_trace.perfetto-trace`（当前工作目录下）

用 Perfetto UI（https://ui.perfetto.dev）打开该文件即可查看可视化 trace。

---

## 六、快速参考命令

### 6.1 完整采样流程（attach 模式）

```bash
# 1. 找到目标进程 PID
pgrep -f minecraft-server

# 2. attach perf 采样（替换 <PID>）
perf record -e task-clock -F 99 --call-graph dwarf,16384 \
    -o /tmp/perf_data.data \
    -p <PID> &

PERF_PID=$!
sleep 120                          # 采样 120 秒
kill -INT $PERF_PID                # 优雅停止
wait $PERF_PID
```

### 6.2 快速分析流程

```bash
# 1. 快速符号级热点（秒级）
perf report -i /tmp/perf_data.data --stdio --no-children -g none > /tmp/perf_hotspots.txt 2>&1

# 2. 聚合所有线程的热点排名
grep "%" /tmp/perf_hotspots.txt | awk '{ ... }' | sort -rn | head -40

# 3. 按子系统归类汇总
grep "%" /tmp/perf_hotspots.txt | awk '{ ... }' | sort -rn
```

### 6.3 清理损坏的 build-id 缓存

```bash
find /home/<user>/.debug/.build-id/ -type l | while read link; do
    if [ ! -e "$link" ]; then
        rm -f "$link"
        echo "已清理损坏链接: $link"
    fi
done
```

### 6.4 生成火焰图

```bash
# 1. 导出原始调用栈数据
perf script -i /tmp/perf_data.data > /tmp/perf_script.out

# 2. clone FlameGraph（如果尚未 clone）
git clone https://github.com/brendangregg/FlameGraph /tmp/FlameGraph 2>/dev/null || true

# 3. 生成火焰图 SVG
/tmp/FlameGraph/stackcollapse-perf.pl /tmp/perf_script.out \
    | /tmp/FlameGraph/flamegraph.pl > /tmp/perf_flame.svg

# 4. 用浏览器打开 /tmp/perf_flame.svg
```

---

## 七、附录：实际采样案例数据

以下数据来自对 `NoiseChunkGenerator::_generateNoiseWithDensityFunction` 的实际采样分析（12632 个 task-clock 样本，120 秒采样）：

### 7.1 采样环境

- **CPU**: Intel(R) Core(TM) i7-14700KF
- **OS**: WSL2 (6.6.87.2-microsoft-standard-WSL2)
- **perf 版本**: 6.8.12
- **事件**: `task-clock:u`，99Hz
- **目标进程**: minecraft-server (PID 301086)
- **线程模型**: 8 个 ServerCompute 线程 + ServerMainThread

### 7.2 CPU 开销分布

| 占比 | 子系统 | 关键函数 |
|------|--------|----------|
| **~16%** | **PerlinNoise 采样** | `PerlinNoise::getValue(double,double,double)` |
| **~14%** | **密度函数树求值** | `CompiledDensityFunction::evalImpl(int,int,int,double*)` |
| **~4%** | 噪声插值 | `NoiseInterpolator::compute` |
| **~4%** | 混合噪声 | `BlendedNoise::compute` |
| **~2%** | NoiseChunk 管理 | `NoiseChunk::updateForZ/updateForX/selectCellXYZ` |
| ~5% | 生物群系采样 | `RTreeSubTree::search`（气候 R 树查询） |
| ~7% | PalettedContainer 读取 | `PalettedContainer::get(int)` |
| ~9% | PalettedContainer 写入/其他 | `packPalettedContainer`、`getAndSet`、`_idFor` |
| ~2% | 地表规则 | `SurfaceRuleContext::cachedY` |
| ~1.5% | 含水层 | `NoiseBasedAquifer::computeSubstance` |
| ~5% | libc 内存操作 | `__memset_avx2`、`__memmove` |
| ~1.8% | Perfetto trace 写入 | `TrackEventInternal::WriteEventName` 等 |
| ~1.4% | 锁竞争 | `pthread_mutex_lock/unlock` |
| ~1.5% | zlib 压缩 | `deflate_slow`、`longest_match` |

### 7.3 核心结论

1. **密度函数树求值是最大热点（~40%）**：`PerlinNoise::getValue` 占 16%，`CompiledDensityFunction::evalImpl` 占 14%，两者合计 30%，加上插值/混合噪声等，密度函数求值链路总占比约 40%。这与 Minecraft 原版的性能特征一致——区块 noise 生成的主要开销就在密度函数树的递归求值上。

2. **PalettedContainer 操作占比偏高（~16%）**：`PalettedContainer::get` (7%) + 写入/其他操作 (9%) 共占 16%。需要进一步通过调用链分析确认这些操作发生在哪个阶段。

3. **Perfetto trace 写入有 1.8% 开销**：代码里大量使用了 `MC_TRACE_SCOPED_EVENT`，trace 事件写入本身有约 1.8% 的 CPU 开销。在性能敏感路径上值得注意。

### 7.4 优化方向建议

1. **密度函数树求值优化**：重点看 `CompiledDensityFunction::evalImpl` 的实现——是否能用 SIMD 优化 `PerlinNoise::getValue`，或减少密度函数树的节点访问次数。

2. **PalettedContainer 操作优化**：16% 的开销偏高，需要看调用链确认这些操作发生在哪个阶段。如果是 `_generateNoiseWithDensityFunction` 内部，那说明密度函数求值过程中有大量区块数据读取。

3. **Perfetto trace 开销优化**：在性能敏感的热点路径上，可以考虑用更轻量的 trace 方式，或在 Release 构建中关闭部分 trace 事件。
