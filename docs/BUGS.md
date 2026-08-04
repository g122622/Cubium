使用java版1.21.11客户端连接到本项目服务端时，观察到大量bug：

1. 时间不更新，昼夜不更替，且进入世界的初始时间是0，不符合mc逻辑
2. 放置床的时候，只能放半张，另一半床不会放置出来
3. 使用燧石点击tnt没反应
4. 玩家快速移动导致区块卸载的时候，服务端有大量下面日志：
```
[2026-08-04 13:24:19.942] [error] Attempted to remove non-existent entity with ID 3892
[2026-08-04 13:24:19.942] [error] Attempted to remove non-existent entity with ID 3894
[2026-08-04 13:24:19.942] [error] Attempted to remove non-existent entity with ID 3901
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 3902
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 3903
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 3906
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 3970
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 4364
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 4235
[2026-08-04 13:24:19.943] [error] Attempted to remove non-existent entity with ID 4050
```
5. 客户端输入命令的时候能看到命令树的智能提示，但提交命令之后没反应
6. 红石不工作（红石火把无法激活红石粉末）
7. 右键无法放置矿车
8. 方块更新异常（例如海里有个海带柱子，我破坏了海带柱子底部的海带，正常情况下应该是上面海带替换为水，然而现实变成了替换为草方块、木板！错乱了。）
9. 流体无法流动，沙子无法下落（在intergrated_server+原生客户端中则不会出现这个bug）

---

## 修复记录

### 第9条（流体不流动 / 沙子不下落）— 已修复（待 Java 客户端实测确认）

**根因**：Java wire codec 出站边界漏掉 `JavaBlockStateIdMap::toJavaGlobalId` 翻译，直接发了项目内部 stateId，Java 客户端按 vanilla globalId 解码→错位。用原生 C++ 客户端连集成服务器时正常，是因为本地路径走 LocalTransport 直传 IR 包不经 codec，IR 层 blockStateId 保持内部 id 语义自洽。

**修复点**（均在 Java wire 出站边界注入翻译，本地路径不受影响）：
- `blockUpdateCodec`（id=8）：encode/decode 加 `toJavaGlobalId`/`fromJavaGlobalId`。收敛流体不流动、沙子下落方块变 air、方块更新错乱（第8条同源）。
- `addEntityCodec`（id=1）：`data` 字段对 FallingBlock（entityTypeId=51）条件翻译为 globalId；附带修正旋转字段写出顺序为 vanilla 的 `xRot, yRot, yHeadRot`。收敛沙子下落实体不可见。
- `EntityMetadataSerializer`：BlockState/OptionalBlockState 字段 serialize 写 globalId、deserialize `fromJavaGlobalId` 反查回内部 id（与 chunk 路径架构统一）。收敛末影人搬方块、矿车展示块、TNT 方块状态。
- `tests/main.cpp`：补 `JavaBlockStateIdMap::initialize()` 使 codec 往返测试在测试环境可用。

**未修（同源但不同语义维度，留 TODO 待后续）**：
- `LevelEvent.data`（方块破坏粒子）、`SectionBlocksUpdate`（未启用）：同源漏翻译，本次不动。

### 第6条（红石不工作 / BlockEvent blockId 错位）— 已修复（待 Java 客户端实测确认）

**根因**：vanilla `ClientboundBlockEventPacket` 第4字段 blockId 是 Java `BuiltInRegistries.BLOCK` 注册表 id（`ByteBufCodecs.registry(Registries.BLOCK)`），既非 stateId 也非 state globalId，是独立语义维度。项目 `ServerWorld::runBlockEvents` 广播时传的是 `state->stateId()`（内部 stateId），三重错位，真 Java 客户端按 Block 注册表 id 校验拒绝该事件（红石活塞/音符盒/箱子声效等异常）。原生客户端正常是因为 `ClientPlayVisitor` 不消费 blockId 字段（仅用 pos+b0+b1 调 `BlockEntity::triggerEvent`），本地直传 IR 不经 codec。

**修复点**：
- 新增数据源 `assets/data/blocks_prismarine_1.21.11.json`（PrismarineJS minecraft-data，含每 block 显式 `id`=vanilla Block 注册表 id，已验证 id==index 全成立、air=0/stone=1/dirt=9/cobblestone=12）。
- 新增烘焙脚本 `scripts/baking/bake_java_block_id_table.ts` + 生成表 `generated/java_block_id_table.gen.{cpp,hpp}`（name→registryId 二分查找表 + 反向稠密数组），仿 `bake_java_item_table.ts` 范式。CMake `add_custom_command` 构建期重生成。
- 新增 `JavaBlockIdMap`（`mappings/JavaBlockIdMap.{hpp,cpp}`）：内部 blockId ↔ Java Block 注册表 id 双向映射。block 内部 id 0=air（有效），故 miss 直接返 0（air），无需像 `JavaItemIdMap` 取 air 真实内部 id。三向查表：`toJavaRegistryId(const Block&)`/`(string_view)`/`(u32 internalBlockId)`（codec 边界用，稠密下标）+ `fromJavaRegistryId`（反向稠密）。
- 两端 bootstrap 登记 `JavaBlockIdMap::instance().initialize()`（`RegistryBootstrap.cpp`/`ClientApplicationBootstrap.cpp`，须在 `VanillaBlocks::initialize` 之后）。
- `ServerWorld::runBlockEvents`：广播值从 `state->stateId()` 改为入队时 `event.block->blockId()`（内部 blockId），与 vanilla "广播触发事件的方块本身"语义一致，且避免方块已变时读到错误 state。
- `blockEventCodec`（id=7）：出站 `toJavaRegistryId(internalBlockId)` 译为 Java Block 注册表 id，decode 对称 `fromJavaRegistryId` 反翻译（客户端不消费 blockId，反翻译仅为 codec 自对称/往返测试）。IR 层 `BlockEvent.blockId` 存内部 blockId（与 `BlockUpdate.blockStateId` 存内部 stateId 同范式），本地直传自洽。
- `PlayerBroadcaster`/`MinecraftServer`/`ServerWorld` 回调链签名 `blockStateId`→`blockId` 同步改名。`tests/main.cpp` 补 `JavaBlockIdMap::initialize()`；`PlayBlockEvent` 往返测试改用动态获取的 chest 内部 blockId。

**验证**：编译 exit0；`JavaBlockIdMap: matched 1115 blocks, 0 fell back to air`（项目 1115 个 block 全部命中 vanilla 映射）；`PlayBlockEvent` 往返 PASSED；`NetworkTestBase` 全 181 例 PASSED 无回归；原生客户端启动两端 map 初始化正常无崩。

