# 加工类方块实体模块

提供熔炉、高炉、烟熏炉、酿造台、信标、潮涌核心、营火等加工类方块实体的实现。

## 目录结构

```
processing/
├── AbstractFurnaceEntity.hpp/cpp  # 熔炉基类（燃烧/熔炼逻辑、ISidedInventory）
├── FurnaceEntity.hpp/cpp          # 普通熔炉（200tick熔炼）
├── BlastFurnaceEntity.hpp/cpp     # 高炉（100tick、仅矿石/金属）
├── SmokerEntity.hpp/cpp           # 烟熏炉（100tick、仅食物）
├── FurnaceInventory.hpp/cpp       # 熔炉专用3槽背包
├── BrewingStandEntity.hpp/cpp     # 酿造台（药水酿造、ISidedInventory）
├── BeaconEntity.hpp/cpp           # 信标（金字塔效果、光束渲染）
├── ConduitEntity.hpp/cpp          # 潮涌核心（水下信标、攻击敌对生物）
├── CampfireBlockEntity.hpp/cpp    # 营火（食物烹饪、4槽位）
└── README.md
```

## 内部模块关系

```
BlockEntity (父模块基类)
       ↑
       │
ContainerBlockEntity (父模块容器基类)
       ↑
       ├──────────────────────┬──────────────────────┐
       │                      │                      │
LockableBlockEntity    BrewingStandEntity    CampfireBlockEntity
(core/ 可锁定容器基类)   (多重继承 ISidedInventory)
       ↑
       │
AbstractFurnaceEntity (熔炉基类，多重继承 ISidedInventory)
       ↑
       ├──────────────────┬──────────────────┐
       │                  │                  │
FurnaceEntity    BlastFurnaceEntity   SmokerEntity
(普通熔炉)         (高炉)              (烟熏炉)

BeaconEntity (信标，独立继承 BlockEntity)
ConduitEntity (潮涌核心，独立继承 BlockEntity)
FurnaceInventory (熔炉背包，非 BlockEntity，被 AbstractFurnaceEntity 组合)
```

## 上下游外部依赖关系

### 上游依赖（谁使用了这个模块）

- `world/block/blocks/` - 熔炉方块、酿造台方块、信标方块等创建和访问方块实体
- `world/chunk/` - 区块加载时反序列化方块实体
- `entity/inventory/container/` - 熔炉 GUI 容器、酿造台 GUI 容器
- `client/renderer/` - 信标光束渲染、熔炉火焰渲染

### 下游依赖（这个模块依赖了谁）

- `world/blockentity/BlockEntity.hpp` - 方块实体基类
- `world/blockentity/ContainerBlockEntity.hpp` - 容器方块实体基类
- `world/blockentity/core/LockableBlockEntity.hpp` - 可锁定基类
- `world/blockentity/core/SimpleInventory.hpp` - 简单背包实现
- `entity/inventory/IInventory.hpp` - 背包接口
- `entity/inventory/ISidedInventory.hpp` - 分面背包接口
- `item/crafting/SmeltingRecipe.hpp` - 熔炼配方
- `item/crafting/CampfireCookingRecipe.hpp` - 营火烹饪配方
- `item/potion/PotionBrewing.hpp` - 药水酿造
- `entity/effect/EffectType.hpp` - 效果类型（信标、潮涌核心）
- `entity/interfaces/IMob.hpp` - 敌对生物接口（潮涌核心攻击目标）

## 容易踩的坑

### 1. 熔炼进度回退

不燃烧时进度应该回退 2，而非清零。这是 MC 1.16.5 的行为。

### 2. 高炉/烟熏炉燃料消耗速度

高炉和烟熏炉的 `getBurnTimeForFuel()` 返回基础燃烧时间的一半，即燃料消耗速度是普通熔炉的 2 倍。

### 3. 输出槽满检查

熔炼前必须检查输出槽是否可以接受产物，检查条件包括：输出槽为空，或输出槽物品可以堆叠且堆叠后不超过最大堆叠数。

### 4. 配方缓存

每次输入变化时重新查询配方，避免每 tick 重复查询。`m_lastRecipe` 缓存上次使用的配方。

### 5. ISidedInventory 槽位访问规则

熔炉：
- 上方 (Up)：输入槽（槽位 0）
- 下方 (Down)：输出槽（槽位 2）、燃料槽（槽位 1）
- 侧面：燃料槽（槽位 1）

酿造台：
- 上方 (Up)：材料槽（槽位 3）
- 下方 (Down)：药水瓶槽 + 材料槽（槽位 0, 1, 2, 3）
- 侧面：药水瓶槽 + 燃料槽（槽位 0, 1, 2, 4）

### 6. 熔炉类型对比

| 特性 | 普通熔炉 | 高炉 | 烟熏炉 |
|-----|---------|------|-------|
| 熔炼时间 | 200 tick | 100 tick | 100 tick |
| 配方类型 | SMELTING | BLASTING | SMOKING |
| 可熔炼物 | 全部 | 仅矿石/金属 | 仅食物 |
| 经验倍率 | 1.0 | 0.5 | 0.5 |
| 燃料消耗 | 正常 | 2倍速度 | 2倍速度 |

### 7. 信标效果范围

效果范围 = `level * 10 + 10` 格，需要正确计算金字塔等级。

### 8. 潮涌核心目标追踪

`m_target` 是运行时指针，`m_targetUuid` 用于持久化。恢复时使用 `_findExistingTarget()` 在攻击范围内搜索。

### 9. 营火冷却速度

熄灭时烹饪进度每 tick 减少 2，而非清零。点燃后从上次进度继续。

### 10. 酿造台材料槽提取限制

`canExtractItem()` 对材料槽（槽位 3）只允许提取玻璃瓶，其他物品不能提取。
