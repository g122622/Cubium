/*
 * artifacts.ts — 失败时的诊断产物落盘。
 *
 * 诊断质量决定调试效率。其中最有价值的是 bot-trace.jsonl：它是「某个包到底发没发」
 * 的唯一直接证据——例如 mineflayer 的 spawn 依赖首个 update_health，失败时能直接从
 * 记录里看出计数为 0，而不必靠猜。
 *
 * 默认仅在失败时保留；成功用例的工作目录直接删除。
 */

import { persistArtifacts } from "../servers/workspace.ts";

/** 一次失败用例的完整诊断包。 */
export interface ArtifactBundle {
    /** 用例 id。 */
    readonly caseId: string;
    /** 用例标题。 */
    readonly title: string;
    /** 被测服务端种类。 */
    readonly serverKind: string;
    /** 失败分类（断言失败 / bot 故障 / 超时 / 服务端崩溃 / 其他）。 */
    readonly failureKind: string;
    /** 失败消息。 */
    readonly failureMessage: string;
    /** 服务端完整日志。 */
    readonly serverLog: string;
    /** 原始包记录（JSONL）。 */
    readonly botTrace: string;
    /** 包计数摘要。 */
    readonly packetSummary: string;
    /** 实际快照（JSON）。 */
    readonly actualSnapshot: string;
    /** 基线快照（JSON；无基线时为空串）。 */
    readonly baselineSnapshot: string;
    /** 字段差异文本（无基线或全等时为空串）。 */
    readonly differences: string;
    /** 运行元信息（端口、耗时、退出码等）。 */
    readonly meta: string;
}

/**
 * 把诊断包写入目录。
 *
 * @param artifactDir 目标目录。
 * @param bundle 诊断内容。
 */
export function writeArtifacts(artifactDir: string, bundle: ArtifactBundle): void {
    const files = new Map<string, string>([
        ["server.log", bundle.serverLog],
        ["bot-trace.jsonl", bundle.botTrace],
        ["packet-summary.txt", bundle.packetSummary],
        ["snapshot.actual.json", bundle.actualSnapshot],
        ["snapshot.baseline.json", bundle.baselineSnapshot],
        ["diff.txt", bundle.differences],
        ["meta.json", bundle.meta],
    ]);
    persistArtifacts(artifactDir, files);
}
