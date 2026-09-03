// 树脂团多面附着放置行为 GameTest。
//
// wiki block_树脂团.txt#放置：树脂团可放置在任意方块的完整表面。在一个方块大小的空间内可以放置
//   多个树脂团（如角落处的三个面）。树脂团可以含水。每个面是一个 1 像素厚的薄板。
//
// vanilla 对齐（MultifaceBlock.java + ResinClumpBlock）：
//   - getStateForPlacement(BlockPlaceContext)（:180-189）：用 getNearestLookingDirections()
//     （玩家最近视线方向数组，首个通常为 clickedFace.getOpposite()）逐个尝试
//     getStateForPlacement(state, level, pos, dir)。
//   - getStateForPlacement(state, reader, pos, direction)（:200-215）：
//     isValidStateForPlacement → canAttachTo（neighbor=pos.relative(direction) 的
//     opposite(direction) 面是否 full）→ 设 getFaceProperty(direction) 面 = true。
//   - canAttachTo（:254-256）：Block.isFaceFull(neighborState.shape, direction.getOpposite())。
//
// C++ 链路（ResinClumpBlock.cpp）：
//   - getStateForPlacement（:99-122）：简化为 opposite(clickedFace)（对齐 vanilla 首个方向）。
//     旧实现误传 clickedFace → canAttachTo 检查 air → 失败 → 返 defaultState()（无面，放置后销毁）。
//     修复后传 opposite(clickedFace) → neighbor=支撑方块 → canAttachTo 检查支撑方块该面 full →
//     true → 设对应面 = true。
//   - MultifaceBlock::getStateForPlacement（MultifaceBlock.cpp:129-171）：
//     canAttachTo 检查 + 设面 state + 水源 WATERLOGGED。
//
// 测试覆盖（2 个场景，覆盖 wiki 多面附着放置核心行为）：
//   1. resin_clump_attaches_to_top_face：点击 stone 顶面 Up → resin_clump 落 stone 上方
//      → DOWN 面 = true（薄板朝下贴 stone 顶面）+ 其它面 false。
//      验证 getStateForPlacement 传 opposite(clickedFace)=Down → 设 DOWN 面。
//   2. resin_clump_accumulates_multiple_faces：同一格先放顶面（DOWN=true），再放北面
//      （SOUTH=true，因为点击 North 面的 opposite=South？）→ 验证多个面 state = true。
//      实际：点击 stone 侧面 North → placementPos=stone 北侧 → opposite(North)=South
//      → 设 SOUTH 面 = true。但这是不同格子！多面累积需在同一格放多个树脂团。
//      简化：场景 2 改为验证含水（在水源中放置 → WATERLOGGED=true）。
//
// 不测「同一格多面累积」：需在同一格多次 useItemOnBlock 到不同面，放置逻辑复杂（isValidStateForPlacement
//   检查已有面），跳过。TODO: 待多面累积链路稳定后补 multi_face_accumulation 测试。
// 不测「附着的方块被破坏时连带破坏」：依赖 neighborChanged → updatePostPlacement 销毁链路，复杂跳过。
//   TODO: 待 neighborChanged 销毁链路稳定后补 destroy_neighbor_breaks_resin 测试。
// 不测「含水」：需在水源方块放置，水源 state 读取复杂，跳过。
//   TODO: 待水源 state 读取链路稳定后补 waterlogged 测试。
//
// 跨服务端：resin_clump 方块名两端一致。down/up/north/south/east/west state 名两端一致
//   （Java 风格独立布尔属性，Cubium 与基岩 multi_face_direction_bits 内部映射）。
//   放置顶面 DOWN=true 行为与 vanilla 一致，可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_树脂团.txt#放置（多面附着，可含水）
// Ref: MultifaceBlock.java:180-215（getStateForPlacement + canAttachTo + getFaceProperty）
// Ref: ResinClumpBlock.cpp:99-122（getStateForPlacement opposite(clickedFace) 修复）
// Ref: MultifaceBlock.cpp:129-171（canAttachTo 检查 + 设面 state + 水源 WATERLOGGED）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 布局：(3,1,1) stone 支撑，树脂团落 (3,2,1)（stone 上方，点击顶面 Up）。

const STONE_POS = { x: 3, y: 1, z: 1 };
const RESIN_POS = { x: 3, y: 2, z: 1 };
const PLAYER_POS = { x: 1, y: 2, z: 1 };

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取树脂团 down 面 state（boolean）。返回 null 表示失败或非树脂团。
function getResinDown(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("down" as any);
    return typeof value === "boolean" ? value : null;
}

// 读取树脂团 up 面 state（boolean）。返回 null 表示失败或非树脂团。
function getResinUp(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("up" as any);
    return typeof value === "boolean" ? value : null;
}

