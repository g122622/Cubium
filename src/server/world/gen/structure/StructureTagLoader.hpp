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

#include "StructureTag.hpp"
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

namespace world::gen::structure {

/**
 * @brief 结构标签 JSON 加载器
 *
 * 从数据包加载结构标签 JSON 文件，对应 MC Java 的 StructureTagsProvider。
 *
 * JSON 格式:
 * @code
 * {
 *   "replace": false,
 *   "values": [
 *     "minecraft:shipwreck",
 *     "minecraft:shipwreck_beached",
 *     "#minecraft:ocean_ruin"
 *   ]
 * }
 * @endcode
 *
 * values 中以 # 开头的条目表示引用其他结构标签（标签嵌套），
 * 加载时会递归解析引用标签的成员。
 *
 * 加载路径: data/<namespace>/tags/worldgen/structure/
 *
 * 多数据包合并语义（对齐 MC Java TagLoader.load()）：
 * - 按数据包优先级从低到高遍历同名标签文件
 * - replace=true：清空已有条目后追加当前数据包的内容
 * - replace=false（默认）：追加当前数据包的内容
 *
 * 参考: net.minecraft.tags.TagLoader (MC 1.21.11)
 */
class StructureTagLoader {
public:
    /**
     * @brief 从数据包列表加载所有结构标签
     *
     * 遍历所有命名空间下的 tags/worldgen/structure/ 目录，
     * 加载所有标签 JSON 文件并合并到 StructureTags 注册表。
     *
     * 必须在 StructureTags::initialize() 之后调用，否则数据包中的
     * 新标签会被丢弃（与 BiomeTagLoader 行为一致）。
     *
     * @param dataPackRepository 数据包仓库
     * @return 加载的标签数量
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(
        const resource::DataPackRepository& dataPackRepository);

    /**
     * @brief 从单个资源包加载所有结构标签
     *
     * @param pack 资源包
     * @return 加载的标签数量
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

    /**
     * @brief 从 JSON 字符串加载单个结构标签
     *
     * @param json JSON 内容
     * @param location 标签资源位置
     * @return 解析的标签，或错误信息
     */
    [[nodiscard]] static Result<std::unique_ptr<StructureTag>> loadFromJson(
        const std::string& json, const ResourceLocation& location);
};

} // namespace world::gen::structure

} // namespace mc
