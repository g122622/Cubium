# 药水物品模块

本目录包含所有药水相关物品的实现。

## 文件结构

```
potion/
├── GlassBottleItem.hpp/cpp     # 玻璃瓶物品（从水源装水）
├── PotionItem.hpp/cpp          # 饮用型药水（32 tick 饮用时间）
├── ThrowablePotionItem.hpp/cpp # 可投掷药水基类（投掷速度 0.5，覆写 getDispenseConfig() 返回 potion() 配置，新增 isLingering() 纯虚方法）
├── SplashPotionItem.hpp/cpp    # 喷溅药水（影响半径 4.0 格）
├── LingeringPotionItem.hpp/cpp # 滞留药水（产生滞留云）
└── README.md                   # 本文件
```

## 类继承关系

```
Item
├── ThrowableItem (投掷物品基类，位于 items/weapon/，实现 ProjectileItem 接口)
│   └── ThrowablePotionItem (可投掷药水基类，覆写 getDispenseConfig() 返回 potion() 配置)
│       ├── SplashPotionItem (喷溅药水)
│       └── LingeringPotionItem (滞留药水)
├── PotionItem (饮用型药水)
└── GlassBottleItem (玻璃瓶)
```

## 内部模块关系

- **ThrowablePotionItem** 是喷溅药水和滞留药水的共同基类，提供药水效果检测、翻译键生成、投掷音效等共享功能
- **ThrowablePotionItem** 通过继承 ThrowableItem 间接实现 ProjectileItem 接口，覆写 `getDispenseConfig()` 返回 `ProjectileDispenseConfig::potion()`（power=1.375, uncertainty=3.0）
- **ThrowablePotionItem** 实现 `createProjectileEntity()` 创建 PotionEntity 并设置物品堆和滞留属性，子类通过 `isLingering()` 控制药水类型
- **SplashPotionItem** 和 **LingeringPotionItem** 通过重写 `isLingering()` 方法区分药水类型，`createProjectile()` 由 ThrowableItem 基类统一处理
- **PotionItem** 独立继承自 Item，实现饮用逻辑
- **GlassBottleItem** 独立继承自 Item，实现装水逻辑

## 上下游外部依赖关系

### 上游依赖（本模块依赖的）

```
ThrowablePotionItem
├── ThrowableItem (基类，位于 items/weapon/，实现 ProjectileItem 接口)
├── ProjectileItem (通过 ThrowableItem 间接实现，覆写 getDispenseConfig() 返回 potion() 配置)
├── ProjectileDispenseConfig (发射器配置，potion() 预设：power=1.375, uncertainty=3.0)
├── PotionUtils (药水工具，位于 potion/)
├── SoundEvents (音效定义)
└── Random (随机数)

SplashPotionItem / LingeringPotionItem
├── ThrowablePotionItem (基类)
├── ProjectileItemEntity (投掷实体，位于 entity/projectile/)
├── Player
└── IWorld

PotionItem
├── Item (基类)
├── PotionUtils (药水工具)
├── UseAction (使用动作枚举)
└── Entity / IWorld

GlassBottleItem
├── Item (基类)
├── Fluids (流体定义，检测水源)
└── Items (获取水瓶物品)
```

### 下游依赖（依赖本模块的）

- **Items.hpp** - 注册所有药水物品
- **PotionEntity** - 处理喷溅/滞留药水的碰撞和效果应用
- **BrewingStandMenu** - 酿造台使用药水和玻璃瓶

## 容易踩的坑

### 1. GlassBottleItem 水源检测

**问题**：液体方块不提供可用的碰撞形状，纯命中测试不足以检测水源。

**解决方案**：需要正确检测水源方块（`Fluids::WATER` 且 `level == 0`），不能仅依赖碰撞形状。

### 2. 药水翻译键生成

**问题**：药水物品的翻译键需要根据药水效果类型动态生成，格式为 `item.minecraft.splash_potion.effect.<效果名>`。

**解决方案**：`ThrowablePotionItem::getTranslationKey()` 会读取 ItemStack 中的药水效果并生成正确的翻译键。

### 3. 滞留药水实体

**问题**：`LingeringPotionItem` 投掷后需要产生 `AreaEffectCloudEntity`（滞留云实体），目前该实体尚未完全实现。

**临时方案**：`PotionEntity` 中有 TODO 标记，滞留药水暂时使用简化的效果应用逻辑。

### 4. 投掷物品基类位置

**问题**：`ThrowableItem` 基类位于 `items/weapon/` 目录而非 `items/throw/`，容易被忽略。

**解决方案**：修改投掷药水相关代码时，需注意基类 `ThrowableItem` 定义在 `items/weapon/ThrowableItem.hpp`。

### 5. ThrowablePotionItem 的发射器配置

**说明**：`ThrowablePotionItem` 覆写了 `getDispenseConfig()` 返回 `ProjectileDispenseConfig::potion()`（power=1.375, uncertainty=3.0），与 ThrowableItem 默认配置不同。这意味着药水从发射器发射时散布减半、力度增加 25%，确保药水更准确地投向目标。

### 6. ThrowablePotionItem 的 isLingering() 纯虚方法

**说明**：`ThrowablePotionItem` 新增 `isLingering()` 纯虚方法，子类必须实现以区分喷溅药水（`SplashPotionItem` 返回 false）和滞留药水（`LingeringPotionItem` 返回 true）。此方法用于 `createProjectileEntity()` 中设置 PotionEntity 的滞留属性，替代了之前通过重写 `createProjectile()` 分别创建不同实体类型的方式。

## 参考

- MC 1.16.5: `net.minecraft.item.SplashPotionItem`
- MC 1.16.5: `net.minecraft.item.LingeringPotionItem`
- MC 1.16.5: `net.minecraft.item.PotionItem`
- MC 1.16.5: `net.minecraft.entity.projectile.PotionEntity`
- MC 1.16.5: `net.minecraft.entity.AreaEffectCloudEntity`
