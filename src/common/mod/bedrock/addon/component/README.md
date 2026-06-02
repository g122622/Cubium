# 自定义组件系统

提供脚本可注册的方块和物品自定义组件，使行为包能够通过JavaScript回调响应游戏事件。

## 目录结构

```
component/
├── README.md                          # 本文件
├── BlockComponentRegistry.hpp/cpp     # 方块组件注册表
├── BlockComponentEvents.hpp           # 方块组件事件数据结构
├── ItemComponentRegistry.hpp/cpp      # 物品组件注册表
├── ItemComponentEvents.hpp            # 物品组件事件数据结构
└── CustomComponentParameters.hpp      # 组件参数容器
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

## JS绑定

自定义组件通过`@minecraft/server`模块导出的`blockComponentRegistry`和`itemComponentRegistry`全局对象注册。

```javascript
import { world, blockComponentRegistry, itemComponentRegistry } from "@minecraft/server";

// 注册方块自定义组件
blockComponentRegistry.registerCustomComponent("minecraft:my_block", {
    onStepOn: (event) => {
        console.log(`Entity stepped on block at ${event.x}, ${event.y}, ${event.z}`);
    },
    onPlace: (event) => {
        console.log(`Block placed at ${event.x}, ${event.y}, ${event.z}`);
    },
    beforeOnPlayerPlace: (event) => {
        // 可取消放置
        event.cancel = true;
    }
});

// 注册物品自定义组件
itemComponentRegistry.registerCustomComponent("minecraft:my_item", {
    onUse: (event) => {
        console.log(`Item used by entity ${event.sourceId}`);
    },
    onHitEntity: (event) => {
        console.log(`Item hit entity ${event.hitEntityId}`);
    },
    beforeDurabilityDamage: (event) => {
        // 可修改耐久伤害
        event.durabilityDamage = 0; // 不消耗耐久
    }
});
```

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
