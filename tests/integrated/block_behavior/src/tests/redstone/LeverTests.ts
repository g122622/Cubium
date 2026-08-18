// 拉杆红石双稳态开关行为 GameTest。
//
// wiki tech_拉杆.txt#红石元件：拉杆是双稳态红石电源——拨动后保持当前信号状态，再次拨动翻转。
//   - 拨动一次：POWERED 翻转（false→true 或 true→false），向输出方向 + 附着方块传递信号。
//   - 与按钮不同：拉杆无自动复位，拨开后持续供电直到再次拨动。
//   - 开启时为毗邻红石线/比较器/中继器提供 15 信号，激活毗邻机械元件，强充能附着导体。
//
// C++ 链路：LeverBlock（LeverBlock.cpp）有三个 state：
//   - HORIZONTAL_FACING（Direction，默认 North，输出/朝向方向）
//   - POWERED（bool，默认 false）
//   - ATTACH_FACE（AttachFace，默认 Wall）
//   - toggle（LeverBlock.cpp:180-193）：翻转 POWERED → setBlockState(flags=2 无邻居更新防循环) →
//     _playClickSound → _notifyNeighbors（手动通知输出方向 + 支撑方块两格）。
//   - _notifyNeighbors（:253-295）：Wall 模式 outputDir=facing，supportPos=pos.offset(opposite(facing))；
//     通知 outputDir 格 + supportPos 格的 neighborChanged（仅这两格，非全 6 向）。
//   - getWeakPower（:195-203）：isPowered?15:0 全向弱信号（基岩全向模型）。
//   - getStrongPower（:205-241）：仅 outputDir 方向输出 15 强信号（强充能附着导体）。
//   - getStateForPlacement（LeverBlock 重写）：朝向与附着面由玩家点击面决定（同按钮，附墙方块语义）：
//     点击顶面→face=floor、facing=玩家水平朝向；点击底面→ceiling、facing=玩家水平朝向；点击墙面→
//     wall、facing=点击面（水平四向）。此前未重写该方法，落回基类 Block::getStateForPlacement
//     返回 defaultState()（facing 恒 North、face 恒 Wall），与预期按点击面决定朝向/附着面的行为不一致。
//     重写后修正。
//
// 拨动入口：GameTestHelper::pullLever（GameTestHelper.cpp:442）已实现（委托 LeverBlock::toggle），
//   JS 侧 test.pullLever(pos)（ScriptTestHelper.cpp:341-358 绑定）。与 pressButton 同范式。
//   放置朝向走 useItemOnBlock 手持拉杆物品点击 stone 顶面/侧面 → BlockItem::tryPlace →
//   getStateForPlacement（facing/face 按点击面）→ setBlockState。
//
// 测试覆盖（5 个场景，覆盖 wiki 双稳态+持续供电+放置朝向核心行为，可跨服务端对比）：
//   1. 拨动开启：pullLever → POWERED 翻 true + 毗邻红石灯亮（充能输出方向）。
//   2. 再次拨动关闭：承接场景 1（powered=true），再 pullLever → POWERED 翻回 false + 灯灭（双稳态翻转）。
//   3. 持续供电无复位：拨开后多 tick 仍 powered=true + 灯亮（无自动复位，区别于按钮 20/30gt 弹起）。
//   4. Floor 4 朝向放置：点击 stone 顶面 Up → face=floor, facing=玩家水平朝向（4 朝向，复用
//      GrindstoneTests Y 轴坐标配方，facing=horizontalDirection 同向非 opposite）。
//   5. Wall 2 朝向放置（区分新旧实现）：点击 stone 侧面 → face=wall, facing=点击面（South/East 两轴，
//      复用 GrindstoneTests Wall 坐标配方）。旧实现落基类 defaultState（face 恒 wall、facing 恒 North），
//      新实现 face=wall、facing=点击面。
//
// 关键约束：
// 1. 拉杆 _notifyNeighbors 只通知 outputDir（Wall+North → North=z-1）和 support（South=z+1）两格，
//    非全 6 向。故被充能红石灯须放在 outputDir 或 support 位置之一才被通知 neighborChanged。
//    本测试红石灯放 outputDir（拉杆 North 侧 z-1），与拉杆水平相邻。
// 2. setBlockType 放拉杆用默认 state（Wall+North），支撑方块在拉杆 South 侧（z+1，facing North 的反方向）。
//    放置顺序：先放支撑 (3,2,3) stone，再放拉杆 (3,2,2)，避免拉杆 neighborChanged 检测支撑缺失而掉落。
// 3. 红石灯放 (3,2,1)（拉杆 North 侧 z-1 = outputDir），被 _notifyNeighbors 通知 → neighborChanged →
//    isBlockPowered（拉杆 weakPower 15）=true → lit=true。
// 4. 红石灯断电有 4tick 延迟（RedstoneLampBlock.cpp:113 scheduleBlockTick 4tick，对齐 wiki 防闪烁）。
//    场景 2 灯灭断言须留足余量（拨动后 + 灯延迟 4tick + 余量）。
// 5. 读 powered/lit state 用 getState("powered"/"lit" as any) 绕过 BlockStateSuperset 白名单。读 facing/face
//    state 用 getState("facing"/"face" as any)（HORIZONTAL_FACING 返小写方向，ATTACH_FACE 返 floor/wall/ceiling）。
// 6. 场景 3 用 runAtTickTime 在远期 tick（如 tick 40，远超按钮 30gt 弹起）断言仍 powered=true，证明无复位。
// 7. 场景 4/5 朝向控制（复用 GrindstoneTests/BellTests 含 pitch lookAtLocation 范式 + 水平 4 朝向坐标
//    配方）。Floor 分支 facing=horizontalDirection（玩家水平朝向，同向非 opposite，需 lookAt 控制 yaw）；
//    Wall 分支 facing=clickedFace（点击面，与玩家朝向无关）。每朝向独立 spawn 玩家避免 yaw 残留；每次清理
//    落点避免残留阻断放置。useItemOnBlock 第三参数 Direction=点击面（clickedFace），原样透传到
//    context.getClickedFace()，placementPos=被点击方块.offset(clickedFace)。此前 Cubium 未重写
//    getStateForPlacement（基类 defaultState，face 恒 wall、facing 恒 North），朝向放置全为默认值，断言
//    失败；重写后修正。
//
// 不测「Java 方向性强输出」：Cubium getStrongPower 已实现方向性，但测试聚焦双稳态+持续供电+放置朝向，
//   方向性强充能导体场景涉强充能链路复杂，跳过。TODO: 待强充能链路测试完善后补。
//
// 跨服务端：拉杆 powered/facing/face state 名两端一致，双稳态翻转 + 持续供电 + 放置朝向（face/facing 按
//   点击面）行为两端一致。朝向测试用 useItemOnBlock 放置（lookAtLocation 是 Cubium 专有朝向控制，但
//   face/facing 按点击面放置行为两端可对比，非 one-sided）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_拉杆.txt#红石元件（双稳态，拨动翻转，无自动复位）
// Ref: LeverBlock.cpp（toggle 翻转 POWERED+notifyNeighbors；getWeakPower 全向 15；getStateForPlacement 按点击面）
// Ref: GameTestHelper.cpp:442（pullLever 委托 LeverBlock::toggle）
// Ref: RedstoneLampBlock.cpp:113（断电 4tick 延迟，防闪烁）
// Ref: GrindstoneTests.ts（附墙方块 Y 轴 Floor/Wall 朝向测试范式 + 坐标配方，拉杆复用同构）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Direction 参数=clickedFace 原样透传 getClickedFace）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 拉杆 (3,2,2) Wall+North，支撑 (3,2,3) stone（拉杆 South 侧 z+1，须先于拉杆放置）。
// 红石灯 (3,2,1)（拉杆 North 侧 z-1 = outputDir，被 _notifyNeighbors 通知）。

