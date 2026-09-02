// 红石矿石踩踏点亮行为 GameTest。
//
// wiki tech_红石矿石.txt#激活：玩家攻击、踩踏或右键红石矿石时，矿石点亮（lit=true）并发光（光照等级 9）。
//   点亮后经过一段时间（随机刻）会熄灭（lit=false）。
//   redstone_ore 与 deepslate_redstone_ore 共用同一方块类（RedstoneOreBlock），行为一致。
//
// vanilla 对齐（RedStoneOreBlock.java:38-85）：
//   - attack（:38-42）：玩家攻击时调 interact 点亮
//   - stepOn（:44-51）：实体踩踏时，若 !isSteppingCarefully()（非潜行），调 interact 点亮
//   - useItemOn（:53-66）：玩家右键时调 interact 点亮
//   - interact（:68-73）：生成粒子 + 若未点亮则 setBlock(LIT=true)
//   - randomTick（:80-85）：若 LIT=true 则重置为 false（熄灭）
//   - isRandomlyTicking（:75-78）：仅 LIT=true 时随机刻
//
// C++ 链路（RedstoneOreBlock.cpp）：
//   - onEntityWalk（:73-77）：实体踩踏时调 interact 点亮（对齐 vanilla stepOn）
//     调用点 Entity::doBlockCollisions()（Entity.cpp:1518）：m_onGround && !isSteppingCarefully 时每帧触发
//   - attack（:67-71）：玩家攻击时调 interact 点亮（对齐 vanilla attack）
//   - interact（:89-97）：若 !LIT 则 setBlock(LIT=true) + scheduleBlockTick(30)
//   - randomTick（:79-87）：若 LIT 则 setBlock(LIT=false)（熄灭，靠随机刻）
//   - getLightLevel（hpp:56-62）：LIT 时返回 9，否则 0
//
// 测试覆盖（2 个场景，覆盖 wiki 踩踏点亮核心行为 + redstone_ore/deepslate_redstone_ore 共用类验证）：
//   1. redstone_ore_lights_up_when_walked_on：SimulatedPlayer 落到 redstone_ore 上 → onEntityWalk → LIT=true
//   2. deepslate_redstone_ore_lights_up_when_walked_on：SimulatedPlayer 落到 deepslate_redstone_ore 上 → LIT=true
//      验证 deepslate_redstone_ore 复用 RedstoneOreBlock 类，踩踏点亮行为一致
//
// 关键约束：
// 1. 红石矿石是完整方块（1×1×1），放 (3,2,1)（glass_pit 内部 air 腔 helper y=2）。
//    setBlockType 走默认 state（lit=false），放置时不点亮。
// 2. onEntityWalk 需要 m_onGround（实体落地）+ !isSteppingCarefully（非潜行）。
//    SimulatedPlayer 在矿石上方 (3,3,1) 生成，自由落体落到矿石顶部（y=3.0）。
//    落地时 m_onGround=true，doBlockCollisions（每帧调用）检测 belowPos=矿石位置 → onEntityWalk → interact → LIT=true。
// 3. interact 同步 setBlockState(LIT=true)，立即生效。用 pollUntilSucceed 轮询读 lit === true。
//    玩家从 y=3 落到 y=3.0（矿石顶部）约需 5-10 tick（重力加速）。
// 4. 读 lit 用 getState("lit" as any)，值域 true/false。
//
// 不测「randomTick 熄灭」：概率性（m_ticksRandomly=true，randomTick 选中概率取决于 randomTickSpeed），
//   非确定，按准则跳过。TODO: 待随机刻熄灭确定性路径稳定后补 lit_resets_after_random_tick 测试。
//   注：Cubium interact 调 scheduleBlockTick(30) 但 RedstoneOreBlock 未重写 tick（落基类空实现），
//   该 scheduleBlockTick 是死代码，熄灭实际靠 randomTick。故不测「scheduleBlockTick 熄灭」。
// 不测「useItemOn 右键点亮」：需 SimulatedPlayer useItemOnBlock 链路，复杂度高于踩踏，跳过。
//   TODO: 待右键交互链路稳定后补 right_click_lights_up 测试。
// 不测「attack 攻击点亮」：需 SimulatedPlayer attack 链路，复杂度高于踩踏，跳过。
//   TODO: 待攻击交互链路稳定后补 attack_lights_up 测试。
//
// 跨服务端：redstone_ore/deepslate_redstone_ore 方块名两端一致。lit state 名两端一致（布尔）。
//   实体踩踏点亮行为与 vanilla 一致，可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_红石矿石.txt#激活（踩踏/攻击/右键点亮）
// Ref: RedStoneOreBlock.java:38-85（vanilla attack/stepOn/interact/randomTick）
// Ref: RedstoneOreBlock.cpp:67-97（onEntityWalk/attack/interact 点亮链路）
// Ref: Entity.cpp:1518（onEntityWalk 调用点：doBlockCollisions 每帧触发）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 布局：
//   (3,2,1) redstone_ore / deepslate_redstone_ore（完整方块，踩踏目标）
//   (3,3,1) SimulatedPlayer 生成位置（矿石上方，自由落体落到矿石顶部）

const ORE_POS = { x: 3, y: 2, z: 1 };
const SPAWN_POS = { x: 3, y: 3, z: 1 };

// 读取矿石 lit（布尔）。返回 null 表示失败或非矿石。
function getOreLit(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("lit" as any);
    return typeof value === "boolean" ? value : null;
}

// 通用踩踏点亮测试：放指定矿石 → SimulatedPlayer 落到矿石上 → 断言 lit=true。
function oreLightsUpWhenWalkedOn(test: Test, oreType: string, testName: string): void {
    test.setBlockType(oreType, ORE_POS);
    test.spawnSimulatedPlayer(SPAWN_POS, "faller");

    pollUntilSucceed(
        test,
        () => getOreLit(test, ORE_POS.x, ORE_POS.y, ORE_POS.z) === true,
        {
            startTick: 5,
            interval: 2,
            maxTick: 40,
            onTimeout: () => {
                test.assert(
                    false,
                    `${testName}: ore should be lit after player landed on it, got lit=${getOreLit(test, ORE_POS.x, ORE_POS.y, ORE_POS.z)}`,
                );
            },
        },
    );
}

// 场景 1：玩家落到 redstone_ore 上 → onEntityWalk → interact → LIT=true。
function redstoneOreLightsUpWhenWalkedOn(test: Test): void {
    oreLightsUpWhenWalkedOn(test, "minecraft:redstone_ore", "redstone_ore_lights_up_when_walked_on");
}

// 场景 2：玩家落到 deepslate_redstone_ore 上 → onEntityWalk → interact → LIT=true。
// 验证 deepslate_redstone_ore 复用 RedstoneOreBlock 类，踩踏点亮行为一致。
function deepslateRedstoneOreLightsUpWhenWalkedOn(test: Test): void {
    oreLightsUpWhenWalkedOn(test, "minecraft:deepslate_redstone_ore", "deepslate_redstone_ore_lights_up_when_walked_on");
}

export function registerRedstoneOreTests(): void {
    GameTest.register("BlockBehaviorTests", "redstone_ore_lights_up_when_walked_on", redstoneOreLightsUpWhenWalkedOn)
        .structureName("gametests:glass_pit")
        .maxTicks(80);

    GameTest.register(
        "BlockBehaviorTests",
        "deepslate_redstone_ore_lights_up_when_walked_on",
        deepslateRedstoneOreLightsUpWhenWalkedOn,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
