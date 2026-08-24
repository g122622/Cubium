// 细雪可行走行为 GameTest（POWDER_SNOW_WALKABLE_MOBS 标签 + 皮革靴子判定）。
//
// 验证 Cubium PowderSnowBlock::getCollisionShapeForEntity 对齐 vanilla 1.21.11
// PowderSnowBlock.getCollisionShape（PowderSnowBlock.java:116-132）的三分支碰撞形状判定，
// 使标签成员（rabbit/endermite/silverfish/fox）与穿皮革靴子的生物在细雪上行走不下沉，
// 非标签光脚生物下沉陷入细雪。
//
// vanilla 架构（PowderSnowBlock.java:116-132，权威源）：
//   getCollisionShape(state, level, pos, CollisionContext)：
//     if (!isPlacement() && ctx instanceof EntityCollisionContext) {
//       Entity entity = ctx.getEntity();
//       if (entity != null) {
//         if (entity.fallDistance > 2.5) return FALLING_COLLISION_SHAPE;       // :120-122 半穿透 0.9 高
//         boolean flag = entity instanceof FallingBlockEntity;
//         if (flag || canEntityWalkOnPowderSnow(entity)                         // :124-127
//             && ctx.isAbove(Shapes.block(), pos, false) && !ctx.isDescending()) {
//           return super.getCollisionShape(...);                                // 完整方块碰撞箱（可行走）
//         }
//       }
//     }
//     return Shapes.empty();                                                    // :131 下沉陷入
//
//   canEntityWalkOnPowderSnow（:139-145）：
//     entity.getType().is(POWDER_SNOW_WALKABLE_MOBS)  → true（rabbit/endermite/silverfish/fox）
//     || (LivingEntity && getItemBySlot(FEET).is(LEATHER_BOOTS))  → true（穿皮革靴子）
//
//   CollisionContext.isDescending() = Entity.isShiftKeyDown()（Entity.java:2590-2592），
//   即玩家潜行态（Cubium Entity::isSneaking()）。非潜行的可行走实体得完整碰撞箱停在细雪顶；
//   穿皮革靴玩家潜行时主动陷入细雪（wiki 第71行）。
//
// Cubium 实现（任务 #226/#287/#288）：
//   - 此前 Block::getCollisionShape(const BlockState&) 签名无实体上下文，细雪碰撞形状无法依实体
//     区分，导致 POWDER_SNOW_WALKABLE_MOBS 标签"已注册但运行时无查询消费方"（系统性缺陷模式），
//     所有实体一律穿过细雪下沉。
//   - 修复：新增 Block::getCollisionShapeForEntity(state, EntityCollisionContext, blockY) 虚方法
//     （基类默认委托 getCollisionShape），物理引擎碰撞收集路径（PhysicsEngine::_getBlockCollisionBoxes
//     / isOnGround / collectCollisionBoxes）经 EntityCollisionContext 透传实体指针、AABB、潜行态
//     到方块碰撞形状判定。仅 PowderSnowBlock 重写按实体判定，其余方块不受影响。
//   - PowderSnowBlock::getCollisionShapeForEntity 实现上述三分支；canEntityWalkOnPowderSnow 查
//     POWDER_SNOW_WALKABLE_MOBS 标签（EntityTypeTags::isInitialized() 安全检查）+ 皮革靴子判定。
//   - descending 从 Entity::isSneaking() 读取（非 movement.y<0，否则可行走实体下落到细雪顶时
//     会因下落中 Y 速度<0 误判 descending 而穿过细雪下沉）。
//
// 正反交叉验证（4 测试，防假通过）：
//   - powder_snow_walkable_fox_does_not_sink：狐狸（POWDER_SNOW_WALKABLE_MOBS 标签成员）→ 停细雪顶不下沉。
//   - powder_snow_walkable_rabbit_does_not_sink：兔子（标签成员）→ 停细雪顶不下沉。
//   - powder_snow_non_walkable_cow_sinks：牛（非标签、光脚）→ 下沉穿过细雪落到下方支撑面。
//   - powder_snow_leather_boots_walkable: 牛穿皮革靴子 → 停细雪顶不下沉（皮革靴子分支）。
//   正例（狐狸/兔子/皮革靴牛）停在细雪顶 dy=-1.000，负例（光脚牛）落到 cobblestone 顶 dy=-2.000，
//   Y 差 1.0 判定清晰。狐狸/兔子为标签正例，皮革靴牛为装备分支正例，光脚牛为负例，三者交叉
//   排除"所有实体都不下沉"（标签/装备分支失效）或"所有实体都下沉"（碰撞形状未生效）假通过。
//
// 几何设计（fall_tower 结构 7×16×7，中心 (3,*,3) 1×1 玻璃管落管，相对坐标）：
//   - (3,0,3) cobblestone：地板支撑（fall_tower y=0 中心格默认 rail 非固体，换成 cobblestone 既
//     防细雪下方虚空，又作下沉实体的落点支撑面）。
//   - (3,1,3) powder_snow：细雪方块（相对 blockY=1）。下方 (3,0,3) cobblestone 支撑。
//   - 实体 spawn (3,3,3)：脚相对 y=3，自由落体下落 1 格到细雪顶（相对 y=2），fallDistance≈1.0 ≤2.5
//     （不触发 FALLING_COLLISION_SHAPE 半穿透分支，进入可行走/下沉判定）。
//   - 可行走实体（狐狸/兔子/皮革靴牛）：细雪返回完整碰撞箱，实体停在细雪顶（相对 y≈2，dy=spawnY-1）。
//   - 下沉实体（光脚牛）：细雪返回 empty，实体穿过细雪继续下落，落到 (3,0,3) cobblestone 顶（相对 y≈1，dy=spawnY-2）。
//   - 中心 1×1 玻璃管（结构自带管壁 y=1..15）围住 (3,*,3) 落管，实体只能垂直下落不偏移。
//
// 判定手段：pollUntilSucceed 轮询区域内目标实体的 location.y，以 spawn 世界 Y（mob.location.y）为
// 锚点用相对偏移判定——GameTest 结构放置基准世界 Y 由框架按批次动态分配（fall_tower 实测世界 Y≈-59，
// 每次跑会变），硬编码绝对 Y 阈值会失效。以 spawnY 为锚点：细雪顶=spawnY-1，cobblestone 顶=spawnY-2。
//   - 可行走→y >= spawnY - WALKABLE_OFFSET(1.2)（实测 dy=-1.000 精确停在细雪顶）
//   - 下沉→y <= spawnY - SINK_OFFSET(1.7)（实测 dy=-2.000 精确落到 cobblestone 顶）
//   - 死区 (-1.7, -1.2) 清晰区分两种行为，无边界抖动误判。
// 区域限定 fall_tower 7×16×7 排除并行测试污染。下落是确定性时序（重力+AABB，零随机），非 flaky。
//
// className 恒为 BlockBehaviorTests（对齐 block_behavior 包约定）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_细雪.txt#影响实体移动（第64-67行：下落>2.5
//      实体 0.9 格碰撞箱；下落≤2.5 的兔子/末影螨/蠹虫/狐狸/皮革靴生物 1 格碰撞箱不下沉；其他实体无碰撞箱下沉）
// Ref: PowderSnowBlock.java:116-132（getCollisionShape 三分支）
// Ref: PowderSnowBlock.java:139-145（canEntityWalkOnPowderSnow 标签+皮革靴）
// Ref: Entity.java:2590-2592（isDescending = isShiftKeyDown，即潜行态）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。中心 (3,*,3) 为 1×1
// 垂直玻璃管落管。用于 getEntities 区域限定查询（必须区域限定——批内并行 tick+不清场，全维度
// getEntities({type}) 跨测试污染，同 PointedDripstoneTests 范式）。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

