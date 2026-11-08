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
 * structure_start_single.ts — 单个结构在指定区块上的起点。
 *
 * `structure_starts` 把「结构集选择 → 权重抽签 → 结构自身生成」三层混在一个单元里，
 * 一旦起点不一致就分不清是哪一层错。这条能力直接点名一个结构，绕开结构集，
 * 只让该结构在这一格的随机源上生成一次：层级差异立刻归因。
 *
 * 语料里的区块坐标都来自原版各结构集真实命中的落点（每格都由放置规则自己选中），
 * 因此每个单元都应该产出有效起点。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 探针输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "structure_start_single_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "structure_start_single_probe_output.jsonl";

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

/** 候选区块表文件名：由 oracle 按放置规则导出（每个结构集真实命中的区块）。 */
const CANDIDATES_FILE_PREFIX = "structure_start_single_candidates_";

/** 参与的能力：结构集 id → 该集合里要单独生成的结构 id。 */
const FEATURES = [
  { structureSet: "minecraft:swamp_huts", structure: "minecraft:swamp_hut" },
  { structureSet: "minecraft:shipwrecks", structure: "minecraft:shipwreck" },
  { structureSet: "minecraft:ocean_ruins", structure: "minecraft:ocean_ruin_cold" },
  { structureSet: "minecraft:ocean_ruins", structure: "minecraft:ocean_ruin_warm" },
  { structureSet: "minecraft:woodland_mansions", structure: "minecraft:mansion" },
  { structureSet: "minecraft:igloos", structure: "minecraft:igloo" },
  { structureSet: "minecraft:trial_chambers", structure: "minecraft:trial_chambers" },
  { structureSet: "minecraft:ancient_cities", structure: "minecraft:ancient_city" },
  { structureSet: "minecraft:villages", structure: "minecraft:village_plains" },
  { structureSet: "minecraft:pillager_outposts", structure: "minecraft:pillager_outpost" },
  { structureSet: "minecraft:mineshafts", structure: "minecraft:mineshaft" },
  { structureSet: "minecraft:strongholds", structure: "minecraft:stronghold" },
] as const;

/** 每个结构取多少个落点。 */
const PER_STRUCTURE = 5;

/** 语料种子。 */
const SEED = 0;

let units: Sample[] = [];

/**
 * 导出某个结构集的候选区块表。
 *
 * 候选行的格式是「维度 种子 X Z 结构集 id」。oracle 一次只接受一个 `--only`，按需逐个导出；
 * 完整枚举所有结构集要按网格逐格扫描，分钟级起步。同心环（要塞）不受 `--only` 过滤，
 * 因此在任何一次导出里都会混进来，读取方按第 5 列分回各自的集合。
 */
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
    // 候选行形如「维度 种子 X Z 结构集 id」。结构集按需逐个导出，但同心环（要塞）不受
    // `--only` 过滤，永远混在文件里，必须按第 5 列把行分回各自的集合。
    const rows = readCandidates(feature.structureSet)
      .map((line) => line.split(/\s+/))
      .filter(([dimension, , , , setId]) => dimension === "overworld" && setId === feature.structureSet);
    // 均匀抽样：候选表按网格扫描顺序排列，只取前 N 个会集中在同一条带里。
    const picked = Array.from({ length: Math.min(PER_STRUCTURE, rows.length) }, (_, index) => {
      return rows[Math.floor((index * rows.length) / Math.max(PER_STRUCTURE, 1))];
    });
    // 稀疏结构（掠夺者前哨站等）在扫描窗口里可能只有个位数落点，此时按实际数量取全量。
    if (picked.length === 0) {
      throw new Error(`structure_start_single: 结构集 ${feature.structureSet} 在扫描窗口内没有任何候选区块`);
    }
    for (const [, seedText, xText, zText] of picked) {
      const chunkX = Number(xText);
      const chunkZ = Number(zText);
      result.push({
        unit: `${feature.structure}/${SEED}/${chunkX}/${chunkZ}`,
        input: {
          dimension: "overworld",
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
  execFileSync("java", ["-cp", classpath, "dev.horizonlab.diff.StructureStartSingleOracle", "run", inputPath, outputPath], {
    cwd: directory,
    stdio: ["ignore", "pipe", "pipe"],
  });
}

export function structureStartSingleDeclaration(): HarnessDeclaration {
  return {
    name: "structure_start_single",
    probe: "structure_start_single",
    oracle: "StructureStartSingleOracle",
    key: "unit",
    units: {
      kind: "computed",
      units: (): string[] => units.map((entry) => entry.unit),
    },
    input: (unit: string): unknown => {
      const entry = units.find((candidate) => candidate.unit === unit);
      if (entry === undefined) {
        throw new Error(`structure_start_single: unknown unit ${unit}`);
      }
      return { ...entry.input, unit };
    },
    beforeUnits: (context: HarnessContext): void => {
      const directory = context.outputDirectory("structure_start_single", "oracle");
      context.compileOracle("StructureStartSingleOracle", "structure_start_single");
      // 候选表由现有 oracle 导出（按真实放置规则算，不手抄坐标）。
      context.compileOracle("StructureStartsOracle", "structure_start_single");
      const classpath = context.oracleClasspath(context.classesDirectory("structure_start_single"));
      units = buildUnits((structureSet) => exportCandidates(directory, classpath, structureSet));
      runOracle(directory, classpath, units);
    },
  } satisfies HarnessDeclaration;
}