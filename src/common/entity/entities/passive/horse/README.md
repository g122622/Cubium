# 马类实体模块

包含所有马类实体的实现。

## 目录结构

```
horse/
├── AbstractHorseEntity.hpp/cpp  # 马类基类
├── HorseEntity.hpp/cpp          # 马
├── DonkeyEntity.hpp/cpp         # 驴
├── MuleEntity.hpp/cpp           # 骡
├── SkeletonHorseEntity.hpp/cpp  # 骷髅马
├── ZombieHorseEntity.hpp/cpp    # 僵尸马
├── LlamaEntity.hpp/cpp          # 羊驼
└── README.md                    # 本文件
```

## 继承层次

```
AnimalEntity
└── AbstractHorseEntity (IRideable, IJumpingMount)
    ├── HorseEntity      # 马 (35种变体)
    ├── DonkeyEntity     # 驴 (背包15格)
    ├── MuleEntity       # 骡 (不育，背包15格)
    ├── SkeletonHorseEntity  # 骷髅马 (亡灵，无需驯服)
    ├── ZombieHorseEntity    # 僵尸马 (亡灵，无需驯服)
    └── LlamaEntity      # 羊驼 (商队，吐口水)
```

## 实体特性

### HorseEntity (马)
| 特性 | 说明 |
|------|------|
| 驯服方式 | 骑乘驯服 |
| 装备栏 | 鞍 + 马铠 |
| 变体 | 7种颜色 × 5种花纹 = 35种 |
| 繁殖物品 | 金苹果、金胡萝卜 |
| 属性 | 随机速度、跳跃、生命值 |

### DonkeyEntity (驴)
| 特性 | 说明 |
|------|------|
| 驯服方式 | 骑乘驯服 |
| 装备栏 | 鞍 + 箱子(15格) |
| 繁殖 | 与马繁殖产生骡 |
| 跳跃 | 较低 |

### MuleEntity (骡)
| 特性 | 说明 |
|------|------|
| 驯服方式 | 骑乘驯服 |
| 装备栏 | 鞍 + 箱子(15格) |
| 繁殖 | 不育 |
| 来源 | 马+驴杂交 |

### SkeletonHorseEntity (骷髅马)
| 特性 | 说明 |
|------|------|
| 驯服 | 无需驯服 |
| 生成 | 雷暴天气陷阱 |
| 免疫 | 溺水、中毒 |
| 燃烧 | 阳光下燃烧 |

### ZombieHorseEntity (僵尸马)
| 特性 | 说明 |
|------|------|
| 驯服 | 无需驯服 |
| 生成 | 命令/刷怪蛋 |
| 免疫 | 溺水、中毒 |
| 燃烧 | 不燃烧 |

### LlamaEntity (羊驼)
| 特性 | 说明 |
|------|------|
| 驯服 | 骑乘驯服 |
| 装备栏 | 地毯 + 箱子(3-15格) |
| 骑乘 | 可骑乘但不可控制 |
| 攻击 | 吐口水攻击 |
| 商队 | 跟随前方羊驼 |
| 变体 | 4种颜色 |

## 接口实现

| 实体 | IRideable | IJumpingMount | IEquipable |
|------|-----------|---------------|------------|
| AbstractHorseEntity | ✅ | ✅ | ✅ |
| HorseEntity | 继承 | 继承 | 继承 |
| DonkeyEntity | 继承 | 继承 | 继承 |
| MuleEntity | 继承 | 继承 | 继承 |
| SkeletonHorseEntity | 继承 | 继承 | 继承 |
| ZombieHorseEntity | 继承 | 继承 | 继承 |
| LlamaEntity | 继承 | 继承 | 继承 |

### IEquipable 接口

`IEquipable` 接口提供装备槽管理功能：

```cpp
class IEquipable {
public:
    virtual i32 getEquipmentSlotCount() const = 0;
    virtual ItemStack getEquipment(i32 slot) const = 0;
    virtual void setEquipment(i32 slot, const ItemStack& item) = 0;
    virtual bool canEquip(const ItemStack& item, i32 slot) const = 0;
};
```

马类实现：
- 槽位 0: 鞍
- 槽位 1: 马铠/装饰

### 装备验证方法

MC 1.16.5 风格的装备验证，通过虚方法实现多态：

