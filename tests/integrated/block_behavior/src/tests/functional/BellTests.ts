// 钟（bell）放置附着方式与红石激活行为 GameTest。
//
// wiki tech_钟.txt：钟可挂在地面（floor）、天花板（ceiling）或墙面上（single_wall/double_wall）。
//   - 放置时附着方式（ATTACHMENT state）由玩家点击的面 + 周围支撑决定：点击顶面→floor，底面→ceiling，
//     侧面→wall（两侧都有墙时 double_wall，否则 single_wall）。侧面墙的朝向（HORIZONTAL_FACING）取
//     点击面的反方向（朝外）。
//   - 钟可被右键侧面敲响、被红石信号激活敲响。红石激活时 POWERED state 翻 true（同步），断电翻回 false。
//     注：右键敲钟不改 POWERED（与 vanilla 一致，POWERED 仅由红石翻转），故本测试不通过 POWERED 断言敲钟。
//   - 钟于 1.14 加入，1.21.11 已包含，属 vanilla 正式特性。
//
// C++ 链路：BellBlock（functional/BellBlock.cpp）继承 Block（手动 hasBlockEntity），三个 state：
//   - horizontal_facing（HORIZONTAL_FACING，默认 North）：钟朝向。
//   - attachment（BELL_ATTACHMENT，默认 Floor）：附着方式 floor/ceiling/single_wall/double_wall。
//   - powered（POWERED，默认 false）：红石激活标志。
//   - getStateForPlacement（:256-328，对齐 MC 1.21.11 BellBlock.getStateForPlacement，用 placementPos 正确）：
//     · Y 轴点击（Up→Floor/Down→Ceiling）：facing = context.horizontalDirection()（玩家水平朝向）。
//     · 侧面点击：facing = opposite(clickedFace)（朝外）。attachment：Z/X 轴两侧都有固体面 → double_wall，
//       否则 single_wall。canSurvive 查支撑墙（clickedFace 反方向）有朝向钟的固体面。
//   - neighborChanged（:482-506，对齐 vanilla）：RedstonePower::isPowered(world, pos) 查 6 向电源。
//     shouldPower != isPowered 时，shouldPower 则 attemptToRing + POWERED 翻 true，否则翻 false。同步 setBlockState。
//   - onBlockActivated（:445-464）：服务端调 onHit（敲钟），不改 POWERED，只触发 BlockEntity 摆动 + 声音 +
//     gameEvent。敲钟返回 Success（短路）。本测试放置时被点击方块是 stone 非 bell，不触发敲钟。
//   - BellBlock 不 override isValidPosition（基类默认 true），放置不被 canPlace 拦截。
//
// 派发链路：SimulatedPlayer::useItemOnBlock(stack, blockLocation, face, faceLocation)（SimulatedPlayer.cpp
//   :265-346）。手持 bell 物品（BlockItem，Items.cpp:2434 registerBlockBackedItem）。点击 stone 侧面：
//   stone onBlockActivated 基类返 Pass → fallback Item.useOn → BlockItem::tryPlace → getStateForPlacement
//   → setBlockState(placementPos)。placementPos = 被点击方块.relative(face)。
//   红石场景用 setBlockWithStates 预置钟（不走放置链路，直接写 state），用 test.pullLever 触发拉杆通知。
//
// 测试覆盖（5 个场景，覆盖 wiki 放置附着方式 + 红石激活 + Floor/Ceiling 朝向核心确定行为）：
//   1. SingleWall 放置：点击 stone 南面（一侧支撑）→ 钟 attachment=single_wall, facing=north。
//   2. DoubleWall 放置：两侧 stone，点击一侧南面 → 钟 attachment=double_wall, facing=north。
//   3. 红石激活 POWERED 翻转：预置钟 + 拉杆拨动 → 钟 powered 翻 true；再拨 → 翻回 false（双稳态）。
//   4. Floor 放置 facing=玩家朝向：点击 stone 顶面 Up（4 朝向）→ 钟 attachment=floor, facing=玩家朝向（同向非 opposite）。
//   5. Ceiling 放置 facing=玩家朝向：点击 stone 底面 Down → 钟 attachment=ceiling, facing=玩家朝向（同向）。
//
// 关键约束：
// 1. 侧面墙放置被点击方块必须是 stone 等非钟方块（钟的 onBlockActivated 会敲钟短路不放置）。本测试
//    被点击方块用 stone，BellBlock::onBlockActivated 不被触发（stone 基类返 Pass），fallback 放置。
// 2. 场景 1/2 点击 (3,2,1) stone 的 South 面（face=South），新钟落 (3,2,2)（placementPos=(3,2,1).relative(South)）。
//    getStateForPlacement 侧面分支：clickedFace=South, axis=Z, supportDir=opposite(South)=North,
//    facing=North。isDoubleWall 查 Z 轴 (3,2,2).North=(3,2,1) 和 (3,2,2).South=(3,2,3)：
//    - 场景 1：(3,2,3) air → SingleWall。
//    - 场景 2：(3,2,3) stone → DoubleWall。
//    两场景同点击面、同落点、同 facing=north，attachment 因对侧支撑不同而异——对照验证双面墙判定。
// 3. 场景 1 (3,2,3) 须为 air（glass_pit 内部默认 air，不放东西即 air）。若误放 stone 会变 DoubleWall。
// 4. 场景 3 红石：钟 (3,2,2) 用 setBlockWithStates 预置 single_wall, facing=north, powered=false
//    （附着 South 侧 (3,2,3) stone 墙）。拉杆 (4,2,2) Wall+West（facing=West → outputDir=West=(3,2,2) 钟），
//    支撑在 East 侧 (5,2,2) stone。pullLever → LeverBlock::_notifyNeighbors 通知 outputDir=(3,2,2) 钟 →
//    BellBlock::neighborChanged → isPowered 查 6 向（拉杆 (4,2,2) East 侧对钟 strong 15）=true → POWERED 翻 true。
//    再 pullLever → isPowered=false → POWERED 翻回 false（双稳态，对齐拉杆无自动复位）。
// 5. BellBlock::neighborChanged 同步 setBlockState，pullLever 返回后即可读 powered。用 pollUntilSucceed 留余量。
// 6. 读 state 用 getState：horizontal_facing 返小写方向字符串（"north" 等），attachment 返小写下划线枚举名
//    （"single_wall"/"double_wall"），powered 返 boolean。用 as any 绕过 BlockStateSuperset 白名单。
// 7. 拉杆须先放支撑 (5,2,2) stone 再放拉杆，避免拉杆 neighborChanged 检测支撑缺失掉落（同 LeverTests 范式）。
//
// 不测「右键敲钟」：敲钟不改 POWERED（与 vanilla 一致），useItemOnBlock 返 bool 无法区分敲钟 Success 与
//   放置 fallback Success，摆动动画是 BlockEntity 内部状态脚本侧不可读。TODO: 待 BlockEntity 状态可读后补。
// Floor/Ceiling 放置：场景 4/5 已测（lookAtLocation 控制玩家 yaw → horizontalDirection → facing=玩家朝向）。
//   BellBlock Y 轴分支 facing=horizontalDirection（玩家朝向同向），与侧面分支 facing=opposite(clickedFace) 不同，
//   也与 furnace/barrel 的 facing=opposite(朝向) 不同——这是 BellBlock Y 轴用 horizontalDirection 的特点。
// 不测「钟回响/村民警报」：涉实体搜索/标签/AI，非确定，跳过。
// 不测「投射物击中敲钟」：需投射物实体 + 碰撞检测，非确定时序，跳过。
//
// 跨服务端：bell 方块名两端一致（minecraft:bell），horizontal_facing/attachment/powered state 名两端一致。
//   放置附着方式（single_wall/double_wall 判定）+ 红石 POWERED 翻转行为与 vanilla 一致。setBlockWithStates
//   预置钟是 Cubium 专有写入（基岩侧用物品放置），但附着判定 + 红石行为本身两端可对比。
//   注：Cubium getStateForPlacement 在无支撑时返 defaultState（会放后掉落），vanilla 返 null（不放）——本测试
//   所有场景都有合法支撑，不受此偏差影响。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_钟.txt（floor/ceiling/wall 放置，红石激活敲响）
// Ref: BellBlock.cpp（getStateForPlacement 侧面分支：double_wall 两侧支撑判定，facing=opposite(clickedFace)）
// Ref: BellBlock.cpp（neighborChanged：isPowered 查 6 向，shouldPower 翻 POWERED，同步 setBlockState）
// Ref: LeverBlock.cpp（_notifyNeighbors Wall 模式通知 outputDir=facing + support=opposite(facing) 两格）
// Ref: LeverTests.ts（拉杆拨动 + 红石通知范式：pullLever + 支撑先于拉杆放置）
// Ref: FlowerPotTests.ts / EndRodTests.ts（useItemOnBlock 放置范式：new ItemStack + cast + face 参数）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1/2：钟 (3,2,2)，被点击 stone (3,2,1)（钟 North 侧），对侧 (3,2,3)（场景1 air / 场景2 stone）。
// 场景 3：钟 (3,2,2) 预置，拉杆 (4,2,2) Wall+West，拉杆支撑 (5,2,2) stone，钟附着墙 (3,2,3) stone。

