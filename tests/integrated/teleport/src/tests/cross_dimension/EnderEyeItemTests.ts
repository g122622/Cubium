// 末影之眼物品激活末地传送门测试：验证末影之眼放入框架、图案匹配激活传送门的核心行为。
//
// wiki 机制（tech_末影之眼.txt、tech_末地传送门框架.txt）：
//   - 末影之眼对末地传送门框架（EYE=false）右键使用 → 将末影之眼放入框架（EYE: false→true），
//     消耗 1 个末影之眼。
//   - 当 12 个框架全部带眼且朝向正确时，构成完整传送门图案，在框架内部 3×3 区域生成 end_portal 方块。
//   - 框架朝向固定四方向（NORTH/SOUTH/EAST/WEST），图案为 5×5：
//       ? v v v ?
//       > ? ? ? <
//       > ? ? ? <
//       > ? ? ? <
//       ? ^ ^ ^ ?
//     其中 v=框架(FACING=NORTH,HAS_EYE=true)，^=框架(FACING=SOUTH,HAS_EYE=true)，
//     >=框架(FACING=WEST,HAS_EYE=true)，<=框架(FACING=EAST,HAS_EYE=true)，?=任意方块（角落与内部）。
//   - 放入末影之眼后框架升高 13/16→1.0（pushEntitiesUp）。
//   - 已有眼的框架再用末影之眼点击 → 返回 Pass（不消耗物品）。
//
// Cubium 实现（EnderEyeItem.cpp）：
//   - onItemUse：仅当点击的是 END_PORTAL_FRAME 且 EYE=false 时生效：
//     1. 设 EYE=true，Block::pushEntitiesUp（框架升高，把上方实体顶起）。
//     2. world.setBlockState(pos, &newState, 2)。
//     3. 播放末影之眼放入框架的事件（levelEvent 1503）。
//     4. 用 BlockPattern 检测完整传送门图案，若匹配则在内部 3×3 区域生成 end_portal 方块。
//   - 消耗由上层 SimulatedPlayer::useItemOnBlock 在 onItemUse 返回 Success 后统一对权威槽
//     shrink(1) 回写完成（SimulatedPlayer.cpp:400-408）。本方法返回 Success 即触发该消耗。
//   - getOrCreatePortalShape：对齐 Java EndPortalFrameBlock.getOrCreatePortalShape()，图案 5×5 单层。
//     字符 '?vvv?', '>\?\?\?<', '>\?\?\?<', '>\?\?\?<', '?^^^?'（\? 转义避免 C++ trigraph）。
//
// 坐标推导（forwards=DOWN, up=SOUTH 匹配组合）：
//   - side = cross(DOWN, SOUTH) = WEST (-x)
//   - finalPos = origin + SOUTH*(-heightIdx) + WEST*widthIdx + DOWN*depthIdx
//   - 设 origin = (5,1,5)（SE 角，图案右下角）：
//       j=0（z=5）：i=0→(5,1,5)='?', i=1→(4,1,5)='v', i=2→(3,1,5)='v', i=3→(2,1,5)='v', i=4→(1,1,5)='?'
//       j=1（z=4）：i=0→(5,1,4)='>', i=4→(1,1,4)='<'
//       j=2（z=3）：i=0→(5,1,3)='>', i=4→(1,1,3)='<'
//       j=3（z=2）：i=0→(5,1,2)='>', i=4→(1,1,2)='<'
//       j=4（z=1，最北行）：i=0→(5,1,1)='?', i=1→(4,1,1)='^', i=2→(3,1,1)='^', i=3→(2,1,1)='^', i=4→(1,1,1)='?'
//   - frontTopLeft = (5,1,5)，传送门区域 = (5,1,5)+(-3,0,-3) = (2,1,2)，中心 3×3 = x∈[2,4], z∈[2,4]
//
// 完整布局（glass_pit Y=1 层，内部 5×5 空间 x∈[1,5], z∈[1,5]）：
//        x=1      x=2      x=3      x=4      x=5
//   z=1  ?        ^(S)     ^(S)     ^(S)     ?
//   z=2  <(E)     ?        ?        ?        >(W)
//   z=3  <(E)     ?        ?        ?        >(W)
//   z=4  <(E)     ?        ?        ?        >(W)
//   z=5  ?        v(N)     v(N)     v(N)     ?
//
// 测试覆盖（4 个核心行为点）：
//   1. ender_eye_placed_in_frame：末影之眼放入框架（EYE: false→true），消耗 1 个末影之眼。
//   2. ender_eye_activates_end_portal：完整 12 框架图案激活末地传送门（内部 3×3 生成 end_portal）。
//   3. incomplete_pattern_no_portal：不完整图案（缺框架或朝向错）不生成 end_portal。
//   4. ender_eye_on_filled_frame_pass：对已有眼框架用末影之眼返回 Pass（useItemOnBlock 返回 false）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末影之眼.txt
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末地传送门框架.txt
// Ref: net.minecraft.world.item.EnderEyeItem#useOn（Java 1.21.11）
// Ref: net.minecraft.world.level.block.EndPortalFrameBlock#getOrCreatePortalShape（Java 1.21.11）
// Ref: EnderEyeItem.cpp（Cubium src/common/item/items/special/）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { GameMode, ItemStack, Direction } from "@minecraft/server";

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 框架位置常量（glass_pit Y=1 层 5×5 内部空间）。
// 完整图案 12 框架位置（按图案行列）：
//   北边 3 个 ^(S): (2,1,1), (3,1,1), (4,1,1)
//   南边 3 个 v(N): (2,1,5), (3,1,5), (4,1,5)
//   西边 3 个 <(E): (1,1,2), (1,1,3), (1,1,4)
//   东边 3 个 >(W): (5,1,2), (5,1,3), (5,1,4)
//   四角 ? 为任意方块（默认 air）：(1,1,1), (5,1,1), (1,1,5), (5,1,5)
//   内部 3×3 ? 为任意方块（默认 air）：x∈[2,4], z∈[2,4]

