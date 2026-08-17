// 按钮红石脉冲行为 GameTest。
//
// wiki tech_按钮.txt#用途/红石元件：按钮是单稳态红石电源——被按压（玩家使用/风爆）后开启，
// 提供短暂红石信号，一段时间后自动关闭（弹起）。
//   - 石按钮/磨制黑石按钮：开启 20 游戏刻（wiki「20」，基岩 10 红石刻=20 游戏刻）。
//   - 木按钮：开启 30 游戏刻（wiki「30」，基岩 15 红石刻=30 游戏刻）。
//   - 开启时为毗邻红石线/比较器/中继器提供 15 信号、激活毗邻机械元件、强充能自身附着导体至 15。
//   - 状态变化时向毗邻提供方块更新（JE 同时 PP+NC 更新），向附着方块毗邻提供 NC 更新。
//
// C++ 链路：AbstractButtonBlock（AbstractButtonBlock.cpp）有三个 state：
//   - HORIZONTAL_FACING（Direction，默认 North，输出/朝向方向）
//   - POWERED（bool，默认 false）
//   - ATTACH_FACE（AttachFace，默认 Wall）
//   - press（AbstractButtonBlock.cpp:252-272）：已按下则 return（防重复）；否则 set POWERED=true（flags=2
//     无邻居更新防循环）+ playClickSound + notifyNeighbors（主动通知 6 向邻居 neighborChanged）+
//     scheduleBlockTick(m_ticksToStayPressed) 调度弹起。
//   - tick（AbstractButtonBlock.cpp:166-181）：POWERED=true 时 set POWERED=false + 弹起音效 + notifyNeighbors。
//   - 木按钮 WOOD_BUTTON_PRESS_TIME=30（WoodButtonBlock.cpp:36），石按钮 STONE_BUTTON_PRESS_TIME=20
//     （StoneButtonBlock.cpp:36），游戏刻口径。
//   - getStateForPlacement（AbstractButtonBlock 重写）：朝向与附着面由玩家点击面决定（附墙方块语义，
//     与朝玩家视线的 dispenser/piston 不同）：点击顶面→face=floor、facing=玩家水平朝向；点击底面→
//     ceiling、facing=玩家水平朝向；点击墙面→wall、facing=点击面（水平四向）。此前未重写该方法，落回
//     基类 Block::getStateForPlacement 返回 defaultState()（facing 恒 North、face 恒 wall），与预期按
//     点击面决定朝向/附着面的行为不一致。重写后修正。石/木按钮继承本类，继承本方法自动获得正确朝向。
//   - getWeakPower/getStrongPower：isPowered?15:0，全向输出（基岩充能模型，Java 方向性输出 TODO 未实现）。
//
// 按压入口：GameTestHelper::pressButton（GameTestHelper.cpp:418-440）已实现（非 stub），
//   dynamic_cast<AbstractButtonBlock*> 校验后调 button->press。JS 侧 test.pressButton(pos)。
//   注意：pullLever/pulseRedstone 是 stub，按钮测试只能用 pressButton。
//   放置朝向走 useItemOnBlock 手持按钮物品点击 stone 顶面/侧面 → BlockItem::tryPlace →
//   getStateForPlacement（基类 facing/face 按点击面）→ setBlockState。
//
// 测试覆盖（7 个场景，覆盖 wiki 脉冲+时序+充能+放置朝向核心行为，可跨服务端对比）：
//   1. 石按钮按压开启：pressButton → POWERED 翻 true + 毗邻红石灯亮（充能毗邻）。
//   2. 木按钮按压开启：pressButton → POWERED 翻 true + 红石灯亮（验证木按钮也能充能）。
//   3. 石按钮 20gt 后弹起：按压后 tick 22+ → POWERED 翻回 false + 灯灭（石按钮 20gt 脉冲）。
//   4. 木按钮 30gt 后弹起：按压后 tick 32+ → POWERED 翻回 false（木按钮 30gt，比石按钮长 10gt）。
//   5. 重复按压不延长脉冲：按压后立即再按压 → 弹起时刻不延后（isPowered 守卫防重复触发）。
//   6. Floor 4 朝向放置：点击 stone 顶面 Up → face=floor, facing=玩家水平朝向（4 朝向，复用
//      GrindstoneTests Y 轴坐标配方，facing=horizontalDirection 同向非 opposite）。
//   7. Wall 2 朝向放置（区分新旧实现）：点击 stone 侧面 → face=wall, facing=点击面（South/East 两轴，
//      复用 GrindstoneTests Wall 坐标配方）。旧实现落基类 defaultState（face 恒 wall、facing 恒 North），
//      新实现 face=wall、facing=点击面。
//
// 关键约束：
// 1. setBlockType 放按钮用默认 state（Wall+North），支撑方块在按钮 South 侧（z+1，facing North 的反方向）。
//    放置顺序：先放支撑 (3,2,2) stone，再放按钮 (3,2,1)，避免按钮自毁（updatePostPlacement 检测支撑缺失）。
//    放按钮后不要再动支撑位方块。
// 2. 被充能红石灯放 (4,2,1)（按钮 East 侧水平相邻）。基岩全向输出模型，水平相邻即被充能。
//    红石灯电平触发：press 同步点亮、弹起同步熄灭，适合做充能断言（不用铜灯——边沿锁存弹起不灭）。
// 3. pressButton 同步置 POWERED=true，pollUntilSucceed 仅作时序余量保险。
// 4. 弹起断言用 runAtTickTime 显式多时间点断言（不用 pollUntilSucceed 断 powered===false——首 tick 即
//    满足 false 会误判「尚未按压」），参照 PressurePlateTests stonePressurePlateIgnoresItem 范式。
// 5. 读 powered state 用 getState("powered" as any) 绕过 BlockStateSuperset 白名单。读 facing/face state
//    用 getState("facing"/"face" as any)（HORIZONTAL_FACING 返小写方向，ATTACH_FACE 返 floor/wall/ceiling）。
// 6. 场景 6/7 朝向控制（复用 GrindstoneTests/BellTests 含 pitch lookAtLocation 范式 + 水平 4 朝向坐标
//    配方）。Floor 分支 facing=horizontalDirection（玩家水平朝向，同向非 opposite，需 lookAt 控制 yaw）；
//    Wall 分支 facing=clickedFace（点击面，与玩家朝向无关）。每朝向独立 spawn 玩家避免 yaw 残留；每次清理
//    落点避免残留阻断放置。useItemOnBlock 第三参数 Direction=点击面（clickedFace），原样透传到
//    context.getClickedFace()，placementPos=被点击方块.offset(clickedFace)。此前 Cubium 未重写
//    getStateForPlacement（基类 defaultState，face 恒 wall、facing 恒 North），朝向放置全为默认值，断言
//    失败；重写后修正。
//
// 不测「箭射中木按钮触发」：投射物碰撞链路未实现，跳过。TODO: 待投射物碰撞实现后补。
// 不测「Java 方向性强输出」：Cubium 是基岩全向输出模型，Java 方向性 TODO 未实现，跳过。
//
// 跨服务端：按钮 powered/facing/face state 名两端一致，脉冲时序（木 30gt/石 20gt）+ 放置朝向（face/facing
//   按点击面）行为两端一致。注意：BE 用红石刻口径（木 15/石 10），Cubium 用游戏刻口径（木 30/石 20），
//   数值等价。朝向测试用 useItemOnBlock 放置（lookAtLocation 是 Cubium 专有朝向控制，但 face/facing 按点击面
//   放置行为两端可对比，非 one-sided）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_按钮.txt#红石元件（按压开启，石 20gt/木 30gt 后弹起）
// Ref: AbstractButtonBlock.cpp（press 置 POWERED+调度弹起 tick，tick 弹起，全向输出，getStateForPlacement 按点击面）
// Ref: WoodButtonBlock.cpp:36（WOOD_BUTTON_PRESS_TIME=30）/ StoneButtonBlock.cpp:36（STONE_BUTTON_PRESS_TIME=20）
// Ref: GameTestHelper.cpp:418-440（pressButton 调 AbstractButtonBlock::press）
// Ref: GrindstoneTests.ts（附墙方块 Y 轴 Floor/Wall 朝向测试范式 + 坐标配方，按钮复用同构）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Direction 参数=clickedFace 原样透传 getClickedFace）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 按钮放 (3,2,1)（Wall+North 默认），支撑 (3,2,2) stone（按钮 South 侧 z+1，须先于按钮放置）。
// 被充能红石灯放 (4,2,1)（按钮 East 侧水平相邻，基岩全向输出充能）。