// setBlockWithStates 的 TS 侧访问器（Test 未在类型中暴露 setBlockWithStates，用 cast 访问，同 TripWireHookTests/EndRodTests）。
type TestWithStates = Test & {
    setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
};

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取钟 attachment state（小写下划线枚举名：floor/ceiling/single_wall/double_wall）。返回 null 表示失败或非钟。
function getAttachment(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("attachment" as any);
    return typeof value === "string" ? value : null;
}

// 读取钟 facing state（小写方向字符串：north/south/east/west）。返回 null 表示失败或非钟。
// 注意：BellBlock 用 HORIZONTAL_FACING()，其 C++ 属性名为 "facing"（非 "horizontal_facing"），
// getState 按 entry.property->name() 匹配 C++ 内部名，故读 "facing"。
function getFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 读取钟 powered state（bool）。返回 null 表示失败或非钟。
function getPowered(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("powered" as any);
    return typeof value === "boolean" ? value : null;
}

// Floor/Ceiling 放置朝向映射表（Y 轴点击，facing=horizontalDirection(玩家朝向)，非 opposite）。
// BellBlock.getStateForPlacement Y 轴分支（BellBlock.cpp:270-287，对齐 vanilla BellBlock.java:178-179）：
//   facing = context.horizontalDirection()（玩家水平朝向本身，非 opposite）。
//   attachment = (clickedFace==Up)?Floor:Ceiling。
// horizontalDirection（BlockItemUseContext.cpp:111-125）：yaw∈[315,360)∪[0,45)→South，[45,135)→West，
//   [135,225)→North，[225,315)→East。lookAtLocation yaw=atan2(-dx,dz)（SimulatedPlayer.cpp:99）。
// 注：horizontalDirection 仅读 yaw 不读 pitch，故 lookAt.y 用 furnace 范式（=playerPos.y 即可，pitch 任意）。
//   但为与 barrel 范式一致并避免边缘场景，仍取 lookAt.y=playerPos.y+1 使 pitch≈0（不影响 horizontalDirection）。
interface FloorFacingCase {
    name: string; // 玩家水平朝向名（lookAt 产生的 horizontalDirection）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（不与 stone/钟落点重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（产生目标 yaw）
    expectedFacing: string; // 钟 facing=horizontalDirection(玩家朝向)，同向非 opposite
}

