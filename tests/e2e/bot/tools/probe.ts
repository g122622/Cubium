/*
 * probe.ts — Phase 0 探针（临时脚本，用于确定 e2e 方案的可行性边界）。
 *
 * 验证三件事：
 *   1. Cubium 独立服能否从「临时游戏目录 + 显式 --config」启动（Phase 0.6）
 *   2. Cubium 启动到 TCP 可连的耗时（Phase 0.4，决定「每用例一进程」是否可行）
 *   3. mineflayer 作为真实 wire 客户端连接时的实际崩溃点（Phase 0.2）
 *
 * 用法（node >= 22 原生 type stripping，ESM，零外部依赖）：
 *   node tests/e2e/bot/tools/probe.ts
 *
 * 注意：探针会把一个已有世界**复制**到临时目录再启动，不会污染用户存档。
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
const PROBE_DIR = path.join(REPO_ROOT, "build", "e2e", "probe", "cubium");
const PORT = 25999;
const WORLD_NAME = "probe";

/** 用于复制出探针世界的源世界（只读，不会被修改）。 */
const SOURCE_WORLD = path.join(os.homedir(), "minecraft_reborn", "saves", "quick_play_world");

/** 轮询等待端口可连，返回从开始到连上的毫秒数。 */
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
                if (Date.now() - startMs > timeoutMs) {
                    reject(new Error(`端口 ${port} 在 ${timeoutMs}ms 内未就绪`));
                } else {
                    setTimeout(attempt, 250);
                }
            });
        };
        attempt();
    });
}

/** 把秒级时间戳格式化为便于阅读的相对时间。 */
function stamp(): string {
    return `[+${String(Date.now() - gStartMs).padStart(6, " ")}ms]`;
}

const gStartMs = Date.now();

/** 构造临时的游戏目录（含 server_options.json；**不预置世界**，验证服务端自建世界的能力）。 */
function prepareGameDir(): string {
    fs.rmSync(PROBE_DIR, { recursive: true, force: true });
    fs.mkdirSync(PROBE_DIR, { recursive: true });

    const options = {
        network: {
            serverPort: PORT,
            bindAddress: "127.0.0.1",
            maxPlayers: 4,
            onlineMode: false,
            motd: "cubium-e2e-probe",
            p2pEnabled: false,
        },
        world: {
            worldName: WORLD_NAME,
            levelName: "probe",
            levelSeed: "0",
            levelType: "flat",
            generateStructures: false,
            enableCommandBlock: false,
        },
        game: {
            defaultGameMode: "creative",
            difficulty: "peaceful",
            hardcore: false,
            pvpEnabled: false,
            allowFlight: true,
            playerIdleTimeout: 0,
            tickRate: 20,
        },
        performance: {
            viewDistance: 4,
            simulationDistance: 4,
            chunkLoadRate: 16,
            maxEntitiesPerChunk: 50,
        },
        security: {
            whiteList: false,
            blackList: false,
            spawnProtection: 0,
            maxPacketSize: 2097152,
            maxTickTime: 60000,
        },
        log: { logLevel: "info", logToFile: false, logFile: "server.log", debugLogging: false },
        version: 1,
    };

    const configPath = path.join(PROBE_DIR, "server_options.json");
    fs.writeFileSync(configPath, JSON.stringify(options, null, 4));
    console.log(`${stamp()} 已写入配置: ${configPath}`);
    return configPath;
}