// 读取按钮 powered state（bool）。返回 null 表示读取失败或非按钮。
function getButtonPowered(test: Test, x: number, y: number, z: number): boolean | null {
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

// 读取 (x,y,z) 按钮 facing state（方向名字符串 "north"/"south"/"east"/"west"）。返回 null 表示失败或非按钮。
// HORIZONTAL_FACING() 的 C++ 属性名为 "facing"（水平四向，无 up/down）。
function getButtonFacing(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("facing" as any);
    return typeof value === "string" ? value : null;
}

// 读取 (x,y,z) 按钮 face state（附着面字符串 "floor"/"wall"/"ceiling"）。返回 null 表示失败或非按钮。
// ATTACH_FACE() 的 C++ 属性名为 "face"。
function getButtonFace(test: Test, x: number, y: number, z: number): string | null {
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

// Floor 4 朝向放置映射表（复用 GrindstoneTests/BellTests 的 Y 轴坐标配方，按钮 Floor 分支 facing=
// horizontalDirection 玩家水平朝向，同向非 opposite）。
// horizontalDirection=orderedByNearest(yaw,pitch)[0] 的水平分量，pitch≈0 时 = 玩家水平朝向。
// lookAtLocation yaw=atan2(-dx,dz)：0→South,90→West,180→North,270→East。
// 【关键】lookAt.y=playerPos.y+1 使 pitch≈0（horizontalDirection 仅读 yaw，但 pitch≈0 与现有范式一致稳妥）。
interface FloorFacingCase {
    name: string; // 玩家水平朝向名（lookAt 产生的 horizontalDirection）
    playerPos: { x: number; y: number; z: number }; // 玩家 spawn 位置（不与 stone/按钮落点重叠）
    lookAt: { x: number; y: number; z: number }; // lookAtLocation 目标（y=playerPos.y+1 保证 pitch≈0）
    expectedFacing: string; // 按钮 facing=玩家水平朝向（同向非 opposite）
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

// Wall 2 朝向放置映射表（复用 GrindstoneTests Wall 坐标配方，按钮 Wall 分支 facing=clickedFace 点击面本身）。
// 每 case 独立 stone 位置 + 点击面 + 落点，玩家站远离落点处（避免实体碰撞阻断放置）。
interface WallFacingCase {
    name: string; // 点击面名（facing=该面）
    stonePos: { x: number; y: number; z: number }; // 被点击 stone 位置（也是 wall 背面支撑）
    clickedFace: Direction; // 点击面（facing=此面）
    buttonPos: { x: number; y: number; z: number }; // 按钮落点（stone.offset(clickedFace)）
    playerPos: { x: number; y: number; z: number }; // 玩家位置（远离落点，避免碰撞；朝向不影响 wall facing）
    expectedFacing: string; // 按钮 facing=clickedFace
}

// 2 朝向（覆盖 Z 轴 South + X 轴 East，验证 wall 分支两轴判定）：
//   South（Z 轴）：stone(3,2,1) 点击 South → 按钮落(3,2,2), facing=south。背面 opposite(south)=north=(3,2,1) stone ✓。
//   East（X 轴）：stone(1,2,2) 点击 East → 按钮落(2,2,2), facing=east。背面 opposite(east)=west=(1,2,2) stone ✓。
// 玩家站远离落点：South case 玩家(3,2,4)（落点(3,2,2) 北侧 2 格外）；East case 玩家(5,2,2)（落点(2,2,2) 东侧 3 格外）。
const WALL_FACING_CASES: WallFacingCase[] = [
    {
        name: "south",
        stonePos: { x: 3, y: 2, z: 1 },
        clickedFace: Direction.South,
        buttonPos: { x: 3, y: 2, z: 2 },
        playerPos: { x: 3, y: 2, z: 4 },
        expectedFacing: "south",
    },
    {
        name: "east",
        stonePos: { x: 1, y: 2, z: 2 },
        clickedFace: Direction.East,
        buttonPos: { x: 2, y: 2, z: 2 },
        playerPos: { x: 5, y: 2, z: 2 },
        expectedFacing: "east",
    },
];

// 放支撑 + 按钮 + 被充能红石灯。
// 顺序：先放支撑 (3,2,2) stone（按钮 South 侧），再放按钮 (3,2,1)，再放红石灯 (4,2,1)。
// 先支撑后按钮，避免按钮 updatePostPlacement 检测支撑缺失而自毁。
function placeButtonSetup(test: Test, buttonType: string): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 2 }); // 支撑（按钮 South 侧 z+1）
    test.setBlockType(buttonType, { x: 3, y: 2, z: 1 }); // 按钮（Wall+North，支撑在 South）
    test.setBlockType("minecraft:redstone_lamp", { x: 4, y: 2, z: 1 }); // 被充能红石灯（按钮 East 侧）
}

