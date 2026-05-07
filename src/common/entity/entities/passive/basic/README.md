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

## 继承

```
MobEntity
└── CreatureEntity
    └── AgeableEntity
        └── AnimalEntity
            ├── PigEntity
            ├── CowEntity
            ├── SheepEntity
            └── ChickenEntity
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

### 鸡 (ChickenEntity)
- 生命值：4
- 繁殖物品：种子
- 每5-10分钟下蛋
- 计时归零时会生成鸡蛋物品实体并重置计时器
- 不会摔伤（滑翔）

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
