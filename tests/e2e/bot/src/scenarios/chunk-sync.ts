/*
 * chunk-sync.ts — 场景 B：区块同步与世界读取。
 *
 * 验证「服务端发出的区块字节被客户端正确解码成预期的世界」。
 * 这一层同时钉死两个此前从未被自动验证过的对齐点：
 *   1. 出站 block state id 必须与 minecraft-data 的 1.21.11 定义一致
 *      （边界经 JavaBlockStateIdMap 翻译，任何偏移都会让客户端解出错误的方块且不报错）。
 *   2. 生物群系 palette 的 registry id 顺序必须与客户端侧一致
 *      （Cubium 硬编码 66 个 biome 按字母序，从未与 minecraft-data 比对过）。
 */

import type { CaseDefinition } from "../case.ts";
import { expectDeepEq, expectEq, expectTrue } from "../assert/expect.ts";
import { biomeIdAt, biomeNameOfId, blockNameAt, layersBelowSurface } from "../assert/surface.ts";

/** 超平坦默认层（bedrock 1 / dirt 2 / grass_block 1），两侧应当一致。 */
const EXPECTED_LAYERS = ["grass_block", "dirt", "dirt", "bedrock"];

export const chunkSyncCases: readonly CaseDefinition[] = [
    {
        id: "chunk-sync/chunks_loaded",
        title: "spawn 后出生点周围区块可读",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 出生点所在列必须可读——这是后续所有用例的前提。
            const surface = blockNameAt(bot, spawnX, surfaceY, spawnZ);
            expectTrue(surface !== undefined, `出生点列 (${spawnX},${spawnZ}) 未加载区块`);
            expectTrue(surface !== "air", `出生点地表不应为空气，实际 ${String(surface)}`);

            // 3x3 邻域内至少要有 8 列可读（允许边缘列因视距边界未到）。
            let readable = 0;
            let total = 0;
            for (let dx = -1; dx <= 1; dx += 1) {
                for (let dz = -1; dz <= 1; dz += 1) {
                    total += 1;
                    if (blockNameAt(bot, spawnX + dx, surfaceY, spawnZ + dz) !== undefined) {
                        readable += 1;
                    }
                }
            }
            expectTrue(
                readable >= 8,
                `出生点 3x3 邻域仅 ${readable}/${total} 列可读，区块同步范围不足`,
            );
            return { spawnColumnReadable: true, neighborhoodReadable: readable >= 8 };
        },
    },

    {
        id: "chunk-sync/flat_layers",
        title: "出生列的超平坦分层与配置一致",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 相对地表取样，故两侧可比（Cubium 地表在 Y=3、vanilla 在 Y=-61）。
            const layers = layersBelowSurface(bot, spawnX, spawnZ, surfaceY, 4);
            expectDeepEq(layers, EXPECTED_LAYERS, `出生列 (${spawnX},${spawnZ}) 自地表向下的分层不符`);

            const above = blockNameAt(bot, spawnX, surfaceY + 1, spawnZ);
            expectEq(above, "air", `地表上方一格应为空气`);
            return {
                layersMatch: true,
                aboveIsAir: true,
            };
        },
    },

    {
        id: "chunk-sync/layers_across_chunk_boundary",
        title: "跨区块边界的地形分层一致（生成无边界伪影）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 取出生列所在区块的两侧边界：x 从 chunkX*16-1 到 chunkX*16，跨越区块接缝。
            const chunkBoundaryX = Math.floor(spawnX / 16) * 16;
            const left = layersBelowSurface(bot, chunkBoundaryX - 1, spawnZ, surfaceY, 4);
            const right = layersBelowSurface(bot, chunkBoundaryX, spawnZ, surfaceY, 4);
            expectDeepEq(left, EXPECTED_LAYERS, `区块边界左侧列 (${chunkBoundaryX - 1},${spawnZ}) 分层不符`);
            expectDeepEq(right, EXPECTED_LAYERS, `区块边界右侧列 (${chunkBoundaryX},${spawnZ}) 分层不符`);
            return { boundaryLayersMatch: true };
        },
    },

    {
        id: "chunk-sync/biome_at_surface",
        title: "地表生物群系可被客户端正确解析（biome registry id 顺序对齐）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 超平坦预设的 biome 固定为 plains。若服务端的 biome registry id 顺序与客户端
            // 不一致，palette 会解出别的 id 且不报错——本用例是那个静默错位的唯一哨兵。
            // 锚点取 **id** 而非 name：prismarine-chunk 的 Block.fromStateId 只携带 biome id、
            // 不注入数据，故 block.biome.name 恒为空串（名字须经 registry 反查）。
            const biomeId = biomeIdAt(bot, spawnX, surfaceY, spawnZ);
            expectTrue(biomeId !== undefined, "地表方块的 biome id 不可读（biome palette 未解析）");
            expectEq(biomeId, 40, "超平坦预设的地表生物群系应为 plains（vanilla registry 中的下标 40）");
            const name = biomeNameOfId(bot, 40);
            expectEq(name, "plains", "客户端 registry 中 id 40 的 biome 名应为 plains（无命名空间前缀）");
            return { biomeIdAtSurface: 40, biomeNameAtSurface: name };
        },
    },

    {
        id: "chunk-sync/dimension_type",
        title: "端点维度类型正确（min_y/height 与 overworld 一致）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot }): Promise<Record<string, unknown>> {
            // 该用例同时是 dimension_type 内联 NBT 的回归保护：客户端取不到该注册表时会
            // **静默退回 minY=0/height=256**（mineflayer game.js:70-77 的 fallback），
            // 而区块 section 是按真实 minY=-64 发送的，退路会导致所有方块位置错位。
            const game = bot.game as unknown as { minY?: number; height?: number; dimension?: string };
            expectEq(game.minY, -64, "维度 min_y 应为 -64（退回默认 0 说明 dimension_type 未送达）");
            expectEq(game.height, 384, "维度 height 应为 384（退回默认 256 说明 dimension_type 未送达）");
            expectEq(game.dimension, "overworld", "当前维度应为 overworld");
            return { minY: -64, height: 384, dimension: "overworld" };
        },
    },

    {
        id: "chunk-sync/spawn_not_in_void",
        title: "出生点落在地表而非虚空",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, surfaceY, trace }): Promise<Record<string, unknown>> {
            // 防的是服务端出生点兜底落在 SEA_LEVEL+1=64（在超平坦世界里那是空气）导致 bot 坠落。
            const feetY = bot.entity.position.y;
            const relative = feetY - surfaceY;
            expectTrue(
                relative >= 0 && relative <= 2,
                `出生点应紧贴地表上方（相对地表 ${relative}，期望 0~2）——偏离说明出生点兜底位置错误`,
            );
            expectEq(
                blockNameAt(bot, Math.floor(bot.entity.position.x), surfaceY, Math.floor(bot.entity.position.z)),
                "grass_block",
                "出生点脚下应为地表草方块",
            );
            return {
                spawnNearSurface: true,
                spawnOnGrass: true,
                onGroundEventually: true,
                receivedChunks: trace.count("map_chunk") > 0,
            };
        },
    },

    {
        id: "chunk-sync/block_names_resolve",
        title: "区块 palette 解码出的方块名可识别（出站 state id 对齐）",
        servers: ["cubium", "vanilla"],
        botCount: 1,
        async run({ bot, surfaceY, spawnX, spawnZ }): Promise<Record<string, unknown>> {
            // 出站方块状态经 JavaBlockStateIdMap 翻译成 vanilla 全局 state id；若翻译表有任何
            // 偏移，客户端会解出 name 为 undefined 或完全不同的方块。逐层核对已知方块名。
            const unknown: string[] = [];
            for (let dx = -2; dx <= 2; dx += 1) {
                for (let dz = -2; dz <= 2; dz += 1) {
                    for (let dy = 0; dy <= 3; dy += 1) {
                        const name = blockNameAt(bot, spawnX + dx, surfaceY - dy, spawnZ + dz);
                        if (name === undefined || name.length === 0) {
                            unknown.push(`(${spawnX + dx},${surfaceY - dy},${spawnZ + dz})`);
                        }
                    }
                }
            }
            expectTrue(unknown.length === 0, `有 ${unknown.length} 个方块无法解析名称：${unknown.slice(0, 5).join(" ")}`);
            return { allSampledBlocksResolved: true };
        },
    },
];
