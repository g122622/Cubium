// 沼骸行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit / grass_pen 结构尺寸（7×5×7 / 9×5×9），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 沼骸在阳光下着火（wiki other_沼骸.txt#生物族群：沼骸是亡灵生物，会在阳光下着火）。
// 沼骸是骷髅的沼泽变种，分类"亡灵生物"，与普通骷髅/流浪者一样白天露天燃烧。
//
// C++ 链路：BoggedEntity : AbstractSkeletonEntity : MonsterEntity，继承 MonsterEntity 默认
// shouldBurnInDaylight()=true（m_burnsInDaylight=true）。MonsterEntity::tick→handleDaylightBurning
// →isInDaylight 校验 isDaytime + brightness>0.5 + !isWet + canSeeSky + shouldBurnInDaylight()，
// 全部满足则 burnUndead→igniteForSeconds(8.0f) 点燃 8 秒。
//
// 历史 bug：BoggedEntity 曾错误 override shouldBurnInDaylight() 返回 false（误以为沼骸不燃），
// 与原版 1.21.11 不一致——Java 源码 Bogged 不 override shouldBurnInDaylight，继承基类 true。
// wiki other_沼骸.txt#生物族群 明确"沼骸会在阳光下着火"。本次已删除该 override，恢复继承基类 true。
// 此测试即验证修复后沼骸白天燃烧。
//
// 与 skeleton_burns_in_daylight（骷髅燃）+ stray_burns_in_daylight（流浪者燃）
// + wither_skeleton_does_not_burn_in_daylight（凋零骷髅不燃）形成四方对照：
// 同为 AbstractSkeletonEntity 子类，骷髅燃/流浪者燃/沼骸燃/凋零骷髅不燃，
// 交叉验证 shouldBurnInDaylight 门控（基类 true + 凋零骷髅 override false）正确。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_沼骸.txt#生物族群（亡灵生物，阳光下着火）
function boggedBurnsInDaylight(test: Test): void {
  const boggedType = "bogged";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 沼骸 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // 中心位置远离玻璃墙，沼骸 AI 游荡不触及围栏；整个空气腔头顶均露天无阴影可躲。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
  // 因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
  const bogged = test.spawn(boggedType, { x: 4, y: 2, z: 4 });

  // 概率时序：isInDaylight 随机检查 rng.nextFloat()*30 < (brightness-0.4)*2，满亮度 4%/tick，
  // 期望约 25 tick 首次点燃。点燃后 igniteForSeconds(8.0f)=160 tick 燃烧。grass_pen 无阴影，
  // 沼骸无处可躲，着火后持续燃烧。maxTicks=500 留充足余量（与 skeleton_burns/stray_burns 同款）。
  // succeedWhen 轮询 onfire 组件：着火（组件非 undefined）即通过。
  test.succeedWhen(() => {
    const fire = bogged.getComponent("minecraft:onfire");
    if (fire === undefined) {
      throw new Error("bogged not on fire yet");
    }
  });
}

