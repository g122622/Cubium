// 苍白苔藓地毯放置与墙面延伸行为 GameTest。
//
// wiki other_苍白苔藓块.txt：苍白苔藓地毯（pale_moss_carpet）可附着在方块侧面/底面。
//   BASE（bottom）为 true 时是平铺在地面上的底座，否则是悬挂在方块面（北/东/南/西）上的薄片，
//   通过 WALL_HEIGHT 属性区分 NONE/LOW/TALL。
//   - canSurvive：底座需下方非空气；非底座需下方为同类型底座方块。
//   - getUpdatedState：根据各水平方向相邻方块是否可附着，决定该方向 LOW/NONE；
//     若上方同类型方块该方向非 NONE 且自身非底座，则升级为 TALL。
//
// C++ 链路（MossyCarpetBlock.cpp）：
//   - 默认 state（:142-147）：BOTTOM=true + 各方向 WALL_HEIGHT=None
//   - getStateForPlacement（:224-227）：调 _getUpdatedState(defaultState(), world, placementPos, includeBase=true)
//   - _getUpdatedState（:188-222）：base=includeBase||BOTTOM；各水平方向
//     _canSupportAtFace→(base?Low:原值):None；Low 且上方同类型非底座→Tall；非底座且下方同类型该方向 None→None
//   - _canSupportAtFace（:179-186）：Up 不支持；其余委托 MultifaceBlock.canAttachTo（相邻方块该面 full）
//   - isValidPosition（:256-266）：底座下方非空气；非底座下方为同类型底座
//   - updatePostPlacement（:229-254）：!isValidPosition→air；_getUpdatedState(includeBase=false)；!_hasFaces→air
//
// 测试覆盖（2 个场景，覆盖 wiki 放置底座 + 相邻方块墙面延伸核心行为）：
//   1. pale_moss_carpet_places_base_on_stone：在 stone 上方放置 pale_moss_carpet
//      → bottom=true + north/east/south/west=none（四周空气无可附着面，仅底座）。
//   2. pale_moss_carpet_extends_wall_toward_supporting_neighbor：放 pale_moss_carpet 在 stone 上方，
//      且 East 侧 (4,2,1) 放 stone（可附着的实心方块）→ east=low（East 方向相邻 stone 可附着→Low）。
//
// 关键约束：
// 1. pale_moss_carpet 是完整方块底座 + 薄片墙面，放 stone 上方 (3,2,1)。
// 2. 场景 1 验证放置在四周空气环境 → 仅底座，各方向 None。
//    - 旧实现（如果有 bug）：可能各方向错误设为 Low。
//    - 新实现：四周空气 → _canSupportAtFace false → None。
// 3. 场景 2 在 East 侧放 stone → _canSupportAtFace(East) 检查 (4,2,1)=stone 的 West 面 full → true
//    → east=low（base=true → Low）。验证墙面延伸行为。
// 4. 读 state 用 getState("bottom"/"north"/"east"/"south"/"west" as any)。
//    bottom 返 boolean；各方向返 "none"/"low"/"tall"（EnumProperty<WallHeight>）。
//
// 不测「TALL 升级」：需上方同类型方块该方向非 NONE 且非底座，布局复杂，跳过。
//   TODO: 待 TALL 升级链路稳定后补 tall_upgrade 测试。
// 不测「非底座悬挂薄片」：需下方为同类型底座方块，布局复杂，跳过。
//   TODO: 待悬挂薄片链路稳定后补 hanging_sheet 测试。
// 不测「无面销毁」：updatePostPlacement 中 !_hasFaces→air，需精确触发邻居更新，跳过。
//   TODO: 待无面销毁链路稳定后补 no_face_destroyed 测试。
//
// 跨服务端：pale_moss_carpet 方块名两端一致。bottom/north/east/south/west state 名两端一致。
//   放置底座（bottom=true + 各方向 none）+ 墙面延伸（相邻 stone → east=low）行为与 vanilla 一致，
//   可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_苍白苔藓块.txt（底座 + 墙面延伸）
// Ref: MossyCarpetBlock.cpp:188-222（_getUpdatedState 墙面延伸逻辑）
// Ref: MossyCarpetBlock.cpp:179-186（_canSupportAtFace 委托 MultifaceBlock.canAttachTo）
// Ref: MultifaceBlock.cpp:173-187（canAttachTo 检查相邻方块该面 full）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 布局：pale_moss_carpet 放 glass_pit 中心 (3,2,3)，下方 (3,1,3) 放 stone 支撑。
// 中心位置确保四周 (3,2,2)/(4,2,3)/(3,2,4)/(2,2,3) 都是 air（远离 glass_pit 玻璃墙），
// 避免玻璃墙被 _canSupportAtFace 误判为可附着面。

const STONE_POS = { x: 3, y: 1, z: 3 };
const CARPET_POS = { x: 3, y: 2, z: 3 };
const PLAYER_POS = { x: 1, y: 2, z: 3 };

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 pale_moss_carpet bottom state（boolean）。返回 null 表示失败。
function getCarpetBottom(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("bottom" as any);
    return typeof value === "boolean" ? value : null;
}

// 读取 pale_moss_carpet 指定方向 wall_height state（"none"/"low"/"tall"）。
// propName 为 "north"/"east"/"south"/"west"。返回 null 表示失败。
function getCarpetWallHeight(test: Test, x: number, y: number, z: number, propName: string): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState(propName as any);
    return typeof value === "string" ? value : null;
}

