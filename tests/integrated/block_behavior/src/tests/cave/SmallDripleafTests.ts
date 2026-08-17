// 小型垂滴叶（small_dripleaf）集成测试：验证双半联动自毁、支撑失效自毁、存活正向、骨粉催熟成大型垂滴叶
// （对齐 wiki 小型垂滴叶#用途 :39 种植支撑 / :41 骨粉催熟成大型垂滴叶）。
//
// wiki block_小型垂滴叶.txt：
//   #用途（:39）："小型垂滴叶可以直接种在[[黏土]]或[[苔藓块]]上，也可以在至少1格深的[[水源]]中种在
//     黏土、苔藓块、苍白苔藓块、草方块、菌丝体、灰化土、泥土、缠根泥土、砂土、耕地、泥巴或沾泥的红树根上。"
//     ——支撑条件：黏土/苔藓块（SMALL_DRIPLEAF_PLACEABLE 标签）可直接种；或水源下方的 dirt 标签/耕地。
//   #用途（:41）："对小型垂滴叶{{ctrl|使用}}骨粉可使之成长为2-5格高的[[大型垂滴叶]]。若小型垂滴叶上方3格内
//     有方块阻挡，则催熟成的大型垂滴叶最高只能长到该方块的下方。"——骨粉催熟独有行为，随机 2-5 格高度。
//   #破坏（:34-36）：剪刀挖掘掉落自身，否则不掉落（掉落物不测）。
//
// ============================ Cubium 实现链路 ============================
// SmallDripleafBlock（cave/SmallDripleafBlock.cpp）：
//   - isValidPosition（:92-110）：上半须下方是同类型下半（half=Lower）；下半须下方 mayPlaceOn 通过。
//   - mayPlaceOn（:112-132）：下方在 SMALL_DRIPLEAF_PLACEABLE 标签（黏土/苔藓块）→ true；或下方在 DIRT
//     标签/耕地 且 上方有水源（fluidAbove.isSource && WATER）→ true。
//   - updatePostPlacement（:134-174）：Y 轴邻居变化时，若来自连接另一半方向（isLower==isUpDirection）且
//     facingState 不再是同类型另一半 → 返 air 自毁（双半联动自毁，:156-163）；下半 facing==Down 且
//     isValidPosition 失败 → 返 air 自毁（支撑失效自毁，:167-171）。
//   - grow（:222-268，骨粉）：随机茎高 1+nextInt(5)∈[1,5]，检查上方空间（air/自身），移除上下半，
//     放 big_dripleaf_stem（茎）+ big_dripleaf（叶片，顶部）。canGrow/canUseBonemeal 恒 true（100% 催熟）。
//
// ============================ 测试设计 ============================
// 前三个测试用 glass_pit 7×5×7（y∈[0,4]，内部 air y∈[1,4]），第四个骨粉测试用 fall_tower 7×16×7
// （中心 (3,*,3) 1×1 玻璃管，y∈[1,14] air，提供骨粉催熟所需的垂直空间）。
//
// 双半结构（glass_pit）：黏土支撑 (3,1,1)、下半 (3,2,1)、上半 (3,3,1)。
//   注：setBlockType 放 small_dripleaf 默认 half=lower；上半须 setBlockWithStates "half=upper"（同
//   DoublePlantTests 范式）。先黏土后下半后上半（下半放置向 (3,3,1) 派发 Up 更新时 (3,3,1) 还是 air，
//   但下半 updatePostPlacement(Up) isLower==isUpDirection 命中，facingState=air 非 this → 会自毁！）。
//   【关键时序】下半先放时上方 air 会导致下半立即自毁。须先放上半再放下半，或用 setBlockWithStates
//   一次性预置。采用：先黏土，再 setBlockWithStates 放上半（half=upper，此时下方 (3,2,1) 是 air，
//   上半 isValidPosition 须下方是同类型下半——air 非 this，但 setBlockWithStates 直写不经 isValidPosition，
//   强放成功），再 setBlockWithStates 放下半（half=lower，向 (3,3,1) 派发 Up 更新，上半 isLower(false)
//   ==isUpDirection(false) 命中，facingState=下半 is(this)&&half!=upper → 保持，上半不自毁；下半放置向
//   (3,1,1) 黏土派发 Down 更新，下半 facing==Down && isValidPosition（下方黏土 mayPlaceOn 通过）→ 保持）。
//
// 测试1 small_dripleaf_survives_on_clay（黏土支撑双半存活，正向防误判）：
//   黏土(3,1,1)+上半(3,3,1)+下半(3,2,1)。两端 isValidPosition 满足（下半下方黏土 mayPlaceOn 通过；
//   上半下方下半）。等待后断言下半、上半均仍是 small_dripleaf（防双半自毁误触发）。
//
// 测试2 small_dripleaf_upper_half_breaks_when_lower_removed（移除下半→上半自毁，双半联动）：
//   黏土+上半+下半。t=20 移除下半(3,2,1)→air。上半 updatePostPlacement(Down) isLower(false)
//   ==isUpDirection(false) 命中，facingState=air 非 this → 返 air 自毁。断言上半变 air。
//
// 测试3 small_dripleaf_lower_half_breaks_when_clay_support_removed（移除黏土支撑→下半自毁，支撑失效）：
//   黏土+上半+下半。t=20 移除黏土(3,1,1)→air。下半 updatePostPlacement(Down) isValidPosition 失败
//   （下方 air 非 mayPlaceOn）→ 返 air 自毁。下半自毁派发 Up 更新 → 上半 updatePostPlacement(Down)
//   facingState=air 非 this → 上半也自毁。断言下半、上半均变 air。
//
// 测试4 small_dripleaf_bonemeal_grows_into_big_dripleaf（骨粉催熟成大型垂滴叶，wiki :41 独有，fall_tower）：
//   clay(3,0,3)+下半(3,1,3)+上半(3,2,3)。SimulatedPlayer 持骨粉对下半 useItemOnBlock。grow 移除上下半
//   (3,1,3)(3,2,3)，随机放 1-5 格 big_dripleaf_stem + 顶部 1 格 big_dripleaf 叶片。断言：中心柱
//   (3,1,3)..(3,7,3) 出现 big_dripleaf 叶片（typeId），且 (3,1,3) 不再是 small_dripleaf（催熟成功）。
//   grow 同步 setBlockState，useItemOnBlock 返回后即可读。随机高度用扫描断言「叶片出现」而非具体位置。
//
// ============================ 排除项（不写测试）============================
// - 骨粉上方阻挡限高（wiki :41「上方3格内有方块阻挡则最高长到该方块下方」）：grow 上方空间检查
//   (:237-247) 遇非 air/自身返回不放置，但「限高到阻挡下方」的精确语义 Cubium 实现为「空间不足则不催熟」
//   （直接 return，不放置任何），与 wiki「长到阻挡下方」语义可能有偏差，且随机高度+边界复杂，跳过。TODO:
//   待 grow 限高语义对齐后可补。
// - 水源+dirt/耕地种植（wiki :39 第二条件）：mayPlaceOn 水源分支需铺水源+流体状态，复杂度高且与黏土
//   分支行为同类（都验证 mayPlaceOn），本测试用黏土分支（SMALL_DRIPLEAF_PLACEABLE 标签）覆盖核心支撑，
//   水源分支留 TODO。
// - 剪子掉落（wiki :36）：依赖物品工具判定 + 掉落物实体，跳过。
// - 堆肥 30%（wiki :44）：依赖堆肥桶 useItem 链路 + 随机，跳过。
// - 朝向 state（facing）：放置朝向取决于玩家面朝方向（wiki 历史 21w11a），依赖玩家放置朝向，setBlockWithStates
//   预置 facing 不验证玩家朝向逻辑，跳过朝向断言（facing 不影响自毁/催熟行为）。
//
// ============================ 跨服务端对比 ============================
// - typeId 两端不一致：JE small_dripleaf（:243）/ BE small_dripleaf_block（:254）。Cubium 是 Java 克隆
//   用 small_dripleaf。基岩 BDS 对比需用 small_dripleaf_block，run_diff 工具按 fullName 对齐，typeId
//   差异不影响测试结构对齐（测试内 typeId 字面量是 Cubium 侧）。此为已知 JE/BE 命名差异，非行为差异。
// - half state 名两端一致（half=upper/lower，Java 式，BE 1.20.30 改用 cardinal_direction 但 half 仍一致）。
// - 双半联动自毁、支撑失效自毁、骨粉催熟成大型垂滴叶，均为 wiki 明文记录的 1.21.11 一致行为
//   （小型垂滴叶 1.17 加入，1.21.11 已含）。
// - 测试用 setBlockType/setBlockWithStates 放黏土/small_dripleaf/air，useItemOnBlock 骨粉，Cubium 侧验证
//   为主（基岩 typeId 命名差异，行为可对比但 typeId 字面量需注意）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_小型垂滴叶.txt#用途（:39 种植支撑条件）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_小型垂滴叶.txt#用途（:41 骨粉催熟成2-5格大型垂滴叶）
// Ref: SmallDripleafBlock.cpp:92-110（isValidPosition 上半须下方下半/下半须下方 mayPlaceOn）
// Ref: SmallDripleafBlock.cpp:112-132（mayPlaceOn SMALL_DRIPLEAF_PLACEABLE 标签 或 水源+dirt/耕地）
// Ref: SmallDripleafBlock.cpp:134-174（updatePostPlacement 双半联动自毁 isLower==isUpDirection + 下半支撑自毁）
// Ref: SmallDripleafBlock.cpp:222-268（grow 骨粉催熟：随机茎高1-5+叶片，移除上下半放茎+叶）
// Ref: DoublePlantTests.ts（双半 setBlockWithStates "half=upper" 范式 + 双半自毁断言）
// Ref: BoneMealTests.ts（SimulatedPlayer useItemOnBlock 骨粉范式 + ItemStack bone_meal）
// Ref: PointedDripstoneTests.ts（fall_tower 7×16×7 中心管坐标范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 双半结构坐标。黏土支撑 (3,1,1)、下半 (3,2,1)、上半 (3,3,1)。
const CLAY = { x: 3, y: 1, z: 1 };
const LOWER = { x: 3, y: 2, z: 1 }; // small_dripleaf 下半
const UPPER = { x: 3, y: 3, z: 1 }; // small_dripleaf 上半