// 读取拉杆 powered state（bool）。返回 null 表示读取失败或非拉杆。
function getLeverPowered(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("powered" as any);
    return typeof value === "boolean" ? value : null;
}

// 读取红石灯 lit state（bool）。返回 null 表示读取失败或非红石灯。
function getLampLit(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("lit" as any);
    return typeof value === "boolean" ? value : null;
}

// 读取 (x,y,z) 拉杆 facing state（方向名字符串 "north"/"south"/"east"/"west"）。返回 null 表示失败或非拉杆。
// HORIZONTAL_FACING() 的 C++ 属性名为 "facing"（水平四向，无 up/down）。
function getLeverFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 拉杆 face state（附着面字符串 "floor"/"wall"/"ceiling"）。返回 null 表示失败或非拉杆。
// ATTACH_FACE() 的 C++ 属性名为 "face"。
function getLeverFace(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("face" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// Floor 4 朝向放置映射表（复用 GrindstoneTests/BellTests 的 Y 轴坐标配方，拉杆 Floor 分支 facing=
// horizontalDirection 玩家水平朝向，同向非 opposite）。
// horizontalDirection=orderedByNearest(yaw,pitch)[0] 的水平分量，pitch≈0 时 = 玩家水平朝向。
// lookAtLocation yaw=atan2(-dx,dz)：0→South,90→West,180→North,270→East。
// 【关键】lookAt.y=playerPos.y+1 使 pitch≈0（horizontalDirection 仅读 yaw，但 pitch≈0 与现有范式一致稳妥）。
interface FloorFacingCase {
    name: string; // 玩家水平朝向名（lookAt 产生的 horizontalDirection）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（不与 stone/拉杆落点重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（y=playerPos.y+1 保证 pitch≈0）
    expectedFacing: string; // 拉杆 facing=玩家水平朝向（同向非 opposite）
}

// 4 朝向推算（playerPos.y=2→眼高3.62，lookAt.y=3→dy=-0.12→pitch≈1.3°，[0]=水平朝向，facing=水平朝向同向）：
//   South（yaw[315,360)∪[0,45)→facing=South）：玩家(1,2,1)，lookAt(3,3,6)，dx=2,dz=5→338°→South→facing=south。
//   West（yaw[45,135)→facing=West）：玩家(5,2,1)，lookAt(0,3,1)，dx=-5,dz=0→90°→West→facing=west。
//   North（yaw[135,225)→facing=North）：玩家(1,2,5)，lookAt(3,3,0)，dx=2,dz=-5→158°→North→facing=north。
//   East（yaw[225,315)→facing=East）：玩家(1,2,1)，lookAt(6,3,1)，dx=5,dz=0→270°→East→facing=east。
// 玩家位置均不与 (3,1,1)/(3,2,1) 重叠；lookAt 目标均在 [0,6] 内不越界。
const FLOOR_FACING_CASES: FloorFacingCase[] = [
    { name: "south", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 3, y: 3, z: 6 }, expectedFacing: "south" },
    { name: "west", playerPos: { x: 5, y: 2, z: 1 }, lookAt: { x: 0, y: 3, z: 1 }, expectedFacing: "west" },
    { name: "north", playerPos: { x: 1, y: 2, z: 5 }, lookAt: { x: 3, y: 3, z: 0 }, expectedFacing: "north" },
    { name: "east", playerPos: { x: 1, y: 2, z: 1 }, lookAt: { x: 6, y: 3, z: 1 }, expectedFacing: "east" },
];

// Wall 2 朝向放置映射表（复用 GrindstoneTests Wall 坐标配方，拉杆 Wall 分支 facing=clickedFace 点击面本身）。
// 每 case 独立 stone 位置 + 点击面 + 落点，玩家站远离落点处（避免实体碰撞阻断放置）。
interface WallFacingCase {
    name: string; // 点击面名（facing=该面）
    stonePos: { x: number; y: number; z: number }; // 被点击 stone 位置（也是 wall 背面支撑）
    clickedFace: Direction; // 点击面（facing=此面）
    leverPos: { x: number; y: number; z: number }; // 拉杆落点（stone.offset(clickedFace)）
    playerPos: { x: number; y: number; z: number }; // 玩家位置（远离落点，避免碰撞；朝向不影响 wall facing）
    expectedFacing: string; // 拉杆 facing=clickedFace
}

// 2 朝向（覆盖 Z 轴 South + X 轴 East，验证 wall 分支两轴判定）：
//   South（Z 轴）：stone(3,2,1) 点击 South → 拉杆落(3,2,2), facing=south。背面 opposite(south)=north=(3,2,1) stone ✓。
//   East（X 轴）：stone(1,2,2) 点击 East → 拉杆落(2,2,2), facing=east。背面 opposite(east)=west=(1,2,2) stone ✓。
// 玩家站远离落点：South case 玩家(3,2,4)（落点(3,2,2) 北侧 2 格外）；East case 玩家(5,2,2)（落点(2,2,2) 东侧 3 格外）。
const WALL_FACING_CASES: WallFacingCase[] = [
    {
        name: "south",
        stonePos: { x: 3, y: 2, z: 1 },
        clickedFace: Direction.South,
        leverPos: { x: 3, y: 2, z: 2 },
        playerPos: { x: 3, y: 2, z: 4 },
        expectedFacing: "south",
    },
    {
        name: "east",
        stonePos: { x: 1, y: 2, z: 2 },
        clickedFace: Direction.East,
        leverPos: { x: 2, y: 2, z: 2 },
        playerPos: { x: 5, y: 2, z: 2 },
        expectedFacing: "east",
    },
];

// 放支撑 + 拉杆 + 红石灯。
// 顺序：先放支撑 (3,2,3) stone（拉杆 South 侧），再放拉杆 (3,2,2)，再放红石灯 (3,2,1)。
// 先支撑后拉杆，避免拉杆 neighborChanged 检测支撑缺失而掉落。
function placeLeverSetup(test: Test, leverType: string): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 3 }); // 支撑（拉杆 South 侧 z+1）
    test.setBlockType(leverType, { x: 3, y: 2, z: 2 }); // 拉杆（Wall+North，支撑在 South）
    test.setBlockType("minecraft:redstone_lamp", { x: 3, y: 2, z: 1 }); // 红石灯（拉杆 North 侧 z-1 = outputDir）
}

