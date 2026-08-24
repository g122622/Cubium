// 装饰类方块行为 GameTest（营火等）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底（满铺 49 glass），y=1..2 为玻璃墙围出的内部 air 空腔，y=3..4 air+顶部框架。
// 方块测试在内部 air 层操作，需特定支撑时显式 setBlockType 覆盖玻璃底。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。

// 营火伤害 GameTest：实体站在点燃的营火上受火焰伤害（血量下降）。
//
// C++ 链路：CampfireBlock::onEntityCollision（CampfireBlock.cpp:234-269）。Entity::checkInsideBlocks
// （Entity.cpp:1317-1343）每 tick 遍历实体 AABB 覆盖的方块网格，对每个方块取
// getEntityInsideCollisionShape（默认完整方块形状，CampfireBlock 未重写）判定相交即调
// onEntityCollision。实体站在营火顶面（脚 y=营火顶 0.4375，floor→营火方块网格 y）AABB 仍与
// 营火方块相交，触发回调。
//
// onEntityCollision 内：isLit(state) 守卫（熄灭不伤害）→ 仅服务端 → dynamic_cast LivingEntity
// （掉落物/投射物等非生物不受）→ 冰霜行者靴子免疫（本测试生物无靴子，不触发免疫）→
// DamageSources::campfire()（火焰伤害，isFire()==true，bypassesInvulnerability()==false）
// → LivingEntity::hurt(campfire, 1.0f)。
//
// 受击免疫节流（关键时序）：LivingEntity::hurt（LivingEntity.cpp:222-248）首行调
// isInvulnerableTo（:778-794），当 m_hurtResistantTime>0 且伤害源不绕过无敌帧时返回 true 直接
// return false 不造成伤害。m_hurtResistantTime 每 tick 在 LivingEntity::tickEntity 中递减（:883-885）。
// onEntityCollision 每 tick 调 hurt，前 10 tick 被无敌帧阻挡，第 11 tick（m_hurtResistantTime 归 0）
// hurt 放行进入 :239 分支重置 m_hurtResistantTime=MAX_HURT_RESISTANT_TIME(10) 并造成伤害。
// 故实际约每 10 tick（半秒）承受一次 hp1，与 wiki"每半秒受到一次"一致。猪 MAX_HEALTH=10，首次
// 伤害后 10→9。
//
// 囚笼（关键）：猪有 AI 会乱跑，必须将其困在营火正上方 1×1 空腔，确保 AABB 持续与营火方块相交。
// 营火放 (3,1,3)（y=1 空腔层，下方 y=0 glass 实心支撑，营火站立方块不更新），猪 spawn (3,2,3)
// （y=2 空腔层，下落至营火顶面站稳）。四周 (2,2,3)/(4,2,3)/(3,2,2)/(3,2,4) 放 glass 围 y=2 层，
// 顶部 (3,3,3) 放 glass 封顶（防猪跳跃挤出）。猪仅能在 (3,2,3) 1×1 内，持续踩营火。
//
// 判定手段：succeedWhen 每 tick 检查猪 health.currentValue < 10（满血 10，首次火焰伤害后 10→9）。
// 区域限定用 glass_pit 7×5×7 排除并行测试污染。首次伤害约在 spawn 后 10 tick（半秒）触发，
// maxTicks=200 留足余量。伤害是确定性时序（纯 tick 递减 + AABB 相交，零随机），非 flaky。
// 注意：营火不再引燃实体（1.19.60+ 移除 setOnFire），仅即时火焰伤害，故不检测 onfire 组件。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_营火.txt#伤害（点燃营火每刻 hp1 火焰伤害，
//      受击免疫使每半秒一次，冰霜行者免疫）
function campfireDamagesEntityOnTop(test: Test): void {
  const pigType = "pig";

  // (3,1,3) 放营火（y=1 空腔层，下方 y=0 glass 实心支撑）。setBlockType 直写用默认状态 LIT=true
  // （CampfireBlock 构造 setDefaultState LIT=true），点燃状态触发伤害。glass_pit 无水，不触发
  // waterlogged 熄灭。
  test.setBlockType("minecraft:campfire", { x: 3, y: 1, z: 3 });

  // 囚笼：四周 y=2 层玻璃围住猪 spawn 格 (3,2,3)，顶部封顶防跳跃挤出。
  // 猪仅能在 (3,2,3) 1×1 内，下落至营火顶面站稳，AABB 持续与营火方块相交触发 onEntityCollision。
  test.setBlockType("minecraft:glass", { x: 2, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 2 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 4 });
  test.setBlockType("minecraft:glass", { x: 3, y: 3, z: 3 });

  // 猪 spawn 于 (3,2,3)（营火正上方），下落至营火顶面站稳。
  test.spawn(pigType, { x: 3, y: 2, z: 3 });

  // 断言猪受营火火焰伤害：succeedWhen 每 tick 检查 health.currentValue < 10。
  // 时序：spawn 后约 10 tick（半秒）首次 hurt 放行，10→9 < 10 满足。
  // 区域限定用 glass_pit 7×5×7 排除并行测试污染。
  test.succeedWhen(() => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation({ x: 0, y: 0, z: 0 }),
      volume: { x: 7, y: 5, z: 7 },
    });
    test.assert(pigs.length > 0, "pig disappeared before taking campfire damage");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue < 10,
      `pig did not take campfire damage, hp=${(health as any).currentValue}`);
  });
}

