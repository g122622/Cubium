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

#include <nlohmann/json_fwd.hpp>

#include <cstddef>
#include <memory>
#include <string>

namespace mc {

class ConfiguredFeatureBase;

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace world::gen::feature {

/**
 * @brief 配置化特征 JSON 加载器
 *
 * 从数据包加载 configured_feature JSON 文件，注册到 ConfiguredFeatureRegistry。
 *
 * JSON 格式 (MC 1.21.11):
 * {
 *   "type": "minecraft:monster_room",
 *   "config": {}
 * }
 *
 * 加载路径: data/<namespace>/worldgen/configured_feature/<path>.json
 * ResourceLocation = <namespace>:<path>（去掉 .json）
 *
 * 严格报错：type 未在 FeatureTypeRegistry 注册时返回 Error 中断加载。
 */
class ConfiguredFeatureLoader {
public:
    /**
     * @brief 从数据包仓库加载所有配置化特征
     * @return 加载的数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& repo);

    /**
     * @brief 从单个资源包加载所有配置化特征
     * @return 加载的数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

    /**
     * @brief 从 JSON 对象加载单个配置化特征
     *
     * @param jsonObj 已解析的 JSON 对象
     * @param id 特征的 ResourceLocation（对应 JSON 文件名）
     * @return 构造并已 setId 的特征，或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<ConfiguredFeatureBase>> loadFromJson(
        const nlohmann::json& jsonObj, const ResourceLocation& id);
};

} // namespace world::gen::feature
} // namespace mc
