// 夜间内部天空光照衰减测试：验证内部天空光照随时间衰减（wiki 行434 核心机制）。
//
// wiki 内部光照公式（tech_亮度.txt#内部光照，行434）：
//   内部光照 = max(方块光照 L_b, 内部天空光照 L_i)
//   内部天空光照 = max(0, 天空光照 - skyDarkening)
//   skyDarkening 由时间与天气计算（calculateSkyDarkening）。
//
// Cubium 实现（InternalLightUtils.cpp:38-83 calculateSkyDarkening）：
//   - 基础 darkening = (1 - brightness) * 11，brightness = (sin(celestialAngle*2π)+1)/2，celestialAngle = dayTime/24000。
//   - 雷暴 min(base+10, 11)，雨天 min(base+3, 11)。
//   - 注意是 sin 曲线（非线性），午夜 dayTime=18000 时 sin=-1 → brightness=0 → base=11。
//
// batch("night")（GameTestServer.cpp:341）设 dayTime=18000（午夜）+ 强制晴天（WeatherEnvironment::Clear，
// clearWeatherTime=100000 持续保持 raining=false/thundering=false）。daylight cycle 开着（未禁 doDaylightCycle），
// dayTime 每 tick 递增，但午夜附近 sin 曲线导数≈0，skyDarkening 在 dayTime∈(18000,24000) 稳定=10
// （仅精确 dayTime=18000 那一瞬=11，前进后即=10）。pollUntilSucceed startTick=5 首次读取时 dayTime 已 >18000，
// skyDarkening 稳定=10。
//
// 关键区分点：skyDarkening 只影响「内部天空光照」（即 brightness），不影响原始 skyLight。
// wiki 行26：「天空光照与时间无关，它不会随着昼夜更替或天气更替而变化」。故夜间露天 skyLight 仍=15，
// 而 brightness=max(0, 15-10)=5。同时断言 skyLight=15 与 brightness=5，精确区分「衰减生效」与「天空光损坏」
// 两种实现错误（若 skyLight 也被错误衰减→skyLight<15，若 brightness 未衰减→brightness=15）。
//
// 设计：grass_pen（9×5×9 露天，y=4 air 露天层）+ skyAccess(true) + batch("night") + setupTicks(20)。
// 露天格 (4,4,3) skyLight=15（露天列垂直直达，与时间无关），brightness=5（夜间衰减 15-10）。
//
// 跨服务端：brightness/skyLight 是 Cubium 专有，基岩端 one-sided。batch("night") 是 Cubium GameTest 扩展
// （基岩 BDS 无对应 batch 环境，需基岩侧另行验证）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#内部光照（行434 max 公式 + 行26 天空光与时间无关）
// Ref: BrightnessTests.ts（晴天白天 skyDarkening=0 → brightness=15，本组补 skyDarkening>0 夜间场景）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getSkyLight, getBrightness } from "../utils/lightAssert.js";

// 夜间露天 brightness 衰减到5：batch("night") dayTime=18000 午夜，skyDarkening=10（午夜前进后稳定），
// 露天格 skyLight=15（与时间无关，不变），brightness=max(0,15-10)=5。
// 验证内部天空光照随时间衰减（skyDarkening>0），同时 skyLight 不受时间影响（区分衰减生效与天空光损坏）。
function nightOpenSkyBrightnessIsFive(test: Test): void {
    pollUntilSucceed(
        test,
        () => {
            return (
                // skyLight 不受时间影响，夜间露天仍=15（wiki 行26）。
                getSkyLight(test, 4, 4, 3) === 15 &&
                // brightness 受 skyDarkening 衰减：夜间 skyDarkening=10，15-10=5。
                getBrightness(test, 4, 4, 3) === 5
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `night open sky (4,4,3): skyLight=${getSkyLight(test, 4, 4, 3)} ` +
                        `brightness=${getBrightness(test, 4, 4, 3)} expected 15/5 (skyDarkening=10 at midnight)`,
                );
            },
        },
    );
}

export function registerNightSkyDarkeningTests(): void {
    GameTest.register("LightingTests", "light_night_open_sky_brightness_is_5", nightOpenSkyBrightnessIsFive)
        .structureName("gametests:grass_pen")
        .batch("night")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
}
