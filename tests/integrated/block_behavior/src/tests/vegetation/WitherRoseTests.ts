// 凋灵玫瑰行为 GameTest（碰撞施加凋零 + 亡灵免疫）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底（满铺 49 glass），y=1..2 为玻璃墙围出的内部 air 空腔，y=3..4 air+顶部框架。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 凋灵玫瑰施加凋零 GameTest：活体生物（猪）踩在凋灵玫瑰上持续获得凋零 I，HP 下降。
//
// C++ 链路：WitherRoseBlock::onEntityCollision（WitherRoseBlock.cpp）。Entity::checkInsideBlocks
// （Entity.cpp:1317-1343）每 tick 遍历实体 AABB 覆盖的方块网格，对相交方块调 onEntityCollision。
// 凋灵玫瑰 noCollision（花），猪穿过花落到下方 glass 顶面（脚 y=1.0），AABB y∈[1.0,1.9] 与
// 花方块网格 y=1（世界 y∈[1,2)）相交，每 tick 触发回调。
//
// onEntityCollision 内：isClientSide 守卫 → 和平难度守卫 → dynamic_cast LivingEntity（仅活体）
// → EntityTypeTags::UNDEAD().contains(typeId) 亡灵免疫（含凋灵骷髅/凋灵 boss）→ hasEffect(Wither)
// 去重 → addEffect(Wither I / 40tick)。对齐 Java WitherRoseBlock#entityInside 与 wiki
// "非和平难度下，凋灵玫瑰会向所有触碰到它的生物持续施加 2 秒的凋零效果"（凋零 I、0:02）。
//
// 凋零伤害时序：EffectInstance.cpp:280-289，interval = 40 >> amplifier = 40（amplifier=0），
// 即每 40 tick（2 秒）造成 1hp 伤害。duration=40 在第 40 tick 触发约 1 次伤害，猪 10→9。
// 受击后伤害免疫（m_hurtResistantTime）节流，但单次伤害足以让 hp<10。
//
// 囚笼（关键，套用 CampfireTests 模式）：猪有 AI 会乱跑，必须困在凋灵玫瑰正上方 1×1 空腔。
// 凋灵玫瑰放 (3,1,3)（y=1 空腔层，下方 y=0 glass 实心支撑），猪 spawn (3,2,3)（y=2 空腔层，
// 下落穿过花到 glass 顶 y=1.0 站稳，脚位于花方块网格内）。四周 (2,2,3)/(4,2,3)/(3,2,2)/(3,2,4)
// 放 glass 围 y=2 层，顶部 (3,3,3) 放 glass 封顶（防猪跳跃挤出）。猪仅能在 (3,2,3) 1×1 内。
//
// 判定手段：succeedWhen 每 tick 检查猪 health.currentValue < 10（满血 10，凋零首次伤害后 10→9）。
// 区域限定用 glass_pit 7×5×7 排除并行测试污染。首次伤害约 spawn 后 40 tick（2 秒）触发，
// maxTicks=200 留足余量。凋零施加是确定性时序（tick 递减 + AABB 相交，零随机），非 flaky。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_凋灵玫瑰.txt#凋灵（非和平难度碰撞施加凋零）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_凋零.txt（凋灵玫瑰来源：I 级、0:02、1hp）
function witherRoseInflictsWitherToLiving(test: Test): void {
  const pigType = "pig";

  // (3,1,3) 放凋灵玫瑰（y=1 空腔层，下方 y=0 glass 实心支撑）。方块 ID 为 minecraft:wither_rose。
  test.setBlockType("minecraft:wither_rose", { x: 3, y: 1, z: 3 });

  // 囚笼：四周 y=2 层玻璃围住猪 spawn 格 (3,2,3)，顶部封顶防跳跃挤出。
  test.setBlockType("minecraft:glass", { x: 2, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 2 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 4 });
  test.setBlockType("minecraft:glass", { x: 3, y: 3, z: 3 });

  // 猪 spawn 于 (3,2,3)（凋灵玫瑰正上方），下落穿过花到 glass 顶站稳，脚位于花方块网格内。
  test.spawn(pigType, { x: 3, y: 2, z: 3 });

  // 断言猪受凋零伤害：succeedWhen 每 tick 检查 health.currentValue < 10。
  // 时序：spawn 后约 40 tick（2 秒）凋零首次造成 1hp，10→9 < 10 满足。
  test.succeedWhen(() => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(pigs.length > 0, "pig disappeared before taking wither damage");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue < 10,
      `pig did not take wither damage from wither rose, hp=${(health as any).currentValue}`);
  });
}