// fall_tower 骨粉测试坐标。clay(3,0,3)、下半(3,1,3)、上半(3,2,3)，中心管向上 air 至 (3,14,3)。
const T_CLAY = { x: 3, y: 0, z: 3 };
const T_LOWER = { x: 3, y: 1, z: 3 };
const T_UPPER = { x: 3, y: 2, z: 3 };

// setBlockWithStates 访问器（Test 未在类型暴露，cast 访问，同 DoublePlantTests/BannerTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 放置黏土支撑 + small_dripleaf 双半（上半 half=upper + 下半 half=lower）。
// 【关键时序】先黏土，再 setBlockWithStates 放上半（half=upper，直写不经 isValidPosition，强放），
// 再放下半（half=lower）。下半放置向 (3,3,1) 派发 Up 更新：上半 isLower(false)==isUpDirection(false) 命中，
// facingState=下半 is(this)&&half!=upper → 保持（上半不自毁）。下半向 (3,1,1) 黏土派发 Down 更新：
// isValidPosition（下方黏土 mayPlaceOn 通过）→ 保持（下半不自毁）。若先放下半，下方 air 会导致下半立即自毁。
function placeClayAndSmallDripleaf(test: Test): void {
    test.setBlockType("minecraft:clay", CLAY);
    (test as TestWithStates).setBlockWithStates("minecraft:small_dripleaf", UPPER, "facing=north,half=upper");
    (test as TestWithStates).setBlockWithStates("minecraft:small_dripleaf", LOWER, "facing=north,half=lower");
}

