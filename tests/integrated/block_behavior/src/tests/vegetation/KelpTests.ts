// 海带行为 GameTest。
//
// wiki tech_海带.txt:
//   :41 "海带可以被放置于上表面方块支撑形状完整的方块上（岩浆块除外）。"
//   :45 "破坏海带植株会摧毁其上方所有海带植株和海带。" 即海带下方支撑方块被移除时，海带失去支撑
//       自毁（vanilla 经 neighborChanged 链）。
//   :63 "对海带使用骨粉可使其生长一格。"
//
// ============================ C++ 链路 ============================
// KelpBlock（ocean/KelpBlock.cpp）继承 Block + IPlantable + IGrowable。
// - updatePostPlacement（:117-139）：facing==Down 时检查 isValidPosition，失败返 air 自毁。
// - randomTick（:141-165）：上方非 air 返；age>=25 返；14% 概率上方放新海带，原位 age+1。
// - IGrowable（:167-214，本次新增）：canGrow 检查 age<25 且上方为 air；
//   canUseBonemeal 恒返 true；grow 在上方放新海带（defaultState age=0），原位 age+1。
//
// ============================ 骨粉生长一格（已修复）============================
// wiki tech_海带.txt#生长（:63）："对海带使用骨粉可使其生长一格。"
// 修复前缺陷：KelpBlock 未实现 IGrowable，BoneMealItem::onItemUse dynamic_cast<IGrowable*> 返 nullptr
//   → 跳过 IGrowable 分支 → 骨粉无效。
// 修复方案：KelpBlock 继承 IGrowable，grow() 在上方放置新海带方块（age=0），原位 age+1。
//   与 randomTick 生长逻辑一致，但骨粉生长应直接延伸一格。
// 测试验证：useItemOnBlock 返 true + 上方 (3,3,1) 出现 kelp（生长延伸到上方）。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃坑）============================
// glass_pit：y=0 glass 底座，y=1..3 air 空腔，y=4 glass 顶部。helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// 结构内容从 origin+(0,1,0) 放置（placeOrigin），helper worldBlockPosition(rel)=origin+rel。
// 故相对 y=N 对应结构内 y=N-1。
//
// 海带骨粉生长测试布局：
//   (3,1,1) 放 stone（下方支撑，isSolid 满足 isValidPosition）。
//   (3,2,1) 放 kelp（age=0，在 stone 上，强放绕过 isValidPosition 不立即自毁）。
//   上方 (3,3,1) 为 air（生长目标格）。
//   对 (3,2,1) 海带 useItemOnBlock 骨粉 → IGrowable::grow 在上方放新海带。
//   断言：useItemOnBlock 返 true + (3,3,1) 出现 kelp（生长延伸到上方）。
//
// ============================ 排除项（不写测试）============================
// - randomTick 向上生长（14% 概率）：概率性，跳过。
// - 海带在水中语义：Cubium 强放绕过 isValidPosition，且 updatePostPlacement 只检查下方支撑
//   （与水无关），符合支撑自毁核心行为点，不为 JE/BE 水语义差异写测试。
//
// ============================ 跨服务端对比 ============================
// - 海带 age state 名两端一致（age 0-25）。
// - 骨粉生长一格两端一致（wiki :63 明文）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海带.txt#生长（:63 骨粉生长一格）
// Ref: KelpBlock.cpp（IGrowable:167-214, updatePostPlacement:117-139, randomTick:141-165）
// Ref: BoneMealItem.cpp:70（dynamic_cast<IGrowable> 检查骨粉有效性）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 内部坐标。
const KELP_SUPPORT = { x: 3, y: 1, z: 1 }; // 下方 stone 支撑
const KELP = { x: 3, y: 2, z: 1 }; // kelp 头部
const KELP_GROW_TARGET = { x: 3, y: 3, z: 1 }; // 上方 air，生长目标格

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 移除海带下方支撑方块（石头）时海带自毁变 air。
//
// 布局：(3,1,1) 铺 stone 作海带下方支撑（isSolid true，isValidPosition 满足），(3,2,1) 放海带
// （在 stone 上，强放绕过 isValidPosition 不立即自毁），再 (3,1,1) 设 air 移除支撑。
// air 放置向 Up 邻居海带派发 updatePostPlacement(Down) → 下方 air isSolid false → isValidPosition
// 失败 → 返回 air。
//
// 判定：succeedWhenBlockPresent 断言海带格 (3,2,1) 海带消失（同 tick 同步）。
function kelpBreaksWhenSupportBelowRemoved(test: Test): void {
    // (3,1,1) 铺 stone 作海带下方支撑（isSolid=true，isValidPosition 下方固体满足）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放海带（在 stone 上，强放绕过 isValidPosition，不立即自毁）。setBlockType 取 defaultState age=0。
    test.setBlockType("minecraft:kelp", { x: 3, y: 2, z: 1 });

    // (3,1,1) 设 air 移除支撑（stone→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向 Up 邻居
    // 海带派发 updatePostPlacement(Down) → 下方 air isSolid false → isValidPosition 失败 → 返回 air。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言海带格 (3,2,1) 海带已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:kelp", { x: 3, y: 2, z: 1 }, false);
}

// 海带骨粉生长一格（wiki :63 明文骨粉可使其生长一格）。
// 布局：(3,1,1) stone 支撑，(3,2,1) kelp(age=0)，上方 (3,3,1) 为 air。
// 骨粉后断言：useItemOnBlock 返 true + (3,3,1) 出现 kelp（生长延伸到上方）。
function kelpBonemealGrows(test: Test): void {
    test.setBlockType("minecraft:stone", KELP_SUPPORT);
    test.setBlockType("minecraft:kelp", KELP);
    test.assert(
        getTypeId(test, KELP) === "minecraft:kelp",
        `kelp should be at ${JSON.stringify(KELP)}, got ${getTypeId(test, KELP)}`,
    );
    test.assert(
        getTypeId(test, KELP_GROW_TARGET) === "minecraft:air",
        `grow target should be air before growth, got ${getTypeId(test, KELP_GROW_TARGET)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对海带 useItemOnBlock 骨粉 → IGrowable::grow 在上方放新海带。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        KELP,
        Direction.Up,
    );

    // 骨粉应成功（返 true），且上方应出现 kelp 头部方块。
    test.assert(
        used,
        `bonemeal on kelp should succeed (used=${used}). ` +
            `wiki:63 bonemeal grows kelp one block. Check IGrowable impl on KelpBlock.`,
    );
    test.assert(
        getTypeId(test, KELP_GROW_TARGET) === "minecraft:kelp",
        `bonemeal growth: expected kelp at ${JSON.stringify(KELP_GROW_TARGET)} (upward), ` +
            `got ${getTypeId(test, KELP_GROW_TARGET)}`,
    );

    test.succeed();
}

export function registerKelpTests(): void {
    GameTest.register("BlockBehaviorTests", "kelp_breaks_when_support_below_removed", kelpBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "kelp_bonemeal_grows", kelpBonemealGrows)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
