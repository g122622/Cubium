// 下界传送门框架形成与点燃测试：验证黑曜石框架尺寸约束（内部 2×3 ~ 21×21）与火方块点燃激活逻辑。
//
// wiki 机制（world_下界传送门.txt#创建传送门、tech_下界传送门（方块）.txt）：
//   - 下界传送门框架须由黑曜石构成，内部点燃火焰（打火石/火矢弓/恶魂火球等）后激活为传送门方块。
//   - 框架内部空间尺寸约束：宽 2~21 格 × 高 3~21 格（对齐 Java PortalShape.MIN_WIDTH=2/MAX_WIDTH=21、
//     MIN_HEIGHT=3/MAX_HEIGHT=21）。
//   - 火焰放置在框架内部任意位置时，FireBlock::onBlockAdded → tryLightNetherPortal 检测并点燃传送门。
//
// Cubium 实现（PortalSize.cpp、FireBlock.cpp）：
//   - PortalSize::findNetherPortal(world, pos, preferXAxis) 先尝试 preferXAxis（West 方向），
//     再尝试 South 方向。
//   - _findBottomLeft：从火方块位置向下搜索（canConnect：air/fire/portal），向左搜索到框架。
//   - _calculateWidth：从 bottomLeft 向右计算宽度（须 2~21）。
//   - _calculateHeight：从 bottomLeft 向上计算高度（须 3~21），检查左右框架与内部空性。
//   - _checkTopFrame：检查顶部框架完整。
//   - lightNetherPortal：在内部区域设置 nether_portal 方块（带 axis 状态）。
//   - FireBlock::onBlockAdded → tryLightNetherPortal（仅主世界/下界维度）。
//
// 测试结构（glass_pit 7×5×7）：
//   - 内部空间 X∈[1,5], Z∈[1,5], Y∈[1,3]（5×5×3）。
//   - Y=0 实心玻璃底板（可替换为黑曜石底框）。
//   - Y=4 玻璃顶（可替换为黑曜石顶框）。
//   - 最小框架（内部 2×3，X 轴向）：
//       底框 Y=0, 顶框 Y=4, 左框 X=2, 右框 X=5, 内部 X=3,4 / Y=1..3 / Z=3。
//
// 测试覆盖（6 个核心行为点）：
//   1. min_frame_x_axis_ignites：最小 X 轴向框架（内部 2×3）被火方块点燃后生成 nether_portal。
//   2. z_axis_portal_frame_ignites：Z 轴向框架被点燃后生成 axis=z 的 nether_portal。
//   3. width_too_narrow_no_portal：内部宽度 1（< MIN_WIDTH=2）不点燃传送门。
//   4. height_too_short_no_portal：内部高度 2（< MIN_HEIGHT=3）不点燃传送门。
//   5. incomplete_top_frame_no_portal：顶框缺失不点燃传送门。
//   6. fire_outside_frame_no_portal：火方块在框架外部不点燃传送门。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界传送门.txt#创建传送门
// Ref: net.minecraft.world.level.portal.PortalShape（Java 1.21.11）
// Ref: PortalSize.cpp（Cubium src/common/world/dimension/teleport/）
// Ref: FireBlock.cpp#onBlockAdded（Cubium src/common/world/block/blocks/nether/）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 读取 (x,y,z) 方块 typeId。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 方块的 axis 状态（"x"/"z"）。返回空串表示读取失败。
function getBlockAxis(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return "";
    }
    const value = block?.permutation?.getState("axis" as any);
    return typeof value === "string" ? value : "";
}

// 放置黑曜石方块（框架方块）。
function placeObsidian(test: Test, x: number, y: number, z: number): void {
    test.setBlockType("minecraft:obsidian", { x, y, z });
}

// 清空指定位置为空气（确保内部为空气）。
function clearToAir(test: Test, x: number, y: number, z: number): void {
    test.setBlockType("minecraft:air", { x, y, z });
}

// 构建 X 轴向下界传送门框架。
// internalW: 内部宽度（2~21），internalH: 内部高度（3~21）。
// innerX, innerY, innerZ: 框架内部左下角坐标（内部区域从此处向 +X 延伸 internalW 格、向 +Y 延伸 internalH 格）。
// 框架轴向为 X（传送门方块 axis=x），即框架在 Z 固定平面上、沿 X 展开宽度、沿 Y 展开高度。
function buildXAxisPortalFrame(
    test: Test,
    innerX: number,
    innerY: number,
    innerZ: number,
    internalW: number,
    internalH: number,
): void {
    // 底框（内部下方一行黑曜石）。
    for (let dx = 0; dx < internalW + 2; dx++) {
        placeObsidian(test, innerX + dx - 1, innerY - 1, innerZ);
    }
    // 顶框（内部上方一行黑曜石）。
    for (let dx = 0; dx < internalW + 2; dx++) {
        placeObsidian(test, innerX + dx - 1, innerY + internalH, innerZ);
    }
    // 左框（内部左侧一列黑曜石）。
    for (let dy = 0; dy < internalH; dy++) {
        placeObsidian(test, innerX - 1, innerY + dy, innerZ);
    }
    // 右框（内部右侧一列黑曜石）。
    for (let dy = 0; dy < internalH; dy++) {
        placeObsidian(test, innerX + internalW, innerY + dy, innerZ);
    }
    // 清空内部（确保是空气）。
    for (let dx = 0; dx < internalW; dx++) {
        for (let dy = 0; dy < internalH; dy++) {
            clearToAir(test, innerX + dx, innerY + dy, innerZ);
        }
    }
}

