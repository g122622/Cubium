# 基础怪物模块

本目录包含基础怪物实体的实现，这些怪物没有复杂的子类型或变体。

## 目录结构

```
src/common/entity/entities/monster/basic/
├── CreeperEntity.hpp/cpp   # 苦力怕
├── SlimeEntity.hpp/cpp     # 史莱姆
├── GiantEntity.hpp/cpp     # 巨人
└── PhantomEntity.hpp/cpp   # 幻翼
```

## 怪物列表

### SlimeEntity（史莱姆）

弹跳的绿色果冻状怪物，具有独特的分裂机制。

**继承层次**：
```
Entity → LivingEntity → MobEntity → CreatureEntity → MonsterEntity → SlimeEntity
```

**特性**：
- **尺寸系统**：尺寸范围 1-4（1=微小，2=小，4=大）
- **分裂机制**：死亡时分裂成 2-4 个小史莱姆
- **弹跳移动**：持续弹跳移动
- **攻击行为**：近战攻击玩家（仅非小尺寸）

**核心方法**：

| 方法 | 说明 |
|------|------|
| `setSlimeSize(size, resetHealth)` | 设置史莱姆尺寸（1-4），同步更新属性和经验值 |
| `getSlimeSize()` | 获取当前尺寸 |
| `isSmallSlime()` | 是否为小史莱姆（尺寸≤1） |
| `canSplit()` | 是否可以分裂（尺寸>1） |
| `performSplit()` | 执行分裂逻辑（在 remove() 中调用） |
| `dealDamage(target)` | 对目标造成近战伤害 |
| `onCollideWithPlayer(player)` | 玩家碰撞处理 |
| `getSquishSound()` | 获取挤压音效 |
| `getJumpSound()` | 获取跳跃音效 |

**尺寸与属性对应**（MC 1.16.5）：

| 尺寸 | 生命值 | 移动速度 | 攻击伤害 | 经验值 |
|------|--------|----------|----------|--------|
| 1 | 1 | 0.3 | 1 | 1 |
| 2 | 4 | 0.4 | 2 | 2 |
| 4 | 16 | 0.6 | 4 | 4 |

**分裂机制**：
- 触发条件：`remove()` 时尺寸 > 1 且已死亡
- 分裂数量：2-4 个随机
- 小史莱姆尺寸：原尺寸 / 2
- 继承属性：自定义名称、无敌状态
- 位置偏移：`(i % 2 - 0.5) * (size / 4.0)`

**声音系统**：
- 小史莱姆：使用 `_small` 后缀音效（hurt_small, death_small, squish_small）
- 大史莱姆：使用标准音效

### CreeperEntity（苦力怕）

会爆炸的敌对生物。

### GiantEntity（巨人）

巨型僵尸变种。

### PhantomEntity（幻翼）

夜间飞行的亡灵生物。

## 使用示例

```cpp
// 创建史莱姆
auto slime = std::make_unique<SlimeEntity>(LegacyEntityType::Slime, EntityId(0));
slime->setWorld(&world);
slime->setSlimeSize(4, true);  // 设置为大史莱姆

// 检查属性
i32 size = slime->getSlimeSize();          // 4
bool canSplit = slime->canSplit();          // true
bool isSmall = slime->isSmallSlime();       // false

// 攻击逻辑
if (slime->canDamagePlayer()) {
    slime->dealDamage(player);
}
```

## 依赖关系

```
MonsterEntity（基类）
    ├── attribute/Attributes.hpp    # 属性系统
    ├── damage/DamageSource.hpp     # 伤害系统
    ├── world/IWorld.hpp            # 世界接口
    ├── sound/SoundEvents.hpp       # 音效事件
    └── util/math/Random.hpp        # 随机数生成
```

## 测试用例

- `tests/common/entity/entities/monster/SlimeEntityTest.cpp`
  - 尺寸系统测试
  - 分裂机制测试
  - 声音系统测试
  - 维度与碰撞箱测试
  - 伤害与经验值测试

## 容易踩的坑

### 1. 史莱姆分裂时机

```cpp
// 错误：在 die() 中调用分裂
void SlimeEntity::die(DamageSource& source) {
    MonsterEntity::die(source);
    performSplit();  // 错误！实体还未被移除
}

// 正确：在 remove() 中调用分裂
void SlimeEntity::remove() {
    if (canSplit() && isDead()) {
        performSplit();
    }
    MonsterEntity::remove();
}
```

### 2. 经验值更新

```cpp
// 错误：只在 setSlimeSize 中更新经验值
void setSlimeSize(i32 size, bool resetHealth) {
    m_size = size;
    // 经验值更新...
}
// 问题：registerAttributes() 直接设置 m_size = 1 绕过了 setSlimeSize

// 正确：在 updateSizeAttributes() 中更新经验值
void updateSizeAttributes() {
    // 更新属性...
    setExperienceValue(m_size);  // 确保经验值同步
}
```

### 3. Entity::remove() 虚函数

```cpp
// 注意：Entity::remove() 现在是虚函数
// 子类重写时必须调用基类方法
void SlimeEntity::remove() override {
    // 自定义逻辑...
    MonsterEntity::remove();  // 必须调用基类
}
```

## 参考

- MC 1.16.5 `net.minecraft.entity.monster.SlimeEntity`
- MC 1.16.5 `net.minecraft.entity.monster.CreeperEntity`
- MC 1.16.5 `net.minecraft.entity.monster.GiantEntity`
- MC 1.16.5 `net.minecraft.entity.monster.PhantomEntity`
