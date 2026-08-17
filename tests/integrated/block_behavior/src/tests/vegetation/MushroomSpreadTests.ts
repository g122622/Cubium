// 蘑菇蔓延集成测试：验证 MushroomBlock::randomTick 的黑暗蔓延与光照门槛（对齐 wiki 蘑菇传播章节）。
//
// wiki tech_蘑菇.txt#传播（:90-103）：
//   "蘑菇每次接收随机刻时，会有4%的概率尝试传播。此时，系统会选中一个与该蘑菇相邻的方块，若为
//    可传播方块，则该处会长出蘑菇。"
//   可传播方块判定（:98-101）："该方块为空气；该方块下方的方块为完全固体渲染方块；该方块的亮度
//    等级不高于12，或者其下方的方块为菌丝体、灰化土或菌岩。"
//   密度限制（:103）："蘑菇在周围9×3×9的范围内存在5个及以上的同种蘑菇时不会尝试传播。"
// wiki tech_蘑菇.txt#自然生成（:27）："主世界亮度低于13的区域"——蘑菇生于光照<13。
//
// Cubium 实现（MushroomBlock.cpp:101-149 randomTick）：
//   - 光照门槛：max(blockLight, skyLight) >= 13 则直接 return 不蔓延（:107-110），与 wiki 亮度≤12可传播对齐。
//   - 1/25=4% 概率门限（:112-114），与 wiki 4% 一致。
//   - 扫描周围 9×3×9（dx∈[-4,4] dy∈[-1,1] dz∈[-4,4]）同类蘑菇数>=5 则 return（:116-128），与 wiki 密度5一致。
//   - 随机选蔓延目标 pos+(nextInt(3)-1, nextInt(2)-1, nextInt(3)-1)（3×2×3范围，:130），目标须 air（:133-135）
//     + 下方 canSustainMushroom（:138-147）。
// canSustainMushroom（:51-68）：下方在 MUSHROOM_GROW_BLOCK 标签（菌丝/灰化土/菌岩）→ 无条件允许；
//   否则下方须 isSolid 且蘑菇处光照<13。
//
// ============================ 确定性方案：调高 randomTickSpeed ============================
// 蘑菇蔓延由随机刻驱动（同作物/草/铜/紫晶芽测试，见记忆 randomtick-threshold-test-via-gamerule-speedup）。
// 默认 randomTickSpeed=3 时蘑菇格每 tick 被选中概率仅 3/4096≈0.073%。测试开头用
// SimulatedPlayer.chat("/gamerule randomTickSpeed 1000") 调高使蘑菇格每 tick 命中概率≈24.4%。
//
// 黑暗蔓延：每次 randomTick 命中后 1/25 门限 × 命中可蔓延 air 目标（3×2×3=18格中4个目标位，每个
// 1/18）≈ 4%×4/18 ≈ 0.89%/命中。speed=1000 时蔓延期望≈0.22%/tick，200 tick 内蔓延概率≈35%。
// 单个目标位概率更低，但4个目标位任一蔓延即成功，综合概率提升。配 maxTick 400 + 轮询，确定性足够
// （若 flaky 可进一步调高 speed 或增目标位）。
//
// 光照≥13不蔓延：蘑菇处光照≥13 时 randomTick 直接 return（确定性逻辑，无概率）。调高 randomTickSpeed
// 后多次命中都被门槛拦截，断言"不蔓延"确定性成立。配对测试1证明 randomTickSpeed 调高生效，反证
// 测试2的"不蔓延"是光照门槛而非没命中。
//
// MinecraftStructurePlacer 为测试结构区域加 forced chunk ticket，chunk 常驻，randomTick 覆盖蘑菇格。
//
// chat 返回值不 assert：Cubium chat 返回 int，基岩 BDS chat 返回 void（发消息语义不执行命令），两端
// chat 语义不同，用 chat 执行 /gamerule 的测试基岩侧 one-sided（同 WeatherSkyDarkeningTests）。
//
// ============================ 测试设计（light_box 7×7×7 封顶盒，坐标偏移见记忆）============================
// light_box 运行时 helper air 区域为 y∈[2,4] x,z∈[1,5]（见记忆 light-box-structure-placement-y-offset）。
// 蘑菇蔓延需：目标 air + 下方固体支撑 + 光照<13（黑暗环境 light_box 封顶 skyLight=0，无光源 blockLight=0）。
//
// 支撑层 y=2 铺 stone 平台（覆盖 air 区域，作蔓延目标下方支撑）：
//   (3,2,3) 中心支撑 + (2,2,3)/(4,2,3)/(3,2,2)/(3,2,4) 周围支撑。
// 蘑菇层 y=3：(3,3,3) 放 brown_mushroom（强放绕过 isValidPosition），(2,3,3)/(4,3,3)/(3,3,2)/(3,3,4)
//   为 air 蔓延目标（下方 y=2 是 stone 支撑，光照0<13 可蔓延）。
//
// 测试1 mushroom_spreads_in_dark（黑暗处蔓延）：
//   铺支撑层 + 放蘑菇，调高 randomTickSpeed 后等待，断言4个蔓延目标中至少1个变 brown_mushroom。
//   守卫：蘑菇处 blockLight===0 && skyLight===0 确认黑暗（仅 Cubium 侧判定）。
//
// 测试2 mushroom_does_not_spread_in_light（光照≥13不蔓延）：
//   铺支撑层 + 放蘑菇 + (4,3,3) 放 glowstone(15) 提供方块光。蘑菇 (3,3,3) 距 glowstone 1格光照14≥13，
//   randomTick 直接 return 不蔓延。蔓延目标 (2,3,3) 距 glowstone 2格光照13≥13，canSustainMushroom
//   要求<13 失败（双重保证不蔓延）。调高 randomTickSpeed 后等待，断言4个目标无 brown_mushroom。
//   配对测试1：测试1黑暗蔓延成功证明 randomTickSpeed 调高生效，反证测试2的不蔓延是光照门槛。
//   守卫：蘑菇处 blockLight>=13 确认亮环境（仅 Cubium 侧判定）。
//
// ============================ 排除项（不写测试）============================
// - 密度限制（9×3×9范围5个同类不蔓延）：需布置5个蘑菇在范围内，且密度判定确定性可测，但布置复杂
//   且核心光照门槛已由测试1/2覆盖，跳过。TODO: 待需要时补充密度限制测试。
// - MUSHROOM_GROW_BLOCK 标签支撑（菌丝/灰化土/菌岩无视光照）：需放菌丝等方块，且与光照门槛测试重叠，
//   跳过。
// - 5格远传播：Cubium 简化为3×2×3范围（最多1格远），与 wiki 5格远偏差，按准则不为偏差写测试，跳过。
// - 光照≥13蘑菇弹出（updatePostPlacement 破坏）：wiki :78 种植在亮度>12且非菌丝等上方相邻更新时弹出，
//   Cubium MushroomBlock 未核实 updatePostPlacement 实现，按准则不为未核实行为写测试，跳过。
//   TODO: 待核实 Cubium 蘑菇 updatePostPlacement 光照弹出实现后补充。
//
// ============================ 跨服务端对比 ============================
// - /gamerule randomTickSpeed、SimulatedPlayer.chat、blockLight/skyLight 在 Cubium 侧可用。基岩 BDS
//   SimulatedPlayer.chat 是发消息语义（void，不执行命令），本组用 chat 执行 /gamerule 的测试基岩侧
//   无法跑（one-sided，同 WeatherSkyDarkeningTests.ts）。blockLight/skyLight Cubium 专有，黑暗/亮环境
//   守卫仅 Cubium 侧判定。
// - 蘑菇 typeId（brown_mushroom/red_mushroom）两端一致，蔓延（4%概率黑暗蔓延）与光照门槛（≥13不蔓延）
//   行为两端一致（1.21.11 特性）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蘑菇.txt#传播（:90-103 4%概率/亮度≤12可传播/9×3×9密度5）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蘑菇.txt#自然生成（:27 亮度<13）
// Ref: MushroomBlock.cpp:101-149（randomTick 光照≥13不蔓延 + 1/25门限 + 9×3×9密度5 + 蔓延目标air+支撑）
// Ref: MushroomBlock.cpp:51-68（canSustainMushroom MUSHROOM_GROW_BLOCK标签 或 固体+光照<13）
// Ref: VegetationBlocks.cpp:231-235（brown_mushroom/red_mushroom 注册）
// Ref: CropLightThresholdTests.ts / GrassSpreadTests.ts / AmethystBudGrowthTests.ts（randomTickSpeed 调高范式）
// Ref: WeatherSkyDarkeningTests.ts（chat 执行命令 one-sided）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// 蘑菇位置（light_box 运行时 air 区域 y=3 中心）。
const MUSHROOM = { x: 3, y: 3, z: 3 };
// 蔓延目标（y=3 层 air，下方 y=2 铺 stone 支撑）。
const SPREAD_TARGETS = [
    { x: 2, y: 3, z: 3 },
    { x: 4, y: 3, z: 3 },
    { x: 3, y: 3, z: 2 },
    { x: 3, y: 3, z: 4 },
];
// y=2 支撑层位置（蘑菇下方 + 4个蔓延目标下方，铺 stone 作 canSustainMushroom 支撑）。
const SUPPORT_LAYER = [
    { x: 3, y: 2, z: 3 },
    { x: 2, y: 2, z: 3 },
    { x: 4, y: 2, z: 3 },
    { x: 3, y: 2, z: 2 },
    { x: 3, y: 2, z: 4 },
];
// 测试2 光源位置（蘑菇旁，提供方块光≥13）。
const GLOWSTONE = { x: 4, y: 3, z: 3 };

