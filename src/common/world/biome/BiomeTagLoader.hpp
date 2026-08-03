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

#include "BiomeTag.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace mc {

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace world::biome {

/**
 * @brief 生物群系标签 JSON 加载器
 *
 * 从数据包加载生物群系标签 JSON 文件。
 *
 * JSON 格式:
 * {
 *   "replace": false,
 *   "values": [
 *     "minecraft:desert",
 *     "minecraft:wooded_hills",
 *     "#minecraft:is_jungle"
 *   ]
 * }
 *
 * values 中以 # 开头的条目表示引用其他标签。
 *
 * 加载路径: data/<namespace>/tags/worldgen/biome/has_structure/
 */
class BiomeTagLoader {
public:
    /**
     * @brief 从数据包列表加载所有生物群系标签
     *
     * 遍历所有命名空间下的 tags/worldgen/biome/ 目录，
     * 加载所有标签 JSON 文件并注册到 BiomeTags。
     *
     * @param dataPackList 数据包列表
     * @return 加载的标签数量
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载所有生物群系标签
     *
     * @param pack 资源包
     * @return 加载的标签数量
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

    /**
     * @brief 从 JSON 字符串加载单个生物群系标签
     *
     * @param json JSON 内容
     * @param location 标签资源位置
     * @return 解析的标签，或错误信息
     */
    [[nodiscard]] static Result<std::unique_ptr<BiomeTag>> loadFromJson(
        const std::string& json, const ResourceLocation& location);
};

} // namespace world::biome

} // namespace mc