async function main(): Promise<void> {
    if (!fs.existsSync(SERVER_EXE)) {
        throw new Error(`服务端可执行文件不存在: ${SERVER_EXE}`);
    }

    const configPath = prepareGameDir();

    console.log(`${stamp()} 启动 Cubium 服务端...`);
    const child: ChildProcess = spawn(SERVER_EXE, ["--config", configPath], {
        cwd: REPO_ROOT,
        stdio: ["pipe", "pipe", "pipe"],
    });

    const logLines: string[] = [];
    const logStream = fs.createWriteStream(path.join(PROBE_DIR, "server.log"), { flags: "a" });
    const record = (chunk: Buffer): void => {
        logStream.write(chunk);
        for (const line of chunk.toString("utf8").split(/\r?\n/)) {
            if (line.trim().length > 0) logLines.push(line);
        }
    };
    child.stdout?.on("data", record);
    child.stderr?.on("data", record);

    let exited = false;
    child.on("exit", (code, signal) => {
        exited = true;
        console.log(`${stamp()} 服务端进程退出: code=${code} signal=${signal}`);
    });

    // ---- 1. 测量启动耗时 ----
    let startupMs: number;
    try {
        startupMs = await waitForPort(PORT, 120_000);
        console.log(`${stamp()} TCP 可连，启动耗时 = ${startupMs} ms`);
    } catch (err) {
        console.log(`${stamp()} 启动失败: ${(err as Error).message}`);
        console.log("---- 服务端日志尾部 ----");
        console.log(logLines.slice(-40).join("\n"));
        child.kill();
        process.exit(1);
    }

    // ---- 2. 连接 bot，观察实际崩溃点 ----
    console.log(`${stamp()} 创建 mineflayer bot...`);
    const mineflayer = (await import("mineflayer")).default;

    let sawError: Error | null = null;
    let sawKicked: unknown = null;
    let sawSpawn = false;

    process.on("uncaughtException", (err) => {
        sawError = err;
        console.log(`${stamp()} [uncaughtException] ${err.constructor.name}: ${err.message}`);
        report();
    });

    const bot = mineflayer.createBot({
        host: "127.0.0.1",
        port: PORT,
        username: "e2e_probe",
        version: "1.21.11",
        auth: "offline",
        checkTimeoutInterval: 30_000,
        hideErrors: false,
    });

    // 记录所有收发的包，用于判断服务端到底走到了哪一步、卡在哪里
    const packetLog: string[] = [];
    bot._client.on("packet", (data: unknown, meta: { name: string; state: string; size: number }) => {
        const entry = `S2C [${meta.state}] ${meta.name} (${meta.size}B)`;
        packetLog.push(entry);
        console.log(`${stamp()} ${entry}`);
    });
    bot._client.on("write", (data: unknown, meta: { name: string; state: string }) => {
        const entry = `C2S [${meta.state}] ${meta.name}`;
        packetLog.push(entry);
        console.log(`${stamp()} ${entry}`);
    });

    // 该监听器注册在 mineflayer 内部监听器之后。
    // 若它不执行，即证明 mineflayer 的 registry_data 监听器抛错并中断了事件链。
    bot._client.on("registry_data", (packet: { id?: string }) => {
        console.log(`${stamp()} [后注册监听器] registry_data 执行了 (id=${packet?.id})`);
    });
    bot._client.on("error", (err: Error) => {
        console.log(`${stamp()} [client error] ${err.constructor.name}: ${err.message}`);
    });
    bot._client.on("end", (reason: string) => console.log(`${stamp()} [client end] ${reason}`));
    bot._client.on("close", () => console.log(`${stamp()} [client close]`));

    bot.on("spawn", async () => {
        sawSpawn = true;
        console.log(`${stamp()} [事件] spawn —— bot 已进入世界`);
        try {
            const Vec3 = (await import("vec3")).default;
            const p = bot.entity.position;
            const bx = Math.floor(p.x);
            const bz = Math.floor(p.z);

            // 轮询等待出生点列可读。不用 bot.waitForChunksToLoad()：它要等满 viewDistance
            // 覆盖的全部列（4 → 81 列），在测试里过慢且可能长时间挂起；出生点所在列通常先到。
            let surfaceY = -1;
            for (let attempt = 0; attempt < 150 && surfaceY < 0; attempt += 1) {
                for (let y = 20; y >= -70; y -= 1) {
                    const b = bot.blockAt(new Vec3(bx, y, bz));
                    if (b && !b.name.endsWith("air")) {
                        surfaceY = y;
                        break;
                    }
                }
                if (surfaceY < 0) await new Promise((r) => setTimeout(r, 100));
            }
            if (surfaceY < 0) {
                console.log(`${stamp()} ✗ 未找到地表方块（列 ${bx},${bz} 全空气，等待 15s）`);
                return;
            }
            console.log(`${stamp()} 地表 Y = ${surfaceY}`);
            const layers: string[] = [];
            for (let i = 0; i <= 4; i += 1) {
                const b = bot.blockAt(new Vec3(bx, surfaceY - i, bz));
                layers.push(b ? b.name : "<null>");
            }
            console.log(`${stamp()} 地表向下分层: ${layers.join(" / ")}`);
            const above = bot.blockAt(new Vec3(bx, surfaceY + 1, bz));
            console.log(`${stamp()} 地表上方一格: ${above ? above.name : "<null>"}`);

            // 注意：mineflayer 的 block.name 不带命名空间前缀（"grass_block" 而非
            // "minecraft:grass_block"），与 bot.blockAt().biome.name 等字段的约定不同。
            const expected = ["grass_block", "dirt", "dirt", "bedrock"];
            const actual = layers.slice(0, 4);
            const layersOk = JSON.stringify(actual) === JSON.stringify(expected);
            console.log(`${stamp()} 分层断言: ${layersOk ? "✓ 通过" : `✗ 失败（期望 ${expected.join("/")}）`}`);

            // biome 读取（验证 biome palette 解码）
            const surfBlock = bot.blockAt(new Vec3(bx, surfaceY, bz));
            const registry = bot.registry as unknown as { biomesArray?: unknown[]; biomes?: unknown };
            console.log(`${stamp()} biome 诊断:`);
            console.log(`  registry.biomesArray 长度 = ${registry.biomesArray?.length ?? "undefined"}`);
            console.log(`  registry.biomesArray[40] = ${JSON.stringify(registry.biomesArray?.[40])?.slice(0, 120)}`);
            console.log(`  registry.biomes 类型 = ${Array.isArray(registry.biomes) ? "array" : typeof registry.biomes}`);
            const rawBiome = (surfBlock as unknown as { biome?: unknown })?.biome;
            console.log(`  block.biome 原始值 = ${JSON.stringify(rawBiome)?.slice(0, 200)}`);
            console.log(`  block.biome.name = ${String((rawBiome as { name?: unknown })?.name)}`);
            console.log(`${stamp()} bot.game: minY=${bot.game.minY} height=${bot.game.height} dim=${bot.game.dimension}`);
            console.log(`${stamp()} bot.entity.position = ${p.x.toFixed(2)}, ${p.y.toFixed(2)}, ${p.z.toFixed(2)}`);
            console.log(`${stamp()} bot.health=${bot.health} food=${bot.food} onGround=${bot.entity.onGround}`);
            console.log(`${stamp()} tab list 玩家数 = ${Object.keys(bot.players).length}`);
        } catch (e) {
            console.log(`${stamp()} ✗ 探测失败: ${(e as Error).message}`);
        }
        setTimeout(report, 2000);
    });
    bot.on("error", (err: Error) => {
        sawError = err;
        console.log(`${stamp()} [事件] error: ${err.constructor.name}: ${err.message}`);
    });
    bot.on("kicked", (reason: unknown) => {
        sawKicked = reason;
        console.log(`${stamp()} [事件] kicked: ${JSON.stringify(reason)}`);
    });
    bot.on("end", (reason: string) => {
        console.log(`${stamp()} [事件] end: ${reason}`);
        report();
    });

    function report(): void {
        console.log("\n================ 探针结论 ================");
        console.log(`Cubium 启动耗时      : ${startupMs} ms`);
        console.log(`bot spawn            : ${sawSpawn}`);
        console.log(`bot error            : ${sawError ? `${sawError.constructor.name}: ${sawError.message}` : "无"}`);
        console.log(`bot kicked           : ${sawKicked ? JSON.stringify(sawKicked) : "无"}`);
        console.log("------------------------------------------");
        console.log(`包序列 (${packetLog.length} 条):`);
        console.log(packetLog.map((l) => `  ${l}`).join("\n"));
        console.log("------------------------------------------");
        console.log("服务端日志中的警告/错误:");
        const bad = logLines.filter((l) => /\[(warning|error|critical)\]/i.test(l));
        console.log(bad.length === 0 ? "  (无)" : bad.map((l) => `  ${l}`).join("\n"));
        console.log("------------------------------------------");
        console.log("服务端日志中的关键行:");
        for (const line of logLines) {
            if (/joined the game|intention|handshake|rejected|dispatch failed|Player '|listening|RegistryData|registry/i.test(line)) {
                console.log(`  ${line}`);
            }
        }
        console.log("------------------------------------------");
        console.log("服务端日志尾部 25 行:");
        console.log(logLines.slice(-25).map((l) => `  ${l}`).join("\n"));
        console.log("==========================================\n");
        child.kill();
        setTimeout(() => {
            fs.rmSync(PROBE_DIR, { recursive: true, force: true });
            process.exit(0);
        }, 500);
    }

    // 兜底超时
    setTimeout(() => {
        if (!exited) {
            console.log(`${stamp()} 40 秒超时，强制收尾`);
            report();
        }
    }, 40_000);
}

await main();
