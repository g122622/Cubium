# 试炼密室方块 (Trial Blocks)

试炼密室相关方块实现，包括试炼刷怪笼、宝库和自动合成器。

## 目录结构

```
trial/
├── TrialBlocks.hpp/cpp   # 试炼刷怪笼、宝库、自动合成器方块
├── HeavyCoreBlock.hpp/cpp # 重质核心方块
└── README.md
```

## 内部模块关系

```
Block (父类)
    ↑
    ├── TrialSpawnerBlock   ──→ TrialSpawnerBlockEntity
    ├── VaultBlock (HorizontalBlock) ──→ VaultBlockEntity
    └── CrafterBlock (HorizontalBlock) ──→ CrafterBlockEntity
```

## CrafterBlock 自动合成器

### 方块状态属性

| 属性 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| FACING | Direction (N/S/E/W) | North | 合成器面朝方向（射出方向） |
| TRIGGERED | Boolean | false | 是否被红石触发 |
| CRAFTING | Boolean | false | 是否正在合成动画中 |

### 红石行为

- **上升沿**：红石信号从无到有时，调度4 tick延时后执行合成，设置 TRIGGERED=true
- **下降沿**：红石信号从有到无时，同时重置 TRIGGERED=false 和 CRAFTING=false，清零倒计时
- **比较器输出**：非空槽位数 + 禁用槽位数（0-15）
- **弱信号输出**：CRAFTING=true 时输出15，否则输出0

### 合成时序

```
Tick 0: 红石信号上升沿 → TRIGGERED=true, 调度4 tick延时
Tick 4: tick() → _dispenseFrom() → 查配方、射出物品、消耗原料、CRAFTING=true、craftingTicksRemaining=6
Tick 5~9: CrafterBlockEntity::tick() → craftingTicksRemaining 递减
Tick 10: craftingTicksRemaining=0 → CRAFTING=false
```

### 物品射出逻辑

1. 优先尝试注入面前容器（箱子/漏斗等）：先尝试堆叠，再尝试空槽位
2. ISidedInventory容器：通过getSlotsForFace/canInsertItem方向性槽位访问插入物品
3. 容器无法接收时，弹出到世界（面朝方向0.7格偏移 + 高斯散射）

### 方块破坏行为

- `onBlockRemoved`：方块被移除时，遍历9格物品栏，将所有物品通过ItemDropHelper掉落到世界，并通知比较器更新
- `onBlockActivated`：玩家右键交互的入口（当前返回Pass，GUI待实现，TODO: ContainerType::Crafter）

### setItem自动重新启用

CrafterBlockEntity通过SimpleInventory的onChanged回调实现：当物品被放入禁用槽位时，
自动将该槽位重新启用（对应MC原版CrafterBlockEntity.setItem()中isSlotDisabled检查）。

## 上下游外部依赖关系

### 上游依赖

- `world/blockentity/trial/` - 方块实体（CrafterBlockEntity、TrialSpawnerBlockEntity、VaultBlockEntity）
- `world/redstone/RedstoneSystem` - 红石信号检测
- `world/tick/TickManager` - 延时tick调度
- `item/crafting/RecipeManager` - 配方查找

### 下游依赖

- `entity/entities/item/ItemEntity` - 物品实体生成
- `entity/inventory/IInventory` - 容器接口（物品注入）
- `util/Direction` - 方向工具

## 容易踩的坑

### 1. CRAFTING 与 TRIGGERED 的区别

TRIGGERED 跟踪红石信号状态（防止重复触发），CRAFTING 跟踪合成动画状态。
红石信号下降沿会同时清除两者并清零倒计时，这意味着合成未完成时红石断开会中断动画。

### 2. 容器注入的方向

合成器面朝方向（FACING）既是射出方向也是容器查找方向。
物品注入到 `pos.offset(facing)` 位置的容器，如果该位置没有容器则弹出到世界。

### 3. 配方匹配时禁用槽位视为空

`CrafterBlockEntity::asCraftInput()` 构建合成输入时，禁用的槽位视为空槽位，
这使得合成器可以合成小于3x3的配方（如2x2配方）。