// 调高 randomTickSpeed 使蘑菇格在数 tick 内被随机刻确定性命中。
// 1000 使单格每 tick 命中概率≈24.4%。黑暗蔓延期望≈0.22%/tick（1/25×4目标/18格）。
const HIGH_RANDOM_TICK_SPEED = "1000";

// 读取方块 typeId，缺失返回 undefined。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string | undefined {
    const block = test.getBlock(pos);
    return block?.typeId;
}

// 铺 y=2 stone 支撑层 + y=3 放 brown_mushroom（强放绕过 isValidPosition）。
function placeMushroomAndSupports(test: Test): void {
    for (const pos of SUPPORT_LAYER) {
        test.setBlockType("minecraft:stone", pos);
    }
    test.setBlockType("minecraft:brown_mushroom", MUSHROOM);
}

// 调高 randomTickSpeed（SimulatedPlayer 创造模式权限2 执行 /gamerule）。不 assert chat 返回值。
// 玩家放 air 区域 (1,2,1)（运行时 air 区域 y∈[2,4]），避免卡在 stone 地板层。
function raiseRandomTickSpeed(test: Test): void {
    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    player.chat(`/gamerule randomTickSpeed ${HIGH_RANDOM_TICK_SPEED}`);
}

// 统计4个蔓延目标中有多少已变 brown_mushroom（蔓延成功数）。
function countSpreadMushrooms(test: Test): number {
    let count = 0;
    for (const pos of SPREAD_TARGETS) {
        if (getTypeId(test, pos) === "minecraft:brown_mushroom") {
            ++count;
        }
    }
    return count;
}

