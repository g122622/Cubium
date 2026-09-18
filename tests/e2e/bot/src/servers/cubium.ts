/*
 * cubium.ts — Cubium 服务端适配。
 *
 * 启动方式：临时游戏目录 + 显式 --config。
 * 之所以必须显式传 --config 而不是用默认配置：GameDirectory::fromConfigPath 取
 * **配置文件所在目录**为游戏目录，因此「把配置写到哪」就等价于「用哪个游戏目录」，
 * 这是实现每用例世界隔离的关键（世界落在 <runDir>/saves/ 下）。
 *
 * 服务端会在世界目录不存在时按 levelType 自动新建世界（StandaloneServer 内），
 * 故 harness 无需预置世界，用完直接删目录即可。
 */

import path from "node:path";
import fs from "node:fs";
import { CUBIUM_SERVER_EXE, SERVER_READY_TIMEOUT_MS, type ServerProfile } from "../config.ts";
import { ServerProcess } from "./server-process.ts";

/** Cubium 启动参数。全部显式传入，不设默认值。 */
export interface CubiumServerOptions {
    /** 本用例独占的运行目录（游戏目录 = 此目录）。 */
    readonly runDir: string;
    /** 监听端口。 */
    readonly port: number;
    /** 世界与游戏配置档。 */
    readonly profile: ServerProfile;
    /** 最大玩家数。 */
    readonly maxPlayers: number;
}

/** 写出 e2e 专用的 server_options.json，返回其路径。 */
function writeServerOptions(options: CubiumServerOptions): string {
    const serverOptions = {
        network: {
            serverPort: options.port,
            bindAddress: "127.0.0.1",
            maxPlayers: options.maxPlayers,
            onlineMode: false,
            motd: "cubium-e2e",
            p2pEnabled: false,
        },
        world: {
            worldName: options.profile.worldName,
            levelName: "e2e",
            levelSeed: options.profile.levelSeed,
            levelType: options.profile.levelType,
            generateStructures: false,
            enableCommandBlock: false,
        },
        game: {
            defaultGameMode: options.profile.gameMode,
            difficulty: options.profile.difficulty,
            hardcore: false,
            pvpEnabled: false,
            allowFlight: true,
            playerIdleTimeout: 0,
            tickRate: 20,
        },
        performance: {
            viewDistance: options.profile.viewDistance,
            simulationDistance: options.profile.simulationDistance,
            chunkLoadRate: 16,
            maxEntitiesPerChunk: 50,
        },
        // spawnProtection=0：否则出生点保护会拦截方块交互（用方块交互用例的隐形杀手）。
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

    const configPath = path.join(options.runDir, "server_options.json");
    fs.mkdirSync(options.runDir, { recursive: true });
    fs.writeFileSync(configPath, JSON.stringify(serverOptions, null, 4));
    return configPath;
}

/**
 * 启动一个 Cubium 服务端并等待 TCP 就绪。
 *
 * @param options 启动参数。
 * @returns 已就绪的服务端进程句柄。调用方负责在结束时 stop()。
 */
export async function startCubiumServer(options: CubiumServerOptions): Promise<ServerProcess> {
    const configPath = writeServerOptions(options);
    const server = new ServerProcess({
        label: `cubium[${path.basename(options.runDir)}]`,
        command: CUBIUM_SERVER_EXE,
        args: ["--config", configPath],
        cwd: options.runDir,
        ready: { kind: "tcp", port: options.port, timeoutMs: SERVER_READY_TIMEOUT_MS },
        shutdown: { kind: "kill" },
        logPath: path.join(options.runDir, "server.log"),
    });
    await server.start();
    return server;
}