// 场景 1：石按钮按压 → POWERED 翻 true + 毗邻红石灯亮（充能毗邻）。
//
// 布局：(3,2,2) stone 支撑 + (3,2,1) 石按钮 + (4,2,1) 红石灯。
// pressButton((3,2,1)) → press 同步 set POWERED=true + notifyNeighbors → 红石灯 (4,2,1) 邻居
// neighborChanged → isBlockPowered(按钮 weakPower 15)=true → lit=true。
//
// 判定：pollUntilSucceed 轮询 powered===true 且 lampLit===true（press 同步触发，留余量）。
function stoneButtonPowersWhenPressed(test: Test): void {
    placeButtonSetup(test, "minecraft:stone_button");

    // pressButton 同步按压石按钮 → POWERED=true + 通知邻居红石灯点亮。
    test.pressButton({ x: 3, y: 2, z: 1 });

    // 轮询断言 powered===true 且 lampLit===true（press 同步触发，pollUntilSucceed 留余量）。
    pollUntilSucceed(
        test,
        () => getButtonPowered(test, 3, 2, 1) === true && getLampLit(test, 4, 2, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(false, `stone button: powered=${getButtonPowered(test, 3, 2, 1)}, lampLit=${getLampLit(test, 4, 2, 1)} (expected both true after press)`);
            },
        },
    );
}

