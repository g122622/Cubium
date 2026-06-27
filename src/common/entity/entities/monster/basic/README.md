# 基础怪物模块

本目录包含基础怪物实体的实现，这些怪物没有复杂的子类型或变体。

## 目录结构

```
src/common/entity/entities/monster/basic/
├── CreeperEntity.hpp/cpp   # 苦力怕：爆炸攻击、高压形态
├── SlimeEntity.hpp/cpp     # 史莱姆：分裂机制、尺寸系统（子类：MagmaCubeEntity）
├── GiantEntity.hpp/cpp     # 巨人：巨型僵尸变种、无AI
└── PhantomEntity.hpp/cpp   # 幻翼：飞行攻击、日光燃烧
```

## 内部模块关系

```
MonsterEntity（基类）
    │
    ├── CreeperEntity ─────────── 独立实现，无子类
    │
    ├── SlimeEntity ───────────── 可被继承（MagmaCubeEntity 在 nether/ 目录）
    │   └── 尺寸系统、分裂机制、弹跳动画
    │
    ├── GiantEntity ───────────── 独立实现，无子类
    │   └── 仅定义尺寸和属性，无AI行为
    │
    └── PhantomEntity ─────────── 继承 FlyingEntity（非 MonsterEntity 直接子类）
        ├── 环绕-俯冲攻击AI目标（在 ai/goal/goals/special/ 中实现）
        ├── PhantomMovementController（直接操控速度向量的飞行移动）
        ├── PhantomLookController（空操作，朝向由移动控制器控制）
        └── 客户端侧翅膀拍打音效和菌丝粒子
```

## 上下游外部依赖关系

**上游依赖**：
- `MonsterEntity` - 敌对生物基类（CreeperEntity、SlimeEntity、GiantEntity 的直接父类）
- `FlyingEntity` - 飞行生物基类（PhantomEntity 的直接父类，继承自 MobEntity）
- `MobEntity` - 生物基类，提供 isInDaylight() 日光检测
- `Attributes` - 属性系统（生命值、攻击力、移动速度等）
- `DamageSource` - 伤害系统
- `SoundEvents` - 音效事件定义
- `ParticleTypes` - 粒子类型定义
- `EntityRegistry` - 实体注册表（分裂时创建新实体）
- `SlimeGoals` - 史莱姆专用AI目标（Float、Attack、FaceRandom、Hop）

**下游依赖**：
- `nether/MagmaCubeEntity` - 继承 SlimeEntity，重写虚函数实现岩浆怪变体
- `client/` - 客户端渲染器使用粒子类型、音效等

## 容易踩的坑

### 1. 史莱姆分裂时机

分裂必须在 `remove()` 中执行，而不是 `die()` 中。因为 `die()` 时实体还未被移除，此时分裂会导致状态混乱。

```cpp
// 正确：在 remove() 中调用分裂
void SlimeEntity::remove() {
    if (canSplit() && isDead()) {
        performSplit();
    }
    MonsterEntity::remove();
}
```

### 2. SlimeEntity 子类化

SlimeEntity 提供了多个虚函数供子类（如 MagmaCubeEntity）重写：
- `getSquishParticle()` - 着地粒子类型（史莱姆：ItemSlime，岩浆怪：Flame）
- `getJumpDelay()` - 跳跃延迟（岩浆怪是史莱姆的4倍）
- `alterSquishAmount()` - 挤压动画衰减速率
- `canDamagePlayer()` - 是否可伤害玩家（岩浆怪小型也能伤害）

### 3. PhantomEntity 继承链

PhantomEntity 继承自 `FlyingEntity`，而非直接继承 `MonsterEntity`。继承链为：
```
Entity → LivingEntity → MobEntity → FlyingEntity → PhantomEntity
```
日光燃烧功能通过 `MobEntity::isInDaylight()` 实现。

### 4. GiantEntity 无AI

GiantEntity 没有注册任何AI目标，只能通过命令生成，不会自然行动。

### 5. GiantEntity 寻路权重

GiantEntity 是唯一偏好明亮区域的 Monster 子类，`getPathWeight()` 返回 `brightness - 0.5f`（而非其他怪物的 `0.5f - brightness`），与 AnimalEntity 类似但不检查草方块。

### 6. CreeperEntity 爆炸状态

苦力怕的状态使用 -1（idle）和 1（igniting/fusing），不是 0/1。获取状态时注意返回值的语义。

## 参考

- MC 1.16.5 `net.minecraft.entity.monster.SlimeEntity`
- MC 1.16.5 `net.minecraft.entity.monster.CreeperEntity`
- MC 1.16.5 `net.minecraft.entity.monster.GiantEntity`
- MC 1.16.5 `net.minecraft.entity.monster.PhantomEntity`
