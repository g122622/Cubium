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
 * test_text_encode_parity.ts — 差分 harness：方块状态的文本表示。
 *
 *   node tests/diff/harness/test_text_encode_parity.ts            # 构建并运行
 *   node tests/diff/harness/test_text_encode_parity.ts --no-build # 用已构建的 probe
 *   node tests/diff/harness/test_text_encode_parity.ts --samples 20
 *
 * 比对两件事，任一不一致即失败（退出码 1）：
 *
 *   1. 方块摘要（probe `run`，输入是方块名）
 *      每个方块的属性名、属性类型、允许取值、状态数必须与参照实现一致。
 *   2. 状态全集（probe `list`）
 *      每个方块展开属性后的状态文本必须逐条一致，形如 minecraft:oak_log[axis=y]，
 *      属性按名字字典序。
 *
 * 退出码：0 全部一致；1 存在差异；2 环境问题（缺 probe、构建失败、缺原版数据）。
 */

import { execFileSync } from "node:child_process";
import { describe as vanillaDescribe, loadBlocks, REPO_ROOT, stateTexts } from "./vanilla.ts";
import { buildProbes, probeExecutable, render, runHarness } from "./runner.ts";

/**
 * 两侧对同一个属性的类型名不一致、但语义等价的配对。
 *
 * DirectionProperty 是 EnumProperty<Direction> 的子类，vanilla 的 `facing` 就是
 * `EnumProperty<Direction>`；这里只做**类型名**归一，不放开取值比较。
 * 只登记确有依据的配对，出现新配对时应当先查证再补进来，而不是一律放宽。
 */
const TYPE_ALIASES: Readonly<Record<string, string>> = { DirectionProperty: "EnumProperty" };

/**
 * 参照数据（minecraft-data 3.115.0 的 1.21.11）缺失、但 Cubium 有的方块。
 *
 * 这不是 Cubium 的错误：minecraft-data 的 blocks.json 少了这两面墙，它们会表现为
 * 「probe 多出 2 个方块 / 648 条状态」。显式列出来，让报告把「参照缺口」与
 * 「真实不一致」分开——否则每次运行都会看到同一条噪声，久而久之就没人看了。
 *
 * 每次运行都会核对：**参照数据里一旦有了**，这里就必须删掉对应条目，否则测试失败。
 * 等切到 Java oracle（直接读服务端 jar）后，本清单应当清空。
 */
const ORACLE_MISSING_BLOCKS: readonly string[] = ["minecraft:purpur_wall", "minecraft:quartz_wall"];

/**
 * 把方块摘要转换成「只比语义、不比排列」的形式，两侧都要经过它。
 *
 * - 属性按名字排序：vanilla 的 StateHolder#toString 按名字字典序输出，注册顺序不是语义；
 * - 取值域排序：minecraft-data 给的是字符串序（"1","10","2"），Cubium 给的是枚举声明序，
 *   两者对「值域相同」这个结论没有影响；
 * - 类型名归一：见 TYPE_ALIASES。
 */
function canonical(value: unknown): unknown {
    if (Array.isArray(value)) {
        return value.map(canonical);
    }
    if (value !== null && typeof value === "object") {
        const source = value as Record<string, unknown>;
        const result: Record<string, unknown> = {};
        for (const [key, item] of Object.entries(source)) {
            if (key === "type" && typeof item === "string") {
                result[key] = TYPE_ALIASES[item] ?? item;
            } else if (key === "values" && Array.isArray(item)) {
                result[key] = item.map(canonical).sort();
            } else if (key === "properties" && Array.isArray(item)) {
                const props = item.map(canonical) as Array<Record<string, unknown>>;
                props.sort((lhs, rhs) => String(lhs.name).localeCompare(String(rhs.name)));
                result[key] = props;
            } else {
                result[key] = canonical(item);
            }
        }
        return result;
    }
    return value;
}

