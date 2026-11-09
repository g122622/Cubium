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
 * structure_blocks_jigsaw_single.ts — 拼图类结构的方块产物。
 *
 * `structure_blocks_single` 只覆盖手工实现的构件（`TemplateStructurePiece` 风格）。
 * 拼图结构走的是另一条放置路径：构件由 `JigsawAssembler` 装配，逐块放置由
 * `SingleJigsawPiece::place` → `Template::placeInWorld(knownShape=true)` 完成，还会
 * 叠加拼图替换、处理器列表、`terrain_matching` 的重力投影等步骤。两条路径任何一步
 * 写错，最终都只会表现为「方块与原点版不同」。
 *
 * 探针与 oracle 沿用 `structure_blocks_single` 的同一套实现：假世界只记录写入，
 * 每个区块用生产同源的随机源依次放置相交构件，最后按坐标输出方块。
 *
 * TODO: 末地城与林地府邸曾因 oracle 空世界桩缺少实体构造环境（潜影贝/灾厄村民）而
 *       未纳入；桩现在已有最小服务端替身，等本能力稳定后单独验证这两者。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 探针输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "structure_blocks_jigsaw_single_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "structure_blocks_jigsaw_single_probe_output.jsonl";

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
const CANDIDATES_FILE_PREFIX = "structure_blocks_jigsaw_single_candidates_";

/** 参与的结构：结构集 id + 该集合里要单独生成的结构 id + 维度。 */
const FEATURES = [
  { structureSet: "minecraft:ancient_cities", structure: "minecraft:ancient_city", dimension: "overworld" },
  { structureSet: "minecraft:trial_chambers", structure: "minecraft:trial_chambers", dimension: "overworld" },
  { structureSet: "minecraft:trail_ruins", structure: "minecraft:trail_ruins", dimension: "overworld" },
  { structureSet: "minecraft:villages", structure: "minecraft:village_plains", dimension: "overworld" },
  { structureSet: "minecraft:pillager_outposts", structure: "minecraft:pillager_outpost", dimension: "overworld" },
  { structureSet: "minecraft:nether_complexes", structure: "minecraft:bastion_remnant", dimension: "the_nether" },
] as const;

/** 每个结构取多少个落点。布局越大的拼图结构放置越慢，这里比手工构件少取一些。 */
const PER_STRUCTURE = 3;

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
      throw new Error(
        `structure_blocks_jigsaw_single: 结构集 ${feature.structureSet} 在扫描窗口内没有任何候选区块`,
      );
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

export function structureBlocksJigsawSingleDeclaration(): HarnessDeclaration {
  return {
    name: "structure_blocks_jigsaw_single",
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
        throw new Error(`structure_blocks_jigsaw_single: unknown unit ${unit}`);
      }
      return { ...entry.input, unit };
    },
    beforeUnits: (context: HarnessContext): void => {
      const directory = context.outputDirectory("structure_blocks_jigsaw_single", "oracle");
      context.compileOracle("StructureBlocksSingleOracle", "structure_blocks_jigsaw_single");
      // 候选表由现有 oracle 导出（按真实放置规则算，不手抄坐标）。
      context.compileOracle("StructureStartsOracle", "structure_blocks_jigsaw_single");
      const classpath = context.oracleClasspath(context.classesDirectory("structure_blocks_jigsaw_single"));
      units = buildUnits((structureSet) => exportCandidates(directory, classpath, structureSet));
      runOracle(directory, classpath, units);
    },
  } satisfies HarnessDeclaration;
}