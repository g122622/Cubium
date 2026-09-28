/*
 * ops.ts — 把 e2e 的 bot 写进服务端的 OP 列表。
 *
 * 少数用例需要服务端执行命令（非主世界维度里的容器、重启前显式落盘等），而命令需要 OP。
 * ops.json 必须在**服务端启动之前**写好——两侧都在启动时读取它，登录时按 UUID 查权限。
 *
 * 两处必须注意的差异：
 *   1. 条目结构两侧一致：`{uuid, name, level, bypassesPlayerLimit}` 的数组。
 *   2. **UUID 字符串格式两侧不同**，且两侧都是精确字符串比较：
 *      - vanilla 用带连字符的标准格式（`xxxxxxxx-xxxx-...`）；
 *      - Cubium 的 `PlayerManager` 存的是 `util::uuidToString()` —— 32 位小写、**无连字符**，
 *        而 `OpListManager::isOp()` 直接比较字符串，格式不对就不生效。
 *      故按服务端种类选择格式，而不是「统一写成带连字符」。
 */

import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";
import type { ServerKind } from "../case.ts";

/** OP 等级：4（Owner），保证 /execute in、/setblock、/fill、/save-all 等全部可用。 */
const OP_LEVEL = 4;

/** 单个 OP 条目。 */
interface OpEntry {
    readonly uuid: string;
    readonly name: string;
    readonly level: number;
    readonly bypassesPlayerLimit: boolean;
}

/**
 * 由用户名派生离线模式 UUID。
 *
 * 算法与两侧服务端一致（Java 的 `UUID.nameUUIDFromBytes(("OfflinePlayer:" + name))`）：
 * MD5 摘要 → 第 6 字节高 4 位置版本号 3 → 第 8 字节高 2 位置 RFC 4122 变体。
 *
 * @param username 玩家名。
 * @param withDashes 是否输出带连字符的标准格式（vanilla 为 true，Cubium 为 false）。
 * @returns UUID 字符串。
 */
export function offlineUuid(username: string, withDashes: boolean): string {
    const digest = crypto.createHash("md5").update(`OfflinePlayer:${username}`, "utf8").digest();
    digest[6] = ((digest[6] ?? 0) & 0x0f) | 0x30;
    digest[8] = ((digest[8] ?? 0) & 0x3f) | 0x80;
    const hex = digest.toString("hex");
    if (!withDashes) {
        return hex;
    }
    return [hex.slice(0, 8), hex.slice(8, 12), hex.slice(12, 16), hex.slice(16, 20), hex.slice(20, 32)].join("-");
}

/**
 * 写出 ops.json。
 *
 * 直接整体覆写：该文件由 harness 独占（Cubium 在每个用例的独立游戏目录里，vanilla 在
 * 共享的解包缓存目录里且每次启动前重写），不存在需要保留的第三方条目。
 *
 * @param opsPath ops.json 的绝对路径。
 * @param usernames 需要授予 OP 的玩家名。
 * @param serverKind 服务端种类（决定 UUID 格式）。
 */
export function writeOpsFile(opsPath: string, usernames: readonly string[], serverKind: ServerKind): void {
    const entries: OpEntry[] = usernames.map((username) => ({
        uuid: offlineUuid(username, serverKind === "vanilla"),
        name: username,
        level: OP_LEVEL,
        bypassesPlayerLimit: false,
    }));
    fs.mkdirSync(path.dirname(opsPath), { recursive: true });
    fs.writeFileSync(opsPath, `${JSON.stringify(entries, null, 2)}\n`);
}
