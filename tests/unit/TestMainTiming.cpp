/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, IN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// 单测进程启动成本的基准用例（gtest 用例，不进入 ctest 登记，由 CI 用 --gtest_filter 直接驱动）。
//
// 背景：CI 上每个 ctest 用例都以独立进程运行，单进程固定开销在 8~14 秒，且 CPU 时间与墙钟
// 基本相同（几乎全部是用户态自旋而非等待 I/O），本机同配置只需 ~0.1 秒。ctest 的
// PerTest 计时对它没有任何归因。
//
// 本用例按顺序输出本进程在几个阶段点的 monotonic 时刻（毫秒）：
//   1. 用例体入口（gtest 全局环境已跑完，进程静态初始化也已完成）
//   2. 手动调用 VanillaBlocks/Items/BlockItemRegistry 之后的时刻——WorldGenRegistryEnvironment
//      已经做过一遍，重复调用有幂等守卫，故差值只反映注册表已就绪时的再度调用成本
//   3. 与 2 相同，用于观察波动
//   4. 可选（环境变量 MC_TIMING_WORLDGEN=1）：加载 noise/density/noise_settings/world_preset，
//      观察世界生成数据加载回路的成本
//   5. 用例体退出前
// CI 作业用 `--gtest_filter=TestMainTiming.Phases` 对多个二进制各跑一次，比对阶段时刻，
// 即可把这些阶段与"其余部分"（全局环境加载标签/注册表、静态初始化、退出）分开。

#include "common/core/GameDirectory.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/gen/density/DensityFunctionLoader.hpp"
#include "server/world/gen/noise/NoiseLoader.hpp"
#include "server/world/gen/settings/FlatLevelGeneratorPresetLoader.hpp"
#include "server/world/gen/settings/NoiseSettingsLoader.hpp"
#include "server/world/gen/settings/WorldPresetLoader.hpp"

#include <gtest/gtest.h>

#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdlib>
#include <string_view>

namespace {

/// steady_clock 当前读数（毫秒）。
inline i64 nowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

} // namespace

/// 记录本进程的启动阶段时刻。输出行前缀固定为 `[TestMainTiming]`，便于 CI 侧正则提取。
TEST(TestMainTiming, Phases)
{
    const i64 enter = nowMs();
    spdlog::info("[TestMainTiming] enter={}", enter);

    mc::VanillaBlocks::initialize();
    mc::Items::ensureInitialized();
    mc::BlockItemRegistry::instance().initializeVanillaBlockItems();
    const i64 registries = nowMs();
    spdlog::info("[TestMainTiming] registries={}", registries);

    // 第二遍：此时全部幂等守卫均已命中，只测「已就绪后的一遍调用」本身。
    mc::VanillaBlocks::initialize();
    mc::Items::ensureInitialized();
    mc::BlockItemRegistry::instance().initializeVanillaBlockItems();
    const i64 registriesAgain = nowMs();
    spdlog::info("[TestMainTiming] registries_again={}", registriesAgain);

    const char* worldGen = std::getenv("MC_TIMING_WORLDGEN");
    if (worldGen != nullptr && std::string_view(worldGen) == "1") {
        mc::resource::DataPackRepository repository;
        const auto scan = repository.scanDirectory(mc::GameDirectory::defaultDirectory().dataPacksDir());
        MC_ASSERT_RELEASE_MSG(scan.success() && scan.value() > 0, "Timing run requires the vanilla data pack");
        mc::world::gen::noise::NoiseLoader::loadFromDataPackRepository(repository);
        mc::world::gen::density::DensityFunctionLoader::loadFromDataPackRepository(repository);
        mc::world::gen::settings::NoiseSettingsLoader::loadFromDataPackRepository(repository);
        mc::world::gen::settings::FlatLevelGeneratorPresetLoader::loadFromDataPackRepository(repository);
        mc::world::gen::settings::WorldPresetLoader::loadFromDataPackRepository(repository);
        spdlog::info("[TestMainTiming] worldgen={}", nowMs());
    }

    EXPECT_GE(registries, enter);
    spdlog::info("[TestMainTiming] exit={}", nowMs());
}
