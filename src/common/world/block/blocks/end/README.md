# 末地方块模块 (End Blocks)

末地方块模块提供末地相关方块的实现。

## 目录结构

```
end/
├── README.md              # 本文档
├── EndPortalBlock.hpp/cpp # 末地传送门、传送门框架、折跃门、紫颂植物、紫颂花、龙蛋
```

## 方块类型

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `EndPortalBlock` | 末地传送门 | 无 |
| `EndPortalFrameBlock` | 末地传送门框架 | EYE, HORIZONTAL_FACING |
| `EndGatewayBlock` | 末地折跃门 | 无 |
| `ChorusPlantBlock` | 紫颂植物 | NORTH/SOUTH/EAST/WEST/DOWN/UP |
| `ChorusFlowerBlock` | 紫颂花 | AGE_0_5 |
| `DragonEggBlock` | 龙蛋 | 无 |

## 核心机制

### 末地传送门
1. 由12个末地传送门框架组成（3x3缺角）
2. 每个框架需要放入末影之眼
3. 全部放入后激活传送门
4. 进入后传送到末地

### 紫颂植物生长
1. 从紫颂花开始生长
2. 可以向六个方向延伸
3. 破坏后掉落紫颂果
4. 紫颂花有6个年龄阶段

### 龙蛋传送
1. 点击龙蛋会传送
2. 传送到附近随机位置
3. 传送时有粒子效果

## 使用方法

```cpp
// 创建末地传送门
auto endPortal = std::make_unique<EndPortalBlock>(
    BlockProperties(Materials::PORTAL)
        .hardness(0.0f)
        .noCollision()
        .lightLevel(15)
);

// 创建传送门框架
auto endPortalFrame = std::make_unique<EndPortalFrameBlock>(
    BlockProperties(Materials::ROCK)
        .hardness(0.0f)
);

// 创建紫颂植物
auto chorusPlant = std::make_unique<ChorusPlantBlock>(
    BlockProperties(Materials::PLANTS)
        .hardness(0.0f)
        .noCollision()
);

// 创建龙蛋
auto dragonEgg = std::make_unique<DragonEggBlock>(
    BlockProperties(Materials::DRAGON_EGG)
        .hardness(0.0f)
        .lightLevel(1)
);
```

## 依赖项

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/Material` | 材质系统 |
| `world/IWorld` | 世界接口 |
| `util/property/Properties` | 方块属性 |
