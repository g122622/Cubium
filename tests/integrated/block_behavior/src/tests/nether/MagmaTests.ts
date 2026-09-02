// 下界类方块行为 GameTest（岩浆块等）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底（满铺 49 glass），y=1..2 为玻璃墙围出的内部 air 空腔，y=3..4 air+顶部框架。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。

// 岩浆块伤害 GameTest：实体站在岩浆块上受烫脚火焰伤害（血量下降）。
//
// C++ 链路：MagmaBlock::onEntityWalk（MagmaBlock.cpp:83-109）。调用点为
// Entity::doBlockCollisions() 末尾（Entity.cpp:1506-1518），门控条件
// m_onGround && !isSteppingCarefully()。doBlockCollisions 每帧由
// LivingEntity::aiStep() 末尾调用（Entity.cpp:1932），Player 则经
// Player::updatePhysics() 末尾显式调用（Player.cpp:1267，因 Player::aiStep
// 重写 LivingEntity::aiStep 且不调用父类 doBlockCollisions）。故站在方块上
// 的实体每帧触发 onEntityWalk（对齐 vanilla Entity.move 末尾
// if(onGround) block.stepOn(...) 语义）。
// onEntityWalk 内：dynamic_cast LivingEntity（非生物不受）→ !isSteppingCarefully
// 门控（潜行实体直接 return）→ DamageSources::hotFloor()（火焰伤害，
// isFire()==true，bypassesInvulnerability()==false）→ LivingEntity::hurt(hotFloor, 1.0f)。
//
// 受击免疫节流（关键时序）：同营火——LivingEntity::hurt 首行 isInvulnerableTo
// （:778-794）当 m_hurtResistantTime>0 且伤害源不绕过无敌帧时直接 return false。
// m_hurtResistantTime 每 tick 递减（:883-885）。onEntityWalk 每 tick 调 hurt，
// 前 10 tick 被无敌帧阻挡，第 11 tick 放行造成伤害并重置无敌帧。故实际约每
// 10 tick（半秒）承受一次 hp1，与 wiki"受击免疫减慢至每半秒一次"一致。
// 猪 MAX_HEALTH=10，首次伤害后 10→9。
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

