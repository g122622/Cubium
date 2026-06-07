# 自定义组件系统

提供脚本可注册的方块和物品自定义组件，使行为包能够通过JavaScript回调响应游戏事件。

## 目录结构

```
component/
├── README.md                          # 本文件
├── BlockComponentRegistry.hpp/cpp     # 方块组件注册表（单例，管理方块自定义组件的注册和事件派发）
├── BlockComponentEvents.hpp           # 方块组件事件数据结构（StepOn/Place/Break等12种事件）
├── ItemComponentRegistry.hpp/cpp      # 物品组件注册表（单例，管理物品自定义组件的注册和事件派发）
├── ItemComponentEvents.hpp            # 物品组件事件数据结构（Use/HitEntity/Consume等7种事件）
└── CustomComponentParameters.hpp      # 组件参数容器（从JSON定义传入，使用std::any存储）
```

## 组件注册表

### BlockComponentRegistry

单例注册表，管理方块自定义组件的注册和事件派发。

**注册方式：** 脚本通过`blockComponentRegistry.registerCustomComponent()`注册。

**派发方式：** Block虚拟方法（如`onEntityWalk`、`onBlockActivated`等）的调用点检查注册表，有回调则执行。

**性能优化：** 使用位掩码`CallbackFlags`快速判断某方块类型是否有特定事件回调，避免遍历组件列表。

### ItemComponentRegistry

结构同BlockComponentRegistry，但针对物品事件。

## 事件类型

### 方块事件

| 事件 | 对应Bedrock API | 可取消 | 触发时机 | 派发位置 |
|------|-----------------|--------|----------|----------|
| StepOn | onStepOn | 否 | 实体踩上方块 | Entity::doBlockCollisionsAfterMove |
| StepOff | onStepOff | 否 | 实体离开方块 | Entity::doBlockCollisionsAfterMove |
| Place | onPlace | 否 | 方块被放置 | BlockItem::tryPlace |
| Break | onBreak | 否 | 方块被破坏 | BlockInteractionManager::handleBlockBreak |
| PlayerBreak | onPlayerBreak | 否 | 玩家破坏方块 | BlockInteractionManager::handleBlockBreak |
| PlayerInteract | onPlayerInteract | 否 | 玩家右键方块 | BlockInteractionManager::handleBlockUse |
| PlayerPlaceBefore | beforeOnPlayerPlace | 是 | 玩家放置方块前 | BlockItem::tryPlace |
| EntityFallOn | onEntityFallOn | 否 | 实体落在方块上 | Entity::doBlockCollisionsAfterMove |
| RandomTick | onRandomTick | 否 | 随机刻 | ServerWorld::tickEnvironment |
| Tick | onTick | 否 | 计划刻 | TickManager::TickManager (block.tick回调) |
| RedstoneUpdate | onRedstoneUpdate | 否 | 红石更新 | (待接入) |
| Entity | onEntity | 否 | 实体触发方块事件 | Entity::doBlockCollisions |
| BlockStateChange | onBlockStateChange | 否 | 方块状态变化 | (待接入) |

### 物品事件

| 事件 | 对应Bedrock API | 可修改 | 触发时机 | 派发位置 |
|------|-----------------|--------|----------|----------|
| Use | onUse | 否 | 玩家右键使用物品 | Item::onItemRightClick |
| UseOn | onUseOn | 否 | 玩家右键方块使用物品 | BlockInteractionManager::handleBlockUse |
| HitEntity | onHitEntity | 否 | 物品击中实体 | Player::attack |
| BeforeDurabilityDamage | onBeforeDurabilityDamage | 可修改damage | 耐久伤害前 | Player::attack |
| MineBlock | onMineBlock | 否 | 物品挖掘方块 | BlockInteractionManager::handleBlockBreak |
| CompleteUse | onCompleteUse | 否 | 物品使用动画完成 | LivingEntity::updateActiveItem |
| Consume | onConsume | 否 | 物品被消耗 | FoodItem::onItemUseFinish |

## 内部模块关系

```
BlockComponentRegistry ←── Block/Item虚拟方法调用点
    ↑                         (Entity::doBlockCollisionsAfterMove,
    │                          BlockInteractionManager::handleBlockUse/handleBlockBreak,
    │                          BlockItem::tryPlace,
    │                          ServerWorld::tickEnvironment,
    │                          TickManager,
    │                          Player::attack,
    │                          LivingEntity::updateActiveItem,
    │                          FoodItem::onItemUseFinish,
    │                          Item::onItemRightClick)
    │
ItemComponentRegistry ←── 同上
    │
    └──→ modules/ScriptCustomComponentBinding.cpp (JS绑定)
         └──→ modules/MinecraftModuleFactory.cpp (模块注册)
```

## 外部依赖关系

- **被依赖者：** `modules/MinecraftModuleFactory.cpp`（JS绑定），`modules/ScriptCustomComponentBinding.cpp`（JS→C++回调桥接）
- **依赖：** `core/Types.hpp`，`spdlog`

## 容易踩的坑

1. **组件名称必须包含命名空间前缀**：如`my_pack:custom_behavior`，否则会输出警告日志
2. **hasXxxCallback是性能优化**：派发事件前必须先调用对应的`hasXxxCallback()`检查，避免不必要的锁竞争和组件遍历
3. **线程安全**：注册表使用`shared_mutex`，读操作用共享锁，写操作用独占锁；派发时持有共享锁，回调内不可再调用注册方法（会死锁）
4. **beforeOnPlayerPlace的cancel传播**：所有回调都会执行，但任一回调设置`event.cancel = true`会导致最终返回true（取消放置）
5. **派发返回值含义**：`dispatchXxx()`返回true表示至少有一个组件处理了该事件，非取消含义
6. **RedstoneUpdate/BlockStateChange事件未接入**：表格中标注"(待接入)"的事件目前派发位置为空，调用dispatch方法永远不会触发回调
7. **CustomComponentParameters不可拷贝**：使用`std::any`存储，拷贝可能昂贵，仅支持移动语义
