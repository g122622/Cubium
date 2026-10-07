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

**当前规模**：注册表 51 个用例，其中 Cubium 侧 45 条、vanilla 侧 43 条可运行（2 条 Cubium 专有；
**6 条被服务端缺陷阻塞、显式标记跳过**，见 §2.1）。两侧基线均已冻结，日常回归只跑 Cubium。

**它的实际价值已被反复验证**：首轮 19 个用例发现并定位了 4 个真实缺陷，全部属于
「单测与 GameTest 都看不见」的类型：

| 缺陷 | 性质 |
|---|---|
| `set_health`(cb 102) 从不发送 | IR/codec/协议表/客户端 visitor **四层齐备，服务端零发送点** → 真客户端永远进不了世界 |
| RegistryData 发「有 key 无 value」的条目 | 既非 vanilla 的「客户端已知（空 entries）」也非「未声明（全量 NBT）」，是非法的中间态 |
| `player_info_update`(cb 68) 从不发送 | 同上四层齐备但零发送点 → 客户端 Tab 列表恒为空 |
| 掉落物挡住方块放置 | `ServerWorld::hasEntityCollision` 未按 `isPushable()` 过滤 → 挖掉方块后掉落物立刻挡住原地放置 |

容器/维度/实体三组用例补齐后又发现 4 个（其中 3 个已修、1 个仍阻塞用例）：

| 缺陷 | 性质 | 状态 |
|---|---|---|
| 菜单工厂硬编码 `getOverworld()` | 玩家在下界开容器会取主世界同坐标的方块实体（串台），或建不出菜单 | 已修 |
| 木桶永远打不开 | 菜单工厂只认 `Chest`/`TrappedChest`，`BarrelEntity` 被拒 → 右键无反应且零日志 | 已修 |
| 潜影盒永远打不开 | `getOpenBoundingBox` 的打开判定盒与方块本体重叠 → `canOpen()` 恒为 false | 已修 |
| 丢物品在服务端完全无效 | `AbstractContainerMenu::dropItem` 的回调**从未被设置**，且点击用无 world 的占位 Player 结算 | 已修 |
| 存档重启后实体与地形不碰撞 | 客户端能看到地形，玩家/生物却一路坠入虚空 | **仍阻塞用例** |
| 跨维度传送后下发未生成的区块 | 下界整片 netherrack，客户端看不到真实地形 | **仍阻塞用例** |
| 初次 tick 非主世界维度即崩溃 | `NaturalSpawner::_createDensityManager` ACCESS_VIOLATION | 未修（用例内以 `doMobSpawning=false` 绕开） |

> **教训**：项目的「四层齐备」口径（IR / codec / 协议表登记 / 消费分支）只核对到**消费侧**，
> **不核对发送侧**。判断某个包"已实现"时必须额外 grep 服务端是否有构造/发送点。
> 同理，「回调已就位」不等于「回调被设置」——`dropItem` 就是有接口、有实现、**零设置点**。

第二轮补齐「多人可见性 / 表现层同步」这一簇后（效果、聊天、装备、箱盖动画、离场实体），
又发现 3 个真实缺陷（均属「服务端自己有、别人看不见 / 自己生效、客户端不知道」），
另 2 条经验证是**健全的**（转为回归保护）：

| 缺陷 | 性质 | 状态 |
|---|---|---|
| 状态效果从不下发 | `LivingEntity::addEffect`/`EffectManager` 只改服务端状态，无「效果变更 → 广播」通路；且 clientbound 的 `entity_effect`(cb 130)/`remove_entity_effect`(cb 76)/`update_mob_effect` **三层全缺** → 客户端状态栏无任何效果图标，服务端却在按效果结算 | **仍阻塞用例** |
| 聊天从不广播 | `ChatHandler::handleChatMessagePacket` 只写一行服务端日志，无任何广播通路；clientbound 聊天包（`player_chat`/`system_chat`/`profileless_chat`）三层全缺 → 多人游戏聊天完全不可用且零报错 | **仍阻塞用例** |
| 装备/手持物从不下发 | 服务端**从不发送** `entity_equipment`(cb 100)——该包三层全缺（全仓库 grep `SetEquipment` 零命中）→ 其他玩家看不到你的手持物与护甲 | **仍阻塞用例** |
| 开箱广播 BlockEvent | `ChestEntity::openContainer`/`closeContainer` → `broadcastChestState` → `blockEvent` 链路**完整可用**，旁观者能收到箱盖开合动画包 | 健全（转回归保护） |
| 离场玩家实体移除 | 断连经 `world.destroyEntity` → `EntityTracker` 向追踪者广播 `RemoveEntities`(cb 75)，旁观者世界里的实体被正确移除（与 Tab 条目移除是两条独立通路，须分别覆盖） | 健全（转回归保护） |