// 场景 2：木按钮按压 → POWERED 翻 true + 红石灯亮（验证木按钮也能充能，与石按钮充能行为一致）。
//
// 布局：同场景 1，按钮换木按钮。pressButton → POWERED=true + 红石灯亮。
function woodButtonPowersWhenPressed(test: Test): void {
    placeButtonSetup(test, "minecraft:oak_button");

    test.pressButton({ x: 3, y: 2, z: 1 });

    pollUntilSucceed(
        test,
        () => getButtonPowered(test, 3, 2, 1) === true && getLampLit(test, 4, 2, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(false, `wood button: powered=${getButtonPowered(test, 3, 2, 1)}, lampLit=${getLampLit(test, 4, 2, 1)} (expected both true after press)`);
            },
        },
    );
}

// 场景 3：石按钮按压 20gt 后弹起 → POWERED 翻回 false + 灯灭（石按钮 20gt 脉冲）。
//
// 布局：同场景 1。pressButton → POWERED=true，调度 STONE_BUTTON_PRESS_TIME(20) tick 弹起。
// 时序：tick 2 按压 → tick 22 弹起（2+20，POWERED=false + notifyNeighbors）。
//   红石灯断电有 4tick 延迟（RedstoneLampBlock.cpp:113 scheduleBlockTick 4tick 后熄灭，对齐 wiki 防闪烁）：
//   按钮弹起 tick 22 → 灯 neighborChanged 调度 4tick → tick 26 灯 lit=false。
//
// 判定：用 runAtTickTime 显式多时间点断言。
//   - tick 15（弹起前）：powered===true 且 lampLit===true（仍在脉冲窗口内，证明曾按压）。
//   - tick 30（弹起 22 + 灯延迟 4 + 余量 4）：powered===false 且 lampLit===false（脉冲结束，灯已延迟熄灭）。
// 不用 pollUntilSucceed 断 powered===false（首 tick 即 false 误判），用多时间点断言区分「未按压」与「弹起后」。
function stoneButtonUnpressesAfterTwentyTicks(test: Test): void {
    placeButtonSetup(test, "minecraft:stone_button");

    // tick 2 按压（留 setup 稳定余量）。调度 20tick 弹起 → tick 22 弹起。
    test.runAtTickTime(2, () => {
        test.pressButton({ x: 3, y: 2, z: 1 });
    });

    // tick 15（弹起前，20gt 窗口内）：断言仍在脉冲（powered=true、灯亮），证明按压生效。
    test.runAtTickTime(15, () => {
        const powered = getButtonPowered(test, 3, 2, 1);
        const lampLit = getLampLit(test, 4, 2, 1);
        test.assert(powered === true && lampLit === true, `stone button should be powered at tick 15 (within 20gt pulse), got powered=${powered}, lampLit=${lampLit}`);
    });

    // tick 30（弹起 tick 22 + 灯断电延迟 4tick + 余量）：断言脉冲结束（powered=false、灯已延迟熄灭）。
    test.runAtTickTime(30, () => {
        const powered = getButtonPowered(test, 3, 2, 1);
        const lampLit = getLampLit(test, 4, 2, 1);
        test.assert(powered === false && lampLit === false, `stone button should unpress after 20 ticks (lamp extinguishes after 4tick delay), got powered=${powered}, lampLit=${lampLit}`);
        test.succeed();
    });
}

