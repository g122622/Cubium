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

namespace mc::world::gen::structure {

/**
 * @brief 结构标签
 *
 * 用于将结构 ID 分组以便功能判断，对应 MC Java 的 StructureTags。
 *
 * 与 BiomeTag 不同，结构标签的成员是 ResourceLocation（结构 ID），
 * 而非 BiomeId。这是因为结构在 Cubium 中以 ResourceLocation 标识
 * （如 minecraft:shipwreck、minecraft:ocean_ruin_cold），
 * 而非整数枚举。
 *
 * 标签支持嵌套引用（#namespace:path），由 StructureTagLoader 在加载时解析。
 *
 * 用法示例:
 * @code
 * // 检查结构是否在标签中
 * if (StructureTags::DOLPHIN_LOCATED().contains(ResourceLocation("minecraft", "shipwreck"))) {
 *     // 该结构可被海豚定位
 * }
 * @endcode
 *
 * 参考: net.minecraft.tags.StructureTags (MC 1.21.11)
 */
class StructureTag {
public:
    /**
     * @brief 构造结构标签
     * @param id 标签资源位置
     * @param replace 是否替换已有标签内容（用于多数据包合并）
     */
    explicit StructureTag(ResourceLocation id, bool replace = false) noexcept;

    /**
     * @brief 获取标签 ID
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
     * @brief 添加结构到标签
     * @param structureId 结构资源位置 ID
     */
    void add(ResourceLocation structureId);

    /**
     * @brief 批量添加结构
     * @param structureIds 结构资源位置 ID 列表
     */
    void addAll(const std::vector<ResourceLocation>& structureIds);

    /**
     * @brief 清空标签中的所有结构
     *
     * 用于多数据包标签合并中 replace=true 时清空已有条目。
     */
    void clear() noexcept;

    /**
     * @brief 检查结构是否在标签中
     * @param structureId 结构资源位置 ID
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const ResourceLocation& structureId) const noexcept;

    /**
     * @brief 获取标签中的所有结构 ID
     */
    [[nodiscard]] const std::unordered_set<ResourceLocation>& getStructureIds() const { return m_structureIds; }

private:
    ResourceLocation m_id;
    bool m_replace = false;
    std::unordered_set<ResourceLocation> m_structureIds;
};

} // namespace mc::world::gen::structure

// 旧命名空间兼容别名
namespace mc {
using StructureTag = ::mc::world::gen::structure::StructureTag;
} // namespace mc
