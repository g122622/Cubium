/*
 * viewer.ts — 可视化调试工具（**人工使用，不参与自动化**）。
 *
 * 起一个 Cubium 服务端 + 一个 bot，并用 prismarine-viewer 在浏览器里实时显示
 * bot 所见的世界。用途：排查区块/方块渲染、实体同步、坐标偏移等肉眼可见的问题。
 *
 * 用法：
 *   cd tests/e2e/bot && npm run viewer
 *   然后浏览器打开 http://localhost:3000
 *
 * 说明：
 *   - 采用 prismarine-viewer 的**浏览器模式**（express + socket.io），不依赖
 *     node-canvas-webgl 等原生模块，Windows 上可直接运行。
 *   - 视角固定在 bot 上（firstPerson），随 bot 的位置与朝向实时更新。
 *   - 按 Ctrl-C 退出，会自动停掉服务端并清理临时目录。
 */

import path from "node:path";
import { E2E_SERVER_PROFILE, WORK_ROOT } from "../src/config.ts";
import { createBot } from "../src/bot/factory.ts";
import { waitForEvent } from "../src/bot/wait.ts";
import { startCubiumServer } from "../src/servers/cubium.ts";
import { allocatePort } from "../src/servers/port.ts";

/** 调试用视距（越大渲染越慢；调试场景通常不需要远处）。 */
const VIEWER_VIEW_DISTANCE = 6;
/** 单次调试会话的自动退出时间（避免忘记关闭而长期占用端口）。 */
const SESSION_TIMEOUT_MS = 30 * 60 * 1000;

async function main(): Promise<void> {
    // 浏览器端口默认由内核分配空闲端口——固定的 3000 常被上一次会话的残留占用，
    // 会以 EADDRINUSE 失败。需要固定端口时用 MC_VIEWER_PORT 指定。
    const viewerPortEnv = process.env.MC_VIEWER_PORT;
    const viewerPort = viewerPortEnv !== undefined && viewerPortEnv.length > 0
        ? Number.parseInt(viewerPortEnv, 10)
        : await allocatePort();

    const serverPort = await allocatePort();
    const runDir = path.join(WORK_ROOT, "viewer");
    process.stdout.write(`启动 Cubium 服务端（端口 ${serverPort}）... `);
    const server = await startCubiumServer({
        runDir,
        port: serverPort,
        profile: E2E_SERVER_PROFILE,
        maxPlayers: 4,
    });
    console.log(`就绪（用时 ${server.startupMs}ms）`);

    const handle = createBot({ port: serverPort, username: "e2e_viewer", traceLimit: 5000 });
    process.stdout.write("等待 bot 进入世界... ");
    await waitForEvent(handle.bot, "spawn", {
        timeoutMs: 30_000,
        what: "spawn",
        describe: () => `已收包类型：${handle.trace.names().join(", ")}`,
    });
    console.log("已进入世界");

    // 动态导入：prismarine-viewer 仅在人工调试时使用，不作为测试运行时的依赖路径。
    const mineflayerViewer = (await import("prismarine-viewer")).mineflayer;
    mineflayerViewer(handle.bot, {
        port: viewerPort,
        firstPerson: true,
        viewDistance: VIEWER_VIEW_DISTANCE,
    });

    const position = handle.bot.entity.position;
    console.log(`\n浏览器打开：http://localhost:${viewerPort}`);
    console.log(`bot 位置：(${position.x.toFixed(1)}, ${position.y.toFixed(1)}, ${position.z.toFixed(1)})`);
    console.log(`服务端日志：${path.join(runDir, "server.log")}`);
    console.log("\n按 Ctrl-C 退出。\n");

    let shuttingDown = false;
    const shutdown = async (reason: string): Promise<void> => {
        if (shuttingDown) {
            return;
        }
        shuttingDown = true;
        console.log(`\n正在退出（${reason}）...`);
        try {
            handle.bot.viewer?.close();
        } catch {
            /* viewer 可能未启动 */
        }
        await handle.dispose();
        await server.stop();
        process.exit(0);
    };

    process.on("SIGINT", () => void shutdown("Ctrl-C"));
    process.on("SIGTERM", () => void shutdown("SIGTERM"));
    setTimeout(() => void shutdown("会话超时"), SESSION_TIMEOUT_MS);
}

await main();
