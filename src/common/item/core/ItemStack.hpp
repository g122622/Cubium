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
#include "common/item/component/AttackRange.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/FireworkExplosion.hpp"
#include "common/item/component/Fireworks.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/ItemRarity.hpp"
#include "common/item/enchantment/EnchantmentContainer.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

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
namespace item {
namespace component {

/**
 * @brief container 组件载荷（物品形式的容器内容）
 *
 * 对应 Java 1.21.11 ItemContainerContents(List<Slot>)：仅记录非空槽位，
 * 槽位索引即容器位置。元素为 ItemStack 指针；该类型的完整定义在 ItemStack 之后才可见，
 * 因此使用它（拷贝/析构）的代码必须包含本头文件。
 */
struct ItemContainerContentsPayload {
    /// 单个非空槽位（vanilla ItemContainerContents.Slot）
    struct Slot {
        i32 index = 0;                   ///< 槽位索引（0..255）
        std::unique_ptr<ItemStack> item; ///< 槽位物品（空指针 = 空槽位）
    };

    std::vector<Slot> slots;
};

/**
 * @brief charged_projectiles 组件载荷
 *
 * 对应 Java 1.21.11 的 List<ItemStack>（外层 Optional 未设置 = 组件不存在，
 * 空列表 = 已充能但当前为空）。
 */
struct ChargedProjectilesPayload {
    std::vector<std::unique_ptr<ItemStack>> items;
};

/// 物品指针列表深拷贝（用于 DataComponentPayload 中需要值语义的写法）
[[nodiscard]] std::vector<std::unique_ptr<ItemStack>> cloneItemStackList(
    const std::vector<std::unique_ptr<ItemStack>>& items);

} // namespace component
} // namespace item
} // namespace mc

namespace mc {

/**
 * @brief ItemStack 中极少使用的字段的外置存储
 *
 * 这些字段内联合计约 176 字节（两个 AdventureModePredicate 就占 112），但绝大多数
 * 物品堆一生都不会用到其中任何一个：lore / can_place_on / can_destroy 的非空率都在
 * 1% 以下，custom_data 约 3~8%。内联会让每一个物品堆——包括装备组件里的 16 个槽、
 * 玩家背包的 41 个槽，以及容器每 tick 的逐槽拷贝——都白付这份开销。
 *
 * 改为单个 unique_ptr<ItemExtras> 惰性持有：未用到时指针为空、零堆分配；ItemStack
 * 因此从 232 字节降到 64 字节。
 *
 * 刻意不含 m_enchantments 与 m_customName：前者在 LivingEntity 每 tick 的装备变更
 * 检测（ItemStack::operator==）里被读取，外置会给这条热路径平添一次间接寻址；后者
 * 的裸指针会经 getCustomNameComponent() 交出并在多处逃逸。
 *
 * 另外承载数据组件对物品基础默认值的"覆盖"（componentOverrides）：这些覆盖同属稀有
 * 字段（仅物品自带或命令/NBT 显式设置），一并外置。未设置时各 getter 回退到 Item 的
 * 基础值，语义与无覆盖一致。
 */
struct ItemExtras {
    std::vector<std::unique_ptr<text::ITextComponent>> lore;
    std::string potionId;
    nlohmann::json customData;
    AdventureModePredicate canPlaceOn;
    AdventureModePredicate canDestroy;

    // ===== 数据组件覆盖（未设置=回退到 Item 基础值） =====
    std::optional<i32> maxStackSizeOverride;                             // max_stack_size
    std::optional<i32> maxDamageOverride;                                // max_damage
    bool unbreakable = false;                                            // unbreakable
    std::unique_ptr<text::ITextComponent> itemName;                      // item_name
    std::optional<std::string> itemModel;                                // item_model（仅序列化）
    std::optional<ItemRarity> rarityOverride;                            // rarity
    std::optional<i32> enchantableOverride;                              // enchantable（附魔能力）
    std::optional<item::component::AttackRange> attackRange;             // attack_range
    std::optional<item::component::FireworkExplosion> fireworkExplosion; // firework_explosion
    std::optional<item::component::Fireworks> fireworks;                 // fireworks
    nlohmann::json customModelData;                                      // custom_model_data（需显式设置的 JSON 对象）
    std::optional<bool> enchantmentGlintOverride;                        // enchantment_glint_override
    std::optional<std::string> instrument;                               // instrument（乐器资源位置）
    std::optional<item::component::ToolPayload> tool;                    // tool（挖掘规则）
    std::optional<i32> ominousBottleAmplifier;                           // ominous_bottle_amplifier
    std::optional<item::component::SuspiciousStewEffectsPayload> stewEffects; // suspicious_stew_effects
    std::optional<item::component::DeathProtectionPayload> deathProtection;   // death_protection
    nlohmann::json blockEntityData; // block_entity_data 组件（放置时写入方块实体）
    nlohmann::json rawComponents;   // 未落地结构组件的原始 JSON（以资源位置为键）

