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
 * path_scenes.ts — 寻路差分的场景库。
 *
 * 一个「场景」= 一块方块场地 + 一个受控实体 + 一组能力开关。两侧（原版 jar 的
 * WalkPathTypeOracle / PathFinderOracle 与 Cubium 的 diff_walk_path_type /
 * diff_path_finder 探针）读**同一份** JSON，因此场景库是差分口径的唯一来源。
 *
 * 场景的组织方式沿用 horizon 的 mob AI 测试思路：不追求「随机大世界」，而是把
 * 每一条判定规则拆成**能单独解释**的小场景，让差异一出现就能指到具体那条规则上。
 *
 * 约定：
 *   - 方块描述用原版命令语法（`minecraft:oak_stairs[facing=east,half=top]`），
 *     两侧各用自己的解析器，语法一致；
 *   - 场景全部相对原点 (0,0,0)，地面摆在 y = -1；
 *   - 实体默认 0.6 × 1.8、抬脚 0.6、站在地面；
 *   - 每格 (dx, dy, dz) 都是相对原点的偏移。
 */

/** 场景里的一格方块。 */
export interface SceneCell {
    readonly dx: number;
    readonly dy: number;
    readonly dz: number;
    readonly block: string;
}

/** 实体的受控属性。 */
export interface SceneMob {
    readonly width: number;
    readonly height: number;
    readonly stepHeight: number;
    readonly onGround: boolean;
    readonly inWater: boolean;
    readonly position: { readonly x: number; readonly y: number; readonly z: number };
    readonly malus: readonly { readonly type: string; readonly value: number }[];
    readonly maxFallDistance?: number;
}

/** 求值器能力开关。 */
export interface SceneFlags {
    readonly canPassDoors: boolean;
    readonly canOpenDoors: boolean;
    readonly canFloat: boolean;
    readonly canWalkOverFences: boolean;
}

/** 一个完整场景。 */
export interface PathScene {
    readonly name: string;
    readonly sizeX: number;
    readonly sizeY: number;
    readonly sizeZ: number;
    readonly cells: readonly SceneCell[];
    readonly mob: SceneMob;
    readonly flags: SceneFlags;
}

const AIR = "minecraft:air";

/** 默认实体：僵尸体型。 */
const DEFAULT_MOB: SceneMob = {
    width: 0.6,
    height: 1.8,
    stepHeight: 0.6,
    onGround: true,
    inWater: false,
    position: { x: 0.5, y: 0, z: 0.5 },
    malus: [],
};

const DEFAULT_FLAGS: SceneFlags = {
    canPassDoors: true,
    canOpenDoors: false,
    canFloat: false,
    canWalkOverFences: false,
};

/** 在 y = -1 铺一层指定方块的平地面。 */
function floor(sizeX: number, sizeZ: number, block = "minecraft:stone"): SceneCell[] {
    const cells: SceneCell[] = [];
    for (let dx = 0; dx < sizeX; dx++) {
        for (let dz = 0; dz < sizeZ; dz++) {
            cells.push({ dx, dy: -1, dz, block });
        }
    }
    return cells;
}

/** 在指定坐标放一格；坐标须在本场景尺寸内。 */
function cell(dx: number, dy: number, dz: number, block: string): SceneCell {
    return { dx, dy, dz, block };
}

function scene(
    name: string,
    sizeX: number,
    sizeY: number,
    sizeZ: number,
    cells: readonly SceneCell[],
    mob: Partial<SceneMob> = {},
    flags: Partial<SceneFlags> = {},
): PathScene {
    return {
        name,
        sizeX,
        sizeY,
        sizeZ,
        cells,
        mob: { ...DEFAULT_MOB, ...mob },
        flags: { ...DEFAULT_FLAGS, ...flags },
    };
}

/**
 * 单格方块类型谱：把「每种方块单独放到脚下 / 脚部 / 头部」各摆一次。
 *
 * 这是覆盖面最广的一批场景：每一种方块类型映射错了都会在这里暴露，而且
 * `fromState` / `static` / `ofMob` 三层逐格输出能直接指出是「方块 → 类型」错了，
 * 还是「叠加脚下 / 邻域」错了。
 */
