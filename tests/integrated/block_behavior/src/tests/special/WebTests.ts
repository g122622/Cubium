// 蜘蛛网方块行为 GameTest（穿过蜘蛛网下落重置 fallDistance 致不摔伤）。
//
// 验证 Cubium WebBlock 的 onEntityCollision → setMotionMultiplier（减速）+ Entity.moveWithCollision
// 内的 FALL_DAMAGE_RESETTING 射线（重置 fallDistance）链路对齐 vanilla 1.21.11：实体从高处穿过
// 蜘蛛网下落，fallDistance 被反复重置，落到下方实方块时不承受摔落伤害（wiki 蜘蛛网：落到蜘蛛网
// 上不会受到摔落伤害）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。
// 中心 (3,*,3) 为 1×1 垂直玻璃管落管：y=0 中心格默认非固体，y=1..14 中心柱 air（下落通道），
// 四周管壁 glass（y=1..15），y=15 顶部 glass 封顶。用于 getEntities 的区域限定查询。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

// 蜘蛛网重置摔落距离 GameTest：猪从高处落到蜘蛛网上（蜘蛛网下方有实方块承接），穿过蜘蛛网
// 期间 fallDistance 被 FALL_DAMAGE_RESETTING 射线反复清零，落到下方 cobblestone 时
// fallDistance≈0，不承受摔落伤害（hp 保持满血 10）。
//
// C++ 链路（对齐 vanilla WebBlock.entityInside → Entity.makeStuckInBlock + Entity.move 射线）：
//   1. WebBlock::onEntityCollision（WebBlock.cpp）调 entity.setMotionMultiplier(slowdown)，
//      slowdown=(0.25,0.05,0.25)，WEAVING 时 (0.5,0.25,0.5)。对齐 vanilla makeStuckInBlock
//      仅设 stuckSpeedMultiplier（vanilla makeStuckInBlock 第一行原有 resetFallDistance，但
//      vanilla 摔伤免疫并非由它实现——见下）。
//   2. 真正的摔伤免疫由 Entity::moveWithCollision 内 _checkFallDamageResettingBlocks 实现，
//      对齐 vanilla Entity.move:718-725 的 FALLDAMAGE_RESETTING ClipContext 射线：当本帧实际
//      位移长度平方 >=1.0 且 fallDistance!=0 时，沿移动方向射长度 min(位移,8) 的射线，命中
//      BlockTags::FALL_DAMAGE_RESETTING 标签方块（= #climbable + sweet_berry_bush + cobweb）
//      即 resetFallDistance。Cubium 用 IWorld::isBlockInLine（DDA 逐格遍历，能命中空碰撞形状
//      方块如蜘蛛网）替代 ClipContext 射线。
//
//   猪穿过蜘蛛网格时，每帧 moveWithCollision 的射线都命中蜘蛛网 → fallDistance 清零。蜘蛛网
//   Y 乘数 0.05 极度减速垂直下落，猪缓慢穿过停留多 tick，fallDistance 持续被重置。穿出蜘蛛网
//   落到 (3,0,3) cobblestone 时 fallDistance≈0，updateFallDistance 的 onGround&&fallDistance>0
//   条件不成立，不触发 _handleLandingOnBlock → 不施加摔落伤害。
//
// vanilla 对照（WebBlock.java:27-34 entityInside）：
//   Vec3 vec3 = new Vec3(0.25, 0.05F, 0.25);
//   if (entity instanceof LivingEntity livingentity && livingentity.hasEffect(MobEffects.WEAVING))
//       vec3 = new Vec3(0.5, 0.25, 0.5);
//   entity.makeStuckInBlock(state, vec3);
// vanilla 摔伤免疫（Entity.java:718-725 move 内）：
//   if (this.fallDistance != 0.0 && d0 >= 1.0) {
//       double d1 = Math.min(vec3.length(), 8.0);
//       Vec3 vec32 = this.position().add(vec3.normalize().scale(d1));
//       BlockHitResult r = this.level().clip(new ClipContext(this.position(), vec32,
//           ClipContext.Block.FALLDAMAGE_RESETTING, ClipContext.Fluid.WATER, this));
//       if (r.getType() != HitResult.Type.MISS) this.resetFallDistance();
//   }
// Cubium 完全对齐（setMotionMultiplier 减速 + _checkFallDamageResettingBlocks 射线重置）。
//
// 几何（fall_tower 中心 1×1 玻璃管囚禁实体垂直自由落体）：
//   - (3,0,3) cobblestone：承接猪的实方块（fall_tower y=0 中心格默认非固体，需铺 cobblestone
//     作猪落地实方块；蜘蛛网无碰撞，猪穿过它落到此 cobblestone 顶面）。
//   - (3,1,3) minecraft:web 蜘蛛网：猪下落穿过此格触发 onEntityCollision → setMotionMultiplier
//     （减速）+ moveWithCollision 射线命中重置 fallDistance。蜘蛛网 getCollisionShape=empty，
//     不阻挡猪下落。
//   - 猪 spawn (3,11,3)：沿玻璃管垂直自由落体，先穿过 (3,1,3) 蜘蛛网（fallDistance 被射线重置），
//     再落到 (3,0,3) cobblestone 顶面（脚 y=1.0），fallDistance≈0 不摔伤。
//
// 判定手段：runAtTickTime(60, ...) 在 60 tick 后检查猪 hp===10（满血，未摔伤）。落差约 10 格
// （y=11→y=1），若无 fallDistance 重置应承受 (10-3)*1=7 摔落伤害（hp=3）。hp===10 精确证明
// fallDistance 被重置致完全免疫。用 runAtTickTime 而非 succeedWhen+hp：succeedWhen 查 hp===10
// 在猪尚未落地时也满足（满血），需确保猪已落地（落体约 20 tick + 穿蜘蛛网减速延长 + 落地余量
// ≈ 50 tick），tick 60 留足余量。区域限定 fall_tower 7×16×7 排除并行测试污染。
//
// 选猪：猪 MAX_HEALTH=10，落差 10 格无重置则 7 伤害（hp=3）与 hp===10 区分明显。猪体积适中
// 落 fall_tower 1×1 玻璃管居中。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_蜘蛛网.txt#掉落（落到蜘蛛网上不受摔落伤害）
// Ref: WebBlock.cpp（onEntityCollision：setMotionMultiplier(slowdown)，WEAVING 分支）
// Ref: Entity.cpp _checkFallDamageResettingBlocks（FALLDAMAGE_RESETTING 射线对齐 Entity.move:718-725）
// Ref: WebBlock.java:27-34（vanilla entityInside：makeStuckInBlock）
// Ref: Entity.java:718-725（vanilla move：FALLDAMAGE_RESETTING ClipContext 射线 resetFallDistance）
function cobwebResetsFallDistanceNoDamage(test: Test): void {
  const pigType = "pig";

  // (3,0,3) 放 cobblestone 作猪落地实方块（fall_tower y=0 中心格默认非固体）。
  test.setBlockType("minecraft:cobblestone", { x: 3, y: 0, z: 3 });

  // (3,1,3) 放蜘蛛网（minecraft:cobweb，Cubium 用 Java 命名 cobweb 非基岩 web）。
  // 蜘蛛网 getCollisionShape=empty，猪可穿过触发 onEntityCollision → setMotionMultiplier
  // （减速 + resetFallDistance）+ moveWithCollision 射线命中重置 fallDistance。
  test.setBlockType("minecraft:cobweb", { x: 3, y: 1, z: 3 });

  // 猪 spawn 于 (3,11,3)，沿 fall_tower 1×1 玻璃管垂直自由落体，穿过 (3,1,3) 蜘蛛网
  // （fallDistance 被射线重置）后落到 (3,0,3) cobblestone 顶面。
  test.spawn(pigType, { x: 3, y: 11, z: 3 });

  // 60 tick 后检查：猪存在且 hp===10（满血，穿过蜘蛛网射线重置 fallDistance 致未摔伤）。
  // 时序：落体约 20 tick + 穿蜘蛛网减速延长 + 落地余量 ≈ 50 tick，tick 60 留足余量。
  // 若未重置 fallDistance，落差 10 格应承受 (10-3)*1=7 伤害（hp=3），hp===10 精确证明免疫。
  test.runAtTickTime(60, () => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(TOWER_FROM),
      volume: TOWER_VOLUME,
    });
    test.assert(pigs.length > 0, "pig disappeared before fall damage check");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue === 10,
      `cobweb did not reset fall distance (expected hp=10 no fall damage, got hp=${(health as any).currentValue};`
        + ` hp<10 means pig took fall damage through cobweb — fallDistance was not reset by FALL_DAMAGE_RESETTING ray)`);
    test.succeed();
  });
}