// 框架朝向映射：图案字符 → facing state 值。
//   v = FACING=NORTH → "north"
//   ^ = FACING=SOUTH → "south"
//   > = FACING=WEST → "west"
//   < = FACING=EAST → "east"
type FrameSpec = { pos: { x: number; y: number; z: number }; facing: string };

// 北边 3 个 ^(S, facing=south)
const NORTH_FRAMES: FrameSpec[] = [
    { pos: { x: 2, y: 1, z: 1 }, facing: "south" },
    { pos: { x: 3, y: 1, z: 1 }, facing: "south" },
    { pos: { x: 4, y: 1, z: 1 }, facing: "south" },
];

// 南边 3 个 v(N, facing=north)
const SOUTH_FRAMES: FrameSpec[] = [
    { pos: { x: 2, y: 1, z: 5 }, facing: "north" },
    { pos: { x: 3, y: 1, z: 5 }, facing: "north" },
    { pos: { x: 4, y: 1, z: 5 }, facing: "north" },
];

// 西边 3 个 <(E, facing=east)
const WEST_FRAMES: FrameSpec[] = [
    { pos: { x: 1, y: 1, z: 2 }, facing: "east" },
    { pos: { x: 1, y: 1, z: 3 }, facing: "east" },
    { pos: { x: 1, y: 1, z: 4 }, facing: "east" },
];

// 东边 3 个 >(W, facing=west)
const EAST_FRAMES: FrameSpec[] = [
    { pos: { x: 5, y: 1, z: 2 }, facing: "west" },
    { pos: { x: 5, y: 1, z: 3 }, facing: "west" },
    { pos: { x: 5, y: 1, z: 4 }, facing: "west" },
];

// 全部 12 框架（北+南+西+东）。
const ALL_FRAMES: FrameSpec[] = [...NORTH_FRAMES, ...SOUTH_FRAMES, ...WEST_FRAMES, ...EAST_FRAMES];

// 传送门中心 3×3 区域（frontTopLeft=(5,1,5) + (-3,0,-3) = (2,1,2)）。
const PORTAL_TOPLEFT = { x: 2, y: 1, z: 2 };

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 末地传送门框架 eye state（布尔 true/false）。
function getEye(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("eye" as any);
    return typeof value === "boolean" ? value : null;
}

