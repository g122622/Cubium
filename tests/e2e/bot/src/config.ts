/*
 * config.ts — e2e/bot harness 的专属配置。
 *
 * 两侧共享的常量（仓库根、Cubium exe、vanilla jar、协议版本、超时）统一放在
 * tests/e2e/shared/config.ts；本文件只保留 bot harness 自己的基线与用例配置，
 * 并 re-export 共享常量，让 bot 内部现有的 `../config.ts` 引用保持不变。
 */

import path from "node:path";

import { REPO_ROOT } from "../../shared/config.ts";
import type { ServerProfile as SharedServerProfile } from "../../shared/config.ts";

export { REPO_ROOT };
export {
    ARTIFACTS_KEEP,
    BOT_TIMEOUT_MS,
    CUBIUM_SERVER_EXE,
    MC_VERSION,
    PROTOCOL_VERSION,
    SERVER_READY_POLL_MS,
    SERVER_READY_TIMEOUT_MS,
    SERVER_STOP_GRACE_MS,
    WORK_ROOT,
    vanillaServerJar,
} from "../../shared/config.ts";
export type { ServerKind, ServerProfile } from "../../shared/config.ts";

/** 基线目录。 */
export const BASELINE_DIR = path.join(REPO_ROOT, "tests", "e2e", "bot", "baselines");

/** 基线 schema 版本。字段结构变更时递增，旧基线将被拒绝比对。 */
export const BASELINE_SCHEMA_VERSION = 1;

/**
 * 单个用例的默认超时（毫秒）。
 *
 * 取 240 秒而不是更小：用例体内可能包含**一次服务端重启**（持久化用例要验证「落盘 → 重启 →
 * 读回」，而本机一次完整启动约 70~90 秒），90 秒的旧值会让这类用例在重启尚未完成时就超时。
 * 这个超时只用于兜住「用例挂死」，不用于度量性能。
 */
export const CASE_TIMEOUT_MS = 240_000;

/** 等待 spawn 的上限。 */
export const SPAWN_TIMEOUT_MS = 30_000;

/**
 * e2e 使用的服务端配置档。
 *
 * viewDistance=4（而非默认 6）：区块发送是每 tick 全量抽干（ChunkSendManager::processPendingSends），
 * 视距直接决定 spawn 期间的发包量，4→81 列是实测可接受的量级。
 * gameMode=creative：挖掘瞬破，使方块交互用例不受「block_break_speed 属性尚未实现」影响
 * ——该属性决定挖掘耗时，缺了它生存模式下的挖掘速度会退化。
 * TODO: 属性同步包（update_attributes）本身已实现，但属性集合里还没有 block_break_speed
 *       （连同其挖掘耗时消费点）。补齐后这些用例应改回 survival 模式。
 */
export const E2E_SERVER_PROFILE: SharedServerProfile = {
    worldName: "e2e",
    simulationDistance: 4,
    viewDistance: 4,
    levelType: "flat",
    levelSeed: "0",
    gameMode: "creative",
    difficulty: "peaceful",
};
