# 普通动物

基础被动动物实体。

## 文件列表

| 文件 | 说明 |
|------|------|
| AnimalEntity.hpp/cpp | 动物基类 |
| PigEntity.hpp/cpp | 猪 |
| CowEntity.hpp/cpp | 牛 |
| SheepEntity.hpp/cpp | 羊 |
| ChickenEntity.hpp/cpp | 鸡 |
| MooshroomEntity.hpp/cpp | 哞菇 |

## 继承

```
MobEntity
└── CreatureEntity
    └── AgeableEntity
        └── AnimalEntity
            ├── PigEntity
            ├── CowEntity
            ├── SheepEntity
            ├── ChickenEntity
            └── MooshroomEntity (implements IShearable)
```

## 动物特性

### 猪 (PigEntity)
- 生命值：10
- 繁殖物品：胡萝卜、马铃薯、甜菜根
- 可骑乘（鞍 + 胡萝卜钓竿）

### 牛 (CowEntity)
- 生命值：10
- 繁殖物品：小麦
- 可挤奶（空桶 → 牛奶桶）

### 羊 (SheepEntity)
- 生命值：8
- 繁殖物品：小麦
- 可剪羊毛（剪刀 → 1-3个对应颜色羊毛）
- 16种颜色（通过 DyeColor 枚举）
- 实现 IShearable 接口
- 剪毛后需吃草重新长出羊毛
- **颜色混合**：繁殖时幼羊颜色由父母颜色混合决定
  - 白色 + 红色 = 粉红色
  - 红色 + 黄色 = 橙色
  - 白色 + 蓝色 = 淡蓝色
  - 蓝色 + 绿色 = 青色
  - 蓝色 + 红色 = 紫色
  - 白色 + 绿色 = 黄绿色
  - 白色 + 黑色 = 灰色
  - 灰色 + 白色 = 淡灰色
  - 无配方时随机选择父母颜色
- **吃草行为**：通过 EatGrassGoal 实现
  - 概率触发（幼年 1/50，成年 1/1000）
  - 吃草后重新长出羊毛（如果被剪过）
  - 幼羊吃草加速成长 60 ticks

### 鸡 (ChickenEntity)
- 生命值：4
- 繁殖物品：种子
- 每5-10分钟下蛋
- 计时归零时会生成鸡蛋物品实体并重置计时器
- 不会摔伤（滑翔）

### 哞菇 (MooshroomEntity)
- 生命值：10
- 繁殖物品：小麦
- 两种皮肤：红色哞菇（默认）、棕色哞菇
- 可剪羊毛：使用剪刀获得蘑菇并变成普通牛（实现 IShearable 接口）
- 可用空碗获取蘑菇汤
- **雷击转换**（MC 1.16.5）：
  - 红色哞菇 + 雷击 → 棕色哞菇
  - 棕色哞菇 + 雷击 → 红色哞菇
  - 播放 `ENTITY_MOOSHROOM_CONVERT` 音效（音量 2.0，音调 1.0）
  - 客户端生成 20 个 `Explosion` 粒子（环形分布，位于实体碰撞箱内）

## 繁殖系统

```cpp
// 喂食
if (animal->isBreedingItem(item) && !animal->isInLove()) {
    animal->setInLove(playerId);
    item.shrink(1);
}

// 繁殖
if (animal1->isInLove() && animal2->isInLove() && animal1->canMateWith(animal2)) {
    auto baby = animal1->spawnBaby(animal2);
    world.spawnEntity(std::move(baby));
    animal1->resetLove();
    animal2->resetLove();
}
```

## 测试用例

| 文件 | 说明 |
|------|------|
| `tests/common/entity/entities/passive/basic/ChickenEntityTest.cpp` | 验证鸡蛋生成与计时器重置 |
| `tests/common/entity/entities/passive/basic/RabbitEntityTest.cpp` | 兔子行为测试 |
| `tests/common/entity/entities/passive/basic/MooshroomEntityTest.cpp` | 哞菇类型系统、雷击转换、音效播放、粒子生成测试 |
| `tests/entity/EatGrassGoalTest.cpp` | 验证羊颜色混合和吃草行为 |
