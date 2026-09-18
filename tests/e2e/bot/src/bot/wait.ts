/*
 * wait.ts — 事件与条件等待助手。
 *
 * 设计要点：**失败要带现场**。纯「超时」二字无法定位问题——等待 spawn 失败时真正想知道的是
 * 「login 收到没」「update_health 发了几次」。因此每个等待都可附带一个 describe 回调，
 * 超时异常里会带上它输出的已观察状态。
 *
 * 另外：等待期间若 bot 触发 error / end / kicked 会**立即失败**而非干等到超时。
 * mineflayer 的协议层错误（如 registry 解析失败）走的是 error 事件且**不关闭连接**，
 * 不主动监听就只能得到一次 30 秒的无声超时。
 */

import type { Bot } from "mineflayer";

/** 等待超时。消息中应包含等待目标与已观察到的状态。 */
export class TimeoutError extends Error {
    constructor(message: string) {
        super(message);
        this.name = "TimeoutError";
    }
}

/** 等待期间 bot 自身报错（非本用例主动等待的事件）。 */
export class BotFailureError extends Error {
    constructor(message: string) {
        super(message);
        this.name = "BotFailureError";
    }
}

/** 等待参数。 */
export interface WaitOptions {
    readonly timeoutMs: number;
    readonly what: string;
    /** 返回当前已观察到的状态描述，超时时附在异常消息里。 */
    readonly describe?: () => string;
}

/** 把 describe 的输出拼成异常后缀。 */
function details(options: WaitOptions): string {
    if (options.describe === undefined) {
        return "";
    }
    try {
        return `\n已观察到的状态：\n${options.describe()}`;
    } catch (err) {
        return `\n(describe 回调本身失败：${(err as Error).message})`;
    }
}

/**
 * 等待 bot 触发某个事件。
 *
 * 等待期间 bot 的 error/end/kicked 会立即让本次等待失败。
 *
 * @returns 事件参数数组。
 */
export function waitForEvent(bot: Bot, event: string, options: WaitOptions): Promise<unknown[]> {
    return new Promise((resolve, reject) => {
        const cleanup = (): void => {
            clearTimeout(timer);
            bot.removeListener(event, onEvent);
            bot.removeListener("error", onBotError);
            bot.removeListener("end", onBotEnd);
            bot.removeListener("kicked", onBotKicked);
        };

        const timer = setTimeout(() => {
            cleanup();
            reject(new TimeoutError(`等待 ${options.what} 超时（${options.timeoutMs}ms）${details(options)}`));
        }, options.timeoutMs);

        const onEvent = (...args: unknown[]): void => {
            cleanup();
            resolve(args);
        };
        const onBotError = (err: Error): void => {
            cleanup();
            reject(new BotFailureError(`等待 ${options.what} 期间 bot 报错：${err.message}${details(options)}`));
        };
        const onBotEnd = (reason: string): void => {
            cleanup();
            reject(new BotFailureError(`等待 ${options.what} 期间连接结束：${reason}${details(options)}`));
        };
        const onBotKicked = (reason: unknown): void => {
            cleanup();
            reject(
                new BotFailureError(
                    `等待 ${options.what} 期间被踢出：${JSON.stringify(reason)}${details(options)}`,
                ),
            );
        };

        bot.once(event, onEvent);
        bot.once("error", onBotError);
        bot.once("end", onBotEnd);
        bot.once("kicked", onBotKicked);
    });
}

/** 轮询等待条件成立。条件抛异常会立即冒泡。 */
export async function waitForCondition(
    predicate: () => boolean,
    options: WaitOptions & { readonly pollMs: number },
): Promise<void> {
    const deadline = Date.now() + options.timeoutMs;
    while (Date.now() < deadline) {
        if (predicate()) {
            return;
        }
        await new Promise((resolve) => setTimeout(resolve, options.pollMs));
    }
    throw new TimeoutError(`等待 ${options.what} 超时（${options.timeoutMs}ms）${details(options)}`);
}

/** 固定延时。 */
export function delay(ms: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, ms));
}
