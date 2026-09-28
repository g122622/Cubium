/*
 * port.ts — 空闲端口分配与占用探测。
 *
 * 历史教训：早期实现只做「bind(0) 让内核挑一个空闲端口」，随即把它交给服务端。这在
 * Windows 上**不成立**——Node 的 listen() 默认带 SO_REUSEADDR，而 Windows 的 SO_REUSEADDR
 * 允许两个 socket 绑定同一端口（与 Linux 语义相反），于是「刚被杀掉的上一个用例服务端
 * 仍持有该端口」时，内核照样把同一个端口号分配给我们。其后果是就绪探测连到了**别人的**
 * 监听者上、随后对方消失，客户端拿到 ECONNREFUSED（实测 35 条用例中有 3 条踩中，
 * 且表现为「用例失败」而非环境问题）。
 *
 * 因此本模块把判据从「bind 得成功」改为「**主动连接没人应答**」：只要有人应答就弃用该端口。
 */

import net from "node:net";

/**
 * 申请一个当前空闲的 TCP 端口（内核挑选）。
 *
 * 注意：返回值**不保证独占**，调用方应使用 allocateVerifiedPort。
 *
 * @returns 内核分配的空闲端口号。
 */
function allocatePort(): Promise<number> {
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
 * 探测某端口是否已有监听者（能建立 TCP 连接即视为有）。
 *
 * 连接超时按「有人在」处理——状态不明时宁可换一个端口，也不要赌。
 *
 * @param port 目标端口。
 * @param timeoutMs 单次连接超时。
 * @returns 是否有监听者。
 */
export function probeListener(port: number, timeoutMs: number): Promise<boolean> {
    return new Promise((resolve) => {
        const socket = net.connect({ host: "127.0.0.1", port });
        let settled = false;
        const finish = (hasListener: boolean): void => {
            if (settled) {
                return;
            }
            settled = true;
            socket.destroy();
            resolve(hasListener);
        };
        const timer = setTimeout(() => finish(true), timeoutMs);
        socket.once("connect", () => {
            clearTimeout(timer);
            finish(true);
        });
        socket.once("error", () => {
            clearTimeout(timer);
            // ECONNREFUSED 说明没有监听者，端口可用。
            finish(false);
        });
    });
}

/**
 * 申请一个**确认无人应答**的端口。
 *
 * @param attempts 最多尝试次数（每次换一个新端口）。
 * @param probeTimeoutMs 单次占用探测的超时。
 * @returns 可独占使用的端口号。
 */
export async function allocateVerifiedPort(attempts: number, probeTimeoutMs: number): Promise<number> {
    for (let attempt = 1; attempt <= attempts; attempt += 1) {
        const port = await allocatePort();
        if (!(await probeListener(port, probeTimeoutMs))) {
            return port;
        }
    }
    throw new Error(`连续 ${attempts} 次申请到的端口都已被监听，无法获得独占端口`);
}

/**
 * 等待端口不再被监听（用于确认上一个服务端真的放开了监听）。
 *
 * @param port 目标端口。
 * @param timeoutMs 总超时。
 * @param pollMs 轮询间隔。
 * @returns 是否在超时前确认端口已释放。
 */
export async function waitForPortReleased(port: number, timeoutMs: number, pollMs: number): Promise<boolean> {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
        if (!(await probeListener(port, pollMs))) {
            return true;
        }
        await new Promise((resolve) => setTimeout(resolve, pollMs));
    }
    return false;
}
