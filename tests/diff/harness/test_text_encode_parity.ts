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
 * test_text_encode_parity.ts — 方块状态的文本表示（Cubium vs 原版 jar）。
 *
 *   node tests/diff/harness/runner.ts run text_encode
 *
 * 两个维度：
 *
 *   1. 状态全集     —— probe `list` 与原版 `BlockStateOracle list` 逐条比对，
 *                      文本形如 minecraft:oak_log[axis=y]，属性按名字字典序；
 *   2. 方块摘要     —— 每个方块的属性名、属性类型、允许取值、状态数。
 *
 * 与初版相比有两点变化：
 *   - 参照实现换成原版 jar，不再依赖 minecraft-data（后者缺 purpur_wall / quartz_wall，
 *     且不给状态级信息）；
 *   - 只比较**原版也有**的方块，原版没有而 Cubium 有的方块单独列出：多出来的方块
 *     同样是缺陷（注册表与存档都会不一致），但不能混进逐字段比对里。
 */

import { execFileSync } from "node:child_process";
import path from "node:path";

import { compileOracle, runJavaOracle } from "./java-oracle.ts";

import { probeExecutable, resolveBuildDir } from "./runner.ts";

/** 两侧输出里标识单元的字段。 */
const KEY = "block";

/**
 * 把属性做「语义规范化」：属性按名排序、取值域排序、DirectionProperty 归一到
 * EnumProperty（前者是后者的子类，原版 facing 就是 EnumProperty<Direction>）。
 *
 * 只归一**类型名**，不放开取值比较。
 */
const TYPE_ALIASES: Readonly<Record<string, string>> = { DirectionProperty: "EnumProperty" };

function canonical(value: unknown): unknown {
    if (Array.isArray(value)) {
        return value.map(canonical);
    }
    if (value !== null && typeof value === "object") {
        const source = value as Record<string, unknown>;
        const result: Record<string, unknown> = {};
        for (const [field, item] of Object.entries(source)) {
            if (field === "type" && typeof item === "string") {
                result[field] = TYPE_ALIASES[item] ?? item;
            } else if (field === "values" && Array.isArray(item)) {
                result[field] = item.map(canonical).sort();
            } else if (field === "properties" && Array.isArray(item)) {
                const properties = item.map(canonical) as Array<Record<string, unknown>>;
                properties.sort((lhs, rhs) => String(lhs.name).localeCompare(String(rhs.name)));
                result[field] = properties;
            } else {
                result[field] = canonical(item);
            }
        }
        return result;
    }
    return value;
}

/** 从 `minecraft:oak_log[axis=y]` 取出方块名。 */
function blockOf(stateText: string): string {
    const bracket = stateText.indexOf("[");
    return bracket < 0 ? stateText : stateText.slice(0, bracket);
}

/**
 * 比对两侧的状态全集。
 *
 * 返回两类结果：原版有而 Cubium 缺的（真缺陷），以及 Cubium 多出来的（也是缺陷，
 * 但性质不同——多注册了原版不存在的方块）。
 */
function compareStateSets(
    cubium: readonly string[],
    vanilla: readonly string[],
): { missing: string[]; extra: string[]; shared: number } {
    const cubiumSet = new Set(cubium);
    const vanillaSet = new Set(vanilla);
    const missing = [...vanillaSet].filter((text) => !cubiumSet.has(text)).sort();
    const extra = [...cubiumSet].filter((text) => !vanillaSet.has(text)).sort();
    return { missing, extra, shared: vanillaSet.size - missing.length };
}

export function main(argv: string[]): number {
    let samples = 5;
    for (let index = 0; index < argv.length; index++) {
        const argument = argv[index];
        if (argument === "--no-build") {
            // 构建由调用方负责；保留参数以兼容旧用法。
        } else if (argument === "--samples" && index + 1 < argv.length) {
            samples = Number(argv[++index]);
        } else {
            console.error(`未知参数：${argument}`);
            return 2;
        }
    }

    const executable = probeExecutable("text_encode");

    // 1. 状态全集：两侧各自枚举自己的全部状态。
    const cubiumStates = execFileSync(executable, ["list"], {
        encoding: "utf8",
        maxBuffer: 1024 * 1024 * 1024,
    })
        .split(/\r?\n/)
        .filter((line) => line.length > 0);

    // 原版枚举走 oracle 的 list 子命令（先编译，确保 classes 目录里有这个类）。
    const workDir = path.join(resolveBuildDir(), "diff", "text_encode", "java");
    const compiled = compileOracle("BlockStateOracle", workDir);
    const vanillaStates = execFileSync(
        "java",
        ["-cp", `${compiled.classesDir}${path.delimiter}${compiled.classpath}`,
            "dev.horizonlab.diff.BlockStateOracle", "list"],
        { encoding: "utf8", maxBuffer: 1024 * 1024 * 1024, cwd: workDir },
    )
        .split(/\r?\n/)
        .filter((line) => line.length > 0);

    const states = compareStateSets(cubiumStates, vanillaStates);
    const vanillaBlocks = new Set(vanillaStates.map(blockOf));
    const extraBlocks = [...new Set(states.extra.map(blockOf))].sort();

    // 2. 方块摘要：只比原版也有的方块。
    const units = [...vanillaBlocks].sort();
    const outcome = runJavaOracle({
        oracle: "BlockStateOracle",
        domain: "text_encode",
        units: () => units,
        input: (unit) => ({ block: unit }),
        key: KEY,
        normalise: canonical,
        samples,
    });

    console.log(
        `  状态全集：原版 ${vanillaStates.length} 条，Cubium ${cubiumStates.length} 条；` +
            `共同 ${states.shared} 条，缺 ${states.missing.length} 条，多 ${states.extra.length} 条`,
    );
    for (const text of states.missing.slice(0, samples)) {
        console.log(`    缺少：${text}`);
    }
    if (states.missing.length > samples) {
        console.log(`    ... 另有 ${states.missing.length - samples} 条缺少的状态`);
    }
    if (extraBlocks.length > 0) {
        console.log(
            `    Cubium 多出的方块（原版 1.21.11 不存在）：${extraBlocks.join("、")}，` +
                `合计 ${states.extra.length} 条状态`,
        );
    }

    const ok = outcome.exitCode === 0 && states.missing.length === 0 && states.extra.length === 0;
    return ok ? 0 : 1;
}

if (import.meta.main) {
    process.exit(main(process.argv.slice(2)));
}