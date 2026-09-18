/*
 * workspace.ts — 运行目录的创建、清理与历史产物轮转。
 *
 * 目录布局：
 *   build/e2e/
 *   ├── runs/<runId>/<caseId>/     每次运行的用例工作目录（游戏目录 = 此处）
 *   ├── artifacts/<runId>/<caseId>/ 失败时保留的诊断产物
 *   └── vanilla-cache/             vanilla bundler 解包复用目录（跨运行共享）
 */

import fs from "node:fs";
import path from "node:path";
import { ARTIFACTS_KEEP, WORK_ROOT } from "../config.ts";

/** 本次运行的标识（时间戳 + 进程号，避免并行运行互相覆盖）。 */
export function makeRunId(): string {
    const now = new Date();
    const pad = (value: number, width: number): string => String(value).padStart(width, "0");
    const stampText =
        `${now.getFullYear()}${pad(now.getMonth() + 1, 2)}${pad(now.getDate(), 2)}` +
        `-${pad(now.getHours(), 2)}${pad(now.getMinutes(), 2)}${pad(now.getSeconds(), 2)}`;
    return `${stampText}-${process.pid}`;
}

/** 本次运行的用例工作目录。 */
export function runDirFor(runId: string, caseId: string): string {
    return path.join(WORK_ROOT, "runs", runId, caseId.replace(/[/\\]/g, "_"));
}

/** 本次运行的诊断产物目录。 */
export function artifactDirFor(runId: string, caseId: string): string {
    return path.join(WORK_ROOT, "artifacts", runId, caseId.replace(/[/\\]/g, "_"));
}

/** vanilla bundler 解包缓存目录（跨运行复用，避免每次用例重复解包 30+ 个 jar）。 */
export function vanillaCacheDir(): string {
    return path.join(WORK_ROOT, "vanilla-cache");
}

/**
 * 递归删除目录，失败不抛异常。
 *
 * Windows 上文件句柄可能尚未释放（服务端刚被杀），此时删除会失败——
 * 这属于可接受的残留，下次运行会重建；不应因此让用例失败。
 *
 * @returns 是否删除成功。
 */
export function removeDirQuietly(dir: string): boolean {
    try {
        fs.rmSync(dir, { recursive: true, force: true, maxRetries: 3, retryDelay: 200 });
        return true;
    } catch {
        return false;
    }
}

/** 保留最近 N 次运行的产物目录，更早的清掉。 */
export function pruneArtifacts(keep: number): void {
    const artifactsRoot = path.join(WORK_ROOT, "artifacts");
    pruneByCount(artifactsRoot, keep);
    const runsRoot = path.join(WORK_ROOT, "runs");
    pruneByCount(runsRoot, keep);
}

function pruneByCount(root: string, keep: number): void {
    if (!fs.existsSync(root)) {
        return;
    }
    const entries = fs
        .readdirSync(root, { withFileTypes: true })
        .filter((entry) => entry.isDirectory())
        .map((entry) => entry.name)
        .sort();
    for (const name of entries.slice(0, Math.max(0, entries.length - keep))) {
        removeDirQuietly(path.join(root, name));
    }
}

/** 把一次运行的诊断产物整体保留到 artifacts 目录。 */
export function persistArtifacts(artifactDir: string, files: ReadonlyMap<string, string>): void {
    fs.mkdirSync(artifactDir, { recursive: true });
    for (const [name, content] of files) {
        fs.writeFileSync(path.join(artifactDir, name), content);
    }
}