// 场景 1：在 stone 上方放置 pale_moss_carpet → bottom=true + 各方向 none。
//
// 布局：(3,1,1) stone 支撑，玩家 (1,2,1) 点击 stone 顶面 Up → pale_moss_carpet 落 (3,2,1)。
// getStateForPlacement → _getUpdatedState(defaultState(), world, (3,2,1), includeBase=true)
//   → base=true；各水平方向 _canSupportAtFace 检查相邻方块（(3,2,0)/(4,2,1)/(3,2,2)/(2,2,1) 均为 air）
//   → false → None。
// 结果：BOTTOM=true + north/east/south/west=none。
//
// 判定：useItemOnBlock 返 true + typeId === "minecraft:pale_moss_carpet"
//   + bottom === true + north/east/south/west === "none"。
function paleMossCarpetPlacesBaseOnStone(test: Test): void {
    test.setBlockType("minecraft:stone", STONE_POS);
    test.assert(getBlockTypeId(test, STONE_POS.x, STONE_POS.y, STONE_POS.z) === "minecraft:stone", `stone should be at ${JSON.stringify(STONE_POS)}, got ${getBlockTypeId(test, STONE_POS.x, STONE_POS.y, STONE_POS.z)}`);

    const player = test.spawnSimulatedPlayer(PLAYER_POS, "farmer");
    const carpetItem = new ItemStack("minecraft:pale_moss_carpet", 1);

    // 点击 stone 顶面 Up → pale_moss_carpet 落 (3,2,1)。
    const used = player.useItemOnBlock(
        carpetItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        STONE_POS,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing pale_moss_carpet");

    pollUntilSucceed(
        test,
        () => {
            return getBlockTypeId(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z) === "minecraft:pale_moss_carpet"
                && getCarpetBottom(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z) === true
                && getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "north") === "none"
                && getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "east") === "none"
                && getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "south") === "none"
                && getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "west") === "none";
        },
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(
                    false,
                    `pale_moss_carpet base wrong: typeId=${getBlockTypeId(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z)} `
                        + `bottom=${getCarpetBottom(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z)} `
                        + `north=${getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "north")} `
                        + `east=${getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "east")} `
                        + `south=${getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "south")} `
                        + `west=${getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "west")} `
                        + `(expected: pale_moss_carpet / bottom=true / all directions none)`,
                );
            },
        },
    );
}

// 场景 2：East 侧放 stone → east=low（墙面延伸）。
//
// 布局：(3,1,1) stone 支撑 + (4,2,1) stone（East 侧可附着方块）。
// 玩家 (1,2,1) 点击 (3,1,1) 顶面 Up → pale_moss_carpet 落 (3,2,1)。
// getStateForPlacement → _getUpdatedState(defaultState(), world, (3,2,1), includeBase=true)
//   → base=true；
//   East 方向 _canSupportAtFace(East) → MultifaceBlock.canAttachTo(world, East, (4,2,1))
//     → Block.isFaceFull(stone.shape, opposite(East)=West) → stone 的 West 面 full → true
//     → East 方向 height = base?Low = Low
//   North/South/West 方向相邻 air → _canSupportAtFace false → None。
// 结果：BOTTOM=true + east=low + north/south/west=none。
//
// 判定：useItemOnBlock 返 true + typeId === "minecraft:pale_moss_carpet"
//   + bottom === true + east === "low" + north/south/west === "none"。
function paleMossCarpetExtendsWallTowardStone(test: Test): void {
    test.setBlockType("minecraft:stone", STONE_POS);
    test.setBlockType("minecraft:stone", { x: 4, y: 2, z: 3 }); // East 侧可附着 stone
    test.assert(getBlockTypeId(test, STONE_POS.x, STONE_POS.y, STONE_POS.z) === "minecraft:stone", `stone should be at ${JSON.stringify(STONE_POS)}, got ${getBlockTypeId(test, STONE_POS.x, STONE_POS.y, STONE_POS.z)}`);
    test.assert(getBlockTypeId(test, 4, 2, 3) === "minecraft:stone", `east stone should be at (4,2,3), got ${getBlockTypeId(test, 4, 2, 3)}`);

    const player = test.spawnSimulatedPlayer(PLAYER_POS, "farmer");
    const carpetItem = new ItemStack("minecraft:pale_moss_carpet", 1);

    // 点击 stone 顶面 Up → pale_moss_carpet 落 (3,2,1)。
    const used = player.useItemOnBlock(
        carpetItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        STONE_POS,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing pale_moss_carpet");

    pollUntilSucceed(
        test,
        () => {
            return getBlockTypeId(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z) === "minecraft:pale_moss_carpet"
                && getCarpetBottom(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z) === true
                && getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "east") === "low"
                && getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "north") === "none"
                && getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "south") === "none"
                && getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "west") === "none";
        },
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(
                    false,
                    `pale_moss_carpet wall extend wrong: typeId=${getBlockTypeId(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z)} `
                        + `bottom=${getCarpetBottom(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z)} `
                        + `east=${getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "east")} `
                        + `north=${getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "north")} `
                        + `south=${getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "south")} `
                        + `west=${getCarpetWallHeight(test, CARPET_POS.x, CARPET_POS.y, CARPET_POS.z, "west")} `
                        + `(expected: east=low [wall extends toward supporting stone], other directions none)`,
                );
            },
        },
    );
}

export function registerMossyCarpetTests(): void {
    GameTest.register("BlockBehaviorTests", "pale_moss_carpet_places_base_on_stone", paleMossCarpetPlacesBaseOnStone)
        .structureName("gametests:glass_pit")
        .maxTicks(60);

    GameTest.register(
        "BlockBehaviorTests",
        "pale_moss_carpet_extends_wall_toward_supporting_neighbor",
        paleMossCarpetExtendsWallTowardStone,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
