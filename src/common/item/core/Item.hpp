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

#include "ActionResult.hpp"
#include "UseAction.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc {

// Forward declarations
class BlockState;
class Item;
class ItemStack;
class Entity;
class ItemRegistry;
class ItemUseContext;
class IWorld;
class Player;
class LivingEntity;
class BlockRaycastResult;
struct Vec3;
class BlockPos;

// Forward declaration for Direction enum (defined in util/Direction.hpp)
// Note: We cannot forward declare an enum class across namespaces,
// so we use the full qualified name where needed, or include Direction.hpp

// Forward declaration for attribute system
namespace entity::attribute {
class Attribute;
class AttributeModifier;
} // namespace entity::attribute

// Forward declaration for food system
namespace item::food {
class Food;
}

// Forward declaration for item group
class ItemGroup;

// Forward declaration for item tag
namespace item::tag {
class ItemTag;
}

// Forward declaration for item attribute modifiers
namespace item {
class ItemAttributeModifiers;
}

// ============================================================================
// 物品稀有度
// ============================================================================

/**
 * @brief 物品稀有度枚举
 */
enum class ItemRarity : u8 {
    Common = 0,   // 普通 - 白色
    Uncommon = 1, // 少见 - 黄色
    Rare = 2,     // 稀有 - 青色
    Epic = 3      // 史诗 - 紫色
};

// ============================================================================
// 物品属性构建器
// ============================================================================

/**
 * @brief 物品属性构建器
 *
 * 用于构建物品属性的流畅接口。
 *
 * 用法示例:
 * @code
 * auto properties = ItemProperties()
 *     .maxStackSize(64)
 *     .maxDamage(250)
 *     .rarity(ItemRarity::Rare);
 * @endcode
 */
class ItemProperties {
public:
    ItemProperties() = default;

    /**
     * @brief 设置最大堆叠数量
     * @param maxStackSize 最大堆叠数（默认64，可设置1-64）
     * @note 如果物品有耐久度，堆叠数自动为1
     */
    ItemProperties& maxStackSize(i32 maxStackSize);

    /**
     * @brief 设置最大耐久度
     * @param maxDamage 最大耐久度（物品可承受的伤害值）
     * @note 设置耐久度后，堆叠数自动变为1
     */
    ItemProperties& maxDamage(i32 maxDamage);

    /**
     * @brief 设置容器物品（如桶装牛奶用完后返回桶）
     * @param containerItem 容器物品指针
     */
    ItemProperties& containerItem(const Item* containerItem);

    /**
     * @brief 设置稀有度
     */
    ItemProperties& rarity(ItemRarity rarity);

    /**
     * @brief 设置是否可燃烧
     */
    ItemProperties& burnable(bool value = true);

    /**
     * @brief 设置是否可修复
     */
    ItemProperties& repairable(bool value = true);

    /**
     * @brief 设置食物属性
     * @param food 食物属性指针
     * @note 设置后物品将被视为食物
     */
    ItemProperties& food(const item::food::Food* food);

    /**
     * @brief 设置创造模式物品组
     * @param group 物品组指针
     */
    ItemProperties& group(const ItemGroup* group);

    // Getters
    [[nodiscard]] i32 maxStackSize() const { return m_maxStackSize; }
    [[nodiscard]] i32 maxDamage() const { return m_maxDamage; }
    [[nodiscard]] const Item* containerItem() const { return m_containerItem; }
    [[nodiscard]] ItemRarity rarity() const { return m_rarity; }
    [[nodiscard]] bool isBurnable() const { return m_burnable; }
    [[nodiscard]] bool isRepairable() const { return m_repairable; }
    [[nodiscard]] const item::food::Food* food() const { return m_food; }
    [[nodiscard]] const ItemGroup* group() const { return m_creativeTab; }

private:
    friend class Item;

