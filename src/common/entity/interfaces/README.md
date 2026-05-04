# Entity Interfaces

实体接口模块，定义实体行为能力的抽象接口。

## 接口列表

| 接口 | 说明 | 实现者示例 |
|------|------|-----------|
| `IAngerable` | 愤怒接口，可记住攻击者 | 狼、铁傀儡、末影人、僵尸猪灵 |
| `IRideable` | 可骑乘接口 | 猪、炽足兽、马类 |
| `IShearable` | 可剪毛接口 | 羊、雪傀儡、哞菇 |
| `IRangedAttackMob` | 远程攻击接口 | 骷髅、烈焰人、女巫 |
| `ICrossbowUser` | 弩使用者接口 | 掠夺者、猪灵 |
| `IFlyingAnimal` | 飞行动物接口 | 蜜蜂、鹦鹉 |
| `IJumpingMount` | 可跳跃骑乘接口 | 马、驴、骡、羊驼 |
| `IEquipable` | 可装备接口 | 猪、马、驴 |

## 设计原则

1. **接口分离原则**：每个接口只描述一种行为能力
2. **单一职责**：接口方法只与该能力相关
3. **默认实现**：提供合理的默认实现，减少子类负担

## 命名空间

所有接口定义在 `mc::entity` 命名空间下。

## 使用示例

```cpp
// 羊实现可剪毛接口
class SheepEntity : public AnimalEntity, public IShearable {
public:
    bool isShearable() const override {
        return m_hasWool && m_shearCooldown <= 0;
    }

    std::vector<ItemStack> shear(Player* player) override {
        m_hasWool = false;
        m_shearCooldown = 100; // 5秒冷却
        return { ItemStack(Items::WOOL, m_woolColor, 1 + m_rng.nextInt(3)) };
    }
};

// 狼实现愤怒接口
class WolfEntity : public TameableEntity, public IAngerable {
public:
    void setAttackTarget(LivingEntity* target) override {
        m_attackTarget = target;
        setAngry(target != nullptr);
    }

    bool isAngry() const override {
        return m_angryTime > 0;
    }
};
```

## 接口实现状态

| 接口 | 已实现者 | 备注 |
|------|----------|------|
| `IAngerable` | TameableEntity, GolemEntity, EndermanEntity | 狼、铁傀儡、末影人已正确实现 |
| `IRideable` | PigEntity, AbstractHorseEntity, StriderEntity | 猪、马类、炽足兽已实现 |
| `IShearable` | SheepEntity, MooshroomEntity, SnowGolemEntity | 羊、哞菇、雪傀儡已实现 |
| `IRangedAttackMob` | SkeletonEntity, BlazeEntity, WitherEntity | 已在实体中实现attackEntityWithRangedAttack |
| `ICrossbowUser` | - | 接口已定义，待PiglinEntity/PillagerEntity实现 |
| `IFlyingAnimal` | BeeEntity, ParrotEntity | 蜜蜂、鹦鹉已实现 |
| `IJumpingMount` | AbstractHorseEntity | 马类跳跃系统已实现 |
| `IEquipable` | - | 接口已定义，待实现 |

## 目录结构

```
interfaces/
├── IAngerable.hpp         # 愤怒接口
├── IRideable.hpp          # 可骑乘接口
├── IShearable.hpp         # 可剪毛接口
├── IRangedAttackMob.hpp   # 远程攻击接口
├── ICrossbowUser.hpp      # 弩使用者接口
├── IFlyingAnimal.hpp      # 飞行动物接口
├── IJumpingMount.hpp      # 可跳跃骑乘接口
├── IEquipable.hpp         # 可装备接口
└── README.md              # 本文件
```

## 依赖关系

```
IAngerable
    └── 需要: LivingEntity (前向声明)

IRideable
    └── 需要: Player (前向声明)

IShearable
    └── 需要: Player, ItemStack (前向声明)

IRangedAttackMob
    └── 需要: LivingEntity (前向声明)

ICrossbowUser (继承自 IRangedAttackMob)
    └── 需要: LivingEntity (前向声明)

IFlyingAnimal
    └── 无外部依赖

IJumpingMount
    └── 无外部依赖

IEquipable
    └── 需要: ItemStack (前向声明)
```
## BoostHelper 辅助类

`BoostHelper` 位于 `entity/core/BoostHelper.hpp`，管理可骑乘实体的鞍和加速状态。

### 功能

| 方法 | 说明 |
|------|------|
| `setSaddled(bool)` | 设置鞍状态 |
| `getSaddled()` | 获取鞍状态 |
| `boost(Random&)` | 触发加速，返回是否成功 |
| `tick()` | 每tick更新加速状态 |
| `isBoosting()` | 是否正在加速 |
| `getBoostProgress()` | 获取加速进度 (0.0-1.0) |
| `getBoostTime()` | 获取加速总时间 |
| `setBoostTime(i32)` | 设置加速时间 |

### 使用示例

```cpp
// 在可骑乘实体中使用
class PigEntity : public AnimalEntity, public IRideable {
    BoostHelper m_boostHelper;

    bool boost() override {
        math::Random rng = getRandom();
        return m_boostHelper.boost(rng);
    }

    void tick() override {
        AnimalEntity::tick();
        m_boostHelper.tick();
    }
};
```

## 最新补充
- 已补 `IFlinging.hpp/.cpp`，对齐 1.16.5 Hoglin / Zoglin 的撞飞型近战公共接口。
- `IFlinging` 当前提供最小公共语义：攻击动画 tick 查询、成年个体的撞飞辅助、基于攻击伤害与击退属性的命中逻辑。
- `BoostHelper` 用于猪和炽足兽的加速控制，与胡萝卜钓竿配合使用。
