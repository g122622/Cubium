#pragma once

#include "common/util/math/random/Random.hpp"
#include "entity/inventory/AbstractContainerMenu.hpp"
#include "entity/inventory/IInventory.hpp"
#include "world/block/BlockPos.hpp"
#include <array>
#include <memory>

namespace mc {

class PlayerInventory;
class EnchantingTableEntity;
class IWorld;

/**
 * @brief 附魔台容器
 *
 * 管理附魔台GUI的槽位布局和附魔选项生成。
 *
 * 槽位布局：
 * - 物品槽：1格（顶部左侧）
 * - 青金石槽：1格（顶部右侧）
 * - 玩家背包：27格主背包 + 9格快捷栏
 *
 * 附魔选项：
 * - 3个附魔槽位，每个显示附魔等级和预览
 * - 需要消耗玩家经验和青金石
 *
 * 参考: net.minecraft.inventory.container.EnchantmentContainer
 */
class EnchantmentContainer : public AbstractContainerMenu {
public:
    /// 物品槽索引
    static constexpr i32 SLOT_ITEM = 0;
    /// 青金石槽索引
    static constexpr i32 SLOT_LAPIS = 1;
    /// 附魔台槽位数量
    static constexpr i32 ENCHANTMENT_SLOTS = 2;

    /// 附魔选项数量
    static constexpr i32 ENCHANTMENT_OPTIONS = 3;

    /// 物品槽位置
    static constexpr i32 ITEM_SLOT_X = 15;
    static constexpr i32 ITEM_SLOT_Y = 47;
    /// 青金石槽位置
    static constexpr i32 LAPIS_SLOT_X = 35;
    static constexpr i32 LAPIS_SLOT_Y = 47;
    /// 玩家背包起始Y位置
    static constexpr i32 PLAYER_INV_Y = 84;
    /// 快捷栏Y位置
    static constexpr i32 HOTBAR_Y = 142;

    // ========== 构造函数 ==========

    /**
     * @brief 构造附魔台容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param position 方块位置（用于检测书架）
     * @param world 世界指针
     */
    EnchantmentContainer(ContainerId id, PlayerInventory* playerInventory, const BlockPos& position, IWorld* world);

    /**
     * @brief 析构函数
     */
    ~EnchantmentContainer() override = default;

    // ========== 槽位访问 ==========

    /**
     * @brief 获取物品槽的物品
     * @return 物品堆
     */
    [[nodiscard]] ItemStack getItemSlot() const;

    /**
     * @brief 获取青金石槽的物品
     * @return 物品堆
     */
    [[nodiscard]] ItemStack getLapisSlot() const;

    // ========== 附魔选项 ==========

    /**
     * @brief 获取附魔选项等级
     * @param index 选项索引（0-2）
     * @return 附魔等级，无效返回0
     */
    [[nodiscard]] i32 getEnchantmentLevel(i32 index) const;

    /**
     * @brief 获取附魔预览ID
     * @param index 选项索引（0-2）
     * @return 附魔ID，无效返回空字符串
     */
    [[nodiscard]] std::string getEnchantmentClue(i32 index) const;

    /**
     * @brief 获取附魔预览ID（字符串形式）
     * @param index 选项索引（0-2）
     * @return 附魔ID字符串，无效返回空字符串
     */
    [[nodiscard]] std::string getEnchantmentClueId(i32 index) const;

    /**
     * @brief 获取附魔预览等级
     * @param index 选项索引（0-2）
     * @return 附魔预览等级，无效返回0
     */
    [[nodiscard]] i32 getEnchantmentWorldClue(i32 index) const;

    /**
     * @brief 获取书架力量（0-15）
     * @return 书架力量
     */
    [[nodiscard]] i32 getEnchantPower() const { return m_enchantPower; }

    /**
     * @brief 检查指定选项是否可用
     * @param index 选项索引（0-2）
     * @return 如果可用返回true
     */
    [[nodiscard]] bool isEnchantmentOptionAvailable(i32 index) const;

    /**
     * @brief 检查玩家是否是创造模式
     * @return 如果关联的玩家是创造模式返回true
     *
     * 参考 MC 1.16.5 EnchantmentContainer.player.isCreative()
     */
    [[nodiscard]] bool isPlayerCreative() const;

    // ========== 附魔操作 ==========

    /**
     * @brief 选择附魔选项
     * @param player 玩家
     * @param optionIndex 选项索引（0-2）
     * @return 是否成功
     */
    bool enchantItem(Player& player, i32 optionIndex);

    // ========== 容器接口 ==========

    /**
     * @brief 检查玩家是否仍可访问附魔台
     */
    [[nodiscard]] bool stillValid(const Player& player) const override;

    /**
     * @brief 容器内容变化时调用
     */
    void slotsChanged(IInventory* inventory) override;

protected:
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

private:
    /**
     * @brief 初始化槽位布局
     */
    void initSlots(PlayerInventory* playerInventory);

    /**
     * @brief 重新计算附魔选项
     */
    void updateEnchantmentOptions();

    /**
     * @brief 计算书架力量
     * @return 书架力量（0-15）
     */
    i32 calculateEnchantPower() const;

    /**
     * @brief 检查方块是否为有效书架
     * @param pos 书架位置
     * @return 如果是有效书架返回true
     */
    [[nodiscard]] bool isValidBookshelf(const BlockPos& pos) const;

    /**
     * @brief 检查方块是否为空气
     * @param pos 方块位置
     * @return 如果是空气返回true
     */
    [[nodiscard]] bool isAirBlock(const BlockPos& pos) const;

    /**
     * @brief 更新附魔种子
     * @param player 玩家
     */
    void updateEnchantmentSeed(Player& player);

private:
    std::unique_ptr<IInventory> m_enchantmentInventory; ///< 附魔台背包
    BlockPos m_position;                                ///< 附魔台位置
    IWorld* m_world;                                    ///< 世界指针
    i32 m_enchantPower = 0;                             ///< 书架力量
    i64 m_enchantmentSeed = 0;                          ///< 附魔种子
    mutable math::Random m_random;                      ///< 随机数生成器

    /// 附魔选项等级（3个）
    std::array<i32, ENCHANTMENT_OPTIONS> m_enchantmentLevels = {0, 0, 0};
    /// 附魔选项ID（3个）
    std::array<std::string, ENCHANTMENT_OPTIONS> m_enchantmentClues;
    /// 附魔选项预览等级（3个）
    std::array<i32, ENCHANTMENT_OPTIONS> m_enchantmentWorldClues = {0, 0, 0};
};

} // namespace mc
