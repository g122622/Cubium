// 野花簇（wildflowers）集成测试：验证种植支撑存活、骨粉催熟增量/满簇弹物品、堆叠放置保持朝向、
// 支撑失效自毁行为
// （对齐 wiki 野花簇#获取 :35 / #用途 :48 :50 :52 + BushBlock 通用支撑失效自毁机制）。
//
// wiki block_野花簇.txt：
//   #获取（:35）："对着不满4簇花的野花簇{{ctrl|使用}}骨粉会增加1簇花；对着含有4簇花的野花簇{{ctrl|使用}}
//     骨粉会掉落1个新的野花簇。"——骨粉 grow 独有：AMOUNT<4 增1；AMOUNT==4 弹出一个自身物品。
//   #用途（:48）："野花簇可以种植在[[草方块]]、[[菌丝体]]、[[灰化土]]、[[泥土]]、[[缠根泥土]]、[[砂土]]、
//     [[耕地]]、[[泥巴]]、[[沾泥的红树根]]、[[苔藓块]]和[[苍白苔藓块]]上。"——支撑面属 DIRT 标签面
//     （草方块/泥土/耕地等），canSustain 委托 canSustainPlant 匹配 DIRT 标签。
//   #用途（:50）："多个野花簇可以放置在同一格内，这与[[海泡菜]]和[[蜡烛]]类似。最多可以在一个方块空间内
//     放置4簇。"——堆叠放置 AMOUNT 1→4（同 SeaPickle/Candle 范式）。
//   #用途（:52）："野花簇有4个朝向，其取决于玩家放置时的方向。若一个位置上已有野花簇后，改变方向放置
//     更多野花簇并不会改变其朝向。"——堆叠保持原 facing，不重置。
//   #破坏（:45）："野花簇被破坏后会掉落自身，掉落个数等于方块内野花簇数。"——掉落数=AMOUNT，依赖
//     掉落物实体/战利品表，跳过。
//
// ============================ Cubium 实现链路 ============================
// FlowerBedBlock（garden/FlowerBedBlock.cpp）继承 BushBlock + IGrowable：
//   - 状态属性：FACING（HORIZONTAL_FACING，north/east/south/west）+ AMOUNT（FLOWER_AMOUNT，flower_amount
//     1-4，默认1）。state 字符串如 "facing=north,flower_amount=1"。
//   - getStateForPlacement（:73-94）：目标位置已有同类型花瓣床 → AMOUNT+1（保持原 facing，:84 with AMOUNT
//     不动 FACING）；否则新放 facing=玩家朝向反方向 + AMOUNT=1。
//   - isReplaceable（:96-122）：玩家未潜行 + 手持同类型物品 + AMOUNT<4 → true（同格替换堆叠）。
//   - grow（:197-222，骨粉）：AMOUNT<4 → setBlockState AMOUNT+1（同步）；AMOUNT==4 → spawnItemEntity
//     弹出一个自身物品（ItemDropHelper）。
//   - canGrow/canUseBonemeal 恒 true（100% 催熟，:179-195）。
//   - 不重写 updatePostPlacement → 继承 BushBlock::updatePostPlacement（agricultural/BushBlock.cpp:67-93）：
//     facing==Down 时重检下方 canSustain，失败则返回 airState（同步返 air 自毁）。
//   - canSustain 走 BushBlock 基类（:121-132 委托下方方块 canSustainPlant，DIRT 标签面 + PlantType 匹配）。
//
// 骨粉派发链路（BoneMealItem.cpp:67-97）：useItemOnBlock → BoneMealItem::onItemUse →
//   dynamic_cast<IGrowable*> → canGrow(true) → canUseBonemeal(true) → grow 同步 setBlockState / spawnItemEntity
//   → shrink(1) 消耗骨粉。grow 同步，useItemOnBlock 返回后即可读 state。
// 堆叠派发链路（同 SeaPickleTests）：useItemOnBlock 手持 wildflowers 物品点击已有花瓣床 →
//   onBlockActivated 基类 Pass → fallback Item.useOn → BlockItem::tryPlace → _canReplace（replaceable=true）
//   → placementPos=原花瓣床格（同格替换）→ getStateForPlacement 堆叠分支 AMOUNT+1 → setBlockState 同步。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃墙空腔）============================
// glass_pit：y=0 glass/cobblestone 底，y=1..2 玻璃墙围出内部 air 空腔，y=3..4 air+顶部框架
// （helper 相对坐标 x,z∈[0,6], y∈[0,4]）。方块测试在内部 air 层操作。
//
// 布局列 (3,*,1)（同 CactusFlowerTests/NetherRootsTests 坐标范式）：支撑 (3,0,1)、野花簇 (3,1,1)。
//   支撑放 y=0（覆盖 glass_pit 默认 glass 底），野花簇放 y=1（内部 air 层第一层）。
//
// 测试1 wildflowers_survives_on_grass_block（草方块支撑存活，正向防误判）：
//   grass_block (3,0,1) + wildflowers (3,1,1)。canSustain(grass_block) 走 BushBlock 委托 grass_block
//   .canSustainPlant——grass_block 属 DIRT 标签 → true。等待后断言 wildflowers 仍存在（防自毁误触发）。
//
// 测试2 wildflowers_bonemeal_increases_flower_amount（骨粉增簇，wiki :35 不满4簇增1）：
//   grass_block (3,0,1) + wildflowers (3,1,1) flower_amount=1。SimulatedPlayer 持骨粉对 (3,1,1) useItemOnBlock。
//   grow AMOUNT=1<4 → setBlockState flower_amount=2。断言 flower_amount===2（同步可读）。
//
// 测试3 wildflowers_bonemeal_drops_item_when_full（骨粉满簇弹物品，wiki :35 含4簇掉落1个）：
//   grass_block (3,0,1) + wildflowers (3,1,1) 预置 flower_amount=4（setBlockWithStates 直接写满）。
//   SimulatedPlayer 持骨粉对 (3,1,1) useItemOnBlock。grow AMOUNT==4 → spawnItemEntity 弹 wildflowers 物品。
//   断言：flower_amount 仍 4（不变），且 wildflowers 物品实体存在（runAtTickTime 留 tick 让实体注册）。
//
// 测试4 wildflowers_stacks_when_placing_on_existing（堆叠放置 AMOUNT+1，wiki :50 同格堆叠4簇）：
//   grass_block (3,0,1) 支撑。SimulatedPlayer 持 wildflowers 物品点击 (3,0,1) 顶面 Up → 首次放置落 (3,1,1)
//   flower_amount=1。再点击 (3,1,1) 顶面 Up → 堆叠 AMOUNT+1=2，再 +1=3，再 +1=4。每步断言 flower_amount 递增。
//   （同 SeaPickleTests 场景1 范式）
//
// 测试5 wildflowers_keeps_facing_when_stacking（堆叠保持原朝向，wiki :52 改变方向不改变朝向）：
//   grass_block (3,0,1) 支撑。SimulatedPlayer 站位 (5,1,1) 朝西放首簇（facing=玩家朝向反方向）。
//   记录首簇 facing，再堆叠 +1，断言 facing 不变（getStateForPlacement 堆叠分支 with AMOUNT 不动 FACING）。
//   注：SimulatedPlayer 默认面朝 +Z？需实测 facing 值，本测试只断言「堆叠前后 facing 相同」不预设具体值。
//
// 测试6 wildflowers_breaks_when_grass_support_removed（移除草方块支撑自毁，wiki 支撑失效）：
//   grass_block (3,0,1) + wildflowers (3,1,1)。t=20 移除 grass_block→air。wildflowers updatePostPlacement(Down)
//   canSustain(air) 失败 → 返 air 自毁。断言 wildflowers 变 air。
//
// 测试7 wildflowers_breaks_when_support_replaced_with_stone（支撑换 stone 自毁，验证 canSustain DIRT 标签）：
//   grass_block (3,0,1) + wildflowers (3,1,1)。t=20 把 grass_block 换 stone。wildflowers updatePostPlacement(Down)
//   canSustain(stone)：stone 非 DIRT 标签，canSustainPlant(stone) false → 返 air 自毁。
//   验证 canSustain DIRT 标签判定（stone 不在 wiki 支撑面清单 :48，自毁）。
//
// ============================ 排除项（不写测试）============================
// - 破坏掉落数=AMOUNT（wiki :45）：依赖战利品表 flower_amount block_state_property + 掉落物实体，跳过。
// - 堆肥 30%（wiki :60）：依赖堆肥桶 useItem 链路 + 随机，跳过。
// - 蜜蜂吸引/繁殖/采粉（wiki :62-63）：依赖蜜蜂 AI + 实体交互，跳过。
// - 生物群系茎部着色（wiki :65-67）：渲染层颜色，无脚本 API，跳过。
// - 自然生成/草方块骨粉生成（wiki :24 :32）：依赖地物 + 随机 + 生物群系判定，跳过。
// - 满4堆叠不上溢（wiki :50 上限4）：getStateForPlacement count>=4 return *existingState（AMOUNT 保持4），
//   与 SeaPickleTests 场景2 同构，且测试3已覆盖满4状态，此边界不重复测。
//
// ============================ 跨服务端对比 ============================
// - wildflowers typeId 两端一致（JE 1.21.5 25w02a / BE 1.21.70 加入，1.21.11 已含，wiki :376 :382）。
//   注：Cubium 注册名 minecraft:wildflowers（GardenBlocks.cpp:71）；JE 网络层 state table 用
//   minecraft:pink_petals（java_block_state_table.gen.cpp，wildflowers 与 pink_petals 共享 FlowerBedBlock），
//   脚本侧 typeId 用注册名 wildflowers，不影响测试。
// - flower_amount state 名两端一致（JE 1.21.4 春意盎然引入 flower_amount 1-4，wiki 方块状态表 :369-371）。
// - 骨粉增簇/满簇弹物品（:35）、堆叠4簇（:50）、朝向保持（:52）、支撑面（:48）均为 wiki 明文记录的
//   1.21.11 一致行为。
// - 测试用 setBlockType/setBlockWithStates 放 grass_block/wildflowers/stone/air + useItemOnBlock 骨粉/野花簇
//   物品，均为两端通用 API，非 one-sided。同 tick 同步自毁（updatePostPlacement 返 air）+ grow 同步
//   setBlockState，pollUntilSucceed 兼容同步与可能延迟。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_野花簇.txt#获取（:35 骨粉不满4簇增1 / 含4簇掉落1个）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_野花簇.txt#用途（:48 支撑面 / :50 堆叠4簇 / :52 朝向保持）
// Ref: FlowerBedBlock.cpp:197-222（grow AMOUNT<4 增1 / AMOUNT==4 spawnItemEntity 弹物品）
// Ref: FlowerBedBlock.cpp:73-94（getStateForPlacement 堆叠分支 AMOUNT+1 保持 facing）
// Ref: FlowerBedBlock.cpp:96-122（isReplaceable 同格替换堆叠，玩家未潜行+同类型物品+AMOUNT<4）
// Ref: BushBlock.cpp:67-93（updatePostPlacement facing==Down && !canSustain → 返 air 同步自毁）
// Ref: BushBlock.cpp:121-132（canSustain 委托下方方块 canSustainPlant，DIRT 标签面 + PlantType）
// Ref: BoneMealItem.cpp:67-97（dynamic_cast<IGrowable> → canGrow → canUseBonemeal → grow 同步）
// Ref: SeaPickleTests.ts（堆叠放置 useItemOnBlock +1 范式 + getState 读数量 + 满4处理）
// Ref: BoneMealTests.ts（SimulatedPlayer useItemOnBlock 骨粉范式 + ItemStack bone_meal）
// Ref: ComposterTests.ts（spawnItemEntity 弹物品 + runAtTickTime + assertItemEntityPresent 时序范式）
// Ref: CactusFlowerTests.ts / NetherRootsTests.ts（glass_pit (3,0,1)支撑+(3,1,1)植物 + 支撑自毁范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 内部 air 层坐标。支撑 (3,1,1)（覆盖 glass 墙），野花簇 (3,2,1)（air，useItemOnBlock 可放置）。
// glass_pit 列 (3,*,1)：y=0 glass 底，y=1 glass 墙，y=2..4 air（内部空腔）。setBlockType 直写覆盖 glass，
// useItemOnBlock 真实放置须命中 air（canPlace 检查目标非 glass 不可替换），故野花簇放 y=2 air 层
// （同 SeaPickleTests stone(3,1,1)+sea_pickle(3,2,1) 布局）。
const SUPPORT = { x: 3, y: 1, z: 1 }; // 下方支撑（grass_block/stone，setBlockType 覆盖 glass 墙）
const FLOWER = { x: 3, y: 2, z: 1 }; // 野花簇位置（air 层，useItemOnBlock 可放置）

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 SeaPickleTests/
// SmallDripleafTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 读取野花簇 flower_amount state（number：1-4）。返回 null 表示读取失败或非野花簇。
// Cubium FLOWER_AMOUNT state C++ 属性名为 "flower_amount"（IntegerProperty::create("flower_amount", 1, 4)），
// getState 对 IntegerProperty 返 number（i32）。用 as any 绕过 BlockStateSuperset 白名单（同 SeaPickleTests）。
function getFlowerAmount(test: Test, pos: { x: number; y: number; z: number }): number | null {
    const block = test.getBlock(pos);
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("flower_amount" as any);
    return typeof value === "number" ? value : null;
}

