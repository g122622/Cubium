// 细雪冰冻伤害效果 GameTest（powder_snow onEntityCollision FREEZE 管线端到端）。
//
// 验证 Cubium 细雪冰冻伤害管线对齐 vanilla 1.21.11：实体陷入细雪→ticksFrozen 累积至阈值
// （140 tick）→isFullyFrozen→每 FREEZE_HURT_FREQUENCY(40) tick 受 1.0 冰冻伤害→血量下降。
// 以及 LivingEntity::canFreeze 的全套皮革装备免疫门控（穿全套皮革 canFreeze=false 不冰冻）。
//
// vanilla 架构（权威源）：
//   PowderSnowBlock.entityInside（PowderSnowBlock.java:171-180）：
//     entity.setIsInPowderSnow(true);
//     if (entity.canFreeze()) {
//       entity.setTicksFrozen(Math.min(entity.getTicksRequiredToFreeze(), entity.getTicksFrozen() + 1));
//     }
//   LivingEntity.aiStep→tickFreeze（LivingEntity.java:2995-3005）：
//     if (!isInPowderSnow || !canFreeze()) { setTicksFrozen(max(0, ticksFrozen - 2)); }  // 解冻速度 2x
//     if (tickCount % 40 == 0 && isFullyFrozen() && canFreeze()) {
//       hurtServer(freeze, 1.0F);  // 每 40 tick 一次冰冻伤害
//     }
//   LivingEntity.hurtServer（LivingEntity.java:1178-1180）：冰冻伤害对 FREEZE_HURTS_EXTRA_TYPES
//   标签实体（blaze/magma_cube/strider）×5。
//   Entity.canFreeze（Entity.java）：FREEZE_IMMUNE_ENTITY_TYPES 标签实体不冰冻；
//   LivingEntity.canFreeze 重写：穿全套皮革装备（4 件 leather）不冰冻。
//
// Cubium 实现（任务 #290）：
//   - PowderSnowBlock::onEntityCollision（PowderSnowBlock.cpp:145-171）setIsInPowderSnow(true) +
//     canFreeze 时 ticksFrozen+1（上限 getTicksRequiredToFreeze=140）。
//   - LivingEntity::tickFreeze（LivingEntity.cpp:1531-1555）：!isInPowderSnow||!canFreeze 时 -2；
//     ticksExisted()%40==0 && isFullyFrozen && canFreeze → hurt(freeze, 1.0f)。
//   - LivingEntity::actuallyHurt（LivingEntity.cpp:355-358）：FREEZE_HURTS_EXTRA_TYPES ×5。
//   - Entity::canFreeze（Entity.cpp:2613-2622）：FREEZE_IMMUNE_ENTITY_TYPES 标签门控。
//   - LivingEntity::canFreeze：全套皮革装备门控。
//
// 测试几何（fall_tower 7×16×7，中心 (3,*,3) 1×1 玻璃管囚笼）：
//   - (3,0,3) cobblestone：支撑面（非可行走实体穿过细雪后落点，挡住下沉防掉虚空）。
//   - (3,1,3) powder_snow：细雪格。实体停在 cobblestone 顶（相对 y=1.0），AABB 占据 y=1.0..2.x，
//     与细雪格 y=1 重叠→doBlockCollisions 每 tick 触发 onEntityCollision→setIsInPowderSnow(true)
//     + ticksFrozen 累积。实体持续待在细雪格内（玻璃管囚禁防 AI 乱跑离开）。
//   - 实体 spawn (3,2,3)：下落 1 格穿过细雪（非可行走，细雪返回 empty）到 cobblestone 顶 y=1.0。
//     下落 1 格 fallDistance≈1.0 ≤2.5，不触发半穿透分支。
//
// 测试实体选用牛（cow）而非僵尸：牛 MAX_HEALTH=10，且牛不会在白天燃烧（僵尸在白天露天燃烧会
// 干扰冰冻伤害判定——掉血可能由燃烧而非冰冻，致假通过）。牛是被动生物不燃烧，掉血唯一来源是
// 细雪冰冻，判定无歧义。牛非 POWDER_SNOW_WALKABLE_MOBS，穿过细雪下沉到 cobblestone 顶停下，
// AABB 与细雪格重叠触发 onEntityCollision（同细雪可行走测试的 cow_sinks 负例几何）。
//
// 时序（确定性，零随机）：
//   - spawn 落地 ~5 tick；ticksFrozen 每 tick +1，140 tick 冻透（isFullyFrozen）；
//   - 每 40 tick（tickCount%40==0）一次 1.0 冰冻伤害，首次伤害约 tick 145-180（取决于落地 ticksExisted 对齐）；
//   - 牛 MAX_HEALTH=10，tick 400 时已受约 5-6 次冰冻伤害（血量 4-5 < 10）。
//   - maxTicks=500 留足余量（140 冻透 + 多次 40 tick 伤害周期 + 落地/对齐余量）。
//
// 正反交叉验证（3 测试，防假通过）：
//   - powder_snow_freeze_damages_cow：牛（光脚、非冰冻免疫）陷入细雪→血量 < 10（受冰冻伤害）。
//   - powder_snow_freeze_immune_leather_armor_cow：牛穿全套皮革→canFreeze=false→血量 = 10（免疫冰冻）。
//   - powder_snow_no_freeze_without_snow_cow：牛站在 cobblestone（无细雪）→血量 = 10（无冰冻环境）。
//   正例（光脚牛）掉血，两负例（皮革牛/无细雪牛）不掉血，交叉排除"所有实体都掉血"
//   （canFreeze 门控失效）或"所有实体都不掉血"（冰冻管线未通）假通过。
//
// 判定手段：runAtTickTime 固定 tick 断言（非 succeedWhen/pollUntilSucceed）。原因同 BlazeTests：
// succeedWhen"条件满足即通过"会在牛尚未冻透（血量仍 10）时提前通过漏判；负向测试（皮革/无细雪）
// 用 succeedWhen 会在冰冻管线万一延迟生效的窗口假通过。固定 tick=400 断言等完整窗口：
//   - 光脚牛：tick 400 已冻透 + 受多次冰冻伤害，血量 < 10。
//   - 皮革/无细雪牛：tick 400 血量仍 = 10（免疫/无冰冻）。
// 区域限定 fall_tower 7×16×7 排除并行测试污染。
//
// className 恒为 BlockBehaviorTests（对齐 block_behavior 包约定）。
//
// FREEZE_HURTS_EXTRA_TYPES 5 倍乘数（烈焰人/岩浆怪/炽足兽 ×5）端到端集成测试暂未覆盖：
// 烈焰人 hover AI 上下浮动会离开细雪格致 ticksFrozen 解冻（几何不确定性）；岩浆怪跳跃 AI 同理；
// 炽足兽在主世界颤抖受伤干扰判定。5 倍乘数的标签查询与 ×5 数值逻辑由单元测试覆盖：
// tests/common/entity/core/EntityFreezeTest.cpp（FREEZE_HURTS_EXTRA_TYPES 标签成员断言）+
// LivingEntity::actuallyHurt:355-358（amount *= 5.0f，对齐 vanilla LivingEntity.java:1178-1180）。
// TODO: 待物理引擎支持强制约束实体位置（如 NoAI 实体固定）后，补 5 倍乘数端到端测试。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_细雪.txt#影响实体（实体陷入细雪累积冰冻，
//      完全冰冻后每 2 秒受 1 点冰冻伤害；穿皮革装备免疫冰冻）
// Ref: PowderSnowBlock.java:171-180（entityInside：setIsInPowderSnow + ticksFrozen+1）
// Ref: LivingEntity.java:2995-3005（tickFreeze：解冻 -2 + 每 40 tick 1.0 伤害）
// Ref: LivingEntity.java:1178-1180（hurtServer：FREEZE_HURTS_EXTRA_TYPES ×5）
// Ref: Entity.java（canFreeze：FREEZE_IMMUNE_ENTITY_TYPES 标签门控）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。中心 (3,*,3) 为 1×1
// 垂直玻璃管落管（结构自带管壁 y=1..15），囚禁实体防 AI 乱跑离开细雪格。区域限定查询排除并行污染。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

