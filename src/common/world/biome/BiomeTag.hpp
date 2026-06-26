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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <unordered_set>
#include <vector>

namespace mc::world::biome {

/**
 * @brief 生物群系标签
 *
 * 用于将生物群系ID分组以便功能判断，
 * 例如判断某个生物群系是否可以生成村庄、要塞等结构。
 *
 * 用法示例:
 * @code
 * // 检查生物群系是否在标签中
 * if (BiomeTags::HAS_STRUCTURE_VILLAGE_PLAINS().contains(biomeId)) {
 *     // 该生物群系可以生成平原村庄
 * }
 * @endcode
 */
class BiomeTag {
public:
    /**
     * @brief 构造生物群系标签
     * @param id 标签资源位置
     * @param replace 是否替换已有标签内容（用于多数据包合并）
     */
    explicit BiomeTag(ResourceLocation id, bool replace = false) noexcept;

    /**
     * @brief 获取标签ID
     */
    [[nodiscard]] const ResourceLocation& getId() const { return m_id; }

    /**
     * @brief 是否替换已有标签内容
     *
     * 在多数据包标签合并中，replace=true 表示清空之前数据包的标签条目后追加，
     * replace=false（默认）表示追加到已有条目。
     */
    [[nodiscard]] bool isReplace() const noexcept { return m_replace; }

    /**
     * @brief 设置替换标志
     */
    void setReplace(bool replace) noexcept { m_replace = replace; }

    /**
     * @brief 添加生物群系到标签
     * @param biomeId 生物群系ID
     */
    void add(BiomeId biomeId);

    /**
     * @brief 批量添加生物群系
     * @param biomeIds 生物群系ID列表
     */
    void addAll(const std::vector<BiomeId>& biomeIds);

    /**
     * @brief 清空标签中的所有生物群系
     *
     * 用于多数据包标签合并中 replace=true 时清空已有条目。
     */
    void clear() noexcept;

    /**
     * @brief 检查生物群系是否在标签中
     * @param biomeId 生物群系ID
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(BiomeId biomeId) const noexcept;

    /**
     * @brief 获取标签中的所有生物群系ID
     */
    [[nodiscard]] const std::unordered_set<BiomeId>& getBiomeIds() const { return m_biomeIds; }

private:
    ResourceLocation m_id;
    bool m_replace = false;
    std::unordered_set<BiomeId> m_biomeIds;
};

} // namespace mc::world::biome

// 旧命名空间兼容别名
namespace mc {
using BiomeTag = ::mc::world::biome::BiomeTag;
} // namespace mc