// 场景 4：木按钮按压 30gt 后弹起 → POWERED 翻回 false（木按钮 30gt，比石按钮长 10gt）。
//
// 布局：木按钮。pressButton → 调度 WOOD_BUTTON_PRESS_TIME(30) tick 弹起。
// 时序：tick 2 按压 → tick 32 弹起。
//
// 判定：多时间点断言。
//   - tick 25（石按钮已弹起但木按钮仍在脉冲窗口内）：powered===true（证明木按钮比石按钮长）。
//   - tick 37（30gt+余量）：powered===false（木按钮弹起）。
function woodButtonUnpressesAfterThirtyTicks(test: Test): void {
    placeButtonSetup(test, "minecraft:oak_button");

    test.runAtTickTime(2, () => {
        test.pressButton({ x: 3, y: 2, z: 1 });
    });

    // tick 25（石按钮 20gt 已弹起，木按钮 30gt 仍在脉冲窗口）：断言木按钮仍 powered=true。
    // 此时间点区分木/石按钮持续时长差异（木比石长 10gt）。
    test.runAtTickTime(25, () => {
        const powered = getButtonPowered(test, 3, 2, 1);
        test.assert(powered === true, `wood button should still be powered at tick 25 (30gt pulse, longer than stone 20gt), got powered=${powered}`);
    });

    // tick 37（30gt+余量）：断言木按钮弹起 powered=false。
    test.runAtTickTime(37, () => {
        const powered = getButtonPowered(test, 3, 2, 1);
        test.assert(powered === false, `wood button should unpress after 30 ticks, got powered=${powered}`);
        test.succeed();
    });
}

// 场景 5：重复按压不延长脉冲——按压后立即再按压，弹起时刻不延后。
//
// wiki/Java 语义：press 有 isPowered 守卫，已按下的按钮再 press 直接 return，不重新调度弹起 tick。
// 故重复按压不会延长脉冲，按钮仍按首次按压的 20gt（石）弹起。
//
// 布局：石按钮。tick 2 首次按压，tick 5 再次按压（仍在脉冲窗口内）。
// 判定：tick 25（首次按压 20gt+余量，未受第二次按压延长）断言 powered===false（仍按时弹起）。
function buttonPressDoesNotExtendPulse(test: Test): void {
    placeButtonSetup(test, "minecraft:stone_button");

    // tick 2 首次按压（调度 20gt 弹起，预期 tick 22 弹起）。
    test.runAtTickTime(2, () => {
        test.pressButton({ x: 3, y: 2, z: 1 });
    });

    // tick 5 再次按压（仍在脉冲窗口，isPowered 守卫应 return 不重调度弹起）。
    test.runAtTickTime(5, () => {
        test.pressButton({ x: 3, y: 2, z: 1 });
    });

    // tick 25（首次按压 20gt 弹起 + 余量，若重复按压延长脉冲则会仍 powered=true）：
    // 断言 powered===false（重复按压未延长脉冲，按首次按压时刻弹起）。
    test.runAtTickTime(25, () => {
        const powered = getButtonPowered(test, 3, 2, 1);
        test.assert(powered === false, `button should not extend pulse on re-press (isPowered guard), got powered=${powered} at tick 25`);
        test.succeed();
    });
}

