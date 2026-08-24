// 火焰类方块行为 GameTest（灵魂火 onEntityCollision 引燃+火焰伤害）。
//
// 验证 Cubium SoulFireBlock（继承 FireBlock）的 onEntityCollision 火焰伤害链路对齐 vanilla 1.21.11：
// 实体接触灵魂火 → hurt(inFire, 2.0) + igniteForSeconds(8) 引燃，血量下降。灵魂火伤害为普通火（1.0）
// 的两倍（wiki tech_灵魂火.txt#伤害）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。
// 中心 (3,*,3) 为 1×1 垂直玻璃管落管：y=0 中心格默认为 rail（非固体），y=1..14 中心柱 air，
// 四周管壁 glass（y=1..15），y=15 顶部 glass 封顶。区域限定查询排除批内并行 tick 跨测试污染。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

// 灵魂火灼烧接触实体 GameTest：实体站在灵魂火方块所在格（灵魂火在脚下格）持续触发 onEntityCollision，
// 受 2.0 火焰伤害（普通火 1.0 的两倍）+ 8 秒引燃，血量下降至 hp<=8。
//
// C++ 链路：SoulFireBlock 继承 FireBlock，构造传 m_fireDamage=2（SoulFireBlock.cpp:36）。onEntityCollision
// 继承 FireBlock::onEntityCollision（FireBlock.cpp:299-328）：
//   1. isImmuneToFire 守卫（火焰免疫实体不伤，本测试猪非免疫）；
//   2. 火焰免疫期倒计时处理（remainingFireTicks<0 时每 tick +1）；
//   3. 非免疫期 forceFireTicks(+1) 累积火焰计时器；
//   4. remainingFireTicks>=0 时 igniteForSeconds(8.0f)（引燃 160 tick）；
//   5. hurt(DamageSources::inFire(), m_fireDamage)（灵魂火 m_fireDamage=2.0）。
//
// 触发链路：Entity::doBlockCollisions（Entity.cpp:1338-1400）每 tick 遍历实体 AABB 覆盖方块格，对每格取
// block.getEntityInsideCollisionShape（:1373）。SoulFireBlock 未重写该方法，基类默认返回 fullCube
// （Block.cpp:371-378），走快速路径 isInsideBlock=true（AABB 与灵魂火格重叠即触发）。灵魂火 getCollisionShape
// 返回 empty（继承 FireBlock，实体可穿过），但实体站灵魂火格上方时 AABB 经 shrink(0.001) 后 minY 落入
// 灵魂火格 floor → 覆盖灵魂火格 → onEntityCollision。
//
// 灵魂火永燃（关键，区别于普通火）：SoulFireBlock::randomTick/tick 空实现（SoulFireBlock.cpp:90-109），
// 状态数 1 无 age，不老化不蔓延不熄灭。故实体可持续站灵魂火格反复触发 onEntityCollision（普通火 age
// 增长会熄灭，难稳定测试；灵魂火是火焰伤害链路的稳定可测目标）。
//
// 受击免疫节流（关键时序）：onEntityCollision 每 tick 调 hurt(inFire, 2.0)，LivingEntity::hurt 首行
// isInvulnerableTo 门控，m_hurtResistantTime>0 时 return。首 tick 实体 m_hurtResistantTime=0（无敌帧
// 未建立），首次 hurt 立即生效造成 2.0 伤害（10→8）并重置 m_hurtResistantTime=MAX(10)。之后前 10 tick
// 被无敌帧阻挡，第 11 tick 放行第二击。引燃（igniteForSeconds 8 秒）使猪燃烧持续掉血（每 40 tick 1.0
// 火焰伤害），但首击 2.0 即时伤害远大于燃烧，tick 15 断言 hp<=8 可靠区分 2.0 分支。
//
// 几何（fall_tower 中心 1×1 玻璃管囚禁实体防 AI 乱跑离开灵魂火格）：
//   - (3,0,3) soul_sand：灵魂火下方支撑（SoulFireBlock::isValidPosition 须下方 soul_fire_base_blocks，
//     soul_sand 在标签内；否则 updatePostPlacement 自毁）。fall_tower y=0 中心格默认 rail 非固体，需
//     替换为 soul_sand 既作支撑又满足灵魂火存活条件。
//   - (3,1,3) soul_fire：灵魂火方块（setBlockType 直写默认 state，下方 soul_sand 合法存活永燃）。
//   - 猪 spawn (3,2,3)：下落 1 格（fallDistance<1 不触发摔伤干扰），落 soul_sand 顶（脚 y=1.0），
//     AABB y∈[1.0,1.9] 占据灵魂火格 y=1，shrink(0.001) minY=1.001 floor=1 覆盖灵魂火格 → onEntityCollision。
//
// 判定手段：runAtTickTime(8, ...) 在 8 tick 后检查猪 hp===8（满血 10，首次 2.0 灵魂火伤害后 10→8）。
// 精确区分窗口：首击约 tick 2（无敌帧未建立首 tick 即生效，2.0 分支 10→8）。第二击需首击 tick2 +
// 无敌帧 10 tick = tick 12+。tick 8 时 2.0 分支 hp=8（首击后无敌帧内），1.0 分支（若误用）hp=9（首击
// 后无敌帧内，第二击未到）。故 hp===8 在 tick 8 精确区分 2.0 vs 1.0 分支（诊断实测 4 次复跑稳定
// hp=8）。用 runAtTickTime 而非 succeedWhen+HP<10：succeedWhen 查 HP<10 在 1.0 伤害时也满足
// （10→9<10），无法区分 2.0 vs 1.0。区域限定 fall_tower 7×16×7 排除并行测试污染。maxTicks=120。
//
// 选猪：猪 MAX_HEALTH=10，2.0 伤害一击 hp=8<=8 满足断言且不致死；猪非火焰免疫（isImmuneToFire=false），
// onEntityCollision 均生效。猪体积小（宽 0.9）落 fall_tower 1×1 玻璃管居中。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_灵魂火.txt#伤害（灵魂火伤害为普通火两倍，2hp）
// Ref: FireBlock.cpp:299-328（onEntityCollision：isImmuneToFire 守卫 + igniteForSeconds(8) + hurt(inFire, m_fireDamage)）
// Ref: SoulFireBlock.cpp:36（SoulFireBlock 构造传 m_fireDamage=2）
// Ref: SoulFireBlock.cpp:90-109（randomTick/tick 空实现，灵魂火永燃不熄灭）
function soulFireDamagesEntityDouble(test: Test): void {
  const pigType = "pig";

  // (3,0,3) 放灵魂沙（灵魂火下方支撑，soul_sand 在 soul_fire_base_blocks 标签，灵魂火存活永燃）。
  // fall_tower y=0 中心格默认 rail 非固体，替换为 soul_sand。
  test.setBlockType("minecraft:soul_sand", { x: 3, y: 0, z: 3 });

  // (3,1,3) 放灵魂火（默认 state，下方 soul_sand 合法存活永燃）。setBlockType 直写不经 isValidPosition。
  test.setBlockType("minecraft:soul_fire", { x: 3, y: 1, z: 3 });

  // 猪 spawn 于 (3,2,3)（灵魂火正上方 1 格），下落 1 格落 soul_sand 顶（脚 y=1.0），AABB 占据灵魂火格 y=1。
  test.spawn(pigType, { x: 3, y: 2, z: 3 });

  // 8 tick 后检查：猪存在且 hp<=8（满血 10，首次 2.0 灵魂火伤害后 10→8）。
  // 精确区分窗口：首击约 tick 2（无敌帧未建立首 tick 即生效，2.0 分支 10→8 / 1.0 分支 10→9）。
  // 第二击需首击 tick2 + 无敌帧 10 tick = tick 12+。tick 8 时：
  //   - 2.0 分支：首击 hp=8<=8 满足；
  //   - 1.0 分支（若误用）：首击 hp=9>8 失败（第二击未到）。
  // 故 hp<=8 在 tick 8 精确区分 2.0 vs 1.0 分支。区域限定 fall_tower 7×16×7 排除并行测试污染。
  test.runAtTickTime(8, () => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(TOWER_FROM),
      volume: TOWER_VOLUME,
    });
    test.assert(pigs.length > 0, "pig disappeared before taking soul fire damage");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue === 8,
      `pig did not take exactly 2.0 soul fire damage (expected hp=8, got hp=${(health as any).currentValue};`
      + ` hp=10 means onEntityCollision not triggered; hp=9 would indicate 1.0 branch instead of 2.0;`
      + ` hp<8 means damage exceeded 2.0 or burn tick applied)`);
    test.succeed();
  });
}

export function registerFireTests(): void {
  GameTest.register("BlockBehaviorTests", "soul_fire_damages_entity_double", soulFireDamagesEntityDouble)
    .structureName("gametests:fall_tower")
    .maxTicks(120);
}
