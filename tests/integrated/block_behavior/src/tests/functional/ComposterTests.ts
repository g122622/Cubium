// 堆肥桶堆肥与收获行为 GameTest。
//
// wiki tech_堆肥桶.txt#用途：堆肥桶接受可堆肥物品（种子/作物/食物等），按概率提升内部等级（level 0-8）。
//   - 投入可堆肥物品：按物品的堆肥概率（chance）尝试提升 level（失败则仅播放音效不升级）。
//   - level 达 7 后，经 20 游戏刻自动转变为 level 8（可收获状态）。
//   - level 8 时右键收获：产出 1 个骨粉，level 重置为 0。
//   - 南瓜派（pumpkin_pie）/蛋糕（cake）堆肥概率为 100%（chance=1.0），投入必升级——本测试用其
//     消除概率不确定性，确定性验证堆肥升级链路。
//
// C++ 链路：ComposterBlock（ComposterBlock.cpp）有 LEVEL_0_8 state（默认 0）。
//   - onBlockActivated（ComposterBlock.cpp:263-313）：第一步 `if (level == 8) { empty(); return Success; }`
//     ——level 8 直接收获，不检查手持物（任何物品/空手右键 level 8 堆肥桶都触发收获）。
//     否则取手持物 → 查 CompostableItems::getCompostChance → attemptCompost。
//   - attemptCompost（:161-210）：level>=7 return（已满/转变中）；否则 random.nextFloat() < chance 时
//     level+1 写回；newLevel==7 时 scheduleBlockTick(pos, block, 20) 调度 20tick 后转变。
//   - tick（:121-135）：level==7 时 20tick 后 setBlockState level=8 + 播放完成音效。
//   - empty（:212-249）：level==8 时掉落骨粉（ItemDropHelper::spawnItemEntity 上方）+ level 重置 0。
//   - CompostableItems：PUMPKIN_PIE/CAKE 注册 chance=1.0f（CompostableItems.cpp:268/271）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（对齐项目网络层
//   ServerPlayHandler 与 vanilla Java 1.21：先 onBlockActivated，Pass 才 fallback onItemUse）。
//   堆肥桶 onBlockActivated 返回 Success 时短路，不消耗手持物（level 8 收获不消耗；level<8 堆肥
//   成功时 onBlockActivated 内部已 shrink 手持物）。
//
// 测试覆盖（3 个场景，覆盖 wiki 堆肥升级+level7→8转变+level8收获核心行为，可跨服务端对比）：
//   1. level 8 收获骨粉：放 level=8 堆肥桶 → 持南瓜派 useItemOnBlock → empty 收获，level 重置 0 +
//      骨粉物品实体掉落（assertItemEntityPresent）。
//   2. 南瓜派堆肥升级：放 level=0 堆肥桶 → 持南瓜派 useItemOnBlock → attemptCompost 100% 升级，
//      level 升至 1。
//   3. level 6→7→8 转变链路：放 level=6 堆肥桶 → 持南瓜派 useItemOnBlock → level 升 7（调度 20tick）
//      → 等 25tick → tick 转变 level 7→8。
//
// 关键约束：
// 1. 用 setBlockWithStates 放指定 level 堆肥桶（setBlockType 只放默认 level=0）。
// 2. 南瓜派 chance=1.0 消除概率不确定性——单次投入必升级，确定性可测。
// 3. level 8 收获不检查手持物（onBlockActivated 第一步 level==8 直接 empty），持南瓜派即可触发。
// 4. level 7→8 转变由 tick 调度（attemptCompost 升到 7 时 scheduleBlockTick 20）。setBlockWithStates
//    直接放 level=7 不会调度 tick，故场景 3 从 level=6 投入升到 7（触发调度）再等转变。
// 5. 读 level state 用 getState("level" as any) 绕过 BlockStateSuperset 白名单。
// 6. 场景 3 用 pollUntilSucceed 轮询 level===8（tick 转变有 20tick 延迟）。
// 7. 骨粉实体检测用 test.assertItemEntityPresent("minecraft:bone_meal", pos, radius, true)。
//
// 不测「概率性堆肥物品（种子 30%）」：非确定，跳过。南瓜派 100% 已覆盖升级链路。
// 不测「漏斗/比较器交互」：涉容器/比较器链路，跳过。TODO: 待比较器实现后补 comparator 输出=level。
//
// 跨服务端：堆肥桶 level state 名两端一致，堆肥升级 + level7→8转变 + level8收获行为与 vanilla 一致。
//   注意：基岩 BDS 无 setBlockWithStates（Cubium 专有），本测试仅 Cubium 端可放指定 level；
//   基岩端需通过投入物品逐步升 level（概率性），跨端对比时基岩端本组测试可能需适配。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_堆肥桶.txt#用途（堆肥升级，level7→8转变20tick，level8收获骨粉）
// Ref: ComposterBlock.cpp（onBlockActivated level8收获/堆肥；attemptCompost 100%升级+调度tick；tick 7→8；empty 掉骨粉+重置）
// Ref: CompostableItems.cpp:268/271（PUMPKIN_PIE/CAKE chance=1.0f 100%堆肥）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 堆肥桶 (3,2,1)，下方 (3,1,1) stone 支撑（堆肥桶需 solidSide 上方放置）。

// 读取堆肥桶 level state（number 0-8）。返回 null 表示读取失败或非堆肥桶。
function getComposterLevel(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("level" as any);
    return typeof value === "number" ? value : null;
}

// 放支撑 + 指定 level 堆肥桶：(3,1,1) stone 支撑，(3,2,1) 堆肥桶 level=<level>。
function placeComposter(test: Test, level: number): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockWithStates("minecraft:composter", { x: 3, y: 2, z: 1 }, `level=${level}`); // 堆肥桶
}

