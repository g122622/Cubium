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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
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
class BlockPos;
class Player;
class DamageSource;

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
     * 物品可堆叠当且仅当最大堆叠数 > 1 且（不可损坏或未损坏）。
     * 注意：有耐久度的物品通常 maxStackSize=1，所以此方法会返回false。
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
     * 如果两边都是对象，递归合并每个字段；否则，源值覆盖目标值。
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
     * @brief 检查此物品堆是否可以被指定伤害源伤害
     *
     * 防火物品（如下界合金物品、下界星）不会被火焰和岩浆伤害源摧毁。
     *
     * @param source 伤害源
     * @return 如果物品可以被此伤害源伤害返回 true，否则返回 false
     */
    [[nodiscard]] bool canBeHurtBy(const DamageSource& source) const;

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
     * @brief 尝试造成伤害（无实体上下文）
     *
     * 考虑耐久保护（Unbreaking）附魔的效果。
     * 当没有实体上下文时，使用线程局部静态随机数生成器。
     * 优先使用带实体参数的重载版本，以获取世界关联的随机源并触发进度事件。
     *
     * @param amount 伤害值
     * @return 是否已损坏（达到最大耐久度）
     */
    bool attemptDamageItem(i32 amount);

    /**
     * @brief 尝试造成伤害（带实体参数）
     *
     * 考虑耐久保护（Unbreaking）附魔的效果。
     * 使用实体所在世界的随机数生成器进行耐久保护概率计算，
     * 与 MC 原版行为一致（原版使用 ServerLevel.getRandom()）。
     * 当实体为空时退回到线程局部静态随机源。
     *
     * @param amount 伤害值
     * @param entity 持有该物品的实体（用于获取世界随机源和触发进度事件）
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

    /**
     * @brief 转化物品堆类型（保留 NBT/组件）
     *
     * 对应 MC 1.21.11 ItemStack#transmuteCopy(Item, int)。
     * 创建新的物品堆，使用指定的物品类型和数量，但保留原物品堆的所有
     * 额外数据（自定义名称、Lore、附魔、自定义 NBT 数据、冒险模式谓词等）。
     *
     * 用于转化配方（crafting_transmute），如收纳袋染色：将无色收纳袋
     * 转化为有色收纳袋时，保留 BundleContents 内容物。
     *
     * @param newItem 新物品类型
     * @param newCount 新数量（默认 1）
     * @return 转化后的物品堆
     */
    [[nodiscard]] ItemStack transmuteCopy(const Item& newItem, i32 newCount = 1) const;

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
     *
     * @param world 世界引用
     * @param player 穿戴护甲的玩家
     */
    void onArmorTick(IWorld& world, LivingEntity& player);

    /**
     * @brief 物品被玩家合成时调用
     *
     * 委托给 Item::onCraftedBy，用于执行物品合成后的特殊初始化。
     * 例如地图物品通过此回调处理缩放和锁定后处理。
     *
     * @param player 合成物品的玩家
     * @param amount 合成数量
     */
    void onCraftedBy(Player& player, i32 amount);

    // ========== 显示名称 ==========

    /**
     * @brief 是否有自定义名称
     *
     * 如果物品堆有自定义名称（如通过铁砧重命名），返回true。
     *
     * @return 是否有自定义名称
     */
    [[nodiscard]] bool hasCustomName() const { return m_customName && !m_customName->getUnformattedText().empty(); }

    /**
     * @brief 获取自定义名称组件
     *
     * 返回自定义名称组件，如果没有则返回 nullptr。
     *
     * @return 自定义名称组件指针
     */
    [[nodiscard]] const text::ITextComponent* getCustomNameComponent() const { return m_customName.get(); }

    /**
     * @brief 获取自定义名称的纯文本
     *
     * 返回自定义名称的纯文本，如果没有则返回空字符串。
     *
     * @return 自定义名称纯文本
     */
    [[nodiscard]] std::string getCustomName() const { return m_customName ? m_customName->getUnformattedText() : ""; }

    /**
     * @brief 设置自定义名称组件
     * @param name 名称组件（所有权转移）
     */
    void setCustomNameComponent(std::unique_ptr<text::ITextComponent> name) { m_customName = std::move(name); }

    /**
     * @brief 设置自定义名称（纯文本，向后兼容）
     * @param name 新名称
     */
    void setCustomName(const std::string& name)
    {
        if (name.empty()) {
            m_customName = nullptr;
        } else {
            m_customName = std::make_unique<text::StringTextComponent>(name);
        }
    }

    /**
     * @brief 清除自定义名称
     */
    void clearCustomName() { m_customName = nullptr; }

    /**
     * @brief 是否有显示名称（自定义名称或物品翻译名称）
     */
    [[nodiscard]] bool hasDisplayName() const { return hasCustomName(); }

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
    [[nodiscard]] bool hasLore() const { return !m_lore.empty(); }

    /**
     * @brief 获取 Lore 列表
     * @return Lore 文本组件列表的常量引用
     */
    [[nodiscard]] const std::vector<std::unique_ptr<text::ITextComponent>>& getLore() const { return m_lore; }

    /**
     * @brief 设置 Lore
     * @param lore Lore 文本组件列表（所有权转移）
     */
    void setLore(std::vector<std::unique_ptr<text::ITextComponent>> lore) { m_lore = std::move(lore); }

    /**
     * @brief 添加一行 Lore
     * @param line Lore 文本组件（所有权转移）
     */
    void addLoreLine(std::unique_ptr<text::ITextComponent> line) { m_lore.push_back(std::move(line)); }

    /**
     * @brief 添加一行 Lore（纯文本）
     * @param line Lore 纯文本
     */
    void addLoreLine(const std::string& line) { m_lore.push_back(std::make_unique<text::StringTextComponent>(line)); }

    /**
     * @brief 清除 Lore
     */
    void clearLore() { m_lore.clear(); }

    // ========== 冒险模式谓词（CanPlaceOn / CanDestroy） ==========

    /**
     * @brief 是否有 CanPlaceOn 谓词
     *
     * 冒险模式下，玩家只能将此物品放置在 CanPlaceOn 列表中的方块上。
     *
     * @return 如果有 CanPlaceOn 标签返回 true
     */
    [[nodiscard]] bool hasCanPlaceOn() const { return !m_canPlaceOn.isEmpty(); }

    /**
     * @brief 获取 CanPlaceOn 谓词
     * @return CanPlaceOn 谓词的常量引用
     */
    [[nodiscard]] const AdventureModePredicate& getCanPlaceOn() const { return m_canPlaceOn; }

    /**
     * @brief 设置 CanPlaceOn 谓词
     * @param predicate 冒险模式谓词
     */
    void setCanPlaceOn(AdventureModePredicate predicate) { m_canPlaceOn = std::move(predicate); }

    /**
     * @brief 检查此物品是否可以在冒险模式下放置在指定方块上
     *
     * 对应 MC Java 的 ItemStack.canPlaceOnBlockInAdventureMode()。
     * 如果物品没有 CanPlaceOn 标签，返回 false。
     * 如果有 CanPlaceOn 标签，检查目标方块是否匹配任一谓词。
     *
     * @param state 目标方块状态
     * @return 如果允许放置返回 true
     */
    [[nodiscard]] bool canPlaceOnBlockInAdventureMode(const BlockState& state) const;

    /**
     * @brief 检查此物品是否可以在冒险模式下放置在指定方块上（带世界参数）
     *
     * 不支持方块实体NBT匹配，仅检查方块状态。
     * 如需NBT匹配，请使用带 BlockPos 的重载版本。
     *
     * @param world 世界引用
     * @param state 目标方块状态
     * @return 如果允许放置返回 true
     */
    [[nodiscard]] bool canPlaceOnBlockInAdventureMode(IWorld& world, const BlockState& state) const;

    /**
     * @brief 检查此物品是否可以在冒险模式下放置在指定方块上（完整版，支持NBT匹配）
     *
     * 对应 MC Java 的 ItemStack.canPlaceOnBlockInAdventureMode(BlockInWorld)。
     * 如果物品没有 CanPlaceOn 标签，返回 false。
     * 如果有 CanPlaceOn 标签，检查目标方块是否匹配任一谓词。
     * 当谓词包含NBT条件时，会从世界获取对应位置的方块实体数据进行匹配。
     *
     * @param world 世界引用
     * @param pos 方块位置（用于获取方块实体）
     * @param state 目标方块状态
     * @return 如果允许放置返回 true
     */
    [[nodiscard]] bool canPlaceOnBlockInAdventureMode(
        IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 是否有 CanDestroy 谓词
     *
     * 冒险模式下，玩家只能用此物品破坏 CanDestroy 列表中的方块。
     *
     * @return 如果有 CanDestroy 标签返回 true
     */
    [[nodiscard]] bool hasCanDestroy() const { return !m_canDestroy.isEmpty(); }

    /**
     * @brief 获取 CanDestroy 谓词
     * @return CanDestroy 谓词的常量引用
     */
    [[nodiscard]] const AdventureModePredicate& getCanDestroy() const { return m_canDestroy; }

    /**
     * @brief 设置 CanDestroy 谓词
     * @param predicate 冒险模式谓词
     */
    void setCanDestroy(AdventureModePredicate predicate) { m_canDestroy = std::move(predicate); }

    /**
     * @brief 检查此物品是否可以在冒险模式下破坏指定方块
     *
     * 对应 MC Java 的 ItemStack.canBreakBlockInAdventureMode()。
     * 如果物品没有 CanDestroy 标签，返回 false。
     * 如果有 CanDestroy 标签，检查目标方块是否匹配任一谓词。
     *
     * @param state 目标方块状态
     * @return 如果允许破坏返回 true
     */
    [[nodiscard]] bool canBreakBlockInAdventureMode(const BlockState& state) const;

    /**
     * @brief 检查此物品是否可以在冒险模式下破坏指定方块（带世界参数）
     *
     * 不支持方块实体NBT匹配，仅检查方块状态。
     * 如需NBT匹配，请使用带 BlockPos 的重载版本。
     *
     * @param world 世界引用
     * @param state 目标方块状态
     * @return 如果允许破坏返回 true
     */
    [[nodiscard]] bool canBreakBlockInAdventureMode(IWorld& world, const BlockState& state) const;

    /**
     * @brief 检查此物品是否可以在冒险模式下破坏指定方块（完整版，支持NBT匹配）
     *
     * 对应 MC Java 的 ItemStack.canBreakBlockInAdventureMode(BlockInWorld)。
     * 如果物品没有 CanDestroy 标签，返回 false。
     * 如果有 CanDestroy 标签，检查目标方块是否匹配任一谓词。
     * 当谓词包含NBT条件时，会从世界获取对应位置的方块实体数据进行匹配。
     *
     * @param world 世界引用
     * @param pos 方块位置（用于获取方块实体）
     * @param state 目标方块状态
     * @return 如果允许破坏返回 true
     */
    [[nodiscard]] bool canBreakBlockInAdventureMode(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    // ========== 堆叠兼容性检查 ==========

    /**
     * @brief 检查两个物品堆是否可以堆叠（物品类型相同且数据兼容）
     * @param other 另一个物品堆
     * @return 是否可以堆叠
     *
     * 这是canMergeWith的别名，用于与MC源码命名保持一致。
     */
    [[nodiscard]] bool canStackWith(const ItemStack& other) const { return canMergeWith(other); }

    // ========== 修复成本（铁砧） ==========

    /**
     * @brief 获取修复成本
     * @return 修复成本
     */
    [[nodiscard]] i32 getRepairCost() const { return m_repairCost; }

    /**
     * @brief 设置修复成本
     * @param cost 修复成本
     */
    void setRepairCost(i32 cost) { m_repairCost = cost; }

    // ========== 药水（potion_contents 组件） ==========

    /**
     * @brief 获取药水 ID（对应 1.21.11 potion_contents 组件的 potion 字段）
     * @return 药水资源位置字符串，空串表示无药水
     */
    [[nodiscard]] const std::string& getPotionId() const { return m_potionId; }

    /**
     * @brief 设置药水 ID
     * @param potionId 药水资源位置字符串，空串表示清除
     */
    void setPotionId(const std::string& potionId) { m_potionId = potionId; }
    void setPotionId(std::string&& potionId) { m_potionId = std::move(potionId); }

    // ========== 容器物品 ==========

    /**
     * @brief 获取容器物品堆
     * @return 容器物品堆，如果没有则返回空堆
     *
     * 例如：牛奶桶用完后返回空桶
     */
    [[nodiscard]] ItemStack getContainerItem() const;

    /**
     * @brief 是否有容器物品
     * @return 如果物品有容器物品返回true
     */
    [[nodiscard]] bool hasContainerItem() const;

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
     * @brief 序列化到 NBT（1.21.11 数据组件格式）
     * @param tag NBT 复合标签（输出参数）
     *
     * NBT 格式（对齐 MC Java 1.21.11）：
     * - id (string): 物品资源位置
     * - count (int): 数量（1..99，0=空）
     * - components (compound, 可选): 数据组件补丁
     *   键为组件资源位置名（如 "minecraft:damage"），值为该组件的 NBT；
     *   以 '!' 前缀的键表示移除该组件。
     *
     * 对应字段：
     * - minecraft:damage           —— m_damage
     * - minecraft:repair_cost      —— m_repairCost
     * - minecraft:custom_name      —— m_customName
     * - minecraft:lore             —— m_lore
     * - minecraft:enchantments     —— m_enchantments
     * - minecraft:potion_contents  —— m_potionId
     * - minecraft:can_place_on     —— m_canPlaceOn
     * - minecraft:can_break        —— m_canDestroy
     * - minecraft:custom_data      —— m_customData（JSON↔NBT 转换）
     */
    void toNbt(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从 NBT 反序列化（1.21.11 数据组件格式）
     * @param tag NBT 复合标签
     * @return 物品堆或错误
     *
     * 同时兼容旧版 {id, Count(byte), tag{...}} 格式：若检测到 "tag" 复合键且无
     * "components" 键，按旧 1.16.5 格式解析（用于读旧存档），否则按 1.21.11 组件格式解析。
     */
    [[nodiscard]] static Result<ItemStack> fromNbt(const nbt::tags::compound_tag& tag);

    // ========== 数据组件补丁转换（NBT/wire 序列化用） ==========

    /**
     * @brief 将当前物品堆的非默认组件字段导出为 DataComponentPatch
     *
     * 仅当字段非默认值时加入 added 列表。供 toNbt 的 components 段与 wire 编码使用。
     */
    [[nodiscard]] item::component::DataComponentPatch toComponentPatch() const;

    /**
     * @brief 从 DataComponentPatch 应用组件到本物品堆
     *
     * added 字段覆盖对应成员；removed 字段重置为默认。供 fromNbt 与 wire 解码使用。
     */
    void applyComponentPatch(const item::component::DataComponentPatch& patch);

    // ========== 比较操作符 ==========

    /**
     * @brief 物品堆相等比较
     *
     * 比较物品类型、数量和耐久度。
     * 空堆与空堆相等。
     */
    bool operator==(const ItemStack& other) const;

    bool operator!=(const ItemStack& other) const { return !(*this == other); }

private:
    const Item* m_item = nullptr;
    i32 m_count = 0;
    i32 m_damage = 0;                                          // 已承受的伤害（耐久度）
    i32 m_repairCost = 0;                                      // 修复成本（铁砧）
    std::unique_ptr<text::ITextComponent> m_customName;        // 自定义名称（铁砧重命名）
    std::vector<std::unique_ptr<text::ITextComponent>> m_lore; // 物品描述（Lore）
    item::enchant::EnchantmentContainer m_enchantments;        // 附魔容器
    std::string m_potionId;                                    // 药水ID（用于药水物品）
    nlohmann::json m_customData;                               // 自定义数据（用于display等扩展标签）
    AdventureModePredicate m_canPlaceOn;                       // 冒险模式可放置方块谓词
    AdventureModePredicate m_canDestroy;                       // 冒险模式可破坏方块谓词

    /// 旧 1.16.5 {tag{...}} 格式回退读取（fromNbt 读旧存档用）
    static void applyLegacyTagCompound(ItemStack& stack, const nbt::tags::compound_tag& tagCompound);

    // 允许 PotionUtils 访问私有成员
    friend class potion::PotionUtils;
};

} // namespace mc