// 4 朝向推算（playerPos.y=2，lookAt.y=3 使 pitch≈0；horizontalDirection 仅 yaw 故 pitch 不影响）：
//   South（yaw[315,360)∪[0,45)→facing=South）：玩家(1,2,1)，lookAt(3,3,6)，dx=2,dz=5→338°→South→facing=South。
//   West（yaw[45,135)→facing=West）：玩家(5,2,1)，lookAt(0,3,1)，dx=-5,dz=0→90°→West→facing=West。
//   North（yaw[135,225)→facing=North）：玩家(1,2,5)，lookAt(3,3,0)，dx=2,dz=-5→158°→North→facing=North。
//   East（yaw[225,315)→facing=East）：玩家(1,2,1)，lookAt(6,3,1)，dx=5,dz=0→270°→East→facing=East。
// 【关键】facing=玩家朝向（同向），与 furnace/barrel 的 facing=opposite(朝向) 相反——这是 BellBlock Y 轴分支
//   用 horizontalDirection（非 getNearestLookingDirection().getOpposite()）的特点。
const FLOOR_FACING_CASES: FloorFacingCase[] = [
    { name: "south", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 3, y: 3, z: 6 }, expectedFacing: "south" },
    { name: "west", playerPos: { x: 5, y: 2, z: 1 }, lookAt: { x: 0, y: 3, z: 1 }, expectedFacing: "west" },
    { name: "north", playerPos: { x: 1, y: 2, z: 5 }, lookAt: { x: 3, y: 3, z: 0 }, expectedFacing: "north" },
    { name: "east", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 6, y: 3, z: 1 }, expectedFacing: "east" },
];

