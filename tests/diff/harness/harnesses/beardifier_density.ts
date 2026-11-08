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
 * beardifier_density.ts — 结构地形平滑（Beardifier）的逐点密度贡献。
 *
 * 起点差分证明「构件摆在哪里」，证明不了「这些构件把地形削成什么形状」。
 * 这条能力在同一个起点上再走一遍 Beardifier：筛选参与计算的刚性片段与连接点、
 * 按起点包围盒派生采样网格，逐点输出密度偏移。片段筛选、地面高度偏移、连接点投影
 * 与三条贡献分支（胡须核 / Bury / Encapsulate）任何一处写错都会落在这些数值上。
 *
 * 语料只挑**带地形适配**的结构集：地形适配为「无」的结构整条被跳过，采样恒为 0，
 * 跑多少格都证明不了什么。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { CompareTransform } from "../core/comparison.ts";
import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 探针输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "beardifier_density_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "beardifier_density_probe_output.jsonl";

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

const CANDIDATES_FILE_PREFIX = "beardifier_density_candidates_";

/** 参与的能力：结构集 id → 该集合里要单独生成的结构 id。 */
const FEATURES = [
  { structureSet: "minecraft:villages", structure: "minecraft:village_plains" },
  { structureSet: "minecraft:trial_chambers", structure: "minecraft:trial_chambers" },
  { structureSet: "minecraft:ancient_cities", structure: "minecraft:ancient_city" },
  { structureSet: "minecraft:pillager_outposts", structure: "minecraft:pillager_outpost" },
] as const;

/** 每个结构取多少个落点（每个落点 567 个采样值，够用即可）。 */
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
    // 候选行形如「维度 种子 X Z 结构集 id」；同心环（要塞）不受 `--only` 过滤，按第 5 列剔除。
    const rows = readCandidates(feature.structureSet)
      .map((line) => line.split(/\s+/))
      .filter(([dimension, , , , setId]) => dimension === "overworld" && setId === feature.structureSet);
    const picked = Array.from({ length: Math.min(PER_STRUCTURE, rows.length) }, (_, index) => {
      return rows[Math.floor((index * rows.length) / Math.max(PER_STRUCTURE, 1))];
    });
    if (picked.length === 0) {
      throw new Error(`beardifier_density: 结构集 ${feature.structureSet} 在扫描窗口内没有任何候选区块`);
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
  execFileSync("java", ["-cp", classpath, "dev.horizonlab.diff.BeardifierDensityOracle", "run", inputPath, outputPath], {
    cwd: directory,
    stdio: ["ignore", "pipe", "pipe"],
  });
}

/**
 * 浮点容差：两侧的差别只来自编译期浮点收缩（`-ffast-math` 会把 `a*b+c` 折成 FMA），
 * 相对误差在 1e-16 量级。判断方式是「足够接近就返回参照值」——比较层因此判定相等，
 * 而参照侧自身经过同一变换后保持不变（等价性校验通过）。真正的算术错误远大于这个量级。
 */
const RELATIVE_TOLERANCE = 1e-9;

function closeEnough(expected: unknown, actual: unknown): unknown {
  if (typeof expected === "number" && typeof actual === "number") {
    if (expected === actual) {
      return expected;
    }
    const scale = Math.max(1, Math.abs(expected), Math.abs(actual));
    return Math.abs(expected - actual) <= RELATIVE_TOLERANCE * scale ? expected : actual;
  }
  if (Array.isArray(expected) && Array.isArray(actual)) {
    return actual.map((value, index) => closeEnough(expected[index], value));
  }
  if (expected !== null && actual !== null && typeof expected === "object" && typeof actual === "object") {
    const result: Record<string, unknown> = { ...(actual as Record<string, unknown>) };
    for (const [field, value] of Object.entries(expected as Record<string, unknown>)) {
      if (field in result) {
        result[field] = closeEnough(value, result[field]);
      }
    }
    return result;
  }
  return actual;
}

const FLOAT_TOLERANCE: CompareTransform = (expected: unknown, actual: unknown): unknown => closeEnough(expected, actual);

export function beardifierDensityDeclaration(): HarnessDeclaration {
  return {
    name: "beardifier_density",
    probe: "beardifier_density",
    oracle: "BeardifierDensityOracle",
    key: "unit",
    normalise: FLOAT_TOLERANCE,
    units: {
      kind: "computed",
      units: (): string[] => units.map((entry) => entry.unit),
    },
    input: (unit: string): unknown => {
      const entry = units.find((candidate) => candidate.unit === unit);
      if (entry === undefined) {
        throw new Error(`beardifier_density: unknown unit ${unit}`);
      }
      return { ...entry.input, unit };
    },
    beforeUnits: (context: HarnessContext): void => {
      const directory = context.outputDirectory("beardifier_density", "oracle");
      context.compileOracle("BeardifierDensityOracle", "beardifier_density");
      // 候选表由现有 oracle 导出（按真实放置规则算，不手抄坐标）。
      context.compileOracle("StructureStartsOracle", "beardifier_density");
      const classpath = context.oracleClasspath(context.classesDirectory("beardifier_density"));
      units = buildUnits((structureSet) => exportCandidates(directory, classpath, structureSet));
      runOracle(directory, classpath, units);
    },
  } satisfies HarnessDeclaration;
}