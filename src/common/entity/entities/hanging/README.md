# 悬挂实体

本目录包含可以挂在墙上的装饰实体。

## 目录结构

```
hanging/
├── HangingEntity.hpp/cpp    # 悬挂实体基类 + PaintingEntity、ItemFrameEntity、LeashKnotEntity
└── README.md                # 本文档
```

## 内部模块关系

```
Entity (core/Entity.hpp)
    └── HangingEntity        # 悬挂实体基类，管理位置、方向、有效检测
        ├── PaintingEntity   # 画作实体（多种尺寸）
        ├── ItemFrameEntity  # 物品展示框（可旋转，红石信号输出）
        └── LeashKnotEntity  # 拴绳结（连接多条拴绳）
```

**HangingEntity 基类职责**：
- 管理 `m_hangingPos`（悬挂位置）和 `m_direction`（悬挂方向）
- 定期检查 `isValidPosition()` / `canPlaceOn()` 验证支撑方块
- 受伤时调用 `hurt()`（覆写自 Entity），执行 `dropItem()` + `remove()` + `markHurt()`

**子类实现**：
- `PaintingEntity`：画作尺寸由 `PAINTING_TYPES` 静态数组定义
- `ItemFrameEntity`：
  - 存储 `ItemStack`，支持旋转（0-7），提供 `getAnalogOutput()` 红石信号
  - `processInitialInteract()`：处理玩家右键交互（放入/旋转/取出物品），每次操作后触发 `notifyComparatorUpdate()` 和 `gameEvent(BLOCK_CHANGE)`
  - `setDisplayedItem(stack, updateComparator=true)`：设置展示物品并重置旋转。`updateComparator` 为 true 时通知红石比较器更新（交互时传 true，NBT 加载时传 false）
  - `setItemRotation(rotation, updateComparator=true)`：设置旋转值（0-7）。`updateComparator` 为 true 时通知红石比较器更新（交互时传 true，NBT 加载时传 false）
  - `rotateItem()`：旋转物品，委托给 `setItemRotation(m_rotation + 1, true)`
  - `notifyComparatorUpdate()`：通知悬挂位置周围的红石比较器重新计算输入信号，调用 `RedstoneSystem::updateComparators()`
  - `dropItem()`：掉落物品展示框本身和内含物品，清空展示物品，触发 `notifyComparatorUpdate()` 和 `gameEvent(BLOCK_CHANGE)`
  - `getAnalogOutput()`：返回红石信号强度（无物品=0，有物品=rotation%8+1）
- `LeashKnotEntity`：管理 `m_leashedEntities` 向量，无绑定时自动消失，覆写 `processInitialInteract()` 处理拴绳转移，`tick()` 中检查栅栏存活和绑定实体存活

## 上下游外部依赖关系

**本目录依赖**：
- `common/entity/core/Entity.hpp` - 实体基类
- `common/entity/entities/item/ItemEntity.hpp` - 物品掉落
- `common/entity/utils/ItemDropHelper.hpp` - 物品掉落工具
- `common/item/Items.hpp` - 物品注册表（PAINTING、LEAD）
- `common/util/Direction.hpp` - 方向枚举和工具
- `common/util/math/random/Random.hpp` - 随机数
- `common/world/IWorld.hpp` - 世界接口
- `common/world/block/Block.hpp` - 方块检测（hasEnoughSolidSide）
- `common/world/gameevent/GameEvents.hpp` - 游戏事件（BLOCK_CHANGE、BLOCK_ATTACH）
- `common/world/gamerule/GameRules.hpp` - 游戏规则（DO_ENTITY_DROPS）
- `common/world/redstone/RedstoneSystem.hpp` - 红石系统（updateComparators）

**被谁依赖**：
- `common/entity/registry/VanillaEntities.hpp` - 实体类型注册
- 服务端世界（实体生成、tick调度）
- 红石系统（ItemFrameEntity 的 `getAnalogOutput()`）

## 容易踩的坑

