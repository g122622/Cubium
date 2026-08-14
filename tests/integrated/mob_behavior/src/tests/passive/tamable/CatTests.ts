// 猫行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 未驯服猫被手持生鱼的玩家诱惑靠近（wiki tech_猫.txt#驯服：猫会被手持生鳕鱼/生鲑鱼的玩家吸引）。
//
// C++ 链路：CatEntity : TameableEntity（CatEntity.cpp:231-300 registerGoals）：
//   goalSelector 优先级3：CatTemptGoal(extends TemptGoal, speed=TEMPT_SPEED=0.6,
//     诱惑物品=COD||SALMON, scaredByMovement=true)（CatEntity.cpp:252-260）。
//     重写 shouldExecute 限定 !isTamed()（CatEntity.cpp:102-109），未驯服猫才被诱惑。
//     TemptGoal 经 getEntitiesInRange(TEMPT_RANGE=10) + dynamic_cast<Player*> 识别附近持鱼玩家
//     （含 SimulatedPlayer），调 navigator()->moveTo(player) 驱动猫走向玩家。
//     scaredByMovement=true：玩家快速移动会吓跑猫，故玩家须静止站立。
//   注意：未驯服猫同时注册了 CatAvoidPlayerGoal(优先级4, AVOID_DISTANCE=16)（CatEntity.cpp:316-317），
//     会主动避开玩家。但 CatTemptGoal 优先级3 先于 AvoidPlayerGoal(4) 评估，且两者共享 Move mutex：
//     玩家持鱼时 TemptGoal 触发占据 mutex，AvoidPlayerGoal 不执行，猫被诱惑而非逃跑。
//     （对齐 vanilla Cat：持猫食时 TemptGoal 优先于 AvoidEntityGoal。）
//
// 环境选择：mediumglass（12×9×11 走廊，helper y=2 z=5 x=2..10 共 9 格，同 CowTests）。
// 玩家手持生鳕鱼（minecraft:cod，CatTemptGoal lambda 判定 COD 通过），静止站立（不调 moveToLocation）。
// 猫 spawn 在走廊远端距玩家 8 格 < TemptRange 10。
//
// 判定手段：猫被诱惑后从 x=10 朝玩家 x=2 方向移动，断言猫出现在玩家附近体积（x:2..6）即通过。
// 时序：TemptGoal 每 tick 评估 + 寻路。猫 0.6 诱惑速度接近 8 格约需 40-60 tick，maxTicks=1000 留充裕余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_猫.txt#驯服（被手持生鱼的玩家吸引）
function catTemptedByFish(test: Test): void {
  const catType = "cat";

  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // mediumglass 内部空腔 helper y=2（结构内 y=1 空气），地板 helper y=1（结构内 y=0 圆石）。
  // 走廊 helper y=2, z=5, x=2..10（9 格）。玩家与猫分置走廊两端，距离 8 格 < TemptRange 10。
  // 玩家静止站立（不调 moveToLocation）——CatTemptGoal scaredByMovement=true，玩家移动会吓跑猫。
  const farmer = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 5 }, "farmer");

  // 主手持生鳕鱼：setItem 第三参 selectSlot=true 同步选中槽 0（主手），
  // 使 getHeldItem(MainHand) 返回鳕鱼，CatTemptGoal lambda（item==COD||SALMON）判定通过。
  const fish = new ItemStack("minecraft:cod", 1);
  // node_modules 中 @minecraft/server 存在两份（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // ItemStack 类型分裂致 setItem 形参类型不兼容；运行时两者均为同一 Cubium ItemStack opaque，强转绕过编译期。
  farmer.setItem(fish as unknown as Parameters<typeof farmer.setItem>[0], 0, true);

  // 猫 spawn 在走廊远端，距玩家 8 格，在 TemptRange(10) 内。spawn 的猫默认未驯服。
  test.spawn(catType, { x: 10, y: 2, z: 5 });

  // 猫被诱惑后从 x=10 朝玩家 x=2 方向移动。断言猫出现在玩家附近体积（x:2..6）即通过。
  // 体积用 helper 坐标：from(x:2,y:2,z:4) to(x:6,y:3,z:6)，覆盖玩家附近 5×2×3 区域。
  test.succeedWhen(() => {
    assertEntityInVolume(test, catType, 2, 2, 4, 6, 3, 6);
  });
}