    ItemExtras() = default;

    /// 深拷贝：lore / itemName 内的文本组件必须逐项 deepCopy，不能共享 unique_ptr
    ItemExtras(const ItemExtras& other)
        : potionId(other.potionId)
        , customData(other.customData)
        , canPlaceOn(other.canPlaceOn)
        , canDestroy(other.canDestroy)
        , maxStackSizeOverride(other.maxStackSizeOverride)
        , maxDamageOverride(other.maxDamageOverride)
        , unbreakable(other.unbreakable)
        , itemName(other.itemName ? other.itemName->deepCopy() : nullptr)
        , itemModel(other.itemModel)
        , rarityOverride(other.rarityOverride)
        , enchantableOverride(other.enchantableOverride)
        , attackRange(other.attackRange)
        , fireworkExplosion(other.fireworkExplosion)
        , fireworks(other.fireworks)
        , customModelData(other.customModelData)
        , enchantmentGlintOverride(other.enchantmentGlintOverride)
        , instrument(other.instrument)
        , tool(other.tool)
        , ominousBottleAmplifier(other.ominousBottleAmplifier)
        , stewEffects(other.stewEffects)
        , deathProtection(other.deathProtection)
        , rawComponents(other.rawComponents)
    {
        lore.reserve(other.lore.size());
        for (const auto& line : other.lore) {
            lore.push_back(line ? line->deepCopy() : nullptr);
        }
    }

    ItemExtras& operator=(const ItemExtras& other)
    {
        if (this != &other) {
            potionId = other.potionId;
            customData = other.customData;
            canPlaceOn = other.canPlaceOn;
            canDestroy = other.canDestroy;
            maxStackSizeOverride = other.maxStackSizeOverride;
            maxDamageOverride = other.maxDamageOverride;
            unbreakable = other.unbreakable;
            itemName = other.itemName ? other.itemName->deepCopy() : nullptr;
            itemModel = other.itemModel;
            rarityOverride = other.rarityOverride;
            enchantableOverride = other.enchantableOverride;
            attackRange = other.attackRange;
            fireworkExplosion = other.fireworkExplosion;
            fireworks = other.fireworks;
            customModelData = other.customModelData;
            enchantmentGlintOverride = other.enchantmentGlintOverride;
            instrument = other.instrument;
            tool = other.tool;
            ominousBottleAmplifier = other.ominousBottleAmplifier;
            stewEffects = other.stewEffects;
            deathProtection = other.deathProtection;
            blockEntityData = other.blockEntityData;
            rawComponents = other.rawComponents;
            lore.clear();
            lore.reserve(other.lore.size());
            for (const auto& line : other.lore) {
                lore.push_back(line ? line->deepCopy() : nullptr);
            }
        }
        return *this;
    }

    /// 是否含任何"组件覆盖"字段（用于惰性分配判断）
    [[nodiscard]] bool hasComponentOverrides() const noexcept
    {
        return maxStackSizeOverride.has_value() || maxDamageOverride.has_value() || unbreakable ||
            itemName != nullptr || itemModel.has_value() || rarityOverride.has_value() ||
            enchantableOverride.has_value() || attackRange.has_value() || fireworkExplosion.has_value() ||
            fireworks.has_value();
    }
};

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

    /**
     * @brief 析构函数
     *
     * 显式声明并在 ItemStack.cpp 中定义：成员中包含以本类指针为元素的
     * 组件载荷（container / charged_projectiles），需要完整类型才能析构。
     */
    ~ItemStack();

    // ========== 基本属性 ==========

    /**
     * @brief 是否为空物品堆
     *
     * 对齐原版 `ItemStack#isEmpty`：**只看数量**。数量为 0 的堆仍记着物品类型，
     * 因此 `grow` 能把它重新变成非空——原版「`set_count` 设成 0、再由
     * `enchanted_count_increase` 加回来」这类函数链依赖这一点。
     *
     * `getItem()` 对数量为 0 的堆返回物品类型本身（原版 `getItem()` 同样如此）；
     * 只有从未设置过物品的默认堆才没有类型。
     */
    [[nodiscard]] bool isEmpty() const
    {
        return m_count <= 0;
    }

    /**
     * @brief 获取物品
     * @return 物品指针；从未设置过物品的默认堆返回 nullptr
     */
    [[nodiscard]] const Item* getItem() const
    {
        return m_item;
    }

    /**
     * @brief 获取数量
     */
    [[nodiscard]] i32 getCount() const
    {
        return m_count;
    }

