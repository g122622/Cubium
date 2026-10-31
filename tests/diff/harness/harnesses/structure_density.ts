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
 * structure_density.ts — 结构地形适配（Beardifier）的采样器语料。
 *
 * 被测对象是「给定一组结构片段与连接点，某个方块坐标处的密度偏移是多少」这一纯函数。
 * 语料覆盖五类地形适配模式、地面高度偏移、连接点、影响范围包围盒的边界内外，以及
 * 24³ 高斯核的完整邻域与极端回绕坐标。
 *
 * 两侧喂的是**同一份语料**：语料在这里生成一次，写进两侧共用的输入，而不是各自推演。
 * 所以生成器必须完全确定——这里只用自乘同余的整数序列，不读时间、不读环境。
 *
 * 每个单元自带 ``pieces``/``junctions``/``affected``（null 表示「没有影响范围包围盒」，
 * 即原版 ``Beardifier.EMPTY`` 的构造形态）与 ``points``（采样坐标），探针与 oracle 都从
 * 输入里构造被测对象，再从零开始算，不共享任何中间状态。
 *
 * 比较口径：结果保留到 1e-9 的相对误差（见 ``normalise``）。两侧在这一层的运算是
 * 同一串 f64 乘加，末位差异只可能来自 libm 的 ``exp``/``sqrt`` 在平台间的 1 ulp 抖动；
 * 真正的公式或排列差异远大于这个量级（相差一个核项就是 1e-2 量级）。
 */

import type { CompareTransform, HarnessDeclaration } from "../core/types.ts";

/** 地形适配模式的五种取值，与两侧枚举同名同序。 */
const TERRAIN_ADAPTATIONS = ["NONE", "BURY", "BEARD_THIN", "BEARD_BOX", "ENCAPSULATE"] as const;

/** 一种测试场景。bundle 是若干单元共用的「结构形状」，points 是各自的采样坐标。 */
interface Scene {
    readonly pieces: readonly (readonly [readonly number[], string, number])[];
    readonly junctions: readonly (readonly number[])[];
    readonly affected: readonly number[] | null;
    readonly points: readonly (readonly number[])[];
}

/** 自乘同余序列：固定种子、全平台一致，只在编排层生成语料。 */
function randomSource(seed: number): (bound: number) => number {
    let state = seed >>> 0;
    return (bound: number): number => {
        state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
        return Math.floor((state / 0x100000000) * bound);
    };
}

/** 24³ 高斯核完整邻域：偏移 -12..11 的每个点，外加核外一格。 */
function kernelNeighborhood(): number[][] {
    const points: number[][] = [];
    for (let x = -13; x <= 12; x++) {
        for (let y = -13; y <= 12; y++) {
            for (let z = -13; z <= 12; z += 5) {
                points.push([x, y, z]);
            }
        }
    }
    // 核边界与角落必须逐个覆盖，步长 5 的采样会漏掉 x=11 / z=11 这类边界。
    for (const x of [-13, -12, -1, 0, 11, 12]) {
        for (const y of [-13, -12, -1, 0, 11, 12]) {
            for (const z of [-13, -12, -1, 0, 11, 12]) {
                points.push([x, y, z]);
            }
        }
    }
    return points;
}

/** 影响范围包围盒的边界内外一圈（含精确落在边界上）。 */
function affectedBoundaryPoints(box: readonly number[]): number[][] {
    const [x0, y0, z0, x1, y1, z1] = box;
    const xs = [x0 - 1, x0, (x0 + x1) >> 1, x1, x1 + 1];
    const ys = [y0 - 1, y0, (y0 + y1) >> 1, y1, y1 + 1];
    const zs = [z0 - 1, z0, (z0 + z1) >> 1, z1, z1 + 1];
    const points: number[][] = [];
    for (const x of xs) {
        for (const y of ys) {
            for (const z of zs) {
                points.push([x, y, z]);
            }
        }
    }
    return points;
}