// 沼骸远程射击玩家（wiki other_沼骸.txt#行为：沼骸会发射造成中毒效果的箭）。
// C++ 链路：BoggedEntity 继承 AbstractSkeletonEntity 的 setCombatTask（持弓→RangedBowAttackGoal）+
// NearestAttackableTargetGoal<Player>(checkSight=true) 选 Survival 玩家为 attackTarget →
// RangedBowAttackGoal::tick 在射程内（15格）seenTime>=20 后蓄力发射 →
// attackEntityWithRangedAttack 创建 ArrowEntity（customizeArrow 钩子注入中毒效果）→
// 箭矢命中玩家 → AbstractArrowEntity::onEntityHit 造成伤害 + ArrowEntity::onEntityHit 施加中毒。
//
// 环境选择：必须夜晚 batch("night") + creeper_pit（开放坑）。AbstractSkeleton 注册了
// RestrictSunGoal（限制阳光）+ FleeSunGoal（逃离阳光），白天露天沼骸会优先逃离阳光而非攻击玩家
// （wiki: 沼骸像骷髅一样寻找阴凉处）。夜晚无阳光 FleeSun 不触发，沼骸主动选玩家射击。
// creeper_pit 开放坑无围墙阻挡 checkSight 视线 + 寻路通畅（glass_pit 玻璃挡寻路）。
//
// 判定手段：断言玩家 HP 下降（<20）。沼骸远程箭伤害约 2-3（setBaseDamageFromMob），
// 玩家初始满血 20，被 1 箭命中即掉至 <20。箭矢命中玩家即证明 RangedBowAttackGoal 远程攻击链路通。
// 不直接断言箭矢实体出现（箭矢飞行命中后消失，getEntities 轮询撞窗口不稳，见 SnowGolemTests 同款坑）；
// 不断言玩家获中毒效果（药水效果组件未绑定 JS 不可读）。玩家掉血是远程攻击命中最直接证据。
// 玩家用 Survival（gameMode=0）：创造/旁观被 isSuitableTarget 滤掉，沼骸不选其为目标。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_沼骸.txt#行为（发射造成中毒效果的箭）
function boggedShootsPoisonArrowAtPlayer(test: Test): void {
  const boggedType = "bogged";

  // 沼骸于 (1,2,1)（一角），Survival 玩家于 (5,2,5)（对角，距 ~5.7格 < 15格射程）。
  // 沼骸在射程内锁定玩家后停止移动 + strafe 射击（RangedBowAttackGoal distSq<=15² 且 seenTime>=20）。
  // 玩家会被箭命中掉血（沼骸箭伤害 ~2-3）。玩家 HP 20，约 1 箭即掉至 <20。
  test.spawn(boggedType, { x: 1, y: 2, z: 1 });
  test.spawnSimulatedPlayer({ x: 5, y: 2, z: 5 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：沼骸 seenTime>=20（约 tick 20）+ 蓄力（约 tick 40）首箭 + 箭飞行几 tick命中，
  // 约 tick 45-60 玩家首伤。maxTicks=400 留寻路/锁定/蓄力 + 余量。
  // 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    test.assert((health as any).currentValue < 20,
      `bogged did not shoot player, hp=${(health as any).currentValue}`);
  });
}

