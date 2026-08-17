// 酿造台（brewing_stand）三槽位 has_bottle state 读写、物品放置与破坏行为 GameTest。
//
// wiki tech_酿造台.txt#破坏/用途/红石比较器/光源：
//   - 破坏：酿造台被破坏后掉落自身和内容物（容器破坏掉落内容物，wiki :47）。
//   - 用途：酿造台界面三个药水瓶槽位，对酿造台按下使用可打开界面（onBlockActivated openContainer）。
//   - has_bottle state：当药水槽位有玻璃瓶或任意药水时，方块模型对应位置显示红色药水纹理
//     （wiki :73，对应 HAS_BOTTLE_0/1/2 三组独立 bool state，槽位有瓶则该 state=true）。
//   - 红石比较器：能检测酿造台五个槽位存储物品的数量（getComparatorInputOverride，wiki :76）。
//   - 光源：酿造台发出亮度为 1 的光（getLightLevel=1，wiki :79）。
//   - 酿造：烈焰粉作能量，每次酿造 20 秒，涉 randomTick/酿造 tick（非确定，不测）。
//
// C++ 链路：BrewingStandBlock（functional/BrewingStandBlock.cpp）三组 bool state：
//   - HAS_BOTTLE_0/1/2（C++ 属性名 "has_bottle_0"/"has_bottle_1"/"has_bottle_2"，BooleanProperty，
//     Properties.hpp:647-668，默认全 false）。三组独立 bool（2^3=8 态），现有测试未覆盖的 state 类型。
//   - getStateForPlacement（:90-93）：返 defaultState()（三 has_bottle 全 false，无 facing state，
//     放置不依赖朝向）。
//   - 无 isValidPosition override（基类返 true，酿造台无支撑要求，可悬空放置，wiki :55 类似）。
//   - 无 updatePostPlacement override（支撑失效不自毁，走基类返 state）。
//   - hasBlockEntity()=true（:122），createBlockEntity（:121-124）返 BrewingStandEntity。
//     放置酿造台时 setBlockState 触发 BlockEntity 创建（hasBlockEntity 链路）。
//   - onBlockRemoved（:174-195）：遍历 BrewingStandEntity inventory，removeItemNoUpdate 掉落内部物品
//     （容器破坏掉落内容物范式，参照 Lectern 场景 4）。空酿造台 inventory 全空，遍历无掉落。
//   - getLightLevel（:96-103）：恒返 1（酿造台恒发光 1 级）。
//   - getComparatorInputOverride（:107-119）：从 BrewingStandEntity::getComparatorSignal 读比较器信号
//     （空酿造台返 0）。
//   - 物品注册：Items.cpp:3728-3729 registerBlockBackedItem(BREWING_STAND, "brewing_stand")，
//     BlockItemRegistry.cpp:792 registerSimpleBlock(BREWING_STAND, "brewing_stand")。物品已注册，可放置。
//
// 派发链路：SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:265-346）。手持 brewing_stand 物品点击
//   stone 顶面 → onBlockActivated 基类 Pass（酿造台 onBlockActivated 仅对已有酿造台 openContainer，
//   放置时 targetBlock 非 brewing_stand 走基类 Pass）→ fallback Item.useOn → BlockItem::onItemUse →
//   tryPlace → getStateForPlacement defaultState → setBlockState（hasBlockEntity 触发 BlockEntity 创建）。
//   SimulatedPlayer 默认创造模式，brewing_stand 不消耗。
//
// 测试覆盖（3 个场景，覆盖 wiki 三槽位 has_bottle state + 物品放置 + 破坏核心确定行为）：
//   1. 三槽位 has_bottle state 组合读写：setBlockWithStates 预置 has_bottle_0=true,has_bottle_1=false,
//      has_bottle_2=true → getState 验证三 bool 独立可读写（全新三 bool 组合 state 类型）。
//   2. 物品放置酿造台 state 确定：useItemOnBlock brewing_stand 点击 stone 顶面 → 酿造台落上方 →
//      三 has_bottle 全 false（getStateForPlacement defaultState），验证放置链路 + 物品已注册。
//   3. 酿造台破坏不崩溃：放酿造台（BlockEntity 创建）→ setBlockType air 破坏 → onBlockRemoved 遍历
//      空 inventory 无掉落 → 位置变 air（验证容器破坏 onBlockRemoved 链路安全）。
//
// 关键约束：
// 1. 酿造台无 facing state，放置不依赖朝向。场景 1 用 setBlockWithStates 预置 has_bottle 组合
//    （绕过物品放置，直接写 state），getState 读 "has_bottle_0"/"has_bottle_1"/"has_bottle_2"（C++ 内部名）。
// 2. 场景 2 用 useItemOnBlock 放置：手持 brewing_stand 点击 (3,1,1) stone 顶面 Up → placementPos=
//    (3,2,1)（stone 不可替换 → 相邻位置上方 air）→ getStateForPlacement defaultState → setBlockState
//    放酿造台 (3,2,1)。断言 typeId=brewing_stand + 三 has_bottle 全 false。
// 3. 场景 3 放酿造台后 setBlockType air 破坏：onBlockRemoved（:174-195）取 BlockEntity inventory
//    （空酿造台 inventory 全空），遍历 removeItemNoUpdate 无非空 stack → 不掉落 → Block::onBlockRemoved
//    基类。断言位置变 air（破坏成功，链路不崩溃）。空酿造台无内容物可掉，不验物品实体（避免否定断言 flaky）。
// 4. 读 has_bottle 用 getState("has_bottle_0" as any)（BooleanProperty 序列化为 bool）。
// 5. 酿造台 noCollision? 否——酿造台有碰撞箱（m_shape 底座+柱，:82-87），但 isValidPosition 基类 true
//    无支撑要求。stone 支撑仅为贴近真实放置，非 isValidPosition 要求。
// 6. 放置是 useItemOnBlock 同步触发（BlockItem::tryPlace 同步 setBlockState），useItemOnBlock 返回后即可
//    读 state。留 maxTicks 余量防时序。
//
// 不测「酿造」：涉 BrewingStandEntity tick + 烈焰粉燃料 + 酿造配方 + 20 秒计时，非确定且复杂，跳过。
//   TODO: 可补 brewing_stand_brews_potion（需控制燃料/材料/计时，非确定）。
// 不测「比较器信号」：需比较器贴酿造台 + 读输出信号强度，链路复杂且 GameTest 读红石信号 API 受限，跳过。
//   TODO: 待比较器读容器信号测试范式完善后补 brewing_stand_comparator_signal。
// 不测「恒发光 1」：属光照范畴（lighting 包），且光照测试已覆盖发光方块，跳过。
// 不测「内容物破坏掉落」：SimulatedPlayer 无 GUI 放物品 API，无法向酿造台槽位放入物品（onBlockActivated
//   openContainer 仅打开 GUI，SimulatedPlayer 无后续放入操作），无法构造有内容物的酿造台。场景 3 仅测
//   空酿造台破坏不崩溃。TODO: 待脚本侧 BlockEntity 容器操作 API（getComponent("inventory")）补全后
//   补 brewing_stand_drops_contents_when_broken（参照 Lectern 场景 4 范式）。
// 不测「村民认领转职牧师」：涉村民 AI 寻路/认领链路，非确定，跳过。
//
// 跨服务端：brewing_stand 方块名两端一致。has_bottle_0/1/2 state 名两端一致（C++ 内部名）。
//   三槽位 has_bottle state 读写 + 物品放置 state 确定 + 破坏不崩溃行为两端与 vanilla 一致。
//   setBlockWithStates 预置 state 是 Cubium 专有写入（基岩侧用物品放置），但 has_bottle state 行为本身
//   两端可对比，非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_酿造台.txt#破坏（掉落自身和内容物）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_酿造台.txt#用途（三药水瓶槽位，has_bottle 纹理）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_酿造台.txt#红石比较器（检测五槽位物品数量）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_酿造台.txt#光源（亮度 1）
// Ref: BrewingStandBlock.cpp（getStateForPlacement defaultState / onBlockRemoved 掉落 / hasBlockEntity / createBlockEntity）
// Ref: BrewingStandBlock.hpp:96-103（getLightLevel 恒 1）/ :107-119（getComparatorInputOverride）
// Ref: Properties.hpp:647-668（HAS_BOTTLE_0/1/2 "has_bottle_0"/"has_bottle_1"/"has_bottle_2" BooleanProperty）
// Ref: LecternTests.ts（容器破坏掉落范式：setBlockType air 破坏 + runAtTickTime + assertItemEntityPresent）
// Ref: TorchTests.ts（useItemOnBlock 放置范式：点击 stone 顶面 → 方块落上方）
// Ref: BigDripleafTests.ts（setBlockWithStates 预置 state + getState 读 C++ 内部属性名范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/3：酿造台 (3,2,1)，下方 (3,1,1) stone 支撑（贴近真实放置）。
// 场景 2：被点击 stone (3,1,1)，酿造台落 (3,2,1)（上方）。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 BigDripleafTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 酿造台 has_bottle_N state（bool）。返回 null 表示失败或非酿造台。
// 注意：HAS_BOTTLE_0/1/2 的 C++ 属性名为 "has_bottle_0"/"has_bottle_1"/"has_bottle_2"
// （BooleanProperty::create("has_bottle_N")，Properties.hpp:647-668），getState 按内部名匹配。
function getHasBottle(test: Test, x: number, y: number, z: number, slot: 0 | 1 | 2): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState(`has_bottle_${slot}` as any);
    return typeof value === "boolean" ? value : null;
}

