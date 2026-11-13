/*
 * server-process.ts — 服务端进程抽象（Cubium 与 vanilla 共用）。
 *
 * 两侧差异全部收敛到 ServerSpec 的两个策略字段：
 *   - ready：**一律用日志行**。TCP 可连不是就绪信号——两侧的监听 socket 都在世界加载完成
 *     之前就已建立（Cubium 在 initialize() 内 listen，vanilla 先开 listen 再加载世界），
 *     此时连上去会拿到空注册表并静默挂起；而且端口可能被上一个用例尚未退出的服务端占着，
 *     探测会连到「别人的」监听者上。故两侧各打印一行明确的就绪日志，harness 以它为准，
 *     并核对日志里的端口号与本次申请的端口一致（等价于核对监听者身份）。
 *   - shutdown：Cubium 在 Windows 上 SIGTERM 走 TerminateProcess，无法触发其 std::signal
 *     优雅退出，故直接硬杀（每用例独立世界目录）；vanilla 支持 stdin `stop`。
 */

import { spawn, type ChildProcess } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

/** 就绪判定策略：匹配日志行，并核对该行是否属于本次启动的进程。 */
export type ReadyStrategy = {
    readonly kind: "log";
    readonly pattern: RegExp;
    readonly timeoutMs: number;
    /**
     * 要求匹配行中出现该端口号；null 表示该服务端不打印端口（vanilla 的 Done 行如此）。
     *
     * 这一项是「监听者身份核对」：只匹配日志文本无法排除读到**别的进程**残留日志的可能。
     */
    readonly requirePort: number | null;
};

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

/** 硬杀后等待进程退出的时间；总等待为其两倍（两次强杀）。 */
const KILL_GRACE_MS = 5_000;

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
    private stopTimedOut = false;

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

    /** 停止流程是否超时（进程可能残留，端口可能仍被占用）。 */
    get stopTimedOutFlag(): boolean {
        return this.stopTimedOut;
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
        const deadline = Date.now() + ready.timeoutMs;
        while (Date.now() < deadline) {
            if (this.stateValue === "crashed") {
                throw new Error("服务端在就绪探测期间退出");
            }
            if (this.findLog(ready.pattern).some((line) => ready.requirePort === null || line.includes(String(ready.requirePort)))) {
                return;
            }
            void child;
            await new Promise((resolve) => setTimeout(resolve, 200));
        }
        throw new Error(
            `等待就绪日志 ${ready.pattern} 超时（${ready.timeoutMs}ms` +
                `${ready.requirePort === null ? "" : `，要求端口 ${ready.requirePort}`}）`,
        );
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

        // 硬杀必须**确认进程真的没了**：上一次实现丢弃了 waitForExit 的返回值，于是
        // 「5 秒内没退干净」会被静默放过，下一个用例拿到同一端口时探测会连到它身上
        // （Windows 的 SO_REUSEADDR 允许端口重绑），随后它退出 → 客户端 ECONNREFUSED。
        killTree(child);
        if (await this.waitForExit(KILL_GRACE_MS)) {
            this.logStream?.end();
            return;
        }
        // 再补一次带子进程树的强杀，仍不退则记为超时退出（由调用方决定如何处理）。
        killTree(child);
        this.stopTimedOut = !(await this.waitForExit(KILL_GRACE_MS));
        if (this.stopTimedOut) {
            console.warn(`      ${this.spec.label} 在 ${KILL_GRACE_MS * 2}ms 内未能停止（进程可能残留）`);
        }
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
