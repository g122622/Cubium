# 端到端 bot 测试（tests/e2e/bot）

以 [mineflayer](https://github.com/PrismarineJS/mineflayer) 作为**真实 wire 客户端**连接 Cubium 服务端，
验证协议兼容性与游戏行为，并以真 vanilla 服务端作为对照基线。

---

## 1. 为什么需要这一层

仓库原有三类测试各覆盖一段，且**只此一段**：

| 测试 | 覆盖范围 | 明确**不**覆盖 |
|---|---|---|
| `tests/unit`（GoogleTest） | 单元逻辑；网络走 `LocalTransport` 零拷贝直传 IR 包对象 | **不经 codec、不经 socket** |
| `tests/integrated`（基岩 GameTest） | 进程内 QuickJS、同步 tick 驱动的行为测试 | **无玩家、不联网** |
| **`tests/e2e/bot`** | **字节 → codec → 服务端业务 → 字节** 全链路 | 渲染、UI |

在 e2e 落地前，「真实字节流」这条路径**没有任何自动化覆盖**，唯一手段是人工用 1.21.11 客户端连测。

**当前规模**：19 个用例，在 Cubium 与官方 vanilla 两侧共 36 条运行**全部通过**，
两侧基线均已冻结。双跑对比是「同一套断言在两个独立实现上都成立」的正面证据。

**它的实际价值已被验证**：首轮 19 个用例就发现并定位了 4 个真实缺陷，全部属于
「单测与 GameTest 都看不见」的类型：

| 缺陷 | 性质 |
|---|---|
| `set_health`(cb 102) 从不发送 | IR/codec/协议表/客户端 visitor **四层齐备，服务端零发送点** → 真客户端永远进不了世界 |
| RegistryData 发「有 key 无 value」的条目 | 既非 vanilla 的「客户端已知（空 entries）」也非「未声明（全量 NBT）」，是非法的中间态 |
| `player_info_update`(cb 68) 从不发送 | 同上四层齐备但零发送点 → 客户端 Tab 列表恒为空 |
| 掉落物挡住方块放置 | `ServerWorld::hasEntityCollision` 未按 `isPushable()` 过滤 → 挖掉方块后掉落物立刻挡住原地放置 |

> **教训**：项目的「四层齐备」口径（IR / codec / 协议表登记 / 消费分支）只核对到**消费侧**，
> **不核对发送侧**。判断某个包"已实现"时必须额外 grep 服务端是否有构造/发送点。

---

## 2. 运行方式

```bash
cd tests/e2e/bot
npm install                 # 首次

node run.ts                 # 回归比对（默认：只跑 Cubium，与冻结基线比）
node run.ts --mode=refresh --accept    # 刷新基线（仅在全部用例通过时写入）
node run.ts --mode=diff     # 双跑对比（Cubium vs vanilla）
node run.ts --case=handshake/spawn      # 只跑 id 含该子串的用例
node run.ts --keep-artifacts           # 保留成功用例的工作目录
node run.ts --with-vanilla             # 把 vanilla 也纳入本次运行范围
```

**退出码**：`0` 全部通过；`1` 有用例失败；`2` 环境或基线问题（如基线缺失/过期、refresh 未加 `--accept`）。

### 三种模式共用同一套用例

| 模式 | 起哪些服务端 | 行为 |
|---|---|---|
| `regress`（默认） | 仅 Cubium | 与 `baselines/cubium.json` 逐字段比对，不一致即失败 |
| `refresh` | 按 `--with-vanilla` | 写回基线，**需 `--accept` 且全部用例通过** |
| `diff` | 两侧都跑 | 对撞产出报告，不写基线 |

用例代码**只有一份**，模式只决定「和谁比」。用例内不做 `if (server === 'cubium')` 分支——
跨服务端差异全部由归一化吸收（见 §4）。确实只能跑 Cubium 的用例在注册表里标 `servers: ['cubium']`。

### 前提

- Node ≥ 22（本仓库脚本使用 Node 原生 type stripping 直接跑 `.ts`，**运行时零外部依赖**）
- 构建产物 `build/bin/RelWithDebInfo/minecraft-server.exe`
- 双跑对比需要 Java 21+ 与官方服务端 jar，路径见 `src/config.ts` 的 `vanillaServerJar()`
  （默认指向本机 gradle 缓存里的 1.21.11 bundler jar，可用 `MC_VANILLA_JAR` 覆盖），**无需联网**

---

## 3. 目录结构

```
tests/e2e/bot/
├── run.ts                    # 唯一入口 CLI
├── src/
│   ├── config.ts             # 常量与路径推导（不随用例变化）
│   ├── case.ts               # 用例契约（CaseDefinition / CaseContext）
│   ├── runner.ts             # 编排：起服务端 → 连 bot → 等等待 → 跑用例 → 采快照 → 比对基线
│   ├── servers/
│   │   ├── server-process.ts # 进程抽象（就绪策略 / 停止策略 / 日志 / 崩溃监听）
│   │   ├── cubium.ts         # Cubium 适配（临时游戏目录 + --config）
│   │   ├── workspace.ts      # 目录隔离、清理、历史产物轮转
│   │   └── port.ts           # 空闲端口分配
│   ├── bot/
│   │   ├── factory.ts        # createBot + 原始包记录（PacketTrace）
│   │   ├── wait.ts           # 等待助手（超时带现场描述）
│   │   └── snapshot.ts       # 快照采集与归一化
│   ├── assert/{expect,surface}.ts   # 断言助手；地表基准与 Y 轴归一化
│   ├── baseline/{store,compare}.ts  # 基线读写与结构化比对
│   ├── diagnostics/artifacts.ts     # 失败诊断落盘
│   └── scenarios/{handshake,chunk-sync,block-interaction}.ts + index.ts
├── baselines/{cubium,vanilla}.json  # 冻结基线（提交进 git）
└── tools/                    # 探针与调试脚本
```

**每个用例一个服务端进程 + 一个独立游戏目录**（用完即删）。实测 Cubium 启动仅约 1.3 秒，
完全负担得起；换来的是用例之间零世界状态污染。

**失败时**诊断产物写入 `build/e2e/artifacts/<runId>/<caseId>/`，含服务端全量日志、
`bot-trace-<n>.jsonl`（**带 payload 的原始包序列**；每个 bot 一份，序号从 1 起）、
快照实际/基线对照、字段 diff、运行元信息。

---

## 4. 写用例的约定

### 4.1 归一化：跨服务端可比的唯一前提

`src/bot/snapshot.ts` 强制以下规则，写用例时必须遵守：

1. **Y 一律相对地表**。Cubium 的超平坦层从 `Y=0` 起（地表 Y=3），vanilla 从维度 `minY=-64` 起
   （地表 Y=-61），**两侧绝对 Y 差 64 格**。所有跨端断言必须用 `surfaceY` 换算。
   `src/assert/surface.ts` 是这一差异的**唯一收敛点**——探测式取地表，不硬编码 3/-61，
   这样将来 Cubium 修好 `FlatChunkGenerator::getMinY` 也能自动跟随。
2. **绝对 Y 只允许出现在 `informational.surfaceY` 这一个字段**，且该字段**不参与比对**。
3. **实体 id / UUID / 用户名 / 时间戳 / 耗时**一律不入快照。
4. **瞬态量不入快照**。典型例子：`onGround`——spawn 瞬间 bot 可能仍在下落的某一帧。
   runner 已统一等待稳定落地，新增类似瞬态字段前请先确认其确定性。

### 4.2 快照锚点要选「id」而非「名字」

`bot.blockAt().biome` 是 prismarine-block 新建的**占位 Biome 对象**：prismarine-chunk 的
`Block.fromStateId(stateId, biomeId)` 只带 id、不注入数据，故其 `name`/`color`/`temperature`
恒为默认值（空串 / 0）。**名字必须经 registry 反查**（`biomeNameOfId`）。
因此钉死 biome registry 顺序的锚点是 **id**（plains 恒为 40），不是 name。

### 4.3 交互类用例的坐标

目标点必须落在 bot 的**触及范围（约 4.5 格）内**。超出时服务端会拒绝，
用例会以「与协议无关的原因」失败（表现为 `Server refused to place ...`）。
建议所有交互点限制在出生点 ±2 格内。

### 4.4 时序：不要假设「spawn 后世界就绪」

区块是**逐 tick 推送**的。runner 会在跑用例前依次等待：
① 出生列可读 → ② 出生点周围 7×7 区域加载完成 → ③ bot 稳定落地。
新增用例若依赖更大范围或更晚的状态，请自行在用例内补充等待，而不是加长固定延时。

### 4.5 多 bot 用例：共享世界，且可控制连接时序

`CaseDefinition.botCount` 声明 runner **预先连接并等待就绪**的 bot 数量（必填，不设默认值）；
需要控制连接时序的用例把它设小，在 `run` 里调用 `ctx.connectBot()` 追加连接——典型场景是
「先让已连接的 bot 改变世界状态（如制造一个实体），再让新 bot 加入观测它」。例如
`handshake/login_entity_id_is_entity` 先丢一个掉落物让实体 id 序列错开，再连第二个 bot。

两个约束：

- **共享物理世界**：同一服务端的多个 bot 会互相推挤，也可能改动同一批方块。用例须让各 bot
  在空间上错开，且不得依赖「自己是世界里唯一的行动者」。
- **数组与连接顺序对应**：`ctx.bots` / `ctx.traces` 按连接顺序排列，`ctx.bot` / `ctx.trace`
  是其中 `[0]` 的别名（主 bot 定义了 surfaceY/spawnX/spawnZ 这套地表基准）。用例内追加连接的
  bot 同样计入这两个数组，并由 runner 在 finally 里统一回收。

---

## 5. 容易踩的坑

> 本节是本文档最值得优先阅读的部分。

### 5.1 Prismarine 别名陷阱（最高频）

Prismarine 生态沿用 1.16 时代的包名，与 1.21.11 官方名**不同**。写包名断言时必须用对：

| Cubium / Mojang 官方名 | Prismarine / nmp 名 | 说明 |
|---|---|---|
| `use_item_on` (sb 63) | `block_place` | 放置方块 |
| `set_health` (cb 102) | `update_health` | **mineflayer 的 spawn 事件完全依赖它** |
| `block_changed_ack` (cb 4) | `acknowledge_player_digging` | 方块预测 ack |
| `teleport_entity` (cb 123) | `entity_teleport` | 实体传送 |
| `level_chunk_with_light` (cb 44) | `map_chunk` | 区块数据 |
| `forget_level_chunk` | `unload_chunk` | 区块卸载 |
| `section_blocks_update` | `multi_block_change` | 多方块更新 |

项目中 `docs/未实现的数据包.md` 第 104 行也记录了这批别名。
**判断某包"没发出去"之前，先确认你在查的名字对**。

### 5.2 mineflayer 的 spawn 由 `update_health` 决定

`spawn` 事件 = 收到**首个 `health > 0`** 的 `update_health`（`lib/plugins/health.js:18`）。
服务端不发这个包时，客户端会停在「已登录但未进入世界」，**且不报错**。
排查时应先看 `bot-trace-<n>.jsonl` 里该包的计数是否为零。

### 5.3 nmp 恒定回空 `select_known_packs`

`minecraft-protocol/src/client/play.js:74` 固定回 `{packs: []}`。按 vanilla
`RegistrySynchronization.packRegistry`（:37-72）的**逐条目**判定语义，客户端未声明 known pack 时
服务端**必须下发完整 NBT**。因此不存在「因为客户端声明了 core 所以只发 id」这条捷径——
对 mineflayer 而言全量 NBT 是唯一路径。

**附带后果**：若服务端发了「有 key 但 value 缺失」的条目，`prismarine-registry/lib/pc/index.js:71`
的 `nbt.simplify(e.value)` 会抛 `TypeError`。注意该行是 `.map()`，在 `?.()` **之前**执行——
**只要有任意一个 entry 的 value 缺失就崩溃，与该注册表有没有 handler 无关**。
崩溃表现为**静默挂起**：nmp 把异常转成 `error` 事件，而 mineflayer 收到 error 后**不关闭连接**。

### 5.4 `bot.blockAt` 需要 Vec3 实例

传普通 `{x, y, z}` 对象会抛 `pos.floored is not a function`（mineflayer 内部调用了 Vec3 的方法）。
统一用 `src/assert/surface.ts` 导出的 `vec3()` / `blockNameAt()`。

### 5.5 `block.name` 不带命名空间前缀

`bot.blockAt(...).name` 返回 `"grass_block"` 而非 `"minecraft:grass_block"`。
写断言时勿加前缀（本仓库曾因期望值多写前缀而误报失败）。

### 5.6 protodef 的 "Chunk size is N" 是噪声，不是错误

`protodef/src/serializer.js:76` 在「一次 TCP 读取包含多个包」（粘包）时打印
`Chunk size is 63 but only 29 was read`。nmp 会自行分流剩余字节，**不影响解析**。
该消息由 `hideErrors` 选项控制；本项目保持 `hideErrors: false`（错误可见性优先），
由 `runner.ts` 的 `installNoiseFilter()` 只过滤这一条已知噪声。

### 5.7 nmp 的发包不能靠事件监听

`client.write(name, params)` **不发射任何事件**（`minecraft-protocol/src/client.js:240`）。
要记录上行包必须包装方法本身——见 `src/bot/factory.ts`。

### 5.8 服务端世界配置的两个陷阱

- **`world.levelType` 此前是死配置**：`StandaloneServer` 曾把世界预设硬编码为 `minecraft:normal`，
  无论配成 `flat` 还是 `amplified` 主世界恒为噪声地形。现已按 `levelType` 映射预设 id。
- **独立服此前无法新建世界**：世界目录不存在就直接报 `World not found`。现已在启动时按配置
  自动创建（`LevelDatCodec::writeInitial`），与 vanilla 行为一致。

另外 `FlatLevelGeneratorSettings` 的 `structure_overrides` 语义与 vanilla 有偏差：
vanilla 的 `Optional.empty` = 使用全部结构集，Cubium **空列表 = 完全不生成结构**。
双跑对比时须让 vanilla 侧显式传 `structure_overrides: []`，否则两侧地形不一致。

### 5.9 用户名不得超过 16 字符，且多 bot 必须互不同名

vanilla 的 `ServerboundHelloPacket` 用 `readUtf(16)` 读取用户名，超长会**直接断连**并报：

```
Failed to decode packet 'serverbound/minecraft:hello'
```

该错误信息**完全指不到用户名上**，极易误判为协议定义不匹配。Cubium 侧不校验该长度，
所以这类问题**只在双跑对比时才会暴露**。`runner.ts` 的 `botUsername()` 已强制截断用户名
主体（需为 `_<序号>` 后缀让出长度），由用例 id 生成用户名时务必注意总长（含前缀）。

同一用例的多个 bot 还必须**互不同名**：vanilla 检测到重名会踢掉**先前**登录的那个连接，
双跑对比会直接崩。用户名统一追加序号正是为此。

### 5.10 vanilla 侧必须显式禁用结构生成

两侧地形必须逐项对齐才能比对区块内容。Cubium 侧 `generateStructures=false` 对应
「`structure_overrides` 空列表 = 不生成结构」，而 vanilla 的 `minecraft:flat` 预设
**默认生成村庄与要塞**。因此 vanilla 侧必须在 `generator-settings` 里显式传
`structure_overrides: []`，否则区块比对必然失败。

### 5.11 refresh 模式是合并写入

基线更新按用例 id 合并，未跑到的用例保留既有条目。因此可以放心用 `--case=` 过滤
刷新单条用例，不必担心把整份基线削掉。

### 5.12 修改 C++ 后必须重新编译

e2e 跑的是 `build/bin/RelWithDebInfo/minecraft-server.exe`，改完服务端代码**必须重新构建**
再跑测试，否则测的是旧二进制。

---

## 6. 经验沉淀

### 6.1 「已实现」的判定必须双端核对

一个包「已实现」需要**两侧**都成立：客户端能收/发，**且服务端确实有发送/处理点**。
本仓库的四层口径只覆盖前者。新增包时建议同时 grep 发送点，并在 `docs/未实现的数据包.md` 中标注。

### 6.2 跨实现对比要把「必然差异」显式归一化

两个实现总有些结构上必然不同的量（如两侧地形基准 Y 差 64 格）。
把它**收敛到单一模块并显式豁免**，好过在各处打补丁——否则测试会因假失败而失去信号价值。

### 6.3 失败要带现场，否则等于没有测试

`PacketTrace` 记录**带 payload** 的收发包序列（不是只有包名和大小）。
本次一个典型例子：定位「放置失败」时，正是 `block_change` 的 payload
（目标位置仍是 `type:0` 空气）证明了服务端确实没放置成功，从而把排查方向从客户端转向服务端。
超时异常也应携带「已观察到的状态」（`wait.ts` 的 `describe` 回调）。

### 6.4 用例设计缺陷与真实缺陷要分清

首轮调试中出现的多数失败其实是**用例自身**的问题，而非服务端缺陷：

- 目标点超出触及范围 → 服务端拒绝放置
- 未等待区块加载完成 → 大量"未加载"假失败
- 查错了 Prismarine 别名 → 断言"包没发出"
- 瞬态量（`onGround`）入快照 → 基线抖动

**判据**：先确认服务端日志里有无对应记录（如 hitPos 拒绝会打 `Rejecting UseItemOn`），
再决定是改用例还是改服务端。真正的服务端缺陷通常在日志里是「静默的」——
没有报错，但行为不对。

### 6.5 数据驱动的 NBT 构造：通用转换 + 一处关键修正

为未声明 known pack 的客户端下发注册表 NBT 时，不必为 23 个注册表逐个手写编码器——
数据包里本就有完整定义（`data/minecraft/<registry_dir>/*.json`），走通用 JSON→NBT 转换即可，
当前覆盖 23 个注册表、328 个条目。

**唯一必须修正的是整数窄化**：通用 `jsonToNbt` 会按数值范围把 `5` 推断为 `byte_tag`、
`6000` 推断为 `short_tag`，而 Java 侧注册表 codec 的整数字段用 `Codec.INT` 解码，
**会拒绝这两者**（`EnchantmentNbtBuilder.cpp` 记录了同一问题的早期案例）。
解法是注册表专用的转换函数，整数一律落 `int_tag`。浮点则无需精确匹配——
Java 的 `FloatCodec`/`DoubleCodec` 接受任意数值 tag。

**条目顺序不可变更**：客户端按 `RegistryData` 的收到顺序自增分配 registry id，
而 `UpdateTags` 的 elementId 正是该索引。因此新实现以既有硬编码列表的顺序为基准，
只做「逐条目补齐 NBT」这一件事，而非遍历目录（文件系统顺序不确定）。

**构建失败时跳过条目而非回退**：若某条目 NBT 构建失败，发「有 key 无 value」会让
客户端解析时抛异常；跳过则该注册表不完整但不崩，且日志会报出覆盖率缺口。

### 6.6 基线只应记录全部通过的运行

`refresh` 模式在有失败用例时**拒绝写入**，且需要显式 `--accept`。
这条约束避免了把「碰巧跑出来的结果」冻结成基线，也避免了错误行为被固化。

---

## 7. 故障排查

| 现象 | 可能原因 | 排查方向 |
|---|---|---|
| bot 连上但永不 spawn | 服务端未发 `set_health` | 查 `bot-trace-<n>.jsonl` 中 `update_health` 计数 |
| 连接静默挂起、无报错 | registry 解析抛错被转成 error 事件 | 查服务端日志与 trace 中最后收到的包 |
| 用例超时但服务端正常 | 等待条件永不满足 | 看超时异常附带的「已观察到的状态」 |
| `bot.game.minY` 是 0 而非 -64 | `dimension_type` 未送达，客户端退回默认 0/256 | 查 `registry_data` 包内容 |
| `Server refused to place ...` | 目标超出触及范围，或被实体/方块占据 | 检查目标坐标与该位置的实际方块 |
| 基线抖动 | 快照含瞬态量 | 参照 §4.1 的归一化规则 |
| 基线比对报「基线已过期」 | schemaVersion 或协议号变更 | 重新 `--mode=refresh --accept` |
