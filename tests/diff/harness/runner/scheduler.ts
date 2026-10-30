/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/*
 * scheduler.ts — 并行执行器：每个能力一个子进程，最多 N 个并发。
 *
 * 子进程（worker.ts）只做一件事：跑一个能力，把结果与它实际加载的模块列表写成 JSON 回报。
 * 主进程负责排队、限流、统计与写状态，因此：
 *   - 状态文件只有一个写者，不需要加锁；
 *   - 某个能力崩溃（原生崩溃、OOM、超时）只影响它自己，主进程记为 error 后继续；
 *   - 并发度可以按机器调（`-j`），默认取 CPU 核心数的三分之一——每个能力自己会起 java/javac
 *     子进程，并发太高只会互相争抢内存与 I/O。
 *
 * 排队顺序：上次耗时长的先跑。总耗时取决于最慢的那一个，先启动长的能让尾部更短；没有记录的
 * 能力按「较慢」估计。
 */

import { spawn } from "node:child_process";
import { cpus, freemem } from "node:os";
import path from "node:path";

import type { RunOutcome } from "../core/types.ts";
import { REPO_ROOT } from "./layout.ts";
import type { HarnessRecord } from "./state.ts";

/** 一个执行单元：能力名 + 交给执行进程的参数。 */
export interface ScheduledHarness {
    readonly name: string;
    /** 上次运行的耗时（秒）；没有记录时为 undefined。 */
    readonly seconds?: number;
    /**
     * 互斥组：同组能力永不并发。
     *
     * 用于「oracle 会占用进程外的固定资源」的情形：战利品族 oracle 要在
     * `java.io.tmpdir/horizon-diff-loot` 建原版服务端世界并持有区域文件锁，两个同类
     * oracle 同时跑会直接 `The process cannot access the file because another process
     * has locked a portion of the file`。这类冲突只能靠串行化规避——资源路径由原版决定，
     * 编排层改不了。
     */
    readonly exclusiveGroup?: string;
}

export interface SchedulerOptions {
    /** 同时运行的能力数。 */
    readonly jobs: number;
    /** 单个能力的超时（秒）；超时后杀掉进程并记为 error。 */
    readonly timeoutSeconds: number;
    /** 执行进程的参数（JSON 序列化后通过 stdin 传入）。 */
    readonly payload: (harness: ScheduledHarness) => unknown;
}

/** 执行进程的结果：成功返回结果，失败返回错误说明。 */
export interface WorkerResult {
    readonly outcome?: RunOutcome;
    readonly error?: string;
    readonly stdout: string;
    readonly stderr: string;
}

/** 默认并发度：CPU 核心数的三分之一，至少 1，最多 8。 */
export function defaultJobs(): number {
    const machine = process.env.MC_DIFF_JOBS;
    if (machine !== undefined && Number.isFinite(Number(machine)) && Number(machine) > 0) {
        return Math.floor(Number(machine));
    }
    return Math.max(1, Math.min(8, Math.floor(cpus().length / 3)));
}

/** 当前可用内存（MiB）；每个能力额外起 JVM，内存紧张时打印提示而不是静默降速。 */
export function freeMemoryMiB(): number {
    return Math.floor(freemem() / (1024 * 1024));
}

/** 执行进程入口。 */
function workerScript(): string {
    return path.join(REPO_ROOT, "tests/diff/harness/worker.ts");
}