### 1. HangingEntity::Direction 与 mc::Direction 的映射

`HangingEntity::Direction` 枚举值（SOUTH=0, WEST=1, NORTH=2, EAST=3）与 `mc::Direction`（North=2, South=3, West=4, East=5）不同，转换时需使用 `ItemFrameEntity::getHorizontalFacing()` 中的映射逻辑。

### 2. 支撑方块检测

`canPlaceOn()` 计算支撑方块位置时，需要根据悬挂实体的面向方向计算背面方向（悬挂实体面向 SOUTH 时，背面是 NORTH），然后使用 `Block::hasEnoughSolidSide()` 检测支撑。

### 3. LeashKnotEntity 生命周期

LeashKnotEntity 在 `tick()` 中执行两项检查：
1. **栅栏存活检查**：调用 `survives()` 验证栅栏方块是否仍然存在。如果栅栏被破坏，释放所有绑定的生物并销毁拴绳结。
2. **绑定实体检查**：遍历 `m_leashedEntities`，移除已死亡的实体。当列表为空时自动调用 `dropItem()` 和 `remove()`。

### 4. LeashKnotEntity 交互

LeashKnotEntity 覆写了 `processInitialInteract()` 以处理玩家右键交互：
- 玩家手持拴绳且有被拴住的生物 → 将生物转移到栅栏结上
- 玩家不潜行且栅栏结上有绑定生物 → 将生物取回拴到玩家身上

### 5. ItemFrameEntity 交互

ItemFrameEntity 覆写了 `processInitialInteract()` 以处理玩家右键交互：
- 空展示框 + 玩家手持物品 → 放入物品
- 有物品的展示框 + 玩家潜行 → 取出物品
- 有物品的展示框 + 玩家不潜行 → 旋转物品

### 6. PaintingEntity 尺寸与碰撞箱

画作的碰撞箱需要根据 `getWidth()` / `getHeight()` 动态计算，不同尺寸的画作占用不同的方块空间。

### 7. ItemFrameEntity 红石信号

`getAnalogOutput()` 返回 `rotation % 8 + 1`（有物品时范围 1-8），无物品返回 0。红石比较器检测时需检查朝向是否一致。

### 8. 游戏规则 doEntityDrops 对悬挂实体掉落的影响

**问题**：所有悬挂实体的 `dropItem()` 方法都受 `GameRuleKeys::DO_ENTITY_DROPS` 游戏规则控制。

**要点**：
- `PaintingEntity::dropItem()`：当 `DO_ENTITY_DROPS` 为 false 时直接返回，不产生画作物品掉落
- `ItemFrameEntity::dropItem()`：当 `DO_ENTITY_DROPS` 为 true 时掉落物品展示框本身和内含物品；当为 false 时仅清空展示物品，不产生掉落。无论游戏规则如何，`m_displayedItem` 都会被清空
- `LeashKnotEntity::dropItem()`：当 `DO_ENTITY_DROPS` 为 false 时直接返回，不产生拴绳物品掉落

参考 MC 1.21.11：`Painting.dropItem()`、`ItemFrame.dropItem()`、`Leashable.tickLeash()` 中的 `ENTITY_DROPS` 检查

### 9. ItemFrameEntity 的 updateComparator 参数

**问题**：`setDisplayedItem()` 和 `setItemRotation()` 都有 `updateComparator` 参数，控制是否触发红石比较器更新。

**要点**：
- 交互操作（玩家右键放入/取出/旋转物品）应传 `true`，以通知红石比较器更新
- NBT 加载时应传 `false`，避免加载过程中触发不必要的更新
- `dropItem()` 内部无条件调用 `notifyComparatorUpdate()`，因为物品展示框被破坏时必须通知比较器
- `rotateItem()` 委托给 `setItemRotation(m_rotation + 1, true)`，始终触发更新

参考 MC 1.21.11：`ItemFrame.setItem(ItemStack, boolean)` 和 `ItemFrame.setRotation(int, boolean)` 中的 `boolean` 参数
