// 打火石/火焰弹点燃下界传送门测试：验证玩家手持打火石或火焰弹右键黑曜石框架内部时，
// 放置 fire 方块并触发 FireBlock::onBlockAdded → tryLightNetherPortal，生成 nether_portal 方块。
//
// wiki 机制（world_下界传送门.txt#创建传送门、tech_打火石.txt、tech_火焰弹.txt）：
//   - 下界传送门可通过打火石或火焰弹在黑曜石框架内部点燃火焰来激活。
//   - 打火石：在相邻空气位置放置 fire 方块，消耗 1 耐久（非数量），64 次使用后损坏。
//   - 火焰弹：在相邻空气位置放置 fire 方块，消耗 1 个火焰弹（非创造模式）。
//   - 火方块放置后，FireBlock::onBlockAdded 检测维度（仅主世界/下界）并调用 tryLightNetherPortal。
//   - tryLightNetherPortal 调用 PortalSize::findNetherPortal 检测框架，成功则 lightNetherPortal。
//
// Cubium 实现：
//   - FlintAndSteelItem::onItemUse（FlintAndSteelItem.cpp:48）：
//     * 检查点击方块是否含 LIT 属性且未点燃 → 点燃（营火/蜡烛等）。
//     * 否则在点击面相邻空气位置放火（getFireForPlacement → 普通火/灵魂火）。
//     * 消耗耐久：hurtAndBreak(heldItem, 1, player, MainHand)（非数量消耗）。
//     * flags=11（NOTIFY_LISTENERS | NO_NEIGHBORDROP）触发 onBlockAdded。
//   - FireChargeItem::onItemUse（FireChargeItem.cpp:54）：
//     * 检查 LIT 属性方块 → 点燃。
//     * 否则相邻空气位置放火。
//     * 消耗：非创造模式 context.getItemStackMut().shrink(1)。
//   - FireBlock::onBlockAdded → tryLightNetherPortal（FireBlock.cpp:287/314）。
//   - SimulatedPlayer::useItemOnBlock（SimulatedPlayer.cpp:314）：
//     * 先 Block.use（onBlockActivated），返回 Pass 才 fallback Item.useOn。
//     * 把传入 stack 设到主手选中槽作为权威手持物源。
//     * Item.useOn 成功后对选中栈 shrink(1) 回写（非创造模式）。
//     * 若 onItemUse 已改变权威槽 itemId（自管理替换）或 damage（耐久损耗），跳过 shrink。
//
// 测试结构（glass_pit 7×5×7）：
//   - 内部空间 X∈[1,5], Z∈[1,5], Y∈[1,3]（5×5×3）。
//   - Y=0 实心玻璃底板（可替换为黑曜石底框）。
//   - Y=4 玻璃顶（可替换为黑曜石顶框）。
//
// 测试覆盖（4 个核心行为点）：
//   1. flint_and_steel_ignites_portal：打火石右键黑曜石框架内部，激活 nether_portal。
//   2. flint_and_steel_durability_cost：打火石点火消耗 1 耐久（非数量），验证耐久损耗链路。
//   3. fire_charge_ignites_portal：火焰弹右键黑曜石框架内部，激活 nether_portal。
//   4. fire_charge_consumed：火焰弹点火消耗 1 个（非创造模式），验证数量消耗链路。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界传送门.txt#创建传送门
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_打火石.txt
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_火焰弹.txt
// Ref: FlintAndSteelItem.cpp#onItemUse（Cubium src/common/item/items/special/）
// Ref: FireChargeItem.cpp#onItemUse（Cubium src/common/item/items/weapon/）
// Ref: SimulatedPlayer.cpp#useItemOnBlock:314（Cubium src/server/test/simulated/）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, GameMode, Direction } from "@minecraft/server";

// 读取 (x,y,z) 方块 typeId。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 (x,y,z) 方块的 axis 状态（"x"/"z"）。返回空串表示读取失败。
function getBlockAxis(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return "";
    }
    const value = block?.permutation?.getState("axis" as any);
    return typeof value === "string" ? value : "";
}

