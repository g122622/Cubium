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

#include "common/resource/ResourceLocation.hpp"
#include <string>
#include <unordered_set>
#include <vector>

namespace mc {

namespace entity {

class EntityType;

} // namespace entity

/**
 * @brief 实体类型标签
 *
 * 用于将具有相同特性的实体类型分组。
 * 内部使用 ResourceLocation 存储实体类型名称（如 "minecraft:arrow"），
 * 与 BlockTag/FluidTag 的设计模式一致。
 *
 * 用法示例:
 * @code
 * if (EntityTypeTags::IMPACT_PROJECTILES().contains(entityTypeId)) {
 *     // 该实体类型属于冲击投射物
 * }
 * @endcode
 */
class EntityTypeTag {
public:
    /**
     * @brief 构造实体类型标签
     * @param id 标签资源位置
     */
    explicit EntityTypeTag(ResourceLocation id) noexcept;

    /**
     * @brief 获取标签ID
     */
    [[nodiscard]] const ResourceLocation& getId() const { return m_id; }

    /**
     * @brief 添加实体类型到标签
     * @param entityTypeId 实体类型资源位置（如 "minecraft:arrow"）
     */
    void add(const ResourceLocation& entityTypeId);

    /**
     * @brief 批量添加实体类型
     * @param entityTypeIds 实体类型资源位置列表
     */
    void addAll(const std::vector<ResourceLocation>& entityTypeIds);

    /**
     * @brief 检查实体类型是否在标签中
     * @param entityTypeId 实体类型资源位置
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const ResourceLocation& entityTypeId) const noexcept;

    /**
     * @brief 检查实体类型是否在标签中
     * @param entityTypeId 实体类型名称字符串（如 "minecraft:arrow"）
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const std::string& entityTypeId) const;

    /**
     * @brief 检查实体类型是否在标签中
     * @param entityType 实体类型引用
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const entity::EntityType& entityType) const;

    /**
     * @brief 清除标签中的所有实体类型
     */
    void clear();

    /**
     * @brief 获取标签中的所有实体类型ID
     */
    [[nodiscard]] const std::unordered_set<ResourceLocation>& getEntityTypeIds() const { return m_entityTypeIds; }

private:
    ResourceLocation m_id;
    std::unordered_set<ResourceLocation> m_entityTypeIds;
};

} // namespace mc
