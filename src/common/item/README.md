# Item 系统

Cubium 物品系统实现，包含核心物品类型、食物、盔甲、工具、附魔、合成等功能模块。

## 目录结构

```
item/
├── core/                         # 核心类型
│   ├── Item.hpp/cpp              # 物品基类（含 onCraftedBy/onCraftedPostProcess 合成回调）
│   ├── ItemStack.hpp/cpp         # 物品堆（数量、耐久、附魔等，含 onCraftedBy 桥接方法）
│   ├── ItemRegistry.hpp/cpp      # 物品注册表
│   ├── ItemGroup.hpp/cpp         # 创造模式物品组
│   ├── UseAction.hpp             # 使用动作枚举（无、吃、喝、阻挡等）
│   ├── ActionResult.hpp          # 动作结果类型
│   └── README.md
├── food/                         # 食物属性定义
│   ├── Food.hpp/cpp              # 食物属性结构（饥饿值、饱和度、效果）
│   ├── Foods.hpp/cpp             # 原版食物定义（苹果、面包等）
│   └── README.md
├── armor/                        # 盔甲材质
│   ├── ArmorMaterial.hpp/cpp     # 盔甲材质接口及原版材质定义
│   └── README.md
├── tier/                         # 工具材质等级
│   ├── IItemTier.hpp             # 材质接口（耐久、效率、伤害等）
│   ├── ItemTiers.hpp/cpp         # 原版材质（木、石、铜、铁、金、钻石、下界合金）
│   └── README.md
├── attribute/                    # 物品属性修饰符
│   ├── ItemAttributeModifiers.hpp/cpp  # 物品属性修饰符管理
│   └── README.md
├── context/                      # 物品使用上下文
│   ├── ItemUseContext.hpp/cpp    # 物品使用上下文（玩家、世界、位置等）
│   ├── BlockItemUseContext.hpp/cpp  # 方块物品放置上下文
│   └── README.md
├── tag/                          # 物品标签
│   ├── ItemTag.hpp/cpp           # 物品标签类
│   ├── ItemTags.hpp/cpp          # 物品标签注册表（FLOWERS、DAMPENS_VIBRATIONS、FIRE_RESISTANT、CHAINS、BARS、WOODEN_SHELVES、SHULKER_BOXES等）
│   ├── ItemTagLoader.hpp/cpp     # 物品标签数据包加载器（从JSON加载标签）
│   └── README.md
├── items/                        # 具体物品实现
│   ├── food/                     # 食物物品
│   │   ├── FoodItem.hpp/cpp      # 食物基类
│   │   ├── HoneyBottleItem.hpp/cpp  # 蜂蜜瓶（清除中毒）
│   │   ├── ChorusFruitItem.hpp/cpp  # 紫颂果（随机传送）
│   │   └── GoldenAppleItem.hpp/cpp  # 金苹果
│   ├── armor/                    # 盔甲物品
│   │   ├── ArmorItem.hpp/cpp     # 盔甲基类
│   │   ├── DyeableArmorItem.hpp/cpp  # 可染色盔甲
│   │   ├── ElytraItem.hpp/cpp    # 鞘翅
│   │   ├── HorseArmorItem.hpp/cpp  # 马铠
│   │   ├── WolfArmorItem.hpp/cpp # 狼铠（MC 1.20.5+）
│   │   └── NautilusArmorItem.hpp/cpp # 鹦鹉螺铠甲（MC 1.21.11）
│   ├── tool/                     # 工具物品
│   │   ├── ToolItem.hpp/cpp      # 工具基类
│   │   ├── TieredItem.hpp/cpp    # 层级物品基类
│   │   ├── PickaxeItem.hpp/cpp   # 镐
│   │   ├── AxeItem.hpp/cpp       # 斧（含原木去皮映射）
│   │   ├── ShovelItem.hpp/cpp    # 锹（含营火熄灭、土径创建）
│   │   ├── HoeItem.hpp/cpp       # 锄
│   │   ├── SwordItem.hpp/cpp     # 剑
│   │   ├── ShearsItem.hpp/cpp    # 剪刀
│   │   └── ToolType.hpp/cpp      # 工具类型枚举
│   ├── block/                    # 方块物品
│   │   ├── BlockItem.hpp/cpp     # 方块物品基类
│   │   ├── WallOrFloorItem.hpp/cpp  # 墙壁/地板物品（告示牌、旗帜等）
│   │   ├── BannerItem.hpp/cpp    # 旗帜物品
│   │   ├── BedItem.hpp/cpp       # 床物品（重写 getStateForPlacement 检查头部位置可替换性）
│   │   ├── GameMasterBlockItem.hpp/cpp  # 管理员方块物品（命令方块、结构方块等）
│   │   └── BlockItemRegistry.hpp/cpp  # 方块物品注册表
│   ├── weapon/                   # 武器物品
│   │   ├── BowItem.hpp/cpp       # 弓
│   │   ├── CrossbowItem.hpp/cpp  # 弩
│   │   ├── TridentItem.hpp/cpp   # 三叉戟
│   │   ├── ShieldItem.hpp/cpp    # 盾牌
│   │   ├── ArrowItem.hpp/cpp     # 普通箭矢
│   │   ├── TippedArrowItem.hpp/cpp  # 药水箭
│   │   ├── SnowballItem.hpp/cpp  # 雪球
│   │   ├── EggItem.hpp/cpp       # 鸡蛋
│   │   ├── EnderPearlItem.hpp/cpp  # 末影珍珠
│   │   └── ExperienceBottleItem.hpp/cpp  # 附魔之瓶
│   ├── potion/                   # 药水物品
│   │   ├── PotionItem.hpp/cpp    # 饮用型药水
│   │   ├── SplashPotionItem.hpp/cpp  # 喷溅药水
│   │   ├── LingeringPotionItem.hpp/cpp  # 滞留药水
│   │   ├── ThrowablePotionItem.hpp/cpp  # 可投掷药水基类
│   │   └── GlassBottleItem.hpp/cpp  # 玻璃瓶
│   ├── special/                  # 特殊物品
│   │   ├── BoneMealItem.hpp/cpp  # 骨粉
│   │   ├── FlintAndSteelItem.hpp/cpp  # 打火石
│   │   ├── HoneycombItem.hpp/cpp  # 蜜脾（涂蜡铜方块、除蜡映射）
│   │   ├── FishingRodItem.hpp/cpp  # 钓鱼竿
│   │   ├── EnchantedBookItem.hpp/cpp  # 附魔书
│   │   ├── KnowledgeBookItem.hpp/cpp  # 知识之书（右键解锁配方列表）
│   │   ├── NameTagItem.hpp/cpp   # 命名牌
│   │   └── ...                   # 其他特殊物品
│   ├── bucket/                   # 桶类物品
│   │   ├── BucketItem.hpp/cpp    # 空桶（支持对牛挤奶）
│   │   ├── WaterBucketItem.hpp/cpp  # 水桶
│   │   ├── LavaBucketItem.hpp/cpp  # 熔岩桶
│   │   ├── MilkBucketItem.hpp/cpp  # 牛奶桶（清除药水效果）
│   │   └── FishBucketItem.hpp/cpp  # 鱼桶（鳕鱼、鲑鱼、河豚、热带鱼）
│   ├── map/                      # 地图物品
│   │   ├── AbstractMapItem.hpp/cpp  # 地图基类
│   │   ├── EmptyMapItem.hpp/cpp  # 空地图
│   │   └── FilledMapItem.hpp/cpp  # 已填充地图（重写 onCraftedPostProcess 处理缩放/锁定）
│   └── BannerPatternItem.hpp/cpp  # 旗帜图案物品
├── enchantment/                  # 附魔系统
│   ├── Enchantment.hpp/cpp       # 附魔基类
│   ├── EnchantmentContainer.hpp/cpp  # 附魔容器（存储在 ItemStack 中）
│   ├── EnchantmentHelper.hpp/cpp  # 附魔工具函数
│   ├── EnchantmentRegistry.hpp/cpp  # 附魔注册表
│   └── enchantments/             # 具体附魔实现
│       ├── FortuneEnchantment.hpp/cpp  # 时运
│       ├── SilkTouchEnchantment.hpp/cpp  # 精准采集
│       ├── weapon/               # 武器附魔
│       │   ├── SharpnessEnchantment.hpp/cpp  # 锋利
│       │   ├── SmiteEnchantment.hpp/cpp  # 亡灵杀手
│       │   ├── BaneOfArthropodsEnchantment.hpp/cpp  # 节肢杀手
│       │   ├── FireAspectEnchantment.hpp/cpp  # 火焰附加
│       │   ├── KnockbackEnchantment.hpp/cpp  # 击退
│       │   ├── LootingEnchantment.hpp/cpp  # 抢夺
│       │   └── SweepingEnchantment.hpp/cpp  # 横扫之刃
│       ├── protection/           # 保护附魔
│       │   ├── ProtectionEnchantment.hpp/cpp  # 保护
│       │   ├── FireProtectionEnchantment.hpp/cpp  # 火焰保护
│       │   ├── BlastProtectionEnchantment.hpp/cpp  # 爆炸保护
│       │   ├── ProjectileProtectionEnchantment.hpp/cpp  # 弹射物保护
│       │   ├── FeatherFallingEnchantment.hpp/cpp  # 摔落缓冲
│       │   ├── RespirationEnchantment.hpp/cpp  # 水下呼吸
│       │   ├── AquaAffinityEnchantment.hpp/cpp  # 水下速掘
│       │   ├── DepthStriderEnchantment.hpp/cpp  # 深海探索者
│       │   ├── FrostWalkerEnchantment.hpp/cpp  # 冰霜行者
│       │   └── ThornsEnchantment.hpp/cpp  # 荆棘
│       ├── tool/                 # 工具附魔
│       │   ├── EfficiencyEnchantment.hpp/cpp  # 效率
│       │   └── UnbreakingEnchantment.hpp/cpp  # 耐久
│       ├── bow/                  # 弓附魔
│       │   ├── PowerEnchantment.hpp/cpp  # 力量
│       │   ├── PunchEnchantment.hpp/cpp  # 冲击
│       │   ├── FlameEnchantment.hpp/cpp  # 火矢
│       │   └── InfinityEnchantment.hpp/cpp  # 无限
│       ├── crossbow/             # 弩附魔
│       │   ├── MultishotEnchantment.hpp/cpp  # 多重射击
│       │   ├── PiercingEnchantment.hpp/cpp  # 穿透
│       │   └── QuickChargeEnchantment.hpp/cpp  # 快速装填
│       ├── trident/              # 三叉戟附魔
│       │   ├── LoyaltyEnchantment.hpp/cpp  # 忠诚
│       │   ├── ChannelingEnchantment.hpp/cpp  # 引雷
│       │   └── RiptideEnchantment.hpp/cpp  # 激流
│       ├── fishing/              # 钓鱼竿附魔
│       │   ├── LuckOfTheSeaEnchantment.hpp/cpp  # 海之眷顾
│       │   └── LureEnchantment.hpp/cpp  # 饵钓
│       └── special/              # 特殊附魔
│           ├── MendingEnchantment.hpp/cpp  # 经验修补
│           ├── BindingCurseEnchantment.hpp/cpp  # 绑定诅咒
│           ├── VanishingCurseEnchantment.hpp/cpp  # 消失诅咒
│           └── SoulSpeedEnchantment.hpp/cpp  # 灵魂疾行
├── crafting/                     # 合成系统
│   ├── IRecipe.hpp               # 配方接口
│   ├── Ingredient.hpp/cpp        # 原料匹配器（支持物品/标签/合并，含延迟标签解析）
│   ├── RecipeManager.hpp/cpp     # 配方管理器
│   ├── RecipeLoader.hpp/cpp      # 配方加载器（从数据包加载）
│   ├── RecipeBook.hpp/cpp        # 配方书
│   ├── RecipeSerializers.hpp/cpp  # 配方序列化器
│   ├── RecipeNetworkSerializer.hpp/cpp  # 配方网络同步
│   ├── ShapedRecipe.hpp/cpp      # 有序合成
│   ├── ShapelessRecipe.hpp/cpp   # 无序合成
│   ├── SmeltingRecipe.hpp/cpp    # 熔炼配方
│   ├── StonecuttingRecipe.hpp/cpp  # 石切配方
│   ├── SmithingRecipe.hpp/cpp    # 锻造配方
│   ├── SpecialRecipe.hpp/cpp     # 特殊配方基类
│   └── special/                  # 特殊配方
│       ├── ArmorDyeRecipe.hpp/cpp  # 盔甲染色
│       ├── BannerDuplicateRecipe.hpp/cpp  # 旗帜复制
│       ├── BookCloningRecipe.hpp/cpp  # 书复制
│       ├── MapCloningRecipe.hpp/cpp  # 地图复制
│       ├── MapExtendingRecipe.hpp/cpp  # 地图扩展
│       ├── RepairItemRecipe.hpp/cpp  # 物品修复
│       ├── ShieldDecorationRecipe.hpp/cpp  # 盾牌装饰
│       └── TippedArrowRecipe.hpp/cpp  # 药水箭合成
├── loot/                         # 战利品表系统
│   └── ...                       # 战利品表相关文件
├── potion/                       # 药水效果工具
│   └── PotionUtils.hpp/cpp       # 药水工具函数（自定义效果、颜色等）
├── Items.hpp/cpp                 # 原版物品注册入口
└── README.md                     # 本文件
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                        Items.hpp/cpp                            │
│                    （原版物品注册入口）                            │
└─────────────────────────────────────────────────────────────────┘
                               │
        ┌──────────────────────┼──────────────────────┐
        ▼                      ▼                      ▼
┌───────────────┐     ┌───────────────┐     ┌───────────────┐
│    core/      │     │    items/     │     │ enchantment/  │
│  物品基础类型   │◄────│  具体物品实现  │────►│   附魔系统    │
│ Item/Stack/   │     │               │     │               │
│ Registry/     │     └───────┬───────┘     └───────────────┘
└───────────────┘             │
        ▲                     │
        │              ┌──────┴──────┐
        │              ▼             ▼
┌───────────────┐  ┌────────┐  ┌────────┐
│   context/    │  │ armor/ │  │  tool/ │
│ 物品使用上下文  │  │ 盔甲材质 │  │工具材质│
└───────────────┘  └────────┘  └────────┘
                               │
                               ▼
                      ┌───────────────┐
                      │  attribute/   │
                      │ 属性修饰符系统 │
                      └───────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                        crafting/                                │
│                         合成系统                                 │
│  IRecipe ← ShapedRecipe/ShapelessRecipe/SmeltingRecipe/...      │
│                          │                                      │
│                    Ingredient                                   │
│                          │                                      │
│                   RecipeManager                                 │
└─────────────────────────────────────────────────────────────────┘
```

