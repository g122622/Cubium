// 蜡烛点燃与堆叠行为 GameTest。
//
// wiki 行为（vanilla CandleBlock / AbstractCandleBlock，1.21.11）：
//   - 蜡烛默认 lit=false，可用打火石/火焰弹点燃（lit=true），点燃时发光（3×CANDLES 等级）。
//   - 含水蜡烛（waterlogged=true）不可点燃（打火石 onItemUse 检查 WATERLOGGED 返 Fail）。
//   - 空手右键已点燃蜡烛可熄灭（lit→false），但 GameTest 脚本 useItemOnBlock 强制要 ItemStack 形参，
//     无法模拟空手，故空手熄灭不在本组测试覆盖（TODO: 待空手点击 API 打通后补）。
//   - 同色蜡烛可堆叠：对已有蜡烛位置 useItemOnBlock 蜡烛物品，getStateForPlacement 检测已有同类型
//     → CANDLES 递增（1→2→3→4，满 4 不再增）。
//   - 投掷物（燃烧的箭）击中可点燃（onProjectileHit），本组不测（涉投射物非确定性）。
//
// C++ 链路：
//   - CandleBlock（decorative/CandleBlock.cpp）有 CANDLES(1-4) + LIT + WATERLOGGED state。
//   - onBlockActivated：空手 + mayBuild + lit → extinguish（熄灭）→ Success；其他返 Pass。
//     打火石非空手 → onBlockActivated 返 Pass → fallback Item.useOn（FlintAndSteelItem.onItemUse）。
//   - FlintAndSteelItem::onItemUse（special/FlintAndSteelItem.cpp:48）：目标含 LIT 属性且未点燃 +
//     非 waterlogged → with(LIT,true) setBlockState → Success；waterlogged → Fail；已点燃 → 转 face 相邻放火。
//   - getStateForPlacement（:94）：已有同类型蜡烛且 CANDLES<4 → CANDLES+1；否则新放 CANDLES=1。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。蜡烛 onBlockActivated 对打火石返 Pass（非空手熄灭条件），fallback 到打火石
//   onItemUse 点燃。创造模式打火石不消耗（useItemOnBlock 第331行 isCreative 守卫），可重复使用。
//
// 测试覆盖（2 个场景，覆盖 wiki 打火石点燃 + 同色蜡烛堆叠核心确定行为）：
//   1. 打火石点燃蜡烛：放蜡烛（lit=false）+ 打火石 useItemOnBlock → lit=true，返 true。
//   2. 蜡烛堆叠：连续 3 次 useItemOnBlock 蜡烛物品 → CANDLES 1→2→3→4。
//
// 关键约束：
// 1. 蜡烛需 solid 下方（isValidPosition 检查 canSupportCenter(below, UP)）——(3,1,1) stone 支撑 +
//    (3,2,1) 蜡烛（minecraft:candle 默认 CANDLES=1, lit=false, waterlogged=false）。
// 2. 读 lit/candles state 用 getState("lit"/"candles" as any) 绕过 BlockStateSuperset 白名单。
// 3. 打火石用 new ItemStack("minecraft:flint_and_steel", 1)（耐久 64，创造不消耗）。
// 4. 蜡烛堆叠用手持蜡烛物品 useItemOnBlock（BlockItem.onItemUse 放置 → getStateForPlacement 检测已有
//    蜡烛 → CANDLES+1）。连续点击同位置堆叠。
// 5. 堆叠场景 SimulatedPlayer 默认创造，蜡烛物品不消耗，可连续 3 次堆叠到 CANDLES=4。
//
// 不测「空手熄灭」：useItemOnBlock 强制要 ItemStack，无法模拟空手。TODO: 待空手点击 API 打通后补。
// 不测「含水蜡烛无法点燃」：构造含水蜡烛需 setBlockWithStates 或水流交互，脚本侧无可靠 API，
//   跳过。TODO: 待含水方块状态可控后补。
// 不测「投掷物点燃」：涉投射物非确定性，跳过。
// 不测「水下/含水自动熄灭」：涉 randomTick 非确定，跳过。
//
// 跨服务端：蜡烛 candle 方块名两端一致，lit/candles state 行为与 vanilla 一致。打火石 flint_and_steel
//   两端一致。打火石点燃 + 蜡烛堆叠行为两端可对比。
//
// Ref: src/common/world/block/blocks/decorative/CandleBlock.cpp（onBlockActivated 空手熄灭/getStateForPlacement 堆叠）
// Ref: src/common/item/items/special/FlintAndSteelItem.cpp（onItemUse 含 LIT 方块点燃 + waterlogged 拦截）
// Ref: src/common/world/block/blocks/decorative/AbstractCandleBlock.hpp（isLit/extinguish/setLit）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 蜡烛 (3,2,1)，下方 (3,1,1) stone 支撑（蜡烛需 canSupportCenter 下方）。

