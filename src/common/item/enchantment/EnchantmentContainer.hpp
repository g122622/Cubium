#pragma once

#include "Enchantment.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/core/Result.hpp"
#include <vector>
#include <utility>
#include <nlohmann/json.hpp>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 附魔实例
 *
 * 存储附魔ID和等级。
 */
struct EnchantmentInstance {
    String enchantmentId;   ///< 附魔ID
    i32 level = 1;          ///< 附魔等级

    EnchantmentInstance() = default;

    EnchantmentInstance(const String& id, i32 lvl)
        : enchantmentId(id), level(lvl) {}

    /**
     * @brief 获取附魔定义
     * @return 附魔指针，如果未注册返回nullptr
     */
    [[nodiscard]] const Enchantment* getEnchantment() const;

    // 比较操作符
    bool operator==(const EnchantmentInstance& other) const {
        return enchantmentId == other.enchantmentId && level == other.level;
    }

    bool operator!=(const EnchantmentInstance& other) const {
        return !(*this == other);
    }
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
    EnchantmentContainer() = default;

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
    [[nodiscard]] i32 getLevel(const String& enchantmentId) const;

    /**
     * @brief 检查是否有指定附魔
     * @param enchantmentId 附魔ID
     */
    [[nodiscard]] bool has(const String& enchantmentId) const;

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
    void set(const String& enchantmentId, i32 level);

    /**
     * @brief 移除附魔
     * @param enchantmentId 附魔ID
     * @return 如果成功移除返回true
     */
    bool remove(const String& enchantmentId);

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
    [[nodiscard]] bool canAdd(const String& enchantmentId) const;

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

    // ========== 比较操作符 ==========

    /**
     * @brief 比较两个附魔容器是否相等
     */
    bool operator==(const EnchantmentContainer& other) const {
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

    bool operator!=(const EnchantmentContainer& other) const {
        return !(*this == other);
    }

private:
    std::vector<EnchantmentInstance> m_enchantments;
};

} // namespace enchant
} // namespace item
} // namespace mc