// 黏土支撑双半存活（正向防误判，验证 isValidPosition 两端满足时不触发双半自毁）。
// wiki :39 黏土可种小型垂滴叶。黏土+上半+下半，不做破坏，等待后断言下半、上半均仍存在。
function smallDripleafSurvivesOnClay(test: Test): void {
    placeClayAndSmallDripleaf(test);

    // 等待足够 tick（超过自毁反应窗口），断言双半均仍存在（isValidPosition 满足，不自毁）。
    pollUntilSucceed(
        test,
        () =>
            getTypeId(test, LOWER) === "minecraft:small_dripleaf" &&
            getTypeId(test, UPPER) === "minecraft:small_dripleaf",
        {
            startTick: 20,
            interval: 10,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `small_dripleaf survive: expected both halves to remain on clay, ` +
                        `got lower=${getTypeId(test, LOWER)} upper=${getTypeId(test, UPPER)} ` +
                        `(clay=${getTypeId(test, CLAY)}; ` +
                        `if air, double-half self-destruct may over-trigger or isValidPosition falsely fails)`,
                );
            },
        },
    );
}

// 移除下半 → 上半自毁（双半联动自毁，wiki 隐含：双高植物另一半消失则自毁，SmallDripleaf 独立实现）。
// 黏土+上半+下半。t=20 移除下半(3,2,1)→air。上半 updatePostPlacement(Down) isLower(false)
// ==isUpDirection(false) 命中，facingState=air 非 this → 返 air 自毁。
function smallDripleafUpperHalfBreaksWhenLowerRemoved(test: Test): void {
    placeClayAndSmallDripleaf(test);

    test.runAtTickTime(20, () => {
        if (getTypeId(test, LOWER) === "minecraft:small_dripleaf") {
            test.setBlockType("minecraft:air", LOWER); // 移除下半，派发 Up 更新触发上半 updatePostPlacement(Down)
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, UPPER) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `upper break on lower removed: expected air at ${JSON.stringify(UPPER)} after removing lower half, ` +
                        `got ${getTypeId(test, UPPER)} ` +
                        `(lower=${getTypeId(test, LOWER)} should be air; ` +
                        `if still small_dripleaf, updatePostPlacement double-half self-destruct (upper on lower-removed) may be missing)`,
                );
            },
        },
    );
}

