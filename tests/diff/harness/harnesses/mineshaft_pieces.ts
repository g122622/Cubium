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
/*
 * mineshaft_pieces.ts — 废弃矿井的构件序列。
 *
 * 与 `structure_starts` 互补：那条能力只看「整座矿井对不对」，一处随机数消耗不同就让
 * 整棵树从某个构件开始分叉，报告里只剩「N 处差异」这种无法定位的信号。
 *
 * 这条能力把矿井拆到构件级：同一种子、同一区块下逐条列出每个构件的生成顺序、类型、
 * 包围盒与朝向。于是「从第几个构件开始分歧」可以直接看出来，再往回追那一次抽取即可。
 *
 * 语料必须由参考侧给出：矿山只落在少数候选区块里，随机撒点几乎全空。这里先用原版的
 * 放置规则枚举候选区块，再让 oracle 打出每个候选区块的构件序列，只保留真的落了矿山的。
 * 少了这一步筛，单元会全是空序列，两侧当然「一致」——那种通过完全无效。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 世界种子：与 structure_starts 同一组，便于交叉核对。 */
const SEEDS = [0, 9248738493917] as const;

/** 每个种子取多少个矿山单元。 */
const PER_SEED = 12;

/** 矿山只出现在主世界。 */
const DIMENSION = "overworld";

/** 候选区块清单的临时文件名。 */
const CANDIDATES_FILE = "mineshaft_pieces_candidates.txt";

/** 探测输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "mineshaft_pieces_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "mineshaft_pieces_probe_output.jsonl";

interface Sample {
    unit: string;
    input: { dimension: string; seed: number; chunkX: number; chunkZ: number };
}

let units: Sample[] = [];

/**
 * 解析 oracle 的候选清单。
 *
 * 行格式是 `<dimension> <seed> <chunkX> <chunkZ>`，列出的是**所有**结构集的候选区块。
 */
function parseCandidateLines(text: string): { chunkX: number; chunkZ: number }[] {
    const result: { chunkX: number; chunkZ: number }[] = [];
    for (const rawLine of text.split(/\r?\n/)) {
        const parts = rawLine.trim().split(/\s+/).filter((part) => part.length > 0);
        if (parts.length < 4) {
            continue;
        }
        const chunkX = Number(parts[2]);
        const chunkZ = Number(parts[3]);
        if (Number.isFinite(chunkX) && Number.isFinite(chunkZ)) {
            result.push({ chunkX, chunkZ });
        }
    }
    return result;
}

/** 用 oracle 打出每个候选区块的构件序列，返回真的落了矿山的那些单元。 */
function collectUnits(directory: string, classpath: string, seed: number): Sample[] {
    const candidatesPath = path.join(directory, CANDIDATES_FILE);
    execFileSync(
        "java",
        ["-cp", classpath, "dev.horizonlab.diff.StructureStartsOracle", "candidates",
            String(seed), String(seed), candidatesPath],
        { cwd: directory, stdio: ["ignore", "pipe", "pipe"] },
    );
    const candidates = parseCandidateLines(fs.readFileSync(candidatesPath, "utf8"));

    const probeInputPath = path.join(directory, PROBE_INPUT_FILE);
    const probeOutputPath = path.join(directory, PROBE_OUTPUT_FILE);
    const probeInput = candidates
        .map((candidate) =>
            JSON.stringify({
                unit: `${DIMENSION}/${seed}/${candidate.chunkX}/${candidate.chunkZ}`,
                dimension: DIMENSION,
                seed,
                chunkX: candidate.chunkX,
                chunkZ: candidate.chunkZ,
            }),
        )
        .join("\n");
    fs.writeFileSync(probeInputPath, `${probeInput}\n`);

    execFileSync(
        "java",
        ["-cp", classpath, "dev.horizonlab.diff.MineshaftPiecesOracle", "run", probeInputPath, probeOutputPath],
        { cwd: directory, stdio: ["ignore", "pipe", "pipe"] },
    );

    const hits = fs
        .readFileSync(probeOutputPath, "utf8")
        .split(/\r?\n/)
        .filter((line) => line.trim().length > 0)
        .map((line) => JSON.parse(line) as { unit: string; present: boolean; pieces: unknown[] })
        .filter((entry) => entry.present && entry.pieces.length > 0);

    const step = Math.max(1, Math.floor(hits.length / PER_SEED));
    const picked: Sample[] = [];
    for (let index = 0; index < hits.length && picked.length < PER_SEED; index += step) {
        const parts = hits[index].unit.split("/");
        picked.push({
            unit: hits[index].unit,
            input: {
                dimension: DIMENSION,
                seed,
                chunkX: Number(parts[2]),
                chunkZ: Number(parts[3]),
            },
        });
    }
    return picked;
}

export function mineshaftPiecesDeclaration(): HarnessDeclaration {
    return {
        name: "mineshaft_pieces",
        probe: "mineshaft_pieces",
        oracle: "MineshaftPiecesOracle",
        key: "unit",
        units: {
            kind: "computed",
            units: (): string[] => units.map((entry) => entry.unit),
        },
        input: (unit: string): unknown => {
            const entry = units.find((candidate) => candidate.unit === unit);
            if (entry === undefined) {
                throw new Error(`mineshaft_pieces: unknown unit ${unit}`);
            }
            return { ...entry.input, unit };
        },
        beforeUnits: (context: HarnessContext): void => {
            const directory = context.outputDirectory("mineshaft_pieces", "oracle");
            context.compileOracle("MineshaftPiecesOracle", "mineshaft_pieces");
            const classpath = context.oracleClasspath(context.classesDirectory("mineshaft_pieces"));
            units = [];
            for (const seed of SEEDS) {
                for (const sample of collectUnits(directory, classpath, seed)) {
                    units.push(sample);
                }
            }
            if (units.length === 0) {
                throw new Error("mineshaft_pieces: 参考侧没有找到任何矿山单元，语料选取失效");
            }
        },
    } satisfies HarnessDeclaration;
}
