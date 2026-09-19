# 交接文档：容器/物品栏链路修复与 e2e 用例补充

> 本文档面向接管本工作的开发者，交代任务背景、已完成内容、当前卡住的问题与后续全部待办。
> 撰写时的分支：`main`。

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

## 三、当前卡住的问题（**首要待办**）

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

**调试手法建议**：在 `sendContainerContent`/`buildMenuContent` 出口打印构造出的 `items` 数组的非空槽下标（一次性日志，定位后删除）。注意 C++ 侧重编译约 30 秒（增量），e2e 单用例约 3 秒。

---

## 四、后续全部待办

### A. e2e 用例

| # | 任务 | 说明 |
|---|---|---|
| 1 | 修完上述错位缺陷后刷新基线 | `node run.ts --mode=refresh --accept`。**注意：refresh 会拒绝记录含失败的运行**，故必须先用例全绿 |
| 2 | 合成用例（工作台 3x3 + shift-click） | 依赖已修好的容器打开链路 |
| 3 | 熔炉用例（放置、打开、烧炼产出、进度同步、LIT 翻转） | 依赖 `container_set_data`（已接通） |

### B. 服务端缺陷（子代理审查结果，**尚未修**）

**高风险**：

1. **菜单工厂硬编码主世界取方块实体** —— `StandaloneServer` 的 `setMenuFactory` 用 `m_dimensionManager->getOverworld()`，完全不看玩家所在维度。玩家在下界/末地开容器会拿到空隙或串到主世界同坐标的方块实体。（同文件关闭回调用了正确的 `getPlayerDimensionWorld()`，可对照。）
2. **`BarrelBlock` 用 `ContainerType::Generic9x3`，但工厂要求实体是 `Chest`/`TrappedChest`** —— `BarrelEntity` 继承 `LootableContainerBlockEntity`，故**桶永远打不开且无日志**。`ShulkerBoxBlock` 同型。
3. **菜单工厂所有失败分支静默返回空菜单**，叠加 `ContainerManager::openContainer` 的无日志 Error 与三个调用方（`openContainerRequest`/`tryOpenCraftingContainer`）丢弃 Error ——整条「右键容器无反应」链路端到端零日志。
4. `IntegratedServer` 的本地客户端容器点击/关闭有 `if (!m_openMenu || id 不匹配) return;` 的静默分支（`ContainerManager::handleClick` 已修的缺陷的本地复刻）。
5. `BlockActionHandler` 把「玩家实体查不到（服务端缺陷）」与「距离过远（反作弊）」揉进同一个无日志分支。
6. `MovementHandler` 中 `serverPlayer == nullptr` 时整段反飞行校验被静默跳过。

**中风险**：`EntityActionHandler`/`MovementHandler`/`PlayerStateHandler` 多处「实体查不到」的静默返回；`ServerPlayHandler` 的兜底分支日志不带变体名。

### C. 遗留 TODO（代码中已标 `TODO`）

- 客户端属性消费点（移动速度预测、挖掘速度、交互距离仍读本地硬编码默认值）。
- 补齐 12 条缺失的 vanilla 属性（尤其 `block_break_speed`；补齐后 e2e 方块交互用例应改回 survival 模式）。
- `update_attributes` 的增量脏刷新（当前只做 spawn/join 时的全量下发）。
- `NbtHelper.cpp` 的属性 id 序列化缺陷；`AttributeCommand` 的 Identifier 字符集校验；属性名迁移（`generic.*` → `minecraft:*`）。
- `ContainerManager` 中的**占位 Player**（`handleClick`/`closeContainer` 各构造一个假 Player 仅为传参）应重构掉。

---

## 五、调试本链路时的陷阱（血泪教训）

### 1. mineflayer 会**本地预测**，用例极易假通过

`creative.setInventorySlot` 与 `clickWindow` 都会**立刻**写本地镜像、**不等服务端回包**（见 mineflayer 的 `creative.js`、prismarine-windows `Window#acceptClick`）。

**后果**：断言「本地槽位变成某物品」这类写法，在服务端把该包整个丢掉时照样通过。

**正确做法**（`tests/e2e/bot/src/scenarios/inventory.ts` 的 `containerSyncCount`）：判据取**服务端回包**（`window_items` / `set_slot` / `container_set_slot`）——本地预测伪造不出来。

**反例（不要用）**：曾试过「抹掉本地预测值再等服务端填回」，对 Cubium 有效但**对 vanilla 无效**——vanilla 只回**增量** `set_slot`，不会重发全量；且把 `slots[i]` 直接置 null 会让 mineflayer 内部读 `itemId` 时崩溃。

`bot.dig` / `bot.placeBlock` / `bot.openContainer` 则**不预测**，等真实回包，可以安全断言。

### 2. **诊断产物目录两侧同名会互相覆盖**

e2e 失败时落盘的 `build/e2e/artifacts/<runId>/<caseId>/` 在 cubium 与 vanilla 两侧**使用同一路径**，后跑的会覆盖先跑的。

用 `--mode=diff` 排查 cubium 问题时，很容易一直在读 vanilla 的日志（本文档撰写过程中就踩过，浪费了大量时间）。

**规避**：排查单侧问题时用 `--mode=regress --case=<子串>`（只跑 cubium）；确需 diff 时先把产物目录改名或立刻取走。

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

# 单元测试（勿跑全量，超时；用过滤器）
./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="*Container*"
```

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
| e2e 框架说明 | `tests/e2e/bot/` 各源文件的文件头注释 |