// 读取野花簇 facing state（string：north/east/south/west）。返回 null 表示读取失败或非野花簇。
// Cubium FACING state（HORIZONTAL_FACING）C++ 属性名为 "facing"，getState 对 DirectionProperty 返 string。
function getFacing(test: Test, pos: { x: number; y: number; z: number }): string | null {
    const block = test.getBlock(pos);
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 放置草方块支撑 + 野花簇（默认 flower_amount=1）。堆叠/骨粉测试共用前置。
function placeGrassAndWildflowers(test: Test): void {
    test.setBlockType("minecraft:grass_block", SUPPORT);
    test.setBlockType("minecraft:wildflowers", FLOWER);
}

// 草方块支撑存活（正向防误判，验证 canSustain(grass_block) DIRT 标签通过时不触发自毁）。
// wiki :48 野花簇可种草方块。grass_block (3,0,1) + wildflowers (3,1,1)，不做破坏，等待后断言存活。
function wildflowersSurvivesOnGrassBlock(test: Test): void {
    placeGrassAndWildflowers(test);

    pollUntilSucceed(
        test,
        () => getTypeId(test, FLOWER) === "minecraft:wildflowers",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `wildflowers survive: expected wildflowers to remain at ${JSON.stringify(FLOWER)} on grass_block, ` +
                        `got ${getTypeId(test, FLOWER)} ` +
                        `(support=${getTypeId(test, SUPPORT)}; ` +
                        `if air, canSustain(grass_block) DIRT-tag may falsely fail or updatePostPlacement over-triggers self-destruct)`,
                );
            },
        },
    );
}

