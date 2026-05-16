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
| `tick()` | 更新实体状态（含着地粒子生成） |

**虚函数**（供子类重写，如 `MagmaCubeEntity`）：

| 虚函数 | 说明 | 史莱姆默认实现 |
|--------|------|----------------|
| `setSlimeSize(size, resetHealth)` | 设置尺寸，子类可添加额外逻辑 | 更新属性（生命值、速度、伤害） |
| `canDamagePlayer()` | 是否可伤害玩家 | `!isSmallSlime() && isServerWorld()` |
| `getJumpDelay()` | 获取跳跃延迟（tick） | 10-30 随机值 |
| `alterSquishAmount()` | 更新挤压动画 | 挤压量 *= 0.6 |
| `getSquishParticle()` | 获取着地粒子类型 | `ParticleTypeId::ItemSlime` |
| `getSquishSound()` | 获取挤压声音 | 根据尺寸返回不同音效 |
| `getJumpSound()` | 获取跳跃声音 | 返回挤压声音 |
| `getHurtSound()` | 获取受伤声音 | 根据尺寸返回不同音效 |
| `getDeathSound()` | 获取死亡声音 | 根据尺寸返回不同音效 |

**子类化示例**：

参见 `src/common/entity/entities/monster/nether/` 目录下的 `MagmaCubeEntity`，它继承自 `SlimeEntity` 并重写了多个虚函数：

```cpp
class MagmaCubeEntity : public SlimeEntity {
public:
    // 重写跳跃延迟（岩浆怪是史莱姆的4倍）
    i32 getJumpDelay() const override {
        return SlimeEntity::getJumpDelay() * 4;
    }
    
    // 重写挤压动画衰减（更慢）
    void alterSquishAmount() override {
        setSquishAmount(squishAmount() * 0.9f);  // 史莱姆为 0.6
    }
    
    // 重写粒子类型（火焰粒子）
    ParticleTypeId getSquishParticle() const override {
        return ParticleTypeId::Flame;
    }
    
    // 重写伤害判断（小型也能伤害玩家）
    bool canDamagePlayer() const override {
        return world() != nullptr && !world()->isClientSide();
    }
};
```

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

**着地粒子效果**（MC 1.16.5）：
- 触发条件：`onGround()` 从 `false` 变为 `true`（首次着地）
- 粒子类型：`ParticleTypeId::ItemSlime`
- 粒子数量：`size * 8`（尺寸 1 = 8 个，尺寸 4 = 32 个）
- 粒子位置：实体脚底，环形分布
- 位置计算：
  ```cpp
  f32 angle = random.nextFloat() * 2.0f * PI;
  f32 radiusFactor = random.nextFloat() * 0.5f + 0.5f;
  f32 offsetX = sin(angle) * size * 0.5f * radiusFactor;
  f32 offsetZ = cos(angle) * size * 0.5f * radiusFactor;
  ```
- 仅客户端生成：`world()->isClientSide()` 检查

### CreeperEntity（苦力怕）

会爆炸的敌对生物。

### GiantEntity（巨人）

巨型僵尸变种。

### PhantomEntity（幻翼）

夜间飞行的亡灵生物。

**特性**：
- **日光燃烧**：白天暴露在阳光下会燃烧
- **飞行能力**：继承自 FlyingEntity，不受重力影响
- **俯冲攻击**：环绕目标后俯冲攻击
- **尺寸变体**：有不同大小的变体

**日光检测逻辑**（MC 1.16.5）：
```cpp
// PhantomEntity::tick()
if (isAlive() && isInDaylight()) {
    setFire(8);  // 燃烧8秒
}
```

**isInDaylight() 方法**：
- 位于 `MobEntity` 基类（幻翼继承链：FlyingEntity → MobEntity）
- 检查条件：世界为白天 → 亮度 > 0.5 → 随机检查 → 天空可见

**AI 目标系统**（MC 1.16.5）：
| 优先级 | 目标类型 | 目标类 | 说明 |
|--------|----------|--------|------|
| 1 | 目标选择 | PhantomAttackPlayerTargetGoal | 寻找 64 格内的玩家作为攻击目标 |
| 1 | 攻击阶段 | PhantomPickAttackGoal | 在环绕和俯冲阶段之间切换 |
| 2 | 俯冲攻击 | PhantomSweepAttackGoal | 执行俯冲攻击，撞击目标造成伤害 |
| 3 | 环绕飞行 | PhantomOrbitPointGoal | 在目标上方环绕飞行 |

**环绕攻击机制**：
- **环绕阶段（CIRCLE）**：在目标上方环绕，等待攻击机会
- **俯冲阶段（SWOOP）**：向目标俯冲，撞击造成伤害
- 环绕参数：半径 5-15 格，高度偏移 -4 到 5 格
- 猫会驱赶幻翼，导致俯冲攻击中断

**核心方法**：

| 方法 | 说明 |
|------|------|
| `getPhantomSize()` | 获取幻翼尺寸（0-64） |
| `setPhantomSize(size)` | 设置幻翼尺寸，更新攻击力和碰撞箱 |
| `getAttackPhase()` | 获取攻击阶段（CIRCLE/SWOOP） |
| `setAttackPhase(phase)` | 设置攻击阶段 |
| `orbitPosition()` | 获取环绕位置 |
| `setOrbitPosition(pos)` | 设置环绕位置 |
| `orbitOffset()` | 获取环绕偏移向量 |
| `setOrbitOffset(offset)` | 设置环绕偏移向量 |

**实现状态**：
| 功能 | 状态 |
|------|------|
| 日光燃烧 | ✅ 已实现 |
| 尺寸系统 | ✅ 已实现 |
| AI 目标 | ✅ 已实现 |

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
  - 着地粒子效果测试（客户端粒子生成、数量与尺寸关系、粒子类型验证）
- `tests/common/entity/PhantomGoalsTest.cpp`
  - 幻翼实体状态测试
  - PhantomAttackPlayerTargetGoal 测试
  - PhantomOrbitPointGoal 测试
  - PhantomPickAttackGoal 测试
  - PhantomSweepAttackGoal 测试
  - 幻翼 AI 目标集成测试

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
