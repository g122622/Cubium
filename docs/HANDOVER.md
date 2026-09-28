# 交接文档：容器/物品栏链路修复与 e2e 用例补充

> 本文档面向接管本工作的开发者，交代任务背景、已完成内容、当前卡住的问题与后续全部待办。
> 撰写时的分支：`main`。
>
> **2026-09-28 更新**：本轮对 e2e 体系做了一次系统性加固（就绪判定、失败分类、诊断产物隔离、
> 新增 5 组共 11 条用例），并顺带修掉了几个服务端缺陷。本文件已按当前状态改写：
> **第三章的「当前卡住的问题」已解决**（cubium 基线在 `86b2c57fe` 刷新，相关用例现已通过）；
> **第四章 A 的待办已完成**；**第四章 B 中「菜单工厂硬编码主世界」与「木桶/潜影盒打不开」已修**；
> 另新增第三章之二记录本轮新发现、且**仍阻塞 3 条用例**的服务端缺陷。详见第八节。

---

## 一、任务背景

### 起因

上游需求分两步：

1. 参照 `E:\dev\MC\mineflayer` 自带的测试用例，为本项目的 e2e（`tests/e2e/bot`）补充 3 条：
   `displayName`、`heldItemChanged`、`playerInventory`。
2. 另外「多增加一些物品栏、合成、熔炉等的测试用例」。

### 调研结论：不改服务端则用例全部跑不起来

e2e 用的是**真实 bot 连独立服务端（TCP）**。调研发现项目的容器系统基本只在「同进程 integrated 客户端」这条路径上走通过，远程路径断在多个环节：

| 问题 | 位置 | 后果 |
|---|---|---|
| `OpenScreen.menuType` 直接 `static_cast<u8>(ContainerType)`，枚举缺 vanilla 的 `crafter_3x3`(7) | `ContainerTypeUtils.cpp` | 从 Anvil 起全部错位 -1。原版客户端会把工作台建成酿造台。项目自研客户端因收发共用同一枚举而自洽，只在第三方客户端暴露 |
| `containerId` 从 0 起分配 | `ContainerManager.cpp` | 与固定的 `PLAYER_CONTAINER_ID=0` 撞号，背包屏点击会被路由到已打开的容器 |
| 远程玩家没有「背包菜单」 | `MinecraftServer::handleOpenPlayerInventoryPacket` 曾是空实现 | 所有 `containerId=0` 的点击被静默拒绝，背包 UI 完全不可交互 |
| 打开容器后不发初始内容 | `StandaloneServer` | `OpenScreen` 只建空壳窗口，mineflayer 的 `openContainer` 会挂死 |
| `ContainerSetContent` 的 carried 恒为空 | 同上 | 客户端光标被清空，与服务端分叉，之后再点会用空光标覆盖服务端导致物品真丢 |
| 远程容器菜单从不被 tick | `ContainerManager` 无 data 回调 | 熔炉的火焰与箭头进度恒为 0 |
| **物品栏有两份互不感知的数据** | `InventoryManager::m_inventories` vs `Player::m_inventory` | 拾取走实体层、点击同步走副本，互相覆盖（拾取到的物品被下次同步抹掉） |

### 用户确认的范围与节奏

- **修复范围**：修到「统一物品栏权威源」（即包含上述最后一条的架构重构）。
- **提交节奏**：按子系统分阶段提交。

---

## 二、已完成的提交

```
00d2c166a fix(container): 修正菜单类型编码错位与容器 id 撞号
68fdc337a refactor(inventory): 统一物品栏权威源到玩家实体
5f4c81254 fix(server): 属性包改到 login 包之后发送，并补齐容器打开时的内容下发
c08711c90 feat(server,block): 远程容器进度同步与熔炉点燃状态写回
```

### 各提交要点

- **00d2c166a**：`ContainerType` 枚举重排为「数值即菜单注册表项序」（补入 `Crafter=7`，其后全部顺延），出站转换与客户端反解同时修正；容器 id 改从 1 起分配。新增 `tests/unit/common/entity/inventory/ContainerTypeUtilsTest.cpp` 锁定全部 wire 取值。
- **68fdc337a**：移除 `InventoryManager` 的物品栏副本，改为「按 playerId 定位到实体上那一份」的转发器，解析器由服务器注入。连带修 `setItem` 拒绝 slot>=36（护甲/副手无法同步）、`PlayerInventory` 未绑定 Player、`resolvePlayerInventory` 简化。多个依赖副本语义的单元测试改为注入解析器。
- **5f4c81254**：**修复真实客户端断连**（用户报告）——属性包此前发在 `login` 包之前，客户端此时 `ClientLevel` 还是 null，处理时 NPE 断连。同时补齐容器打开后的内容下发与 carried。
- **c08711c90**：`ContainerManager::tickMenus()` + `setOnContainerData` 接通远程路径的 `container_set_data`；修 `AbstractFurnaceEntity::updateBurnState` 空实现（LIT 永不翻转）。

