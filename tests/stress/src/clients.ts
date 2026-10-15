import mineflayer, { type Bot } from "mineflayer";
import { once } from "node:events";
import { performance } from "node:perf_hooks";
import { MC_VERSION } from "../../e2e/bot/src/config.ts";
import type { Options } from "./config.ts";

export interface ClientEvent {
    timeMs: number;
    username: string;
    event: string;
    detail: string;
}

/** 压测只累计包数和连接事件，避免保存逐包快照影响负载发生器。 */
export class StressClient {
    readonly bot: Bot;
    readonly username: string;
    spawned = false;
    expectedEnd = false;
    ended = false;
    failure: string | null = null;
    spawnMs: number | null = null;
    chunksReceived = 0;
    chunksUnloaded = 0;
    corrections = 0;
    packetsSent = 0;
    private readonly ready: Promise<void>;

    /** 创建连接并立即安装生命周期监听，连接对象由 ClientPool 持有。 */
    constructor(port: number, username: string, options: Options, record: (event: ClientEvent) => void) {
        this.username = username;
        const started = performance.now();
        this.bot = mineflayer.createBot({
            host: "127.0.0.1", port, username, version: MC_VERSION, auth: "offline",
            viewDistance: options.viewDistance, checkTimeoutInterval: options.spawnTimeoutSeconds * 1000,
            hideErrors: false,
        });
        const event = (name: string, detail: string): void => record({
            timeMs: Date.now(), username, event: name, detail,
        });
        this.bot.on("error", (error) => {
            if (!this.expectedEnd) this.failure = error.message;
            event(this.expectedEnd ? "shutdown_error" : "error", error.message);
        });
        this.bot.on("kicked", (reason) => {
            const detail = typeof reason === "string" ? reason : JSON.stringify(reason);
            if (!this.expectedEnd) this.failure = detail;
            event(this.expectedEnd ? "shutdown_kick" : "kicked", detail);
        });
        this.bot.on("end", (reason) => {
            this.ended = true;
            if (!this.expectedEnd) this.failure ??= `Unexpected disconnect: ${reason}`;
            event("end", String(reason));
        });
        this.bot._client.on("packet", (_data, meta) => {
            if (meta.name === "map_chunk") this.chunksReceived += 1;
            if (meta.name === "unload_chunk") this.chunksUnloaded += 1;
            if (meta.name === "position" && this.spawned) this.corrections += 1;
        });
        this.ready = this.waitForSpawn(options.spawnTimeoutSeconds * 1000).then(() => {
            this.spawned = true;
            this.spawnMs = performance.now() - started;
            event("spawn", this.spawnMs.toFixed(1));
        });
    }

    /** 等待完整 spawn，并清除所有临时监听器和超时。 */
    private waitForSpawn(timeoutMs: number): Promise<void> {
        return new Promise((resolve, reject) => {
            const cleanup = (): void => {
                clearTimeout(timer);
                this.bot.off("spawn", spawned);
                this.bot.off("error", failed);
                this.bot.off("end", ended);
            };
            const spawned = (): void => { cleanup(); resolve(); };
            const failed = (error: Error): void => { cleanup(); reject(error); };
            const ended = (): void => failed(new Error(this.failure ?? "Disconnected before spawn"));
            const timer = setTimeout(() => failed(new Error(`${this.username} spawn timed out`)), timeoutMs);
            this.bot.once("spawn", spawned);
            this.bot.once("error", failed);
            this.bot.once("end", ended);
        });
    }

    /** 连接等待由创建者立即消费，登录失败不会形成未处理的 Promise。 */
    async waitUntilReady(): Promise<void> {
        await this.ready;
    }

    /** 检查连接和协议状态。 */
    assertHealthy(): void {
        if (this.failure !== null) throw new Error(`${this.username}: ${this.failure}`);
        if (this.spawned && !this.expectedEnd && this.bot._client.state !== "play") {
            throw new Error(`${this.username}: client left Play state`);
        }
    }

