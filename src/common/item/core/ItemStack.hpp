#pragma once

#include "../../core/Types.hpp"
#include "../../network/packet/PacketSerializer.hpp"
#include "../../core/Result.hpp"
#include "../../util/text/ITextComponent.hpp"
#include "../../util/text/StringTextComponent.hpp"
#include "../enchantment/EnchantmentContainer.hpp"
#include "../../util/nbt/Nbt.hpp"
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>

// Forward declarations
namespace mc {

// Forward declarations
class Item;
class BlockState;
class LivingEntity;
class Entity;
class IWorld;

namespace potion {
class PotionUtils;
}

} // namespace mc

namespace mc {

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

    /**
     * @brief 拷贝构造函数（深拷贝）
     */
    ItemStack(const ItemStack& other);

    /**
     * @brief 拷贝赋值运算符（深拷贝）
     */
    ItemStack& operator=(const ItemStack& other);

    /**
     * @brief 移动构造函数
     */
    ItemStack(ItemStack&& other) noexcept = default;

    /**
     * @brief 移动赋值运算符
     */
    ItemStack& operator=(ItemStack&& other) noexcept = default;

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

    /**
     * @brief 是否可堆叠
     *
     * MC 1.16.5: 物品可堆叠当且仅当最大堆叠数 > 1 且（不可损坏或未损坏）
     * 注意：有耐久度的物品通常 maxStackSize=1，所以此方法会返回false
     * @return 如果物品可以堆叠返回true
     */
    [[nodiscard]] bool isStackable() const;

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
    [[nodiscard]] i32 getEnchantmentLevel(const std::string& enchantmentId) const;

    /**
     * @brief 检查是否有指定附魔
     * @param enchantmentId 附魔ID
     */
    [[nodiscard]] bool hasEnchantment(const std::string& enchantmentId) const;

    /**
     * @brief 添加或更新附魔
     * @param enchantmentId 附魔ID
     * @param level 附魔等级
     */
    void addEnchantment(const std::string& enchantmentId, i32 level);

    // ========== 自定义数据 ==========

    /**
     * @brief 是否包含自定义标签
     * @return 如果存在任意自定义数据则返回true
     */
    [[nodiscard]] bool hasTag() const;

    /**
     * @brief 获取根自定义标签
     * @return 根标签指针，不存在时返回nullptr
     */
    [[nodiscard]] const nlohmann::json* getTag() const;

    /**
     * @brief 获取可修改的根自定义标签
     * @return 根标签指针，不存在时返回nullptr
     */
    [[nodiscard]] nlohmann::json* getTag();

    /**
     * @brief 获取或创建根自定义标签
     * @return 根标签引用
     */
    [[nodiscard]] nlohmann::json& getOrCreateTag();

    /**
     * @brief 获取子标签
     * @param name 子标签名称
     * @return 子标签指针，不存在或不是对象时返回nullptr
     */
    [[nodiscard]] const nlohmann::json* getChildTag(const std::string& name) const;

    /**
     * @brief 获取或创建子标签
     * @param name 子标签名称
     * @return 子标签引用
     */
    [[nodiscard]] nlohmann::json& getOrCreateChildTag(const std::string& name);

    /**
     * @brief 移除子标签
     * @param name 子标签名称
     */
    void removeChildTag(const std::string& name);

    /**
     * @brief 合并 JSON 标签到现有标签
     *
     * 参考 MC 1.16.5 CompoundNBT.merge() 行为：
     * - 如果两边都是对象，递归合并每个字段
     * - 否则，源值覆盖目标值
     *
     * @param other 要合并的 JSON 对象
     */
    void mergeTag(const nlohmann::json& other);

    /**
     * @brief 合并 JSON 标签到现有标签（移动语义）
     * @param other 要合并的 JSON 对象
     */
    void mergeTag(nlohmann::json&& other);

    /**
     * @brief 递归合并两个 JSON 对象
     *
     * 参考 MC 1.16.5 CompoundNBT.merge() 实现。
     * 对于对象类型的字段，递归合并；其他类型直接覆盖。
     *
     * @param target 目标 JSON 对象（会被修改）
     * @param source 源 JSON 对象
     */
    static void mergeJsonObjects(nlohmann::json& target, const nlohmann::json& source);

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

