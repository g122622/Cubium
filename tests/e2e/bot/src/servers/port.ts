/*
 * port.ts — 空闲端口分配。
 *
 * 做法与上游 mineflayer/test/common/util.js 的 getPort() 一致：绑 0 号端口让内核挑，
 * 读到实际端口后立刻释放。
 *
 * 已知局限：从「读出端口」到「服务端真正绑定」之间存在 TOCTOU 窗口，理论上会被抢占。
 * 抢占的处理不在此处——由调用方 runner.launchServer 识别服务端启动失败日志中的端口冲突
 * 特征后换端口重试。本模块只负责「申请一个当前空闲的端口」这一件事。
 */

import net from "node:net";

/**
 * 申请一个当前空闲的 TCP 端口。
 *
 * @returns 内核分配的空闲端口号。
 */
export function allocatePort(): Promise<number> {
    return new Promise((resolve, reject) => {
        const probe = net.createServer();
        probe.once("error", reject);
        probe.listen(0, "127.0.0.1", () => {
            const address = probe.address();
            if (address === null || typeof address === "string") {
                probe.close();
                reject(new Error("无法从探测 socket 读取端口号"));
                return;
            }
            const { port } = address;
            probe.close(() => resolve(port));
        });
    });
}

/**
 * 轮询等待端口可连。
 *
 * @param port 目标端口。
 * @param timeoutMs 总超时。
 * @returns 从调用到连上的毫秒数。
 */
export function waitForPort(port: number, timeoutMs: number): Promise<number> {
    const startMs = Date.now();
    return new Promise((resolve, reject) => {
        const attempt = (): void => {
            const socket = net.connect({ host: "127.0.0.1", port });
            socket.once("connect", () => {
                socket.destroy();
                resolve(Date.now() - startMs);
            });
            socket.once("error", () => {
                socket.destroy();
                if (Date.now() - startMs > timeoutMs) {
                    reject(new Error(`端口 ${port} 在 ${timeoutMs}ms 内未就绪`));
                } else {
                    setTimeout(attempt, 200);
                }
            });
        };
        attempt();
    });
}
