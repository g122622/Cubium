# 下界方块模块 (Nether Blocks)

下界方块模块提供下界相关方块的实现。

## 目录结构

```
nether/
├── README.md           # 本文档
├── FireBlock.hpp/cpp   # 火、灵魂火、下界传送门、下界疣
```

## 方块类型

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `FireBlock` | 普通火焰，可蔓延 | AGE_0_15, NORTH/SOUTH/EAST/WEST/UP |
| `SoulFireBlock` | 灵魂火焰（蓝色，更高伤害） | 同 FireBlock |
| `NetherPortalBlock` | 下界传送门 | HORIZONTAL_AXIS |
| `NetherWartBlock` | 下界疣（可生长） | AGE_0_3 |

## 核心机制

### 火焰蔓延
1. 火焰有年龄（AGE_0_15）
2. 年龄越大越稳定，越不容易熄灭
3. 可以蔓延到周围可燃方块
4. 检查周围是否有可燃物

### 下界传送门
1. 由黑曜石框架组成
2. 通过点火激活
3. 实体碰撞后传送
4. 水平轴向（X 或 Z）

### 下界疣生长
1. 只能种在灵魂沙上
2. 4个生长阶段（AGE_0_3）
3. 随机 tick 生长

## 使用方法

```cpp
// 创建火焰
auto fire = std::make_unique<FireBlock>(
    BlockProperties(Materials::FIRE)
        .hardness(0.0f)
        .noCollision()
        .lightLevel(15)
);

// 创建灵魂火
auto soulFire = std::make_unique<SoulFireBlock>(
    BlockProperties(Materials::FIRE)
        .hardness(0.0f)
        .noCollision()
        .lightLevel(10)
);

// 创建下界传送门
auto portal = std::make_unique<NetherPortalBlock>(
    BlockProperties(Materials::PORTAL)
        .hardness(0.0f)
        .noCollision()
        .lightLevel(11)
);

// 创建下界疣
auto netherWart = std::make_unique<NetherWartBlock>(
    BlockProperties(Materials::PLANTS)
        .hardness(0.0f)
        .noCollision()
);
```

## 依赖项

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/Material` | 材质系统 |
| `world/IWorld` | 世界接口 |
| `util/property/Properties` | 方块属性 |
