// /weather 命令 GameTest：设置天气（晴/雨/雷暴）。
//
// 覆盖 wiki 命令章节核心行为：
//   - /weather clear|rain|thunder [<时长>]：设置天气（Ref: wiki commands/weather.txt）
//
// 设计要点：
//   1. WeatherCommand 经 weatherManager->setClear/setRain/setThunder 修改 WeatherState
//      （raining/thundering 标志 + 计时器），非占位。setRain 设 raining=true，强度 rainStrength
//      由 _updateStrength 每 tick 渐变 +0.01（STRENGTH_CHANGE_RATE），约 20 tick 到 RAIN_THRESHOLD(0.2)。
//      isRaining() 检查 rainStrength > 0.2（非裸 raining 标志），故 /weather rain 后不会立即返 true，
//      须等强度渐变——这是对齐 Java/基岩的原版行为（非偏差）。
//   2. 本次补全 Dimension.isRaining()/isThundering() 脚本读取绑定（IWorld::isRaining 经
//      ServerWorld override 委托 WeatherManager::isRaining 检查强度阈值），解锁端到端测试。
//      测试用 pollUntilSucceed 轮询 isRaining()，等强度渐变到阈值后断言（maxTick 200 足够覆盖
//      20 tick 渐变 + 余量）。
//   3. 自然天气循环干扰防护：doWeatherCycle 默认 true 时 WeatherManager 会随机触发降雨，干扰
//      命令设置的天气断言。测试开头 /gamerule doWeatherCycle false 关自然循环，确保天气仅由命令控制。
//      doWeatherCycle 是世界级状态，runOnFinish 恢复 true 防污染。
//   4. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_weather.txt（天气命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed, waitForCondition } from "../../utils/test/poll.js";
import { asDim } from "../../utils/script/cubiumExtensions.js";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// /weather rain 设置降雨，等强度渐变后断言 isRaining()=true。
// 关 doWeatherCycle 隔离自然循环；runOnFinish 恢复 doWeatherCycle + clear。
// Ref: wiki commands/weather.txt（weather rain）
function weatherRain(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/gamerule doWeatherCycle false");
    player.chat("/weather rain");

    pollUntilSucceed(
        test,
        () => dim.isRaining(),
        {
            maxTick: 200,
            onTimeout: () => test.assert(false, "expected isRaining()=true after /weather rain (strength gradient)"),
        },
    );

    test.runOnFinish(() => {
        player.chat("/weather clear");
        player.chat("/gamerule doWeatherCycle true");
    });
}

// /weather thunder 设置雷暴，等强度渐变后断言 isThundering()=true。
// 雷暴阈值 THUNDER_THRESHOLD=0.9，渐变需 90 tick，maxTick 200 足够。
// Ref: wiki commands/weather.txt（weather thunder）
function weatherThunder(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/gamerule doWeatherCycle false");
    player.chat("/weather thunder");

    pollUntilSucceed(
        test,
        () => dim.isThundering(),
        {
            maxTick: 200,
            onTimeout: () => test.assert(false, "expected isThundering()=true after /weather thunder"),
        },
    );

    test.runOnFinish(() => {
        player.chat("/weather clear");
        player.chat("/gamerule doWeatherCycle true");
    });
}

// /weather clear 清除降雨：先 rain 等强度上来，再 clear，等强度渐变下去后断言 isRaining()=false。
// 验证 setClear 把 raining 标志设 false 致强度渐变回 0。
// 用 waitForCondition 分阶段：阶段1 等雨起 → onReady 里 clear（仅调一次）→ 阶段2 等雨停 → succeed。
// Ref: wiki commands/weather.txt（weather clear）
function weatherClear(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const dim = asDim(player.dimension);

    player.chat("/gamerule doWeatherCycle false");
    player.chat("/weather rain");

    // 阶段1：等雨强度到阈值（isRaining=true），onReady 里 clear（仅一次，避免重复调用）。
    waitForCondition(
        test,
        () => dim.isRaining(),
        () => {
            player.chat("/weather clear");
            // 阶段2：clear 后等强度渐变到 < 阈值（rainStrength 从 ~1.0 降到 0.2 需 ~80 tick）。
            pollUntilSucceed(
                test,
                () => !dim.isRaining(),
                {
                    startTick: 100, // 给 clear 后强度渐变留足时间
                    maxTick: 200,
                    onTimeout: () => test.assert(false, "expected isRaining()=false after /weather clear"),
                },
            );
        },
        {
            maxTick: 200,
            onTimeout: () => test.assert(false, "rain did not start within 200 ticks"),
        },
    );

    test.runOnFinish(() => {
        player.chat("/gamerule doWeatherCycle true");
    });
}

export function registerWeatherTests(): void {
    // 天气是世界级单例状态，GameTest 同批测试并行执行共享同一 ServerWorld 天气，
    // 三个天气测试若同批并行会互相覆盖（一个 /weather clear 清掉另一个的雨）。
    // 故各用独占 batch 名（前缀非 day/night → 走 day 环境=Clear，由测试内 /weather 自行设天气），
    // 独占 batch 仅含 1 个测试，串行执行，互不干扰。同 [[gametest-world-state-gamerule-difficulty-batch-isolation]] 范式。
    GameTest.register("CommandTests", "weather_rain", weatherRain)
        .structureName("gametests:cmd_arena")
        .batch("weather_rain_solo")
        .maxTicks(220);

    GameTest.register("CommandTests", "weather_thunder", weatherThunder)
        .structureName("gametests:cmd_arena")
        .batch("weather_thunder_solo")
        .maxTicks(220);

    GameTest.register("CommandTests", "weather_clear", weatherClear)
        .structureName("gametests:cmd_arena")
        .batch("weather_clear_solo")
        .maxTicks(260);
}
