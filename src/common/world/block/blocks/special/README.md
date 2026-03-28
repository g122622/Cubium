# 特殊方块模块 (Special Blocks)

特殊方块模块提供管理、装饰和特殊功能方块的实现。

## 目录结构

```
special/
├── README.md            # 本文档
├── SpecialBlocks.hpp/cpp # 所有特殊方块
```

## 方块类型

### 管理类方块

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `BarrierBlock` | 屏障（不可见不可破坏） | 无 |
| `StructureVoidBlock` | 结构空位 | 无 |
| `StructureBlock` | 结构方块 | MODE |
| `JigsawBlock` | 拼图方块 | ORIENTATION |

### 命令方块

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `CommandBlock` | 脉冲命令方块 | FACING, CONDITIONAL, POWERED |
| `RepeatingCommandBlock` | 循环命令方块 | 同上 |
| `ChainCommandBlock` | 连锁命令方块 | 同上 |

### 物理方块

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `SlimeBlock` | 粘液块（弹跳） | 无 |
| `HoneyBlock` | 蜂蜜块（粘滞） | 无 |

### 功能方块

| 类名 | 说明 | 状态属性 |
|------|------|----------|
| `SpongeBlock` | 海绵（吸水） | 无 |
| `WetSpongeBlock` | 湿润海绵 | 无 |

## 核心机制

### 屏障方块
- 完全不透明但不可见
- 只有创造模式可见轮廓
- 不可破坏（生存模式）

### 命令方块
- 脉冲：单次执行
- 循环：每 tick 执行
- 连锁：前方命令后执行
- 可设置条件执行

### 粘液块
- 实体落在上面弹跳
- 活塞推动时粘住方块
- 弹跳高度可调整

### 蜂蜜块
- 实体在上面减速
- 活塞推动时粘住方块
- 无弹跳效果

### 海绵
- 可吸收半径内的水
- 吸水后变成湿润海绵
- 可在熔炉烤干

## 使用方法

```cpp
// 创建屏障
auto barrier = std::make_unique<BarrierBlock>(
    BlockProperties(Materials::BARRIER)
        .hardness(-1.0f)  // 不可破坏
);

// 创建命令方块
auto commandBlock = std::make_unique<CommandBlock>(
    BlockProperties(Materials::REDSTONE_LIGHT)
        .hardness(-1.0f)
);

// 创建粘液块
auto slime = std::make_unique<SlimeBlock>(
    BlockProperties(Materials::CLAY)
        .hardness(0.0f)
        .resistance(0.0f)
);

// 创建海绵
auto sponge = std::make_unique<SpongeBlock>(
    BlockProperties(Materials::SPONGE)
        .hardness(0.6f)
);
```

## 依赖项

| 模块 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/block/Material` | 材质系统 |
| `world/IWorld` | 世界接口 |
| `util/property/Properties` | 方块属性 |
