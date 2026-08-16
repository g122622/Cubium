// 方块变更触发光照重算测试：验证光照引擎对方块变更的增量更新（checkBlock 增亮/减亮队列）。
//
// 光照引擎在方块变更时入队 m_lightQueue，由 ServerWorld::tick 批量重算：
//   - 新增光源 → 增亮队列传播（BlockLightEngine::checkBlock 入 increase）
//   - 移除光源 → 减亮队列回收（入 decrease）
//   - 新增/移除遮挡 → 天空光列重新传播
//
// 验证点：
//   1. 放荧石邻格亮14 → 移除荧石(设air) → 邻格变0（光源消失，方块光回收）。
//   2. 放石头挡露天列 → 下方暗 → 移除石头 → 下方恢复15（遮挡移除，天空光重亮）。
//   3. 放火把邻格亮13 → 替换为更强光源荧石 → 邻格升到14（光源升级，重算取 max）。
//
// 关键时序：每步方块变更后都需等待光照重算稳定，再断言，再进行下一步。用 runAtTickTime
// 分阶段执行：t1 放方块 → t2 断言亮 → t3 移除 → t4 断言暗。阶段间留足 tick 让光照传播。
//
// 跨服务端：blockLight/skyLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#方块光照（光源移除后光消失）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { getBlockLight, getSkyLight } from "../utils/lightAssert.js";

// 阶段间光照重算等待 tick。方块变更入队后需数 tick 传播，10 tick 足够单光源小范围稳定。
const RELIGHT_TICKS = 12;

// 移除光源后方块光消失：放荧石于 (3,2,3)，邻格 (4,2,3) 亮 14；移除荧石(设air)，邻格变 0。
// 坐标用 helper y=2 = 结构内 y=1 air 层（光源放地板正上方 air，邻格也是 air），避免光源嵌在
// stone 地板层（y=1=结构内 y=0）致邻格 stone opacity=15 阻断传播（非引擎 bug，是坐标误用）。
function removingSourceKillsBlockLight(test: Test): void {
    const src = { x: 3, y: 2, z: 3 };
    const neighbor = { x: 4, y: 2, z: 3 };

    // t=1：放荧石。
    test.runAtTickTime(1, () => {
        test.setBlockType("minecraft:glowstone", src);
    });
    // t=1+RELIGHT：断言邻格亮 14（光源已传播）。
    test.runAtTickTime(1 + RELIGHT_TICKS, () => {
        test.assert(
            getBlockLight(test, neighbor.x, neighbor.y, neighbor.z) === 14,
            `after placing glowstone, neighbor blockLight=${getBlockLight(
                test,
                neighbor.x,
                neighbor.y,
                neighbor.z,
            )} expected 14`,
        );
        // 移除荧石（设 air）。
        test.setBlockType("minecraft:air", src);
    });
    // t=1+2*RELIGHT：断言邻格变 0（光源消失，方块光回收）。
    test.runAtTickTime(1 + 2 * RELIGHT_TICKS, () => {
        test.assert(
            getBlockLight(test, neighbor.x, neighbor.y, neighbor.z) === 0,
            `after removing glowstone, neighbor blockLight=${getBlockLight(
                test,
                neighbor.x,
                neighbor.y,
                neighbor.z,
            )} expected 0 (light should recede)`,
        );
        test.succeed();
    });
}

// 移除遮挡后天空光恢复：grass_pen 露天，放石头挡 (4,4,3) 列 → (4,3,3) 暗 → 移除石头 → 恢复 15。
function removingOccluderRestoresSkyLight(test: Test): void {
    const occluder = { x: 4, y: 4, z: 3 };
    const probe = { x: 4, y: 3, z: 3 };

    // t=1：放石头遮挡露天列。
    test.runAtTickTime(1, () => {
        test.setBlockType("minecraft:stone", occluder);
    });
    // t=1+RELIGHT：断言 probe 被遮挡 skyLight=0。
    test.runAtTickTime(1 + RELIGHT_TICKS, () => {
        test.assert(
            getSkyLight(test, probe.x, probe.y, probe.z) === 0,
            `after placing stone occluder, probe skyLight=${getSkyLight(
                test,
                probe.x,
                probe.y,
                probe.z,
            )} expected 0`,
        );
        // 移除石头（设 air）恢复露天。
        test.setBlockType("minecraft:air", occluder);
    });
    // t=1+2*RELIGHT：断言 probe 恢复 skyLight=15（遮挡移除，天空光重亮）。
    test.runAtTickTime(1 + 2 * RELIGHT_TICKS, () => {
        test.assert(
            getSkyLight(test, probe.x, probe.y, probe.z) === 15,
            `after removing stone occluder, probe skyLight=${getSkyLight(
                test,
                probe.x,
                probe.y,
                probe.z,
            )} expected 15 (sky light should restore)`,
        );
        test.succeed();
    });
}

// 光源升级重算取 max：放火把(14) 邻格 13 → 替换为荧石(15) → 邻格升到 14。
// 坐标用 helper y=2 = 结构内 y=1 air 层：火把 standing 放地板正上方 air，下方 stone 地板支撑；
// 邻格 (4,2,3) 也是 air，可正常传播。避免 y=1（结构内 y=0 stone 地板层）致邻格 stone 阻光。
function upgradingSourceRelights(test: Test): void {
    const src = { x: 3, y: 2, z: 3 };
    const neighbor = { x: 4, y: 2, z: 3 };

    // t=1：放火把(14)。
    test.runAtTickTime(1, () => {
        test.setBlockType("minecraft:torch", src);
    });
    // t=1+RELIGHT：断言邻格 13（火把14减1）。
    test.runAtTickTime(1 + RELIGHT_TICKS, () => {
        test.assert(
            getBlockLight(test, neighbor.x, neighbor.y, neighbor.z) === 13,
            `after placing torch, neighbor blockLight=${getBlockLight(
                test,
                neighbor.x,
                neighbor.y,
                neighbor.z,
            )} expected 13`,
        );
        // 替换为荧石(15)（覆盖火把）。
        test.setBlockType("minecraft:glowstone", src);
    });
    // t=1+2*RELIGHT：断言邻格升到 14（荧石15减1，比火把的13更亮，重算取 max）。
    test.runAtTickTime(1 + 2 * RELIGHT_TICKS, () => {
        test.assert(
            getBlockLight(test, neighbor.x, neighbor.y, neighbor.z) === 14,
            `after upgrading to glowstone, neighbor blockLight=${getBlockLight(
                test,
                neighbor.x,
                neighbor.y,
                neighbor.z,
            )} expected 14 (relight takes max)`,
        );
        test.succeed();
    });
}

export function registerBlockChangeRelightTests(): void {
    GameTest.register("LightingTests", "light_removing_source_kills_block_light", removingSourceKillsBlockLight)
        .structureName("gametests:light_box")
        .maxTicks(80);
    GameTest.register("LightingTests", "light_removing_occluder_restores_sky_light", removingOccluderRestoresSkyLight)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(80);
    GameTest.register("LightingTests", "light_upgrading_source_relights", upgradingSourceRelights)
        .structureName("gametests:light_box")
        .maxTicks(80);
}