> **教训（第二轮）**：**「表现层同步」是最容易长期潜伏的一簇缺陷**——它们不改变世界状态、
> 不影响服务端权威逻辑，因此**不发也不会报错**，只是「箱子开了盖子不动」「别人看不到你拿的剑」。
> 定位这一类缺陷的有效判据是**旁观者视角**（第二个 bot 看到的），而非当事人（当事人有客户端预测，
> 会自我欺骗）。**加用例时优先问「这件事在别人眼里对不对」**。

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
node run.ts --include-skipped          # 连被阻塞而跳过的用例也跑（验证服务端修复时用）
```

**退出码**：`0` 全部通过；`1` 有用例失败；`2` 环境或基线问题（基线缺失/过期、refresh 未加
`--accept`、**「环境问题」类失败**——端口被占/连不上服务端这类与用例语义无关的失败）。

**耗时参考**（本机实测）：Cubium 侧约 75~90 秒/用例（其中服务端启动与区块流送占 70 秒以上），
vanilla 侧约 10~25 秒/用例。整套双跑刷新约 80~100 分钟，单侧 Cubium 回归约 55~65 分钟。
用例体的超时上限是 240 秒（`config.ts` 的 `CASE_TIMEOUT_MS`），因为持久化用例要在体内重启一次
服务端（一次启动就要 70 秒以上）。

> 注：上表是**首次落地时**的旧观测值；后续本机实测 Cubium 单用例已降到 2~7 秒（含
> `protocol/keep_alive_round_trip` 的 18 秒心跳等待），单侧全量回归约 3~4 分钟。

### 2.1 被阻塞的用例（`skipReason`）

用例契约里有 `skipReason` 字段：非 null 时 runner **不运行、不写基线**，但每次运行都会把原因
打印出来（`--include-skipped` 可强制运行）。它只用于「用例已经写好、但被服务端缺陷阻塞、
当前不可能通过」的情况——目的是让阻塞点变成注册表里可见的一条记录，而不是让整套测试长期红灯。
写这个字段时必须写清**具体缺陷 + 证据路径 + 解除条件**。当前被阻塞的 6 条：

| 用例 | 阻塞缺陷 |
|---|---|
| `containers/container_in_nether_is_not_overworld` | 跨维度传送后服务端下发未生成的默认区块（整片 netherrack），客户端看不到下界真实地形 |
| `persistence/block_changes_survive_restart` | 从存档重启后实体与地形不再碰撞，玩家一路坠入虚空 |
| `persistence/container_content_survives_restart` | 同上（bot 无法落地，读不到世界内容） |
| `effects/give_effect_reaches_client` | 施加效果后从不下发 `entity_effect`(cb 130)，客户端状态栏看不到效果（且该包三层全缺） |
| `chat/message_broadcast_to_other_player` | 收到聊天只记日志、不广播，其他玩家收不到聊天（clientbound 聊天包三层全缺） |
| `player-visuals/held_item_syncs_to_others` | 从不下发 `entity_equipment`(cb 100)，其他玩家看不到你的手持物与护甲（该包三层全缺） |

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
│   │   ├── vanilla.ts        # vanilla 适配（共享解包缓存 + 每用例世界名）
│   │   ├── ops.ts            # 需要命令权限的用例：写 ops.json（两侧 UUID 格式不同）
│   │   ├── workspace.ts      # 目录隔离、清理、历史产物轮转
│   │   └── port.ts           # 端口分配与「有没有人在监听」的探测
│   ├── bot/
│   │   ├── factory.ts        # createBot + 原始包记录（PacketTrace）
│   │   ├── wait.ts           # 等待助手（超时带现场描述）
│   │   └── snapshot.ts       # 快照采集与归一化
│   ├── assert/{expect,surface}.ts   # 断言助手；地表基准与 Y 轴归一化
│   ├── baseline/{store,compare}.ts  # 基线读写与结构化比对
│   ├── diagnostics/artifacts.ts     # 失败诊断落盘
│   └── scenarios/
│       ├── shared.ts         # 槽位常量与交互助手（setSlot / 容器同步计数 / runCommand / 物理等待）
│       ├── handshake.ts      # A 握手与连接生命周期
│       ├── chunk-sync.ts     # B 区块同步与世界读取
│       ├── block-interaction.ts # C 方块交互
│       ├── inventory.ts      # D 物品栏与容器往返
│       ├── crafting.ts       # E 合成
│       ├── furnace.ts        # F 熔炉
│       ├── containers.ts     # G 容器种类与维度（木桶 / 潜影盒 / 下界）
│       ├── persistence.ts    # H 存档持久化（重启后核对）
│       ├── entities.ts       # I 掉落物实体闭环
│       ├── movement.ts       # J 玩家移动与移动触发的区块加载
│       ├── protocol.ts       # K 协议存活与容错（keep_alive / 聊天）
│       ├── status-effects.ts # L 状态效果的客户端下发（entity_effect）
│       ├── chat.ts           # M 聊天广播（多人可见性）
│       ├── player-visuals.ts # N 玩家视觉状态同步（手持物/装备）
│       ├── block-events.ts   # O 方块事件广播（箱盖开合动画）
│       ├── player-lifecycle.ts # P 玩家实体在他人世界的生命周期（离场移除）
│       └── index.ts          # 显式注册表
├── baselines/{cubium,vanilla}.json  # 冻结基线（提交进 git）
└── tools/                    # 探针与调试脚本
```

