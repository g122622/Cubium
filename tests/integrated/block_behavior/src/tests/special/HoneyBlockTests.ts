// 蜂蜜块行为 GameTest（摔落减伤）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。
// 中心 (3,*,3) 为 1×1 垂直玻璃管落管：y=0 满铺 cobblestone 底（中心格被测试覆盖为被测方块），
// y=1..14 中心柱 air（下落通道），四周管壁 glass（y=1..15），y=15 顶部 (3,15,3) glass 封顶。
// 专门为大落差摔落测试设计（现有结构最高仅 9 格，不足以让蜜块 0.2 倍率减伤后伤害可测量）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

// 蜂蜜块摔落减伤 GameTest：猪从高处落到蜂蜜块上承受减伤后的少量伤害（非完全免疫）。
//
// C++ 链路：HoneyBlock::onFallenUpon（HoneyBlock.cpp）。Entity::updateFallDistance
// （Entity.cpp:1044-1056）着地且 m_fallDistance>0 时调 _handleLandingOnBlock（:1090-1108）→
// Block::onFallenUpon → HoneyBlock 重写以 damageMultiplier=0.2 调 entity.causeFallDamage
// （对齐 Java HoneyBlock#fallOn 传 0.2F）。LivingEntity::causeFallDamage（LivingEntity.cpp:1206-1246）
// 计算 (effectiveDistance - 3.0f) * 0.2，即摔落伤害减为 20%（减伤 80%）。
// onLanded（HoneyBlock.cpp）仅 Y 速度归零（不弹跳），不重置 fallDistance——由 onFallenUpon 处理减伤。
//
// 落差设计：蜜块放 (3,0,3)（替换 cobble 底中心），猪 spawn (3,11,3)，落差 = 11 - 1 = 10 格
// （猪脚 y=11.0 落到蜜块顶面 y=1.0）。fallDistance≈10。蜜块伤害 (10-3)*0.2=1.4，猪 10→8.6。
// 对比普通方块伤害 (10-3)*1=7（猪 10→3），蜜块减伤 80% 后伤害仅 1.4（见对照测试）。
// 落差 10 > 3 确保有伤害，且蜜块 1.4 > 0 证明非完全免疫（区别于粘液块 0.0）。
//
// 囚笼：fall_tower 中心 1×1 玻璃管（结构自带管壁 y=1..15）已围住 (3,*,3) 落管，猪 spawn 后
// 只能垂直下落，无 AI 乱跑。封顶 y=15 防弹出。蜜块不弹跳（onLanded Y 归零），落地后稳定站立。
//
// 判定手段：succeedWhen 每 tick 检查猪 health.currentValue > 8（受约 1.4 减伤，hp≈8.6 > 8）。
// 普通方块同落差伤害 7（hp=3 < 8），故 hp>8 证明蜜块减伤生效（非落差不足假阳性）。
// 落地是确定性时序（重力 + AABB，零随机），非 flaky。maxTicks=200 留足自由落体 10 格时间。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_蜂蜜块.txt#掉落（减伤 80%）
function honeyBlockReducesFallDamage(test: Test): void {
  const pigType = "pig";

  // (3,0,3) 放蜂蜜块（替换 cobble 底中心）。方块 ID 为 minecraft:honey_block。
  test.setBlockType("minecraft:honey_block", { x: 3, y: 0, z: 3 });

  // 猪 spawn 于 (3,11,3)（蜜块正上方 10 格），自由落体到蜜块顶面，落差 10 格。
  // fall_tower 中心玻璃管已围住落管，猪只能垂直下落。
  test.spawn(pigType, { x: 3, y: 11, z: 3 });

  // 断言蜜块减伤：succeedWhen 每 tick 检查 health.currentValue > 8。
  // 蜜块伤害 (10-3)*0.2=1.4，hp≈8.6 > 8。普通方块伤害 7（hp=3），故 hp>8 证明减伤。
  // 区域限定用 fall_tower 7×16×7 排除并行测试污染。
  test.succeedWhen(() => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(TOWER_FROM),
      volume: TOWER_VOLUME,
    });
    test.assert(pigs.length > 0, "pig disappeared before fall damage check");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue > 8,
      `honey block did not reduce fall damage enough, hp=${(health as any).currentValue}`);
  });
}

// 对照测试：猪从同样高度落到石头（普通方块）承受完整摔落伤害（血量大降）。
// 验证落差 10 格确会造成重伤，使蜜块减伤判定有意义（非"落差不足本就不受伤"的假阳性）。
// C++ 链路：Block::onFallenUpon 默认实现（Block.cpp:289-302）以 multiplier=1.0 调
// causeFallDamage，伤害 (10-3)*1=7，猪 10→3。
function stoneBlockDealsHeavyFallDamage(test: Test): void {
  const pigType = "pig";

  // (3,0,3) 放石头（普通方块，走 Block::onFallenUpon 默认 multiplier=1.0 完整摔落伤害）。
  test.setBlockType("minecraft:stone", { x: 3, y: 0, z: 3 });

  // 猪 spawn 于 (3,11,3)，自由落体到石头顶面，落差 10 格，承受 (10-3)*1=7 摔落伤害。
  test.spawn(pigType, { x: 3, y: 11, z: 3 });

  // 断言猪承受重伤：succeedWhen 每 tick 检查 health.currentValue < 8。
  // 石头伤害 7，hp=3 < 8。蜜块减伤后 hp≈8.6 > 8，故 hp<8 证明落差足以重伤，蜜块减伤有意义。
  test.succeedWhen(() => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(TOWER_FROM),
      volume: TOWER_VOLUME,
    });
    test.assert(pigs.length > 0, "pig disappeared before fall damage check");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue < 8,
      `pig did not take heavy fall damage on stone, hp=${(health as any).currentValue}`);
  });
}

export function registerHoneyBlockTests(): void {
  GameTest.register("BlockBehaviorTests", "honey_block_reduces_fall_damage", honeyBlockReducesFallDamage)
    .structureName("gametests:fall_tower")
    .maxTicks(200);
  GameTest.register("BlockBehaviorTests", "stone_block_deals_heavy_fall_damage", stoneBlockDealsHeavyFallDamage)
    .structureName("gametests:fall_tower")
    .maxTicks(200);
}
