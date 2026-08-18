// 下界菌索（crimson_roots/warped_roots）与下界苗（nether_sprouts）集成测试：验证种植支撑存活、
// 支撑失效自毁、支撑替换为不可种植方块自毁行为
// （对齐 wiki 绯红菌索#用途 / 下界苗#用途 种植支撑面 + BushBlock 通用支撑失效自毁机制）。
//
// wiki world_绯红菌索.txt#用途（:42）："绯红菌索可被种植在[[菌岩]]、[[灵魂土]]、[[草方块]]、[[菌丝体]]、
//   [[灰化土]]、[[泥土]]、[[缠根泥土]]、[[砂土]]、[[耕地]]、[[泥巴]]、[[沾泥的红树根]]、[[苔藓块]]和
//   [[苍白苔藓块]]上。"
// wiki world_下界苗.txt#用途（:38）：同样的支撑面清单（菌岩、灵魂土、草方块...苍白苔藓块）。
//   ——支撑条件：nylium（菌岩）/soul_soil（灵魂土）+ DIRT 标签面（草方块/泥土/耕地等）+ 苔藓块。
//   支撑失效（下方变 air 或非支撑面）时植物自毁（BushBlock 通用机制，wiki 隐含：植物无支撑则破坏）。
//
// ============================ Cubium 实现链路 ============================
// NetherRootsBlock（nether/NetherRootsBlock.cpp）继承 BushBlock，不重写 updatePostPlacement → 继承
//   BushBlock::updatePostPlacement（agricultural/BushBlock.cpp:67-93）：facing==Down 时重检下方 canSustain，
//   失败则返回 airState（**同步返 air**，引擎同 tick 替换方块为 air，非 scheduleBlockTick 延迟）。
// NetherRootsBlock::canSustain（:49-67）：crimson_nylium/warped_nylium/soul_soil 显式 true，否则回退
//   BushBlock::canSustain（:121-132 委托下方方块 canSustainPlant，DIRT 标签面 + PlantType 匹配）。
// NetherSproutsBlock（nether/NetherSproutsBlock.cpp）同结构，canSustain 同 nylium/soul_soil + 回退。
//   注：crimson_roots 与 warped_roots 共用 NetherRootsBlock 类，行为相同；本测试测 crimson_roots 代表，
//   另测 nether_sprouts 验证 NetherSproutsBlock 独立类 + soul_soil 支撑分支。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃墙空腔）============================
// glass_pit：y=0 glass/cobblestone 底，y=1..2 玻璃墙围出内部 air 空腔，y=3..4 air+顶部框架
// （helper 相对坐标 x,z∈[0,6], y∈[0,4]）。方块测试在内部 air 层操作。
//
// 布局列 (3,*,1)（同 CactusTests/SmallDripleafTests 坐标范式）：支撑 (3,0,1)、植物 (3,1,1)。
//   支撑放 y=0（覆盖 glass_pit 默认 glass 底），植物放 y=1（内部 air 层第一层）。
//
// 测试1 crimson_roots_survives_on_crimson_nylium（菌岩支撑存活，正向防误判）：
//   crimson_nylium (3,0,1) + crimson_roots (3,1,1)。canSustain(crimson_nylium) 显式 true。
//   不做破坏，等待后断言 crimson_roots 仍存在（防 updatePostPlacement 误触发自毁）。
//
// 测试2 crimson_roots_breaks_when_support_removed（移除支撑自毁，wiki 支撑失效）：
//   crimson_nylium (3,0,1) + crimson_roots (3,1,1)。t=20 移除 crimson_nylium→air。crimson_roots
//   updatePostPlacement(Down) canSustain(air) 失败 → 返 air 自毁。断言 crimson_roots 变 air。
//
// 测试3 crimson_roots_breaks_when_support_replaced_with_stone（支撑换 stone 自毁，验证 canSustain 标签判定）：
//   crimson_nylium (3,0,1) + crimson_roots (3,1,1)。t=20 把 crimson_nylium 换 stone。crimson_roots
//   updatePostPlacement(Down) canSustain(stone)：stone 非 nylium/soul_soil，回退 BushBlock::canSustain
//   委托 stone.canSustainPlant——stone 非 DIRT 标签，不支撑 PlantType::Nether → false → 返 air 自毁。
//   验证 canSustain 标签判定（stone 不在 wiki 支撑面清单，自毁）。
//
// 测试4 nether_sprouts_breaks_when_soul_soil_removed（soul_soil 支撑下界苗移除自毁，验证 NetherSproutsBlock + soul_soil 分支）：
//   soul_soil (3,0,1) + nether_sprouts (3,1,1)。t=20 移除 soul_soil→air。nether_sprouts
//   updatePostPlacement(Down) canSustain(air) 失败 → 返 air 自毁。断言 nether_sprouts 变 air。
//   验证 NetherSproutsBlock 独立类继承 BushBlock 自毁链路 + soul_soil 显式支撑分支。
//
// ============================ 排除项（不写测试）============================
// - warped_roots：与 crimson_roots 共用 NetherRootsBlock 类，canSustain 同链路（crimson_nylium/warped_nylium
//   都显式 true），测 crimson_roots 即代表，不重复测 warped_roots 避免冗余。
// - 花盆种植（wiki :44 绯红菌索可种花盆）：花盆是 FlowerPotBlock 含植物状态，依赖花盆物品放置链路，跳过。
// - 堆肥（wiki 绯红菌索65%/下界苗50%）：依赖堆肥桶 useItem + 随机，跳过。
// - 剪子掉落（wiki :40）：依赖物品工具判定 + 掉落物实体，跳过。
// - DIRT 标签面种植（草方块/泥土等）：canSustain 回退 BushBlock::canSustain 委托 canSustainPlant，
//   与 nylium 显式分支同类（都验证 canSustain），本测试用 nylium/soul_soil 显式分支覆盖核心支撑，
//   DIRT 标签面分支留 TODO（需确认 canSustainPlant 对 grass_block/dirt 的 PlantType::Nether 注册）。
//
// ============================ 跨服务端对比 ============================
// - crimson_roots/warped_roots/nether_sprouts typeId 两端一致（1.16 加入，1.21.11 已含）。
// - 种植支撑面（:42/:38 菌岩/灵魂土/草方块...苍白苔藓块）两端 wiki 明文一致。
// - 支撑失效自毁为 BushBlock 通用机制（Java BushBlock.updateShape + canSurvive），两端一致。
// - 测试用 setBlockType 放 nylium/soul_soil/stone/crimson_roots/nether_sprouts/air，均为两端通用 API，
//   非_one-sided。同 tick 同步自毁（updatePostPlacement 返 air），pollUntilSucceed 兼容同步与可能延迟。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_绯红菌索.txt#用途（:42 菌岩/灵魂土/草方块...苍白苔藓块支撑面）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界苗.txt#用途（:38 同支撑面清单）
// Ref: BushBlock.cpp:67-93（updatePostPlacement facing==Down && !canSustain → 返 air 同步自毁）
// Ref: BushBlock.cpp:121-132（canSustain 委托下方方块 canSustainPlant，DIRT 标签面 + PlantType）
// Ref: NetherRootsBlock.cpp:49-67（canSustain crimson_nylium/warped_nylium/soul_soil 显式 + 回退 BushBlock）
// Ref: CactusTests.ts（glass_pit (3,0,1)支撑+(3,1,1)植物 + setBlockType 触发自毁 + succeedWhenBlockPresent 范式）
// Ref: SmallDripleafTests.ts（pollUntilSucceed 轮询自毁断言范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 内部 air 层坐标。支撑 (3,0,1)（覆盖 glass 底），植物 (3,1,1)。
const SUPPORT = { x: 3, y: 0, z: 1 }; // 下方支撑（nylium/soul_soil/stone）
const PLANT = { x: 3, y: 1, z: 1 }; // 植物位置（crimson_roots/nether_sprouts）

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 菌岩支撑存活（正向防误判，验证 canSustain(crimson_nylium) 显式 true 时不触发自毁）。
// wiki :42 绯红菌索可种菌岩。crimson_nylium (3,0,1) + crimson_roots (3,1,1)，不做破坏，等待后断言存活。
function crimsonRootsSurvivesOnCrimsonNylium(test: Test): void {
    test.setBlockType("minecraft:crimson_nylium", SUPPORT);
    test.setBlockType("minecraft:crimson_roots", PLANT);

    // 等待足够 tick，断言 crimson_roots 仍存在（canSustain 满足，不自毁）。
    pollUntilSucceed(
        test,
        () => getTypeId(test, PLANT) === "minecraft:crimson_roots",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `crimson_roots survive: expected crimson_roots to remain at ${JSON.stringify(PLANT)} on nylium, ` +
                        `got ${getTypeId(test, PLANT)} ` +
                        `(support=${getTypeId(test, SUPPORT)}; ` +
                        `if air, updatePostPlacement may over-trigger self-destruct or canSustain(nylium) falsely fails)`,
                );
            },
        },
    );
}