// (3,0,3) cobblestone 支撑面 + (3,1,3) powder_snow 细雪格。实体 spawn (3,2,3) 下落穿过细雪
// 落 cobblestone 顶（相对 y=1.0），AABB 与细雪格 y=1 重叠持续触发 onEntityCollision。
const COBBLE_POS = { x: 3, y: 0, z: 3 };
const POWDER_SNOW_POS = { x: 3, y: 1, z: 3 };
const SPAWN_POS = { x: 3, y: 2, z: 3 };

// 断言 tick：等牛冻透（140 tick）+ 多次冰冻伤害周期（40 tick/次）+ 落地/对齐余量。
// tick 400 时光脚牛已受约 5-6 次冰冻伤害（血量 4-5），皮革/无细雪牛仍满血 10。
const ASSERT_TICK = 400;

// 测试实体：牛（cow）。选用牛而非僵尸：牛不燃烧（僵尸白天燃烧干扰冰冻判定），MAX_HEALTH=10，
// 非细雪可行走（穿过细雪下沉到 cobblestone 顶，AABB 与细雪格重叠触发 onEntityCollision）。
const COW_TYPE = "cow";

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 FrostWalkerTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 读取 fall_tower 区域内指定类型实体的当前血量（取第一个）。返回 -1 表示区域内无该实体（已死亡/消失）。
// 区域限定排除并行测试污染（批内并行 tick+不清场，全维度 getEntities 跨测试污染）。
function readEntityHp(test: Test, entityType: string): number {
    const entities = test.getDimension().getEntities({
        type: entityType,
        location: test.worldLocation(TOWER_FROM),
        volume: TOWER_VOLUME,
    });
    if (entities.length === 0) {
        return -1;
    }
    const health = (entities[0] as any).getComponent("minecraft:health");
    return (health as any).currentValue as number;
}

