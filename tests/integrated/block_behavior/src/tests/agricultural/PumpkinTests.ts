// 南瓜剪刀雕刻行为 GameTest。
//
// wiki tech_南瓜.txt#雕刻：对一个南瓜使用剪刀，可以使其变成一个雕刻南瓜（carved_pumpkin），并掉落
//   4 份（JE）/1 份（BE）南瓜种子。Cubium 对齐 JE 掉 4 份。同时使南瓜顶部纹理旋转方向。剪刀雕刻产生
//   的雕刻南瓜若满足建造生物结构也可生成生物（铁傀儡/雪傀儡/凋灵），本组不测生成（需结构+实体）。
//
// C++ 链路：PumpkinBlock（agricultural/MelonPumpkinBlocks.cpp）。
//   - onBlockActivated（:97）：手持物非剪刀 → Pass；剪刀 + m_carvedPumpkin 非空 → 计算 carved 朝向
//     （hit.face() 上下则用玩家 yaw；水平则用 hit.face）+ 雕刻音效 + setBlockState(pos, carvedState, 11)
//     （pumpkin→carved_pumpkin 带 HORIZONTAL_FACING）+ 掉落4南瓜种子（ItemDropHelper）→ Success。
//   - 南瓜 minecraft:pumpkin（Material::EARTH，完整方块，无 canSurvive 自毁）。
//   - 雕刻南瓜 minecraft:carved_pumpkin（Material::EARTH，有 HORIZONTAL_FACING state）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock Block.use 前置分支（先 onBlockActivated，Pass 才 fallback）。
//   南瓜 onBlockActivated 手持剪刀 → Success 短路（不走 fallback，剪刀不触发 onItemUse 放置）。
//
// 测试覆盖（3 个场景，覆盖 wiki 剪刀雕刻 + 雕刻朝向核心确定行为）：
//   1. 剪刀雕刻南瓜：放南瓜 + 剪刀 useItemOnBlock → 原位 (3,2,1) 变 carved_pumpkin，返 true。
//   2. 顶面点击雕刻朝向：剪刀点击南瓜顶面 Up（4 朝向）→ facing=opposite(玩家水平朝向)。
//   3. 侧面点击雕刻朝向：剪刀点击南瓜南面 South → facing=south（点击面本身）。
//
// 关键约束：
// 1. 南瓜完整方块（Material::EARTH），无 canSurvive 自毁，放 (3,2,1)（minecraft:pumpkin）无需支撑。
// 2. 剪刀用 new ItemStack("minecraft:shears", 1)（耐久 238，创造模式 attemptDamageItem 不消耗耐久）。
// 3. onBlockActivated 检查 heldItem==Items::SHEARS，剪刀匹配 → 雕刻。useItemOnBlock 调 onBlockActivated
//   前把 stack 设到主手选中槽（SimulatedPlayer.cpp:287），getHeldItem 读到剪刀。
// 4. 判定原位 block.typeId：pumpkin → carved_pumpkin（方块类型转换）。不判定朝向（取决于玩家 yaw，
//   非本组聚焦）也不判定种子掉落（掉落物实体非确定）。
// 5. useItemOnBlock 传 Direction.Up，onBlockActivated 走 yaw 分支计算 facing（SimulatedPlayer 默认 yaw
//   决定朝向），但朝向不影响 carved_pumpkin 类型判定。
//
// 雕刻南瓜朝向：场景 2/3 已测。PumpkinBlock.onBlockActivated（MelonPumpkinBlocks.cpp:117-135，
//   对齐 vanilla PumpkinBlock.java:42-43）：Y 轴点击 facing=opposite(玩家水平朝向)=vanilla
//   player.getDirection().getOpposite()；水平点击 facing=hit.face=vanilla direction。lookAtLocation
//   控制 yaw（horizontalDirection 仅 yaw，lookAt.y=playerPos.y+1 使 pitch≈0 不影响 facing）。
// 不测「掉落4南瓜种子」：掉落物实体生成非确定（位置/时序），本组聚焦方块类型转换，跳过。
//   TODO: 待掉落物断言 API 完善后补南瓜种子数量测试。
// 不测「雕刻南瓜生成生物」：需铁傀儡/雪傀儡/凋灵结构 + 实体生成，复杂且非本组聚焦，跳过。
// 不测「非剪刀物品右键南瓜无反应」：onBlockActivated 非剪刀返 Pass → fallback 物品放置（如放方块），
//   语义弱，跳过。
//
// 跨服务端：南瓜 pumpkin / 雕刻南瓜 carved_pumpkin 方块名两端一致，剪刀雕刻 pumpkin→carved_pumpkin
//   行为与 vanilla 一致。基岩无 setBlockWithStates 需求（本组用 setBlockType 放默认 pumpkin），
//   两端均可放；剪刀雕刻行为两端可对比（BE 掉1种子 vs JE 掉4种子，但本组只断言方块转换不断言种子数）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_南瓜.txt#雕刻（剪刀使南瓜变雕刻南瓜，掉4南瓜种子）
// Ref: MelonPumpkinBlocks.cpp（PumpkinBlock::onBlockActivated 剪刀→setBlockState carved+掉种子→Success）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，调 onBlockActivated 前设主手选中槽）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 南瓜 (3,2,1)，完整方块无需支撑。