---

## 三、原「当前卡住的问题」（**已解决**，保留供追溯）

> **状态**：已解决。`inventory/player_inventory_click_applies` 现已通过，cubium 基线于
> `86b2c57fe`（`fix(container,crafting): 修正 open_screen 标题编码、工作台槽位布局与背包合成格写入`）
> 刷新。以下为当时的记录，其中「已确认的事实」仍然是对该链路的准确描述，排查方向已不再需要。

### 现象

新增的 e2e 用例 `inventory/player_inventory_click_applies` 失败：

```
等待 物品经背包屏点击移动到主背包 超时
已观察到的状态：槽位 9 当前为 null
```

### 已确认的事实

1. **服务端确实受理了点击**：C2S 发了 2 次 `window_click`（`windowId:0, slot:36` 然后 `slot:9`），服务端日志无任何拒绝记录，`handleClick` 成功返回，S2C 回了 **6 条 `window_items`**。
2. **但回包的槽位内容错位**：解析 trace 发现物品出现在 **`items[0]`**（合成结果槽，本应恒空），而不是期望的 `items[9]`（主背包首槽）或 `items[36]`（快捷栏首槽）。
3. **映射函数本身是对的**：`InventorySlotMapping::menuSlotToPlayerInvId(0)` 返回 -1；`buildMenuContent()`（`InventorySlotMapping.cpp:31`）按菜单槽 0..45 顺序循环、对 -1 填空，逻辑正确。
4. `handleSetCreativeModeSlotPacket` 的 `slotNum → mappedSlot` 映射也正确（36 → 索引 0）。

### 因此错位发生在尚未定位的位置

**下一步的排查方向**（按可能性排序）：

1. **`ContainerSetContent` 的 items 是否真的由 `buildMenuContent` 构造**。已知有多处「绕过 `InventoryManager` 自建全量下发」的独立路径（`ItemPickupManager::_sendInventoryUpdate`、`GiveCommand`/`ClearCommand`/`LootCommand`/`ReplaceItemCommand` 各自的 `syncInventoryToClient` 局部函数）——**逐处核对这些路径用的是哪套映射**。这是最可疑的方向。
2. `toItemStackView()` 对空栈的表示是否会让数组项数与槽位错位。
3. 客户端（mineflayer）对 `window_items` 的解析是否会做槽位重排（可能性低，但可用一个「已知内容」的包比对验证）。

**调试手法建议**：在 `sendContainerContent`/`buildMenuContent` 出口打印构造出的 `items` 数组的非空槽下标（一次性日志，定位后删除）。注意 C++ 侧重编译约 30 秒（增量）。

> 耗时说明：本文档早期写的「e2e 单用例约 3 秒」在当前机器上**不再成立**——一次完整服务端启动
> （世界创建 + 数据包加载）要 70 秒以上，因此 Cubium 侧实测约 75~90 秒/用例，vanilla 侧约
> 10~25 秒/用例。改用例时请以此为准（见 `docs/test/E2E_BOT_TEST.md` 的耗时参考）。

---

## 四、后续全部待办

### A. e2e 用例（**已完成**）

| # | 任务 | 状态 |
|---|---|---|
| 1 | 修完上述错位缺陷后刷新基线 | ✅ 已于 `86b2c57fe` 完成，其后又多次整体刷新（含 vanilla 侧补齐） |
| 2 | 合成用例（工作台 3x3 + shift-click） | ✅ `crafting.ts` 4 条：空网格不产出、3x3 经光标合成、shift+点击、背包 2x2 |
| 3 | 熔炉用例（放置、打开、烧炼产出、进度同步、LIT 翻转） | ✅ `furnace.ts` 2 条：打开收到内容、烧炼产出 + `container_set_data` + LIT 翻转 |

