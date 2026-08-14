// 豹猫行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 未信任豹猫被手持生鱼的玩家诱惑靠近（wiki tech_豹猫.txt#驯服：豹猫会被手持生鳕鱼/生鲑鱼的玩家吸引）。
//
// C++ 链路：OcelotEntity : AnimalEntity（OcelotEntity.cpp:243-295 registerGoals）：
//   goalSelector 优先级3：OcelotTemptGoal(extends TemptGoal, speed=TEMPT_SPEED=0.6,
//     诱惑物品=COD||SALMON, scaredByMovement=true)（OcelotEntity.cpp:252-260）。
//     重写 isScaredByPlayerMovement() = TemptGoal::isScaredByPlayerMovement() && !isTrusting()
//     （OcelotEntity.cpp:460-464），未信任豹猫才会被玩家移动吓跑，故玩家须静止站立。
//     TemptGoal 经 getEntitiesInRange(TEMPT_RANGE=10) + dynamic_cast<Player*> 识别附近持鱼玩家
//     （含 SimulatedPlayer），调 navigator()->moveTo(player) 驱动豹猫走向玩家。
//   注意：未信任豹猫同时注册了 OcelotAvoidPlayerGoal(优先级4, 检测距离16)（_setupTrustingAI 添加），
//     会主动躲避玩家。但 OcelotTemptGoal 优先级3 先于 AvoidPlayerGoal(4) 评估，且共享 Move mutex：
//     玩家持鱼时 TemptGoal 触发占据 mutex，AvoidPlayerGoal 不执行，豹猫被诱惑而非逃跑。
//
// 环境选择：mediumglass（12×9×11 走廊，helper y=2 z=5 x=2..10 共 9 格，同 CowTests/CatTests）。
// 玩家手持生鳕鱼（minecraft:cod），静止站立（不调 moveToLocation）——未信任豹猫 scaredByMovement。
// 豹猫 spawn 在走廊远端距玩家 8 格 < TemptRange 10。spawn 的豹猫默认未信任。
//
// 判定手段：豹猫被诱惑后从 x=10 朝玩家 x=2 方向移动，断言豹猫出现在玩家附近体积（x:2..6）即通过。
// 时序：TemptGoal 每 tick 评估 + 寻路。豹猫 0.6 诱惑速度接近 8 格约需 40-60 tick，maxTicks=1000 留充裕余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_豹猫.txt#驯服（被手持生鱼的玩家吸引）
function ocelotTemptedByFish(test: Test): void {
  const ocelotType = "ocelot";

  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // mediumglass 内部空腔 helper y=2（结构内 y=1 空气），地板 helper y=1（结构内 y=0 圆石）。
  // 走廊 helper y=2, z=5, x=2..10（9 格）。玩家与豹猫分置走廊两端，距离 8 格 < TemptRange 10。
  // 玩家静止站立（不调 moveToLocation）——未信任豹猫 scaredByMovement，玩家移动会吓跑豹猫。
  const farmer = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 5 }, "farmer");

  // 主手持生鳕鱼：setItem 第三参 selectSlot=true 同步选中槽 0（主手），
  // 使 getHeldItem(MainHand) 返回鳕鱼，OcelotTemptGoal lambda（item==COD||SALMON）判定通过。
  const fish = new ItemStack("minecraft:cod", 1);
  // node_modules 中 @minecraft/server 存在两份（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // ItemStack 类型分裂致 setItem 形参类型不兼容；运行时两者均为同一 Cubium ItemStack opaque，强转绕过编译期。
  farmer.setItem(fish as unknown as Parameters<typeof farmer.setItem>[0], 0, true);

  // 豹猫 spawn 在走廊远端，距玩家 8 格，在 TemptRange(10) 内。spawn 的豹猫默认未信任。
  test.spawn(ocelotType, { x: 10, y: 2, z: 5 });

  // 豹猫被诱惑后从 x=10 朝玩家 x=2 方向移动。断言豹猫出现在玩家附近体积（x:2..6）即通过。
  // 体积用 helper 坐标：from(x:2,y:2,z:4) to(x:6,y:3,z:6)，覆盖玩家附近 5×2×3 区域。
  test.succeedWhen(() => {
    assertEntityInVolume(test, ocelotType, 2, 2, 4, 6, 3, 6);
  });
}

