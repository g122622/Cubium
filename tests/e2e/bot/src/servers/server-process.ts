/*
 * server-process.ts — 服务端进程抽象（Cubium 与 vanilla 共用）。
 *
 * 两侧差异全部收敛到 ServerSpec 的两个策略字段：
 *   - ready：Cubium 用 TCP 就绪（accept 循环在 initialize() 内启动）；vanilla 必须用日志行
 *     （它先开 listen 再加载世界，TCP 可连不代表世界已就绪）。
 *   - shutdown：Cubium 在 Windows 上 SIGTERM 走 TerminateProcess，无法触发其 std::signal
 *     优雅退出，故直接硬杀（世界目录一次性、用完即删）；vanilla 支持 stdin `stop`。
 */

import { spawn, type ChildProcess } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { waitForPort } from "./port.ts";

/** 就绪判定策略。 */
export type ReadyStrategy =
    | { readonly kind: "tcp"; readonly port: number; readonly timeoutMs: number }
    | { readonly kind: "log"; readonly pattern: RegExp; readonly timeoutMs: number };

/** 停止策略。 */
export type ShutdownStrategy =
    | { readonly kind: "kill" }
    | { readonly kind: "stdin"; readonly command: string; readonly graceMs: number };

/** 一份服务端启动规格。所有字段必须显式提供（不设默认值）。 */
export interface ServerSpec {
    /** 用于日志与诊断的人类可读标签（如 "cubium/e2e"）。 */
    readonly label: string;
    /** 可执行文件（或 java 等解释器）。 */
    readonly command: string;
    readonly args: readonly string[];
    readonly cwd: string;
    readonly ready: ReadyStrategy;
    readonly shutdown: ShutdownStrategy;
    /** 服务端 stdout/stderr 的落盘路径。 */
    readonly logPath: string;
}

/** 进程退出信息。 */
export interface ExitInfo {
    readonly code: number | null;
    readonly signal: NodeJS.Signals | null;
    /** 从 launch 到退出的毫秒数。 */
    readonly elapsedMs: number;
    /** 是否在主动停止流程中退出（true 表示退出属预期，不算崩溃）。 */
    readonly expected: boolean;
}

export type ServerState = "idle" | "starting" | "ready" | "stopping" | "stopped" | "crashed";

/** 内存中保留的最大日志行数（诊断用；完整日志始终落盘）。 */
const MAX_IN_MEMORY_LOG_LINES = 4000;

/**
 * 服务端进程的全局登记表。
 *
 * 用途：进程若在用例中途因异常退出（含 runner 自身崩溃/被 Ctrl-C），仍有子进程存活时会
 * 变成孤儿——尤其 vanilla 是 java 进程，比 Cubium 更重。在 process 退出钩子里按登记表统一清理。
 */
const LIVE_CHILDREN = new Set<ChildProcess>();
let exitHookInstalled = false;

function killTree(child: ChildProcess): void {
    if (child.pid === undefined || child.exitCode !== null) {
        return;
    }
    if (process.platform === "win32") {
        // /T 连带子进程树，/F 强制。Cubium 无子进程，vanilla 的 bundler 在同 JVM 内。
        try {
            spawn("taskkill", ["/PID", String(child.pid), "/T", "/F"], { stdio: "ignore" });
        } catch {
            /* 尽力而为 */
        }
    } else {
        try {
            process.kill(-child.pid, "SIGKILL");
        } catch {
            try {
                child.kill("SIGKILL");
            } catch {
                /* 尽力而为 */
            }
        }
    }
}

function installExitHook(): void {
    if (exitHookInstalled) {
        return;
    }
    exitHookInstalled = true;
    const cleanup = (): void => {
        for (const child of LIVE_CHILDREN) {
            killTree(child);
        }
        LIVE_CHILDREN.clear();
    };
    process.on("exit", cleanup);
    process.on("SIGINT", () => {
        cleanup();
        process.exit(130);
    });
    process.on("SIGTERM", () => {
        cleanup();
        process.exit(143);
    });
}

export class ServerProcess {
    private readonly spec: ServerSpec;
    private readonly logLines: string[] = [];
    private child: ChildProcess | null = null;
    private logStream: fs.WriteStream | null = null;
    private stateValue: ServerState = "idle";
    private exitInfoValue: ExitInfo | null = null;
    private launchedAtMs = 0;
    private readyAtMs = 0;
    private exitWaiters: Array<(info: ExitInfo) => void> = [];

    constructor(spec: ServerSpec) {
        this.spec = spec;
    }

    get state(): ServerState {
        return this.stateValue;
    }

    get exitInfo(): ExitInfo | null {
        return this.exitInfoValue;
    }

    /** 从 launch 到就绪的毫秒数（未就绪时为 -1）。 */
    get startupMs(): number {
        return this.readyAtMs > 0 ? this.readyAtMs - this.launchedAtMs : -1;
    }

    /** 日志尾部若干行，用于失败诊断。 */
    tail(lineCount: number): string[] {
        return this.logLines.slice(-lineCount);
    }