**每个用例一个服务端进程 + 一个独立游戏目录**（用完即删），因此用例之间零世界状态污染。
代价是一次完整启动要 70 秒以上（含世界创建与数据包加载），这让整套用例的耗时以「分钟/条」计——
见 §2 的耗时参考。

**失败时**诊断产物写入 `build/e2e/artifacts/<runId>/<serverKind>/<caseId>/`（**按服务端分目录**：
早期版本两侧共用同一路径，双跑时后跑的会覆盖先跑的，排查 cubium 问题时很容易一直在读 vanilla
的日志），含服务端全量日志、`bot-trace-<n>.jsonl`（**带 payload 的原始包序列**；每个 bot 一份，
序号从 1 起）、快照实际/基线对照、字段 diff、运行元信息。

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
   （注意 `player.onGround` 仍在快照里：它是在 runner 等到「稳定落地」之后才采集的，
   因此是确定量；跨维度/重生场景下这个前提会失效，见 §5.14。）

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
  是**主 bot** 的别名（主 bot 定义了 surfaceY/spawnX/spawnZ 这套地表基准）。用例内追加连接的
  bot 同样计入这两个数组，并由 runner 在 finally 里统一回收。

### 4.6 需要命令权限的用例：`opPlayers`

少数用例要让服务端执行命令（把玩家送进下界、用 `setblock` 造场景、`/save-all` 落盘）。
命令需要 OP，而两侧都**只在启动时**读取 `ops.json`，故 `CaseDefinition.opPlayers` 为 true 时
runner 会在起服务端**之前**把本用例可能连接的 bot 名全部写进 ops.json（`servers/ops.ts`）。

坑：**两侧要求的 UUID 字符串格式不同**——vanilla 要带连字符的标准格式，Cubium 的
`PlayerManager` 存的是 `util::uuidToString()`（32 位小写、无连字符），而 `OpListManager::isOp()`
是精确字符串比较，格式不对就静默不生效（命令被忽略、只回一条聊天消息）。

### 4.7 需要重启服务端的用例：`ctx.restartServer()`