/** 把状态全集做多重集差：重复出现的状态也是缺陷，集合差会把它吃掉。 */
function stateTextDifferences(actual: readonly string[], expected: readonly string[]): string[] {
    const count = (items: readonly string[]): Map<string, number> => {
        const counts = new Map<string, number>();
        for (const item of items) {
            counts.set(item, (counts.get(item) ?? 0) + 1);
        }
        return counts;
    };
    const actualCounts = count(actual);
    const expectedCounts = count(expected);
    const lines: string[] = [];
    for (const text of [...expectedCounts.keys()].sort()) {
        const missing = (expectedCounts.get(text) ?? 0) - (actualCounts.get(text) ?? 0);
        if (missing > 0) {
            lines.push(missing === 1 ? `probe 缺少：${text}` : `probe 缺少 ${missing} 次：${text}`);
        }
    }
    for (const text of [...actualCounts.keys()].sort()) {
        const extra = (actualCounts.get(text) ?? 0) - (expectedCounts.get(text) ?? 0);
        if (extra > 0) {
            lines.push(extra === 1 ? `probe 多出：${text}` : `probe 多出 ${extra} 次：${text}`);
        }
    }
    return lines;
}

/** 依次构建并运行；返回进程退出码。 */
export function main(argv: string[]): number {
    let build = true;
    let samples = 5;
    for (let i = 0; i < argv.length; i++) {
        const argument = argv[i];
        if (argument === "--no-build") {
            build = false;
        } else if (argument === "--samples" && i + 1 < argv.length) {
            samples = Number(argv[++i]);
        } else {
            console.error(`未知参数：${argument}`);
            return 2;
        }
    }

    let blocks;
    try {
        blocks = loadBlocks();
    } catch (error) {
        console.error(`环境问题：${(error as Error).message}`);
        return 2;
    }

    try {
        if (build) {
            buildProbes("text_encode");
        }
        const result = runHarness({
            harness: "text_encode",
            domain: "text_encode",
            units: blocks.map((block) => block.name),
            oracleOutputs: blocks.map((block) => JSON.stringify(canonical(vanillaDescribe(block)))),
            key: "block",
            // probe 侧同样规范化：DPO 的类型名/枚举顺序与参照数据的差异不属本能力语义。
            compareTransform: canonical,
        });

        const executable = probeExecutable("text_encode");
        // 输出约 3 万行（2,000+ 个方块状态文本），必须显式放大 maxBuffer：默认 1 MB 会 ENOBUFS。
        const listed = execFileSync(executable, ["list"], {
            cwd: REPO_ROOT,
            encoding: "utf8",
            maxBuffer: 1024 * 1024 * 1024,
        })
            .split(/\r?\n/)
            .filter((line) => line.length > 0);
        const expectedTexts = blocks.flatMap(stateTexts).sort();
        // 先在参照数据里核对缺口清单：参照数据补齐后必须删掉条目，否则报错，
        // 防止清单变成无人维护的永久豁免。
        const oracleNames = new Set(blocks.map((block) => block.name));
        const staleExemptions = ORACLE_MISSING_BLOCKS.filter((name) => oracleNames.has(name));
        if (staleExemptions.length > 0) {
            console.error(
                `参照数据已包含这些方块，请从 ORACLE_MISSING_BLOCKS 删除：${staleExemptions.join("、")}`,
            );
            return 2;
        }

        const exempt = new Set(ORACLE_MISSING_BLOCKS);
        const actualTexts = listed.filter((text) => !exempt.has(text.split("[")[0]));
        const stateDifferences = stateTextDifferences(actualTexts, expectedTexts);
        const exemptedStates = listed.length - actualTexts.length;

        console.log(render(result, samples));
        console.log(
            `  状态全集：probe ${actualTexts.length} 条，参照实现 ${expectedTexts.length} 条，` +
                (stateDifferences.length === 0 ? "一致" : `不一致（${stateDifferences.length} 处）`),
        );
        if (exemptedStates > 0) {
            console.log(
                `  另有 ${exemptedStates} 条状态属于参照数据缺失的方块（${ORACLE_MISSING_BLOCKS.join("、")}），未计入`,
            );
        }
        for (const line of stateDifferences.slice(0, samples)) {
            console.log(`    ${line}`);
        }
        if (stateDifferences.length > samples) {
            console.log(`    ... 另有 ${stateDifferences.length - samples} 处，见 report.json`);
        }

        return result.differences.length === 0 && stateDifferences.length === 0 ? 0 : 1;
    } catch (error) {
        console.error(`环境问题：${(error as Error).message}`);
        return 2;
    }
}

if (import.meta.main) {
    process.exitCode = main(process.argv.slice(2));
}