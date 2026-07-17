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

namespace mc {
namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace world::gen::settings {

/**
 * @brief noise_settings JSON 加载器（MC 1.21.11 worldgen/noise_settings）
 *
 * 从数据包加载 7 个 noise_settings JSON（overworld/large_biomes/amplified/caves/
 * floating_islands/nether/end），经 DimensionSettings::fromJson 解析后注册到
 * NoiseSettingsRegistry（name→DimensionSettings）。
 *
 * 依赖：必须在 DensityFunctionLoader 之后加载（noise_router 15 字段引用命名 DF，
 * 经 DensityFunctionLoader::resolveHolderElement 解析，字符串引用查 DensityFunctionRegistry）。
 *
 * 加载路径: data/<namespace>/worldgen/noise_settings/<path>.json
 */
class NoiseSettingsLoader {
public:
    /**
     * @brief 从数据包列表加载所有 noise_settings
     *
     * 先 clear() NoiseSettingsRegistry，再逐文件解析注入，最后 markLoadedFromDatapack(true)。
     *
     * @param dataPackList 数据包列表
     * @return 加载的 noise_settings 数量
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载所有 noise_settings
     *
     * @param pack 资源包
     * @return 加载的 noise_settings 数量
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);
};

} // namespace world::gen::settings
} // namespace mc