const UNIT_BLOCKS: readonly string[] = [
    "stone", "dirt", "grass_block", "sand", "gravel", "clay", "netherrack", "end_stone",
    // 台阶 / 楼梯：顶面高度不是整格，是「地面高度」最容易出错的地方
    "oak_slab[type=bottom]", "oak_slab[type=top]", "oak_slab[type=double]",
    "stone_slab[type=bottom]", "oak_stairs[facing=east,half=bottom]", "oak_stairs[facing=east,half=top]",
    "soul_sand", "farmland[moisture=0]", "dirt_path", "mud", "honey_block", "slime_block",
    // 危险方块
    "fire", "soul_fire", "magma_block", "cactus", "sweet_berry_bush", "wither_rose",
    "pointed_dripstone[vertical_direction=up,thickness=tip]", "powder_snow",
    "campfire[lit=true]", "campfire[lit=false]", "soul_campfire[lit=true]", "lava_cauldron",
    // 流体
    "water", "water[level=3]", "lava", "lava[level=5]",
    // 可穿行 / 不可穿行
    "oak_leaves", "azalea_leaves", "short_grass", "poppy", "snow[layers=1]", "snow[layers=8]",
    "cobweb", "scaffolding", "barrier", "glass", "chest", "white_carpet",
    // 门 / 栅栏门 / 栅栏 / 墙：门与栅栏的判定叠加了实体能力，必须单独覆盖
    "oak_door[facing=east,half=lower,open=false]", "oak_door[facing=east,half=lower,open=true]",
    "iron_door[facing=east,half=lower,open=false]", "iron_door[facing=east,half=lower,open=true]",
    "oak_fence_gate[facing=east,open=false]", "oak_fence_gate[facing=east,open=true]",
    "oak_fence", "cobblestone_wall[up=true]",
    // 铁轨
    "rail", "powered_rail[powered=true]", "detector_rail",
    // 活板门 / 睡莲 / 垂滴叶
    "oak_trapdoor[facing=north,half=top,open=false]", "oak_trapdoor[facing=north,half=top,open=true]",
    "lily_pad", "big_dripleaf[facing=east,tilt=none]", "cocoa[age=2,facing=north]",
];

/**
 * 「需要基座」的方块 → 它需要的基座方块名。
 *
 * 世界在写入这些方块时会调用各自的 `canSurvive`，不满足就原地变回空气：
 *   - `fire` / `soul_fire`：原版 `BaseFireBlock#canSurvive` 走 `SOUL_FIRE_BASE_BLOCKS`；
 *   - 三种铁轨：原版要求下方能托住它（这里用石头，属于可靠支撑）；
 *   - 活板门 / 睡莲 / 垂滴叶：原版同样需要有效支撑面才立得住。
 *
 * 只有出现在 `UNIT_BLOCKS` 里的条目才会被用到；表里多留的条目是无害的。
 */
const SUPPORT_FOR: Readonly<Record<string, string>> = {
    "fire": "netherrack",
    "soul_fire": "soul_soil",
    "rail": "stone",
    "powered_rail[powered=true]": "stone",
    "detector_rail": "stone",
    "oak_trapdoor[facing=north,half=top,open=false]": "stone",
    "oak_trapdoor[facing=north,half=top,open=true]": "stone",
    "lily_pad": "water",
};

