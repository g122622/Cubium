// 幼年动物（FollowParentGoal + spawn_baby 事件）行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 幼年牛跟随成年母牛（FollowParentGoal 触发）
// （wiki tech_牛.txt#行为：幼年动物会跟随附近的成年同类父母）。
//
// C++ 链路：
//   1) test.spawn("cow<minecraft:spawn_baby>", pos) 经 GameTestHelper::spawnEntity 解析 <spawnEvent> 后缀
//      → applySpawnEvent 派发 minecraft:spawn_baby 事件 → dynamic_cast<AgeableEntity*> → setChild(true)
//      （AgeableEntity.cpp:89-92，setGrowingAge(-24000) 幼体）。
//   2) CowEntity::registerGoals 优先级4注册 FollowParentGoal(this, 1.1)（CowEntity.cpp:158）。
//   3) FollowParentGoal::shouldExecute（FollowParentGoal.cpp:43-61）：
//      - getGrowingAge() < 0（幼体）通过
//      - findParent() 在 FOLLOW_PARENT_SEARCH_RANGE=8.0 格内搜索成年**同类**（entityType() 相同）
//      - distanceSq >= FOLLOW_PARENT_MIN_DISTANCE_SQ=9.0（距父母 >3 格）才触发
//   4) FollowParentGoal::tick（FollowParentGoal.cpp:95-110）：每 adjustedTickDelay(10)=5 tick 调
//      navigator->moveTo(parent, 1.1) 驱动小牛向母牛移动。
//
// 同类过滤修复：findParent 的 predicate 此前仅检查成年未检查同类，导致小牛会跟随成年羊/猪
// （对齐缺口，与 vanilla FollowParentGoal.getEntitiesOfClass(getClass()) 不符）。已修复为
// entityType() 同类过滤（与 AnimalEntity::canMateWith 同范式）。本测试验证小牛跟随母牛（同类）。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。
//   - 玻璃墙把牛限制在内部 7×3×7 空气腔，FollowParentGoal moveTo 不会让牛跑出查询区域。
//   - 不需 night batch/skyAccess：牛是被动生物不燃不刷怪干扰。
//
// 几何：小牛 (2,2,4) + 母牛 (6,2,4) 距 4 格（distSq=16 > MIN 9 触发跟随，< SEARCH 64 在搜索范围内）。
//   小牛速度 0.2×1.1=0.22 格/tick，向母牛移动 4 格约需 20-40 tick 靠近。
//
// 判定手段：FollowParentGoal 触发后小牛向母牛移动，距离从 4 格减小到 <2 格（靠近）。
//   用 getEntities 区域限定取小牛与母牛世界坐标算水平距离，pollUntilSucceed 轮询距离 <2 格。
//   小牛 RandomWalkingGoal 优先级5 低于 FollowParentGoal 优先级4，FollowParent 触发时抢占，
//   整体位移趋势朝母牛，距离稳定减小。
//
// 时序：spawn_baby 注册 + FollowParentGoal shouldExecute + moveTo 靠近。startTick=30 留 spawn+
// 选父母时间，maxTick=400 留充足余量吸收小牛慢速移动的非确定性。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_牛.txt#行为（幼年动物跟随父母）
// Ref: FollowParentGoal.cpp findParent（同类过滤，对齐 vanilla getEntitiesOfClass(getClass)）
function babyCowFollowsMother(test: Test): void {
    const cowType = "cow";

    // 母牛 (6,2,4) + 小牛 (2,2,4) 距 4 格。脚下 y=1 grass_block 支撑防下落。
    test.spawn(cowType, { x: 6, y: 2, z: 4 });
    // spawn_baby 事件经 applySpawnEvent → setChild(true) 生成幼年牛。
    test.spawn(`${cowType}<minecraft:spawn_baby>`, { x: 2, y: 2, z: 4 });

    // 母牛世界坐标（helper (6,2,4) 经 worldLocation 转换）。
    const motherWorld = test.worldLocation({ x: 6, y: 2, z: 4 });

    // 轮询：小牛跟随母牛靠近，水平距离 <3.5 格（初始 4 格）。
    // 阈值 3.5 的依据：FollowParentGoal::shouldExecute 要求 distSq >= FOLLOW_PARENT_MIN_DISTANCE_SQ=9.0
    // （距父母 >3 格）才触发跟随，距 <3 格时 shouldExecute 返回 false 停止跟随。故小牛会靠近到
    // ~3 格（MIN_DISTANCE 门槛）就停下，不会到 2 格内。判定 <3.5 格（明显从初始 4 格靠近）即证明
    // FollowParentGoal 触发驱动了位移。判定 <2 格会因 MIN_DISTANCE 门槛永远不满足而假失败。
    // 区域限定用 PEN（grass_pen 9×5×9）排除并行测试污染。
    pollUntilSucceed(test, () => {
        const cows = test.getDimension().getEntities({
            type: cowType,
            location: test.worldLocation(PEN_FROM),
            volume: PEN_VOLUME,
        });
        if (cows.length < 2) return false;
        // 找出小牛：取距母牛最远的牛作为小牛候选（母牛在 (6,2,4) 不动，小牛从 (2,2,4) 靠近）。
        let baby = cows[0];
        let maxDist = -1;
        for (const c of cows) {
            const dx = c.location.x - motherWorld.x;
            const dz = c.location.z - motherWorld.z;
            const d = dx * dx + dz * dz;
            if (d > maxDist) {
                maxDist = d;
                baby = c;
            }
        }
        const dx = baby.location.x - motherWorld.x;
        const dz = baby.location.z - motherWorld.z;
        return dx * dx + dz * dz < 3.5 * 3.5; // 距母牛 <3.5 格（跟随靠近到 MIN_DISTANCE 门槛附近）
    }, {
        startTick: 30,
        interval: 10,
        maxTick: 400,
        onTimeout: () => {
            const cows = test.getDimension().getEntities({
                type: cowType,
                location: test.worldLocation(PEN_FROM),
                volume: PEN_VOLUME,
            });
            // 超时时打印小牛与母牛距离帮助诊断。
            let babyDist = -1;
            for (const c of cows) {
                const dx = c.location.x - motherWorld.x;
                const dz = c.location.z - motherWorld.z;
                const d = Math.sqrt(dx * dx + dz * dz);
                if (d > babyDist || babyDist < 0) babyDist = d;
            }
            test.assert(false,
                `baby cow did not follow mother: cowCount=${cows.length} maxBabyDist=${babyDist?.toFixed(2)} (expected <3.5)`);
        },
    });
}

export function registerBabyCowTests(): void {
    GameTest.register("MobBehaviorTests", "baby_cow_follows_mother", babyCowFollowsMother)
        .structureName("gametests:grass_pen")
        .maxTicks(500);
}
