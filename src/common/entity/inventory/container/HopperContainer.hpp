#pragma once

#include "entity/inventory/Container.hpp"
#include "entity/inventory/IInventory.hpp"
#include <memory>

namespace mc {

class PlayerInventory;

namespace blockentity {

/**
 * @brief 漏斗容器
 *
 * 管理漏斗GUI的槽位布局。
 *
 * 槽位布局：
 * - 漏斗：5格，1行5列
 * - 玩家背包：27格主背包 + 9格快捷栏
 *
 * 参考: net.minecraft.inventory.container.HopperContainer
 */
class HopperContainer : public Container {
public:
    /// 漏斗槽位数量
    static constexpr i32 HOPPER_SIZE = 5;

    /// 漏斗槽位起始Y位置
    static constexpr i32 HOPPER_SLOT_Y = 51;

    /// 玩家背包起始Y位置
    static constexpr i32 PLAYER_INV_Y = 85;

    /// 快捷栏Y位置
    static constexpr i32 HOTBAR_Y = 143;

    /// 槽位宽度
    static constexpr i32 SLOT_SIZE = 18;

    // ========== 构造函数 ==========

    /**
     * @brief 构造漏斗容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param hopperInventory 漏斗背包
     */
    HopperContainer(ContainerId id,
                    PlayerInventory* playerInventory,
                    IInventory* hopperInventory);

    /**
     * @brief 析构函数
     */
    ~HopperContainer() override = default;

    // ========== 属性访问 ==========

    /**
     * @brief 获取漏斗背包
     */
    [[nodiscard]] IInventory* getHopperInventory() const { return m_hopperInventory; }

    // ========== 快速移动 ==========

protected:
    ItemStack doQuickMove(i32 slotIndex, ItemStack cursorItem) override;

private:
    /**
     * @brief 初始化槽位布局
     * @param playerInventory 玩家背包
     */
    void initSlots(PlayerInventory* playerInventory);

    IInventory* m_hopperInventory;  ///< 漏斗背包
};

} // namespace blockentity
} // namespace mc
