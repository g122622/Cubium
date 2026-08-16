// 南瓜剪刀雕刻行为 GameTest。
//
// wiki tech_南瓜.txt#雕刻：对一个南瓜使用剪刀，可以使其变成一个雕刻南瓜（carved_pumpkin），并掉落
//   4 份（JE）/1 份（BE）南瓜种子。Cubium 对齐 JE 掉 4 份。同时使南瓜顶部纹理旋转方向。剪刀雕刻产生
//   的雕刻南瓜若满足建造生物结构也可生成生物（铁傀儡/雪傀儡/凋灵），本组不测生成（需结构+实体）。
//
// C++ 链路：PumpkinBlock（agricultural/MelonPumpkinBlocks.cpp）。
//   - onBlockActivated（:97）：手持物非剪刀 → Pass；剪刀 + m_carvedPumpkin 非空 → 计算 carved 朝向
//     （hit.face() 上下则用玩家 yaw；水平则用 hit.face）+ 雕刻音效 + setBlockState(pos, carvedState, 11)
//     （pumpkin→carved_pumpkin 带 HORIZONTAL_FACING）+ 掉落4南瓜种子（ItemDropHelper）→ Success。
//   - 南瓜 minecraft:pumpkin（Material::EARTH，完整方块，无 canSurvive 自毁）。
//   - 雕刻南瓜 minecraft:carved_pumpkin（Material::EARTH，有 HORIZONTAL_FACING state）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock Block.use 前置分支（先 onBlockActivated，Pass 才 fallback）。
//   南瓜 onBlockActivated 手持剪刀 → Success 短路（不走 fallback，剪刀不触发 onItemUse 放置）。
//
// 测试覆盖（1 个场景，覆盖 wiki 剪刀雕刻核心确定行为）：
//   1. 剪刀雕刻南瓜：放南瓜 + 剪刀 useItemOnBlock → 原位 (3,2,1) 变 carved_pumpkin，返 true。
//
// 关键约束：
// 1. 南瓜完整方块（Material::EARTH），无 canSurvive 自毁，放 (3,2,1)（minecraft:pumpkin）无需支撑。
// 2. 剪刀用 new ItemStack("minecraft:shears", 1)（耐久 238，创造模式 attemptDamageItem 不消耗耐久）。
// 3. onBlockActivated 检查 heldItem==Items::SHEARS，剪刀匹配 → 雕刻。useItemOnBlock 调 onBlockActivated
//   前把 stack 设到主手选中槽（SimulatedPlayer.cpp:287），getHeldItem 读到剪刀。
// 4. 判定原位 block.typeId：pumpkin → carved_pumpkin（方块类型转换）。不判定朝向（取决于玩家 yaw，
//   非本组聚焦）也不判定种子掉落（掉落物实体非确定）。
// 5. useItemOnBlock 传 Direction.Up，onBlockActivated 走 yaw 分支计算 facing（SimulatedPlayer 默认 yaw
//   决定朝向），但朝向不影响 carved_pumpkin 类型判定。
//
// 不测「掉落4南瓜种子」：掉落物实体生成非确定（位置/时序），本组聚焦方块类型转换，跳过。
//   TODO: 待掉落物断言 API 完善后补南瓜种子数量测试。
// 不测「雕刻南瓜生成生物」：需铁傀儡/雪傀儡/凋灵结构 + 实体生成，复杂且非本组聚焦，跳过。
// 不测「非剪刀物品右键南瓜无反应」：onBlockActivated 非剪刀返 Pass → fallback 物品放置（如放方块），
//   语义弱，跳过。
// 不测「雕刻南瓜朝向」：朝向取决于玩家 yaw + hit.face，SimulatedPlayer 默认 yaw 固定但非本组聚焦，跳过。
//
// 跨服务端：南瓜 pumpkin / 雕刻南瓜 carved_pumpkin 方块名两端一致，剪刀雕刻 pumpkin→carved_pumpkin
//   行为与 vanilla 一致。基岩无 setBlockWithStates 需求（本组用 setBlockType 放默认 pumpkin），
//   两端均可放；剪刀雕刻行为两端可对比（BE 掉1种子 vs JE 掉4种子，但本组只断言方块转换不断言种子数）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_南瓜.txt#雕刻（剪刀使南瓜变雕刻南瓜，掉4南瓜种子）
// Ref: MelonPumpkinBlocks.cpp（PumpkinBlock::onBlockActivated 剪刀→setBlockState carved+掉种子→Success）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，调 onBlockActivated 前设主手选中槽）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 南瓜 (3,2,1)，完整方块无需支撑。

// 读取 (x,y,z) 方块 typeId。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 场景 1：剪刀雕刻南瓜——放南瓜 + 剪刀 useItemOnBlock → 原位 (3,2,1) 变 carved_pumpkin，返 true。
//
// 布局：(3,2,1) 南瓜（minecraft:pumpkin，默认完整方块）。
// onBlockActivated：手持剪刀 == Items::SHEARS + m_carvedPumpkin 非空 → 计算 carved 朝向（hit.face=Up
//   走 yaw 分支）+ 雕刻音效 + setBlockState(pos, carved_pumpkin, 11) + 掉4南瓜种子 → Success。
//
// 判定：useItemOnBlock 返 true（Success），原位 (3,2,1) typeId === "minecraft:carved_pumpkin"
//   （南瓜经剪刀雕刻变为雕刻南瓜）。
function pumpkinCarvesWithShears(test: Test): void {
    test.setBlockType("minecraft:pumpkin", { x: 3, y: 2, z: 1 }); // 南瓜
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:pumpkin", `pumpkin should be at (3,2,1) before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const shears = new ItemStack("minecraft:shears", 1);

    // 对南瓜 useItemOnBlock 剪刀 → onBlockActivated 剪刀 → setBlockState carved_pumpkin + 掉种子 → Success。
    const used = farmer.useItemOnBlock(
        shears as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when carving pumpkin with shears");

    // 判定：原位 (3,2,1) 变 carved_pumpkin（南瓜经剪刀雕刻变为雕刻南瓜）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:carved_pumpkin", `carved pumpkin should be at (3,2,1) after carving, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerPumpkinTests(): void {
    GameTest.register("BlockBehaviorTests", "pumpkin_carves_with_shears", pumpkinCarvesWithShears)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
