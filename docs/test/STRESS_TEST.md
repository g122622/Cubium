# 机器人压力测试

代码位于 `tests/stress/`，直接在此目录安装依赖和运行，不建立 `bot/` 子目录。
技术栈与 `tests/e2e/bot/` 一致：Node.js 原生执行 TypeScript、mineflayer 4.39.0、
minecraft-protocol 1.68.0、minecraft-data 3.115.0，目标 Java 版 1.21.11（协议 774）。
服务端进程清理、端口分配和离线 OP 列表复用 e2e 基础设施，压测不保存逐包快照。

## 运行

前提：Node.js 24+、已构建的服务端，以及正常安装的 Vanilla 数据包。
构建时须启用 `MC_ENABLE_TRACING` 或 `MC_ENABLE_TRACY`，以保留已有内存采样线程；
默认构建已启用 Perfetto。压测启动时显式传入 `--profiler_enabled=false`，不录制完整 trace。

```powershell
cd tests/stress
npm ci
node run.ts                         # 全部用例、全部速度档位，严格串行
node run.ts --case=movement         # 全部移动速度档位
node run.ts --case=packets
node run.ts --case=joins
node run.ts --help
npm run check                       # 仅检查 CLI、CSV、恢复窗口和长 tick 阶段归属
```

每个用例（包括每个速度档位）启动独立服务端，使用独立新世界、相同种子。
默认普通世界、创造模式、和平难度、视距和模拟距离均为 8。控制机器人使用 OP，
位于维度最高建造高度上方，避免碰撞和落体干扰，移动始终沿 +X。

| 用例 | 默认施压过程 | 恢复阶段 |
|---|---|---|
| `movement-<速度>` | 5.6、16、32、64、128、256 格/秒逐档串行，每档移动 60 秒 | 保持连接与当前位置，停止移动 15 秒 |
| `packets` | 单玩家每秒 1000 个有效协议包，持续 60 秒；均匀混发旋转、挥手、着地状态 | 停止主动发包 15 秒，保留正常协议响应 |
| `joins` | 100 名负载玩家，每秒发起 10 次加入；全部完成 spawn 后保持在线 60 秒 | 负载玩家集中退出，控制玩家保留，观察 15 秒 |

每个用例先预热 10 秒，再采集 10 秒基线。多人用例的服务器容量包含额外的 1 名控制玩家，
保持在线阶段服务端应报告 101 名玩家。负载玩家出生于同一区域，主要考察登录、区块下发和实体广播；
跨区域持续区块生成由移动用例覆盖。每一档都执行，即使前一档失败也继续；
服务端未能停止或端口未释放时中止后续用例，防止进程重叠。

### 调整负载

```powershell
# 自选速度仍逐档串行，默认每档完整移动与恢复时长不变。
node run.ts --case=movement --speeds=4.3,32,128,512

# 默认保留移动速度校验；此参数仅在本次移动压测中豁免速度阈值。
node run.ts --case=movement --speeds=128,256 --skip-movement-speed-check

# 同时发起 200 名玩家的真实登录与出生流程。
node run.ts --case=joins --players=200 --join-rate=0

# 5000 包/秒，持续两分钟。
node run.ts --case=packets --packets-per-second=5000 --packet-seconds=120

# 小规模冒烟：仅用于验证脚本链路，不能代替完整时长的性能结论。
node run.ts --world=default --speeds=5.6,64 --warmup-seconds=1 --baseline-seconds=2 --move-seconds=3 --recovery-seconds=2 --packet-seconds=3 --players=3 --join-rate=3 --hold-seconds=3
```

其余参数包括 `--server=<可执行文件>`、`--output=<产物根目录>`、`--world=default|flat`、
`--seed=0`、`--view-distance=8`、`--simulation-distance=8`、`--spawn-timeout-seconds=120`。
所有参数都在 CLI 边界确定，最终有效值写入 `options.json`。发包速率范围为 1～100000 包/秒，
避免无界同步批次阻塞发生器的健康检查和清理流程。

默认测试真实玩家路径，保留服务端移动速度校验。高速或低 TPS 下可能触发
`moved too quickly` 回弹，未达到目标距离会判失败；这类结果不能直接代表区块系统吞吐。
需要让极高速档实际持续跨区块时，显式加 `--skip-movement-speed-check`：运行器仅为移动用例
传入默认关闭的服务端 flag `--stress_skip_movement_speed_check=true`，用例名追加 `-unchecked`。
两种模式的结果分别保留；豁免只跳过玩家速度阈值，坐标与传送确认仍经过原有处理。

