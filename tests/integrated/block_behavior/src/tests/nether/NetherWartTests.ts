// 下界疣（nether_wart）行为 GameTest：验证骨粉无效 + randomTick 生长。
//
// wiki tech_下界疣.txt#用途#种植（:45-50）：
//   "下界疣只能种在灵魂沙上，它可以在任何维度中生长。
//    下界疣在每个随机刻都有10%的概率生长一个阶段……
//    下界疣的成长速度不受光照或其他环境因素的影响，骨粉对下界疣没有作用，蜜蜂对其授粉同样没有作用。"
//   关键行为：① 骨粉无效；② randomTick 10% 概率生长（age 0→3，maxAge=3）。
//
// Cubium 实现（NetherWartBlock.cpp）：
//   - randomTick（:105-116）：age < getMaxAge(3) 时，random.nextInt(10)==0（10% 概率）→ withAge(age+1)。
//   - isValidPosition（:88-103）：仅 SOUL_SAND / SOUL_SOIL 上可放。
//   - getStateForPlacement（:83-86）：默认状态 age=0。
//   - 不实现 IGrowable 接口（仅继承 Block + IPlantable），故骨粉链路在 dynamic_cast 处返 nullptr。
//
// ============================ 骨粉无效链路 ============================
// BoneMealItem::onItemUse（src/common/item/items/special/BoneMealItem.cpp:61-123）：
//   - dynamic_cast<const IGrowable*>(&block) → NetherWartBlock 不实现 IGrowable → 返 nullptr。
//   - growable == nullptr → 跳过 IGrowable 分支 → 尝试水中海草分支（下界疣非水）→ 返 Fail。
//   - useItemOnBlock 收到 onItemUse 返 Fail → fallback 链路无其他 Item → 返 false。
//   - 判定：useItemOnBlock 返 false（骨粉无效），age 仍为 0（未生长）。
//
// ============================ randomTick 生长链路 ============================
// BaseGameTestInstance::tick（src/server/test/facade/BaseGameTestInstance.cpp:52）每 tick 驱动世界 randomTick。
// NetherWartBlock::randomTick（:105-116）：age=0 时 10% 概率 → age=1。
// 调高 randomTickSpeed（SimulatedPlayer.chat("/gamerule randomTickSpeed 1000")）使下界疣格每 tick 命中
// 概率≈24.4%，20 tick 内至少命中一次概率≈99.5%。
// 生长判定：age 从 0 增长到 >=1（10% 概率/次，speed=1000 时每 tick 命中概率高）。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃坑）============================
// glass_pit：y=0 glass 底座，y=1..3 air 空腔，y=4 glass 顶部。helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// 结构内容从 origin+(0,1,0) 放置（placeOrigin），helper worldBlockPosition(rel)=origin+rel。
// 故相对 y=N 对应结构内 y=N-1。
//
// 测试1 nether_wart_bonemeal_has_no_effect（骨粉对下界疣无效）：
//   布局：(3,1,1) 放 soul_sand（下界疣支撑），上方 (3,2,1) 放 nether_wart(age=0)。
//   SimulatedPlayer 持骨粉对下界疣 (3,2,1) useItemOnBlock(Up) → BoneMealItem::onItemUse →
//   dynamic_cast<IGrowable> 返 nullptr → Fail → useItemOnBlock 返 false。
//   判定：useItemOnBlock 返 false（骨粉无效）+ age 仍为 0（未生长）。
//
// 测试2 nether_wart_grows_via_random_tick（randomTick 10% 概率生长）：
//   布局：(3,1,1) 放 soul_sand，上方 (3,2,1) 放 nether_wart(age=0)。
//   调高 randomTickSpeed 使下界疣格在数 tick 内被随机刻命中。
//   轮询 age >= 1（10% 概率/次，speed=1000 时高概率命中）。
//
// ============================ 排除项（不写测试）============================
// - 种植行为：下界疣物品 minecraft:nether_wart 注册为普通 Item（无 PlaceableItem 关联），
//   "右键灵魂沙种植"链路未实现，按"不为 Cubium 与 vanilla 不一致行为写测试"准则，不写种植测试。
// - 破坏掉落：下界疣破坏掉落 1 个（age<3）或 2-4 个（age=3），需破坏物品链路，超出本组范围。
// - 酿造：下界疣酿造粗制药水，属酿造系统，超出本组范围。
//
// ============================ 跨服务端对比 ============================
// - nether_wart/soul_sand typeId 两端一致（1.0 加入，1.21.11 已含）。
// - age 状态属性两端一致（age 0-3）。
// - 骨粉无效行为两端一致（NetherWartBlock 不实现 IGrowable，骨粉 dynamic_cast 返 nullptr → Fail）。
// - randomTick 10% 生长行为两端一致（wiki 明文）。
// - /gamerule randomTickSpeed Cubium 侧可用，基岩侧 chat 执行命令 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_下界疣.txt#用途#种植（:45-50 骨粉无效+10%生长）
// Ref: NetherWartBlock.cpp:105-116（randomTick 10% 生长）、:88-103（isValidPosition 灵魂沙/灵魂土）
// Ref: BoneMealItem.cpp:61-123（onItemUse → dynamic_cast<IGrowable> 返 nullptr → Fail）
// Ref: CropBoneMealTests.ts（作物骨粉测试范式）、MyceliumTests.ts（randomTickSpeed 调高范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { BlockPermutation, ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// 下界疣支撑位（soul_sand）与下界疣位（y=2 air 层）。
const SOUL_SAND_POS = { x: 3, y: 1, z: 1 };
const WART_POS = { x: 3, y: 2, z: 1 };

