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

#include <memory>
#include <string_view>

#include "Enchantment.hpp"
#include "common/core/Result.hpp"
#include "common/network/codec/PacketSerializer.hpp"
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>

// Forward declaration
namespace mc {
namespace nbt {
namespace tags {
struct compound_tag;
struct list_tag;
} // namespace tags
} // namespace nbt
} // namespace mc

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 附魔实例
 *
 * 存储附魔ID和等级。
 */
struct EnchantmentInstance {
    std::string enchantmentId; ///< 附魔ID
    i32 level = 1;             ///< 附魔等级

    EnchantmentInstance() = default;

    EnchantmentInstance(std::string_view id, i32 lvl)
        : enchantmentId(id)
        , level(lvl)
    {}

    /**
     * @brief 获取附魔定义
     * @return 附魔指针，如果未注册返回nullptr
     */
    [[nodiscard]] const Enchantment* getEnchantment() const;

    // 比较操作符
    bool operator==(const EnchantmentInstance& other) const
    {
        return enchantmentId == other.enchantmentId && level == other.level;
    }

    bool operator!=(const EnchantmentInstance& other) const { return !(*this == other); }
};

/**
 * @brief 附魔存储容器
 *
 * 存储物品上的所有附魔。
 * 支持：
 * - 添加/移除附魔
 * - 查询附魔等级
 * - 序列化/反序列化
 */
class EnchantmentContainer {
public:
    EnchantmentContainer() noexcept = default;

    // ========== 查询 ==========

    /**
     * @brief 是否有任何附魔
     */
    [[nodiscard]] bool isEmpty() const { return m_enchantments.empty(); }

    /**
     * @brief 获取附魔数量
     */
    [[nodiscard]] size_t size() const { return m_enchantments.size(); }

    /**
     * @brief 获取指定附魔的等级
     * @param enchantmentId 附魔ID
     * @return 附魔等级（0表示无此附魔）
     */
    [[nodiscard]] i32 getLevel(std::string_view enchantmentId) const;

    /**
     * @brief 检查是否有指定附魔
     * @param enchantmentId 附魔ID
     */
    [[nodiscard]] bool has(std::string_view enchantmentId) const;

    /**
     * @brief 检查是否有指定类型的附魔
     * @param type 附魔类型
     */
    [[nodiscard]] bool hasType(EnchantmentType type) const;

    /**
     * @brief 获取所有附魔
     */
    [[nodiscard]] const std::vector<EnchantmentInstance>& getAll() const { return m_enchantments; }

    // ========== 修改 ==========

    /**
     * @brief 添加或更新附魔
     * @param enchantmentId 附魔ID
     * @param level 附魔等级
     */
    void set(std::string_view enchantmentId, i32 level);

    /**
     * @brief 移除附魔
     * @param enchantmentId 附魔ID
     * @return 如果成功移除返回true
     */
    bool remove(std::string_view enchantmentId);

    /**
     * @brief 清除所有附魔
     */
    void clear() { m_enchantments.clear(); }

    // ========== 兼容性检查 ==========

    /**
     * @brief 检查是否可以添加指定附魔
     * @param enchantmentId 附魔ID
     * @return 如果与现有附魔兼容返回true
     */
    [[nodiscard]] bool canAdd(std::string_view enchantmentId) const;

    // ========== 序列化 ==========

    /**
     * @brief 序列化到网络包
     */
    void serialize(network::PacketSerializer& ser) const;

    /**
     * @brief 从网络包反序列化
     */
    [[nodiscard]] static Result<EnchantmentContainer> deserialize(network::PacketDeserializer& deser);

    /**
     * @brief 序列化到 JSON
     * @return JSON 数组
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 从 JSON 反序列化
     * @param json JSON 数组
     * @return 附魔容器
     */
    [[nodiscard]] static Result<EnchantmentContainer> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化到 NBT
     * @return NBT 列表标签
     *
     * MC 1.16.5 附魔格式：
     * - id (string): 附魔ID
     * - lvl (short): 附魔等级
     */
    [[nodiscard]] std::unique_ptr<nbt::tags::list_tag> toNbt() const;

    /**
     * @brief 从 NBT 反序列化
     * @param list NBT 列表标签
     * @return 附魔容器
     */
    [[nodiscard]] static EnchantmentContainer fromNbt(const nbt::tags::list_tag& list);

    // ========== 比较操作符 ==========

    /**
     * @brief 比较两个附魔容器是否相等
     */
    bool operator==(const EnchantmentContainer& other) const
    {
        if (m_enchantments.size() != other.m_enchantments.size()) {
            return false;
        }
        for (size_t i = 0; i < m_enchantments.size(); ++i) {
            if (m_enchantments[i] != other.m_enchantments[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const EnchantmentContainer& other) const { return !(*this == other); }

private:
    std::vector<EnchantmentInstance> m_enchantments;
};

} // namespace enchant
} // namespace item
} // namespace mc
