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
//   - toggle（LeverBlock.cpp:151-164）：翻转 POWERED → setBlockState(flags=2 无邻居更新防循环) →
//     _playClickSound → _notifyNeighbors（手动通知输出方向 + 支撑方块两格）。
//   - _notifyNeighbors（:224-266）：Wall 模式 outputDir=facing，supportPos=pos.offset(opposite(facing))；
//     通知 outputDir 格 + supportPos 格的 neighborChanged（仅这两格，非全 6 向）。
//   - getWeakPower（:166-174）：isPowered?15:0 全向弱信号（基岩全向模型）。
//   - getStrongPower（:176-212）：仅 outputDir 方向输出 15 强信号（强充能附着导体）。
//
// 拨动入口：GameTestHelper::pullLever（GameTestHelper.cpp:442）已实现（委托 LeverBlock::toggle），
//   JS 侧 test.pullLever(pos)（ScriptTestHelper.cpp:341-358 绑定）。与 pressButton 同范式。
//
// 测试覆盖（3 个场景，覆盖 wiki 双稳态开关+持续供电核心行为，可跨服务端对比）：
//   1. 拨动开启：pullLever → POWERED 翻 true + 毗邻红石灯亮（充能输出方向）。
//   2. 再次拨动关闭：承接场景 1（powered=true），再 pullLever → POWERED 翻回 false + 灯灭（双稳态翻转）。
//   3. 持续供电无复位：拨开后多 tick 仍 powered=true + 灯亮（无自动复位，区别于按钮 20/30gt 弹起）。
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
// 5. 读 powered/lit state 用 getState("powered"/"lit" as any) 绕过 BlockStateSuperset 白名单。
// 6. 场景 3 用 runAtTickTime 在远期 tick（如 tick 40，远超按钮 30gt 弹起）断言仍 powered=true，证明无复位。
//
// 不测「Java 方向性强输出」：Cubium getStrongPower 已实现方向性，但测试聚焦双稳态+持续供电，
//   方向性强充能导体场景涉强充能链路复杂，跳过。TODO: 待强充能链路测试完善后补。
// 不测「Floor/Ceiling 朝向」：setBlockType 只能放默认 Wall，无 place 逻辑，跳过。
//
// 跨服务端：拉杆 powered state 名两端一致，双稳态翻转 + 持续供电行为两端一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_拉杆.txt#红石元件（双稳态，拨动翻转，无自动复位）
// Ref: LeverBlock.cpp（toggle 翻转 POWERED+notifyNeighbors；getWeakPower 全向 15；_notifyNeighbors 通知 outputDir+support）
// Ref: GameTestHelper.cpp:442（pullLever 委托 LeverBlock::toggle）
// Ref: RedstoneLampBlock.cpp:113（断电 4tick 延迟，防闪烁）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
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
}
