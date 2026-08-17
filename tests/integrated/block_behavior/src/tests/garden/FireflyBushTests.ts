// 萤火虫灌木（firefly_bush）集成测试：验证种植支撑存活、骨粉蔓延生成新灌木、无可种植邻居时骨粉不生效、
// 支撑失效自毁行为（对齐 wiki 萤火虫灌木丛#骨粉 :26 / #用途 :40 / #破坏 :37）。
//
// wiki block_萤火虫灌木丛.txt：
//   #骨粉（:26）："对着{{only|be|for=不含雪的}}萤火虫灌木丛使用骨粉会在曼哈顿距离为1的随机可种植方块上
//     生成一个新的萤火虫灌木丛。"——骨粉 grow 独有：在水平方向（曼哈顿距离1）的随机可种植方块生成
//     新灌木。JE+BE 行为一致（:26 only be 限定仅「不含雪」，骨粉行为本身两端一致）。
//   #用途（:40）："萤火虫灌木丛可以种植在草方块、菌丝体、灰化土、泥土、缠根泥土、砂土、耕地、泥巴、
//     沾泥的红树根、苔藓块和苍白苔藓块上。"——支撑面属 DIRT 标签面，canSustain 委托 canSustainPlant
//     匹配 DIRT 标签（草方块/泥土/耕地等）。
//   #破坏（:37）："萤火虫灌木丛被破坏后会掉落自身。"——掉落依赖战利品表/掉落物实体，跳过。
//   #光源（:47）：发光等级2（lighting 包已测 FireflyBushEmissionTests，本包不重复）。
//   #堆肥（:52）：30% 堆肥概率，依赖堆肥桶 useItem 链路 + 随机，跳过。
//
// ============================ Cubium 实现链路 ============================
// FireflyBushBlock（garden/FireflyBushBlock.cpp）继承 BushBlock + IGrowable：
//   - canGrow（hasSpreadableNeighbourPos 等价）：正序遍历水平4方向邻居，找首个「air + 其下方可支撑
//     （isValidPosition，即下方 DIRT 标签/耕地）」的位置，存在则 true。
//   - canUseBonemeal：恒 true（骨粉100%成功）。
//   - grow（findSpreadableNeighbourPos 等价）：用 IRandom::shuffle（Fisher-Yates）打乱水平4方向
//     随机序遍历，找首个可种植蔓延位置 setBlockState(defaultState, 3) 生成新灌木。曼哈顿距离1（仅水平）。
//   - 不重写 updatePostPlacement → 继承 BushBlock::updatePostPlacement（agricultural/BushBlock.cpp:67-93）：
//     facing==Down 时重检下方 canSustain，失败返 airState（同步返 air 自毁）。
//   - canSustain 走 BushBlock 基类（:121-132 委托下方方块 canSustainPlant，DIRT 标签面 + PlantType 匹配）。
//
// 骨粉派发链路（BoneMealItem.cpp:67-97）：useItemOnBlock → BoneMealItem::onItemUse →
//   dynamic_cast<IGrowable*> → canGrow（hasSpreadableNeighbourPos）→ canUseBonemeal(true) → grow
//   （findSpreadableNeighbourPos shuffle + setBlockState 同步生成新灌木）→ shrink(1) 消耗骨粉。
//   grow 同步 setBlockState，useItemOnBlock 返回后即可读新灌木方块。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃墙空腔）============================
// glass_pit 布局（实测 .mcstructure）：y=0 全 glass 底；y=1 cobble 角柱+glass 边框，内部 x,z∈[1,5] 为 air；
// y=2 同 y=1（内部 air）；y=3/y=4 开放。方块测试在内部 air 层操作。
//
// 列 (3,*,1)：(3,1,1) 与 (3,2,1) 均为 air（内部空腔）。SUPPORT=(3,1,1) 支撑，BUSH=(3,2,1) 原灌木。
// (3,2,1) 的4个水平邻居：west (2,2,1)=air 下方(2,1,1)=air；east (4,2,1)=air 下方(4,1,1)=air；
// north (3,2,0)=glass（玻璃墙，非 air）；south (3,2,2)=air 下方(3,1,2)=air。
//
// 测试1 firefly_bush_survives_on_grass_block（草方块支撑存活，正向防误判）：
//   grass_block (3,1,1) + firefly_bush (3,2,1)。canSustain(grass_block) DIRT 标签通过，等待后断言存活。
//
// 测试2 firefly_bush_bonemeal_spreads_to_plantable_neighbour（骨粉蔓延生成新灌木，wiki :26 核心行为）：
//   grass_block (3,1,1) 支撑 + firefly_bush (3,2,1)。把 (4,1,1) 设 grass_block，使 east 邻居 (4,2,1)
//   成为唯一可种植蔓延目标（air + 下方 grass_block 可支撑）。其余3邻居：north (3,2,0)=glass 非 air 跳过；
//   west (2,2,1) 下方(2,1,1)=air 不可支撑；south (3,2,2) 下方(3,1,2)=air 不可支撑。
//   SimulatedPlayer 持骨粉对 (3,2,1) useItemOnBlock → grow findSpreadableNeighbourPos shuffle 遍历，
//   唯一目标 (4,2,1) 命中（无论随机序如何，仅此1个可种植位置）→ setBlockState firefly_bush。
//   断言 (4,2,1) 变 firefly_bush（确定性：唯一可种植目标消除方向随机性）。
//   注：原灌木 (3,2,1) 骨粉后不变化（grow 不修改原方块，只在邻居生成新灌木），断言原灌木仍 firefly_bush。
//
// 测试3 firefly_bush_bonemeal_no_spread_without_plantable_neighbour（无可种植邻居骨粉不生效，反向验证 canGrow）：
//   grass_block (3,1,1) 支撑 + firefly_bush (3,2,1)。所有水平邻居下方保持 air/glass 不可支撑：
//   west (2,1,1)=air、east (4,1,1)=air、south (3,1,2)=air 不可支撑；north (3,2,0)=glass 非 air。
//   canGrow=hasSpreadableNeighbourPos 遍历无果→false，BoneMealItem 不调 grow（canGrow false 短路）。
//   SimulatedPlayer 持骨粉 useItemOnBlock，断言：4邻居位置均非 firefly_bush（无蔓延），原灌木仍 firefly_bush。
//   验证 canGrow 正确判定「无可种植邻居」时骨粉不生效（防误判：canGrow 漏判会致 grow 误生成）。
//
// 测试4 firefly_bush_breaks_when_grass_support_removed（移除草方块支撑自毁，wiki 支撑失效）：
//   grass_block (3,1,1) + firefly_bush (3,2,1)。t=20 移除 grass_block→air。firefly_bush updatePostPlacement(Down)
//   canSustain(air) 失败 → 返 air 自毁。断言 (3,2,1) 变 air。
//
// ============================ 排除项（不写测试）============================
// - 破坏掉落自身（wiki :37）：依赖战利品表/掉落物实体，Cubium 自毁链路不掉落（项目级缺陷，见记忆
//   block-self-destruct-no-drops-project-defect），跳过。TODO 待项目级自毁掉落缺陷修复后补。
// - 堆肥30%（wiki :52）：依赖堆肥桶 useItem 链路 + 随机，跳过。
// - 含雪变体（wiki :42 BE 可含雪）：含雪状态依赖雪层方块叠加 + 骨粉排除含雪，BE 专属且 Cubium 含雪链路
//   未完整实现，跳过。TODO 待含雪机制实现后补。
// - 萤火虫粒子/环境音效（wiki :49 :55）：依赖客户端粒子系统 + 时间/露天判定，无脚本 API 可测，跳过。
// - 自然生成（wiki :17-19）：依赖地物 + 生物群系 + 随机，跳过。
//
// ============================ 跨服务端对比 ============================
// - firefly_bush typeId 两端一致（JE 1.21.5 25w02a / BE 1.21.70 加入，1.21.11 已含，wiki :297 :307）。
// - 骨粉蔓延生成新灌木（:26）为 wiki 明文记录的 JE+BE 一致行为（:26 仅 be 限定「不含雪」，骨粉行为本身
//   两端一致），可两端对比。
// - 支撑面（:40 DIRT 标签）、支撑失效自毁（BushBlock 通用机制）两端一致。
// - 测试用 setBlockType 放 grass_block/firefly_bush/air + useItemOnBlock 骨粉（bone_meal 物品两端通用），
//   均为两端通用 API，非 one-sided。注：firefly_bush 物品 Cubium 未注册（见记忆
//   garden-new-blocks-missing-item-registration），但本测试用 setBlockType 放原灌木不经物品链路，骨粉物品
//   已注册，故不受物品注册缺失影响。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_萤火虫灌木丛.txt#骨粉（:26 曼哈顿距离1随机可种植方块生成新灌木）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_萤火虫灌木丛.txt#用途（:40 支撑面 DIRT 标签）/ #破坏（:37 掉落自身）
// Ref: FireflyBushBlock.cpp（canGrow hasSpreadableNeighbourPos / canUseBonemeal true / grow findSpreadableNeighbourPos shuffle + setBlockState）
// Ref: BushBlock.cpp:67-93（updatePostPlacement facing==Down && !canSustain → 返 air 同步自毁）
// Ref: BushBlock.cpp:121-132（canSustain 委托下方方块 canSustainPlant，DIRT 标签面 + PlantType）
// Ref: BoneMealItem.cpp:67-97（dynamic_cast<IGrowable> → canGrow → canUseBonemeal → grow 同步生成新灌木）
// Ref: WildflowersTests.ts（glass_pit (3,1,1)支撑+(3,2,1)植物 + useItemOnBlock 骨粉 + 支撑自毁范式）
// Ref: BonemealableBlock.java（hasSpreadableNeighbourPos 正序 / findSpreadableNeighbourPos shuffledCopy 随机序）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 内部 air 层坐标。SUPPORT=(3,1,1) 支撑（覆盖 air），BUSH=(3,2,1) 原灌木（air 层）。
// glass_pit 列 (3,*,1)：(3,1,1) 与 (3,2,1) 均为内部 air（实测 .mcstructure）。setBlockType 直写覆盖 air，
// useItemOnBlock 骨粉点击 BUSH 即可（骨粉物品已注册，无需 firefly_bush 物品）。
const SUPPORT = { x: 3, y: 1, z: 1 }; // 下方支撑（grass_block，setBlockType 覆盖 air）
const BUSH = { x: 3, y: 2, z: 1 }; // 萤火虫灌木位置（air 层，setBlockType 放置）