// 场景 1：level 8 收获骨粉——放 level=8 堆肥桶 → 持南瓜派 useItemOnBlock → empty 收获，level 重置 0 +
// 骨粉物品实体掉落。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) level=8 堆肥桶。
// onBlockActivated 第一步 level==8 → empty()（掉落骨粉 + level 重置 0）→ return Success 短路（不消耗南瓜派）。
//
// 判定：
//   - 收获后 level === 0（empty 重置）。
//   - 骨粉物品实体存在（assertItemEntityPresent，empty 在堆肥桶上方掉落骨粉）。
function composterHarvestBonemealAtLevel8(test: Test): void {
    placeComposter(test, 8);

    // 收获前断言 level===8（确认初始状态）。
    test.assert(getComposterLevel(test, 3, 2, 1) === 8, `composter level should be 8 before harvest, got ${getComposterLevel(test, 3, 2, 1)}`);

    // SimulatedPlayer 持南瓜派（level 8 收获不检查手持物，持任意物品即可触发 empty）。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const pie = new ItemStack("minecraft:pumpkin_pie", 1);

    // 对 level=8 堆肥桶 useItemOnBlock → onBlockActivated level==8 → empty 收获 + return Success。
    const used = farmer.useItemOnBlock(
        pie as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when harvesting bonemeal from level-8 composter");

    // 判定 1：收获后 level === 0（empty 重置 level）。
    test.assert(getComposterLevel(test, 3, 2, 1) === 0, `composter level should be 0 after harvest, got ${getComposterLevel(test, 3, 2, 1)}`);

    // 判定 2：骨粉物品实体存在（empty 在堆肥桶上方掉落骨粉，searchRadius 覆盖堆肥桶上方区域）。
    // 用 runAtTickTime 留 1 tick 让物品实体生成，再断言（empty 同步 spawnItemEntity，但实体注册可能延 1 tick）。
    test.runAtTickTime(2, () => {
        test.assertItemEntityPresent("minecraft:bone_meal", { x: 3, y: 2, z: 1 }, 1.5, true);
        test.succeed();
    });
}

// 场景 2：南瓜派堆肥升级——放 level=0 堆肥桶 → 持南瓜派 useItemOnBlock → attemptCompost 100% 升级，
// level 升至 1。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) level=0 堆肥桶。
// onBlockActivated level=0(!=8) → 取手持南瓜派 → chance=1.0 → attemptCompost(level 0→1) → return Success。
//
// 判定：useItemOnBlock 后 level === 1（南瓜派 100% 升级）。
function composterCompostsWithPumpkinPie(test: Test): void {
    placeComposter(test, 0);

    test.assert(getComposterLevel(test, 3, 2, 1) === 0, `composter level should be 0 before composting, got ${getComposterLevel(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const pie = new ItemStack("minecraft:pumpkin_pie", 1);

    // 对 level=0 堆肥桶 useItemOnBlock 南瓜派 → onBlockActivated attemptCompost 100% 升级 level 0→1。
    const used = farmer.useItemOnBlock(
        pie as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when composting pumpkin pie");

    // 判定：level === 1（南瓜派 100% 堆肥概率，单次投入必升级）。
    test.assert(getComposterLevel(test, 3, 2, 1) === 1, `composter level should be 1 after pumpkin pie compost, got ${getComposterLevel(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：level 6→7→8 转变链路——放 level=6 堆肥桶 → 持南瓜派 useItemOnBlock → level 升 7（调度 20tick）
// → 等 25tick → tick 转变 level 7→8。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) level=6 堆肥桶。
// onBlockActivated level=6(!=8) → 南瓜派 chance=1.0 → attemptCompost(level 6→7) → newLevel==7 时
// scheduleBlockTick(pos, block, 20)。20 tick 后 tick() → level 7→8 + 完成音效。
//
// 判定：
//   - useItemOnBlock 后 level === 7（南瓜派升级 6→7）。
//   - pollUntilSucceed 轮询 level === 8（tick 转变，20tick 延迟 + 余量）。
function composterLevel7To8Transitions(test: Test): void {
    placeComposter(test, 6);

    test.assert(getComposterLevel(test, 3, 2, 1) === 6, `composter level should be 6 before composting, got ${getComposterLevel(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const pie = new ItemStack("minecraft:pumpkin_pie", 1);

    // 对 level=6 堆肥桶 useItemOnBlock 南瓜派 → attemptCompost level 6→7 + scheduleBlockTick(20)。
    const used = farmer.useItemOnBlock(
        pie as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when composting pumpkin pie to level 7");

    // 升级后断言 level === 7（南瓜派 100% 升级 6→7，触发 20tick 转变调度）。
    test.assert(getComposterLevel(test, 3, 2, 1) === 7, `composter level should be 7 after pumpkin pie compost, got ${getComposterLevel(test, 3, 2, 1)}`);

    // 轮询断言 level === 8（tick 转变，20tick 延迟 + 余量）。tick() 在 level==7 时 20tick 后 setBlockState level=8。
    pollUntilSucceed(
        test,
        () => getComposterLevel(test, 3, 2, 1) === 8,
        {
            startTick: 2,
            interval: 2,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `composter level should transition 7→8 after 20 ticks, got ${getComposterLevel(test, 3, 2, 1)}`);
            },
        },
    );
}

export function registerComposterTests(): void {
    GameTest.register("BlockBehaviorTests", "composter_harvest_bonemeal_at_level_8", composterHarvestBonemealAtLevel8)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "composter_composts_with_pumpkin_pie", composterCompostsWithPumpkinPie)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "composter_level_7_to_8_transitions", composterLevel7To8Transitions)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
