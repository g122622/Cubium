#pragma once

#include "../core/Types.hpp"
#include "../network/packet/PacketSerializer.hpp"
#include "../core/Result.hpp"
#include "enchantment/EnchantmentContainer.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declarations
class Item;
class BlockState;

/**
 * @brief 物品堆
 *
 * 表示游戏中的一个物品实例，包含物品类型、数量和额外数据（耐久、附魔等）。
 * ItemStack是不可变的值类型，修改操作返回新的ItemStack。
 *
 * 参考: net.minecraft.item.ItemStack
 *
 * 关键概念：
 * - 空堆（Empty）：item为nullptr或count为0，isEmpty()返回true
 * - 堆叠限制：同一物品可以堆叠到maxStackSize，受耐久度影响
 * - 分割：split()方法从堆中分离指定数量
 * - 合并：canMergeWith()检查是否可合并，grow/shrink调整数量
 */
class ItemStack {
public:
    /**
     * @brief 空物品堆常量
     *
     * 表示空的物品堆，用于表示"无物品"状态。
     */
    static const ItemStack EMPTY;

    /**
     * @brief 默认构造函数（创建空物品堆）
     */
    ItemStack() = default;

    /**
     * @brief 构造物品堆
     * @param item 物品类型
     * @param count 数量（默认1）
     */
    explicit ItemStack(const Item& item, i32 count = 1);

    /**
     * @brief 从物品指针构造
     * @param item 物品指针（可为nullptr表示空）
     * @param count 数量（默认1）
     */
    explicit ItemStack(const Item* item, i32 count = 1);

    // ========== 基本属性 ==========

    /**
     * @brief 是否为空物品堆
     *
     * 空堆的条件：
     * - item为nullptr
     * - count为0或负数
     */
    [[nodiscard]] bool isEmpty() const { return m_item == nullptr || m_count <= 0; }

    /**
     * @brief 获取物品
     * @return 物品指针，空堆返回nullptr
     */
    [[nodiscard]] const Item* getItem() const { return m_item; }

    /**
     * @brief 获取数量
     */
    [[nodiscard]] i32 getCount() const { return m_count; }

    /**
     * @brief 设置数量
     * @param count 新数量
     * @note 数量<=0会使堆变为空
     */
    void setCount(i32 count);

    /**
     * @brief 增加数量
     * @param amount 增加量（可为负数）
     */
    void grow(i32 amount) { setCount(m_count + amount); }

    /**
     * @brief 减少数量
     * @param amount 减少量
     */
    void shrink(i32 amount) { setCount(m_count - amount); }

    /**
     * @brief 获取最大堆叠数量
     */
    [[nodiscard]] i32 getMaxStackSize() const;

    // ========== 附魔 ==========

    /**
     * @brief 获取附魔容器
     * @return 附魔容器的常量引用
     */
    [[nodiscard]] const item::enchant::EnchantmentContainer& getEnchantments() const { return m_enchantments; }

    /**
     * @brief 获取可修改的附魔容器
     * @return 附魔容器的引用
     */
    item::enchant::EnchantmentContainer& getEnchantmentsMutable() { return m_enchantments; }

    /**
     * @brief 是否有附魔
     */
    [[nodiscard]] bool hasEnchantments() const { return !m_enchantments.isEmpty(); }

    /**
     * @brief 获取指定附魔的等级
     * @param enchantmentId 附魔ID
     * @return 附魔等级（0表示无此附魔）
     */
    [[nodiscard]] i32 getEnchantmentLevel(const String& enchantmentId) const;

    /**
     * @brief 检查是否有指定附魔
     * @param enchantmentId 附魔ID
     */
    [[nodiscard]] bool hasEnchantment(const String& enchantmentId) const;

    /**
     * @brief 添加或更新附魔
     * @param enchantmentId 附魔ID
     * @param level 附魔等级
     */
    void addEnchantment(const String& enchantmentId, i32 level);

    // ========== 耐久度 ==========

    /**
     * @brief 是否可损坏
     */
    [[nodiscard]] bool isDamageable() const;

    /**
     * @brief 是否已损坏
     */
    [[nodiscard]] bool isDamaged() const;

    /**
     * @brief 获取当前耐久度（已承受的伤害）
     */
    [[nodiscard]] i32 getDamage() const { return m_damage; }