// 移除菌岩支撑 → 自毁（wiki 支撑失效，BushBlock 通用机制）。
// crimson_nylium (3,0,1) + crimson_roots (3,1,1)。t=20 移除 nylium→air。crimson_roots updatePostPlacement(Down)
// canSustain(air) 失败 → 返 air 自毁。
function crimsonRootsBreaksWhenSupportRemoved(test: Test): void {
    test.setBlockType("minecraft:crimson_nylium", SUPPORT);
    test.setBlockType("minecraft:crimson_roots", PLANT);

    test.runAtTickTime(20, () => {
        if (getTypeId(test, PLANT) === "minecraft:crimson_roots") {
            test.setBlockType("minecraft:air", SUPPORT); // 移除 nylium，派发 Up 更新触发 crimson_roots updatePostPlacement(Down)
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, PLANT) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `crimson_roots break on support removed: expected air at ${JSON.stringify(PLANT)} after removing nylium, ` +
                        `got ${getTypeId(test, PLANT)} ` +
                        `(support=${getTypeId(test, SUPPORT)} should be air; ` +
                        `if still crimson_roots, BushBlock updatePostPlacement Down-fail->air may be missing)`,
                );
            },
        },
    );
}

// 支撑换 stone → 自毁（验证 canSustain 标签判定，stone 非 wiki 支撑面）。
// crimson_nylium (3,0,1) + crimson_roots (3,1,1)。t=20 把 nylium 换 stone。crimson_roots updatePostPlacement(Down)
// canSustain(stone)：stone 非 nylium/soul_soil，回退 BushBlock::canSustain 委托 stone.canSustainPlant——stone 非
// DIRT 标签 → false → 返 air 自毁。
function crimsonRootsBreaksWhenSupportReplacedWithStone(test: Test): void {
    test.setBlockType("minecraft:crimson_nylium", SUPPORT);
    test.setBlockType("minecraft:crimson_roots", PLANT);

    test.runAtTickTime(20, () => {
        if (getTypeId(test, PLANT) === "minecraft:crimson_roots") {
            // 把 nylium 换成 stone（真实变化，派发 Up 更新）。stone 非 wiki 支撑面，canSustain 失败。
            test.setBlockType("minecraft:stone", SUPPORT);
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, PLANT) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `crimson_roots break on stone support: expected air at ${JSON.stringify(PLANT)} after replacing nylium with stone, ` +
                        `got ${getTypeId(test, PLANT)} ` +
                        `(support=${getTypeId(test, SUPPORT)} should be stone; ` +
                        `if still crimson_roots, canSustain(stone) may falsely return true (stone should not sustain Nether plant))`,
                );
            },
        },
    );
}

