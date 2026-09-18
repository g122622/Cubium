/*
 * probe-raw.ts — Phase 0 协议层探针（临时脚本）。
 *
 * 绕过 mineflayer，直接用 node-minecraft-protocol 连接 Cubium 服务端，
 * 打印每个收发包的原始内容，重点是 registry_data 的 entries 结构。
 *
 * 用途：确定「registry_data 的 value 缺失」到底有没有触发 prismarine-registry 的崩溃，
 *       以及服务端在第几个 registry 之后停止发送。
 *
 * 用法：
 *   node tests/e2e/bot/tools/probe-raw.ts
 */

import { spawn, type ChildProcess } from "node:child_process";
import fs from "node:fs";
import net from "node:net";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const REPO_ROOT = path.resolve(__dirname, "..", "..", "..", "..");
const SERVER_EXE = path.join(REPO_ROOT, "build", "bin", "RelWithDebInfo", "minecraft-server.exe");
const PROBE_DIR = path.join(REPO_ROOT, "build", "e2e", "probe", "raw");
const PORT = 25998;
const SOURCE_WORLD = path.join(os.homedir(), "minecraft_reborn", "saves", "quick_play_world");

const gStartMs = Date.now();
function stamp(): string {
    return `[+${String(Date.now() - gStartMs).padStart(6, " ")}ms]`;
}

function waitForPort(port: number, timeoutMs: number): Promise<number> {
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
                if (Date.now() - startMs > timeoutMs) reject(new Error(`端口 ${port} 未就绪`));
                else setTimeout(attempt, 200);
            });
        };
        attempt();
    });
}

function prepareGameDir(): string {
    fs.rmSync(PROBE_DIR, { recursive: true, force: true });
    fs.mkdirSync(path.join(PROBE_DIR, "saves"), { recursive: true });
    fs.cpSync(SOURCE_WORLD, path.join(PROBE_DIR, "saves", "probe"), { recursive: true });

    const options = {
        network: { serverPort: PORT, bindAddress: "127.0.0.1", maxPlayers: 4, onlineMode: false, motd: "probe", p2pEnabled: false },
        world: { worldName: "probe", levelName: "probe", levelSeed: "0", levelType: "default", generateStructures: false, enableCommandBlock: false },
        game: { defaultGameMode: "creative", difficulty: "peaceful", hardcore: false, pvpEnabled: false, allowFlight: true, playerIdleTimeout: 0, tickRate: 20 },
        performance: { viewDistance: 4, simulationDistance: 4, chunkLoadRate: 16, maxEntitiesPerChunk: 50 },
        security: { whiteList: false, blackList: false, spawnProtection: 0, maxPacketSize: 2097152, maxTickTime: 60000 },
        log: { logLevel: "info", logToFile: false, logFile: "server.log", debugLogging: false },
        version: 1,
    };
    const configPath = path.join(PROBE_DIR, "server_options.json");
    fs.writeFileSync(configPath, JSON.stringify(options, null, 4));
    return configPath;
}

async function main(): Promise<void> {
    const configPath = prepareGameDir();

    const child: ChildProcess = spawn(SERVER_EXE, ["--config", configPath], {
        cwd: REPO_ROOT,
        stdio: ["pipe", "pipe", "pipe"],
    });
    const logLines: string[] = [];
    const logStream = fs.createWriteStream(path.join(PROBE_DIR, "server.log"), { flags: "a" });
    const record = (chunk: Buffer): void => {
        logStream.write(chunk);
        for (const line of chunk.toString("utf8").split(/\r?\n/)) if (line.trim()) logLines.push(line);
    };
    child.stdout?.on("data", record);
    child.stderr?.on("data", record);

    const startupMs = await waitForPort(PORT, 120_000);
    console.log(`${stamp()} Cubium TCP 就绪，耗时 ${startupMs} ms`);

    const mc = (await import("minecraft-protocol")).default;
    const client = mc.createClient({
        host: "127.0.0.1",
        port: PORT,
        username: "probe_raw",
        version: "1.21.11",
        auth: "offline",
        checkTimeoutInterval: 30_000,
        hideErrors: false,
    });

    let registryCount = 0;
    const registrySummary: string[] = [];
    /** 统计所有收到的包类型 -> 次数，用于确认服务端出站包的实际覆盖。 */
    const packetCounts = new Map<string, number>();

    client.on("packet", (data: Record<string, unknown>, meta: { name: string; state: string; size: number }) => {
        packetCounts.set(meta.name, (packetCounts.get(meta.name) ?? 0) + 1);
        // 逐条打印过于冗长，只打印非区块包
        if (meta.name !== "map_chunk" && meta.name !== "level_chunk_with_light") {
            console.log(`${stamp()} S2C [${meta.state}] ${meta.name} (${meta.size}B)`);
        }

        if (meta.name === "registry_data") {
            registryCount += 1;
            const id = String(data.id);
            const entries = (data.entries ?? []) as Array<{ key: string; value?: unknown }>;
            const withValue = entries.filter((e) => e.value !== undefined).length;
            const line = `  #${registryCount} id=${id} entries=${entries.length} 带value=${withValue}` +
                ` 首条=${entries.length > 0 ? JSON.stringify({ key: entries[0].key, hasValue: entries[0].value !== undefined }) : "(空)"}`;
            registrySummary.push(line);
            console.log(line);
        }
    });

    client.on("error", (err: Error) => {
        console.log(`${stamp()} [client error] ${err.constructor.name}: ${err.message}`);
    });
    client.on("end", (reason: string) => console.log(`${stamp()} [client end] ${reason}`));
    client.on("kick_disconnect", (p: unknown) => console.log(`${stamp()} [kicked] ${JSON.stringify(p)}`));

    process.on("uncaughtException", (err) => {
        console.log(`${stamp()} [uncaughtException] ${err.constructor.name}: ${err.message}`);
        console.log(err.stack?.split("\n").slice(0, 8).join("\n"));
        finish();
    });

    function finish(): void {
        console.log("\n================ 协议层探针结论 ================");
        console.log(`收到的 registry_data 数量: ${registryCount}`);
        for (const s of registrySummary) console.log(s);
        console.log("------------------------------------------");
        console.log("收到的 S2C 包类型统计（按次数降序）:");
        const sorted = [...packetCounts.entries()].sort((a, b) => b[1] - a[1]);
        for (const [name, count] of sorted) console.log(`  ${String(count).padStart(6)}  ${name}`);
        console.log("------------------------------------------");
        console.log("mineflayer spawn 依赖的关键包是否出现:");
        for (const key of ["update_health", "set_health", "login", "position", "keep_alive", "player_info", "commands", "game_state_change"]) {
            const n = packetCounts.get(key) ?? 0;
            console.log(`  ${key.padEnd(20)} ${n > 0 ? `✓ ${n} 次` : "✗ 未出现"}`);
        }
        console.log("------------------------------------------");
        console.log("服务端日志尾部 8 行:");
        console.log(logLines.slice(-8).map((l) => `  ${l}`).join("\n"));
        console.log("================================================\n");
        child.kill();
        setTimeout(() => {
            fs.rmSync(PROBE_DIR, { recursive: true, force: true });
            process.exit(0);
        }, 500);
    }

    setTimeout(() => {
        console.log(`${stamp()} 30 秒超时，强制收尾`);
        finish();
    }, 30_000);
}

await main();
