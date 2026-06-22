# 可驯服动物

可被玩家驯服的动物实体。

## 目录结构树

```
tamable/
├── TameableEntity.hpp/cpp    # 可驯服实体基类（IAngerable 接口、getTeam重写、wantsToAttack虚方法）
├── ShoulderRidingEntity.hpp  # 肩膀乘坐实体基类（鹦鹉专用）
├── WolfEntity.hpp/cpp        # 狼（骨头驯服、攻击保护、wantsToAttack过滤）
├── CatEntity.hpp/cpp         # 猫（生鱼驯服、11种皮肤、项圈染色、interactMob交互）
├── OcelotEntity.hpp/cpp      # 豹猫（信任机制、丛林生物）
└── ParrotEntity.hpp/cpp      # 鹦鹉（种子驯服、可站肩膀、不可繁殖）
```

## 内部模块关系

```
AnimalEntity
└── TameableEntity (+ IAngerable)
    ├── WolfEntity
    ├── CatEntity
    ├── OcelotEntity（信任机制，非完全驯服）
    └── ShoulderRidingEntity
        └── ParrotEntity (+ IFlyingAnimal)
```

**关键继承说明**：
- `TameableEntity` 继承 `AnimalEntity` 并实现 `IAngerable` 接口
- `OcelotEntity` 虽在 tamable 目录下，但使用信任机制（`isTrusting()`）而非标准驯服系统
- `ShoulderRidingEntity` 是 `ParrotEntity` 专用的中间基类

## 上下游外部依赖关系

### 依赖的上游模块
- `entity/core/` - Entity, LivingEntity, MobEntity, CreatureEntity, AgeableEntity, AnimalEntity 基类
- `entity/interfaces/IAngerable.hpp` - 愤怒接口
- `entity/interfaces/IFlyingAnimal.hpp` - 飞行动物接口（鹦鹉）
- `entity/ai/` - Goal 系统（SwimGoal, PanicGoal, SitGoal, BreedGoal, TemptGoal, FollowOwnerGoal, FollowParentGoal, LeapAtTargetGoal, MeleeAttackGoal, AvoidEntityGoal 等）
- `entity/attributes/` - 属性系统（MAX_HEALTH, MOVEMENT_SPEED, ATTACK_DAMAGE）
- `world/IWorld.hpp` - 世界接口
- `item/Items.hpp` - 物品定义（骨头、生鱼、种子、肉类等）

### 被下游模块依赖
- `server/entity/` - 服务器端实体生成、AI 调度
- `client/renderer/entity/` - 客户端实体渲染
- `world/spawn/` - 生物群系生成时的实体放置
- `entity/VanillaEntities.hpp` - 实体类型注册

## 容易踩的坑

### 驯服物品判断
- **必须重写 `isTameItem()`**：不要在交互逻辑中硬编码物品类型，应重写虚方法
- **狼用骨头、猫用生鱼、鹦鹉用种子**：每种动物有特定驯服物品

### AI 目标注册顺序
- **优先级数字越小越优先**：Goal 注册时优先级参数 0 最高
- **子类必须调用父类 `registerGoals()`**：否则丢失基础行为
- **动态 AI 管理**：猫和豹猫需根据驯服/信任状态动态添加移除 Goal，参考 `setupTamedAI()` 模式

### OcelotEntity 信任机制
- **不继承标准驯服系统**：豹猫用 `isTrusting()`/`setTrusting()` 而非 `isTamed()`/`setTamed()`
- **驯服概率不同**：豹猫信任建立是 1/3 概率，鹦鹉是 1/10，狼和猫是 1/3
- **猎物攻击目标**：豹猫目标选择器注册了小鸡攻击目标（NearestAttackableTargetGoal<ChickenEntity>，优先级1）和幼年海龟攻击目标（NearestAttackableTargetGoal<TurtleEntity>，优先级1，BABY_ON_LAND_SELECTOR 过滤：仅攻击 `isChild() && !isInWater()` 的海龟）

### ParrotEntity 特殊性
- **不能繁殖**：`isBreedingItem()` 始终返回 false，`spawnBaby()` 返回 nullptr
- **可站肩膀**：通过 `ShoulderRidingEntity` 基类实现
- **免疫摔落伤害**：作为飞行动物

### 狼的食物系统
- **狼可吃腐肉**：且不会获得饥饿效果，因为治疗逻辑只调用 `heal()` 不应用食物效果
- **驯服前后属性变化**：驯服后生命值 8→20，攻击力 2→4

### 动态 AI 移除
- **猫驯服后移除躲避行为**：`setupTamedAI()` 中移除 `CatAvoidPlayerGoal`
- **豹猫信任后移除躲避行为**：`setupTrustingAI()` 中移除 `OcelotAvoidPlayerGoal`

### 队伍继承与攻击过滤
- **`getTeam()` 重写**：TameableEntity 重写了 `getTeam()`，已驯服动物继承主人的队伍，未驯服时回退到 AnimalEntity 默认逻辑
- **`wantsToAttack(target, owner)` 虚方法**：TameableEntity 提供此虚方法供子类重写攻击过滤逻辑，默认返回 true（允许攻击所有目标）
- **WolfEntity 重写 `wantsToAttack()`**：实现 MC 精确的攻击过滤规则——永远不攻击苦力怕/恶魂/盔甲架，不攻击已驯服的同类（除非主人不同），PvP保护检查（当目标和主人都是玩家时调用 `canHarmPlayer()`），不攻击已驯服的驯服动物和马
- **OwnerHurtByTargetGoal / OwnerHurtTargetGoal 均调用 `wantsToAttack()`**，因此狼的攻击限制自动生效

### 猫的交互系统（interactMob）
- **交互优先级**（已驯服 + 主人）：①项圈染色（染料 + 颜色不同）→ ②喂食治疗（猫食 + 生命未满）→ ③父类处理（繁殖/成长）→ ④切换坐下/站起
- **交互优先级**（未驯服）：猫食（生鳕鱼/生鲑鱼）→ 尝试驯服（1/3概率）
- **非主人无法交互**：已驯服的猫只有主人可以交互，非主人交互直接传递给父类
- **驯服成功广播**：`broadcastEntityStatus(TamingSucceeded/TamingFailed)` 发送心形/烟雾粒子
- **项圈染色**：默认红色，支持 17 种染料物品映射（16 种标准染料 + 骨粉=白色 + 墨囊=黑色等）
- **食物治疗量**：生鳕鱼/生鲑鱼治疗 2.0 生命值
- **待实现目标**：猫的目标选择器缺少兔子攻击目标和幼年海龟攻击目标（待 NonTameRandomTargetGoal 实现后添加，见 CatEntity::registerGoals 中的 TODO 注释，对齐 MC 原版 Cat.registerGoals()）

### TameableEntity NBT 序列化
- **Sitting** (byte/bool) - 是否坐下
- **OwnerUUID** (string) - 主人 UUID
- **AngerTime** (i32) - 愤怒剩余时间
- **CatEntity 额外字段**：CatType (i32) 猫皮肤类型、CollarColor (i32) 项圈颜色（默认红色时省略）