持久化类用例要验证「落盘 → 重启 → 读回」。`ctx.restartServer()` 会断开当前全部 bot、停止
服务端、确认端口释放，再用**同一个游戏目录**（即同一个存档）启动新进程（端口可能变化）。
重启后必须自己 `ctx.connectBot()` 重连；**重启后第一个连接的 bot 成为新的主 bot**，最终快照
采自它（`ctx.bot` / `ctx.trace` 是 getter，会跟随变化）。

两个必须注意的点：

- 用例体内包含一次完整启动，耗时 70 秒以上，因此 `CASE_TIMEOUT_MS` 取 240 秒。
- harness 只能**硬杀** Cubium（Windows 上无法触发其优雅退出），所以存档内容必须先用
  `/save-all`（或其他服务端自身的落盘路径）显式落盘，否则测出来的是「关服方式」而不是存档缺陷。

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

### 5.13 就绪判定只能用「服务端自己打印的日志行 + 端口核对」

早期实现用「TCP 能连」当就绪信号，实测有两个致命问题：

1. **监听 socket 在世界加载完成之前就建立了**（Cubium 在 `initialize()` 内 listen）。此时连上去
   会拿到空注册表并静默挂起；vanilla 同理（先开 listen 再加载世界）。
2. **端口可能被上一个用例尚未退干净的服务端占着**。Windows 的 `SO_REUSEADDR` 语义允许两个
   socket 绑同一端口（与 Linux 相反），所以「bind(0) 分配到的端口」并不保证独占——探测会连到
   **别人的**监听者上，对方随后退出，客户端就收到 `ECONNREFUSED`。实测 35 条用例里有 3 条
   栽在这里（表现为「用例失败」，与环境无关的外观）。

现行做法：两侧都打印明确就绪行（Cubium 是 `Server ready: accepting connections on port <n>`，
vanilla 是 `Done (X.XXXs)! For help`），harness 以它为准，并核对行里的端口号与本次申请的端口
一致（等价于核对监听者身份）；端口分配额外做「主动连接没人应答」的筛除。握手期的
socket 层失败被归类为**环境问题**（退出码 2）并换端口重试，不再伪装成用例缺陷。

### 5.14 跨维度传送后必须先等物理 tick 恢复，否则连包都发不出去

`bot.openContainer` → `activateBlock` → `bot.lookAt(..., false)` 会等一个 `physicsTick`；而
mineflayer 收到 `respawn`（维度切换/重生）后会先关掉物理，**延迟 1500ms 才恢复**，且
`physicsTick` 还要求「玩家自身所在列已有区块数据」。不等这一步的后果很隐蔽：
**客户端连 `use_item_on` 都不会发出去**，服务端侧毫无痕迹，用例只在 15 秒后超时。
`shared.ts` 提供 `waitForPhysicsTick()`；另外 `onGround` 在这种窗口期是**传送前的旧值**，
用它等落地会立刻假通过（该用例改为「脚下方块实心 + onGround + 高度连续多次不变」三重判据）。

### 5.15 vanilla 在「预测与服务端结算一致」时不发增量包

`containerSyncCount`（数 `window_items`/`set_slot`）是判断 Cubium 是否受理点击的好判据，
但对 vanilla **不成立**：实测点击被正常结算、客户端预测也一致时，vanilla 一个增量包都不发。
因此「搬动物品到容器」这类用例的判据要改成服务端权威且两侧都成立的观察——
**关窗后重新打开，断言全量内容**（重新打开必然触发全量下发）。

### 5.16 存档重启后的世界（当前阻塞两条持久化用例）

用已有存档启动服务端后，客户端**能看到地形**（`blockAt` 得到 grass_block/bedrock，81 列区块
正常送达），但玩家从出生点一路坠入虚空（y 持续降到 -400 以下），服务端全程不做位置纠正；
同一时间服务端还在批量刷出又立刻销毁生物（3000+ 次 spawn/destroy）。症状指向「从存档加载的
区块没有进入碰撞/刷怪判定」。复现方式：把任意一次 e2e 失败留下的
`build/e2e/runs/<runId>/<caseId>/` 作为游戏目录直接起服务端，再连一个 bot 观察其 y 即可。