// 岩浆块潜行免疫 GameTest：玩家潜行站在岩浆块上不受烫脚伤害（血量保持满血）。
//
// wiki tech_岩浆块.txt#伤害（:60）："玩家也可以通过潜行……完全免疫伤害。"
// C++ 门控：onEntityWalk 派发点 Entity::doBlockCollisions()（Entity.cpp:1511）双重门控
// m_onGround && !isSteppingCarefully()——潜行实体 isSteppingCarefully()=true 致派发点不调
// onEntityWalk。方块侧 MagmaBlock::onEntityWalk（MagmaBlock.cpp:96-99）另有 isSteppingCarefully
// 自查（双重保险）。故潜行玩家站岩浆块上 onEntityWalk 不触发，无 hurt 调用，HP 不降。
//
// 关键链路（潜行态持续）：SimulatedPlayer 经脚本 isSneaking setter 设潜行（ScriptSimulatedPlayer.cpp
// isSneaking 绑定）。setter 经 handleMovementInput 设 m_inputSneaking=true + setSneaking(true)，
// 使 Player::_applyCachedMovementInput（Player.cpp:903-915，updatePhysics 每 tick 调用）每 tick
// 持续 setSneaking(true) 保持潜行（否则 m_inputSneaking=false + m_isSneaking=true 会触发
// setSneaking(false) 清除潜行）。isSteppingCarefully()→isSneaking()→Player::m_isSneaking=true。
//
// 与 magma_damages_entity_on_top（猪非潜行受伤）正反对照：潜行免疫 vs 非潜行受伤 = 潜行门控正确。
// 不用猪测潜行（猪不会自主潜行，脚本 isSneaking 对非 Player 实体虽可设但 LivingEntity::isSneaking
// 继承 Entity 默认 false，且 MobEntity 无 _applyCachedMovementInput 清除问题——但猪的潜行语义与
// 玩家不同，故用 SimulatedPlayer 测玩家潜行免疫，贴合 wiki"玩家可通过潜行免疫"原文）。
//
// 布局：岩浆块 (3,1,3)（glass_pit y=1 空腔层，下方 y=0 glass 支撑）+ y=2,y=3 两层四周 glass 囚笼
// （2 格高墙，不在猪正上方 (3,3,3) 放玻璃防挤压——onEntityWalk 依赖 m_onGround）。SimulatedPlayer
// spawn (3,2,3) Survival 模式（0，HP=20），落地站稳后设 isSneaking=true。
//
// 判定：pollUntilSucceed 轮询读 player HP === 20（满血，完全免疫）。留足 tick 让玩家落地站稳 +
// 首次 onEntityWalk 触发窗口（若潜行门控缺失，落地后约 10 tick 首次 hurt 放行，HP 20→19，
// 故等够时间后仍满血即证免疫）。
//
// 跨服务端：magma_block 方块名两端一致。SimulatedPlayer.isSneaking 是官方 @minecraft/server
// Entity 属性（index.d.ts:5672，可读写）。潜行免疫烫脚行为两端一致，可跨服务端对比。
function magmaSneakingPlayerImmuneToDamage(test: Test): void {
  // (3,1,3) 放岩浆块（完整方块，玩家可站顶面）。下方 y=0 glass 实心支撑。
  test.setBlockType("minecraft:magma_block", { x: 3, y: 1, z: 3 });

  // 囚笼：y=2 和 y=3 两层四周 glass（2 格高墙），不在玩家正上方 (3,3,3) 放玻璃防挤压。
  // 顶部依靠结构 y=4 顶框玻璃防玩家跳出。同 magma_damages_entity_on_top。
  for (const y of [2, 3]) {
    test.setBlockType("minecraft:glass", { x: 2, y, z: 3 });
    test.setBlockType("minecraft:glass", { x: 4, y, z: 3 });
    test.setBlockType("minecraft:glass", { x: 3, y, z: 2 });
    test.setBlockType("minecraft:glass", { x: 3, y, z: 4 });
  }

  // SimulatedPlayer spawn (3,2,3) Survival 模式（0=GameModeSurvival，HP=20）。
  const player = test.spawnSimulatedPlayer({ x: 3, y: 2, z: 3 }, "sneaker", 0 as any) as any;

  // 设潜行：经 isSneaking setter（ScriptSimulatedPlayer.cpp 绑定）设 m_inputSneaking=true +
  // setSneaking(true)，使潜行态每 tick 持续保持。延迟 1 tick 设置（spawn 同步返回后玩家已存在，
  // 立即设潜行亦可，但留 1 tick 让 spawn 完全注册，避免首帧竞态）。
  test.runAtTickTime(1, () => {
    player.isSneaking = true;
  });

  // 轮询断言玩家满血（潜行免疫）。startTick=20 留足落地 + 首次 onEntityWalk 触发窗口
  // （若门控缺失，落地后约 10 tick 首次 hurt 放行 HP 20→19，故等 20+ tick 后仍满血即证免疫）。
  pollUntilSucceed(
    test,
    () => {
      const health = player.getComponent("minecraft:health");
      return (health as any).currentValue === 20;
    },
    {
      startTick: 20,
      interval: 5,
      maxTick: 120,
      onTimeout: () => {
        const health = player.getComponent("minecraft:health");
        test.assert(
          false,
          `sneaking player should be immune to magma damage (HP stay 20), `
            + `but HP=${(health as any).currentValue} (if HP<20 sneaking gate missing `
            + `[isSneaking setter not persisting or isSteppingCarefully not reading sneaking]; `
            + `if HP=20 correct)`,
        );
      },
    },
  );
}

export function registerMagmaTests(): void {
  GameTest.register("BlockBehaviorTests", "magma_damages_entity_on_top", magmaDamagesEntityOnTop)
    .structureName("gametests:glass_pit")
    .maxTicks(200);
  GameTest.register(
    "BlockBehaviorTests",
    "magma_sneaking_player_immune_to_damage",
    magmaSneakingPlayerImmuneToDamage,
  )
    .structureName("gametests:glass_pit")
    .maxTicks(200);
}
