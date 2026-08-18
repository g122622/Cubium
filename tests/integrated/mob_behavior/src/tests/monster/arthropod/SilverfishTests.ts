// 蠹虫行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 蠹虫主动攻击玩家致掉血（wiki tech_蠹虫.txt#行为：蠹虫会主动攻击玩家）。
//
// C++ 链路：SilverfishEntity : MonsterEntity（位于 monster/arthropod/，节肢生物），registerGoals 注册：
//   targetSelector 优先级2：NearestAttackableTargetGoal<LivingEntity>(checkSight=true, chance=0,
//     谓词仅放行 PLAYER)——每 tick 评估，选最近 Survival 玩家为 attackTarget。
//   goalSelector 优先级4：MeleeAttackGoal(this, 1.0, false)——读 attackTarget，navigator->moveTo 贴近，
//     distSq <= getAttackReachSqr((0.4*2)^2+0.6=1.24, 即约1.11格) 且攻击冷却结束时造成 ATTACK_DAMAGE(1.0) 伤害。
//   MeleeAttackGoal 攻击冷却 ATTACK_COOLDOWN_TICKS=20 经 adjustedTickDelay 减半约 10 tick。
//   蠹虫另有特有 goal：SilverfishSummonOthersGoal(优先级3,受伤召唤虫蚀方块内同伴)+
//   SilverfishHideInStoneGoal(优先级5,空闲1/10概率钻入可虫蚀方块消失)。HideInStone 要求"无攻击目标"
//   才触发——本测试蠹虫有玩家目标，HideInStone 不触发，蠹虫不会钻走消失。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被玻璃阻挡。
// 蠹虫无 RestrictSun/FleeSun goal + setBurnsInDaylight(false)（节肢生物不燃），白天默认环境即可主动攻击，
// 不需 batch("night")。蠹虫是节肢生物陆地行走（width 0.4 / height 0.3），creeper_pit 平地寻路通畅。
// 蠹虫(2,2,3)+玩家(3,2,3)，水平距 1 格，<1.11 攻击距离 → 蠹虫选目标后立即近战命中。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中（无散布，MeleeAttackGoal 到冷却即 hurt），
// 伤害 1.0，玩家满血 20 → 19。确定型近战用"玩家掉血"判定稳定
// （见 guardian-laser-deterministic-hit-test-strategy 确定型攻击判定策略）。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 NearestAttackableTarget 谓词滤掉）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蠹虫.txt#行为（主动攻击玩家）
function silverfishAttacksPlayer(test: Test): void {
  const silverfishType = "silverfish";

  // 蠹虫 (2,2,3)、Survival 玩家 (3,2,3)，水平距 1 格，同处结构 y=2 层。
  // 距 1 格 < 1.11 攻击距离，蠹虫选目标后 MeleeAttackGoal 直接命中（无需寻路接近）。
  // 蠹虫受 MonsterEntity 重力会下落，故脚下 (2,1,3) 放玻璃支撑；玩家脚下 (3,1,3) 放玻璃。
  // creeper_pit 开放坑无围墙，checkSight 射线不被阻挡。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 1, z: 3 });
  test.spawn(silverfishType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：NearestAttackableTarget 选目标（chance=0 每 tick 评估）+ MeleeAttackGoal 攻击冷却约 10 tick。
  // 完整周期约 10-20 tick，maxTicks=400 留充裕余量（蠹虫攻击前可能先游荡几 tick 选目标）。
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
      `silverfish did not damage player, hp=${(health as any).currentValue}`);
  });
}

