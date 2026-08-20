// 村民（Villager）行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 9, y: 5, z: 9 };

// 村民被僵尸杀死后在 Hard 难度下 100% 感染转化为僵尸村民
// （wiki mob_僵尸村民.txt#生成 + tech_僵尸.txt#感染：僵尸杀死村民时，Normal 难度 50% 概率、
//   Hard 难度 100% 概率将村民转化为僵尸村民，保留村民的职业/类型/等级/经验数据）。
//
// C++ 链路（VillagerEntity::_tryConvertToZombieVillager，对齐 Java 1.21.11 Zombie.killedEntity +
// Zombie.convertVillagerToZombieVillager）：
//   1) 僵尸近战攻击村民至 HP 归零 → LivingEntity::actuallyHurt（LivingEntity.cpp:374）调 die(source)。
//   2) VillagerEntity::die（VillagerEntity.cpp）最前面调 _tryConvertToZombieVillager(cause)：
//      取 cause.getEntity()（攻击者=僵尸），dynamic_cast<ZombieEntity*> 确认是僵尸系生物
//      （覆盖 Zombie/Husk/ZombieVillager，排除 ZombifiedPiglin），DifficultyHelper::getVillagerInfectionChance
//      难度门控（Easy/Peaceful 0%、Normal 50%、Hard 100%），村民自身 random.nextFloat() 判定。
//   3) 满足则创建 ZombieVillagerEntity，复制位置/旋转/VillagerData（职业/类型/等级/经验）/婴儿状态/装备/
//      自定义名称/持久化，finalizeSpawn(Conversion)，spawnEntity 生成，播放 ENTITY_ZOMBIE_INFECT 音效 +
//      ZOMBIE_INFECT_SOUND(1026) 世界事件，清空原村民装备，remove() 移除原村民，return true 跳过正常死亡流程。
//
// 难度选择：Hard（infectionChance=1.0）确定性转化，排除 Normal 50% 概率的非确定性。GameTestServer 默认
// Normal 难度，须 chat("/difficulty hard") 切换。difficulty 是世界级状态跨测试持久化不自动重置，故：
//   - 独占 batch（night 前缀已含 night_infect 语义，沿用 night 批次）串行执行，避免与同批其他依赖 Normal
//     难度的测试并行竞态。
//   - runOnFinish 恢复 normal 防污染后续批次（PASSED/FAILED/TIMEOUT 三态均触发，早于 GameTestHelper 析构）。
//
// 环境选择：grass_pen（9×5×9）+ batch("night") + skyAccess(true)。
//   - night batch：夜晚亡灵不燃，僵尸存活攻击村民。若 day batch 露天僵尸燃烧致死——燃烧致死的伤害源是
//     fire 非 zombie，不触发感染（dynamic_cast<ZombieEntity*> 失败），且僵尸烧死无法攻击村民。
//   - skyAccess(true)：GameTestServer gridStartY=-59 把结构埋在地下 worldgen 石头中，结构上方全是 worldgen
//     方块致内部 skyLight=0 黑暗——NaturalSpawner 会持续自然生成 zombie 污染计数（假阳性）。skyAccess 清空
//     结构上方方块制造露天列 skyLight=15，NaturalSpawner 怪物光照门槛 max<=7 拒绝 → 隔离自然生成。
//     （夜晚 skyLight 仍=15：Cubium getSkyLight 返回原始天空光不减 skyDarkening，NaturalSpawner 拒绝；
//      night batch 仅影响亡灵白天燃烧判定，不影响 NaturalSpawner 光照门槛。）
//   - setupTicks(20)：清空上方方块后光照变更入队 m_lightQueue，需若干世界 tick 重算 skyLight 达 15。
//
// 几何：grass_pen y=0 grass_block 地板，y=1..3 空气腔+玻璃墙，y=4 露天。helper y=2 = 结构 y=1 air 腔。
//
// 核心难点——确保僵尸能杀死村民：
//   村民 MOVEMENT_SPEED=0.5 远快于僵尸 0.23，且村民有 AvoidHostileGoal（优先级 1，12 格内发现僵尸主动逃离）。
//   在开放场地僵尸永远追不上村民，村民也不会被杀死 → 无法触发 die → 无法触发感染转化。
//   故须用方块把村民困死在僵尸近战范围内，村民无法逃离。
//
// 解决方案——1×2 死胡同通道把村民和僵尸困在相邻 1 格：
//   - 村民 (3,2,3) 在死胡同底，僵尸 (4,2,3) 紧邻 1 格。
//   - (2,2,3) glass：死胡同底墙（村民 -x 方向，村民被击退撞此墙停下）。
//   - (5,2,3) glass：通道口（僵尸 +x 方向，防僵尸被击退/随机行走后退离开攻击范围）。
//   - (3,2,2)(3,2,4)(4,2,2)(4,2,4) glass：两侧 ±z 墙，村民无法侧向逃离。
//   - 村民被夹在 (2,2,3) 墙与 (4,2,3) 僵尸之间，±z 两侧是墙，AvoidHostileGoal 的 tryMoveTo 朝远离僵尸
//     方向（-x）但 (2,2,3) 是 glass navigator 走不过去，村民原地不动。僵尸 MeleeAttackGoal 距村民 1 格
//     （distSq=1.0 ≤ getAttackReachSqr=2.04）在攻击范围内，每 20 tick 攻击一次（ATTACK_DAMAGE=3），
//     村民 MAX_HEALTH=20，约 7 次攻击（约 140 tick）杀死，die 触发感染转化。
//   - 封顶 (3,3,3)(4,3,3) stone 遮 canSeeSky 防 daylight cycle 跨入白天时僵尸燃烧致死（燃烧致死伤害源
//     是 fire 非 zombie，不触发感染）。night batch 初始 dayTime=18000，跑数百 tick 后跨白天，封顶双保险。
//
// 攻击范围公式（MeleeAttackGoal::getAttackReachSqr）：(attackerWidth*2)^2 + targetWidth。
//   僵尸 width=0.6，村民 width=0.6：(1.2)^2 + 0.6 = 1.44 + 0.6 = 2.04。距离 1 格 distSq=1.0 ≤ 2.04 ✓。
//
// 判定手段：转化完成后原 villager 消失（remove）+ zombie_villager 出现（通道内）。pollUntilSucceed 轮询：
//   区域内 zombie_villager 数>=1 且 villager 数==0。Hard 难度 100% 转化确定性发生。
//
// 注：转化后 zombie_villager 继承僵尸 AI 会走动，但通道两侧+前后 glass 困住无法漂移出查询区域。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mob_僵尸村民.txt#生成（僵尸杀村民感染转化）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_僵尸.txt#感染（Normal 50%/Hard 100% 概率）
// Ref: VillagerEntity.cpp _tryConvertToZombieVillager（对齐 Java Zombie.killedEntity/convertVillagerToZombieVillager）
// Ref: DifficultyHelper::getVillagerInfectionChance（难度→感染概率映射）
function villagerInfectedToZombieVillagerByZombie(test: Test): void {
    const villagerType = "villager";
    const zombieType = "zombie";
    const zombieVillagerType = "zombie_villager";

    // 1×2 死胡同通道：村民 (3,2,3) + 僵尸 (4,2,3) 紧邻 1 格。
    // (2,2,3) 死胡同底墙 + (5,2,3) 通道口墙：夹住村民与僵尸在相邻 1 格，防任一方向漂移离开攻击范围。
    test.setBlockType("minecraft:glass", { x: 2, y: 2, z: 3 });
    test.setBlockType("minecraft:glass", { x: 5, y: 2, z: 3 });
    // 两侧 ±z 墙：村民无法侧向逃离死胡同。
    test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 2 });
    test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 4 });
    test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 2 });
    test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 4 });
    // 封顶：stone（不透明）遮 canSeeSky，防 daylight cycle 跨入白天时僵尸燃烧致死（燃烧致死不触发感染）。
    // night batch 初始夜晚，但跑数百 tick 后跨白天，封顶双保险。
    test.setBlockType("minecraft:stone", { x: 3, y: 3, z: 3 });
    test.setBlockType("minecraft:stone", { x: 4, y: 3, z: 3 });

    // 村民 (3,2,3) + 僵尸 (4,2,3) 紧邻 1 格，脚下 y=1 grass_block 支撑防下落。
    test.spawn(zombieType, { x: 4, y: 2, z: 3 });
    test.spawn(villagerType, { x: 3, y: 2, z: 3 });

    // 创造玩家执行管理命令（permLevel=2）：切 Hard 难度（infectionChance=1.0 确定性转化）。
    const player = test.spawnSimulatedPlayer({ x: 7, y: 2, z: 7 }, "operator");
    player.chat("/difficulty hard");
    // 恢复 normal 防污染后续批次（difficulty 世界级跨测试持久化不自动重置）。
    test.runOnFinish(() => {
        player.chat("/difficulty normal");
    });

    // 轮询：转化完成后 zombie_villager>=1 且 villager==0。
    // 僵尸选目标+近战杀死村民约 140 tick（7 次 ×20 tick 冷却），转化瞬间发生。startTick=40 留 spawn 注册+
    // 选目标+首攻时间，maxTick=600 留充足余量覆盖多次攻击 + 转化时序。
    pollUntilSucceed(test, () => {
        const zombieVillagers = test.getDimension().getEntities({
            type: zombieVillagerType,
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        const villagers = test.getDimension().getEntities({
            type: villagerType,
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        return zombieVillagers.length >= 1 && villagers.length === 0;
    }, {
        startTick: 40,
        interval: 10,
        maxTick: 600,
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
            const zs = test.getDimension().getEntities({
                type: zombieType,
                location: test.worldLocation(PIT_FROM),
                volume: PIT_VOLUME,
            });
            test.assert(false,
                `villager not infected: zombieVillager=${zvs.length} villager=${vls.length} zombie=${zs.length}`);
        },
    });
}

export function registerVillagerTests(): void {
    GameTest.register("MobBehaviorTests", "villager_infected_to_zombie_villager_by_zombie", villagerInfectedToZombieVillagerByZombie)
        // 独占 batch（night 前缀）串行执行：/difficulty hard 是世界级命令，避免与同批依赖 Normal
        // 难度的测试并行竞态。night batch 同时保证夜晚亡灵不燃。
        .batch("night")
        .structureName("gametests:grass_pen")
        // skyAccess(true)：隔离 NaturalSpawner 自然生成污染（露天 skyLight=15 拒绝怪物生成）。
        .skyAccess(true)
        // setupTicks(20)：清空上方方块后光照重算稳定。
        .setupTicks(20)
        .maxTicks(700);
}
