// 蛋糕吃蛋糕行为 GameTest。
//
// wiki tech_蛋糕.txt#用途：蛋糕是可食用方块，可分 7 片食用。右键吃一片（bites+1），每片恢复
//   2 点饥饿值和 0.1 饱和度；bites 达 6 时再吃移除蛋糕方块。创造模式/满饥饿无法进食。
//   吃蛋糕不消耗手持物（空手右键即可）。
//
// C++ 链路：CakeBlock（functional/CakeBlock.cpp）有 BITES_0_6 state（默认 0）。
//   - onBlockActivated（已补全，对齐 vanilla CakeBlock.use）：
//     · canEat(false) 创造/旁观返 false，否则 needsFood（foodLevel<20）。
//     · canEat 通过 → eatSlice（bites<6 时 setBlockState bites+1；bites==6 时 setBlockState air）
//       + player.foodStats().addStats(2, 0.1f)（恢复饥饿）→ return Success。
//     · canEat 不通过 → return Pass（创造/满饥饿吃不了）。
//   - eatSlice（:178-192）：bites<6 → bites+1 写回；bites==6 → 移除方块（air）。
//   - 此前 CakeBlock 未 override onBlockActivated（基类返 Pass），蛋糕完全无法吃——生产 bug，已修复。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。蛋糕 onBlockActivated canEat 通过返 Success 短路；canEat 不通过返 Pass 后
//   fallback（手持 stick 普通 Item，onItemUse 默认 Pass）→ useItemOnBlock 返 false。
//   吃蛋糕不消耗手持物，用手持 stick 触发（onBlockActivated 不检查 hand/heldItem，canEat 通过即吃）。
//
// 饥饿管理：SimulatedPlayer 默认创造模式（canEat 创造 false 吃不了），需切生存模式（spawn 第 3 参
//   gameMode=0）。生存模式初始 foodLevel=20 满饥饿（canEat false 吃不了），生存模式无 OP 权限无法
//   /effect 降饥饿，故用 Cubium 测试扩展 SimulatedPlayer.setFoodLevel(level) 直接设定饥饿值，确定性
//   验证「饥饿<20 才能吃」。setFoodLevel 是 Cubium 扩展（基岩 SimulatedPlayer 无此 API），TS 侧用
//   (player as any).setFoodLevel(n) 调用（无类型声明）。
//
// 测试覆盖（4 个场景，覆盖 wiki 吃蛋糕/bites递增/最后一片移除/创造满饥饿吃不了核心行为）：
//   1. 生存饥饿吃蛋糕：生存 + setFoodLevel(10) + stick useItemOnBlock → bites 0→1，返 true。
//   2. 满饥饿吃不了：生存 + setFoodLevel(20) + stick useItemOnBlock → bites 仍 0，返 false。
//   3. 创造模式吃不了：创造（默认）+ stick useItemOnBlock → bites 仍 0，返 false。
//   4. 吃到最后一片移除：生存 + setFoodLevel(0) + 连续 7 次 useItemOnBlock → 蛋糕位置变 air。
//
// 关键约束：
// 1. 蛋糕需放在固体方块上方（isValidPosition 检查 belowState.isSolid）——(3,1,1) 放 stone 支撑，
//    (3,2,1) 放蛋糕（minecraft:cake 默认 bites=0）。
// 2. 读 bites state 用 getState("bites" as any) 绕过 BlockStateSuperset 白名单。
// 3. 切生存模式：spawnSimulatedPlayer 第 3 参 gameMode=0（Survival）。
// 4. setFoodLevel 用 (player as any).setFoodLevel(n)（Cubium 扩展，无 TS 类型声明）。
// 5. 吃蛋糕用手持 stick 触发（onBlockActivated 不检查手持物，canEat 通过即吃，stick 不被消耗）。
//    不能空手（useItemOnBlock 强制要 ItemStack），stick 是普通 Item 不影响判定。
// 6. 场景 4 连续吃 7 次：bites 0→1→...→6，第 7 次 bites=6 → eatSlice 移除蛋糕变 air。每次吃恢复
//    饥饿+2，但 setFoodLevel(0) 后 7 次吃最多恢复到 14，仍 <20，canEat 始终通过。
//
// 不测「比较器输出」：脚本侧无直接读比较器输出 API，跳过。比较器输出=(7-bites)*2 由 getComparatorInputOverride。
//   TODO: 待比较器读取链路打通后补 bites=3 比较器输出=8 测试。
//
// 跨服务端：蛋糕 cake 方块名两端一致，bites state 行为与 vanilla 一致。注意 setFoodLevel 是 Cubium
//   扩展，基岩 BDS 无此 API，跨端对比时基岩端本组测试需改用 /effect 降饥饿（需 OP 权限）或跳过。
//   吃蛋糕 bites 递增 + 最后一片移除行为两端可对比（基岩端需手动控饥饿）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蛋糕.txt#用途（吃蛋糕，bites递增，最后一片移除，每片恢复2饥饿）
// Ref: CakeBlock.cpp（onBlockActivated canEat→eatSlice+addStats；eatSlice bites<6递增/bites==6移除）
// Ref: Player.cpp:2346（canEat 创造/旁观 false，否则 needsFood foodLevel<20）
// Ref: FoodStats.cpp（addStats(2,0.1f) 恢复饥饿；setFoodLevel clamp [0,20]）
// Ref: SimulatedPlayer.cpp（setFoodLevel 转发 foodStats().setFoodLevel，测试扩展）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 蛋糕 (3,2,1)，下方 (3,1,1) stone 支撑（蛋糕需 solid 上方放置）。

