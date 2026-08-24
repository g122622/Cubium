// 炼药锅流体装填行为 GameTest。
//
// wiki tech_炼药锅.txt#装料：炼药锅可用水桶/岩浆桶/水瓶等装填流体。
//   - 空炼药锅 + 水桶右键 → 水炼药锅（water_cauldron，水位 level=3，满）。
//   - 空炼药锅 + 岩浆桶右键 → 岩浆炼药锅（lava_cauldron）。
//   - 空炼药锅 + 水瓶右键 → 水炼药锅（水位 level=1，一瓶）。
//   - 非流体容器物品（如石头）右键炼药锅 → 不触发装填（onBlockActivated 返 Pass）。
//   装填后炼药锅方块类型从 cauldron 变为 water_cauldron/lava_cauldron（不同方块，非同方块 state）。
//
// C++ 链路：CauldronBlock（CauldronBlock.cpp）空炼药锅无 level state（方块类型 cauldron）。
//   - onBlockActivated（CauldronBlock.cpp:143-170）：取手持物，依次 _handleBucketInteraction →
//     _handleBottleInteraction，任一非 Pass 即返回；都 Pass 则整体 Pass。
//   - _handleBucketInteraction（:237-278）：水桶（Items::WATER_BUCKET）→ 替换为 water_cauldron
//     defaultState.with(LEVEL_1_3, 3)（满水位 3）+ 倒水音效 + FLUID_PLACE 事件；非创造模式水桶变空桶。
//     岩浆桶（Items::LAVA_BUCKET）→ 替换为 lava_cauldron。
//   - 装填是方块类型替换（setBlockState 写新方块 state），非同方块 state 变化。
//   - LavaCauldronBlock（LavaCauldronBlock.cpp:94-148）：岩浆炼药锅（lava_cauldron，满/空二态无 level
//     state）。onBlockActivated 空桶（Items::BUCKET）→ setBlockState cauldron（空炼药锅）+ 岩浆桶
//     （创造模式跳过空桶替换）+ ITEM_BUCKET_FILL_LAVA 音效 + FLUID_PICKUP 事件 → Success。
//     岩浆炼药锅始终满，岩浆桶/水桶/其他交互返 Pass（不可再装填）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。炼药锅 onBlockActivated 处理桶/瓶交互返回 Success，短路不 fallback。
//   useItemOnBlock 调 onBlockActivated 前把 stack 设到主手选中槽，使 onBlockActivated 的
//   player.getHeldItem(hand) 读到水桶/岩浆桶。
//
// 测试覆盖（5 个场景，覆盖 wiki 水桶/岩浆桶装填+玻璃瓶取水+空桶取岩浆+非容器不误触发核心行为）：
//   1. 水桶装水：空炼药锅 + 水桶 useItemOnBlock → water_cauldron level=3。
//   2. 岩浆桶装岩浆：空炼药锅 + 岩浆桶 useItemOnBlock → lava_cauldron。
//   3. 非容器不误触发：空炼药锅 + 石头 useItemOnBlock → 仍 cauldron（桶/瓶交互都 Pass，方块不变）。
//   4. 玻璃瓶取水：water_cauldron level=3 + 玻璃瓶 useItemOnBlock → level 3→2（one-sided，setBlockWithStates
//      放满水炼药锅）。
//   5. 空桶取岩浆：lava_cauldron + 空桶 useItemOnBlock → cauldron（空炼药锅）+ 岩浆桶。两端可对比
//      （setBlockType 放 lava_cauldron 默认 state，无需 setBlockWithStates）。
//
// 关键约束：
// 1. 炼药锅需放在固体方块上方（isValidPosition 检查 belowState.isSolid）——(3,1,1) 放 stone 支撑，
//    (3,2,1) 放空炼药锅（minecraft:cauldron）。
// 2. 装填是方块类型替换：判定用 getBlock 检查方块 typeId 是否变为 water_cauldron/lava_cauldron，
//    water_cauldron 再读 level state===3。
// 3. SimulatedPlayer 默认创造模式：水桶/岩浆桶不消耗（创造跳过空桶替换），但仍装水/岩浆（Success 返回）。
// 4. 场景 3 用石头（非桶非瓶）→ onBlockActivated 桶/瓶交互都 Pass → 整体 Pass → fallback Item.useOn
//    （石头无 onItemUse 返 Pass）→ useItemOnBlock 返回 false，炼药锅不变。
//
// 不测「水瓶装水（level=1）」：水瓶交互走 _handleBottleInteraction，涉及 PotionItem/玻璃瓶替换链路，
//   复杂度高于桶，跳过。TODO: 待瓶交互链路验证后补 cauldron_fills_with_water_bottle。
// 不测「降水/滴石填充」：handlePrecipitation 概率性（雨5%/雪10%），receiveStalactiteDrip 需滴石，
//   非确定/复杂，跳过。
// 不测「空桶取水（water_cauldron level3→0 + 水桶）」：与玻璃瓶取水同类（level 递减），本组测玻璃瓶
//   已覆盖「取水 level 递减」行为点，跳过。
//
// 跨服务端：炼药锅 cauldron/water_cauldron/lava_cauldron 方块名两端一致，桶装填行为与 vanilla 一致。
//   注意：基岩 BDS 无 setBlockWithStates，但本测试用 setBlockType 放空炼药锅（默认 state，无需设 level），
//   两端均可放；装填后判定方块类型两端一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_炼药锅.txt#装料（水桶/岩浆桶装填）
// Ref: CauldronBlock.cpp（onBlockActivated 桶/瓶交互；_handleBucketInteraction 水桶→water_cauldron level=3/岩浆桶→lava_cauldron）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 炼药锅 (3,2,1)，下方 (3,1,1) stone 支撑（炼药锅需 solid 上方放置）。