function unitBlockScenes(): PathScene[] {
    const scenes: PathScene[] = [];
    for (const block of UNIT_BLOCKS) {
        const described = `minecraft:${block}`;
        const tag = block.replace(/[[\],=]/g, "_");
        // 「需要支撑」的方块（火、灵魂火、铁轨）在没有合法基座时会被世界直接移除：
        // 原版 `BaseFireBlock#canSurvive` 要求下方属于 `SOUL_FIRE_BASE_BLOCKS`，
        // 铁轨要求下方能托住它。探针不模拟「放置流程」，所以这里按原版规则显式补基座，
        // 否则这一格写进去的是空气，差分就退化成「双方都在看空气」而失去意义。
        const support = SUPPORT_FOR[block];
        // 3×3×3 的网格：中心格在 (1,0,1)，把所有位置都覆盖到，便于同时观察邻域规则。
        // 脚下 = 中心格的 y-1；脚部 = 中心格；头部 = 中心格 y+1。
        const belowCells = support === undefined ? [] : [cell(1, -2, 1, `minecraft:${support}`)];
        const feetCells = support === undefined ? [] : [cell(1, -1, 1, `minecraft:${support}`)];
        const headCells = support === undefined ? [] : [cell(1, 0, 1, `minecraft:${support}`)];
        scenes.push(scene(`below/${tag}`, 3, 3, 3, [
            ...floor(3, 3),
            ...belowCells,
            cell(1, -1, 1, described),
        ]));
        scenes.push(scene(`feet/${tag}`, 3, 3, 3, [
            ...floor(3, 3),
            ...feetCells,
            cell(1, 0, 1, described),
        ]));
        scenes.push(scene(`head/${tag}`, 3, 3, 3, [
            ...floor(3, 3),
            ...headCells,
            cell(1, 1, 1, described),
        ]));
        // 相邻格：检查 3×3×3 邻域的危险判定（火、岩浆、仙人掌会把整格降级）。
        // 实体站在中心格 (1,0,1) 的正中：该格自身是空气、脚下是石头，
        // 因此 static 一定走邻域判定，ofMob 与它同值——两层互不遮蔽。
        scenes.push(scene(`adjacent/${tag}`, 3, 3, 3, [
            ...floor(3, 3),
            ...(support === undefined ? [] : [cell(2, -1, 1, `minecraft:${support}`)]),
            cell(2, 0, 1, described),
        ], { position: { x: 1.5, y: 0, z: 1.5 } }));
    }
    return scenes;
}

/**
 * 结构场景：跳跃、跌落、门、栅栏、窄缝、阶梯、天花板。
 *
 * 这些是「多条规则叠加」的场景，单格测试覆盖不到：例如「抬脚高度 + 天花板高度」
 * 共同决定能不能跳上台阶，「实体宽度 + 栅栏宽度」共同决定能不能挤过栅栏缝。
 */
