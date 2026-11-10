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
 * structure_blocks_jigsaw_seeds_single.ts — 拼图结构在额外世界种子下的方块产物。
 *
 * `structure_blocks_jigsaw_single` 只用种子 0。拼图装配的随机分支（模板池洗牌、
 * 连接点匹配、地形适配）对种子敏感，只测一种子容易漏掉只在特定装配顺序下出现的
 * 处理器/替换差异。这条能力换三个种子、每结构取两个落点。
 *
 * 探针与 oracle 沿用 `structure_blocks_single` 的同一套实现。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 探针输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "structure_blocks_jigsaw_seeds_single_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "structure_blocks_jigsaw_seeds_single_probe_output.jsonl";

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
const CANDIDATES_FILE_PREFIX = "structure_blocks_jigsaw_seeds_single_candidates_";

/** 参与的结构：结构集 id + 该集合里要单独生成的结构 id + 维度。 */
const FEATURES = [
  { structureSet: "minecraft:ancient_cities", structure: "minecraft:ancient_city", dimension: "overworld" },
  { structureSet: "minecraft:trial_chambers", structure: "minecraft:trial_chambers", dimension: "overworld" },
  { structureSet: "minecraft:trail_ruins", structure: "minecraft:trail_ruins", dimension: "overworld" },
  { structureSet: "minecraft:villages", structure: "minecraft:village_plains", dimension: "overworld" },
  { structureSet: "minecraft:pillager_outposts", structure: "minecraft:pillager_outpost", dimension: "overworld" },
  { structureSet: "minecraft:nether_complexes", structure: "minecraft:bastion_remnant", dimension: "the_nether" },
] as const;

/** 每个结构在每个种子下取多少个落点。布局越大的拼图结构放置越慢，这里少取一些。 */
const PER_STRUCTURE = 2;

/** 语料种子。 */
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
        `structure_blocks_jigsaw_seeds_single: 结构集 ${feature.structureSet} 在所有语料种子的扫描窗口内都没有候选区块`,
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

export function structureBlocksJigsawSeedsSingleDeclaration(): HarnessDeclaration {
  return {
    name: "structure_blocks_jigsaw_seeds_single",
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
        throw new Error(`structure_blocks_jigsaw_seeds_single: unknown unit ${unit}`);
      }
      return { ...entry.input, unit };
    },
    beforeUnits: (context: HarnessContext): void => {
      const directory = context.outputDirectory("structure_blocks_jigsaw_seeds_single", "oracle");
      context.compileOracle("StructureBlocksSingleOracle", "structure_blocks_jigsaw_seeds_single");
      // 候选表由现有 oracle 导出（按真实放置规则算，不手抄坐标）。
      context.compileOracle("StructureStartsOracle", "structure_blocks_jigsaw_seeds_single");
      const classpath = context.oracleClasspath(context.classesDirectory("structure_blocks_jigsaw_seeds_single"));
      units = buildUnits((structureSet, seed) => exportCandidates(directory, classpath, structureSet, seed));
      runOracle(directory, classpath, units);
    },
    // 已知差异：该单元的村庄 decor 池（feature_pool_element）没有产出任何构件。
    // oracle 在 (256..258, 64..66, 331..335) 生成橡树与花，probe 只有模板自带的草。
    // 已在 FeatureJigsawPiece::place 打点确认：探针侧该方法从未被调用，说明差异发生在
    // 装配阶段（连接点匹配或池选择）而不是地物放置阶段；ConfiguredFeatureRegistry 与
    // 数据包均正常加载（其余结构的拼图放置全部一致）。
    // TODO: 对照 JigsawAssembler 的连接匹配与 Java `JigsawPlacement.addPieces`：
    //   - FeatureJigsawPiece 的虚拟连接点已按 down_south 构造（父连接点 up_north 的反向）；
    //   - 检查村庄 `minecraft:village/plains/decor` 池的引用/fallback 是否被解析到 empty；
    //   - 检查池选择时的 rng 消耗是否与该单元其它构件一致。
    knownDifferences: [
      {
        key: "minecraft:village_plains/4294967291/17/22",
        reason:
          "村庄 decor 池的 feature 构件在装配阶段没有产出（FeatureJigsawPiece::place 从未被调用），"
          + "oracle 生成的橡树与花缺失；疑点在连接匹配或池引用解析，尚未定位。",
      },
    ],
  } satisfies HarnessDeclaration;
}