// 凋灵玫瑰亡灵免疫 GameTest：亡灵族（骷髅）踩凋灵玫瑰不获得凋零效果，HP 保持满血。
//
// C++ 链路：WitherRoseBlock::onEntityCollision 内 EntityTypeTags::UNDEAD().contains(typeId)
// 守卫——骷髅属 UNDEAD 标签，提前 return 不施加凋零。对齐 Java WitherRoseBlock#entityInside
// （entity.getType().is(UNDEAD) 免疫）与 wiki"凋灵玫瑰对亡灵生物无效"。
//
// batch("night")：骷髅是亡灵，白天阳光燃烧会掉血干扰判定（与凋零无关）。夜晚 batch 设时间为
// 夜晚，骷髅不燃，HP 保持满血 20，此时 hp 不降才能归因于"凋灵玫瑰亡灵免疫"而非"阳光未触发"。
//
// 囚笼同 inflicts_wither：凋灵玫瑰 (3,1,3)，骷髅 spawn (3,2,3)，四周+顶玻璃围 1×1。
//
// 判定手段：runAtTickTime(120, ...) 在 120 tick（6 秒，远超凋零 40tick 首次伤害窗口）后检查
// 骷髅存在且 hp == 20（满血未受凋零），满足则 test.succeed()，否则 assert 失败。
// 用 runAtTickTime 而非 succeedWhen：succeedWhen 每 tick 检查，hp==20 在凋零未生效时全程为真会
// 立即 succeed（spawn 第 1 tick 就过），无法验证"持续 6 秒仍未掉血"。runAtTickTime(120) 强制等到
// 120 tick，证明整个窗口内亡灵免疫始终生效。夜晚无阳光伤害，hp==20 唯一可能掉血源是凋零，
// 未掉血即证明免疫。maxTicks=200。
//
// 【实体存活框架保障】GameTestHelper::spawnEntity 对所有 Mob 调 enablePersistence（对齐 Java
// GameTestHelper.spawn），使 DespawnManager 因 isNoDespawnRequired()==true 短路保留实体。骷髅
// 在无陪伴玩家的测试场景下不会被自然消失机制误清（详见 GameTestHelper.cpp spawnEntity 注释）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_凋灵玫瑰.txt#凋灵（亡灵免疫）
function witherRoseSpareUndead(test: Test): void {
  const skeletonType = "skeleton";

  // (3,1,3) 放凋灵玫瑰（y=1 空腔层，下方 y=0 glass 实心支撑）。
  test.setBlockType("minecraft:wither_rose", { x: 3, y: 1, z: 3 });

  // 囚笼：四周 y=2 层玻璃围住骷髅 spawn 格 (3,2,3)，顶部封顶防跳跃挤出。
  test.setBlockType("minecraft:glass", { x: 2, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 2 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 4 });
  test.setBlockType("minecraft:glass", { x: 3, y: 3, z: 3 });

  // 骷髅 spawn 于 (3,2,3)，下落穿过花到 glass 顶站稳，脚位于花方块网格内。
  test.spawn(skeletonType, { x: 3, y: 2, z: 3 });

  // 120 tick（6 秒，远超凋零 40tick 首次伤害窗口）后检查：骷髅存在且 hp==20（满血）。
  // 区域限定用 glass_pit 7×5×7 排除并行测试污染。满血即证明亡灵免疫凋零生效。
  test.runAtTickTime(120, () => {
    const sks = test.getDimension().getEntities({
      type: skeletonType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(sks.length > 0, "skeleton disappeared (should be kept alive by spawn persistence)");
    const health = sks[0].getComponent("minecraft:health");
    test.assert(
      (health as any).currentValue === 20,
      `skeleton took wither damage (should be immune as undead), hp=${(health as any).currentValue}`,
    );
    test.succeed();
  });
}

export function registerWitherRoseTests(): void {
  GameTest.register("BlockBehaviorTests", "wither_rose_inflicts_wither_to_living", witherRoseInflictsWitherToLiving)
    .structureName("gametests:glass_pit")
    .maxTicks(200);
  GameTest.register("BlockBehaviorTests", "wither_rose_spare_undead", witherRoseSpareUndead)
    .batch("night")
    .structureName("gametests:glass_pit")
    .maxTicks(200);
}
