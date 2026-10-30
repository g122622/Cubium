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
 * worker.ts — 一次只跑一个能力的执行进程（由 runner.ts 的调度器启动）。
 *
 * 协议：
 *   stdin  一行 JSON：`{ harness, buildDir, fresh }`
 *   stdout 一行 JSON：`{ outcome }` 或 `{ error }`
 * 日志一律走 stderr。主进程取 stdout 的最后一行有效 JSON。
 *
 * 流程：
 *   1. 读参数，装模块解析钩子（记录这个能力实际加载的仓库内文件）；
 *   2. 加载运行层与能力声明，跑一次；
 *   3. 把结果与模块列表写成一行 JSON。
 *
 * 退出码：0 通过，1 存在差异/运行错误，2 执行进程自身失败（参数、加载错误）。主进程只认
 * 那一行 JSON，退出码仅用于诊断。
 */

import { installModuleTrace, loadedModules } from "./runner/module-trace.ts";

interface WorkerPayload {
    readonly harness: string;
    readonly buildDir: string;
    /** 是否禁用 oracle 缓存（`--fresh`）。 */
    readonly fresh: boolean;
}

/** 从 stdin 读完一行 JSON。 */
function readPayload(): Promise<WorkerPayload> {
    return new Promise((resolve, reject) => {
        const chunks: Buffer[] = [];
        process.stdin.on("data", (chunk: Buffer) => chunks.push(chunk));
        process.stdin.on("end", () => {
            try {
                resolve(JSON.parse(Buffer.concat(chunks).toString("utf8")) as WorkerPayload);
            } catch (error) {
                reject(error as Error);
            }
        });
        process.stdin.on("error", reject);
    });
}

function emit(payload: unknown): void {
    process.stdout.write(`${JSON.stringify(payload)}\n`);
}

async function main(): Promise<void> {
    const payload = await readPayload();
    // 钩子在加载任何业务模块之前装上：要记录的是「这个能力运行中解析到的模块」。
    installModuleTrace();
    // 动态导入：静态导入会在这行代码之前完成模块加载，钩子就看不到它们了。
    const { projectLayout } = await import("./runner/layout.ts");
    const { loadState } = await import("./runner/state.ts");
    const { loadOracleEnvironment } = await import("./oracle/environment.ts");
    const { OracleCache } = await import("./oracle/cache.ts");
    const { HarnessSession, runDeclaration } = await import("./runner/session.ts");
    const { findHarness } = await import("./harnesses/index.ts");

    const layout = projectLayout(payload.buildDir);
    const state = loadState(layout.stateDirectory);
    const declaration = findHarness(payload.harness);
    if (declaration === undefined) {
        throw new Error(`未知能力：${payload.harness}`);
    }
    const session = new HarnessSession({
        layout,
        hashes: state.hashes,
        oracleCache: new OracleCache(layout.stateDirectory, !payload.fresh),
        environment: loadOracleEnvironment(layout),
    });
    const outcome = runDeclaration(session, declaration);
    emit({ outcome: { ...outcome, modules: loadedModules() } });
    // 显式退出：执行进程可能持有注册表单例，交给静态析构会在退出阶段访问已销毁的单例。
    process.exit(outcome.status === "pass" ? 0 : 1);
}

void main().catch((error: Error) => {
    emit({ error: error.stack ?? error.message });
    process.exit(2);
});