/** 全部场景；顺序固定，单元键由它在数组里的位置决定。 */
function scenes(): Scene[] {
    const result: Scene[] = [];
    /** 一个 8×9×7 的包围盒，最低点含 groundLevelDelta 后正好落在整数网格上。 */
    const unitBox: readonly number[] = [0, 0, 0, 7, 8, 9];
    const wideAffected: readonly number[] = [-100, -100, -100, 100, 100, 100];
    const kernelPoints = kernelNeighborhood();

    // 1) 空结构：没有片段、没有连接点。affected 为 null 时两侧都必须恒返回 0；
    //    非空 affected 但列表为空时同样恒为 0（原版 compute 里两条路径都提前返回）。
    result.push({ pieces: [], junctions: [], affected: null, points: [[0, 0, 0], [-12, -12, -12], [11, 11, 11]] });
    result.push({ pieces: [], junctions: [], affected: [-1, -1, -1, 1, 1, 1], points: [[0, 0, 0], [2, 2, 2]] });
    result.push({ pieces: [], junctions: [], affected: [0, 0, 0, 0, 0, 0], points: [[0, 0, 0]] });

    // 2) 只有连接点：覆盖 24³ 核邻域与核外一格，累加顺序按语料顺序。
    result.push({ pieces: [], junctions: [[0, 0, 0]], affected: wideAffected, points: kernelPoints });
    result.push({ pieces: [], junctions: [[5, 70, -3]], affected: wideAffected, points: kernelPoints });

    // 3) 单片段 × 每种地形适配 × 地面高度偏移。偏移取核边界与 int 两端，
    //    用于验证「最低点 + delta」的 int 回绕与核索引的越界判定。
    const deltas = [-2147483648, -33, -12, -1, 0, 1, 12, 33, 2147483647];
    for (const adaptation of TERRAIN_ADAPTATIONS) {
        for (const delta of deltas) {
            result.push({
                pieces: [[unitBox, adaptation, delta]],
                junctions: [],
                affected: wideAffected,
                points: kernelPoints,
            });
        }
    }

    // 4) 片段 + 连接点混合，验证两类贡献的累加顺序。
    for (const adaptation of ["BURY", "BEARD_THIN", "ENCAPSULATE"] as const) {
        result.push({
            pieces: [[unitBox, adaptation, 2]],
            junctions: [[3, 5, 4], [-6, 2, 7]],
            affected: wideAffected,
            points: kernelPoints,
        });
    }

    // 5) 影响范围包围盒恰好在结构附近：逐点验证提前退出边界（含边界本身）。
    for (const affected of [[-1, -1, -1, 12, 12, 12], [0, 0, 0, 7, 8, 9], [3, 5, 7, 3, 5, 7], [-40, -40, -40, 40, 40, 40]]) {
        for (const adaptation of ["BEARD_THIN", "BEARD_BOX", "ENCAPSULATE", "BURY"] as const) {
            result.push({
                pieces: [[unitBox, adaptation, 0]],
                junctions: [[2, 3, 2]],
                affected,
                points: affectedBoundaryPoints(affected).concat(kernelPoints),
            });
        }
    }

    // 6) 随机语料：0..16 个片段（包围盒在 ±32 内、高 0..16）、0..10 个连接点。
    //    片段顺序参与累加，所以保留生成顺序，不做排序。
    const next = randomSource(0x53544244); // "STBD"
    const pick = <T>(values: readonly T[]): T => values[next(values.length)];
    for (let scenario = 0; scenario < 64; scenario++) {
        const pieces: (readonly [readonly number[], string, number])[] = [];
        for (let i = 0; i < scenario % 17; i++) {
            const x = pick([-32, -17, -1, 0, 9, 31]) + next(2) - 1;
            const y = pick([-64, -8, 0, 4, 60]) + next(2) - 1;
            const z = pick([-31, -3, 0, 12, 32]) + next(2) - 1;
            pieces.push([
                [x, y, z, x + next(17), y + next(17), z + next(17)],
                pick(TERRAIN_ADAPTATIONS),
                pick(deltas),
            ]);
        }
        const junctions: (readonly number[])[] = [];
        for (let i = 0; i < scenario % 11; i++) {
            junctions.push([next(65) - 32, next(129) - 64, next(65) - 32]);
        }
        const affected = scenario % 13 === 0 ? null : [-48, -80, -48, 48, 80, 48];
        const points: number[][] = [];
        for (let i = 0; i < 128; i++) {
            points.push([next(129) - 64, next(161) - 80, next(129) - 64]);
        }
        result.push({ pieces, junctions, affected, points });
    }

    // 7) 极端坐标：int 两端附近的片段与连接点，检验回绕后的相对距离与核索引。
    for (const coordinate of [-2147483648, -2147483616, 2147483615, 2147483647]) {
        const low = [coordinate, coordinate, coordinate];
        const high = [Math.min(2147483647, coordinate + 8), Math.min(2147483647, coordinate + 8), Math.min(2147483647, coordinate + 8)];
        result.push({
            pieces: [[[...low, ...high], "BEARD_BOX", 2147483647]],
            junctions: [[coordinate, coordinate, coordinate]],
            affected: [-2147483648, -2147483648, -2147483648, 2147483647, 2147483647, 2147483647],
            points: [[coordinate, coordinate, coordinate], [0, 0, 0], [-1, -1, -1], [coordinate + 12, coordinate + 12, coordinate + 12]],
        });
    }

    return result;
}