// 读取 (x,y,z) 方块 typeId。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 雕刻南瓜 facing state（小写方向字符串：north/south/east/west）。返回 null 表示失败或非雕刻南瓜。
// CarvedPumpkinBlock 用 HORIZONTAL_FACING()，C++ 属性名 "facing"。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 顶面点击雕刻朝向映射表（点击南瓜顶面 Up，Y 轴分支 facing=opposite(玩家水平朝向)，同 furnace）。
// PumpkinBlock.onBlockActivated（MelonPumpkinBlocks.cpp:117-135，对齐 vanilla PumpkinBlock.java:42-43）：
//   Y 轴点击 → facing=opposite(玩家水平朝向)；水平点击 → facing=hit.face（点击面本身）。
// playerFacing 由 yaw 映射 4 向（同 horizontalDirection）。lookAtLocation yaw=atan2(-dx,dz)。
// horizontalDirection/playerFacing 仅读 yaw，故 lookAt.y=playerPos.y+1 使 pitch≈0（不影响 facing）。
interface CarveFacingCase {
    name: string; // 玩家水平朝向名
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（不与南瓜 (3,2,1) 重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（产生目标 yaw）
    expectedFacing: string; // 雕刻南瓜 facing=opposite(玩家水平朝向)
}

// 4 朝向推算（playerPos.y=2，lookAt.y=3 使 pitch≈0；Y 轴 facing=opposite(玩家朝向)）：
//   South（yaw[315,360)∪[0,45)→facing=North）：玩家(1,2,1) lookAt(3,3,6)→338°→South→North。
//   West（yaw[45,135)→facing=East）：玩家(5,2,1) lookAt(0,3,1)→90°→West→East。
//   North（yaw[135,225)→facing=South）：玩家(1,2,5) lookAt(3,3,0)→158°→North→South。
//   East（yaw[225,315)→facing=West）：玩家(1,2,1) lookAt(6,3,1)→270°→East→West。
const CARVE_FACING_CASES: CarveFacingCase[] = [
    { name: "south", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 3, y: 3, z: 6 }, expectedFacing: "north" },
    { name: "west", playerPos: { x: 5, y: 2, z: 1 }, lookAt: { x: 0, y: 3, z: 1 }, expectedFacing: "east" },
    { name: "north", playerPos: { x: 1, y: 2, z: 5 }, lookAt: { x: 3, y: 3, z: 0 }, expectedFacing: "south" },
    { name: "east", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 6, y: 3, z: 1 }, expectedFacing: "west" },
];