// 场景 1：拨动开启——pullLever → POWERED 翻 true + 毗邻红石灯亮（充能输出方向）。
//
// 布局：(3,2,3) stone 支撑 + (3,2,2) 拉杆 + (3,2,1) 红石灯。
// pullLever((3,2,2)) → toggle 翻转 POWERED=false→true + _notifyNeighbors 通知 outputDir(North=z-1=(3,2,1)) →
// 红石灯 (3,2,1) neighborChanged → isBlockPowered（拉杆 weakPower 15）=true → lit=true。
//
// 判定：pollUntilSucceed 轮询 powered===true 且 lampLit===true（toggle 同步触发，留余量）。
function leverPowersWhenPulled(test: Test): void {
    placeLeverSetup(test, "minecraft:lever");

    // pullLever 同步拨动拉杆 → POWERED=true + 通知 outputDir 红石灯点亮。
    test.pullLever({ x: 3, y: 2, z: 2 });

    // 轮询断言 powered===true 且 lampLit===true（toggle 同步触发，pollUntilSucceed 留余量）。
    pollUntilSucceed(
        test,
        () => getLeverPowered(test, 3, 2, 2) === true && getLampLit(test, 3, 2, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(false, `lever: powered=${getLeverPowered(test, 3, 2, 2)}, lampLit=${getLampLit(test, 3, 2, 1)} (expected both true after pull)`);
            },
        },
    );
}

