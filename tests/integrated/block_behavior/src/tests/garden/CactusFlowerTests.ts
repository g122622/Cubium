// 仙人掌花（cactus_flower）集成测试：验证种植支撑存活（cactus/farmland/顶面中心完整方块）与支撑失效自毁
// （对齐 wiki 仙人掌花#用途 :40 放置在顶面中心完整的方块如仙人掌顶部 + BushBlock 通用支撑失效自毁）。
//
// wiki block_仙人掌花.txt#用途（:40）："仙人掌花能够被放置在任何顶面[[判定箱|中心完整]]的方块（例如
//   仙人掌）顶部。"——canSustain：cactus（显式）/ farmland（显式）/ isFaceSturdy(Up, Center)（兜底，
//   顶面中心完整的实心方块如 stone）。
//   #用途（:42）："不同于其他一格高的花，仙人掌花不能放进花盆。"——花盆不测（依赖花盆物品放置链路）。
//
// ============================ Cubium 实现链路 ============================
// CactusFlowerBlock（garden/CactusFlowerBlock.cpp）继承 FlowerBlock（vegetation/FlowerBlock.cpp :40
//   FlowerBlock : BushBlock），不重写 updatePostPlacement → 继承 BushBlock::updatePostPlacement
//   （agricultural/BushBlock.cpp:67-93）：facing==Down 时重检下方 canSustain，失败则返回 airState
//   （**同步返 air**，引擎同 tick 替换，非 scheduleBlockTick 延迟）。
// CactusFlowerBlock::canSustain（:42-54）：cactus 显式 true / farmland 显式 true /
//   groundState.isFaceSturdy(world, groundPos, Up, SupportType::Center)（兜底，顶面中心完整）。
//   注：FlowerBlock::canSustain 基类（:55-64 用 material.isSolid()）被 CactusFlowerBlock 重写覆盖，
//   仙人掌花走 cactus/farmland/Center 判定，不走 material.isSolid()。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃墙空腔）============================
// glass_pit：y=0 glass/cobblestone 底，y=1..2 玻璃墙围出内部 air 空腔，y=3..4 air+顶部框架
// （helper 相对坐标 x,z∈[0,6], y∈[0,4]）。方块测试在内部 air 层操作。
//
// 布局列 (3,*,1)（同 CactusTests/NetherRootsTests 坐标范式）：支撑 (3,0,1)、花 (3,1,1)。
//
// 测试1 cactus_flower_survives_on_cactus（仙人掌支撑存活，canSustain cactus 显式分支正向）：
//   cactus (3,0,1) + cactus_flower (3,1,1)。canSustain(cactus) 显式 true。等待后断言存活。
//
// 测试2 cactus_flower_survives_on_farmland（耕地支撑存活，canSustain farmland 显式分支正向）：
//   farmland (3,0,1) + cactus_flower (3,1,1)。canSustain(farmland) 显式 true。等待后断言存活。
//
// 测试3 cactus_flower_survives_on_stone（石头支撑存活，canSustain isFaceSturdy(Up,Center) 兜底正向）：
//   stone (3,0,1) + cactus_flower (3,1,1)。canSustain(stone)：stone 非 cactus/farmland，走兜底
//   isFaceSturdy(Up, Center)——stone 是完整实心方块，顶面中心完整 → true。等待后断言存活。
//   验证 Center 面兜底分支对实心方块通过（wiki「顶面中心完整的方块」）。
//
// 测试4 cactus_flower_breaks_when_cactus_support_removed（移除仙人掌支撑自毁，wiki 支撑失效）：
//   cactus (3,0,1) + cactus_flower (3,1,1)。t=20 移除 cactus→air。cactus_flower updatePostPlacement(Down)
//   canSustain(air) 失败（air 非 cactus/farmland，isFaceSturdy(air) false）→ 返 air 自毁。断言变 air。
//
// ============================ 排除项（不写测试）============================
// - 花盆不可放置（wiki :42）：依赖花盆物品放置链路 + 花盆含植物状态，跳过。
// - 蜜蜂吸引/繁殖/采粉（wiki :44-46）：依赖蜜蜂 AI + 实体交互，跳过。
// - 堆肥（wiki :48）：依赖堆肥桶 useItem + 随机，跳过。
// - 合成材料（wiki :43）：合成台链路，跳过。
// - Center 面失败自毁（顶面非中心完整方块如 glass_pane/slab）：isFaceSturdy(Up,Center) 对薄片/半砖
//   的判定需逐方块确认，边界复杂，本测试用 stone 正向验证 Center 分支通过，失败分支留 TODO。TODO: 待
//   isFaceSturdy(Up,Center) 对 glass_pane/slab 判定确认后补 cactus_flower_breaks_on_non_center_face。
// - 破坏掉落自身（wiki :38）：依赖掉落物实体，跳过。
//
// ============================ 跨服务端对比 ============================
// - cactus_flower typeId 两端一致（JE 1.21 Tricky Trials / BE 1.21 加入，1.21.11 已含）。
// - 放置在顶面中心完整方块（:40）两端 wiki 明文一致。支撑失效自毁为 BushBlock 通用机制两端一致。
// - 测试用 setBlockType 放 cactus/farmland/stone/cactus_flower/air，均为两端通用 API，非 one-sided。
//   同 tick 同步自毁（updatePostPlacement 返 air），pollUntilSucceed 兼容同步与可能延迟。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_仙人掌花.txt#用途（:40 顶面中心完整方块如仙人掌顶部）
// Ref: CactusFlowerBlock.cpp:42-54（canSustain cactus/farmland 显式 + isFaceSturdy(Up,Center) 兜底）
// Ref: BushBlock.cpp:67-93（updatePostPlacement facing==Down && !canSustain → 返 air 同步自毁）
// Ref: FlowerBlock.cpp:40（FlowerBlock : BushBlock，CactusFlowerBlock 继承自毁链路）
// Ref: CactusTests.ts / NetherRootsTests.ts（glass_pit (3,0,1)支撑+(3,1,1)植物 + 支撑自毁范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 内部 air 层坐标。支撑 (3,0,1)（覆盖 glass 底），花 (3,1,1)。
const SUPPORT = { x: 3, y: 0, z: 1 }; // 下方支撑（cactus/farmland/stone）
const FLOWER = { x: 3, y: 1, z: 1 }; // 仙人掌花位置

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 仙人掌支撑存活（canSustain cactus 显式分支正向防误判）。
// wiki :40 仙人掌花可放仙人掌顶部。cactus (3,0,1) + cactus_flower (3,1,1)，不做破坏，等待后断言存活。
function cactusFlowerSurvivesOnCactus(test: Test): void {
    test.setBlockType("minecraft:cactus", SUPPORT);
    test.setBlockType("minecraft:cactus_flower", FLOWER);

    pollUntilSucceed(
        test,
        () => getTypeId(test, FLOWER) === "minecraft:cactus_flower",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `cactus_flower survive on cactus: expected cactus_flower to remain at ${JSON.stringify(FLOWER)}, ` +
                        `got ${getTypeId(test, FLOWER)} ` +
                        `(support=${getTypeId(test, SUPPORT)}; ` +
                        `if air, canSustain(cactus) may falsely fail or updatePostPlacement over-triggers self-destruct)`,
                );
            },
        },
    );
}

