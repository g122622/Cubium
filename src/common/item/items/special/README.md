# 特殊物品模块 (Special Items)

特殊物品模块提供功能性物品的实现。

## 目录结构

```text
special/
├── README.md                    # 本文档
├── BoneMealItem.hpp/cpp         # 骨粉（加速植物生长、海草生成）
├── BrushItem.hpp/cpp            # 刷子（考古学工具，刷可疑方块和犰狳）
├── BucketItem.hpp/cpp           # 桶（空桶、水桶、岩浆桶）
├── bundle/                      # 收纳袋子模块（1 无色 + 16 色 = 17 变体）
│   ├── BundleContents.hpp/cpp   #   收纳袋内容物数据结构（重量系统、序列化）
│   └── BundleItem.hpp/cpp       #   收纳袋物品（插槽覆盖协议、内容物管理）
├── EnchantedBookItem.hpp/cpp    # 附魔书（存储附魔）
├── FishBucketItem.hpp/cpp       # 鱼桶（放置水并生成鱼）
├── FlintAndSteelItem.hpp/cpp    # 打火石（点火、点燃下界传送门）
├── HarnessItem.hpp/cpp          # 欢乐诡鬼装备（16色变体，装备 HappyGhast，无护甲值无耐久）
├── HoneycombItem.hpp/cpp        # 蜜脾（涂蜡铜方块、涂蜡告示牌阻止文字修改）
├── MilkBucketItem.hpp/cpp       # 牛奶桶（清除药水效果）
├── MusicDiscItem.hpp/cpp        # 音乐唱片（放入唱片机播放，比较器信号1-15）
├── KnowledgeBookItem.hpp/cpp   # 知识之书（右键解锁配方列表，NBT存储recipes数组）
├── LeadItem.hpp/cpp             # 拴绳（绑定生物到栅栏、玩家右键生物拴住/解除）
├── NameTagItem.hpp/cpp          # 命名牌（给生物命名、持久化）
├── OnAStickItem.hpp/cpp         # 钓竿类物品基类（控制可骑乘实体）
├── PotterySherdItem.hpp/cpp     # 陶片（饰纹陶罐合成材料，关联DecoratedPotPattern枚举）
├── PowderSnowBucketItem.hpp/cpp # 细雪桶（放置细雪方块、使用后返回空桶）
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
│ HarnessItem 持有 DyeColor，装备交互待 HappyGhastEntity 集成 │
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
9. **SpawnEggItem 刷怪笼分支**：`onItemUse()` 在常规生成逻辑之前先检测点击的方块是否为 `MobSpawnerBlockEntity`，若是则设置刷怪笼的实体类型而非生成生物。此分支与 `SpawnerBlock::onBlockActivated()` 中的刷怪蛋检测逻辑配合，确保无论交互入口如何（方块优先回调或物品回调），刷怪蛋都能正确设置刷怪笼实体类型。非创造模式下消耗 1 个刷怪蛋
10. **SpawnEggItem 注册**：87 种刷怪蛋统一注册在 `Items::_registerSpawnEggs()` 中，注册名为 `minecraft:xxx_spawn_egg`。`SpawnEggItem` 内部 `EntityType` 仅作为名称载体（工厂为空），实际实体生成由 `MobEntity::_spawnOffspringFromSpawnEgg` 或 `SpawnEggItem::spawnEntity` 通过 `EntityRegistry::getType(name)->create()` 完成。颜色数据来源：历史型沿用 MC Java 1.16.5 内置 background/foreground (ARGB)，新增实体从原版资源包纹理提取。MC 1.21.11 已将颜色迁移至客户端纹理，本项目中颜色仅作为 API 字段保留，不参与服务端逻辑。详见 `src/common/item/README.md` 第 21 节
11. **MusicDiscItem 信号强度**：比较器输出范围[1, 15]，构造函数有 `MC_ASSERT_RELEASE_MSG` 断言。JukeboxBlock::onBlockActivated() 通过 `isMusicDisc()` 识别唱片，JukeboxEntity::getComparatorSignal() 通过 `dynamic_cast<MusicDiscItem*>` 获取信号强度
12. **HoneycombItem 涂蜡映射使用 "construct on first use" 模式**：`getWaxablesMap()` 和 `getWaxOffMap()` 是函数局部静态变量，首次调用时初始化。必须确保 `VanillaBlocks` 已初始化后再调用，否则所有铜方块指针为 nullptr，映射表将为空
13. **HoneycombItem::getWaxedOff 供 AxeItem 使用**：AxeItem 除蜡逻辑调用此静态方法，无需实例化 HoneycombItem
14. **HoneycombItem 告示牌涂蜡路径**：`onItemUse()` 先检测告示牌 SignEntity 再检测铜块。`AbstractSignBlock::onBlockActivated()` 也实现了涂蜡交互（检测蜜脾手持物品），两条路径互为补充
15. **PotterySherdItem 关联 DecoratedPotPattern**：每个陶片物品持有 DecoratedPotPattern 枚举值，该枚举与 MC 原版 DecoratedPotPattern 一一对应。待 DecoratedPotBlockEntity 实现后需建立陶片物品到图案的双向映射
16. **SmithingTemplateItem tooltip 通过 LanguageManager 翻译**：appliesTo/ingredients/baseSlotDescription/additionsSlotDescription 字段存储翻译键，addInformation() 通过 LanguageManager::get() 翻译后显示。tooltip 格式完整复刻 MC Java（后缀标题、空行、标题+描述）。baseSlotDescription/additionsSlotDescription 用于锻造台界面，尚未集成
17. **SmithingTemplateItem 与锻造台配方系统**：此类当前仅提供物品标识和提示信息，待 TrimPattern 注册表和 SmithingTrimRecipe/SmithingTransformRecipe 实现后需进行集成。空槽位图标路径（baseSlotEmptyIcons/additionalSlotEmptyIcons）尚未添加
18. **LeadItem 拴绳交互流程**：`onItemUse()` 只对栅栏方块生效（`BlockTags::FENCES()`），搜索半径16格内被当前玩家拴住的生物（`mob->leashHolderUuid() == player->uuid()`），通过 `LeashKnotEntity::getOrCreateKnot()` 创建/复用拴绳结，调用 `mob->setLeashedToFence()` 转移绑定。玩家右键生物拴住/解除拴绳的逻辑在 `MobEntity::processInitialInteract()` 中
19. **BrushItem 刷扫机制**（对齐 MC 1.21.11 `BrushItem`）：
    - **触发时机**：`onUseTick` 中 `elapsedTicks % ANIMATION_DURATION(10) == BRUSH_TICK_IN_CYCLE(4) + 1` 即 `elapsedTicks = 5, 15, 25, ...` 时触发刷扫（`elapsedTicks` 从1开始）
    - **射线检测**：每次刷扫触发 tick 通过 `calculateHitResult` 从玩家眼睛位置沿视线方向做方块射线检测，最大距离取自 `Player::blockInteractionRange()`（生存/冒险 4.5，创造 5.0，由 `generic.block_interaction_range` 属性决定）；未命中时调用 `stopActiveHand()` 取消使用（对齐 MC `releaseUsingItem`）
    - **粒子生成**：命中方块且 `shouldSpawnTerrainParticles() && !isInvisibleRenderType()` 时，调用 `spawnDustParticles` 生成 7~11 个 `Block` 粒子（对齐 MC `random.nextInt(7, 12)`），粒子方向由 `DustParticlesDelta::fromDirection` 按命中面方向计算，主手/副手决定方向镜像
    - **音效播放**：命中 `BrushableBlock` 时使用其绑定的 `getBrushSound()`（可疑沙=BRUSH_SAND、可疑沙砾=BRUSH_GRAVEL），其他方块使用 `BRUSH_GENERIC`，音量1.0、音调1.0、类别 `Blocks`
    - **非玩家实体**：`onUseTick` 检查 `dynamic_cast<Player*>(&entity)`，非玩家实体立即 `stopActiveHand()`
    - **刷扫完成与耐久消耗**：仅在服务端且命中方块实体为 `BrushableBlockEntity` 时调用 `brushableEntity->brush(gameTime, world, entity, hitDirection, stack)`。`brush()` 返回 true 表示刷扫完成（累计 10 次刷扫），此时调用 `LivingEntity::hurtAndBreak(stack, 1, &entity, handToEquipmentSlot(activeHand))` 消耗 1 点耐久。完成时的物品掉落、`BRUSH_BLOCK_COMPLETE (3008)` 世界事件、方块转换为由 `BrushableBlockEntity::brushingCompleted()` 内部处理（详见 `world/blockentity/interactive/README.md` 第 #18 条）
    - **犰狳交互**：`itemInteractionForEntity` 当前返回 false。TODO: 待 `ArmadilloEntity` 实现后，添加刷犰狳掉落 `armadillo_scute` 物品、播放 `ARMADILLO_BRUSH` 音效、消耗 `ARMADILLO_DURABILITY_COST(16)` 耐久的逻辑
20. **HarnessItem 装备交互待集成**：当前 HarnessItem 仅作为普通物品注册（含 DyeColor 成员），无护甲值无耐久，颜色为物品固有属性（非 NBT 染色），与 HorseArmorItem 多实例模式一致，不要误用 DyeableArmorItem 的 NBT 染色机制。TODO: 实现 itemInteractionForEntity（右键装备 HappyGhast）、剪刀剪下逻辑、HARNESS_EQUIP/HARNESS_UNEQUIP 音效、EntityTypeTags::CAN_EQUIP_HARNESS 标签，需待 HappyGhastEntity 实现后集成。合成与染色配方由数据包驱动（datapacks/Vanilla 下 32 个配方文件已就绪），创造模式物品栏由 CreativeInventory::buildCreativePaletteEntries 自动遍历所有注册物品，无需手动注册