    /** 检查发送链路的背压，防止将本地队列增长误记为服务端已承受的负载。 */
    assertWritable(): void {
        this.assertHealthy();
        const client = this.bot._client;
        if (!client.serializer.writable || client.serializer.writableLength > 512 || client.socket.writableLength > 1024 * 1024) {
            throw new Error(`${this.username}: load generator send pipeline is backlogged`);
        }
    }

    /** 通过现有协议序列化器发送有效包并累计主动施加的负载。 */
    write(name: string, params: object): void {
        this.assertWritable();
        this.bot._client.write(name, params);
        this.packetsSent += 1;
    }

    /** 用同一 TCP 流中的 ping/pong 确认服务端已处理此前的负载。 */
    async barrier(timeoutMs: number): Promise<number> {
        const started = performance.now();
        const id = BigInt(Date.now());
        const abort = new AbortController();
        const timer = setTimeout(() => abort.abort(), timeoutMs);
        try {
            const received = once(this.bot._client, "ping_response", { signal: abort.signal });
            this.write("ping_request", { id });
            const [packet] = await received;
            if (BigInt(packet.id) !== id) throw new Error("Mismatched pong response");
            return performance.now() - started;
        } finally {
            clearTimeout(timer);
            abort.abort();
        }
    }

    /** 以服务端传送回包确认位置，而非仅检查机器人的本地预测。 */
    async teleport(command: string, timeoutMs: number): Promise<void> {
        const abort = new AbortController();
        const timer = setTimeout(() => abort.abort(), timeoutMs);
        try {
            const moved = once(this.bot._client, "position", { signal: abort.signal });
            this.bot.chat(command);
            await moved;
        } finally {
            clearTimeout(timer);
            abort.abort();
        }
    }

    /** 停止连接并等待 TCP 关闭，退出风暴可以被完整观测。 */
    async close(): Promise<void> {
        this.expectedEnd = true;
        if (this.ended) return;
        const abort = new AbortController();
        const timer = setTimeout(() => abort.abort(), 3000);
        try {
            const ended = once(this.bot._client, "end", { signal: abort.signal });
            this.bot.quit();
            await ended;
        } catch {
            this.bot._client.socket?.destroy();
        } finally {
            clearTimeout(timer);
            abort.abort();
        }
    }
}

/** 管理用例内全部连接；已开始但未完成登录的连接也纳入清理。 */
export class ClientPool {
    readonly clients: StressClient[] = [];
    readonly events: ClientEvent[] = [];
    private readonly port: number;
    private readonly options: Options;

    /** 绑定本用例的独占端口和明确配置。 */
    constructor(port: number, options: Options) {
        this.port = port;
        this.options = options;
    }

    /** 创建并等待一个真实协议机器人。 */
    async connect(username: string): Promise<StressClient> {
        const client = new StressClient(this.port, username, this.options, (event) => this.events.push(event));
        this.clients.push(client);
        await client.waitUntilReady();
        return client;
    }

    /** 检查非主动退出连接是否健康。 */
    assertHealthy(): void {
        for (const client of this.clients) if (!client.expectedEnd) client.assertHealthy();
    }

    /** 并发退出，避免每个玩家固定延迟使退出阶段被拉长。 */
    async closeAll(): Promise<void> {
        await Promise.allSettled(this.clients.map((client) => client.close()));
    }

    /** 返回体量受限的负载发生器统计。 */
    snapshot(): object {
        return {
            connected: this.clients.filter((client) => client.spawned && !client.ended).length,
            spawned: this.clients.filter((client) => client.spawned).length,
            failures: this.clients.filter((client) => client.failure !== null).length,
            chunksReceived: this.clients.reduce((sum, client) => sum + client.chunksReceived, 0),
            chunksUnloaded: this.clients.reduce((sum, client) => sum + client.chunksUnloaded, 0),
            packetsSent: this.clients.reduce((sum, client) => sum + client.packetsSent, 0),
            runnerRssMiB: process.memoryUsage().rss / 1024 / 1024,
        };
    }
}
