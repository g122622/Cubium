// 干草块行为 GameTest（摔落减伤）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。
// 中心 (3,*,3) 为 1×1 垂直玻璃管落管：y=0 满铺 cobblestone 底（中心格被测试覆盖为被测方块），
// y=1..14 中心柱 air（下落通道），四周管壁 glass（y=1..15），y=15 顶部 (3,15,3) glass 封顶。
// 专门为大落差摔落测试设计。详见 HoneyBlockTests 同结构说明。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

// 干草块摔落减伤 GameTest：猪从高处落到干草块上承受减伤后的少量伤害（非完全免疫）。
//
// C++ 链路：HayBlock::onFallenUpon（HayBlock.cpp）。Entity::updateFallDistance
// （Entity.cpp:1044-1056）着地且 m_fallDistance>0 时调 _handleLandingOnBlock（:1090-1108）→
// Block::onFallenUpon → HayBlock 重写以 damageMultiplier=0.2 调 entity.causeFallDamage
// （对齐 Java HayBlock#fallOn 传 0.2F）。LivingEntity::causeFallDamage（LivingEntity.cpp:1206-1246）
// 计算 (effectiveDistance - 3.0f) * 0.2，即摔落伤害减为 20%（减伤 80%）。
// HayBlock 继承 RotatedPillarBlock 保留 axis 属性，onLanded 不重写（不弹跳，行为同普通方块）。
//
// 落差设计：干草块放 (3,0,3)（替换 cobble 底中心），猪 spawn (3,11,3)，落差 = 11 - 1 = 10 格
// （猪脚 y=11.0 落到干草块顶面 y=1.0）。fallDistance≈10。干草块伤害 (10-3)*0.2=1.4，猪 10→8.6。
// 干草块与蜜块减伤乘数相同（0.2），本测试验证干草块这个独立方块类也走了 0.2 减伤路径
// （确认 BuildingBlocks 注册分发到 HayBlock 而非默认 RotatedPillarBlock 的 1.0）。
//
// 囚笼：fall_tower 中心 1×1 玻璃管（结构自带管壁 y=1..15）已围住 (3,*,3) 落管，猪 spawn 后
// 只能垂直下落，无 AI 乱跑。封顶 y=15 防弹出。干草块不弹跳，落地后稳定站立。
//
// 判定手段：succeedWhen 每 tick 检查猪 health.currentValue > 8（受约 1.4 减伤，hp≈8.6 > 8）。
// 普通方块同落差伤害 7（hp=3 < 8），由同批 HoneyBlockTests.stone_block_deals_heavy_fall_damage
// 对照证明落差足以重伤，故 hp>8 证明干草块减伤生效（非落差不足假阳性）。
// 落地是确定性时序（重力 + AABB，零随机），非 flaky。maxTicks=200 留足自由落体 10 格时间。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_干草块.txt#掉落（减伤 80%）
function hayBlockReducesFallDamage(test: Test): void {
  const pigType = "pig";

  // (3,0,3) 放干草块（替换 cobble 底中心）。方块 ID 为 minecraft:hay_block。
  test.setBlockType("minecraft:hay_block", { x: 3, y: 0, z: 3 });

  // 猪 spawn 于 (3,11,3)（干草块正上方 10 格），自由落体到干草块顶面，落差 10 格。
  // fall_tower 中心玻璃管已围住落管，猪只能垂直下落。
  test.spawn(pigType, { x: 3, y: 11, z: 3 });

  // 断言干草块减伤：succeedWhen 每 tick 检查 health.currentValue > 8。
  // 干草块伤害 (10-3)*0.2=1.4，hp≈8.6 > 8。普通方块伤害 7（hp=3），故 hp>8 证明减伤。
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
      `hay block did not reduce fall damage enough, hp=${(health as any).currentValue}`);
  });
}

export function registerHayBlockTests(): void {
  GameTest.register("BlockBehaviorTests", "hay_block_reduces_fall_damage", hayBlockReducesFallDamage)
    .structureName("gametests:fall_tower")
    .maxTicks(200);
}