// 放置黑曜石方块（框架方块）。
function placeObsidian(test: Test, x: number, y: number, z: number): void {
    test.setBlockType("minecraft:obsidian", { x, y, z });
}

// 清空指定位置为空气（确保内部为空气）。
function clearToAir(test: Test, x: number, y: number, z: number): void {
    test.setBlockType("minecraft:air", { x, y, z });
}

// 构建 X 轴向下界传送门框架（内部宽 2、高 3）。
// 布局：glass_pit 内部放置最小 X 轴向框架。
//   - 内部左下角 (3,1,3)，内部宽 2（X=3,4）、高 3（Y=1,2,3）。
//   - 底框 Y=0（X=2..5）、顶框 Y=4（X=2..5）。
//   - 左框 X=2、右框 X=5（Y=1..3）。
function buildMinXAxisPortalFrame(test: Test): void {
    const innerX = 3;
    const innerY = 1;
    const innerZ = 3;
    const internalW = 2;
    const internalH = 3;

    // 底框（Y=0，X=2..5）。
    for (let dx = 0; dx < internalW + 2; dx++) {
        placeObsidian(test, innerX + dx - 1, innerY - 1, innerZ);
    }
    // 顶框（Y=4，X=2..5）。
    for (let dx = 0; dx < internalW + 2; dx++) {
        placeObsidian(test, innerX + dx - 1, innerY + internalH, innerZ);
    }
    // 左框（X=2，Y=1..3）。
    for (let dy = 0; dy < internalH; dy++) {
        placeObsidian(test, innerX - 1, innerY + dy, innerZ);
    }
    // 右框（X=5，Y=1..3）。
    for (let dy = 0; dy < internalH; dy++) {
        placeObsidian(test, innerX + internalW, innerY + dy, innerZ);
    }
    // 清空内部（确保是空气）。
    for (let dx = 0; dx < internalW; dx++) {
        for (let dy = 0; dy < internalH; dy++) {
            clearToAir(test, innerX + dx, innerY + dy, innerZ);
        }
    }
}

// 检查内部区域是否全部为 nether_portal 方块且轴向正确。
function isPortalFormed(
    test: Test,
    innerX: number,
    innerY: number,
    innerZ: number,
    internalW: number,
    internalH: number,
    axis: string,
): boolean {
    for (let d = 0; d < internalW; d++) {
        for (let dy = 0; dy < internalH; dy++) {
            let x = innerX;
            let z = innerZ;
            if (axis === "x") {
                x = innerX + d;
            } else {
                z = innerZ + d;
            }
            const pos = { x, y: innerY + dy, z };
            if (getBlockTypeId(test, pos.x, pos.y, pos.z) !== "minecraft:nether_portal") {
                return false;
            }
            if (getBlockAxis(test, pos.x, pos.y, pos.z) !== axis) {
                return false;
            }
        }
    }
    return true;
}