function structureScenes(): PathScene[] {
    const scenes: PathScene[] = [];

    // 台阶：一、二、三格高的墙，分别对应「能跳上」与「跳不上」。
    for (const height of [1, 2, 3]) {
        const cells = floor(6, 3);
        for (let dx = 3; dx < 6; dx++) {
            for (let dz = 0; dz < 3; dz++) {
                for (let dy = 0; dy < height; dy++) {
                    cells.push(cell(dx, dy, dz, "minecraft:stone"));
                }
            }
        }
        scenes.push(scene(`step/height${height}`, 6, height + 3, 3, cells));
    }

    // 天花板：头顶一格处的方块会把跳跃路径堵死。
    scenes.push(scene("ceiling/low", 4, 3, 3, [
        ...floor(4, 3),
        // 在 y = 2 铺满顶板，实体高 1.8 刚好卡住。
        ...floorAt(4, 3, 2, "minecraft:stone"),
    ]));

    // 栅栏：一整排，正常宽度过不去，宽度 < 0.5 可以挤过栅栏柱缝。
    scenes.push(scene("fence/row", 5, 3, 3, [
        ...floor(5, 3),
        ...[0, 1, 2].map((dz) => cell(2, 0, dz, "minecraft:oak_fence")),
    ]));
    scenes.push(scene("fence/row-narrow", 5, 3, 3, [
        ...floor(5, 3),
        ...[0, 1, 2].map((dz) => cell(2, 0, dz, "minecraft:oak_fence")),
    ], { width: 0.4 }));

    // 关闭的木门：只有具备开门能力的实体才能通过。
    scenes.push(scene("door/closed-wood", 5, 3, 3, [
        ...floor(5, 3),
        cell(2, 0, 1, "minecraft:oak_door[facing=east,half=lower,open=false]"),
        cell(2, 1, 1, "minecraft:oak_door[facing=east,half=upper,open=false]"),
    ], { position: { x: 0.5, y: 0, z: 1.5 } }, { canOpenDoors: true, canPassDoors: true }));
    scenes.push(scene("door/closed-wood-locked", 5, 3, 3, [
        ...floor(5, 3),
        cell(2, 0, 1, "minecraft:oak_door[facing=east,half=lower,open=false]"),
        cell(2, 1, 1, "minecraft:oak_door[facing=east,half=upper,open=false]"),
    ], { position: { x: 0.5, y: 0, z: 1.5 } }, { canOpenDoors: false, canPassDoors: true }));
    scenes.push(scene("door/open-cannot-pass", 5, 3, 3, [
        ...floor(5, 3),
        cell(2, 0, 1, "minecraft:oak_door[facing=east,half=lower,open=true]"),
        cell(2, 1, 1, "minecraft:oak_door[facing=east,half=upper,open=true]"),
    ], { position: { x: 0.5, y: 0, z: 1.5 } }, { canPassDoors: false }));

    // 铁轨：与实体所在列对齐时才可上下。铁轨必须摆在地面之上（dy = 0），
    // 摆进地面层会顶掉石头、失去支撑而被世界移除。
    scenes.push(scene("rail/aligned", 4, 3, 3, [
        ...floor(4, 3),
        cell(0, 0, 0, "minecraft:rail"),
        cell(1, 0, 0, "minecraft:rail"),
        cell(2, 0, 0, "minecraft:rail"),
    ]));

    // 实体尺寸谱：同一块地形下，不同宽高得到不同的 ofMob 结果。
    for (const [label, width, height] of [
        ["needle", 0.25, 0.5], ["chicken", 0.4, 0.7], ["zombie", 0.6, 1.95],
        ["cow", 0.9, 1.4], ["ravager", 1.95, 2.2], ["giant", 3.6, 10.8],
    ] as const) {
        scenes.push(scene(`size/${label}`, 5, 4, 5, [
            ...floor(5, 5),
            cell(2, 0, 0, "minecraft:oak_fence"),
            cell(2, 0, 1, "minecraft:oak_fence"),
            cell(0, 0, 0, "minecraft:water"),
            cell(4, 0, 4, "minecraft:fire"),
        ], { width, height, position: { x: 1.5, y: 0, z: 1.5 } }));
    }

    // 水：会浮的实体（canFloat）在水里会找水面，不会浮的直接把水当不可走。
    scenes.push(scene("water/pool", 5, 4, 5, [
        ...floor(5, 5),
        ...[1, 2, 3].flatMap((dx) => [1, 2, 3].map((dz) => cell(dx, 0, dz, "minecraft:water"))),
    ], { position: { x: 1.5, y: 0, z: 1.5 } }));
    scenes.push(scene("water/pool-float", 5, 4, 5, [
        ...floor(5, 5),
        ...[1, 2, 3].flatMap((dx) => [1, 2, 3].map((dz) => cell(dx, 0, dz, "minecraft:water"))),
    ], { position: { x: 1.5, y: 0, z: 1.5 } }, { canFloat: true }));

    // 岩浆池：默认不可通行，声明代价 0 后可通行（下界生物）。
    scenes.push(scene("lava/pool", 5, 4, 5, [
        ...floor(5, 5),
        cell(2, 0, 2, "minecraft:lava"),
    ], { position: { x: 2.5, y: 0, z: 2.5 } }));
    scenes.push(scene("lava/pool-strider", 5, 4, 5, [
        ...floor(5, 5),
        cell(2, 0, 2, "minecraft:lava"),
    ], {
        position: { x: 2.5, y: 0, z: 2.5 },
        malus: [{ type: "lava", value: 0 }, { type: "water", value: -1 }],
    }));

    // 负代价：把水声明成 -1 后，水格与周边水岸都应被排除。
    scenes.push(scene("malus/water-blocked", 5, 3, 5, [
        ...floor(5, 5),
        ...[1, 2, 3].flatMap((dx) => [1, 2, 3].map((dz) => cell(dx, 0, dz, "minecraft:water"))),
    ], {
        position: { x: 1.5, y: 0, z: 1.5 },
        malus: [{ type: "water", value: -1 }, { type: "water_border", value: -1 }],
    }));

    // 抬脚高度谱：同一级台阶，抬脚不同则「跳得上 / 跳不上」不同。
    for (const stepHeight of [0, 0.6, 1.0, 1.5]) {
        const cells = floor(5, 3);
        for (let dx = 3; dx < 5; dx++) {
            for (let dz = 0; dz < 3; dz++) {
                cells.push(cell(dx, 0, dz, "minecraft:stone"));
            }
        }
        scenes.push(scene(`stepHeight/${stepHeight}`, 5, 4, 3, cells, { stepHeight }));
    }

    // 天花板 + 台阶：抬脚够得着但头顶没有空间，必须被拒绝。
    {
        const cells = floor(5, 3);
        for (let dx = 3; dx < 5; dx++) {
            for (let dz = 0; dz < 3; dz++) {
                cells.push(cell(dx, -1, dz, "minecraft:stone"));
            }
        }
        cells.push(...floorAt(5, 3, 2, "minecraft:stone"));
        scenes.push(scene("ceiling/with-step", 5, 4, 3, cells, { stepHeight: 1.0 }));
    }

    return scenes;
}

