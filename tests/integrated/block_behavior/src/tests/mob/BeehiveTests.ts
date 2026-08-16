// 蜂巢/蜂箱右键刮满蜂蜜行为 GameTest。
//
// wiki tech_蜜蜂.txt#行为：当蜂巢 honey_level 达到 5（充满蜂蜜，向下滴蜜粒子）时，玩家可从中收集：
//   - 剪刀：收集 3 个蜜脾（honeycomb），honey_level→0。
//   - 玻璃瓶：消耗 1 玻璃瓶，获得 1 蜂蜜瓶，honey_level→0。
//   收集会使巢内蜜蜂在激怒状态下被弹出；但在蜂巢正下方 6 格内放置营火/灵魂营火（烟熏）可避免激怒
//   （此时仅 honey_level→0，不激怒蜜蜂）。空蜂巢（无蜜蜂）收集无激怒副作用。
//
// C++ 链路：BeehiveBlock（mob/BeehiveBlock.cpp）有 HONEY_LEVEL_0_5 state（默认 0）。
//   - onBlockActivated（:150）：honeyLevel<5 → Pass（未满不交互）；honeyLevel==5 时：
//     · 剪刀 → dropHoneycomb（掉 3 蜜脾）+ 消耗剪刀耐久 + 音效 → success=true。
//     · 玻璃瓶 → 非创造 shrink(1) 消耗玻璃瓶 + 给蜂蜜瓶（替换/入背包/掉落）+ 音效 → success=true。
//     success 后：!isSmokeyPos（下方无营火烟熏）→ hiveContainsBees?则 angerNearbyBees +
//       releaseBeesAndResetHoneyLevel（Emergency）；否则 resetHoneyLevel。两分支都 honey_level→0。
//   - resetHoneyLevel（:242）：withHoneyLevel(state,0) setBlockState 写回。
//   - 空蜂巢（无 BeehiveBlockEntity 蜜蜂数据）hiveContainsBees=false，不 angerNearbyBees，
//     releaseBeesAndResetHoneyLevel 内 emptyAllLivingFromHive 空操作，honey_level→0 无副作用。
//   - HONEY_LEVEL state 名 "honey_level"（Properties.hpp HONEY_LEVEL_0_5 = IntegerProperty("honey_level",0,5)）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock Block.use 前置分支（先 onBlockActivated，Pass 才 fallback）。
//   蜂巢 honey_level=5 + 剪刀/玻璃瓶 → onBlockActivated Success 短路（不走 fallback）。
//
// 测试覆盖（2 个场景，覆盖 wiki 剪刀刮蜜脾 + 玻璃瓶取蜂蜜瓶核心确定行为）：
//   1. 剪刀刮满蜂蜜：放 honey_level=5 蜂巢 + 剪刀 useItemOnBlock → honey_level=0，返 true。
//   2. 玻璃瓶取蜂蜜：放 honey_level=5 蜂巢 + 玻璃瓶 useItemOnBlock → honey_level=0，返 true。
//
// 关键约束：
// 1. 蜂巢完整方块（Material::WOOD），无 canSurvive 自毁，放 (3,2,1)（minecraft:beehive）。
//    setBlockType 只能放默认 honey_level=0，而刮蜂蜜需 honey_level=5，故用 Cubium 专有 setBlockWithStates
//    放 "honey_level=5" 蜂巢（弥补 setBlockType 不足，见 cubium-gametest-augment.d.ts）。
// 2. 读 honey_level state 用 getState("honey_level" as any) 绕过 BlockStateSuperset 白名单。
// 3. 空蜂巢无蜜蜂：setBlockWithStates 放置创建空 BeehiveBlockEntity，hiveContainsBees=false，刮蜂蜜
//    走 resetHoneyLevel（无营火分支 releaseBeesAndResetHoneyLevel，但无蜜蜂无激怒副作用）。
//    故不放置营火也能安全测试（无蜜蜂可激怒/弹出）。
// 4. 剪刀创造模式不消耗耐久（attemptDamageItem 创造守卫），可重复使用；玻璃瓶创造模式不 shrink
//    （creativeMode 守卫），可重复使用。两场景各 new ItemStack 重新设入选中槽。
// 5. 蜂巢 bee_nest 与 beehive 共用 BeehiveBlock 类（仅注册名不同），行为一致；本组用 beehive（人造蜂箱）。
//
// 不测「蜜脾/蜂蜜瓶掉落物」：掉落物实体生成非确定（位置/时序），且本组聚焦 honey_level state 变化，
//   跳过。TODO: 待掉落物断言 API 完善后补蜜脾数量/蜂蜜瓶入背包测试。
// 不测「无营火激怒蜜蜂」：需蜂巢内有蜜蜂（BeehiveBlockEntity bees 数据），构造复杂且激怒/攻击非确定，
//   跳过。TODO: 待蜂巢蜜蜂数据可控后补。
// 不测「营火烟熏避免激怒」：同上需蜜蜂，跳过。
// 不测「honey_level<5 时右键无反应」：onBlockActivated honeyLevel<5 返 Pass，useItemOnBlock 走 fallback
//   （剪刀/玻璃瓶 onItemUse 无放置逻辑返 Pass/Fail），返 false。可测但语义弱（返 false 多因），跳过。
//
// 跨服务端：蜂巢 beehive 方块名两端一致，honey_level state 名两端一致，剪刀/玻璃瓶刮满蜂蜜 honey_level→0
//   行为与 vanilla 一致。但 setBlockWithStates 是 Cubium 专有（基岩 BDS Test 无此 API），本组测试为
//   one-sided（仅 Cubium 可跑，基岩因无法放 honey_level=5 蜂巢而跳过/不可对比）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蜜蜂.txt#行为（honey_level=5 可收集，剪刀3蜜脾/玻璃瓶蜂蜜瓶，→0）
// Ref: BeehiveBlock.cpp（onBlockActivated honeyLevel==5 剪刀/玻璃瓶→resetHoneyLevel→Success；HONEY_LEVEL_0_5 state）
// Ref: cubium-gametest-augment.d.ts（setBlockWithStates Cubium 专有，放带 state 方块）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 蜂巢 (3,2,1)，完整方块无需支撑。

