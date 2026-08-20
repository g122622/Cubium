// 哞菇行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 哞菇被闪电劈中红↔棕变种翻转（wiki tech_哞菇.txt#闪电：红色哞菇被雷击变棕色，棕色被雷击变红色）。
//
// C++ 链路：LightningBoltEntity::_damageEntities 对命中范围(±3 XZ)内实体先 hurt(5.0) 再调
// entity->onStruckByLightning()（EffectEntities.cpp:509-511）。MooshroomEntity::onStruckByLightning
// （MooshroomEntity.cpp:366-396）调 setMooshroomType(isRed() ? Brown : Red) 翻转变种 + 播放 convert
// 音效 + 客户端粒子。无难度门控（转化产物仍是哞菇，无和平消失问题，区别于 PigEntity 转化需非 Peaceful）。
// 哞菇 10 血，闪电伤害 5，存活 5 血，转化当 tick 完成，实体仍在（不 remove、不换 typeId）。
//
// 判定手段：读 minecraft:mark_variant 组件（Cubium 绑定 MinecraftModuleFactory.cpp getComponent，
// 对 MooshroomEntity 返回 MarkVariantComponent，readonly value 为 MooshroomType 枚举值 Red=0/Brown=1，
// 与 NBT "Type" 字段一致）。
//
// 对照组设计：生成两只哞菇——实验组 (2,2,3) 与闪电同格被劈（应翻转为棕色 value=1），
// 对照组 (5,2,5) 远离闪电（距实验组 >3 命中范围，未被劈，保持红色 value=0）。
// 双断言组合：实验组棕色 + 对照组红色，精确验证"被劈才翻转"——对照组排除 mark_variant 组件默认恒为
// 某值（如恒 1）的假通过，实验组排除默认棕色的假通过。两组均存活（闪电仅命中实验组范围）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）y=0 grass_block 脚踩，无围墙不影响闪电命中。
// 实验组 (1,2,1) 与闪电同位，闪电 ±3 XZ 命中范围（DAMAGE_RADIUS_XZ=3.0，AABB [pos-3,pos+3]）
// 覆盖；对照组 (5,2,5) 距闪电 (1,2,1) 的 X/Z 均超出 ±3（闪电 X 范围[-2,4]，对照组 x=5>4 出界；
// 闪电 Z 范围[-2,4]，对照组 z=5>4 出界），确保对照组未被劈。
// 时序：闪电首 tick 即 _damageEntities → hurt(5) + onStruckByLightning → setMooshroomType(Brown)。
// 转化当 tick 完成。用 runAtTickTime 延迟若干 tick（待闪电触发 + 实体查询就绪）后断言。
// maxTicks=200 留闪电生成 + 首 tick 触发 + 余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_哞菇.txt#闪电（红↔棕变种翻转）
function mooshroomLightningConvert(test: Test): void {
  const mooshroomType = "mooshroom";
  const lightningType = "lightning_bolt";

  // 实验组 (1,2,1) 与闪电同位被劈；对照组 (5,2,5) 距闪电 X/Z 均超出 ±3 未被劈。
  // 两组脚踩结构内 y=0 grass_block（helper-y=2→结构内 y=1 空气）。
  const struck = test.spawn(mooshroomType, { x: 1, y: 2, z: 1 });
  const control = test.spawn(mooshroomType, { x: 5, y: 2, z: 5 });
  test.spawn(lightningType, { x: 1, y: 2, z: 1 });

  // 断言：实验组翻转为棕色（value=1）+ 对照组保持红色（value=0），两组均存活。
  // 用 spawn 返回的引用读组件——变种是哞菇自身状态，不依赖坐标查询，引用稳定。
  // runAtTickTime(20) 等待闪电首 tick 触发 + 实体查询就绪后断言并 succeed。
  test.runAtTickTime(20, () => {
    test.assertEntityPresentInArea(mooshroomType, true);

    const struckVariant = struck.getComponent("minecraft:mark_variant");
    test.assert(struckVariant !== undefined, "struck mooshroom has no mark_variant component");
    test.assert((struckVariant as any).value === 1,
      `struck mooshroom did not convert to brown, value=${(struckVariant as any).value}`);

    const controlVariant = control.getComponent("minecraft:mark_variant");
    test.assert(controlVariant !== undefined, "control mooshroom has no mark_variant component");
    test.assert((controlVariant as any).value === 0,
      `control mooshroom was not red, value=${(controlVariant as any).value}`);

    test.succeed();
  });
}

