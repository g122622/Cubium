/*
 * config.ts — 指令端到端 diff 的专属配置。
 *
 * 两侧共享的常量与路径统一放在 tests/e2e/shared/config.ts；本文件只保留 command
 * harness 自己的世界档、棋盘与超时，并 re-export 共享常量。
 */

import path from "node:path";

import { WORK_ROOT } from "../../shared/config.ts";
import type { ServerProfile } from "../../shared/config.ts";

export {
    ARTIFACTS_KEEP,
    BOT_TIMEOUT_MS,
    CUBIUM_SERVER_EXE,
    MC_VERSION,
    PROTOCOL_VERSION,
    REPO_ROOT,
    SERVER_READY_TIMEOUT_MS,
    SERVER_STOP_GRACE_MS,
    WORK_ROOT,
    vanillaServerJar,
} from "../../shared/config.ts";
export type { ServerKind, ServerProfile } from "../../shared/config.ts";

/** command harness 的工作根目录。 */
export const COMMAND_WORK_ROOT = path.join(WORK_ROOT, "command");

/** 两侧共用的世界配置：flat、创造、和平、种子 0。 */
export const COMMAND_PROFILE: ServerProfile = {
    worldName: "command",
    simulationDistance: 4,
    viewDistance: 4,
    levelType: "flat",
    levelSeed: "0",
    gameMode: "creative",
    difficulty: "peaceful",
};

/** bot 用户名（离线模式）。两侧必须一致，ops.json 按它授权。 */
export const COMMAND_BOT_USERNAME = "e2e_cmd";

/** 单侧允许的最大玩家数。 */
export const COMMAND_MAX_PLAYERS = 2;

/** 等待 bot spawn 的上限。 */
export const COMMAND_SPAWN_TIMEOUT_MS = 30_000;

/** 单条指令等待反馈的上限；超时表示该指令没有下发聊天反馈。 */
export const COMMAND_FEEDBACK_TIMEOUT_MS = 1_500;

/**
 * 反馈静默阈值：收到消息后连续这么久没有新消息，就认为本指令的反馈已结束。
 *
 * 必须大于一个 tick（50ms）：一条指令的多行反馈可能跨 tick 到达；又必须远小于
 * 下一条指令的发送间隔，否则会把延迟消息算进下一条。
 */
export const COMMAND_FEEDBACK_QUIET_MS = 120;

/** 单条用例的上限（只用于兜住挂死）。 */
export const COMMAND_CASE_TIMEOUT_MS = 60_000;

/** 棋盘基准 Y：放在两侧都存在的空中高度，避开 Cubium(Y=0) 与 vanilla(Y=-64) 的地形差。 */
export const BOARD_BASE_Y = 100;

/** 单个用例棋盘的边长（X/Z）。 */
export const BOARD_SIZE = 32;
