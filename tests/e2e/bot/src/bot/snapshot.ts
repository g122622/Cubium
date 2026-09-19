/*
 * snapshot.ts — 用例状态快照的采集与归一化。
 *
 * **归一化是快照能被跨服务端比对的前提。** 强制规则：
 *   1. 所有 Y 转为「相对地表」（surfaceY 本身只是 informational，不参与相等性比对）。
 *   2. 所有 X/Z 转为「相对出生点」偏移。
 *   3. 实体 id / UUID / 用户名 / 时间戳 / 耗时一律**不入快照**——耗时单列 metrics，只报告不比对。
 *   4. 数组字段在比对前排序（除非顺序本身是被测语义）。
 * 这些规则的依据是 docs/test/INTEGRATED_TEST.md 第 4 节记录的非确定性教训。
 */

import type { Bot } from "mineflayer";
import { biomeIdAt, biomeNameOfId, blockNameAt, isAir, layersAboveSurface, layersBelowSurface } from "../assert/surface.ts";

/** 快照中的信息性字段（记录但**不参与**比对）。 */
export interface InformationalFields {
    /** 绝对地表 Y：Cubium 为 3、vanilla 为 -61，必然不同，故豁免比对。 */
    readonly surfaceY: number;
}

/** 生命周期段。 */
export interface LifecycleSnapshot {
    readonly spawned: boolean;
    readonly kickReason: string | null;
    /** 是否收到过 login(cb 48)。 */
    readonly sawLoginPacket: boolean;
    /** 是否收到过 update_health（mineflayer 的 spawn 触发条件）。 */
    readonly sawUpdateHealthPacket: boolean;
}

/**
 * 世界段。
 *
 * minY / height 用**绝对值**：它们描述维度类型本身，两侧（同为 overworld）应当一致。
 * 不用「相对地表」换算——Cubium 的超平坦层从 Y=0 起、vanilla 从 minY=-64 起，
 * 但两者的 min_y/height 定义相同，相对地表换算反而会把这份一致性掩盖掉。
 */
export interface WorldSnapshot {
    readonly dimension: string;
    readonly minY: number;
    readonly height: number;
}

/**
 * 地形段（全部相对地表，故跨服务端可比）。
 *
 * 两侧的超平坦层配置一致（bedrock 1 / dirt 2 / grass_block 1），差别仅在于起始 Y，
 * 相对化后即为同一组值。
 */
export interface TerrainSnapshot {
    /** 自地表向下的方块名（含地表本身）。 */
    readonly layersBelowSurface: readonly string[];
    /** 地表上方 2 格（应为空气）。 */
    readonly layersAboveSurface: readonly string[];
    /**
     * 地表方块的生物群系 id（跨端可比；plains 在 vanilla registry 中恒为 40）。
     * 记录 id 而非名字：prismarine 的 block.biome 是只带 id 的占位对象，名字须经 registry 反查。
     */
    readonly biomeIdAtSurface: number;
    /** 经 registry 反查得到的生物群系名（无命名空间前缀）。 */
    readonly biomeNameAtSurface: string;
}

/** 玩家段（全部相对化；不含 id/uuid/用户名）。 */
export interface PlayerSnapshot {
    readonly gameMode: string;
    readonly health: number;
    readonly food: number;
    /** 脚位置 Y 相对地表（正常应约 1）。 */
    readonly yRelativeToSurface: number;
    readonly onGround: boolean;
    readonly standingOn: string | null;
}

/** 一份用例快照。 */
export interface CaseSnapshot {
    readonly informational: InformationalFields;
    readonly lifecycle: LifecycleSnapshot;
    readonly world: WorldSnapshot;
    readonly terrain: TerrainSnapshot;
    readonly player: PlayerSnapshot;
    readonly extra: Record<string, unknown>;
}

/** 采集通用状态所需的外部输入。 */
export interface CaptureInput {
    readonly bot: Bot;
    readonly surfaceY: number;
    readonly spawned: boolean;
    readonly kickReason: string | null;
    readonly sawLoginPacket: boolean;
    readonly sawUpdateHealthPacket: boolean;
    /** 用例自有的附加字段（须已归一化，不得含绝对 Y/X/Z 或 id）。 */
    readonly extra: Record<string, unknown>;
}

/**
 * 把浮点量化到 2 位小数。
 *
 * 玩家位置由客户端物理积分得出，同一稳定状态下也可能出现 4.0 与 3.9999998 这类
 * 末位差异；直接入快照会造成为基线抖动。量化到 0.01 足以表达"站在地表上方约 1 格"
 * 这类语义，同时消除浮点噪声。
 */
function quantize(value: number): number {
    return Math.round(value * 100) / 100;
}

/** 采集一份快照。 */
export function captureSnapshot(input: CaptureInput): CaseSnapshot {
    const { bot } = input;
    const position = bot.entity?.position;
    const feetY = position === undefined ? 0 : position.y;
    const blockX = Math.floor(position?.x ?? 0);
    const blockZ = Math.floor(position?.z ?? 0);
    const below = blockNameAt(bot, blockX, Math.floor(feetY) - 1, blockZ);

    const game = bot.game as unknown as Record<string, unknown>;
    const minY = typeof game.minY === "number" ? game.minY : 0;
    const height = typeof game.height === "number" ? game.height : 0;

    return {
        informational: { surfaceY: input.surfaceY },
        lifecycle: {
            spawned: input.spawned,
            kickReason: input.kickReason,
            sawLoginPacket: input.sawLoginPacket,
            sawUpdateHealthPacket: input.sawUpdateHealthPacket,
        },
        world: {
            dimension: String(game.dimension ?? "<未知>"),
            minY,
            height,
        },
        terrain: {
            layersBelowSurface: layersBelowSurface(bot, blockX, blockZ, input.surfaceY, 4),
            layersAboveSurface: layersAboveSurface(bot, blockX, blockZ, input.surfaceY, 2),
            biomeIdAtSurface: biomeIdAt(bot, blockX, input.surfaceY, blockZ) ?? -1,
            biomeNameAtSurface: biomeNameOfId(bot, biomeIdAt(bot, blockX, input.surfaceY, blockZ) ?? -1) ?? "<未知>",
        },
        player: {
            gameMode: String(game.gameMode ?? "<未知>"),
            health: bot.health ?? -1,
            food: bot.food ?? -1,
            yRelativeToSurface: quantize(feetY - input.surfaceY),
            onGround: bot.entity?.onGround ?? false,
            standingOn: isAir(below) ? null : (below ?? null),
        },
        extra: input.extra,
    };
}
