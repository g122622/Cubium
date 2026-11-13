/*
 * config.ts — e2e 两侧共享的常量与路径推导。
 *
 * 只放「不随用例变化」且 Cubium/vanilla 两侧都要用的量；各 harness 自己的配置
 * （基线目录、世界档、用例超时）留在各自目录的 config.ts 里，再从本文件 re-export。
 */

import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));

/** 仓库根目录（tests/e2e/shared → 上溯三级）。 */
export const REPO_ROOT = path.resolve(__dirname, "..", "..", "..");

/**
 * Cubium 服务端可执行文件。
 *
 * 按平台补后缀：本仓库使用 Ninja Multi-Config 生成器，本地开发从
 * `build/bin/RelWithDebInfo/` 定位；CI 可显式传入隔离构建的绝对路径。
 */
export const CUBIUM_SERVER_EXE = process.env.CUBIUM_SERVER_EXE !== undefined
    ? path.resolve(process.env.CUBIUM_SERVER_EXE)
    : path.join(
        REPO_ROOT,
        "build",
        "bin",
        "RelWithDebInfo",
        process.platform === "win32" ? "minecraft-server.exe" : "minecraft-server",
    );

/** 每次运行的工作根目录（隔离目录、日志、诊断产物都在此下）。 */
export const WORK_ROOT = path.join(REPO_ROOT, "build", "e2e");

/** 目标 Minecraft 版本，须与 Cubium 的协议版本精确匹配。 */
export const MC_VERSION = "1.21.11";

/** 协议号。 */
export const PROTOCOL_VERSION = 774;

/** 服务端就绪探测：TCP 轮询间隔与总超时。 */
export const SERVER_READY_POLL_MS = 200;
export const SERVER_READY_TIMEOUT_MS = 120_000;

/** 服务端优雅停止等待时间（超时后硬杀）。 */
export const SERVER_STOP_GRACE_MS = 3_000;

/** bot 连接超时（传给 mineflayer 的 checkTimeoutInterval）。 */
export const BOT_TIMEOUT_MS = 30_000;

/** 保留的历史诊断目录数量上限。 */
export const ARTIFACTS_KEEP = 10;

/** 服务端种类。 */
export type ServerKind = "cubium" | "vanilla";

/** e2e 服务端配置档。 */
export interface ServerProfile {
    /** 世界名（同时作为 saves/ 下的目录名）。 */
    readonly worldName: string;
    /** 模拟距离。 */
    readonly simulationDistance: number;
    /** 视距。 */
    readonly viewDistance: number;
    /** 世界类型。 */
    readonly levelType: string;
    /** 世界种子。 */
    readonly levelSeed: string;
    /** 游戏模式。 */
    readonly gameMode: string;
    /** 难度。 */
    readonly difficulty: string;
}

/** 官方 vanilla 1.21.11 服务端 jar（本地缓存，双跑对比用）。可用环境变量覆盖。 */
export function vanillaServerJar(): string {
    const fromEnv = process.env.MC_VANILLA_JAR;
    if (fromEnv !== undefined && fromEnv.length > 0) {
        return fromEnv;
    }
    return path.join(
        os.homedir(),
        ".gradle",
        "caches",
        "fabric-loom",
        MC_VERSION,
        "minecraft-server.jar",
    );
}
