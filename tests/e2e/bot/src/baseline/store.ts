/*
 * store.ts — 基线的读写与失效判定。
 *
 * 基线是**冻结产物**（提交进 git），这是「日常只跑 Cubium 并与基线比对」成立的前提。
 * schemaVersion 或协议号不匹配时拒绝比对而不是静默产生一堆假失败——那会让人以为
 * 功能坏了，实际只是基线过期。
 */

import fs from "node:fs";
import path from "node:path";
import { BASELINE_SCHEMA_VERSION, MC_VERSION, PROTOCOL_VERSION } from "../config.ts";
import type { CaseSnapshot } from "../bot/snapshot.ts";
import type { ServerKind } from "../case.ts";

/** 基线中单条用例的记录。 */
export interface BaselineEntry {
    readonly snapshot: CaseSnapshot;
    readonly metrics: Record<string, number>;
}

/** 一份基线文件。 */
export interface Baseline {
    readonly schemaVersion: number;
    readonly meta: {
        readonly server: ServerKind;
        readonly mcVersion: string;
        readonly protocolVersion: number;
        readonly frozenAt: string;
        readonly gitCommit: string;
        readonly note: string;
    };
    readonly cases: Record<string, BaselineEntry>;
}

/** 基线读取结果。 */
export type BaselineLoadResult =
    | { readonly kind: "ok"; readonly baseline: Baseline }
    | { readonly kind: "missing" }
    | { readonly kind: "stale"; readonly reason: string };

/** 读取基线文件。 */
export function loadBaseline(filePath: string): BaselineLoadResult {
    if (!fs.existsSync(filePath)) {
        return { kind: "missing" };
    }
    let parsed: Baseline;
    try {
        parsed = JSON.parse(fs.readFileSync(filePath, "utf8")) as Baseline;
    } catch (err) {
        return { kind: "stale", reason: `基线文件无法解析：${(err as Error).message}` };
    }
    if (parsed.schemaVersion !== BASELINE_SCHEMA_VERSION) {
        return {
            kind: "stale",
            reason: `基线 schemaVersion=${parsed.schemaVersion}，当前要求 ${BASELINE_SCHEMA_VERSION}`,
        };
    }
    if (parsed.meta?.protocolVersion !== PROTOCOL_VERSION) {
        return {
            kind: "stale",
            reason: `基线 protocolVersion=${String(parsed.meta?.protocolVersion)}，当前 ${PROTOCOL_VERSION}`,
        };
    }
    return { kind: "ok", baseline: parsed };
}

/** 写出基线文件。 */
export function saveBaseline(filePath: string, baseline: Baseline): void {
    fs.mkdirSync(path.dirname(filePath), { recursive: true });
    fs.writeFileSync(filePath, `${JSON.stringify(baseline, null, 2)}\n`);
}

/** 基线文件路径。 */
export function baselinePath(dir: string, server: ServerKind): string {
    return path.join(dir, `${server}.json`);
}

/** 构造一份空基线骨架。 */
export function makeBaseline(server: ServerKind, gitCommit: string, note: string): Baseline {
    return {
        schemaVersion: BASELINE_SCHEMA_VERSION,
        meta: {
            server,
            mcVersion: MC_VERSION,
            protocolVersion: PROTOCOL_VERSION,
            frozenAt: new Date().toISOString(),
            gitCommit,
            note,
        },
        cases: {},
    };
}

/** 读取当前 git commit（失败返回空串，不阻断流程）。 */
export function currentGitCommit(repoRoot: string, runner: (cmd: string, args: string[], cwd: string) => string): string {
    try {
        return runner("git", ["rev-parse", "--short", "HEAD"], repoRoot).trim();
    } catch {
        return "";
    }
}
