#BlockEntity 模块

方块实体系统，用于存储方块状态无法表示的复杂数据。

##目录结构

``` blockentity /
├── BlockEntity.hpp #方块实体基类
├── BlockEntity.cpp #基类实现
├── BlockEntityType.hpp #方块实体类型枚举
├── BlockEntityType.cpp #类型转换函数
├── ContainerBlockEntity.hpp #容器方块实体基类
├── ContainerBlockEntity.cpp #容器方块实体实现
├── BLOCK_ENTITY_PLAN.md #方块实体补全计划
├── core / #核心基础设施
│   ├── BlockEntityRegistry.hpp / cpp #方块实体注册表（工厂方法创建实例）
│   ├── BlockEntityDeserializer.hpp / cpp #NBT反序列化器（从存档数据创建实例）
│   ├── LockableBlockEntity.hpp / cpp #可锁定容器基类（支持钥匙锁定和自定义名称）
│   ├── LootableContainerBlockEntity.hpp / cpp #可填充战利品表的容器基类
│   ├── SimpleInventory.hpp / cpp #通用背包实现（用于箱子 / 漏斗等容器）
│   └── README.md
├── storage / #存储类方块实体
│   ├── ChestEntity.hpp / cpp #箱子实体（27格存储、双箱合并）
│   ├── TrappedChestEntity.hpp / cpp #陷阱箱实体（输出红石信号）
│   ├── DoubleSidedInventory.hpp / cpp #双箱合并容器（54格）
│   ├── EnderChestEntity.hpp / cpp #末影箱实体（玩家独立存储）
│   ├── ShulkerBoxEntity.hpp / cpp #潜影盒实体（保留物品、防递归嵌套）
│   ├── BarrelEntity.hpp / cpp #木桶实体（27格、无上方方块限制）
│   └── README.md
├── transport / #传输类方块实体
│   ├── IHopper.hpp / cpp #漏斗接口（统一处理漏斗方块和漏斗矿车）
│   ├── HopperEntity.hpp / cpp #漏斗实体（物品传输、ISidedInventory）
│   └── README.md
├── processing / #加工类方块实体
│   ├── AbstractFurnaceEntity.hpp / cpp #熔炉基类（燃烧 / 熔炼逻辑、ISidedInventory）
│   ├── FurnaceEntity.hpp / cpp #普通熔炉（200tick熔炼）
│   ├── BlastFurnaceEntity.hpp / cpp #高炉（100tick、仅矿石 / 金属）
│   ├── SmokerEntity.hpp / cpp #烟熏炉（100tick、仅食物）
│   ├── FurnaceInventory.hpp / cpp #熔炉专用3槽背包
│   ├── BrewingStandEntity.hpp / cpp #酿造台（药水酿造、ISidedInventory）
│   ├── BeaconEntity.hpp / cpp #信标（金字塔效果、光束渲染）
│   ├── ConduitEntity.hpp / cpp #潮涌核心（水下信标、攻击敌对生物）
│   ├── CampfireBlockEntity.hpp / cpp #营火（食物烹饪、4槽位）
│   └── README.md
├── redstone / #红石类方块实体
│   ├── CommandBlockEntity.hpp / cpp #命令方块实体（脉冲 / 循环 / 连锁三种模式）
│   ├── ComparatorEntity.hpp / cpp #比较器实体（存储输出信号强度）
│   ├── DaylightDetectorEntity.hpp / cpp #日光探测器实体（定期更新信号）
│   └── README.md
├── interactive / #交互类方块实体
│   ├── EnchantingTableEntity.hpp / cpp #附魔台（附魔力量计算、书本动画）
│   ├── PistonBlockEntity.hpp / cpp #活塞实体（方块移动动画）
│   ├── DispenserBlockEntity.hpp / cpp #发射器方块实体基类（9格存储、战利品表填充）
│   ├── DropperBlockEntity.hpp / cpp #投掷器方块实体（继承DispenserBlockEntity）
│   ├── SignEntity.hpp / cpp #告示牌（富文本存储、点击事件）
│   ├── BannerEntity.hpp / cpp #旗帜（图案存储、最多6层）
│   ├── BannerPattern.hpp / cpp #旗帜图案类型定义
│   ├── BeehiveBlockEntity.hpp / cpp #蜂巢（蜜蜂存储、蜂蜜等级管理）
│   ├── JukeboxEntity.hpp / cpp #唱片机（唱片播放、1槽位）
│   ├── LecternEntity.hpp / cpp #讲台（书本展示、翻页、红石信号）
│   ├── ShelfBlockEntity.hpp / cpp #书架（3槽位物品存储、比较器3位二进制信号、侧链连接）
│   ├── EndGatewayEntity.hpp / cpp #末地折跃门（传送逻辑、两套冷却时间）
│   ├── DecoratedPotPattern.hpp / cpp #饰纹陶罐图案类型定义（24种图案：Blank + 20考古学 +
        3试炼密室）
│   ├── DecoratedPotBlockEntity.hpp / cpp #饰纹陶罐（四面图案PotDecorations、1格物品容器、摇晃动画、比较器信号）
│   ├── CopperGolemStatueBlockEntity.hpp / cpp #铜傀儡雕像（CUSTOM_NAME存储、removeStatue复活铜傀儡）
│   ├── BrushableBlockEntity.hpp / cpp #可刷方块实体（可疑沙/可疑沙砾，考古战利品表、刷扫计数与冷却、DUSTED状态、完成后转换为普通方块）
│   └── README.md
└── trial / #试炼相关方块实体
    ├── TrialSpawnerBlockEntity.hpp / cpp #试炼刷怪笼
    ├── VaultBlockEntity.hpp / cpp #宝库
    ├── CrafterBlockEntity.hpp / cpp #自动合成器（9格存储、槽位锁定、6tick合成动画）
    └── README.md
└── spawner / #刷怪笼方块实体
    ├── MobSpawnerBlockEntity.hpp / cpp #刷怪笼（实体生成逻辑、加权候选列表、NBT持久化）
    └── README.md
└── sculk / #幽匿方块实体
    ├── SculkSensorBlockEntity.hpp / cpp #幽匿感测体（振动检测、频率输出、VibrationSystem::Data 序列化）
    ├── SculkShriekerBlockEntity.hpp /
            cpp #幽匿尖啸体（振动检测、警告等级递增、VibrationSystem::Data 序列化）
    └── README.md
```

            ##内部模块关系