**关键依赖链**：
- `ItemStack` → `Item`（核心物品堆依赖物品基类）
- `ItemStack` → `EnchantmentContainer`（物品堆包含附魔）
- `Item` → `ItemGroup`（物品归属创造模式物品组）
- `ArmorItem/ToolItem` → `IItemTier`/`ArmorMaterial`（具体物品依赖材质定义）
- `FoodItem` → `Food`（食物物品依赖食物属性）
- `BlockItem` → `Block`（方块物品依赖方块类型）
- `RecipeManager` → `IRecipe` → `Ingredient`（配方系统依赖链）

## 上下游外部依赖关系

### 上游依赖（本模块依赖的外部模块）

| 外部模块 | 依赖内容 |
|---------|---------|
| `common/world/block/` | `Block`、`BlockState`（BlockItem 放置方块） |
| `common/world/IWorld.hpp` | 世界接口（物品使用、方块放置） |
| `common/entity/` | `Player`、`LivingEntity`、`IEntity`（物品交互主体） |
| `common/entity/attribute/` | `Attribute`、`AttributeModifier`（属性修饰符） |
| `common/util/nbt/` | NBT 序列化（ItemStack 数据存储） |
| `common/util/math/random/` | `Random`（随机数生成） |
| `common/util/math/ray/` | `Raycast`（物品使用射线检测） |
| `common/core/ResourceLocation.hpp` | 资源位置标识符 |
| `common/network/` | 数据包序列化（配方同步、物品数据） |

