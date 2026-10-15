import fs from "node:fs";
import path from "node:path";
import { ServerProcess } from "../../e2e/bot/src/servers/server-process.ts";
import { allocateVerifiedPort } from "../../e2e/bot/src/servers/port.ts";
import { writeOpsFile } from "../../e2e/bot/src/servers/ops.ts";
import type { Options } from "./config.ts";

export { ServerProcess };

/** 复用 e2e 的进程清理、端口探测和 OP 规则；每个用例创建独立游戏目录。 */
export async function prepareServer(options: Options, directory: string, skipMovementSpeedCheck: boolean): Promise<{ server: ServerProcess; port: number }> {
    const port = await allocateVerifiedPort(20, 500);
    fs.mkdirSync(directory, { recursive: true });
    writeOpsFile(path.join(directory, "ops.json"), ["StressControl"], "cubium");
    const configPath = path.join(directory, "server_options.json");
    fs.writeFileSync(configPath, JSON.stringify({
        network: {
            serverPort: port, bindAddress: "127.0.0.1", maxPlayers: Math.max(options.players + 1, 2),
            onlineMode: false, motd: "cubium-stress", p2pEnabled: false,
        },
        world: {
            worldName: "stress", levelName: "stress", levelSeed: options.seed,
            levelType: options.world, generateStructures: true, enableCommandBlock: false,
        },
        game: {
            defaultGameMode: "creative", difficulty: "peaceful", hardcore: false,
            pvpEnabled: false, allowFlight: true, playerIdleTimeout: 0, tickRate: 20,
        },
        performance: {
            viewDistance: options.viewDistance, simulationDistance: options.simulationDistance,
            chunkLoadRate: 16, maxEntitiesPerChunk: 50,
        },
        security: {
            whiteList: false, blackList: false, spawnProtection: 0,
            maxPacketSize: 2097152, maxTickTime: 60000,
        },
        log: { logLevel: "info", logToFile: false, logFile: "server.log", debugLogging: false },
        version: 1,
    }, null, 2));
    return {
        port,
        server: new ServerProcess({
            label: `stress[${path.basename(directory)}]`, command: options.server,
            args: ["--config", configPath, "--profiler_enabled=false",
                "--stress_metrics_file", path.join(directory, "server-metrics.csv"),
                `--stress_skip_movement_speed_check=${skipMovementSpeedCheck}`],
            cwd: directory,
            ready: {
                kind: "log", pattern: /Server ready: accepting connections on port/,
                timeoutMs: 120_000, requirePort: port,
            },
            shutdown: { kind: "kill" }, logPath: path.join(directory, "server.log"),
        }),
    };
}
