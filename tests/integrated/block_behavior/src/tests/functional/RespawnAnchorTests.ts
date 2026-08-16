// 重生锚萤石充能行为 GameTest。
//
// wiki tech_重生锚.txt#用途：对重生锚使用荧石（萤石）可充能，charges 0→4。每级亮度 +4
//   （charges 1/2/3/4 对应亮度 3/7/11/15）。4 块荧石补满。充能不挑维度（主世界/下界均可充能）。
//   非萤石右键已充能重生锚在非下界维度会爆炸（移除方块+爆炸威力 5），在下界则消耗一次充能设置
//   重生点。本组仅测萤石充能（确定可测），不测爆炸（非确定涉方块破坏）/设置重生点（需下界维度）。
//
// C++ 链路：RespawnAnchorBlock（functional/RespawnAnchorBlock.cpp）有 CHARGES_0_4 state（默认 0）。
//   - onBlockActivated（:153）：手持萤石 + charges<4 → charge(charges+1) + 消耗萤石 shrink(1) +
//     充能音效 → return Success。充能不挑维度（hasGlowstone 分支在维度检查之前）。
//   - charge（:132）：charges<4 → with(CHARGES, charges+1) setBlockState 写回。
//   - 非萤石 + 非下界维度（respawnAnchorWorks=false）→ 爆炸（移除方块+createExplosion）→ Success。
//     GameTest 默认主世界（respawnAnchorWorks=false），故 charges=4 后再用萤石会走爆炸分支（
//     hasGlowstone && charges<4 为 false → 跳过充能 → 非下界爆炸），本组不测此边界。
//   - 非萤石 + 下界 + charges>0 → discharge + setSpawnPoint（设置重生点）。需下界维度，不测。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。重生锚 onBlockActivated 手持萤石返 Success 短路（不走 fallback）。
//   萤石是 BlockItem，但 onBlockActivated 在 fallback 之前已处理，不会走到萤石 onItemUse 放置。
//
// 测试覆盖（2 个场景，覆盖 wiki 萤石充能核心确定行为）：
//   1. 萤石充能一次：放重生锚（charges=0）+ 萤石 useItemOnBlock → charges=1，返 true。
//   2. 连续充能三次：连续 3 次萤石 useItemOnBlock → charges 0→1→2→3。
//
// 关键约束：
// 1. 重生锚完整方块无需支撑，直接放 (3,2,1)（minecraft:respawn_anchor 默认 charges=0）。
// 2. 读 charges state 用 getState("charges" as any) 绕过 BlockStateSuperset 白名单。
//    重生锚 charges state 名为 "charges"（Java 命名，见 Properties.hpp CHARGES_0_4）。
// 3. 萤石用 new ItemStack("minecraft:glowstone", 1)。
// 4. 充能消耗萤石（onBlockActivated 内 shrink(1)），创造模式也消耗（无 isCreative 守卫）。每次循环
//    new ItemStack 重新构造萤石设入选中槽，避免消耗后空手。
// 5. 不充能到 charges=4：charges=4 后再用萤石在主世界会走爆炸分支（非确定），本组停在 charges=3。
//
// 不测「charges=4 满充能后再用萤石」：主世界 charges=4 + 萤石 → 跳过充能 → 爆炸分支（非确定）。
// 不测「非下界爆炸」：涉 createExplosion 方块破坏范围，非确定。
// 不测「下界设置重生点」：GameTest 默认主世界维度，需切换维度 API。
// 不测「亮度等级」：脚本侧无直接读方块亮度 API。
//
// 跨服务端：重生锚 respawn_anchor 方块名两端一致，charges state 行为与 vanilla 一致。萤石 glowstone
//   两端一致。萤石充能 charges 递增行为两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_重生锚.txt#用途（萤石充能 charges 0→4，每级亮度+4）
// Ref: RespawnAnchorBlock.cpp（onBlockActivated 萤石+charges<4→charge+shrink；charge charges+1）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 重生锚 (3,2,1)，完整方块无需支撑。

