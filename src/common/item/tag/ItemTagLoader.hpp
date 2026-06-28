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

#include "ItemTag.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <string>

namespace mc {

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace item::tag {

/**
 * @brief 物品标签 JSON 加载器
 *
 * 从数据包加载物品标签 JSON 文件。
 *
 * JSON 格式:
 * {
 *   "replace": false,
 *   "values": [
 *     "minecraft:diamond",
 *     "#minecraft:arrows"
 *   ]
 * }
 *
 * values 中以 # 开头的条目表示引用其他标签。
 *
 * 加载路径: data/<namespace>/tags/item/
 *
 * 多数据包合并语义：
 * - 默认追加模式：新数据包的标签内容追加到已有标签内容之后
 * - replace=true 时：清空已有标签内容，仅使用当前数据包的内容
 * - 数据包优先级从低到高遍历
 */
class ItemTagLoader {
public:
    /**
     * @brief 从数据包仓库加载所有物品标签
     *
     * 遍历所有命名空间下的 tags/item/ 目录，
     * 加载所有标签 JSON 文件并注册到 ItemTags。
     *
     * 必须在 ItemTags::initialize() 之后调用，以确保内置标签已注册。
     *
     * @param dataPackList 数据包仓库
     * @return 加载的标签数量
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载所有物品标签
     *
     * 与 loadFromDataPackRepository() 不同，此方法仅加载单个资源包中的标签，
     * 不支持多数据包合并语义。适用于测试或单包加载场景。
     * 主加载流程应使用 loadFromDataPackRepository()。
     *
     * @param pack 资源包
     * @return 加载的标签数量
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

    /**
     * @brief 从 JSON 字符串加载单个物品标签
     *
     * @param json JSON 内容
     * @param location 标签资源位置
     * @return 解析的标签，或错误信息
     */
    [[nodiscard]] static Result<std::unique_ptr<ItemTag>> loadFromJson(
        const std::string& json, const ResourceLocation& location);
};

} // namespace item::tag
} // namespace mc
