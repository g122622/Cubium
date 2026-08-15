// 铁砧(Anvil)行为 GameTest（下落砸中实体造成坠落伤害）。
//
// 本测试验证铁砧作为下落的方块(FallingBlockEntity)落地时对碰撞箱内实体造成伤害的链路，
// 与 HayBlockTests/PointedDripstoneTests 的"实体摔落到方块上"(onFallenUpon)方向相反：
// 此处是"方块砸到实体上"(FallingBlockEntity::_hurtEntities)。
//
// C++ 链路：
//   1. AnvilBlock 继承 FallingBlock（AnvilBlock.hpp）。setBlockType 放铁砧触发 FallingBlock::
//      onBlockAdded 调度 2 tick 后 tick → 检测下方空气 → 生成 FallingBlockEntity。
//   2. AnvilBlock::onStartFalling 在 entity 上设：
//        setHurtEntities(true)
//        setFallDamagePerDistance(2.0f)   // 每格 2 伤害
//        setFallDamageMax(40)             // 上限 40
//   3. FallingBlockEntity::tick 重力下落，onGround() 后 _handleLanding。
//   4. _handleLanding 若 m_hurtEntities 调 _hurtEntities。
//   5. _hurtEntities：
//        fallDistance = m_fallStartY - y()        // 下落高度
//        effectiveDistance = ceil(fallDistance - 1) // 第一格不计
//        damage = min(effectiveDistance * 2.0, 40)  // wiki 公式 min{2(h-1),40}
//        hurtBox = boundingBox()                   // 铁砧 AABB（size 0.98×0.98）
//        对 hurtBox 内每个 LivingEntity hurt(anvil 伤害源, damage)
//
// wiki 行为（other_铁砧.txt:78-82,490）：
//   "铁砧在下方没有不可替代的方块时会变为下落的方块。"
//   "铁砧落地时，会对当前位置的实体造成 min{2(h-1),40} 的伤害，其中 h 为下落的高度。"
//   "铁砧下落所能造成的最大伤害是 40hp，能够杀死同一位置上绝大多数具有满生命值的生物。"
//   公式与 C++ 一致：effectiveDistance=h-1（整数 h），damage=min(2(h-1),40)。
//
// 落差设计（fall_tower 7×16×7，中心 (3,*,3) 1×1 玻璃管）：
//   中心列结构自带 y=0,1 cobblestone 地基（Cubium 实跑核实），y=2..14 air 为下落通道。
//   (3,0,3) 显式 setBlockType cobblestone：① 固体支撑铁砧落地（铁砧撞到 cobble 停在 (3,2,3)）；
//                                            ② 兜底阻挡，防铁砧穿到 y<0。
//   猪 spawn (3,2,3)：站在 cobble(y1) 顶面，脚 y=2.0，占据 (3,2,3) 格——即铁砧落点格。
//                     （不可 spawn 在 (3,1,3)：卡进 cobble 固体被弹飞至虚空，此前诊断已证。）
//   铁砧放 (3,14,3)：下方 air 触发下落，落差 h = 14 - 2 = 12 格（落到 cobble 顶面 y2.0 上方）。
//   damage = min(2*(12-1), 40) = min(22, 40) = 22 >> 猪 MAX_HEALTH 10，猪必死（实体消失）。
//
// 关键时序：铁砧 setBlockType 后 2 tick 才生成 FallingBlockEntity 开始下落；猪 spawn 后立即下落
// 占位。两者在 (3,2,3) 汇合：猪先到（自由落体 ~ 数 tick），铁砧后到（落差 12 格 ~ 26 tick）。
// FallingBlockEntity 无实体碰撞（穿过猪），靠方块碰撞落地，停在 cobble 上方 (3,2,3)，
// _hurtEntities 的 hurtBox 与猪 AABB 相交（同格），砸死猪。maxTicks=200 留足下落时间。
//
// 囚笼：fall_tower 中心 1×1 玻璃管（结构自带管壁 y=1..15）已围住 (3,*,3) 落管，猪只能垂直活动，
// 无法 AI 乱跑离开落点。封顶 y=15 防弹出。
//
// 判定手段：succeedWhen 每 tick 检查猪实体已消失（length === 0，被铁砧砸死）。区域限定
// fall_tower 7×16×7 排除并行测试污染。下落是确定性时序（重力 + AABB，零随机），非 flaky。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_铁砧.txt#下落的方块（min{2(h-1),40} 伤害）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。
// 中心 (3,*,3) 为 1×1 垂直玻璃管落管：y=0 中心格默认 rail（非固体），y=1..14 中心柱 air
// （下落通道），四周管壁 glass（y=1..15），y=15 顶部 (3,15,3) glass 封顶。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

// 铁砧下落砸死下方实体 GameTest：铁砧从 fall_tower 顶部下落，砸中下方落点格的猪，
// 造成 22 伤害（≥猪 10 血），猪被砸死（实体消失）。
function anvilFallingKillsEntityBelow(test: Test): void {
  const pigType = "pig";

  // (3,0,3) 放 cobblestone：固体支撑铁砧落地 + 兜底阻挡。fall_tower 中心列 y=0,1 结构自带 cobble
  // 地基（Cubium 实跑核实），此处显式覆盖一次确保固体支撑（与 PointedDripstoneTests 同模式）。
  test.setBlockType("minecraft:cobblestone", { x: 3, y: 0, z: 3 });

  // 猪 spawn 于 (3,2,3)：cobble 顶面在 y=2.0（cobble 占 y0,y1），猪脚 y=2.0 站 cobble 顶，
  // 占据 (3,2,3) 格——即铁砧落点格。猪 AABB y∈[2.0,2.9]。
  // 注意：猪不能 spawn 在 cobble 所在格 (3,1,3)（会卡进固体被弹飞，此前诊断猪被弹到 y=-58 虚空）。
  test.spawn(pigType, { x: 3, y: 2, z: 3 });

  // (3,14,3) 放铁砧：下方 (3,13,3) air 触发 FallingBlock 下落，落差 12 格（落到 cobble 上方 y2）。
  // setBlockType 放铁砧（默认 facing，facing 不影响伤害）。2 tick 后生成 FallingBlockEntity。
  // 铁砧落到 cobble(y1) 顶面 y2.0，铁砧 AABB y∈[2.0,2.98]，与猪 AABB y∈[2.0,2.9] 相交 → hurtBox 命中。
  test.setBlockType("minecraft:anvil", { x: 3, y: 14, z: 3 });

  // 断言猪被铁砧砸死：succeedWhen 每 tick 检查猪实体已消失（length === 0）。
  // 铁砧伤害 = min(2*(12-1), 40) = 22 >> 10，猪必死。区域限定 fall_tower 7×16×7 排除并行污染。
  test.succeedWhen(() => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(TOWER_FROM),
      volume: TOWER_VOLUME,
    });
    test.assert(
      pigs.length === 0,
      `pig survived anvil fall (should take 22 damage and die), remaining=${pigs.length}`,
    );
  });
}

export function registerAnvilTests(): void {
  GameTest.register("BlockBehaviorTests", "anvil_falling_kills_entity_below", anvilFallingKillsEntityBelow)
    .structureName("gametests:fall_tower")
    .maxTicks(200);
}