// 未驯服猫主动猎杀兔子（wiki tech_猫.txt#行为：猫会攻击兔子；vanilla Cat targetSelector
// 注册 NonTameRandomTargetGoal<Rabbit>，未驯服猫猎杀附近兔子）。
//
// C++ 链路：CatEntity registerGoals：
//   targetSelector 优先级1：NonTamedTargetGoal<RabbitEntity>(this, checkSight=false)
//     （CatEntity.cpp:287-289），未驯服猫在 FOLLOW_RANGE(默认16) 内搜索最近兔子设 attackTarget。
//   goalSelector 优先级8：MeleeAttackGoal(this, 1.0, true)（本次新增，CatEntity.cpp:274-280）。
//     shouldExecute 读 attackTarget，接近到攻击距离内 _attackTarget→hurt(兔, ATTACK_DAMAGE=3.0)。
//     此前 Cat 缺攻击 goal，NonTamedTargetGoal 设了目标却无 goal 执行攻击，猫锁定兔子但不攻击。
// registerAttributes（CatEntity.cpp:321-340）：MAX_HEALTH=10, MOVEMENT_SPEED=0.3,
//   ATTACK_DAMAGE=3.0（本次新增，对齐 vanilla Cat.createAttributes）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）。猫(2,2,3)+兔(4,2,3)，水平距 2 格（近距，避免兔逃跑追不上）。
// 脚下放玻璃支撑。兔 3 血，猫伤害 3，一击致死。
//
// 判定手段：断言兔 HP 下降（<3）或兔已死亡消失。近战确定性命中，伤害 3.0，兔满血 3 → 一击致死消失。
// 时序：NonTamedTargetGoal 选兔(每 tick) + MeleeAttackGoal 接近 2 格 + 攻击冷却 + hurt(3.0)。
// 猫 MeleeAttackGoal 速度 1.0 接近 2 格约需 20-30 tick，maxTicks=1000 留充裕余量吸收兔逃跑 + 非确定性。
// 兔查询用区域限定排除并行测试污染；type 用 "minecraft:rabbit"。
// 注意：NonTamedTargetGoal shouldExecute 限定 !isTamed()，spawn 的猫默认未驯服，满足条件。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_猫.txt#行为（攻击兔子）
function catHuntsRabbit(test: Test): void {
  const catType = "cat";
  const rabbitType = "rabbit";

  // 猫 (2,2,3)、兔 (4,2,3)，水平距 2 格，同处结构 y=2 层。
  // 近距 2 格确保猫选目标后快速命中（兔首击前静止，首击后逃跑但猫 MeleeAttackGoal 速度1.0 > 兔0.3 可追）。
  // 猫脚下 (2,1,3) 放玻璃支撑；兔脚下 (4,1,3) 放玻璃。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });
  test.spawn(catType, { x: 2, y: 2, z: 3 });
  test.spawn(rabbitType, { x: 4, y: 2, z: 3 });

  // 断言兔掉血或死亡：succeedWhen 每 tick 持续检查兔 HP<3 或已消失。
  // 兔 3 血 / 猫 3 伤害 = 一击致死，兔被击后消失，length==0 也算通过（已被猫攻击死亡）。
  test.succeedWhen(() => {
    const rabbits = test.getDimension().getEntities({
      type: "minecraft:rabbit",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    // 兔已死亡消失（被猫打死）——攻击行为生效。
    if (rabbits.length === 0) {
      return;
    }
    const health = rabbits[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "rabbit has no health component");
    test.assert((health as any).currentValue < 3,
      `cat did not attack rabbit, hp=${(health as any).currentValue}`);
  });
}

export function registerCatTests(): void {
  GameTest.register("MobBehaviorTests", "cat_tempted_by_fish", catTemptedByFish)
    .structureName("gametests:mediumglass")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "cat_hunts_rabbit", catHuntsRabbit)
    .structureName("gametests:creeper_pit")
    .maxTicks(1000);
}