    /**
     * @brief 设置数量
     * @param count 新数量
     * @note 数量<=0会使堆变为空
     *
     * 对齐原版 `ItemStack#setCount`：**不做 maxStackSize 裁剪**（原版 setCount 只赋值，
     * 裁剪是独立的 `limitSize`）。这里的上限裁剪曾使 `split`/`shrink` 等"拆出 N 件、剩余 M 件"
     * 的路径对不可堆叠物品（maxStackSize=1）错误地把剩余数裁成 1：
     * 例如护甲架从 3 件胸甲中拆 1 件装备后，玩家手里应剩 2 件，却被打成 1 件。
     * 需要按堆叠上限收敛的调用方请显式调用 limitSize(getMaxStackSize())。
     */
    void setCount(i32 count);

    /**
     * @brief 按给定上限裁剪数量（对齐原版 ItemStack#limitSize）
     * @param limit 上限；仅当当前非空且数量 > limit 时裁剪
     */
    void limitSize(i32 limit)
    {
        if (!isEmpty() && m_count > limit) {
            setCount(limit);
        }
    }

    /**
     * @brief 增加数量
     * @param amount 增加量（可为负数）
     */
    void grow(i32 amount)
    {
        setCount(m_count + amount);
    }

    /**
     * @brief 减少数量
     * @param amount 减少量
     */
    void shrink(i32 amount)
    {
        setCount(m_count - amount);
    }

    /**
     * @brief 获取最大堆叠数量
     *
     * 优先取 max_stack_size 组件覆盖，缺省回退物品基础值；有耐久度的物品恒为 1。
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
    [[nodiscard]] const item::enchant::EnchantmentContainer& getEnchantments() const
    {
        return m_enchantments;
    }

    /**
     * @brief 获取可修改的附魔容器
     * @return 附魔容器的引用
     */
    item::enchant::EnchantmentContainer& getEnchantmentsMutable()
    {
        return m_enchantments;
    }

    /**
     * @brief 是否有附魔
     */
    [[nodiscard]] bool hasEnchantments() const
    {
        return !m_enchantments.isEmpty();
    }

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

    // ========== 数据组件覆盖（覆盖物品基础默认值） ==========

    /**
     * @brief 是否携带不可损坏组件
     *
     * 对齐 vanilla `ItemStack.isDamageableItem()` 中的 `!has(UNBREAKABLE)` 门控：
     * 置位后物品不再消耗耐久，也不会被视为可损坏。
     */
    [[nodiscard]] bool isUnbreakable() const
    {
        return m_extras && m_extras->unbreakable;
    }

    /**
     * @brief 设置/清除不可损坏组件
     * @param value true 置位（惰性分配 extras），false 清除（不反向分配）
     */
    void setUnbreakable(bool value);

    /**
     * @brief 获取稀有度（含组件覆盖与附魔提升）
     *
     * 对齐 vanilla `ItemStack.getRarity()`：基础稀有度取 rarity 组件（缺省回退物品
     * 自身稀有度），若已附魔则按 COMMON/UNCOMMON→RARE、RARE→EPIC 提升。
     */
    [[nodiscard]] ItemRarity getRarity() const;

    /**
     * @brief 获取 item_name 组件覆盖的显示名
     * @return 组件文本组件指针，未设置返回 nullptr
     */
    [[nodiscard]] const text::ITextComponent* getItemNameComponent() const
    {
        return (m_extras && m_extras->itemName) ? m_extras->itemName.get() : nullptr;
    }

    /**
     * @brief 设置 item_name 组件（默认显示名覆盖）
     * @param name 文本组件（所有权转移）
     */
    void setItemNameComponent(std::unique_ptr<text::ITextComponent> name);

    /**
     * @brief 获取 item_model 组件（模型资源位置，仅序列化承载）
     * @return 资源位置字符串指针，未设置返回 nullptr
     */
    [[nodiscard]] const std::string* getItemModel() const
    {
        return (m_extras && m_extras->itemModel) ? &*m_extras->itemModel : nullptr;
    }

    /**
     * @brief 设置 item_model 组件
     * @param model 模型资源位置（空串清除）
     */
    void setItemModel(const std::string& model);

    /**
     * @brief 获取附魔能力（enchantable 组件覆盖，缺省回退物品自身附魔能力）
     *
     * 对齐 vanilla `EnchantmentHelper.getEnchantmentCost/selectEnchantment` 读取
     * `DataComponents.ENCHANTABLE` 的 value。
     */
    [[nodiscard]] i32 getEnchantmentValue() const;

    /**
     * @brief 是否可附魔（enchantable 组件存在且当前无附魔）
     *
     * 对齐 vanilla `ItemStack.isEnchantable()`：`has(ENCHANTABLE) && ENCHANTMENTS.isEmpty()`。
     */
    [[nodiscard]] bool isEnchantable() const;

    /**
     * @brief 设置 enchantable 组件覆盖
     * @param value 附魔能力（<=0 清除）
     */
    void setEnchantableOverride(i32 value);

