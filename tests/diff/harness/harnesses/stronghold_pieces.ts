/*
 * stronghold_pieces.ts — 要塞的构件序列。
 *
 * 与 `structure_starts` 互补：那条能力把要塞当成一整块比较，而要塞动辄两三百个构件，
 * 一处随机数消耗不同就让整棵树从某个构件开始分叉，报告里只剩「这个区块不一致」。
 *
 * 这条能力把要塞拆到构件级：同一种子、同一区块下逐条列出每个构件的生成顺序、类型、
 * 包围盒、朝向与深度，于是「从第几个构件开始分歧」可以直接看出来，再往回追那一次抽取。
 *
 * 语料由参考侧给出：要塞只落在同心环计算出的少数区块里，随机撒点几乎全空。这里先用
 * 原版的候选区块枚举（`StructureStartsOracle candidates`）拿到候选，再用 oracle 打出
 * 每个候选区块的构件序列，只保留真的落了要塞的。少了这一步筛，单元会全是空序列，
 * 两侧当然「一致」——那种通过完全无效。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 世界种子：与 structure_starts 同一组，便于交叉核对。 */
const SEEDS = [0, 9248738493917] as const;

/** 每个种子取多少个要塞单元。 */
const PER_SEED = 6;

/** 要塞只出现在主世界。 */
const DIMENSION = "overworld";

/** 候选区块清单的临时文件名。 */
const CANDIDATES_FILE = "stronghold_pieces_candidates.txt";

/** 探测输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "stronghold_pieces_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "stronghold_pieces_probe_output.jsonl";

interface Sample {
  unit: string;
  input: { dimension: string; seed: number; chunkX: number; chunkZ: number };
}

let units: Sample[] = [];

/** 解析候选清单行 `<dimension> <seed> <chunkX> <chunkZ>`。 */
function parseCandidateLines(
  text: string,
): { chunkX: number; chunkZ: number }[] {
  const result: { chunkX: number; chunkZ: number }[] = [];
  for (const rawLine of text.split(/\r?\n/)) {
    const parts = rawLine
      .trim()
      .split(/\s+/)
      .filter((part) => part.length > 0);
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

/**
 * 用 oracle 的 `candidates` 取该种子的要塞候选区块，返回真的落了要塞的那些单元。
 *
 * `candidates` 对同心环放置直接输出环上位置，只在这个放置类型上枚举；全局随机散布的
 * 结构集（上百个）每个都要完整判定一遍，要塞根本用不到。因此这里让 oracle 只跑要塞候选，
 * 而不是把全部结构集的候选都打出来再筛。
 */
function collectUnits(
  directory: string,
  classpath: string,
  seed: number,
  env: NodeJS.ProcessEnv,
): Sample[] {
  const candidatesPath = path.join(directory, CANDIDATES_FILE);
  execFileSync(
    "java",
    [
      "-cp",
      classpath,
      "dev.horizonlab.diff.StructureStartsOracle",
      "candidates",
      "--only",
      "minecraft:strongholds",
      String(seed),
      candidatesPath,
    ],
    { cwd: directory, env, stdio: ["ignore", "pipe", "pipe"] },
  );
  const candidates = parseCandidateLines(
    fs.readFileSync(candidatesPath, "utf8"),
  );

  const hits = candidates.map((candidate) => ({
    unit: `${DIMENSION}/${seed}/${candidate.chunkX}/${candidate.chunkZ}`,
    chunkX: candidate.chunkX,
    chunkZ: candidate.chunkZ,
  }));

  // 等距取样：候选清单本身已是原版随机性的产物，再随机一次只会让语料在运行之间漂移。
  const step = Math.max(1, Math.floor(hits.length / PER_SEED));
  const picked: Sample[] = [];
  for (
    let index = 0;
    index < hits.length && picked.length < PER_SEED;
    index += step
  ) {
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

export function strongholdPiecesDeclaration(): HarnessDeclaration {
  return {
    name: "stronghold_pieces",
    probe: "stronghold_pieces",
    oracle: "StrongholdPiecesOracle",
    key: "unit",
    units: {
      kind: "computed",
      units: (): string[] => units.map((entry) => entry.unit),
    },
    input: (unit: string): unknown => {
      const entry = units.find((candidate) => candidate.unit === unit);
      if (entry === undefined) {
        throw new Error(`stronghold_pieces: unknown unit ${unit}`);
      }
      return { ...entry.input, unit };
    },
    beforeUnits: (context: HarnessContext): void => {
      const directory = context.outputDirectory("stronghold_pieces", "oracle");
      context.compileOracle("StrongholdPiecesOracle", "stronghold_pieces");
      const classpath = context.oracleClasspath(
        context.classesDirectory("stronghold_pieces"),
      );
      units = [];
      for (const seed of SEEDS) {
        for (const sample of collectUnits(directory, classpath, seed, context.environment)) {
          units.push(sample);
        }
      }
      if (units.length === 0) {
        throw new Error(
          "stronghold_pieces: 参考侧没有找到任何要塞单元，语料选取失效",
        );
      }
    },
  } satisfies HarnessDeclaration;
}
