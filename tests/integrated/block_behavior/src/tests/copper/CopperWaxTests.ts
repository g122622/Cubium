// 铜方块涂蜡与去蜡行为 GameTest。
//
// wiki other_铜.txt#涂蜡：蜜脾右键未涂蜡铜方块 → 涂蜡变体（阻止氧化）；斧头右键涂蜡铜方块 → 去蜡
//   回未涂蜡变体。涂蜡/去蜡是方块类型替换（typeId 变化），非同方块 state 变化。
//   - 未涂蜡 copper_block + 蜜脾右键 → waxed_copper_block（涂蜡，阻止氧化）。
//   - 涂蜡 waxed_copper_block + 斧头右键 → copper_block（去蜡）。
//   - 铜方块于 1.17 加入，1.21.11 已包含，属 vanilla 正式特性。
//
// C++ 链路：
//   - copper_block 是 WeatheringCopperBlock（WeatheringCopperBlock.hpp:48，实现 IOxidizableBlock），
//     无 onBlockActivated 重写 → 基类返 Pass → fallback Item.useOn。
//   - waxed_copper_block 是 WaxedCopperBlock（WeatheringCopperBlock.hpp:122，: public Block 不实现
//     IOxidizableBlock），无 onBlockActivated → fallback Item.useOn。
//   - 蜜脾 HoneycombItem::onItemUse（HoneycombItem.cpp:47-103）：告示牌分支（非铜）→ 铜块分支：
//     getWaxed(state) 查 getWaxablesMap（copper_block→waxed_copper_block，:150-151）→
//     setBlockState(waxed, 11) + WAX_ON 事件 → Success。
//   - 斧头 AxeItem::onItemUse（AxeItem.cpp:57-114）：MC Java 顺序 1.去皮 2.去氧化 3.除蜡。
//     对 waxed_copper_block：步骤1 非原木跳过；步骤2 waxed_copper_block 非 IOxidizableBlock 跳过；
//     步骤3 getWaxedOff(waxed_copper_block) → copper_block state → setBlockState(copper, 11) +
//     WAX_OFF 事件 + hurtAndBreak(斧头耐久-1) → Success。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。铜方块 onBlockActivated（基类）返 Pass → fallback 蜜脾/斧头 onItemUse。
//   useItemOnBlock 调 onBlockActivated 前把 stack 设到主手选中槽，使 onItemUse 读到蜜脾/斧头。
//   创造模式蜜脾/斧头不消耗（不 shrink/hurtAndBreak 由 isCreative 守卫或 useItemOnBlock 第331行守卫）。
//
// 测试覆盖（2 个场景，覆盖 wiki 涂蜡+去蜡往返核心确定行为）：
//   1. 蜜脾涂蜡：copper_block + 蜜脾 useItemOnBlock → waxed_copper_block（typeId 变化），返 true。
//   2. 斧头去蜡：waxed_copper_block + 斧头 useItemOnBlock → copper_block（typeId 变化），返 true。
//   两场景互补，构成 copper_block ↔ waxed_copper_block 完整往返，填补 CopperGolemStatueTests 中
//   记录的「涂蜡/去蜡链路未测」TODO。
//
// 关键约束：
// 1. 铜方块无支撑要求（无 isValidPosition 重写），放 (3,2,1) 即可，下方 (3,1,1) stone 仅为惯例。
// 2. 涂蜡/去蜡是方块类型替换（setBlockState 写新方块 typeId），判定用 block.typeId 变化。
// 3. 创造模式 SimulatedPlayer 蜜脾/斧头不消耗，可重复使用。
// 4. 场景 2 直接 setBlockType 放 waxed_copper_block（涂蜡变体默认 state，无需 setBlockWithStates）。
//
// 不测「去氧化（斧头刮削 exposed_copper→copper_block）」：与去蜡同为 AxeItem 步骤2，是不同行为点
//   （去氧化走 IOxidizableBlock::getPreviousOxidationBlock，去蜡走 getWaxedOff）。可补但本组聚焦
//   涂蜡/去蜡往返。TODO: 待需扩展铜氧化链路覆盖时补 copper_scraped_by_axe。
// 不测「氧化（age 递增）」：走 randomTick 随机刻（5.69% 概率 + 邻居计数），非确定，跳过。
// 不测「铜傀儡像涂蜡变体斧头去蜡」：CopperGolemStatueTests 已记录 TODO，涂蜡/去蜡链路与本组同
//   （HoneycombItem/AxeItem），本组用 copper_block 验证通用链路即可，铜傀儡像去蜡留待其专测补。
//
// 跨服务端：铜方块 copper_block/waxed_copper_block 方块名两端一致，蜜脾涂蜡/斧头去蜡行为与
//   vanilla 一致。两端均可放默认 state 铜方块，涂蜡/去蜡行为两端可对比，非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_铜.txt#涂蜡（蜜脾涂蜡/斧头去蜡）
// Ref: HoneycombItem.cpp（onItemUse 铜块分支 getWaxed→setBlockState waxed；getWaxablesMap copper_block→waxed_copper_block）
// Ref: AxeItem.cpp（onItemUse 步骤3 getWaxedOff→setBlockState unwaxed + WAX_OFF；步骤1去皮/步骤2去氧化优先）
// Ref: WeatheringCopperBlock.hpp（copper_block=WeatheringCopperBlock(IOxidizableBlock)；waxed_copper_block=WaxedCopperBlock(非IOxidizableBlock)）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，Pass fallback onItemUse）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 铜方块 (3,2,1)，下方 (3,1,1) stone 支撑（惯例，铜方块无强制支撑要求）。

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 场景 1：蜜脾涂蜡——copper_block + 蜜脾 useItemOnBlock → waxed_copper_block（typeId 变化），返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) copper_block（未涂蜡铜方块）。
// 蜜脾 useItemOnBlock → 铜块 onBlockActivated（基类 Pass）→ fallback HoneycombItem.onItemUse →
//   getWaxed(copper_block) 查 map → waxed_copper_block → setBlockState(waxed, 11) + WAX_ON → Success。
//
// 判定：useItemOnBlock 返 true（Success），typeId 从 copper_block → waxed_copper_block。
function copperBlockWaxedByHoneycomb(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:copper_block", { x: 3, y: 2, z: 1 }); // 未涂蜡铜方块
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:copper_block", `block should be copper_block before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const honeycomb = new ItemStack("minecraft:honeycomb", 1);

    // 对 copper_block useItemOnBlock 蜜脾 → fallback HoneycombItem.onItemUse → waxed_copper_block → Success。
    const used = farmer.useItemOnBlock(
        honeycomb as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when waxing copper_block with honeycomb");

    // 判定：typeId 从 copper_block → waxed_copper_block（涂蜡，阻止氧化）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:waxed_copper_block", `block should become waxed_copper_block after waxing, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：斧头去蜡——waxed_copper_block + 斧头 useItemOnBlock → copper_block（typeId 变化），返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) waxed_copper_block（涂蜡铜方块，setBlockType 直接放涂蜡变体）。
// 斧头 useItemOnBlock → waxed_copper_block onBlockActivated（基类 Pass）→ fallback AxeItem.onItemUse →
//   步骤1 去皮（非原木跳过）→ 步骤2 去氧化（waxed_copper_block 非 IOxidizableBlock 跳过）→
//   步骤3 getWaxedOff(waxed_copper_block) → copper_block → setBlockState(copper, 11) + WAX_OFF → Success。
//
// 判定：useItemOnBlock 返 true（Success），typeId 从 waxed_copper_block → copper_block。
function waxedCopperBlockUnwaxedByAxe(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:waxed_copper_block", { x: 3, y: 2, z: 1 }); // 涂蜡铜方块
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:waxed_copper_block", `block should be waxed_copper_block before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const axe = new ItemStack("minecraft:diamond_axe", 1);

    // 对 waxed_copper_block useItemOnBlock 斧头 → fallback AxeItem.onItemUse 步骤3 除蜡 → copper_block → Success。
    const used = farmer.useItemOnBlock(
        axe as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when unwaxing waxed_copper_block with axe");

    // 判定：typeId 从 waxed_copper_block → copper_block（去蜡，恢复可氧化）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:copper_block", `block should become copper_block after unwaxing, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerCopperWaxTests(): void {
    GameTest.register("BlockBehaviorTests", "copper_block_waxed_by_honeycomb", copperBlockWaxedByHoneycomb)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "waxed_copper_block_unwaxed_by_axe", waxedCopperBlockUnwaxedByAxe)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