// (3,2,1) 的4个水平邻居（曼哈顿距离1）。north (3,2,0)=glass 玻璃墙非 air；其余3邻居 air。
// 测试2 把 EAST_NEIGHBOUR_BELOW=(4,1,1) 设 grass_block，使 EAST_NEIGHBOUR=(4,2,1) 成唯一可种植目标。
const WEST_NEIGHBOUR = { x: 2, y: 2, z: 1 };
const EAST_NEIGHBOUR = { x: 4, y: 2, z: 1 };
const NORTH_NEIGHBOUR = { x: 3, y: 2, z: 0 };
const SOUTH_NEIGHBOUR = { x: 3, y: 2, z: 2 };
// east 邻居下方（测试2 设 grass_block 使其可支撑）。
const EAST_NEIGHBOUR_BELOW = { x: 4, y: 1, z: 1 };

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 放置 grass_block 支撑 + 萤火虫灌木。前置共用。
function placeGrassAndFireflyBush(test: Test): void {
    test.setBlockType("minecraft:grass_block", SUPPORT);
    test.setBlockType("minecraft:firefly_bush", BUSH);
}

// 草方块支撑存活（正向防误判，验证 canSustain(grass_block) DIRT 标签通过时不触发自毁）。
// wiki :40 萤火虫灌木可种草方块。grass_block (3,1,1) + firefly_bush (3,2,1)，等待后断言存活。
function fireflyBushSurvivesOnGrassBlock(test: Test): void {
    placeGrassAndFireflyBush(test);

    pollUntilSucceed(
        test,
        () => getTypeId(test, BUSH) === "minecraft:firefly_bush",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `firefly_bush survive: expected firefly_bush to remain at ${JSON.stringify(BUSH)} on grass_block, ` +
                        `got ${getTypeId(test, BUSH)} ` +
                        `(support=${getTypeId(test, SUPPORT)}; ` +
                        `if air, canSustain(grass_block) DIRT-tag may falsely fail or updatePostPlacement over-triggers self-destruct)`,
                );
            },
        },
    );
}

