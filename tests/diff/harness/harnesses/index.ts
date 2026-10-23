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
 * index.ts — 差分 harness 注册表与批量入口。
 *
 *   node tests/diff/harness/runner.ts run all          # 全部
 *   node tests/diff/harness/runner.ts run item_summary # 单个
 *
 * 每个 harness 自带参照实现，互不依赖：任何一个失败都不影响其余 harness 运行。
 * 参照实现有两类：
 *   - 原版 Java oracle（优先）：直接调用原版 jar 的 API，权威、逐字节可比；
 *   - minecraft-data（回退）：原版 jar 不可用时的离线数据源，覆盖字段更少。
 */

import { runJavaOracle } from "../java-oracle.ts";
import type { HarnessSpec } from "../runner.ts";
import { runSpec } from "../runner.ts";
import { createEnchantmentTableHarness } from "./enchantment_table.ts";
import { createEntityShapeHarness } from "./entity_shape.ts";
import { createItemSummaryHarness } from "./item_summary.ts";

/** 已登记、使用 minecraft-data 的声明式 harness。 */
const DATA_SPECS: ReadonlyArray<() => HarnessSpec> = [createEnchantmentTableHarness];

/** 已登记、使用原版 Java oracle 的能力：oracle 类名 + 单元来源。 */
interface JavaHarness {
    readonly name: string;
    readonly oracle: string;
    /** probe 输入单元；缺省时由 oracle 的 list 子命令枚举。 */
    readonly units?: () => string[];
    readonly input: (unit: string) => unknown;
    readonly key: string;
}

const JAVA_HARNESSES: ReadonlyArray<() => JavaHarness> = [
    () => {
        const spec = createItemSummaryHarness();
        return { name: spec.name, oracle: "ItemSummaryOracle", units: spec.units, input: spec.input, key: spec.key };
    },
    () => {
        const spec = createEntityShapeHarness();
        return { name: spec.name, oracle: "EntityShapeOracle", input: spec.input, key: spec.key };
    },
];

function allDataSpecs(): HarnessSpec[] {
    return DATA_SPECS.map((factory) => factory());
}

/** 跑全部 harness；任一失败即返回非零。 */
export function runAll(): number {
    let failures = 0;
    let total = 0;

    for (const factory of JAVA_HARNESSES) {
        const harness = factory();
        total++;
        try {
            if (runJavaOracle({ ...harness, domain: harness.name }).exitCode !== 0) {
                failures++;
            }
        } catch (error) {
            console.error(`环境问题（${harness.name}）：${(error as Error).message}`);
            failures++;
        }
    }

    for (const spec of allDataSpecs()) {
        total++;
        try {
            if (runSpec(spec).exitCode !== 0) {
                failures++;
            }
        } catch (error) {
            console.error(`环境问题（${spec.name}）：${(error as Error).message}`);
            failures++;
        }
    }

    console.log(failures === 0 ? `\n全部 ${total} 个 harness 通过` : `\n${failures} / ${total} 个 harness 未通过`);
    return failures === 0 ? 0 : 1;
}

function findJavaHarness(name: string): JavaHarness | undefined {
    return JAVA_HARNESSES.map((factory) => factory()).find((harness) => harness.name === name);
}

/** 跑指定名字的 harness。 */
export async function runNamed(name: string): Promise<number> {
    if (name === "text_encode") {
        // text_encode 是手写 harness（比状态全集与摘要两件事），单独入口。
        const { main } = await import("../test_text_encode_parity.ts");
        return main(["--no-build"]);
    }

    const javaHarness = findJavaHarness(name);
    if (javaHarness !== undefined) {
        try {
            return runJavaOracle({ ...javaHarness, domain: javaHarness.name }).exitCode;
        } catch (error) {
            console.error(`环境问题（${name}）：${(error as Error).message}`);
            return 2;
        }
    }

    const spec = allDataSpecs().find((candidate) => candidate.name === name);
    if (spec === undefined) {
        console.error(`未知 harness：${name}`);
        return 2;
    }
    try {
        return runSpec(spec).exitCode;
    } catch (error) {
        console.error(`环境问题（${name}）：${(error as Error).message}`);
        return 2;
    }
}