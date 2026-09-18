/*
 * vanilla.ts — 官方 vanilla 服务端适配（双跑对比的对照端）。
 *
 * 与 Cubium 的三处关键差异，全部体现在 ServerSpec 的策略字段上：
 *
 *   1. **就绪信号必须用日志**。vanilla 先开监听端口、再加载世界，TCP 可连并不代表
 *      世界已就绪（客户端此时连上会卡在加载界面）。故以 stdout 的
 *      `Done (X.XXXs)! For help, type "help"` 为主信号。
 *   2. **支持 stdin `stop` 优雅停机**（Cubium 在 Windows 上只能硬杀）。
 *   3. **工作目录跨用例共享**。vanilla 的 bundler 会按其 **cwd** 解包 30+ 个依赖 jar，
 *      若每个用例一个 cwd 就要重复解包（每次数秒）。因 runner 是串行执行，故共享同一
 *      cwd（bundler 只解包一次），靠每次改写 `server.properties` 的 `level-name` 与
 *      `server-port` 实现世界与端口隔离。世界目录用完即删。
 *
 * 地形配置必须与 Cubium 侧逐项对齐，否则区块内容无法比较：
 *   Cubium 侧是 bedrock 1 / dirt 2 / grass_block 1、biome=plains、不生成结构
 *   （generateStructures=false → structure_overrides 取空列表）。
 *   vanilla 的 minecraft:flat 预设默认会生成村庄与要塞，故此处显式传
 *   generator-settings 把 structure_overrides 置空。
 */

import fs from "node:fs";
import path from "node:path";
import { SERVER_READY_TIMEOUT_MS, vanillaServerJar, type ServerProfile } from "../config.ts";
import { vanillaCacheDir } from "./workspace.ts";
import { ServerProcess } from "./server-process.ts";

/** vanilla 启动参数。全部显式传入。 */
export interface VanillaServerOptions {
    /** 本用例的工作目录（仅用于存放日志；服务端 cwd 是共享缓存目录）。 */
    readonly runDir: string;
    /** 监听端口。 */
    readonly port: number;
    /** 世界配置档（与 Cubium 侧同一份，确保两侧地形一致）。 */
    readonly profile: ServerProfile;
    /** 最大玩家数。 */
    readonly maxPlayers: number;
    /** 本用例专属的世界名（避免与其他用例的世界目录冲突）。 */
    readonly worldName: string;
}

/** vanilla 的 flat 生成器设置：层配置与 Cubium 侧严格一致，并显式禁用结构生成。 */
function flatGeneratorSettings(): string {
    return JSON.stringify({
        layers: [
            { block: "minecraft:bedrock", height: 1 },
            { block: "minecraft:dirt", height: 2 },
            { block: "minecraft:grass_block", height: 1 },
        ],
        biome: "minecraft:plains",
        // 关键：vanilla 的 flat 预设默认生成村庄/要塞，而 Cubium 侧
        // generateStructures=false 对应「空列表 = 不生成结构」。不显式置空会导致
        // 两侧地形不一致（vanilla 侧多出建筑），区块比对必然失败。
        structure_overrides: [],
        features: false,
        lakes: false,
    });
}

/** 生成 server.properties 内容。 */
function serverProperties(options: VanillaServerOptions): string {
    // java.util.Properties 格式：值里的 `:` 与 `=` 需转义为 `\:` / `\=`。
    const entries: ReadonlyArray<readonly [string, string]> = [
        ["server-port", String(options.port)],
        ["online-mode", "false"],
        ["level-name", options.worldName],
        ["level-type", "minecraft\\:flat"],
        ["generator-settings", flatGeneratorSettings()],
        ["level-seed", options.profile.levelSeed],
        ["spawn-protection", "0"],
        ["gamemode", options.profile.gameMode],
        ["difficulty", options.profile.difficulty],
        ["view-distance", String(options.profile.viewDistance)],
        ["simulation-distance", String(options.profile.simulationDistance)],
        ["max-players", String(options.maxPlayers)],
        ["enable-command-block", "false"],
        ["allow-nether", "false"],
        ["sync-chunk-writes", "false"],
    ];
    return `${entries.map(([key, value]) => `${key}=${value}`).join("\n")}\n`;
}

/**
 * 启动一个 vanilla 服务端并等待其打印就绪日志。
 *
 * @returns 已就绪的服务端进程句柄。
 */
export async function startVanillaServer(options: VanillaServerOptions): Promise<ServerProcess> {
    const jarPath = vanillaServerJar();
    if (!fs.existsSync(jarPath)) {
        throw new Error(
            `vanilla 服务端 jar 不存在：${jarPath}\n` +
                `（预期位于 gradle 缓存中；也可用环境变量 MC_VANILLA_JAR 指定其他路径）`,
        );
    }

    const cwd = vanillaCacheDir();
    fs.mkdirSync(cwd, { recursive: true });
    // eula.txt 是 vanilla 首启的硬性前置，缺失会直接退出。
    fs.writeFileSync(path.join(cwd, "eula.txt"), "eula=true\n");
    fs.writeFileSync(path.join(cwd, "server.properties"), serverProperties(options));

    const server = new ServerProcess({
        label: `vanilla[${options.worldName}]`,
        command: "java",
        args: ["-Xmx1G", "-jar", jarPath, "nogui"],
        cwd,
        ready: {
            kind: "log",
            pattern: /Done \([\d.]+s\)! For help/,
            timeoutMs: SERVER_READY_TIMEOUT_MS,
        },
        shutdown: { kind: "stdin", command: "stop", graceMs: 20_000 },
        logPath: path.join(options.runDir, "server.log"),
    });
    await server.start();
    return server;
}

/** 删除某次运行产生的世界目录（共享 cwd 下的隔离清理）。 */
export function removeVanillaWorld(worldName: string): void {
    const worldDir = path.join(vanillaCacheDir(), worldName);
    try {
        fs.rmSync(worldDir, { recursive: true, force: true, maxRetries: 3, retryDelay: 200 });
    } catch {
        /* 残留不影响下次运行（worldName 含 runId） */
    }
}