// 豹猫主动猎杀小鸡（wiki tech_豹猫.txt#行为：豹猫会攻击小鸡；vanilla Ocelot targetSelector
// 注册 NearestAttackableTargetGoal<Chicken>，未信任豹猫猎杀附近小鸡）。
//
// C++ 链路：OcelotEntity registerGoals：
//   targetSelector 优先级1：NearestAttackableTargetGoal<ChickenEntity>(this, checkSight=false,
//     chance=0 每 tick)（OcelotEntity.cpp:282-283），在 FOLLOW_RANGE(默认16) 内搜索最近小鸡设 attackTarget。
//   goalSelector 优先级7：LeapAtTargetGoal(this, 0.3f)（跳跃扑击）。
//   goalSelector 优先级8：OcelotAttackGoal(this)（OcelotEntity.cpp:269，定义于 OcelotEntity.cpp:467-547）。
//     shouldExecute 读 attackTarget，tick 中接近目标 + 距离达标时 attackEntityAsMob→hurt(鸡, 3.0)。
//     攻击冷却 ATTACK_COOLDOWN_TICKS=20，停止距离 STOP_ATTACK_DISTANCE_SQ=225(15格)。
// registerAttributes（OcelotEntity.cpp:297-310）：MAX_HEALTH=10, MOVEMENT_SPEED=0.3,
//   ATTACK_DAMAGE=3.0（已正确 registerAttribute）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）。豹猫(2,2,3)+鸡(4,2,3)，水平距 2 格（近距，避免鸡逃跑追不上）。
// 脚下放玻璃支撑。鸡 4 血，豹猫伤害 3，首击后鸡剩 1 血（不致死），需 2 击。
//
// 判定手段：断言鸡 HP 下降（<4）或鸡已死亡消失。近战确定性命中，伤害 3.0，鸡满血 4 → 首击剩 1。
// 时序：NearestAttackableTargetGoal 选鸡(每 tick) + OcelotAttackGoal 接近 2 格 + 攻击冷却(20tick) + hurt(3.0)。
// 豹猫 OcelotAttackGoal 速度 1.0 接近 2 格约需 20-30 tick，maxTicks=1000 留充裕余量吸收鸡逃跑 + 非确定性。
// 鸡查询用区域限定排除并行测试污染；type 用 "minecraft:chicken"。
// 注意：NearestAttackableTargetGoal<Chicken> 无 isTrusting 门控（豹猫无论信任与否都猎鸡），spawn 默认即可。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_豹猫.txt#行为（攻击小鸡）
function ocelotHuntsChicken(test: Test): void {
  const ocelotType = "ocelot";
  const chickenType = "chicken";

  // 豹猫 (2,2,3)、鸡 (4,2,3)，水平距 2 格，同处结构 y=2 层。
  // 近距 2 格确保豹猫选目标后快速命中（鸡首击前静止，首击后逃跑但豹猫 OcelotAttackGoal 速度1.0 > 鸡0.25 可追）。
  // 豹猫脚下 (2,1,3) 放玻璃支撑；鸡脚下 (4,1,3) 放玻璃。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });
  test.spawn(ocelotType, { x: 2, y: 2, z: 3 });
  test.spawn(chickenType, { x: 4, y: 2, z: 3 });

  // 断言鸡掉血或死亡：succeedWhen 每 tick 持续检查鸡 HP<4 或已消失。
  // 鸡 4 血 / 豹猫 3 伤害 = 首击剩 1 血，二击致死消失。length==0 也算通过（已被豹猫攻击死亡）。
  test.succeedWhen(() => {
    const chickens = test.getDimension().getEntities({
      type: "minecraft:chicken",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    // 鸡已死亡消失（被豹猫打死）——攻击行为生效。
    if (chickens.length === 0) {
      return;
    }
    const health = chickens[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "chicken has no health component");
    test.assert((health as any).currentValue < 4,
      `ocelot did not attack chicken, hp=${(health as any).currentValue}`);
  });
}

export function registerOcelotTests(): void {
  GameTest.register("MobBehaviorTests", "ocelot_tempted_by_fish", ocelotTemptedByFish)
    .structureName("gametests:mediumglass")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "ocelot_hunts_chicken", ocelotHuntsChicken)
    .structureName("gametests:creeper_pit")
    .maxTicks(1000);
}