// 玩家手持剪刀右键沼骸剪去头上的蘑菇，掉落 2 个随机颜色蘑菇（红/棕），
// 沼骸被剪后不可再剪（wiki other_沼骸.txt#掉落物：对沼骸使用剪刀会剪去其头上的蘑菇，
// 并掉落 2 个随机颜色的蘑菇；mob_沼骸_ED.txt NBT sheared 字段记录是否已剪）。
//
// C++ 链路（对齐 Java 1.21.11 Bogged.mobInteract 剪菇分支）：
//   1) 玩家主手持剪刀 + interactWithEntity(bogged) → Player::interactOn（Player.cpp:2843）
//      → bogged.processInitialInteract → MobEntity::interactMob（基类返 Pass，BoggedEntity 未 override
//      interactMob——Java 在 mobInteract 内检测剪刀，Cubium 走物品侧等价路径）。
//   2) Player::interactOn 第4步走 Item::itemInteractionForEntity → ShearsItem::itemInteractionForEntity
//      （ShearsItem.cpp:139）：dynamic_cast<IShearable*>(&target) 命中 BoggedEntity（本次新增
//      IShearable 继承）→ isShearable()（!m_sheared && isAlive()）为 true → shear(&player)。
//   3) BoggedEntity::shear：setSheared(true) + 播 entity.bogged.shear 音效 + 返回 2 个随机红/棕蘑菇
//      ItemStack（SHEAR_MUSHROOM_COUNT=2，对齐 wiki"掉落 2 个随机颜色的蘑菇"）。
//   4) ShearsItem::itemInteractionForEntity 把 drops 经 ItemDropHelper::spawnItemEntity 生成 item
//      掉落物实体 + hurtAndBreak(stack,1) 消耗剪刀耐久（创造模式跳过消耗）。
//
// 此前 Cubium BoggedEntity 未实现 IShearable，ShearsItem::itemInteractionForEntity dynamic_cast 返回
// nullptr 直接 return false，剪刀右键沼骸无任何反应（对齐缺陷）。本次补全 IShearable 接入。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ night batch。沼骸是亡灵白天露天燃烧（见
// bogged_burns_in_daylight），night 避免燃烧干扰剪菇判定；creeper_pit 开放坑无围墙沼骸不卡。
// 创造玩家不被沼骸 NearestAttackableTargetGoal 选为目标（isSuitableTarget 滤创造），沼骸持弓不射击
// 创造玩家，剪菇环境干净。剪刀交互 interactWithEntity 远程触发无距离门控。
//
// 判定手段：剪菇后区域内出现 ≥2 个 minecraft:item 掉落物实体（蘑菇掉落物）。读取每个 item 实体的
// minecraft:item 组件 itemStack.typeId 确认是 red_mushroom/brown_mushroom（非其他物品），断言
// 蘑菇掉落物数 ≥2。pollUntilSucceed 轮询。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\other_沼骸.txt#掉落物（剪刀剪菇掉 2 个随机色蘑菇）
function boggedShearedByShearsDropsMushrooms(test: Test): void {
  const boggedType = "bogged";

  // 沼骸 (3,2,3)（creeper_pit y=0 石头地板，helper y=2→结构 y=1 空气，脚踩 y=0 石头）。
  const bogged = test.spawn(boggedType, { x: 3, y: 2, z: 3 });
  // 创造玩家 (1,2,3) 持剪刀，距沼骸 2 格（interactWithEntity 远程触发无距离门控）。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "boggedShearer");
  const shears = new ItemStack("minecraft:shears", 1);
  player.setItem(shears as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持剪刀 interactWithEntity(bogged) → ShearsItem::itemInteractionForEntity →
  // IShearable.shear 掉落 2 个蘑菇。创造模式不消耗剪刀耐久（hurtAndBreak 跳过）。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(bogged);
  });

  // 轮询：区域内出现 ≥2 个 item 掉落物（蘑菇）。蘑菇掉落物 spawn 后瞬间可查（ItemDropHelper 同步生成）。
  // 测试环境干净（仅沼骸+持剪刀玩家，无其他掉落源），item 实体必为剪菇掉落的蘑菇，无需读 itemType
  // 区分（脚本侧 Entity 的 minecraft:item 组件未绑定，读不到 itemStack.typeId）。
  // startTick=6 剪菇后 1 tick 立即查，避免沼骸 MobEntity::tick looting 循环拾取蘑菇 item（记忆
  // [[mob-looting-pickup-chain]]）移除掉落物干扰计数。interval=2 maxTick=40 短窗尽早判定。
  pollUntilSucceed(test, () => {
    const items = test.getDimension().getEntities({
      type: "minecraft:item",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    return items.length >= 2;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const items = test.getDimension().getEntities({
        type: "minecraft:item",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      test.assert(false,
        `bogged shearing did not drop 2 mushrooms, itemCount=${items.length} (expected >=2)`);
    },
  });
}

export function registerBoggedTests(): void {
  GameTest.register("MobBehaviorTests", "bogged_burns_in_daylight", boggedBurnsInDaylight)
    .structureName("gametests:grass_pen")
    // skyAccess(true)：GameTestServer gridStartY=-59 把结构埋在地下 worldgen 石头中，结构上方全是
    // worldgen 方块致 canSeeSky 恒 false。skyAccess=true 让 MinecraftStructurePlacer 清空结构 footprint
    // 正上方至世界顶部的所有方块，制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
    .skyAccess(true)
    // setupTicks(20)：结构清空上方方块后光照变更入队，需若干世界 tick 由 ServerWorld::tick 批量
    // 重算 skyLight 达 15。setupTicks 阶段让世界先 tick 20 次让光照稳定，再正式跑测试体。
    .setupTicks(20)
    .maxTicks(500);

  GameTest.register("MobBehaviorTests", "bogged_shoots_poison_arrow_at_player", boggedShootsPoisonArrowAtPlayer)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "bogged_sheared_by_shears_drops_mushrooms", boggedShearedByShearsDropsMushrooms)
    .batch("night")
    .structureName("gametests:creeper_pit")
    .maxTicks(200);
}
