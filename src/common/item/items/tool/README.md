# 工具模块 (Tool Module)

本模块实现了 Minecraft 中的工具系统，包括镐、斧、锹、锄、剑、剪刀等工具类物品。

## 目录结构

```
src/common/item/items/tool/
├── ToolType.hpp/cpp           # 工具类型枚举（镐、斧、锹、锄、剑、剪刀）
├── TieredItem.hpp/cpp         # 层级物品基类（提供材质层级支持）
├── ToolItem.hpp/cpp           # 挖掘工具基类（镐、斧、锹、锄的公共逻辑）
├── PickaxeItem.hpp/cpp        # 镐类工具
├── AxeItem.hpp/cpp            # 斧类工具（含去皮、除蜡功能）
├── ShovelItem.hpp/cpp         # 锹类工具（含土径创建功能）
├── HoeItem.hpp/cpp            # 锄类工具（含耕地创建功能）
├── SwordItem.hpp/cpp          # 剑类武器（继承自 TieredItem，非 ToolItem）
└── ShearsItem.hpp/cpp         # 剪刀工具（继承自 Item，独立工具类）
```

## 内部模块关系

```
Item
  └── TieredItem (层级物品基类)
        ├── ToolItem (挖掘工具基类)
        │     ├── PickaxeItem (镐)
        │     ├── AxeItem (斧)
        │     ├── ShovelItem (锹)
        │     └── HoeItem (锄)
        └── SwordItem (剑) - 注意：剑不是 ToolItem

ShearsItem (剪刀) - 独立继承自 Item
```

**关键设计**：
- `TieredItem` 提供材质层级支持（木、石、铁、金、钻石、下界合金）
- `ToolItem` 提供挖掘速度计算、耐久度消耗、有效方块判断等公共逻辑
- `SwordItem` 继承自 `TieredItem` 而非 `ToolItem`，具有不同的耐久消耗机制
- `ShearsItem` 独立继承自 `Item`，实现特殊的采集逻辑

## 上下游外部依赖关系

**本模块依赖**：
- `src/common/item/Item.hpp` - 物品基类
- `src/common/item/ItemStack.hpp` - 物品堆
- `src/common/item/tier/IItemTier.hpp` - 工具层级接口
- `src/common/item/crafting/Ingredient.hpp` - 合成材料
- `src/common/item/items/special/HoneycombItem.hpp` - 蜜脾物品（AxeItem 除蜡依赖 HoneycombItem::getWaxedOff 映射）
- `src/common/world/block/Block.hpp` - 方块定义
- `src/common/world/block/BlockState.hpp` - 方块状态
- `src/common/world/block/Material.hpp` - 材质定义
- `src/common/world/block/VanillaBlocks.hpp` - 原版方块注册

**被依赖**：
- `src/common/item/Items.hpp` - 物品注册，注册各材质的工具实例
- `src/common/entity/player/PlayerEntity.hpp` - 玩家实体，使用工具进行挖掘和攻击

## 容易踩的坑

### 1. 初始化顺序问题

工具注册时需要有效的方块指针（`VanillaBlocks`），如果方块未初始化会导致空指针。必须确保初始化顺序正确：先 `VanillaBlocks::initialize()`，再 `Items::initialize()`。

### 2. 剑不是 ToolItem

剑继承自 `TieredItem` 而非 `ToolItem`，耐久消耗机制不同：
- 剑攻击实体消耗 1 耐久（工具消耗 2）
- 剑破坏方块消耗 2 耐久（工具消耗 1）

### 3. 挖掘等级判断逻辑各异

不同工具的 `canHarvestBlock` 实现不同：
- 镐对 ROCK/IRON/ANVIL 材质总是可以采集
- 锹对 SNOW 材质总是可以采集
- 其他工具需要匹配工具类型

### 4. 材质检查 vs 方块检查优先级

工具有效性判断有两种方式：先检查材质（`isEffectiveMaterial`），再检查特定方块（`isEffectiveBlock`）。修改时需注意这个优先级。

### 5. 静态映射表的初始化顺序

AxeItem、ShovelItem、HoeItem 的静态映射表（去皮、土径、耕地）在构造函数中初始化，但静态方法可能在任何工具实例创建前被调用。应使用"construct on first use"模式（函数局部静态变量）避免初始化顺序问题。

### 6. 有效方块集合的空指针

`initializeEffectiveBlocks()` 在构造函数中调用，如果方块指针为 `nullptr`，会导致集合为空。应使用条件检查：`if (VanillaBlocks::STONE) blocks.insert(VanillaBlocks::STONE);`

### 7. AxeItem 除蜡不应播放双重音效

AxeItem 除蜡（wax-off）时仅调用 `world.playEvent(WorldEvents::WAX_OFF, pos, 0)` 即可，**不需要**额外调用 `playSound`。`WAX_OFF` 事件（ID 3004）本身已包含音效和粒子效果，额外调用 `playSound` 会导致双重音效。这与去皮（stripping）不同——去皮仅播放 `playSound(ITEM_AXE_STRIP)` 而不触发 levelEvent。参见 HoneycombItem::onItemUse 中 WAX_ON 的处理方式（仅调用 playEvent）。

### 8. AxeItem onItemUse 处理顺序

MC 原版 AxeItem::useOn 的处理顺序为：1.去皮 → 2.除蜡 → 3.去氧化。当前项目已实现去皮和除蜡，去氧化（scraping）尚未实现，需建立反向氧化链映射。