本轮又新增 5 组共 11 条：`containers.ts`（木桶/潜影盒/下界）、`persistence.ts`（重启后核对）、
`entities.ts`（丢弃与拾取闭环）、`movement.ts`（行走与移动触发的区块加载）、`protocol.ts`
（keep_alive 往返、聊天后连接健康）。当前注册表 46 条，其中 43 条可运行、3 条被服务端缺陷阻塞
（见第八节）。

### B. 服务端缺陷（子代理审查结果）

**高风险**：

1. ✅ **已修**：菜单工厂硬编码主世界取方块实体 —— `StandaloneServer::_setupContainerCallbacks`
   现改用 `m_dimensionManager->getPlayerDimensionWorld(playerId)`，与关闭回调同一口径。
   （对应的下界 e2e 用例仍被「跨维度区块下发」缺陷阻塞，见第八节。）
2. ✅ **已修**：木桶/潜影盒打不开 —— 工厂的 `Generic9x3`/`ShulkerBox` 分支现接受
   `BlockEntityType::Barrel` / `ShulkerBox`；另外 `ShulkerBoxBlock::getOpenBoundingBox` 的
   打开判定盒与方块本体重叠（`canOpen()` 恒为 false）也已修正。两条用例
   （`containers/barrel_*`、`containers/shulker_box_*`）在两侧均通过。
3. 🔶 **部分改善**：菜单工厂的失败分支现已全部 `warn`（无维度世界 / 无方块实体 / 实体类型不符），
   `BlockActionHandler` 的交互拒绝、`BlockInteractionManager` 的交互前置校验、`TeleportManager`
   的确认分支也都补了 `warn`。`ContainerManager::openContainer` 的无日志 Error 与调用方丢弃
   Error 仍在。
4. ⬜ 未修：`IntegratedServer` 的本地客户端容器点击/关闭静默分支（本地客户端已停止维护）。
5. ✅ **已修**：`BlockActionHandler` 把「玩家实体查不到」与「距离过远」揉进同一个无日志分支 ——
   交互拒绝现在打印具体原因（`player entity not found ...` 与 `out of interaction range` 分开）。
6. ⬜ 未修：`MovementHandler` 中 `serverPlayer == nullptr` 时反飞行校验被静默跳过。

**中风险**：`EntityActionHandler`/`MovementHandler`/`PlayerStateHandler` 多处「实体查不到」的静默返回；`ServerPlayHandler` 的兜底分支日志不带变体名。

**本轮新增发现（详见第八节）**：丢物品在服务端完全无效（✅ 已修）；存档重启后实体不与地形碰撞
（⬜ 仍阻塞用例）；跨维度传送后下发未生成的区块（⬜ 仍阻塞用例）；初次 tick 非主世界维度崩溃（⬜ 未修）。

### C. 遗留 TODO（代码中已标 `TODO`）

- 客户端属性消费点（移动速度预测、挖掘速度、交互距离仍读本地硬编码默认值）。
- 补齐 12 条缺失的 vanilla 属性（尤其 `block_break_speed`；补齐后 e2e 方块交互用例应改回 survival 模式）。
- `update_attributes` 的增量脏刷新（当前只做 spawn/join 时的全量下发）。
- `NbtHelper.cpp` 的属性 id 序列化缺陷；`AttributeCommand` 的 Identifier 字符集校验；属性名迁移（`generic.*` → `minecraft:*`）。
- `ContainerManager` 中的**占位 Player**：`handleClick` 现已在能解析到真实实体时传入真实实体（丢弃
  物品必须如此），占位 Player 仅作为「解析不到实体时」的兜底并会打告警；`closeContainer` 仍在用
  占位 Player，仍应重构掉。

---

## 五、调试本链路时的陷阱（血泪教训）

### 1. mineflayer 会**本地预测**，用例极易假通过

`creative.setInventorySlot` 与 `clickWindow` 都会**立刻**写本地镜像、**不等服务端回包**（见 mineflayer 的 `creative.js`、prismarine-windows `Window#acceptClick`）。

**后果**：断言「本地槽位变成某物品」这类写法，在服务端把该包整个丢掉时照样通过。

**正确做法**（`tests/e2e/bot/src/scenarios/inventory.ts` 的 `containerSyncCount`）：判据取**服务端回包**（`window_items` / `set_slot` / `container_set_slot`）——本地预测伪造不出来。

**反例（不要用）**：曾试过「抹掉本地预测值再等服务端填回」，对 Cubium 有效但**对 vanilla 无效**——vanilla 只回**增量** `set_slot`，不会重发全量；且把 `slots[i]` 直接置 null 会让 mineflayer 内部读 `itemId` 时崩溃。

