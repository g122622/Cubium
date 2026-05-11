# 特殊动物

包含特殊行为的被动/中立动物。

## 目录结构

```
special/
├── BeeEntity.hpp/cpp         # 蜜蜂
├── FoxEntity.hpp/cpp         # 狐狸
├── PandaEntity.hpp/cpp       # 熊猫
├── PolarBearEntity.hpp/cpp   # 北极熊
├── StriderEntity.hpp/cpp     # 炽足兽
├── TurtleEntity.hpp/cpp      # 海龟
└── README.md                 # 本文档
```

## 实体列表

| 实体 | 说明 | 接口 | 状态 |
|------|------|------|------|
| BeeEntity | 蜜蜂 | IFlyingAnimal | ✅ 完成 |
| FoxEntity | 狐狸 | - | ✅ 完成 |
| PandaEntity | 熊猫 | - | ✅ 完成 |
| PolarBearEntity | 北极熊 | - | ✅ 完成 |
| StriderEntity | 炽足兽 | IRideable | ✅ 完成 |
| TurtleEntity | 海龟 | - | ✅ 完成 |

## 蜜蜂 (BeeEntity)

### 特性
- 飞行行为（实现 `IFlyingAnimal`）
- 授粉机制
- 蜂巢记忆
- 群体攻击
- 螫刺后死亡

### 行为
| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 在水中游泳 |
| 1 | AttackGoal | 攻击目标 |
| 2 | EnterBeehiveGoal | 进入蜂巢 |
| 3 | FindPollinationTargetGoal | 寻找授粉目标 |
| 4 | FindBeehiveGoal | 寻找蜂巢 |
| 5 | TemptGoal | 被花朵诱惑 |
| 6 | FollowParentGoal | 幼体跟随父母 |

## 狐狸 (FoxEntity)

### 特性
- **皮肤类型**: 红色狐狸 (Red) 和雪地狐狸 (Snow) 两种变体
- **信任机制**: 最多信任 2 个玩家，信任玩家不会触发逃跑行为
- **繁殖系统**: 使用甜浆果繁殖，幼狐遗传父母皮肤类型和信任玩家
- **叼物品行为**: 可以叼起地上的物品
- **夜间活动**: 白天睡觉，晚上活动
- **潜行狩猎**: 跳跃攻击小动物

### 信任机制
- `trusts(playerId)`: 检查是否信任某玩家
- `addTrustedPlayer(playerId)`: 添加信任玩家（最多 2 个，超出时替换最早的）
- `removeTrustedPlayer(playerId)`: 移除信任玩家
- `getFirstTrustedPlayer()`: 获取第一个信任的玩家
- `getTrustedPlayers()`: 获取所有信任玩家列表

### 繁殖系统
- `isBreedingItem(itemStack)`: 检查物品是否可用于繁殖（仅甜浆果）
- `spawnBaby(partner)`: 创建幼狐
  - 50% 概率继承任一父母的皮肤类型
  - 继承父母的信任玩家（最多 2 个，按添加顺序）
  - 设置为幼体状态

### 行为
| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 在水中游泳 |
| 1 | EscapeDangerGoal | 逃离危险 |
| 2 | AttackGoal | 攻击目标 |
| 3 | FollowParentGoal | 幼体跟随父母 |

### 属性
| 属性 | 值 | 说明 |
|------|-----|------|
| MAX_HEALTH | 10.0 | 生命值 |
| MOVEMENT_SPEED | 0.3 | 移动速度 |
| 成体眼睛高度 | 0.4 | - |
| 幼体眼睛高度 | 0.2 | - |

## 熊猫 (PandaEntity)

### 特性
- 7种性格基因
  - 普通 (Normal)
  - 懒惰 (Lazy)
  - 忧郁 (Worried)
  - 顽皮 (Playful)
  - 好斗 (Aggressive)
  - 虚弱 (Weak)
  - 棕色 (Brown)
- 打喷嚏机制
- 打滚行为

### 基因遗传
- 子代基因由父母基因随机决定
- 隐性基因（虚弱、棕色）需要双亲携带

## 北极熊 (PolarBearEntity)

### 特性
- 幼崽跟随父母
- 游泳行为
- 攻击保护机制
- 站立动画

### 行为
| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 在水中游泳 |
| 1 | AttackGoal | 攻击目标 |
| 2 | FollowParentGoal | 幼体跟随父母 |

## 炽足兽 (StriderEntity)

### 特性
- **熔岩行走**: 在熔岩表面行走，不沉入熔岩
- **可骑乘**: 实现 `IRideable` 接口
- **温度敏感**: 离开熔岩会发抖、减速
- **鞍装备**: 需要鞍才能控制方向

### 接口实现
```cpp
class StriderEntity : public AnimalEntity, public entity::IRideable {
public:
    // IRideable 接口
    bool hasSaddle() const override;
    void setSaddle(bool saddle) override;
    void onPlayerStartRiding(Player* player) override;
    void onPlayerStopRiding(Player* player) override;
    f32 getSteeringSpeed() const override;
    bool boost() override;
    bool canBeSteered() const override;
    void travelTowards(const Vector3& travelVec) override;
};
```

### 熔岩行走机制
- 在熔岩表面时设置 `onGround = true`
- 温度系统：检测周围熔岩块
- 发抖动画：离开熔岩时播放
- 速度调整：离开熔岩时速度降低

### 网络同步
- 使用 `PlayerInputPacket` 同步玩家输入
- 使用 `MoveVehiclePacket` 同步载具位置
- 使用 `EntityActionPacket` 同步实体动作

## 海龟 (TurtleEntity)

### 特性
- 出生地记忆
- 产卵行为
- 幼崽成长
- 水陆两栖

### 行为
| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 在水中游泳 |
| 1 | PanicGoal | 受伤逃跑 |
| 2 | BreedGoal | 繁殖 |
| 3 | TemptGoal | 被海草诱惑 |
| 4 | GoHomeGoal | 返回出生地产卵 |

## 测试覆盖

测试文件位于 `tests/common/entity/entities/passive/special/`，包含：
- **FoxEntityTest.cpp**: 狐狸实体测试（20 个测试）
  - 狐狸类型测试（默认类型、设置/获取）
  - 信任系统测试（添加、移除、最多 2 个、去重）
  - 繁殖物品测试（接受甜浆果、拒绝其他物品）
  - spawnBaby 测试（创建幼狐、遗传类型、遗传信任、位置设置）
  - 属性测试（生命值、移动速度）
  - 眼睛高度测试（成体/幼体差异）
  - 睡眠状态测试
  - 叼物品功能测试
- 炽足兽熔岩行走测试
- 炽足兽骑乘系统测试
- 蜜蜂授粉测试
- 海龟出生地记忆测试
