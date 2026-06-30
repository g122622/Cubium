# 特殊物品模块 (Special Items)

特殊物品模块提供功能性物品的实现。

## 目录结构

```text
special/
├── README.md                    # 本文档
├── BoneMealItem.hpp/cpp         # 骨粉（加速植物生长、海草生成）
├── BucketItem.hpp/cpp           # 桶（空桶、水桶、岩浆桶、细雪桶）
├── EnchantedBookItem.hpp/cpp    # 附魔书（存储附魔）
├── FishBucketItem.hpp/cpp       # 鱼桶（放置水并生成鱼）
├── FlintAndSteelItem.hpp/cpp    # 打火石（点火、点燃下界传送门）
├── HoneycombItem.hpp/cpp        # 蜜脾（涂蜡铜方块、涂蜡告示牌阻止文字修改）
├── MilkBucketItem.hpp/cpp       # 牛奶桶（清除药水效果）
├── MusicDiscItem.hpp/cpp        # 音乐唱片（放入唱片机播放，比较器信号1-15）
├── LeadItem.hpp/cpp             # 拴绳（绑定生物到栅栏、玩家右键生物拴住/解除）
├── NameTagItem.hpp/cpp          # 命名牌（给生物命名、持久化）
├── OnAStickItem.hpp/cpp         # 钓竿类物品基类（控制可骑乘实体）
├── PotterySherdItem.hpp/cpp     # 陶片（饰纹陶罐合成材料，关联DecoratedPotPattern枚举）
├── SaddleItem.hpp/cpp           # 鞍（装备可骑乘实体）
├── SmithingTemplateItem.hpp/cpp # 锻造模板（盔甲纹饰/下界合金升级，锻造台配方模板）
├── SpawnEggItem.hpp/cpp         # 生成蛋（右键生成实体）
├── StickItems.hpp/cpp           # 具体钓竿物品（胡萝卜钓竿、诡异菌钓竿）
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│ 特殊物品模块 (Special Items)                                │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐  继承    ┌──────────────┐                 │
│  │ OnAStickItem │◄─────────│CarrotOnAStick│                 │
│  │ (基类)       │          │ Item         │                 │
│  └──────┬──────┘          └──────────────┘                 │
│         │                  ┌───────────────┐                │
│         └──────────────────│WarpedFungus   │                │
│            继承            │OnAStickItem   │                │
│                           └───────────────┘                │
│                                                             │
│ OnAStickItem 控制 IRideable 实体（猪、炽足兽）              │
│ SaddleItem 装备 IEquipable 实体（猪、炽足兽、马等）          │
│ HoneycombItem 维护 WAXABLES/WAX_OFF_MAP 铜块涂蜡/除蜡映射，也支持告示牌涂蜡 │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

**内部依赖（本模块依赖）：**
- `Item` 基类、`ItemStack`、`ItemActionResult`（物品系统核心）
- `IRideable`、`IEquipable` 接口（实体交互）
- `IGrowable` 接口（骨粉与植物交互）
- `Fluid`、`FluidState`（桶与流体交互）
- `EntityType`（生成蛋、鱼桶创建实体）
- `enchant::Enchantment`（附魔书存储附魔）

**外部依赖（谁依赖本模块）：**
- `Items` 静态注册表（物品注册）
- `Player` 玩家实体（物品使用）
- 实体模块（猪、炽足兽、牛等实体与物品交互）
- `VanillaBlocks` 铜方块注册表（HoneycombItem 涂蜡映射依赖）
- `SignEntity` 告示牌方块实体（HoneycombItem 告示牌涂蜡交互依赖）
- `AxeItem` 除蜡逻辑（使用 HoneycombItem::getWaxedOff 查询除蜡映射）

## 容易踩的坑

1. **OnAStickItem 耐久度消耗顺序**：MC 1.16.5 中，先触发 `IRideable::boost()` 加速，再消耗耐久度
2. **钓鱼竿转换**：耐久度耗尽后转换为钓鱼竿，需确保 `Items::FISHING_ROD` 已注册
3. **实体类型匹配**：OnAStickItem 使用字符串 ID 匹配（如 `"minecraft:pig"`），需与实体注册 ID 一致
4. **canBeSteered 条件**：需同时满足：有鞍 + 有乘客 + 玩家手持正确钓竿
5. **IRideable::boost() 返回值**：加速可能失败（已在加速中或没有鞍），需检查返回值
6. **BoneMealItem 水下使用**：需检查目标位置是否为完整水源方块（流体等级==8）
7. **SpawnEggItem 实体类型不可拷贝**：`EntityType` 的拷贝构造函数是 deleted 的，`getEntityType()` 返回 `const EntityType&`（引用），构造函数参数按值传递后需用 `std::move` 初始化成员
8. **SpawnEggItem 命名空间**：`spawnEntity()` 方法中的 `SpawnReason` 属于 `world::spawn` 命名空间，非 `entity` 命名空间
9. **MusicDiscItem 信号强度**：比较器输出范围[1, 15]，构造函数有 `MC_ASSERT_RELEASE_MSG` 断言。JukeboxBlock::onBlockActivated() 通过 `isMusicDisc()` 识别唱片，JukeboxEntity::getComparatorSignal() 通过 `dynamic_cast<MusicDiscItem*>` 获取信号强度
10. **HoneycombItem 涂蜡映射使用 "construct on first use" 模式**：`getWaxablesMap()` 和 `getWaxOffMap()` 是函数局部静态变量，首次调用时初始化。必须确保 `VanillaBlocks` 已初始化后再调用，否则所有铜方块指针为 nullptr，映射表将为空
11. **HoneycombItem::getWaxedOff 供 AxeItem 使用**：AxeItem 除蜡逻辑调用此静态方法，无需实例化 HoneycombItem
12. **HoneycombItem 告示牌涂蜡路径**：`onItemUse()` 先检测告示牌 SignEntity 再检测铜块。`AbstractSignBlock::onBlockActivated()` 也实现了涂蜡交互（检测蜜脾手持物品），两条路径互为补充
13. **PotterySherdItem 关联 DecoratedPotPattern**：每个陶片物品持有 DecoratedPotPattern 枚举值，该枚举与 MC 原版 DecoratedPotPattern 一一对应。待 DecoratedPotBlockEntity 实现后需建立陶片物品到图案的双向映射
14. **SmithingTemplateItem 提示文本为翻译键占位**：appliesTo/ingredients/baseSlotDescription/additionsSlotDescription 字段当前存储翻译键字符串，待翻译系统完善后应改为 ITextComponent
15. **SmithingTemplateItem 与锻造台配方系统**：此类当前仅提供物品标识和提示信息，待 TrimPattern 注册表和 SmithingTrimRecipe/SmithingTransformRecipe 实现后需进行集成
16. **LeadItem 拴绳交互流程**：`onItemUse()` 只对栅栏方块生效（`BlockTags::FENCES()`），搜索半径16格内被当前玩家拴住的生物（`mob->leashHolderUuid() == player->uuid()`），通过 `LeashKnotEntity::getOrCreateKnot()` 创建/复用拴绳结，调用 `mob->setLeashedToFence()` 转移绑定。玩家右键生物拴住/解除拴绳的逻辑在 `MobEntity::processInitialInteract()` 中
