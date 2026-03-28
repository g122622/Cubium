# 珊瑚方块模块 (Coral Blocks)

珊瑚方块模块提供水下珊瑚类方块的实现。

## 目录结构

```
coral/
├── README.md           # 本文档
├── CoralBlock.hpp/cpp  # 珊瑚方块（水下固体方块）
```

## 方块类型

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `CoralBlock` | 珊瑚块（水下，离开水变死珊瑚） | WATERLOGGED |
| `CoralFanBlock` | 珊瑚扇（地面放置） | WATERLOGGED, HORIZONTAL_FACING |
| `CoralWallFanBlock` | 墙珊瑚扇（墙面放置） | WATERLOGGED, FACING |
| `CoralBlockBlock` | 珊瑚块（固体，不死亡） | 无 |

## 珊瑚颜色

```cpp
enum class CoralColor : u8 {
    Tube = 0,      // 管状珊瑚（蓝色）
    Brain = 1,     // 脑珊瑚（粉色）
    Bubble = 2,    // 气泡珊瑚（紫色）
    Fire = 3,      // 火焰珊瑚（红色）
    Horn = 4       // 角珊瑚（黄色）
};
```

## 核心机制

### 珊瑚死亡
1. 检查是否在水中（WATERLOGGED 或周围有水）
2. 如果离开水，变成死珊瑚（灰色版本）
3. 死珊瑚不能恢复

### 放置规则
1. 珊瑚扇需要附着在固体表面
2. 墙珊瑚扇只能附着在水平墙面
3. 珊瑚块可以独立放置

## 使用方法

```cpp
// 创建蓝色珊瑚
auto tubeCoral = std::make_unique<CoralBlock>(
    CoralColor::Tube,
    deadTubeCoralBlockId,  // 死珊瑚方块ID
    BlockProperties(Materials::CORAL)
        .hardness(0.0f)
        .noCollision()
);

// 创建珊瑚扇
auto tubeCoralFan = std::make_unique<CoralFanBlock>(
    CoralColor::Tube,
    deadTubeCoralFanBlockId,
    BlockProperties(Materials::CORAL)
        .hardness(0.0f)
        .noCollision()
);

// 创建珊瑚块
auto tubeCoralBlock = std::make_unique<CoralBlockBlock>(
    CoralColor::Tube,
    BlockProperties(Materials::CORAL)
        .hardness(1.5f)
        .resistance(6.0f)
);
```

## 依赖项

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/Material` | 材质系统 |
| `world/IWorld` | 世界接口 |
| `util/property/Properties` | 方块属性 |
