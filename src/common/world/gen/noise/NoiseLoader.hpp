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

#include "Noises.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <nlohmann/json_fwd.hpp>

namespace mc {

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace world::gen::noise {

/**
 * @brief 噪声参数 JSON 加载器（MC 1.21.11 worldgen/noise）
 *
 * 从数据包加载 NoiseParameters JSON，注册到 Noises 静态注册表。
 *
 * JSON 格式 (MC 1.21.11):
 * {
 *   "firstOctave": -10,
 *   "amplitudes": [1.5, 0.0, 1.0, 0.0, 0.0, 0.0]
 * }
 *
 * 加载路径: data/<namespace>/worldgen/noise/<path>.json
 *
 * 加载流程：clear() 清空硬编码兜底 → 逐文件解析注入 → markLoadedFromDatapack()
 * 置位，使后续 Noises::get()/has() 跳过 initialize() 兜底。
 */
class NoiseLoader {
public:
    /**
     * @brief 从数据包列表加载所有噪声参数
     *
     * 先 clear() 再注入，最后 markLoadedFromDatapack()。
     *
     * @param dataPackList 数据包列表
     * @return 加载的噪声参数数量
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载所有噪声参数
     *
     * @param pack 资源包
     * @return 加载的噪声参数数量
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

private:
    /**
     * @brief 从 JSON 对象解析单个噪声参数
     *
     * @param jsonObj 已解析的 JSON 对象
     * @param location 噪声资源位置（用于错误信息）
     * @return 噪声参数，或错误信息
     */
    [[nodiscard]] static Result<NoiseParameters> loadFromJson(
        const nlohmann::json& jsonObj, const ResourceLocation& location);
};

} // namespace world::gen::noise
} // namespace mc
