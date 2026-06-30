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
│   ├── ItemTiers.hpp/cpp         # 原版材质（木、石、铁、金、钻石、下界合金）
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
│   ├── ItemTags.hpp/cpp          # 物品标签注册表（FLOWERS等）
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
│   │   └── HorseArmorItem.hpp/cpp  # 马铠
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
| `server/core/PacketHandler.cpp` | 物品相关数据包处理 |
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
客户端 UseEntityPacket → 服务端 PacketHandler::handleUseEntity()
    → Player::interactOn(entity, hand)
    → Item::itemInteractionForEntity() [BucketItem/ShearsItem/NameTagItem]
```

已实现：`BucketItem`（对牛挤奶）、`ShearsItem`（剪羊毛/雪傀儡/哞菇）、`NameTagItem`（对实体命名）。

### 15. 物品伤害源判断 canBeHurtBy

`ItemStack::canBeHurtBy(const DamageSource&)` 方法判断物品堆是否能被指定伤害源伤害。防火物品（`FIRE_RESISTANT` 标签，如下界合金物品、下界星）不会被火焰和岩浆伤害源摧毁。

调用链：`ItemEntity::hurt()` → `ItemStack::canBeHurtBy(source)` → `Item::isIn(ItemTags::FIRE_RESISTANT()) && source.isFire()`

与此相关的还有 `ItemEntity::isImmuneToFire()` 重写，它检查物品是否防火来决定物品实体的火焰免疫性。

### 16. 头颅物品注册

头颅物品（骷髅头颅、凋灵骷髅头颅、玩家头颅、僵尸头、苦力怕头、龙首、猪灵头）注册在 `Items::_registerSkulls()` 中。

当前注册为普通 `Item`（最大堆叠 64），因为 `SkullBlock` / `WallSkullBlock` 尚未实现。MC Java 中头颅使用 `StandingAndWallBlockItem`（本项目对应 `WallOrFloorItem`），可放置在地板或墙壁上。待方块系统完善后应升级为 `WallOrFloorItem` 注册。

`FillPlayerHeadFunction` 使用 `Items::PLAYER_HEAD` 进行物品类型检查（引用相等性比较），与 MC Java 的 `stack.is(Items.PLAYER_HEAD)` 行为一致。只有玩家头颅物品会被填充玩家档案（SkullOwner），其他头颅类型不受影响。
