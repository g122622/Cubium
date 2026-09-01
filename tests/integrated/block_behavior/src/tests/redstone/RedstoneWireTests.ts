// 红石线视觉连接行为 GameTest（红石线与相邻红石元件/红石线的水平连接判定）。
//
// wiki tech_红石粉.txt#形状（:174-177）：红石线在放置或接收方块更新时自动调整形状：
//   - 连到四周的红石线。
//   - 连到红石比较器，或是指向或背向它的红石中继器。
//   - 连到四周侦测器的输出端。
//   - 连到四周的其他红石电源元件、传输元件（含拉杆、红石火把等）。
// 红石线 north/east/south/west state（RedstoneSide enum: none/side/up）表示该方向连接形态。
//
// C++ 链路：RedstoneWireBlock::getConnection（RedstoneWireBlock.cpp:351-405）判定某方向连接：
//   1. 邻位是红石线 / canConnectTo（canProvidePower 或 canConnectRedstone）→ Side。
//   2. 邻位是 isNormalCube（固体不透明）且其上方有红石线 → Up（向上爬墙）。
//   3. 否则 None。
// canConnectTo（:142-172）：红石线互连恒 true；中继器/比较器按朝向；观察者按输出端朝向；其他方块
// 走 canProvidePower / canConnectRedstone。LeverBlock::canProvidePower 恒 true（LeverBlock.hpp:76-80），
// 故红石线连拉杆 → Side。石头 canProvidePower=false 且 canConnectRedstone=false，且 isNormalCube=true
// 但其上方无红石线 → None。
//
// 更新链路：RedstoneWireBlock::onBlockAdded（:207-212）放置时调 updatePower（:315-338）→
// calculateConnections（:340-352）同步全量重算四方向连接。updatePower 无条件重算连接（RedStoneWireBlock#onPlace→updatePowerStrength），
// 不依赖 power 是否变化——红石线视觉连接判定
// （shouldConnectTo）与电源是否激活无关，仅看相邻方块是否为红石线/canProvidePower 元件。故「先放
// East 邻位邻居，再放红石线 A」时，A 放置瞬间 onBlockAdded 同步算好 east 连接，立即可读。
// updatePostPlacement（:174-205）则在水平邻居变化时同步重算该方向（用于后放邻居场景，红石线邻居
// 经此链路互相传播连接）。纯同步（onBlockAdded 路径），无 tick 延迟。
// （注：updatePower 旧实现仅在 oldPower!=newPower 时重算连接，导致连拉杆等未激活电源元件时连接保持
// none——已修复，见 RedstoneWireBlock.cpp updatePower 注释。）
//
// 放置语义：setBlockType 走 _resolveBlock 取 defaultState（power=0, 四方向 none），不经
// getStateForPlacement。红石线无 isValidPosition 支撑自毁（updatePostPlacement 只处理水平连接），
// 故强放在任意位置不立即自毁。onBlockAdded 同步全量重算连接。
//
// 测试覆盖（3 个场景，行为两端一致，可跨服务端对比）：
//   1. 红石线连红石线（同类）→ east=side。
//   2. 红石线连拉杆（电源元件 canProvidePower）→ east=side。拉杆用 face=floor 放置（支撑在下方
//      helper y=1 结构内 y'=0 glass 地板固体），避免默认 face=wall 因支撑在结构外 air 被
//      neighborChanged 自毁。
//   3. 红石线不连石头（非红石元件，上方无红石线）→ east=none。
//
// 测试范式：先放 East 邻位 (4,2,1) 邻居方块，再放 (3,2,1) 红石线 A → A onBlockAdded 同步算 east。
// （若先放 A 再放邻居，A 需经 neighborChanged→scheduleBlockTick→tick 重算，走 tick 延迟路径；本文件
// 用 onBlockAdded 同步路径更稳，pollUntilSucceed 留余量。）
//
// 不测信号强度衰减（拉杆 power=15 → 红石线衰减）：拉杆需激活（toggle）才有 power，且信号传播走
// tick 调度（neighborChanged→scheduleBlockTick），非 onBlockAdded 同步路径，时序复杂。本文件聚焦
// 「视觉连接判定」核心行为点（同步），信号强度留待后续。TODO: 可补 redstone_wire_power_decay 测试。
// 不测向上爬墙（Up 连接）：需高低差红石线 + 红石导体阻挡判定，布局复杂，本文件聚焦水平 Side/None
// 核心判定。TODO: 可补 redstone_wire_ascends_wall 测试覆盖 Up 分支。
// 不测中继器/比较器/观察者朝向连接：需 setBlockWithStates 设 facing，且朝向判定逻辑独立，本文件
// 聚焦红石线互连 + 拉杆（canProvidePower）+ 石头（不连）三类基础判定。
//
// 跨服务端：红石线 north/east/south/west state 名两端一致（Java 式 RedstoneSide enum，值
// none/side/up），连接判定规则两端一致（红石线互连/电源元件 canProvidePower/非元件不连），可跨
// 服务端对比。getState("east") 用 as any 绕过 BlockStateSuperset 白名单（同栅栏/树叶范式）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_红石粉.txt#形状（红石线连接判定）
// Ref: RedstoneWireBlock.cpp（getConnection/calculateConnections/onBlockAdded/canConnectTo）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 结构内布局（结构内坐标 y'，helper y=N 对应结构内 y=N-1，因 placeOrigin 抬高一格）：
//   结构 y'=0（helper y=1）：满铺 glass 地板（固体）
//   结构 y'=1,2（helper y=2,3）：外圈圆石/玻璃墙 + 内部 5×5 air 腔（x,z∈[1,5]）
//   结构 y'=3,4（helper y=4,5）：零散圆石柱
// 方块测试在内部 air 腔 helper y=2（结构 y'=1）操作，拉杆 face=floor 支撑落在 helper y=1（结构 y'=0
// glass 地板，固体）——见下方 assertWireConnectsTo 坐标映射说明。
// 注意 helper y=0 对应结构内 y'=-1（结构外，超平坦草地之上的 air），切勿把需支撑的方块（拉杆/按钮等）
// 的支撑点落在 helper y=0，否则 neighborChanged 检测到 air 支撑会自毁。

