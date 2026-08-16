// 龙蛋右键传送行为 GameTest。
//
// wiki tech_龙蛋.txt：龙蛋右键（攻击也）触发传送，随机移到附近 16×16×16 区域内空气位（1.21.9 后
//   不传送至虚空/高度限制之上）。传送后原位变 air，新位放龙蛋。传送位置随机，仅能断言原位变化。
//
// C++ 链路：DragonEggBlock（end/DragonEggBlock.cpp）。
//   - onBlockActivated（:58）：不检查手持物，直接 _teleport → return Success（即使传送失败也 Success）。
//   - _teleport（:84）：随机找 1000 次内 16×16×16 区域空气位，服务端在新位 setBlockState 龙蛋 +
//     原位 setBlockState air。
//   - attack（:76）：左键也触发 _teleport，但 SimulatedPlayer.attack 为 stub（MethodNotImplemented），
//     不可测，故本组只测右键传送。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated）。龙蛋
//   onBlockActivated 始终返 Success（不检查手持物），短路不 fallback。用手持 stick 触发（onBlockActivated
//   不检查手持物，stick 不被消耗，仅占位 useItemOnBlock 的 ItemStack 形参）。
//
// 测试覆盖（1 个场景，覆盖 wiki 右键传送核心行为）：
//   1. 右键传送原位变 air：放龙蛋 + stick useItemOnBlock → 原位 (3,2,1) 变 air，返 true。
//
// 关键约束：
// 1. 龙蛋完整方块无需支撑，直接放 (3,2,1)（minecraft:dragon_egg）。
// 2. 传送目标位置随机，仅断言原位变 air（不能断言新位置，随机性非确定）。
// 3. glass_pit 内空气充足，_teleport 1000 次尝试必能找到空气位，传送必成功。
// 4. 用手持 stick 触发（onBlockActivated 不检查手持物，stick 不被消耗）。
// 5. 传送是服务端逻辑（!isClientSide），GameTest 服务端跑，走服务端分支。
//
// 不测「攻击触发传送」：SimulatedPlayer.attack 为 stub，不可测。TODO: 待 attack API 就绪后补。
// 不测「传送新位置」：传送位置随机，非确定，无法断言具体坐标。
// 不测「不传送至虚空/高度限制」：需构造边界场景，复杂且边缘，跳过。
//
// 跨服务端：龙蛋 dragon_egg 方块名两端一致，右键传送原位变 air 行为两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_龙蛋.txt（右键/攻击传送，16×16×16 范围）
// Ref: DragonEggBlock.cpp（onBlockActivated→_teleport→Success；_teleport 服务端新位放蛋+原位 air）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 龙蛋 (3,2,1)，完整方块无需支撑。

// 读取 (x,y,z) 方块 typeId。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 场景 1：右键传送原位变 air——放龙蛋 + stick useItemOnBlock → 原位 (3,2,1) 变 air，返 true。
//
// 布局：(3,2,1) 龙蛋。
// onBlockActivated：不检查手持物 → _teleport → 服务端随机找空气位放蛋 + 原位 air → return Success。
//
// 判定：useItemOnBlock 返 true（Success），原位 (3,2,1) typeId === "minecraft:air"（传送后原位变 air）。
function dragonEggTeleportsOnUse(test: Test): void {
    test.setBlockType("minecraft:dragon_egg", { x: 3, y: 2, z: 1 }); // 龙蛋
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:dragon_egg", `dragon egg should be at (3,2,1) before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对龙蛋 useItemOnBlock stick → onBlockActivated → _teleport → 原位 air → Success。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when teleporting dragon egg");

    // 判定：原位 (3,2,1) 变 air（传送后原位置变空气）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:air", `dragon egg pos should be air after teleport, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerDragonEggTests(): void {
    GameTest.register("BlockBehaviorTests", "dragon_egg_teleports_on_use", dragonEggTeleportsOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
