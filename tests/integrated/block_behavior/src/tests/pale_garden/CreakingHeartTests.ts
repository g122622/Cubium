// 嘎枝之心放置默认状态 + 轴向放置行为 GameTest。
//
// wiki other_嘎枝之心.txt#用途：嘎枝之心可以放置在不同的方向，类似于原木。它具有失活（uprooted）、
//   休眠（dormant）和激活（awake）状态，其中初始处于失活状态（玩家放置默认 uprooted）。
//   当嘎枝之心两端的方块都是苍白橡木原木/苍白橡木（pale_oak_logs 标签）且朝向（AXIS）一致时，
//   才会脱离失活状态进入休眠/激活（周期检查 tick → updateState）。玩家放置时周围无原木，保持失活。
//   natural 属性区分自然生成（true）与玩家放置（false）：仅自然生成的嘎枝之心被玩家破坏掉落 20-24 经验。
//
// vanilla 对齐（CreakingHeartBlock.java）：
//   - 默认 state（:49-51）：AXIS=Y, STATE=UPROOTED, NATURAL=false
//   - getStateForPlacement（:144-147）：AXIS=clickedFace.getAxis()（点击面轴向，同原木 pillar 放置语义）
//     → 调 updateState：若 hasRequiredLogs 且 STATE==UPROOTED → 按 CREAKING_ACTIVE 设 AWAKE/DORMANT；
//       否则保持 UPROOTED。NATURAL 不改（保持 false，玩家放置非自然生成）。
//   - updateState（:106-117）：hasRequiredLogs（轴线两端均为 pale_oak_logs 且 AXIS 一致）&& STATE==UPROOTED
//     → 按 CREAKING_ACTIVE 环境属性（夜晚→AWAKE，其他→DORMANT）；否则不变。
//   - hasRequiredLogs（:119-130）：沿 AXIS 方向两端方块须是 pale_oak_logs 且 AXIS 一致。
//   - 周期检查 tick（:99-104）：updateShape 调 scheduleTick(1) → tick → updateState。
//   - 比较器输出 getAnalogOutputSignal（:202-210）：UPROOTED 返 0；否则按绑定嘎枝距离公式
//     15 - floor(min(d,32)/32*15)（依赖 CreakingHeartBlockEntity 距离追踪）。
//
// C++ 链路（CreakingHeartBlock.cpp）：
//   - 默认 state（:62-64）：CREAKING_HEART_STATE=Uprooted, NATURAL=false（AXIS 默认 Y 由 RotatedPillarBlock 基类设）
//   - getStateForPlacement（:72-94）：AXIS=getAxis(clickedFace)（对齐 vanilla clickedFace.getAxis()），
//     STATE=Uprooted（对齐 vanilla——放置时周围无原木，updateState 不改 state），
//     NATURAL=false（对齐 vanilla——玩家放置非自然生成）。
//   - getComparatorInputOverride（:96-110）：简化为 STATE 映射（Uprooted=0/Dormant=1/Awake=2），
//     非 vanilla 距离公式（依赖未实现的方块实体，见 TODO）。
//
// 测试覆盖（2 个场景，覆盖 wiki 放置默认状态 + 点击面轴向放置核心行为）：
//   1. creaking_heart_default_state_when_placed_on_top：玩家手持 creaking_heart 点击 stone 顶面 Up
//      → AXIS=Y（点击顶面轴向 Y）+ STATE=uprooted（放置默认失活）+ natural=false（玩家放置非自然）。
//      验证 getStateForPlacement 三个 state 均对齐 vanilla。
//   2. creaking_heart_axis_from_clicked_face：点击 stone 侧面 South → AXIS=Z（点击面轴向 Z）。
//      验证 AXIS 按 clickedFace.getAxis() 设置（非旧实现按玩家水平朝向 horizontalDirection）。
//
// 关键约束：
// 1. 嘎枝之心是完整方块（1×1×1，Material::WOOD），放 stone 上方点击 Up 落到 (3,2,1)。
// 2. 场景 1 验证三个 state 全对齐 vanilla：axis=y + creaking_heart_state=uprooted + natural=false。
//    - 旧实现：NATURAL=true（应为 false）、STATE=Dormant（应为 Uprooted）、AXIS 按 horizontalDirection（应按 clickedFace）。
//    - 新实现：全对齐 vanilla。
// 3. 场景 2 点击 South 面（Z 轴方向）→ AXIS=Z。旧实现按玩家朝向设 axis（玩家朝南/北→Z），
//    碰巧 South→Z 一致，但语义错误（应按点击面）。改用点击 East 面（X 轴方向）→ AXIS=X 验证，
//    与玩家朝向解耦（玩家朝南 click East 旧实现 axis=Z、新实现 axis=X，区分明显）。
// 4. 读 state 用 getState("axis"/"creaking_heart_state"/"natural" as any) 绕过 BlockStateSuperset 白名单。
//    axis 返 "x"/"y"/"z"（EnumProperty<Axis>）；creaking_heart_state 返 "uprooted"/"dormant"/"awake"
//    （EnumProperty<CreakingHeartState>）；natural 返 boolean（BooleanProperty）。
// 5. SimulatedPlayer mayBuild=true（创造模式），useItemOnBlock 放置不消耗物品。
//
// 不测「周期检查状态切换」：依赖 tick → updateState → hasRequiredLogs + CREAKING_ACTIVE 环境属性链路，
//   均未实现（见 CreakingHeartBlock.cpp TODO）。TODO: 待周期检查链路补全后补 uprooted_to_dormant 测试。
// 不测「比较器输出距离公式」：依赖 CreakingHeartBlockEntity 距离追踪（未实现），Cubium 简化为 STATE 映射。
//   TODO: 待方块实体距离追踪补全后补 comparator_distance_output 测试。
// 不测「生成嘎枝/树脂团」：依赖方块实体 + CreakingEntity AI（未实现）。
//   TODO: 待嘎枝生成链路补全后补 spawn_creaking/resin_clump 测试。
// 不测「自然生成 NATURAL=true 经验掉落」：自然生成结构未实现。
//   TODO: 待自然生成结构补全后补 natural_drops_experience 测试。
//
// 跨服务端：creaking_heart 方块名两端一致。axis/creaking_heart_state/natural state 名两端一致。
//   放置默认 state（axis=clickedFace.getAxis() + uprooted + natural=false）行为与 vanilla 一致，
//   可跨服务端对比。useItemOnBlock 放置（lookAtLocation 是 Cubium 专有朝向控制，但 axis 按 clickedFace
//   放置行为两端可对比，非 one-sided）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_嘎枝之心.txt#用途（放置方向类似原木，初始失活状态）
// Ref: CreakingHeartBlock.java:144-147（getStateForPlacement AXIS=clickedFace.getAxis() + updateState）
// Ref: CreakingHeartBlock.java:106-117（updateState hasRequiredLogs + CREAKING_ACTIVE 切换）
// Ref: CreakingHeartBlock.cpp:72-94（getStateForPlacement 对齐 vanilla + TODO 标注未实现链路）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Direction 参数=clickedFace 原样透传 getClickedFace）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 嘎枝之心放 (3,2,1)，下方 (3,1,1) 放 stone 支撑。

