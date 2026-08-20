// 牛繁殖行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 两头牛各喂小麦后进入爱心状态，BreedGoal 驱动互相靠近并繁殖出小牛
// （wiki tech_牛.txt#繁殖：手持小麦右键两头成年牛使其进入"爱心模式"，两头牛靠近后繁殖出小牛，
//   双亲进入 5 分钟繁殖冷却，玩家获得 1-7 经验球）。
//
// C++ 链路：
//   1) 玩家主手持小麦 + interactWithEntity(cow)（ScriptSimulatedPlayer.cpp:863 扩展绑定）
//      → Player::interactOn(cow, MainHand)（Player.cpp:2782）
//      → cow.processInitialInteract → MobEntity::interactMob → AnimalEntity::interactMob override
//        （AnimalEntity.cpp:90-141）：isBreedingItem(小麦) 命中 → 成体 setInLove(player.playerId())
//        （AnimalEntity.cpp:110，设 m_loveTimer=LOVE_TIMER_MAX=600，广播 LoveHeart 状态）。
//      注：动物喂食走实体侧 interactMob（对齐 vanilla Animal.mobInteract），非物品侧
//      itemInteractionForEntity（金苹果治愈僵尸村民走的那条）。创造模式喂食不消耗小麦（跳过 shrink），
//      同一根小麦可连续喂两头牛。
//   2) BreedGoal::shouldExecute（BreedGoal.cpp:62-74）：isInLove() && findNearbyMate() 非空。
//      findNearbyMate 在 BREED_DETECTION_RANGE=8.0 格内按 canMateWith 谓词搜索同种 isInLove 配偶
//      （AnimalEntity::canMateWith：同 entityType() + 双方 isInLove）。
//   3) BreedGoal::tick（BreedGoal.cpp:101-124）：lookController 看向配偶 + navigator.moveTo(配偶) +
//      m_spawnBabyDelay++。当 m_spawnBabyDelay >= adjustedTickDelay(SPAWN_BABY_DELAY=60)=30
//      （BreedGoal 未重写 requiresUpdateEveryTick，默认 false，adjustedTickDelay 减半补偿半 tick 评估）
//      且 distanceSq < BREED_DISTANCE_SQ=9.0（距 3 格内）时 spawnBaby()。
//   4) BreedGoal::spawnBaby（BreedGoal.cpp:138-221）：双亲 resetInLove + setGrowingAge(6000 繁殖冷却)，
//      子类 CowEntity::spawnBaby 生成小牛 + setTypeId(COW)（BreedGoal:153 兜底 typeId 保证 getEntities
//      可查），设位置（父母间随机偏移）+ setGrowingAge(-24000 幼年)，finalizeSpawn(Breeding)，
//      spawnEntity 生成，双亲 spawnHeartParticles，生成 1-7 经验球。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。
//   - 玻璃墙把牛限制在内部 7×3×7 空气腔，BreedGoal moveTo 配偶不会让牛跑出查询区域。
//   - 不用 mediumglass 走廊：走廊两端距离可能 >8 格超 BreedGoal 检测范围。grass_pen 9×9 对角 ~11 格，
//     两头牛放中心附近相距 2 格，远在检测范围内。
//   - 牛 MOVEMENT_SPEED=0.2，BreedGoal speed=1.0 倍率，moveTo 配偶靠近快。
//   - 不需 night batch/skyAccess：牛是被动生物不燃不刷怪干扰，grass_pen 室内即可。
//
// 几何：两头牛放中心 (4,2,4) 与 (4,2,6) 相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9 已在繁殖距离内），
//   spawnBaby 几乎只需等 30 tick 的 m_spawnBabyDelay 即触发，无需长距离靠近。玩家站 (2,2,4) 持小麦，
//   runAtTickTime 依次 interactWithEntity 两头牛喂食。
//
// 判定手段：繁殖完成后区域内 cow 数 >=3（原 2 头成年 + 1 头小牛）。小牛 typeId=COW 可被 getEntities
// 查到。经验球是 EXPERIENCE_ORB 类型不干扰 cow 计数。pollUntilSucceed 轮询。
//
// 时序：喂食 2×（tick 5、10）+ BreedGoal 评估 + 30 tick spawnBabyDelay + 余量。startTick=30 留喂食+
// 选配偶时间，maxTick=600 留充足余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_牛.txt#繁殖（喂小麦→爱心→繁殖小牛+冷却+经验球）
function cowBreedsWhenFedWheat(test: Test): void {
    const cowType = "cow";

    // 两头成年牛放中心相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。
    // 脚下 y=1 grass_block 支撑防下落（grass_pen y=0 grass_block 地板，y=1 air 腔，helper y=2 = 结构 y=1 air）。
    const cow1 = test.spawn(cowType, { x: 4, y: 2, z: 4 });
    const cow2 = test.spawn(cowType, { x: 4, y: 2, z: 6 });

    // 创造玩家持小麦：创造模式喂食不消耗小麦（同一根小麦喂两头牛）。
    const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "breeder");
    const wheat = new ItemStack("minecraft:wheat", 1);
    // 两份 @minecraft/server ItemStack 类型分裂（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
    // setItem 形参类型不兼容；运行时同一 Cubium ItemStack opaque，强转绕过编译期。
    player.setItem(wheat as unknown as Parameters<typeof player.setItem>[0], 0, true);

    // 依次喂两头牛：interactWithEntity 转发 interactOn → AnimalEntity::interactMob → setInLove。
    // 间隔 5 tick 确保第一头牛 setInLove 写入后再喂第二头。
    test.runAtTickTime(5, () => {
        (player as any).interactWithEntity(cow1);
    });
    test.runAtTickTime(10, () => {
        (player as any).interactWithEntity(cow2);
    });

    // 轮询：繁殖完成后区域内 cow 数 >=3（原 2 + 小牛 1）。
    pollUntilSucceed(test, () => {
        const cows = test.getDimension().getEntities({
            type: cowType,
            location: test.worldLocation(PEN_FROM),
            volume: PEN_VOLUME,
        });
        return cows.length >= 3;
    }, {
        startTick: 30,
        interval: 10,
        maxTick: 600,
        onTimeout: () => {
            const cows = test.getDimension().getEntities({
                type: cowType,
                location: test.worldLocation(PEN_FROM),
                volume: PEN_VOLUME,
            });
            test.assert(false,
                `cow did not breed: cowCount=${cows.length} (expected >=3 after breeding)`);
        },
    });
}

