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
//     （StoneButtonBlock.cpp:36），对齐 Java 游戏刻口径。
//   - getWeakPower/getStrongPower：isPowered?15:0，全向输出（基岩充能模型，Java 方向性输出 TODO 未实现）。
//
// 按压入口：GameTestHelper::pressButton（GameTestHelper.cpp:418-440）已实现（非 stub），
//   dynamic_cast<AbstractButtonBlock*> 校验后调 button->press。JS 侧 test.pressButton(pos)。
//   注意：pullLever/pulseRedstone 是 stub，按钮测试只能用 pressButton。
//
// 测试覆盖（5 个场景，覆盖 wiki 脉冲+时序+充能核心行为，可跨服务端对比）：
//   1. 石按钮按压开启：pressButton → POWERED 翻 true + 毗邻红石灯亮（充能毗邻）。
//   2. 木按钮按压开启：pressButton → POWERED 翻 true + 红石灯亮（验证木按钮也能充能）。
//   3. 石按钮 20gt 后弹起：按压后 tick 22+ → POWERED 翻回 false + 灯灭（石按钮 20gt 脉冲）。
//   4. 木按钮 30gt 后弹起：按压后 tick 32+ → POWERED 翻回 false（木按钮 30gt，比石按钮长 10gt）。
//   5. 重复按压不延长脉冲：按压后立即再按压 → 弹起时刻不延后（isPowered 守卫防重复触发）。
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
// 5. 读 powered state 用 getState("powered" as any) 绕过 BlockStateSuperset 白名单。
//
// 不测「箭射中木按钮触发」：投射物碰撞链路未实现，跳过。TODO: 待投射物碰撞实现后补。
// 不测「attachFace=Floor/Ceiling 朝向」：setBlockType 只能放默认 Wall，无 place 逻辑，跳过。
// 不测「Java 方向性强输出」：Cubium 是基岩全向输出模型，Java 方向性 TODO 未实现，跳过。
//
// 跨服务端：按钮 powered state 名两端一致，脉冲时序（木 30gt/石 20gt）与 vanilla Java 一致。
//   注意：BE 用红石刻口径（木 15/石 10），Cubium 用游戏刻口径（木 30/石 20），数值等价。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_按钮.txt#红石元件（按压开启，石 20gt/木 30gt 后弹起）
// Ref: AbstractButtonBlock.cpp（press 置 POWERED+调度弹起 tick，tick 弹起，全向输出）
// Ref: WoodButtonBlock.cpp:36（WOOD_BUTTON_PRESS_TIME=30）/ StoneButtonBlock.cpp:36（STONE_BUTTON_PRESS_TIME=20）
// Ref: GameTestHelper.cpp:418-440（pressButton 调 AbstractButtonBlock::press）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
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
}