// 场景 1：打火石右键黑曜石框架内部，激活 nether_portal。
//
// wiki 机制（world_下界传送门.txt#创建传送门、tech_打火石.txt#用途）：
//   - 打火石可点燃黑曜石框架内部，激活为下界传送门。
//   - 打火石放置 fire 方块后，FireBlock::onBlockAdded 触发 tryLightNetherPortal。
//
// 布局：glass_pit 内部放置最小 X 轴向框架（内部 2×3）。
//   - 玩家手持打火石，对底框黑曜石 (3,0,3) 顶面（Direction.Up）使用 useItemOnBlock。
//   - 打火石在 (3,1,3)（点击面上方相邻空气）放置 fire 方块。
//   - fire 方块 onBlockAdded → tryLightNetherPortal → lightNetherPortal。
//
// 判定：内部 2×3 区域全部为 nether_portal（axis=x）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_打火石.txt#用途
// Ref: FlintAndSteelItem.cpp#onItemUse（放火分支）
// Ref: FireBlock.cpp#onBlockAdded（tryLightNetherPortal）
function flintAndSteelIgnitesPortal(test: Test): void {
    buildMinXAxisPortalFrame(test);

    // spawn SimulatedPlayer（生存模式，验证耐久消耗），手持 1 个 flint_and_steel。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "igniter", GameMode.survival);
    const flintAndSteel = new ItemStack("minecraft:flint_and_steel", 1);

    // 对底框黑曜石 (3,0,3) 顶面使用打火石。
    // 打火石在点击面相邻空气位置放火：(3,0,3) 顶面 → (3,1,3) 放 fire 方块。
    const used = player.useItemOnBlock(
        flintAndSteel as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 0, z: 3 },
        Direction.Up,
    );

    test.assert(used, "useItemOnBlock should return true when using flint_and_steel on obsidian");

    // 验证内部 2×3 区域全部为 nether_portal（axis=x）。
    test.assert(
        isPortalFormed(test, 3, 1, 3, 2, 3, "x"),
        "flint_and_steel should ignite nether portal blocks",
    );

    test.succeed();
}

// 场景 2：打火石点火消耗 1 耐久（非数量），验证耐久损耗链路。
//
// wiki 机制（tech_打火石.txt#用途）：打火石每次使用消耗 1 耐久，64 次使用后损坏。
//   - 耐久损耗由 hurtAndBreak(heldItem, 1, player, MainHand) 处理。
//   - 数量不消耗（耐久损耗 ≠ 数量消耗）。
//
// 布局：同场景 1，玩家手持 1 个 flint_and_steel（耐久满 0）。
//   - 对底框黑曜石顶面使用打火石。
//
// 判定：
//   1. 打火石耐久从 0 变为 1（damage +1）。
//   2. 打火石数量仍为 1（未消耗数量）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_打火石.txt#用途
// Ref: FlintAndSteelItem.cpp#onItemUse（hurtAndBreak 耐久损耗）
function flintAndSteelDurabilityCost(test: Test): void {
    buildMinXAxisPortalFrame(test);

    // spawn SimulatedPlayer（生存模式），手持 1 个 flint_and_steel（耐久满 0）。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "igniter", GameMode.survival);
    const flintAndSteel = new ItemStack("minecraft:flint_and_steel", 1);

    // 对底框黑曜石 (3,0,3) 顶面使用打火石。
    const used = player.useItemOnBlock(
        flintAndSteel as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 0, z: 3 },
        Direction.Up,
    );

    test.assert(used, "useItemOnBlock should return true when using flint_and_steel on obsidian");

    // 验证耐久损耗：damage 从 0 变为 1。
    // 读取主手物品经 equippable.getEquipment("Mainhand")（返回 owned ItemStack | undefined）。
    const equippable = player.getComponent("minecraft:equippable") as any;
    const heldItem = equippable?.getEquipment?.("Mainhand");
    test.assert(
        heldItem !== undefined && heldItem.typeId === "minecraft:flint_and_steel" && heldItem.amount === 1,
        `flint_and_steel should not be consumed (amount 1), got ${heldItem?.typeId ?? "empty"} amount ${heldItem?.amount ?? 0}`,
    );
    // 耐久损耗：damage 从 0 变为 1（每次使用 +1 耐久损耗）。
    const damage = heldItem?.getComponent?.("minecraft:durability")?.damage
        ?? (heldItem as any)?.damage
        ?? -1;
    test.assert(
        damage === 1,
        `flint_and_steel durability damage should be 1 (cost 1 durability), got ${damage}`,
    );

    test.succeed();
}

