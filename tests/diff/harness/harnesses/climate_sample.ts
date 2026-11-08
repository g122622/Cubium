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
 * climate_sample.ts — 气候采样器在三维网格上的取值。
 *
 * 群系采样只有「最终选中哪个群系」一层结果，一旦错位就分不清是气候噪声算错、
 * 还是参数表/RTree 最近邻选错。这条能力把采样器单独拎出来：固定网格逐点输出
 * 六条气候参数的量化值（与参数表同口径），比较是精确的整数比较。
 *
 * 单元是「维度 × 种子 × 区块」的笛卡尔积，区块刻意跨越正负坐标与远距离，
 * 让气候参数落在各种组合区间上。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 探针输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "climate_sample_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "climate_sample_probe_output.jsonl";

interface Sample {
  readonly unit: string;
  readonly input: {
    readonly dimension: string;
    readonly seed: number;
    readonly chunkX: number;
    readonly chunkZ: number;
  };
}

/** 行分隔符：oracle 输出可能是 CRLF。 */
const LINE_BREAK = new RegExp(`${String.fromCharCode(92)}r?${String.fromCharCode(92)}n`);

/** 参与采样的维度。 */
const DIMENSIONS = ["overworld", "the_nether", "the_end"] as const;

/** 种子与区块坐标：正负坐标与远距离各取一些。 */
const SEEDS = [0, 12345];

const CHUNKS = [
  { x: 0, z: 0 },
  { x: 37, z: -92 },
  { x: -1000, z: 500 },
] as const;

/** 用 oracle 的 `run` 子命令一对一跑完输入。 */
function runOracle(directory: string, classpath: string, inputs: readonly Sample[]): void {
  const inputPath = path.join(directory, PROBE_INPUT_FILE);
  const outputPath = path.join(directory, PROBE_OUTPUT_FILE);
  const lines = inputs.map((entry) => JSON.stringify({ unit: entry.unit, ...entry.input }));
  fs.writeFileSync(inputPath, `${lines.join("\n")}\n`);
  execFileSync("java", ["-cp", classpath, "dev.horizonlab.diff.ClimateSampleOracle", "run", inputPath, outputPath], {
    cwd: directory,
    stdio: ["ignore", "pipe", "pipe"],
  });
}

function buildUnits(): Sample[] {
  const result: Sample[] = [];
  for (const dimension of DIMENSIONS) {
    for (const seed of SEEDS) {
      for (const chunk of CHUNKS) {
        result.push({
          unit: `${dimension}/${seed}/${chunk.x}/${chunk.z}`,
          input: { dimension, seed, chunkX: chunk.x, chunkZ: chunk.z },
        });
      }
    }
  }
  return result;
}

export function climateSampleDeclaration(): HarnessDeclaration {
  const units: Sample[] = buildUnits();
  return {
    name: "climate_sample",
    probe: "climate_sample",
    oracle: "ClimateSampleOracle",
    key: "unit",
    units: { kind: "computed", units: (): string[] => units.map((entry) => entry.unit) },
    input: (unit: string): unknown => {
      const entry = units.find((candidate) => candidate.unit === unit);
      if (entry === undefined) {
        throw new Error(`climate_sample: unknown unit ${unit}`);
      }
      return { ...entry.input, unit };
    },
    beforeUnits: (context: HarnessContext): void => {
      const directory = context.outputDirectory("climate_sample", "oracle");
      context.compileOracle("ClimateSampleOracle", "climate_sample");
      // 维度构造复用既有 oracle：它负责按维度名装配生成器与随机状态。
      context.compileOracle("StructureStartsOracle", "climate_sample");
      const classpath = context.oracleClasspath(context.classesDirectory("climate_sample"));
      runOracle(directory, classpath, units);
    },
  } satisfies HarnessDeclaration;
}