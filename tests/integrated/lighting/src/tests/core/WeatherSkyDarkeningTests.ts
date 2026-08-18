// 天气内部天空光照衰减测试：验证内部天空光照随天气衰减（wiki 行434 核心机制）。
//
// wiki 内部光照公式（tech_亮度.txt#内部光照，行434）：
//   内部天空光照 = max(0, 天空光照 - skyDarkening)
//   skyDarkening 由时间与天气计算：雷暴 min(base+10,11)，雨天 min(base+3,11)（InternalLightUtils.cpp:38-53）。
//
// Cubium 实现（InternalLightUtils.cpp:44-52 calculateSkyDarkening）：
//   if (isThundering) return min(baseDarkening + 10, 11);
//   if (isRaining)    return min(baseDarkening + 3, 11);
//   return baseDarkening;  // baseDarkening 由 celestialAngle sin 曲线算（正午0）
// 关键：只查 isRaining()/isThundering() 布尔，不查 rainStrength，故 setRain/setThunder 后立即生效（无需渐变）。
//
// GameTest 天气控制：GameTestServer 只为 day/night batch 注册 Clear 环境，无 rain/thunder batch。
// 但 SimulatedPlayer.chat(command) 以创造模式权限级别2执行命令（SimulatedPlayer.cpp:182-192 isCreative()?2:0），
// /weather 命令需权限2（WeatherCommand.cpp:111），故脚本可经 SimulatedPlayer 触发并保持雨天/雷暴。
// /weather rain 100000 → WeatherManager::setRain(100000) → raining=true 持续100k tick（远超测试寿命，稳定）。
// /weather thunder 100000 → setThunder → raining=true,thundering=true 持续100k tick。
//
// 设计：默认 batch（day，dayTime=6000 正午晴天，baseDarkening=0）+ grass_pen 露天 + skyAccess(true)。
// 正午晴天 baseDarkening=0，天气贡献易隔离：
//   - 雨天：skyDarkening=min(0+3,11)=3，露天 brightness=max(0,15-3)=12。
//   - 雷暴：skyDarkening=min(0+10,11)=10，露天 brightness=max(0,15-10)=5。
//   - skyLight 始终=15（wiki 行26 天空光与天气无关），不受 skyDarkening 影响。
// 同时断言 skyLight=15 与 brightness=衰减值，区分「衰减生效」与「天空光损坏」。
//
// 玩家放置：grass_pen 内部 7×7 x,z∈[1,7]，玩家放角落 (1,2,1) 不挡露天格 (4,4,3) 的天空光
// （实体不挡天空光，仅方块挡；且 (1,2,1) 远离 (4,4,3)）。创造模式 GameMode.creative 获取权限2。
//
// 命令异步：chat 执行命令后天气即时生效（布尔门控），但命令派发可能跨 tick，pollUntilSucceed 轮询等到
// brightness 达预期。onTimeout 打印实际 skyLight/brightness 便于诊断（命令失败→天气不变→brightness 仍15）。
//
// 跨服务端：brightness/skyLight 是 Cubium 专有，基岩端 one-sided。SimulatedPlayer.chat 是 Cubium 补全
// （基岩 BDS SimulatedPlayer 无 chat），天气衰减测试基岩侧无法跑，one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#内部光照（行434 skyDarkening 天气计算 + 行26 天空光与天气无关）
// Ref: NightSkyDarkeningTests.ts（夜间 baseDarkening=10 时间衰减场景，本组补天气 +3/+10 场景）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getSkyLight, getBrightness } from "../utils/lightAssert.js";

// 雨天露天 brightness 衰减到12：正午晴天 baseDarkening=0，/weather rain 100000 设雨天，
// skyDarkening=min(0+3,11)=3，露天 brightness=max(0,15-3)=12，skyLight 仍=15（与天气无关）。
// 验证雨天 +3 衰减，同时 skyLight 不受天气影响（区分衰减生效与天空光损坏）。
// spawnSimulatedPlayer 不传 gameMode，默认 Creative（ScriptTestHelper.cpp:518），权限级别2可执行 /weather。
function rainOpenSkyBrightnessIsTwelve(test: Test): void {
    // 创造模式 SimulatedPlayer（权限级别2）执行 /weather rain 100000 持续雨天。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "weather_rain");
    player.chat("/weather rain 100000");
    pollUntilSucceed(
        test,
        () => {
            return (
                // skyLight 不受天气影响，雨天露天仍=15（wiki 行26）。
                getSkyLight(test, 4, 4, 3) === 15 &&
                // brightness 受 skyDarkening 衰减：雨天 skyDarkening=3，15-3=12。
                getBrightness(test, 4, 4, 3) === 12
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 120,
            onTimeout: () => {
                test.assert(
                    false,
                    `rain open sky (4,4,3): skyLight=${getSkyLight(test, 4, 4, 3)} ` +
                        `brightness=${getBrightness(test, 4, 4, 3)} expected 15/12 ` +
                        `(rain skyDarkening=3; if brightness=15 weather command may have failed)`,
                );
            },
        },
    );
}

// 雷暴露天 brightness 衰减到5：正午晴天 baseDarkening=0，/weather thunder 100000 设雷暴，
// skyDarkening=min(0+10,11)=10，露天 brightness=max(0,15-10)=5，skyLight 仍=15。
// 验证雷暴 +10 衰减（被 min 上限11 截断为10）。注意雷暴白天 brightness=5 与夜间（baseDarkening=10）相同，
// 因 min(0+10,11)=10 与夜间 baseDarkening=10 衰减量一致。
function thunderOpenSkyBrightnessIsFive(test: Test): void {
    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "weather_thunder");
    player.chat("/weather thunder 100000");
    pollUntilSucceed(
        test,
        () => {
            return (
                // skyLight 不受天气影响，雷暴露天仍=15。
                getSkyLight(test, 4, 4, 3) === 15 &&
                // brightness 受 skyDarkening 衰减：雷暴 skyDarkening=min(0+10,11)=10，15-10=5。
                getBrightness(test, 4, 4, 3) === 5
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 120,
            onTimeout: () => {
                test.assert(
                    false,
                    `thunder open sky (4,4,3): skyLight=${getSkyLight(test, 4, 4, 3)} ` +
                        `brightness=${getBrightness(test, 4, 4, 3)} expected 15/5 ` +
                        `(thunder skyDarkening=10; if brightness=15 weather command may have failed)`,
                );
            },
        },
    );
}

export function registerWeatherSkyDarkeningTests(): void {
    GameTest.register("LightingTests", "light_rain_open_sky_brightness_is_12", rainOpenSkyBrightnessIsTwelve)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(200);
    GameTest.register("LightingTests", "light_thunder_open_sky_brightness_is_5", thunderOpenSkyBrightnessIsFive)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(200);
}