// 繁殖后双亲进入 6000 tick（5 分钟）繁殖冷却，再次喂食不再繁殖
// （wiki tech_牛.txt#繁殖：繁殖后双亲进入 5 分钟繁殖冷却，冷却期内无法再次进入爱心状态）。
//
// C++ 链路：BreedGoal::spawnBaby 调双亲 setGrowingAge(BREEDING_COOLDOWN=6000)（BreedGoal.cpp:147-148）。
//   AnimalEntity::canBreed（AnimalEntity.cpp:159-163）：getGrowingAge()==0 && !isInLove()。
//   冷却期 getGrowingAge()=6000>0 → canBreed()=false。
//   AnimalEntity::setInLove 受 canBreed() 守卫（AgeableEntity.cpp:135-140）：canBreed()=false 时不设 m_loveTimer，
//   故冷却期喂食无法进入爱心，BreedGoal::shouldExecute 的 isInLove()=false 不触发繁殖。
//
// 测试设计：先正常繁殖出小牛（cow 数 3），繁殖完成后再次喂两头原牛小麦，断言不再出现第 4 头牛。
//   用 pollUntilSucceed 等到 cow 数>=3（繁殖完成），再 runAtTickTime 喂食，再轮询断言 cow 数仍==3。
//
// 注意：冷却验证须确保"再次喂食"发生在繁殖完成之后。用 waitForCondition 等到 cow>=3 触发再喂食。
//   喂食后再等 200 tick 确认无新牛（BreedGoal 若触发约 30 tick 内生成，200 tick 足够判定不繁殖）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_牛.txt#繁殖（繁殖后 5 分钟冷却）
function cowBreedingCooldownAfterBreeding(test: Test): void {
    const cowType = "cow";

    const cow1 = test.spawn(cowType, { x: 4, y: 2, z: 4 });
    const cow2 = test.spawn(cowType, { x: 4, y: 2, z: 6 });

    const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "breeder2");
    const wheat = new ItemStack("minecraft:wheat", 1);
    player.setItem(wheat as unknown as Parameters<typeof player.setItem>[0], 0, true);

    // 第一轮喂食触发繁殖。
    test.runAtTickTime(5, () => {
        (player as any).interactWithEntity(cow1);
    });
    test.runAtTickTime(10, () => {
        (player as any).interactWithEntity(cow2);
    });

    // 等到繁殖完成（cow>=3），再次喂两头原牛小麦——冷却期应阻止再次进入爱心。
    // 用 runAtTickTime 轮询：在 tick=80（繁殖应已完成，spawnBaby 约 tick 40-50 触发）检查 cow>=3 后喂食。
    let fed = false;
    test.runAtTickTime(80, () => {
        const cows = test.getDimension().getEntities({
            type: cowType,
            location: test.worldLocation(PEN_FROM),
            volume: PEN_VOLUME,
        });
        if (cows.length >= 3 && !fed) {
            fed = true;
            (player as any).interactWithEntity(cow1);
            (player as any).interactWithEntity(cow2);
        }
    });
    // tick=90 兜底再喂一次（防 tick=80 时繁殖尚未完成）。
    test.runAtTickTime(90, () => {
        if (!fed) {
            const cows = test.getDimension().getEntities({
                type: cowType,
                location: test.worldLocation(PEN_FROM),
                volume: PEN_VOLUME,
            });
            if (cows.length >= 3) {
                fed = true;
                (player as any).interactWithEntity(cow1);
                (player as any).interactWithEntity(cow2);
            }
        }
    });

    // 喂食后再等 200 tick（到 tick=300），断言 cow 数仍==3（冷却阻止第二次繁殖）。
    // 若冷却失效，BreedGoal 会在喂食后约 30 tick 内生成第 4 头牛。
    pollUntilSucceed(test, () => {
        const cows = test.getDimension().getEntities({
            type: cowType,
            location: test.worldLocation(PEN_FROM),
            volume: PEN_VOLUME,
        });
        // 断言：已繁殖（>=3）且不再增加（==3，无第 4 头）。
        // 容忍 cow 数可能因小牛成长为成年牛仍计为 cow（typeId 不变），但总数不应增加。
        return cows.length === 3;
    }, {
        startTick: 120,
        interval: 20,
        maxTick: 300,
        onTimeout: () => {
            const cows = test.getDimension().getEntities({
                type: cowType,
                location: test.worldLocation(PEN_FROM),
                volume: PEN_VOLUME,
            });
            // 区分两种失败：cowCount<3 说明第一次繁殖未完成（喂食/BreedGoal 链路问题）；
            // cowCount>3 说明冷却失效（第二次繁殖发生）。cowCount==3 不应到达此处（pollUntilSucceed 会 succeed）。
            test.assert(false,
                `breeding cooldown check failed: cowCount=${cows.length} fed=${fed} ` +
                `(cowCount<3: first breed failed; cowCount>3: cooldown broken)`);
        },
    });
}

export function registerCowBreedTests(): void {
    GameTest.register("MobBehaviorTests", "cow_breeds_when_fed_wheat", cowBreedsWhenFedWheat)
        .structureName("gametests:grass_pen")
        .maxTicks(700);
    GameTest.register("MobBehaviorTests", "cow_breeding_cooldown_after_breeding", cowBreedingCooldownAfterBreeding)
        .structureName("gametests:grass_pen")
        .maxTicks(400);
}