| 方法 | 说明 |
|------|------|
| `canEquipSaddle()` | 检查实体是否能装备鞍（默认 true，羊驼返回 false） |
| `hasArmorSlot()` | 检查实体是否支持马铠/装饰槽位（默认 false） |
| `isValidArmorForSlot(item)` | 检查物品是否是有效的马铠/装饰（默认 false） |
| `canEquip(item, slot)` | 检查物品是否能放入指定槽位 |

**各实体装备支持**：

| 实体 | 支持鞍 | 支持马铠/装饰 | `hasArmorSlot()` | `canEquipSaddle()` | `isValidArmorForSlot()` |
|------|--------|--------------|------------------|-------------------|------------------------|
| HorseEntity | ✅ | ✅ 马铠 | `true` | `true` | 检查 `HorseArmorItem` |
| DonkeyEntity | ✅ | ❌ | `false` | `true` | `false` |
| MuleEntity | ✅ | ❌ | `false` | `true` | `false` |
| LlamaEntity | ❌ | ✅ 地毯 | `true` | `false` | 检查 `ItemTags::CARPETS` |
| SkeletonHorseEntity | ✅ | ❌ | `false` | `true` | `false` |
| ZombieHorseEntity | ✅ | ❌ | `false` | `true` | `false` |

**使用示例**：

```cpp
// 检查鞍是否能装备到槽位 0
ItemStack saddleStack(Items::SADDLE, 1);
bool canEquip = horse.canEquip(saddleStack, 0);  // true

// 检查马铠是否能装备到槽位 1
ItemStack ironArmorStack(Items::IRON_HORSE_ARMOR, 1);
bool canEquipArmor = horse.canEquip(ironArmorStack, 1);  // true（马）
bool canEquipArmorLlama = llama.canEquip(ironArmorStack, 1);  // false（羊驼不支持马铠）

// 检查地毯是否能装备到羊驼槽位 1
ItemStack carpetStack(Items::WHITE_CARPET, 1);
bool canEquipCarpet = llama.canEquip(carpetStack, 1);  // true（羊驼支持地毯）
```

### 数据同步

马类实体使用 `DataParameter` 同步状态：
- `STATUS_PARAM`: 鞍、驯服、繁殖、进食、扬蹄、张嘴状态
- `OWNER_UUID_PARAM`: 主人 UUID

## 驯服系统

### 驯服流程

马类实体的驯服通过 `RunAroundLikeCrazyGoal` AI 目标实现：

1. **骑乘驯服**：玩家骑上未驯服的马
2. **疯狂奔跑**：马随机移动，每 tick 有 1/50 概率检查驯服
3. **驯服检查**：
   - 增加驯服进度（temper += 5）
   - 如果 `random(maxTemper) < temper`，驯服成功
4. **驯服成功**：调用 `setTamedBy(player)`
5. **驯服失败**：甩下玩家，播放扬蹄动画和愤怒音效

### 核心方法

#### setTamedBy(Player*)
由玩家驯服此马：
```cpp
bool setTamedBy(Player* player);
```
- 设置主人 UUID
- 设置驯服状态
- 触发进度（TameAnimalTrigger）
- 发送爱心粒子效果（状态码 7）

#### makeMad()
让马愤怒（驯服失败时调用）：
```cpp
void makeMad();
```
- 触发扬蹄动画
- 播放愤怒音效（通过 `getAngrySound()`）

#### makeHorseRear()
让马后腿站立：
```cpp
void makeHorseRear();
```
- 设置 `jumpRearingCounter = 1`
- 设置扬蹄状态（网络同步）

#### 扬蹄状态管理
```cpp
bool isRearing() const;     // 检查是否正在扬蹄
void setRearing(bool);       // 设置扬蹄状态
```

#### 主人 UUID 管理
```cpp
std::string getOwnerUuid() const;  // 获取主人UUID
void setOwnerUuid(const std::string& uuid);  // 设置主人UUID
```

### 扬蹄动画状态

状态标志使用位掩码存储在 `STATUS_PARAM` 中：

| 标志 | 位 | 说明 |
|------|-----|------|
| `STATUS_FLAG_TAME` | 2 | 已驯服 |
| `STATUS_FLAG_SADDLE` | 4 | 已装备鞍 |
| `STATUS_FLAG_BRED` | 8 | 已繁殖 |
| `STATUS_FLAG_EATING` | 16 | 正在吃 |
| `STATUS_FLAG_REARING` | 32 | 正在扬蹄 |
| `STATUS_FLAG_MOUTH_OPEN` | 64 | 嘴张开 |

### 状态辅助方法

