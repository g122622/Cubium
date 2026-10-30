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
 * structure_starts.ts — 结构起点的共同语料。
 *
 * 被测对象是「给定维度、世界种子与区块坐标，这个区块里落下了哪些结构起点、它们由哪些
 * 片段按什么顺序装配而成」。两侧各自跑自己那一段生产逻辑（原版 `createStructures`、
 * 本项目 `generateStructureStarts`），输入完全一致。
 *
 * 语料怎么选：结构只在放置规则算出的极少数「候选区块」里落点（随机散布的入选率约
 * spacing⁻²，同心环更是几十万分之一的区块），随机撒点几乎全是空结果。因此语料先用
 * **原版自己的放置规则**把候选区块枚举出来，再挑一部分喂给两侧——这样每个单元都至少
 * 在「候选区块」这一层有意义，覆盖到真正的装配逻辑。
 *
 * 候选区块从 oracle 的一次性导出里来（`beforeUnits` 里跑 oracle 的 `candidates` 子命令）。
 * 导出的是「原版的候选区块」而不是本项目的：反过来做会把本项目的放置偏差藏起来，
 * 参照侧必须独立给出这一层。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 世界种子。取两个差异较大的值，覆盖彼此不相干的群系与放置落点。 */
const SEEDS = [0, 9248738493917] as const;

/** 维度名，与两侧的维度枚举同名。 */
const DIMENSIONS = ["overworld", "the_end"] as const;

/** 每个维度从原版候选区块里取多少个（均匀抽样，保证间隔覆盖整张表）。 */
const PER_DIMENSION = 40;

/** 同心环（要塞）额外取多少个：它的落点在很远的环上，逐块扫描扫不到。 */
const RING_SAMPLES = 24;

/** oracle 导出候选区块的临时文件名。 */
const CANDIDATES_FILE = "structure_starts_candidates.txt";

interface Candidate {
    readonly dimension: string;
    readonly seed: number;
    readonly chunkX: number;
    readonly chunkZ: number;
}

/** 自乘同余序列：固定种子、全平台一致。 */
function randomSource(seed: number): (bound: number) => number {
    let state = seed >>> 0;
    return (bound: number): number => {
        state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
        return Math.floor((state / 0x100000000) * bound);
    };
}

/**
 * 均匀地从一个有序列表里取 n 个：按等距下标取，首尾都落在表里。
 *
 * 不用随机抽取——候选区块表本身已经是原版随机性的产物，再随机一次只会让语料
 * 在不同运行之间漂移；等距取样还能把列表的首尾（最远/最近的环）都覆盖到。
 */
function evenly<T>(items: readonly T[], count: number): T[] {
    if (items.length <= count) {
        return [...items];
    }
    const result: T[] = [];
    for (let i = 0; i < count; i++) {
        result.push(items[Math.floor((i * items.length) / count)]);
    }
    return result;
}

function parseCandidate(line: string): Candidate {
    const [dimension, seedText, xText, zText] = line.trim().split(/\s+/);
    return { dimension, seed: Number(seedText), chunkX: Number(xText), chunkZ: Number(zText) };
}

/**
 * 读 oracle 导出的候选区块，按「维度 + 种子」分组后等距取样。
 *
 * 每组末尾追加一份「候选区块的邻居」：相邻区块里通常没有结构，用来覆盖「候选区块判定
 * 不通过」这条路径——只喂候选区块会让两侧在放置规则上的偏差（例如多一个/少一个候选）
 * 永远测不出来。
 */
function pickUnits(lines: readonly string[]): Candidate[] {
    const groups = new Map<string, Candidate[]>();
    for (const line of lines) {
        if (line.trim() === "") {
            continue;
        }
        const candidate = parseCandidate(line);
        const key = `${candidate.dimension}/${candidate.seed}`;
        const bucket = groups.get(key);
        if (bucket === undefined) {
            groups.set(key, [candidate]);
        } else {
            bucket.push(candidate);
        }
    }
    const units: Candidate[] = [];
    for (const bucket of groups.values()) {
        const ring = bucket.filter((candidate) => candidate.chunkX * candidate.chunkX + candidate.chunkZ * candidate.chunkZ > 4096);
        const spread = bucket.filter((candidate) => !ring.includes(candidate));
        for (const candidate of evenly(spread, PER_DIMENSION)) {
            units.push(candidate);
            units.push({ ...candidate, chunkX: candidate.chunkX + 1 });
            units.push({ ...candidate, chunkZ: candidate.chunkZ - 1 });
        }
        for (const candidate of evenly(ring, RING_SAMPLES)) {
            units.push(candidate);
        }
    }
    // 再补几个「大概率什么都没有」的区块：靠近原点的普通区块，覆盖「整个维度无落点」的路径。
    const next = randomSource(0x53545354); // "STST"
    for (const dimension of DIMENSIONS) {
        for (const seed of SEEDS) {
            for (let i = 0; i < 8; i++) {
                units.push({ dimension, seed, chunkX: next(64) - 32, chunkZ: next(64) - 32 });
            }
        }
    }
    return units;
}

function unitKey(candidate: Candidate): string {
    return `${candidate.dimension}/${candidate.seed}/${candidate.chunkX}/${candidate.chunkZ}`;
}

/** 单元键：`<维度>/<种子>/<区块X>/<区块Z>`，两侧按同一份语料构造。 */
export function structureStartsDeclaration(): HarnessDeclaration {
    let units: { readonly unit: string; readonly input: Candidate }[] = [];
    return {
        name: "structure_starts",
        probe: "structure_starts",
        oracle: "StructureStartsOracle",
        key: "unit",
        units: {
            kind: "computed",
            units: (): string[] => units.map((entry) => entry.unit),
        },
        input: (unit: string): unknown => {
            const entry = units.find((candidate) => candidate.unit === unit);
            if (entry === undefined) {
                throw new Error(`structure_starts: unknown unit ${unit}`);
            }
            return { ...entry.input, unit };
        },
        beforeUnits: (context: HarnessContext): void => {
            // 候选区块由参照实现给出：本项目的放置规则是否与原版一致，正是要测的东西之一。
            // 若改成由探针导出，两侧的放置偏差就永远无法暴露。
            const directory = context.outputDirectory("structure_starts", "oracle");
            context.compileOracle("StructureStartsOracle", "structure_starts");
            const candidatesPath = path.join(directory, CANDIDATES_FILE);
            execFileSync(
                "java",
                [
                    "-cp",
                    context.oracleClasspath(context.classesDirectory("structure_starts")),
                    "dev.horizonlab.diff.StructureStartsOracle",
                    "candidates",
                    String(SEEDS[0]),
                    String(SEEDS[1]),
                    candidatesPath,
                ],
                { cwd: directory, env: context.environment, stdio: ["ignore", "pipe", "pipe"] },
            );
            const candidates = pickUnits(fs.readFileSync(candidatesPath, "utf8").split(/\r?\n/));
            units = candidates.map((candidate) => ({ unit: unitKey(candidate), input: candidate }));
        },
    } satisfies HarnessDeclaration;
}