// 场景 1：SingleWall 放置——点击 stone 南面（一侧支撑）→ 钟 attachment=single_wall, facing=north。
//
// 布局：(3,2,1) 放 stone（被点击方块，钟 North 侧支撑）。手持 bell useItemOnBlock 点击 (3,2,1) 南面
//   （face=South），新钟落 (3,2,2)。(3,2,3)（钟 South 侧）保持 air。
// getStateForPlacement：clickedFace=South, axis=Z, supportDir=opposite(South)=North, facing=North。
//   isDoubleWall 查 Z 轴：(3,2,2).North=(3,2,1) stone solid ✓ + (3,2,2).South=(3,2,3) air ✗ → 非 double → SingleWall。
//   canSurvive 查 supportDir=North 方向 (3,2,1) 的 South 面（clickedFace）solid ✓ → 返 single_wall, facing=north。
//
// 判定：(3,2,2) typeId === "minecraft:bell" 且 attachment==="single_wall" 且 horizontal_facing==="north"。
function bellSingleWallWhenPlacedOnOneSidedWall(test: Test): void {
    // (3,2,1) 放 stone（被点击方块，钟 North 侧单面墙支撑）。对侧 (3,2,3) 保持 air。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:stone", `stone should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 2 }, "farmer");
    const bell = new ItemStack("minecraft:bell", 1);

    // 手持 bell 点击 (3,2,1) 南面 South → 新钟落 (3,2,2)。stone onBlockActivated Pass → fallback 放置。
    // getStateForPlacement 侧面分支：对侧 (3,2,3) air → SingleWall, facing=opposite(South)=North。
    const used = farmer.useItemOnBlock(
        bell as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.South,
    );
    test.assert(used, "useItemOnBlock should return true when placing bell on one-sided wall");

    // 判定：新钟 (3,2,2) 是 bell，attachment=single_wall（一侧支撑），facing=north（朝外，opposite(South)）。
    test.assert(getBlockTypeId(test, 3, 2, 2) === "minecraft:bell", `new bell should be at (3,2,2), got ${getBlockTypeId(test, 3, 2, 2)}`);
    test.assert(getAttachment(test, 3, 2, 2) === "single_wall", `bell attachment should be single_wall (one-sided), got ${getAttachment(test, 3, 2, 2)}`);
    test.assert(getFacing(test, 3, 2, 2) === "north", `bell facing should be north (opposite of South face), got ${getFacing(test, 3, 2, 2)}`);

    test.succeed();
}