| 方法 | 说明 |
|------|------|
| `isTame()` / `setTame(bool)` | 驯服状态 |
| `hasSaddle()` / `setSaddle(bool)` | 鞍装备状态 |
| `isRearing()` / `setRearing(bool)` | 扬蹄动画状态 |
| `isEating()` / `setEating(bool)` | 进食状态 |
| `isBred()` / `setBred(bool)` | 繁殖状态 |
| `isMouthOpen()` / `setMouthOpen(bool)` | 嘴巴张开状态 |

## 繁殖系统（MC 1.16.5）

### 食物效果

马类实体通过 `handleEating()` 方法处理食物效果：

#### AbstractHorseEntity 食物效果
| 食物 | 治疗 | 成长(ticks) | 驯服进度 | 触发繁殖 |
|------|------|------------|---------|---------|
| 小麦 | 2 | 20 | +3 | ❌ |
| 糖 | 1 | 30 | +3 | ❌ |
| 干草块 | 20 | 180 | 0 | ❌ |
| 苹果 | 3 | 60 | +3 | ❌ |
| 金胡萝卜 | 4 | 60 | +5 | ✅（需驯服）|
| 金苹果 | 10 | 240 | +10 | ✅（需驯服）|
| 附魔金苹果 | 10 | 240 | +10 | ✅（需驯服）|

#### LlamaEntity 食物效果
| 食物 | 治疗 | 成长(ticks) | 驯服进度 | 触发繁殖 |
|------|------|------------|---------|---------|
| 小麦 | 2 | 10 | +3 | ❌ |
| 干草块 | 10 | 90 | +6 | ✅ |

### 繁殖方法

#### canMateWith()
检查是否可以与另一动物交配：
```cpp
bool canMateWith(const AnimalEntity& other) const override;
```
- HorseEntity：马+马=马，马+驴=骡
- DonkeyEntity：驴+驴=驴，驴+马=骡
- LlamaEntity：羊驼+羊驼=羊驼（骡不育）

#### spawnBaby()
生成后代：
```cpp
std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;
```
- 属性遗传：`(parent1 + parent2 + random) / 3`
- 颜色遗传（马）：4/9 父本，4/9 母本，1/9 随机
- 强度遗传（羊驼）：`max(parent1, parent2) + random(0,1)`

### 玩家交互

玩家右键点击马匹时调用 `interactMob()`：

1. **手持食物**：调用 `handleEating()` 处理喂食效果
2. **未驯服**：尝试骑乘（驯服流程）
3. **已驯服**：装备鞍或打开背包

## 骑乘更新系统

### updateRiding()

每 tick 调用，更新马类实体的动画状态和乘客位置：

```cpp
void updateRiding();
```

**功能**：
1. 更新扬蹄动画进度 (`m_rearingAmount`)
2. 更新低头吃草动画进度 (`m_headLean`)
3. 更新张嘴动画进度 (`m_mouthOpenness`)
4. 调用 `updatePassengers()` 更新所有乘客位置

**动画更新算法**（参考 MC 1.16.5）：

```cpp
// 扬蹄动画渐入
if (isRearing()) {
    rearingAmount += (1.0f - rearingAmount) * 0.4f + 0.05f;
}
// 扬蹄动画渐出（三次方平滑）
else {
    rearingAmount += (0.8f * rearingAmount³ - rearingAmount) * 0.6f - 0.05f;
}
```

### updatePassengerPosition()

重写父类方法，处理扬蹄时的乘客位置偏移：

```cpp
void updatePassengerPosition(Entity& passenger) override;
```

**功能**：
1. 调用父类基础定位
2. 扬蹄时根据 `prevRearingAmount` 计算额外位置偏移：
   - X/Z 方向偏移：`0.7f * prevRearingAmount * sin/cos(yaw)`
   - Y 方向额外高度：`0.15f * prevRearingAmount`

### 动画插值方法

用于渲染时平滑过渡动画：

```cpp
// 获取扬蹄动画进度（用于渲染）
f32 getRearingAmount(f32 partialTicks) const;

// 获取低头吃草动画进度（用于渲染）
f32 getHeadLeanAmount(f32 partialTicks) const;

// 获取张嘴动画进度（用于渲染）
f32 getMouthOpennessAmount(f32 partialTicks) const;
```

**参数**：
- `partialTicks`：帧间插值时间（0.0-1.0）

**返回值**：
- 插值后的动画进度（0.0-1.0）

**使用示例**（渲染器中）：