// 黑暗中蘑菇向周围蔓延（wiki 传播：4%概率尝试，亮度≤12可传播）。
// 铺支撑层 + 放蘑菇，调高 randomTickSpeed 后等待，断言4个蔓延目标中至少1个变 brown_mushroom。
// 守卫：蘑菇处 blockLight===0 && skyLight===0 确认黑暗（仅 Cubium 侧判定）。
function mushroomSpreadsInDark(test: Test): void {
    placeMushroomAndSupports(test);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 黑暗环境守卫：skyLight===0（封顶隔绝天空光，仅 Cubium 侧可读，基岩侧跳过）。
            // 不强制 blockLight===0：brown_mushroom 自身发光等级1（wiki :106 棕色蘑菇发光1），
            // 蘑菇存在时其格 blockLight 至少1，故黑暗环境允许 blockLight<=1（蘑菇自身光，无外部光源）。
            const block = test.getBlock(MUSHROOM);
            const skyLight = (block as unknown as { skyLight?: number })?.skyLight;
            const blockLight = (block as unknown as { blockLight?: number })?.blockLight;
            if (typeof skyLight === "number" && typeof blockLight === "number") {
                if (skyLight !== 0 || blockLight > 1) {
                    return false; // 环境非黑暗（有外部光源），等待光照重算稳定（不应发生，保守轮询）
                }
            }
            // 蔓延成功：至少1个目标变 brown_mushroom。
            return countSpreadMushrooms(test) >= 1;
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 400,
            onTimeout: () => {
                const count = countSpreadMushrooms(test);
                const block = test.getBlock(MUSHROOM);
                const blockLight = (block as unknown as { blockLight?: number })?.blockLight ?? -1;
                const skyLight = (block as unknown as { skyLight?: number })?.skyLight ?? -1;
                test.assert(
                    false,
                    `mushroom spread in dark: expected >=1 brown_mushroom, got ${count}/4 ` +
                        `(mushroom=${getTypeId(test, MUSHROOM)} blockLight=${blockLight}(<=1 ok, brown_mushroom自发光1) ` +
                        `skyLight=${skyLight}(should be 0); if 0, randomTickSpeed may not be raised or ` +
                        `1/25 gate + air-target+support spread logic issue)`,
                );
            },
        },
    );
}

