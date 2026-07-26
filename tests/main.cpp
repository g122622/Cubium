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

// 测试程序入口：安装 CrashHandler 后运行所有 GoogleTest 用例。
// 崩溃（SEH 异常、信号、纯虚调用、std::terminate、MC_ASSERT_RELEASE 触发的 abort）时
// 输出调用栈，便于定位测试失败/挂起根因。参考 src/client/main.cpp 与 src/server/main.cpp。

#include "common/core/GameDirectory.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/assert/CrashHandler.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/density/DensityFunctionLoader.hpp"
#include "common/world/gen/noise/NoiseLoader.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorPresetLoader.hpp"
#include "common/world/gen/settings/NoiseSettingsLoader.hpp"
#include "common/world/gen/settings/WorldPresetLoader.hpp"
#include "server/world/ChunkTaskScheduler.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>

namespace {

/// 全局测试环境：在所有用例运行前一次性加载 noise_settings 数据驱动注册表。
///
/// RandomState::create 现为数据驱动唯一路径（查 NoiseSettingsRegistry），
/// 故任何调用 create() 的测试都需 Noises / DensityFunctionRegistry / NoiseSettingsRegistry
/// 已从原版数据包加载。注册表为进程级单例，加载一次即对全部测试可见。
/// 数据包目录缺失时（非开发机）静默跳过——此类测试会因 registry 为空而断言失败，
/// 属预期（CI/开发机数据包应存在）。
class WorldGenRegistryEnvironment : public ::testing::Environment {
public:
    void SetUp() override
    {
        const auto dataPackDir = mc::GameDirectory::defaultDirectory().dataPacksDir();
        if (!std::filesystem::exists(dataPackDir)) {
            return;
        }
        mc::resource::DataPackRepository repo;
        auto scanResult = repo.scanDirectory(dataPackDir);
        if (!scanResult.success() || scanResult.value() == 0) {
            return;
        }
        // noise_settings 的 JSON 解析会调 BlockStateParser::parse("minecraft:stone")，
        // 依赖 BlockRegistry 已注册原版方块。VanillaBlocks::initialize() 幂等（s_initialized 守卫），
        // 在此显式调用使本环境自包含——不依赖其它测试文件的静态全局 Environment 注册顺序。
        mc::VanillaBlocks::initialize();

        // 加载顺序：noise → density_function → noise_settings → flat_level_generator_preset（依赖拓扑）。
        // flat preset 依赖 BlockRegistry（层方块，已由 VanillaBlocks::initialize 注册）与
        // BiomeLoader::biomeIdByName（编译期静态 biome 名映射表），不依赖运行时 BiomeRegistry。
        mc::world::gen::noise::NoiseLoader::loadFromDataPackRepository(repo);
        mc::world::gen::density::DensityFunctionLoader::loadFromDataPackRepository(repo);
        mc::world::gen::settings::NoiseSettingsLoader::loadFromDataPackRepository(repo);
        mc::world::gen::settings::FlatLevelGeneratorPresetLoader::loadFromDataPackRepository(repo);
        // world_preset 依赖 noise_settings（noise 维度存 RL）+ flat_preset（flat 维度内联 settings 复用
        // FlatLevelGeneratorSettings::fromSettingsObject）。
        mc::world::gen::settings::WorldPresetLoader::loadFromDataPackRepository(repo);
    }
};

} // namespace

/// 跨用例隔离监听器：每用例结束后重置 ChunkTaskScheduler 的 thread_local 同步调度上下文。
///
/// 全二进制直跑（`mc_tests --gtest_filter=...`）时同 worker 线程跨用例复用 thread_local
/// SyncSchedulingContext，若上一用例残留 pending 队列，下一用例首次 runInlineAndDrain 会
/// 多一次虚假调度，是 GameEventServerTest 等用例全二进制 flaky 的污染源之一。
/// 生产路径下 depth 出口必归零、pending 必空，故重置对生产无影响。
/// 监听器挂默认事件转发器之后，仅 OnTestEnd 做清理，不干扰断言/输出。
class TestIsolationListener : public ::testing::EmptyTestEventListener {
public:
    void OnTestEnd(const ::testing::TestInfo& /*test_info*/) override
    {
        mc::server::ChunkTaskScheduler::resetThreadLocalContext();
    }
};

int main(int argc, char* argv[])
{
    // 安装崩溃处理器：捕获 SEH 异常、信号、纯虚函数调用、std::terminate 等，
    // 输出调用栈和局部变量信息到终端。多次调用安全（只安装一次）。
    mc::assert::CrashHandler::install();

    ::testing::InitGoogleTest(&argc, argv);
    // 注册全局环境：在 RUN_ALL_TESTS 之前加载世界生成注册表。
    ::testing::AddGlobalTestEnvironment(new WorldGenRegistryEnvironment());
    // 注册跨用例隔离监听器：每用例结束重置 thread_local 调度上下文，根除全二进制
    // 直跑时的跨用例共享状态污染（详见 docs/TEST.md「跨测试隔离模型」）。
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestIsolationListener());
    const int result = RUN_ALL_TESTS();

    // CrashHandler 不需要 uninstall：进程即将退出，操作系统回收所有资源。
    return result;
}