// 读取重生锚 charges state（number 0-4）。返回 null 表示读取失败。
// 重生锚 charges state 名为 "charges"（Java 命名，见 Properties.hpp CHARGES_0_4 = IntegerProperty("charges",0,4)）。
// 兼容 "respawn_anchor_charges"（基岩命名）回退。
function getRespawnAnchorCharges(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    for (const name of ["charges", "respawn_anchor_charges"] as const) {
        const value = block?.permutation?.getState(name as any);
        if (typeof value === "number") {
            return value;
        }
    }
    return null;
}

// 放重生锚：(3,2,1) 重生锚（minecraft:respawn_anchor 默认 charges=0，完整方块无需支撑）。
function placeRespawnAnchor(test: Test): void {
    test.setBlockType("minecraft:respawn_anchor", { x: 3, y: 2, z: 1 }); // 重生锚 charges=0
}

// 场景 1：萤石充能一次——放重生锚（charges=0）+ 萤石 useItemOnBlock → charges=1，返 true。
//
// 布局：(3,2,1) 重生锚 charges=0。
// onBlockActivated：手持萤石 + charges=0<4 → charge(charges 0→1) + shrink(1) → return Success。
//
// 判定：useItemOnBlock 返 true（Success），charges === 1（充能一次）。
function respawnAnchorChargesWithGlowstone(test: Test): void {
    placeRespawnAnchor(test);
    test.assert(getRespawnAnchorCharges(test, 3, 2, 1) === 0, `anchor charges should be 0 before, got ${getRespawnAnchorCharges(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const glowstone = new ItemStack("minecraft:glowstone", 1);

    // 对重生锚 useItemOnBlock 萤石 → onBlockActivated 萤石+charges<4 → charge charges 0→1 → Success。
    const used = farmer.useItemOnBlock(
        glowstone as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when charging respawn anchor with glowstone");

    // 判定：charges === 1（充能一次）。
    test.assert(getRespawnAnchorCharges(test, 3, 2, 1) === 1, `anchor charges should be 1 after charging, got ${getRespawnAnchorCharges(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：连续充能三次——连续 3 次萤石 useItemOnBlock → charges 0→1→2→3。
//
// 布局：(3,2,1) 重生锚 charges=0。
// 每次萤石 useItemOnBlock → charge(charges+1) + shrink(1)。连续 3 次 charges 0→1→2→3。
// 停在 charges=3（不充到 4：charges=4 后主世界再用萤石走爆炸分支，非确定）。
//
// 判定：3 次充能后 charges === 3。
function respawnAnchorChargesThreeTimes(test: Test): void {
    placeRespawnAnchor(test);
    test.assert(getRespawnAnchorCharges(test, 3, 2, 1) === 0, `anchor charges should be 0 before, got ${getRespawnAnchorCharges(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");

    // 连续 3 次萤石充能：charges 0→1→2→3。每次 new ItemStack 重新构造（充能消耗萤石 shrink(1)）。
    for (let i = 0; i < 3; ++i) {
        const glowstone = new ItemStack("minecraft:glowstone", 1);
        const used = farmer.useItemOnBlock(
            glowstone as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
            { x: 3, y: 2, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true on charge #${i + 1}`);
        const expected = i + 1; // 0→1→2→3
        test.assert(getRespawnAnchorCharges(test, 3, 2, 1) === expected, `anchor charges should be ${expected} after charge #${i + 1}, got ${getRespawnAnchorCharges(test, 3, 2, 1)}`);
    }

    // 判定：charges === 3（连续充能三次）。
    test.assert(getRespawnAnchorCharges(test, 3, 2, 1) === 3, `anchor charges should be 3 after 3 charges, got ${getRespawnAnchorCharges(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerRespawnAnchorTests(): void {
    GameTest.register("BlockBehaviorTests", "respawn_anchor_charges_with_glowstone", respawnAnchorChargesWithGlowstone)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "respawn_anchor_charges_three_times", respawnAnchorChargesThreeTimes)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
