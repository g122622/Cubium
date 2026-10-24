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
 * export_entity_attributes_reference.ts — 用原版 oracle 重算生物属性参考表。
 *
 *   node tests/diff/harness/scripts/export_entity_attributes_reference.ts [--write]
 *
 * 不带 --write 时只把参考表打印到标准输出，便于 diff 复核；带 --write 时直接覆盖
 * harnesses/entity_attributes_reference.ts（人工维护的 RUNTIME_DIVERGENT 之外的
 * 注释和类型保持不变，因为整个数据段都是机械生成的）。
 *
 * 参考表的意义：entity_attributes harness 的通过条件是「Cubium 的值 == 表里的值」，
 * 而不是「== oracle 输出的值」。oracle 用的是实体类型的默认属性供应器，对运行时改写
 * 属性的生物（史莱姆按尺寸、马按随机数）不等于默认值，所以这张表是比对基准。
 */

import fs from "node:fs";
import path from "node:path";

import { runJavaOracle } from "../java-oracle.ts";
import { REPO_ROOT, resolveBuildDir } from "../runner.ts";
import {
    RUNTIME_DIVERGENT,
    VANILLA_ENTITY_ATTRIBUTES,
} from "../harnesses/entity_attributes_reference.ts";

const ORACLE = "EntityAttributesOracle";
const TARGET = path.join(
    REPO_ROOT,
    "tests/diff/harness/harnesses/entity_attributes_reference.ts",
);

function bitsToNumber(hex: string): number {
    const buffer = Buffer.alloc(8);
    buffer.writeBigUInt64BE(BigInt(hex));
    return buffer.readDoubleBE(0);
}

function formatNumber(value: number): string {
    const rounded = Math.fround(value);
    const text = String(rounded);
    return text.includes(".") || text.includes("e") ? text : `${text}.0`;
}

function main(): number {
    const write = process.argv.includes("--write");
    const outcome = runJavaOracle({
        domain: "entity_attributes",
        buildDir: resolveBuildDir(undefined),
        oracle: ORACLE,
        input: (unit: string) => ({ entity: unit }),
        key: "entity",
        // 参考表的静态条目（运行时改写的生物）不在 oracle 的输出里，用固定单元列表。
        units: () => [...VANILLA_ENTITY_ATTRIBUTES.keys()],
        oracleMissing: RUNTIME_DIVERGENT,
    });

    const oracleValues = new Map<string, Map<string, number>>();
    for (const line of fs.readFileSync(outcome.oracleOutputPath, "utf8").split("\n")) {
        if (line.trim() === "") {
            continue;
        }
        const record = JSON.parse(line) as {
            entity: string;
            missing?: boolean;
            attributes?: Record<string, string>;
        };
        if (record.missing === true) {
            continue;
        }
        const values = new Map<string, number>();
        for (const [key, bits] of Object.entries(record.attributes ?? {})) {
            values.set(key, bitsToNumber(bits));
        }
        oracleValues.set(record.entity, values);
    }

    const lines: string[] = [];
    for (const [entity, reference] of VANILLA_ENTITY_ATTRIBUTES) {
        const fresh = oracleValues.get(entity);
        let values: Map<string, number>;
        if (fresh === undefined) {
            if (!RUNTIME_DIVERGENT.includes(entity)) {
                console.error(`跳过 ${entity}：oracle 没有输出，也不在 RUNTIME_DIVERGENT 里`);
                return 1;
            }
            values = new Map(reference);
        } else {
            // 只保留参考表登记过的键与 oracle 真有的键的交集：新增键要人工补进表。
            values = new Map();
            for (const key of reference.keys()) {
                const value = fresh.get(key);
                if (value !== undefined) {
                    values.set(key, value);
                }
            }
        }
        const body = [...values]
            .map(([key, value]) => `["${key}", ${formatNumber(value)}]`)
            .join(", ");
        lines.push(`    ["${entity}", new Map([${body}])],`);
    }

    const block = lines.join("\n");
    if (!write) {
        console.log(block);
        return 0;
    }

    // 数据段形如 `= new Map<\n    string,\n    ReadonlyMap<string, number>\n>([\n  ...条目...\n  ]);`，
    // 只替换 `])(` 与 `\n]);` 之间的条目，注释与 RUNTIME_DIVERGENT 原样保留。
    const source = fs.readFileSync(TARGET, "utf8");
    const open = source.indexOf(">([\n");
    const close = source.lastIndexOf("\n]);");
    if (open === -1 || close === -1 || close < open) {
        console.error("找不到参考表的数据段（导出格式已变化？）");
        return 1;
    }
    const head = source.slice(0, open + ">([\n".length);
    const tail = source.slice(close + 1);
    fs.writeFileSync(TARGET, `${head}${block}${tail}`);
    console.log(`已写入 ${TARGET}（${lines.length} 个生物）`);
    return 0;
}

process.exitCode = main();