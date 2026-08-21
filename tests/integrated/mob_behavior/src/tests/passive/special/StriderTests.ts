// 炽足兽行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";
import { fillBlock } from "../../../utils/block/build.js";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const GLASS_PIT_FROM = { x: 0, y: 0, z: 0 };
const GLASS_PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 炽足兽跟随手持诡异菌的玩家（wiki tech_炽足兽.txt#繁殖：炽足兽用诡异菌繁殖，会被持诡异菌的玩家诱惑跟随）。
//
// C++ 链路：StriderEntity::registerGoals 优先级3 注册 TemptGoal(this, 1.4, predicate, false)
// （StriderEntity.cpp:472-490），predicate 匹配 Items::WARPED_FUNGUS 或 WARPED_FUNGUS_ON_A_STICK，
// scaredByMovement=false。TemptGoal 经 getEntitiesInRange + dynamic_cast<Player*> 识别附近持诱惑物玩家
// （含 SimulatedPlayer），调 navigator()->moveTo(player) 驱动炽足兽走向玩家。与 cow_follows_wheat 同款链路，
// 仅诱惑物品不同（牛=小麦，炽足兽=诡异菌）。TemptRange 默认 16（vanilla Strider FOLLOW_RANGE，StriderEntity.cpp:62）。
//
// 环境选择：mediumglass（12×9×11）走廊，与 cow_follows_wheat 同结构。结构放置有 +1 抬升：
// helper-y=N 对应结构内 y=N-1。炽足兽 size(0.9, 1.8)，helper-y=2（结构内 y=1 空气腔）站立，头顶结构内
// y=2 空气（走廊高 ≥3 格足够 1.8 高实体）。无熔岩环境：MoveToLavaGoal（优先级4）shouldExecute 找不到
// 熔岩返回 false，不干扰 TemptGoal（优先级3 更高）。cold 状态（离开熔岩）仅降速不阻断移动。
//
// 判定手段：玩家静止持诡异菌，炽足兽 spawn 在走廊远端（距玩家 8 格 < TemptRange 16），succeedWhen 断言
// 炽足兽出现在玩家附近体积（x:2..6）即被诱惑跟随过来。复刻 cow_follows_wheat 的 assertEntityInVolume。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_炽足兽.txt#繁殖（诡异菌诱惑跟随）
function striderFollowsWarpedFungus(test: Test): void {
  const striderType = "strider";

  // 玩家 spawn 走廊一端 helper (2,2,5)，主手持诡异菌。gameMode 省略走 Cubium 默认创造模式
  // （创造模式不影响 TemptGoal 的手持物品检测，同 cow_follows_wheat）。
  const farmer = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 5 }, "farmer");

  // 主手持诡异菌：setItem 第三参 selectSlot=true 同步选中槽 0（主手），使 getHeldItem(MainHand) 返回
  // 诡异菌，TemptGoal predicate 才能识别诱惑源。
  const fungus = new ItemStack("minecraft:warped_fungus", 1);
  // node_modules 中 @minecraft/server 存在两份（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // ItemStack 类型分裂致 setItem 形参类型不兼容；运行时两者均为同一 Cubium ItemStack opaque，强转绕过编译期。
  farmer.setItem(fungus as unknown as Parameters<typeof farmer.setItem>[0], 0, true);

  // 炽足兽 spawn 走廊远端 helper (10,2,5)，距玩家 8 格 < TemptRange 16。
  test.spawn(striderType, { x: 10, y: 2, z: 5 });

  // 炽足兽被诱惑后从 x=10 朝玩家 x=2 方向移动。断言炽足兽出现在玩家附近体积（x:2..6）即通过。
  // 体积用 helper 坐标：from(2,2,4) to(6,3,6)，覆盖玩家附近 5×2×3 区域（同 cow_follows_wheat）。
  test.succeedWhen(() => {
    assertEntityInVolume(test, striderType, 2, 2, 4, 6, 3, 6);
  });
}

