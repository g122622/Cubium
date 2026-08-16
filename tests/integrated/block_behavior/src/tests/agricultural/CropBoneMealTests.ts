// 作物骨粉催熟行为 GameTest（胡萝卜/马铃薯 + maxAge 钳制）。
//
// 与 BoneMealTests（小麦 age0→[2,5]）互补：覆盖多作物骨粉链路 + maxAge 钩制逻辑。
// 小麦/胡萝卜/马铃薯骨粉增量同为 getBonemealAgeIncrease（=2+nextInt(4)→[2,5]，CropBlock.cpp:211-219），
// canUseBonemeal 恒 true（100% 即时催熟），grow 同步 setBlockState（flags=2）。这些作物骨粉行为与
// vanilla 一致，可跨服务端对比。
//
// 甜菜根/火把花骨粉 Cubium 实现有偏差（BeetrootBlock.cpp:91-99 getBonemealAgeIncrease 固定返回 1，
// 而 vanilla 是 75% 概率 +1、25% 无变化），按「不为 Cubium 与 vanilla 不一致行为写测试」准则，
// 本文件不写甜菜根/火把花骨粉测试。
// TODO: 待 Cubium 实现甜菜根/火把花骨粉的 75% 概率判定（对齐 vanilla）后补充。
//
// maxAge 钳制（CropBlock::grow，CropBlock.cpp:190-203）：newAge = min(getAge + bonemealIncrease, maxAge)。
// 小麦/胡萝卜/马铃薯 maxAge=7。放 age=6 作物骨粉，6+[2,5]=8..11 经 min 钳制为 7。验证钳制逻辑。
//
// 判定链路同 BoneMealTests：SimulatedPlayer.useItemOnBlock 派发 BoneMealItem::onItemUse →
// CropBlock::grow 同步写回，getBlock().permutation.getState("age") 立即可读。
// 跨服务端：小麦/胡萝卜/马铃薯 state 名两端均为 age，值域 0-7 一致，骨粉增量 [2,5] 一致，可对比。
// 注意：基岩创造模式对作物直接成熟（age=maxAge），但 SimulatedPlayer 默认走标准增量（Cubium grow
// 与模式无关），两端 SimulatedPlayer 均走生存语义增量。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_骨粉.txt#催熟（小麦/胡萝卜/马铃薯骨粉生长2-5阶段）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_作物机制.txt（骨粉催熟机制）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { BlockPermutation, ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 通用作物骨粉催熟断言：放 age=startAge 的指定作物 → SimulatedPlayer 骨粉 → 断言 age 满足 predicate。
//
// @param test GameTest Test 对象
// @param cropType 作物 typeId（minecraft:carrots / minecraft:potatoes / minecraft:wheat）
// @param startAge 骨粉前作物 age
// @param checkAge 断言谓词，返回 true 表示 age 符合预期
// @param label 超时/失败错误标签
function assertBonemealCrop(
    test: Test,
    cropType: string,
    startAge: number,
    checkAge: (age: number) => boolean,
    label: string,
): void {
    const cropPos = { x: 3, y: 2, z: 1 };
    const farmlandPos = { x: 3, y: 1, z: 1 };

    // 耕地支撑（作物需在耕地上方，canSustain 检查）。
    test.setBlockType("minecraft:farmland", farmlandPos);

    // 放指定 age 的作物。BlockPermutation.resolve 跨服务端通用（两端 state 名均为 age）。
    // any 绕过 @minecraft/server 两版本 BlockPermutation 类型冲突（见 sweetBerryBush.ts 注释）。
    const perm = BlockPermutation.resolve(cropType, { age: startAge }) as any;
    (test as unknown as {
        setBlockPermutation: (blockData: unknown, blockLocation: { x: number; y: number; z: number }) => void;
    }).setBlockPermutation(perm, cropPos);

    // SimulatedPlayer 持骨粉对作物使用（direction=Up，从上方使用）。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        cropPos,
        Direction.Up,
    );
    test.assert(used, `${label}: useItemOnBlock should return true`);

    // 判定：骨粉 grow 同步 setBlockState，useItemOnBlock 返回后立即可读 age。
    const block = test.getBlock(cropPos);
    test.assert(block !== undefined, `${label}: getBlock should return Block object`);
    const age = block?.permutation?.getState("age");
    test.assert(
        typeof age === "number" && checkAge(age),
        `${label}: age check failed (got ${age})`,
    );

    test.succeed();
}

// 胡萝卜骨粉催熟：age=0 → age∈[2,5]（与小麦同机制，验证多作物骨粉链路）。
// CarrotBlock 继承 CropBlock，getBonemealAgeIncrease 走基类 2+nextInt(4)=[2,5]，canUseBonemeal 恒 true。
// Ref: tech_骨粉.txt#催熟
function bonemealGrowsCarrot(test: Test): void {
    assertBonemealCrop(
        test,
        "minecraft:carrots",
        0,
        (age) => age >= 2 && age <= 5,
        "carrot age0 bonemeal",
    );
}

// 马铃薯骨粉催熟：age=0 → age∈[2,5]（同胡萝卜，验证多作物骨粉链路）。
// PotatoBlock 继承 CropBlock，机制与胡萝卜/小麦一致。
function bonemealGrowsPotato(test: Test): void {
    assertBonemealCrop(
        test,
        "minecraft:potatoes",
        0,
        (age) => age >= 2 && age <= 5,
        "potato age0 bonemeal",
    );
}

// 小麦 maxAge 钳制：age=6 → age=7（maxAge=7 钳制，验证 grow 的 min(newAge, maxAge) 逻辑）。
// 6+[2,5]=8..11 经 min 钳制为 7。断言 age==7（恰好成熟）。
// Ref: CropBlock.cpp:190-203 grow 的 min 钳制
function bonemealClampsWheatAtMaxAge(test: Test): void {
    assertBonemealCrop(
        test,
        "minecraft:wheat",
        6,
        (age) => age === 7,
        "wheat age6 bonemeal clamp",
    );
}

// 胡萝卜 maxAge 钳制：age=6 → age=7（同小麦，验证多作物 maxAge 钳制）。
function bonemealClampsCarrotAtMaxAge(test: Test): void {
    assertBonemealCrop(
        test,
        "minecraft:carrots",
        6,
        (age) => age === 7,
        "carrot age6 bonemeal clamp",
    );
}

export function registerCropBoneMealTests(): void {
    GameTest.register("BlockBehaviorTests", "bonemeal_grows_carrot", bonemealGrowsCarrot)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "bonemeal_grows_potato", bonemealGrowsPotato)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "bonemeal_clamps_wheat_at_max_age", bonemealClampsWheatAtMaxAge)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "bonemeal_clamps_carrot_at_max_age", bonemealClampsCarrotAtMaxAge)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