// 营火信号火 GameTest：营火下方放干草块时 signal_fire state 变 true（烟雾信号高度增加）。
//
// wiki tech_灵魂营火.txt#烟雾信号（:67）："若灵魂营火下方有干草捆，烟雾上飘的高度将会增加至约
// 25格。" 普通营火同理（信号火）。Cubium CampfireBlock::updatePostPlacement（CampfireBlock.cpp:
// 139-145）当 facing==Down 时调 _isHayBlock(world, currentPos)（:364-375，检查下方方块是否为
// HAY_BLOCK），若与当前 signal_fire 不一致则返回 state.with(SIGNAL_FIRE, signalFire)。反应同 tick
// 同步（updatePostPlacement 直接返回新 state，ServerWorld 立即 setBlockState）。
//
// 关键约束：
// 1. setBlockType 走 _resolveBlock 取 defaultState（lit=true, signal_fire=false），不经
//    getStateForPlacement（getStateForPlacement 才会放置时即据下方干草块设 signalFire）。故先放营火
//    （signal_fire=false），再放下方干草块触发营火 Down 方向 updatePostPlacement 更新 signal_fire=true。
// 2. 放干草块必须是真实状态变化（下方原本 air，air→hay_block 非 no-op）以派发邻居更新。hay_block
//    放置向 Up 邻居营火派发 updatePostPlacement(Down) → _isHayBlock true → signal_fire=true。
// 3. 营火 y=2（glass_pit 空腔层），干草块 y=1（营火下方，原本 air）。营火上方 y=3 须为 air（不阻挡，
//    本测试不测烟雾粒子，仅断言 signal_fire state）。
//
// 不测烟雾粒子上飘高度：粒子是客户端渲染行为，无头跑不可测，仅断言 signal_fire state（服务端
// 状态，与 vanilla 一致）。
// 反向复位（移除干草块→signal_fire=false）由 campfire_signal_fire_clears_when_hay_removed 覆盖，
// 与"放置触发"对称走同一条 updatePostPlacement(Down) 路径，构成双向覆盖。
//
// 跨服务端：signal_fire state 名两端一致（signal_fire，Java 式 bool），下方干草块触发判定两端一致，
// 可跨服务端对比。getState("signal_fire") 用 as any 绕过 BlockStateSuperset 白名单（同栅栏/树叶范式）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_灵魂营火.txt#烟雾信号（下方干草捆增加烟雾高度）
// Ref: CampfireBlock.cpp（updatePostPlacement Down 分支 + _isHayBlock）
function campfireSignalFireWhenHayBelow(test: Test): void {
  // (3,2,1) 放营火（defaultState lit=true signal_fire=false，下方 y=1 原本 air，无干草块故 signal_fire
  // 保持 false）。setBlockType 直写不经 getStateForPlacement，故放置时 signal_fire=false。
  test.setBlockType("minecraft:campfire", { x: 3, y: 2, z: 1 });

  // (3,1,1) 放干草块（营火下方，原本 air，air→hay_block 真实变化派发邻居更新）。hay_block 放置向
  // Up 邻居营火派发 updatePostPlacement(Down) → _isHayBlock(y=1)=true → signal_fire=true。
  test.setBlockType("minecraft:hay_block", { x: 3, y: 1, z: 1 });

  // 断言营火 (3,2,1) signal_fire state 为 true。updatePostPlacement 同步，succeedWhen 每 tick 检查。
  test.succeedWhen(() => {
    const block = test.getBlock({ x: 3, y: 2, z: 1 });
    test.assert(block !== undefined, "campfire block disappeared");
    // getState 的 key 类型是 keyof BlockStateSuperset（官方白名单），"signal_fire" 不在其中（Cubium
    // 营火专有 state 名）。用 as any 绕过编译期类型边界（同 LeavesDistanceTests/FenceConnectionTests）。
    const signalFire = block?.permutation?.getState("signal_fire" as any);
    test.assert(signalFire === true,
      `campfire signal_fire should be true when hay block below, got ${signalFire}`);
  });
}

