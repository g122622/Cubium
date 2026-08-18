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
//   4. 放荧石邻格亮14 → 替换为更弱光源火把 → 邻格降到13（光源降级，减亮回收旧强光后按新弱光重算）。
//
// 关键时序：每步方块变更后都需等待光照重算稳定，再断言，再进行下一步。光照重算异步且耗时
// 随场景规模变化，固定 tick 等待易 flaky（早断言读到旧值/晚断言浪费）。改用 waitForCondition
// 轮询：每阶段等到目标光照值出现再进入下一阶段（onReady 触发下一步方块变更或 succeed）。
//
// 跨服务端：blockLight/skyLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#方块光照（光源移除后光消失）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { waitForCondition } from "../../utils/test/poll.js";
import { getBlockLight, getSkyLight } from "../utils/lightAssert.js";

// 移除光源后方块光消失：放荧石于 (3,2,3)，邻格 (4,2,3) 亮 14；移除荧石(设air)，邻格变 0。
// 坐标用 helper y=2 = 结构内 y=1 air 层（光源放地板正上方 air，邻格也是 air），避免光源嵌在
// stone 地板层（y=1=结构内 y=0）致邻格 stone opacity=15 阻断传播（非引擎 bug，是坐标误用）。
//
// 两阶段轮询：阶段1 等"邻格亮14"→onReady 移除荧石并启动阶段2；阶段2 等"邻格变0"→onReady succeed。
function removingSourceKillsBlockLight(test: Test): void {
    const src = { x: 3, y: 2, z: 3 };
    const neighbor = { x: 4, y: 2, z: 3 };

    // t=1：放荧石。
    test.runAtTickTime(1, () => {
        test.setBlockType("minecraft:glowstone", src);
    });

    // 阶段1：等邻格亮 14（光源传播完成）→ 移除荧石，启动阶段2。
    waitForCondition(
        test,
        () => getBlockLight(test, neighbor.x, neighbor.y, neighbor.z) === 14,
        () => {
            test.setBlockType("minecraft:air", src);
            // 阶段2：等邻格变 0（光源消失，方块光回收）→ succeed。
            waitForCondition(
                test,
                () => getBlockLight(test, neighbor.x, neighbor.y, neighbor.z) === 0,
                () => {
                    test.succeed();
                },
                {
                    startTick: 4,
                    interval: 4,
                    maxTick: 70,
                    onTimeout: () => {
                        test.assert(
                            false,
                            `after removing glowstone, neighbor blockLight=${getBlockLight(
                                test,
                                neighbor.x,
                                neighbor.y,
                                neighbor.z,
                            )} expected 0 (light should recede)`,
                        );
                    },
                },
            );
        },
        {
            startTick: 4,
            interval: 4,
            maxTick: 70,
            onTimeout: () => {
                test.assert(
                    false,
                    `after placing glowstone, neighbor blockLight=${getBlockLight(
                        test,
                        neighbor.x,
                        neighbor.y,
                        neighbor.z,
                    )} expected 14`,
                );
            },
        },
    );
}

// 移除遮挡后天空光恢复：grass_pen 露天，放石头挡 (4,4,3) 列 → (4,3,3) 不再满亮 → 移除石头 → 恢复 15。
// 放石头后 probe 由水平露天列侧传得 14（非 0，见 SkyLightTests 注释的侧传语义）；移除石头后该列
// 重新成为露天列，垂直天空光直达，probe 恢复 15。验证天空光遮挡/恢复的增量重算链路。
function removingOccluderRestoresSkyLight(test: Test): void {
    const occluder = { x: 4, y: 4, z: 3 };
    const probe = { x: 4, y: 3, z: 3 };

    // t=1：放石头遮挡露天列。
    test.runAtTickTime(1, () => {
        test.setBlockType("minecraft:stone", occluder);
    });

    // 阶段1：等 probe 被遮挡后侧传得 14 → 移除石头恢复露天，启动阶段2。
    waitForCondition(
        test,
        () => getSkyLight(test, probe.x, probe.y, probe.z) === 14,
        () => {
            test.setBlockType("minecraft:air", occluder);
            // 阶段2：等 probe 恢复 skyLight=15（遮挡移除，天空光重亮）→ succeed。
            waitForCondition(
                test,
                () => getSkyLight(test, probe.x, probe.y, probe.z) === 15,
                () => {
                    test.succeed();
                },
                {
                    startTick: 4,
                    interval: 4,
                    maxTick: 70,
                    onTimeout: () => {
                        test.assert(
                            false,
                            `after removing stone occluder, probe skyLight=${getSkyLight(
                                test,
                                probe.x,
                                probe.y,
                                probe.z,
                            )} expected 15 (sky light should restore)`,
                        );
                    },
                },
            );
        },
        {
            startTick: 4,
            interval: 4,
            maxTick: 70,
            onTimeout: () => {
                test.assert(
                    false,
                    `after placing stone occluder, probe skyLight=${getSkyLight(
                        test,
                        probe.x,
                        probe.y,
                        probe.z,
                    )} expected 14 (side-propagated, not 0)`,
                );
            },
        },
    );
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

    // 阶段1：等邻格 13（火把14减1）→ 替换为荧石(15)，启动阶段2。
    waitForCondition(
        test,
        () => getBlockLight(test, neighbor.x, neighbor.y, neighbor.z) === 13,
        () => {
            test.setBlockType("minecraft:glowstone", src);
            // 阶段2：等邻格升到 14（荧石15减1，比火把的13更亮，重算取 max）→ succeed。
            waitForCondition(
                test,
                () => getBlockLight(test, neighbor.x, neighbor.y, neighbor.z) === 14,
                () => {
                    test.succeed();
                },
                {
                    startTick: 4,
                    interval: 4,
                    maxTick: 70,
                    onTimeout: () => {
                        test.assert(
                            false,
                            `after upgrading to glowstone, neighbor blockLight=${getBlockLight(
                                test,
                                neighbor.x,
                                neighbor.y,
                                neighbor.z,
                            )} expected 14 (relight takes max)`,
                        );
                    },
                },
            );
        },
        {
            startTick: 4,
            interval: 4,
            maxTick: 70,
            onTimeout: () => {
                test.assert(
                    false,
                    `after placing torch, neighbor blockLight=${getBlockLight(
                        test,
                        neighbor.x,
                        neighbor.y,
                        neighbor.z,
                    )} expected 13`,
                );
            },
        },
    );
}