    i32 m_maxStackSize = mc::item::DEFAULT_MAX_STACK_SIZE;
    i32 m_maxDamage = 0;
    const Item* m_containerItem = nullptr;
    ItemRarity m_rarity = ItemRarity::Common;
    bool m_burnable = false;
    bool m_repairable = true;
    const item::food::Food* m_food = nullptr;
    const ItemGroup* m_creativeTab = nullptr;
};

// ============================================================================
// 物品基类
// ============================================================================

/**
 * @brief 物品基类
 *
 * 所有物品类型的基类。物品通过 ItemRegistry 注册，
 * 每个物品有一个唯一的物品ID。
 *
 * 用法示例:
 * @code
 * // 注册普通物品
 * auto& stick = ItemRegistry::instance().registerItem(
 *     ResourceLocation("minecraft:stick"),
 *     ItemProperties().maxStackSize(64)
 * );
 *
 * // 注册耐久物品
 * auto& sword = ItemRegistry::instance().registerItem<SwordItem>(
 *     ResourceLocation("minecraft:diamond_sword"),
 *     ItemProperties().maxDamage(1561)
 * );
 * @endcode
 */
class Item {
public:
    virtual ~Item() = default;

    // 禁止拷贝
    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;

    // ========================================================================
    // 静态方法
    // ========================================================================

    /**
     * @brief 根据物品ID获取物品
     */
    [[nodiscard]] static Item* getItem(ItemId itemId);

    /**
     * @brief 根据资源位置获取物品
     */
    [[nodiscard]] static Item* getItem(const ResourceLocation& id);

    /**
     * @brief 遍历所有物品
     */
    static void forEachItem(std::function<void(Item&)> callback);

    // ========================================================================
    // 基本属性
    // ========================================================================

    /**
     * @brief 获取物品资源位置
     */
    [[nodiscard]] const ResourceLocation& itemLocation() const { return m_itemLocation; }

    /**
     * @brief 获取物品ID
     */
    [[nodiscard]] ItemId itemId() const { return m_itemId; }

    /**
     * @brief 获取最大堆叠数量
     */
    [[nodiscard]] i32 maxStackSize() const { return m_maxStackSize; }

    /**
     * @brief 获取最大耐久度
     * @return 最大耐久度，0表示不可损坏
     */
    [[nodiscard]] i32 maxDamage() const { return m_maxDamage; }

    /**
     * @brief 是否可损坏
     */
    [[nodiscard]] bool isDamageable() const { return m_maxDamage > 0; }

    /**
     * @brief 是否为护甲物品
     *
     * 护甲物品在玩家受伤时会在护甲槽中受到耐久损耗。
     * ArmorItem 和 ElytraItem 重写此方法返回 true。
     * 该方法同时用于耐久保护附魔的护甲概率计算。
     *
     * @return 是否为护甲物品
     */
    [[nodiscard]] virtual bool isArmor() const { return false; }

    /**
     * @brief 获取容器物品
     */
    [[nodiscard]] const Item* containerItem() const { return m_containerItem; }

    /**
     * @brief 是否有容器物品
     */
    [[nodiscard]] bool hasContainerItem() const { return m_containerItem != nullptr; }

    /**
     * @brief 获取稀有度
     */
    [[nodiscard]] ItemRarity rarity() const { return m_rarity; }

    /**
     * @brief 获取稀有度（带ItemStack参数）
     *
     * 考虑附魔状态对稀有度的影响：
     * - 附魔物品至少为稀有（RARE）
     * - 已附魔的稀有物品变为史诗（EPIC）
     *
     * @param stack 物品堆
     * @return 稀有度
     */
    [[nodiscard]] ItemRarity getRarity(const ItemStack& stack) const;

    /**
     * @brief 是否可燃烧
     */
    [[nodiscard]] bool isBurnable() const { return m_burnable; }

    /**
     * @brief 是否可修复
     */
    [[nodiscard]] bool isRepairable() const { return m_repairable; }