// 场景 2：再次拨动关闭——承接场景 1（powered=true），再 pullLever → POWERED 翻回 false + 灯灭（双稳态翻转）。
//
// 布局：承接场景 1——拉杆 powered=true（灯亮），再 pullLever → toggle 翻转 POWERED=true→false +
// _notifyNeighbors 通知红石灯 → 红石灯 neighborChanged → isBlockPowered=false → 调度 4tick 后 lit=false。
//
// 判定：用 runAtTickTime 分阶段断言。
//   - tick 5（首次拨动后）：powered===true 且 lampLit===true（首次拨动已开启）。
//   - tick 8 再次 pullLever（翻转关闭）。
//   - tick 25（再次拨动 + 灯断电延迟 4tick + 余量）：powered===false 且 lampLit===false（双稳态翻转关闭）。
function leverUnpowersWhenPulledAgain(test: Test): void {
    placeLeverSetup(test, "minecraft:lever");

    // tick 2 首次拨动（开启，powered: false→true）。
    test.runAtTickTime(2, () => {
        test.pullLever({ x: 3, y: 2, z: 2 });
    });

    // tick 5（首次拨动后）：断言已开启（powered=true、灯亮），证明首次拨动生效。
    test.runAtTickTime(5, () => {
        const powered = getLeverPowered(test, 3, 2, 2);
        const lampLit = getLampLit(test, 3, 2, 1);
        test.assert(powered === true && lampLit === true, `lever should be powered after first pull, got powered=${powered}, lampLit=${lampLit}`);
    });

    // tick 8 再次拨动（双稳态翻转关闭，powered: true→false）。
    test.runAtTickTime(8, () => {
        test.pullLever({ x: 3, y: 2, z: 2 });
    });

    // tick 25（再次拨动 8 + 灯断电延迟 4tick + 余量）：断言翻转关闭（powered=false、灯已延迟熄灭）。
    test.runAtTickTime(25, () => {
        const powered = getLeverPowered(test, 3, 2, 2);
        const lampLit = getLampLit(test, 3, 2, 1);
        test.assert(powered === false && lampLit === false, `lever should unpower after second pull (bistable), got powered=${powered}, lampLit=${lampLit}`);
        test.succeed();
    });
}

