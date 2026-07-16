/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// mc_command_tests 程序入口：与 tests/main.cpp 对齐。
// 注册全局 WorldGenRegistryEnvironment 在所有用例运行前加载 noise / density_function /
// noise_settings 数据驱动注册表——RandomState::create 现为数据驱动唯一路径（查
// NoiseSettingsRegistry），命令测试中构造 NoiseChunkGenerator/维度的用例需注册表已就绪。
// 崩溃时输出调用栈（CrashHandler）。参考 tests/main.cpp。

#include "common/core/GameDirectory.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/assert/CrashHandler.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/density/DensityFunctionLoader.hpp"
#include "common/world/gen/noise/NoiseLoader.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorPresetLoader.hpp"
#include "common/world/gen/settings/NoiseSettingsLoader.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>

namespace {

/// 全局测试环境：在所有用例运行前一次性加载 noise_settings 数据驱动注册表。
///
/// RandomState::create 现为数据驱动唯一路径（查 NoiseSettingsRegistry），故任何调用
/// create() 的命令测试都需 Noises / DensityFunctionRegistry / NoiseSettingsRegistry 已从
/// 原版数据包加载。注册表为进程级单例，加载一次即对全部测试可见。数据包目录缺失时
/// （非开发机）静默跳过——此类测试会因 registry 为空而断言失败，属预期。
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
        mc::world::gen::noise::NoiseLoader::loadFromDataPackRepository(repo);
        mc::world::gen::density::DensityFunctionLoader::loadFromDataPackRepository(repo);
        mc::world::gen::settings::NoiseSettingsLoader::loadFromDataPackRepository(repo);
        mc::world::gen::settings::FlatLevelGeneratorPresetLoader::loadFromDataPackRepository(repo);
    }
};

} // namespace

int main(int argc, char* argv[])
{
    // 安装崩溃处理器：捕获 SEH 异常、信号、纯虚函数调用、std::terminate 等，输出调用栈。
    mc::assert::CrashHandler::install();

    ::testing::InitGoogleTest(&argc, argv);
    // 注册全局环境：在 RUN_ALL_TESTS 之前加载世界生成注册表。
    ::testing::AddGlobalTestEnvironment(new WorldGenRegistryEnvironment());
    const int result = RUN_ALL_TESTS();

    return result;
}
