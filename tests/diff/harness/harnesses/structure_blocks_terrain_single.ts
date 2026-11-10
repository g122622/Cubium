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
 * structure_blocks_terrain_single.ts — 平坦地形下的结构方块产物。
 *
 * `structure_blocks_single` 的假世界永远是空的：放下之前读到的一切都是空气、所有高度图
 * 都退化为世界最低点。它因此覆盖不到「先读地形再决定写什么/写哪里」的分支。
 *
 * 这条能力在假世界里铺一份两侧完全一致的确定性地形：
 *   - 主世界：y<=50 石头，51..62 水；
 *   - 下界：y<=31 下界岩；
 *   - 末地：y<=50 末地石。
 *
 * 两侧各自按自己的世界 API 语义提供高度：
 *   - 探针（Cubium IWorld）：`getHeight` 返回最高**方块**的 Y；
 *   - oracle（原版 LevelReader）：`getHeight` 返回首个可放置 Y（最高方块 Y + 1）。
 * 两种语义的差值正是结构放置需要正确换算的地方，也是本能力要暴露的差异来源。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

interface Sample {
  readonly unit: string;
  readonly input: {
    readonly dimension: string;
    readonly seed: number;
    readonly chunkX: number;
    readonly chunkZ: number;
    readonly structure: string;
    readonly terrain: boolean;
  };
}

/** 行分隔符：oracle 输出可能是 CRLF。 */
const LINE_BREAK = new RegExp(`${String.fromCharCode(92)}r?${String.fromCharCode(92)}n`);

/** 候选区块表文件名：由 oracle 按放置规则导出。 */
const CANDIDATES_FILE_PREFIX = "structure_blocks_terrain_single_candidates_";

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

/** 每个结构取多少个落点。 */
const PER_STRUCTURE = 5;

/** 语料种子：多个种子扩大覆盖（不同种子抽到的构件组合不同）。 */
const SEEDS = [0, 9248738493917] as const;

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
            terrain: true,
          },
        });
      }
    }
    if (!pickedAny) {
      throw new Error(
        `structure_blocks_terrain_single: 结构集 ${feature.structureSet} 在扫描窗口内没有任何候选区块`,
      );
    }
  }
  return result;
}

export function structureBlocksTerrainSingleDeclaration(): HarnessDeclaration {
  return {
    name: "structure_blocks_terrain_single",
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
        throw new Error(`structure_blocks_terrain_single: unknown unit ${unit}`);
      }
      return { ...entry.input, unit };
    },
    beforeUnits: (context: HarnessContext): void => {
      const directory = context.outputDirectory("structure_blocks_terrain_single", "oracle");
      context.compileOracle("StructureBlocksSingleOracle", "structure_blocks_terrain_single");
      // 候选表由现有 oracle 导出（按真实放置规则算，不手抄坐标）。
      context.compileOracle("StructureStartsOracle", "structure_blocks_terrain_single");
      const classpath = context.oracleClasspath(context.classesDirectory("structure_blocks_terrain_single"));
      units = buildUnits((structureSet, seed) => exportCandidates(directory, classpath, structureSet, seed));
    },
  } satisfies HarnessDeclaration;
}