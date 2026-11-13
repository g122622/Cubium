/*
 * feedback.ts — 采集 bot 收到的命令反馈。
 *
 * 反馈的承载通道两侧不同，必须同时收两种：
 *   - **system_chat**（position "system"）：命令成功/失败、tellraw、join 等系统消息。
 *   - **profileless_chat**（position "chat"）：vanilla 的 `/say` 走聊天广播而非系统消息，
 *     Cubium 则降级为 system_chat——同一句 `/say` 在两侧落在不同 position 上。
 * actionbar（"game_info"）不是命令反馈的稳定通道，直接丢弃。
 */

import type { Bot } from "mineflayer";

/** 一条反馈。 */
export interface FeedbackEntry {
    /** 客户端按 en_us 渲染后的纯文本。 */
    readonly text: string;
    /** mineflayer 的 position："system" 或 "chat"；诊断用，不参与比对。 */
    readonly position: string;
    /** 原始组件（mineflayer 的 ChatMessage）；诊断用。 */
    readonly raw: unknown;
}

/** 一次反馈采集窗口。 */
export interface FeedbackWindow {
    readonly entries: FeedbackEntry[];
    dispose(): void;
}

/** 视为命令反馈的 position 集合。 */
const FEEDBACK_POSITIONS: ReadonlySet<string> = new Set(["chat", "system"]);

/**
 * 打开一个反馈采集窗口。
 *
 * @param bot 目标 bot。
 * @returns 窗口句柄；调用方负责 dispose()。
 */
export function openFeedbackWindow(bot: Bot): FeedbackWindow {
    const entries: FeedbackEntry[] = [];
    const listener = (message: string, position: string, raw: unknown): void => {
        if (!FEEDBACK_POSITIONS.has(position)) {
            return;
        }
        entries.push({ text: message, position, raw });
    };
    bot.on("messagestr", listener);
    return {
        entries,
        dispose(): void {
            bot.removeListener("messagestr", listener);
        },
    };
}
