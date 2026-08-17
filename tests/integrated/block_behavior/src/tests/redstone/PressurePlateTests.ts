// 压力板实体探测行为 GameTest。
//
// wiki block_压力板.txt#用途/红石元件：压力板探测其上方的实体（玩家/生物/物品实体）。
//   - 开启条件：实体在压力板所在方块的检测框 (0.125,0,0.125)-(0.875,0.25,0.875) 内（即使碰撞箱
//     只有一部分在其中）。
//   - 木压力板：探测所有实体（含物品实体），开启时信号 15。
//   - 石压力板：只探测生物（不含物品实体），开启时信号 15。
//   - 木/石压力板每 20 红石刻（10 游戏刻）更新一次状态；测重压力板每 10 红石刻（5 游戏刻）。
//   - 开启时强充能附着方块、激活毗邻机械元件、提供红石信号；压力板无碰撞箱，不阻碍实体移动。
//
// C++ 链路：AbstractPressurePlateBlock（AbstractPressurePlateBlock.cpp）有 powered（POWERED bool，
//   默认 false）state（木/石类）。测重类重写为 POWER_0_15。
//   - onEntityCollision 调度 tick；tick 调 calculateSignalStrength（木类查询检测 AABB 内实体返回 15/0；
//     石类只统计生物；测重类统计实体数）并更新 powered + 通知邻居。
//   - hasEntityOnPlate 用 0.125-0.875 x 0.0-0.25 检测框（对齐 wiki 检测框）。
//   - 木类 calculateSignalStrength 探测所有实体；石类只探测 Mob/生物（对齐 wiki「石压力板只探测生物」）。
//
// 测试覆盖（4 个场景，覆盖 wiki 实体探测 + 木/石差异核心行为）：
//   1. 木压力板被生物触发开启：猪站到木压力板上 → POWERED 翻 true（探测所有实体）。
//   2. 石压力板被生物触发开启：猪站到石压力板上 → POWERED 翻 true（探测生物）。
//   3. 木压力板被物品实体触发开启：物品落到木压力板上 → POWERED 翻 true（木类探测物品实体）。
//   4. 石压力板不被物品触发：物品落到石压力板上 → POWERED 保持 false（石类不探测物品实体）。
//
// 关键约束：
// 1. 压力板每 10gt 更新状态，实体站上去后最多 10gt 延迟才 POWERED=true，pollUntilSucceed 留足余量。
// 2. 压力板无碰撞箱，实体实际站在下方支撑方块上。压力板放 (3,2,1)，下方 (3,1,1) 放 stone 支撑，
//    实体 spawn (3,3,1) 落到压力板上方（实体脚进入检测框触发）。
// 3. 压力板须放在「方块支撑形状上表面完整」的方块上方——(3,1,1) stone 提供支撑。
// 4. 读 powered state 用 getState("powered" as any) 绕过白名单。
// 5. 物品实体用 test.spawnItem 生成（落在压力板检测框内触发木压力板）。
//    物品须静止落到压力板检测框 (0.125,0,0.125)-(0.875,0.25,0.875) 内才触发——这依赖
//    ItemEntity::tick 无条件调 doBlockCollisions()（见 ItemEntity.cpp:tick 注释），物品静止后仍每 tick
//    触发 onEntityCollision，压力板才能探测到物品实体。
//
// 不测「测重压力板信号随实体数变化」：需精确控制多实体计数 + 信号强度断言，时序与计数非确定，跳过。
//   TODO: 可补 weighted_pressure_plate_signal_scales_with_entity_count。
// 不测「强充能附着方块/激活毗邻机械元件」：需放红石粉/灯链路，复杂，跳过。
// 不测「压力板支撑失效掉落」：与 Fence/Wall 支撑测试同类，跳过。
//
// 跨服务端：压力板 powered state 名两端一致，实体探测+木/石差异两端一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_压力板.txt#用途（探测实体，木=所有实体，石=仅生物）
// Ref: AbstractPressurePlateBlock.cpp（onEntityCollision 调度 tick，calculateSignalStrength 木/石差异）
// Ref: hasEntityOnPlate 检测框 (0.125,0,0.125)-(0.875,0.25,0.875) 对齐 wiki
// Ref: ItemEntity.cpp:tick（无条件 doBlockCollisions，物品静止仍触发 onEntityCollision）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 压力板放 (3,2,1)，下方 (3,1,1) 放 stone 支撑（压力板须放在支撑形状完整的方块上方）。
// 实体 spawn (3,3,1) 落到压力板上方（实体脚进入检测框触发）。

// 读取压力板 powered state（bool）。返回 null 表示读取失败或非压力板。
function getPlatePowered(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("powered" as any);
    return typeof value === "boolean" ? value : null;
}

// 在 (3,1,1) 放 stone 支撑 + (3,2,1) 放压力板。压力板须放在支撑形状完整的方块上方。
function placePlateOnStone(test: Test, plateType: string): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType(plateType, { x: 3, y: 2, z: 1 });
}