### 下游依赖（依赖本模块的外部模块）

| 外部模块 | 依赖内容 |
|---------|---------|
| `server/player/ServerPlayer.hpp` | 玩家物品栏、物品使用 |
| `server/interaction/InventoryManager.hpp` | 背包管理 |
| `server/interaction/BlockInteractionManager.hpp` | 方块交互（物品使用） |
| `server/interaction/MiningManager.hpp` | 挖掘管理（工具耐久） |
| `server/world/drop/BlockDropHandler.hpp` | 方块掉落物生成 |
| `server/world/entity/ItemPickupManager.hpp` | 物品拾取 |
| `server/network/ServerPlayRouter.cpp` | 物品相关入站数据包处理（UseEntity/UseItem 分支） |
| `server/command/commands/` | GiveCommand、FillCommand 等命令 |
| `client/world/entity/ClientEntity.hpp` | 客户端实体物品渲染 |
| `common/world/blockentity/` | 方块实体物品交互（试炼机关等） |

## 容易踩的坑

### 1. 物品堆栈大小限制

不同物品有不同的堆栈大小限制（通常为 64，部分物品为 16 或 1）。创建 `ItemStack` 时需要检查 `maxStackSize`。

### 2. 盔甲自动装备槽位冲突

盔甲物品支持右键自动装备对应槽位；如果目标槽位已被占用，则保持原物品不变并返回透传结果。

