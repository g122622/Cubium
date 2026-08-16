// 铜傀儡像姿势切换行为 GameTest。
//
// wiki other_铜傀儡像.txt#姿势：铜傀儡像一共有 4 种姿势——"站立""坐下""奔跑""大字"。玩家空手对着
//   铜傀儡像按下使用键可改变其姿势。红石比较器与之连接时发出红石信号，4 种姿势分别对应 1-4 的信号强度。
//   铜傀儡像于 Java 1.21.9（25w31a）/基岩 1.21.111 加入，1.21.11 已包含，属 vanilla 正式特性（非试验性）。
//
// C++ 链路：CopperGolemStatueBlock（copper/CopperGolemStatueBlock.cpp）有 HORIZONTAL_FACING +
//   COPPER_GOLEM_POSE + WATERLOGGED 三个 state。
//   - onBlockActivated（:153-226）：取手持物，若为斧头走斧头分支（基础变体生成铜傀儡 / 涂蜡变体 Pass
//     让 AxeItem 去蜡）；非斧头（含空手）→ updatePose 循环切换姿态 → Success。
//   - updatePose（:274-293）：getNextPose(current) 算下一姿态，state.with(COPPER_GOLEM_POSE, nextPose)
//     setBlockState(flags=3) + 音效 + BLOCK_CHANGE 事件。
//   - getNextPose（:253-272）：Standing→Sitting→Running→Star→Standing 循环（对应 MC Java
//     Pose.getNextPose = BY_ID.apply(ordinal+1)，OutOfBoundsStrategy.ZERO 超范围回 0）。
//   - getComparatorInputOverride（:228-235）：返 POSE.ordinal()+1（1-4），对应 wiki 比较器信号。
//   - 默认 state（:88-92）：facing=North, pose=Standing, waterlogged=false。
//   - pose 枚举字符串（EnumProperty.cpp:931-944）：standing/sitting/running/star。
//
// 派发链路：interactWithBlock（空手右键）已补全（ScriptSimulatedPlayer.cpp），复用 useItemOnBlock(空
//   ItemStack)，空堆只走 ① Block.use(onBlockActivated)，不走 ② Item.useOn。铜傀儡像 onBlockActivated
//   非斧头分支 updatePose 返 Success，短路不 fallback。需用新 spawn 的 SimulatedPlayer（选中槽为空）
//   确保空手——若先前 useItemOnBlock 设过选中槽物品会污染空手判定（同 CandleTests 场景3 模式）。
//
// 测试覆盖（2 个场景，覆盖 wiki 4 种姿势循环切换核心确定行为）：
//   1. 单次切换：铜傀儡像（standing）+ interactWithBlock（空手右键）→ pose 翻 sitting，返 true。
//   2. 完整循环：连续 4 次 interactWithBlock → standing→sitting→running→star→standing（验证 4 姿态
//      全覆盖 + 循环回绕到 standing）。
//
// 关键约束：
// 1. 铜傀儡像无支撑要求（无 isValidPosition 重写，updatePostPlacement 不自毁），放 (3,2,1) 即可，
//    下方 (3,1,1) 放 stone 支撑仅为惯例（与现有测试一致）。
// 2. 读 pose state 用 getState("copper_golem_pose" as any) 绕过 BlockStateSuperset 白名单，返回
//    枚举名字符串（standing/sitting/running/star）。
// 3. 空手右键用 interactWithBlock（Cubium 补全的 SimulatedPlayer 方法，类型未声明，用 as any 绕过）。
//    用新 spawn 的 SimulatedPlayer 确保选中槽为空（空手），避免先前操作污染。
// 4. 创造模式 SimulatedPlayer mayBuild=true，满足 onBlockActivated 无额外前置（铜傀儡像不检查 mayBuild）。
//
// 不测「斧头敲击生成铜傀儡」：走 onBlockActivated 斧头分支 → BlockEntity.removeStatue → spawnEntity
//   生成铜傀儡 + setBlockState(air)。涉铜傀儡实体完整性 + spawnEntity 链路，复杂度高，跳过。
//   TODO: 待铜傀儡实体 spawn 链路验证后补 copper_golem_statue_spawns_golem_with_axe。
// 不测「比较器信号 1-4」：需放置比较器方块 + 红石信号判定，复杂度高，且 pose state 已断言姿势切换。
//   跳过。TODO: 待红石比较器读取链路便捷后补。
// 不测「氧化/涂蜡」：受 randomTick 氧化机制影响（非确定），跳过。
// 不测「涂蜡变体斧头去蜡」：走 AxeItem.onItemUse 去蜡链路，涉 HoneycombItem::getWaxedOff，复杂，
//   跳过。TODO: 待去蜡链路验证后补。
//
// 跨服务端：铜傀儡像 copper_golem_statue 方块名两端一致（Java 1.21.9/基岩 1.21.111 加入），pose state
//   行为与 vanilla 一致。空手右键姿势循环两端可对比。
//   注意：基岩 BDS 无 interactWithBlock（Cubium 补全），场景在基岩侧为 one-sided（仅 Cubium 跑）。
//   但姿势循环行为本身（onBlockActivated 非斧头 updatePose）两端语义一致，Cubium 验证即可。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_铜傀儡像.txt#姿势（空手使用键切换 4 种姿势）
// Ref: CopperGolemStatueBlock.cpp（onBlockActivated 非斧头 updatePose；getNextPose Standing→Sitting→Running→Star→Standing）
// Ref: Properties.hpp（CopperGolemPose 枚举 Standing/Sitting/Running/Star）
// Ref: EnumProperty.cpp（pose 字符串 standing/sitting/running/star）
// Ref: SimulatedPlayer.cpp（interactWithBlock 空手右键绑定，复用 useItemOnBlock 空堆路径）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 铜傀儡像 (3,2,1)，下方 (3,1,1) stone 支撑（惯例，铜傀儡像无强制支撑要求）。