// 取红石线指定方向的连接 state（"none"/"side"/"up"）。返回 null 表示读取失败。
function getWireConnection(test: Test, x: number, y: number, z: number, dir: string): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState(dir as any);
    return typeof value === "string" ? value : null;
}

// 通用红石线连接断言：先放 East 邻位 (4,2,1) 邻居方块，再放 (3,2,1) 红石线 A，断言 A east===expected。
//
// 坐标用 helper y=2（结构内 y'=1，glass_pit 内部 air 腔），非 helper y=1（结构内 y'=0 glass 地板）。
// 拉杆 face=floor 的支撑点 = pos.down() = helper (4,1,1) = 结构内 y'=0 = glass 地板（固体），拉杆不自毁。
// 历史坐标 bug：原用 helper y=1，拉杆 face=floor 支撑点 = helper (4,0,1) = 结构内 y'=-1 = 结构外 air
// （gridStartY=4，结构方块 origin 在世界 y=4，超平坦草方块 y=3，结构内容从世界 y=5 起，helper y=0
// 落在世界 y=4 = 草方块上方 air）。LeverBlock::neighborChanged 检测到 air 支撑自毁拉杆，红石线 east
// 邻位变 air → east=none，致 wire-lever 误判失败（wire-wire/wire-stone 因无支撑需求在 y=1 仍通过，
// 掩盖了同一坐标映射 bug）。详见 memory gametest-structure-coord-mapping-trap。
//
// @param test GameTest Test 对象
// @param neighborType 邻居方块 typeId
// @param expected A east 期望值（"side"/"none"）
// @param label 超时错误标签
// @param neighborStates 邻居方块可选 state 字符串（如拉杆需 "face=floor" 提供下方支撑避免被
//   neighborChanged 自毁；默认 undefined 走 setBlockType 默认 state）
function assertWireConnectsTo(
    test: Test,
    neighborType: string,
    expected: string,
    label: string,
    neighborStates?: string,
): void {
    const wirePos = { x: 3, y: 2, z: 1 };
    const neighborPos = { x: 4, y: 2, z: 1 };

    // East 邻位 (4,2,1) 先放邻居方块。先放邻居保证红石线 A 放置时 onBlockAdded 同步算 east 连接。
    // 拉杆等需附着面的方块须用 setBlockWithStates 提供有效支撑方向（如 face=floor 支撑在下方
    // (4,1,1) 结构内 y'=0 glass 地板，固体），否则红石线 A 放置时向拉杆派发 neighborChanged 会因
    // 支撑缺失自毁。
    if (neighborStates !== undefined) {
        test.setBlockWithStates(neighborType, neighborPos, neighborStates);
    } else {
        test.setBlockType(neighborType, neighborPos);
    }

    // (3,2,1) 放红石线 A。onBlockAdded → updatePower → calculateConnections 同步全量重算四方向连接
    // （含 east）。getConnection(East) 检查 (4,2,1) 邻居类型决定 east=Side/None。
    test.setBlockType("minecraft:redstone_wire", wirePos);

    // 轮询断言 A east === expected。onBlockAdded 同步，pollUntilSucceed 留余量。
    pollUntilSucceed(
        test,
        () => getWireConnection(test, 3, 2, 1, "east") === expected,
        {
            startTick: 5,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(
                    false,
                    `${label}: wire east should be ${expected}, got ${getWireConnection(test, 3, 2, 1, "east")}`,
                );
            },
        },
    );
}

