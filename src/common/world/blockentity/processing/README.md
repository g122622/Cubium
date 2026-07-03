#加工类方块实体模块

提供熔炉、高炉、烟熏炉、酿造台、信标、潮涌核心、营火等加工类方块实体的实现。

##目录结构

``` processing /
├── AbstractFurnaceEntity.hpp / cpp #熔炉基类（燃烧 / 熔炼逻辑、ISidedInventory）
├── FurnaceEntity.hpp / cpp #普通熔炉（200tick熔炼）
├── BlastFurnaceEntity.hpp / cpp #高炉（100tick、仅矿石 / 金属）
├── SmokerEntity.hpp / cpp #烟熏炉（100tick、仅食物）
├── FurnaceInventory.hpp / cpp #熔炉专用3槽背包
├── BrewingStandEntity.hpp / cpp #酿造台（药水酿造、ISidedInventory）
├── BeaconEntity.hpp / cpp #信标（金字塔效果、光束渲染）
├── ConduitEntity.hpp / cpp #潮涌核心（水下信标、攻击敌对生物）
├── CampfireBlockEntity.hpp /
        cpp #营火（食物烹饪、4槽位）
└── README.md
```

        ##内部模块关系

``` BlockEntity(父模块基类)
       ↑
       │ ContainerBlockEntity(父模块容器基类)
       ↑
       ├──────────────────────┬──────────────────────┐
       │                      │                      │ LockableBlockEntity BrewingStandEntity
        CampfireBlockEntity(core / 可锁定容器基类)(多重继承 ISidedInventory)
       ↑
       │ AbstractFurnaceEntity(熔炉基类，多重继承 ISidedInventory)
       ↑
       ├──────────────────┬──────────────────┐
       │                  │                  │ FurnaceEntity BlastFurnaceEntity SmokerEntity(普通熔炉)(高炉)(烟熏炉)

            BeaconEntity(信标，独立继承 BlockEntity) ConduitEntity(潮涌核心，独立继承 BlockEntity)
                FurnaceInventory(熔炉背包，非 BlockEntity，被 AbstractFurnaceEntity 组合)
```

        ##上下游外部依赖关系

        ## #上游依赖（谁使用了这个模块）

    - `world / block / blocks /` - 熔炉方块、酿造台方块、信标方块等创建和访问方块实体
    - `world / chunk /` - 区块加载时反序列化方块实体
    - `entity / inventory / container /` - 熔炉 GUI 容器、酿造台 GUI 容器 - `client / renderer /` -
    信标光束渲染、熔炉火焰渲染

    ## #下游依赖（这个模块依赖了谁）

    - `world / blockentity / BlockEntity.hpp` - 方块实体基类
    - `world / blockentity / ContainerBlockEntity.hpp` - 容器方块实体基类
    - `world / blockentity / core / LockableBlockEntity.hpp` - 可锁定基类
    - `world / blockentity / core / SimpleInventory.hpp` - 简单背包实现
    - `entity / inventory / IInventory.hpp` - 背包接口 - `entity / inventory / ISidedInventory.hpp` - 分面背包接口
    - `item / crafting / SmeltingRecipe.hpp` - 熔炼配方 - `item / crafting / CampfireCookingRecipe.hpp` - 营火烹饪配方
    - `item / potion / PotionBrewing.hpp` - 药水酿造 - `entity / effect / EffectType.hpp` - 效果类型（信标、潮涌核心）
    - `entity / interfaces / IMob.hpp` -
    敌对生物接口（潮涌核心攻击目标）

        ##容易踩的坑

        ## #1. 熔炼进度回退

        不燃烧时进度应该回退 2，而非清零。这是 MC 1.16.5 的行为。

        ## #2. 高炉
        /
        烟熏炉燃料消耗速度

        高炉和烟熏炉的 `getBurnTimeForFuel()` 返回基础燃烧时间的一半，即燃料消耗速度是普通熔炉的 2 倍。

        ## #3. 输出槽满检查

        熔炼前必须检查输出槽是否可以接受产物，检查条件包括：输出槽为空，或输出槽物品可以堆叠且堆叠后不超过最大堆叠数。

        ## #4. 配方缓存

        每次输入变化时重新查询配方，避免每 tick 重复查询。`m_lastRecipe` 缓存上次使用的配方。

        ## #5. ISidedInventory 槽位访问规则

        熔炉：
    - 上方(Up)：输入槽（槽位 0） - 下方(Down)：输出槽（槽位 2）、燃料槽（槽位 1） -
    侧面：燃料槽（槽位 1）

    酿造台：
    - 上方(Up)：材料槽（槽位 3） - 下方(Down)：药水瓶槽 + 材料槽（槽位 0,
    1, 2, 3） - 侧面：药水瓶槽 + 燃料槽（槽位 0, 1, 2,
    4）

    ## #6. 燃料系统（getBurnTimeByItem）