// 调高 randomTickSpeed 使下界疣格在数 tick 内被随机刻确定性命中。
// 1000 使单格每 tick 命中概率≈24.4%。randomTick 命中后 10% 概率生长，speed=1000 时多次命中后生长概率→1。
const HIGH_RANDOM_TICK_SPEED = "1000";

// 读取下界疣方块的 age 状态。返回 -1 表示读取失败。
function getWartAge(test: Test): number {
    const block = test.getBlock(WART_POS);
    const age = block?.permutation?.getState("age");
    return typeof age === "number" ? age : -1;
}

// 布置灵魂沙 + age=startAge 的下界疣。
function placeSoulSandAndWart(test: Test, startAge: number): void {
    test.setBlockType("minecraft:soul_sand", SOUL_SAND_POS);
    // BlockPermutation.resolve 跨服务端通用（两端 state 名均为 age）。
    const perm = BlockPermutation.resolve("minecraft:nether_wart", { age: startAge }) as any;
    (test as unknown as {
        setBlockPermutation: (blockData: unknown, blockLocation: { x: number; y: number; z: number }) => void;
    }).setBlockPermutation(perm, WART_POS);
}

// 骨粉对下界疣无效（wiki: 骨粉对下界疣没有作用）。
// 布局：(3,1,1) soul_sand，上方 (3,2,1) nether_wart(age=0)。
// SimulatedPlayer 持骨粉对下界疣 useItemOnBlock(Up) → BoneMealItem::onItemUse →
// dynamic_cast<IGrowable> 返 nullptr（NetherWartBlock 不实现 IGrowable）→ Fail → useItemOnBlock 返 false。
function netherWartBonemealHasNoEffect(test: Test): void {
    placeSoulSandAndWart(test, 0);
    test.assert(getWartAge(test) === 0, `nether_wart age should be 0 before, got ${getWartAge(test)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对下界疣 useItemOnBlock 骨粉 → BoneMealItem::onItemUse dynamic_cast<IGrowable> 返 nullptr → Fail。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        WART_POS,
        Direction.Up,
    );
    test.assert(!used, `useItemOnBlock should return false when bonemealing nether_wart (no IGrowable), got used=${used}`);

    // 判定：age 仍为 0（骨粉无效，未生长）。
    test.assert(
        getWartAge(test) === 0,
        `nether_wart bonemeal no-effect: age should remain 0, got ${getWartAge(test)} ` +
            `(if age>0, grow may have been wrongly invoked; if used=true, IGrowable cast may be missing)`,
    );

    test.succeed();
}

// randomTick 10% 概率生长（wiki: 每个随机刻都有10%的概率生长一个阶段）。
// 布局：(3,1,1) soul_sand，上方 (3,2,1) nether_wart(age=0)。
// 调高 randomTickSpeed 使下界疣格在数 tick 内被随机刻命中。轮询 age >= 1。
function netherWartGrowsViaRandomTick(test: Test): void {
    placeSoulSandAndWart(test, 0);
    test.assert(getWartAge(test) === 0, `nether_wart age should be 0 before, got ${getWartAge(test)}`);

    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    player.chat(`/gamerule randomTickSpeed ${HIGH_RANDOM_TICK_SPEED}`);

    pollUntilSucceed(
        test,
        () => getWartAge(test) >= 1,
        {
            startTick: 40,
            interval: 20,
            maxTick: 160,
            onTimeout: () => {
                test.assert(
                    false,
                    `nether_wart random-tick growth: expected age>=1, got ${getWartAge(test)} ` +
                        `(if age=0, randomTickSpeed may not be raised or randomTick 10% growth missing)`,
                );
            },
        },
    );
}

export function registerNetherWartTests(): void {
    GameTest.register("BlockBehaviorTests", "nether_wart_bonemeal_has_no_effect", netherWartBonemealHasNoEffect)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "nether_wart_grows_via_random_tick", netherWartGrowsViaRandomTick)
        .structureName("gametests:glass_pit")
        .maxTicks(280);
}
