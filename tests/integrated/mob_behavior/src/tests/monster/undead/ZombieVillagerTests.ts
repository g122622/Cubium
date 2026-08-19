// 僵尸村民行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸（9×5×9，helper 相对坐标）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
// 用全尺寸而非更小中心区域：zombie_villager 治愈过程中会走动（ZombieEntity AI 追击玩家），
// finishConverting 在 zombie_villager 走动后的位置生成 villager，villager 生成后也会移动。
// 查询区域须覆盖整个 grass_pen（玻璃墙围住的 9×9 内）才能稳定捕获 villager。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 9, y: 5, z: 9 };

// 僵尸村民在虚弱效果下被喂食金苹果后，经 3600-6000 tick（3-5 分钟）转化为村民
// （wiki mob_僵尸村民.txt#治愈：对僵尸村民施加虚弱效果后用金苹果（普通金苹果或附魔金苹果）右键，
//   僵尸村民开始抖动并发出粒子，3-5 分钟后转化为村民）。
//
// C++ 链路：
//   1) 虚弱效果：脚本 Entity.addEffect("weakness", duration, options)（MinecraftModuleFactory.cpp 绑定）
//      → LivingEntity::addEffect(EffectInstance(Weakness, duration, ...))。对齐基岩官方 Entity.addEffect。
//      此前仅 EffectCommand(/effect) 能施效果且只支持玩家选择器（EntityArgumentType::players()），
//      对非玩家实体（zombie_villager）不可用；脚本 addEffect 绑定补全后解锁实体效果施加。
//   2) 金苹果喂食：SimulatedPlayer.interactWithEntity(zombieVillager)（ScriptSimulatedPlayer.cpp 绑定）
//      → Player::interactOn(target, Hand::MainHand)（Player.cpp:2771）。
//      interactOn 流程：① processInitialInteract（zombie_villager 无交易/容器，返 Pass）；
//      ② 取主手物品调 Item::itemInteractionForEntity。
//   3) GoldenAppleItem::itemInteractionForEntity（GoldenAppleItem.cpp:119）：
//      dynamic_cast<ZombieVillagerEntity*> + isAlive + hasEffect(Weakness) → startConverting(player.uuid(),
//      3600 + rng.nextInt(2401))。使用金苹果瞬间检查虚弱存在即可，startConverting 后虚弱消失不影响转化
//      （转化计时器与虚弱效果解耦）。
//   4) ZombieVillagerEntity::tick（cpp:468）每 tick 递减 m_conversionTime，到 0 调 finishConverting（cpp:263）：
//      创建 minecraft:villager 实体，复制位置/装备/村民职业，spawnEntity 生成村民，remove() 移除原僵尸村民。
//
// 时序：转化 3600-6000 tick（3-5 分钟）。喂食在 tick=5 触发 startConverting，转化完成 tick=5+conversionTime
// （最大 5+6000=6005）。pollUntilSucceed maxTick=7900 + maxTicks=8000 留充足余量覆盖最大转化时间。
//
// 环境选择：grass_pen（9×5×9）+ batch("night") + 玻璃笼困住 zombie_villager + stone 封顶。原因：
//   1. 燃烧中断：僵尸村民是亡灵生物，白天露天燃烧致死中断治愈。night batch 初始 dayTime=18000，
//      但 daylight cycle 每 tick 递增，跑数千 tick（转化需 3600-6000 tick）后跨入白天，露天亡灵燃烧。
//      故笼顶放 stone（不透明）遮 canSeeSky，全程不燃。
//   2. 位置漂移：zombie_villager 继承 ZombieEntity AI 会走动，finishConverting 在走动后位置生成 villager，
//      villager 生成后也移动，致 villager 可能漂移出查询区域（实测 9×5×9 仍有 ~37% 概率移出失败）。
//      故用玻璃笼四面围住 zombie_villager 困在 (3,2,3)，无法走动，villager 生成位置稳定可控。
//   3. 治愈不依赖光照/时间/空间，笼不影响转化机制（仅限制移动）。
//   4. night batch 双保险降低燃烧概率。
//
// 判定手段：转化完成后原 zombie_villager 消失（remove）+ villager 出现（笼内）。pollUntilSucceed 轮询：
//   区域内 villager 数>=1 且 zombie_villager 数==0。两条同时成立证明转化完成。
//   转化是 3600-6000 tick 后确定性发生（无随机失败，仅时间随机），pollUntilSucceed 在 maxTick=7900 内捕获。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mob_僵尸村民.txt#治愈（虚弱+金苹果→村民）
function zombieVillagerCuredByGoldenApple(test: Test): void {
  const zombieVillagerType = "zombie_villager";
  const villagerType = "villager";

  // 建玻璃笼困住 zombie_villager，阻止其在治愈过程中走动。
  // 原因：zombie_villager 继承 ZombieEntity AI（追击玩家/村民），治愈 3600-6000 tick 期间会走动，
  // finishConverting 在 zombie_villager 走动后的位置生成 villager，且 villager 生成后也会移动，
  // 致 villager 可能漂移出查询区域（实测 9×5×9 区域仍有 ~37% 概率 villager 移出）。建笼后
  // zombie_villager 困在 (3,2,3) 1 格空腔无法走动，villager 生成在笼内 (3,2,3) 附近，位置稳定可控。
  // 笼布局：四面 glass 墙 (2,2,3)(4,2,3)(3,2,2)(3,2,4) 围住 (3,2,3)；顶 (3,4,3) stone 封顶遮 canSeeSky；
  // 底 (3,1,3) 保留 grass_block 地板支撑。zombie_villager 困在 1 格空腔（宽 0.6 足够）。
  // player 站笼外 (5,2,3) interactWithEntity（interactOn 不检查距离，脚本侧已解包实体直接调 C++）。
  test.setBlockType("minecraft:glass", { x: 2, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 3 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 2 });
  test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 4 });
  // 封顶：stone（不透明）遮 canSeeSky，避免 daylight cycle 跨入白天时 zombie_villager 燃烧致死中断治愈。
  // night batch 初始 dayTime=18000，但 daylight cycle 每 tick 递增，跑数千 tick 后跨入白天，露天亡灵燃烧。
  test.setBlockType("minecraft:stone", { x: 3, y: 4, z: 3 });

  // 僵尸村民 spawn 于笼内 (3,2,3)（grass_pen y=0 grass_block 地板支撑防下落，y=1 空气腔）。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity 因
  // Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests/DrownedTests 同款注释）。
  const zombieVillager = test.spawn(zombieVillagerType, { x: 3, y: 2, z: 3 });

  // SimulatedPlayer 站笼外 (5,2,3)，距笼内 zombie_villager 2 格。interactWithEntity 不检查距离。
  // 默认创造模式：金苹果治愈链路在创造模式也走 itemInteractionForEntity（创造模式只跳过 shrink 不跳过
  // startConverting），且金苹果不消耗更方便（创造模式恢复物品）。
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "healer");

  // 主手设金苹果：setItem 第三参 selectSlot=true 同步选中槽 0（主手），使 getHeldItem(MainHand) 返回金苹果。
  // 两份 @minecraft/server ItemStack 类型分裂，强转绕过编译期（见 CowTests 同款注释）。
  const goldenApple = new ItemStack("minecraft:golden_apple", 1);
  player.setItem(goldenApple as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 施虚弱：addEffect("weakness", 1200 ticks, { showParticles: false })。
  // duration=1200 tick（60s）远超喂食瞬间，确保 interactWithEntity 时虚弱仍在。
  // showParticles:false 减少粒子干扰（不影响治愈）。
  // addEffect 是 Cubium 扩展绑定（基岩官方 Entity.addEffect），TS 类型无此方法，as any 绕过。
  (zombieVillager as any).addEffect("weakness", 1200, { showParticles: false });

  // 喂食金苹果：interactWithEntity 转发 interactOn → itemInteractionForEntity → startConverting。
  // interactWithEntity 是 Cubium 扩展绑定（基岩官方 SimulatedPlayer.interactWithEntity），as any 绕过。
  // 等待 5 tick 确保虚弱效果写入 effectManager 后再喂食（addEffect 同步写，但留 1 个 tick 余量防边界）。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(zombieVillager);
  });

  // 轮询：转化完成后 villager>=1 且 zombie_villager==0。
  // 3600-6000 tick 转化确定性发生，maxTick=7900 留 spawn+施虚弱+喂食+余量（覆盖最大 6005 完成时间）。
  pollUntilSucceed(test, () => {
    const villagers = test.getDimension().getEntities({
      type: villagerType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    const zombieVillagers = test.getDimension().getEntities({
      type: zombieVillagerType,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    return villagers.length >= 1 && zombieVillagers.length === 0;
  }, {
    maxTick: 7900,
    onTimeout: () => {
      const zvs = test.getDimension().getEntities({
        type: zombieVillagerType,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const vls = test.getDimension().getEntities({
        type: villagerType,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      test.assert(false,
        `zombie_villager not cured: zombieVillager=${zvs.length} villager=${vls.length}`);
    },
  });
}

export function registerZombieVillagerTests(): void {
  GameTest.register("MobBehaviorTests", "zombie_villager_cured_by_golden_apple", zombieVillagerCuredByGoldenApple)
    .batch("night")
    .structureName("gametests:grass_pen")
    .maxTicks(8000);
}