### 3. DyeableArmorItem 颜色标签管理

**问题**：`DyeableArmorItem` 将颜色存储在 `ItemStack` 的结构化标签树中，清除颜色时未清除空的 `display` 标签会导致元数据相等性发散。

**解决方案**：清除颜色时也必须清除空的 `display` 标签，否则盔甲堆将停止按预期合并。

### 4. GlassBottleItem 水源检测

**问题**：`GlassBottleItem` 在决定瓶子是否可以装满之前，沿玩家视线进行采样，液体方块不提供可用的碰撞形状，纯命中测试不足以检测水源。

**解决方案**：需要正确检测水源方块，不能仅依赖碰撞形状。

### 5. CreativeInventory 初始化顺序

**问题**：`CreativeInventory` 相关测试和启动代码如果初始化顺序错误，创造物品库会出现空列表或缺失方块物品。

**解决方案**：必须按 `VanillaBlocks::initialize()` → `Items::initialize()` → `BlockItemRegistry::instance().initializeVanillaBlockItems()` 的顺序初始化。

### 6. CraftingMenu 菜单验证

**问题**：`CraftingMenu::stillValid()` 需要正确检查玩家到工作台的距离。

**解决方案**：保持工作台可访问性绑定到方块实体位置，使容器有效性匹配预期的交互范围。

