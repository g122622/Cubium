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

#pragma once

#include "common/core/GameDirectory.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "server/world/gen/biome/BiomeLoader.hpp"
#include "server/world/gen/carver/ConfiguredCarverLoader.hpp"
#include "server/world/gen/feature/ConfiguredFeatureLoader.hpp"
#include "server/world/gen/feature/FeatureTypeRegistry.hpp"
#include "server/world/gen/jigsaw/ProcessorListLoader.hpp"
#include "server/world/gen/placement/PlacedFeatureLoader.hpp"
#include "server/world/gen/placement/PlacementRegistry.hpp"
#include "server/world/gen/structure/StructureDefinitionLoader.hpp"
#include "server/world/gen/structure/StructureManager.hpp"
#include "server/world/gen/structure/StructureSetLoader.hpp"
#include "server/world/gen/structure/StructureTagLoader.hpp"
#include "server/world/gen/structure/pools/Pools.hpp"

#include "common/util/assert/AssertMacros.hpp"

#include <filesystem>

namespace mc {
namespace test {

/**
 * @brief 数据驱动世界生成注册表测试夹具
 *
 * 数据驱动迁移后，carver/feature 走 ConfiguredCarverRegistry/PlacedFeatureRegistry，
 * 这些注册表仅在 MinecraftServer::initializeRegistries 中由数据包加载。直接构造
 * NoiseChunkGenerator 并调用 applyCarvers/placeFeatures 的单元测试若不预先加载注册表，
 * 会得到空雕刻/空装饰（注册表为空，id 解析为 nullptr 被跳过）。
 *
 * 本夹具按 MC 顺序从默认数据包目录（~/minecraft_reborn/datapacks/）加载全部 worldgen
 * 注册表：PlacementRegistry → FeatureTypeRegistry → ConfiguredFeature →
 * PlacedFeature → ConfiguredCarver → Biome。供需要真实雕刻/装饰管线的测试调用。
 *
 * 注册表为进程级单例，重复调用安全（多次加载会累积覆盖同名条目）。
 *
 * @return 数据包目录存在且扫描成功返回 true；目录缺失返回 false（调用方应 GTEST_SKIP）
 */
inline bool loadVanillaWorldGenRegistries()
{
    const auto dataPackDir = GameDirectory::defaultDirectory().dataPacksDir();
    if (!std::filesystem::exists(dataPackDir)) {
        return false;
    }

    resource::DataPackRepository repo;
    auto scanResult = repo.scanDirectory(dataPackDir);
    if (!scanResult.success() || scanResult.value() == 0) {
        return false;
    }

    PlacementRegistry::instance().initialize();
    world::gen::feature::initializeBuiltinFeatureTypes();
    world::gen::feature::ConfiguredFeatureLoader::loadFromDataPackRepository(repo);
    world::gen::placement::PlacedFeatureLoader::loadFromDataPackRepository(repo);
    world::gen::carver::ConfiguredCarverLoader::loadFromDataPackRepository(repo);
    BiomeRegistry::instance().initialize();
    world::biome::BiomeLoader::loadFromDataPackRepository(repo);

    // 结构管线：模板池 → 结构定义 → 处理器列表 → 结构标签 → 结构集。
    //
    // 【为何必须加载】结构注册表是纯数据驱动（无硬编码兜底，兜底表已于 2026-09 删除）。
    // 若测试不加载这一段，StructureRegistry 与 StructureSetRegistry 均为空 →
    // 不生成任何结构。更早的历史教训：曾有一份按"结构类型基础名"注册的硬编码兜底表
    // （minecraft:village、minecraft:ocean_ruin），与 structure_set JSON 引用的细分 id
    // （minecraft:village_plains、minecraft:ocean_ruin_cold）对不上，导致
    // _hasBiomesForStructureSet 把村庄/海底废墟/古迹废墟整集静默跳过，parity 数值
    // 完全不含结构贡献，且全过程不产生任何报错。该教训是"夹具缺陷让结论失真"的典型。
    //
    // 顺序与生产路径 RegistryBootstrap::initializeAll 一致（模板池须先于结构定义，
    // 结构标签须先于结构集），装配完成后做与生产同款的启动期完整性校验。
    world::gen::structure::pools::Pools::initialize();
    (void)world::gen::structure::StructureRegistry::loadTemplatePoolsFromDataPacks(repo);
    world::gen::structure::StructureRegistry::clear();
    {
        const auto structureResult = world::gen::structure::StructureDefinitionLoader::loadFromDataPackRepository(repo);
        MC_ASSERT_RELEASE_MSG(structureResult.success(), "structures failed to load from data packs");
    }
    (void)world::gen::jigsaw::ProcessorListLoader::loadFromDataPackRepository(repo);
    {
        const auto tagResult = world::gen::structure::StructureTagLoader::loadFromDataPackRepository(repo);
        MC_ASSERT_RELEASE_MSG(tagResult.success(), "structure tags failed to load from data packs");
    }
    world::gen::structure::StructureSetRegistry::instance().clear();
    {
        const auto setResult = world::gen::structure::StructureSetLoader::loadFromDataPackRepository(repo);
        MC_ASSERT_RELEASE_MSG(setResult.success(), "structure sets failed to load from data packs");
    }
    world::gen::structure::StructureSetRegistry::instance().validateAgainstStructureRegistry();

    return true;
}

} // namespace test
} // namespace mc
