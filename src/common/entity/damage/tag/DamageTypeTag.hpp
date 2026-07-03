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

#include "common/entity/damage/DamageSource.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace mc {

/**
 * @brief 伤害类型标签
 *
 * 用于将具有相同特性的伤害类型分组，对应 MC 1.21.11 的 DamageTypeTags 系统。
 *
 * 与 EntityTypeTag（基于 ResourceLocation）不同，DamageTypeTag 内部使用
 * DamageType 枚举存储成员，因为 DamageType 是项目内置的强类型枚举，
 * 使用枚举存储可提供类型安全且性能更高（避免字符串比较和 ResourceLocation 解析）。
 *
 * 数据包加载时，通过字符串名（如 "minecraft:drown"）到 DamageType 枚举的映射
 * 完成解析，映射由 DamageTypeNames 命名空间提供。
 *
 * 用法示例:
 * @code
 * if (DamageTypeTags::BYPASSES_WOLF_ARMOR().contains(source.type())) {
 *     // 该伤害绕过狼铠
 * }
 * // 或通过 DamageSource::is() 方法
 * if (source.is(DamageTypeTags::BYPASSES_WOLF_ARMOR())) {
 *     // 该伤害绕过狼铠
 * }
 * @endcode
 *
 * 参考: net.minecraft.tags.DamageTypeTags (MC 1.21.11)
 */
class DamageTypeTag {
public:
    /**
     * @brief 构造伤害类型标签
     * @param id 标签资源位置
     */
    explicit DamageTypeTag(ResourceLocation id) noexcept;

    /**
     * @brief 获取标签ID
     */
    [[nodiscard]] const ResourceLocation& getId() const { return m_id; }

    /**
     * @brief 添加伤害类型到标签
     * @param type 伤害类型枚举
     */
    void add(DamageType type);

    /**
     * @brief 批量添加伤害类型
     * @param types 伤害类型列表
     */
    void addAll(const std::vector<DamageType>& types);

    /**
     * @brief 添加伤害类型到标签（按资源位置，用于数据包加载）
     * @param typeId 伤害类型资源位置（如 "minecraft:drown"）
     * @return 是否成功添加（若资源位置无法映射到 DamageType 则返回 false）
     */
    bool addByResourceLocation(const ResourceLocation& typeId);

    /**
     * @brief 检查伤害类型是否在标签中
     * @param type 伤害类型枚举
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(DamageType type) const noexcept;

    /**
     * @brief 检查伤害源的伤害类型是否在标签中
     * @param source 伤害源
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const DamageSource& source) const;

    /**
     * @brief 检查伤害类型是否在标签中（按资源位置）
     * @param typeId 伤害类型资源位置
     * @return 是否在标签中
     */
    [[nodiscard]] bool containsByResourceLocation(const ResourceLocation& typeId) const;

    /**
     * @brief 清除标签中的所有伤害类型
     */
    void clear();

    /**
     * @brief 获取标签中的所有伤害类型
     */
    [[nodiscard]] const std::unordered_set<DamageType>& getDamageTypes() const { return m_damageTypes; }

private:
    ResourceLocation m_id;
    std::unordered_set<DamageType> m_damageTypes;
};

/**
 * @brief 伤害类型名称映射工具
 *
 * 提供 DamageType 枚举与 MC 1.21.11 资源位置名（如 "minecraft:drown"）之间的双向映射，
 * 供 DamageTypeTag 数据包加载和序列化使用。
 *
 * 参考: net.minecraft.world.damagesource.DamageTypes (MC 1.21.11)
 */
namespace DamageTypeNames {
/**
 * @brief 根据 DamageType 枚举获取资源位置
 * @param type 伤害类型枚举
 * @return 资源位置（如 "minecraft:drown"），未知类型返回空 ResourceLocation
 */
[[nodiscard]] ResourceLocation getResourceLocation(DamageType type);

/**
 * @brief 根据资源位置获取 DamageType 枚举
 * @param location 资源位置（如 "minecraft:drown"）
 * @return 伤害类型枚举，未找到返回 std::nullopt
 */
[[nodiscard]] std::optional<DamageType> fromResourceLocation(const ResourceLocation& location);

/**
 * @brief 根据字符串获取 DamageType 枚举
 * @param name 伤害类型名（如 "minecraft:drown" 或 "drown"）
 * @return 伤害类型枚举，未找到返回 std::nullopt
 */
[[nodiscard]] std::optional<DamageType> fromString(const std::string& name);
} // namespace DamageTypeNames

} // namespace mc
