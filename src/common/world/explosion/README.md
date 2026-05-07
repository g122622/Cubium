# 爆炸系统 (Explosion System)

## 概述

本模块实现了完整的 Minecraft 1.16.5 风格的爆炸系统，包括：

- **方块破坏**：使用射线追踪算法计算受影响的方块
- **实体伤害**：基于距离和遮挡计算伤害
- **击退效果**：根据爆炸强度应用击退
- **粒子和音效**：生成爆炸视觉效果和声音
- **爆炸模式**：支持 NONE/BREAK/DESTROY 三种模式
- **附魔保护**：爆炸保护附魔减少伤害和击退
- **火焰生成**：在条件满足时生成火焰

## 目录结构

```
explosion/
├── ExplosionMode.hpp      # 爆炸模式枚举
├── ExplosionContext.hpp   # 爆炸上下文基类
├── ExplosionContext.cpp
├── Explosion.hpp          # 爆炸核心类
├── Explosion.cpp
└── README.md              # 本文档
```

## 核心类

### ExplosionMode

定义爆炸对方块的影响方式：

```cpp
enum class ExplosionMode : u8 {
    None,    // 仅造成伤害和击退，不破坏方块
    Break,   // 破坏方块但不掉落物品（TNT 默认模式）
    Destroy  // 破坏方块并掉落物品（苦力怕默认模式）
};
```

### ExplosionContext

爆炸上下文基类，用于自定义爆炸行为：

```cpp
class ExplosionContext {
public:
    // 获取方块的爆炸抗性（考虑流体）
    virtual std::optional<f32> getExplosionResistance(
        const BlockState& blockState,
        const fluid::FluidState* fluidState) const;

    // 判断方块是否可被破坏
    virtual bool canDestroyBlock(
        const BlockState& blockState,
        f32 explosionPower) const;
};

// 实体相关的爆炸上下文
class EntityExplosionContext : public ExplosionContext {
    // 允许实体自定义爆炸行为（如凋灵之首、TNT 矿车）
};
```

### Explosion

爆炸核心类，实现完整的爆炸流程：

```cpp
class Explosion {
public:
    Explosion(IWorld& world,
              const Vector3& position,
              f32 radius,
              ExplosionMode mode = ExplosionMode::Destroy,
              bool causesFire = false,
              Entity* source = nullptr,
              std::unique_ptr<DamageSource> damageSource = nullptr);

    void explode();  // 执行爆炸

    // 结果查询
    const std::vector<BlockPos>& affectedBlocks() const;
    const std::unordered_map<u64, Vector3>& playerKnockback() const;
};
```

## 爆炸算法

### 方块破坏（射线追踪）

1. 从爆炸中心发射 16×16×16 立方体表面的射线（共 1352 条）
2. 每条射线步进 0.3 格
3. 初始强度 = radius × (0.7 + random × 0.6)
4. 每步衰减 = (resistance + 0.3) × 0.3
5. 强度 > 0 时标记方块为受影响
6. **流体影响**：水和岩浆具有 100.0 的爆炸抗性

### 实体伤害

1. 影响范围 = radius × 2
2. 计算距离系数 = distance / (radius × 2)
3. 计算阻挡密度（视线检测）
   - 使用射线追踪检测实体与爆炸中心之间的方块阻挡
   - 采样实体碰撞箱内的多个点
4. 伤害 = floor((impact² + impact) / 2 × 7 × radius + 1)
5. 击退 = 归一化方向向量 × impact
6. **爆炸保护附魔**：
   - 减少伤害：damage × (1 - min(EPF, 20) / 25)
   - 减少击退：knockback × (1 - EPF × 0.15)

### 火焰生成

1. 仅在 causesFire = true 时触发
2. 1/3 概率在破坏的方块位置生成火焰
3. 前提条件：下方方块必须是不透明固体方块

## 使用方法

### 创建爆炸

```cpp
// 通过世界接口创建爆炸
world.createExplosion(
    Vector3(x, y, z),    // 爆炸中心
    4.0f,                // 爆炸半径
    ExplosionMode::Break, // 爆炸模式
    false,               // 是否生成火焰
    tntEntity            // 爆炸源实体
);
```

### 自定义爆炸行为

```cpp
// 创建自定义爆炸上下文
class CustomExplosionContext : public ExplosionContext {
public:
    std::optional<f32> getExplosionResistance(
        const BlockState& blockState,
        const fluid::FluidState* fluidState) const override {
        // 例如：凋灵之首可以破坏更高抗性的方块
        return ExplosionContext::getExplosionResistance(blockState, fluidState);
    }

    bool canDestroyBlock(
        const BlockState& blockState,
        f32 explosionPower) const override {
        // 例如：TNT 矿车不破坏铁轨
        return true;
    }
};
```

### 方块爆炸响应

```cpp
class MyBlock : public Block {
public:
    // 获取爆炸抗性
    f32 getExplosionResistance(const BlockState& state) const override {
        return 10.0f;  // 高抗性
    }

    // 是否掉落物品
    bool canDropFromExplosion(const BlockState& state) const override {
        return false;  // 爆炸时不掉落
    }

    // 爆炸回调
    void onBlockExploded(IWorld& world, const BlockPos& pos, const BlockState& state) const override {
        // 例如：TNT 方块被爆炸时点燃
    }
};
```

## 爆炸半径参考

| 实体 | 半径 | 模式 |
|------|------|------|
| TNT | 4.0 | Break |
| 苦力怕（普通）| 3.0 | Destroy |
| 苦力怕（高压）| 6.0 | Destroy |
| 恶魂火球 | 1.0 | Destroy |
| 凋灵之首 | 1.0 | Destroy |
| 凋灵（召唤）| 7.0 | Destroy |
| 末地水晶 | 6.0 | Destroy |
| 床（其他维度）| 5.0 | Destroy |

## 与其他系统的集成

### 粒子系统

爆炸会自动生成以下粒子：
- 半径 >= 2.0：`HugeExplosion`（大爆炸发射器）
- 半径 < 2.0：`Explosion`（普通爆炸粒子）

### 音效系统

播放 `minecraft:entity.generic.explode` 音效。

### 伤害系统

使用 `DamageSources::explosion(source)` 创建爆炸伤害。
支持爆炸保护附魔减少伤害和击退。

### 方块掉落

通过 `BlockDropHandler::generateDrops()` 生成掉落物。
调用 `Block::canDropFromExplosion()` 判断是否掉落。

## 注意事项

1. **性能考虑**：大规模爆炸可能影响性能，应考虑限制同时进行的爆炸数量
2. **服务端/客户端同步**：爆炸在服务端计算，结果广播给客户端
3. **游戏规则**：应检查 `mobGriefing` 规则以决定是否破坏方块
4. **方块实体**：被破坏的方块实体（如箱子）应正确处理其内容物
5. **实体免疫**：实体可通过 `isImmuneToExplosions()` 方法免疫爆炸伤害