// 移除黏土支撑 → 下半自毁 + 上半链式自毁（支撑失效自毁，wiki :39 支撑条件）。
// 黏土+上半+下半。t=20 移除黏土(3,1,1)→air。下半 updatePostPlacement(Down) isValidPosition 失败
// （下方 air 非 mayPlaceOn）→ 返 air 自毁。下半自毁派发 Up 更新 → 上半 updatePostPlacement(Down)
// facingState=air 非 this → 上半也自毁。
function smallDripleafLowerHalfBreaksWhenClaySupportRemoved(test: Test): void {
    placeClayAndSmallDripleaf(test);

    test.runAtTickTime(20, () => {
        if (getTypeId(test, LOWER) === "minecraft:small_dripleaf") {
            test.setBlockType("minecraft:air", CLAY); // 移除黏土，派发 Up 更新触发下半 updatePostPlacement(Down)
        }
    });

    pollUntilSucceed(
        test,
        () => getTypeId(test, LOWER) === "minecraft:air" && getTypeId(test, UPPER) === "minecraft:air",
        {
            startTick: 25,
            interval: 2,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `lower break on clay removed: expected both halves air after removing clay, ` +
                        `got lower=${getTypeId(test, LOWER)} upper=${getTypeId(test, UPPER)} ` +
                        `(clay=${getTypeId(test, CLAY)} should be air; ` +
                        `if lower air but upper not, upper self-destruct on lower-removed may be missing)`,
                );
            },
        },
    );
}