// 构建 Z 轴向下界传送门框架。
// internalW: 内部宽度（2~21，沿 Z 轴展开），internalH: 内部高度（3~21，沿 Y 轴展开）。
// innerX, innerY, innerZ: 框架内部左下角坐标（内部区域从此处向 +Z 延伸 internalW 格、向 +Y 延伸 internalH 格）。
// 框架轴向为 Z（传送门方块 axis=z），即框架在 X 固定平面上、沿 Z 展开宽度、沿 Y 展开高度。
function buildZAxisPortalFrame(
    test: Test,
    innerX: number,
    innerY: number,
    innerZ: number,
    internalW: number,
    internalH: number,
): void {
    // 底框（内部下方一行黑曜石，沿 Z 展开）。
    for (let dz = 0; dz < internalW + 2; dz++) {
        placeObsidian(test, innerX, innerY - 1, innerZ + dz - 1);
    }
    // 顶框（内部上方一行黑曜石，沿 Z 展开）。
    for (let dz = 0; dz < internalW + 2; dz++) {
        placeObsidian(test, innerX, innerY + internalH, innerZ + dz - 1);
    }
    // 左框（内部左侧一列黑曜石，沿 Y 展开，Z=innerZ-1）。
    for (let dy = 0; dy < internalH; dy++) {
        placeObsidian(test, innerX, innerY + dy, innerZ - 1);
    }
    // 右框（内部右侧一列黑曜石，沿 Y 展开，Z=innerZ+internalW）。
    for (let dy = 0; dy < internalH; dy++) {
        placeObsidian(test, innerX, innerY + dy, innerZ + internalW);
    }
    // 清空内部（确保是空气）。
    for (let dz = 0; dz < internalW; dz++) {
        for (let dy = 0; dy < internalH; dy++) {
            clearToAir(test, innerX, innerY + dy, innerZ + dz);
        }
    }
}

// 检查内部区域是否全部为 nether_portal 方块且轴向正确。
// X 轴向（axis=x）：内部方块沿 X 展开（innerX+dx），Z 固定。
// Z 轴向（axis=z）：内部方块沿 Z 展开（innerZ+dz），X 固定。
// innerX, innerY, innerZ: 内部左下角；internalW/internalH: 内部宽/高；axis: 期望轴向。
function isPortalFormed(
    test: Test,
    innerX: number,
    innerY: number,
    innerZ: number,
    internalW: number,
    internalH: number,
    axis: string,
): boolean {
    for (let d = 0; d < internalW; d++) {
        for (let dy = 0; dy < internalH; dy++) {
            let x = innerX;
            let z = innerZ;
            if (axis === "x") {
                x = innerX + d;
            } else {
                z = innerZ + d;
            }
            const pos = { x, y: innerY + dy, z };
            if (getBlockTypeId(test, pos.x, pos.y, pos.z) !== "minecraft:nether_portal") {
                return false;
            }
            if (getBlockAxis(test, pos.x, pos.y, pos.z) !== axis) {
                return false;
            }
        }
    }
    return true;
}

// 场景 1：最小 X 轴向框架（内部 2×3）被火方块点燃后生成 nether_portal。
//
// wiki 机制（world_下界传送门.txt#创建传送门）：
//   - 最小下界传送门框架外框 4×5（内部 2×3）。
//   - 在框架内部点燃火焰后激活为传送门方块。
//
// 布局：glass_pit 内部放置最小 X 轴向框架。
//   - 内部左下角 (3,1,3)，内部宽 2（X=3,4）、高 3（Y=1,2,3）。
//   - 底框 Y=0（替换玻璃底板）、顶框 Y=4（替换玻璃顶）。
//   - 左框 X=2、右框 X=5（Y=1..3）。
//   - 火方块放置在内部 (3,2,3)。
//
// 判定：内部 2×3 区域全部为 nether_portal（axis=x）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界传送门.txt#创建传送门
// Ref: PortalSize.cpp#findNetherPortal + lightNetherPortal
function minFrameXAxisIgnites(test: Test): void {
    const innerX = 3;
    const innerY = 1;
    const innerZ = 3;
    const internalW = 2;
    const internalH = 3;

    buildXAxisPortalFrame(test, innerX, innerY, innerZ, internalW, internalH);

    // 在框架内部放置火方块触发点燃。
    test.setBlockType("minecraft:fire", { x: innerX, y: innerY + 1, z: innerZ });

    // 验证内部 2×3 区域全部为 nether_portal（axis=x）。
    test.assert(
        isPortalFormed(test, innerX, innerY, innerZ, internalW, internalH, "x"),
        "min frame (internal 2×3, X axis) should ignite into nether_portal blocks",
    );

    test.succeed();
}