``` BlockEntity(基类)
│
├── ContainerBlockEntity(容器基类)
│   │
│   ├── LockableBlockEntity(可锁定容器基类，mc::blockentity 命名空间)
│   │   │
│   │   ├── LootableContainerBlockEntity(可填充战利品表的容器基类)
│   │   │   │
│   │   │   ├── ChestEntity(箱子) → TrappedChestEntity(陷阱箱)
│   │   │   ├── BarrelEntity(木桶)
│   │   │   ├── ShulkerBoxEntity(潜影盒，多重继承 ISidedInventory)
│   │   │   └── DispenserBlockEntity(发射器) → DropperBlockEntity(投掷器)
│   │   │
│   │   ├── HopperEntity(漏斗，多重继承 IHopper)
│   │   │
│   │   └── AbstractFurnaceEntity(熔炉基类，多重继承 ISidedInventory)
│   │       ├── FurnaceEntity(普通熔炉)
│   │       ├── BlastFurnaceEntity(高炉)
│   │       └── SmokerEntity(烟熏炉)
│   │
│   ├── BrewingStandEntity(酿造台，多重继承 ISidedInventory)
│   ├── CampfireBlockEntity(营火)
│   ├── JukeboxEntity(唱片机)
│   ├── CrafterBlockEntity(自动合成器，9格 + 槽位锁定)
│   ├── ShelfBlockEntity(雕纹书架)
│   └── DecoratedPotBlockEntity(饰纹陶罐，四面图案PotDecorations、1格物品容器、摇晃动画、比较器信号)
│
├── EnchantingTableEntity(附魔台)
├── PistonBlockEntity(活塞)
├── CommandBlockEntity(命令方块，实现 ICommandSource)
├── ComparatorEntity(比较器)
├── DaylightDetectorEntity(日光探测器)
├── BeaconEntity(信标)
├── ConduitEntity(潮涌核心)
├── SignEntity(告示牌)
├── BannerEntity(旗帜)
├── BeehiveBlockEntity(蜂巢)
├── LecternEntity(讲台)
├── EndGatewayEntity(末地折跃门)
├── EnderChestEntity(末影箱)
├── TrialSpawnerBlockEntity(试炼刷怪笼)
├── VaultBlockEntity(宝库)
├── CrafterBlockEntity(自动合成器)
├── MobSpawnerBlockEntity(刷怪笼)
├── SculkSensorBlockEntity(幽匿感测体)
└── SculkShriekerBlockEntity(幽匿尖啸体)

                BlockEntityRegistry ──创建──→ BlockEntity（及其子类） BlockEntityDeserializer ──反序列化──→
            BlockEntity（通过 Registry 创建）
```

            ##上下游外部依赖关系

            ## #上游依赖（谁使用了这个模块）

        - `world / chunk /` -
        区块加载时通过 `BlockEntityDeserializer` 反序列化方块实体，区块保存时调用 `save()` - `world / block / blocks
            /` -
        方块类（ChestBlock、FurnaceBlock、HopperBlock 等）创建和访问对应的方块实体
        - `server /` - 服务器启动时调用 `BlockEntityRegistry::registerBuiltinTypes()`，处理玩家交互时访问方块实体
        - `client / renderer /` - 客户端渲染方块实体（信标光束、活塞动画等） - `entity / inventory / container /` -
        GUI 容器类访问方块实体的背包

        ## #下游依赖（这个模块依赖了谁）

        - `world / block / BlockPos.hpp` - 方块位置 - `world / block / BlockState.hpp` - 方块状态
        - `resource / ResourceLocation.hpp` - 资源位置 - `entity / inventory / IInventory.hpp` - 背包接口
        - `entity / inventory / ISidedInventory.hpp` - 分面背包接口（熔炉、漏斗、潜影盒）
        - `entity / inventory / CraftingInventory.hpp` - 合成背包 - `entity / ItemEntity.hpp` - 物品实体（漏斗捕获）
        - `item / crafting / RecipeManager.hpp` - 配方管理 - `item / crafting / SmeltingRecipe.hpp` - 熔炼配方
        - `entity / loot / LootTableManager.hpp` - 战利品表管理器 - `util / nbt / Nbt.hpp` - NBT 序列化
        - `command / ICommandSource.hpp` - 命令源接口（命令方块） - `world / gameevent / VibrationSystem.hpp` -
        振动系统（幽匿感测体、幽匿尖啸体）

            ##容易踩的坑

            ## #1. 忘记调用 setChanged()

                修改方块实体数据后必须调用 `setChanged()` 触发区块保存，否则数据可能丢失。

            ## #2. getBlockState() 尚未实现

            当前 `BlockEntity::getBlockState()` 返回 `nullptr`，需要 World 类支持后才能实现。使用前需要检查返回值。

            ## #3. onlyOpsCanSetNbt() 权限控制

