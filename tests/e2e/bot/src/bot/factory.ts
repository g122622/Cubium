/*
 * factory.ts — bot 创建与原始包记录。
 *
 * 关键约定：
 *   - hideErrors 必须为 false。mineflayer 默认 true 会吞掉协议层错误，故障时只剩"超时"没有原因。
 *   - 包记录器挂在 bot._client 的 'packet'/'write' 上（nmp 的不稳定 API，但正是 e2e 需要的粒度）。
 *     它的首要用途是回答「某包到底发没发」——例如 mineflayer 的 spawn 依赖首个
 *     update_health(cb, Mojang 名 set_health)，失败时能直接从记录里看出计数为 0。
 */

import mineflayer from "mineflayer";
import type { Bot } from "mineflayer";
import { BOT_TIMEOUT_MS, MC_VERSION } from "../config.ts";

/** 单条包记录。 */
export interface PacketRecord {
    readonly direction: "S2C" | "C2S";
    readonly state: string;
    readonly name: string;
    readonly size: number;
    /**
     * 包内容摘要（JSON，超长截断）。
     *
     * 诊断价值极高——例如「服务端回的 block_update 里目标位置究竟是什么方块」这类问题，
     * 只看包名和大小无法回答。区块等大包的 payload 会被截断，避免记录膨胀。
     */
    readonly params?: string;
}

/** 记录 payload 的最大字符数（超过则截断）。 */
const PARAMS_LIMIT = 400;

function summarizeParams(params: unknown): string | undefined {
    if (params === undefined || params === null) {
        return undefined;
    }
    try {
        const text = JSON.stringify(params);
        if (text === undefined) {
            return undefined;
        }
        return text.length > PARAMS_LIMIT ? `${text.slice(0, PARAMS_LIMIT)}…` : text;
    } catch {
        return "<不可序列化>";
    }
}

/** 环形包记录器。 */
export class PacketTrace {
    private readonly entries: PacketRecord[] = [];
    private readonly limit: number;

    constructor(limit: number) {
        this.limit = limit;
    }

    record(entry: PacketRecord): void {
        this.entries.push(entry);
        if (this.entries.length > this.limit) {
            this.entries.splice(0, this.entries.length - this.limit);
        }
    }

    get all(): readonly PacketRecord[] {
        return this.entries;
    }

    /** 收到/发出的指定包名次数。 */
    count(name: string): number {
        return this.entries.filter((entry) => entry.name === name).length;
    }

    has(name: string): boolean {
        return this.count(name) > 0;
    }

    /** 去重后的包名列表（按首次出现顺序）。 */
    names(): string[] {
        return [...new Set(this.entries.map((entry) => entry.name))];
    }

    /** 导出为 JSONL，供失败诊断落盘。 */
    toJsonl(): string {
        return this.entries.map((entry) => JSON.stringify(entry)).join("\n");
    }

    /** 包名计数摘要（诊断报告用）。 */
    summary(): string {
        const counts = new Map<string, number>();
        for (const entry of this.entries) {
            const key = `${entry.direction} ${entry.name}`;
            counts.set(key, (counts.get(key) ?? 0) + 1);
        }
        return [...counts.entries()]
            .sort((a, b) => b[1] - a[1])
            .map(([key, count]) => `${String(count).padStart(6)}  ${key}`)
            .join("\n");
    }
}

/** bot 句柄。 */
export interface BotHandle {
    readonly bot: Bot;
    readonly trace: PacketTrace;
    /** 关闭 bot 连接并摘除监听器。 */
    dispose(): Promise<void>;
}

/** bot 创建参数。全部显式传入。 */
export interface BotOptions {
    readonly port: number;
    /** 用户名（离线模式），建议带用例标识便于在服务端日志中定位。 */
    readonly username: string;
    /** 包记录环形缓冲上限。 */
    readonly traceLimit: number;
}

/**
 * 创建一个已连上但尚未等待 spawn 的 bot。
 *
 * @param options 连接参数。
 * @returns bot 句柄；调用方负责 await 事件并最终 dispose()。
 */
export function createBot(options: BotOptions): BotHandle {
    const trace = new PacketTrace(options.traceLimit);

    const bot = mineflayer.createBot({
        host: "127.0.0.1",
        port: options.port,
        username: options.username,
        version: MC_VERSION,
        auth: "offline",
        checkTimeoutInterval: BOT_TIMEOUT_MS,
        // 必须为 false：true 会吞掉协议层错误（如 registry 解析失败），
        // 使故障退化为无声超时。
        hideErrors: false,
    });

    // nmp 的不稳定 API。用于记录原始包，是「包到底发没发」的唯一直接证据。
    const client = bot._client as unknown as {
        on(event: string, listener: (...args: unknown[]) => void): void;
        write(name: string, params: unknown): void;
        state?: string;
    };
    client.on("packet", (...args: unknown[]) => {
        const meta = args[1] as { name: string; state: string; size: number } | undefined;
        if (meta !== undefined) {
            trace.record({
                direction: "S2C",
                state: meta.state,
                name: meta.name,
                size: meta.size,
                params: summarizeParams(args[0]),
            });
        }
    });

    // 上行包：nmp 的 write() 不发射任何事件，只能包装方法本身来观测。
    const originalWrite = client.write.bind(client);
    client.write = (name: string, params: unknown): void => {
        trace.record({
            direction: "C2S",
            state: client.state ?? "<未知>",
            name,
            size: 0,
            params: summarizeParams(params),
        });
        originalWrite(name, params);
    };

    return {
        bot,
        trace,
        async dispose(): Promise<void> {
            try {
                bot.quit();
            } catch {
                /* 连接可能已断 */
            }
            // 给 socket 一点时间优雅关闭，让服务端日志干净收尾。
            await new Promise((resolve) => setTimeout(resolve, 150));
        },
    };
}