### 7. ChestContainer 和 FurnaceContainer 需要真正的 PlayerInventory

**问题**：通过遗留的 `Container` 路由创建箱子/熔炉 GUI 会导致功能不完整。

**解决方案**：`ChestContainer` 和 `FurnaceContainer` 现在需要真正的 `PlayerInventory` 并位于 `AbstractContainerMenu` 下，使用共享菜单工厂/打开容器钩子。

### 8. InventoryManager 背包同步回调

**问题**：服务器侧背包变更如果走 `inventoryManager()`，但没有设置回调，客户端不会收到更新。

**解决方案**：`InventoryManager::setOnInventoryUpdate()` 在 `MinecraftServer::initializeInteractionManagers()` 里已经接好，服务器侧背包变更如果走 `inventoryManager()`，就要依赖这条回调刷新客户端，不要再手写一套新的同步分支。

### 9. BlockItem 放置上下文需要非 const IWorld

**问题**：`BlockItemUseContext` 需要修改世界状态（放置方块、消耗物品等），因此需要非 const 的 `IWorld&` 引用。

**解决方案**：`ItemUseContext` 和 `BlockItemUseContext` 使用非 const `IWorld&` 引用，支持 `setBlockState` 等修改操作。

### 10. WallOrFloorItem 用于可挂墙物品

**问题**：告示牌、旗帜、头颅等物品既可以放在地上也可以贴在墙上，需要特殊处理。

**解决方案**：使用 `WallOrFloorItem` 类，根据玩家视线方向自动选择放置地板方块或墙壁方块。

### 11. BlockItem 实体碰撞检查

**问题**：方块放置时需要检查是否与实体碰撞，否则玩家可以将方块放置到其他实体内部。

**解决方案**：`BlockItem::canPlace()` 在放置前检查方块的碰撞箱是否与实体相交：
- 使用 `CollisionShape::getWorldBoxes()` 获取方块的世界坐标碰撞箱
- 调用 `IWorld::hasEntityCollision()` 检查实体碰撞
- 排除放置者实体本身，避免玩家阻止自己放置方块
- 对无碰撞箱的方块（如水、空气）跳过此检查

**参考**：MC 1.16.5 `world.func_226663_a_(state, pos, ISelectionContext.dummy())`

### 12. 附魔回调触发时机

**问题**：附魔效果（如节肢杀手的缓慢、荆棘的反伤）需要在正确的时机触发。