const HEART_POS = { x: 3, y: 2, z: 1 };
const STONE_POS = { x: 3, y: 1, z: 1 };
// 玩家位置（远离落点避免碰撞，朝向不影响场景 1 顶面放置）。
const PLAYER_POS = { x: 1, y: 2, z: 1 };

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取嘎枝之心 axis state（"x"/"y"/"z"）。返回 null 表示读取失败或非嘎枝之心。
function getHeartAxis(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("axis" as any);
    return typeof value === "string" ? value : null;
}

// 读取嘎枝之心 creaking_heart_state state（"uprooted"/"dormant"/"awake"）。返回 null 表示失败。
function getHeartState(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("creaking_heart_state" as any);
    return typeof value === "string" ? value : null;
}

// 读取嘎枝之心 natural state（boolean）。返回 null 表示失败或非嘎枝之心。
function getHeartNatural(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("natural" as any);
    return typeof value === "boolean" ? value : null;
}

// 场景 1：玩家手持 creaking_heart 点击 stone 顶面 Up → 放置后验证三个默认 state 全对齐 vanilla。
//   - AXIS=Y（点击顶面 Up → getAxis(Up)=Axis::Y）
//   - creaking_heart_state=uprooted（放置默认失活，对齐 vanilla）
//   - natural=false（玩家放置非自然生成，对齐 vanilla）
//
// 判定：useItemOnBlock 返 true（放置成功）+ typeId === "minecraft:creaking_heart"
//   + axis === "y" + state === "uprooted" + natural === false。
//   - 旧实现会失败：NATURAL=true（应 false）、STATE=dormant（应 uprooted）。
//   - 新实现全对齐 vanilla。
function creakingHeartDefaultStateWhenPlacedOnTop(test: Test): void {
    test.setBlockType("minecraft:stone", STONE_POS);
    test.assert(getBlockTypeId(test, STONE_POS.x, STONE_POS.y, STONE_POS.z) === "minecraft:stone", `stone should be at ${JSON.stringify(STONE_POS)}, got ${getBlockTypeId(test, STONE_POS.x, STONE_POS.y, STONE_POS.z)}`);

    const player = test.spawnSimulatedPlayer(PLAYER_POS, "farmer");
    const heartItem = new ItemStack("minecraft:creaking_heart", 1);

    // 点击 stone 顶面 Up → 嘎枝之心落 (3,2,1)。getClickedFace()=Up → getAxis(Up)=Y。
    const used = player.useItemOnBlock(
        heartItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        STONE_POS,
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when placing creaking_heart on top");

    // 轮询断言放置成功 + 三个 state 对齐 vanilla（useItemOnBlock 同步放置，pollUntilSucceed 留余量）。
    pollUntilSucceed(
        test,
        () => {
            return getBlockTypeId(test, HEART_POS.x, HEART_POS.y, HEART_POS.z) === "minecraft:creaking_heart"
                && getHeartAxis(test, HEART_POS.x, HEART_POS.y, HEART_POS.z) === "y"
                && getHeartState(test, HEART_POS.x, HEART_POS.y, HEART_POS.z) === "uprooted"
                && getHeartNatural(test, HEART_POS.x, HEART_POS.y, HEART_POS.z) === false;
        },
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(
                    false,
                    `creaking_heart default state wrong: typeId=${getBlockTypeId(test, HEART_POS.x, HEART_POS.y, HEART_POS.z)} `
                        + `axis=${getHeartAxis(test, HEART_POS.x, HEART_POS.y, HEART_POS.z)} `
                        + `state=${getHeartState(test, HEART_POS.x, HEART_POS.y, HEART_POS.z)} `
                        + `natural=${getHeartNatural(test, HEART_POS.x, HEART_POS.y, HEART_POS.z)} `
                        + `(expected: creaking_heart / axis=y / state=uprooted / natural=false)`,
                );
            },
        },
    );
}