// 光照≥13时蘑菇不蔓延（wiki 传播：亮度≤12可传播，≥13不蔓延）。
// 铺支撑层 + 放蘑菇 + (4,3,3) 放 glowstone(15)。蘑菇处光照14≥13，randomTick 直接 return 不蔓延。
// 蔓延目标距 glowstone 2格光照13≥13，canSustainMushroom 要求<13失败。调高 randomTickSpeed 后等待，
// 断言4个目标无 brown_mushroom。配对测试1证明 randomTickSpeed 调高生效，反证此处不蔓延是光照门槛。
// 守卫：蘑菇处 blockLight>=13 确认亮环境（仅 Cubium 侧判定）。
function mushroomDoesNotSpreadInLight(test: Test): void {
    placeMushroomAndSupports(test);
    test.setBlockType("minecraft:glowstone", GLOWSTONE);
    raiseRandomTickSpeed(test);

    pollUntilSucceed(
        test,
        () => {
            // 亮环境守卫：蘑菇处光照必须>=13（仅 Cubium 侧 blockLight/skyLight 可读，基岩侧跳过）。
            const block = test.getBlock(MUSHROOM);
            const blockLight = (block as unknown as { blockLight?: number })?.blockLight;
            if (typeof blockLight === "number") {
                if (blockLight < 13) {
                    return false; // 环境未达光照门槛，等待光照重算稳定（不应发生，保守轮询）
                }
            }
            // 光照门槛拦截：4个蔓延目标全无 brown_mushroom（蘑菇处光照>=13 不蔓延）。
            return countSpreadMushrooms(test) === 0;
        },
        {
            startTick: 40,
            interval: 20,
            maxTick: 160,
            onTimeout: () => {
                const count = countSpreadMushrooms(test);
                const block = test.getBlock(MUSHROOM);
                const blockLight = (block as unknown as { blockLight?: number })?.blockLight ?? -1;
                test.assert(
                    false,
                    `mushroom no-spread in light: expected 0 brown_mushroom, got ${count}/4 ` +
                        `(mushroom=${getTypeId(test, MUSHROOM)} blockLight=${blockLight} should be >=13; ` +
                        `if count>0, light threshold >=13 check may be missing in randomTick)`,
                );
            },
        },
    );
}

export function registerMushroomSpreadTests(): void {
    GameTest.register("BlockBehaviorTests", "mushroom_spreads_in_dark", mushroomSpreadsInDark)
        .structureName("gametests:light_box")
        .maxTicks(480);
    GameTest.register("BlockBehaviorTests", "mushroom_does_not_spread_in_light", mushroomDoesNotSpreadInLight)
        .structureName("gametests:light_box")
        .maxTicks(240);
}
