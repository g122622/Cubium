/*
 * case.ts — 用例契约。
 *
 * 一条用例是一个纯函数：拿到已就绪的 bot 与地表基准，做操作、断言，返回可序列化的附加字段。
 * 起服务端、连 bot、探测地表、采快照、比基线全部由 runner 负责——用例只写游戏语义。
 *
 * servers 字段标明该用例适用于哪一侧：绝大多数用例两侧都跑（用于双跑对比），
 * 少数只对 Cubium 成立（如验证某个 Cubium 专有缺陷已修复）。
 */

import type { Bot } from "mineflayer";
import type { PacketTrace } from "./bot/factory.ts";

/** 被测服务端种类。 */
export type ServerKind = "cubium" | "vanilla";

/** 用例运行上下文（由 runner 构造）。 */
export interface CaseContext {
    readonly bot: Bot;
    readonly trace: PacketTrace;
    readonly serverKind: ServerKind;
    /** 探测得到的地表 Y（Cubium=3，vanilla=-61）。 */
    readonly surfaceY: number;
    /** 出生点所在方块的 X/Z。 */
    readonly spawnX: number;
    readonly spawnZ: number;
    /** 本用例的工作目录（可用于落盘临时文件）。 */
    readonly runDir: string;
}

/** 一条用例。 */
export interface CaseDefinition {
    /** 用例 id，形如 "handshake/spawn_event"。同时作为基线条目键与运行目录名。 */
    readonly id: string;
    /** 人类可读标题。 */
    readonly title: string;
    /** 适用于哪些服务端。 */
    readonly servers: readonly ServerKind[];
    /**
     * 执行用例。
     *
     * @returns 附加到快照 extra 段的字段。**必须已归一化**：不得含绝对 Y/X/Z、
     *          实体 id、UUID、时间戳或耗时（耗时应走 metrics，但当前版本尚未单列）。
     */
    readonly run: (ctx: CaseContext) => Promise<Record<string, unknown>>;
}
