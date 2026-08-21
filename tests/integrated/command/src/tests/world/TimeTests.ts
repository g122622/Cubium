// /time 命令 GameTest：设置/增加/查询世界时间。
//
// 覆盖 wiki 命令章节核心行为：
//   - /time set <值|day|noon|night|midnight>：设置一天内时间（Ref: wiki commands/time.txt）
//   - /time add <值>：时间向前推进
//   - /time query <daytime|day|gametime>：查询时间
//
// 设计要点：
//   1. TimeCommand 经 server->timeManager().setDayTime/addDayTime 修改世界时间（非占位）。
//      此前脚本侧仅 system.currentTick（游戏总 tick 非 dayTime），无法断言 /time 效果。
//      本次补全 Dimension.getTimeOfDay()（IWorld::dayTimeOfDay % 24000）与 getDayTime()
//      （原始 dayTime）脚本读取绑定，解锁端到端测试。
//   2. dayTimeOfDay 是 % 24000 的一天内时间。/time set <值> 设置后立即同步可读。
//      日夜循环（doDaylightCycle 默认 true）每 tick +1，命令同步执行后立即读差值 0-2 tick，
//      断言用区间 [target, target+50) 容忍流逝（不关 daylight cycle 避免 gamerule 跨测试污染）。
//   3. 具名预设：day=1000, noon=6000, night=13000, midnight=18000（TimeCommand.cpp 常量）。
//   4. /time add <值> 在当前 dayTime 基础上加值，dayTimeOfDay 相应推进（可跨日回绕）。
//   5. /time query daytime 返回命令 i32 结果（dayTimeOfDay），但 Cubium chat 返回值是命令
//      successCount 非 query 值，故 query 不用返回值断言，改用 getTimeOfDay() 直接读。
//   6. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_time.txt（时间命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { asDim } from "../../utils/script/cubiumExtensions.js";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// 容忍日夜循环推进的区间断言：getTimeOfDay 应在 [target, target+50) 内。
// 50 tick 余量覆盖 chat 同步执行延迟 + daylight cycle 流逝，远小于 24000 周期不会误回绕。
function assertTimeNear(test: Test, actual: number, target: number, label: string): void {
    test.assert(
        actual >= target && actual < target + 50,
        `${label}: expected getTimeOfDay in [${target}, ${target + 50}), got ${actual}`,
    );
}

// /time set 1000 设置一天内时间为 1000，断言 getTimeOfDay 在 [1000, 1050)。
// Ref: wiki commands/time.txt（time set <值>）
function timeSetInteger(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/time set 1000");

    assertTimeNear(test, dim.getTimeOfDay(), 1000, "time set 1000");
    test.succeed();
}

// /time set day（=1000）具名预设。
// Ref: wiki commands/time.txt（time set day/noon/night/midnight）
function timeSetDay(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/time set day");

    assertTimeNear(test, dim.getTimeOfDay(), 1000, "time set day");
    test.succeed();
}

// /time set noon（=6000）正午。
function timeSetNoon(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/time set noon");

    assertTimeNear(test, dim.getTimeOfDay(), 6000, "time set noon");
    test.succeed();
}

// /time set night（=13000）日落后。
function timeSetNight(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/time set night");

    assertTimeNear(test, dim.getTimeOfDay(), 13000, "time set night");
    test.succeed();
}

// /time set midnight（=18000）午夜。
function timeSetMidnight(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/time set midnight");

    assertTimeNear(test, dim.getTimeOfDay(), 18000, "time set midnight");
    test.succeed();
}

// /time add <值> 在当前时间基础上推进：先 set 1000 锚定，再 add 500，断言在 [1500, 1550)。
// Ref: wiki commands/time.txt（time add <值>）
function timeAddAdvances(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/time set 1000");
    player.chat("/time add 500");

    assertTimeNear(test, dim.getTimeOfDay(), 1500, "time set 1000 + add 500");
    test.succeed();
}

export function registerTimeTests(): void {
    GameTest.register("CommandTests", "time_set_integer", timeSetInteger)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "time_set_day", timeSetDay)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "time_set_noon", timeSetNoon)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "time_set_night", timeSetNight)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "time_set_midnight", timeSetMidnight)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "time_add_advances", timeAddAdvances)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