**解决方案**：
- `LivingEntity::onAttackEntity()` 触发攻击回调 → `EnchantmentHelper::applyArthropodEnchantmentDamage()`
- `LivingEntity::actuallyHurt()` 触发受伤回调 → `EnchantmentHelper::applyThornsEnchantments()`

### 13. 合成回调 onCraftedBy/onCraftedPostProcess

**问题**：物品合成后需要进行特殊后处理（如地图缩放/锁定），需要在合成取出时自动触发。

**解决方案**：`Item::onCraftedBy(ItemStack&, IWorld&, Player&)` 和 `Item::onCraftedPostProcess(ItemStack&, IWorld&)` 构成双层回调。`onCraftedBy` 由 `ItemStack::onCraftedBy()` 在 `ServerPlayer::onItemCrafted()` 中调用，默认转发给 `onCraftedPostProcess`。子类重写 `onCraftedPostProcess` 执行合成后处理。`FilledMapItem` 重写此方法处理 `map_scale_direction`（缩放）和 `map_lock`（锁定）NBT 标签。

### 14. 玩家→实体物品交互调用链

`Item::itemInteractionForEntity()` 完整调用链路：
```
客户端 UseEntityPacket → 服务端 ServerPlayRouter 的 UseEntity 分支
    → Player::interactOn(entity, hand)
    → Item::itemInteractionForEntity() [BucketItem/ShearsItem/NameTagItem]
```

已实现：`BucketItem`（对牛挤奶）、`ShearsItem`（剪羊毛/雪傀儡/哞菇）、`NameTagItem`（对实体命名）。

### 15. MC 1.21+ 数据包目录命名兼容

**问题**：MC 1.21+ 数据包使用单数目录名（`loot_table/`、`recipe/`、`predicate/`、`function/`），而旧版使用复数（`loot_tables/`、`recipes/`、`predicates/`、`functions/`）。加载器必须同时支持两种形式。

**解决方案**：所有资源加载器（`LootTableLoader`、`RecipeLoader`、`LootPredicateLoader`、`FunctionLoader`）的路径过滤和 ID 推导逻辑均已更新，同时匹配单数和复数目录名。`ItemTagLoader` 不受影响（使用 `listResourceStacks` 精确定位 `tags/item/` 目录，不做路径子串匹配）。

### 16. MC 1.21+ 配方 JSON 格式兼容

**问题**：MC 1.21+ 配方 JSON 格式有两项重大变更：`ingredient` 字段支持字符串格式（如 `"minecraft:raw_iron"`），`result` 字段使用 `"id"` 替代 `"item"`。

**解决方案**：`RecipeSerializers::parseIngredient()` 现在支持字符串格式（自动转为 `{"item": "..."}` 对象格式解析），`parseResult()` 同时支持 `"id"` 和 `"item"` 字段（`"item"` 优先以保持向后兼容）。`RecipeLoader` 也支持从 `recipe/` 和 `recipes/` 两种目录加载。

### 17. 粗矿物品注册（Raw Ore Items）

**新增物品**：`RAW_IRON`、`RAW_COPPER`、`RAW_GOLD`（粗矿物品）及其对应方块物品 `RAW_IRON_BLOCK`、`RAW_COPPER_BLOCK`、`RAW_GOLD_BLOCK`。

- 粗矿物品注册在 `Items::_registerMaterials()` 中，使用 `registry.registerItem()`
- 粗矿块物品使用 `registerBlockBackedItem()` 绑定到对应 `VanillaBlocks` 方块
- 方块映射在 `BlockItemRegistry::initializeVanillaBlockItems()` 中通过 `registerSimpleBlock()` 完成
- `SCUTE` 重命名为 `TURTLE_SCUTE`（ID 从 `minecraft:scute` 更改为 `minecraft:turtle_scute`，MC 1.20.5+ 变更）
- 数据包中的战利品表、配方、标签文件已存在，加载器修复后可正常加载

### 18. 物品伤害源判断 canBeHurtBy

`ItemStack::canBeHurtBy(const DamageSource&)` 方法判断物品堆是否能被指定伤害源伤害。防火物品（`FIRE_RESISTANT` 标签，如下界合金物品、下界星）不会被火焰和岩浆伤害源摧毁。

调用链：`ItemEntity::hurt()` → `ItemStack::canBeHurtBy(source)` → `Item::isIn(ItemTags::FIRE_RESISTANT()) && source.isFire()`