// 场景 1：三槽位 has_bottle state 组合读写——预置 has_bottle_0=true,1=false,2=true → getState 可读。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 酿造台（setBlockWithStates 预置 has_bottle_0=true,has_bottle_1=false,
//   has_bottle_2=true，绕过物品放置直接写 state）。
//
// 判定：getState("has_bottle_0")===true 且 has_bottle_1===false 且 has_bottle_2===true（三 bool 独立可读写，
//   验证三槽位 state 组合 2^3 态中一种非默认组合）。
//
// 此场景验证 wiki「三药水瓶槽位独立 has_bottle state」+ 全新三 bool 组合 state 类型：现有测试未覆盖三组
//   独立 bool state 的组合读写（candle 是单 candles int，turtle_egg 是单 eggs int）。预置非默认组合
//   （0=true,1=false,2=true）验证三 bool 互不干扰独立读写。
function brewingStandHasBottleStateReadable(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    // setBlockWithStates 预置 has_bottle_0=true,has_bottle_1=false,has_bottle_2=true（非默认组合）。
    (test as TestWithStates).setBlockWithStates(
        "minecraft:brewing_stand",
        { x: 3, y: 2, z: 1 },
        "has_bottle_0=true,has_bottle_1=false,has_bottle_2=true",
    );
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:brewing_stand", `brewing_stand should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    // 断言三 has_bottle 独立读写：0=true, 1=false, 2=true（预置的非默认组合）。
    test.assert(getHasBottle(test, 3, 2, 1, 0) === true, `has_bottle_0 should be true, got ${getHasBottle(test, 3, 2, 1, 0)}`);
    test.assert(getHasBottle(test, 3, 2, 1, 1) === false, `has_bottle_1 should be false, got ${getHasBottle(test, 3, 2, 1, 1)}`);
    test.assert(getHasBottle(test, 3, 2, 1, 2) === true, `has_bottle_2 should be true, got ${getHasBottle(test, 3, 2, 1, 2)}`);

    test.succeed();
}

