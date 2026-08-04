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
- 第6条红石（BlockEvent 第4字段）：vanilla 期望 Block 注册表 id（非 stateId 非 globalId），需新建 `JavaBlockIdMap` + PrismarineJS blocks.json 数据源，属独立工作。已在 `PlayerBroadcaster::broadcastBlockEventInRange` 与 `blockEventCodec` 标 TODO。
- `LevelEvent.data`（方块破坏粒子）、`SectionBlocksUpdate`（未启用）：同源漏翻译，本次不动。