// 读取 (x,y,z) 末地传送门框架 facing state（方向名字符串 "north"/"south"/"east"/"west"）。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 放置单个末地传送门框架（指定 facing 和 eye 状态）。
function placeFrame(test: Test, spec: FrameSpec, eye: boolean): void {
    (test as TestWithStates).setBlockWithStates(
        "minecraft:end_portal_frame",
        spec.pos,
        `facing=${spec.facing},eye=${eye}`,
    );
}

// 放置全部 12 框架（无眼状态），用于激活测试前置布局。
function placeAllFramesEmptyEye(test: Test): void {
    for (const frame of ALL_FRAMES) {
        placeFrame(test, frame, false);
    }
}

// 检查传送门中心 3×3 区域是否全部为 end_portal 方块。
function isPortalFormed(test: Test): boolean {
    for (let dx = 0; dx < 3; dx++) {
        for (let dz = 0; dz < 3; dz++) {
            const pos = { x: PORTAL_TOPLEFT.x + dx, y: PORTAL_TOPLEFT.y, z: PORTAL_TOPLEFT.z + dz };
            if (getBlockTypeId(test, pos.x, pos.y, pos.z) !== "minecraft:end_portal") {
                return false;
            }
        }
    }
    return true;
}

// 场景 1：末影之眼放入框架（EYE: false→true），消耗 1 个末影之眼。
//
// wiki 机制（tech_末影之眼.txt#用途）：对末地传送门框架右键使用末影之眼 → 放入框架（EYE: false→true），
//   消耗 1 个末影之眼。
//
// 布局：(1,1,1) 放置一个 eye=false, facing=north 的末地传送门框架。SimulatedPlayer 站在框架旁，
//   手持 1 个 ender_eye useItemOnBlock 点击 (1,1,1) 框架顶面 Up。
//
// 判定：
//   1. useItemOnBlock 返回 true（物品成功使用）。
//   2. 框架 eye state 从 false 变为 true。
//   3. 玩家主手 ender_eye 数量从 1 变为 0（消耗 1 个）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末影之眼.txt#用途
// Ref: EnderEyeItem.cpp#onItemUse（设 EYE=true，返回 Success 触发上层消耗）
function enderEyePlacedInFrame(test: Test): void {
    // 放置一个无眼的末地传送门框架。
    placeFrame(test, { pos: { x: 3, y: 1, z: 3 }, facing: "north" }, false);
    test.assert(getEye(test, 3, 1, 3) === false, `eye should be false before placing, got ${getEye(test, 3, 1, 3)}`);

    // spawn SimulatedPlayer（生存模式，验证物品消耗），手持 1 个 ender_eye，点击框架顶面。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "frame_filler", GameMode.survival);
    const enderEye = new ItemStack("minecraft:ender_eye", 1);
    const used = player.useItemOnBlock(
        enderEye as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 3 },
        Direction.Up,
    );

    test.assert(used, "useItemOnBlock should return true when placing ender_eye in frame");
    test.assert(getEye(test, 3, 1, 3) === true, `eye should be true after placing, got ${getEye(test, 3, 1, 3)}`);

    // 验证消耗：玩家主手 ender_eye 数量应为 0。
    // 读取主手物品经 equippable.getEquipment("Mainhand")（返回 owned ItemStack | undefined）。
    const equippable = player.getComponent("minecraft:equippable") as any;
    const heldItem = equippable?.getEquipment?.("Mainhand");
    test.assert(
        heldItem === undefined || heldItem.typeId !== "minecraft:ender_eye",
        `ender_eye should be consumed (count 0), got ${heldItem?.typeId ?? "empty"} count ${heldItem?.amount ?? 0}`,
    );

    test.succeed();
}

