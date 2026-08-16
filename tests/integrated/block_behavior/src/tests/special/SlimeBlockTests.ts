// 特殊类方块行为 GameTest（粘液块等）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// mediumglass 结构尺寸 12×9×11（helper 相对坐标 x∈[0,11], y∈[0,8], z∈[0,10]）。
// y=0 为 cobblestone 实心底，y=1..7 为玻璃墙围出的内部 air 空腔（x∈[2,10], z∈[2,10]），
// y=8 为玻璃顶框。用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const MED_FROM = { x: 0, y: 0, z: 0 };
const MED_VOLUME = { x: 12, y: 9, z: 11 };

// 粘液块摔落免疫 GameTest：猪从高处落到粘液块上不承受摔落伤害（血量保持满血）。
//
// C++ 链路：SlimeBlock::onFallenUpon（SlimeBlock.cpp）。Entity::updateFallDistance
// （Entity.cpp:1044-1056）着地且 m_fallDistance>0 时调 _handleLandingOnBlock（:1090-1108）→
// Block::onFallenUpon(world, pos, state, entity, fallDistance)。SlimeBlock 重写 onFallenUpon，
// 以 damageMultiplier=0.0 调 entity.causeFallDamage（对齐 Java SlimeBlock#fallOn 传 0.0F）。
// LivingEntity::causeFallDamage（LivingEntity.cpp:1206-1224）计算伤害
// (effectiveDistance - 3.0f) * damageMultiplier，multiplier=0 使伤害恒为 0，免疫摔落。
// 对比 Block::onFallenUpon 默认实现用 multiplier=1.0，会施加完整摔落伤害。
//
// 弹跳（onLanded，SlimeBlock.cpp）：落地后 onLanded 反弹 Y 速度（LivingEntity 系数 1.0），
// 猪弹起后反复弹跳衰减。首摔免疫后，每次再落地 fallDistance 递减（<3 不触发伤害）且
// onFallenUpon 仍 multiplier=0，全程不受伤。封顶防猪弹出管外落到 cobble 受伤。
//
// 落差设计：粘液块放 (6,1,6)（y=1 空腔底层，下方 y=0 cobble 实心支撑），猪 spawn (6,7,6)
// （y=7 空腔顶层），1×1 玻璃管围 (6,*,6) 垂直路径防 AI 乱跑。落差 = 7 - 2 = 5 格
// （猪脚 y=7.0 落到粘液块顶面 y=2.0），fallDistance≈5。普通方块伤害 (5-3)*1=2（猪 10→8），
// 粘液块免疫则保持 10。落差 5 > 3 确保普通方块确会受伤，使免疫判定有意义（见对照测试）。
//
// 判定手段：succeedWhen 每 tick 检查猪 health.currentValue == 10（满血）。
// 首摔落地约在 spawn 后 10+ tick（自由落体 5 格），弹跳衰减全程满血。
// maxTicks=200 留足弹跳衰减余量。落地是确定性时序（重力 + AABB，零随机），非 flaky。
// Ref: net.minecraft.world.level.block.SlimeBlock#fallOn（damageMultiplier=0.0F 免疫摔落伤害）
function slimeBlockPreventsFallDamage(test: Test): void {
  const pigType = "pig";

  // (6,1,6) 放粘液块（y=1 空腔底层，下方 y=0 cobble 实心支撑）。方块 ID 为 minecraft:slime_block。
  test.setBlockType("minecraft:slime_block", { x: 6, y: 1, z: 6 });

  // 1×1 玻璃管：围 (6,*,6) 垂直路径 y=2..7 四周 glass，限制猪只能垂直下落防 AI 乱跑。
  // 顶部 y=8 已是 mediumglass 玻璃顶，封顶防猪弹跳出管外。落差 5 格，弹跳系数 1.0 弹起约 5 格，
  // 从粘液块顶 y=2 弹到 y=7（管顶下方一格），不撞 y=8 顶。
  for (const y of [2, 3, 4, 5, 6, 7]) {
    test.setBlockType("minecraft:glass", { x: 5, y, z: 6 });
    test.setBlockType("minecraft:glass", { x: 7, y, z: 6 });
    test.setBlockType("minecraft:glass", { x: 6, y, z: 5 });
    test.setBlockType("minecraft:glass", { x: 6, y, z: 7 });
  }

  // 猪 spawn 于 (6,7,6)（粘液块正上方 5 格），自由落体到粘液块顶面。
  test.spawn(pigType, { x: 6, y: 7, z: 6 });

  // 断言猪免疫摔落伤害：succeedWhen 每 tick 检查 health.currentValue == 10（满血）。
  // 区域限定用 mediumglass 12×9×11 排除并行测试污染。
  test.succeedWhen(() => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(MED_FROM),
      volume: MED_VOLUME,
    });
    test.assert(pigs.length > 0, "pig disappeared before fall damage check");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue === 10,
      `pig took fall damage on slime block, hp=${(health as any).currentValue}`);
  });
}