// 场景 3：持续供电无复位——拨开后远期 tick 仍 powered=true + 灯亮（无自动复位，区别于按钮 20/30gt 弹起）。
//
// 布局：同场景 1。pullLever → POWERED=true。在 tick 40（远超按钮 30gt 弹起窗口）断言仍 powered=true。
// 此场景区分拉杆（双稳态持续）与按钮（单稳态自动复位）：按钮 30gt 后必弹起，拉杆 40gt 仍开启。
function leverStaysPowered(test: Test): void {
    placeLeverSetup(test, "minecraft:lever");

    // tick 2 拨动开启（powered: false→true）。
    test.runAtTickTime(2, () => {
        test.pullLever({ x: 3, y: 2, z: 2 });
    });

    // tick 40（远超按钮 30gt 弹起窗口）：断言拉杆仍 powered=true + 灯亮（无自动复位，双稳态持续供电）。
    test.runAtTickTime(40, () => {
        const powered = getLeverPowered(test, 3, 2, 2);
        const lampLit = getLampLit(test, 3, 2, 1);
        test.assert(powered === true && lampLit === true, `lever should stay powered at tick 40 (no auto-reset, bistable), got powered=${powered}, lampLit=${lampLit}`);
        test.succeed();
    });
}

// 场景 4：Floor 4 朝向放置——点击 stone 顶面 Up → face=floor, facing=玩家水平朝向（同向，非 opposite）。
//
// 布局：(3,1,1) stone（被点击方块，拉杆 Floor 下方支撑）。每朝向独立 spawn 玩家在 playerPos →
//   lookAtLocation(lookAt) 设朝向 → 清理 (3,2,1) → 手持 lever useItemOnBlock 点击 (3,1,1) 顶面 Up →
//   placementPos=(3,2,1) → getStateForPlacement Y 轴分支：clickedFace=Up→floor, facing=horizontalDirection
//   (玩家水平朝向) → setBlockState 放拉杆 (3,2,1)。
//
// 判定：4 朝向放置后 (3,2,1) face===floor 且 facing===expectedFacing（玩家水平朝向，同向非 opposite）。
//
// 此场景验证 LeverBlock.getStateForPlacement Y 轴 Up 分支：face=floor, facing=horizontalDirection（玩家
//   朝向同向，同砂轮/钟 Y 轴分支）。旧实现落基类 defaultState（face 恒 wall、facing 恒 North），face 断言
//   即失败（floor≠wall）；重写后修正。每朝向用新 player 避免 yaw 残留；每次清理 (3,2,1) 避免拉杆残留阻断
//   放置。坐标配方与 GrindstoneTests Y_AXIS_FACING_CASES 完全一致（已验证可放置）。
function leverFloorFacingEqualsPlayerDirection(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FLOOR_FACING_CASES) {
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设朝向：yaw=atan2(-dx,dz) → horizontalDirection → 拉杆 facing=玩家水平朝向（同向）。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向拉杆残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 lever 点击 (3,1,1) stone 顶面 Up → 拉杆落 (3,2,1)。
        // getStateForPlacement Y 轴分支：clickedFace=Up→floor, facing=horizontalDirection(玩家水平朝向)。
        const leverItem = new ItemStack("minecraft:lever", 1);
        const used = player.useItemOnBlock(
            leverItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing floor lever facing ${c.expectedFacing} (player facing ${c.name})`);

        // 断言拉杆 (3,2,1) 已放置且 face=floor, facing=玩家水平朝向（同向非 opposite）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:lever", `lever should be placed at (3,2,1) for facing ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        test.assert(getLeverFace(test, 3, 2, 1) === "floor", `lever face should be floor, got ${getLeverFace(test, 3, 2, 1)}`);
        const facing = getLeverFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `lever facing should be ${c.expectedFacing} (player direction, not opposite), got ${facing}`);
    }

    test.succeed();
}