// 场景 2：完整 12 框架图案激活末地传送门（内部 3×3 生成 end_portal）。
//
// wiki 机制（tech_末影之眼.txt#用途 + tech_末地传送门框架.txt#方块状态）：
//   - 12 个框架全部带眼且朝向正确 → 构成完整传送门图案 → 内部 3×3 生成 end_portal 方块。
//   - 图案字符朝向：v=NORTH, ^=SOUTH, >=WEST, <=EAST，全部 HAS_EYE=true。
//
// 布局：placeAllFramesEmptyEye 放置 12 个无眼框架（正确朝向）。然后用 SimulatedPlayer 手持 ender_eye
//   逐一点击 12 个框架放入末影之眼。第 12 个框架放入后，BlockPattern 匹配成功，内部 3×3 生成 end_portal。
//
// 判定：传送门中心 3×3 区域（x∈[2,4], z∈[2,4], y=1）全部为 end_portal 方块。
//
// 注意：useItemOnBlock 内部对 stack 拷贝操作，每次消耗 1 个。需给玩家足够多的 ender_eye（12 个）。
//   useItemOnBlock 每次调用会重新设入手持物，消耗后回写，故 12 次调用消耗 12 个。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末影之眼.txt#用途
// Ref: net.minecraft.world.level.block.EndPortalFrameBlock#getOrCreatePortalShape
// Ref: EnderEyeItem.cpp#getOrCreatePortalShape + onItemUse（图案匹配后生成 end_portal）
function enderEyeActivatesEndPortal(test: Test): void {
    // 放置 12 个无眼框架（正确朝向）。
    placeAllFramesEmptyEye(test);

    // spawn SimulatedPlayer（生存模式），手持 12 个 ender_eye。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "portal_activator", GameMode.survival);
    const enderEyeStack = new ItemStack("minecraft:ender_eye", 12);

    // 逐一点击 12 个框架放入末影之眼。每次 useItemOnBlock 消耗 1 个 ender_eye。
    for (const frame of ALL_FRAMES) {
        const used = player.useItemOnBlock(
            enderEyeStack as unknown as Parameters<typeof player.useItemOnBlock>[0],
            frame.pos,
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing ender_eye in frame at ${JSON.stringify(frame.pos)}`);
        test.assert(getEye(test, frame.pos.x, frame.pos.y, frame.pos.z) === true, `eye should be true after placing at ${JSON.stringify(frame.pos)}`);
    }

    // 验证传送门中心 3×3 区域全部为 end_portal 方块。
    test.assert(isPortalFormed(test), "end_portal should be formed in the 3×3 center area after all 12 frames are filled");

    test.succeed();
}

// 场景 3：不完整图案（缺框架或朝向错）不生成 end_portal。
//
// wiki 机制：只有 12 个框架全部带眼且朝向正确时才激活传送门。缺框架、朝向错误、缺眼均不激活。
//
// 布局：placeAllFramesEmptyEye 放置 12 个无眼框架，但故意将其中一个框架的 facing 改为错误方向
//   （例如将北边的 ^（SOUTH）改为 NORTH）。然后逐一放入 12 个末影之眼。
//
// 判定：传送门中心 3×3 区域不应出现 end_portal 方块（图案不匹配，onItemUse 不生成 end_portal）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末影之眼.txt#用途
// Ref: EnderEyeItem.cpp#onItemUse（图案匹配失败时不生成 end_portal）
function incompletePatternNoPortal(test: Test): void {
    // 放置 12 个无眼框架，但将北边中间的框架 facing 改为错误方向（原 SOUTH，改为 NORTH）。
    for (const frame of ALL_FRAMES) {
        let facing = frame.facing;
        // 故意将 (3,1,1) 的 facing 从 south 改为 north（错误朝向）。
        if (frame.pos.x === 3 && frame.pos.y === 1 && frame.pos.z === 1) {
            facing = "north";
        }
        placeFrame(test, { pos: frame.pos, facing }, false);
    }

    // 验证 (3,1,1) 的 facing 确实为 north（错误朝向）。
    test.assert(getFacing(test, 3, 1, 1) === "north", `(3,1,1) facing should be north (wrong), got ${getFacing(test, 3, 1, 1)}`);

    // spawn SimulatedPlayer（生存模式），手持 12 个 ender_eye。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "incomplete_filler", GameMode.survival);
    const enderEyeStack = new ItemStack("minecraft:ender_eye", 12);

    // 逐一点击 12 个框架放入末影之眼。
    for (const frame of ALL_FRAMES) {
        player.useItemOnBlock(
            enderEyeStack as unknown as Parameters<typeof player.useItemOnBlock>[0],
            frame.pos,
            Direction.Up,
        );
    }

    // 验证传送门中心 3×3 区域不应有 end_portal 方块。
    test.assert(!isPortalFormed(test), "end_portal should NOT be formed when pattern is incomplete (wrong facing)");

    test.succeed();
}

// 场景 4：对已有眼框架用末影之眼返回 Pass（useItemOnBlock 返回 false）。
//
// wiki 机制（tech_末影之眼.txt#用途）：末影之眼一旦被放置在末地传送门框架上就无法被拆下来。
//   对已有眼的框架再用末影之眼点击 → EnderEyeItem.onItemUse 返回 Pass → useItemOnBlock 返回 false。
//
// 布局：(3,1,3) 放置一个 eye=true 的末地传送门框架。SimulatedPlayer 手持 1 个 ender_eye，
//   useItemOnBlock 点击 (3,1,3) 框架顶面 Up。
//
// 判定：
//   1. useItemOnBlock 返回 false（onItemUse 返回 Pass，不消耗物品）。
//   2. 玩家主手 ender_eye 数量仍为 1（未消耗）。
//   3. 框架 eye state 仍为 true（未改变）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_末影之眼.txt#用途
// Ref: EnderEyeItem.cpp#onItemUse（EYE=true 时返回 Pass）
function enderEyeOnFilledFramePass(test: Test): void {
    // 放置一个有眼的末地传送门框架。
    placeFrame(test, { pos: { x: 3, y: 1, z: 3 }, facing: "north" }, true);
    test.assert(getEye(test, 3, 1, 3) === true, `eye should be true, got ${getEye(test, 3, 1, 3)}`);

    // spawn SimulatedPlayer（生存模式），手持 1 个 ender_eye，点击已有眼框架顶面。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "eye_on_filled", GameMode.survival);
    const enderEye = new ItemStack("minecraft:ender_eye", 1);
    const used = player.useItemOnBlock(
        enderEye as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 3 },
        Direction.Up,
    );

    // onItemUse 返回 Pass → useItemOnBlock 返回 false（不消耗物品）。
    test.assert(!used, "useItemOnBlock should return false when clicking ender_eye on filled frame (Pass)");

    // 验证未消耗：玩家主手 ender_eye 数量仍为 1。
    // 读取主手物品经 equippable.getEquipment("Mainhand")（返回 owned ItemStack | undefined）。
    const equippable = player.getComponent("minecraft:equippable") as any;
    const heldItem = equippable?.getEquipment?.("Mainhand");
    test.assert(
        heldItem !== undefined && heldItem.typeId === "minecraft:ender_eye" && heldItem.amount === 1,
        `ender_eye should not be consumed (count 1), got ${heldItem?.typeId ?? "empty"} count ${heldItem?.amount ?? 0}`,
    );

    // 框架 eye state 仍为 true。
    test.assert(getEye(test, 3, 1, 3) === true, `eye should still be true, got ${getEye(test, 3, 1, 3)}`);

    test.succeed();
}

export function registerEnderEyeItemTests(): void {
    GameTest.register("TeleportTests", "ender_eye_placed_in_frame", enderEyePlacedInFrame)
        .structureName("gametests:glass_pit")
        .maxTicks(60);

    GameTest.register("TeleportTests", "ender_eye_activates_end_portal", enderEyeActivatesEndPortal)
        .structureName("gametests:glass_pit")
        .maxTicks(120);

    GameTest.register("TeleportTests", "incomplete_pattern_no_portal", incompletePatternNoPortal)
        .structureName("gametests:glass_pit")
        .maxTicks(120);

    GameTest.register("TeleportTests", "ender_eye_on_filled_frame_pass", enderEyeOnFilledFramePass)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