// 场景 2：DoubleWall 放置——两侧 stone，点击一侧南面 → 钟 attachment=double_wall, facing=north。
//
// 布局：(3,2,1) stone（被点击方块，钟 North 侧）+ (3,2,3) stone（钟 South 侧，对侧支撑）。手持 bell
//   useItemOnBlock 点击 (3,2,1) 南面（face=South），新钟落 (3,2,2)。
// getStateForPlacement：clickedFace=South, axis=Z, supportDir=North, facing=North。
//   isDoubleWall 查 Z 轴：(3,2,2).North=(3,2,1) stone solid ✓ + (3,2,2).South=(3,2,3) stone solid ✓ → DoubleWall。
//   返 double_wall, facing=north。
//
// 判定：(3,2,2) typeId === "minecraft:bell" 且 attachment==="double_wall" 且 horizontal_facing==="north"。
//
// 对照场景 1：同点击面 South、同落点 (3,2,2)、同 facing=north，仅对侧 (3,2,3) 由 air 变 stone →
//   attachment 从 single_wall 变 double_wall，验证双面墙两侧支撑判定。
function bellDoubleWallWhenPlacedOnTwoSidedWall(test: Test): void {
    // (3,2,1) stone（被点击方块）+ (3,2,3) stone（对侧支撑，构成双面墙）。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 3 });
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:stone", `stone should be at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getBlockTypeId(test, 3, 2, 3) === "minecraft:stone", `stone should be at (3,2,3), got ${getBlockTypeId(test, 3, 2, 3)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 2 }, "farmer");
    const bell = new ItemStack("minecraft:bell", 1);

    // 手持 bell 点击 (3,2,1) 南面 South → 新钟落 (3,2,2)。两侧 stone → DoubleWall, facing=north。
    const used = farmer.useItemOnBlock(
        bell as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.South,
    );
    test.assert(used, "useItemOnBlock should return true when placing bell on two-sided wall");

    // 判定：新钟 (3,2,2) 是 bell，attachment=double_wall（两侧支撑），facing=north。
    test.assert(getBlockTypeId(test, 3, 2, 2) === "minecraft:bell", `new bell should be at (3,2,2), got ${getBlockTypeId(test, 3, 2, 2)}`);
    test.assert(getAttachment(test, 3, 2, 2) === "double_wall", `bell attachment should be double_wall (two-sided), got ${getAttachment(test, 3, 2, 2)}`);
    test.assert(getFacing(test, 3, 2, 2) === "north", `bell facing should be north (opposite of South face), got ${getFacing(test, 3, 2, 2)}`);

    test.succeed();
}

// 场景 3：红石激活 POWERED 翻转——预置钟 + 拉杆拨动 → 钟 powered 翻 true；再拨 → 翻回 false。
//
// 布局：钟 (3,2,2) 预置 single_wall, facing=north, powered=false（附着 South 侧 (3,2,3) stone 墙）。
//   拉杆 (4,2,2) Wall+West（facing=West → outputDir=West=(3,2,2) 钟），支撑在 East 侧 (5,2,2) stone。
//   pullLever((4,2,2)) → LeverBlock::toggle 翻转拉杆 POWERED + _notifyNeighbors 通知 outputDir=(3,2,2) 钟。
//   BellBlock::neighborChanged → isPowered 查 6 向（拉杆 (4,2,2) 在钟 East 侧，对钟 strong 15）=true →
//   POWERED 翻 true。再 pullLever → 拉杆断电 → 通知钟 → isPowered=false → POWERED 翻回 false（双稳态）。
//
// 判定：pollUntilSucceed 轮询钟 (3,2,2) powered===true（首次拨动）。runAtTickTime 二次拨动后轮询 powered===false。
function bellPowersWhenLeverPulled(test: Test): void {
    // 钟附着墙 (3,2,3) stone（钟 South 侧 single_wall 支撑，facing=north 附着 South）。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 3 });

    // 钟拉杆支撑 (5,2,2) stone（拉杆 East 侧，Wall+West 的 support=opposite(West)=East）。
    test.setBlockType("minecraft:stone", { x: 5, y: 2, z: 2 });

    // 预置钟 (3,2,2) single_wall, facing=north, powered=false（不走放置链路，直接写 state）。
    // 注意：facing 的 C++ 属性名是 "facing"（HORIZONTAL_FACING 创建名为 "facing" 的属性），
    // setBlockWithStates 按 C++ 内部名查属性，传错名会静默忽略（保持默认值）。
    (test as TestWithStates).setBlockWithStates("minecraft:bell", { x: 3, y: 2, z: 2 }, "attachment=single_wall,facing=north,powered=false");
    test.assert(getBlockTypeId(test, 3, 2, 2) === "minecraft:bell", `bell should be at (3,2,2), got ${getBlockTypeId(test, 3, 2, 2)}`);
    test.assert(getPowered(test, 3, 2, 2) === false, `bell should be unpowered before lever pull, got ${getPowered(test, 3, 2, 2)}`);

    // 拉杆 (4,2,2) Wall+West（facing=West → outputDir=West=(3,2,2) 钟）。先放支撑 (5,2,2) stone（已放）再放拉杆。
    (test as TestWithStates).setBlockWithStates("minecraft:lever", { x: 4, y: 2, z: 2 }, "facing=west,face=wall,powered=false");
    test.assert(getBlockTypeId(test, 4, 2, 2) === "minecraft:lever", `lever should be at (4,2,2), got ${getBlockTypeId(test, 4, 2, 2)}`);

    // pullLever 同步拨动拉杆 → 拉杆 POWERED=true + _notifyNeighbors 通知 outputDir=(3,2,2) 钟 →
    // 钟邻居 (4,2,2) 拉杆强信号 → isPowered=true → POWERED 翻 true。
    test.pullLever({ x: 4, y: 2, z: 2 });

    // 轮询断言钟 (3,2,2) powered===true（拉杆激活，红石同步翻转）。
    pollUntilSucceed(
        test,
        () => getPowered(test, 3, 2, 2) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(false, `bell powered should be true after lever pull, got ${getPowered(test, 3, 2, 2)} | leverType=${getBlockTypeId(test, 4, 2, 2)}`);
            },
        },
    );

    // 等首次激活稳定后二次拨动拉杆（断电，双稳态翻转）。
    test.runAtTickTime(6, () => {
        if (getPowered(test, 3, 2, 2) !== true) {
            test.assert(false, `bell should be powered before second lever pull, got ${getPowered(test, 3, 2, 2)}`);
            return;
        }
        // 二次 pullLever → 拉杆 POWERED=false + 通知钟 → isPowered=false → POWERED 翻回 false。
        test.pullLever({ x: 4, y: 2, z: 2 });
    });

    // 轮询断言钟 (3,2,2) powered===false（拉杆断电，双稳态翻回）。
    pollUntilSucceed(
        test,
        () => getPowered(test, 3, 2, 2) === false,
        {
            startTick: 12,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `bell powered should be false after second lever pull, got ${getPowered(test, 3, 2, 2)}`);
            },
        },
    );
}