// 读取蜡烛 lit state（boolean）。返回 null 表示读取失败或非蜡烛。
function getCandleLit(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("lit" as any);
    return typeof value === "boolean" ? value : null;
}

// 读取蜡烛 candles state（number 1-4）。返回 null 表示读取失败或非蜡烛。
function getCandleCount(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("candles" as any);
    return typeof value === "number" ? value : null;
}

// 放支撑 + 蜡烛：(3,1,1) stone 支撑，(3,2,1) 蜡烛（minecraft:candle 默认 candles=1, lit=false）。
function placeCandle(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:candle", { x: 3, y: 2, z: 1 }); // 蜡烛 candles=1, lit=false
}

// 场景 1：打火石点燃蜡烛——放蜡烛（lit=false）+ 打火石 useItemOnBlock → lit=true，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 蜡烛 lit=false。
// 蜡烛 onBlockActivated 对打火石：heldItem 非空（打火石）→ 不满足「空手熄灭」→ 返 Pass。
// fallback FlintAndSteelItem.onItemUse：蜡烛含 LIT 属性且 lit=false + 非 waterlogged →
// with(LIT,true) setBlockState → Success。
//
// 判定：useItemOnBlock 返 true（Success），lit === true（点燃）。
function candleLitByFlintAndSteel(test: Test): void {
    placeCandle(test);
    test.assert(getCandleLit(test, 3, 2, 1) === false, `candle lit should be false before, got ${getCandleLit(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const flintAndSteel = new ItemStack("minecraft:flint_and_steel", 1);

    // 对蜡烛 useItemOnBlock 打火石 → onBlockActivated Pass → fallback 打火石 onItemUse 点燃 → Success。
    const used = farmer.useItemOnBlock(
        flintAndSteel as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when lighting candle with flint and steel");

    // 判定：lit === true（打火石点燃蜡烛）。
    test.assert(getCandleLit(test, 3, 2, 1) === true, `candle lit should be true after lighting, got ${getCandleLit(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：蜡烛堆叠——连续 3 次 useItemOnBlock 蜡烛物品 → CANDLES 1→2→3→4。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 蜡烛 candles=1（已放）。
// 每次对 (3,2,1) useItemOnBlock 蜡烛物品（BlockItem.onItemUse 放置）→ getStateForPlacement 检测
// 已有同类型蜡烛且 CANDLES<4 → CANDLES+1。创造模式蜡烛不消耗，连续 3 次堆叠到 CANDLES=4。
//
// 判定：3 次堆叠后 candles === 4（满堆叠）。
function candleStacksUpToFour(test: Test): void {
    placeCandle(test);
    test.assert(getCandleCount(test, 3, 2, 1) === 1, `candle count should be 1 before, got ${getCandleCount(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");

    // 连续 3 次 useItemOnBlock 蜡烛物品堆叠：candles 1→2→3→4。
    for (let i = 0; i < 3; ++i) {
        const candleItem = new ItemStack("minecraft:candle", 1);
        const used = farmer.useItemOnBlock(
            candleItem as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
            { x: 3, y: 2, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true on stack #${i + 1} (candle placement)`);
        const expected = i + 2; // 1→2→3→4
        test.assert(getCandleCount(test, 3, 2, 1) === expected, `candle count should be ${expected} after stack #${i + 1}, got ${getCandleCount(test, 3, 2, 1)}`);
    }

    // 判定：candles === 4（满堆叠，再放不增）。
    test.assert(getCandleCount(test, 3, 2, 1) === 4, `candle count should be 4 (max) after stacking, got ${getCandleCount(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerCandleTests(): void {
    GameTest.register("BlockBehaviorTests", "candle_lit_by_flint_and_steel", candleLitByFlintAndSteel)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "candle_stacks_up_to_four", candleStacksUpToFour)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