`bot.dig` / `bot.placeBlock` / `bot.openContainer` 则**不预测**，等真实回包，可以安全断言。

### 2. **诊断产物目录两侧同名会互相覆盖**（✅ 已修）

e2e 失败时落盘的产物目录在 cubium 与 vanilla 两侧**曾使用同一路径** `build/e2e/artifacts/<runId>/<caseId>/`，
后跑的会覆盖先跑的。（本文档撰写过程中就踩过，浪费了大量时间。）

**现状**：路径已改为 `build/e2e/artifacts/<runId>/<serverKind>/<caseId>/`，双跑时两侧产物各自独立。

### 3. 服务端入站有**一次 tick 延迟**

包走「接收线程 `enqueueInbound` → 主线程 `drainInbound`」派发。客户端动作后立刻断言服务端效果会读到旧值——不是服务端慢，是设计如此（跨线程安全）。**用例必须显式等待。**

### 4. 新增包若「被静默丢弃」，多半是**未登记**而非 codec 错

`IdDispatchCodec::decode` 对未登记的 id 返回 `ProtocolError`，调用方可能静默跳过。排查时先确认该 id 是否登记在**服务端接收表**（`JavaProtocolTables.cpp` 的 `PacketFlow::Serverbound`），再看 codec。

vanilla 的 serverbound 注册在 `GameProtocols.java` 中**按字母序**，id 即注册顺序（`container_click` = 17）。

### 5. 客户端崩溃日志的混淆名可反查

`java.lang.NullPointerException: Cannot invoke "hif.a(int)" because "this.B" is null` 这类信息，结合 `Incoming Packet: clientbound/minecraft:xxx` 判断是哪个包，再用参考源码（`D:\Minecraft\MC研究\Minecraft1.21.11源码`）对照字段与处理时序即可定位。**属性包那次就是这么查出「发在 login 包之前」的。**

---

## 六、常用命令

```bash
# 构建（仅主会话可执行，子代理禁止编译）
./scripts/configure.sh build

# 格式化（提交前必做，需 node24）
nvm use 24 && node scripts/format/clang_format_all.ts

# e2e：只跑 cubium（排查单侧问题用这个）
cd tests/e2e/bot && node run.ts --mode=regress
cd tests/e2e/bot && node run.ts --mode=regress --case=<id 子串>

# e2e：双跑对比（不写基线）
cd tests/e2e/bot && node run.ts --mode=diff

# e2e：刷新基线（仅当全部用例通过）
cd tests/e2e/bot && node run.ts --mode=refresh --accept

# e2e：两侧一起刷新（cubium + vanilla，约 80~100 分钟）
cd tests/e2e/bot && node run.ts --mode=refresh --accept --with-vanilla

# e2e：连被阻塞而跳过的用例也跑（验证服务端修复时用）
cd tests/e2e/bot && node run.ts --mode=regress --include-skipped --case=<id 子串>

# 单元测试（勿跑全量，超时；用过滤器）
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="*Container*"
```

**退出码**：`0` 全部通过；`1` 有用例失败；`2` 环境或基线问题。注意**「环境问题」也走 2**：
端口被残留进程占用、握手期连不上服务端这类与用例语义无关的失败会被自动换端口重试一次，
仍失败才计入结果——排查时先看是否有残留的 `minecraft-server.exe`。

**已知的 3 条既有失败单元测试**（与本链路无关，用户已确认可忽略）：
`OnChangedBlockChainTest.StopLocationBasedEffectsClearsModifier`、
`SpongeBlockDropTest.DropResourcesGeneratesEntitiesWhenLootTableExists`、
`GameTestRegistrationFixture.ClearAllTestMethodsEmptiesRegistry`。

---

## 七、参考路径

| 用途 | 路径 |
|---|---|
| vanilla 1.21.11 源码 | `D:\Minecraft\MC研究\Minecraft1.21.11源码` |
| mineflayer 源码与自带测试 | `E:\dev\MC\mineflayer` |
| e2e 用例 | `tests/e2e/bot/src/scenarios/` |
| e2e 框架说明 | `docs/test/E2E_BOT_TEST.md`（含坑清单与故障排查表） |

---

## 八、本轮（2026-09-28）e2e 体系加固与新发现

### 8.1 harness 侧改动