// 对照测试：猪从同样高度落到石头（普通方块）承受摔落伤害（血量下降）。
// 验证落差 5 格确会造成伤害，使粘液块免疫判定有意义（非"落差不足本就不受伤"的假阳性）。
// C++ 链路：Block::onFallenUpon 默认实现（Block.cpp:289-302）以 multiplier=1.0 调
// causeFallDamage，伤害 (5-3)*1=2，猪 10→8。
function stoneBlockDealsFallDamage(test: Test): void {
  const pigType = "pig";

  // (6,1,6) 放石头（普通方块，走 Block::onFallenUpon 默认 multiplier=1.0 摔落伤害）。
  test.setBlockType("minecraft:stone", { x: 6, y: 1, z: 6 });

  // 同粘液块测试：1×1 玻璃管围 (6,*,6) y=2..7。
  for (const y of [2, 3, 4, 5, 6, 7]) {
    test.setBlockType("minecraft:glass", { x: 5, y, z: 6 });
    test.setBlockType("minecraft:glass", { x: 7, y, z: 6 });
    test.setBlockType("minecraft:glass", { x: 6, y, z: 5 });
    test.setBlockType("minecraft:glass", { x: 6, y, z: 7 });
  }

  // 猪 spawn 于 (6,7,6)，自由落体到石头顶面，落差 5 格，承受 (5-3)*1=2 摔落伤害。
  test.spawn(pigType, { x: 6, y: 7, z: 6 });

  // 断言猪承受摔落伤害：succeedWhen 每 tick 检查 health.currentValue < 10。
  // 时序：spawn 后约 10+ tick（自由落体 5 格）落地结算伤害，10→8 < 10 满足。
  test.succeedWhen(() => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(MED_FROM),
      volume: MED_VOLUME,
    });
    test.assert(pigs.length > 0, "pig disappeared before fall damage check");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue < 10,
      `pig did not take fall damage on stone, hp=${(health as any).currentValue}`);
  });
}