// 场景 2：Z 轴向框架被点燃后生成 axis=z 的 nether_portal。
//
// wiki 机制：下界传送门可沿 X 或 Z 轴构建。Z 轴向框架点燃后生成 axis=z 的传送门方块。
//
// 布局：glass_pit 内部放置 Z 轴向最小框架。
//   - 内部左下角 (3,1,3)，内部宽 2（Z=3,4）、高 3（Y=1,2,3）。
//   - 底框 Y=0、顶框 Y=4。
//   - 左框 Z=2、右框 Z=5（Y=1..3）。
//   - 火方块放置在内部 (3,2,3)。
//
// 判定：内部 2×3 区域全部为 nether_portal（axis=z）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界传送门.txt#创建传送门
// Ref: PortalSize.cpp#findNetherPortal（preferXAxis=true 先试 X 轴，失败再试 Z 轴）
function zAxisPortalFrameIgnites(test: Test): void {
    const innerX = 3;
    const innerY = 1;
    const innerZ = 3;
    const internalW = 2;
    const internalH = 3;

    buildZAxisPortalFrame(test, innerX, innerY, innerZ, internalW, internalH);

    // 在框架内部放置火方块触发点燃。
    test.setBlockType("minecraft:fire", { x: innerX, y: innerY + 1, z: innerZ });

    // 验证内部 2×3 区域全部为 nether_portal（axis=z）。
    test.assert(
        isPortalFormed(test, innerX, innerY, innerZ, internalW, internalH, "z"),
        "Z axis frame (internal 2×3) should ignite into nether_portal (axis=z) blocks",
    );

    test.succeed();
}

// 场景 3：内部宽度 1（< MIN_WIDTH=2）不点燃传送门。
//
// wiki 机制：框架内部宽度须 >= 2。宽度 1 的框架不构成有效下界传送门。
//
// 布局：构建内部宽 1 的"框架"（左框 X=2、右框 X=4，中间仅 X=3 一格空气）。
//   - 底框 Y=0、顶框 Y=4。
//   - 火方块放置在内部 (3,2,3)。
//
// 判定：内部不应生成 nether_portal 方块（宽度 1 < MIN_WIDTH=2）。
//
// Ref: PortalSize.cpp#_tryFindPortalOnAxis（width < MIN_WIDTH 返回 nullopt）
function widthTooNarrowNoPortal(test: Test): void {
    // 内部宽 1：左框 X=2，右框 X=4，中间 X=3 一格。
    const innerX = 3;
    const innerY = 1;
    const innerZ = 3;
    const internalW = 1;
    const internalH = 3;

    buildXAxisPortalFrame(test, innerX, innerY, innerZ, internalW, internalH);

    // 在框架内部放置火方块。
    test.setBlockType("minecraft:fire", { x: innerX, y: innerY + 1, z: innerZ });

    // 验证内部未生成 nether_portal 方块。
    const portalType = "minecraft:nether_portal";
    for (let dy = 0; dy < internalH; dy++) {
        const typeId = getBlockTypeId(test, innerX, innerY + dy, innerZ);
        test.assert(typeId !== portalType, `width 1 should not form portal, got ${typeId} at y=${innerY + dy}`);
    }

    test.succeed();
}

// 场景 4：内部高度 2（< MIN_HEIGHT=3）不点燃传送门。
//
// wiki 机制：框架内部高度须 >= 3。高度 2 的框架不构成有效下界传送门。
//
// 布局：构建内部宽 2、高 2 的框架。
//   - 内部左下角 (3,1,3)，内部宽 2（X=3,4）、高 2（Y=1,2）。
//   - 底框 Y=0、顶框 Y=3。
//   - 左框 X=2、右框 X=5（Y=1,2）。
//   - 火方块放置在内部 (3,2,3)。
//
// 判定：内部不应生成 nether_portal 方块（高度 2 < MIN_HEIGHT=3）。
//
// Ref: PortalSize.cpp#_tryFindPortalOnAxis（height < MIN_HEIGHT 返回 nullopt）
function heightTooShortNoPortal(test: Test): void {
    const innerX = 3;
    const innerY = 1;
    const innerZ = 3;
    const internalW = 2;
    const internalH = 2;

    buildXAxisPortalFrame(test, innerX, innerY, innerZ, internalW, internalH);

    // 在框架内部放置火方块。
    test.setBlockType("minecraft:fire", { x: innerX, y: innerY + 1, z: innerZ });

    // 验证内部未生成 nether_portal 方块。
    const portalType = "minecraft:nether_portal";
    for (let dx = 0; dx < internalW; dx++) {
        for (let dy = 0; dy < internalH; dy++) {
            const typeId = getBlockTypeId(test, innerX + dx, innerY + dy, innerZ);
            test.assert(typeId !== portalType, `height 2 should not form portal, got ${typeId}`);
        }
    }

    test.succeed();
}