### 5.17 跨维度传送后的区块下发（当前阻塞下界容器用例）

玩家进入下界后，客户端拿到的区块是**未生成的默认内容**：实测在下界 `(0,125,0)` 周围 5×5
（含本该是空气的层）全读到 `netherrack`，且此后用命令改动的方块也不会可靠地出现在客户端
（有 block_change 在区块数据之前到达而被丢弃的迹象）。服务端一侧的同坐标状态是正常的
（`/setblock` 返回 1）。因此「在下界放置/打开容器」的用例当前无法成立。

附带发现：**首次 tick 非主世界维度时服务端会崩溃**——`NaturalSpawner::_createDensityManager`
读取 0x0 触发 ACCESS_VIOLATION（栈：`NaturalSpawner.cpp:965 ← tick:376 ← ServerDimension::tick:187`）。
下界用例目前用 `/gamerule doMobSpawning false` 绕开（`ServerDimension::tick` 用该规则门控自然
刷怪），修好后应移除该绕行。

### 5.18 「表现层同步」须用旁观者视角判据

「服务端自己有、客户端不知道」的一类缺陷（状态效果、聊天广播、装备/手持物、箱盖动画、
离场实体移除）有共同特征：**它们不改变世界状态、不影响服务端权威逻辑，因此不发也不会报错**。
当事人（第一个 bot）身上往往还叠加了客户端预测，会把「服务端没发」伪装成「已经生效」：

- 创造模式改槽后 `bot.heldItem` 已是钻石剑（本地预测），但旁观者看到的仍是空手；
- 开箱者本地会自己播放盖子动画，但旁观者看不到；
- `tossStack` 后本地槽位已清空，但物品是否真的进了世界要等实体出现。

因此这类用例的判据必须落在**另一个玩家**身上：B 侧观察到的东西只可能来自服务端。
`CaseDefinition.botCount = 2`（或 `botCount: 1` + `ctx.connectBot()`）是标准做法，
`chat.ts` / `player-visuals.ts` / `block-events.ts` / `player-lifecycle.ts` 均如此。

数包时优先数**原始包**（`ctx.traces[i].count("block_action")`），而不是 mineflayer 的高层
事件——后者会先解析方块名、只在可解析时才 emit，引入与「包发没发」无关的假失败。

### 5.19 表现层用例的取证：必须验证「用例确实触发到了被测分支」

写一个预期会红的用例时，光看「它红了」不足以断定是服务端缺陷——也可能用例压根没跑到被测
分支（命令被静默拒绝、物品没拿到、目标点超出触及范围）。**取证步骤**：

1. 跑 `--mode=refresh --include-skipped`（不带 `--accept`）让用例真正执行，产物落在
   `build/e2e/artifacts/<runId>/<serverKind>/<caseId>/`；
2. 读 `server.log` 确认「上游动作确实成功了」——例如效果用例要看到
   `system_chat: "Gave Speed to 1 player(s)"`，而不是 `Unknown effect`；
3. 再读 `bot-trace-<n>.jsonl` 确认「下游包计数为 0」（`entity_effect` / `player_chat` /
   `entity_equipment` 等）；
4. 两条合起来才是完整证据链：**服务端确认做了，客户端确认没收到**。

只用「用例超时」当证据，很容易把「用例自己没触发」误判成「服务端缺陷」。

### 5.20 用例内部传送后必须把玩家送回**确定的**落点

`runner` 只在 spawn 阶段等待「稳定落地」（§4.1 的瞬态量约束只覆盖那一段）。用例内部一旦
`/tp`，玩家落点就由地形决定，而**落点不同会让快照里的 `standingOn`/`yRelativeToSurface` 不同**：

`entities/dropped_item_is_picked_up` 把玩家 `/tp` 到掉落物当时的坐标（掉落物有初速度、落点常在
半空），实测不同次运行分别落在「草方块（相对地表 1）」与「地表被挖掉后的泥土（相对地表 0）」，
基线因此抖动——一度把 `onGround=false / standingOn=dirt` 的**中间态**冻进基线，
下一次运行就报「快照与基线不一致」，看起来像服务端回归，实为用例设计缺陷。