    /**
     * @brief 尝试造成伤害（带实体参数）
     *
     * MC 1.16.5: 考虑耐久保护（Unbreaking）附魔的效果。
     * 耐久附魔每级有 level/(level+1) 概率避免损耗。
     * 对于盔甲，概率减半。
     *
     * @param amount 伤害值
     * @param entity 持有该物品的实体（用于耐久保护计算）
     * @return 是否已损坏（达到最大耐久度）
     */
    bool attemptDamageItem(i32 amount, LivingEntity* entity);

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

    /**
     * @brief 物品在背包中每tick调用
     *
     * 委托给 Item::inventoryTick，用于更新地图、时钟等物品。
     * 参考: net.minecraft.item.ItemStack#inventoryTick
     *
     * @param world 世界引用
     * @param entity 持有实体（通常是玩家）
     * @param itemSlot 物品栏槽位索引
     * @param isSelected 是否为当前选中的物品
     */
    void inventoryTick(IWorld& world, Entity& entity, i32 itemSlot, bool isSelected);

    /**
     * @brief 护甲物品每tick调用
     *
     * 委托给 Item::onArmorTick，用于实现护甲特殊效果。
     * 参考: net.minecraft.item.ItemStack#onArmorTick (Forge)
     *
     * @param world 世界引用
     * @param player 穿戴护甲的玩家
     */
    void onArmorTick(IWorld& world, LivingEntity& player);

    // ========== 显示名称 ==========

    /**
     * @brief 是否有自定义名称
     *
     * 如果物品堆有自定义名称（如通过铁砧重命名），返回true。
     *
     * @return 是否有自定义名称
     */
    [[nodiscard]] bool hasCustomName() const {
        return m_customName && !m_customName->getUnformattedText().empty();
    }

    /**
     * @brief 获取自定义名称组件
     *
     * 返回自定义名称组件，如果没有则返回 nullptr。
     *
     * @return 自定义名称组件指针
     */
    [[nodiscard]] const text::ITextComponent* getCustomNameComponent() const {
        return m_customName.get();
    }

    /**
     * @brief 获取自定义名称的纯文本
     *
     * 返回自定义名称的纯文本，如果没有则返回空字符串。
     *
     * @return 自定义名称纯文本
     */
    [[nodiscard]] std::string getCustomName() const {
        return m_customName ? m_customName->getUnformattedText() : "";
    }

    /**
     * @brief 设置自定义名称组件
     * @param name 名称组件（所有权转移）
     */
    void setCustomNameComponent(std::unique_ptr<text::ITextComponent> name) {
        m_customName = std::move(name);
    }

    /**
     * @brief 设置自定义名称（纯文本，向后兼容）
     * @param name 新名称
     */
    void setCustomName(const std::string& name) {
        if (name.empty()) {
            m_customName = nullptr;
        } else {
            m_customName = std::make_unique<text::StringTextComponent>(name);
        }
    }

    /**
     * @brief 清除自定义名称
     */
    void clearCustomName() {
        m_customName = nullptr;
    }

    /**
     * @brief 是否有显示名称（自定义名称或物品翻译名称）
     */
    [[nodiscard]] bool hasDisplayName() const {
        return hasCustomName();
    }

    /**
     * @brief 获取显示名称
     *
     * 返回用于UI显示的名称。如果有自定义名称，返回自定义名称；
     * 否则返回物品的翻译键。
     *
     * @return 显示名称组件
     */
    [[nodiscard]] std::unique_ptr<text::ITextComponent> getDisplayName() const;

    // ========== Lore（物品描述） ==========

    /**
     * @brief 是否有 Lore
     * @return 如果有 Lore 返回 true
     */
    [[nodiscard]] bool hasLore() const {
        return !m_lore.empty();
    }

    /**
     * @brief 获取 Lore 列表
     * @return Lore 文本组件列表的常量引用
     */
    [[nodiscard]] const std::vector<std::unique_ptr<text::ITextComponent>>& getLore() const {
        return m_lore;
    }