// 场景 3：火焰弹右键黑曜石框架内部，激活 nether_portal。
//
// wiki 机制（world_下界传送门.txt#创建传送门、tech_火焰弹.txt#用途）：
//   - 火焰弹可点燃黑曜石框架内部，激活为下界传送门。
//   - 火焰弹放置 fire 方块后，FireBlock::onBlockAdded 触发 tryLightNetherPortal。
//
// 布局：glass_pit 内部放置最小 X 轴向框架（内部 2×3）。
//   - 玩家手持火焰弹，对底框黑曜石 (3,0,3) 顶面使用 useItemOnBlock。
//   - 火焰弹在 (3,1,3) 放置 fire 方块。
//
// 判定：内部 2×3 区域全部为 nether_portal（axis=x）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_火焰弹.txt#用途
// Ref: FireChargeItem.cpp#onItemUse（放火分支）
function fireChargeIgnitesPortal(test: Test): void {
    buildMinXAxisPortalFrame(test);

    // spawn SimulatedPlayer（生存模式），手持 1 个 fire_charge。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "igniter", GameMode.survival);
    const fireCharge = new ItemStack("minecraft:fire_charge", 1);

    // 对底框黑曜石 (3,0,3) 顶面使用火焰弹。
    const used = player.useItemOnBlock(
        fireCharge as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 0, z: 3 },
        Direction.Up,
    );

    test.assert(used, "useItemOnBlock should return true when using fire_charge on obsidian");

    // 验证内部 2×3 区域全部为 nether_portal（axis=x）。
    test.assert(
        isPortalFormed(test, 3, 1, 3, 2, 3, "x"),
        "fire_charge should ignite nether portal blocks",
    );

    test.succeed();
}

// 场景 4：火焰弹点火消耗 1 个（非创造模式），验证数量消耗链路。
//
// wiki 机制（tech_火焰弹.txt#用途）：火焰弹每次使用消耗 1 个。
//   - FireChargeItem::onItemUse 内部 context.getItemStackMut().shrink(1) 消耗。
//
// 布局：同场景 3，玩家手持 1 个 fire_charge。
//   - 对底框黑曜石顶面使用火焰弹。
//
// 判定：
//   1. 火焰弹数量从 1 变为 0（消耗 1 个）。
//   2. 主手槽为空（已消耗）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_火焰弹.txt#用途
// Ref: FireChargeItem.cpp#onItemUse（shrink(1) 消耗）
function fireChargeConsumed(test: Test): void {
    buildMinXAxisPortalFrame(test);

    // spawn SimulatedPlayer（生存模式），手持 1 个 fire_charge。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 1, z: 1 }, "igniter", GameMode.survival);
    const fireCharge = new ItemStack("minecraft:fire_charge", 1);

    // 对底框黑曜石 (3,0,3) 顶面使用火焰弹。
    const used = player.useItemOnBlock(
        fireCharge as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 0, z: 3 },
        Direction.Up,
    );

    test.assert(used, "useItemOnBlock should return true when using fire_charge on obsidian");

    // 验证消耗：主手 fire_charge 数量应为 0（消耗 1 个）。
    const equippable = player.getComponent("minecraft:equippable") as any;
    const heldItem = equippable?.getEquipment?.("Mainhand");
    test.assert(
        heldItem === undefined || heldItem.typeId !== "minecraft:fire_charge",
        `fire_charge should be consumed (count 0), got ${heldItem?.typeId ?? "empty"} count ${heldItem?.amount ?? 0}`,
    );

    test.succeed();
}

export function registerFlintAndSteelIgnitesTests(): void {
    GameTest.register("TeleportTests", "flint_and_steel_ignites_portal", flintAndSteelIgnitesPortal)
        .structureName("gametests:glass_pit")
        .maxTicks(40);

    GameTest.register("TeleportTests", "flint_and_steel_durability_cost", flintAndSteelDurabilityCost)
        .structureName("gametests:glass_pit")
        .maxTicks(40);

    GameTest.register("TeleportTests", "fire_charge_ignites_portal", fireChargeIgnitesPortal)
        .structureName("gametests:glass_pit")
        .maxTicks(40);

    GameTest.register("TeleportTests", "fire_charge_consumed", fireChargeConsumed)
        .structureName("gametests:glass_pit")
        .maxTicks(40);
}