与此相关的还有 `ItemEntity::isImmuneToFire()` 重写，它检查物品是否防火来决定物品实体的火焰免疫性。

### 16. 头颅物品注册

头颅物品（骷髅头颅、凋灵骷髅头颅、玩家头颅、僵尸头、苦力怕头、龙首、猪灵头）注册在 `Items::_registerSkulls()` 中。

当前注册为普通 `Item`（最大堆叠 64），因为 `SkullBlock` / `WallSkullBlock` 尚未实现。MC Java 中头颅使用 `StandingAndWallBlockItem`（本项目对应 `WallOrFloorItem`），可放置在地板或墙壁上。待方块系统完善后应升级为 `WallOrFloorItem` 注册。

`FillPlayerHeadFunction` 使用 `Items::PLAYER_HEAD` 进行物品类型检查（引用相等性比较），与 MC Java 的 `stack.is(Items.PLAYER_HEAD)` 行为一致。只有玩家头颅物品会被填充玩家档案（SkullOwner），其他头颅类型不受影响。

### 19. 锁链物品注册与 CHAINS 标签

MC 1.21+ 将原 `minecraft:chain` 重命名为 `minecraft:iron_chain`，与铜锁链命名风格统一。所有铜锁链变体（4个氧化变种 + 4个涂蜡变种）均已注册为 BlockItem。

**物品注册**：
- 铁锁链：`Items::CHAIN`，注册为 `minecraft:iron_chain`（BlockItem，堆叠64）
- 铜锁链：8个变种均通过 `BlockItemRegistry::initializeVanillaBlockItems()` 注册

**CHAINS 标签**：
- `BlockTags::CHAINS()` 包含铁锁链 + 8个铜锁链方块 = 9项
- `ItemTags::CHAINS()` 包含铁锁链 + 8个铜锁链物品 = 9项
- 对应 MC 原版标签 `minecraft:chains`

**铜锁链涂蜡/刮蜡集成**：
- `WeatheringCopperChainBlock` 继承 `IOxidizableBlock`，支持斧头刮蜡/除锈
- `HoneycombItem::WAXABLES_MAP` 包含铜锁链的涂蜡映射（未涂蜡→涂蜡）
- 与其他铜方块（铜块、铜栏杆、铜门等）使用相同的铜氧化机制

### 20. 床物品注册

16色床物品使用自定义 `BedItem` 子类注册，而非普通的 `BlockItem`。`BedItem` 继承 `BlockItem` 并重写 `getStateForPlacement()`，以检查床头位置的可替换性。

**注册方式**：`Items::_registerBeds()` 中通过 `registry.registerItem<BedItem>()` 注册，每种颜色关联对应的 `VanillaBlocks::XXX_BED` 方块，最大堆叠数为 1。

**BedItem 核心职责**：
- 重写 `getStateForPlacement()`：根据玩家朝向设置 `HORIZONTAL_FACING` 属性，检查头部位置（`placementPos.offset(facing)`）是否可替换（`canBeReplaced()`），不可替换时返回 `nullptr` 阻止放置
- 放置后由 `BedBlock::onBlockPlacedBy()` 自动在脚部前方放置头部方块
- 返回脚部（FOOT）状态的默认朝向，头部由 `onBlockPlacedBy` 自动创建

**与普通 BlockItem 的区别**：普通 `BlockItem` 的 `getStateForPlacement()` 直接委托给方块的 `getStateForPlacement()`，而 `BedItem` 自行实现检查逻辑，确保双格结构（头部+脚部）的完整性。

**16色床物品**：WHITE_BED、ORANGE_BED、MAGENTA_BED、LIGHT_BLUE_BED、YELLOW_BED、LIME_BED、PINK_BED、GRAY_BED、LIGHT_GRAY_BED、CYAN_BED、PURPLE_BED、BLUE_BED、BROWN_BED、GREEN_BED、RED_BED、BLACK_BED

### 21. 刷怪蛋物品注册

MC 1.21.11 共 87 种刷怪蛋物品，统一注册在 `Items::_registerSpawnEggs()` 中。每种刷怪蛋注册名为 `minecraft:xxx_spawn_egg`，最大堆叠数 64。

**注册方式**：`registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:xxx_spawn_egg"), makeEntityTypeForSpawnEgg("minecraft:xxx"), primaryColor, secondaryColor, ItemProperties().maxStackSize(64))`

