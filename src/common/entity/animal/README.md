# Animal 模块 (动物实体)

本模块实现了 Minecraft 中的被动动物实体系统，包括猪、牛、羊、鸡等基础动物。

## 目录结构

```
src/common/entity/animal/
├── AnimalEntity.hpp      # 动物实体基类头文件
├── AnimalEntity.cpp      # 动物实体基类实现
├── ChickenEntity.hpp     # 鸡实体头文件
├── ChickenEntity.cpp     # 鸡实体实现
├── CowEntity.hpp         # 牛实体头文件
├── CowEntity.cpp         # 牛实体实现
├── PigEntity.hpp         # 猪实体头文件
├── PigEntity.cpp         # 猪实体实现
├── SheepEntity.hpp       # 羊实体头文件
└── SheepEntity.cpp       # 羊实体实现
```

## 文件详细说明

### AnimalEntity.hpp / AnimalEntity.cpp

**职责**: 动物实体基类，提供所有动物的通用行为。

**主要内容**:
- **繁殖系统**:
  - `isBreedingItem()`: 检查物品是否可用于繁殖（虚函数，子类重写）
  - `canMateWith()`: 检查是否可以与另一动物交配
  - `spawnBaby()`: 生成幼体（纯虚函数，子类必须实现）
- **爱心状态**:
  - `getInLove()` / `setInLove()`: 爱心计时器管理
  - `getLoveCause()` / `setLoveCause()`: 获取/设置喂食玩家ID
- **AI 目标注册**:
  - 优先级 0: SwimGoal（游泳）
  - 优先级 1: PanicGoal（恐慌逃跑）
  - 优先级 2: BreedGoal（繁殖）
  - 优先级 3: TemptGoal（食物诱惑，子类需配置）
  - 优先级 4: FollowParentGoal（跟随父母）
  - 优先级 5: RandomWalkingGoal（随机漫步）
  - 优先级 6: LookAtGoal（看向玩家）
  - 优先级 7: LookRandomlyGoal（随机看向）
- **常量**:
  - `IN_LOVE_DURATION = 600` (爱心状态持续 30 秒)

**继承关系**: `AnimalEntity` -> `AgeableEntity` -> `CreatureEntity` -> `MobEntity` -> `LivingEntity` -> `Entity`

---

### ChickenEntity.hpp / ChickenEntity.cpp

**职责**: 鸡实体实现，提供下蛋和种子繁殖功能。

**主要内容**:
- **下蛋系统**:
  - `getEggTimer()`: 获取下蛋计时器
  - `resetEggTimer()`: 重置下蛋计时器（随机 5-10 分钟）
- **常量**:
  - `EGG_TIME_MIN = 6000` (最小下蛋间隔 5 分钟)
  - `EGG_TIME_MAX = 12000` (最大下蛋间隔 10 分钟)
- **繁殖物品**: 种子（TODO: ItemTags::SEEDS）
- **特有行为**: 每隔 5-10 分钟随机下蛋一个

**待实现**:
- [ ] 下蛋时生成蛋物品实体
- [ ] 种子检测逻辑

---

### CowEntity.hpp / CowEntity.cpp

**职责**: 牛实体实现，提供小麦繁殖和挤奶功能。

**主要内容**:
- **繁殖物品**: 小麦（TODO: Items::WHEAT）
- **挤奶**: TODO（需要桶交互）

**待实现**:
- [ ] 小麦检测逻辑
- [ ] 挤奶交互

---

### PigEntity.hpp / PigEntity.cpp

**职责**: 猪实体实现，提供胡萝卜繁殖和骑乘功能。

**主要内容**:
- **繁殖物品**: 胡萝卜（TODO: Items::CARROT）
- **骑乘系统**:
  - `hasSaddle()` / `setSaddle()`: 鞍状态管理
- **特有行为**: 可装备鞍并骑乘（需要胡萝卜钓竿控制）

**待实现**:
- [ ] 胡萝卜检测逻辑
- [ ] 骑乘交互
- [ ] 胡萝卜钓竿控制

---

### SheepEntity.hpp / SheepEntity.cpp

**职责**: 羊实体实现，提供小麦繁殖、剪羊毛和羊毛颜色系统。

**主要内容**:
- **羊毛系统**:
  - `getWoolColor()` / `setWoolColor()`: 羊毛颜色（0=白色）
  - `hasWool()` / `setWool()`: 是否有羊毛
- **剪羊毛**:
  - `shear()`: 剪下羊毛，返回羊毛数量
- **吃草**:
  - `getEatAnimationTimer()`: 吃草动画计时器
- **繁殖物品**: 小麦（TODO: Items::WHEAT）
- **颜色遗传**: 子类可混合父母颜色

**待实现**:
- [ ] 小麦检测逻辑
- [ ] 羊毛掉落逻辑
- [ ] 羊毛重新生长
- [ ] 颜色遗传混合
- [ ] EatGrassGoal

---

## 模块整体说明

### 整体职责

本模块实现 Minecraft 中的被动动物实体系统，提供：
1. 动物基类（繁殖、爱心状态、AI 目标）
2. 四种具体动物实现（猪、牛、羊、鸡）
3. 每种动物的特有行为（下蛋、挤奶、骑乘、剪羊毛）

### 输入和输出