/** 在指定 y 铺满一层。 */
function floorAt(sizeX: number, sizeZ: number, dy: number, block: string): SceneCell[] {
    const cells: SceneCell[] = [];
    for (let dx = 0; dx < sizeX; dx++) {
        for (let dz = 0; dz < sizeZ; dz++) {
            cells.push({ dx, dy, dz, block });
        }
    }
    return cells;
}

/** 全部路径类型场景（顺序固定：报告里的单元顺序与它一致）。 */
export const PATH_SCENES: readonly PathScene[] = [...unitBlockScenes(), ...structureScenes()];

/** 场景名列表（供编排层做单元枚举）。 */
export function pathSceneNames(): string[] {
    return PATH_SCENES.map((entry) => entry.name);
}

/** 按名字取场景。 */
export function pathScene(name: string): PathScene {
    const found = PATH_SCENES.find((entry) => entry.name === name);
    if (found === undefined) {
        throw new Error(`未知寻路场景：${name}`);
    }
    return found;
}

/** 逐格路径类型能力的输入（`diff_walk_path_type` / `WalkPathTypeOracle`）。 */
export function walkPathTypeInput(name: string): unknown {
    const entry = pathScene(name);
    return {
        unit: entry.name,
        origin: { x: 0, y: 0, z: 0 },
        sizeX: entry.sizeX,
        sizeY: entry.sizeY,
        sizeZ: entry.sizeZ,
        cells: entry.cells,
        mob: entry.mob,
        flags: entry.flags,
    };
}

/**
 * A* 寻路能力的输入。
 *
 * 目标与搜索参数按场景名派生，保持「同一个场景在两种能力里地形完全一致」：
 * 目标固定放在场地的对角角落，`maxPathLength` 给足，`reachRange` 取 1。
 */
export function pathFinderInput(name: string): unknown {
    const entry = pathScene(name);
    const targetX = entry.sizeX - 1;
    const targetZ = entry.sizeZ - 1;
    return {
        unit: entry.name,
        origin: { x: 0, y: 0, z: 0 },
        sizeX: entry.sizeX,
        sizeY: entry.sizeY,
        sizeZ: entry.sizeZ,
        cells: entry.cells,
        mob: entry.mob,
        flags: entry.flags,
        targets: [{ x: targetX, y: 0, z: targetZ }],
        maxPathLength: 64,
        reachRange: 1,
        maxVisitedNodes: 512,
        maxVisitedNodesMultiplier: 1,
    };
}