`BlockEntity::onlyOpsCanSetNbt()` 虚方法默认返回 false，命令方块等方块实体重写返回
            true。当玩家通过物品放置方块实体时，`BlockItem::setTileEntityNBT()` 会检查此标志，仅 OP
            玩家可设置受保护的方块实体 NBT。

            ## #3. 线程安全问题

`tick()` 和 `load()`/`save()` 可能在不同线程调用，需要注意：
        - 熔炉等需要 tick 的方块实体应该使用互斥锁保护数据 - 静态方块实体（如箱子）可以返回 `needsTick() ==
    false` 避免不必要的开销

        ## #4. 打开计数下溢

`closeContainer()` 不会让计数变为负数，但不匹配的 open
        /
        close 调用会导致计数错误。确保每次 open 都有对应的 close。

        ## #6. 类型转换失败处理

`blockEntityTypeFromId()` 对未知类型返回 `BlockEntityType::Unknown`，需要处理这种情况。

        ## #7. LockableBlockEntity 命名空间

`LockableBlockEntity`、`LootableContainerBlockEntity` 位于 `mc::blockentity` 命名空间，而非 `mc` 直接命名空间。

        ## #8. 熔炼进度回退（AbstractFurnaceEntity）

        不燃烧时进度应该回退 2，而非清零。

        ## #9. 活塞完成后必须调用 updateFromNeighbourShapes

`PistonBlockEntity::clearPistonBlockEntity()` 在放置最终方块状态之前调用 `Block::
            updateFromNeighbourShapes()` 更新被移动方块的形状。这是因为方块（如栅栏、楼梯、红石线等）的形状取决于邻居，活塞移动后方块到达新位置需要重新计算连接状态。如果跳过此步骤，被活塞推动的栅栏不会与邻居栅栏连接，楼梯不会正确调整形状等。

        ## #9. 漏斗传输冷却

        传输冷却必须在成功传输后设置，否则会连续传输。漏斗链优化时目标漏斗的冷却时间减少 1 tick（7 tick 而非 8 tick）。

        ## #10. 战利品表填充时机