// 光源降级重算（减亮队列回收 + 弱光重传播）：放荧石(15) 邻格=14 → 替换为火把(14) → 邻格降到 13。
// 与 upgradingSourceRelights（弱→强，邻格升）对称，此处验证强→弱：减亮队列回收荧石 15 光后，
// 火把 14 重新传播得邻格 13（14-1）。验证光照引擎在光源降级时正确回收旧强光并按新弱光源重算
// （非残留旧 14 值）。若减亮逻辑未回收，邻格会残留 14（旧荧石光），断言 13 即可捕获。
// 坐标用 helper y=2 = 结构内 y=1 air 层：光源放地板正上方 air，邻格也是 air，正常传播。
function downgradingSourceDimsBlockLight(test: Test): void {
    const src = { x: 3, y: 2, z: 3 };
    const neighbor = { x: 4, y: 2, z: 3 };

    // t=1：放荧石(15)。
    test.runAtTickTime(1, () => {
        test.setBlockType("minecraft:glowstone", src);
    });

    // 阶段1：等邻格亮 14（荧石15减1）→ 替换为火把(14)，启动阶段2。
    waitForCondition(
        test,
        () => getBlockLight(test, neighbor.x, neighbor.y, neighbor.z) === 14,
        () => {
            test.setBlockType("minecraft:torch", src);
            // 阶段2：等邻格降到 13（火把14减1，减亮回收旧荧石15光后按火把重算）→ succeed。
            waitForCondition(
                test,
                () => getBlockLight(test, neighbor.x, neighbor.y, neighbor.z) === 13,
                () => {
                    test.succeed();
                },
                {
                    startTick: 4,
                    interval: 4,
                    maxTick: 70,
                    onTimeout: () => {
                        test.assert(
                            false,
                            `after downgrading to torch, neighbor blockLight=${getBlockLight(
                                test,
                                neighbor.x,
                                neighbor.y,
                                neighbor.z,
                            )} expected 13 (dim relight after glowstone removed)`,
                        );
                    },
                },
            );
        },
        {
            startTick: 4,
            interval: 4,
            maxTick: 70,
            onTimeout: () => {
                test.assert(
                    false,
                    `after placing glowstone, neighbor blockLight=${getBlockLight(
                        test,
                        neighbor.x,
                        neighbor.y,
                        neighbor.z,
                    )} expected 14`,
                );
            },
        },
    );
}

export function registerBlockChangeRelightTests(): void {
    GameTest.register("LightingTests", "light_removing_source_kills_block_light", removingSourceKillsBlockLight)
        .structureName("gametests:light_box")
        .maxTicks(150);
    GameTest.register("LightingTests", "light_removing_occluder_restores_sky_light", removingOccluderRestoresSkyLight)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
    GameTest.register("LightingTests", "light_upgrading_source_relights", upgradingSourceRelights)
        .structureName("gametests:light_box")
        .maxTicks(150);
    GameTest.register("LightingTests", "light_downgrading_source_dims_block_light", downgradingSourceDimsBlockLight)
        .structureName("gametests:light_box")
        .maxTicks(150);
}