// 粘液块弹跳反弹 GameTest：猪从高处落到粘液块上后 Y 速度取反反弹，弹起高度接近原落差。
//
// C++ 链路：SlimeBlock::onLanded（SlimeBlock.cpp:48-74）。Entity::updateFallDistance
// （Entity.cpp）着地时调 _handleLandingOnBlock → Block::onLanded。SlimeBlock 重写 onLanded：
// 非潜行且 vy<0 时，LivingEntity 系数 1.0，setVelocity(vx, -vy*1.0, vz) 反弹 Y 速度。
// 对比 Block::onLanded 默认实现仅 Y 速度归零（不反弹）。此前 slime_block 注册为 SimpleBlock
// 走基类 onLanded（不反弹，猪落地即停），SlimeBlock::onLanded 沦为死代码——本次修复
// （NaturalBlocks.cpp 改 registerBlock<blocks::SlimeBlock>）后弹跳启用。
// 参考: net.minecraft.world.level.block.SlimeBlock#updateEntityMovementAfterFallOn / bounceUp
// （LivingEntity 系数 1.0：vec3.y<0 时 setDeltaMovement(x, -y*1.0, z)）
//
// 落差设计：复用 mediumglass 玻璃管（与 slimeBlockPreventsFallDamage 同结构）。粘液块放
// (6,1,6)（y=1 空腔底层，下方 y=0 cobble 支撑），猪 spawn (6,7,6)，落差 5 格。自由落体 5 格
// 落地 vy≈-0.88（重力 0.08/tick，约 11 tick），反弹系数 1.0 → vy≈+0.88，弹起高度 vy²/(2g)≈4.8 格，
// 从粘液块顶（相对 y=2）弹到相对 y≈6.8（接近 spawn 点 y=7）。
//
// 判定手段：pollUntilSucceed 密集轮询（startTick=15 落地反弹后，interval=2 捕获弹起峰值，
// maxTick=100 留衰减余量）检查猪世界 y 曾 > spawn 世界 y - 1.5（即弹起到接近 spawn 高度）。
// SimpleBlock 不弹跳则猪落地停在粘液块顶（相对 y≈2-3，世界 y 远低于 spawn-1.5），永不满足→超时 FAIL。
// SlimeBlock 弹跳则首次反弹即满足→succeed。区域限定用 mediumglass 12×9×11 排除并行测试污染。
// 落地+反弹是确定性物理时序（重力 + AABB，零随机），非 flaky。
function slimeBlockBouncesEntityUpward(test: Test): void {
  const pigType = "pig";

  // (6,1,6) 放粘液块（y=1 空腔底层，下方 y=0 cobble 实心支撑）。
  test.setBlockType("minecraft:slime_block", { x: 6, y: 1, z: 6 });

  // 1×1 玻璃管：围 (6,*,6) 垂直路径 y=2..7 四周 glass，限制猪垂直弹跳防 AI 乱跑。
  for (const y of [2, 3, 4, 5, 6, 7]) {
    test.setBlockType("minecraft:glass", { x: 5, y, z: 6 });
    test.setBlockType("minecraft:glass", { x: 7, y, z: 6 });
    test.setBlockType("minecraft:glass", { x: 6, y, z: 5 });
    test.setBlockType("minecraft:glass", { x: 6, y, z: 7 });
  }

  // 猪 spawn 于 (6,7,6)（粘液块正上方 5 格），自由落体到粘液块顶面后反弹。
  test.spawn(pigType, { x: 6, y: 7, z: 6 });

  // spawn 世界 y（相对 y=7 转世界坐标），反弹阈值 = spawnY - 1.5（弹起到接近 spawn 高度即满足）。
  const spawnWorldY = test.worldLocation({ x: 6, y: 7, z: 6 }).y;
  const bounceThreshold = spawnWorldY - 1.5;

  // 密集轮询：落地反弹约在 spawn 后 11+ tick，startTick=15 留落地余量；interval=2 捕获弹起峰值
  // （反弹上升仅约 12 tick 即达峰值，interval 过大会错过峰值）；maxTick=100 留多次衰减弹跳余量。
  let maxY = -Infinity;
  pollUntilSucceed(
    test,
    () => {
      const pigs = test.getDimension().getEntities({
        type: pigType,
        location: test.worldLocation(MED_FROM),
        volume: MED_VOLUME,
      });
      if (pigs.length === 0) {
        return false;
      }
      const y = pigs[0].location.y;
      if (y > maxY) {
        maxY = y;
      }
      // 猪世界 y 超过反弹阈值即证明弹跳生效（SimpleBlock 不弹则猪停在粘液块顶 y≈落地点，远低于阈值）。
      return y > bounceThreshold;
    },
    {
      startTick: 15,
      interval: 2,
      maxTick: 100,
      onTimeout: () => {
        test.assert(false, `pig did not bounce upward on slime block (onLanded not rebounding), spawnY=${spawnWorldY}, threshold=${bounceThreshold}, maxYReached=${maxY}`);
      },
    },
  );
}

export function registerSlimeBlockTests(): void {
  GameTest.register("BlockBehaviorTests", "slime_block_prevents_fall_damage", slimeBlockPreventsFallDamage)
    .structureName("gametests:mediumglass")
    .maxTicks(200);
  GameTest.register("BlockBehaviorTests", "stone_block_deals_fall_damage", stoneBlockDealsFallDamage)
    .structureName("gametests:mediumglass")
    .maxTicks(200);
  GameTest.register("BlockBehaviorTests", "slime_block_bounces_entity_upward", slimeBlockBouncesEntityUpward)
    .structureName("gametests:mediumglass")
    .maxTicks(200);
}