    /**
     * @brief 设置 Lore
     * @param lore Lore 文本组件列表（所有权转移）
     */
    void setLore(std::vector<std::unique_ptr<text::ITextComponent>> lore) {
        m_lore = std::move(lore);
    }

    /**
     * @brief 添加一行 Lore
     * @param line Lore 文本组件（所有权转移）
     */
    void addLoreLine(std::unique_ptr<text::ITextComponent> line) {
        m_lore.push_back(std::move(line));
    }

    /**
     * @brief 添加一行 Lore（纯文本）
     * @param line Lore 纯文本
     */
    void addLoreLine(const std::string& line) {
        m_lore.push_back(std::make_unique<text::StringTextComponent>(line));
    }

    /**
     * @brief 清除 Lore
     */
    void clearLore() {
        m_lore.clear();
    }

    // ========== 堆叠兼容性检查 ==========

    /**
     * @brief 检查两个物品堆是否可以堆叠（物品类型相同且数据兼容）
     * @param other 另一个物品堆
     * @return 是否可以堆叠
     *
     * 这是canMergeWith的别名，用于与MC源码命名保持一致。
     */
    [[nodiscard]] bool canStackWith(const ItemStack& other) const {
        return canMergeWith(other);
    }

    // ========== 修复成本（铁砧） ==========

    /**
     * @brief 获取修复成本
     * @return 修复成本
     *
     * 参考: net.minecraft.item.ItemStack.getRepairCost()
     */
    [[nodiscard]] i32 getRepairCost() const { return m_repairCost; }

    /**
     * @brief 设置修复成本
     * @param cost 修复成本
     *
     * 参考: net.minecraft.item.ItemStack.setRepairCost()
     */
    void setRepairCost(i32 cost) { m_repairCost = cost; }

    // ========== 容器物品 ==========

    /**
     * @brief 获取容器物品堆
     * @return 容器物品堆，如果没有则返回空堆
     *
     * 参考: net.minecraft.item.ItemStack.getContainerItem()
     * 例如：牛奶桶用完后返回空桶
     */
    [[nodiscard]] ItemStack getContainerItem() const;

    /**
     * @brief 是否有容器物品
     * @return 如果物品有容器物品返回true
     */
    [[nodiscard]] bool hasContainerItem() const;

    // ========== 序列化 ==========

    /**
     * @brief 序列化到网络包
     */
    void serialize(network::PacketSerializer& ser) const;

    /**
     * @brief 从网络包反序列化
     */
    [[nodiscard]] static Result<ItemStack> deserialize(network::PacketDeserializer& deser);

    /**
     * @brief 序列化到 JSON
     * @return JSON 对象
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 从 JSON 反序列化
     * @param json JSON 对象
     * @return 物品堆
     */
    [[nodiscard]] static Result<ItemStack> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化到 NBT
     * @param tag NBT 复合标签（输出参数）
     * @return NBT 复合标签引用
     *
     * 参考 MC 1.16.5 ItemStack.write()
     * NBT 格式：
     * - id (string): 物品资源位置
     * - Count (byte): 数量
     * - tag (compound, 可选): 物品标签
     *   - Damage (int): 耐久度
     *   - Enchantments (list): 附魔
     *   - display (compound): 显示数据
     *     - Name: 自定义名称
     *     - Lore: 描述
     *   - RepairCost (int): 修复成本
     *   - Potion (string): 药水ID
     */
    void toNbt(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从 NBT 反序列化
     * @param tag NBT 复合标签
     * @return 物品堆或错误
     */
    [[nodiscard]] static Result<ItemStack> fromNbt(const nbt::tags::compound_tag& tag);

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
    i32 m_repairCost = 0;   // 修复成本（铁砧）
    std::unique_ptr<text::ITextComponent> m_customName;  // 自定义名称（铁砧重命名）
    std::vector<std::unique_ptr<text::ITextComponent>> m_lore;  // 物品描述（Lore）
    item::enchant::EnchantmentContainer m_enchantments;  // 附魔容器
    std::string m_potionId;      // 药水ID（用于药水物品）
    nlohmann::json m_customData;  // 自定义数据（用于display等扩展标签）

    // 允许 PotionUtils 访问私有成员
    friend class potion::PotionUtils;
};

} // namespace mc