    /** 全部内存日志。 */
    get logs(): readonly string[] {
        return this.logLines;
    }

    /** 在给定正则内查找日志行（诊断断言用）。 */
    findLog(pattern: RegExp): string[] {
        return this.logLines.filter((line) => pattern.test(line));
    }

    /** 启动进程并等待就绪。失败抛异常（异常消息含日志尾部）。 */
    async start(): Promise<void> {
        installExitHook();
        fs.mkdirSync(path.dirname(this.spec.logPath), { recursive: true });
        this.logStream = fs.createWriteStream(this.spec.logPath, { flags: "a" });

        this.stateValue = "starting";
        this.launchedAtMs = Date.now();

        const child = spawn(this.spec.command, [...this.spec.args], {
            cwd: this.spec.cwd,
            stdio: ["pipe", "pipe", "pipe"],
            env: process.env,
        });
        this.child = child;
        LIVE_CHILDREN.add(child);

        const onData = (chunk: Buffer): void => {
            this.logStream?.write(chunk);
            for (const line of chunk.toString("utf8").split(/\r?\n/)) {
                if (line.trim().length === 0) {
                    continue;
                }
                this.logLines.push(line);
                if (this.logLines.length > MAX_IN_MEMORY_LOG_LINES) {
                    this.logLines.splice(0, this.logLines.length - MAX_IN_MEMORY_LOG_LINES);
                }
            }
        };
        child.stdout?.on("data", onData);
        child.stderr?.on("data", onData);

        child.on("exit", (code, signal) => {
            const expected = this.stateValue === "stopping";
            this.exitInfoValue = {
                code,
                signal,
                elapsedMs: Date.now() - this.launchedAtMs,
                expected,
            };
            LIVE_CHILDREN.delete(child);
            if (!expected && this.stateValue === "starting") {
                this.stateValue = "crashed";
            } else if (!expected && this.stateValue === "ready") {
                this.stateValue = "crashed";
            } else {
                this.stateValue = "stopped";
            }
            for (const waiter of this.exitWaiters) {
                waiter(this.exitInfoValue);
            }
            this.exitWaiters = [];
        });

        try {
            await this.waitUntilReady(child);
        } catch (err) {
            const tail = this.tail(30).join("\n");
            await this.stop();
            throw new Error(`${this.spec.label} 启动失败：${(err as Error).message}\n---- 日志尾部 ----\n${tail}`);
        }
        this.readyAtMs = Date.now();
        this.stateValue = "ready";
    }

    private async waitUntilReady(child: ChildProcess): Promise<void> {
        const { ready } = this.spec;
        if (ready.kind === "tcp") {
            const elapsed = await waitForPort(ready.port, ready.timeoutMs);
            // TCP 可连后再补一小段等待：Cubium 的 accept 循环在 initialize() 内启动，
            // 而 run() 的 tick 主循环在其返回后才开始，玩家的入站包需经 pollNetwork → tick
            // → drainInbound 才能派发。这个窗口极小但存在。
            void elapsed;
            await new Promise((resolve) => setTimeout(resolve, 200));
            if (this.stateValue === "crashed") {
                throw new Error("服务端在就绪探测期间退出");
            }
            return;
        }

        const deadline = Date.now() + ready.timeoutMs;
        while (Date.now() < deadline) {
            if (this.stateValue === "crashed") {
                throw new Error("服务端在就绪探测期间退出");
            }
            if (this.logLines.some((line) => ready.pattern.test(line))) {
                return;
            }
            void child;
            await new Promise((resolve) => setTimeout(resolve, 200));
        }
        throw new Error(`等待日志匹配 ${ready.pattern} 超时（${ready.timeoutMs}ms）`);
    }

    /** 停止进程。幂等。 */
    async stop(): Promise<void> {
        const child = this.child;
        if (child === null || child.exitCode !== null || this.stateValue === "stopped") {
            this.stateValue = this.stateValue === "crashed" ? "crashed" : "stopped";
            this.logStream?.end();
            return;
        }
        this.stateValue = "stopping";

        if (this.spec.shutdown.kind === "stdin" && child.stdin !== null && child.stdin.writable) {
            const exited = this.waitForExit(this.spec.shutdown.graceMs);
            try {
                child.stdin.write(`${this.spec.shutdown.command}\n`);
            } catch {
                /* 管道可能已关闭 */
            }
            const graceful = await exited;
            if (graceful) {
                this.logStream?.end();
                return;
            }
        }

        killTree(child);
        await this.waitForExit(5000);
        this.logStream?.end();
    }

    /** 等待进程退出；返回是否在超时前退出。 */
    private waitForExit(timeoutMs: number): Promise<boolean> {
        if (this.exitInfoValue !== null) {
            return Promise.resolve(true);
        }
        return new Promise((resolve) => {
            const timer = setTimeout(() => resolve(false), timeoutMs);
            this.exitWaiters.push(() => {
                clearTimeout(timer);
                resolve(true);
            });
        });
    }
}
