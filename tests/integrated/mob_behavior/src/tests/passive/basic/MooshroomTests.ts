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

// 玩家主手槽（slot 0）物品数量。
function getMainHandAmount(player: any): number {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.amount ?? 0;
}

// 遍历玩家背包所有槽位，统计指定 typeId 物品的总数量。
// 用于蘑菇煲（maxStackSize=1）取汤后落非主手槽的断言——不能只查 slot 0。
function countItemInInventory(player: any, typeId: string): number {
  const inv = player.getComponent("minecraft:inventory") as any;
  const container = inv?.container;
  if (!container) return 0;
  // EntityInventoryComponent 绑定 container.size（PlayerInventory 41 槽：0..40）。
  const size = container.size ?? 41;
  let total = 0;
  for (let i = 0; i < size; i++) {
    const stack = container.getItem?.(i) as any;
    if (stack && stack.typeId === typeId) {
      total += stack.amount ?? 0;
    }
  }
  return total;
}

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

// 玩家持空碗右键成年哞菇获得蘑菇煲（wiki tech_哞菇.txt#哞菇与蘑菇煲：手持碗右键哞菇获得蘑菇煲，
// 消耗1空碗，哞菇不变）。
//
// C++ 链路（对齐 Java 1.21.11 Mooshroom.getInteractionItem / MushroomStew，已逐段核查）：
//   1) 玩家持空碗 interactWithEntity(哞菇) → Player::interactOn → MobEntity::processInitialInteract
//      → MooshroomEntity::interactMob（MooshroomEntity.cpp:156）分支1 `item == Items::BOWL && !isChild()`
//      （:162）命中。
//   2) 无迷之炖菜效果 → stewStack = ItemStack(MUSHROOM_STEW, 1)（:194）。
//   3) 非创造模式 heldItem.shrink(1) 消耗空碗（:200-202，heldItem 是 player.getHeldItem(hand) 权威手持
//      引用，与 [[nametag-consumption]] / [[saddle-consumption]] 修复范式一致，无拷贝不回写 bug）。
//   4) player.inventory().add(stewStack) 添加蘑菇煲到背包（:205），背包满 remaining>0 走
//      ItemDropHelper::spawnItemEntity 掉落（:206-211）。
//   5) 返回 Success，Player::interactOn 第3步直接 return（不走第4步 itemInteractionForEntity）。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。Survival 玩家 (1,2,3) 主手持 2 空碗，
// 哞菇 (3,2,3)（距玩家 2 格，interactWithEntity 远程触发无距离门控）。
//
// 判定手段（双重断言）：
//   1. 主手槽（slot 0）空碗数量 2→1（消耗1个空碗）；
//   2. 玩家背包出现 minecraft:mushroom_stew（数量≥1）。
// 蘑菇煲 maxStackSize=1，取汤后落非主手槽（主手仍是碗），故遍历全背包（countItemInInventory）统计，
// 不能只查 slot 0。Survival 模式（创造跳过 shrink 无消耗证据）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_哞菇.txt#哞菇与蘑菇煲（碗取蘑菇煲）
function mooshroomBowlGivesStewTest(test: Test): void {
  const mooshroomType = "mooshroom";
  const stewType = "minecraft:mushroom_stew";

  // 哞菇 (3,2,3)（creeper_pit y=0 grass_block 地板，helper y=2→结构 y=1 空气，脚踩 y=0 grass_block）。
  const mooshroom = test.spawn(mooshroomType, { x: 3, y: 2, z: 3 });
  // Survival 玩家 (1,2,3) 持 2 空碗（slot 0 主手）。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "bowlUser", 0 as any);
  const bowls = new ItemStack("minecraft:bowl", 2);
  player.setItem(bowls as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持空碗 interactWithEntity(哞菇) → interactMob 分支1 → shrink(1) + inventory.add(stew)。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(mooshroom);
  });

  // 轮询双重断言：主手碗数量 2→1 + 背包出现 mushroom_stew（≥1）。
  // 哞菇是 MobEntity::tick looting 循环候选（canPickUpLoot），但默认不拾取（无 wantsToPickUp），
  // 蘑菇煲不会从背包被取走，断言稳定。startTick=6 interact 后 1 tick 即可查。
  pollUntilSucceed(test, () => {
    if (getMainHandAmount(player) !== 1) return false;
    return countItemInInventory(player, stewType) >= 1;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      test.assert(false,
        `mooshroom_bowl_gives_stew: failed: mainHandBowl=${getMainHandAmount(player)} (expected 1) `
        + `stewInInv=${countItemInInventory(player, stewType)} (expected >=1)`);
    },
  });
}

