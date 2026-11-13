/*
 * probes.ts — 探针执行。
 *
 * 探针是一条**只读**的状态查询指令（`data get`、`scoreboard players get`、`execute if`…），
 * 它的反馈就是判定值。相比在 steps 里堆一串查询，探针把「执行」与「观察」分开：
 * 报告里的差异路径是 `probes.<key>`，一眼能看出是哪项状态对不上。
 *
 * 首版保存归一化后的纯文本即可；需要结构化字段的解析（如提取 SNBT 主体）在用例确实
 * 需要时再加，避免先造一层没人用的抽象。
 */

import type { Bot } from "mineflayer";

import type { BoardOrigin, ProbeSpec } from "../case.ts";
import { substituteCoordinates } from "../case.ts";
import { runStep } from "../driver/chat-driver.ts";
import { normalizeFeedback } from "../compare/normalize.ts";

/** 一次探针的结果。 */
export interface ProbeOutcome {
    readonly key: string;
    /** 替换坐标后的指令原文。 */
    readonly command: string;
    /** 归一化后的反馈。 */
    readonly text: string;
}

/**
 * 依次执行探针。
 *
 * @param bot 已就绪的 bot。
 * @param probes 探针定义。
 * @param origin 本用例的棋盘原点。
 * @param timeoutMs 单条探针等待反馈的上限。
 * @returns 探针结果，顺序与输入一致。
 */
export async function runProbes(
    bot: Bot,
    probes: readonly ProbeSpec[],
    origin: BoardOrigin,
    timeoutMs: number,
): Promise<readonly ProbeOutcome[]> {
    const outcomes: ProbeOutcome[] = [];
    for (const probe of probes) {
        const command = substituteCoordinates(probe.command, origin);
        const step = await runStep(bot, command, timeoutMs);
        outcomes.push({ key: probe.key, command, text: normalizeFeedback(step.feedback) });
    }
    return outcomes;
}