// 炽足兽免疫火焰/熔岩伤害（wiki tech_炽足兽.txt#行为：炽足兽免疫火焰与岩浆伤害，可在熔岩上行走不受伤）。
//
// C++ 链路：炽足兽实体类型注册了 immuneToFire()（VanillaEntities.cpp STRIDER 注册 .immuneToFire()，
// 对齐 Java EntityType.STRIDER.fireImmune()）。Entity::isImmuneToFire() 默认实现查实体类型标志
// （Entity.cpp:2098-2108），返回 true。火焰免疫以 isImmuneToFire() 为权威：
//   - Entity::lavaHurt/lavaIgnite（Entity.cpp:2112/2119）开头 isImmuneToFire() 直接 return，
//     免疫实体不点燃、不受岩浆伤害。
//   - FireBlock::onEntityCollision（FireBlock.cpp:306）首行 isImmuneToFire() return，免疫实体不点燃不受伤。
//   - FireTickSystem（FireTickSystem.cpp:25）isImmuneToFire() 立即 clearFire 跳过 hurt。
// isOnFire() 非虚无法 override，火焰免疫通过类型标志承载，等价于 vanilla Strider.isOnFire(){return false}。
//
// 对照组设计（复刻 magmacube_immune_to_fire 模式）：把 y=0..1 铺成两层 lava，y=2..4 保持 air。
// 实体 spawn 于 y=3 下落浸入 y=1 熔岩层，碰撞箱与 lava 方块重叠触发 LiquidBlock::entityInside →
// lavaIgnite + lavaHurt。两层 lava 必要：单层 y=0 时实体站在 lava 表面碰撞箱不触及 y=0，entityInside
// 不触发（见 MagmaCubeTests 同款注释）。
//   - 炽足兽 isImmuneToFire=true → lavaHurt 跳过，保持满血 20。
//   - 对照猪 isImmuneToFire=false → lavaHurt 造成 4 点伤害/次，掉血或死亡。
// 双断言组合：炽足兽满血（免疫）+ 猪掉血/死亡（机制有效），交叉验证排除"熔岩机制未实现"假性通过。
//
// 判定手段：炽足兽 health.currentValue >= 20（满血免疫）+ 猪 currentValue < 10 或已死亡消失（lavaHurt 生效）。
// 炽足兽 _updateLavaWalking 浮力逻辑使其在熔岩表面漂浮，但碰撞箱底部触及 y=1 熔岩层即触发 lavaHurt，
// 免疫后跳过伤害，与漂浮行为无冲突。
// maxTicks=300：实体下落 + 浸入熔岩 + lavaHurt 每 tick 判定 + 余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_炽足兽.txt#行为（免疫火焰岩浆伤害）
function striderDoesNotBurnInLava(test: Test): void {
  const striderType = "strider";
  const pigType = "pig";

  // y=0..1 铺两层 lava，y=2..4 保持 air。实体 spawn y=3 下落浸入 y=1 熔岩层触发 lavaHurt。
  fillBlock(test, "lava", 0, 0, 0, 6, 1, 6);

  // 炽足兽 (2,3,2)，对照猪 (4,3,4)，间隔足够不互相干扰，都落入 lava 层。
  // 炽足兽 HP=20，免疫火焰应保持 20；猪 HP=10，浸入熔岩应掉血(<10)或死亡。
  test.spawn(striderType, { x: 2, y: 3, z: 2 });
  test.spawn(pigType, { x: 4, y: 3, z: 4 });

  // 断言：炽足兽存活且满血（免疫熔岩）+ 对照猪受伤(<10)或死亡消失（lavaHurt 生效）。
  // 两端同时成立才证明"熔岩伤害机制有效 + 炽足兽免疫"。实体查询用区域限定排除并行测试污染。
  // 炽足兽 MAX_HEALTH=20（StriderEntity::registerAttributes 显式覆盖 AnimalEntity 默认 10 为 20，
  // 对齐 wiki {{hp|20}}），免疫熔岩应保持满血 20。
  test.succeedWhen(() => {
    const striders = test.getDimension().getEntities({
      type: striderType,
      location: test.worldLocation(GLASS_PIT_FROM),
      volume: GLASS_PIT_VOLUME,
    });
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(GLASS_PIT_FROM),
      volume: GLASS_PIT_VOLUME,
    });

    // 炽足兽应存活且满血（免疫熔岩）。
    test.assert(striders.length > 0, "strider died in lava (should be immune)");
    const striderHealth = striders[0].getComponent("minecraft:health");
    test.assert(striderHealth !== undefined, "strider has no health component");
    test.assert((striderHealth as any).currentValue >= 20,
      `strider should be immune to lava, hp=${(striderHealth as any).currentValue}`);

    // 对照猪应受伤（HP<10）或已死亡消失——证明熔岩伤害机制确实生效。
    if (pigs.length > 0) {
      const pigHealth = pigs[0].getComponent("minecraft:health");
      test.assert(pigHealth !== undefined, "pig has no health component");
      test.assert((pigHealth as any).currentValue < 10,
        `pig should take lava damage, hp=${(pigHealth as any).currentValue}`);
    }
    // 猪已死亡（pigs.length===0）也满足"对照实体受伤"——死亡是受伤的极端情形。
  });
}