// 营火信号火复位 GameTest：营火下方干草块被移除（设 air）时 signal_fire state 复位 false。
//
// 与 campfire_signal_fire_when_hay_below 对称，走同一条 updatePostPlacement(Down) 路径的复位分支：
// hay_block→air 真实变化派发邻居更新 → 营火 updatePostPlacement(Down, air) → _isHayBlock false →
// signal_fire=false（CampfireBlock.cpp:140-144，signalFire 与当前不一致则 with(SIGNAL_FIRE,false)）。
//
// 布局：(3,2,1) 营火 + (3,1,1) 干草块（先放使 signal_fire=true），再 (3,1,1) 设 air 移除干草块。
// air 放置向 Up 邻居营火派发 updatePostPlacement(Down) → _isHayBlock false → signal_fire=false。
//
// 判定：succeedWhen 轮询营火 signal_fire === false（干草块移除后复位）。
// 区域限定同 campfire_signal_fire_when_hay_below（仅断言 (3,2,1) 单格 state，无 getEntities 污染）。
function campfireSignalFireClearsWhenHayRemoved(test: Test): void {
  // (3,2,1) 放营火（signal_fire=false），(3,1,1) 放干草块 → 营火 signal_fire=true。
  test.setBlockType("minecraft:campfire", { x: 3, y: 2, z: 1 });
  test.setBlockType("minecraft:hay_block", { x: 3, y: 1, z: 1 });

  // 等一拍确保 signal_fire=true 已生效（干草块放置触发 updatePostPlacement 同步写，留余量防时序）。
  test.runAtTickTime(2, () => {
    const block = test.getBlock({ x: 3, y: 2, z: 1 });
    const signalFire = block?.permutation?.getState("signal_fire" as any);
    test.assert(signalFire === true,
      `campfire signal_fire should be true before hay removal, got ${signalFire}`);
    // (3,1,1) 设 air 移除干草块 → 营火 updatePostPlacement(Down, air) → _isHayBlock false → signal_fire=false。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });
  });

  // 断言营火 (3,2,1) signal_fire 复位 false。updatePostPlacement 同步，succeedWhen 每 tick 检查。
  test.succeedWhen(() => {
    const block = test.getBlock({ x: 3, y: 2, z: 1 });
    test.assert(block !== undefined, "campfire block disappeared");
    const signalFire = block?.permutation?.getState("signal_fire" as any);
    test.assert(signalFire === false,
      `campfire signal_fire should be false after hay removed, got ${signalFire}`);
  });
}

