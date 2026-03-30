# Entity Interfaces

实体接口模块，定义实体行为能力的抽象接口。

## 接口列表

| 接口 | 说明 | 实现者示例 |
|------|------|-----------|
| `IAngerable` | 愤怒接口，可记住攻击者 | 狼、铁傀儡、末影人、僵尸猪灵 |
| `IRideable` | 可骑乘接口 | 猪、炽足兽 |
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

## 依赖关系

```
ICrossbowUser
    └── extends IRangedAttackMob

其他接口相互独立
```
