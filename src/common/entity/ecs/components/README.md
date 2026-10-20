# 实体数据组件

## 目录结构

```text
components/
├── AABBShapeComponent.hpp # 轴对齐包围盒组件
├── ArmorStandStateComponent.hpp # 盔甲架状态组件（不可见/小型/手臂/禁用槽位/底座/姿态）
├── ArrowEffectsComponent.hpp # 药水箭效果组件
├── ArrowStateComponent.hpp # 箭矢/蜂针状态组件
├── AttributeComponent.hpp # 属性状态组件
├── BuiltInEntityComponents.hpp # 内建组件裸指针缓存
├── ChestedHorseComponent.hpp # 可装箱马类中间层组件（A 类本地字段）
├── ChestMinecartComponent.hpp # 箱子矿车组件
├── CommandBlockMinecartComponent.hpp # 命令方块矿车组件
├── DamagingProjectileComponent.hpp # 伤害型投射物组件
├── EntityFlagsComponent.hpp # 实体标志位组件
├── EntityOwnerComponent.hpp # Entity 所有权组件——ECS 反向持有 OOP 实体对象（非拥有裸指针）
├── EntityRotationComponent.hpp # 实体旋转组件
├── EntityStateComponent.hpp # 实体轻量同步元数据组件
├── EntityUniqueIDComponent.hpp # 实体持久唯一 ID 组件
├── EquipmentComponent.hpp # 装备状态组件
├── EvokerFangsComponent.hpp # 唤魔者尖牙状态组件
├── ExperienceBottleComponent.hpp # 经验瓶投掷物组件
├── EyeOfEnderComponent.hpp # 末影之眼状态组件
├── FireballStateComponent.hpp # 火球族状态组件
├── FireComponent.hpp # 火焰状态组件
├── FireworkRocketComponent.hpp # 烟花火箭状态组件
├── FishingBobberComponent.hpp # 钓鱼浮标状态组件
├── FreezeComponent.hpp # 冰冻状态组件
├── FurnaceMinecartComponent.hpp # 熔炉矿车组件
├── HealthComponent.hpp # 生命值状态组件
├── HopperMinecartComponent.hpp # 漏斗矿车组件
├── HorseAnimationComponent.hpp # 马类动画组件（A 类本地字段）
├── HorseAttributeComponent.hpp # 马类属性组件（B 类属性字段）
├── HorseBoostComponent.hpp # 马类加速组件（A 类本地字段）
├── HorseInventoryComponent.hpp # 马类库存组件（A 类本地字段，unique_ptr 包裹）
├── HorseJumpComponent.hpp # 马类跳跃组件（B 类强时序字段）
├── HorseStatusComponent.hpp # 马类状态标志组件（C 类同步字段）
├── HorseTamingComponent.hpp # 马类驯服进度组件（A 类本地字段）
├── HurtStateComponent.hpp # 受击/死亡状态组件
├── MinecartDisplayComponent.hpp # 矿车显示方块组件
├── MinecartStateComponent.hpp # 矿车运行状态组件
├── MobFlagComponent.hpp # 敌对生物标记组件（tag component）
├── PhysicsStateComponent.hpp # 物理状态组件
├── PlayerScoreComponent.hpp # 玩家分数组件
├── PortalComponent.hpp # 传送门状态组件
├── PotionProjectileComponent.hpp # 药水投掷物组件
├── ProjectileArrowStateComponent.hpp # 投射物箭矢状态组件
├── ProjectileItemComponent.hpp # 投掷物承载物品组件
├── ProjectileOwnerComponent.hpp # 弹射物发射者组件
├── ShulkerBulletComponent.hpp # 潜影贝子弹状态组件
├── SpawnerMinecartComponent.hpp # 刷怪笼矿车组件
├── SpectralArrowComponent.hpp # 光灵箭状态组件
├── StateVectorComponent.hpp # 位置状态向量组件
├── TntMinecartComponent.hpp # TNT 矿车组件
├── TridentStateComponent.hpp # 三叉戟状态组件
├── VelocityComponent.hpp # 速度组件
└── WindChargeStateComponent.hpp # 风弹状态组件
```

## 内部模块关系

组件由 EntityRegistry 持有，实体继承壳通过 EntityContext 查询；内置位置、速度和碰撞箱组件供移动及物理系统使用。

## 上下游外部依赖关系

上游为实体行为、系统和组件序列化器；下游依赖基础值类型、数学类型和必要的生命周期引用。

## 容易踩的坑

- 更新位置时必须同步碰撞箱和空间索引，不能只写 StateVectorComponent。
- 跨 tick 的非拥有实体缓存使用生命周期引用，目标销毁后不能解引用旧指针检查存活；UUID 用于重新解析。
- 组件移动不得使实体继承壳缓存的组件地址失效，含不可移动资源的组件须按既有所有权约定封装。