// 场景 4：Floor 放置 facing=玩家水平朝向——点击 stone 顶面 Up → 钟 attachment=floor, facing=玩家朝向（同向）。
//
// 布局：每朝向独立 spawn 玩家在 playerPos → lookAtLocation(lookAt) 设朝向 → 清理 (3,2,1) → 手持 bell
//   useItemOnBlock 点击 (3,1,1) stone 顶面 Up → placementPos=(3,2,1)（stone 上方）→
//   getStateForPlacement Y 轴分支：clickedFace=Up→Floor，facing=horizontalDirection(玩家朝向) →
//   canSurvive hasEnoughSolidSide(world, (3,1,1), Up)=stone solid ✓ → 返 floor, facing=玩家朝向。
//
// 判定：4 朝向放置后 (3,2,1) attachment===floor 且 facing===expectedFacing（玩家朝向，同向非 opposite）。
//
// 此场景补 BellTests 此前 TODO（bell_floor_ceiling_facing）：验证 wiki「钟放上表面（floor）」+
//   BellBlock Y 轴分支 facing=horizontalDirection（玩家朝向本身）。与 furnace/barrel 的 facing=opposite(朝向)
//   相反——BellBlock Y 轴用 horizontalDirection 而非 getNearestLookingDirection().getOpposite()。
//   每朝向用新 player 避免 yaw 残留；每次清理 (3,2,1) 避免钟残留阻断放置。
function bellFloorFacingEqualsPlayerDirection(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FLOOR_FACING_CASES) {
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设朝向：yaw=atan2(-dx,dz) → horizontalDirection → 钟 facing=玩家朝向（同向）。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向钟残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 bell 点击 (3,1,1) stone 顶面 Up → 钟落 (3,2,1)。
        // getStateForPlacement Y 轴分支：clickedFace=Up→Floor, facing=horizontalDirection(玩家朝向)。
        const bell = new ItemStack("minecraft:bell", 1);
        const used = player.useItemOnBlock(
            bell as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing floor bell facing ${c.expectedFacing} (player facing ${c.name})`);

        // 断言钟 (3,2,1) 已放置且 attachment=floor, facing=玩家朝向（同向非 opposite）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:bell", `bell should be placed at (3,2,1) for facing ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        test.assert(getAttachment(test, 3, 2, 1) === "floor", `bell attachment should be floor, got ${getAttachment(test, 3, 2, 1)}`);
        const facing = getFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `bell facing should be ${c.expectedFacing} (player direction, not opposite), got ${facing}`);
    }

    test.succeed();
}