**输入**:
- 世界 tick 更新
- 玩家交互（喂食、挤奶、剪羊毛、骑乘）
- AI 目标调度

**输出**:
- 实体移动和行为
- 物品掉落（蛋、羊毛）
- 幼体生成

### 依赖项

**内部依赖**:
```
AnimalEntity
  └── AgeableEntity (年龄系统、成长)
      └── CreatureEntity (移动、寻路)
          └── MobEntity (AI 系统)
              └── LivingEntity (生命值、属性)
                  └── Entity (基类)
```

**AI Goal 依赖**:
- `SwimGoal`: 游泳行为
- `PanicGoal`: 受伤恐慌逃跑
- `BreedGoal`: 繁殖行为
- `TemptGoal`: 食物诱惑跟随
- `FollowParentGoal`: 幼体跟随父母
- `RandomWalkingGoal`: 随机漫步
- `LookAtGoal`: 看向玩家
- `LookRandomlyGoal`: 随机看向

**其他依赖**:
- `ItemStack`: 物品检测
- `math::Random`: 随机数生成
- `IWorld`: 世界接口

### 使用方法

#### 创建自定义动物

```cpp
#include "common/entity/animal/AnimalEntity.hpp"

class MyAnimalEntity : public AnimalEntity {
public:
    MyAnimalEntity(LegacyEntityType type, EntityId id)
        : AnimalEntity(type, id)
    {
        registerGoals();
    }

    // 实现繁殖检测
    bool isBreedingItem(const ItemStack& itemStack) const override {
        // return itemStack.getItem() == Items::MY_FOOD;
        return false;
    }

    // 实现交配检测
    bool canMateWith(const AnimalEntity& other) const override {
        return AnimalEntity::canMateWith(other);
        // 可添加类型检查
    }

    // 实现幼体生成
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override {
        // return std::make_unique<MyAnimalEntity>(...);
        return nullptr;
    }

protected:
    void registerGoals() override {
        AnimalEntity::registerGoals();
        // 添加动物特有目标
    }
};
```

#### 注册实体到工厂

```cpp
// 使用静态工厂方法
EntityRegistry::register(EntityType::MY_ANIMAL, MyAnimalEntity::create);
```

### 容易踩的坑

1. **繁殖物品检测未实现**
   - 当前所有 `isBreedingItem()` 返回 `false`
   - 需要完成物品系统后实现具体检测逻辑

2. **spawnBaby() 返回 nullptr**
   - 当前所有动物的 `spawnBaby()` 返回 `nullptr`
   - 需要实现幼体创建逻辑

3. **AI 目标 TemptGoal 未配置**
   - 基类注释掉了 TemptGoal 注册
   - 子类需要添加食物检测谓词

4. **爱心状态重复管理**
   - `AnimalEntity` 有 `m_inLoveTimer`
   - `AgeableEntity` 有 `m_loveTimer`
   - 两者功能重叠，注意不要混用

5. **实体 ID 分配**
   - 工厂方法中使用临时 ID 0
   - 实际 ID 由 `EntityManager` 分配
   - 不要使用静态计数器避免线程安全问题

6. **继承链构造顺序**
   - 基类构造函数调用 `registerGoals()`
   - 此时子类成员尚未初始化
   - 如需访问子类成员，请在子类构造函数中再次调用

### 涉及的测试用例

**渲染模型测试** (`tests/client/renderer/entity/AnimalModelTests.cpp`):
- `PigReasonableBoundsAndHorizontalBody`: 验证猪模型边界框和水平姿态
- `CowReasonableBoundsAndHorizontalBody`: 验证牛模型边界框和水平姿态
- `SheepReasonableBoundsAndHorizontalBody`: 验证羊模型边界框和水平姿态
- `ChickenReasonableBounds`: 验证鸡模型边界框

**注意**: 当前没有动物实体逻辑的单元测试。建议添加以下测试：
- [ ] 繁殖系统测试
- [ ] 爱心状态测试
- [ ] 下蛋计时器测试
- [ ] 羊毛剪毛测试
- [ ] AI 目标优先级测试

## 类图

```
                    ┌──────────────┐
                    │    Entity    │
                    └──────┬───────┘
                           │
                    ┌──────▼───────┐
                    │ LivingEntity │
                    └──────┬───────┘
                           │
                    ┌──────▼───────┐
                    │   MobEntity  │
                    └──────┬───────┘
                           │
                    ┌──────▼───────┐
                    │CreatureEntity│
                    └──────┬───────┘
                           │
                    ┌──────▼───────┐
                    │ AgeableEntity│
                    └──────┬───────┘
                           │
                    ┌──────▼───────┐
                    │ AnimalEntity │
                    └──────┬───────┘
                           │
       ┌───────────┬───────┼───────┬───────────┐
       │           │       │       │           │
┌──────▼─────┐┌────▼────┐┌──▼───┐┌──▼────┐┌────▼────┐
│ChickenEntity││CowEntity││PigEntity││SheepEntity│
└────────────┘└─────────┘└───────┘└─────────┘
```

## 参考

- Minecraft Java Edition 1.16.5 源码
  - `net.minecraft.entity.passive.AnimalEntity`
  - `net.minecraft.entity.passive.ChickenEntity`
  - `net.minecraft.entity.passive.CowEntity`
  - `net.minecraft.entity.passive.PigEntity`
  - `net.minecraft.entity.passive.SheepEntity`