// 细雪方块放置位置（相对坐标 blockY=1）。下方 (3,0,3) cobblestone 支撑。
const COBBLE_POS = { x: 3, y: 0, z: 3 };
const POWDER_SNOW_POS = { x: 3, y: 1, z: 3 };
// 实体 spawn 位置（相对 y=3，细雪顶上方 1 格，下落 1 格 ≤2.5）。
const SPAWN_POS = { x: 3, y: 3, z: 3 };

// Y 判定阈值用相对 spawn 位置的偏移表达，不硬编码绝对世界 Y——GameTest 结构放置基准世界 Y
// 由框架按批次分配（fall_tower 实测世界 Y≈-60），绝对阈值会失效。以 spawn 世界 Y 为锚点：
//   - 细雪顶世界 Y = spawnY - 1（spawn 在细雪上方 1 格）
//   - cobblestone 顶世界 Y = spawnY - 2（cobblestone 在细雪下方 1 格）
// 可行走实体停在细雪顶 → entityY ≈ spawnY - 1；下沉实体落到 cobblestone 顶 → entityY ≈ spawnY - 2。
// 留 0.3 死区避免边界抖动：可行走 y >= spawnY - 1.2，下沉 y <= spawnY - 1.7。
const WALKABLE_OFFSET = 1.2;   // entityY >= spawnY - WALKABLE_OFFSET 视为停在细雪顶
const SINK_OFFSET = 1.7;       // entityY <= spawnY - SINK_OFFSET 视为落到 cobblestone 顶

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 FrostWalkerTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 读取 fall_tower 区域内指定类型实体的 Y 坐标（取第一个）。区域限定排除并行测试污染。
// 返回 null 表示区域内暂无该类型实体（spawn 未完成或已消失）。
function readEntityY(test: Test, entityType: string): number | null {
    const entities = test.getDimension().getEntities({
        type: entityType,
        location: test.worldLocation(TOWER_FROM),
        volume: TOWER_VOLUME,
    });
    if (entities.length === 0) {
        return null;
    }
    return (entities[0] as any).location.y as number;
}

