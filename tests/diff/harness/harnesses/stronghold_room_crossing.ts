/*
 * stronghold_room_crossing.ts — 单个房间构件的放置公式。
 *
 * 整树比对只能告诉我们「第几个构件开始分叉」，却分不清是「房间自己算错位置」还是
 * 「随机流在更早的一步就分开了」。这条能力把范围收窄到一个构件：给定门的位置、朝向与
 * 随机源种子，房间的包围盒必须与原版逐坐标一致，且构造期消耗的随机数个数也必须一致
 * （否则后一条随机流就错位）。
 *
 * 语料是固定的小网格：四个朝向 × 几个覆盖负坐标与 y 边界的位置。样本在运行之间不漂移。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 探针输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "stronghold_room_crossing_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "stronghold_room_crossing_probe_output.jsonl";

interface Sample {
  unit: string;
  input: {
    direction: string;
    seed: number;
    x: number;
    y: number;
    z: number;
    depth: number;
  };
}

/** 门朝向：覆盖局部 X 与局部 Z 两条扩展轴。 */
const DIRECTIONS = ["NORTH", "SOUTH", "WEST", "EAST"] as const;

/** 位置样本：负坐标、区块边界与原版真实落点各取其一。 */
// 种子固定成「抽取序列可核对」的值：房间构造只消耗两次抽取。
const POSITIONS = [
  { x: 0, y: 64, z: 0, seed: 1 },
  { x: -204, y: 56, z: -1710, seed: 2 },
  { x: 1042, y: -6, z: 20002, seed: 3 },
  { x: -11374, y: 17, z: -12478, seed: 4 },
] as const;

let units: Sample[] = [];

/** 固定网格生成单元。 */
function buildUnits(): Sample[] {
  const result: Sample[] = [];
  for (const direction of DIRECTIONS) {
    for (const sample of POSITIONS) {
      result.push({
        unit: `${direction.toLowerCase()}/${sample.x}/${sample.y}/${sample.z}/${sample.seed}`,
        input: {
          direction,
          seed: sample.seed,
          x: sample.x,
          y: sample.y,
          z: sample.z,
          depth: 3,
        },
      });
    }
  }
  return result;
}

/** 用 oracle 的 `run` 子命令一对一跑完输入，返回 unit → 输出 的映射。 */
function runOracle(
  directory: string,
  classpath: string,
  inputs: Sample[],
): void {
  const inputPath = path.join(directory, PROBE_INPUT_FILE);
  const outputPath = path.join(directory, PROBE_OUTPUT_FILE);
  const lines = inputs.map((entry) =>
    JSON.stringify({ unit: entry.unit, ...entry.input }),
  );
  fs.writeFileSync(inputPath, `${lines.join("\n")}\n`);
  execFileSync(
    "java",
    [
      "-cp",
      classpath,
      "dev.horizonlab.diff.StrongholdRoomCrossingOracle",
      "run",
      inputPath,
      outputPath,
    ],
    { cwd: directory, stdio: ["ignore", "pipe", "pipe"] },
  );
}

export function strongholdRoomCrossingDeclaration(): HarnessDeclaration {
  return {
    name: "stronghold_room_crossing",
    probe: "stronghold_room_crossing",
    oracle: "StrongholdRoomCrossingOracle",
    key: "unit",
    units: {
      kind: "computed",
      units: (): string[] => units.map((entry) => entry.unit),
    },
    input: (unit: string): unknown => {
      const entry = units.find((candidate) => candidate.unit === unit);
      if (entry === undefined) {
        throw new Error(`stronghold_room_crossing: unknown unit ${unit}`);
      }
      return { ...entry.input, unit };
    },
    beforeUnits: (context: HarnessContext): void => {
      units = buildUnits();
      context.compileOracle(
        "StrongholdRoomCrossingOracle",
        "stronghold_room_crossing",
      );
      const classpath = context.oracleClasspath(
        context.classesDirectory("stronghold_room_crossing"),
      );
      runOracle(
        context.outputDirectory("stronghold_room_crossing", "oracle"),
        classpath,
        units,
      );
    },
  } satisfies HarnessDeclaration;
}
