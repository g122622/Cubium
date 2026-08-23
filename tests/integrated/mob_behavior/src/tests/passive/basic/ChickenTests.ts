// 鸡行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// mediumglass 结构尺寸 12×9×11（helper 相对坐标 x∈[0,11], y∈[0,8], z∈[0,10]）。
// y=0 为 cobblestone 实心底，y=1..7 为玻璃墙围出的内部 air 空腔，y=8 为玻璃顶框。
// 用于摔落测试的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 12×9×11。
const MED_FROM = { x: 0, y: 0, z: 0 };
const MED_VOLUME = { x: 12, y: 9, z: 11 };

// 鸡下蛋：成年鸡每隔 6000-12000 tick（5-10 分钟）下 1 个鸡蛋，鸡蛋以掉落物实体（minecraft:item，
// 持 EGG 物品）形式 spawn 在鸡身旁。ChickenEntity::tick 内 eggTimer 每 tick 递减，到 0 时
// spawn ItemEntity(EGG) + 播放音效 + 重置计时器（仅成年、非鸡骑士）。
// 本测试 spawn 成年鸡，断言结构内出现 item 掉落物实体即通过。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鸡.txt#下蛋
function chickenLayEgg(test: Test): void {
  const chickenType = "chicken";

  // 结构 grass_pen（9×5×9 开放玻璃围栏 + 满铺草地，y=0 草地 helper-y=1，y=1 空气腔 helper-y=2）。
  // spawn 2 只成年鸡分散站位，任一只下蛋即通过（提高触发概率，缩短期望等待时间）。
  test.spawn(chickenType, { x: 3, y: 2, z: 3 });
  test.spawn(chickenType, { x: 5, y: 2, z: 5 });

  // eggTimer 初值 6000-12000 随机。2 只鸡取最小值期望约 3000-6000 tick 首次下蛋，最坏 12000 tick。
  // maxTicks=13000 留余量。下蛋 spawn 的 item 掉落物实体类型="item"（minecraft:item）。
  // 用 assertEntityInVolume（基于 getEntities，指定 worldLocation 体积）覆盖整个 grass_pen 内腔，
  // 断言 item 实体出现。item 掉落在草地（helper y=1）上，鸡在 y=2，体积覆盖 y=1..4 全内腔。
  test.succeedWhen(() => {
    assertEntityInVolume(test, "item", 1, 1, 1, 7, 4, 7);
  });
}