// 骨粉催熟成大型垂滴叶（wiki :41 独有，fall_tower 提供垂直空间）。
// clay(3,0,3)+下半(3,1,3)+上半(3,2,3)。SimulatedPlayer 持骨粉对下半 useItemOnBlock → grow 移除上下半，
// 随机放 1-5 格 big_dripleaf_stem + 顶部 big_dripleaf 叶片。断言中心柱出现 big_dripleaf 叶片 +
// (3,1,3) 不再是 small_dripleaf。grow 同步，useItemOnBlock 返回后即可读。
function smallDripleafBonemealGrowsIntoBigDripleaf(test: Test): void {
    test.setBlockType("minecraft:clay", T_CLAY);
    // 同 placeClayAndSmallDripleaf 时序：先上半后下半，避免下半先放上方 air 自毁。
    (test as TestWithStates).setBlockWithStates("minecraft:small_dripleaf", T_UPPER, "facing=north,half=upper");
    (test as TestWithStates).setBlockWithStates("minecraft:small_dripleaf", T_LOWER, "facing=north,half=lower");

    // SimulatedPlayer 持骨粉。spawn 在中心管内 (3,3,3)（上半上方 air），对下半 (3,1,3) 使用骨粉。
    const farmer = test.spawnSimulatedPlayer({ x: 3, y: 3, z: 3 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对下半 useItemOnBlock。grow 从下半 basePos 处理（移除上下半 + 放茎/叶）。
    // direction=Up（从上方使用），faceLocation 默认方块中心。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        T_LOWER,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when bone meal grows small_dripleaf into big_dripleaf");

    // grow 同步 setBlockState，useItemOnBlock 返回后即可读。随机茎高 1-5 + 叶片 1 格，叶片在 basePos+stemHeight
    // （即 (3,1+stemHeight,3)，stemHeight∈[1,5]，叶片 y∈[2,6]）。扫描中心柱 (3,1,3)..(3,7,3) 找 big_dripleaf 叶片。
    // 同时断言 (3,1,3) 不再是 small_dripleaf（催熟已发生，原下半被移除或替换为茎）。
    pollUntilSucceed(
        test,
        () => {
            if (getTypeId(test, T_LOWER) === "minecraft:small_dripleaf") {
                return false; // 催熟未发生（下半仍是 small_dripleaf）
            }
            // 扫描中心柱找 big_dripleaf 叶片（grow 放置的顶部叶片）。
            for (let y = 2; y <= 7; ++y) {
                if (getTypeId(test, { x: 3, y, z: 3 }) === "minecraft:big_dripleaf") {
                    return true;
                }
            }
            return false;
        },
        {
            startTick: 5,
            interval: 5,
            maxTick: 40,
            onTimeout: () => {
                const col: string[] = [];
                for (let y = 1; y <= 7; ++y) {
                    col.push(`y${y}=${getTypeId(test, { x: 3, y, z: 3 })}`);
                }
                test.assert(
                    false,
                    `bonemeal grow: expected big_dripleaf leaf in column (3,1-7,3) after bone meal, ` +
                        `column=[${col.join(", ")}] ` +
                        `(used=${used}; if no big_dripleaf, grow may not place leaf or useItemOnBlock bonemeal link is missing)`,
                );
            },
        },
    );
}

export function registerSmallDripleafTests(): void {
    GameTest.register("BlockBehaviorTests", "small_dripleaf_survives_on_clay", smallDripleafSurvivesOnClay)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "small_dripleaf_upper_half_breaks_when_lower_removed", smallDripleafUpperHalfBreaksWhenLowerRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "small_dripleaf_lower_half_breaks_when_clay_support_removed", smallDripleafLowerHalfBreaksWhenClaySupportRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "small_dripleaf_bonemeal_grows_into_big_dripleaf", smallDripleafBonemealGrowsIntoBigDripleaf)
        .structureName("gametests:fall_tower")
        .maxTicks(80);
}