// 通用细雪可行走测试骨架：放置细雪 + cobblestone 支撑，spawn 实体，轮询断言其 Y 落在预期区间。
//   - entityType: 实体类型 typeId
//   - equipBoots: 是否给实体穿皮革靴子（null=不穿，true=穿皮革靴）
//   - expectWalkable: true=预期停在细雪顶（entityY >= spawnY - WALKABLE_OFFSET），
//                     false=预期下沉（entityY <= spawnY - SINK_OFFSET）
//   - testName: 失败诊断用的测试名
function powderSnowWalkableTest(
    test: Test,
    entityType: string,
    equipLeatherBoots: boolean,
    expectWalkable: boolean,
    testName: string,
): void {
    // (3,0,3) cobblestone 地板支撑（fall_tower y=0 中心格默认 rail 非固体，换 cobblestone 防虚空 +
    // 作下沉实体落点）。(3,1,3) 细雪方块。
    test.setBlockType("minecraft:cobblestone", COBBLE_POS);
    test.setBlockType("minecraft:powder_snow", POWDER_SNOW_POS);

    // spawn 实体于细雪上方 1 格（脚相对 y=3，下落 1 格到细雪顶，fallDistance≈1.0 ≤2.5）。
    // mob 断言为 any：getComponent 返回 EquippableComponent|undefined 严格类型 + "Feet" 字符串需
    // EquipmentSlot 类型，用 any 绕过（同 FrostWalkerTests/ArmorDamageReductionTests 范式）。
    const mob = test.spawn(entityType, SPAWN_POS) as any;

    // spawn 世界 Y 锚点：GameTest 结构放置基准世界 Y 由框架按批次分配（fall_tower 实测世界 Y≈-60），
    // 不能硬编码绝对阈值。spawnY 是实体脚部初始世界 Y，细雪顶 = spawnY - 1，cobblestone 顶 = spawnY - 2。
    const spawnY = (mob.location.y as number);

    // 若需穿皮革靴子，spawn 同步返回后立即穿戴（落地前已穿好），使 canEntityWalkOnPowderSnow
    // 的皮革靴分支生效。mob 有 equippable 组件（Cubium 善意扩展，同 FrostWalkerTests 范式）。
    if (equipLeatherBoots) {
        const boots = makeItem("minecraft:leather_boots");
        mob.getComponent("minecraft:equippable").setEquipment("Feet", boots);
    }

    // 轮询断言实体 Y 落在预期区间。下落是确定性时序，但留足 tick 等落地稳定。
    // 实测：可行走实体 dy=-1.000（精确停在细雪顶），下沉实体 dy=-2.000（精确落到 cobblestone 顶），
    // 死区 (SINK_OFFSET, WALKABLE_OFFSET)=(-1.7,-1.2) 清晰区分两种行为，无边界抖动。
    pollUntilSucceed(test, () => {
        const y = readEntityY(test, entityType);
        if (y === null) {
            return false;
        }
        if (expectWalkable) {
            return y >= spawnY - WALKABLE_OFFSET;
        }
        return y <= spawnY - SINK_OFFSET;
    }, {
        startTick: 10,
        interval: 4,
        maxTick: 120,
        onTimeout: () => {
            const y = readEntityY(test, entityType);
            const walkableThr = spawnY - WALKABLE_OFFSET;
            const sinkThr = spawnY - SINK_OFFSET;
            test.assert(false,
                `${testName}: expected ${expectWalkable ? `walkable (y>=${walkableThr.toFixed(2)})`
                    : `sinking (y<=${sinkThr.toFixed(2)})`}`
                + ` but entityY=${y} (spawnY=${spawnY.toFixed(2)}, powder_snow top=${(spawnY - 1).toFixed(2)},`
                + ` cobblestone top=${(spawnY - 2).toFixed(2)})`);
        },
    });
}

