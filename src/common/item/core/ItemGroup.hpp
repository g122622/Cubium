#pragma once

#include "../../core/Types.hpp"
#include "../../resource/ResourceLocation.hpp"
#include <vector>
#include <functional>

namespace mc {

class Item;
class ItemStack;

/**
 * @brief 创造模式物品组（标签页）
 *
 * 定义创造模式物品栏的分类。
 * 参考: net.minecraft.item.ItemGroup
 *
 * 用法示例:
 * @code
 * // 在物品注册时设置物品组
 * auto& sword = ItemRegistry::instance().registerItem<SwordItem>(
 *     ResourceLocation("minecraft:diamond_sword"),
 *     ItemProperties().maxDamage(1561).group(&ItemGroups::COMBAT)
 * );
 * @endcode
 */
class ItemGroup {
public:
    /**
     * @brief 物品组类型枚举
     *
     * 定义了所有原版物品组的顺序。
     */
    enum class Type : i32 {
        BuildingBlocks = 0,     ///< 建筑方块
        Decorations = 1,        ///< 装饰方块
        Redstone = 2,           ///< 红石
        Transportation = 3,     ///< 交通工具
        Misc = 4,               ///< 杂项
        Food = 5,               ///< 食物
        Tools = 6,              ///< 工具
        Combat = 7,             ///< 战斗
        Brewing = 8,            ///< 酿造
        Hotbar = 9,             ///< 快捷栏（保存的热门物品）
        Inventory = 10,         ///< 背包（生存模式物品栏）
        Search = 11             ///< 搜索
    };

    /**
     * @brief 构造物品组
     * @param type 物品组类型
     * @param id 物品组ID（如"building_blocks"）
     */
    ItemGroup(Type type, String id);

    /**
     * @brief 获取物品组类型
     */
    [[nodiscard]] Type getType() const { return m_type; }

    /**
     * @brief 获取物品组ID
     */
    [[nodiscard]] const String& getId() const { return m_id; }

    /**
     * @brief 获取物品组图标
     *
     * 返回显示在物品栏标签上的图标物品。
     * 默认返回第一个填充的物品。
     *
     * @return 图标物品堆
     */
    [[nodiscard]] ItemStack getIconItem() const;

    /**
     * @brief 填充物品列表
     *
     * 将所有属于此物品组的物品添加到列表中。
     *
     * @param items 物品列表
     */
    void fill(std::vector<ItemStack>& items) const;

    /**
     * @brief 设置填充函数
     * @param fillFunc 填充函数
     */
    void setFillFunction(std::function<void(std::vector<ItemStack>&)> fillFunc) {
        m_fillFunc = std::move(fillFunc);
    }

    /**
     * @brief 设置图标物品
     * @param item 图标物品
     */
    void setIconItem(const Item* item);

    /**
     * @brief 是否在第一列
     */
    [[nodiscard]] bool isFirstColumn() const {
        return static_cast<i32>(m_type) <= 5;
    }

    /**
     * @brief 获取标签页索引
     */
    [[nodiscard]] i32 getTabIndex() const {
        return static_cast<i32>(m_type);
    }

private:
    Type m_type;
    String m_id;
    const Item* m_iconItem = nullptr;
    std::function<void(std::vector<ItemStack>&)> m_fillFunc;
};

} // namespace mc
