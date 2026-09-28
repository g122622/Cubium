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
    /**
     * 关闭当前服务端并用**同一个游戏目录**重新启动它（重启后端口可能变化）。
     *
     * 用途：验证存档的持久化——改完世界（先用命令落盘，见 opPlayers）→ 重启 → 重连 →
     * 确认改动仍在。重启会断开当前所有 bot（连接随进程一起消失），因此重启后必须重新
     * `connectBot()`；重启后**第一个**连接的 bot 会成为新的主 bot（决定 bot/trace 与最终快照）。
     */
    readonly restartServer: () => Promise<void>;
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
     * 本用例是否需要让 bot 拥有 OP 权限（命令权限）。
     *
     * 为 true 时 runner 会在**启动服务端之前**把本用例可能连接的 bot 名写进 ops.json
     * （服务端只在启动时读取该文件）。需要服务端执行命令的用例必须声明本字段——
     * 否则命令会被静默忽略（权限不足只回一条聊天消息，用例会以超时告终）。
     */
    readonly opPlayers: boolean;
    /**
     * 跳过原因；`null` 表示正常执行。
     *
     * 非 null 时 runner **不运行**该用例、不写基线，并在运行时打印原因与解除条件。
     * 这个字段只允许用于「已经写好、但被服务端缺陷阻塞、当前不可能通过」的用例：
     * 它把阻塞点变成注册表里可见的一条记录（而不是让整套测试长期红灯），
     * 修好服务端缺陷后把这里改回 null 即可启用。**原因里必须写明具体缺陷与证据来源。**
     */
    readonly skipReason: string | null;
    /**
     * 执行用例。
     *
     * @returns 附加到快照 extra 段的字段。**必须已归一化**：不得含绝对 Y/X/Z、
     *          实体 id、UUID、时间戳或耗时（耗时应走 metrics，但当前版本尚未单列）。
     */
    readonly run: (ctx: CaseContext) => Promise<Record<string, unknown>>;
}
