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
 *   node tests/diff/tools/export_entity_attributes_reference.ts [--write]
 *
 * 不带 `--write` 时只把参考表打印到标准输出，便于 diff 复核；带 `--write` 时直接覆盖
 * `harness/harnesses/entity_attributes_reference.ts` 的数据段（注释与 RUNTIME_DIVERGENT
 * 原样保留，那一部分是人工维护的）。
 *
 * 参考表的意义：entity_attributes 能力的通过条件是「Cubium 的值 == 表里的值」，而不是
 * 「== oracle 输出的值」。oracle 用的是实体类型的默认属性供应器，对运行时改写属性的生物
 * （史莱姆按尺寸、马按随机数）不等于默认值，所以这张表才是比对基准。
 *
 * 走的是与正式运行同一个 oracle 缓存：表没变时几乎立即返回。
 */

import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration } from "../harness/core/types.ts";
import { HARNESSES } from "../harness/harnesses/index.ts";
import {
    RUNTIME_DIVERGENT,
    VANILLA_ENTITY_ATTRIBUTES,
} from "../harness/harnesses/entity_attributes_reference.ts";
import { OracleCache } from "../harness/oracle/cache.ts";
import { loadOracleEnvironment } from "../harness/oracle/environment.ts";
import { projectLayout, REPO_ROOT } from "../harness/runner/layout.ts";
import { HarnessSession, runDeclaration } from "../harness/runner/session.ts";
import { loadState, saveState } from "../harness/runner/state.ts";

/** 参考表文件（相对仓库根）。 */
const TARGET = path.join(REPO_ROOT, "tests/diff/harness/harnesses/entity_attributes_reference.ts");

/** oracle 输出的属性值是 u64 位模式，还原成 double。 */
function bitsToNumber(hex: string): number {
    const buffer = Buffer.alloc(8);
    buffer.writeBigUInt64BE(BigInt(hex));
    return buffer.readDoubleBE(0);
}

/**
 * f32 语义下的**最短往返写法**（与参考表既有条目的风格一致：`10.0`、`0.25`、`0.2`）。
 *
 * 值与 `entity_attributes` 能力实际读到的一致：C++ 侧用 float 字段承载，读出来就是
 * f32 精确值。这里先降到 f32，再用「最短且能还原同一 f32」的十进制写法表示——
 * 与 Java `Float.toString` 的规则相同，避免 `0.2` 被写成 `0.20000000298023224`。
 */
function formatNumber(value: number): string {
    const target = Math.fround(value);
    for (let precision = 1; precision <= 9; precision++) {
        const text = target.toPrecision(precision);
        if (Math.fround(Number(text)) === target) {
            const plain = Number(text).toString();
            return plain.includes(".") || plain.includes("e") ? plain : `${plain}.0`;
        }
    }
    return String(target);
}

/** 导出用的能力声明：单元取参考表的实体集合，运行时改写的实体按 oracle 缺口剔除。 */
function exportDeclaration(): HarnessDeclaration {
    const base = HARNESSES.find((harness) => harness.name === "entity_attributes");
    if (base === undefined) {
        throw new Error("HARNESSES 里没有 entity_attributes");
    }
    // RUNTIME_DIVERGENT 不并入 oracleMissing：参考表为它们保留静态条目，导出时正是靠
    // 「oracle 没输出 + 在 RUNTIME_DIVERGENT 里」这条规则沿用旧值，剔除后就无从判断了。
    return {
        ...base,
        units: { kind: "computed", units: () => [...VANILLA_ENTITY_ATTRIBUTES.keys()] },
    };
}

/** 状态文件里已有的全部哈希路径：导出进程只读了其中一小部分，裁剪会丢掉其余能力的缓存。 */
function liveHashPaths(state: ReturnType<typeof loadState>): Set<string> {
    return new Set(Object.keys(state.hashes));
}

export async function main(argv: readonly string[]): Promise<number> {
    const write = argv.includes("--write");
    // 参考表为「实现缺口」保留的静态条目（见 harnesses/known_missing_entity_attributes.ts）：
    // 这些实体在 Cubium 侧还不存在（或没有属性系统），oracle 按定义也不给值，导出时沿用旧值。
    const staticEntries = new Set<string>((await import("../harness/harnesses/known_missing_entity_attributes.ts")).KNOWN_MISSING);
    const layout = projectLayout();
    const state = loadState(layout.stateDirectory);
    const session = new HarnessSession({
        layout,
        hashes: state.hashes,
        oracleCache: new OracleCache(layout.stateDirectory, true),
        environment: loadOracleEnvironment(layout),
    });
    const declaration = exportDeclaration();
    const outcome = runDeclaration(session, declaration);
    // 导出依赖 oracle 的输出文件；差异（Cubium 与表不一致）不影响导出本身的结果。
    if (outcome.oracleOutput === undefined || outcome.oracleOutput.length === 0) {
        console.error(`oracle 没有产出结果：${outcome.failure ?? "未知原因"}`);
        return 1;
    }

    const oracleValues = new Map<string, Map<string, number>>();
    for (const line of fs.readFileSync(outcome.oracleOutput, "utf8").split(/\r?\n/)) {
        if (line.trim() === "") {
            continue;
        }
        const record = JSON.parse(line) as { entity: string; missing?: boolean; attributes?: Record<string, string> };
        if (record.missing === true) {
            continue;
        }
        // 键带 `minecraft:` 命名空间（原版注册名），参考表用的是不带命名空间的短名。
        const values = new Map<string, number>();
        for (const [key, bits] of Object.entries(record.attributes ?? {})) {
            values.set(key.replace(/^minecraft:/, ""), bitsToNumber(bits));
        }
        oracleValues.set(record.entity, values);
    }

    const lines: string[] = [];
    for (const [entity, reference] of VANILLA_ENTITY_ATTRIBUTES) {
        const fresh = oracleValues.get(entity);
        let values: Map<string, number>;
        if (fresh === undefined) {
            if (!RUNTIME_DIVERGENT.includes(entity) && !staticEntries.has(entity)) {
                console.error(`跳过 ${entity}：oracle 没有输出，也不在 RUNTIME_DIVERGENT / 实现缺口清单里`);
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

    const block = `${lines.join("\n")}\n`;
    if (!write) {
        console.log(block);
        return 0;
    }

    // 数据段形如 `= new Map<\n    string,\n    ReadonlyMap<string, number>\n>([\n ... ]);`，
    // 只替换 `>([\n` 与 `\n]);` 之间的条目，注释与 RUNTIME_DIVERGENT 原样保留。
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
    // 哈希缓存里有新的文件哈希要落盘（不裁剪：本进程只读了其中一小部分）。
    saveState(layout.stateDirectory, { ...state }, liveHashPaths(state));
    console.log(`已写入 ${TARGET}（${lines.length} 个生物）`);
    return 0;
}

if (import.meta.main) {
    process.exitCode = await main(process.argv.slice(2));
}