// 破坏虫蚀方块时生成蠹虫（wiki tech_虫蚀方块.txt#破坏：破坏虫蚀方块会生成蠹虫，精准采集除外）。
//
// C++ 链路：InfestedBlock::spawnAfterBreak（InfestedBlock.cpp:56-104）——
//   检查 doTileDrops（true 放行）+ silk touch（tool 非空且附魔才跳过）+ isClientSide（服务端生成），
//   满足则 make_unique<SilverfishEntity> + setPosition + finalizeSpawn(Event) + spawnEntity。
//   不查光照/难度/玩家距离——破坏即生成（对齐 Java InfestedBlock.spawnAfterBreak）。
//
// 触发方式：setBlockType air 是直接 setBlockState 替换不走 destroy 链路，不触发 spawnAfterBreak。
//   须用 /setblock <pos> air destroy 命令：SetBlockCommand::_setBlockDestroy（SetBlockCommand.cpp:193-197）
//   → executeSetBlock(doDrop=true) → 替换后调 oldState->getBlock().spawnAfterBreak（:135-138，tool=nullptr
//   跳过 silk touch 检查）。命令需创造玩家 permLevel=2 经 chat 执行。
//
// 布局：glass_pit 内放 infested_stone 于 (3,2,3)，创造玩家 (4,2,3) 执行 /setblock destroy。
//   破坏后蠹虫生成于 (3.5,2,3.5)，下落至 y=1 站 grass_block（脚下 (3,1,3) air，重力下落）。
//   周围无可虫蚀宿主（grass_block 非宿主），SilverfishHideInStoneGoal 找不到宿主不钻走，蠹虫漫游存活。
//   创造玩家被 NearestAttackableTargetGoal 谓词滤掉，蠹虫无 attackTarget，HideInStone 不因有目标被禁用，
//   但 infest 检查失败仍不钻入（仅 RandomWalkingGoal 漫游）。
//
// 判定：pollUntilSucceed 断言区域内 silverfish>=1。蠹虫生成在命令执行下一 tick（spawnAfterBreak 同步
//   spawnEntity，下一 tick 入 m_entities）。命令经 chat 异步执行，留 maxTick=200 余量。
//
// 注：chat 执行命令仅 Cubium 端有效（基岩 BDS chat 是发消息语义），本测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_虫蚀方块.txt#破坏（破坏虫蚀方块生成蠹虫）
// Ref: InfestedBlock.cpp:56-104（spawnAfterBreak 不查光照/难度，破坏即生成）
// Ref: SetBlockCommand.cpp:135-138（destroy 模式调 spawnAfterBreak，tool=nullptr 跳过 silk touch）
function silverfishSpawnsFromInfestedBlock(test: Test): void {
  const silverfishType = "silverfish";
  const infestedPos = { x: 3, y: 2, z: 3 };

  // 放虫蚀石头于 (3,2,3)。infested_stone 已注册（BuildingBlocks.cpp:397）。
  test.setBlockType("minecraft:infested_stone", infestedPos);

  // 创造玩家 (4,2,3) 执行 /setblock destroy 命令（permLevel=2）。
  const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "breaker");

  // 等 5 tick 让玩家 spawn 注册稳定，再执行 destroy 命令。
  test.runAtTickTime(5, () => {
    // worldLocation 把 helper 相对坐标 (3,2,3) 转世界绝对坐标（浮点 Vector3）。
    // /setblock <x> <y> <z> air destroy：BlockPosArgumentType 解析整数坐标，Math.floor 取整。
    const w = test.worldLocation(infestedPos);
    player.chat(`/setblock ${Math.floor(w.x)} ${Math.floor(w.y)} ${Math.floor(w.z)} air destroy`);
  });

  // 轮询：破坏后蠹虫生成，区域内 silverfish>=1。
  pollUntilSucceed(test, () => {
    const silverfish = test.getDimension().getEntities({
      type: silverfishType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    return silverfish.length >= 1;
  }, {
    maxTick: 200,
    onTimeout: () => test.assert(false,
      `no silverfish spawned from infested block (silverfish=${test.getDimension().getEntities({
        type: silverfishType, location: test.worldLocation(PIT_FROM), volume: PIT_VOLUME,
      }).length})`),
  });
}

// 蠹虫受伤时召唤周围虫蚀方块中的同伴（wiki tech_蠹虫.txt#行为：受伤时呼唤附近虫蚀方块内的同伴）。
//
// C++ 链路：
//   SilverfishEntity::hurt（EndermiteEntity.cpp:161-176）检查 source.isEntitySource()||isMagic() →
//   m_summonGoal->notifyHurt() 设 m_lookForFriends=SUMMON_DURATION(20)。
//   SilverfishSummonOthersGoal::tick（SilverfishGoals.cpp:175-238）每 tick 递减计时器，归零后遍历
//   21×11×21（dx,dz∈[-10,10], dy∈[-5,5]）区域找虫蚀方块（dynamic_cast<InfestedBlock>），
//   mobGriefing=true 时 setBlockState(air) + spawnAfterBreak 生成新蠹虫，50% 概率停止。
//
// 触发方式：Survival 玩家 attackEntity(蠹虫) → SimulatedPlayer::attack → Player::attack(target)
//   （ScriptSimulatedPlayer.cpp:588-604 已实现，非 stub）→ DamageSources::playerAttack(this)
//   （EntitySource）→ 蠹虫 hurt 中 isEntitySource()=true → notifyHurt。
//   玩家 ATTACK_DAMAGE=1.0（EntityDefaultAttributes.hpp:48），damage>0 调 hurt（Player::attack:2452）。
//
// 布局：glass_pit 内蠹虫 (3,2,3)，Survival 玩家 (4,2,3)。蠹虫周围紧邻放 infested_stone：
//   (2,2,3)/(3,2,2)/(3,2,4)（(4,2,3) 是玩家位不放）。蠹虫受伤后 summonGoal 20 tick 后从中心向外遍历，
//   紧邻虫蚀方块被破坏生成新蠹虫。原蠹虫 + 召唤的 ≥1 = silverfish>=2。
//
// 蠹虫存活保证：蠹虫 MAX_HEALTH=8，玩家空手 1 伤害/次，单次 attackEntity 不致死。蠹虫受伤后
//   HurtByTargetGoal 以玩家为 attackTarget，SilverfishHideInStoneGoal shouldExecute 检查
//   attackTarget!=null 返回 false（不钻走）。蠹虫追玩家但 Survival 玩家 HP=20，蠹虫 1 伤害/次，
//   测试窗口内玩家不死，蠹虫不消失。
//
// 判定：pollUntilSucceed 断言区域内 silverfish>=2（原蠹虫+召唤同伴）。summonGoal 20 tick 计时 +
//   遍历破坏 + spawnAfterBreak 生成，maxTick=400 留余量。
//
// 注：mobGriefing 默认 true（GameTestServer 不改）。chat 不需执行命令，本测试两端通用
//   （attackEntity 已对齐基岩语义）。但基岩 SimulatedPlayer.attackEntity 行为可能差异，仍以 Cubium 为主。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蠹虫.txt#行为（受伤呼唤同伴）
// Ref: SilverfishGoals.cpp:162-238（notifyHurt 设计时器，tick 遍历破坏虫蚀方块生成蠹虫）
// Ref: EndermiteEntity.cpp:161-176（SilverfishEntity::hurt isEntitySource→notifyHurt）
function silverfishSummonsOthersWhenHurt(test: Test): void {
  const silverfishType = "silverfish";

  // 蠹虫脚下 (3,1,3) 放 glass 支撑（glass_pit y=1 air，防蠹虫下落），玩家脚下 (4,1,3) 同理。
  test.setBlockType("minecraft:glass", { x: 3, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });

  // 蠹虫 (3,2,3)，Survival 玩家 (4,2,3)（直线 1 格，attackEntity 不受距离限制远程命中）。
  const silverfish = test.spawn(silverfishType, { x: 3, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "attacker", 0 as any);

  // 蠹虫周围紧邻放 infested_stone：(2,2,3)/(3,2,2)/(3,2,4)（(4,2,3) 玩家位不放）。
  // summonGoal 从中心 (3,2,3) 向外遍历，紧邻虫蚀方块优先被破坏生成同伴。
  test.setBlockType("minecraft:infested_stone", { x: 2, y: 2, z: 3 });
  test.setBlockType("minecraft:infested_stone", { x: 3, y: 2, z: 2 });
  test.setBlockType("minecraft:infested_stone", { x: 3, y: 2, z: 4 });

  // tick 8 后玩家攻击蠹虫：留 8 tick 让实体 spawn 注册 + infested_stone 放置稳定。
  // attackEntity 转发 Player::attack → playerAttack(EntitySource) → 蠹虫 hurt → notifyHurt 设 20 tick 计时器。
  test.runAtTickTime(8, () => {
    player.attackEntity(silverfish);
  });

  // 轮询：蠹虫受伤 20 tick 后召唤同伴，区域内 silverfish>=2（原蠹虫+新同伴）。
  pollUntilSucceed(test, () => {
    const silverfishCount = test.getDimension().getEntities({
      type: silverfishType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    return silverfishCount.length >= 2;
  }, {
    maxTick: 400,
    onTimeout: () => test.assert(false,
      `silverfish did not summon others when hurt (silverfish=${test.getDimension().getEntities({
        type: silverfishType, location: test.worldLocation(PIT_FROM), volume: PIT_VOLUME,
      }).length})`),
  });
}

export function registerSilverfishTests(): void {
  GameTest.register("MobBehaviorTests", "silverfish_attacks_player", silverfishAttacksPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "silverfish_spawns_from_infested_block", silverfishSpawnsFromInfestedBlock)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "silverfish_summons_others_when_hurt", silverfishSummonsOthersWhenHurt)
    .structureName("gametests:glass_pit")
    .maxTicks(400);
}