// 场景 2：点击 stone 侧面 East → AXIS=X（点击面轴向 X），验证 AXIS 按 clickedFace.getAxis() 设置。
//
// 布局：stone 放 (4,2,1)，玩家 (1,2,1) 朝东 click stone 的 East 面（+X 方向）→ 嘎枝之心落 (5,2,1)。
//   玩家朝东 lookAt East，但 AXIS 仅由 clickedFace（East→Axis::X）决定，与玩家朝向无关。
//   旧实现按玩家朝向（horizontalDirection）设 axis：玩家朝东→axis=X，碰巧与 East 一致；
//   为与旧实现区分，本场景玩家朝南（horizontalDirection=South→Z）click East 面：
//   - 旧实现：axis=Z（按玩家朝向南）—— 错误
//   - 新实现：axis=X（按点击面 East）—— 对齐 vanilla
//
// 判定：useItemOnBlock 返 true + typeId === "minecraft:creaking_heart" + axis === "x"。
function creakingHeartAxisFromClickedFace(test: Test): void {
    // stone 放 (4,2,1)（被点击方块，East 侧落点为 (5,2,1)）。
    const stonePos = { x: 4, y: 2, z: 1 };
    const heartPos = { x: 5, y: 2, z: 1 };
    test.setBlockType("minecraft:stone", stonePos);
    test.assert(getBlockTypeId(test, stonePos.x, stonePos.y, stonePos.z) === "minecraft:stone", `stone should be at ${JSON.stringify(stonePos)}, got ${getBlockTypeId(test, stonePos.x, stonePos.y, stonePos.z)}`);

    const player = test.spawnSimulatedPlayer(PLAYER_POS, "farmer");
    // 玩家朝南 lookAt（+Z 方向），horizontalDirection=South（旧实现会设 axis=Z）。
    // 但 AXIS 按 clickedFace=East 决定，与玩家朝向无关 → 新实现 axis=X。
    player.lookAtLocation({ x: 1, y: 3, z: 6 });

    const heartItem = new ItemStack("minecraft:creaking_heart", 1);
    // 点击 stone 的 East 面 → 嘎枝之心落 stonePos.offset(East) = (5,2,1)。getClickedFace()=East → getAxis=X。
    const used = player.useItemOnBlock(
        heartItem as unknown as Parameters<typeof player.useItemOnBlock>[0],
        stonePos,
        Direction.East,
    );
    test.assert(used, "useItemOnBlock should return true when placing creaking_heart on east face");

    pollUntilSucceed(
        test,
        () => {
            return getBlockTypeId(test, heartPos.x, heartPos.y, heartPos.z) === "minecraft:creaking_heart"
                && getHeartAxis(test, heartPos.x, heartPos.y, heartPos.z) === "x";
        },
        {
            startTick: 2,
            interval: 4,
            maxTick: 30,
            onTimeout: () => {
                test.assert(
                    false,
                    `creaking_heart axis wrong: typeId=${getBlockTypeId(test, heartPos.x, heartPos.y, heartPos.z)} `
                        + `axis=${getHeartAxis(test, heartPos.x, heartPos.y, heartPos.z)} `
                        + `(expected: creaking_heart / axis=x [from clicked face East], `
                        + `not axis=z [old impl by player horizontal direction South])`,
                );
            },
        },
    );
}

export function registerCreakingHeartTests(): void {
    GameTest.register("BlockBehaviorTests", "creaking_heart_default_state_when_placed_on_top", creakingHeartDefaultStateWhenPlacedOnTop)
        .structureName("gametests:glass_pit")
        .maxTicks(60);

    GameTest.register("BlockBehaviorTests", "creaking_heart_axis_from_clicked_face", creakingHeartAxisFromClickedFace)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