// 红石线连红石线（同类）→ east=side。
// canConnectTo 第1条：邻居 redstone_wire → is(REDSTONE_WIRE) true → Side。
function redstoneWireConnectsToWire(test: Test): void {
    assertWireConnectsTo(test, "minecraft:redstone_wire", "side", "wire-wire");
}

// 红石线连拉杆（电源元件）→ east=side。
// canConnectTo：拉杆非红石线/中继器/比较器/观察者，走 canProvidePower 分支。LeverBlock::canProvidePower
// 恒 true（LeverBlock.hpp:76-80）→ Side。
//
// 拉杆放置用 face=floor（setBlockWithStates）而非默认 face=wall：LeverBlock::neighborChanged
// （LeverBlock.cpp:126-162）检查支撑方块 isAir，缺失即自毁。face=floor 时 supportPos = pos.down()。
// 红石线 A 在 helper (3,2,1)（结构内 y'=1 air 腔），拉杆在 (4,2,1)，face=floor 支撑点 = (4,1,1) =
// 结构内 y'=0 glass 地板（固体），拉杆不自毁，红石线正常连。若误用 helper y=1（结构内 y'=0 glass
// 地板位）放拉杆，face=floor 支撑点 = (4,0,1) = 结构内 y'=-1 结构外 air，拉杆自毁致 east=none。
// attachFace 不影响 canProvidePower（恒 true），故连接判定 east=side 不变。
function redstoneWireConnectsToLever(test: Test): void {
    assertWireConnectsTo(test, "minecraft:lever", "side", "wire-lever", "face=floor");
}

// 红石线不连石头 → east=none。
// canConnectTo：石头非红石元件，canProvidePower=false，canConnectRedstone=false → 不连。石头
// isNormalCube=true，但 getConnection 检查其上方 (4,3,1) 无红石线（air）→ 不走 Up → None。
function redstoneWireDoesNotConnectToStone(test: Test): void {
    assertWireConnectsTo(test, "minecraft:stone", "none", "wire-stone");
}

export function registerRedstoneWireTests(): void {
    GameTest.register("BlockBehaviorTests", "redstone_wire_connects_to_wire", redstoneWireConnectsToWire)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "redstone_wire_connects_to_lever", redstoneWireConnectsToLever)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "redstone_wire_does_not_connect_to_stone", redstoneWireDoesNotConnectToStone)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