// 耕地支撑存活（canSustain farmland 显式分支正向）。
// wiki :40 顶面中心完整方块。farmland (3,0,1) + cactus_flower (3,1,1)，等待后断言存活。
function cactusFlowerSurvivesOnFarmland(test: Test): void {
    test.setBlockType("minecraft:farmland", SUPPORT);
    test.setBlockType("minecraft:cactus_flower", FLOWER);

    pollUntilSucceed(
        test,
        () => getTypeId(test, FLOWER) === "minecraft:cactus_flower",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `cactus_flower survive on farmland: expected cactus_flower to remain at ${JSON.stringify(FLOWER)}, ` +
                        `got ${getTypeId(test, FLOWER)} ` +
                        `(support=${getTypeId(test, SUPPORT)}; ` +
                        `if air, canSustain(farmland) may falsely fail)`,
                );
            },
        },
    );
}

// 石头支撑存活（canSustain isFaceSturdy(Up,Center) 兜底分支正向）。
// wiki :40 顶面中心完整的方块。stone (3,0,1) + cactus_flower (3,1,1)。canSustain(stone) 走兜底
// isFaceSturdy(Up, Center)——stone 完整实心方块顶面中心完整 → true。等待后断言存活。
function cactusFlowerSurvivesOnStone(test: Test): void {
    test.setBlockType("minecraft:stone", SUPPORT);
    test.setBlockType("minecraft:cactus_flower", FLOWER);

    pollUntilSucceed(
        test,
        () => getTypeId(test, FLOWER) === "minecraft:cactus_flower",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `cactus_flower survive on stone: expected cactus_flower to remain at ${JSON.stringify(FLOWER)} ` +
                        `(stone top face should be Center-sturdy), ` +
                        `got ${getTypeId(test, FLOWER)} ` +
                        `(support=${getTypeId(test, SUPPORT)}; ` +
                        `if air, isFaceSturdy(Up,Center) for stone may falsely return false)`,
                );
            },
        },
    );
}

// 移除仙人掌支撑 → 自毁（wiki 支撑失效，BushBlock 通用机制）。
// cactus (3,0,1) + cactus_flower (3,1,1)。t=20 移除 cactus→air。cactus_flower updatePostPlacement(Down)
// canSustain(air) 失败（air 非 cactus/farmland，isFaceSturdy(air) false）→ 返 air 自毁。
function cactusFlowerBreaksWhenCactusSupportRemoved(test: Test): void {
    test.setBlockType("minecraft:cactus", SUPPORT);
    test.setBlockType("minecraft:cactus_flower", FLOWER);

    test.runAtTickTime(20, () => {
        if (getTypeId(test, FLOWER) === "minecraft:cactus_flower") {
            test.setBlockType("minecraft:air", SUPPORT); // 移除 cactus，派发 Up 更新触发 cactus_flower updatePostPlacement(Down)
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
                    `cactus_flower break on cactus removed: expected air at ${JSON.stringify(FLOWER)} after removing cactus, ` +
                        `got ${getTypeId(test, FLOWER)} ` +
                        `(support=${getTypeId(test, SUPPORT)} should be air; ` +
                        `if still cactus_flower, BushBlock updatePostPlacement Down-fail->air may be missing)`,
                );
            },
        },
    );
}

export function registerCactusFlowerTests(): void {
    GameTest.register("BlockBehaviorTests", "cactus_flower_survives_on_cactus", cactusFlowerSurvivesOnCactus)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "cactus_flower_survives_on_farmland", cactusFlowerSurvivesOnFarmland)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "cactus_flower_survives_on_stone", cactusFlowerSurvivesOnStone)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "cactus_flower_breaks_when_cactus_support_removed", cactusFlowerBreaksWhenCactusSupportRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
}
