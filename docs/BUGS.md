使用java版1.21.11客户端连接到本项目服务端时，观察到大量bug：

1. 时间不更新，昼夜不更替，且进入世界的初始时间是0，不符合mc逻辑
2. 放置床的时候，只能放半张，另一半床不会放置出来
3. 使用燧石点击tnt没反应/右键无法放置矿车/右键打不开工作台。怀疑是共性问题
   - 【已修复】根因有两个独立断点：
     - P0（共性根因，三场景全卡）：服务端从不发送 `BlockChangedAck` 包。真 Java 客户端的 `use_item_on`/`use_item`/挖掘包带递增 sequence，服务端必须回 `BlockChangedAck(sequence)` 推进客户端方块预测状态机，收不到 ack 则状态机卡死、后续右键静默失效。clientbound 全链路已就绪，唯独服务端发送端缺失。已在 `ServerPlayHandler` 三处 handler（handleBlockPlacementPacket/handleUseItemPacket/handleBlockInteractionPacket）补发 ack（立即发，对齐 vanilla `ackBlockChangesUpTo` 语义）。
     - P1（矿车专致命）：服务端 `handleBlockPlacementPacket` 对非 block-item 手持物只派发 `Block::onBlockActivated`（vanilla useWithoutItem），从不派发 `Item::onItemUse`（vanilla `ServerPlayerGameMode.useItemOn` 第③步 Item.useOn），矿车靠 `MinecartItem::onItemUse` 放置故永远放不下。已新增 `BlockInteractionManager::handleItemUseOn`，在非 block-item 分支对齐 vanilla ②→③ 派发顺序（handleBlockUse 未短路才调 handleItemUseOn）。
     - TNT 点火（TNTBlock::onBlockActivated 自洽）和工作台打开（tryOpenCraftingContainer 旁路）在 P0 修好后即工作；矿车需 P0+P1 同时修。
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
6. 红石不工作（红石火把无法激活红石粉末）
8. 能够攻击动物，但是攻击时动物不会显示红色受伤动画和声音（动物的其他受伤途径也没有动画和声音）
9. registerGoals 非幂等致基类 goal 翻倍是预存在的体系问题（如 MonsterEntity::SwimGoal 翻倍）