**做法**：用例结束前把玩家送回**出生点所在列的地表上方**（`spawnX + 0.5, surfaceY + 1,
spawnZ + 0.5`）并等到「落在出生列 + 脚下是草方块 + onGround」三者同时成立——那是 runner
探测 `surfaceY` 用的同一个参考点，也是全用例唯一的确定性落点。坐标随出生点走，不硬编码。

**排查线索**：若某用例在重复刷新时基线值反复变化（而非稳定报同一个差异），先怀疑落点/
位置类瞬态量，而不是服务端行为。

玩家进入下界后，客户端拿到的区块是**未生成的默认内容**：实测在下界 `(0,125,0)` 周围 5×5
（含本该是空气的层）全读到 `netherrack`，且此后用命令改动的方块也不会可靠地出现在客户端
（有 block_change 在区块数据之前到达而被丢弃的迹象）。服务端一侧的同坐标状态是正常的
（`/setblock` 返回 1）。因此「在下界放置/打开容器」的用例当前无法成立。

附带发现：**首次 tick 非主世界维度时服务端会崩溃**——`NaturalSpawner::_createDensityManager`
读取 0x0 触发 ACCESS_VIOLATION（栈：`NaturalSpawner.cpp:965 ← tick:376 ← ServerDimension::tick:187`）。
下界用例目前用 `/gamerule doMobSpawning false` 绕开（`ServerDimension::tick` 用该规则门控自然
刷怪），修好后应移除该绕行。

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

### 6.5 静默失败要一层层加日志才能定位

容器链路的排查反复卡在「某一层静默 return」上：交互被拒 → 菜单工厂空手返回 → 点击没落到菜单，
每一层都可以什么都不说。补齐 `warn` 之后才看到真正的拒绝点（例如
`BlockActionHandler` 的「等待传送确认」分支、`BlockInteractionManager` 的交互前置校验、
菜单工厂的各失败分支）。**加日志本身就是修复的一部分**——按项目的日志规范，任何偏离正常路径的
分支都必须至少 `warn`；`dropItem` 那种「有接口、有实现、零设置点」的情况只能靠
「异常路径留痕」暴露出来。

### 6.6 数据驱动的 NBT 构造：通用转换 + 一处关键修正

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
| `connect ECONNREFUSED`（握手期） | 端口被上一个用例残留的进程占着 | 已归为「环境问题」并自动换端口重试；反复出现时查是否有残留的 `minecraft-server.exe` |
| 打开容器超时、服务端日志无任何记录 | 客户端因物理未恢复而**没发** `use_item_on`；或服务端在更早的分支静默拒绝 | 看 trace 里有没有 `block_place`；服务端已补 `Rejecting UseItemOn ...` / `Container menu creation failed ...` 告警 |
| 客户端的槽位变来变去但服务端不认 | mineflayer 的 `clickWindow`/`setInventorySlot`/`toss` 都是本地预测 | 判据取服务端回包（`containerSyncCount`）或「关窗后重开看全量内容」 |
| bot 传送/重生后一路下坠 | 该场景下区块或碰撞状态异常（见 §5.16 / §5.17） | 先用 `blockAt` 确认客户端是否看得到地形 |
| `bot.game.minY` 是 0 而非 -64 | `dimension_type` 未送达，客户端退回默认 0/256 | 查 `registry_data` 包内容 |
| `Server refused to place ...` | 目标超出触及范围，或被实体/方块占据 | 检查目标坐标与该位置的实际方块 |
| 基线抖动 | 快照含瞬态量 | 参照 §4.1 的归一化规则 |
| 基线比对报「基线已过期」 | schemaVersion 或协议号变更 | 重新 `--mode=refresh --accept` |
| 用例被打印成「跳过」 | 注册表里标了 `skipReason` | 读打印出的原因（含证据路径与解除条件），修好服务端后改回 `null` |
| 旁观者收不到某个同步包（效果/装备/聊天/箱盖） | 服务端可能压根没发（表现层缺陷）或发给了错误的对象 | 查旁观者 `bot-trace-<n>.jsonl` 的包计数；同时核对服务端日志确认上游动作已执行（见 §5.19） |
