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
 * @brief flat_level_generator_preset JSON 加载器（MC 1.21.11 worldgen/flat_level_generator_preset）
 *
 * 从数据包加载 9 个 flat_level_generator_preset JSON（classic_flat/the_void/tunnelers_dream/
 * water_world/snowy_kingdom/desert/overworld/bottomless_pit/redstone_ready），经
 * FlatLevelGeneratorSettings::fromJson 解析（biome RL→BiomeId + layers + features/lakes +
 * structure_overrides 三态）后注册到 FlatLevelGeneratorPresetRegistry（name→settings）。
 *
 * 依赖：BiomeLoader::biomeIdByName（biome 名→BiomeId 映射表）与 BlockRegistry（层方块默认状态），
 * 须在 VanillaBlocks::initialize 与 BiomeRegistry::initialize 之后加载。
 *
 * 加载路径: data/<namespace>/worldgen/flat_level_generator_preset/<path>.json
 */
class FlatLevelGeneratorPresetLoader {
public:
    /**
     * @brief 从数据包列表加载所有 flat 预设
     *
     * 先 clear() FlatLevelGeneratorPresetRegistry，再逐文件解析注入，最后 markLoadedFromDatapack(true)。
     *
     * @param dataPackList 数据包列表
     * @return 加载的 flat 预设数量
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载所有 flat 预设
     *
     * @param pack 资源包
     * @return 加载的 flat 预设数量
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);
};

} // namespace world::gen::settings
} // namespace mc
