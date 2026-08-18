// Dimension.id 属性测试：验证 Dimension.id 返回正确的维度名字符串。
//
// 基岩 API（@minecraft/server Dimension）：readonly id: string，返回维度名（如 "minecraft:overworld"）。
// Cubium 绑定（MinecraftModuleFactory.cpp Dimension.id）：读 IWorld::dimension() 映射到基岩维度名
// （OVERWORLD→"minecraft:overworld"，NETHER→"minecraft:nether"，THE_END→"minecraft:the_end"）。
// 注意输出侧用基岩名 "minecraft:nether"（非内部注册表名 "minecraft:the_nether"），与 world.getDimension
// 读入侧归一化对称。
//
// 跨服务端：overworld.id 两端可测；nether.id/the_end.id 基岩 GameTest 可能无下界/末地维度，加守卫，
// 倾向 Cubium one-sided。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

function dimensionIdReturnsCorrectString(test: Test): void {
    // 主世界 id：两端可测。
    const overworld = test.getDimension();
    test.assert(overworld.id === "minecraft:overworld", `overworld id mismatch: ${overworld.id}`);

    // 下界 id：getDimension 可能返回 undefined（维度未加载/回调未注册），加守卫。
    const nether = world.getDimension("minecraft:nether");
    if (nether !== undefined) {
        test.assert(nether.id === "minecraft:nether", `nether id mismatch: ${nether.id}`);
    }

    // 末地 id：同上守卫。
    const end = world.getDimension("minecraft:the_end");
    if (end !== undefined) {
        test.assert(end.id === "minecraft:the_end", `the_end id mismatch: ${end.id}`);
    }

    test.succeed();
}

export function registerDimensionIdTests(): void {
    GameTest.register("TeleportTests", "dimension_id_returns_correct_string", dimensionIdReturnsCorrectString)
        .structureName("gametests:glass_pit")
        .maxTicks(20);
}
