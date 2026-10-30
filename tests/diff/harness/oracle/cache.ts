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
 * cache.ts — 复用原版 oracle 的上一次输出。
 *
 * 参照实现（原版 jar）只在「输入或 oracle 代码变了」时才会给出不同答案。项目侧（探针逻辑、
 * 公式）的改动不影响它，却会让整个能力重跑一遍 oracle：一次 `java Oracle run ...` 要
 * 1~10 秒，几十个能力加起来就是几分钟。
 *
 * 缓存键覆盖 oracle 输出可能变化的一切：
 *   - oracle 源文件的 import 闭包内容（含共享辅助类）；
 *   - 原版 jar 与依赖库（工具链指纹：同样是「同样的 jar 同样的代码」）；
 *   - 命令行的其余部分（子命令、额外参数）；
 *   - 输入文件的内容（单元清单与每行输入）。
 *
 * 命中条件很强：键相同就意味着「同样的字节、同样的 oracle 代码、同样的 jar」。但缓存仍可能
 * 被罕见因素污染（环境变量、时区、未纳入指纹的本地文件），因此命中却失败时由 session.ts
 * 重跑一次 oracle 复核，重跑一致则删键。
 *
 * `--fresh` 完全关闭缓存。
 */

import fs from "node:fs";
import path from "node:path";

import type { FileHash, HashCache } from "../core/hash.ts";
import { digestOf, hashFiles } from "../core/hash.ts";
import { REPO_ROOT } from "../runner/layout.ts";

export interface OracleCacheKeyInputs {
    /** oracle 源文件（仓库相对路径）。 */
    readonly sources: readonly string[];
    /** 原版 jar 与依赖库目录（绝对路径）。 */
    readonly toolchainPaths: readonly string[];
    /** 命令行里除输入/输出路径之外的参数（子命令、额外开关）。 */
    readonly argv: readonly string[];
    /** 处理的单元清单。 */
    readonly units: readonly string[];
    /** 每行输入（与 units 一一对应）。 */
    readonly inputs: readonly string[];
}

export class OracleCache {
    private readonly directory: string;
    private readonly enabled: boolean;

    constructor(stateDirectory: string, enabled: boolean) {
        this.directory = path.join(stateDirectory, "oracle");
        this.enabled = enabled;
    }

    /** 计算缓存键（工具链与源文件的哈希跨能力复用同一份缓存）。 */
    key(inputs: OracleCacheKeyInputs, cache: HashCache): string {
        const entries = new Map<string, FileHash>();
        const sources = hashFiles(
            inputs.sources.map((file) => path.join(REPO_ROOT, ...file.split("/"))),
            cache,
        );
        for (const [file, hash] of sources) {
            entries.set(path.relative(REPO_ROOT, file).split(path.sep).join("/"), hash);
        }
        for (const [file, hash] of hashFiles(inputs.toolchainPaths, cache)) {
            entries.set(file, hash);
        }
        entries.set("<argv>", digestOf(new Map([["argv", JSON.stringify(inputs.argv)]])));
        entries.set("<units>", digestOf(new Map([["units", JSON.stringify(inputs.units)]])));
        entries.set("<inputs>", digestOf(new Map([["inputs", JSON.stringify(inputs.inputs)]])));
        return digestOf(entries);
    }

    /** 取出缓存的原版输出文件；未命中返回 undefined。 */
    lookup(key: string): string | undefined {
        if (!this.enabled) {
            return undefined;
        }
        const file = path.join(this.directory, `${key}.jsonl`);
        return fs.existsSync(file) ? file : undefined;
    }

    /**
     * 把一次成功的 oracle 运行结果纳入缓存。
     *
     * 只缓存「oracle 自己成功退出」的输出：失败的运行可能只写了一半文件，重放会把一次
     * 环境问题固化成永久的差异。
     */
    store(key: string, outputFile: string): void {
        if (!this.enabled) {
            return;
        }
        fs.mkdirSync(this.directory, { recursive: true });
        const target = path.join(this.directory, `${key}.jsonl`);
        const temporary = `${target}.${process.pid}.tmp`;
        fs.copyFileSync(outputFile, temporary);
        fs.renameSync(temporary, target);
    }

    /** 删除一个键（缓存被证明有污染时）。 */
    invalidate(key: string): void {
        fs.rmSync(path.join(this.directory, `${key}.jsonl`), { force: true });
    }
}