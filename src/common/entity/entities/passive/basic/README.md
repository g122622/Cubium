# 普通动物

基础被动动物实体，继承自 `AgeableEntity`，支持繁殖行为。

## 目录结构

```
basic/
├── AnimalEntity.hpp/cpp       # 动物基类，定义繁殖系统核心逻辑
├── PigEntity.hpp/cpp          # 猪（可实现 IRideable/IEquipable 接口）
├── CowEntity.hpp/cpp          # 牛（可被空桶挤奶）
├── SheepEntity.hpp/cpp        # 羊（实现 IShearable 接口，16 种颜色）
├── ChickenEntity.hpp/cpp      # 鸡（自动下蛋、无摔落伤害）
├── RabbitEntity.hpp/cpp       # 兔子（8 种皮肤，含杀手兔变种）
└── MooshroomEntity.hpp/cpp    # 哞菇（继承 CowEntity，实现 IShearable，迷之炖菜效果）
```

## 内部模块关系

```
AgeableEntity (父类)
    │
    └── AnimalEntity (本目录基类)
            │  提供：繁殖系统（isBreedingItem、canMateWith、spawnBaby）
            │  提供：爱心状态管理（setInLove、resetInLove）
            │  注册基础属性（MAX_HEALTH=10, MOVEMENT_SPEED=0.2）
            │
            ├── CowEntity ──────────┬── MooshroomEntity (继承 CowEntity)
            │   (挤奶在 BucketItem)  │      ├── IShearable (剪毛返回蘑菇，转换为普通牛)
            │                        │      ├── interactMob (空碗→蘑菇汤/迷之炖菜，花朵→存储效果)
            │                        │      ├── _getStewEffectFromItem (花朵→迷之炖菜效果映射)
            │                        │      └── NBT序列化 (Type, StewEffect)
            │                        │
            ├── SheepEntity          │
            │   └── IShearable       │
            │                        │
            ├── PigEntity            │
            │   ├── IRideable        │
            │   └── IEquipable       │
            │                        │
            ├── ChickenEntity        │
            │   └── 翅膀动画/下蛋    │
            │                        │
            └── RabbitEntity ────────┘
                └── 8 种皮肤/杀手兔变种
```

## 上下游外部依赖关系

**上游依赖（本目录使用）：**
- `entity/core/AgeableEntity` - 年龄系统基类
- `entity/ai/goal/goals/` - AI 目标：BreedGoal、FollowParentGoal、TemptGoal、PanicGoal、SwimGoal
- `entity/interfaces/IShearable` - 剪毛接口（SheepEntity、MooshroomEntity）
- `entity/interfaces/IRideable` - 骑乘接口（PigEntity）
- `entity/interfaces/IEquipable` - 装备接口（PigEntity）
- `entity/core/BoostHelper` - 猪加速辅助
- `util/color/DyeColor` - 羊毛颜色枚举
- `item/Items` - 物品检查（小麦、胡萝卜、碗、蘑菇汤等）
- `item/items/block/BlockItemRegistry` - 方块物品注册表（花朵→迷之炖菜效果映射）
- `world/block/blocks/vegetation/FlowerBlock` - 花朵方块（存储迷之炖菜效果类型和持续时间）
- `entity/effect/EffectType` - 药水效果类型（迷之炖菜效果枚举）

**下游依赖（使用本目录）：**
- `entity/core/VanillaEntities.hpp` - 注册所有实体类型
- `item/items/special/BucketItem` - 挤奶交互（牛）
- `item/crafting/special/ArmorDyeRecipe` - 羊毛染色
- `entity/ai/goal/goals/BreedGoal` - 繁殖目标
- `entity/ai/goal/goals/FollowParentGoal` - 幼体跟随父母
- `client/renderer/.../SheepWoolLayer` - 羊毛渲染层

## 容易踩的坑

1. **猪的鞍存储**：猪不存储实际 ItemStack，只存储布尔值。`getEquipment(0)` 有鞍时返回 `ItemStack(Items::SADDLE, 1)`，`setEquipment(0, saddle)` 只设置布尔状态。

2. **羊的颜色混合**：繁殖时幼羊颜色由父母颜色按配方混合，无配方时随机选择父母颜色。配方如：白+红=粉红、蓝+红=紫等。