// 读取 (x,y,z) 方块 typeId（Cubium Block 暴露 typeId 属性，非官方 type.id）。返回空串表示读取失败。
function getBlockTypeId(test: Test, x: number, y: number, z: number): string {
    const block = test.getBlock({ x, y, z }) as unknown as { typeId?: string } | undefined;
    return block?.typeId ?? "";
}

// 读取 water_cauldron level state（number 1-3，Java 口径 state 名 level）。返回 null 表示读取失败或非水炼药锅。
function getWaterCauldronLevel(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("level" as any);
    return typeof value === "number" ? value : null;
}

// 放支撑 + 空炼药锅：(3,1,1) stone 支撑，(3,2,1) 空炼药锅（minecraft:cauldron 默认 state）。
function placeCauldron(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:cauldron", { x: 3, y: 2, z: 1 }); // 空炼药锅
}

// 场景 1：水桶装水——空炼药锅 + 水桶 useItemOnBlock → water_cauldron level=3。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 空炼药锅。
// onBlockActivated 取手持水桶 → _handleBucketInteraction WATER_BUCKET → 替换为 water_cauldron
// defaultState.with(LEVEL_1_3, 3) → 返回 Success。
//
// 判定：getBlock typeId === "minecraft:water_cauldron" 且 level === 3。
function cauldronFillsWithWaterBucket(test: Test): void {
    placeCauldron(test);
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:cauldron", `cauldron should be empty before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const waterBucket = new ItemStack("minecraft:water_bucket", 1);

    // 对空炼药锅 useItemOnBlock 水桶 → onBlockActivated 水桶交互 → water_cauldron level=3。
    const used = farmer.useItemOnBlock(
        waterBucket as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when filling cauldron with water bucket");

    // 判定：方块类型变为 water_cauldron，level=3（满）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:water_cauldron", `cauldron should become water_cauldron, got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getWaterCauldronLevel(test, 3, 2, 1) === 3, `water_cauldron level should be 3 (full), got ${getWaterCauldronLevel(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：岩浆桶装岩浆——空炼药锅 + 岩浆桶 useItemOnBlock → lava_cauldron。
//
// 布局：同场景 1。onBlockActivated 取手持岩浆桶 → _handleBucketInteraction LAVA_BUCKET →
// 替换为 lava_cauldron defaultState → 返回 Success。
//
// 判定：getBlock typeId === "minecraft:lava_cauldron"。
function cauldronFillsWithLavaBucket(test: Test): void {
    placeCauldron(test);
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:cauldron", `cauldron should be empty before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const lavaBucket = new ItemStack("minecraft:lava_bucket", 1);

    const used = farmer.useItemOnBlock(
        lavaBucket as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when filling cauldron with lava bucket");

    // 判定：方块类型变为 lava_cauldron。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:lava_cauldron", `cauldron should become lava_cauldron, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 3：非容器不误触发——空炼药锅 + 木棍 useItemOnBlock → 仍 cauldron（桶/瓶交互都 Pass）。
//
// 布局：同场景 1。onBlockActivated 取手持木棍 → _handleBucketInteraction（木棍非桶 Pass）→
// _handleBottleInteraction（木棍非瓶 Pass）→ 整体 Pass → fallback Item.useOn（木棍无 onItemUse 行为，
// 默认返 Pass）→ useItemOnBlock 返回 false，炼药锅不变。
//
// 注意：不能用石头等 BlockItem 测「非容器不误触发」——BlockItem::onItemUse 会尝试在点击方块旁放置
// 该方块（face=Up → 炼药锅上方放石头），返回 Success，与「容器交互」无关。木棍是普通 Item（非
// BlockItem），onItemUse 默认 Pass，不触发放置，才能干净验证「非容器物品不触发炼药锅装填」。
//
// 判定：useItemOnBlock 返回 false（未触发装填），方块仍 cauldron。
function cauldronIgnoresNonFluidItem(test: Test): void {
    placeCauldron(test);
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:cauldron", `cauldron should be empty before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对空炼药锅 useItemOnBlock 木棍 → 桶/瓶交互都 Pass，木棍无 onItemUse → 不装填。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    // 木棍非流体容器，不应触发装填（useItemOnBlock 返回 false）。
    test.assert(!used, `useItemOnBlock should return false for non-fluid item (stick), got ${used}`);

    // 判定：炼药锅仍为空 cauldron（未装填）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:cauldron", `cauldron should remain empty cauldron for non-fluid item, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 4：玻璃瓶取水——water_cauldron level=3 + 玻璃瓶 useItemOnBlock → level 3→2。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) water_cauldron level=3（setBlockWithStates 直接放满水炼药锅）。
// water_cauldron 是 LayeredCauldronBlock（非 CauldronBlock 空炼药锅），其 onBlockActivated：
//   玻璃瓶 → _handleBottleInteraction → GLASS_BOTTLE 分支 → lowerFillLevel（level-1）+ 水瓶 → Success。
// lowerFillLevel：level>1 → with(LEVEL_1_3, level-1)（level 3→2）；level==1 → 替换为空 cauldron。
//
// 判定：useItemOnBlock 返 true（Success），level === 2（玻璃瓶取走一格水）。
// one-sided：setBlockWithStates 放满水炼药锅是 Cubium 专有 API（基岩 BDS 无），仅 Cubium 跑。
function cauldronDrainedByGlassBottle(test: Test): void {
    // setBlockWithStates 放满水炼药锅（water_cauldron level=3）。one-sided：Cubium 专有 API。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    (test as unknown as {
        setBlockWithStates: (type: string, pos: { x: number; y: number; z: number }, states: string) => boolean;
    }).setBlockWithStates("minecraft:water_cauldron", { x: 3, y: 2, z: 1 }, "level=3");
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:water_cauldron", `block should be water_cauldron before, got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getWaterCauldronLevel(test, 3, 2, 1) === 3, `water_cauldron level should be 3 before, got ${getWaterCauldronLevel(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const glassBottle = new ItemStack("minecraft:glass_bottle", 1);

    // 对满水炼药锅 useItemOnBlock 玻璃瓶 → LayeredCauldronBlock.onBlockActivated → _handleBottleInteraction
    // → GLASS_BOTTLE 分支 lowerFillLevel(level 3→2) + 水瓶 → Success。
    const used = farmer.useItemOnBlock(
        glassBottle as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when draining water_cauldron with glass bottle");

    // 判定：level === 2（玻璃瓶取走一格水，water_cauldron 仍存在，水位降为 2）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:water_cauldron", `block should remain water_cauldron after one drain, got ${getBlockTypeId(test, 3, 2, 1)}`);
    test.assert(getWaterCauldronLevel(test, 3, 2, 1) === 2, `water_cauldron level should be 2 after glass bottle drain, got ${getWaterCauldronLevel(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 5：空桶取岩浆——lava_cauldron + 空桶 useItemOnBlock → cauldron（空炼药锅）+ 岩浆桶。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) lava_cauldron（setBlockType 放默认 state，满岩浆，无需 level state）。
// 这是与场景 2（装岩浆走 CauldronBlock._handleBucketInteraction LAVA_BUCKET）不同的代码路径——
//   取岩浆走 LavaCauldronBlock::onBlockActivated BUCKET 分支（LavaCauldronBlock.cpp:111-142）：
//   空桶 → setBlockState cauldron（空炼药锅）+ ITEM_BUCKET_FILL_LAVA 音效 + FLUID_PICKUP 事件 +
//   非创造模式空桶变岩浆桶 → Success。
// 与场景 4（玻璃瓶取水，one-sided）互补：本场景两端均可放 lava_cauldron（setBlockType 默认 state，
//   无 setBlockWithStates），取岩浆行为两端可对比，非 one-sided。
//
// 判定：useItemOnBlock 返 true（Success），方块类型从 lava_cauldron → cauldron（空炼药锅）。
function cauldronDrainedToEmptyByBucket(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:lava_cauldron", { x: 3, y: 2, z: 1 }); // 满岩浆炼药锅（默认 state）
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:lava_cauldron", `block should be lava_cauldron before, got ${getBlockTypeId(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const bucket = new ItemStack("minecraft:bucket", 1);

    // 对满岩浆炼药锅 useItemOnBlock 空桶 → LavaCauldronBlock.onBlockActivated BUCKET 分支 →
    // setBlockState cauldron（空炼药锅）+ 岩浆桶 → Success。
    const used = farmer.useItemOnBlock(
        bucket as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when draining lava_cauldron with empty bucket");

    // 判定：方块类型从 lava_cauldron → cauldron（空炼药锅，岩浆被空桶取走）。
    test.assert(getBlockTypeId(test, 3, 2, 1) === "minecraft:cauldron", `block should become empty cauldron after lava drained, got ${getBlockTypeId(test, 3, 2, 1)}`);

    test.succeed();
}

// 岩浆炼药锅灼烧落入实体 GameTest：实体落入 lava_cauldron 内容区域触发 onEntityCollision，
// 受岩浆引燃（lavaIgnite 15 秒）+ 岩浆伤害（lavaHurt 4.0），血量下降。
//
// C++ 链路（独立于 LiquidBlock 岩浆流体的另一条岩浆伤害链路）：
//   Entity::doBlockCollisions（Entity.cpp:1338-1400）每 tick 遍历实体 AABB 覆盖方块格，对每格取
//   block.getEntityInsideCollisionShape（:1373）。LavaCauldronBlock 重写该方法返回 m_filledShape
//   （LavaCauldronBlock.cpp:165-172，外部外壁 ∪ 岩浆内容 lavaInside），非 fullBlock 走精确路径
//   insideShape.intersects(aabb, pos)（:1380）。实体落入内容区域（lavaInside box(2,4,2,14,15,14)
//   像素）AABB 与之相交 → isInsideBlock → onEntityCollision（:176-187）→ entity.lavaIgnite() +
//   entity.lavaHurt()。
//   - lavaIgnite（Entity.cpp:2156-2161）：!isImmuneToFire → igniteForSeconds(15.0f)（引燃 300 tick）。
//   - lavaHurt（Entity.cpp:2163-2178）：!isImmuneToFire → hurt(DamageSources::lava(), 4.0f)，受
//     LivingEntity 受击免疫节流（m_hurtResistantTime 前 10 tick 阻挡，第 11 tick 放行）。
//
// vanilla 对照（LavaCauldronBlock.java）：
//   getEntityInsideCollisionShape 返回 FILLED_SHAPE（=Shapes.or(SHAPE, SHAPE_INSIDE)）；
//   entityInside → CLEAR_FREEZE + LAVA_IGNITE + runAfter(LAVA_IGNITE, Entity::lavaHurt)。
//   Cubium 完全对齐（lavaIgnite + lavaHurt），唯一偏差：Cubium 未调 CLEAR_FREEZE（清除冰冻），
//   不影响岩浆伤害判定。
//
// 几何（fall_tower 7×16×7，中心 (3,*,3) 1×1 垂直玻璃管囚禁实体垂直下落）：
//   - (3,0,3) cobblestone：固体支撑（fall_tower y=0 中心格默认 rail 非固体，需铺 cobblestone 作
//     lava_cauldron 下方支撑，炼药锅 isValidPosition 需 belowState.isSolid）。
//   - (3,1,3) lava_cauldron：岩浆炼药锅（默认 state 满岩浆，setBlockType 直写）。
//     外壁 m_outerShape（base 0~3/16 + 四面墙 3/16~1.0）顶部中央 12×12 开口（x,z∈[2/16,14/16]），
//     内部 lavaInside box(2,4,2,14,15,14)（y∈[4/16,15/16] 像素）。
//   - 猪 spawn (3,11,3)：沿玻璃管垂直自由落体，经开口落入炼药锅内部空腔，停在 base 顶 y=1+3/16
//     =1.1875。猪 AABB y∈[1.1875, 2.0875] 与 lavaInside（世界 y∈[1.25, 1.9375]）相交
//     [1.25, 1.9375] → 每 tick onEntityCollision → lavaHurt(4.0)。
//
// 关键约束（实体能否落入开口）：
//   猪宽 0.9，AABB 水平 [3.05, 3.95]，炼药锅开口 [3.125, 3.875]（2/16~14/16）。猪 AABB 边缘
//   3.05 < 3.125 略超开口，但 fall_tower 1×1 玻璃管强制猪水平居中（管壁 glass 在 (2,*,3)/(4,*,3)
//   等），猪中心对准 (3.5,3.5)，下落动量使猪穿过开口落入内部空腔（外壁在猪 AABB 边缘的微小重叠
//   被下落动量克服，vanilla 实体落入炼药锅同理）。诊断阶段已实测猪落入并受伤。
//
// 受击免疫节流（关键时序）：lavaHurt 每 tick 调 hurt(lava, 4.0)，前 10 tick 被无敌帧阻挡，
// 第 11 tick 放行造成 4.0 伤害，猪 10→6。引燃（lavaIgnite 15 秒）使猪燃烧持续掉血（每 40 tick
// 1.0 火焰伤害），但首击 4.0 岩浆伤害远大于燃烧，tick 40 断言 hp<10 可靠区分（4.0 一击 hp=6）。
//
// 判定手段：runAtTickTime(40, ...) 在 40 tick 后检查猪 hp<10（满血 10，首次 4.0 岩浆伤害后
// 10→6）。用 runAtTickTime 而非 succeedWhen：succeedWhen 查 HP<10 会在岩浆伤害生效后立即通过，
// 但需确保猪已落入内容区域（落体约 20 tick + 无敌帧 10 tick + 伤害 1 tick ≈ 31 tick），tick 40
// 留足落入+首击余量。区域限定 fall_tower 7×16×7 排除并行测试污染。
//
// 选猪而非牛/羊：猪 MAX_HEALTH=10，4.0 岩浆伤害一击 hp=6<10 满足断言且不致死（便于观察 hp 下降）；
// 猪非火焰免疫（isImmuneToFire=false），lavaIgnite/lavaHurt 均生效。牛 MAX_HEALTH=10 同理可选，
// 但猪体积小更易落入开口。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_炼药锅.txt#装料（岩浆炼药锅：实体落入受
//      岩浆伤害并引燃，与岩浆流体同等伤害）
// Ref: LavaCauldronBlock.cpp:165-187（getEntityInsideCollisionShape 返 m_filledShape + onEntityCollision
//      lavaIgnite + lavaHurt）
// Ref: Entity.cpp:2156-2178（lavaIgnite 15 秒引燃 + lavaHurt 4.0 伤害）
// Ref: LavaCauldronBlock.java（vanilla entityInside：LAVA_IGNITE + lavaHurt，FILLED_SHAPE）
function lavaCauldronBurnsEntityInside(test: Test): void {
    const pigType = "pig";
    // fall_tower 7×16×7 区域限定查询常量（排除批内并行 tick 跨测试污染）。
    const TOWER_FROM = { x: 0, y: 0, z: 0 };
    const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

    // (3,0,3) 放 cobblestone 作 lava_cauldron 下方固体支撑（fall_tower y=0 中心格默认 rail 非固体）。
    test.setBlockType("minecraft:cobblestone", { x: 3, y: 0, z: 3 });

    // (3,1,3) 放岩浆炼药锅（默认 state 满岩浆，setBlockType 直写，无需 setBlockWithStates）。
    test.setBlockType("minecraft:lava_cauldron", { x: 3, y: 1, z: 3 });

    // 猪 spawn 于 (3,11,3)，沿 fall_tower 1×1 玻璃管垂直自由落体，落入炼药锅内部空腔。
    test.spawn(pigType, { x: 3, y: 11, z: 3 });

    // 40 tick 后检查：猪存在且 hp<10（满血 10，首次 4.0 岩浆伤害后 10→6）。
    // 时序：落体约 20 tick + 无敌帧 10 tick + 首击 1 tick ≈ 31 tick，tick 40 留足余量。
    test.runAtTickTime(40, () => {
        const pigs = test.getDimension().getEntities({
            type: pigType,
            location: test.worldLocation(TOWER_FROM),
            volume: TOWER_VOLUME,
        });
        test.assert(pigs.length > 0, "pig disappeared before taking lava cauldron damage");
        const health = pigs[0].getComponent("minecraft:health");
        test.assert((health as any).currentValue < 10,
            `pig did not take lava cauldron damage (expected hp<10, got hp=${(health as any).currentValue};`
            + ` hp=10 means pig did not enter lava cauldron content region)`);
        test.succeed();
    });
}

export function registerCauldronTests(): void {
    GameTest.register("BlockBehaviorTests", "cauldron_fills_with_water_bucket", cauldronFillsWithWaterBucket)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "cauldron_fills_with_lava_bucket", cauldronFillsWithLavaBucket)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "cauldron_ignores_non_fluid_item", cauldronIgnoresNonFluidItem)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "cauldron_drained_by_glass_bottle", cauldronDrainedByGlassBottle)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "cauldron_drained_to_empty_by_bucket", cauldronDrainedToEmptyByBucket)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "lava_cauldron_burns_entity_inside", lavaCauldronBurnsEntityInside)
        .structureName("gametests:fall_tower")
        .maxTicks(200);
}
