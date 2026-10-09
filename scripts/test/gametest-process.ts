import { spawn } from "node:child_process";
import { performance } from "node:perf_hooks";

export interface ProcessResult {
    code: number | null;
    signal: string | null;
    timedOut: boolean;
}

/** 独立于服务端主线程的进程期限；超时强制终止服务端，Unix 同时清理进程组。 */
export function runProcess(command: string, args: string[], deadline: number): Promise<ProcessResult> {
    return new Promise((resolve, reject) => {
        const child = spawn(command, args, { stdio: "inherit", detached: process.platform !== "win32" });
        let timedOut = false;
        const timer = setTimeout(() => {
            timedOut = true;
            console.error("[gametest-runner] WALL-CLOCK TIMEOUT: overall integration deadline (1800 seconds)");
            if (process.platform === "win32") {
                // Node 在 Windows 直接调用 TerminateProcess，避免 taskkill 启动延迟延长期限。
                child.kill("SIGKILL");
            } else {
                try {
                    process.kill(-child.pid!, "SIGKILL");
                } catch (err) {
                    if ((err as NodeJS.ErrnoException).code !== "ESRCH") child.kill("SIGKILL");
                }
            }
        }, Math.max(0, deadline - performance.now()));
        child.once("error", (err) => {
            clearTimeout(timer);
            reject(err);
        });
        child.once("close", (code, signal) => {
            clearTimeout(timer);
            resolve({ code, signal, timedOut });
        });
    });
}
