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
 */

#pragma once

#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>

namespace mc {
namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace world::gen::settings {

/**
 * @brief world_preset JSON 加载器（MC 1.21.11 worldgen/world_preset）
 *
 * 从数据包加载 6 个 world_preset JSON（normal/flat/large_biomes/amplified/
 * debug_all_block_states/single_biome_surface），经 WorldPreset::fromJson 解析
 * （dimensions map + generator.type[noise|flat|debug] + biome_source[multi_noise|the_end|fixed]
 * + settings[noise_settings RL | flat 内联对象]）后注册到 WorldPresetRegistry（name→preset）。
 *
 * 依赖：FlatLevelGeneratorSettings::fromSettingsObject（flat 维度内联 settings，依赖
 * BlockRegistry 与 BiomeLoader::biomeIdByName），须在 VanillaBlocks::initialize 之后加载。
 * noise 维度不在此解析 noise_settings（仅存 RL，装配期由 RandomState::create 查 NoiseSettingsRegistry）。
 *
 * 加载路径: data/<namespace>/worldgen/world_preset/<path>.json
 */
class WorldPresetLoader {
public:
    /**
     * @brief 从数据包列表加载所有世界预设
     *
     * 先 clear() WorldPresetRegistry，再逐文件解析注入，最后 markLoadedFromDatapack(true)。
     *
     * @param dataPackList 数据包列表
     * @return 加载的世界预设数量
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载所有世界预设
     *
     * @param pack 资源包
     * @return 加载的世界预设数量
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);
};

} // namespace world::gen::settings
} // namespace mc