| 改动 | 原因 |
|---|---|
| 就绪判定改为「服务端日志行 + 端口核对」 | TCP 可连不等于世界已加载；且端口可能被上一个用例残留进程占着（Windows `SO_REUSEADDR` 允许重绑），实测 3/35 条用例因此假失败 |
| 端口分配增加「主动连接没人应答」筛除；停止流程校验进程真的退出 | 同上 |
| 握手期 socket 失败归为「环境问题」（退出码 2）并换端口重试一次 | 环境抖动不再伪装成用例缺陷 |
| 诊断产物目录加 `serverKind` 一层 | 消除双跑互相覆盖 |
| 用例契约新增 `opPlayers` / `skipReason` / `ctx.restartServer()` | 需要命令权限、需要显式记录阻塞点、需要验证存档持久化 |
| `CASE_TIMEOUT_MS` 90s → 240s | 持久化用例体内含一次服务端重启（≥70s） |
| 交互/容器/传送链路的静默分支补 `warn` 日志 | 「右键无反应且零日志」无法定位 |
| 场景文件去重：槽位常量与交互助手收敛到 `scenarios/shared.ts` | 原先 inventory/crafting/furnace 各有一份副本 |

### 8.2 顺带修掉的服务端缺陷

1. **丢物品在服务端完全无效**（`AbstractContainerMenu::dropItem` 的回调从未被设置）+ 点击用
   无 world 的占位 Player 结算 → 第三方客户端（mineflayer 的 `toss`/`tossStack`）丢物品
   「客户端以为丢了、服务端什么都没有」。现在 `ContainerManager::_installMenuCallbacks`
   统一安装回调，`handleClick` 接收真实玩家实体。
2. **木桶/潜影盒永远打不开**（菜单工厂只认 Chest/TrappedChest；潜影盒另有
   `getOpenBoundingBox` 与本体重叠导致 `canOpen()` 恒 false）。
3. **菜单工厂不看玩家维度**（固定取主世界）。

### 8.3 仍阻塞 3 条 e2e 用例的服务端缺陷（**未修，优先处理**）

| # | 缺陷 | 症状与证据 | 解除条件 |
|---|---|---|---|
| 1 | **初次 tick 非主世界维度即崩溃** | `NaturalSpawner::_createDensityManager` 读 0x0 触发 ACCESS_VIOLATION，栈：`NaturalSpawner.cpp:965 ← tick:376 ← ServerDimension::tick:187`；服务端整体退出（exit code 3221225477） | 修好崩溃；下界用例现以 `/gamerule doMobSpawning false` 绕开，修好后移除该绕行 |
| 2 | **跨维度传送后下发未生成的区块** | 客户端在下界 `(0,125,0)` 周围 5×5（含本该是空气的层）全读到 `netherrack`；命令改动的方块也不会可靠到达客户端 | 阻塞 `containers/container_in_nether_is_not_overworld`（该用例对应的菜单工厂缺陷已修，修好区块下发后应直接通过） |
| 3 | **存档重启后实体不与地形碰撞** | 客户端能看到地形（`blockAt` = grass_block/bedrock，81 列区块正常），玩家却从出生点坠入虚空（y < -400），服务端不做纠正；同时间段服务端批量刷出又立刻销毁生物（3000+ 次 spawn/destroy） | 阻塞两条 `persistence/*` 用例 |

**复现 3 的快捷方式**：任取一次 e2e 失败留下的 `build/e2e/runs/<runId>/<caseId>/`（内含已保存的
`saves/`），直接用它当游戏目录起 `minecraft-server.exe --config <runDir>/server_options.json`，
再连一个 bot 观察其 y 是否持续下降。

### 8.4 mineflayer 侧的两个新坑（写用例必看）

- **跨维度传送/重生后 `openContainer` 会卡住**：mineflayer 收到 `respawn` 后先关物理、延迟 1500ms
  恢复，而 `openContainer` 内部的 `lookAt(..., false)` 要等一个 `physicsTick`。不等这一步，
  **客户端连 `use_item_on` 都发不出去**，服务端侧毫无痕迹。用 `shared.ts` 的
  `waitForPhysicsTick()`。同一窗口期内 `onGround` 还是传送前的旧值，不能用它等落地。
- **vanilla 在「客户端预测与服务端结算一致」时不发增量包**：`containerSyncCount` 那套判据对
  vanilla 不成立。改为「关窗后重新打开，断言全量内容」——两侧都成立且是服务端权威状态。