// 场景 5：顶框缺失不点燃传送门。
//
// wiki 机制：框架须完整（底框、左右框、顶框均由黑曜石构成）。顶框缺失时 _checkTopFrame 失败，
//   不构成有效下界传送门。
//
// 布局：构建最小 X 轴向框架（内部 2×3），但故意移除顶框（Y=4 的黑曜石替换为空气）。
//   - 火方块放置在内部 (3,2,3)。
//
// 判定：内部不应生成 nether_portal 方块（顶框缺失 → _checkTopFrame 返回 false）。
//
// Ref: PortalSize.cpp#_checkTopFrame（顶部框架不完整返回 false）
function incompleteTopFrameNoPortal(test: Test): void {
    const innerX = 3;
    const innerY = 1;
    const innerZ = 3;
    const internalW = 2;
    const internalH = 3;

    buildXAxisPortalFrame(test, innerX, innerY, innerZ, internalW, internalH);

    // 移除顶框（Y=4 的黑曜石替换为空气）。
    for (let dx = 0; dx < internalW + 2; dx++) {
        clearToAir(test, innerX + dx - 1, innerY + internalH, innerZ);
    }

    // 在框架内部放置火方块。
    test.setBlockType("minecraft:fire", { x: innerX, y: innerY + 1, z: innerZ });

    // 验证内部未生成 nether_portal 方块。
    const portalType = "minecraft:nether_portal";
    for (let dx = 0; dx < internalW; dx++) {
        for (let dy = 0; dy < internalH; dy++) {
            const typeId = getBlockTypeId(test, innerX + dx, innerY + dy, innerZ);
            test.assert(typeId !== portalType, `incomplete top frame should not form portal, got ${typeId}`);
        }
    }

    test.succeed();
}

// 场景 6：火方块在框架外部不点燃传送门。
//
// wiki 机制：火方块须放置在有效框架内部才能点燃传送门。火方块在框架外部（无有效框架）不点燃。
//
// 布局：glass_pit 内部仅放置火方块（无黑曜石框架）。
//   - 火方块放置在 (3,2,3)。
//
// 判定：火方块位置不应生成 nether_portal 方块（无有效框架）。
//
// Ref: PortalSize.cpp#findNetherPortal（无有效框架返回 nullopt）
function fireOutsideFrameNoPortal(test: Test): void {
    const fireX = 3;
    const fireY = 2;
    const fireZ = 3;

    // 在无框架的位置放置火方块。
    test.setBlockType("minecraft:fire", { x: fireX, y: fireY, z: fireZ });

    // 验证火方块位置未生成 nether_portal 方块。
    const typeId = getBlockTypeId(test, fireX, fireY, fireZ);
    test.assert(typeId !== "minecraft:nether_portal", `fire outside frame should not form portal, got ${typeId}`);

    test.succeed();
}

export function registerNetherPortalFrameSizeTests(): void {
    GameTest.register("TeleportTests", "min_frame_x_axis_ignites", minFrameXAxisIgnites)
        .structureName("gametests:glass_pit")
        .maxTicks(40);

    GameTest.register("TeleportTests", "z_axis_portal_frame_ignites", zAxisPortalFrameIgnites)
        .structureName("gametests:glass_pit")
        .maxTicks(40);

    GameTest.register("TeleportTests", "width_too_narrow_no_portal", widthTooNarrowNoPortal)
        .structureName("gametests:glass_pit")
        .maxTicks(40);

    GameTest.register("TeleportTests", "height_too_short_no_portal", heightTooShortNoPortal)
        .structureName("gametests:glass_pit")
        .maxTicks(40);

    GameTest.register("TeleportTests", "incomplete_top_frame_no_portal", incompleteTopFrameNoPortal)
        .structureName("gametests:glass_pit")
        .maxTicks(40);

    GameTest.register("TeleportTests", "fire_outside_frame_no_portal", fireOutsideFrameNoPortal)
        .structureName("gametests:glass_pit")
        .maxTicks(40);
}