// 读取铜傀儡像 pose state（枚举名字符串 standing/sitting/running/star）。返回 null 表示读取失败或非铜傀儡像。
function getStatuePose(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("copper_golem_pose" as any);
    return typeof value === "string" ? value : null;
}

// 放支撑 + 铜傀儡像：(3,1,1) stone 支撑，(3,2,1) 铜傀儡像（minecraft:copper_golem_statue 默认 pose=standing）。
function placeStatue(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:copper_golem_statue", { x: 3, y: 2, z: 1 }); // 铜傀儡像 pose=standing
}

// 场景 1：单次切换——铜傀儡像（standing）+ interactWithBlock（空手右键）→ pose 翻 sitting，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 铜傀儡像 pose=standing。
// interactWithBlock（空手右键）→ onBlockActivated：heldItem 空手（非斧头）→ updatePose →
//   getNextPose(Standing)=Sitting → with(COPPER_GOLEM_POSE, Sitting) setBlockState → Success。
//
// 判定：interactWithBlock 返 true（Success），pose === "sitting"（从 standing 翻到 sitting）。
function statuePoseChangesOnInteract(test: Test): void {
    placeStatue(test);
    test.assert(getStatuePose(test, 3, 2, 1) === "standing", `statue pose should be standing before, got ${getStatuePose(test, 3, 2, 1)}`);

    // 新 spawn SimulatedPlayer 确保选中槽为空（空手），避免先前操作污染。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");

    // interactWithBlock 空手右键铜傀儡像 → onBlockActivated 非斧头 → updatePose(Standing→Sitting) → Success。
    // interactWithBlock 为 Cubium 补全的 SimulatedPlayer 方法（类型定义未声明），用 as any 绕过类型检查。
    const used = (farmer as unknown as { interactWithBlock: (pos: unknown, dir: unknown) => boolean })
        .interactWithBlock({ x: 3, y: 2, z: 1 }, Direction.Up);
    test.assert(used, "interactWithBlock should return true when changing statue pose");

    // 判定：pose === "sitting"（从 standing 单次翻转到 sitting）。
    test.assert(getStatuePose(test, 3, 2, 1) === "sitting", `statue pose should be sitting after interact, got ${getStatuePose(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：完整循环——连续 4 次 interactWithBlock → standing→sitting→running→star→standing。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 铜傀儡像 pose=standing。
// 每次 interactWithBlock → updatePose → getNextPose 循环：standing→sitting→running→star→standing。
//   第 4 次从 star 回绕到 standing（对应 MC Java BY_ID OutOfBoundsStrategy.ZERO 超范围回 0）。
//
// 判定：4 次后 pose === "standing"（完整循环回绕到起点），且每次中间态符合预期顺序。
function statuePoseCyclesThroughAllPoses(test: Test): void {
    placeStatue(test);
    test.assert(getStatuePose(test, 3, 2, 1) === "standing", `statue pose should be standing before, got ${getStatuePose(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const interact = (farmer as unknown as { interactWithBlock: (pos: unknown, dir: unknown) => boolean });

    // wiki 4 种姿势循环顺序：standing→sitting→running→star→standing。
    const expectedPoses = ["sitting", "running", "star", "standing"];
    for (let i = 0; i < expectedPoses.length; ++i) {
        const used = interact.interactWithBlock({ x: 3, y: 2, z: 1 }, Direction.Up);
        test.assert(used, `interactWithBlock should return true on pose change #${i + 1}`);
        const expected = expectedPoses[i];
        test.assert(getStatuePose(test, 3, 2, 1) === expected, `statue pose should be ${expected} after interact #${i + 1}, got ${getStatuePose(test, 3, 2, 1)}`);
    }

    // 判定：4 次后 pose === "standing"（完整循环回绕到起点，覆盖 wiki 全部 4 种姿势）。
    test.assert(getStatuePose(test, 3, 2, 1) === "standing", `statue pose should cycle back to standing after 4 interacts, got ${getStatuePose(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerCopperGolemStatueTests(): void {
    GameTest.register("BlockBehaviorTests", "copper_golem_statue_pose_changes_on_interact", statuePoseChangesOnInteract)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "copper_golem_statue_pose_cycles_through_all_poses", statuePoseCyclesThroughAllPoses)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