3. **羊的剪毛冷却**：`SheepEntity` 有 `m_shearCooldown` 字段，剪毛后需等待才能再次剪毛。

4. **哞菇剪毛后转换**：剪毛会生成 5 个蘑菇并转换为普通牛，继承位置、朝向、生命值、自定义名称等状态。

5. **哞菇繁殖变异**：双亲类型相同时有 1/1024 概率变异为另一种类型（红↔棕）。

6. **哞菇迷之炖菜**：棕色哞菇被喂食花朵后存储迷之炖菜效果，用空碗右键获得迷之炖菜（含 NBT 效果标签）。红色哞菇用空碗获得普通蘑菇汤。花朵到效果的映射由 `FlowerBlock` 存储，`_getStewEffectFromItem()` 查询。效果持续时间以秒为单位存储，非瞬间效果在写入 NBT 时乘以 20 转换为 tick。

7. **哞菇 NBT 序列化**：Type 字段为 i8（0=红, 1=棕），StewEffect 为复合标签（EffectId: i8, EffectDuration: i32）。

6. **鸡骑士标记**：`m_chickenJockey = true` 时不下蛋。

7. **兔子类型值**：`RabbitType::Killer = 99`，不是按顺序排列的枚举值。MC 1.21.11 中杀手兔不再自然生成，只能通过命令或刷怪蛋指定 ID 99 生成。

8. **AnimalEntity::registerGoals() 是空的**：子类必须自己注册完整的 AI 目标列表，不能依赖基类注册。

9. **兔子类型与群系**：`setRandomRabbitType()` 根据生成位置的群系决定类型——雪地群系（白色/白色斑点）、沙漠群系（金色）、其他群系（棕色/椒盐色/黑色）。`getDefaultRabbitTypeForBiome()` 可用于繁殖时获取群系类型。

10. **兔子跳跃动画状态机**（参考 MC 1.21.11 `Rabbit.java`）：
    - **字段**：`m_rabbitJumpTicks`（当前跳跃已持续 tick）、`m_rabbitJumpDuration`（总持续 tick，0=未在跳跃中）。**与 `LivingEntity::m_jumpTicks`（跳跃冷却）语义不同，独立存储**。
    - **启动**：`setJumping(true)` 调用 `startJumping()`，后者设置 `m_rabbitJumpDuration=10`、`m_rabbitJumpTicks=0` 并广播 `EntityStatusPacket::RabbitJump(1)` 状态码。`startJumping()` 幂等：动画进行中（`m_rabbitJumpDuration != 0`）时跳过重置，避免 `JumpController::tick()` 每 tick 调用 `setJumping(true)` 导致反复归零。
    - **推进**：`aiStep()` 重写先调用 `LivingEntity::aiStep()`，再推进 `m_rabbitJumpTicks`；达到 `m_rabbitJumpDuration` 时归零并调用 `LivingEntity::setJumping(false)`（直接调基类，避免再次播音效/广播）。
    - **完成度**：`getJumpCompletion(partialTick) = m_rabbitJumpDuration == 0 ? 0 : (m_rabbitJumpTicks + partialTick) / m_rabbitJumpDuration`，供客户端计算 `jumpRotation = sin(completion * PI)`。
    - **广播**：项目架构下 `LivingEntity::jump()` 非虚函数无法重写（MC 在 `jumpFromGround()` 中广播），故在 `startJumping()` 中即广播，略早一个 tick，但客户端位置插值会平滑过渡。
    - **未实现的 MC 原版特性**：`RabbitJumpControl`（兔子专属跳跃控制器，含 `canJump`/`setCanJump` 状态机）、`RabbitMoveControl`（移动控制器，控制跳跃速度）、`jumpDelayTicks`（着陆延迟，慢速 10 tick / 快速 1 tick）、`wasOnGround`（着陆检测）、`RaidGardenGoal`（偷胡萝卜）、`moreCarrotTicks`（偷食冷却）尚未实现。当前由通用 `JumpController` + `LivingEntity::aiStep()` 自动跳跃 + `m_jumpTicks` 冷却提供最小可行行为。详见 `RabbitEntity.cpp` 顶部 TODO 注释。