// 场景 2：物品放置酿造台 state 确定——useItemOnBlock brewing_stand 点击 stone 顶面 → 酿造台落上方，
//   三 has_bottle 全 false（getStateForPlacement defaultState）。
//
// 布局：(3,1,1) stone（被点击方块）。手持 brewing_stand useItemOnBlock 点击 (3,1,1) 顶面 Up →
//   placementPos=(3,2,1)（stone 不可替换 → 相邻位置上方 air）→ getStateForPlacement defaultState
//   （三 has_bottle 全 false）→ setBlockState 放酿造台 (3,2,1)（hasBlockEntity 触发 BlockEntity 创建）。
//
// 判定：(3,2,1) typeId === "minecraft:brewing_stand" 且三 has_bottle 全 false（放置 state 确定）。
//
// 此场景验证酿造台物品放置链路：物品已注册（useItemOnBlock 成功放置）+ getStateForPlacement 返
//   defaultState（三 has_bottle 全 false，放置时无瓶）+ hasBlockEntity 触发 BlockEntity 创建不崩溃。
function brewingStandPlacedWithDefaultState(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const brewingStand = new ItemStack("minecraft:brewing_stand", 1);

    // 手持 brewing_stand 点击 (3,1,1) 顶面 Up → 酿造台落 (3,2,1)。stone onBlockActivated Pass → fallback
    // 放置。getStateForPlacement defaultState（三 has_bottle 全 false）→ setBlockState。
    const used = farmer.useItemOnBlock(
        brewingStand as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 1, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing brewing stand");

    // 判定：酿造台 (3,2,1) 已放置，三 has_bottle 全 false（defaultState，放置时无瓶）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:brewing_stand", `brewing_stand should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getHasBottle(test, 3, 2, 1, 0) === false, `has_bottle_0 should be false after placement, got ${getHasBottle(test, 3, 2, 1, 0)}`);
    test.assert(getHasBottle(test, 3, 2, 1, 1) === false, `has_bottle_1 should be false after placement, got ${getHasBottle(test, 3, 2, 1, 1)}`);
    test.assert(getHasBottle(test, 3, 2, 1, 2) === false, `has_bottle_2 should be false after placement, got ${getHasBottle(test, 3, 2, 1, 2)}`);

    test.succeed();
}

// 场景 3：酿造台破坏不崩溃——放酿造台（BlockEntity 创建）→ setBlockType air 破坏 → onBlockRemoved
//   遍历空 inventory 无掉落 → 位置变 air（验证容器破坏 onBlockRemoved 链路安全）。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 酿造台（setBlockType 放置，BlockEntity 创建）。
// setBlockType("minecraft:air", (3,2,1)) 破坏酿造台 → BrewingStandBlock::onBlockRemoved（:174-195）：
//   取 BlockEntity inventory（空酿造台全空），遍历 removeItemNoUpdate 无非空 stack → 不掉落 →
//   Block::onBlockRemoved 基类。位置变 air。
//
// 判定：破坏后 (3,2,1) typeId === "minecraft:air"（酿造台已破坏，onBlockRemoved 容器链路不崩溃）。
//
// 此场景验证酿造台容器破坏 onBlockRemoved 链路安全性：放酿造台（BlockEntity 创建）后破坏，onBlockRemoved
//   遍历空 inventory 不崩溃，位置正确变 air。空酿造台无内容物可掉落（SimulatedPlayer 无 GUI 放物品 API，
//   无法构造有内容物的酿造台，故仅测空破坏不崩溃，见文件头 TODO）。留 2 tick 让 onBlockRemoved 同步执行。
function brewingStandBreaksWhenRemoved(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:brewing_stand", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:brewing_stand", `brewing_stand should be at (3,2,1) before break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    // setBlockType air 破坏酿造台 → onBlockRemoved 遍历空 inventory 无掉落 → 位置变 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言酿造台 (3,2,1) 已破坏变 air（onBlockRemoved 容器链路不崩溃）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `brewing_stand pos should be air after break, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerBrewingStandTests(): void {
    GameTest.register("BlockBehaviorTests", "brewing_stand_has_bottle_state_readable", brewingStandHasBottleStateReadable)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "brewing_stand_placed_with_default_state", brewingStandPlacedWithDefaultState)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "brewing_stand_breaks_when_removed", brewingStandBreaksWhenRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