    /**
     * @brief 获取 attack_range 组件
     * @return 组件指针，未设置返回 nullptr（由持有者回退到 entityAttackRange 默认值）
     */
    [[nodiscard]] const item::component::AttackRange* getAttackRange() const
    {
        return (m_extras && m_extras->attackRange) ? &*m_extras->attackRange : nullptr;
    }

    /**
     * @brief 获取 suspicious_stew_effects 组件
     * @return 组件指针，未设置返回 nullptr
     */
    [[nodiscard]] const item::component::SuspiciousStewEffectsPayload* getStewEffects() const
    {
        return (m_extras && m_extras->stewEffects) ? &*m_extras->stewEffects : nullptr;
    }

    /**
     * @brief 设置 suspicious_stew_effects 组件
     * @param effects 效果列表（空列表清除组件）
     */
    void setStewEffects(item::component::SuspiciousStewEffectsPayload effects);

    /**
     * @brief 设置 attack_range 组件
     * @param range 攻击范围
     */
    void setAttackRange(item::component::AttackRange range);

    /**
     * @brief 获取 firework_explosion 组件（烟花之星的爆炸数据）
     * @return 组件指针，未设置返回 nullptr
     */
    [[nodiscard]] const item::component::FireworkExplosion* getFireworkExplosion() const
    {
        return (m_extras && m_extras->fireworkExplosion) ? &*m_extras->fireworkExplosion : nullptr;
    }

    /**
     * @brief 设置 firework_explosion 组件
     * @param explosion 爆炸数据
     */
    void setFireworkExplosion(item::component::FireworkExplosion explosion);

    /**
     * @brief 获取 fireworks 组件（烟花火箭的飞行时长与爆炸列表）
     * @return 组件指针，未设置返回 nullptr
     */
    [[nodiscard]] const item::component::Fireworks* getFireworks() const
    {
        return (m_extras && m_extras->fireworks) ? &*m_extras->fireworks : nullptr;
    }

    /**
     * @brief 设置 fireworks 组件
     * @param fireworks 烟花火箭数据
     */
    void setFireworks(item::component::Fireworks fireworks);

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
    [[nodiscard]] i32 getDamage() const
    {
        return m_damage;
    }

    /**
     * @brief 设置当前耐久度
     * @param damage 已承受的伤害值
     */
    void setDamage(i32 damage);

    /**
     * @brief 获取最大耐久度
     *
     * 优先取 max_damage 组件覆盖，缺省回退物品基础值。
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
     * 对应 MC 1.21.11 ItemStack#transmuteCopy(Item, i32)。
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
    [[nodiscard]] bool hasCustomName() const
    {
        return m_customName && !m_customName->getUnformattedText().empty();
    }

    /**
     * @brief 获取自定义名称组件
     *
     * 返回自定义名称组件，如果没有则返回 nullptr。
     *
     * @return 自定义名称组件指针
     */
    [[nodiscard]] const text::ITextComponent* getCustomNameComponent() const
    {
        return m_customName.get();
    }

    /**
     * @brief 获取自定义名称的纯文本
     *
     * 返回自定义名称的纯文本，如果没有则返回空字符串。
     *
     * @return 自定义名称纯文本
     */
    [[nodiscard]] std::string getCustomName() const
    {
        return m_customName ? m_customName->getUnformattedText() : "";
    }

    /**
     * @brief 设置自定义名称组件
     * @param name 名称组件（所有权转移）
     */
    void setCustomNameComponent(std::unique_ptr<text::ITextComponent> name)
    {
        m_customName = std::move(name);
    }

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
    void clearCustomName()
    {
        m_customName = nullptr;
    }