// 狐狸（POWDER_SNOW_WALKABLE_MOBS 标签成员）在细雪上行走不下沉，停在细雪顶。
function powderSnowWalkableFoxDoesNotSink(test: Test): void {
    powderSnowWalkableTest(test, "fox", false, true, "powder_snow_walkable_fox_does_not_sink");
}

// 兔子（POWDER_SNOW_WALKABLE_MOBS 标签成员）在细雪上行走不下沉，停在细雪顶。
function powderSnowWalkableRabbitDoesNotSink(test: Test): void {
    powderSnowWalkableTest(test, "rabbit", false, true, "powder_snow_walkable_rabbit_does_not_sink");
}

// 牛（非标签、光脚）在细雪上下沉，穿过细雪落到下方 cobblestone 支撑面。
function powderSnowNonWalkableCowSinks(test: Test): void {
    powderSnowWalkableTest(test, "cow", false, false, "powder_snow_non_walkable_cow_sinks");
}

// 牛穿皮革靴子在细雪上行走不下沉（canEntityWalkOnPowderSnow 皮革靴分支），停在细雪顶。
function powderSnowLeatherBootsWalkable(test: Test): void {
    powderSnowWalkableTest(test, "cow", true, true, "powder_snow_leather_boots_walkable");
}

export function registerPowderSnowWalkableTests(): void {
    GameTest.register("BlockBehaviorTests", "powder_snow_walkable_fox_does_not_sink", powderSnowWalkableFoxDoesNotSink)
        .structureName("gametests:fall_tower")
        .maxTicks(160);
    GameTest.register(
        "BlockBehaviorTests",
        "powder_snow_walkable_rabbit_does_not_sink",
        powderSnowWalkableRabbitDoesNotSink,
    )
        .structureName("gametests:fall_tower")
        .maxTicks(160);
    GameTest.register("BlockBehaviorTests", "powder_snow_non_walkable_cow_sinks", powderSnowNonWalkableCowSinks)
        .structureName("gametests:fall_tower")
        .maxTicks(160);
    GameTest.register("BlockBehaviorTests", "powder_snow_leather_boots_walkable", powderSnowLeatherBootsWalkable)
        .structureName("gametests:fall_tower")
        .maxTicks(160);
}
