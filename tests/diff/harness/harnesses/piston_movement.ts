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
 * piston_movement.ts — 活塞推动结构能力的场景库与输入构造。
 *
 * 场景是「一小块方块台 + 一个活塞 + 朝向 + 是否伸出」，两侧读同一份描述：
 * 探针把方块描述字符串解析成自己的 `BlockState`，oracle 用原版
 * `BlockStateParser.parseForBlock` 解析同一串。方块名与属性已由
 * `block_state_names` 保证在两侧一致，因此这里不依赖任何 ID 映射。
 *
 * 场景由**固定种子**的确定性伪随机生成：同一份代码永远产出同一批单元，
 * 增量缓存（按依赖内容哈希判定）才不会失效。
 */

/** 一个场景：活塞位置、朝向序号（原版 Direction.from3DDataValue 顺序）、是否伸出、方块表。 */
export interface PistonScene {
    readonly name: string;
    readonly piston: readonly [number, number, number];
    readonly dir: number;
    readonly extending: boolean;
    readonly cells: readonly (readonly [number, number, number, string])[];
}

/** 场景规模：推到 12 方块上限附近，才能覆盖「超限失败」这条分支。 */
const MAX_CELLS = 14;

/**
 * 活塞附近常见的方块池。
 *
 * 刻意覆盖推动语义的几个关键分类：可推、不可推（黑曜石/基岩）、粘性（粘液块/蜂蜜块）、
 * 会破坏的（花/火把等 push-reaction=destroy）、以及有方块实体的（箱子/熔炉）。
 */
const BLOCKS: readonly string[] = [
    "minecraft:stone",
    "minecraft:dirt",
    "minecraft:oak_planks",
    "minecraft:glass",
    "minecraft:cobblestone",
    "minecraft:slime_block",
    "minecraft:honey_block",
    "minecraft:obsidian",
    "minecraft:bedrock",
    "minecraft:chest",
    "minecraft:furnace",
    "minecraft:torch",
    "minecraft:redstone_wire",
    "minecraft:oak_sapling",
    "minecraft:water",
    "minecraft:air",
];

/** 活塞状态的两种形态：面向任意水平方向，伸出/未伸出。 */
const PISTON_FACINGS: readonly string[] = ["north", "south", "east", "west", "up", "down"];

/**
 * 确定性伪随机（mulberry32）。
 *
 * 不使用 Math.random：单元集必须跨进程、跨机器稳定，否则缓存与报告都不可复现。
 */
function createRandom(seed: number): () => number {
    let state = seed >>> 0;
    return (): number => {
        state = (state + 0x6d2b79f5) >>> 0;
        let t = state;
        t = Math.imul(t ^ (t >>> 15), t | 1);
        t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
        return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
}

/** 从数组里取一个元素。 */
function pick<T>(random: () => number, values: readonly T[]): T {
    return values[Math.floor(random() * values.length)] as T;
}

/**
 * 生成圈定的场景集。
 *
 * 分三组，覆盖不同分支：
 *   - `simple_*`：只有活塞，推动方向前方若干格铺同一方块（覆盖链长与 12 上限）；
 *   - `sticky_*`：前方放粘液块/蜂蜜块并挂上分支方块（覆盖粘连分支与碰撞重排序）；
 *   - `mixed_*`：随机撒入方块池（覆盖破坏分支与不可推方块）。
 */
export function pistonScenes(): PistonScene[] {
    const scenes: PistonScene[] = [];

    // ---- 组 1：直线链。前方 1..14 格同一方块，覆盖空、可推、不可推、破坏四类 ----
    const chainBlocks: readonly string[] = [
        "minecraft:stone",
        "minecraft:obsidian",
        "minecraft:bedrock",
        "minecraft:slime_block",
        "minecraft:torch",
        "minecraft:oak_sapling",
        "minecraft:water",
    ];
    for (const block of chainBlocks) {
        for (let length = 0; length <= MAX_CELLS; length++) {
            const cells: [number, number, number, string][] = [];
            for (let offset = 1; offset <= length; offset++) {
                cells.push([offset, 0, 0, block]);
            }
            scenes.push({
                name: `chain/${block.slice("minecraft:".length)}/${length}`,
                piston: [0, 0, 0],
                dir: 5, // east
                extending: true,
                cells,
            });
        }
    }

    // ---- 组 2：粘性方块的分支。粘块两侧挂方块，覆盖 _addBranchingBlocks ----
    const stickyBlocks: readonly string[] = ["minecraft:slime_block", "minecraft:honey_block"];
    const branchBlocks: readonly string[] = ["minecraft:stone", "minecraft:obsidian", "minecraft:glass"];
    for (const sticky of stickyBlocks) {
        for (const branch of branchBlocks) {
            for (const attach of [1, 2] as const) {
                const cells: [number, number, number, string][] = [[1, 0, 0, sticky]];
                // 在推进方向的垂直面上挂方块（y 与 z 两个轴向都试）。
                cells.push([attach, 1, 0, branch]);
                cells.push([attach, 0, 1, branch]);
                cells.push([attach, -1, 0, branch]);
                scenes.push({
                    name: `branch/${sticky.slice("minecraft:".length)}/${branch.slice("minecraft:".length)}/${attach}`,
                    piston: [0, 0, 0],
                    dir: 5,
                    extending: true,
                    cells,
                });
            }
        }
    }

    // ---- 组 3：随机混合台。固定种子，覆盖任意组合与碰撞重排序 ----
    const random = createRandom(0x5eed);
    for (let index = 0; index < 400; index++) {
        const cells: [number, number, number, string][] = [];
        for (let x = 1; x <= 4; x++) {
            for (let y = -1; y <= 1; y++) {
                for (let z = -1; z <= 1; z++) {
                    if (random() < 0.55) {
                        const block = pick(random, BLOCKS);
                        if (block !== "minecraft:air") {
                            cells.push([x, y, z, block]);
                        }
                    }
                }
            }
        }
        const facing = PISTON_FACINGS[Math.floor(random() * PISTON_FACINGS.length)] as string;
        scenes.push({
            name: `mixed/${index}`,
            piston: [0, 0, 0],
            dir: PISTON_FACINGS.indexOf(facing),
            extending: true,
            cells,
        });
    }

    // ---- 组 4：收回。活塞已伸出时，前方 head 之外 2 格处的方块被拉回 ----
    for (const block of ["minecraft:stone", "minecraft:slime_block", "minecraft:obsidian"] as const) {
        scenes.push({
            name: `retract/${block.slice("minecraft:".length)}`,
            piston: [0, 0, 0],
            dir: 5,
            extending: false,
            cells: [[2, 0, 0, block]],
        });
        scenes.push({
            name: `retract_sticky/${block.slice("minecraft:".length)}`,
            piston: [0, 0, 0],
            dir: 5,
            extending: false,
            cells: [
                [1, 0, 0, "minecraft:piston_head[facing=east]"],
                [2, 0, 0, block],
            ],
        });
    }

    return scenes;
}

/** 场景名列表（能力声明用它做 `units: computed`）。 */
export function pistonSceneNames(): string[] {
    return pistonScenes().map((scene) => scene.name);
}

/** 场景名 → 探针与 oracle 共用的一行输入。 */
export function pistonMovementInput(unit: string): Record<string, unknown> {
    const scene = pistonScenes().find((candidate) => candidate.name === unit);
    if (scene === undefined) {
        throw new Error(`piston_movement：未知场景 ${unit}`);
    }
    return {
        unit: scene.name,
        piston: [...scene.piston],
        dir: scene.dir,
        extending: scene.extending,
        cells: scene.cells.map((cell) => [...cell]),
    };
}