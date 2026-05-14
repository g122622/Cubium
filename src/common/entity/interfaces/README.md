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
| `IRideable` | PigEntity, AbstractHorseEntity, StriderEntity | 猪、马类、炽足兽已实现；ride() 方法正确设置 AI 移动速度 |
| `IShearable` | SheepEntity, MooshroomEntity, SnowGolemEntity | 羊、哞菇、雪傀儡已实现 |
| `IRangedAttackMob` | SkeletonEntity, BlazeEntity, WitherEntity | 已在实体中实现attackEntityWithRangedAttack |
| `ICrossbowUser` | - | 接口已定义，待PiglinEntity/PillagerEntity实现 |
| `IFlyingAnimal` | BeeEntity, ParrotEntity | 蜜蜂、鹦鹉已实现 |
| `IJumpingMount` | AbstractHorseEntity | 马类跳跃系统已实现 |
| `IEquipable` | AbstractHorseEntity | 马类装备系统已实现（鞍槽+马铠槽） |

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
| `init(manager, boostTimeParam, saddledParam)` | 初始化数据管理器引用 |
| `syncFromDataManager()` | 从 EntityDataManager 同步数据 |
| `setSaddledFromBoolean(bool)` | 设置鞍状态（通过DataManager同步） |
| `getSaddled()` | 获取鞍状态 |
| `boost(Random&)` | 触发加速，返回是否成功 |
| `tick()` | 每tick更新加速状态 |
| `isBoosting()` | 是否正在加速 |
| `getBoostTime()` | 获取加速总时间 |
| `setBoostTime(i32)` | 设置加速时间 |

**注意**: `BoostHelper` 现在需要与 `EntityDataManager` 集成以支持网络同步。使用前必须调用 `init()` 方法初始化。

### 使用示例

```cpp
// 在可骑乘实体中使用
class PigEntity : public AnimalEntity, public IRideable {
    BoostHelper m_boostHelper;
    
    void registerData() override {
        AnimalEntity::registerData();
        m_boostTimeParam = m_dataManager.registerParam(i32(0));
        m_saddledParam = m_dataManager.registerParam(false);
        m_boostHelper.init(m_dataManager, m_boostTimeParam, m_saddledParam);
    }

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

## IRideable 速度设置逻辑

### ride() 方法

`IRideable::ride()` 方法处理骑乘实体的移动逻辑，包括速度设置：

```cpp
bool IRideable::ride(MobEntity& mount, BoostHelper& helper, const Vector3& travelVec)
{
    // ... 检查骑乘条件 ...

    if (mount.canPassengerSteer()) {
        // 获取骑乘速度（子类实现）
        f32 speed = getSteeringSpeed();

        // 加速计算（使用胡萝卜钓竿/诡异菌钓竿时）
        if (helper.saddledRaw) {
            f32 progress = helper.field_233611_b_ / helper.boostTimeRaw;
            speed += speed * 1.15f * std::sin(progress * PI);
        }

        // 设置 AI 移动速度
        mount.setAIMoveSpeed(speed);

        // 调用移动
        travelTowards(Vector3(0.0f, 0.0f, 1.0f));
    }
}
```

### 速度计算公式

**猪 (PigEntity)**:
- 基础速度：`MOVEMENT_SPEED = 0.25`
- 骑乘速度：`speed * 0.225 = 0.05625`
- 最大加速：`speed * 2.15 = 0.1209`

**炽足兽 (StriderEntity)**:
- 基础速度：`MOVEMENT_SPEED = 0.175`
- 正常骑乘速度：`speed * 0.55 = 0.09625`
- 寒冷骑乘速度：`speed * 0.23 = 0.04025`
- 正常行走速度乘数：`1.0`
- 寒冷行走速度乘数：`0.66`

### 加速因子

加速时使用正弦函数计算加速因子：
```
加速速度 = 基础速度 + 基础速度 * 1.15 * sin(progress * PI)
```

- `progress = 0` 或 `progress = 1`：无加速（sin(0) = sin(PI) = 0）
- `progress = 0.5`：最大加速（sin(0.5 * PI) = 1.0）

### 相关测试

测试文件位于 `tests/common/entity/interfaces/IRideableTest.cpp`，包含：
- 基础骑乘速度设置测试
- 加速因子计算测试
- 速度边界条件测试
- 炽足兽特殊速度测试
- BoostHelper 集成测试
