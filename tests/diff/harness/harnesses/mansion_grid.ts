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
 * mansion_grid.ts — 林地府邸的布局网格。
 *
 * 府邸起点由「网格布局 → 放置器」两段组成，两者共用同一条随机流。构件序列一旦错位，
 * 分不清是走廊递归、房间识别（网格）错，还是外墙/房间放置错。这条能力把网格单独拎出来，
 * 用一串连续种子覆盖「有/无三楼」「多种房间类型」的布局分支。
 *
 * 种子直接按固定步长递增：同一段随机流上的相邻种子覆盖的布局形态差异很大。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 探针输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "mansion_grid_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "mansion_grid_probe_output.jsonl";

interface Sample {
  readonly unit: string;
  readonly input: { readonly seed: number };
}

/** 种子数量与步长。 */
const SEED_COUNT = 40;
const SEED_STEP = 7919;

let units: Sample[] = [];

/** 固定种子网格。 */
function buildUnits(): Sample[] {
  const result: Sample[] = [];
  for (let index = 0; index < SEED_COUNT; index++) {
    const seed = index * SEED_STEP;
    result.push({ unit: `seed/${seed}`, input: { seed } });
  }
  return result;
}

/** 用 oracle 的 `run` 子命令一对一跑完输入。 */
function runOracle(directory: string, classpath: string, inputs: Sample[]): void {
  const inputPath = path.join(directory, PROBE_INPUT_FILE);
  const outputPath = path.join(directory, PROBE_OUTPUT_FILE);
  const lines = inputs.map((entry) => JSON.stringify({ unit: entry.unit, ...entry.input }));
  fs.writeFileSync(inputPath, `${lines.join("\n")}\n`);
  execFileSync("java", ["-cp", classpath, "dev.horizonlab.diff.MansionGridOracle", "run", inputPath, outputPath], {
    cwd: directory,
    stdio: ["ignore", "pipe", "pipe"],
  });
}

export function mansionGridDeclaration(): HarnessDeclaration {
  return {
    name: "mansion_grid",
    probe: "mansion_grid",
    oracle: "MansionGridOracle",
    key: "unit",
    units: {
      kind: "computed",
      units: (): string[] => units.map((entry) => entry.unit),
    },
    input: (unit: string): unknown => {
      const entry = units.find((candidate) => candidate.unit === unit);
      if (entry === undefined) {
        throw new Error(`mansion_grid: unknown unit ${unit}`);
      }
      return { ...entry.input, unit };
    },
    beforeUnits: (context: HarnessContext): void => {
      units = buildUnits();
      context.compileOracle("MansionGridOracle", "mansion_grid");
      const classpath = context.oracleClasspath(context.classesDirectory("mansion_grid"));
      runOracle(context.outputDirectory("mansion_grid", "oracle"), classpath, units);
    },
  } satisfies HarnessDeclaration;
}