    /**
     * @brief 检查物品是否可附魔
     *
     * 物品可附魔当且仅当：
     * - 堆叠数为1
     * - 物品可损坏（有耐久度）
     *
     * @param stack 物品堆
     * @return 是否可附魔
     */
    [[nodiscard]] virtual bool isEnchantable(const ItemStack& stack) const;

    /**
     * @brief 检查物品堆是否可以用作修复材料
     *
     * 子类（如ArmorItem、ToolItem）会重写此方法来检查材料类型。
     * 例如：钻石工具只能用钻石修复。
     *
     * @param toRepair 待修复的物品堆
     * @param repair 修复材料物品堆
     * @return 是否可以修复
     */
    [[nodiscard]] virtual bool getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const;

    // ========================================================================
    // 物品堆相关
    // ========================================================================

    /**
     * @brief 创建默认的物品堆
     */
    [[nodiscard]] ItemStack getDefaultInstance() const;

    // ========================================================================
    // 虚方法 - 子类可重写
    // ========================================================================

    /**
     * @brief 获取挖掘速度
     * @param stack 物品堆
     * @param state 目标方块状态
     * @return 挖掘速度倍率（默认1.0）
     */
    [[nodiscard]] virtual f32 getDestroySpeed(const ItemStack& stack, const BlockState& state) const;

    /**
     * @brief 是否可以采集方块
     * @param state 目标方块状态
     * @return 是否可以采集
     */
    [[nodiscard]] virtual bool canHarvestBlock(const BlockState& state) const;

    /**
     * @brief 获取翻译键
     */
    [[nodiscard]] virtual std::string getTranslationKey() const;

    /**
     * @brief 获取翻译键（带物品堆）
     */
    [[nodiscard]] virtual std::string getTranslationKey(const ItemStack& stack) const;

    /**
     * @brief 获取物品名称
     *
     * 返回物品的简单名称（不含格式）。
     * 子类可重写以提供自定义名称。
     *
     * @return 物品名称
     */
    [[nodiscard]] virtual std::string getName() const;

    /**
     * @brief 获取附魔能力
     * @return 附魔能力值（0表示不可附魔）
     */
    [[nodiscard]] virtual i32 getItemEnchantability() const { return 0; }

    /**
     * @brief 物品是否为食物
     */
    [[nodiscard]] virtual bool isFood() const { return m_food != nullptr; }

    /**
     * @brief 获取使用时间（如食物食用时间）
     *
     * 如果物品是食物，返回食物的使用时间；
     * 否则返回 0。
     *
     * @param stack 物品堆
     * @return 使用时间（ticks），0表示不可使用
     */
    [[nodiscard]] virtual i32 getUseDuration(const ItemStack& stack) const;

    // ========================================================================
    // 物品使用 - 新增虚方法
    // ========================================================================

    /**
     * @brief 在方块上使用物品
     *
     * 当玩家右键点击方块时调用。
     *
     * @param context 物品使用上下文
     * @return 动作结果类型
     */
    virtual ActionResultType onItemUse(ItemUseContext& context);

