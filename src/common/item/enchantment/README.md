# Enchantment 附魔系统

本目录实现了 Minecraft 1.16.5 风格的附魔系统，包含全部 34 种原版附魔。

## 目录结构

```
enchantment/
├── Enchantment.hpp              # 附魔基类定义
├── Enchantment.cpp              # 附魔基类实现
├── EnchantmentContainer.hpp     # 附魔容器（存储物品上的附魔）
├── EnchantmentContainer.cpp     # 附魔容器实现
├── EnchantmentHelper.hpp        # 附魔查询工具类
├── EnchantmentHelper.cpp        # 附魔查询工具类实现
├── EnchantmentRegistry.hpp      # 附魔注册表
├── EnchantmentRegistry.cpp      # 附魔注册表实现
└── enchantments/                # 具体附魔实现
    ├── AllEnchantments.hpp      # 所有附魔统一包含
    ├── AllEnchantments.cpp      # 所有附魔注册
    ├── FortuneEnchantment.hpp   # 时运附魔
    ├── SilkTouchEnchantment.hpp # 精准采集附魔
    ├── protection/              # 保护类附魔（11种）
    ├── weapon/                  # 武器类附魔（7种）
    ├── tool/                    # 工具类附魔（4种）
    ├── bow/                     # 弓类附魔（4种）
    ├── fishing/                 # 钓鱼类附魔（2种）
    ├── trident/                 # 三叉戟附魔（4种）
    ├── crossbow/                # 弩类附魔（3种）
    └── special/                 # 特殊附魔（4种）
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                     EnchantmentRegistry                      │
│                    （全局注册表，单例模式）                    │
└──────────────────────────┬──────────────────────────────────┘
                           │ 注册/查询
                           ▼
┌─────────────────────────────────────────────────────────────┐
│                      Enchantment                             │
│                    （附魔抽象基类）                           │
│  • 定义附魔属性：ID、等级、类型、稀有度                        │
│  • 定义回调接口：onEntityDamaged、onUserHurt                  │
│  • 定义兼容性检查：isCompatibleWith                           │
└──────────────────────────┬──────────────────────────────────┘
                           │ 继承
        ┌──────────────────┼──────────────────┐
        ▼                  ▼                  ▼
┌───────────────┐  ┌───────────────┐  ┌───────────────┐
│ProtectionEnch │  │ DamageEnch    │  │ 其他具体附魔   │
│  保护类基类    │  │  伤害类基类    │  │  Fortune等    │
└───────────────┘  └───────────────┘  └───────────────┘

┌─────────────────────────────────────────────────────────────┐
│                   EnchantmentContainer                       │
│                （附魔存储容器，归属于ItemStack）               │
│  • 存储附魔ID和等级                                           │
│  • 支持NBT/JSON/网络序列化                                    │
│  • 兼容性检查（canAdd）                                       │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    EnchantmentHelper                         │
│                    （静态工具类）                             │
│  • 查询物品附魔等级、特定附魔检测                              │
│  • 计算伤害加成、保护值                                       │
│  • 附魔回调分发：applyArthropodEnchantments、applyThorns      │
│  • 附魔台生成：calcItemStackEnchantability、buildEnchantmentList│
└─────────────────────────────────────────────────────────────┘
```

**关键依赖流：**
- `EnchantmentRegistry` 持有所有 `Enchantment` 实例
- `EnchantmentContainer` 通过 ID 引用 `Enchantment`
- `EnchantmentHelper` 提供统一查询入口，不持有状态
- 具体附魔类继承 `Enchantment`，实现回调方法

## 上下游外部依赖关系

**上游依赖（本目录依赖的外部模块）：**
- `mc::core::Result` - 错误处理
- `mc::core::Types` - 基础类型（i32, u8, f32 等）
- `mc::math::Random` - 随机数生成
- `mc::nbt` - NBT 序列化
- `mc::network::PacketSerializer/Deserializer` - 网络序列化
- `nlohmann::json` - JSON 序列化
- `ItemStack` - 物品堆（canApply、canApplyAtEnchantingTable 参数）
- `LivingEntity/Entity/Player` - 回调方法参数

**下游依赖（依赖本模块的外部模块）：**
- `ItemStack` - 持有 `EnchantmentContainer` 存储附魔
- `LivingEntity` - 调用 `EnchantmentHelper::applyArthropodEnchantmentDamage` 和 `applyThornsEnchantments`
- `AnvilMenu` - 铁砧合并附魔，检查兼容性
- `EnchantmentScreen` - 附魔台 GUI，计算附魔选项
- `LootTable` - 战利品表中的 `ApplyBonusFunction` 使用时运计算
- `ProjectileEntity` - 投射物命中时调用 `applyArthropodEnchantments`

**调用链示例：**
```
LivingEntity::onAttackEntity()
    → EnchantmentHelper::applyArthropodEnchantmentDamage()
        → Enchantment::onEntityDamaged()
            → BaneOfArthropodsEnchantment::onEntityDamaged() [施加缓慢]

LivingEntity::actuallyHurt()
    → EnchantmentHelper::applyThornsEnchantments()
        → ThornsEnchantment::onUserHurt() [反伤]
```

## 容易踩的坑

### 1. 附魔互斥关系

部分附魔互斥，不能同时存在于同一物品上：

| 附魔组 | 互斥附魔 |
|--------|----------|
| 保护类 | 保护、火焰保护、爆炸保护、弹射物保护互斥（摔落保护除外） |
| 伤害类 | 锋利、亡灵杀手、节肢杀手互斥 |
| 工具类 | 时运与精准采集互斥 |
| 弓附魔 | 无限与经验修补互斥 |
| 三叉戟 | 激流与忠诚、引雷互斥 |
| 弩附魔 | 多重射击与穿透互斥 |
| 靴子附魔 | 深海探索者与冰霜行者互斥 |

使用 `EnchantmentContainer::canAdd()` 检查兼容性。

### 2. 附魔回调的正确触发位置

- `onEntityDamaged` - 在 **攻击者攻击目标后** 触发，用于节肢杀手施加缓慢
- `onUserHurt` - 在 **持有者受伤时** 触发，用于荆棘反伤
- 投射物命中后需调用 `applyArthropodEnchantments`（遍历攻击者所有装备）

### 3. 时运效果不由附魔直接处理

时运附魔的效果由战利品表的 `ApplyBonusFunction` 处理，不在 `FortuneEnchantment` 类中实现逻辑。三种时运公式：
- `OreDrops`: 钻石矿、绿宝石矿
- `Uniform`: 煤矿、红石矿
- `Binomial`: 树叶、萤石

### 4. 耐久计算的概率性

`UnbreakingEnchantment::shouldIgnoreDurabilityLoss()` 每次耐久消耗时调用，返回 true 表示忽略损耗：
- 工具：`(level + 1) / (level + 1)` 概率忽略
- 护甲：额外有 60% 概率不触发耐久效果

### 5. EnchantmentHelper 部分方法有默认参数违规

`getEnchantmentDatas()`、`buildEnchantmentList()`、`addRandomEnchantment()` 有默认参数 `allowTreasure = false`，违反项目规范，调用时应显式传参。

### 6. 荆棘反伤的耐久消耗

荆棘触发反伤时会消耗护甲耐久，需要在 `ThornsEnchantment::onUserHurt` 中处理，不要在调用方处理。

### 7. 附魔注册时机

`EnchantmentRegistry::initialize()` 必须在游戏启动时调用，在使用任何附魔之前完成注册。注册后的附魔实例由注册表持有所有权。
