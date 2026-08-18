// 普通铁轨（rail）SHAPE 连接弯折行为 GameTest。
//
// wiki mechanism_铁轨.txt#普通铁轨：普通铁轨支持弯轨（道岔），当铁轨处于 L 形连接（恰好两个相邻水平
//   方向有铁轨且呈 90°，如南+东）时，SHAPE 从直轨（NorthSouth/EastWest）弯折为弯轨
//   （SouthEast/SouthWest/NorthWest/NorthEast）。这是普通铁轨区别于动力/激活/探测铁轨（仅直轨+斜坡，
//   isStraight=true 不支持弯轨）的核心几何行为。普通铁轨无 POWERED state，纯连接形状计算。
//
// C++ 链路：RailBlock（redstone/RailBlock.cpp）继承 AbstractRailBlock，构造传 isStraight=false（:45，
// 支持弯轨）。SHAPE + WATERLOGGED 两个 state，默认 shape=NorthSouth（:62-63）。
//   - onBlockAdded（AbstractRailBlock.cpp:133-141）：放置即 updateDir(world, pos, state, true) 强制
//     重算连接形状并传播到相邻铁轨。updateBlock=true 直接 setBlockState 写入 + 传播。
//   - neighborChanged（:176-213）：邻居变化 → updateState → updateDir(place)。基类 updateState（:244-255）
//     调 updateDir；RailBlock override（RailBlock.cpp:72-83）额外处理 T 型道岔红石切换（本测试不涉及）。
//   - RailState::place（RailState.cpp:260-415）：核心形状计算。
//     第一步 hasNeighborRail 查 4 水平方向（:271-274，含同层/上/下三层查铁轨）。
//     第三步弯轨选择（:296-310，仅 !m_isStraight 普通铁轨）：
//       se(south&&east) && !north && !west → SouthEast；
//       sw(south&&west) && !north && !east → SouthWest；
//       nw(north&&west) && !south && !east → NorthWest；
//       ne(north&&east) && !south && !west → NorthEast。
//     即恰好两个相邻方向有铁轨且呈 90° → 弯轨。
//   - AbstractRailBlock::isValidPosition（:215-221）：下方须 canSupportRigidBlock，无支撑 neighborChanged
//     自毁（:199-203）。故铁轨下方须放 stone 支撑。
//
// 测试覆盖（2 个场景，覆盖 wiki L 形弯折+回直核心确定行为）：
//   1. L 形弯折：中心铁轨 + 南铁轨 + 东铁轨（L 形三连接）→ 中心 SHAPE 从 NorthSouth 弯折为 SouthEast。
//   2. 弯折回直：L 形弯折后移除东铁轨 → 中心 south 邻居在、east 无 → SHAPE 回 NorthSouth（直轨）。
//
// 关键约束：
// 1. 铁轨须放 solid 支撑上方——中心/南/东铁轨下方各放 stone 支撑，否则 neighborChanged 无支撑自毁。
// 2. 放置顺序：先放中心铁轨（默认 NorthSouth），再放南铁轨、东铁轨。放南铁轨时中心 neighborChanged
//    → place：south 有、east 无 → hasNS && !hasEW → NorthSouth（不变）。放东铁轨时中心 neighborChanged
//    → place：south 有、east 有、north/west 无 → se=true → SouthEast 弯折。
// 3. setBlockType 放铁轨走 _resolveBlock 取 defaultState（shape=NorthSouth），onBlockAdded 触发 updateDir
//    重算（含邻居连接）。放置是同步 updateBlock=true，形状变化立即写入 + 传播。
// 4. 读 SHAPE state 用 getState("shape" as any)——Cubium 普通铁轨 SHAPE state 的 C++ 属性名为 "shape"
//    （RailBlock.hpp:104 RailShapeProperty::create("shape")，与 StairsTests 读楼梯 shape 同名）。getState
//    按 entry.property->name() 匹配 C++ 内部属性名（非基岩对外名 "rail_direction"）。EnumProperty 返回
//    枚举名字符串（north_south/south_east 等）。用 as any 绕过 BlockStateSuperset 白名单。
// 5. 弯折是 onBlockAdded/neighborChanged 同步触发，pollUntilSucceed 留余量防多级传播时序。
//
// 不测「T 型道岔红石切换」：三连接+红石信号切换弯轨方向（RailBlock::updateState :76-82），涉红石+多
//   连接复杂布局，本文件聚焦 L 形弯折基础几何。TODO: 可补 rail_t_junction_switched_by_redstone。
// 不测「斜坡铁轨（AscendingXxx）」：需高低差铁轨布局（上方/下方层铁轨），且斜坡支撑自毁（shouldBeRemoved）
//   增加复杂度，本文件聚焦平面 L 形弯折。TODO: 可补 rail_ascending_shape。
// 不测「动力/激活铁轨弯折」：二者 isStraight=true 不支持弯轨（RailShapeProperty::createStraight 仅 6 值），
//   无弯折行为可测。
//
// 跨服务端：rail 方块名两端一致，shape state 名两端一致（C++ 内部名，基岩对外 rail_direction 经
//   TemplateLoader 映射，脚本侧 getState 用 C++ 名 "shape"），L 形弯折行为两端一致（恰好
//   两相邻方向呈 90° → 弯轨）。两端均可放 rail 默认 state，弯折行为两端可对比，非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mechanism_铁轨.txt#普通铁轨（弯轨道岔，L 形连接弯折）
// Ref: RailBlock.cpp（isStraight=false 支持弯轨；onBlockAdded/neighborChanged → updateDir/place 重算）
// Ref: RailState.cpp（place 第三步弯轨选择：se/sw/nw/ne 恰好两相邻方向 → 弯轨）
// Ref: AbstractRailBlock.cpp（onBlockAdded updateDir(true) 放置即重算；isValidPosition 下方 solid 支撑）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景布局：中心铁轨 (3,2,2)，南铁轨 (3,2,3)（z+1），东铁轨 (4,2,2)（x+1），呈 L 形。
// 下方支撑 (3,1,2)/(3,1,3)/(4,1,2) stone（铁轨需 canSupportRigidBlock 下方）。