```cpp
// 渲染时获取平滑的扬蹄角度
f32 rearingAngle = horse.getRearingAmount(partialTicks) * MAX_REARING_ANGLE;
// 应用到马模型
horseModel.setRearingAmount(rearingAngle);
```

## 使用示例

```cpp
// 创建马
auto horse = std::make_unique<HorseEntity>(LegacyEntityType::Unknown, 0);
horse->randomizeAppearance();  // 随机外观
horse->setTame(true);
horse->setSaddle(true);

// 骑乘
horse->onPlayerStartRiding(player);

// 跳跃
horse->startJumping();
// 蓄力...
horse->stopJumping();

// 加速（胡萝卜钓竿）
horse->boost();

// 驯服
horse->setTamedBy(player);  // 设置主人，触发进度，显示爱心粒子

// 让马愤怒（驯服失败时）
horse->makeMad();  // 扬蹄 + 播放愤怒音效
```

## 跳跃提升效果

马类实体支持跳跃提升药水效果加成。当马拥有跳跃提升效果时，跳跃力度会根据效果等级增加。

### 效果计算

参考 MC 1.16.5 `AbstractHorseEntity.travel()` 第 716-727 行：

```java
if (this.isPotionActive(Effects.JUMP_BOOST)) {
   d1 = d0 + (double)((float)(this.getActivePotionEffect(Effects.JUMP_BOOST).getAmplifier() + 1) * 0.1F);
}
```

**公式**：`跳跃力度 = 基础跳跃力度 + 跳跃提升等级 × 0.1`

| 跳跃提升等级 | 跳跃力度增量 |
|-------------|-------------|
| I           | +0.1        |
| II          | +0.2        |
| III         | +0.3        |
| ...         | +等级×0.1   |

### 实现位置

- `AbstractHorseEntity::travel()` - 骑乘时的跳跃处理
- `AbstractHorseEntity::performJump()` - 执行跳跃方法

### 代码示例

```cpp
// 给马添加跳跃提升 II 效果
entity::effect::EffectInstance jumpBoost(
    entity::effect::EffectType::JumpBoost,  // 效果类型
    200,                                      // 持续时间 (ticks)
    1,                                        // amplifier (II = 1)
    false,                                    // 是否环境效果
    true                                      // 是否显示粒子
);
horse->addEffect(std::move(jumpBoost));

// 检查效果等级
i32 level = horse->getEffectLevel(entity::effect::EffectType::JumpBoost);
// level = 2 (因为 amplifier + 1 = 2)
```

## 测试用例

马类实体的测试位于以下文件：

| 测试文件 | 测试内容 |
|---------|---------|
| `tests/entity/HorseSupportTypesTest.cpp` | 马类型支持测试 |
| `tests/entity/HorseAppearanceSupportTypesTest.cpp` | 马外观变体测试 |
| `tests/entity/HorseJumpBoostTest.cpp` | 跳跃提升药水效果测试 |
| `tests/entity/HorseTamingTest.cpp` | 驯服系统、扬蹄动画、状态管理测试 |
| `tests/common/entity/entities/passive/horse/HorseCanEquipTest.cpp` | 装备验证测试 |

### 新增测试用例

`HorseTamingTest.cpp` 包含以下动画相关测试：

- `RearingAmount_InitialValueIsZero` - 初始扬蹄动画为 0
- `RearingAmount_IncreasesWhenRearing` - 扬蹄状态设置正确
- `RearingAmount_DecreasesWhenNotRearing` - 取消扬蹄后状态正确
- `RearingAmount_InterpolationFormula` - 插值公式正确
- `HeadLeanAmount_InitialValueIsZero` - 初始低头动画为 0
- `HeadLeanAmount_SetEatingState` - 进食状态设置正确
- `HeadLeanAmount_ClearsWhenRearing` - 扬蹄时进食状态被清除
- `MouthOpennessAmount_InitialValueIsZero` - 初始张嘴动画为 0
- `MouthOpennessAmount_SetMouthOpenState` - 张嘴状态设置正确
- `MouthOpennessAmount_InterpolationFormula` - 插值公式正确

## 参考

- MC 1.16.5 AbstractHorseEntity
- MC 1.16.5 HorseEntity
- MC 1.16.5 DonkeyEntity
- MC 1.16.5 MuleEntity
- MC 1.16.5 SkeletonHorseEntity
- MC 1.16.5 ZombieHorseEntity
- MC 1.16.5 LlamaEntity
- MC 1.16.5 RunAroundLikeCrazyGoal