`AbstractFurnaceEntity::getBurnTimeByItem()` 实现了 MC Java `FuelValues.vanillaBurnTimes()` 的燃料注册逻辑。

    ####燃烧时间表

    | 燃烧时间(tick) | 物品 | | -- -- -- -- -- -- -- --| -- -- --| | 20000 | 岩浆桶 | | 16000 | 煤炭块 | | 2400 | 烈焰棒
    | | 1600 | 煤炭、木炭 | | 1200 | 木船、带箱木船 | | 800 | 悬挂告示牌（所有可燃木材） | | 4001 | 干海带块 | | 300 |
    原木、木板、木头、去皮原木 /
        木头、楼梯、栅栏、栅栏门、活板门、压力板、书架、雕纹书架、音符盒、合成台、光照探测器、梯子、箱子、陷阱箱、织布机、木桶、制图台、制箭台、锻造台、堆肥桶、讲台、唱片机、弓、钓鱼竿、弩、旗帜、红树木根
    | | 200 | 木质门、木制工具、告示牌 | | 150 | 木质台阶 | |
    100 | 木棍、碗、树苗、木质按钮、杜鹃花、枯草、落叶、羊毛、地毯、死灌木 | | 50 | 竹子、脚手架 |

    ####NON_FLAMMABLE_WOOD 排除

            MC Java 使用 `NON_FLAMMABLE_WOOD` 标签排除下界木材（绯红
            / Crimson、诡异 / Warped），这些物品燃烧时间为 0：

        - 绯红茎、诡异茎、绯红木板、诡异木板 - 绯红 / 诡异楼梯、台阶、栅栏、栅栏门、门、活板门、压力板、按钮 -
        绯红 /
            诡异告示牌、悬挂告示牌

            当前实现通过** 不在燃料列表中注册** 这些物品来实现排除，而非标签查询。当标签系统完善后可重构为标签驱动的方式。

            ####新增木材类型的燃料注册

            红树(Mangrove)、樱花(Cherry)、苍白橡木(Pale Oak)、竹木(Bamboo)
                的燃烧时间与主世界木材一致。这些木材类型的方块物品已在 `BlockItemRegistry` 中注册，告示牌物品已在 `Items` 中注册（通过 `WallOrFloorItem`），代码中使用 `isBlockInList()` 和 `isItemInList()` 判断燃料类型。

            ####带箱子的船燃料注册

        带箱子的船（Chest Boat）燃烧时间与普通船相同，均为 1200 tick。10 种带箱子的船物品已在 `Items` 中注册（使用 `BoatItem` 类，`hasChest = true`），燃料条目已添加到 `getBurnTimeByItem()` 中与普通船合并判断。

        ## #7. 熔炉类型对比

    | 特性 | 普通熔炉 | 高炉 | 烟熏炉 | | -- -- -| -- -- -- -- -| -- -- --| -- -- -- -| | 熔炼时间 | 200 tick | 100 tick
    | 100 tick | | 配方类型 | SMELTING | BLASTING | SMOKING | | 可熔炼物 | 全部 | 仅矿石 / 金属 | 仅食物 | | 经验倍率 |
    1.0 | 0.5 | 0.5 | | 燃料消耗 | 正常 | 2倍速度 | 2倍速度 |

    ## #8. 信标效果范围

    效果范围 = `level * 10 +
    10` 格，需要正确计算金字塔等级。

    ## #9. 潮涌核心目标追踪

`m_target` 是运行时指针，`m_targetUuid` 用于持久化。恢复时使用 `_findExistingTarget()` 在攻击范围内搜索。

    ## #10. 潮涌核心含水检测

`_isWaterAt()` 使用 `IWorld::isWaterAt(
        pos)` 进行流体状态检查（而非仅检查水方块），因此正确覆盖水方块和含水方块两种情况。不要改为 `BlockState::
        is(VanillaBlocks::WATER)`，那会遗漏含水方块。

    ## #11. 营火冷却速度

    熄灭时烹饪进度每 tick 减少 2，而非清零。点燃后从上次进度继续。

    ## #12. 酿造台材料槽提取限制

`canExtractItem()` 对材料槽（槽位 3）只允许提取玻璃瓶，其他物品不能提取。

    ## #13. 酿造台自定义名称（铁砧重命名传递）

`BrewingStandEntity` 重写 `getCustomName()` / `setCustomName()` 维护独立的 `m_customName` 字段，对应 MC Java `BaseContainerBlockEntity` 的 `customName` 字段。序列化由 `ContainerBlockEntity::load/save` 自动处理 `CustomName` JSON 字段，无需本类额外编码。

放置时由 `BrewingStandBlock::onBlockPlacedBy()` 从放置物品继承自定义名称（对应 MC Java `BaseContainerBlockEntity.applyImplicitComponents` 机制）：

```cpp
void BrewingStandBlock::onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    if (stack.hasCustomName()) {
        BlockEntity* entity = world.getBlockEntity(pos);
        if (entity != nullptr && entity->getType() == BlockEntityType::BrewingStand) {
            auto* brewingStand = static_cast<blockentity::BrewingStandEntity*>(entity);
            brewingStand->setCustomName(stack.getCustomName());
        }
    }
}
```

`clone()` 同步拷贝 `m_customName`，确保方块实体复制时自定义名称不丢失。
