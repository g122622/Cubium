/*
 * chat-driver.ts — 逐条聊天指令驱动。
 *
 * 每条指令独立开一个反馈窗口：发送后收集反馈，直到**静默**超过阈值或总超时。
 * 只等第一条会丢消息：一条指令可以产生多行反馈（函数调用会先报「Running function」，
 * 再报函数体内的每条输出），截断会让后面的行串到下一步。
 *
 * 逐条串行是刻意的——两侧服务端每 tick 处理一次上行包，批量连发会让「哪条反馈属于哪条
 * 指令」无法对应，而反馈文本正是首版的主要判定维度。
 */

import type { Bot } from "mineflayer";

import { openFeedbackWindow, type FeedbackEntry } from "../observe/feedback.ts";

/** 单条指令的执行结果。 */
export interface StepResult {
    /** 指令原文（含前导 /）。 */
    readonly command: string;
    /** 本次窗口内收到的反馈。 */
    readonly feedback: readonly FeedbackEntry[];
    /** 是否在超时前没有收到任何反馈。 */
    readonly timedOut: boolean;
    /** 从发送到收集结束的毫秒数。 */
    readonly elapsedMs: number;
}

/**
 * 执行一条指令并等待反馈。
 *
 * @param bot 已就绪的 bot。
 * @param command 指令原文，必须带前导 `/`。
 * @param timeoutMs 等待反馈的上限（也是最后一条之后再无反馈时的兜底）。
 * @param quietMs 静默阈值：收到消息后连续这么久没有新消息就认为本指令的反馈结束。
 * @returns 单条执行结果。
 */
export async function runStep(
    bot: Bot,
    command: string,
    timeoutMs: number,
    quietMs: number,
): Promise<StepResult> {
    const window = openFeedbackWindow(bot);
    const startedAt = Date.now();
    try {
        bot.chat(command);
        await collectFeedback(window, timeoutMs, quietMs);
        return {
            command,
            feedback: [...window.entries],
            timedOut: window.entries.length === 0,
            elapsedMs: Date.now() - startedAt,
        };
    } finally {
        window.dispose();
    }
}

/**
 * 轮询收集反馈，直到静默足够久或超时。
 *
 * 轮询而不是事件回调：窗口的 entries 是唯一事实来源，轮询让「消息到达」与
 * 「读取结果」不会出现竞态。
 */
async function collectFeedback(
    window: { readonly entries: readonly FeedbackEntry[] },
    timeoutMs: number,
    quietMs: number,
): Promise<void> {
    const deadline = Date.now() + timeoutMs;
    let lastCount = 0;
    let lastChangeAt = Date.now();
    while (Date.now() < deadline) {
        const count = window.entries.length;
        if (count !== lastCount) {
            lastCount = count;
            lastChangeAt = Date.now();
        }
        // 已经有反馈，且静默足够久：本指令的反馈结束。
        if (count > 0 && Date.now() - lastChangeAt >= quietMs) {
            return;
        }
        await new Promise((resolve) => setTimeout(resolve, 10));
    }
}