// 两只哞菇各喂小麦后进入爱心状态，BreedGoal 驱动互相靠近并繁殖出小哞菇
// （wiki tech_哞菇.txt#繁殖：手持小麦右键两只成年哞菇使其进入"爱心模式"，两只哞菇靠近后繁殖出小哞菇，
//   双亲进入 5 分钟繁殖冷却，玩家获得 1-7 经验球；小哞菇皮肤类型由双亲遗传，同色双亲有 1/1024
//   概率变异为另一色，异色双亲随机继承双亲之一）。
//
// 本测试验证基础繁殖链路（不验证皮肤遗传概率）。皮肤遗传的 1/1024 变异概率极低，单次测试无法稳定
// 触发；异色双亲随机继承概率 50%，单次结果非确定。故皮肤遗传不设计确定性断言（留 TODO）。
//
// C++ 链路（对齐 MC Java 1.21.11 Mooshroom + BreedGoal，与 cow_breeds_when_fed_wheat 同构）：
//   1) 玩家主手持小麦 + interactWithEntity(mooshroom) → Player::interactOn → mooshroom.processInitialInteract
//      → MobEntity::interactMob → AnimalEntity::interactMob override（AnimalEntity.cpp:90-141）：
//      MooshroomEntity 未重写 isBreedingItem，继承 CowEntity::isBreedingItem（小麦）命中
//      → 成体 canBreed() → setInLove(player.playerId())。创造模式喂食不消耗小麦，同一根喂两只哞菇。
//   2) BreedGoal::shouldExecute（BreedGoal.cpp:62-74）：isInLove() && findNearbyMate() 非空。
//   3) BreedGoal::tick：navigator.moveTo(配偶) + m_spawnBabyDelay++，达 adjustedTickDelay(60)=30
//      且 distSq<BREED_DISTANCE_SQ=9 时 spawnBaby()。
//   4) MooshroomEntity::spawnBaby（MooshroomEntity.cpp:320-362）：构造 MooshroomEntity 幼体
//      + setChild(true) + 遗传 setMooshroomType；BreedGoal:153 兜底 setTypeId(MOOSHROOM) 保证 getEntities 可查。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。两只哞菇放中心 (4,2,4) 与 (4,2,6) 相距 2 格
//   （distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。哞菇继承牛 MOVEMENT_SPEED=0.2，BreedGoal speed=1.0。
//
// 判定手段：繁殖完成后区域内 mooshroom 数 >=3（原 2 + 幼体 1）。pollUntilSucceed 轮询。
// 时序：喂食 2×（tick 5、10）+ BreedGoal 评估 + 30 tick spawnBabyDelay + 余量。startTick=30，maxTick=700。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_哞菇.txt#繁殖（喂小麦→爱心→繁殖小哞菇+冷却+经验球）
function mooshroomBreedsWhenFedWheat(test: Test): void {
  const mooshroomType = "mooshroom";

  // 两只成年哞菇放中心相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。
  const mooshroom1 = test.spawn(mooshroomType, { x: 4, y: 2, z: 4 });
  const mooshroom2 = test.spawn(mooshroomType, { x: 4, y: 2, z: 6 });

  // 创造玩家持小麦：创造模式喂食不消耗小麦（同一根喂两只哞菇）。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "mooshroomBreeder");
  const wheat = new ItemStack("minecraft:wheat", 1);
  // 两份 @minecraft/server ItemStack 类型分裂（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // setItem 形参类型不兼容；运行时同一 Cubium ItemStack opaque，强转绕过编译期。
  player.setItem(wheat as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 依次喂两只哞菇：interactWithEntity 转发 interactOn → AnimalEntity::interactMob → setInLove。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(mooshroom1);
  });
  test.runAtTickTime(10, () => {
    (player as any).interactWithEntity(mooshroom2);
  });

  // 轮询：繁殖完成后区域内 mooshroom 数 >=3（原 2 + 幼体 1）。
  pollUntilSucceed(test, () => {
    const mooshrooms = test.getDimension().getEntities({
      type: mooshroomType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    return mooshrooms.length >= 3;
  }, {
    startTick: 30,
    interval: 10,
    maxTick: 700,
    onTimeout: () => {
      const mooshrooms = test.getDimension().getEntities({
        type: mooshroomType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      test.assert(false,
        `mooshroom did not breed: mooshroomCount=${mooshrooms.length} (expected >=3 after feeding wheat)`);
    },
  });
}

export function registerMooshroomTests(): void {
  GameTest.register("MobBehaviorTests", "mooshroom_lightning_convert", mooshroomLightningConvert)
    .structureName("gametests:creeper_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "mooshroom_breeds_when_fed_wheat", mooshroomBreedsWhenFedWheat)
    .structureName("gametests:grass_pen")
    .maxTicks(700);
}