// 玩家手持鞍右键成年炽足兽装鞍（wiki tech_炽足兽.txt#骑乘：玩家可对成年炽足兽使用鞍使其可被骑乘）。
//
// C++ 链路（对齐 Java 1.21.11 Strider.mobInteract，已逐段核查）：
//   1) 玩家持鞍 interactWithEntity(炽足兽) → Player::interactOn → MobEntity::processInitialInteract
//      → StriderEntity::interactMob（StriderEntity.cpp:208）。鞍非 isBreedingItem，!isFood 命中；
//      hasSaddle()=false（未装鞍）骑乘分支不命中 → 喂食分支 isFood=false 跳过 → 末尾
//      canEquip(heldItem, 0) 命中（鞍可装备槽0）返回 Pass（StriderEntity.cpp:275-277）。
//   2) Player::interactOn 见 interactMob 返 Pass → 第4步走 Item::itemInteractionForEntity
//      → SaddleItem::itemInteractionForEntity（SaddleItem.cpp:76）：dynamic_cast<IEquipable>+<IRideable>
//      通过 + !isChild + !hasSaddle + 鞍槽空 → rideable->setSaddle(true)（StriderEntity::setSaddle
//      置 m_saddled=true）+ setEquipment(0, 鞍)。
//   3) 装鞍后 hasSaddle()=true。
//
// 判定手段：getComponent("minecraft:is_saddled") 存在（非 undefined）即已装鞍。is_saddled 是基岩标准
// 组件 EntityIsSaddledComponent（componentId="minecraft:is_saddled"，"When added, this component
// signifies that this entity is currently saddled"），组件存在即装鞍（无 property，与 is_charged 同款
// 存在性语义）。本次会话补全其脚本绑定（MinecraftModuleFactory.cpp getComponent "minecraft:is_saddled"
// 分支：dynamic_cast<IRideable*>/AbstractHorseEntity + hasSaddle()==true 才返组件）。
//
// 创造模式不消耗鞍（SaddleItem creativeMode 跳过 shrink）。炽足兽不在熔岩也能装鞍（interactMob 持鞍
// 返 Pass 走 SaddleItem，独立于熔岩环境）。用 glass_pit 陆地玻璃坑（无需熔岩），炽足兽 (3,2,3)。
//
// 时序：tick 5 interactWithEntity 触发装鞍（同步链路），pollUntilSucceed 轮询 is_saddled 组件存在。
// startTick=6 留1tick 装鞍链路执行，interval=2，maxTick=40 留余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_炽足兽.txt#骑乘（对成年炽足兽使用鞍装鞍）
function striderSaddledByPlayerTest(test: Test): void {
  const striderType = "minecraft:strider";

  // 炽足兽 (3,2,3)（glass_pit y=0 grass_block 地板，helper y=2→结构 y=1 空气，脚踩 y=0 grass_block）。
  const strider = test.spawn(striderType, { x: 3, y: 2, z: 3 });
  // 创造玩家 (1,2,3) 持鞍（创造模式不消耗鞍）。interactWithEntity 远程触发无距离门控。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "striderSaddler");
  const saddle = new ItemStack("minecraft:saddle", 1);
  player.setItem(saddle as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持鞍 interactWithEntity(炽足兽) → SaddleItem::itemInteractionForEntity → setSaddle(true)。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(strider);
  });

  // 轮询断言：is_saddled 组件存在（装鞍成功）。组件存在即装鞍（无 value 属性，对齐基岩存在性语义）。
  pollUntilSucceed(test, () => {
    const comp = strider.getComponent("minecraft:is_saddled") as any;
    return comp !== undefined;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const comp = strider.getComponent("minecraft:is_saddled") as any;
      test.assert(false,
        `strider not saddled after interactWithEntity (is_saddled=${comp === undefined ? "undefined" : "present"} expected present)`);
    },
  });
}

// 对照：未装鞍炽足兽 is_saddled 组件为 undefined（验证"组件存在即装鞍"语义，排除"组件恒存在"假通过）。
//
// spawn 炽足兽但不装鞍，断言 getComponent("minecraft:is_saddled")===undefined。验证 is_saddled 组件对
// 未装鞍炽足兽返回 undefined（而非恒存在致 striderSaddledByPlayerTest 假通过）。
// maxTick=40 短窗，不装鞍炽足兽保持无鞍状态。
function striderUnsaddledByDefaultTest(test: Test): void {
  const striderType = "minecraft:strider";
  const strider = test.spawn(striderType, { x: 3, y: 2, z: 3 });

  // tick 20 断言未装鞍（is_saddled 组件 undefined）+ succeed。
  test.runAtTickTime(20, () => {
    const comp = strider.getComponent("minecraft:is_saddled") as any;
    test.assert(comp === undefined,
      `strider should be unsaddled by default, is_saddled=${comp === undefined ? "undefined" : "present"} (expected undefined)`);
    test.succeed();
  });
}

export function registerStriderTests(): void {
  GameTest.register("MobBehaviorTests", "strider_follows_warped_fungus", striderFollowsWarpedFungus)
    .structureName("gametests:mediumglass")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "strider_does_not_burn_in_lava", striderDoesNotBurnInLava)
    .structureName("gametests:glass_pit")
    .maxTicks(300);

  GameTest.register("MobBehaviorTests", "strider_saddled_by_player", striderSaddledByPlayerTest)
    .structureName("gametests:glass_pit")
    .maxTicks(80);

  GameTest.register("MobBehaviorTests", "strider_unsaddled_by_default", striderUnsaddledByDefaultTest)
    .structureName("gametests:glass_pit")
    .maxTicks(60);
}