    /**
     * @brief 设置当前耐久度
     * @param damage 已承受的伤害值
     */
    void setDamage(i32 damage);

    /**
     * @brief 获取最大耐久度
     */
    [[nodiscard]] i32 getMaxDamage() const;

    /**
     * @brief 尝试造成伤害
     * @param amount 伤害值
     * @return 是否已损坏（达到最大耐久度）
     */
    bool attemptDamageItem(i32 amount);

    // ========== 堆叠操作 ==========

    /**
     * @brief 检查是否可以与另一个堆合并
     * @param other 另一个物品堆
     * @return 是否可以合并
     *
     * 合并条件：
     * - 物品类型相同
     * - 当前堆未满
     * - 两堆都没有耐久度或耐久度相同
     */
    [[nodiscard]] bool canMergeWith(const ItemStack& other) const;

    /**
     * @brief 检查物品类型是否相同
     * @param other 另一个物品堆
     * @return 物品类型是否相同
     */
    [[nodiscard]] bool isSameItem(const ItemStack& other) const;

    /**
     * @brief 从当前堆分割出指定数量
     * @param amount 要分割的数量
     * @return 新的物品堆（包含分割的数量）
     *
     * 分割后当前堆数量减少。
     * 如果amount >= 当前数量，返回当前堆的副本，当前堆变为空。
     */
    ItemStack split(i32 amount);

    /**
     * @brief 复制物品堆
     * @return 完全相同的副本
     */
    [[nodiscard]] ItemStack copy() const;

    // ========== 物品功能 ==========

    /**
     * @brief 获取挖掘速度
     * @param state 目标方块状态
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 getDestroySpeed(const BlockState& state) const;

    /**
     * @brief 是否可以采集方块
     * @param state 目标方块状态
     * @return 是否可以采集
     */
    [[nodiscard]] bool canHarvestBlock(const BlockState& state) const;

    // ========== 显示名称 ==========

    /**
     * @brief 是否有自定义名称
     *
     * 如果物品堆有自定义名称（如通过铁砧重命名），返回true。
     * 目前返回false，未来可支持NBT标签。
     *
     * @return 是否有自定义名称
     */
    [[nodiscard]] bool hasCustomName() const { return !m_customName.empty(); }

    /**
     * @brief 获取自定义名称
     *
     * 返回自定义名称，如果没有则返回空字符串。
     *
     * @return 自定义名称
     */
    [[nodiscard]] String getCustomName() const { return m_customName; }

    /**
     * @brief 设置自定义名称
     * @param name 新名称
     */
    void setCustomName(const String& name) { m_customName = name; }

    /**
     * @brief 获取显示名称
     *
     * 返回用于UI显示的名称。如果有自定义名称，返回自定义名称；
     * 否则返回物品的翻译键。
     *
     * @return 显示名称
     */
    [[nodiscard]] String getDisplayName() const;

    // ========== 堆叠兼容性检查 ==========

    /**
     * @brief 检查两个物品堆是否可以堆叠（物品类型相同且数据兼容）
     * @param other 另一个物品堆
     * @return 是否可以堆叠
     *
     * 这是canMergeWith的别名，用于与MC源码命名保持一致。
     */
    [[nodiscard]] bool canStackWith(const ItemStack& other) const {
        return isSameItem(other) && m_damage == other.m_damage;
    }

    // ========== 序列化 ==========

    /**
     * @brief 序列化到网络包
     */
    void serialize(network::PacketSerializer& ser) const;

    /**
     * @brief 从网络包反序列化
     */
    [[nodiscard]] static Result<ItemStack> deserialize(network::PacketDeserializer& deser);

    // ========== 比较操作符 ==========

    /**
     * @brief 物品堆相等比较
     *
     * 比较物品类型、数量和耐久度。
     * 空堆与空堆相等。
     */
    bool operator==(const ItemStack& other) const;

    bool operator!=(const ItemStack& other) const {
        return !(*this == other);
    }

private:
    const Item* m_item = nullptr;
    i32 m_count = 0;
    i32 m_damage = 0;       // 已承受的伤害（耐久度）
    String m_customName;    // 自定义名称（铁砧重命名）
    item::enchant::EnchantmentContainer m_enchantments;  // 附魔容器
};

} // namespace mc
