# 特殊物品模块 (Special Items)

特殊物品模块提供功能性物品的实现。

## 目录结构

```
special/
├── README.md              # 本文档
├── BoneMealItem.cpp/hpp   # 骨粉
├── BucketItem.cpp/hpp     # 桶（空桶、水桶、岩浆桶）
├── SpawnEggItem.cpp/hpp   # 生成蛋
├── FishBucketItem.cpp/hpp # 鱼桶
├── MilkBucketItem.cpp/hpp # 牛奶桶
├── NameTagItem.cpp/hpp    # 命名牌
├── SaddleItem.cpp/hpp     # 鞍
├── ShearsItem.cpp/hpp     # 剪刀
├── OnAStickItem.cpp/hpp   # 钓竿类物品基类
├── StickItems.cpp/hpp     # 具体钓竿物品（胡萝卜钓竿、诡异菌钓竿）
```

## 物品类型

| 类名 | 说明 | 实现进度 |
|------|------|----------|
| `BoneMealItem` | 骨粉（肥料，海草生成） | 完成 |
| `BucketItem` | 桶（空/水/岩浆） | 完成 |
| `SpawnEggItem` | 生成蛋 | 实体生成完成 |
| `FishBucketItem` | 鱼桶 | 完成 |
| `MilkBucketItem` | 牛奶桶 | 完成 |
| `NameTagItem` | 命名牌 | 完成 |
| `SaddleItem` | 鞍 | 完成 |
| `ShearsItem` | 剪刀 | 完成 |
| `OnAStickItem` | 钓竿类物品基类 | 完成 |
| `CarrotOnAStickItem` | 胡萝卜钓竿（控制猪） | 完成 |
| `WarpedFungusOnAStickItem` | 诡异菌钓竿（控制炽足兽） | 完成 |

## 核心机制

### OnAStickItem (MC 1.16.5)

钓竿类物品的泛型基类，用于控制可骑乘实体。

**设计模式：**
- 泛型基类：`OnAStickItem` 提供通用的加速逻辑
- 实体类型匹配：通过 `entityId` 参数指定目标实体类型
- 耐久度消耗：每次加速消耗配置的耐久度
- 损坏转换：耐久度耗尽后自动转换为钓鱼竿

**使用流程：**
1. 玩家骑乘目标实体（猪或炽足兽）
2. 实体装备鞍（`IRideable::hasSaddle() == true`）
3. 玩家手持对应的钓竿物品
4. 右键使用触发加速（`IRideable::boost()`）
5. 消耗耐久度，耐久度耗尽后转换为钓鱼竿

**核心方法：**
```cpp
class OnAStickItem : public Item {
public:
    // 右键使用，触发加速
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    // 获取目标实体类型ID
    const std::string& getEntityTypeId() const;

    // 获取每次加速消耗的耐久度
    i32 getDurabilityCost() const;

    // 附魔能力返回 1
    i32 getItemEnchantability() const override;
};
```

**参考 MC 1.16.5:**
- `net.minecraft.item.OnAStickItem`
- `net.minecraft.item.CarrotOnAStickItem`
- `net.minecraft.item.WarpedFungusOnAStickItem`

### CarrotOnAStickItem (MC 1.16.5)

胡萝卜钓竿，用于控制骑乘的猪。

**特性：**
- 耐久度：25
- 每次加速消耗：7 耐久度
- 目标实体：`minecraft:pig`
- 可用次数：3 次（25 ÷ 7 = 3，剩余 4 耐久度）
- 附魔能力：1

**与 PigEntity 的集成：**
```cpp
// PigEntity::canBeSteered() 检查玩家是否持有胡萝卜钓竿
bool PigEntity::canBeSteered() const {
    if (!hasSaddle()) return false;
    // 检查玩家主手或副手是否持有胡萝卜钓竿
    const ItemStack& mainHand = player->getHeldItem(Hand::MainHand);
    if (mainHand.getItem() == Items::CARROT_ON_A_STICK) return true;
    const ItemStack& offHand = player->getHeldItem(Hand::OffHand);
    if (offHand.getItem() == Items::CARROT_ON_A_STICK) return true;
    return false;
}
```

### WarpedFungusOnAStickItem (MC 1.16.5)

诡异菌钓竿，用于控制骑乘的炽足兽。

**特性：**
- 耐久度：100
- 每次加速消耗：1 耐久度
- 目标实体：`minecraft:strider`
- 可用次数：100 次
- 附魔能力：1

**与 StriderEntity 的集成：**
```cpp
// StriderEntity::canBeSteered() 检查玩家是否持有诡异菌钓竿
bool StriderEntity::canBeSteered() const {
    if (!hasSaddle()) return false;
    // 检查玩家主手或副手是否持有诡异菌钓竿
    const ItemStack& mainHand = player->getHeldItem(Hand::MainHand);
    if (mainHand.getItem() == Items::WARPED_FUNGUS_ON_A_STICK) return true;
    const ItemStack& offHand = player->getHeldItem(Hand::OffHand);
    if (offHand.getItem() == Items::WARPED_FUNGUS_ON_A_STICK) return true;
    return false;
}
```

### BoneMealItem (MC 1.16.5)

骨粉物品，用于加速植物生长和生成海草：