// 读取普通铁轨 SHAPE state（枚举名字符串）。返回 null 表示读取失败或非铁轨。
// Cubium 普通铁轨 SHAPE state C++ 属性名为 "shape"（RailBlock.hpp:104，与 StairsTests 楼梯 shape 同名）。
// getState 按 entry.property->name() 匹配 C++ 内部名，EnumProperty 返回枚举名字符串。
function getRailShape(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("shape" as any);
    return typeof value === "string" ? value : null;
}

// 放支撑 + 三块 L 形铁轨：中心 (3,2,2) + 南 (3,2,3) + 东 (4,2,2)。
// 放置顺序：中心 → 南 → 东。放东铁轨时中心 neighborChanged → place 计算 se → SouthEast 弯折。
function placeLRails(test: Test): void {
    // 支撑（铁轨需 canSupportRigidBlock 下方，否则 neighborChanged 无支撑自毁）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 2 }); // 中心支撑
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 3 }); // 南铁轨支撑
    test.setBlockType("minecraft:stone", { x: 4, y: 1, z: 2 }); // 东铁轨支撑

    // 中心铁轨（默认 NorthSouth）。onBlockAdded updateDir(true) 重算（此时无邻居 → 保持 NorthSouth）。
    test.setBlockType("minecraft:rail", { x: 3, y: 2, z: 2 });

    // 南铁轨（z+1）。放置触发中心 neighborChanged → place：south 有、east 无 → NorthSouth（不变）。
    test.setBlockType("minecraft:rail", { x: 3, y: 2, z: 3 });

    // 东铁轨（x+1）。放置触发中心 neighborChanged → place：south 有、east 有、north/west 无 →
    // se=true → SouthEast 弯折。
    test.setBlockType("minecraft:rail", { x: 4, y: 2, z: 2 });
}

// 场景 1：L 形弯折——中心 + 南 + 东三铁轨 → 中心 SHAPE 从 NorthSouth 弯折为 SouthEast。
//
// 布局：placeLRails 放 L 形三铁轨。放东铁轨时中心 neighborChanged → RailState::place：
//   north=false, south=true(南铁轨), west=false, east=true(东铁轨) → se=true && !north && !west
//   → shape=SouthEast（弯轨）。updateBlock=true 同步 setBlockState 写入。
//
// 判定：pollUntilSucceed 轮询中心 (3,2,2) SHAPE === "south_east"（L 形弯折）。
function railBendsIntoLShapeAtCorner(test: Test): void {
    placeLRails(test);

    // 轮询断言中心 (3,2,2) SHAPE === "south_east"（南+东 L 形弯折）。
    // onBlockAdded/neighborChanged 同步触发，pollUntilSucceed 留余量防多级传播时序。
    pollUntilSucceed(
        test,
        () => getRailShape(test, 3, 2, 2) === "south_east",
        {
            startTick: 2,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `rail center shape: should be south_east (L-bend), got ${getRailShape(test, 3, 2, 2)}`);
            },
        },
    );
}

// 场景 2：弯折回直——L 形弯折后移除东铁轨 → 中心 SHAPE 回 NorthSouth（south 邻居在，east 无）。
//
// 布局：承接场景 1——中心 SHAPE=SouthEast（L 形弯折）。runAtTickTime 等弯折稳定后 (4,2,2) 设 air
//   移除东铁轨。东铁轨移除触发中心 neighborChanged → RailState::place：
//   north=false, south=true, west=false, east=false → hasNS && !hasEW → NorthSouth（直轨，回直）。
//
// 判定：pollUntilSucceed 轮询中心 (3,2,2) SHAPE === "north_south"（回直）。
function railStraightensWhenLBranchRemoved(test: Test): void {
    placeLRails(test);

    // 等弯折稳定（中心 SHAPE=SouthEast）后移除东铁轨。
    test.runAtTickTime(5, () => {
        if (getRailShape(test, 3, 2, 2) !== "south_east") {
            test.assert(false, `rail center should be south_east before removing east rail, got ${getRailShape(test, 3, 2, 2)}`);
            return;
        }
        // (4,2,2) 设 air 移除东铁轨 → 中心 neighborChanged → place：仅 south 有 → NorthSouth（回直）。
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 2 });
    });

    // 轮询断言中心 (3,2,2) SHAPE === "north_south"（东铁轨移除后回直）。
    pollUntilSucceed(
        test,
        () => getRailShape(test, 3, 2, 2) === "north_south",
        {
            startTick: 12,
            interval: 4,
            maxTick: 50,
            onTimeout: () => {
                test.assert(false, `rail center shape: should be north_south after east rail removed, got ${getRailShape(test, 3, 2, 2)}`);
            },
        },
    );
}

export function registerRailTests(): void {
    GameTest.register("BlockBehaviorTests", "rail_bends_into_l_shape_at_corner", railBendsIntoLShapeAtCorner)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "rail_straightens_when_l_branch_removed", railStraightensWhenLBranchRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