// 灵魂土支撑下界苗移除 → 自毁（验证 NetherSproutsBlock 独立类 + soul_soil 显式支撑分支）。
// soul_soil (3,0,1) + nether_sprouts (3,1,1)。t=20 移除 soul_soil→air。nether_sprouts updatePostPlacement(Down)
// canSustain(air) 失败 → 返 air 自毁。
function netherSproutsBreaksWhenSoulSoilRemoved(test: Test): void {
    test.setBlockType("minecraft:soul_soil", SUPPORT);
    test.setBlockType("minecraft:nether_sprouts", PLANT);

    test.runAtTickTime(20, () => {
        if (getTypeId(test, PLANT) === "minecraft:nether_sprouts") {
            test.setBlockType("minecraft:air", SUPPORT); // 移除 soul_soil，派发 Up 更新触发 nether_sprouts updatePostPlacement(Down)
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, PLANT) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `nether_sprouts break on soul_soil removed: expected air at ${JSON.stringify(PLANT)} after removing soul_soil, ` +
                        `got ${getTypeId(test, PLANT)} ` +
                        `(support=${getTypeId(test, SUPPORT)} should be air; ` +
                        `if still nether_sprouts, NetherSproutsBlock updatePostPlacement Down-fail->air may be missing)`,
                );
            },
        },
    );
}

export function registerNetherRootsTests(): void {
    GameTest.register("BlockBehaviorTests", "crimson_roots_survives_on_crimson_nylium", crimsonRootsSurvivesOnCrimsonNylium)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "crimson_roots_breaks_when_support_removed", crimsonRootsBreaksWhenSupportRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "crimson_roots_breaks_when_support_replaced_with_stone", crimsonRootsBreaksWhenSupportReplacedWithStone)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "nether_sprouts_breaks_when_soul_soil_removed", netherSproutsBreaksWhenSoulSoilRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
}