/** 运行一个能力，返回它的结果；进程异常退出时抛错由调用方记录。 */
function runWorker(harness: ScheduledHarness, options: SchedulerOptions): Promise<WorkerResult> {
    return new Promise<WorkerResult>((resolve) => {
        const child = spawn(process.execPath, [workerScript(), "--trace"], {
            cwd: REPO_ROOT,
            stdio: ["pipe", "pipe", "pipe"],
            env: { ...process.env, NODE_NO_WARNINGS: "1" },
            windowsHide: true,
        });
        const stdoutChunks: Buffer[] = [];
        const stderrChunks: Buffer[] = [];
        let finished = false;
        const timer = setTimeout(() => {
            if (!finished) {
                child.kill();
                stderrChunks.push(Buffer.from(`\n[diff] ${harness.name} 超时（${options.timeoutSeconds}s），已终止\n`));
            }
        }, options.timeoutSeconds * 1000);
        child.stdout.on("data", (chunk: Buffer) => stdoutChunks.push(chunk));
        child.stderr.on("data", (chunk: Buffer) => stderrChunks.push(chunk));
        child.on("error", (error) => {
            if (!finished) {
                finished = true;
                clearTimeout(timer);
                resolve({ error: error.message, stdout: "", stderr: "" });
            }
        });
        child.on("close", (code) => {
            if (finished) {
                return;
            }
            finished = true;
            clearTimeout(timer);
            const stdout = Buffer.concat(stdoutChunks).toString("utf8");
            const stderr = Buffer.concat(stderrChunks).toString("utf8");
            // 协议：stdout 最后一行为一行 JSON（执行进程自己把日志写到 stderr）。
            const lines = stdout.split(/\r?\n/).filter((line) => line.trim().length > 0);
            const last = lines[lines.length - 1];
            if (last === undefined) {
                resolve({ error: `执行进程没有输出（退出码 ${String(code)}）：${stderr.slice(-4000)}`, stdout, stderr });
                return;
            }
            try {
                const parsed = JSON.parse(last) as { outcome?: RunOutcome; error?: string };
                resolve({ outcome: parsed.outcome, error: parsed.error, stdout, stderr });
            } catch {
                resolve({ error: `执行进程输出无法解析（退出码 ${String(code)}）：${last.slice(0, 2000)}`, stdout, stderr });
            }
        });
        child.stdin.end(JSON.stringify(options.payload(harness)));
    });
}

/**
 * 并行运行全部能力。
 *
 * @param harnesses 待运行的能力
 * @param options 并发度、超时与参数
 * @param onDone 每个能力结束时回调（用于即时报告进度）
 */
export async function runAll(
    harnesses: readonly ScheduledHarness[],
    options: SchedulerOptions,
    onDone: (harness: ScheduledHarness, result: WorkerResult, elapsedSeconds: number) => void,
): Promise<void> {
    // 长的先跑：总耗时取决于最慢的一个。
    const queue = [...harnesses].sort((left, right) => (right.seconds ?? 60) - (left.seconds ?? 60));
    /** 互斥组里正在运行的能力（组名 → 能力名）。 */
    const occupied = new Map<string, string>();
    let running = 0;
    await new Promise<void>((resolve) => {
        const launch = (): void => {
            // 队列是静态的：跳过「组被占用」的项，从剩下的里面按原顺序挑第一个能跑的。
            // 被跳过的项不会丢——同组的前一个结束时会再次调用 launch。
            for (let position = 0; position < queue.length; position++) {
                if (running >= options.jobs) {
                    return;
                }
                const harness = queue[position];
                if (harness.exclusiveGroup !== undefined && occupied.has(harness.exclusiveGroup)) {
                    continue;
                }
                queue.splice(position, 1);
                position--;
                if (harness.exclusiveGroup !== undefined) {
                    occupied.set(harness.exclusiveGroup, harness.name);
                }
                running++;
                const started = process.hrtime.bigint();
                void runWorker(harness, options).then((result) => {
                    running--;
                    if (harness.exclusiveGroup !== undefined) {
                        occupied.delete(harness.exclusiveGroup);
                    }
                    onDone(harness, result, Number(process.hrtime.bigint() - started) / 1e9);
                    if (queue.length === 0 && running === 0) {
                        resolve();
                    } else {
                        launch();
                    }
                });
            }
            if (queue.length === 0 && running === 0) {
                resolve();
            }
        };
        launch();
    });
}

/** 从状态记录里取出上次耗时（没有记录时 undefined）。 */
export function previousSeconds(record: HarnessRecord | undefined): number | undefined {
    return record?.seconds;
}