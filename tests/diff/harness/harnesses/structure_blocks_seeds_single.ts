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
 * structure_blocks_seeds_single.ts — 手工构件结构在额外世界种子下的方块产物。
 *
 * `structure_blocks_single` 只用两个种子，每个结构取五个落点：房间图、房型抽签、
 * 柱子/装饰的随机分支覆盖率有限（海底神殿的翼楼样式、废弃矿井的房型就曾经
 * 因为随机流错位而只在个别单元暴露）。这条能力换三个种子、每结构取两个落点，
 * 在不重复已有单元的前提下扩大随机分支覆盖。
 *
 * 探针与 oracle 沿用 `structure_blocks_single` 的同一套实现。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 探针输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "structure_blocks_seeds_single_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "structure_blocks_seeds_single_probe_output.jsonl";

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
const CANDIDATES_FILE_PREFIX = "structure_blocks_seeds_single_candidates_";

/** 参与的结构：结构集 id + 该集合里要单独生成的结构 id + 维度。 */
const FEATURES = [
  { structureSet: "minecraft:nether_complexes", structure: "minecraft:fortress", dimension: "the_nether" },
  { structureSet: "minecraft:mineshafts", structure: "minecraft:mineshaft", dimension: "overworld" },
  { structureSet: "minecraft:strongholds", structure: "minecraft:stronghold", dimension: "overworld" },
  { structureSet: "minecraft:desert_pyramids", structure: "minecraft:desert_pyramid", dimension: "overworld" },
  { structureSet: "minecraft:jungle_temples", structure: "minecraft:jungle_pyramid", dimension: "overworld" },
  { structureSet: "minecraft:swamp_huts", structure: "minecraft:swamp_hut", dimension: "overworld" },
  { structureSet: "minecraft:ocean_monuments", structure: "minecraft:monument", dimension: "overworld" },
  { structureSet: "minecraft:buried_treasures", structure: "minecraft:buried_treasure", dimension: "overworld" },
  { structureSet: "minecraft:ruined_portals", structure: "minecraft:ruined_portal", dimension: "overworld" },
  { structureSet: "minecraft:nether_fossils", structure: "minecraft:nether_fossil", dimension: "the_nether" },
  { structureSet: "minecraft:igloos", structure: "minecraft:igloo", dimension: "overworld" },
  { structureSet: "minecraft:ocean_ruins", structure: "minecraft:ocean_ruin_cold", dimension: "overworld" },
  { structureSet: "minecraft:ocean_ruins", structure: "minecraft:ocean_ruin_warm", dimension: "overworld" },
  { structureSet: "minecraft:shipwrecks", structure: "minecraft:shipwreck", dimension: "overworld" },
  { structureSet: "minecraft:shipwrecks", structure: "minecraft:shipwreck_beached", dimension: "overworld" },
  { structureSet: "minecraft:woodland_mansions", structure: "minecraft:mansion", dimension: "overworld" },
  { structureSet: "minecraft:end_cities", structure: "minecraft:end_city", dimension: "the_end" },
] as const;

/** 每个结构在每个种子下取多少个落点。 */
const PER_STRUCTURE = 2;

/** 语料种子：与 `structure_blocks_single` 的 [0, 9248738493917] 不重叠。 */
const SEEDS = [123456789, 9876543210123, 4294967291] as const;

let units: Sample[] = [];

/** 导出某个结构集在指定种子下的候选区块表。 */
function exportCandidates(directory: string, classpath: string, structureSet: string, seed: number): string[] {
  const outputPath = path.join(
    directory,
    `${CANDIDATES_FILE_PREFIX}${structureSet.replace(":", "_")}_${seed}.txt`,
  );
  execFileSync(
    "java",
    [
      "-cp",
      classpath,
      "dev.horizonlab.diff.StructureStartsOracle",
      "candidates",
      "--only",
      structureSet,
      String(seed),
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

/** 展平成逐个单元：每个结构在每个种子的候选表里各取均匀分布的若干落点。 */
function buildUnits(readCandidates: (structureSet: string, seed: number) => string[]): Sample[] {
  const result: Sample[] = [];
  for (const feature of FEATURES) {
    let pickedAny = false;
    for (const seed of SEEDS) {
      const rows = readCandidates(feature.structureSet, seed)
        .map((line) => line.split(/\s+/))
        .filter(([dimension, , , , setId]) => dimension === feature.dimension && setId === feature.structureSet);
      if (rows.length === 0) {
        continue;
      }
      pickedAny = true;
      const picked = Array.from({ length: Math.min(PER_STRUCTURE, rows.length) }, (_, index) => {
        return rows[Math.floor((index * rows.length) / Math.max(PER_STRUCTURE, 1))];
      });
      for (const [, seedText, xText, zText] of picked) {
        const chunkX = Number(xText);
        const chunkZ = Number(zText);
        result.push({
          unit: `${feature.structure}/${seed}/${chunkX}/${chunkZ}`,
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
    if (!pickedAny) {
      throw new Error(
        `structure_blocks_seeds_single: 结构集 ${feature.structureSet} 在所有语料种子的扫描窗口内都没有候选区块`,
      );
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

export function structureBlocksSeedsSingleDeclaration(): HarnessDeclaration {
  return {
    name: "structure_blocks_seeds_single",
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
        throw new Error(`structure_blocks_seeds_single: unknown unit ${unit}`);
      }
      return { ...entry.input, unit };
    },
    beforeUnits: (context: HarnessContext): void => {
      const directory = context.outputDirectory("structure_blocks_seeds_single", "oracle");
      context.compileOracle("StructureBlocksSingleOracle", "structure_blocks_seeds_single");
      // 候选表由现有 oracle 导出（按真实放置规则算，不手抄坐标）。
      context.compileOracle("StructureStartsOracle", "structure_blocks_seeds_single");
      const classpath = context.oracleClasspath(context.classesDirectory("structure_blocks_seeds_single"));
      units = buildUnits((structureSet, seed) => exportCandidates(directory, classpath, structureSet, seed));
      runOracle(directory, classpath, units);
    },
  } satisfies HarnessDeclaration;
}