// 通用细雪冰冻测试骨架：放 cobblestone 支撑 + 细雪格（或无细雪对照），spawn 牛（可穿全套皮革），
// 固定 tick 断言血量预期。
//   - placeSnow: true=放细雪格（测试冰冻），false=不放（无细雪对照）
//   - equipLeather: true=穿全套皮革装备（canFreeze=false 免疫）
//   - expectDamaged: true=预期 tick 400 血量 < 10（受冰冻伤害），false=预期血量 = 10（免疫/无冰冻）
//   - testName: 失败诊断用
function powderSnowFreezeTest(
    test: Test,
    placeSnow: boolean,
    equipLeather: boolean,
    expectDamaged: boolean,
    testName: string,
): void {
    // (3,0,3) cobblestone 支撑面（挡住非可行走实体下沉防掉虚空，作落点）。
    test.setBlockType("minecraft:cobblestone", COBBLE_POS);
    // (3,1,3) 细雪格（placeSnow=false 时不放，留 air 作无细雪对照）。
    if (placeSnow) {
        test.setBlockType("minecraft:powder_snow", POWDER_SNOW_POS);
    }

    // spawn 牛于细雪上方 1 格（脚相对 y=2，下落 1 格穿过细雪落 cobblestone 顶 y=1.0）。
    // 牛非 POWDER_SNOW_WALKABLE_MOBS，细雪返回 empty，穿过细雪下沉到 cobblestone 顶停下，
    // AABB 占据 y=1.0..2.x 与细雪格 y=1 重叠，持续触发 onEntityCollision 累积 ticksFrozen。
    const cow = test.spawn(COW_TYPE, SPAWN_POS) as any;

    // 穿全套皮革装备（4 件 leather）：LivingEntity::canFreeze 查全套皮革返回 false，免疫冰冻。
    // 同 FrostWalkerTests setEquipment 范式，落地前已穿好使 canFreeze 门控从首 tick 生效。
    if (equipLeather) {
        const equippable = cow.getComponent("minecraft:equippable");
        equippable.setEquipment("Head", makeItem("minecraft:leather_helmet"));
        equippable.setEquipment("Chest", makeItem("minecraft:leather_chestplate"));
        equippable.setEquipment("Legs", makeItem("minecraft:leather_leggings"));
        equippable.setEquipment("Feet", makeItem("minecraft:leather_boots"));
    }

    // 固定 tick 断言（等完整窗口，防 succeedWhen 时序窗口假通过，同 BlazeTests 范式）。
    test.runAtTickTime(ASSERT_TICK, () => {
        const hp = readEntityHp(test, COW_TYPE);
        if (expectDamaged) {
            // 光脚牛陷入细雪：冻透后受冰冻伤害，血量 < 10。
            test.assert(hp > 0 && hp < 10,
                `${testName}: expected cow took freeze damage (hp<10) but hp=${hp}`);
        } else {
            // 皮革牛（canFreeze=false 免疫）或无细雪环境：血量 = 10（无冰冻）。
            test.assert(hp === 10,
                `${testName}: expected cow full health (hp=10, immune/no-snow) but hp=${hp}`);
        }
        test.succeed();
    });
}

// 牛（光脚、非冰冻免疫）陷入细雪，冻透后受冰冻伤害，血量 < 10。
function powderSnowFreezeDamagesCow(test: Test): void {
    powderSnowFreezeTest(test, true, false, true, "powder_snow_freeze_damages_cow");
}

// 牛穿全套皮革装备（canFreeze=false 免疫冰冻），陷入细雪血量保持 10（负向对照，验证皮革门控）。
function powderSnowFreezeImmuneLeatherArmorCow(test: Test): void {
    powderSnowFreezeTest(test, true, true, false, "powder_snow_freeze_immune_leather_armor_cow");
}

// 牛站在 cobblestone（无细雪环境），无冰冻源，血量保持 10（负向对照，排除非细雪因素掉血假通过）。
function powderSnowNoFreezeWithoutSnowCow(test: Test): void {
    powderSnowFreezeTest(test, false, false, false, "powder_snow_no_freeze_without_snow_cow");
}

export function registerPowderSnowFreezeTests(): void {
    GameTest.register("BlockBehaviorTests", "powder_snow_freeze_damages_cow", powderSnowFreezeDamagesCow)
        .structureName("gametests:fall_tower")
        .maxTicks(500);
    GameTest.register("BlockBehaviorTests", "powder_snow_freeze_immune_leather_armor_cow", powderSnowFreezeImmuneLeatherArmorCow)
        .structureName("gametests:fall_tower")
        .maxTicks(500);
    GameTest.register("BlockBehaviorTests", "powder_snow_no_freeze_without_snow_cow", powderSnowNoFreezeWithoutSnowCow)
        .structureName("gametests:fall_tower")
        .maxTicks(500);
}