// 读取蛋糕 bites state（number 0-6）。返回 null 表示读取失败或非蛋糕。
function getCakeBites(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("bites" as any);
    return typeof value === "number" ? value : null;
}

// 读取 (x,y,z) 方块 typeId。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 放支撑 + 蛋糕：(3,1,1) stone 支撑，(3,2,1) 蛋糕（minecraft:cake 默认 bites=0）。
function placeCake(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:cake", { x: 3, y: 2, z: 1 }); // 蛋糕 bites=0
}

// 场景 1：生存饥饿吃蛋糕——生存 + setFoodLevel(10) + stick useItemOnBlock → bites 0→1，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 蛋糕 bites=0。
// 生存模式 setFoodLevel(10)（foodLevel=10<20）→ canEat(true) → eatSlice(bites 0→1) + addStats(2,0.1)
// → return Success。
//
// 判定：useItemOnBlock 返 true（Success），bites === 1（吃了一片）。
function cakeEatenWhenHungrySurvival(test: Test): void {
    placeCake(test);
    test.assert(getCakeBites(test, 3, 2, 1) === 0, `cake bites should be 0 before, got ${getCakeBites(test, 3, 2, 1)}`);

    // 生存模式（gameMode=0）spawn，setFoodLevel(10) 使 foodLevel<20 可进食。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer", 0 as any);
    (farmer as any).setFoodLevel(10);

    const stick = new ItemStack("minecraft:stick", 1);
    // 对蛋糕 useItemOnBlock stick → onBlockActivated canEat(10<20) → eatSlice bites 0→1 → Success。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when eating cake (hungry survival)");

    // 判定：bites === 1（吃了一片）。
    test.assert(getCakeBites(test, 3, 2, 1) === 1, `cake bites should be 1 after eating, got ${getCakeBites(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：满饥饿吃不了——生存 + setFoodLevel(20) + stick useItemOnBlock → bites 仍 0，返 false。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 蛋糕 bites=0。
// 生存模式 setFoodLevel(20)（满饥饿 foodLevel=20）→ canEat(false)（needsFood false）→ return Pass。
// Pass → fallback Item.useOn（stick 普通 Item，onItemUse 默认 Pass）→ useItemOnBlock 返 false。
//
// 判定：useItemOnBlock 返 false（满饥饿吃不了），bites === 0（未吃）。
function cakeNotEatenWhenFullHunger(test: Test): void {
    placeCake(test);
    test.assert(getCakeBites(test, 3, 2, 1) === 0, `cake bites should be 0 before, got ${getCakeBites(test, 3, 2, 1)}`);

    // 生存模式 setFoodLevel(20) 满饥饿 → canEat false。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer", 0 as any);
    (farmer as any).setFoodLevel(20);

    const stick = new ItemStack("minecraft:stick", 1);
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(!used, `useItemOnBlock should return false when full hunger, got ${used}`);

    // 判定：bites === 0（满饥饿吃不了，蛋糕未动）。
    test.assert(getCakeBites(test, 3, 2, 1) === 0, `cake bites should remain 0 when full hunger, got ${getCakeBites(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：创造模式吃不了——创造（默认）+ stick useItemOnBlock → bites 仍 0，返 false。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 蛋糕 bites=0。
// 创造模式（默认 spawn 不传 gameMode）→ canEat(false)（isCreative true）→ return Pass → fallback → false。
//
// 判定：useItemOnBlock 返 false（创造模式吃不了），bites === 0（未吃）。
function cakeNotEatenInCreativeMode(test: Test): void {
    placeCake(test);
    test.assert(getCakeBites(test, 3, 2, 1) === 0, `cake bites should be 0 before, got ${getCakeBites(test, 3, 2, 1)}`);

    // 创造模式（默认，不传 gameMode）。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");

    const stick = new ItemStack("minecraft:stick", 1);
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(!used, `useItemOnBlock should return false in creative mode, got ${used}`);

    // 判定：bites === 0（创造模式吃不了，蛋糕未动）。
    test.assert(getCakeBites(test, 3, 2, 1) === 0, `cake bites should remain 0 in creative mode, got ${getCakeBites(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 4：吃到最后一片移除——生存 + setFoodLevel(0) + 连续 7 次 useItemOnBlock → 蛋糕位置变 air。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 蛋糕 bites=0。
// 生存模式 setFoodLevel(0)（foodLevel=0<20，canEat 始终通过，每次吃恢复+2 最多到 14 仍<20）。
// 连续 7 次 useItemOnBlock：bites 0→1→2→3→4→5→6，第 7 次 bites=6 → eatSlice 移除蛋糕（setBlockState air）。
//
// 判定：7 次吃完后蛋糕位置 typeId === "minecraft:air"（最后一片移除蛋糕）。
function cakeRemovedAfterEatingAllSlices(test: Test): void {
    placeCake(test);
    test.assert(getCakeBites(test, 3, 2, 1) === 0, `cake bites should be 0 before, got ${getCakeBites(test, 3, 2, 1)}`);

    // 生存模式 setFoodLevel(0)，确保连续 7 次吃 canEat 始终通过（每次+2，最多到 14<20）。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer", 0 as any);
    (farmer as any).setFoodLevel(0);

    const stick = new ItemStack("minecraft:stick", 1);
    // 连续吃 7 片：bites 0→1→2→3→4→5→6，第 7 次 bites=6 → eatSlice 移除蛋糕变 air。
    for (let i = 0; i < 7; ++i) {
        const used = farmer.useItemOnBlock(
            stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
            { x: 3, y: 2, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true on eat #${i + 1} (canEat always true at low hunger)`);
    }

    // 判定：蛋糕位置变为 air（第 7 片 eatSlice bites==6 移除蛋糕）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `cake pos should be air after eating all slices, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerCakeTests(): void {
    GameTest.register("BlockBehaviorTests", "cake_eaten_when_hungry_survival", cakeEatenWhenHungrySurvival)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "cake_not_eaten_when_full_hunger", cakeNotEatenWhenFullHunger)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "cake_not_eaten_in_creative_mode", cakeNotEatenInCreativeMode)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "cake_removed_after_eating_all_slices", cakeRemovedAfterEatingAllSlices)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
