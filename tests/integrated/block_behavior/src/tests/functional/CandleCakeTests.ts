// 蜡烛蛋糕点燃/熄灭行为 GameTest。
//
// wiki tech_蛋糕.txt + tech_蜡烛.txt#蜡烛蛋糕：蜡烛蛋糕（candle_cake）是插着蜡烛的蛋糕，默认未点燃
//   （lit=false）。可用打火石或火焰弹点燃（lit→true），点燃后空手右键蜡烛部分（上半部 y>0.5）可熄灭
//   （lit→false）。点燃的蜡烛蛋糕发出亮度 3 的光。吃蜡烛蛋糕需空手右键（创造模式不能进食）。
//
// C++ 链路：CandleCakeBlock（functional/CandleCakeBlock.cpp）仅有 LIT state（无 CANDLES/WATERLOGGED）。
//   - onBlockActivated（:157-217）：
//     · 空手 + isLit + hitY>0.5 → extinguish（lit→false）→ Success（熄灭分支）。
//     · 非空手（含打火石）→ 跳过熄灭分支 → canEat 检查：创造模式 canEat=false → 返 Pass → fallback
//       Item.useOn（FlintAndSteelItem.onItemUse）点燃。
//     · 空手 + 未点燃 → canEat 创造 false → Pass（创造模式空手点未点燃蜡烛蛋糕无反应）。
//   - canLight（:222）：hasProperty(LIT) && !get(LIT)，即未点燃可点燃。
//
// 点燃派发链路：打火石 useItemOnBlock → CandleCake onBlockActivated 非空手 canEat(创造)=false→Pass →
//   fallback FlintAndSteelItem.onItemUse：含 LIT 属性且 lit=false → with(LIT,true) setBlockState → Success。
//   （蜡烛蛋糕无 WATERLOGGED，打火石不拦截 waterlogged。）
// 熄灭派发链路：interactWithBlock（空手右键）→ onBlockActivated 空手 + isLit + hitY>0.5 → extinguish
//   → with(LIT,false) → Success。
//
// 测试覆盖（2 个场景，覆盖 wiki 蜡烛蛋糕点燃 + 空手熄灭核心确定行为）：
//   1. 打火石点燃蜡烛蛋糕：candle_cake(lit=false) + 打火石 useItemOnBlock → lit=true，返 true。
//   2. 空手熄灭点燃的蜡烛蛋糕：candle_cake(lit=true) + interactWithBlock（空手）→ lit=false，返 true。
//
// 关键约束：
// 1. 蜡烛蛋糕完整方块，放 (3,2,1)（minecraft:candle_cake 默认 lit=false）。
// 2. 场景 2 需放点燃的蜡烛蛋糕：用 setBlockWithStates("minecraft:candle_cake", pos, "lit=true") 直接放
//    lit=true 状态（one-sided，Cubium 专有 API），避免先点燃再熄灭的链路耦合。
// 3. 打火石用 new ItemStack("minecraft:flint_and_steel", 1)（耐久 64，创造模式不消耗）。
// 4. 熄灭需空手点击上半部（hitY>0.5）：interactWithBlock 内部 faceLocation.y=1.0（顶面），hitY=1.0>0.5 ✓。
// 5. 创造模式 canEat=false：打火石点蜡烛蛋糕走 onBlockActivated canEat 检查返 Pass → fallback 打火石点燃；
//   空手点未点燃蜡烛蛋糕也走 canEat 返 Pass（无反应），故场景 2 必须用 setBlockWithStates 放 lit=true。
// 6. 读 lit state 用 getState("lit" as any)。
//
// 不测「吃蜡烛蛋糕转蛋糕方块」：需生存模式 + 饥饿值不满（canEat=true），SimulatedPlayer 创造模式不能
//   进食，转生存模式涉及游戏模式切换与饥饿值控制，构造繁琐。TODO: 待生存模式 SimulatedPlayer 饥饿值
//   控制完善后补 candle_cake_eaten_to_cake 测试。
// 不测「火焰弹点燃」：与打火石点燃同 lit→true 行为点，本组测打火石已覆盖点燃，跳过。
// 不测「含水蜡烛蛋糕不可点燃」：蜡烛蛋糕无 WATERLOGGED 属性（CandleCakeBlock 不含水），跳过。
// 不测「点燃发光亮度」：亮度需光照系统测量，且 cubium 光照与 vanilla 有已知偏差，跳过。
//
// 跨服务端：蜡烛蛋糕 candle_cake 方块名两端一致，lit state 行为与 vanilla 一致。场景 1 打火石点燃两端
//   可对比（用 setBlockType 放默认 candle_cake）；场景 2 用 setBlockWithStates 放 lit=true 为 one-sided
//   （基岩无 setBlockWithStates），但熄灭行为本身两端可对比（若基岩能放 lit=true 蜡烛蛋糕）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蜡烛.txt#蜡烛蛋糕（蜡烛蛋糕点燃/熄灭/食用）
// Ref: CandleCakeBlock.cpp（onBlockActivated 空手熄灭+非空手canEat Pass让fallback点燃；canLight）
// Ref: FlintAndSteelItem.cpp（onItemUse 含 LIT 方块 lit=false→with(LIT,true)→Success）
// Ref: ScriptSimulatedPlayer.cpp interactWithBlock（空手右键复用 useItemOnBlock 空堆路径，faceLocation.y=1.0）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 蜡烛蛋糕 (3,2,1)。

