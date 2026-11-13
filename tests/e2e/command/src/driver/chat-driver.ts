/*
 * chat-driver.ts — 逐条聊天指令驱动。
 *
 * 每条指令独立开一个反馈窗口：发送后等待第一条聊天反馈，超时按「无反馈」记录。
 * 逐条串行是刻意的——两侧服务端每 tick 处理一次上行包，批量连发会让「哪条反馈
 * 属于哪条指令」无法对应，而反馈文本正是首版的主要判定维度。
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
    /** 从发送到收到第一条反馈（或超时）的毫秒数。 */
    readonly elapsedMs: number;
}

/**
 * 执行一条指令并等待反馈。
 *
 * @param bot 已就绪的 bot。
 * @param command 指令原文，必须带前导 `/`。
 * @param timeoutMs 等待第一条反馈的上限。
 * @returns 单条执行结果。
 */
export async function runStep(bot: Bot, command: string, timeoutMs: number): Promise<StepResult> {
    const window = openFeedbackWindow(bot);
    const startedAt = Date.now();
    try {
        bot.chat(command);
        await waitForFirstFeedback(window, timeoutMs);
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
 * 等到窗口中出现第一条反馈或超时。
 *
 * 轮询而不是事件回调：窗口的 entries 是唯一事实来源，轮询让「第一条反馈到达」
 * 与「读取结果」不会出现竞态。
 */
async function waitForFirstFeedback(
    window: { readonly entries: readonly FeedbackEntry[] },
    timeoutMs: number,
): Promise<void> {
    const deadline = Date.now() + timeoutMs;
    while (window.entries.length === 0 && Date.now() < deadline) {
        await new Promise((resolve) => setTimeout(resolve, 10));
    }
}
