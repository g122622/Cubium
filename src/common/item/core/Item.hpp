#pragma once

#include "../../core/Types.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "UseAction.hpp"
#include "ActionResult.hpp"
#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace mc {

// Forward declarations
class BlockState;
class Item;
class ItemStack;
class ItemRegistry;
class ItemUseContext;
class IWorld;
class Player;
class LivingEntity;
class Entity;
class BlockRaycastResult;
struct Vec3;
struct BlockPos;

// Forward declaration for Direction enum (defined in util/Direction.hpp)
// Note: We cannot forward declare an enum class across namespaces,
// so we use the full qualified name where needed, or include Direction.hpp

// Forward declaration for attribute system
namespace entity::attribute {
    class Attribute;
    class AttributeModifier;
}

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

// ============================================================================
// 物品稀有度
// ============================================================================

/**
 * @brief 物品稀有度枚举
 *
 * 参考: net.minecraft.item.Rarity
 */
enum class ItemRarity : u8 {
    Common = 0,     // 普通 - 白色
    Uncommon = 1,   // 少见 - 黄色
    Rare = 2,       // 稀有 - 青色
    Epic = 3        // 史诗 - 紫色
};

// ============================================================================
// 物品属性构建器
// ============================================================================

/**
 * @brief 物品属性构建器
 *
 * 用于构建物品属性的流畅接口。参考MC的 Item.Properties。
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
 * * @brief 设置是否可修复
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

    i32 m_maxStackSize = 64;
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
 * 参考: net.minecraft.item.Item
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
     * @brief 是否可燃烧
     */
    [[nodiscard]] bool isBurnable() const { return m_burnable; }

    /**
     * @brief 是否可修复
     */
    [[nodiscard]] bool isRepairable() const { return m_repairable; }

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
    [[nodiscard]] virtual f32 getDestroySpeed(const ItemStack& stack,
                                               const BlockState& state) const;

    /**
     * @brief 是否可以采集方块
     * @param state 目标方块状态
     * @return 是否可以采集
     */
    [[nodiscard]] virtual bool canHarvestBlock(const BlockState& state) const;

    /**
     * @brief 获取翻译键
     */
    [[nodiscard]] virtual String getTranslationKey() const;

    /**
     * @brief 获取翻译键（带物品堆）
     */
    [[nodiscard]] virtual String getTranslationKey(const ItemStack& stack) const;

    /**
     * @brief 获取物品名称
     *
     * 返回物品的简单名称（不含格式）。
     * 子类可重写以提供自定义名称。
     *
     * @return 物品名称
     */
    [[nodiscard]] virtual String getName() const;

    /**
     * @brief 获取附魔能力
     * @return 附魔能力值（0表示不可附魔）
     */
    [[nodiscard]] virtual i32 getItemEnchantability() const { return 0; }

    /**
     * @brief 物品是否为食物
     */
    [[nodiscard]] virtual bool isFood() const { return false; }

    /**
     * @brief 获取使用时间（如食物食用时间）
     * @return 使用时间（ticks），0表示不可使用
     */
    [[nodiscard]] virtual i32 getUseDuration(const ItemStack& /*stack*/) const { return 0; }

    // ========================================================================
    // 物品使用 - 新增虚方法
    // ========================================================================

    /**
     * @brief 在方块上使用物品
     *
     * 当玩家右键点击方块时调用。
     * 参考: net.minecraft.item.Item#onItemUse
     *
     * @param context 物品使用上下文
     * @return 动作结果类型
     */
    virtual ActionResultType onItemUse(ItemUseContext& context);

    /**
     * @brief 右键使用物品
     *
     * 当玩家右键点击（不针对方块）时调用。
     * 参考: net.minecraft.item.Item#onItemRightClick
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
     * 参考: net.minecraft.item.Item#itemInteractionForEntity
     *
     * @param stack 物品堆
     * @param player 玩家
     * @param target 目标实体
     * @param hand 使用的手
     * @return 是否成功交互
     */
    virtual bool itemInteractionForEntity(ItemStack& stack, Player& player,
                                          LivingEntity& target, Hand hand);

    /**
     * @brief 物品使用完成
     *
     * 当物品使用时间结束时调用（如食物吃完）。
     * 参考: net.minecraft.item.Item#onItemUseFinish
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 使用的实体
     * @return 使用后的物品堆
     */
    virtual ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, LivingEntity& entity);

    /**
     * @brief 玩家停止使用物品
     *
     * 当玩家在未完成使用时间就停止时调用（如弓箭蓄力释放）。
     * 参考: net.minecraft.item.Item#onPlayerStoppedUsing
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 使用的实体
     * @param timeLeft 剩余使用时间（ticks）
     */
    virtual void onPlayerStoppedUsing(ItemStack& stack, IWorld& world,
                                      LivingEntity& entity, i32 timeLeft);

    /**
     * @brief 获取使用动作类型
     *
     * 用于客户端播放正确的动画。
     * 参考: net.minecraft.item.Item#getUseAction
     *
     * @param stack 物品堆
     * @return 使用动作类型
     */
    [[nodiscard]] virtual UseAction getUseAction(const ItemStack& /*stack*/) const {
        return UseAction::None;
    }

    // ========================================================================
    // 物品Tick与提示
    // ========================================================================

    /**
     * @brief 物品在物品栏中每tick调用
     *
     * 用于更新地图、时钟等物品。
     * 参考: net.minecraft.item.Item#inventoryTick
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 持有实体
     * @param itemSlot 物品栏槽位
     * @param isSelected 是否被选中
     */
    virtual void inventoryTick(ItemStack& stack, IWorld& world, Entity& entity,
                               i32 itemSlot, bool isSelected);

    /**
     * @brief 添加物品提示信息
     *
     * 当鼠标悬停在物品上时调用，用于添加提示文本。
     * 参考: net.minecraft.item.Item#addInformation
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param tooltip 提示文本列表
     * @param advanced 是否显示高级提示
     */
    virtual void addInformation(const ItemStack& stack, IWorld& world,
                                std::vector<String>& tooltip, bool advanced) const;

    /**
     * @brief 是否有附魔光效
     *
     * 如果物品附魔了，返回true以显示附魔光效。
     * 参考: net.minecraft.item.Item#hasEffect
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
     * 参考: net.minecraft.item.Item#isIn
     *
     * @param tag 物品标签
     * @return 是否在标签中
     */
    [[nodiscard]] virtual bool isIn(const item::tag::ItemTag& tag) const;

    /**
     * @brief 获取创造模式物品组
     *
     * 用于创造模式物品栏分类显示。
     * 参考: net.minecraft.item.Item#getGroup
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
     * 参考: net.minecraft.item.Item#getFood
     *
     * @return 食物属性指针，非食物返回nullptr
     */
    [[nodiscard]] virtual const item::food::Food* getFood() const { return nullptr; }

    /**
     * @brief 是否可以食用
     *
     * 检查玩家当前是否可以食用此物品。
     * 参考: net.minecraft.item.Item#isFood
     *
     * @param stack 物品堆
     * @param player 玩家
     * @return 是否可以食用
     */
    [[nodiscard]] virtual bool canEat(const ItemStack& stack, const Player& player) const;

    /**
     * @brief 物品被破坏时调用
     *
     * 当物品耐久度耗尽时调用。
     * 参考: net.minecraft.item.Item#onDestroyed
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 持有实体
     */
    virtual void onDestroyed(ItemStack& stack, IWorld& world, Entity& entity);

    /**
     * @brief 转换为字符串
     */
    [[nodiscard]] virtual String toString() const {
        return m_itemLocation.toString();
    }

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
    i32 m_maxStackSize = 64;
    i32 m_maxDamage = 0;
    const Item* m_containerItem = nullptr;
    ItemRarity m_rarity = ItemRarity::Common;
    bool m_burnable = false;
    bool m_repairable = true;

    // 新增成员变量
    const item::food::Food* m_food = nullptr;       ///< 食物属性（非食物为nullptr）
    const ItemGroup* m_creativeTab = nullptr;       ///< 创造模式物品组
};

} // namespace mc