// 场景 5：Ceiling 放置 facing=玩家水平朝向——点击 stone 底面 Down → 钟 attachment=ceiling, facing=玩家朝向。
//
// 布局：(3,3,1) stone（被点击方块，钟在其下方）。玩家 (1,2,1) 朝南 lookAtLocation({3,3,6})（yaw≈338°→South）。
//   手持 bell useItemOnBlock 点击 (3,3,1) 底面 Down → placementPos=(3,2,1)（stone 下方）→
//   getStateForPlacement Y 轴分支：clickedFace=Down→Ceiling，facing=horizontalDirection(South) →
//   canSurvive canSupportCenter(world, (3,3,1), Down)=stone 中心支撑 ✓ → 返 ceiling, facing=south。
//
// 判定：(3,2,1) attachment===ceiling 且 facing===south（玩家朝向 South，同向）。
//
// 此场景验证 wiki「钟放底面下方（ceiling）」+ BellBlock Y 轴分支 Down→Ceiling，facing=horizontalDirection。
//   canSupportCenter 判定 stone 中心支撑（与 Floor 的 hasEnoughSolidSide 完整面判定不同，验证 ceiling 分支）。
//   仅测 1 朝向（South）聚焦 ceiling 分支判定 + facing 同向；4 朝向已在场景 4 floor 验证（ceiling 同逻辑）。
function bellCeilingFacingEqualsPlayerDirection(test: Test): void {
    // (3,3,1) stone（钟在其下方，被点击底面的方块，需中心支撑 canSupportCenter）。
    test.setBlockType("minecraft:stone", { x: 3, y: 3, z: 1 });
    test.assert(getBlockTypeId(test, 3, 3, 1) === "minecraft:stone", `stone should be at (3,3,1), got ${getBlockTypeId(test, 3, 3, 1)}`);
    // 清理钟落点 (3,2,1)。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "p_south");
    // 朝南：lookAt (3,3,6)，dx=2,dz=5→338°→South。
    player.lookAtLocation({ x: 3, y: 3, z: 6 });

    // 手持 bell 点击 (3,3,1) 底面 Down → 钟落 (3,2,1)（stone 下方）。
    // getStateForPlacement Y 轴分支：clickedFace=Down→Ceiling, facing=horizontalDirection(South)。
    const bell = new ItemStack("minecraft:bell", 1);
    const used = player.useItemOnBlock(
        bell as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 3, z: 1 },
        Direction.Down,
    );
    test.assert(used, "useItemOnBlock should return true when placing ceiling bell");

    // 断言钟 (3,2,1) 已放置且 attachment=ceiling, facing=south（玩家朝向，同向）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:bell", `bell should be placed at (3,2,1), got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getAttachment(test, 3, 2, 1) === "ceiling", `bell attachment should be ceiling, got ${getAttachment(test, 3, 2, 1)}`);
    test.assert(getFacing(test, 3, 2, 1) === "south", `bell facing should be south (player direction, not opposite), got ${getFacing(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerBellTests(): void {
    GameTest.register("BlockBehaviorTests", "bell_single_wall_when_placed_on_one_sided_wall", bellSingleWallWhenPlacedOnOneSidedWall)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "bell_double_wall_when_placed_on_two_sided_wall", bellDoubleWallWhenPlacedOnTwoSidedWall)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "bell_powers_when_lever_pulled", bellPowersWhenLeverPulled)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "bell_floor_facing_equals_player_direction", bellFloorFacingEqualsPlayerDirection)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "bell_ceiling_facing_equals_player_direction", bellCeilingFacingEqualsPlayerDirection)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