// 场景 6：Floor 4 朝向放置——点击 stone 顶面 Up → face=floor, facing=玩家水平朝向（同向，非 opposite）。
//
// 布局：(3,1,1) stone（被点击方块，按钮 Floor 下方支撑）。每朝向独立 spawn 玩家在 playerPos →
//   lookAtLocation(lookAt) 设朝向 → 清理 (3,2,1) → 手持 stone_button useItemOnBlock 点击 (3,1,1) 顶面 Up →
//   placementPos=(3,2,1) → getStateForPlacement Y 轴分支：clickedFace=Up→floor, facing=horizontalDirection
//   (玩家水平朝向) → updatePostPlacement Floor 查 supportDir=Down=(3,1,1)=stone 非空气（不自毁）→
//   setBlockState 放按钮 (3,2,1)。
//
// 判定：4 朝向放置后 (3,2,1) face===floor 且 facing===expectedFacing（玩家水平朝向，同向非 opposite）。
//
// 此场景验证 AbstractButtonBlock.getStateForPlacement Y 轴 Up 分支：face=floor, facing=horizontalDirection
//   （玩家朝向同向，同砂轮/钟 Y 轴分支）。旧实现落基类 defaultState（face 恒 wall、facing 恒 North），
//   face 断言即失败（floor≠wall）；重写后修正。每朝向用新 player 避免 yaw 残留；每次清理 (3,2,1) 避免
//   按钮残留阻断放置。坐标配方与 GrindstoneTests Y_AXIS_FACING_CASES 完全一致（已验证可放置）。
function buttonFloorFacingEqualsPlayerDirection(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.assert(getBlockTypeId(test, 3, 1, 1) === "minecraft:stone", `stone should be at (3,1,1), got ${getBlockTypeId(test, 3, 1, 1)}`);

    for (const c of FLOOR_FACING_CASES) {
        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // lookAtLocation 设朝向：yaw=atan2(-dx,dz) → horizontalDirection → 按钮 facing=玩家水平朝向（同向）。
        player.lookAtLocation(c.lookAt);

        // 清理 (3,2,1) 避免上一朝向按钮残留阻断放置。
        test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

        // 手持 stone_button 点击 (3,1,1) stone 顶面 Up → 按钮落 (3,2,1)。
        // getStateForPlacement Y 轴分支：clickedFace=Up→floor, facing=horizontalDirection(玩家水平朝向)。
        const buttonItem = new ItemStack("minecraft:stone_button", 1);
        const used = player.useItemOnBlock(
            buttonItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            { x: 3, y: 1, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true when placing floor button facing ${c.expectedFacing} (player facing ${c.name})`);

        // 断言按钮 (3,2,1) 已放置且 face=floor, facing=玩家水平朝向（同向非 opposite）。
        test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:stone_button", `button should be placed at (3,2,1) for facing ${c.name}, got ${getBlockTypeId(test, 3, 2, 1)}`);
        test.assert(getButtonFace(test, 3, 2, 1) === "floor", `button face should be floor, got ${getButtonFace(test, 3, 2, 1)}`);
        const facing = getButtonFacing(test, 3, 2, 1);
        test.assert(facing === c.expectedFacing, `button facing should be ${c.expectedFacing} (player direction, not opposite), got ${facing}`);
    }

    test.succeed();
}

// 场景 7：Wall 2 朝向放置（区分新旧实现）——点击 stone 侧面 → face=wall, facing=点击面（South/East 两轴）。
//
// 布局：每 case 独立 stone 位置 + 点击面。手持 stone_button useItemOnBlock 点击 stone 侧面（clickedFace）→
//   placementPos=stone.offset(clickedFace) → getStateForPlacement 水平分支：face=wall, facing=clickedFace →
//   updatePostPlacement Wall 查 supportDir=opposite(clickedFace)=stone 本身非空气（不自毁）→ setBlockState。
//
// 判定：每 case 按钮落点 face===wall 且 facing===expectedFacing（点击面本身，非 opposite）。
//
// 此场景验证 AbstractButtonBlock.getStateForPlacement 水平分支：face=wall, facing=clickedFace（点击面本身）。
//   旧实现落基类 defaultState（face 恒 wall、facing 恒 North）——face 碰巧相同（默认 wall）但 facing 恒 North，
//   故 South/East 两 case 的 facing 断言失败（south≠north、east≠north）；重写后修正为 facing=clickedFace。
//   2 朝向覆盖 Z 轴(South)+X 轴(East) 验证 wall 分支两轴判定。玩家朝向不影响 wall facing（facing=clickedFace），
//   玩家位置仅须远离落点避免碰撞。坐标配方与 GrindstoneTests WALL_FACING_CASES 完全一致（已验证可放置）。
function buttonWallFacingEqualsClickedFace(test: Test): void {
    for (const c of WALL_FACING_CASES) {
        // 放被点击 stone（也是 wall 背面支撑，opposite(clickedFace)=stone 本身须非空气）。
        test.setBlockType("minecraft:stone", c.stonePos);
        test.assert(getBlockTypeId(test, c.stonePos.x, c.stonePos.y, c.stonePos.z) === "minecraft:stone", `stone should be at (${c.stonePos.x},${c.stonePos.y},${c.stonePos.z}), got ${getBlockTypeId(test, c.stonePos.x, c.stonePos.y, c.stonePos.z)}`);
        // 清理按钮落点。
        test.setBlockType("minecraft:air", c.buttonPos);

        const player = test.spawnSimulatedPlayer(c.playerPos, `p_${c.name}`);
        // 玩家朝向不影响 wall facing（facing=clickedFace），lookAtLocation 仅自然朝向（避免 spawn 默认朝向）。
        player.lookAtLocation(c.stonePos);

        // 手持 stone_button 点击 stone 侧面 clickedFace → 按钮落 buttonPos。
        // getStateForPlacement 水平分支：face=wall, facing=clickedFace。
        const buttonItem = new ItemStack("minecraft:stone_button", 1);
        const used = player.useItemOnBlock(
            buttonItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
            c.stonePos,
            c.clickedFace,
        );
        test.assert(used, `useItemOnBlock should return true when placing wall button facing ${c.expectedFacing} (clicked face ${c.name})`);

        // 断言按钮落点 face=wall, facing=点击面本身（非 opposite）。
        test.assert(getBlockTypeId(test, c.buttonPos.x, c.buttonPos.y, c.buttonPos.z) === "minecraft:stone_button", `button should be placed at (${c.buttonPos.x},${c.buttonPos.y},${c.buttonPos.z}) for ${c.name}, got ${getBlockTypeId(test, c.buttonPos.x, c.buttonPos.y, c.buttonPos.z)}`);
        test.assert(getButtonFace(test, c.buttonPos.x, c.buttonPos.y, c.buttonPos.z) === "wall", `button face should be wall for ${c.name}, got ${getButtonFace(test, c.buttonPos.x, c.buttonPos.y, c.buttonPos.z)}`);
        const facing = getButtonFacing(test, c.buttonPos.x, c.buttonPos.y, c.buttonPos.z);
        test.assert(facing === c.expectedFacing, `button facing should be ${c.expectedFacing} (clicked face, not opposite) for ${c.name}, got ${facing}`);
    }

    test.succeed();
}

export function registerButtonTests(): void {
    GameTest.register("BlockBehaviorTests", "stone_button_powers_when_pressed", stoneButtonPowersWhenPressed)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "wood_button_powers_when_pressed", woodButtonPowersWhenPressed)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "stone_button_unpresses_after_20_ticks", stoneButtonUnpressesAfterTwentyTicks)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "wood_button_unpresses_after_30_ticks", woodButtonUnpressesAfterThirtyTicks)
        .structureName("gametests:glass_pit")
        .maxTicks(90);
    GameTest.register("BlockBehaviorTests", "button_press_does_not_extend_pulse", buttonPressDoesNotExtendPulse)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "button_floor_facing_equals_player_direction", buttonFloorFacingEqualsPlayerDirection)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "button_wall_facing_equals_clicked_face", buttonWallFacingEqualsClickedFace)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
