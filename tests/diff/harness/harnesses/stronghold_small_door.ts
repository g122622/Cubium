/*
 * stronghold_small_door.ts — 要塞小门三个落点公式。
 *
 * 三个公式各自只差一两个坐标，但一旦偏移，整座要塞的碰撞格局就会换一种走法，
 * 后续随机流随之整片错位。语料取真实构件的几何（直廊 / 五向交叉点 / 房间 / 图书馆 /
 * 传送门房间在各朝向下展开后的原点与尺寸），覆盖四方向与各构件实际使用的偏移量。
 *
 * 参照侧由 oracle 现算：落点直接取原版 `generateSmallDoorChild*` 的结果，而不是在
 * 本地照着公式再写一遍——那样等于拿同一份可能的误读再比一次。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";

import type { HarnessDeclaration, HarnessContext } from "../core/types.ts";

/** 探针输入/输出的临时文件名。 */
const PROBE_INPUT_FILE = "stronghold_small_door_probe_input.jsonl";
const PROBE_OUTPUT_FILE = "stronghold_small_door_probe_output.jsonl";

interface Sample {
  unit: string;
  input: {
    direction: string;
    seed: number;
    x: number;
    y: number;
    z: number;
    sizeX: number;
    sizeY: number;
    sizeZ: number;
    offsetX: number;
    offsetY: number;
    verticalOffset: number;
    positionOffset: number;
    parentDepth: number;
  };
}

/** 真实构件几何：尺寸与 `orientBox` 的局部偏移。 */
const SHAPES = [
  { sizeX: 5, sizeY: 5, sizeZ: 7, offsetX: -1, offsetY: -1 },
  { sizeX: 10, sizeY: 9, sizeZ: 11, offsetX: -4, offsetY: -3 },
  { sizeX: 11, sizeY: 7, sizeZ: 11, offsetX: -4, offsetY: -1 },
  { sizeX: 14, sizeY: 11, sizeZ: 15, offsetX: -4, offsetY: -1 },
  { sizeX: 11, sizeY: 8, sizeZ: 16, offsetX: -4, offsetY: -1 },
] as const;

const DIRECTIONS = ["NORTH", "SOUTH", "WEST", "EAST"] as const;

/**
 * 各构件 `addChildren` 里实际传给落点方法的两个实参：(垂直偏移, 沿门偏移)。
 *
 * 注意不能拿构件**自身**记录的朝向套这些实参：从门反推落点时原版用的是「导入口朝向」，
 * 而构件自身那个朝向是反的（记 EAST 的构件导入口实际朝 WEST）。所以这里按「导入口朝向 =
 * direction」来喂，两种实参落在落点盒近侧/远侧的情形都覆盖到。
 *
 * 取真实出现过的组合：正向 1/1、直廊侧门 1/2、五向交叉点左右门 1/1、1/4、7/4、7/5。
 */
const OFFSETS = [
  [1, 1],
  [1, 2],
  [1, 4],
  [7, 4],
  [7, 5],
] as const;

/**
 * 真实要塞里出现过的构造原点，覆盖负坐标与大坐标。
 * y 取正数：落点盒 minY 必须 > 10 才过深度门控，负海拔的语料会让两侧一起落空。
 */
const ORIGINS = [
  [-206, 200, -1705],
  [2594, 19, 17538],
  [-2830, 120, -10190],
  [4178, 40, -9790],
] as const;

let units: Sample[] = [];

/** 固定网格生成单元：几何 × 方向 × 偏移 × 原点。 */
function buildUnits(): Sample[] {
  const result: Sample[] = [];
  let index = 0;
  for (const direction of DIRECTIONS) {
    for (const shape of SHAPES) {
      for (const [verticalOffset, positionOffset] of OFFSETS) {
        const [x, y, z] = ORIGINS[index % ORIGINS.length];
        index += 1;
        result.push({
          unit: `${direction.toLowerCase()}/${shape.sizeX}x${shape.sizeZ}/${verticalOffset}/${positionOffset}/${index}`,
          input: {
            direction,
            seed: 42,
            x,
            y,
            z,
            ...shape,
            verticalOffset,
            positionOffset,
            parentDepth: 2,
          },
        });
      }
    }
  }
  return result;
}

/** 用 oracle 的 `run` 子命令一对一跑完输入。 */
function runOracle(directory: string, classpath: string, inputs: Sample[]): void {
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
      "dev.horizonlab.diff.StrongholdSmallDoorOracle",
      "run",
      inputPath,
      outputPath,
    ],
    { cwd: directory, stdio: ["ignore", "pipe", "pipe"] },
  );
}

export function strongholdSmallDoorDeclaration(): HarnessDeclaration {
  return {
    name: "stronghold_small_door",
    probe: "stronghold_small_door",
    oracle: "StrongholdSmallDoorOracle",
    key: "unit",
    units: {
      kind: "computed",
      units: (): string[] => units.map((entry) => entry.unit),
    },
    input: (unit: string): unknown => {
      const entry = units.find((candidate) => candidate.unit === unit);
      if (entry === undefined) {
        throw new Error(`stronghold_small_door: unknown unit ${unit}`);
      }
      return { ...entry.input, unit };
    },
    beforeUnits: (context: HarnessContext): void => {
      units = buildUnits();
      context.compileOracle("StrongholdSmallDoorOracle", "stronghold_small_door");
      const classpath = context.oracleClasspath(
        context.classesDirectory("stronghold_small_door"),
      );
      runOracle(
        context.outputDirectory("stronghold_small_door", "oracle"),
        classpath,
        units,
      );
    },
  } satisfies HarnessDeclaration;
}