# FlyingEntity - 飞行生物基类

飞行生物的基类，用于恶魂、幻翼等可以飞行的实体。

## 文件位置
- `core/FlyingEntity.hpp`
- `core/FlyingEntity.cpp`

## 继承
```
MobEntity
└── FlyingEntity
    ├── GhastEntity (恶魂)
    └── PhantomEntity (幻翼)
```

## 功能

### 飞行移动
- 不受重力影响
- 可以在空中自由移动
- 特殊的寻路逻辑

### 属性
- 飞行速度
- 飞行高度限制

## 使用示例

```cpp
class GhastEntity : public FlyingEntity {
public:
    void registerGoals() override {
        m_goalSelector.addGoal(1, std::make_unique<FlyGoal>(this));
        m_goalSelector.addGoal(2, std::make_unique<LookAtGoal>(this, Player::class, 100.0f));
        m_targetSelector.addGoal(1, std::make_unique<NearestAttackableTargetGoal<Player>>(this));
    }
};
```