// 读取蜂巢 honey_level state（number 0-5）。返回 null 表示读取失败或非蜂巢。
// honey_level state 名 "honey_level"（Java 命名，见 Properties.hpp HONEY_LEVEL_0_5 = IntegerProperty("honey_level",0,5)）。
function getBeehiveHoneyLevel(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("honey_level" as any);
    return typeof value === "number" ? value : null;
}

// 放满蜂蜜蜂巢：(3,2,1) 蜂巢 honey_level=5（用 Cubium 专有 setBlockWithStates 放带 state 方块）。
function placeFullBeehive(test: Test): void {
    (test as unknown as {
        setBlockWithStates(blockType: string, loc: { x: number; y: number; z: number }, statesStr: string): void;
    }).setBlockWithStates("minecraft:beehive", { x: 3, y: 2, z: 1 }, "honey_level=5");
}

// 场景 1：剪刀刮满蜂蜜——放 honey_level=5 蜂巢 + 剪刀 useItemOnBlock → honey_level=0，返 true。
//
// 布局：(3,2,1) 蜂巢 honey_level=5（空蜂巢无蜜蜂）。
// onBlockActivated：honeyLevel==5 + 剪刀 → dropHoneycomb(3蜜脾) + 消耗耐久 + 音效 → success=true →
//   !isSmokeyPos（无营火）→ hiveContainsBees=false（空蜂巢）→ releaseBeesAndResetHoneyLevel
//   （Emergency，无蜜蜂空操作）→ honey_level→0 → Success。
//
// 判定：useItemOnBlock 返 true（Success），honey_level === 0（剪刀刮蜜后蜂蜜归零）。
function beehiveShearsHarvestsHoney(test: Test): void {
    placeFullBeehive(test);
    test.assert(getBeehiveHoneyLevel(test, 3, 2, 1) === 5, `beehive honey_level should be 5 before, got ${getBeehiveHoneyLevel(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const shears = new ItemStack("minecraft:shears", 1);

    // 对蜂巢 useItemOnBlock 剪刀 → onBlockActivated honeyLevel==5 剪刀 → dropHoneycomb + resetHoneyLevel → Success。
    const used = farmer.useItemOnBlock(
        shears as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when shearing full beehive");

    // 判定：honey_level === 0（剪刀刮蜜脾后蜂蜜归零）。
    test.assert(getBeehiveHoneyLevel(test, 3, 2, 1) === 0, `beehive honey_level should be 0 after shearing, got ${getBeehiveHoneyLevel(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：玻璃瓶取蜂蜜——放 honey_level=5 蜂巢 + 玻璃瓶 useItemOnBlock → honey_level=0，返 true。
//
// 布局：(3,2,1) 蜂巢 honey_level=5（空蜂巢无蜜蜂）。
// onBlockActivated：honeyLevel==5 + 玻璃瓶 → 非创造 shrink(1)（创造不消耗）+ 给蜂蜜瓶 + 音效 →
//   success=true → !isSmokeyPos → releaseBeesAndResetHoneyLevel（空操作）→ honey_level→0 → Success。
//
// 判定：useItemOnBlock 返 true（Success），honey_level === 0（玻璃瓶取蜜后蜂蜜归零）。
function beehiveBottleHarvestsHoney(test: Test): void {
    placeFullBeehive(test);
    test.assert(getBeehiveHoneyLevel(test, 3, 2, 1) === 5, `beehive honey_level should be 5 before, got ${getBeehiveHoneyLevel(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const bottle = new ItemStack("minecraft:glass_bottle", 1);

    // 对蜂巢 useItemOnBlock 玻璃瓶 → onBlockActivated honeyLevel==5 玻璃瓶 → 给蜂蜜瓶 + resetHoneyLevel → Success。
    const used = farmer.useItemOnBlock(
        bottle as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when harvesting honey with bottle");

    // 判定：honey_level === 0（玻璃瓶取蜂蜜后蜂蜜归零）。
    test.assert(getBeehiveHoneyLevel(test, 3, 2, 1) === 0, `beehive honey_level should be 0 after bottle harvest, got ${getBeehiveHoneyLevel(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerBeehiveTests(): void {
    GameTest.register("BlockBehaviorTests", "beehive_shears_harvests_honey", beehiveShearsHarvestsHoney)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "beehive_bottle_harvests_honey", beehiveBottleHarvestsHoney)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