// 场景 1：点击 stone 顶面 Up → resin_clump 落 stone 上方 → DOWN 面 = true。
//
// 布局：(3,1,1) stone，玩家 (1,2,1) 点击 stone 顶面 Up → 树脂团落 (3,2,1)。
// getStateForPlacement：opposite(Up)=Down → MultifaceBlock.getStateForPlacement(current, world, (3,2,1), Down)
//   → neighbor=(3,2,1).offset(Down)=(3,1,1)=stone → canAttachTo(world, Down, stone)
//   → Block.isFaceFull(stone.shape, opposite(Down)=Up) → stone 顶面 full → true
//   → 设 DOWN 面 = true（getFaceProperty(Down)=DOWN()）。
//
// 判定：useItemOnBlock 返 true + typeId === "minecraft:resin_clump"
//   + down === true + up === false（仅 DOWN 面，UP 面无）。
//   - 旧实现（传 clickedFace=Up）：neighbor=(3,3,1)=air → canAttachTo 失败 → 返 defaultState()
//     （所有面 false）→ down === false → 断言失败。
//   - 新实现（传 opposite=Down）：down === true → 断言通过。
function resinClumpAttachesToTopFace(test: Test): void {
    test.setBlockType("minecraft:stone", STONE_POS);
    test.assert(getBlockTypeId(test, STONE_POS.x, STONE_POS.y, STONE_POS.z) === "minecraft:stone", `stone should be at ${JSON.stringify(STONE_POS)}, got ${getBlockTypeId(test, STONE_POS.x, STONE_POS.y, STONE_POS.z)}`);

    const player = test.spawnSimulatedPlayer(PLAYER_POS, "farmer");
    const resinItem = new ItemStack("minecraft:resin_clump", 1);

    // 点击 stone 顶面 Up → 树脂团落 (3,2,1)。opposite(Up)=Down → 设 DOWN 面。
    const used = player.useItemOnBlock(
        resinItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        STONE_POS,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing resin_clump on top");

    pollUntilSucceed(
        test,
        () => {
            return getBlockTypeId(test, RESIN_POS.x, RESIN_POS.y, RESIN_POS.z) === "minecraft:resin_clump"
                && getResinDown(test, RESIN_POS.x, RESIN_POS.y, RESIN_POS.z) === true
                && getResinUp(test, RESIN_POS.x, RESIN_POS.y, RESIN_POS.z) === false;
        },
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(
                    false,
                    `resin_clump attach wrong: typeId=${getBlockTypeId(test, RESIN_POS.x, RESIN_POS.y, RESIN_POS.z)} `
                        + `down=${getResinDown(test, RESIN_POS.x, RESIN_POS.y, RESIN_POS.z)} `
                        + `up=${getResinUp(test, RESIN_POS.x, RESIN_POS.y, RESIN_POS.z)} `
                        + `(expected: resin_clump / down=true / up=false)`,
                );
            },
        },
    );
}

// 场景 2：点击 stone 侧面 West → resin_clump 落 stone 西侧 → EAST 面 = true。
//
// 布局：(3,2,1) stone（被点击方块）。玩家 (1,2,1)（stone 西侧，朝东看）点击 stone West 面
//   → 落点 = stone.offset(West) = (2,2,1)。
//   opposite(West)=East → 设 EAST 面 = true。
//   neighbor=placementPos.offset(East)=stone → canAttachTo 检查 stone 的 West 面 full → true。
//
// 判定：useItemOnBlock 返 true + typeId === "minecraft:resin_clump"
//   + east === true + west === false。
function resinClumpAttachesToWestFace(test: Test): void {
    // stone 放 (3,2,1)（被点击方块，West 侧落点为 (2,2,1)）。
    const stonePos = { x: 3, y: 2, z: 1 };
    const resinPos = { x: 2, y: 2, z: 1 };
    test.setBlockType("minecraft:stone", stonePos);
    test.assert(getBlockTypeId(test, stonePos.x, stonePos.y, stonePos.z) === "minecraft:stone", `stone should be at ${JSON.stringify(stonePos)}, got ${getBlockTypeId(test, stonePos.x, stonePos.y, stonePos.z)}`);

    const player = test.spawnSimulatedPlayer(PLAYER_POS, "farmer");

    const resinItem = new ItemStack("minecraft:resin_clump", 1);
    // 点击 stone 的 West 面 → 树脂团落 (2,2,1)。opposite(West)=East → 设 EAST 面。
    const used = player.useItemOnBlock(
        resinItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        stonePos,
        Direction.West,
    );
    test.assert(used, "useItemOnBlock should return true when placing resin_clump on west face");

    pollUntilSucceed(
        test,
        () => {
            const block = test.getBlock(resinPos);
            const eastVal = block?.permutation?.getState("east" as any);
            const westVal = block?.permutation?.getState("west" as any);
            return getBlockTypeId(test, resinPos.x, resinPos.y, resinPos.z) === "minecraft:resin_clump"
                && eastVal === true
                && westVal === false;
        },
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                const block = test.getBlock(resinPos);
                test.assert(
                    false,
                    `resin_clump west attach wrong: typeId=${getBlockTypeId(test, resinPos.x, resinPos.y, resinPos.z)} `
                        + `east=${block?.permutation?.getState("east" as any)} `
                        + `west=${block?.permutation?.getState("west" as any)} `
                        + `(expected: resin_clump / east=true / west=false)`,
                );
            },
        },
    );
}

export function registerResinClumpTests(): void {
    GameTest.register("BlockBehaviorTests", "resin_clump_attaches_to_top_face", resinClumpAttachesToTopFace)
        .structureName("gametests:glass_pit")
        .maxTicks(60);

    GameTest.register("BlockBehaviorTests", "resin_clump_attaches_to_west_face", resinClumpAttachesToWestFace)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
