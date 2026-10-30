/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/*
 * hash.ts — 内容哈希与「路径 + 内容」的组合摘要。
 *
 * 增量复跑的唯一判据是**内容**：同一批依赖文件内容不变，重新运行必然得到同一结果，
 * 上一次的通过结论可以沿用。大小与修改时间只决定「要不要重新读文件算哈希」，从不参与
 * 判定——复制、checkout、touch 都会改 mtime，内容却没变。
 *
 * 哈希缓存随状态文件持久化，但不常驻内存：每次运行从上次的状态恢复、结束写回。这样同一个
 * 仓库在多个构建目录之间切换时，缓存不会因为两边 mtime 不同而整体失效。
 */

import { createHash } from "node:crypto";
import fs from "node:fs";

/** 64 位十六进制内容哈希；文件不存在时是 "missing"。 */
export type FileHash = string;

/** 哈希缓存的持久化格式：路径 → [文件大小, 修改时间（纳秒）, 内容哈希]。 */
export type HashCache = Record<string, [number, string, FileHash]>;

function mtimeKey(stat: fs.BigIntStats): string {
    // 纳秒级 mtime：毫秒级会在「同一毫秒内两次写入」时把变化漏掉。
    return String(stat.mtimeNs);
}

/**
 * 计算一组文件的内容哈希。
 *
 * @param paths 绝对路径；目录会被跳过并报告（目录内容不属于「文件内容」判据）
 * @param previous 上次运行留下的缓存（原地更新）
 */
export function hashFiles(paths: Iterable<string>, previous: HashCache): Map<string, FileHash> {
    const result = new Map<string, FileHash>();
    for (const path of paths) {
        if (result.has(path)) {
            continue;
        }
        let stat: fs.BigIntStats;
        try {
            stat = fs.statSync(path, { bigint: true });
        } catch {
            result.set(path, "missing");
            delete previous[path];
            continue;
        }
        if (stat.isDirectory()) {
            console.warn(`[diff] 依赖集合里出现目录，已忽略：${path}`);
            continue;
        }
        const stamp = mtimeKey(stat);
        const cached = previous[path];
        if (cached !== undefined && cached[0] === Number(stat.size) && cached[1] === stamp) {
            result.set(path, cached[2]);
            continue;
        }
        const digest = createHash("sha256").update(fs.readFileSync(path)).digest("hex");
        previous[path] = [Number(stat.size), stamp, digest];
        result.set(path, digest);
    }
    return result;
}

/**
 * 把文件哈希集合折叠成一个摘要。
 *
 * 路径是摘要的一部分：同样的内容出现在不同路径下仍然是不同的依赖。按路径排序后折叠，
 * 保证同一集合无论收集顺序如何都得到同一摘要。
 */
export function digestOf(hashes: Map<string, FileHash>): string {
    const digest = createHash("sha256");
    for (const path of [...hashes.keys()].sort()) {
        digest.update(path);
        digest.update("\0");
        digest.update(hashes.get(path) ?? "missing");
        digest.update("\n");
    }
    return digest.digest("hex");
}

/** 字符串内容的摘要（用于构造键，不参与文件缓存）。 */
export function digestText(text: string): string {
    return digestOf(new Map([["<text>", createHash("sha256").update(text).digest("hex")]]));
}

/** 只保留仍然存在的缓存条目，避免状态文件随分支切换无限增长。 */
export function pruneHashCache(cache: HashCache, live: Set<string>, limit = 200_000): HashCache {
    if (Object.keys(cache).length <= limit) {
        return cache;
    }
    const kept: HashCache = {};
    for (const [path, entry] of Object.entries(cache)) {
        if (live.has(path)) {
            kept[path] = entry;
        }
    }
    return kept;
}