**EntityType 构造**：`SpawnEggItem` 持有 `EntityType` 副本（不可拷贝、可移动），但 `EntityType` 的工厂返回 `nullptr`——`SpawnEggItem` 内部 `EntityType` 仅作为名称载体。实际实体生成由 `MobEntity::_spawnOffspringFromSpawnEgg`（右键生物生成幼体）或 `SpawnEggItem::spawnEntity`（右键方块生成实体）通过 `EntityRegistry::getType(name)->create(world, registry)` 完成，因此刷怪蛋内 `EntityType` 的工厂从不被实际调用。

`makeEntityTypeForSpawnEgg(const char*)` 是 `Items.cpp` 匿名命名空间中的辅助函数，使用 `EntityType::Builder` + `const_cast<std::string&>(type.name())` 写入实体注册名（与 `EntityRegistry::registerType` 内部一致），规避 `EntityType` 不可拷贝的约束。

**颜色数据来源**：
- 历史型刷怪蛋（1.16.5 之前 Java 内置）：沿用 MC Java `SpawnEggItem` 的 `background`/`foreground` (ARGB) 常量，该数据在各版本间保持稳定
- 新增实体刷怪蛋（1.17+ 实验性/未实现实体）：从原版资源包纹理 `assets/minecraft/textures/item/xxx_spawn_egg.png` 提取主色/次色
- MC 1.21.11 已将颜色从 Java 代码迁移至客户端纹理，本项目中颜色仅作为 API 字段保留（`SpawnEggItem::getPrimaryColor/getSecondaryColor`），不参与服务端逻辑

**两种使用路径**：
1. **右键方块**：`SpawnEggItem::onItemUse` 在方块面上方生成对应实体。生成位置对齐 Java `useOn`：点击方块碰撞形状为空（草、花等）时在方块自身位置生成，否则在面偏移位置生成；生成位置不可替换时返回 `Fail`。生成成功后触发 `ENTITY_PLACE` 振动事件并消耗物品（非创造模式）。刷怪笼方块走单独分支：设置刷怪笼实体类型并触发 `BLOCK_CHANGE` 事件。
2. **右键生物**：`MobEntity::processInitialInteract` 检测手持刷怪蛋 → `_spawnOffspringFromSpawnEgg` 比较刷怪蛋实体名与目标实体 `getTypeId()`，匹配时通过 `EntityRegistry::getType(getTypeId())->create(world, registry)` 生成幼体（仅 `AgeableEntity` 子类支持，非年龄型实体不生成）
3. **右键空气/液体**：`SpawnEggItem::onItemRightClick` 沿视线做液体射线检测（对齐 Java `getPlayerPOVHitResult(Fluid.SOURCE_ONLY)`），在首个水源方块位置生成实体；命中水源时触发 `ENTITY_PLACE` 事件、消耗物品（非创造模式）并记录 `ITEM_USED` 统计，未命中水源返回 `Pass`

**`spawnEntity` 反查真实工厂**：刷怪蛋持有的 `EntityType` 副本工厂为空（仅作名称载体），`SpawnEggItem::spawnEntity` 必须通过 `EntityRegistry::instance().getType(m_entityType.name())` 反查真实 `EntityType`，再调 `realType->create(&world, *registry)` 生成实体；反查失败（未知实体类型）返回 `false`。生成前还有和平难度检查（对齐 Java `isAllowedInPeaceful`）：怪物类实体（`EntityClassification::Monster`）在和平难度不生成。生成后对 `MobEntity` 调用 `finalizeSpawn` 进行基于区域难度的装备初始化。

**注意事项**：
- 刷怪蛋类型匹配使用实体类型名称字符串比较（如 `"minecraft:pig"`），而非 `EntityType` 对象比较，避免 `EntityType` 不可拷贝的问题
- 客户端预测：`isClientSide() == true` 时直接返回 `Success`，不消耗物品、不生成实体（实际由服务端处理）
- 创造模式下不消耗刷怪蛋物品
- 服务端生成实体后经 `ServerWorld::spawnEntity` → `EntityTracker::notifyEntityTracked` → `_sendSpawnPacket` 向视距内在线玩家下发 `AddEntity` 包，`entityTypeId` 取 vanilla 1.21.11 `entity_type` 注册表 id（`Entity::getJavaEntityTypeId` 按 name 查 `JavaEntityTypeIdMap`），Java 客户端据此 spawn 对应实体