// 对照测试：猪从同样高度直接落到 cobblestone（无蜘蛛网）承受完整摔落伤害（血量大降）。
// 验证落差 10 格确会造成重伤，使蜘蛛网免疫判定有意义（非"落差不足本就不受伤"的假阳性）。
// C++ 链路：Block::onFallenUpon 默认实现（Block.cpp）以 multiplier=1.0 调 causeFallDamage，
// 伤害 (10-3)*1=7，猪 10→3。
function cobwebAbsentDealsFallDamage(test: Test): void {
  const pigType = "pig";

  // (3,0,3) 放 cobblestone（普通方块，走 Block::onFallenUpon 默认 multiplier=1.0 完整摔落伤害）。
  test.setBlockType("minecraft:cobblestone", { x: 3, y: 0, z: 3 });

  // (3,1,3) 保持 air（无蜘蛛网），猪直接落到 cobblestone 承受完整摔落伤害。
  test.setBlockType("minecraft:air", { x: 3, y: 1, z: 3 });

  // 猪 spawn 于 (3,11,3)，自由落体到 cobblestone 顶面，落差 10 格，承受 (10-3)*1=7 摔落伤害。
  test.spawn(pigType, { x: 3, y: 11, z: 3 });

  // 断言猪承受重伤：runAtTickTime(60) 检查 hp<8。石头伤害 7（hp=3 < 8）。
  // 蜘蛛网免疫 hp=10 > 8，故 hp<8 证明落差足以重伤，蜘蛛网免疫有意义（非落差不足假阳性）。
  test.runAtTickTime(60, () => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(TOWER_FROM),
      volume: TOWER_VOLUME,
    });
    test.assert(pigs.length > 0, "pig disappeared before fall damage check");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue < 8,
      `pig should take heavy fall damage without cobweb (expected hp<8, got hp=${(health as any).currentValue};`
        + ` hp>=8 means fall distance was insufficient — cobweb immunity test above would be a false positive)`);
    test.succeed();
  });
}

export function registerWebTests(): void {
  GameTest.register("BlockBehaviorTests", "cobweb_resets_fall_distance_no_damage", cobwebResetsFallDistanceNoDamage)
    .structureName("gametests:fall_tower")
    .maxTicks(200);
  GameTest.register("BlockBehaviorTests", "cobweb_absent_deals_fall_damage", cobwebAbsentDealsFallDamage)
    .structureName("gametests:fall_tower")
    .maxTicks(200);
}