// 骨粉增簇（wiki :35 不满4簇增加1簇）。
// grass_block (3,0,1) + wildflowers (3,1,1) flower_amount=1。SimulatedPlayer 持骨粉对 (3,1,1) useItemOnBlock。
// grow AMOUNT=1<4 → setBlockState flower_amount=2（同步）。断言 flower_amount===2。
function wildflowersBonemealIncreasesFlowerAmount(test: Test): void {
    placeGrassAndWildflowers(test);
    test.assert(
        getFlowerAmount(test, FLOWER) === 1,
        `wildflowers flower_amount should be 1 before bonemeal, got ${getFlowerAmount(test, FLOWER)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对野花簇 (3,1,1) 使用骨粉。direction=Up（从上方使用），faceLocation 默认方块中心。
    // BoneMealItem::onItemUse → dynamic_cast<IGrowable> → canGrow(true) → canUseBonemeal(true) → grow
    // AMOUNT=1<4 → setBlockState flower_amount=2（同步）。useItemOnBlock 返回后即可读。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        FLOWER,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when bone meal increases wildflowers flower_amount");

    // grow 同步 setBlockState，useItemOnBlock 返回后即可读。留 small pollUntilSucceed 兼容可能延迟。
    pollUntilSucceed(
        test,
        () => getFlowerAmount(test, FLOWER) === 2,
        {
            startTick: 2,
            interval: 2,
            maxTick: 20,
            onTimeout: () => {
                test.assert(
                    false,
                    `wildflowers bonemeal increase: expected flower_amount=2 after bone meal, ` +
                        `got ${getFlowerAmount(test, FLOWER)} (typeId=${getTypeId(test, FLOWER)}; ` +
                        `if still 1, FlowerBedBlock::grow AMOUNT<4 increment may be missing)`,
                );
            },
        },
    );
}

// 骨粉满簇弹物品（wiki :35 含有4簇花的野花簇使用骨粉会掉落1个新的野花簇）。
// grass_block (3,0,1) + wildflowers (3,1,1) 预置 flower_amount=4。SimulatedPlayer 持骨粉 useItemOnBlock。
// grow AMOUNT==4 → spawnItemEntity 弹 wildflowers 物品（ItemDropHelper）。断言：flower_amount 仍 4（不变），
// 且 wildflowers 物品实体存在（runAtTickTime 留 tick 让实体注册，同 ComposterTests 范式）。
function wildflowersBonemealDropsItemWhenFull(test: Test): void {
    test.setBlockType("minecraft:grass_block", SUPPORT);
    // 预置 flower_amount=4（直接写满，同 SeaPickleTests 场景2 范式）。
    (test as TestWithStates).setBlockWithStates("minecraft:wildflowers", FLOWER, "facing=north,flower_amount=4");
    test.assert(
        getFlowerAmount(test, FLOWER) === 4,
        `wildflowers flower_amount should be 4 before bonemeal, got ${getFlowerAmount(test, FLOWER)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        FLOWER,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when bone meal drops item from full wildflowers");

    // 判定 1：flower_amount 仍 4（满簇不增，grow AMOUNT==4 分支不 setBlockState 改 AMOUNT）。
    test.assert(
        getFlowerAmount(test, FLOWER) === 4,
        `wildflowers flower_amount should stay 4 after bonemeal on full (grow drops item, no increment), ` +
            `got ${getFlowerAmount(test, FLOWER)}`,
    );

    // 判定 2：wildflowers 物品实体存在（grow spawnItemEntity 弹出自身物品，searchRadius 覆盖野花簇上方区域）。
    // 用 runAtTickTime 留 tick 让物品实体注册（spawnItemEntity 同步，但实体注册可能延 tick，同 ComposterTests）。
    pollUntilSucceed(
        test,
        () => {
            // flower_amount 须仍 4（grow 不应改 AMOUNT），且物品实体出现。
            if (getFlowerAmount(test, FLOWER) !== 4) {
                return false;
            }
            try {
                test.assertItemEntityPresent("minecraft:wildflowers", FLOWER, 1.5, true);
                return true;
            } catch {
                return false;
            }
        },
        {
            startTick: 2,
            interval: 2,
            maxTick: 30,
            onTimeout: () => {
                test.assert(
                    false,
                    `wildflowers bonemeal drop: expected wildflowers item entity near ${JSON.stringify(FLOWER)} ` +
                        `after bone meal on full (flower_amount=${getFlowerAmount(test, FLOWER)}; ` +
                        `if no item, FlowerBedBlock::grow AMOUNT==4 spawnItemEntity may be missing)`,
                );
            },
        },
    );
}

// 堆叠放置 AMOUNT+1（wiki :50 多个野花簇可放同一格，最多4簇；同 SeaPickleTests 场景1 范式）。
// grass_block (3,0,1) 支撑。SimulatedPlayer 持 wildflowers 物品点击 (3,0,1) 顶面 Up → 首次放置落 (3,1,1)
// flower_amount=1。再点击 (3,1,1) 顶面 Up → 堆叠 AMOUNT+1=2，再 +1=3，再 +1=4。每步断言递增。
function wildflowersStacksWhenPlacingOnExisting(test: Test): void {
    test.setBlockType("minecraft:grass_block", SUPPORT);
    test.assert(
        getTypeId(test, SUPPORT) === "minecraft:grass_block",
        `grass_block should be at ${JSON.stringify(SUPPORT)}, got ${getTypeId(test, SUPPORT)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");

    // 放置首簇：点击 (3,1,1) grass_block 顶面 Up → 野花簇落 (3,2,1) flower_amount=1。
    let used = farmer.useItemOnBlock(
        new ItemStack("minecraft:wildflowers", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        SUPPORT,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing first wildflowers");
    test.assert(getTypeId(test, FLOWER) === "minecraft:wildflowers", `wildflowers should be at ${JSON.stringify(FLOWER)}, got ${getTypeId(test, FLOWER)}`);
    test.assert(getFlowerAmount(test, FLOWER) === 1, `first wildflowers flower_amount should be 1, got ${getFlowerAmount(test, FLOWER)}`);

    // 堆叠 +1 到 2：点击 (3,1,1) 顶面 Up → 同格替换堆叠，flower_amount=2。
    used = farmer.useItemOnBlock(
        new ItemStack("minecraft:wildflowers", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        FLOWER,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when stacking wildflowers to 2");
    test.assert(getFlowerAmount(test, FLOWER) === 2, `wildflowers flower_amount should be 2 after stack, got ${getFlowerAmount(test, FLOWER)}`);

    // 堆叠 +1 到 3：点击 (3,1,1) 顶面 Up → flower_amount=3。
    used = farmer.useItemOnBlock(
        new ItemStack("minecraft:wildflowers", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        FLOWER,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when stacking wildflowers to 3");
    test.assert(getFlowerAmount(test, FLOWER) === 3, `wildflowers flower_amount should be 3 after stack, got ${getFlowerAmount(test, FLOWER)}`);

    // 堆叠 +1 到 4：点击 (3,1,1) 顶面 Up → flower_amount=4（达上限）。
    used = farmer.useItemOnBlock(
        new ItemStack("minecraft:wildflowers", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        FLOWER,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when stacking wildflowers to 4");
    test.assert(getFlowerAmount(test, FLOWER) === 4, `wildflowers flower_amount should be 4 after stack, got ${getFlowerAmount(test, FLOWER)}`);

    test.succeed();
}

// 堆叠保持原朝向（wiki :52 已有野花簇后改变方向放置更多野花簇并不会改变其朝向）。
// grass_block (3,0,1) 支撑。SimulatedPlayer 站位 (5,1,1) 放首簇（facing=玩家朝向反方向，记录值）。
// 再堆叠 +1，断言 facing 不变（getStateForPlacement 堆叠分支 with AMOUNT 不动 FACING）。
// 注：不预设 facing 具体值（取决于 SimulatedPlayer 默认朝向），只断言「堆叠前后 facing 相同」。
function wildflowersKeepsFacingWhenStacking(test: Test): void {
    test.setBlockType("minecraft:grass_block", SUPPORT);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");

    // 放置首簇：点击 (3,0,1) 顶面 Up → 野花簇落 (3,1,1)，facing=玩家朝向反方向，flower_amount=1。
    const usedFirst = farmer.useItemOnBlock(
        new ItemStack("minecraft:wildflowers", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        SUPPORT,
        Direction.Up,
    );
    test.assert(usedFirst, "useItemOnBlock should return true when placing first wildflowers for facing test");
    test.assert(getTypeId(test, FLOWER) === "minecraft:wildflowers", `wildflowers should be at ${JSON.stringify(FLOWER)}, got ${getTypeId(test, FLOWER)}`);

    const facingBefore = getFacing(test, FLOWER);
    test.assert(
        facingBefore !== null && ["north", "east", "south", "west"].includes(facingBefore as string),
        `wildflowers facing should be a valid horizontal direction before stack, got ${facingBefore}`,
    );

    // 堆叠 +1 到 2：点击 (3,1,1) 顶面 Up。getStateForPlacement 堆叠分支 with(AMOUNT,2) 不动 FACING。
    const usedStack = farmer.useItemOnBlock(
        new ItemStack("minecraft:wildflowers", 1) as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        FLOWER,
        Direction.Up,
    );
    test.assert(usedStack, "useItemOnBlock should return true when stacking wildflowers for facing test");
    test.assert(getFlowerAmount(test, FLOWER) === 2, `wildflowers flower_amount should be 2 after stack, got ${getFlowerAmount(test, FLOWER)}`);

    // 断言 facing 不变（堆叠保持原朝向，wiki :52）。
    const facingAfter = getFacing(test, FLOWER);
    test.assert(
        facingAfter === facingBefore,
        `wildflowers facing should not change when stacking (wiki :52), before=${facingBefore} after=${facingAfter}`,
    );

    test.succeed();
}

// 移除草方块支撑 → 自毁（wiki 支撑失效，BushBlock 通用机制）。
// grass_block (3,0,1) + wildflowers (3,1,1)。t=20 移除 grass_block→air。wildflowers updatePostPlacement(Down)
// canSustain(air) 失败 → 返 air 自毁。
function wildflowersBreaksWhenGrassSupportRemoved(test: Test): void {
    placeGrassAndWildflowers(test);

    test.runAtTickTime(20, () => {
        if (getTypeId(test, FLOWER) === "minecraft:wildflowers") {
            test.setBlockType("minecraft:air", SUPPORT); // 移除 grass_block，派发 Up 更新触发 wildflowers updatePostPlacement(Down)
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, FLOWER) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `wildflowers break on grass removed: expected air at ${JSON.stringify(FLOWER)} after removing grass_block, ` +
                        `got ${getTypeId(test, FLOWER)} ` +
                        `(support=${getTypeId(test, SUPPORT)} should be air; ` +
                        `if still wildflowers, BushBlock updatePostPlacement Down-fail->air may be missing)`,
                );
            },
        },
    );
}

// 支撑换 stone → 自毁（验证 canSustain DIRT 标签判定，stone 非 wiki 支撑面 :48）。
// grass_block (3,0,1) + wildflowers (3,1,1)。t=20 把 grass_block 换 stone。wildflowers updatePostPlacement(Down)
// canSustain(stone)：stone 非 DIRT 标签，canSustainPlant(stone) false → 返 air 自毁。
function wildflowersBreaksWhenSupportReplacedWithStone(test: Test): void {
    placeGrassAndWildflowers(test);

    test.runAtTickTime(20, () => {
        if (getTypeId(test, FLOWER) === "minecraft:wildflowers") {
            // 把 grass_block 换成 stone（真实变化，派发 Up 更新）。stone 非 wiki 支撑面，canSustain 失败。
            test.setBlockType("minecraft:stone", SUPPORT);
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, FLOWER) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `wildflowers break on stone support: expected air at ${JSON.stringify(FLOWER)} after replacing grass_block with stone, ` +
                        `got ${getTypeId(test, FLOWER)} ` +
                        `(support=${getTypeId(test, SUPPORT)} should be stone; ` +
                        `if still wildflowers, canSustain(stone) DIRT-tag may falsely return true (stone should not sustain wildflowers))`,
                );
            },
        },
    );
}

export function registerWildflowersTests(): void {
    GameTest.register("BlockBehaviorTests", "wildflowers_survives_on_grass_block", wildflowersSurvivesOnGrassBlock)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "wildflowers_bonemeal_increases_flower_amount", wildflowersBonemealIncreasesFlowerAmount)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "wildflowers_bonemeal_drops_item_when_full", wildflowersBonemealDropsItemWhenFull)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "wildflowers_stacks_when_placing_on_existing", wildflowersStacksWhenPlacingOnExisting)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "wildflowers_keeps_facing_when_stacking", wildflowersKeepsFacingWhenStacking)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "wildflowers_breaks_when_grass_support_removed", wildflowersBreaksWhenGrassSupportRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "wildflowers_breaks_when_support_replaced_with_stone", wildflowersBreaksWhenSupportReplacedWithStone)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
}