// 玩家持剪刀右键哞菇剪蘑菇变牛（wiki tech_哞菇.txt#哞菇与剪刀：手持剪刀右键哞菇，哞菇变牛，
// 掉落 5 个对应颜色蘑菇）。
//
// C++ 链路（对齐 Java 1.21.11 Mooshroom.shear / ShearsItem，已逐段核查）：
//   1) 玩家持剪刀 interactWithEntity(哞菇) → Player::interactOn → MobEntity::processInitialInteract
//      → MooshroomEntity::interactMob：剪刀非碗、非棕色+花朵，fallthrough 到 CowEntity::interactMob
//      （无 override）→ AnimalEntity::interactMob → MobEntity::interactMob 基类返 Pass。
//   2) processInitialInteract 返 Pass → Player::interactOn 第4步 item->itemInteractionForEntity
//      → ShearsItem::itemInteractionForEntity（ShearsItem.cpp:138）dynamic_cast<IShearable*>(&target)
//      命中（MooshroomEntity 多重继承 entity::IShearable，MooshroomEntity.hpp:68）→ isShearable()=true
//      → shearable->shear(&player)（ShearsItem.cpp:158）。
//   3) MooshroomEntity::shear（MooshroomEntity.cpp:75）：取 RED/BROWN_MUSHROOM 经 BlockItemRegistry 转
//      BlockItem，drops.emplace_back(mushroomItem, 5)（5 个蘑菇单堆叠）→ 构造 CowEntity 复制位置/朝向/
//      血量/自定义名/持久性/无敌 → spawnEntity(cow) + setTypeId(COW) → remove() 移除哞菇。
//   4) ShearsItem 把 drops（1 个 count=5 的 ItemStack）经 ItemDropHelper::spawnItemEntity 生成
//      minecraft:item 掉落实体（5 个蘑菇以单堆叠掉落为 1 个 ItemEntity）+ hurtAndBreak 消耗剪刀 1 耐久
//      （非创造）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）。创造玩家 (1,2,3) 持剪刀，哞菇 (3,2,3)。
// 哞菇是被动生物不燃烧（区别沼骸需 night 避光），无需 night batch。
//
// 判定手段（三重断言）：
//   1. 区域内出现 minecraft:cow ≥1（剪蘑菇变牛成功）；
//   2. 区域内 minecraft:mooshroom ==0（哞菇已 remove 转化）；
//   3. 区域内出现 minecraft:item ≥1（5 个蘑菇以单堆叠掉落为 1 个 item 实体；脚本侧 minecraft:item
//      组件未绑定读不到 itemStack.typeId/amount，故只能断言 ≥1，见 BoggedTests 同款注释）。
// 牛变体继承哞菇位置同 tick 同步生效，startTick=6 剪后 1 tick 即可查。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_哞菇.txt#哞菇与剪刀（剪刀剪哞菇变牛+掉5蘑菇）
function mooshroomShearedByShearsBecomesCowTest(test: Test): void {
  const mooshroomType = "mooshroom";
  const cowType = "minecraft:cow";

  // 哞菇 (3,2,3)（creeper_pit y=0 grass_block 地板）。创造玩家 (1,2,3) 持剪刀。
  const mooshroom = test.spawn(mooshroomType, { x: 3, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "mooshroomShearer");
  const shears = new ItemStack("minecraft:shears", 1);
  player.setItem(shears as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持剪刀 interactWithEntity(哞菇) → ShearsItem::itemInteractionForEntity → shear
  // 变牛 + 掉 5 蘑菇。创造模式不消耗剪刀耐久（hurtAndBreak 跳过）。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(mooshroom);
  });

  // 轮询三重断言：cow≥1 + mooshroom==0 + item≥1。
  // 牛生成在哞菇原位 (3,2,3)，区域限定 PIT_VOLUME 排除并行测试污染。
  pollUntilSucceed(test, () => {
    const cows = test.getDimension().getEntities({
      type: cowType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (cows.length < 1) return false;
    const mooshrooms = test.getDimension().getEntities({
      type: mooshroomType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (mooshrooms.length !== 0) return false;
    const items = test.getDimension().getEntities({
      type: "minecraft:item",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    return items.length >= 1;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const cows = test.getDimension().getEntities({
        type: cowType,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const mooshrooms = test.getDimension().getEntities({
        type: mooshroomType,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const items = test.getDimension().getEntities({
        type: "minecraft:item",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      test.assert(false,
        `mooshroom_sheared_becomes_cow: failed: cowCount=${cows.length} (expected >=1) `
        + `mooshroomCount=${mooshrooms.length} (expected 0) `
        + `itemCount=${items.length} (expected >=1)`);
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

  GameTest.register("MobBehaviorTests", "mooshroom_bowl_gives_stew", mooshroomBowlGivesStewTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);

  GameTest.register("MobBehaviorTests", "mooshroom_sheared_by_shears_becomes_cow", mooshroomShearedByShearsBecomesCowTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
}