// 场景 1：剪刀雕刻南瓜——放南瓜 + 剪刀 useItemOnBlock → 原位 (3,2,1) 变 carved_pumpkin，返 true。
//
// 布局：(3,2,1) 南瓜（minecraft:pumpkin，默认完整方块）。
// onBlockActivated：手持剪刀 == Items::SHEARS + m_carvedPumpkin 非空 → 计算 carved 朝向（hit.face=Up
//   走 yaw 分支）+ 雕刻音效 + setBlockState(pos, carved_pumpkin, 11) + 掉4南瓜种子 → Success。
//
// 判定：useItemOnBlock 返 true（Success），原位 (3,2,1) typeId === "minecraft:carved_pumpkin"
//   （南瓜经剪刀雕刻变为雕刻南瓜）。
function pumpkinCarvesWithShears(test: Test): void {
    test.setBlockType("minecraft:pumpkin", { x: 3, y: 2, z: 1 }); // 南瓜
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:pumpkin", `pumpkin should be at (3,2,1) before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const shears = new ItemStack("minecraft:shears", 1);

    // 对南瓜 useItemOnBlock 剪刀 → onBlockActivated 剪刀 → setBlockState carved_pumpkin + 掉种子 → Success。
    const used = farmer.useItemOnBlock(
        shears as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when carving pumpkin with shears");

    // 判定：原位 (3,2,1) 变 carved_pumpkin（南瓜经剪刀雕刻变为雕刻南瓜）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:carved_pumpkin", `carved pumpkin should be at (3,2,1) after carving, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：顶面点击雕刻朝向=opposite(玩家朝向)——剪刀点击南瓜顶面 Up（4 朝向）→ facing=opposite(玩家水平朝向)。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设朝向 → 放南瓜 (3,2,1) → 手持剪刀
//   useItemOnBlock 点击 (3,2,1) 顶面 Up → onBlockActivated Y 轴分支：facing=opposite(玩家水平朝向) →
//   setBlockState(pos, carved_pumpkin facing=expectedFacing, 11)。断言 facing=expectedFacing。
//
// 判定：4 朝向后 (3,2,1) typeId==="minecraft:carved_pumpkin" 且 facing===expectedFacing
//   （South→north, West→east, North→south, East→west，facing=opposite(玩家朝向)）。
//
// 此场景补 PumpkinTests 此前 TODO（雕刻南瓜朝向）：验证 wiki「剪刀雕刻南瓜」+ PumpkinBlock.onBlockActivated
//   Y 轴分支 facing=opposite(玩家水平朝向)（对齐 vanilla PumpkinBlock.java:42-43 的
//   player.getDirection().getOpposite()）。每朝向用新 player 避免 yaw 残留；每次重放南瓜避免残留。
function pumpkinCarveFacingOppositePlayerDirection(test: Test): void {
    for (const c of CARVE_FACING_CASES) {
        // 重放南瓜 (3,2,1)（每朝向独立放，避免上一朝向 carved_pumpkin 残留）。
        test.setBlockType("minecraft:pumpkin", { x: 3, y: 2, z: 1 });
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:pumpkin", `pumpkin should be at (3,2,1) before carving ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);

        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设朝向：yaw → 玩家水平朝向 → 雕刻 facing=opposite(朝向)。
        player.lookAtLocation(c.lookAt);

        // 手持剪刀点击 (3,2,1) 顶面 Up → onBlockActivated Y 轴分支 facing=opposite(玩家水平朝向)。
        const shears = new ItemStack("minecraft:shears", 1);
        const used = player.useItemOnBlock(
            shears as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 2, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when carving pumpkin facing ${c.expectedFacing} (player facing ${c.name})`);

        // 断言 (3,2,1) 变 carved_pumpkin 且 facing=expectedFacing（opposite(玩家水平朝向)）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:carved_pumpkin", `carved pumpkin should be at (3,2,1) for facing ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        const facing = getFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `carved pumpkin facing should be ${c.expectedFacing} (opposite of player facing ${c.name}), got ${facing}`);
    }

    test.succeed();
}

// 场景 3：侧面点击雕刻朝向=点击面——剪刀点击南瓜南面 South → facing=south（点击面本身）。
//
// 布局：(3,2,1) 南瓜。玩家 (3,2,3) 朝北 lookAtLocation({3,3,2})（朝南瓜方向，yaw 不影响水平分支）。
//   手持剪刀 useItemOnBlock 点击 (3,2,1) 南面 South（face=South）→ onBlockActivated 水平分支：
//   facing=hit.face=South → setBlockState(pos, carved_pumpkin facing=south, 11)。
//
// 判定：(3,2,1) typeId==="minecraft:carved_pumpkin" 且 facing==="south"（点击面本身，非 opposite）。
//
// 此场景验证 PumpkinBlock.onBlockActivated 水平分支 facing=hit.face（点击面本身，对齐 vanilla
//   PumpkinBlock.java:43 的 direction1=direction）。与 Y 轴分支 facing=opposite(朝向) 对照——
//   水平点击朝向取点击面，垂直点击朝向取玩家朝向反方向。
function pumpkinCarveFacingEqualsClickedFace(test: Test): void {
    test.setBlockType("minecraft:pumpkin", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:pumpkin", `pumpkin should be at (3,2,1) before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const player = test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "p_side");
    // 朝北看南瓜（yaw 不影响水平分支 facing=hit.face，仅自然朝向）。
    player.lookAtLocation({ x: 3, y: 3, z: 2 });

    // 手持剪刀点击 (3,2,1) 南面 South → onBlockActivated 水平分支 facing=hit.face=South。
    const shears = new ItemStack("minecraft:shears", 1);
    const used = player.useItemOnBlock(
        shears as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.South,
    );
    test.assert(used, "useItemOnBlock should return true when carving pumpkin on south face");

    // 断言 (3,2,1) 变 carved_pumpkin 且 facing=south（点击面本身，非 opposite）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:carved_pumpkin", `carved pumpkin should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getFacing(test, 3, 2, 1) === "south", `carved pumpkin facing should be south (clicked face), got ${getFacing(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerPumpkinTests(): void {
    GameTest.register("BlockBehaviorTests", "pumpkin_carves_with_shears", pumpkinCarvesWithShears)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "pumpkin_carve_facing_opposite_player_direction", pumpkinCarveFacingOppositePlayerDirection)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "pumpkin_carve_facing_equals_clicked_face", pumpkinCarveFacingEqualsClickedFace)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
