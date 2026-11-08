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
 * structure_blocks_single.ts — 单个结构在候选区块上的方块产物。
 *
 * `structure_start_single` 只比到构件列表；构件真正写入世界的方块（`postProcess`）
 * 没有任何覆盖。这条能力在同一份起点上继续跑一步：起点覆盖的每个区块各跑一遍放置，
 * 比较最终方块表。
 *
 * 语料沿用 `structure_start_single` 的候选区块导出（由原版放置规则算出）。
 *
 * TODO: 矿井（minecraft:mineshaft）与要塞（minecraft:stronghold）目前仍有差异：
 *       矿井的 postProcess 还是自造的简化实现（不按原版开凿 cave_air，也不做邻接判定），
 *       要塞的装饰件（门、铁栏杆、按钮、火把、台阶）尚未补齐。两者都要按原版逐个片段
 *       重写后再纳入「必须全绿」的范围。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 探针输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "structure_blocks_single_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "structure_blocks_single_probe_output.jsonl";

interface Sample {
  readonly unit: string;
  readonly input: {
    readonly dimension: string;
    readonly seed: number;
    readonly chunkX: number;
    readonly chunkZ: number;
    readonly structure: string;
  };
}

/** 行分隔符：oracle 输出可能是 CRLF。 */
const LINE_BREAK = new RegExp(`${String.fromCharCode(92)}r?${String.fromCharCode(92)}n`);

/** 候选区块表文件名：由 oracle 按放置规则导出。 */
const CANDIDATES_FILE_PREFIX = "structure_blocks_single_candidates_";

/** 参与的结构：结构集 id + 该集合里要单独生成的结构 id + 维度。 */
const FEATURES = [
  { structureSet: "minecraft:nether_complexes", structure: "minecraft:fortress", dimension: "the_nether" },
  { structureSet: "minecraft:mineshafts", structure: "minecraft:mineshaft", dimension: "overworld" },
  { structureSet: "minecraft:strongholds", structure: "minecraft:stronghold", dimension: "overworld" },
  { structureSet: "minecraft:desert_pyramids", structure: "minecraft:desert_pyramid", dimension: "overworld" },
  { structureSet: "minecraft:jungle_temples", structure: "minecraft:jungle_pyramid", dimension: "overworld" },
  { structureSet: "minecraft:swamp_huts", structure: "minecraft:swamp_hut", dimension: "overworld" },
] as const;

/** 每个结构取多少个落点。 */
const PER_STRUCTURE = 5;

/** 语料种子。 */
const SEED = 0;

let units: Sample[] = [];

/** 导出某个结构集的候选区块表。 */
function exportCandidates(directory: string, classpath: string, structureSet: string): string[] {
  const outputPath = path.join(directory, `${CANDIDATES_FILE_PREFIX}${structureSet.replace(":", "_")}.txt`);
  execFileSync(
    "java",
    [
      "-cp",
      classpath,
      "dev.horizonlab.diff.StructureStartsOracle",
      "candidates",
      "--only",
      structureSet,
      String(SEED),
      outputPath,
    ],
    { cwd: directory, stdio: ["ignore", "pipe", "pipe"] },
  );
  return fs
    .readFileSync(outputPath, "utf8")
    .split(LINE_BREAK)
    .map((line) => line.trim())
    .filter((line) => line !== "");
}

/** 展平成逐个单元：每个结构取候选表里均匀分布的若干落点。 */
function buildUnits(readCandidates: (structureSet: string) => string[]): Sample[] {
  const result: Sample[] = [];
  for (const feature of FEATURES) {
    const rows = readCandidates(feature.structureSet)
      .map((line) => line.split(/\s+/))
      .filter(([dimension, , , , setId]) => dimension === feature.dimension && setId === feature.structureSet);
    if (rows.length === 0) {
      throw new Error(`structure_blocks_single: 结构集 ${feature.structureSet} 在扫描窗口内没有任何候选区块`);
    }
    const picked = Array.from({ length: Math.min(PER_STRUCTURE, rows.length) }, (_, index) => {
      return rows[Math.floor((index * rows.length) / Math.max(PER_STRUCTURE, 1))];
    });
    for (const [, seedText, xText, zText] of picked) {
      const chunkX = Number(xText);
      const chunkZ = Number(zText);
      result.push({
        unit: `${feature.structure}/${SEED}/${chunkX}/${chunkZ}`,
        input: {
          dimension: feature.dimension,
          seed: Number(seedText),
          chunkX,
          chunkZ,
          structure: feature.structure,
        },
      });
    }
  }
  return result;
}

/** 用 oracle 的 `run` 子命令一对一跑完输入。 */
function runOracle(directory: string, classpath: string, inputs: Sample[]): void {
  const inputPath = path.join(directory, PROBE_INPUT_FILE);
  const outputPath = path.join(directory, PROBE_OUTPUT_FILE);
  const lines = inputs.map((entry) => JSON.stringify({ unit: entry.unit, ...entry.input }));
  fs.writeFileSync(inputPath, `${lines.join("\n")}\n`);
  execFileSync("java", ["-cp", classpath, "dev.horizonlab.diff.StructureBlocksSingleOracle", "run", inputPath, outputPath], {
    cwd: directory,
    stdio: ["ignore", "pipe", "pipe"],
  });
}

export function structureBlocksSingleDeclaration(): HarnessDeclaration {
  return {
    name: "structure_blocks_single",
    probe: "structure_blocks_single",
    oracle: "StructureBlocksSingleOracle",
    key: "unit",
    units: {
      kind: "computed",
      units: (): string[] => units.map((entry) => entry.unit),
    },
    input: (unit: string): unknown => {
      const entry = units.find((candidate) => candidate.unit === unit);
      if (entry === undefined) {
        throw new Error(`structure_blocks_single: unknown unit ${unit}`);
      }
      return { ...entry.input, unit };
    },
    beforeUnits: (context: HarnessContext): void => {
      const directory = context.outputDirectory("structure_blocks_single", "oracle");
      context.compileOracle("StructureBlocksSingleOracle", "structure_blocks_single");
      // 候选表由现有 oracle 导出（按真实放置规则算，不手抄坐标）。
      context.compileOracle("StructureStartsOracle", "structure_blocks_single");
      const classpath = context.oracleClasspath(context.classesDirectory("structure_blocks_single"));
      units = buildUnits((structureSet) => exportCandidates(directory, classpath, structureSet));
      runOracle(directory, classpath, units);
    },
  } satisfies HarnessDeclaration;
}