/*
 * config.ts — e2e harness 的全部常量与路径推导。
 *
 * 约定：本文件只放「不随用例变化」的量。任何随用例变化的量（端口、目录、用例名）
 * 一律由 runner 显式传参，不在此处设默认值——避免数据流被多层默认值掩盖。
 */

import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));

/** 仓库根目录（tests/e2e/bot/src → 上溯四级）。 */
export const REPO_ROOT = path.resolve(__dirname, "..", "..", "..", "..");

/** Cubium 服务端可执行文件。 */
export const CUBIUM_SERVER_EXE = path.join(REPO_ROOT, "build", "bin", "RelWithDebInfo", "minecraft-server.exe");

/** 每次运行的工作根目录（隔离目录、日志、诊断产物都在此下）。 */
export const WORK_ROOT = path.join(REPO_ROOT, "build", "e2e");

/** 基线目录。 */
export const BASELINE_DIR = path.join(REPO_ROOT, "tests", "e2e", "bot", "baselines");

/** 目标 Minecraft 版本，须与 Cubium 的 kJavaProtocolVersion(774) 精确匹配。 */
export const MC_VERSION = "1.21.11";

/** 协议号，写入基线 meta 用于失效判定。 */
export const PROTOCOL_VERSION = 774;

/** 基线 schema 版本。字段结构变更时递增，旧基线将被拒绝比对。 */
export const BASELINE_SCHEMA_VERSION = 1;

/** 服务端就绪探测：TCP 轮询间隔与总超时。 */
export const SERVER_READY_POLL_MS = 200;
export const SERVER_READY_TIMEOUT_MS = 120_000;

/** 服务端优雅停止等待时间（超时后硬杀）。 */
export const SERVER_STOP_GRACE_MS = 3_000;

/** 单个用例的默认超时（毫秒）。 */
export const CASE_TIMEOUT_MS = 90_000;

/** bot 连接超时（传给 mineflayer 的 checkTimeoutInterval）。 */
export const BOT_TIMEOUT_MS = 30_000;

/** 等待 spawn 的上限。 */
export const SPAWN_TIMEOUT_MS = 30_000;

/** 保留的历史诊断目录数量上限。 */
export const ARTIFACTS_KEEP = 10;

/** e2e 专用服务端配置（world.viewDistance 等）。 */
export interface ServerProfile {
    /** 世界名（同时作为 saves/ 下的目录名）。 */
    readonly worldName: string;
    /** 模拟距离。 */
    readonly simulationDistance: number;
    /** 视距。区块发送为每 tick 全量抽干，降低视距可显著加快 spawn。 */
    readonly viewDistance: number;
    /** 世界类型：e2e 用 flat，因其地形可逐格预测。 */
    readonly levelType: string;
    /** 世界种子。 */
    readonly levelSeed: string;
    /** 游戏模式。 */
    readonly gameMode: string;
    /** 难度。 */
    readonly difficulty: string;
}

/**
 * e2e 使用的服务端配置档。
 *
 * viewDistance=4（而非默认 6）：区块发送是每 tick 全量抽干（ChunkSendManager::processPendingSends），
 * 视距直接决定 spawn 期间的发包量，4→81 列是实测可接受的量级。
 * gameMode=creative：挖掘瞬破，使方块交互用例不受「update_attributes(cb 129) 未实现」影响
 * （该包提供 block_break_speed，缺失会让生存模式挖掘速度退化）。
 */
export const E2E_SERVER_PROFILE: ServerProfile = {
    worldName: "e2e",
    simulationDistance: 4,
    viewDistance: 4,
    levelType: "flat",
    levelSeed: "0",
    gameMode: "creative",
    difficulty: "peaceful",
};

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
        "1.21.11",
        "minecraft-server.jar",
    );
}
