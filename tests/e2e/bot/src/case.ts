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
    /** 本用例声明的全部 bot，按连接顺序排列（数量由 CaseDefinition.botCount 决定）。 */
    readonly bots: readonly Bot[];
    /** 与 bots 同序的包记录。 */
    readonly traces: readonly PacketTrace[];
    /**
     * 主 bot（即 bots[0]）。
     *
     * 它是地表基准的来源——surfaceY/spawnX/spawnZ 均由它的出生位置探测得到，
     * 故单 bot 用例直接用主 bot 即可。多 bot 用例若要观测「对方」的行为，从 bots 里取。
     */
    readonly bot: Bot;
    /** 主 bot 的包记录（即 traces[0]）。 */
    readonly trace: PacketTrace;
    /**
     * 追加连接一个 bot 并等待其就绪（spawn + 稳定落地）。
     *
     * 用于需要控制连接时序的用例：runner 已按 botCount 预先连好若干 bot，本方法在用例执行
     * 期间追加连接——典型场景是「先让已连接的 bot 改变世界状态，再让新 bot 加入观测它」。
     * 追加的 bot 同样计入 bots / traces，并由 runner 统一回收。
     */
    readonly connectBot: () => Promise<Bot>;
    readonly serverKind: ServerKind;
    /** 探测得到的地表 Y（Cubium=3，vanilla=-61）。 */
    readonly surfaceY: number;
    /** 主 bot 出生点所在方块的 X/Z。 */
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
     * runner 在用例开始前**预先连接并等待就绪**的 bot 数量（≥1）。所有用例都必须显式声明，
     * 不设默认值。
     *
     * 需要控制连接时序的用例（例如「先让已连接的 bot 制造实体使 id 序列错位，再让新 bot 加入」）
     * 应把此值设为较小值，并在 run 中调用 ctx.connectBot() 追加连接。
     *
     * 多 bot 用例共用同一个服务端进程：它们共享物理世界（会互相推挤）与方块状态，
     * 设计时须让各 bot 在空间上错开，且不得依赖「自己是世界里唯一的行动者」。
     */
    readonly botCount: number;
    /**
     * 执行用例。
     *
     * @returns 附加到快照 extra 段的字段。**必须已归一化**：不得含绝对 Y/X/Z、
     *          实体 id、UUID、时间戳或耗时（耗时应走 metrics，但当前版本尚未单列）。
     */
    readonly run: (ctx: CaseContext) => Promise<Record<string, unknown>>;
}