`LootableContainerBlockEntity::fillWithLoot()` 已在基类实现，子类无需重写。填充通过 `IWorld::
            lootTableManager()` 获取战利品表管理器，只有 ServerWorld 返回有效指针。

        ## #11. ISidedInventory 槽位访问规则

        熔炉、酿造台、潜影盒实现了 `ISidedInventory`，漏斗传输物品时会根据方向检查可访问槽位。详见各子模块 README.md。

        ## #12. 方块实体通知客户端使用 notifyBlockUpdate

        方块实体内部数据变化后（如营火烹饪物品、箱子开合状态），应调用 `IWorld::notifyBlockUpdate(
            pos)` 通知客户端刷新显示。不要使用 `setBlockState(pos, state, 3)`，因为 `setBlockState` 在 `oldState
    ==
    newState` 时直接返回
    false，不触发 `m_onBlockChanged` 回调，客户端收不到更新。`notifyBlockUpdate` 即使方块状态未改变也会触发客户端同步，对应
    MC Java 的 `Level.sendBlockUpdated(pos, state, state, 3)`。

## #13. 方块事件系统（blockEvent / triggerEvent）

方块实体通过 `BlockEntity::triggerEvent(int id, int type)` 接收服务端广播的方块事件，用于客户端视觉效果同步（箱子开合动画、陶罐摇晃、末地折跃门冷却等）。

**服务端流程：**
1. 方块实体调用 `IWorld::blockEvent(pos, block, paramA, paramB)` 将事件入队
2. `ServerWorld::tick()` 中处理队列，验证方块仍匹配后调用 `Block::triggerEvent()`
3. `Block::triggerEvent()` 默认委托给 `BlockEntity::triggerEvent()`
4. 事件执行成功后通过 `BlockEventPacket` 广播给附近客户端

**客户端流程：**
1. 收到 `BlockEventPacket` 后查找 `BlockState` 和 `Block`
2. 调用 `Block::triggerEvent()` 触发客户端视觉效果

**已实现 triggerEvent 的方块实体：**
- `ChestEntity` — 箱子开合动画（id=1, type=打开人数）
- `EnderChestEntity` — 末影箱开合动画（id=1, type=打开人数）
- `ShulkerBoxEntity` — 潜影盒开合动画（id=1, type=打开人数）
- `DecoratedPotBlockEntity` — 陶罐摇晃动画（id=1, type=摇晃样式）
- `EndGatewayEntity` — 末地折跃门冷却（id=1, type=0）
- `MobSpawnerBlockEntity` — 刷怪笼生成事件（id=1, type=0）

**注意：** 方块实体需要在合适的时机主动调用 `world.blockEvent()` 来触发事件广播，例如箱子在 `broadcastChestState()` 中调用。

## #14. 客户端同步数据快照（getUpdateTag）

`BlockEntity::getUpdateTag()` 是用于客户端同步的 NBT 快照生成方法，对应 MC Java 的 `BlockEntity.getUpdateTag(HolderLookup.Provider)`。

**调用链：**
1. 方块实体数据变化后，服务端调用 `ServerWorld::broadcastBlockEntity(pos)`
2. `MinecraftServer::broadcastBlockEntityInRange()` 回调触发：
   - 通过 `ServerWorld::getBlockEntity(pos)` 获取方块实体
   - 调用 `entity->getUpdateTag()` 生成 NBT 复合标签，包装为 `shared_ptr<nbt::CompoundTag>`
   - 构造 `ir::play::BlockEntityData`（blockPosPacked + blockEntityType + CompoundTag，无长度前缀）广播给 64 格范围内玩家
3. 客户端 `ClientPlayVisitor::_handleBlockEntityData` 接收，转发给 `ClientWorld::onBlockEntityData(pos, type, const CompoundTag&)`
4. `ClientWorld` 通过 `BlockEntityRegistry` 查找/创建实例，调用 `loadFromNBT()` 更新状态

**默认实现：**
```cpp
nbt::CompoundTag BlockEntity::getUpdateTag() const
{
    nbt::CompoundTag tag;
    saveToNBT(tag);  // 写入完整状态（含 id/x/y/z 及子类自定义字段）
    return tag;
}
```

**子类重写场景：** 默认实现调用 `saveToNBT()` 写入完整数据，适用于大多数方块实体。子类可重写 `getUpdateTag()` 提供精简的更新数据（例如仅同步告示牌文本、箱子物品等），以减少网络带宽占用。重写时仍需写入 `id`/`x`/`y`/`z` 公共字段以便客户端识别。

**与 `saveToNBT()` 的区别：**
- `saveToNBT()` — 用于存档持久化，写入完整状态
- `getUpdateTag()` — 用于客户端同步，可写精简快照（默认实现复用 `saveToNBT()`）