// 场景 1：木压力板被生物（猪）触发开启 → POWERED 翻 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 木压力板，猪 spawn (3,3,1) 落到压力板上方。
// 猪碰撞箱脚部进入检测框 (0.125,0,0.125)-(0.875,0.25,0.875) → onEntityCollision 调度 tick →
// calculateSignalStrength（木类探测所有实体，返回 15）→ powered=true。
//
// 判定：pollUntilSucceed 轮询 powered===true（压力板每 10gt 更新，留余量）。
function woodenPressurePlatePowersWhenMobStands(test: Test): void {
    placePlateOnStone(test, "minecraft:oak_pressure_plate");

    // 猪 spawn (3,3,1) 落到压力板上方（自由落体 1 格，脚进入检测框）。
    test.spawn("pig", { x: 3, y: 3, z: 1 });

    // 轮询断言 powered === true（木压力板探测所有实体，猪触发开启）。
    // startTick=10 留猪落地 + 压力板 10gt 更新余量；interval=4；maxTick=60。
    pollUntilSucceed(
        test,
        () => getPlatePowered(test, 3, 2, 1) === true,
        {
            startTick: 10,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `wooden pressure plate powered: should be true when pig stands on it, got ${getPlatePowered(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 2：石压力板被生物（猪）触发开启 → POWERED 翻 true。
//
// 布局：同场景 1，压力板换石压力板。猪 spawn (3,3,1)。
// 石压力板 calculateSignalStrength 只探测生物（猪是生物）→ powered=true。
//
// 判定：pollUntilSucceed 轮询 powered===true。
function stonePressurePlatePowersWhenMobStands(test: Test): void {
    placePlateOnStone(test, "minecraft:stone_pressure_plate");

    test.spawn("pig", { x: 3, y: 3, z: 1 });

    pollUntilSucceed(
        test,
        () => getPlatePowered(test, 3, 2, 1) === true,
        {
            startTick: 10,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `stone pressure plate powered: should be true when pig stands on it, got ${getPlatePowered(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 3：木压力板被物品实体触发开启 → POWERED 翻 true（wiki 关键差异：木类探测物品实体）。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 木压力板，spawnItem 在 (3.5,3,1.5) 生成物品落到压力板上方。
// 物品自由落体落到压力板检测框 (0.125,0,0.125)-(0.875,0.25,0.875) 内 → ItemEntity::tick 无条件
// doBlockCollisions → onEntityCollision 调度 tick → calculateSignalStrength（木类探测所有实体含物品，
// 返回 15）→ powered=true。
//
// 判定：pollUntilSucceed 轮询 powered===true。物品有拾取延迟+下落+10gt 更新，留足余量。
// spawnItem 签名：Cubium 是 (itemType: string, location)——简化实现（ScriptTestHelper.cpp:911-914），
// 偏离官方基岩 (itemStack: ItemStack, location)。Cubium ItemStack JS 类未充实（空壳），故用字符串。
// TODO: Cubium ItemStack JS 类充实后改回 ItemStack 对象以对齐官方签名。
function woodenPressurePlatePowersWhenItemDropped(test: Test): void {
    placePlateOnStone(test, "minecraft:oak_pressure_plate");

    // spawnItem 在压力板上方生成物品实体（落在检测框内触发木压力板）。用 stone 物品（常见，无特殊行为）。
    (test.spawnItem as any)("minecraft:stone", { x: 3.5, y: 3, z: 1.5 });

    // 轮询断言 powered === true（木压力板探测物品实体）。startTick=10 留物品下落+压力板 10gt 更新余量。
    pollUntilSucceed(
        test,
        () => getPlatePowered(test, 3, 2, 1) === true,
        {
            startTick: 10,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `wooden pressure plate powered: should be true when item dropped on it, got ${getPlatePowered(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 4：石压力板不被物品触发（保持 false）——wiki 关键差异：石类只探测生物，不探测物品实体。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 石压力板，spawnItem 在 (3.5,3,1.5) 生成物品落到压力板上方。
// 物品落到石压力板检测框内 → onEntityCollision 调度 tick → calculateSignalStrength（石类只探测生物，
// 物品非生物返回 0）→ powered 保持 false。
//
// 判定：不能用 pollUntilSucceed(powered===false)（首 tick 即满足，无法区分「保持 false」与「尚未触发」）。
// 用显式多时间点断言：在压力板更新窗口后（tick 15，>10gt 更新周期）断言 powered 仍 false，tick 30 再断言。
function stonePressurePlateIgnoresItem(test: Test): void {
    placePlateOnStone(test, "minecraft:stone_pressure_plate");

    // spawnItem 在石压力板上方生成物品实体（Cubium 字符串签名，见上方说明）。
    (test.spawnItem as any)("minecraft:stone", { x: 3.5, y: 3, z: 1.5 });

    // 石压力板不探测物品实体 → powered 保持 false。在压力板更新窗口后断言 powered 仍 false。
    test.runAtTickTime(15, () => {
        const powered = getPlatePowered(test, 3, 2, 1);
        test.assert(powered === false, `stone pressure plate should ignore items (powered should stay false at tick 15), got powered=${powered}`);
    });
    test.runAtTickTime(30, () => {
        const powered = getPlatePowered(test, 3, 2, 1);
        test.assert(powered === false, `stone pressure plate should still ignore items at tick 30, got powered=${powered}`);
        test.succeed();
    });
}

export function registerPressurePlateTests(): void {
    GameTest.register("BlockBehaviorTests", "wooden_pressure_plate_powers_when_mob_stands", woodenPressurePlatePowersWhenMobStands)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "stone_pressure_plate_powers_when_mob_stands", stonePressurePlatePowersWhenMobStands)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "wooden_pressure_plate_powers_when_item_dropped", woodenPressurePlatePowersWhenItemDropped)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "stone_pressure_plate_ignores_item", stonePressurePlateIgnoresItem)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