// 灵魂营火 2.0 伤害 GameTest：实体站在点燃的灵魂营火顶面受 2.0 火焰伤害（普通营火 1.0 的两倍）。
//
// C++ 链路：SoulCampfireBlock 继承 CampfireBlock，构造传 m_lightValue=10（CampfireBlock.cpp:431）。
// CampfireBlock::onEntityCollision（CampfireBlock.cpp:232-267）伤害量分支
// `const f32 damageAmount = (m_lightValue == 10) ? 2.0f : 1.0f;`（:264）——灵魂营火 m_lightValue==10
// 走 2.0f 分支，普通营火 m_lightValue==15 走 1.0f 分支。对齐 vanilla wiki"灵魂营火伤害为营火两倍"
// （tech_灵魂营火.txt 历史 20w22a）。
//
// 触发链路同 campfire_damages_entity_on_top：Entity::doBlockCollisions 每 tick 遍历实体 AABB 覆盖
// 方块网格，灵魂营火未重写 getEntityInsideCollisionShape（基类默认 fullCube），实体站灵魂营火顶面
// （矮方块 7/16=0.4375 高，脚 y=0.4375，shrink 后 floor→灵魂营火方块网格 y）AABB 与灵魂营火方块
// 相交触发 onEntityCollision。onEntityCollision 内：isLit 守卫 → 服务端守卫 → LivingEntity 守卫 →
// hurt(campfire, 2.0f)。
//
// 受击免疫节流同营火：m_hurtResistantTime 前 10 tick 阻挡，第 11 tick 放行造成 2.0 伤害，猪 10→8。
// 2.0 伤害是本测试核心区分点：若 2.0 分支失效退化为 1.0，猪首击 10→9（HP=9），不满足 HP<=8 断言。
//
// 判定手段：runAtTickTime(15, ...) 在 15 tick 后检查猪 hp<=8（满血 10，首次 2.0 灵魂营火伤害后
// 10→8）。用 runAtTickTime 而非 succeedWhen+HP<10：succeedWhen 查 HP<10 在 1.0 伤害时也满足
// （10→9<10），无法区分 2.0 vs 1.0。HP<=8 严格要求 2.0 伤害（1.0 伤害首击 HP=9>8 失败）。
// tick 15 是精确区分窗口：2.0 分支首击（tick 10）后 HP=8，1.0 分支首击（tick 10）后 HP=9，
// 1.0 分支二击需 tick 20+（无敌帧 10 tick），tick 15 时 1.0 分支仍 HP=9。诊断实测：灵魂营火
// tick 10 HP=8，普通营火 tick 10 HP=9，差 1 精确区分 2.0 vs 1.0 分支。maxTicks=120。
//
// 囚笼同 campfire_damages_entity_on_top：灵魂营火 (3,1,3)，猪 spawn (3,2,3)，四周+顶玻璃围 1×1。
// 灵魂营火无水平固体邻居自毁（CampfireBlock 无 updatePostPlacement 自毁逻辑），下方 glass 实心支撑
// 站立方块。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_灵魂营火.txt#伤害（灵魂营火伤害为营火两倍，2hp）
// Ref: CampfireBlock.cpp:264（m_lightValue==10 ? 2.0f : 1.0f 伤害分支）
// Ref: CampfireBlock.cpp:431（SoulCampfireBlock 构造传 m_lightValue=10）
function soulCampfireDamagesEntityDouble(test: Test): void {
  const pigType = "pig";

  // (3,1,3) 放灵魂营火（y=1 空腔层，下方 y=0 glass 实心支撑）。setBlockType 直写用默认状态 LIT=true
  // （SoulCampfireBlock 继承 CampfireBlock 默认状态 lit=true），点燃状态触发伤害。
  test.setBlockType("minecraft:soul_campfire", { x: 3, y: 1, z: 3 });

  // 囚笼：四周 y=2 层玻璃围住猪 spawn 格 (3,2,3)，顶部封顶防跳跃挤出。
  test.setBlockType("minecraft:glass", { x: 2, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 2 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 4 });
  test.setBlockType("minecraft:glass", { x: 3, y: 3, z: 3 });

  // 猪 spawn 于 (3,2,3)（灵魂营火正上方），下落至灵魂营火顶面站稳。
  test.spawn(pigType, { x: 3, y: 2, z: 3 });

  // 15 tick 后检查：猪存在且 hp<=8（满血 10，首次 2.0 灵魂营火伤害后 10→8）。
  // 区分窗口（诊断实测）：首次伤害约 tick 10 发生（落地快+无敌帧）。tick 15 时：
  //   - 2.0 分支：首击 10→8，hp=8<=8 满足；
  //   - 1.0 分支：首击 10→9，第二次需 tick 20+（无敌帧 10 tick），tick 15 时 hp=9>8 失败。
  // 故 hp<=8 在 tick 15 精确区分 2.0 vs 1.0 分支（诊断实测：灵魂营火 tick10 hp=8，普通营火 tick10 hp=9）。
  // 区域限定用 glass_pit 7×5×7 排除并行测试污染。
  test.runAtTickTime(15, () => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation({ x: 0, y: 0, z: 0 }),
      volume: { x: 7, y: 5, z: 7 },
    });
    test.assert(pigs.length > 0, "pig disappeared before taking soul campfire damage");
    const health = pigs[0].getComponent("minecraft:health");
    test.assert((health as any).currentValue <= 8,
      `pig did not take 2.0 soul campfire damage (expected hp<=8, got hp=${(health as any).currentValue};`
      + ` hp=9 would indicate 1.0 branch instead of 2.0)`);
    test.succeed();
  });
}

export function registerCampfireTests(): void {
  GameTest.register("BlockBehaviorTests", "campfire_damages_entity_on_top", campfireDamagesEntityOnTop)
    .structureName("gametests:glass_pit")
    .maxTicks(200);
  GameTest.register("BlockBehaviorTests", "campfire_signal_fire_when_hay_below", campfireSignalFireWhenHayBelow)
    .structureName("gametests:glass_pit")
    .maxTicks(60);
  GameTest.register("BlockBehaviorTests", "campfire_signal_fire_clears_when_hay_removed", campfireSignalFireClearsWhenHayRemoved)
    .structureName("gametests:glass_pit")
    .maxTicks(80);
  GameTest.register("BlockBehaviorTests", "soul_campfire_damages_entity_double", soulCampfireDamagesEntityDouble)
    .structureName("gametests:glass_pit")
    .maxTicks(120);
}