// 骨粉蔓延生成新灌木（wiki :26 曼哈顿距离1随机可种植方块生成新灌木，核心行为）。
// grass_block (3,1,1) 支撑 + firefly_bush (3,2,1)。把 (4,1,1) 设 grass_block，使 east 邻居 (4,2,1) 成
// 唯一可种植蔓延目标。SimulatedPlayer 持骨粉对 (3,2,1) useItemOnBlock → grow findSpreadableNeighbourPos
// shuffle 遍历，唯一目标 (4,2,1) 命中 → setBlockState firefly_bush。断言 (4,2,1) 变 firefly_bush（确定性）。
function fireflyBushBonemealSpreadsToPlantableNeighbour(test: Test): void {
    placeGrassAndFireflyBush(test);
    // east 邻居下方设 grass_block，使 (4,2,1) 成唯一可种植蔓延目标（消除方向随机性）。
    test.setBlockType("minecraft:grass_block", EAST_NEIGHBOUR_BELOW);
    test.assert(
        getTypeId(test, SUPPORT) === "minecraft:grass_block" && getTypeId(test, EAST_NEIGHBOUR_BELOW) === "minecraft:grass_block",
        `grass_block supports should be placed (SUPPORT=${getTypeId(test, SUPPORT)} EAST_BELOW=${getTypeId(test, EAST_NEIGHBOUR_BELOW)})`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对原灌木 (3,2,1) 使用骨粉。direction=Up（从上方使用）。
    // BoneMealItem::onItemUse → dynamic_cast<IGrowable> → canGrow(hasSpreadableNeighbourPos, east 目标存在→true)
    // → canUseBonemeal(true) → grow findSpreadableNeighbourPos shuffle 遍历，唯一目标 (4,2,1) 命中
    // → setBlockState firefly_bush（同步）。useItemOnBlock 返回后即可读。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        BUSH,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when bone meal spreads firefly_bush to plantable neighbour");

    // grow 同步 setBlockState，留 small pollUntilSucceed 兼容可能延迟。
    pollUntilSucceed(
        test,
        () => {
            // 原灌木 (3,2,1) 须仍 firefly_bush（grow 不改原方块，只在邻居生成新灌木）。
            if (getTypeId(test, BUSH) !== "minecraft:firefly_bush") {
                return false;
            }
            // east 邻居 (4,2,1) 须变 firefly_bush（唯一可种植蔓延目标命中）。
            return getTypeId(test, EAST_NEIGHBOUR) === "minecraft:firefly_bush";
        },
        {
            startTick: 2,
            interval: 2,
            maxTick: 20,
            onTimeout: () => {
                test.assert(
                    false,
                    `firefly_bush bonemeal spread: expected firefly_bush at ${JSON.stringify(EAST_NEIGHBOUR)} after bone meal, ` +
                        `got ${getTypeId(test, EAST_NEIGHBOUR)} ` +
                        `(bush=${getTypeId(test, BUSH)} should stay firefly_bush; east_below=${getTypeId(test, EAST_NEIGHBOUR_BELOW)} should be grass_block; ` +
                        `if east neighbour not firefly_bush, FireflyBushBlock::grow findSpreadableNeighbourPos may be missing or canGrow falsely false)`,
                );
            },
        },
    );
}

// 无可种植邻居时骨粉不生效（反向验证 canGrow=hasSpreadableNeighbourPos 正确判定无目标）。
// grass_block (3,1,1) 支撑 + firefly_bush (3,2,1)。所有水平邻居下方保持 air/glass 不可支撑：
// west (2,1,1)=air、east (4,1,1)=air、south (3,1,2)=air 不可支撑；north (3,2,0)=glass 非 air。
// canGrow=hasSpreadableNeighbourPos 遍历无果→false，BoneMealItem 不调 grow。useItemOnBlock 后断言：
// 4邻居均非 firefly_bush（无蔓延），原灌木仍 firefly_bush。
function fireflyBushBonemealNoSpreadWithoutPlantableNeighbour(test: Test): void {
    placeGrassAndFireflyBush(test);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对原灌木 (3,2,1) 使用骨粉。canGrow=hasSpreadableNeighbourPos 无可种植邻居→false，
    // BoneMealItem.cpp:74 canGrow 短路，不调 canUseBonemeal/grow。useItemOnBlock 返回 false（骨粉未生效）。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        BUSH,
        Direction.Up,
    );
    // useItemOnBlock 返回 false（骨粉未消耗，canGrow false 短路）。不强制断言 used（基岩侧骨粉行为可能不同），
    // 改用方块状态断言蔓延未发生（更鲁棒）。

    // 留 tick 确保无延迟生成后断言。
    pollUntilSucceed(
        test,
        () => {
            // 原灌木须仍 firefly_bush（未被骨粉破坏）。
            if (getTypeId(test, BUSH) !== "minecraft:firefly_bush") {
                return false;
            }
            // 4邻居均须非 firefly_bush（无可种植邻居时 grow 不应被调用，无蔓延）。
            const spread =
                getTypeId(test, WEST_NEIGHBOUR) === "minecraft:firefly_bush" ||
                getTypeId(test, EAST_NEIGHBOUR) === "minecraft:firefly_bush" ||
                getTypeId(test, NORTH_NEIGHBOUR) === "minecraft:firefly_bush" ||
                getTypeId(test, SOUTH_NEIGHBOUR) === "minecraft:firefly_bush";
            return !spread;
        },
        {
            startTick: 2,
            interval: 2,
            maxTick: 20,
            onTimeout: () => {
                test.assert(
                    false,
                    `firefly_bush bonemeal no-spread: expected no firefly_bush at any neighbour after bone meal with no plantable neighbour, ` +
                        `but spread detected ` +
                        `(west=${getTypeId(test, WEST_NEIGHBOUR)} east=${getTypeId(test, EAST_NEIGHBOUR)} ` +
                        `north=${getTypeId(test, NORTH_NEIGHBOUR)} south=${getTypeId(test, SOUTH_NEIGHBOUR)}; ` +
                        `bush=${getTypeId(test, BUSH)}; ` +
                        `if any neighbour is firefly_bush, FireflyBushBlock::canGrow hasSpreadableNeighbourPos may falsely return true)`,
                );
            },
        },
    );
}