// 读取 (x,y,z) 方块 lit state（boolean）。返回 null 表示读取失败或方块无 lit 属性。
function getCandleCakeLit(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("lit" as any);
    return typeof value === "boolean" ? value : null;
}

// 场景 1：打火石点燃蜡烛蛋糕——candle_cake(lit=false) + 打火石 useItemOnBlock → lit=true，返 true。
//
// 布局：(3,2,1) 蜡烛蛋糕（minecraft:candle_cake 默认 lit=false）。
// useItemOnBlock ① onBlockActivated（非空手打火石，跳过熄灭分支 → canEat(创造)=false → Pass）→
//   ② fallback FlintAndSteelItem.onItemUse：candle_cake 含 LIT 且 lit=false → with(LIT,true)
//   setBlockState → Success。
//
// 判定：useItemOnBlock 返 true（Success），lit === true（蜡烛蛋糕被点燃）。
function candleCakeLitByFlintAndSteel(test: Test): void {
    test.setBlockType("minecraft:candle_cake", { x: 3, y: 2, z: 1 }); // 蜡烛蛋糕 lit=false
    test.assert(getCandleCakeLit(test, 3, 2, 1) === false, `candle_cake lit should be false before, got ${getCandleCakeLit(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const flintAndSteel = new ItemStack("minecraft:flint_and_steel", 1);

    // 对蜡烛蛋糕 useItemOnBlock 打火石 → onBlockActivated canEat(创造)=false Pass → fallback 打火石点燃 → Success。
    const used = farmer.useItemOnBlock(
        flintAndSteel as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when lighting candle_cake with flint and steel");

    // 判定：lit === true（蜡烛蛋糕被点燃）。
    test.assert(getCandleCakeLit(test, 3, 2, 1) === true, `candle_cake lit should be true after lighting, got ${getCandleCakeLit(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：空手熄灭点燃的蜡烛蛋糕——candle_cake(lit=true) + interactWithBlock（空手）→ lit=false，返 true。
//
// 布局：(3,2,1) 蜡烛蛋糕 lit=true（setBlockWithStates 直接放点燃状态，one-sided）。
// interactWithBlock（空手右键）→ onBlockActivated：空手 + isLit + hitY>0.5（faceLocation.y=1.0）→
//   extinguish（with(LIT,false) setBlockState）→ Success。
//
// 判定：interactWithBlock 返 true（Success），lit === false（点燃的蜡烛蛋糕被空手熄灭）。
function candleCakeExtinguishedByEmptyHand(test: Test): void {
    // setBlockWithStates 放点燃的蜡烛蛋糕（lit=true）。one-sided：Cubium 专有 API。
    (test as unknown as {
        setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
    }).setBlockWithStates("minecraft:candle_cake", { x: 3, y: 2, z: 1 }, "lit=true");
    test.assert(getCandleCakeLit(test, 3, 2, 1) === true, `candle_cake lit should be true before extinguish, got ${getCandleCakeLit(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");

    // interactWithBlock 空手右键点燃的蜡烛蛋糕 → onBlockActivated 空手+isLit+hitY>0.5 → extinguish → Success。
    // interactWithBlock 为 Cubium 补全的 SimulatedPlayer 方法（类型定义未声明），用 as any 绕过类型检查。
    const used = (farmer as unknown as { interactWithBlock: (pos: unknown, dir: unknown) => boolean })
        .interactWithBlock({ x: 3, y: 2, z: 1 }, Direction.Up);
    test.assert(used, "interactWithBlock should return true when extinguishing lit candle_cake with empty hand");

    // 判定：lit === false（点燃的蜡烛蛋糕被空手熄灭）。
    test.assert(getCandleCakeLit(test, 3, 2, 1) === false, `candle_cake lit should be false after extinguish, got ${getCandleCakeLit(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerCandleCakeTests(): void {
    GameTest.register("BlockBehaviorTests", "candle_cake_lit_by_flint_and_steel", candleCakeLitByFlintAndSteel)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "candle_cake_extinguished_by_empty_hand", candleCakeExtinguishedByEmptyHand)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