## 测量和判定

服务端通过 `--stress_metrics_file=<路径>` 导出 `server-metrics.csv`。内存由
`ProfilerManager` 原有专用线程以 1000Hz 采样，CSV 在每个 tick 完成后读取最近样本，
因此导出频率通常约 20Hz；卡顿时随实际 tick 频率降低。内存采样不在 tick 主线程执行。
该记录适合观察持续增长与回落，不能保留两个 tick 之间的所有瞬时内存尖峰。

CSV 的 `tick_ms` 是一次 `tick()` 的实际执行耗时，不含 20TPS 节流等待；`interval_ms`
是相邻 tick 起始时间间隔，正常情况下约为 50ms。两者分别统计，避免将 tick 间隔当作执行耗时。
`time_ms` 与 `memory_sample_ms` 均为 Unix 毫秒。tick 执行区间只要与阶段重叠便计入该阶段的
tick 统计，因此跨阶段的长 tick 可以出现在多个阶段中；内存按 `memory_sample_ms` 归入阶段及图表秒桶，
不会拿随后完成的 tick 样本冒充阶段内的内存读数。恢复后另有 `verification` 阶段等待协议确认，
用来保留跨越恢复结束时刻才完成的长 tick。CSV 另含玩家数、区块持有者数、已加载区块数和实体数。

内存同时报告工作集与提交量，单位 MiB。Windows 的提交量来自 `PagefileUsage`；
Linux/macOS 的现有平台接口返回 `VmSize` / `virtual_size`，属于虚拟地址空间，不能直接当作 Windows 私有提交量比较。
CSV 每秒 flush，异常硬退出可能丢失最后约一秒尚未 flush 的尾部记录。

默认判定规则：

- 崩溃、意外掉线、出生超时、采样缺失、发送链路积压均判失败。
- 移动距离须达到请求距离的 90%，并通过服务端将玩家传送到自身位置的回包核实权威位置；移动过程中须收到新区块。
- 主动发包数须达到请求包数的 90%；随后用同一 TCP 流的 ping/pong 确认服务端已处理此前数据。
- 多人用例须全部完成 spawn，保持在线阶段服务端人数须等于负载玩家数加控制玩家。
- 性能回落默认只报告，不因内存分配器保留空闲页而直接失败。

恢复比较使用最后 5 秒（恢复阶段不足 5 秒时取全段），与本用例基线比较。
报告给出内存中位数比值、tick P95 比值，以及是否满足内存不高于基线 120%、tick P95 不高于 50ms。
需要以此作为门禁时使用 `--strict-recovery`，阈值可通过 `--memory-ratio=1.2 --tick-limit-ms=50` 调整。
工作集与提交量在严格模式下均须满足阈值。

退出码：`0` 全部通过；`1` 存在用例失败；`2` 参数或环境错误、无法继续安全地串行运行。
发生器与服务端位于同一台机器，报告另存 Node 进程 RSS 与事件循环延迟，辅助识别发生器瓶颈。

## 产物

所有结果保留在 `build/stress/<时间戳-进程号>/`，不写入源码树，也不自动删除历史压测世界：

```text
<run>/
├── index.html                   # 全部档位与用例结果入口
├── options.json                 # 最终有效参数
├── summary.json                 # 按运行顺序汇总通过与失败
└── <case>/
    ├── report.html              # 阶段表格与可悬停读数的内存、tick 曲线
    ├── summary.json             # 负载达成、分位数、峰值、恢复窗口和错误
    ├── server-metrics.csv       # 服务端逐 tick 原始测量
    ├── phases.json              # 基线、负载和恢复的绝对时间边界
    ├── generator.json           # 负载发生器每秒累计状态
    ├── events.jsonl             # 出生、断开、踢出和协议错误
    ├── server.log               # 完整服务端日志
    ├── server_options.json      # 本用例服务端配置
    └── saves/                   # 独立世界
```

`report.html` 可以直接离线打开。曲线按秒汇总，完整统计保留逐 tick 数据的 P50/P95/最大值。
高速度下发生位置纠正、区块下发停滞或内存长期不回落时，优先结合
`serverConfirmedDistance`、区块数量、`interval_ms` 和对应阶段日志判断瓶颈。

发包负载避免高频切换快捷栏：服务端回显选中槽位时，mineflayer 会再次写入槽位；多个尚未确认的
切槽包交错后会形成回声风暴，使负载超出设定速率并阻塞发生器。当前包组合没有这种自动回写行为。