/**
 * 结果规范化：两侧的 f64 末位可能相差 1 ulp（libm 的 exp/sqrt 不保证跨平台逐位一致），
 * 按 1e-9 的相对误差接受；超过即判为真差异。
 *
 * 只改动被测侧：比较层会校验该变换对参照侧是恒等映射。
 */
function floatComparer(): CompareTransform {
    const closeEnough = (expected: number, actual: number): boolean => {
        const scale = Math.max(Math.abs(expected), Math.abs(actual), Number.MIN_VALUE);
        return Math.abs(expected - actual) <= scale * 1e-9;
    };
    const walk = (expected: unknown, actual: unknown): unknown => {
        if (typeof expected === "number" && typeof actual === "number") {
            return closeEnough(expected, actual) ? expected : actual;
        }
        if (Array.isArray(expected) && Array.isArray(actual)) {
            return actual.map((item: unknown, index: number) => walk(expected[index], item));
        }
        if (expected !== null && actual !== null && typeof expected === "object" && typeof actual === "object") {
            const result: Record<string, unknown> = { ...(actual as Record<string, unknown>) };
            for (const [key, value] of Object.entries(expected as Record<string, unknown>)) {
                result[key] = walk(value, (actual as Record<string, unknown>)[key]);
            }
            return result;
        }
        return actual;
    };
    return walk;
}

/** 单元键：`<场景下标>`，两侧按同一份语料构造。 */
export function structureDensityDeclaration(): HarnessDeclaration {
    let corpus: { readonly unit: string; readonly scene: Scene }[] = [];
    return {
        name: "structure_density",
        probe: "structure_density",
        oracle: "StructureDensityOracle",
        key: "unit",
        units: {
            kind: "computed",
            units: (): string[] => corpus.map((entry) => entry.unit),
        },
        input: (unit: string): unknown => {
            const entry = corpus.find((candidate) => candidate.unit === unit);
            if (entry === undefined) {
                throw new Error(`structure_density: unknown unit ${unit}`);
            }
            return entry.scene;
        },
        beforeUnits: (): void => {
            corpus = scenes().map((scene, index) => ({ unit: `scene/${index}`, scene }));
        },
        normalise: floatComparer(),
    } satisfies HarnessDeclaration;
}

