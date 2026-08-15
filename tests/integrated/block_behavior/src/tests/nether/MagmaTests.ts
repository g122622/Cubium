// 下界类方块行为 GameTest（岩浆块等）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底（满铺 49 glass），y=1..2 为玻璃墙围出的内部 air 空腔，y=3..4 air+顶部框架。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。

// 岩浆块伤害 GameTest：实体站在岩浆块上受烫脚火焰伤害（血量下降）。
//
// C++ 链路：MagmaBlock::onEntityWalk（MagmaBlock.cpp:83-110）。Entity::move 落地后行走处理
// （Entity.cpp:1447-1449）当 m_onGround && !isSteppingCarefully() 时每 tick 调 onEntityWalk。
// onEntityWalk 内：dynamic_cast LivingEntity（非生物不受）→ 冰霜行者靴子免疫（hasFrostWalker
// 检查 Feet 槽，对齐 wiki + CampfireBlock 一致）→ DamageSources::hotFloor()（火焰伤害，
// isFire()==true，bypassesInvulnerability()==false）→ LivingEntity::hurt(hotFloor, 1.0f)。
//
// 受击免疫节流（关键时序）：同营火——LivingEntity::hurt 首行 isInvulnerableTo（:778-794）当
// m_hurtResistantTime>0 且伤害源不绕过无敌帧时直接 return false。m_hurtResistantTime 每 tick
// 递减（:883-885）。onEntityWalk 每 tick 调 hurt，前 10 tick 被无敌帧阻挡，第 11 tick 放行造成
// 伤害并重置无敌帧。故实际约每 10 tick（半秒）承受一次 hp1，与 wiki"受击免疫减慢至每半秒一次"
// 一致。猪 MAX_HEALTH=10，首次伤害后 10→9。
//
// 囚笼（关键）：猪有 AI 会乱跑，必须将其困在岩浆块正上方，确保持续 m_onGround 站在岩浆块上
// 触发 onEntityWalk。岩浆块放 (3,1,3)（y=1 空腔层，下方 y=0 glass 实心支撑），猪 spawn (3,2,3)
// （y=2 空腔层，下落至岩浆块顶面 y=2.0 站稳）。囚笼为 2 格高（y=2,y=3 两层四周 glass），顶部
// 不在猪正上方 (3,3,3) 放玻璃（猪高 ~0.87，y=2 站立头顶 y=2.87 < y=3 air，不顶头挤压——挤压
// 会导致 m_onGround 不稳定，onEntityWalk 依赖 m_onGround 严格 true）。顶部依靠结构 y=4 顶框玻璃
// 防止猪跳出 2 格高囚笼。猪仅能在 (3,2,3) 1×1 内站岩浆块上。
// 注意：与营火测试（onEntityCollision，AABB 相交即触发，不依赖 m_onGround）不同，岩浆块用
// onEntityWalk（依赖 m_onGround），故囚笼绝不能挤压猪致其悬空。
function magmaDamagesEntityOnTop(test: Test): void {
  const pigType = "pig";

  // (3,1,3) 放岩浆块（y=1 空腔层，下方 y=0 glass 实心支撑）。岩浆块是完整方块，猪可站其顶面。
  // 方块 ID 为 minecraft:magma_block（对齐 Java/vanilla 注册表，非 magma）。
  test.setBlockType("minecraft:magma_block", { x: 3, y: 1, z: 3 });

  // 囚笼：y=2 和 y=3 两层四周 glass（2 格高墙），不在猪正上方 (3,3,3) 放玻璃防挤压。
  // 顶部依靠结构 y=4 顶框玻璃防猪跳出。
  for (const y of [2, 3]) {
    test.setBlockType("minecraft:glass", { x: 2, y, z: 3 });
    test.setBlockType("minecraft:glass", { x: 4, y, z: 3 });
    test.setBlockType("minecraft:glass", { x: 3, y, z: 2 });
    test.setBlockType("minecraft:glass", { x: 3, y, z: 4 });
  }

  // 猪 spawn 于 (3,2,3)（岩浆块正上方），下落至岩浆块顶面站稳。
  test.spawn(pigType, { x: 3, y: 2, z: 3 });

  // 断言猪受岩浆块烫脚伤害：succeedWhen 每 tick 检查 health.currentValue < 10。
  // 时序：spawn 落地后约 10 tick（半秒）首次 hurt 放行，10→9 < 10 满足。
  // 区域限定用 glass_pit 7×5×7 排除并行测试污染。
  test.succeedWhen(() => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation({ x: 0, y: 0, z: 0 }),
      volume: { x: 7, y: 5, z: 7 },
    });
    test.assert(pigs.length > 0, "pig disappeared before taking magma damage");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue < 10,
      `pig did not take magma damage, hp=${(health as any).currentValue}`);
  });
}

export function registerMagmaTests(): void {
  GameTest.register("BlockBehaviorTests", "magma_damages_entity_on_top", magmaDamagesEntityOnTop)
    .structureName("gametests:glass_pit")
    .maxTicks(200);
}