    /**
     * @brief 右键使用物品
     *
     * 当玩家右键点击（不针对方块）时调用。
     *
     * @param world 世界引用
     * @param player 玩家引用
     * @param hand 使用的手
     * @return 动作结果（包含结果物品堆）
     */
    virtual ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand);

    /**
     * @brief 与实体交互
     *
     * 当玩家右键点击实体时调用（如剪羊毛）。
     *
     * @param stack 物品堆
     * @param player 玩家
     * @param target 目标实体
     * @param hand 使用的手
     * @return 是否成功交互
     */
    virtual bool itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand);

    /**
     * @brief 物品使用完成
     *
     * 当物品使用时间结束时调用（如食物吃完）。
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 使用的实体
     * @return 使用后的物品堆
     */
    virtual ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity);

    /**
     * @brief 玩家停止使用物品
     *
     * 当玩家在未完成使用时间就停止时调用（如弓箭蓄力释放）。
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 使用的实体
     * @param timeLeft 剩余使用时间（ticks）
     */
    virtual void onPlayerStoppedUsing(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 timeLeft);

    /**
     * @brief 获取使用动作类型
     *
     * 用于客户端播放正确的动画。
     *
     * @param stack 物品堆
     * @return 使用动作类型
     */
    [[nodiscard]] virtual UseAction getUseAction(const ItemStack& /*stack*/) const { return UseAction::None; }

    // ========================================================================
    // 物品Tick与提示
    // ========================================================================

    /**
     * @brief 物品在物品栏中每tick调用
     *
     * 用于更新地图、时钟等物品。
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 持有实体
     * @param itemSlot 物品栏槽位
     * @param isSelected 是否被选中
     */
    virtual void inventoryTick(ItemStack& stack, IWorld& world, Entity& entity, i32 itemSlot, bool isSelected) const;

    /**
     * @brief 护甲物品每tick调用
     *
     * 当物品在护甲栏时每tick调用。用于实现护甲特殊效果（如鞘翅飞行）。
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param player 穿戴护甲的玩家
     */
    virtual void onArmorTick(ItemStack& stack, IWorld& world, LivingEntity& player) const;

    /**
     * @brief 添加物品提示信息
     *
     * 当鼠标悬停在物品上时调用，用于添加提示文本。
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param tooltip 提示文本列表
     * @param advanced 是否显示高级提示
     */
    virtual void addInformation(
        const ItemStack& stack, IWorld& world, std::vector<std::string>& tooltip, bool advanced) const;

    /**
     * @brief 是否有附魔光效
     *
     * 如果物品附魔了，返回true以显示附魔光效。
     *
     * @param stack 物品堆
     * @return 是否显示附魔光效
     */
    [[nodiscard]] virtual bool hasEffect(const ItemStack& stack) const;

    // ========================================================================
    // 标签与分类
    // ========================================================================

    /**
     * @brief 检查物品是否在指定标签中
     *
     * 用于配方和功能判断（如"minecraft:logs"标签）。
     *
     * @param tag 物品标签
     * @return 是否在标签中
     */
    [[nodiscard]] virtual bool isIn(const item::tag::ItemTag& tag) const;

    /**
     * @brief 获取创造模式物品组
     *
     * 用于创造模式物品栏分类显示。
     *
     * @return 物品组指针，nullptr表示不显示在创造模式中
     */
    [[nodiscard]] const ItemGroup* getCreativeTab() const { return m_creativeTab; }

    // ========================================================================
    // 食物相关
    // ========================================================================

    /**
     * @brief 获取食物属性
     *
     * 如果物品是食物，返回食物属性指针。
     *
     * @return 食物属性指针，非食物返回nullptr
     */
    [[nodiscard]] virtual const item::food::Food* getFood() const { return nullptr; }

    /**
     * @brief 是否可以食用
     *
     * 检查玩家当前是否可以食用此物品。
     *
     * @param stack 物品堆
     * @param player 玩家
     * @return 是否可以食用
     */
    [[nodiscard]] virtual bool canEat(const ItemStack& stack, const Player& player) const;

    /**
     * @brief 物品被玩家合成时调用
     *
     * 当物品通过工作台、熔炉、切石机、锻造台等途径被玩家合成时调用。
     * 默认实现转发给 onCraftedPostProcess。
     * 子类可重写以执行合成时的特殊处理（如地图物品的后处理）。
     *
     * @param stack 合成产生的物品堆
     * @param world 世界引用
     * @param player 合成物品的玩家
     */
    virtual void onCraftedBy(ItemStack& stack, IWorld& world, Player& player);

    /**
     * @brief 物品合成后处理
     *
     * 当物品被合成后进行后处理。由 onCraftedBy 默认调用，
     * 也可由系统合成（如合成器方块）直接调用。
     * 子类可重写以执行创建后的特殊初始化。
     *
     * 例如 FilledMapItem 重写此方法以处理 map_scale_direction NBT 标签，
     * 执行地图缩放或锁定操作。
     *
     * @param stack 合成产生的物品堆
     * @param world 世界引用
     */
    virtual void onCraftedPostProcess(ItemStack& stack, IWorld& world);

    /**
     * @brief 物品被破坏时调用
     *
     * 当物品耐久度耗尽时调用。
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 持有实体
     */
    virtual void onDestroyed(ItemStack& stack, IWorld& world, Entity& entity);

    // ========================================================================
    // 工具相关 - 新增方法
    // ========================================================================

    /**
     * @brief 攻击实体时调用
     *
     * 当持有此物品的玩家攻击实体时调用。
     * 用于工具耐久度消耗（剑、斧等）。
     *
     * @param stack 物品堆
     * @param target 被攻击的实体
     * @param attacker 攻击者
     * @return 是否成功攻击
     */
    virtual bool hitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker);

    /**
     * @brief 破坏方块时调用
     *
     * 当持有此物品的玩家破坏方块时调用。
     * 用于工具耐久度消耗（镐、斧、铲、锄等）。
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param state 被破坏的方块状态
     * @param pos 方块位置
     * @param breaker 破坏者（玩家）
     * @return 是否成功破坏
     */
    virtual bool onBlockDestroyed(
        ItemStack& stack, IWorld& world, const BlockState& state, const BlockPos& pos, LivingEntity& breaker);

    /**
     * @brief 物品是否适合作为方块工具
     *
     * 检查物品是否可以用于采集指定方块。
     *
     * @param state 方块状态
     * @return 是否适合
     */
    [[nodiscard]] virtual bool isSuitableFor(const BlockState& state) const;

    /**
     * @brief 获取物品的默认属性修饰符
     *
     * 返回物品对装备槽位的属性修饰符。
     * 例如：武器返回攻击伤害和攻击速度修饰符。
     *
     * @param slot 装备槽位
     * @return 属性修饰符
     */
    [[nodiscard]] virtual item::ItemAttributeModifiers getAttributeModifiers(i32 equipmentSlot) const;

    /**
     * @brief 填充物品到创造模式物品组
     *
     * 将物品添加到创造模式物品组列表中。
     * 子类可重写以添加多个变体（如药水、附魔书）。
     *
     * @param group 物品组
     * @param items 物品列表
     */
    virtual void fillItemGroup(const ItemGroup& group, std::vector<ItemStack>& items) const;

    /**
     * @brief 检查物品是否在指定物品组中
     *
     * @param group 物品组
     * @return 是否在组中
     */
    [[nodiscard]] virtual bool isInGroup(const ItemGroup& group) const;

    /**
     * @brief 转换为字符串
     */
    [[nodiscard]] virtual std::string toString() const { return m_itemLocation.toString(); }

protected:
    friend class ItemRegistry;
    friend class ItemProperties;

    /**
     * @brief 构造物品
     * @param properties 物品属性
     */
    explicit Item(ItemProperties properties);

    // 由 ItemRegistry 设置
    ResourceLocation m_itemLocation;
    ItemId m_itemId = 0;

    // 由构造函数设置
    i32 m_maxStackSize = mc::item::DEFAULT_MAX_STACK_SIZE;
    i32 m_maxDamage = 0;
    const Item* m_containerItem = nullptr;
    ItemRarity m_rarity = ItemRarity::Common;
    bool m_burnable = false;
    bool m_repairable = true;

    // 新增成员变量
    const item::food::Food* m_food = nullptr; ///< 食物属性（非食物为nullptr）
    const ItemGroup* m_creativeTab = nullptr; ///< 创造模式物品组
};

} // namespace mc