    /**
     * @brief 是否有显示名称（自定义名称或物品翻译名称）
     */
    [[nodiscard]] bool hasDisplayName() const
    {
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
    [[nodiscard]] bool hasLore() const
    {
        return m_extras && !m_extras->lore.empty();
    }

    /**
     * @brief 获取 Lore 列表
     * @return Lore 文本组件列表的常量引用；无 extras 时返回共享的空列表
     */
    [[nodiscard]] const std::vector<std::unique_ptr<text::ITextComponent>>& getLore() const
    {
        return m_extras ? m_extras->lore : _emptyLore();
    }

    /**
     * @brief 设置 Lore
     * @param lore Lore 文本组件列表（所有权转移）
     */
    void setLore(std::vector<std::unique_ptr<text::ITextComponent>> lore)
    {
        if (lore.empty()) {
            // 置空不该反向分配一份 extras
            if (m_extras) {
                m_extras->lore.clear();
            }
            return;
        }
        _ensureExtras().lore = std::move(lore);
    }

    /**
     * @brief 添加一行 Lore
     * @param line Lore 文本组件（所有权转移）
     */
    void addLoreLine(std::unique_ptr<text::ITextComponent> line)
    {
        _ensureExtras().lore.push_back(std::move(line));
    }

    /**
     * @brief 添加一行 Lore（纯文本）
     * @param line Lore 纯文本
     */
    void addLoreLine(const std::string& line)
    {
        _ensureExtras().lore.push_back(std::make_unique<text::StringTextComponent>(line));
    }

    /**
     * @brief 清除 Lore
     */
    void clearLore()
    {
        if (m_extras) {
            m_extras->lore.clear();
        }
    }

    // ========== 冒险模式谓词（CanPlaceOn / CanDestroy） ==========

    /**
     * @brief 是否有 CanPlaceOn 谓词
     *
     * 冒险模式下，玩家只能将此物品放置在 CanPlaceOn 列表中的方块上。
     *
     * @return 如果有 CanPlaceOn 标签返回 true
     */
    [[nodiscard]] bool hasCanPlaceOn() const
    {
        return m_extras && !m_extras->canPlaceOn.isEmpty();
    }

    /**
     * @brief 获取 CanPlaceOn 谓词
     * @return CanPlaceOn 谓词的常量引用；无 extras 时返回共享的空谓词
     */
    [[nodiscard]] const AdventureModePredicate& getCanPlaceOn() const
    {
        return m_extras ? m_extras->canPlaceOn : _emptyPredicate();
    }

    /**
     * @brief 设置 CanPlaceOn 谓词
     * @param predicate 冒险模式谓词
     */
    void setCanPlaceOn(AdventureModePredicate predicate)
    {
        if (predicate.isEmpty()) {
            // 置空不分配；已分配时原地赋空值，保持已有引用的地址稳定
            if (m_extras) {
                m_extras->canPlaceOn = std::move(predicate);
            }
            return;
        }
        _ensureExtras().canPlaceOn = std::move(predicate);
    }

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
    [[nodiscard]] bool hasCanDestroy() const
    {
        return m_extras && !m_extras->canDestroy.isEmpty();
    }

    /**
     * @brief 获取 CanDestroy 谓词
     * @return CanDestroy 谓词的常量引用；无 extras 时返回共享的空谓词
     */
    [[nodiscard]] const AdventureModePredicate& getCanDestroy() const
    {
        return m_extras ? m_extras->canDestroy : _emptyPredicate();
    }

    /**
     * @brief 设置 CanDestroy 谓词
     * @param predicate 冒险模式谓词
     */
    void setCanDestroy(AdventureModePredicate predicate)
    {
        if (predicate.isEmpty()) {
            if (m_extras) {
                m_extras->canDestroy = std::move(predicate);
            }
            return;
        }
        _ensureExtras().canDestroy = std::move(predicate);
    }

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
    [[nodiscard]] bool canStackWith(const ItemStack& other) const
    {
        return canMergeWith(other);
    }

    // ========== 修复成本（铁砧） ==========

    /**
     * @brief 获取修复成本
     * @return 修复成本
     */
    [[nodiscard]] i32 getRepairCost() const
    {
        return m_repairCost;
    }

    /**
     * @brief 设置修复成本
     * @param cost 修复成本
     */
    void setRepairCost(i32 cost)
    {
        m_repairCost = cost;
    }

    // ========== custom_model_data / 附魔光效 / instrument / tool / 不祥之瓶 / container ==========

    /**
     * @brief 获取 custom_model_data 条目
     *
     * 返回显式设置过的 JSON 对象（可含 floats/flags/strings/colors 子数组）；未设置时返回空对象。
     */
    [[nodiscard]] const nlohmann::json& getCustomModelData() const
    {
        return m_extras ? m_extras->customModelData : _emptyJson();
    }

    /**
     * @brief 获取 block_entity_data 组件
     *
     * 物品被放置为方块时写入该方块实体（对应 vanilla `minecraft:block_entity_data`）。
     * 未设置时返回空对象。与旧式 `BlockEntityTag` 不同，该组件是 1.20.5+ 的正式数据组件，
     * 且对方块实体受 `onlyOpsCanSetNbt` 门控的来源同样生效。
     */
    [[nodiscard]] const nlohmann::json& getBlockEntityData() const
    {
        return m_extras ? m_extras->blockEntityData : _emptyJson();
    }

    /**
     * @brief 是否携带 block_entity_data 组件
     */
    [[nodiscard]] bool hasBlockEntityData() const
    {
        return m_extras != nullptr && m_extras->blockEntityData.is_object() && !m_extras->blockEntityData.empty();
    }

    /**
     * @brief 设置 block_entity_data 组件（传空对象表示移除）
     */
    void setBlockEntityData(nlohmann::json data)
    {
        if (!data.is_object() || data.empty()) {
            if (m_extras) {
                m_extras->blockEntityData = nlohmann::json{};
            }
            return;
        }
        _ensureExtras().blockEntityData = std::move(data);
    }

    /**
     * @brief 设置 custom_model_data（传空对象表示移除）
     */
    void setCustomModelData(nlohmann::json data)
    {
        if (data.is_object() && data.empty()) {
            if (m_extras) {
                m_extras->customModelData = nlohmann::json{};
            }
            return;
        }
        _ensureExtras().customModelData = std::move(data);
    }

    /**
     * @brief 获取附魔光效覆盖（enchantment_glint_override），未设置返回 nullopt
     */
    [[nodiscard]] std::optional<bool> getEnchantmentGlintOverride() const
    {
        return m_extras ? m_extras->enchantmentGlintOverride : std::nullopt;
    }

    /**
     * @brief 设置附魔光效覆盖（nullopt 表示移除覆盖）
     */
    void setEnchantmentGlintOverride(std::optional<bool> value)
    {
        if (!value.has_value()) {
            if (m_extras) {
                m_extras->enchantmentGlintOverride.reset();
            }
            return;
        }
        _ensureExtras().enchantmentGlintOverride = value;
    }

    /**
     * @brief 获取乐器资源位置（instrument 组件），未设置返回空串
     */
    [[nodiscard]] std::string getInstrument() const
    {
        return m_extras ? m_extras->instrument.value_or("") : "";
    }

    /**
     * @brief 设置乐器资源位置（空串表示移除）
     */
    void setInstrument(std::string instrumentId)
    {
        if (instrumentId.empty()) {
            if (m_extras) {
                m_extras->instrument.reset();
            }
            return;
        }
        _ensureExtras().instrument = std::move(instrumentId);
    }

    /**
     * @brief 获取 death_protection 组件（物品栈覆盖优先，否则回退物品默认值）
     *
     * 对齐 Java 1.21.11 ItemStack.get(DataComponents.DEATH_PROTECTION)（item 默认组件 + 栈覆盖合并）。
     * 与 getTool 不同：回退到物品默认值时返回的引用绑定物品对象（Item 生命周期长于物品栈，
     * 无悬垂风险），调用方解构拷贝即可安全使用。
     *
     * @return 死亡保护定义，未设置返回 nullopt
     */
    [[nodiscard]] const std::optional<item::component::DeathProtectionPayload>& getDeathProtection() const;

    /**
     * @brief 设置物品栈的 death_protection 组件覆盖（nullopt 表示移除覆盖，回退物品默认值）
     */
    void setDeathProtection(std::optional<item::component::DeathProtectionPayload> value)
    {
        _ensureExtras().deathProtection = std::move(value);
    }

    /**
     * @brief 获取 tool 组件（未设置返回 nullopt）
     */
    [[nodiscard]] const std::optional<item::component::ToolPayload>& getTool() const
    {
        static const std::optional<item::component::ToolPayload> EMPTY;
        return m_extras ? m_extras->tool : EMPTY;
    }

    /**
     * @brief 设置 tool 组件（nullopt 表示移除）
     */
    void setTool(std::optional<item::component::ToolPayload> tool)
    {
        _ensureExtras().tool = std::move(tool);
    }

    /**
     * @brief 获取不祥之瓶等级覆盖（0..4；未设置时为物品基础默认值）
     */
    [[nodiscard]] i32 getOminousBottleAmplifier() const
    {
        if (m_extras && m_extras->ominousBottleAmplifier.has_value()) {
            return *m_extras->ominousBottleAmplifier;
        }
        return _itemDefaultOminousBottleAmplifier();
    }

    /**
     * @brief 设置不祥之瓶等级覆盖（nullopt 表示移除）
     */
    void setOminousBottleAmplifier(std::optional<i32> amplifier)
    {
        _ensureExtras().ominousBottleAmplifier = amplifier;
    }

    /// 物品形式的容器内容（container 组件）；未设置时返回 nullptr
    [[nodiscard]] const item::component::ItemContainerContentsPayload* getContainerContents() const
    {
        return m_containerContents.get();
    }

    /// 设置物品形式的容器内容（空列表表示空容器）
    void setContainerContents(item::component::ItemContainerContentsPayload contents)
    {
        m_containerContents = std::make_shared<item::component::ItemContainerContentsPayload>(std::move(contents));
    }

    /// 移除容器内容组件
    void clearContainerContents()
    {
        m_containerContents.reset();
    }

    /// 获取充能投射物列表（charged_projectiles 组件）；未设置时返回 nullptr
    [[nodiscard]] const item::component::ChargedProjectilesPayload* getChargedProjectiles() const
    {
        return m_chargedProjectiles.get();
    }

    /// 设置充能投射物列表
    void setChargedProjectiles(item::component::ChargedProjectilesPayload projectiles)
    {
        m_chargedProjectiles = std::make_shared<item::component::ChargedProjectilesPayload>(std::move(projectiles));
    }

    /// 移除充能投射物组件
    void clearChargedProjectiles()
    {
        m_chargedProjectiles.reset();
    }

    // ========== 药水（potion_contents 组件） ==========

    /**
     * @brief 获取药水 ID（对应 1.21.11 potion_contents 组件的 potion 字段）
     * @return 药水资源位置字符串，空串表示无药水
     */
    [[nodiscard]] const std::string& getPotionId() const
    {
        return m_extras ? m_extras->potionId : _emptyString();
    }

    /**
     * @brief 设置药水 ID
     * @param potionId 药水资源位置字符串，空串表示清除
     */
    void setPotionId(const std::string& potionId)
    {
        if (potionId.empty()) {
            if (m_extras) {
                m_extras->potionId.clear();
            }
            return;
        }
        _ensureExtras().potionId = potionId;
    }

    void setPotionId(std::string&& potionId)
    {
        if (potionId.empty()) {
            if (m_extras) {
                m_extras->potionId.clear();
            }
            return;
        }
        _ensureExtras().potionId = std::move(potionId);
    }

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
     * - count (i32): 数量（1..99，0=空）
     * - components (compound, 可选): 数据组件补丁
     *   键为组件资源位置名（如 "minecraft:damage"），值为该组件的 NBT；
     *   以 '!' 前缀的键表示移除该组件。
     *
     * 对应字段（标 m_extras 的寄存在 ItemExtras 中）：
     * - minecraft:damage           —— m_damage
     * - minecraft:repair_cost      —— m_repairCost
     * - minecraft:custom_name      —— m_customName
     * - minecraft:lore             —— m_extras->lore
     * - minecraft:enchantments     —— m_enchantments
     * - minecraft:potion_contents  —— m_extras->potionId
     * - minecraft:can_place_on     —— m_extras->canPlaceOn
     * - minecraft:can_break        —— m_extras->canDestroy
     * - minecraft:custom_data      —— m_extras->customData（JSON↔NBT 转换）
     * - minecraft:max_stack_size   —— m_extras->maxStackSizeOverride
     * - minecraft:max_damage       —— m_extras->maxDamageOverride
     * - minecraft:unbreakable      —— m_extras->unbreakable
     * - minecraft:item_name        —— m_extras->itemName
     * - minecraft:item_model       —— m_extras->itemModel
     * - minecraft:rarity           —— m_extras->rarityOverride
     * - minecraft:enchantable      —— m_extras->enchantableOverride
     * - minecraft:attack_range     —— m_extras->attackRange
     * - minecraft:firework_explosion —— m_extras->fireworkExplosion
     * - minecraft:fireworks        —— m_extras->fireworks
     */
    void toNbt(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从 NBT 反序列化（1.21.11 数据组件格式）
     * @param tag NBT 复合标签
     * @return 物品堆或错误
     *
     * 读取键 "id"、"count"（数值，缺省 1）与 "components"（数据组件补丁 compound）。
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

    /**
     * @brief 读取尚未结构化落地的原始组件值（JSON）
     *
     * 仅对以 rawComponents 承载的组件有意义（如 chicken/variant 的 EitherHolder 序列化形态）。
     * 未设置该组件时返回 nullptr。
     */
    [[nodiscard]] const nlohmann::json* rawComponent(item::component::DataComponentType type) const;

    /**
     * @brief 本物品堆是否显式带有某个组件（覆盖或物品基础默认值）
     *
     * 按组件归口判断，不暴露存储形态：旗子上的 `banner_patterns` 既可能落在既有字段里，
     * 也可能落在原始组件表里，调用方只关心「有没有」。
     */
    [[nodiscard]] bool hasComponent(item::component::DataComponentType type) const;

    /**
     * @brief 读取 chicken/variant 组件（EitherHolder<ChickenVariant>）解析出的变种 id
     *
     * 返回 ChickenEntity.hpp 中 ChickenVariant 枚举值（Temperate=0/Warm=1/Cold=2）；
     * 组件缺失或无法解析时返回 nullopt。
     * 文档态形如 {"id":"minecraft:warm"} 或直接 "minecraft:warm"；均按注册表顺序
     * 映射到枚举值（Temperate=0 / Warm=1 / Cold=2）。未设置或无法解析时返回 nullopt。
     */
    [[nodiscard]] std::optional<i32> chickenVariantId() const;

    /**
     * @brief 设置单个组件（覆盖物品基础默认值）
     */
    void setComponent(item::component::DataComponentType type, item::component::DataComponentPayload value);

    /**
     * @brief 移除单个组件的显式设置（回退到物品基础默认值）
     */
    void removeComponent(item::component::DataComponentType type);

    // ========== 比较操作符 ==========

    /**
     * @brief 物品堆相等比较
     *
     * 比较物品类型、数量和耐久度。
     * 空堆与空堆相等。
     */
    bool operator==(const ItemStack& other) const;

    bool operator!=(const ItemStack& other) const
    {
        return !(*this == other);
    }

private:
    /// 确保 extras 已分配并返回可写引用
    ///
    /// 已分配时**原地复用、绝不重建**：getLore()/getCanPlaceOn() 等会把内部引用交给
    /// 调用方（例如 EnchantmentHelper 先取引用再逐个增删），重建会让那些引用立刻悬垂。
    ItemExtras& _ensureExtras()
    {
        if (!m_extras) {
            m_extras = std::make_unique<ItemExtras>();
        }
        return *m_extras;
    }

    /// customData 的只读访问：无 extras 时返回共享的空 json（null）
    ///
    /// 其余外置字段的只读访问直接复用公开 getter（getLore/getPotionId/getCanPlaceOn/
    /// getCanDestroy），它们的"无 extras 返回共享空实例"语义与旧的内联成员一致。
    [[nodiscard]] const nlohmann::json& _dataRef() const
    {
        return m_extras ? m_extras->customData : _emptyJson();
    }

    /// 无 extras 时 _dataRef() 返回的共享空 json
    [[nodiscard]] static const nlohmann::json& _emptyJson();

    /// 无 extras 时 getLore() 返回的共享空列表
    [[nodiscard]] static const std::vector<std::unique_ptr<text::ITextComponent>>& _emptyLore();
    /// 无 extras 时 getCanPlaceOn()/getCanDestroy() 返回的共享空谓词
    [[nodiscard]] static const AdventureModePredicate& _emptyPredicate();
    /// 无 extras 时 getPotionId() 返回的共享空串
    [[nodiscard]] static const std::string& _emptyString();

    /// 物品类型声明的不祥之瓶等级基础值（不祥之瓶为 0，其余物品同为 0）
    [[nodiscard]] i32 _itemDefaultOminousBottleAmplifier() const;

    /// tool 组件规则是否命中方块状态（HolderSet 语义：ID / 标签包含 / 被包含）
    [[nodiscard]] static bool _toolRuleMatches(const item::component::ToolRule& rule, const BlockState& state);
    /// tool 组件：按规则取挖掘速度，未命中取默认速度
    [[nodiscard]] static f32 _toolMiningSpeed(const item::component::ToolPayload& tool, const BlockState& state);
    /// tool 组件：按规则取"能否掉落"，未命中返回 false
    [[nodiscard]] static bool _toolCorrectForDrops(const item::component::ToolPayload& tool, const BlockState& state);

    /// 由 JSON 还原单次烟花爆炸（fromJson 用）
    [[nodiscard]] static item::component::FireworkExplosion _fireworkExplosionFromJson(const nlohmann::json& json);

    /// 比较两个物品堆的组件覆盖集合是否一致（operator== 用）
    [[nodiscard]] bool _componentOverridesEqual(const ItemStack& other) const;

    const Item* m_item = nullptr;
    i32 m_count = 0;
    i32 m_damage = 0;                                   // 已承受的伤害（耐久度）
    i32 m_repairCost = 0;                               // 修复成本（铁砧）
    std::unique_ptr<text::ITextComponent> m_customName; // 自定义名称（铁砧重命名）
    item::enchant::EnchantmentContainer m_enchantments; // 附魔容器

    /// 稀有字段的外置存储，惰性分配；见 ItemExtras 的说明
    std::unique_ptr<ItemExtras> m_extras;
    // charged_projectiles / container 单独存放：其元素为 ItemStack 指针，
    // 拷贝语义需要完整类型（ItemExtras 定义处 ItemStack 尚不完整），故出其外。
    // 浅指针共享：拷贝 ItemStack 时共享同一个不可变组件载荷（载荷内部物品堆不可变）；
    // 修改走 set*（替换为新载荷）。这样拷贝构造不需要在卡头文件里实例化载荷拷贝。
    std::shared_ptr<const item::component::ChargedProjectilesPayload> m_chargedProjectiles;
    std::shared_ptr<const item::component::ItemContainerContentsPayload> m_containerContents;

    // 允许 PotionUtils 访问私有成员
    friend class potion::PotionUtils;
};

// 紧凑性守卫：ItemStack 会被内联进装备组件（16 个槽）、玩家背包（41 个槽）等容器，
// 并被容器每 tick 逐槽拷贝。稀有字段外置到 ItemExtras，容器/投射物组件以共享指针承载
// （共享指针 16 字节，ItemStack 由 64 字节增至 96 字节）；此处只做粗粒度的回归拦截，
// 精确值由 ItemStackTest.CompactLayout 断言。
static_assert(sizeof(ItemStack) < 128, "ItemStack 应保持紧凑，成员增补前请先确认内存影响");

} // namespace mc