**主要功能：**
- **onItemUse()**: 对 IGrowable 方块使用，加速生长
- **applyBonemeal()**: 静态方法，应用骨粉效果
- **growSeagrass()**: 在水下生成海草（MC 1.16.5 完整实现）
- **spawnBonemealParticles()**: 生成快乐村民粒子效果

**海草生成逻辑 (MC 1.16.5 对齐)：**
- 检查目标位置是否为完整水源方块（流体等级 == 8）
- 128 次循环随机偏移位置，尝试放置海草
- 根据生物群系决定生成内容：
  - 普通海洋：生成海草
  - 温暖海洋：有机会生成珊瑚扇或墙珊瑚
- 已有海草有 10% 概率升级为高海草
- 使用 `WALL_CORALS` 和 `UNDERWATER_BONEMEALS` 方块标签

```cpp
// 对 IGrowable 方块使用骨粉
ItemStack boneMealStack(Items::BONE_MEAL, 1);
bool success = BoneMealItem::applyBonemeal(boneMealStack, world, pos, player);

// 在水下生成海草
math::Random random(world.seed());
bool placed = BoneMealItem::growSeagrass(world, pos, random);
```

**IGrowable 集成：**
- 对 `SeagrassBlock` 使用骨粉会将其变成 `TallSeagrassBlock`（高海草）
- 需要上方有水源方块才能成功
- 参考 MC 1.16.5: `net.minecraft.item.BoneMealItem.growSeagrass()`

### BucketItem (MC 1.16.5)

桶物品，用于存储和运输流体。

**类型：**
- 空桶 (`Items::BUCKET`)
- 水桶 (`Items::WATER_BUCKET`)
- 岩浆桶 (`Items::LAVA_BUCKET`)

**主要功能：**
- **onItemUse()**: 放置流体到世界
- **itemInteractionForEntity()**: 对牛使用获得牛奶桶
- **getEmptyBucket()**: 获取空桶物品

### SaddleItem (MC 1.16.5)

鞍物品，用于装备可骑乘实体。

**主要功能：**
- **onItemUseOnEntity()**: 对实体使用鞍
- 检查实体是否实现 `IEquipable` 接口
- 检查实体是否可以装备鞍（`canEquip()`）
- 设置鞍状态（`setSaddle(true)`）

**与 IRideable 的关系：**
- 鞍是骑乘控制的先决条件
- `IRideable::hasSaddle()` 返回 true 才能被控制
- `IRideable::canBeSteered()` 需要鞍 + 正确的钓竿物品

## 模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                     特殊物品模块                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐    ┌──────────────┐    ┌───────────────┐  │
│  │ OnAStickItem │◄───│CarrotOnAStick│    │WarpedFungus   │  │
│  │   (基类)     │    │    Item      │    │OnAStickItem   │  │
│  └──────┬──────┘    └──────────────┘    └───────────────┘  │
│         │                                                   │
│         │ 控制                                               │
│         ▼                                                   │
│  ┌──────────────┐                      ┌────────────────┐  │
│  │  IRideable   │◄─────────────────────│   SaddleItem   │  │
│  │  (接口)      │      装备             │                │  │
│  └──────┬───────┘                      └────────────────┘  │
│         │                                                   │
│         │ 实现                                               │
│         ▼                                                   │
│  ┌──────────────┐    ┌──────────────┐                       │
│  │  PigEntity   │    │StriderEntity │  ...                  │
│  └──────────────┘    └──────────────┘                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## 依赖关系

**内部依赖：**
- `Item` 基类（物品系统核心）
- `ItemStack`（物品堆栈）
- `ItemActionResult`（动作结果）
- `IRideable` 接口（可骑乘实体）
- `BoostHelper`（加速辅助类）
- `Items` 静态注册表

**外部依赖：**
- `Player`（玩家实体）
- `Entity`（实体基类）
- `IWorld`（世界接口）
- `Hand` 枚举（主手/副手）

## 测试用例

测试文件位于 `tests/common/item/special/OnAStickItemTest.cpp`：

- **物品注册测试**：验证钓竿物品已正确注册
- **属性测试**：验证耐久度、耐久消耗、附魔能力
- **实体类型匹配测试**：验证胡萝卜钓竿匹配猪，诡异菌钓竿匹配炽足兽
- **耐久度消耗测试**：验证使用次数计算
- **canBeSteered 集成测试**：验证有鞍无乘客、有鞍有乘客等情况

## 容易踩的坑

1. **耐久度消耗顺序**：MC 1.16.5 中，先触发加速，再消耗耐久度
2. **钓鱼竿转换**：耐久度耗尽后，物品转换为钓鱼竿，需要检查 `Items::FISHING_ROD` 是否已注册
3. **实体类型匹配**：使用字符串 ID 匹配（如 `"minecraft:pig"`），确保与实体注册 ID 一致
4. **canBeSteered 条件**：需要同时满足：有鞍 + 有乘客 + 玩家手持正确钓竿
5. **IRideable::boost()**：加速可能失败（已在加速中或没有鞍），需要检查返回值

## MC 1.16.5 参考文件

- `net.minecraft.item.OnAStickItem`：钓竿基类
- `net.minecraft.item.CarrotOnAStickItem`：胡萝卜钓竿
- `net.minecraft.item.WarpedFungusOnAStickItem`：诡异菌钓竿
- `net.minecraft.entity.passive.PigEntity`：猪实体
- `net.minecraft.entity.passive.StriderEntity`：炽足兽实体
- `net.minecraft.entity.IRideable`：可骑乘接口
