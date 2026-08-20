// 海豚行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 海豚受击后反击玩家（wiki tech_海豚.txt#攻击：海豚是中性生物，被攻击后会反击攻击者）。
//
// C++ 链路：DolphinEntity : WaterMobEntity（非 AnimalEntity）。registerGoals（DolphinEntity.cpp:267-320）：
//   targetSelector 优先级1：HurtByTargetGoal(this, true, predicate)——受击后设 attackTarget=攻击者
//     并呼叫附近同类（alertAllies=true，DolphinEntity.cpp:313-319）。谓词只排除 Guardian/ElderGuardian，
//     玩家攻击者不被排除，会反击（HurtByTargetGoal::shouldExecute 读 getLastHurtBy 判 isSuitableTarget，
//     Survival 玩家可通过，Creative/Spectator 被滤掉）。
//   goalSelector 优先级6：MeleeAttackGoal(this, 1.2, true)——speed=1.2，longMemory=true，
//     shouldExecute 读 attackTarget，寻路接近到攻击距离内 _attackTarget 读 ATTACK_DAMAGE(3.0)+hurt(玩家)。
//   注意：海豚未注册 NearestAttackableTargetGoal<Player>（targetSelector 仅 HurtByTargetGoal 一个），
//   故海豚不会主动攻击玩家，仅在受击后才反击——这正是本测试要验证的"受击后敌对攻击玩家"。
// registerAttributes（DolphinEntity.cpp:322-334）：MAX_HEALTH=10.0, ATTACK_DAMAGE=3.0。
//   MeleeAttackGoal::getAttackReachSqr=(attacker.width*2)²+target.width=(0.9*2)²+0.6=3.84，
//   开方约 1.96 格——玩家需在 1.96 格内。命中后 _attackTarget 读 ATTACK_DAMAGE(3.0) 并 hurt 玩家。
//   冷却 ATTACK_COOLDOWN_TICKS=20，经 adjustedTickDelay 减半约 10 tick。
//
// 陆地可行性（关键约束排除）：DolphinEntity : WaterMobEntity，陆地上每 tick 调 updateAirSupply 消耗
//   空气（WaterMobEntity::updateAirSupply，水生反逻辑：水中 setAir(maxAir) 回满，陆地 air-1，
//   air<=-20 时 hurt(drown,2.0)）。海豚 MAX_AIR=4800（DolphinEntity.cpp:99 setAir(4800)），
//   air 4800→0 耗 4800 tick + 0→-20 再 20 tick，首次窒息伤害在第 ~4820 tick。maxTicks=800 远小于此，
//   测试窗口内海豚完全不窒息掉血——窒息不干扰断言，掉血只能来自玩家攻击海豚（不影响海豚存活）。
//   注：海豚 DATA_MOISTNESS_LEVEL_PARAM 默认 2400，但 moistness 离水递减业务联动 TODO 未实现
//   （DolphinEntity.hpp:297），陆地上不因湿润度掉血。窒息是唯一陆地掉血源。
//
// 寻路可行性：海豚 navigator 是 MobEntity 默认 GroundPathNavigation（WalkNodeProcessor），陆地可寻路
//   （未覆盖 _createDefaultNavigator，与美西螈陆地测试同范式，见 AxolotlTests.ts）。
// FindWaterGoal（优先级0，mutex={Move}）shouldExecute 需 16 格内有水，creeper_pit 无水→返回 false，
//   不锁 Move flag，MeleeAttackGoal 可正常运行寻路。RandomSwimmingGoal（优先级4，mutex={Move}）陆地
//   无水 getPathWeight=0 不主动执行，不抢占 MeleeAttackGoal。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，MeleeAttackGoal 寻路通畅 + checkSight 射线不被阻挡。
// 海豚是水生友好生物（非亡灵/怪物），不在阳光下燃烧，白天即可反击（不 batch night）。
// 海豚(2,2,3)+Survival 玩家(5,2,3)，水平距 3 格。海豚脚下 (2,1,3) 放玻璃支撑；玩家脚下 (5,1,3) 放玻璃。
// 玩家 tick 8 后 attackEntity(海豚) 触发 HurtByTargetGoal 反击（attackEntity 不受距离限制，
// 基岩语义 attack can be performed at any distance，见 WolfTests/PolarBearTests 同款注释）。
// 海豚被攻击后设 attackTarget=玩家，MeleeAttackGoal 寻路接近 3 格 + 攻击冷却后 hurt(玩家, 3.0)。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中（无散布），伤害 3.0，玩家满血 20 → 17。
// 玩家在陆地不溺水，HP 掉血只能来自海豚攻击，断言干净（maxTicks=1200 < 玩家陆地无溺水问题，
//   玩家陆地不消耗 air）。
// 时序：玩家攻击(8/16/24/32 多次) + HurtByTargetGoal 设目标 + MeleeAttackGoal 寻路接近 3 格 + 攻击冷却 + hurt(3.0)。
//   海豚 MOVEMENT_SPEED=1.2（属性值，moveController 调制后较快），3 格接近 + 冷却约需 30-60 tick，
//   maxTicks=1200 留充裕余量吸收并行环境 tick 抖动（单跑 800 tick 稳定，并行下寻路/冷却偶发延迟需更长窗口）。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 滤掉不可被攻击/反击）。
// 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海豚.txt#攻击（被攻击后反击攻击者）
function dolphinRetaliatesWhenAttacked(test: Test): void {
  const dolphinType = "dolphin";

  // 海豚 (2,2,3)、Survival 玩家 (5,2,3)，水平距 3 格，同处结构 y=2 层。
  // 海豚脚下 (2,1,3) 放玻璃支撑；玩家脚下 (5,1,3) 放玻璃。
  // creeper_pit 开放坑无围墙，海豚反击寻路通畅。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  const dolphin = test.spawn(dolphinType, { x: 2, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 8/16/24/32 后玩家多次攻击海豚：留 8 tick 让实体完成 spawn 注册 + 首 tick 稳定，之后每隔 8 tick
  // 攻击一次共 4 次。多次攻击确保 HurtByTargetGoal 被触发设 attackTarget=玩家（attackEntity 远程命中，
  // 基岩语义 attack can be performed at any distance，见 WolfTests/PolarBearTests 同款注释）。单次攻击在并行
  // 环境下偶发未及时触发 HurtByTargetGoal（tick 抖动），多次攻击提高可靠性。
  test.runAtTickTime(8, () => {
    player.attackEntity(dolphin);
  });
  test.runAtTickTime(16, () => {
    player.attackEntity(dolphin);
  });
  test.runAtTickTime(24, () => {
    player.attackEntity(dolphin);
  });
  test.runAtTickTime(32, () => {
    player.attackEntity(dolphin);
  });

  // 断言玩家掉血：pollUntilSucceed 轮询玩家 HP<20（海豚反击命中）。
  // 时序：玩家攻击(8/16/24/32) + HurtByTargetGoal 设目标 + MeleeAttackGoal 寻路接近 3 格 + 攻击冷却 + hurt(3.0)。
  // 海豚 1.2 速度接近 3 格约需 30-60 tick，maxTicks 留充裕余量（且远小于海豚陆地窒息线 4820）。
  // 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
  // onTimeout 诊断：超时时打印海豚/玩家位置、距离、HP，定位是"未设目标"还是"寻路不动"还是"未命中"。
  pollUntilSucceed(test, () => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (players.length === 0) return false;
    const health = players[0].getComponent("minecraft:health") as any;
    if (health === undefined) return false;
    return health.currentValue < 20;
  }, {
    maxTick: 400,
    onTimeout: () => {
      const players = test.getDimension().getEntities({
        type: "minecraft:player",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const dolphins = test.getDimension().getEntities({
        type: "minecraft:dolphin",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const playerHp = players.length > 0
        ? (players[0].getComponent("minecraft:health") as any)?.currentValue
        : "gone";
      const dolphinHp = dolphins.length > 0
        ? (dolphins[0].getComponent("minecraft:health") as any)?.currentValue
        : "gone";
      const dist = (players.length > 0 && dolphins.length > 0)
        ? Math.hypot(players[0].location.x - dolphins[0].location.x,
            players[0].location.z - dolphins[0].location.z)
        : -1;
      const dPos = dolphins.length > 0
        ? `(${dolphins[0].location.x.toFixed(1)},${dolphins[0].location.y.toFixed(1)},${dolphins[0].location.z.toFixed(1)})`
        : "gone";
      const pPos = players.length > 0
        ? `(${players[0].location.x.toFixed(1)},${players[0].location.y.toFixed(1)},${players[0].location.z.toFixed(1)})`
        : "gone";
      test.assert(false,
        `dolphin did not retaliate (dolphin=${dolphins.length} hp=${dolphinHp} pos=${dPos}; player=${players.length} hp=${playerHp} pos=${pPos}; dist=${dist.toFixed(2)})`);
    },
  });
}

export function registerDolphinTests(): void {
  GameTest.register("MobBehaviorTests", "dolphin_retaliates_when_attacked", dolphinRetaliatesWhenAttacked)
    .structureName("gametests:creeper_pit")
    .maxTicks(450);
}