// 场景 5：Wall 2 朝向放置（区分新旧实现）——点击 stone 侧面 → face=wall, facing=点击面（South/East 两轴）。
//
// 布局：每 case 独立 stone 位置 + 点击面。手持 lever useItemOnBlock 点击 stone 侧面（clickedFace）→
//   placementPos=stone.offset(clickedFace) → getStateForPlacement 水平分支：face=wall, facing=clickedFace →
//   setBlockState。
//
// 判定：每 case 拉杆落点 face===wall 且 facing===expectedFacing（点击面本身，非 opposite）。
//
// 此场景验证 LeverBlock.getStateForPlacement 水平分支：face=wall, facing=clickedFace（点击面本身）。旧实现
//   落基类 defaultState（face 恒 wall、facing 恒 North）——face 碰巧相同（默认 wall）但 facing 恒 North，
//   故 South/East 两 case 的 facing 断言失败（south≠north、east≠north）；重写后修正为 facing=clickedFace。
//   2 朝向覆盖 Z 轴(South)+X 轴(East) 验证 wall 分支两轴判定。玩家朝向不影响 wall facing（facing=clickedFace），
//   玩家位置仅须远离落点避免碰撞。坐标配方与 GrindstoneTests WALL_FACING_CASES 完全一致（已验证可放置）。
function leverWallFacingEqualsClickedFace(test: Test): void {
    for (const c of WALL_FACING_CASES) {
        // 放被点击 stone（也是 wall 背面支撑，opposite(clickedFace)=stone 本身须非空气）。
        test.setBlockType("minecraft:stone", c.stonePos);
        test.assert(getBlockTypeId(test, c.stonePos.x, c.stonePos.y, c.stonePos.z) === "minecraft:stone", `stone should be at (${c.stonePos.x},${c.stonePos.y},${c.stonePos.z}), got ${getBlockTypeId(test, c.stonePos.x, c.stonePos.y, c.stonePos.z)}`);
        // 清理拉杆落点。
        test.setBlockType("minecraft:air", c.leverPos);

        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // 玩家朝向不影响 wall facing（facing=clickedFace），lookAtLocation 仅自然朝向（避免 spawn 默认朝向）。
        player.lookAtLocation(c.stonePos);

        // 手持 lever 点击 stone 侧面 clickedFace → 拉杆落 leverPos。
        // getStateForPlacement 水平分支：face=wall, facing=clickedFace。
        const leverItem = new ItemStack("minecraft:lever", 1);
        const used = player.useItemOnBlock(
            leverItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            c.stonePos,
            c.clickedFace,
        );
        test.assert(used, `useItemOnBlock should return true when placing wall lever facing ${c.expectedFacing} (clicked face ${c.name})`);

        // 断言拉杆落点 face=wall, facing=点击面本身（非 opposite）。
        test.assert(getBlockTypeId(test, c.leverPos.x, c.leverPos.y, c.leverPos.z) === "minecraft:lever", `lever should be placed at (${c.leverPos.x},${c.leverPos.y},${c.leverPos.z}) for ${c.name}, got ${getBlockTypeId(test, c.leverPos.x, c.leverPos.y, c.leverPos.z)}`);
        test.assert(getLeverFace(test, c.leverPos.x, c.leverPos.y, c.leverPos.z) === "wall", `lever face should be wall for ${c.name}, got ${getLeverFace(test, c.leverPos.x, c.leverPos.y, c.leverPos.z)}`);
        const facing = getLeverFacing(test, c.leverPos.x, c.leverPos.y, c.leverPos.z);
        test.assert(facing === c.expectedFacing, `lever facing should be ${c.expectedFacing} (clicked face, not opposite) for ${c.name}, got ${facing}`);
    }

    test.succeed();
}

export function registerLeverTests(): void {
    GameTest.register("BlockBehaviorTests", "lever_powers_when_pulled", leverPowersWhenPulled)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "lever_unpowers_when_pulled_again", leverUnpowersWhenPulledAgain)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "lever_stays_powered", leverStaysPowered)
        .structureName("gametests:glass_pit")
        .maxTicks(90);
    GameTest.register("BlockBehaviorTests", "lever_floor_facing_equals_player_direction", leverFloorFacingEqualsPlayerDirection)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "lever_wall_facing_equals_clicked_face", leverWallFacingEqualsClickedFace)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