// 移除草方块支撑 → 自毁（wiki 支撑失效，BushBlock 通用机制）。
// grass_block (3,1,1) + firefly_bush (3,2,1)。t=20 移除 grass_block→air。firefly_bush updatePostPlacement(Down)
// canSustain(air) 失败 → 返 air 自毁。
function fireflyBushBreaksWhenGrassSupportRemoved(test: Test): void {
    placeGrassAndFireflyBush(test);

    test.runAtTickTime(20, () => {
        if (getTypeId(test, BUSH) === "minecraft:firefly_bush") {
            test.setBlockType("minecraft:air", SUPPORT); // 移除 grass_block，派发 Up 更新触发 firefly_bush updatePostPlacement(Down)
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, BUSH) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `firefly_bush break on grass removed: expected air at ${JSON.stringify(BUSH)} after removing grass_block, ` +
                        `got ${getTypeId(test, BUSH)} ` +
                        `(support=${getTypeId(test, SUPPORT)} should be air; ` +
                        `if still firefly_bush, BushBlock updatePostPlacement Down-fail->air may be missing)`,
                );
            },
        },
    );
}

export function registerFireflyBushTests(): void {
    GameTest.register("BlockBehaviorTests", "firefly_bush_survives_on_grass_block", fireflyBushSurvivesOnGrassBlock)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "firefly_bush_bonemeal_spreads_to_plantable_neighbour", fireflyBushBonemealSpreadsToPlantableNeighbour)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "firefly_bush_bonemeal_no_spread_without_plantable_neighbour", fireflyBushBonemealNoSpreadWithoutPlantableNeighbour)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "firefly_bush_breaks_when_grass_support_removed", fireflyBushBreaksWhenGrassSupportRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
}
