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
 * end_city_pieces.ts — 末地城的构件序列。
 *
 * 末地城没有模板池，整座城由生成器递归拼出来，`structure_starts` 对它只能看到整体
 * 包围盒。这条能力把整座城拆到构件级：同一种子、同一区块下逐条列出构件的生成顺序、
 * 模板名、包围盒、旋转与生成深度。
 *
 * 为什么必须有这条能力：1.21.11 的 `recursiveChildren` 在提交一批构件前，用一次
 * `nextInt()` 的返回值覆盖这批构件的 `genDepth`，随后只与 `genDepth` 不同的已有构件
 * 比对包围盒；桥段固定 `genDepth = -1` 以豁免全部比对。这条规则决定了整座城的形状。
 * 只看整体包围盒时，规则写错与随机数消耗写错表现完全一样；拆到构件级才分得开。
 *
 * 语料必须由参考侧给出：末地城只落在末地岛的少数区块里，随机撒点几乎全空。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 世界种子：与 structure_starts 同一组，便于交叉核对。 */
const SEEDS = [0, 9248738493917] as const;

/** 每个种子取多少个末地城单元。 */
const PER_SEED = 8;

/** 末地城只在末地。 */
const DIMENSION = "the_end";

/** 候选区块清单的临时文件名。 */
const CANDIDATES_FILE = "end_city_pieces_candidates.txt";

/** 探测输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "end_city_pieces_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "end_city_pieces_probe_output.jsonl";

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

/** 用 oracle 打出每个候选区块的构件序列，返回真的落了末地城的那些单元。 */
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
        ["-cp", classpath, "dev.horizonlab.diff.EndCityPiecesOracle", "run", probeInputPath, probeOutputPath],
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

export function endCityPiecesDeclaration(): HarnessDeclaration {
    return {
        name: "end_city_pieces",
        probe: "end_city_pieces",
        oracle: "EndCityPiecesOracle",
        key: "unit",
        units: {
            kind: "computed",
            units: (): string[] => units.map((entry) => entry.unit),
        },
        input: (unit: string): unknown => {
            const entry = units.find((candidate) => candidate.unit === unit);
            if (entry === undefined) {
                throw new Error(`end_city_pieces: unknown unit ${unit}`);
            }
            return { ...entry.input, unit };
        },
        beforeUnits: (context: HarnessContext): void => {
            const directory = context.outputDirectory("end_city_pieces", "oracle");
            context.compileOracle("EndCityPiecesOracle", "end_city_pieces");
            const classpath = context.oracleClasspath(context.classesDirectory("end_city_pieces"));
            units = [];
            for (const seed of SEEDS) {
                for (const sample of collectUnits(directory, classpath, seed)) {
                    units.push(sample);
                }
            }
            if (units.length === 0) {
                throw new Error("end_city_pieces: 参考侧没有找到任何末地城单元，语料选取失效");
            }
        },
    } satisfies HarnessDeclaration;
}