// 两只鸡各喂小麦种子后进入爱心状态，BreedGoal 驱动互相靠近并繁殖出小鸡
// （wiki tech_鸡.txt#繁殖：手持任意种子右键两只成年鸡使其进入"爱心模式"，两只鸡靠近后繁殖出小鸡，
//   双亲进入 5 分钟繁殖冷却，玩家获得 1-7 经验球）。
//
// C++ 链路（对齐 MC Java 1.21.11 Chicken + BreedGoal，与 cow_breeds_when_fed_wheat 同构）：
//   1) 玩家主手持小麦种子 + interactWithEntity(chicken) → Player::interactOn → chicken.processInitialInteract
//      → MobEntity::interactMob → AnimalEntity::interactMob override（AnimalEntity.cpp:90-141）：
//      ChickenEntity::isBreedingItem(小麦种子) 命中（ChickenEntity.cpp:107-114 item==Items::WHEAT_SEEDS
//      ||PUMPKIN_SEEDS||MELON_SEEDS||BEETROOT_SEEDS）→ 成体 canBreed() → setInLove(player.playerId())。
//      创造模式喂食不消耗种子，同一根种子喂两只鸡。
//   2) BreedGoal::shouldExecute（BreedGoal.cpp:62-74）：isInLove() && findNearbyMate() 非空。
//   3) BreedGoal::tick：navigator.moveTo(配偶) + m_spawnBabyDelay++，达 adjustedTickDelay(60)=30
//      且 distSq<BREED_DISTANCE_SQ=9 时 spawnBaby()。
//   4) ChickenEntity::spawnBaby（ChickenEntity.cpp:121-139）：构造 ChickenEntity 幼体 + setChild(true)；
//      BreedGoal:153 兜底 setTypeId(CHICKEN) 保证 getEntities 可查。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。两只鸡放中心 (4,2,4) 与 (4,2,6) 相距 2 格
//   （distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。鸡 MOVEMENT_SPEED=0.25，BreedGoal speed=1.0。
//
// 判定手段：繁殖完成后区域内 chicken 数 >=3（原 2 + 幼体 1）。pollUntilSucceed 轮询。
// 时序：喂食 2×（tick 5、10）+ BreedGoal 评估 + 30 tick spawnBabyDelay + 余量。startTick=30，maxTick=700。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鸡.txt#繁殖（喂种子→爱心→繁殖小鸡+冷却+经验球）
function chickenBreedsWhenFedSeeds(test: Test): void {
  const chickenType = "chicken";

  // 两只成年鸡放中心相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。
  const chicken1 = test.spawn(chickenType, { x: 4, y: 2, z: 4 });
  const chicken2 = test.spawn(chickenType, { x: 4, y: 2, z: 6 });

  // 创造玩家持小麦种子：创造模式喂食不消耗种子（同一根喂两只鸡）。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "chickenBreeder");
  const seeds = new ItemStack("minecraft:wheat_seeds", 1);
  // 两份 @minecraft/server ItemStack 类型分裂（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // setItem 形参类型不兼容；运行时同一 Cubium ItemStack opaque，强转绕过编译期。
  player.setItem(seeds as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 依次喂两只鸡：interactWithEntity 转发 interactOn → AnimalEntity::interactMob → setInLove。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(chicken1);
  });
  test.runAtTickTime(10, () => {
    (player as any).interactWithEntity(chicken2);
  });

  // 轮询：繁殖完成后区域内 chicken 数 >=3（原 2 + 幼体 1）。
  pollUntilSucceed(test, () => {
    const chickens = test.getDimension().getEntities({
      type: chickenType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    return chickens.length >= 3;
  }, {
    startTick: 30,
    interval: 10,
    maxTick: 700,
    onTimeout: () => {
      const chickens = test.getDimension().getEntities({
        type: chickenType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      test.assert(false,
        `chicken did not breed: chickenCount=${chickens.length} (expected >=3 after feeding seeds)`);
    },
  });
}

// 鸡免疫摔落伤害（wiki tech_鸡.txt#行为：鸡不会受到摔落伤害；对齐 vanilla 鸡是
// EntityTypeTags.FALL_DAMAGE_IMMUNE 成员）。
//
// C++ 链路（对齐 vanilla LivingEntity.calculateFallDamage:1755 + Entity.isInvulnerableToBase:2922）：
//   Entity::updateFallDistance 着地且 m_fallDistance>0 时调 _handleLandingOnBlock →
//   Block::onFallenUpon（Block.cpp:302 默认实现 multiplier=1.0）→ entity.causeFallDamage(dist,1.0,fall())
//   → LivingEntity::causeFallDamage（LivingEntity.cpp:1439）开头查 EntityTypeTags::FALL_DAMAGE_IMMUNE
//   标签含 chicken → return 不 hurt（对齐 vanilla calculateFallDamage 标签免疫返 0）。
//   此外 LivingEntity::isInvulnerableTo（IS_FALL && FALL_DAMAGE_IMMUNE）作纵深防御。
//
// 此前缺陷：Cubium FALL_DAMAGE_IMMUNE 标签无任何运行时查询（EntityTypeTags.cpp 旧注释误称"由各实体
//   causeFallDamage override 实现"，但 chicken/magma_cube/iron_golem 等均无 override），鸡从高处摔下
//   会受摔落伤害——与 vanilla 相反。本次修复在 LivingEntity::causeFallDamage + isInvulnerableTo 补标签查询。
//
// 落差设计：复用 mediumglass 玻璃管范式（同 SlimeBlockTests）。石头放 (6,1,6)（y=1 空腔底层，下方 y=0
//   cobble 实心支撑），鸡 spawn (6,7,6)（y=7 空腔顶层），1×1 玻璃管围 (6,*,6) y=2..7 垂直路径防 AI 乱跑。
//   落差 = 7 - 2 = 5 格（鸡脚 y=7.0 落到石头顶面 y=2.0），fallDistance≈5。普通方块伤害 (5-3)*1=2，
//   鸡 HP=4 若不免疫会 4→2；鸡免疫则保持 4。落差 5 > 3 确保普通方块确会受伤（见对照测试
//   pig_takes_fall_damage），使免疫判定有意义。
//
// 判定手段：pollUntilSucceed 延迟 startTick=30 检查（落地约 spawn 后 11+ tick，startTick=30 确保已落地
//   结算伤害）。鸡免疫则落地后 HP 仍 4，首检即满足 succeed。若未免疫鸡会 4→2 <4，不满足→超时 FAIL。
//   不能用 succeedWhen（首 tick 鸡还在下落 HP=4 满血会假通过——未落地就因 HP==4 通过）。
//   落地是确定性物理时序（重力 + AABB，零随机），非 flaky。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鸡.txt#行为（鸡不受摔落伤害）
// Ref: LivingEntity.cpp:1439（causeFallDamage FALL_DAMAGE_IMMUNE 标签免疫）/ :1005（isInvulnerableTo IS_FALL 免疫）
function chickenImmuneToFallDamage(test: Test): void {
  const chickenType = "chicken";

  // (6,1,6) 放石头（普通方块，走 Block::onFallenUpon 默认 multiplier=1.0 摔落伤害）。
  test.setBlockType("minecraft:stone", { x: 6, y: 1, z: 6 });

  // 1×1 玻璃管：围 (6,*,6) 垂直路径 y=2..7 四周 glass，限制鸡只能垂直下落防 AI 乱跑。
  // 鸡碰撞箱 0.5×0.7，1×1 管内空间充足。顶部 y=8 已是 mediumglass 玻璃顶封顶。
  for (const y of [2, 3, 4, 5, 6, 7]) {
    test.setBlockType("minecraft:glass", { x: 5, y, z: 6 });
    test.setBlockType("minecraft:glass", { x: 7, y, z: 6 });
    test.setBlockType("minecraft:glass", { x: 6, y, z: 5 });
    test.setBlockType("minecraft:glass", { x: 6, y, z: 7 });
  }

  // 鸡 spawn 于 (6,7,6)（石头正上方 5 格），自由落体到石头顶面，落差 5 格。
  test.spawn(chickenType, { x: 6, y: 7, z: 6 });

  // 轮询断言鸡免疫摔落：落地后 HP 仍 4（满血）。startTick=30 确保已落地结算伤害。
  // 鸡免疫则 HP=4 满足；未免疫则 4→2 <4 不满足→超时 FAIL（onTimeout 打印实际 HP 诊断）。
  pollUntilSucceed(test, () => {
    const chickens = test.getDimension().getEntities({
      type: chickenType,
      location: test.worldLocation(MED_FROM),
      volume: MED_VOLUME,
    });
    if (chickens.length === 0) return false;
    const health = chickens[0].getComponent("minecraft:health");
    if (health === undefined) return false;
    return (health as any).currentValue === 4;
  }, {
    startTick: 30,
    interval: 5,
    maxTick: 120,
    onTimeout: () => {
      const chickens = test.getDimension().getEntities({
        type: chickenType,
        location: test.worldLocation(MED_FROM),
        volume: MED_VOLUME,
      });
      const hp = chickens.length > 0
        ? (chickens[0].getComponent("minecraft:health") as any)?.currentValue
        : "chicken disappeared";
      test.assert(false,
        `chicken took fall damage (should be FALL_DAMAGE_IMMUNE), hp=${hp} (expected 4)`);
    },
  });
}

// 对照测试：猪从同样高度落到石头承受摔落伤害（血量下降）。
// 验证落差 5 格确会造成伤害，使鸡免疫判定有意义（非"落差不足本就不受伤"的假阳性）。
// 猪不在 FALL_DAMAGE_IMMUNE 标签，走 LivingEntity::causeFallDamage 正常扣血 (5-3)*1=2，猪 10→8。
// 与 chickenImmuneToFallDamage 配对：鸡免疫 + 猪受伤，交叉验证 FALL_DAMAGE_IMMUNE 标签门控正确。
// 复用 mediumglass 玻璃管范式（同 SlimeBlockTests stone_block_deals_fall_damage）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鸡.txt#行为（普通生物受摔落伤害对照）
function pigTakesFallDamage(test: Test): void {
  const pigType = "pig";

  // (6,1,6) 放石头（普通方块，multiplier=1.0 摔落伤害）。猪不在 FALL_DAMAGE_IMMUNE 标签。
  test.setBlockType("minecraft:stone", { x: 6, y: 1, z: 6 });

  // 1×1 玻璃管围 (6,*,6) y=2..7（猪碰撞箱 0.9×0.9，1×1 管内空间充足）。
  for (const y of [2, 3, 4, 5, 6, 7]) {
    test.setBlockType("minecraft:glass", { x: 5, y, z: 6 });
    test.setBlockType("minecraft:glass", { x: 7, y, z: 6 });
    test.setBlockType("minecraft:glass", { x: 6, y, z: 5 });
    test.setBlockType("minecraft:glass", { x: 6, y, z: 7 });
  }

  // 猪 spawn 于 (6,7,6)，自由落体到石头顶面，落差 5 格，承受 (5-3)*1=2 摔落伤害。
  test.spawn(pigType, { x: 6, y: 7, z: 6 });

  // 轮询断言猪承受摔落伤害：落地后 HP<10。startTick=30 确保已落地结算伤害。
  // 猪受伤 10→8 <10 满足 succeed；若摔落伤害机制失效猪保持 10 →超时 FAIL（暴露假性免疫）。
  pollUntilSucceed(test, () => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(MED_FROM),
      volume: MED_VOLUME,
    });
    if (pigs.length === 0) return false;
    const health = pigs[0].getComponent("minecraft:health");
    if (health === undefined) return false;
    return (health as any).currentValue < 10;
  }, {
    startTick: 30,
    interval: 5,
    maxTick: 120,
    onTimeout: () => {
      const pigs = test.getDimension().getEntities({
        type: pigType,
        location: test.worldLocation(MED_FROM),
        volume: MED_VOLUME,
      });
      const hp = pigs.length > 0
        ? (pigs[0].getComponent("minecraft:health") as any)?.currentValue
        : "pig disappeared";
      test.assert(false,
        `pig did not take fall damage (fall damage mechanism broken), hp=${hp} (expected <10)`);
    },
  });
}

export function registerChickenTests(): void {
  GameTest.register("MobBehaviorTests", "chicken_lay_egg", chickenLayEgg)
    .structureName("gametests:grass_pen")
    .maxTicks(13000);

  GameTest.register("MobBehaviorTests", "chicken_breeds_when_fed_seeds", chickenBreedsWhenFedSeeds)
    .structureName("gametests:grass_pen")
    .maxTicks(700);

  GameTest.register("MobBehaviorTests", "chicken_immune_to_fall_damage", chickenImmuneToFallDamage)
    .structureName("gametests:mediumglass")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "pig_takes_fall_damage", pigTakesFallDamage)
    .structureName("gametests:mediumglass")
    .maxTicks(200);
}
